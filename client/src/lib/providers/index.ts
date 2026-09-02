import type { ProviderId } from "@/lib/types";
import { ollamaProvider } from "./ollama";
import { anthropicProvider } from "./anthropic";
import { openaiProvider } from "./openai";
import { geminiProvider } from "./gemini";
import { deepseekProvider } from "./deepseek";
import type { Provider } from "./types";

export const PROVIDERS: Record<ProviderId, Provider> = {
  ollama: ollamaProvider,
  anthropic: anthropicProvider,
  openai: openaiProvider,
  gemini: geminiProvider,
  deepseek: deepseekProvider,
};

export const PROVIDER_LIST: Provider[] = Object.values(PROVIDERS);

export function getProvider(id: ProviderId): Provider {
  return PROVIDERS[id];
}

export type { Provider, ProviderStreamParams, ProviderStatus } from "./types";