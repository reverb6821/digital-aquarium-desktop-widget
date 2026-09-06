import { Link, useRouterState } from "@tanstack/react-router";
import { AlertCircle, Download, Settings } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Separator } from "@/components/ui/separator";
import { SidebarTrigger } from "@/components/ui/sidebar";
import {
  UnifiedModelSelector,
  type ModelOption,
} from "@/components/shared/unified-model-selector";
import type { ProviderStatus } from "@/lib/providers";

export interface ConversationHeaderProps {
  modelOptions: ModelOption[];
  value: ModelOption;
  onModelChange: (option: ModelOption) => void;
  disabled?: boolean;
  activeStatus?: ProviderStatus & { checked: boolean };
  activeProviderRequiresApiKey: boolean;
  showOllamaTools: boolean;
  onPullModelsClick: () => void;
}

/**
 * Unico header dell'app: brand + trigger sidebar + toolbar della conversazione
 * attiva (selettore modello, stato provider, azioni). Prima erano due barre
 * sovrapposte (Header globale in __root.tsx + questo componente) — unificate qui.
 */
export function ConversationHeader({
  modelOptions,
  value,
  onModelChange,
  disabled,
  activeStatus,
  activeProviderRequiresApiKey,
  showOllamaTools,
  onPullModelsClick,
}: ConversationHeaderProps) {
  const pathname = useRouterState({ select: (s) => s.location.pathname });
  const isSettings = pathname === "/settings";

  const errorMessage =
    modelOptions.length === 0
      ? "Nessun modello disponibile — configura un provider in Impostazioni"
      : activeStatus?.checked && !activeStatus.ok
        ? activeProviderRequiresApiKey
          ? activeStatus.error || "API key mancante o non valida"
          : "Ollama non raggiungibile su localhost:11434"
        : null;

  return (
    <header className="flex h-(--header-height) shrink-0 items-center gap-2 border-b border-sidebar-border px-4 transition-[width,height] ease-linear group-has-data-[collapsible=icon]/sidebar-wrapper:h-(--header-height) lg:px-6">
      <SidebarTrigger className="-ml-1 shrink-0" />
      <Separator orientation="vertical" className="h-4 shrink-0" />

      <Link to="/" className="flex shrink-0 items-center gap-2">
        <span className="relative flex h-2 w-2">
          <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-primary opacity-60" />
          <span className="relative inline-flex h-2 w-2 rounded-full bg-primary" />
        </span>
        <span className="font-display text-sm font-medium tracking-tight text-foreground">
          Ollama Chat
        </span>
      </Link>

      <Separator orientation="vertical" className="h-4 shrink-0" />

      <div className="shrink-0">
        <UnifiedModelSelector
          options={modelOptions}
          value={value}
          onChange={onModelChange}
          disabled={disabled}
        />
      </div>

      {errorMessage && (
        <div className="flex min-w-0 items-center gap-1.5 truncate font-data text-xs text-destructive">
          <AlertCircle className="h-3.5 w-3.5 shrink-0" />
          <span className="truncate">{errorMessage}</span>
        </div>
      )}

      <div className="ml-auto flex shrink-0 items-center gap-2">
        {showOllamaTools && (
          <Button
            variant="outline"
            size="icon"
            onClick={onPullModelsClick}
            title="Scarica o rimuovi modelli Ollama"
          >
            <Download className="h-4 w-4" />
          </Button>
        )}
        <Separator orientation="vertical" className="h-4" />
        <Link to="/settings">
          <Button
            variant={isSettings ? "default" : "outline"}
            size="icon"
            title="Impostazioni: provider e API key"
          >
            <Settings className="h-4 w-4" />
          </Button>
        </Link>
      </div>
    </header>
  );
}