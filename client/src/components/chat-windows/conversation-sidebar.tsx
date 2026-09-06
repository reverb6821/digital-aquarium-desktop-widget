import { Plus, Trash2, MessageSquare } from "lucide-react";
import {
  Sidebar,
  SidebarContent,
  SidebarGroup,
  SidebarGroupContent,
  SidebarHeader,
  SidebarMenu,
  SidebarMenuAction,
  SidebarMenuButton,
  SidebarMenuItem,
} from "@/components/ui/sidebar";
import type { ConversationSummary } from "@/lib/storage";

export interface ConversationSidebarProps {
  conversations: ConversationSummary[];
  activeId: string | null;
  onSelect: (id: string) => void;
  onNew: () => void;
  onDelete: (id: string) => void;
  variant?: "sidebar" | "floating" | "inset";
}

export function ConversationSidebar({
  conversations,
  activeId,
  onSelect,
  onNew,
  onDelete,
  ...props
}: ConversationSidebarProps) {
  return (
    <Sidebar collapsible="offcanvas" {...props}>
      <SidebarHeader>
        <SidebarMenu>
          <SidebarMenuItem>
            <SidebarMenuButton
              tooltip="Nuova conversazione"
              className="min-w-8 bg-primary text-primary-foreground duration-200 ease-linear hover:bg-primary/90 hover:text-primary-foreground active:bg-primary/90 active:text-primary-foreground"
              onClick={onNew}
            >
              <Plus className="h-4 w-4 shrink-0" />
              <span className="group-data-[collapsible=icon]:hidden">Nuova conversazione</span>
            </SidebarMenuButton>
          </SidebarMenuItem>
        </SidebarMenu>
      </SidebarHeader>

      <SidebarContent>
        <SidebarGroup>
          <SidebarGroupContent>
            {conversations.length === 0 ? (
              <p className="px-2 py-4 text-center text-xs text-sidebar-foreground/60 group-data-[collapsible=icon]:hidden">
                Nessuna conversazione salvata
              </p>
            ) : (
              <SidebarMenu>
                {conversations.map((c) => {
                  const isActive = c.id === activeId;
                  const title = c.title || "Nuova conversazione";
                  return (
                    <SidebarMenuItem key={c.id}>
                      <SidebarMenuButton
                        isActive={isActive}
                        onClick={() => onSelect(c.id)}
                        tooltip={title}
                        className="border-l-2 border-l-transparent pl-2.5 data-[active=true]:border-l-primary"
                      >
                        <MessageSquare
                          className={isActive ? "text-primary" : "text-sidebar-foreground/60"}
                        />
                        <span className="truncate">{title}</span>
                      </SidebarMenuButton>
                      <SidebarMenuAction
                        showOnHover
                        className="hover:text-destructive"
                        onClick={(e) => {
                          e.stopPropagation();
                          onDelete(c.id);
                        }}
                      >
                        <Trash2 />
                        <span className="sr-only">Elimina</span>
                      </SidebarMenuAction>
                    </SidebarMenuItem>
                  );
                })}
              </SidebarMenu>
            )}
          </SidebarGroupContent>
        </SidebarGroup>
      </SidebarContent>
    </Sidebar>
  );
}