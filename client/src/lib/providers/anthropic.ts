import type { Provider, ProviderStreamParams, ProviderStatus } from "./types";

const ANTHROPIC_HOST = "https://api.anthropic.com";
const ANTHROPIC_VERSION = "2023-06-01";

function anthropicHeaders(apiKey: string): HeadersInit {
  return {
    "content-type": "application/json",
    "x-api-key": apiKey,
    "anthropic-version": ANTHROPIC_VERSION,
    // Necessario per permettere la chiamata diretta dal webview dell'app:
    // l'API Anthropic è pensata per l'uso server-side e blocca le richieste
    // browser via CORS a meno di questo header esplicito.
    "anthropic-dangerous-direct-browser-access": "true",
  };
}

interface AnthropicModelsResponse {
  data: Array<{ id: string }>;
}

interface AnthropicStreamEvent {
  type: string;
  delta?: { type?: string; text?: string };
  error?: { message?: string };
}

export const anthropicProvider: Provider = {
  id: "anthropic",
  name: "Claude (Anthropic)",
  requiresApiKey: true,
  // Fallback statico: la lista reale arriva da /v1/models una volta salvata una chiave valida.
  fallbackModels: ["claude-opus-5", "claude-sonnet-5", "claude-haiku-4-5-20251001"],

  async checkStatus(apiKey?: string): Promise<ProviderStatus> {
    if (!apiKey) return { ok: false, models: [], error: "Nessuna API key configurata" };
    try {
      const res = await fetch(`${ANTHROPIC_HOST}/v1/models`, {
        headers: anthropicHeaders(apiKey),
      });
      if (!res.ok) {
        const text = await res.text().catch(() => "");
        return { ok: false, models: [], error: `Errore ${res.status}: ${text || res.statusText}` };
      }
      const data = (await res.json()) as AnthropicModelsResponse;
      return { ok: true, models: data.data?.map((m) => m.id) ?? [] };
    } catch (err) {
      return { ok: false, models: [], error: (err as Error).message };
    }
  },

  async streamChat({
    apiKey,
    model,
    messages,
    systemPrompt,
    temperature,
    topP,
    maxTokens,
    onToken,
    signal,
  }: ProviderStreamParams): Promise<string> {
    if (!apiKey) throw new Error("Manca l'API key di Anthropic (Impostazioni → API key)");

    const response = await fetch(`${ANTHROPIC_HOST}/v1/messages`, {
      method: "POST",
      headers: anthropicHeaders(apiKey),
      body: JSON.stringify({
        model,
        max_tokens: maxTokens || 1024,
        system: systemPrompt?.trim() || undefined,
        // Anthropic non usa role "system" dentro l'array messages: lo passiamo a parte sopra.
        messages: messages
          .filter((m) => m.role !== "system")
          .map((m) => ({ role: m.role, content: m.content })),
        temperature,
        top_p: topP,
        stream: true,
      }),
      signal,
    });

    if (!response.ok || !response.body) {
      const text = await response.text().catch(() => "");
      throw new Error(
        `Anthropic ha risposto con errore ${response.status}: ${text || response.statusText}`
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
      const lines = buffer.split("\n");
      buffer = lines.pop() ?? "";

      for (const line of lines) {
        const trimmed = line.trim();
        // Il formato SSE manda anche righe "event: ..."; ci basta il payload "data: ...".
        if (!trimmed.startsWith("data:")) continue;

        const jsonStr = trimmed.slice(5).trim();
        if (!jsonStr) continue;

        let json: AnthropicStreamEvent;
        try {
          json = JSON.parse(jsonStr) as AnthropicStreamEvent;
        } catch {
          continue;
        }

        if (json.type === "error") {
          throw new Error(json.error?.message || "Errore sconosciuto da Anthropic");
        }

        if (json.type === "content_block_delta" && json.delta?.type === "text_delta" && json.delta.text) {
          fullText += json.delta.text;
          onToken(json.delta.text);
        }
      }
    }

    return fullText;
  },
};