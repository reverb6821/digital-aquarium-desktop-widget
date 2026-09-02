import { storage } from "@neutralinojs/lib";
import type { Conversation, ModelParams, ProviderId } from "./types";

const CONVERSATION_PREFIX = "conversation-";
const LAST_SETTINGS_KEY = "app-last-settings";

export interface ConversationSummary {
  id: string;
  title: string;
  updatedAt: number;
}

export async function saveConversation(conversation: Conversation): Promise<void> {
  await storage.setData(CONVERSATION_PREFIX + conversation.id, JSON.stringify(conversation));
}

export async function loadConversation(id: string): Promise<Conversation | null> {
  try {
    const raw = await storage.getData(CONVERSATION_PREFIX + id);
    return JSON.parse(raw) as Conversation;
  } catch {
    return null; // chiave inesistente o dato corrotto
  }
}

export async function deleteConversation(id: string): Promise<void> {
  try {
    await storage.removeData(CONVERSATION_PREFIX + id);
  } catch {
    /* la chiave potrebbe già non esistere, va bene così */
  }
}

/**
 * Elenca tutte le conversazioni salvate, più recenti prima.
 * NB: per l'MVP legge ogni conversazione per intero per ricavarne
 * titolo/data; con molte conversazioni conviene passare a un indice
 * separato invece di rileggere tutto ad ogni avvio.
 */
export async function listConversations(): Promise<ConversationSummary[]> {
  let keys: string[] = [];
  try {
    keys = await storage.getKeys();
  } catch {
    return [];
  }

  const summaries: ConversationSummary[] = [];
  for (const key of keys) {
    if (!key.startsWith(CONVERSATION_PREFIX)) continue;
    try {
      const raw = await storage.getData(key);
      const conv = JSON.parse(raw) as Conversation;
      summaries.push({ id: conv.id, title: conv.title, updatedAt: conv.updatedAt });
    } catch {
      /* voce corrotta, la saltiamo */
    }
  }

  return summaries.sort((a, b) => b.updatedAt - a.updatedAt);
}

interface LastSettings {
  provider: ProviderId;
  model: string;
  params: ModelParams;
}

/** Ricorda l'ultimo provider/modello/parametri usati, per pre-compilare le nuove conversazioni. */
export async function saveLastSettings(
  provider: ProviderId,
  model: string,
  params: ModelParams
): Promise<void> {
  await storage.setData(
    LAST_SETTINGS_KEY,
    JSON.stringify({ provider, model, params } satisfies LastSettings)
  );
}

export async function loadLastSettings(): Promise<LastSettings | null> {
  try {
    const raw = await storage.getData(LAST_SETTINGS_KEY);
    return JSON.parse(raw) as LastSettings;
  } catch {
    return null;
  }
}