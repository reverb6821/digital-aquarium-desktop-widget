import type {
  ChatMessage,
  OllamaChatStreamChunk,
  OllamaTagsResponse,
  OllamaPullChunk,
} from "./types";

// Host di Ollama. In sviluppo locale è quasi sempre questo.
// Ricorda di avviare Ollama con: OLLAMA_ORIGINS="*" ollama serve
export const OLLAMA_HOST = "http://localhost:11434";

// Usato solo come fallback se non c'è ancora nessun modello selezionato
// e la lista da `ollama list` non è (ancora) disponibile.
export const DEFAULT_MODEL = "llama3";

export interface StreamChatParams {
  model: string;
  messages: ChatMessage[];
  systemPrompt?: string;
  temperature?: number;
  topP?: number;
  numCtx?: number;
  onToken: (chunkText: string) => void;
  signal: AbortSignal;
}

/**
 * Invia una conversazione a Ollama e riceve la risposta in streaming.
 * @returns il testo completo generato
 */
export async function streamChat({
  model,
  messages,
  systemPrompt,
  temperature,
  topP,
  numCtx,
  onToken,
  signal,
}: StreamChatParams): Promise<string> {
  const fullMessages: ChatMessage[] = systemPrompt?.trim()
    ? [{ role: "system", content: systemPrompt }, ...messages]
    : messages;

  const response = await fetch(`${OLLAMA_HOST}/api/chat`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      model,
      messages: fullMessages,
      stream: true,
      options: {
        temperature,
        top_p: topP,
        num_ctx: numCtx,
      },
    }),
    signal,
  });

  if (!response.ok || !response.body) {
    const text = await response.text().catch(() => "");
    throw new Error(
      `Ollama ha risposto con errore ${response.status}: ${text || response.statusText}`
    );
  }

  const reader = response.body.getReader();
  const decoder = new TextDecoder();
  let buffer = "";
  let fullText = "";

  while (true) {
    const { done, value } = await reader.read();
    if (done) break;

    buffer += decoder.decode(value, { stream: true });

    // Ollama manda un oggetto JSON per riga (NDJSON)
    const lines = buffer.split("\n");
    buffer = lines.pop() ?? ""; // l'ultima riga potrebbe essere incompleta

    for (const line of lines) {
      const trimmed = line.trim();
      if (!trimmed) continue;

      let json: OllamaChatStreamChunk;
      try {
        json = JSON.parse(trimmed) as OllamaChatStreamChunk;
      } catch {
        continue; // riga corrotta/incompleta, la ignoriamo
      }

      if (json.message?.content) {
        fullText += json.message.content;
        onToken(json.message.content);
      }

      if (json.done) {
        return fullText;
      }
    }
  }

  return fullText;
}

/**
 * Scarica un modello da Ollama (equivalente a `ollama pull <model>`),
 * segnalando il progresso via NDJSON streaming.
 *
 * @param model nome del modello, es. "llama3.1" o "qwen2.5:14b"
 * @param onProgress chiamato per ogni riga di stato ricevuta
 * @param signal per poter annullare il download
 */
export async function streamPull(
  model: string,
  onProgress: (chunk: OllamaPullChunk) => void,
  signal: AbortSignal
): Promise<void> {
  const response = await fetch(`${OLLAMA_HOST}/api/pull`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ name: model, stream: true }),
    signal,
  });

  if (!response.ok || !response.body) {
    const text = await response.text().catch(() => "");
    throw new Error(
      `Ollama ha risposto con errore ${response.status}: ${text || response.statusText}`
    );
  }

  const reader = response.body.getReader();
  const decoder = new TextDecoder();
  let buffer = "";

  while (true) {
    const { done, value } = await reader.read();
    if (done) break;

    buffer += decoder.decode(value, { stream: true });
    const lines = buffer.split("\n");
    buffer = lines.pop() ?? "";

    for (const line of lines) {
      const trimmed = line.trim();
      if (!trimmed) continue;

      let json: OllamaPullChunk;
      try {
        json = JSON.parse(trimmed) as OllamaPullChunk;
      } catch {
        continue;
      }

      if (json.error) {
        throw new Error(json.error);
      }

      onProgress(json);

      if (json.status === "success") {
        return;
      }
    }
  }
}

/**
 * Elimina un modello scaricato localmente (equivalente a `ollama rm <model>`).
 */
export async function deleteModel(model: string): Promise<void> {
  const response = await fetch(`${OLLAMA_HOST}/api/delete`, {
    method: "DELETE",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ name: model }),
  });

  if (!response.ok) {
    const text = await response.text().catch(() => "");
    throw new Error(
      `Ollama ha risposto con errore ${response.status}: ${text || response.statusText}`
    );
  }
}

export interface OllamaCheckResult {
  ok: boolean;
  models: string[];
}

/**
 * Controlla se Ollama è raggiungibile e restituisce la lista modelli
 * installati (usata sia per il banner di stato sia per il selettore).
 */
export async function checkOllama(): Promise<OllamaCheckResult> {
  try {
    const res = await fetch(`${OLLAMA_HOST}/api/tags`);
    if (!res.ok) return { ok: false, models: [] };
    const data = (await res.json()) as OllamaTagsResponse;
    return { ok: true, models: data.models?.map((m) => m.name) ?? [] };
  } catch {
    return { ok: false, models: [] };
  }
}