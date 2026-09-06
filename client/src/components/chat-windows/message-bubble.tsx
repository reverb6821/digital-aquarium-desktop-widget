import { Bubble, BubbleContent } from "@/components/ui/bubble";
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
      <Bubble align={isUser ? "end" : "start"} variant="outline">
        <BubbleContent
          className={cn(
            "whitespace-pre-wrap break-words text-left text-sm [overflow-wrap:anywhere]",
            isUser
              ? "border-accent/30 bg-secondary text-foreground"
              : "border-primary/25 bg-card text-foreground"
          )}
        >
          {content}
          {isStreaming && (
            <span
              aria-hidden="true"
              className="ml-0.5 inline-block h-3 w-1.5 animate-pulse bg-primary align-middle motion-reduce:animate-none"
            />
          )}
        </BubbleContent>
      </Bubble>
    </div>
  );
}