import { streamChat as ollamaStreamChat, checkOllama, DEFAULT_MODEL } from "@/lib/ollama";
import type { Provider, ProviderStreamParams, ProviderStatus } from "./types";

export const ollamaProvider: Provider = {
  id: "ollama",
  name: "Ollama (locale)",
  requiresApiKey: false,
  fallbackModels: [DEFAULT_MODEL],

  async checkStatus(): Promise<ProviderStatus> {
    const { ok, models } = await checkOllama();
    return { ok, models };
  },

  async streamChat(params: ProviderStreamParams): Promise<string> {
    return ollamaStreamChat({
      model: params.model,
      messages: params.messages,
      systemPrompt: params.systemPrompt,
      temperature: params.temperature,
      topP: params.topP,
      numCtx: params.numCtx,
      onToken: params.onToken,
      signal: params.signal,
    });
  },
};