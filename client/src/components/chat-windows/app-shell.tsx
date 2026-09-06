import { useState } from "react";
import { ConversationSidebar } from "@/components/chat-windows/conversation-sidebar";
import { ConversationHeader } from "@/components/chat-windows/conversation-header";
import { PullModelDialog } from "@/components/pull-model-dialog";
import { SidebarInset, SidebarProvider } from "@/components/ui/sidebar";
import { useChat } from "@/components/chat-windows/chat-context";
import type { ModelOption } from "@/components/shared/unified-model-selector";

/**
 * Layout applicativo globale. Prima sidebar e header vivevano dentro
 * ChatWindow (montati solo su "/"): questo li rende visibili a ogni livello
 * dell'app, montandoli una volta sola in __root.tsx attorno a <Outlet />.
 */
export function AppShell({ children }: { children: React.ReactNode }) {
  const [showPullDialog, setShowPullDialog] = useState(false);
  const {
    conversation,
    summaries,
    isGenerating,
    modelOptions,
    activeStatus,
    activeProvider,
    showOllamaTools,
    providerStatus,
    newConversation,
    selectConversation,
    deleteConversation,
    changeModel,
    refreshProviderStatus,
  } = useChat();

  const headerValue: ModelOption = conversation
    ? { provider: conversation.provider, model: conversation.model }
    : { provider: "ollama", model: "" };

  return (
    <SidebarProvider
      style={
        {
          "--sidebar-width": "calc(var(--spacing) * 72)",
          "--header-height": "calc(var(--spacing) * 12)",
        } as React.CSSProperties
      }
    >
      <ConversationSidebar
        variant="inset"
        conversations={summaries}
        activeId={conversation?.id ?? null}
        onSelect={selectConversation}
        onNew={newConversation}
        onDelete={deleteConversation}
      />
      <SidebarInset>
        <ConversationHeader
          modelOptions={modelOptions}
          value={headerValue}
          onModelChange={changeModel}
          disabled={isGenerating}
          activeStatus={activeStatus}
          activeProviderRequiresApiKey={activeProvider.requiresApiKey}
          showOllamaTools={showOllamaTools}
          onPullModelsClick={() => setShowPullDialog(true)}
        />

        <div className="flex min-h-0 flex-1 flex-col">{children}</div>
      </SidebarInset>

      {showOllamaTools && (
        <PullModelDialog
          open={showPullDialog}
          onOpenChange={setShowPullDialog}
          installedModels={providerStatus.ollama?.models ?? []}
          onChanged={() => refreshProviderStatus("ollama")}
        />
      )}
    </SidebarProvider>
  );
}