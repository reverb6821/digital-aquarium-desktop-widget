import { Plus, Trash2, MessageSquare } from "lucide-react";
import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";
import type { ConversationSummary } from "@/lib/storage";

export interface ConversationSidebarProps {
  conversations: ConversationSummary[];
  activeId: string | null;
  onSelect: (id: string) => void;
  onNew: () => void;
  onDelete: (id: string) => void;
}

export function ConversationSidebar({
  conversations,
  activeId,
  onSelect,
  onNew,
  onDelete,
}: ConversationSidebarProps) {
  return (
    <aside className="flex w-60 shrink-0 flex-col border-r bg-secondary/30">
      <div className="p-2">
        <Button
          className="w-full justify-start gap-2"
          variant="outline"
          size="sm"
          onClick={onNew}
        >
          <Plus className="h-4 w-4" />
          Nuova conversazione
        </Button>
      </div>

      <div className="chat-scroll flex-1 space-y-0.5 overflow-y-auto px-2 pb-2">
        {conversations.length === 0 && (
          <p className="px-2 py-4 text-center text-xs text-muted-foreground">
            Nessuna conversazione salvata
          </p>
        )}
        {conversations.map((c) => (
          <div
            key={c.id}
            className={cn(
              "group flex cursor-pointer items-center gap-1.5 rounded-md px-2 py-1.5 text-sm",
              c.id === activeId ? "bg-accent text-accent-foreground" : "hover:bg-accent/60"
            )}
            onClick={() => onSelect(c.id)}
          >
            <MessageSquare className="h-3.5 w-3.5 shrink-0 text-muted-foreground" />
            <span className="flex-1 truncate">{c.title || "Nuova conversazione"}</span>
            <button
              type="button"
              className="hidden shrink-0 rounded p-0.5 text-muted-foreground hover:text-destructive group-hover:block"
              onClick={(e) => {
                e.stopPropagation();
                onDelete(c.id);
              }}
              title="Elimina"
            >
              <Trash2 className="h-3.5 w-3.5" />
            </button>
          </div>
        ))}
      </div>
    </aside>
  );
}