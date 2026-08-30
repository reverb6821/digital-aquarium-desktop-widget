import type { Provider, ProviderStreamParams, ProviderStatus } from "./types";

const OPENAI_HOST = "https://api.openai.com";

interface OpenAiModelsResponse {
  data: Array<{ id: string }>;
}

interface OpenAiStreamChunk {
  choices?: Array<{ delta?: { content?: string } }>;
}

export const openaiProvider: Provider = {
  id: "openai",
  name: "ChatGPT (OpenAI)",
  requiresApiKey: true,
  fallbackModels: ["gpt-4o", "gpt-4o-mini"],

  async checkStatus(apiKey?: string): Promise<ProviderStatus> {
    if (!apiKey) return { ok: false, models: [], error: "Nessuna API key configurata" };
    try {
      const res = await fetch(`${OPENAI_HOST}/v1/models`, {
        headers: { Authorization: `Bearer ${apiKey}` },
      });
      if (!res.ok) {
        const text = await res.text().catch(() => "");
        return { ok: false, models: [], error: `Errore ${res.status}: ${text || res.statusText}` };
      }
      const data = (await res.json()) as OpenAiModelsResponse;
      // /v1/models include anche modelli non-chat (embeddings, whisper, ecc.):
      // filtriamo per tenere solo quelli plausibilmente chat-capable.
      const models = (data.data ?? [])
        .map((m) => m.id)
        .filter((id) => /^(gpt|o[0-9]|chatgpt)/.test(id))
        .sort();
      return { ok: true, models };
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
    if (!apiKey) throw new Error("Manca l'API key di OpenAI (Impostazioni → API key)");

    const fullMessages = systemPrompt?.trim()
      ? [{ role: "system" as const, content: systemPrompt }, ...messages]
      : messages;

    const response = await fetch(`${OPENAI_HOST}/v1/chat/completions`, {
      method: "POST",
      headers: {
        "content-type": "application/json",
        Authorization: `Bearer ${apiKey}`,
      },
      body: JSON.stringify({
        model,
        messages: fullMessages,
        temperature,
        top_p: topP,
        max_tokens: maxTokens || undefined,
        stream: true,
      }),
      signal,
    });

    if (!response.ok || !response.body) {
      const text = await response.text().catch(() => "");
      throw new Error(
        `OpenAI ha risposto con errore ${response.status}: ${text || response.statusText}`
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
        if (!trimmed.startsWith("data:")) continue;

        const payload = trimmed.slice(5).trim();
        if (!payload) continue;
        if (payload === "[DONE]") return fullText;

        let json: OpenAiStreamChunk;
        try {
          json = JSON.parse(payload) as OpenAiStreamChunk;
        } catch {
          continue;
        }

        const delta = json.choices?.[0]?.delta?.content;
        if (delta) {
          fullText += delta;
          onToken(delta);
        }
      }
    }

    return fullText;
  },
};