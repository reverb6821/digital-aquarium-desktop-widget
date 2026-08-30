import type { ChatMessage, ProviderId } from "@/lib/types";

export interface ProviderStreamParams {
  apiKey?: string;
  model: string;
  messages: ChatMessage[];
  systemPrompt?: string;
  temperature?: number;
  topP?: number;
  numCtx?: number; // usato solo da Ollama
  maxTokens?: number; // usato solo dai provider cloud
  onToken: (chunkText: string) => void;
  signal: AbortSignal;
}

export interface ProviderStatus {
  ok: boolean;
  models: string[];
  error?: string;
}

export interface Provider {
  id: ProviderId;
  name: string;
  requiresApiKey: boolean;
  /** Mostrati finché non arriva una lista reale (o se il provider non offre un endpoint di listing) */
  fallbackModels: string[];
  /** Verifica raggiungibilità (Ollama) o validità della chiave (provider cloud) e i modelli disponibili */
  checkStatus: (apiKey?: string) => Promise<ProviderStatus>;
  streamChat: (params: ProviderStreamParams) => Promise<string>;
}