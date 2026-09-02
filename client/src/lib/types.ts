export type Role = "user" | "assistant" | "system";

export interface ChatMessage {
  role: Role;
  content: string;
}

/** Forma di ogni riga NDJSON restituita da POST /api/chat con stream:true */
export interface OllamaChatStreamChunk {
  model: string;
  created_at: string;
  message?: {
    role: Role;
    content: string;
  };
  done: boolean;
  done_reason?: string;
  total_duration?: number;
  eval_count?: number;
}

/** Forma di GET /api/tags */
export interface OllamaTagsResponse {
  models: Array<{
    name: string;
    model: string;
    modified_at: string;
    size: number;
  }>;
}

export interface OllamaStatus {
  checked: boolean;
  ok: boolean;
  models: string[];
}

/** Forma di ogni riga NDJSON restituita da POST /api/pull con stream:true */
export interface OllamaPullChunk {
  status: string; // es. "pulling manifest", "downloading", "verifying sha256 digest", "success"
  digest?: string;
  total?: number;
  completed?: number;
  error?: string;
}

export type ProviderId = "ollama" | "anthropic" | "openai" | "gemini" | "deepseek";

/** Parametri di generazione. numCtx è specifico di Ollama, maxTokens dei provider cloud. */
export interface ModelParams {
  temperature: number;
  topP: number;
  numCtx: number;
  maxTokens: number;
  systemPrompt: string;
}

export const DEFAULT_PARAMS: ModelParams = {
  temperature: 0.8,
  topP: 0.9,
  numCtx: 4096,
  maxTokens: 1024,
  systemPrompt: "",
};

/** Una conversazione completa, così come viene persistita su disco */
export interface Conversation {
  id: string;
  title: string;
  provider: ProviderId;
  model: string;
  params: ModelParams;
  messages: ChatMessage[];
  createdAt: number;
  updatedAt: number;
}