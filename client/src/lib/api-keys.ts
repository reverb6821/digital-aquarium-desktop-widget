import { storage } from "@neutralinojs/lib";
import type { ProviderId } from "@/lib/types";

const PREFIX = "apikey:";

/**
 * ATTENZIONE: le chiavi sono salvate in chiaro nello storage locale
 * dell'app (file system dell'utente), non cifrate. È lo stesso approccio
 * usato da molti client desktop di LLM, ma vale la pena saperlo.
 */
export async function saveApiKey(provider: ProviderId, key: string): Promise<void> {
  const trimmed = key.trim();
  if (!trimmed) {
    await deleteApiKey(provider);
    return;
  }
  await storage.setData(PREFIX + provider, trimmed);
}

export async function loadApiKey(provider: ProviderId): Promise<string | null> {
  try {
    const value = await storage.getData(PREFIX + provider);
    return value || null;
  } catch {
    return null; // chiave non ancora salvata
  }
}

export async function deleteApiKey(provider: ProviderId): Promise<void> {
  try {
    await storage.removeData(PREFIX + provider);
  } catch {
    /* già assente, va bene così */
  }
}

export async function loadAllApiKeys(
  providers: ProviderId[]
): Promise<Partial<Record<ProviderId, string>>> {
  const entries = await Promise.all(providers.map(async (p) => [p, await loadApiKey(p)] as const));
  const result: Partial<Record<ProviderId, string>> = {};
  for (const [p, key] of entries) {
    if (key) result[p] = key;
  }
  return result;
}