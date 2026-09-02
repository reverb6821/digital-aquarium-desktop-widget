import { os, computer, storage } from "@neutralinojs/lib";

const SERVICE = "ollama-chat";

/**
 * Le API native di Neutralino a volte rifiutano con un oggetto tipo
 * `{ code, message }` invece che con un vero Error (da cui il classico
 * "[object Object]" nei log se non lo si gestisce esplicitamente).
 */
function describeError(err: unknown): string {
  if (err instanceof Error) return err.message;
  if (err && typeof err === "object") {
    const obj = err as Record<string, unknown>;
    const code = typeof obj.code === "string" ? obj.code : undefined;
    const message = typeof obj.message === "string" ? obj.message : undefined;
    if (code || message) return [code, message].filter(Boolean).join(": ");
    try {
      return JSON.stringify(err);
    } catch {
      /* fallthrough */
    }
  }
  return String(err);
}

type Platform = "macos" | "windows" | "linux" | "unknown";

let cachedPlatform: Platform | null = null;

async function detectPlatform(): Promise<Platform> {
  if (cachedPlatform) return cachedPlatform;
  let info: { name: string };
  try {
    info = await computer.getOSInfo();
  } catch (err) {
    throw new KeychainUnavailableError(
      `Impossibile rilevare il sistema operativo (${describeError(err)}). ` +
        `Se hai appena aggiunto "computer.*" a neutralino.config.json, riavvia ` +
        `completamente l'app con \`neu run\` — il permesso si applica solo al riavvio, ` +
        `non basta il hot-reload di Vite.`
    );
  }
  const name = info.name.toLowerCase();
  if (name.includes("darwin") || name.includes("mac")) cachedPlatform = "macos";
  else if (name.includes("windows")) cachedPlatform = "windows";
  else if (name.includes("linux")) cachedPlatform = "linux";
  else cachedPlatform = "unknown";
  return cachedPlatform;
}

export class KeychainUnavailableError extends Error {}

