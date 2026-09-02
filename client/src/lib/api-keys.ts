import * as keychain from "./keychain";
import type { ProviderId } from "@/lib/types";

export async function saveApiKey(provider: ProviderId, key: string): Promise<void> {
  const trimmed = key.trim();
  if (!trimmed) {
    await deleteApiKey(provider);
    return;
  }
  await keychain.saveSecret(provider, trimmed);
}

export async function loadApiKey(provider: ProviderId): Promise<string | null> {
  try {
    return await keychain.loadSecret(provider);
  } catch {
    return null;
  }
}

export async function deleteApiKey(provider: ProviderId): Promise<void> {
  try {
    await keychain.deleteSecret(provider);
  } catch {
    /* già assente o keychain non disponibile, va bene così */
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