import { cn } from "@/lib/utils";
import type { Role } from "@/lib/types";

export interface MessageBubbleProps {
  role: Role;
  content: string;
  isStreaming?: boolean;
}

export function MessageBubble({ role, content, isStreaming }: MessageBubbleProps) {
  const isUser = role === "user";

  return (
    <div className={cn("flex w-full", isUser ? "justify-end" : "justify-start")}>
      <div
        className={cn(
          "max-w-[75%] rounded-lg px-3 py-2 text-sm whitespace-pre-wrap break-words",
          isUser
            ? "bg-primary text-primary-foreground"
            : "bg-muted text-foreground"
        )}
      >
        {content}
        {isStreaming && (
          <span className="ml-0.5 inline-block h-3 w-1.5 animate-pulse bg-current align-middle" />
        )}
      </div>
    </div>
  );
}