/** Escape sicuro per un argomento in shell POSIX (macOS/Linux): apici singoli. */
function shellQuotePosix(value: string): string {
  return `'${value.replace(/'/g, `'\\''`)}'`;
}

// ---------- macOS: Keychain reale, via `security` ----------

async function macSave(account: string, secret: string): Promise<void> {
  const cmd = `security add-generic-password -a ${shellQuotePosix(account)} -s ${shellQuotePosix(
    SERVICE
  )} -w ${shellQuotePosix(secret)} -U`;
  const result = await os.execCommand(cmd);
  if (result.exitCode !== 0) {
    throw new Error(`Errore Keychain (salvataggio): ${result.stdErr || result.stdOut}`);
  }
}

async function macLoad(account: string): Promise<string | null> {
  const cmd = `security find-generic-password -a ${shellQuotePosix(
    account
  )} -s ${shellQuotePosix(SERVICE)} -w`;
  const result = await os.execCommand(cmd);
  if (result.exitCode !== 0) return null; // non trovata
  return result.stdOut.trim() || null;
}

async function macDelete(account: string): Promise<void> {
  const cmd = `security delete-generic-password -a ${shellQuotePosix(
    account
  )} -s ${shellQuotePosix(SERVICE)}`;
  await os.execCommand(cmd); // se non esiste, ignoriamo l'errore
}

// ---------- Linux: Secret Service (GNOME Keyring/KWallet), via `secret-tool` ----------

async function linuxSave(account: string, secret: string): Promise<void> {
  const cmd = `secret-tool store --label=${shellQuotePosix(
    `${SERVICE} (${account})`
  )} service ${shellQuotePosix(SERVICE)} account ${shellQuotePosix(account)}`;
  const result = await os.execCommand(cmd, { stdIn: secret });
  if (result.exitCode !== 0) {
    if (/not found|comando non trovato|command not found/i.test(result.stdErr)) {
      throw new KeychainUnavailableError(
        "secret-tool non è installato. Installa il pacchetto 'libsecret-tools' (Debian/Ubuntu) o 'libsecret' (Fedora/Arch)."
      );
    }
    throw new Error(`Errore Secret Service (salvataggio): ${result.stdErr || result.stdOut}`);
  }
}

async function linuxLoad(account: string): Promise<string | null> {
  const cmd = `secret-tool lookup service ${shellQuotePosix(SERVICE)} account ${shellQuotePosix(
    account
  )}`;
  const result = await os.execCommand(cmd);
  if (result.exitCode !== 0) return null;
  return result.stdOut.trim() || null;
}

async function linuxDelete(account: string): Promise<void> {
  const cmd = `secret-tool clear service ${shellQuotePosix(SERVICE)} account ${shellQuotePosix(
    account
  )}`;
  await os.execCommand(cmd);
}

// ---------- Windows: DPAPI via PowerShell ----------
//
// Non usiamo le API Win32 di Credential Manager (CredWrite/CredRead) perché
// richiederebbero P/Invoke via PowerShell, fragile da scriptare in modo affidabile
// da riga di comando. DPAPI (ConvertTo/From-SecureString) offre la stessa proprietà
// di sicurezza — cifratura legata all'utente Windows loggato — con un'unica
// chiamata nativa, senza dipendenze extra da installare. Il blob cifrato risultante
// finisce nel nostro storage locale: è al sicuro perché solo lo stesso account
// Windows può decifrarlo.

const DPAPI_SAVE_SCRIPT = `
$plain = [Console]::In.ReadToEnd()
$secure = ConvertTo-SecureString -String $plain -AsPlainText -Force
$enc = ConvertFrom-SecureString -SecureString $secure
Write-Output $enc
`.trim();

const DPAPI_LOAD_SCRIPT = `
$enc = [Console]::In.ReadToEnd()
try {
  $secure = ConvertTo-SecureString -String $enc
  $bstr = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure)
  $plain = [System.Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr)
  [System.Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
  Write-Output $plain
} catch {
  exit 1
}
`.trim();

/** powershell -EncodedCommand vuole UTF-16LE in base64: evita ogni problema di
 *  quoting tra cmd.exe e PowerShell, qualunque sia il contenuto dello script. */
function toEncodedCommand(script: string): string {
  const bytes = new Uint8Array(script.length * 2);
  for (let i = 0; i < script.length; i++) {
    const code = script.charCodeAt(i);
    bytes[i * 2] = code & 0xff;
    bytes[i * 2 + 1] = (code >> 8) & 0xff;
  }
  let binary = "";
  for (const b of bytes) binary += String.fromCharCode(b);
  return btoa(binary);
}

async function runPowershell(script: string, stdIn: string) {
  const encoded = toEncodedCommand(script);
  return os.execCommand(`powershell -NoProfile -NonInteractive -EncodedCommand ${encoded}`, {
    stdIn,
  });
}

const WIN_STORAGE_PREFIX = "keychain-dpapi-";

async function windowsSave(account: string, secret: string): Promise<void> {
  const result = await runPowershell(DPAPI_SAVE_SCRIPT, secret);
  if (result.exitCode !== 0 || !result.stdOut.trim()) {
    throw new Error(`Errore DPAPI (salvataggio): ${result.stdErr || "cifratura fallita"}`);
  }
  await storage.setData(WIN_STORAGE_PREFIX + account, result.stdOut.trim());
}

async function windowsLoad(account: string): Promise<string | null> {
  let blob: string;
  try {
    blob = await storage.getData(WIN_STORAGE_PREFIX + account);
  } catch {
    return null;
  }
  const result = await runPowershell(DPAPI_LOAD_SCRIPT, blob);
  if (result.exitCode !== 0) return null;
  return result.stdOut.trim() || null;
}

async function windowsDelete(account: string): Promise<void> {
  await storage.removeData(WIN_STORAGE_PREFIX + account).catch(() => {});
}

// ---------- API pubblica, dispatch per piattaforma ----------

export async function saveSecret(account: string, secret: string): Promise<void> {
  try {
    const platform = await detectPlatform();
    if (platform === "macos") return await macSave(account, secret);
    if (platform === "linux") return await linuxSave(account, secret);
    if (platform === "windows") return await windowsSave(account, secret);
    throw new KeychainUnavailableError("Sistema operativo non supportato per il keychain.");
  } catch (err) {
    if (err instanceof Error) throw err;
    throw new Error(describeError(err));
  }
}

export async function loadSecret(account: string): Promise<string | null> {
  try {
    const platform = await detectPlatform();
    if (platform === "macos") return await macLoad(account);
    if (platform === "linux") return await linuxLoad(account);
    if (platform === "windows") return await windowsLoad(account);
    return null;
  } catch (err) {
    if (err instanceof Error) throw err;
    throw new Error(describeError(err));
  }
}

export async function deleteSecret(account: string): Promise<void> {
  try {
    const platform = await detectPlatform();
    if (platform === "macos") return await macDelete(account);
    if (platform === "linux") return await linuxDelete(account);
    if (platform === "windows") return await windowsDelete(account);
  } catch (err) {
    if (err instanceof Error) throw err;
    throw new Error(describeError(err));
  }
}