import type { Provider, ProviderStreamParams, ProviderStatus } from "./types";

const GEMINI_HOST = "https://generativelanguage.googleapis.com";
const GEMINI_VERSION = "v1beta";

function geminiHeaders(apiKey: string): HeadersInit {
  return {
    "content-type": "application/json",
    "x-goog-api-key": apiKey,
  };
}

interface GeminiModelsResponse {
  models: Array<{ name: string; supportedGenerationMethods?: string[] }>;
}

interface GeminiStreamChunk {
  candidates?: Array<{ content?: { parts?: Array<{ text?: string }> } }>;
  error?: { message?: string };
}

export const geminiProvider: Provider = {
  id: "gemini",
  name: "Gemini (Google)",
  requiresApiKey: true,
  fallbackModels: ["gemini-2.5-pro", "gemini-2.5-flash"],

  async checkStatus(apiKey?: string): Promise<ProviderStatus> {
    if (!apiKey) return { ok: false, models: [], error: "Nessuna API key configurata" };
    try {
      const res = await fetch(`${GEMINI_HOST}/${GEMINI_VERSION}/models`, {
        headers: geminiHeaders(apiKey),
      });
      if (!res.ok) {
        const text = await res.text().catch(() => "");
        return { ok: false, models: [], error: `Errore ${res.status}: ${text || res.statusText}` };
      }
      const data = (await res.json()) as GeminiModelsResponse;
      const models = (data.models ?? [])
        .filter((m) => m.supportedGenerationMethods?.includes("generateContent"))
        .map((m) => m.name.replace(/^models\//, ""));
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
    if (!apiKey) throw new Error("Manca l'API key di Gemini (Impostazioni → API key)");

    const url = `${GEMINI_HOST}/${GEMINI_VERSION}/models/${model}:streamGenerateContent?alt=sse`;

    const response = await fetch(url, {
      method: "POST",
      headers: geminiHeaders(apiKey),
      body: JSON.stringify({
        // Gemini non usa role "system" dentro contents: va nel campo systemInstruction a parte.
        contents: messages
          .filter((m) => m.role !== "system")
          .map((m) => ({
            role: m.role === "assistant" ? "model" : "user",
            parts: [{ text: m.content }],
          })),
        systemInstruction: systemPrompt?.trim() ? { parts: [{ text: systemPrompt }] } : undefined,
        generationConfig: {
          temperature,
          topP,
          maxOutputTokens: maxTokens || 1024,
        },
      }),
      signal,
    });

    if (!response.ok || !response.body) {
      const text = await response.text().catch(() => "");
      throw new Error(
        `Gemini ha risposto con errore ${response.status}: ${text || response.statusText}`
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
        const jsonStr = trimmed.slice(5).trim();
        if (!jsonStr) continue;

        let json: GeminiStreamChunk;
        try {
          json = JSON.parse(jsonStr) as GeminiStreamChunk;
        } catch {
          continue;
        }

        if (json.error) {
          throw new Error(json.error.message || "Errore sconosciuto da Gemini");
        }

        const delta = json.candidates?.[0]?.content?.parts?.[0]?.text;
        if (delta) {
          fullText += delta;
          onToken(delta);
        }
      }
    }

    return fullText;
  },
};