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
