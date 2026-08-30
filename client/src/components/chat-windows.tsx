import { useCallback, useEffect, useRef, useState } from "react";
import { Send, Square, AlertCircle, Settings2, Download, KeyRound } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Textarea } from "@/components/ui/textarea";
import { MessageBubble } from "@/components/message-bubble";
import { ModelSelector } from "@/components/model-selector";
import { ProviderSelector } from "@/components/providers/provider-selector";
import { SettingsPanel } from "@/components/settings-panel";
import { ConversationSidebar } from "@/components/conversation-sidebar";
import { PullModelDialog } from "@/components/pull-model-dialog";
import { ApiKeysDialog } from "@/components/providers/apikeys-dialog";
import { DEFAULT_MODEL } from "@/lib/ollama";
import { PROVIDER_LIST, getProvider, type ProviderStatus } from "@/lib/providers";
import { loadAllApiKeys } from "@/lib/api-keys";
import {
  saveConversation,
  loadConversation,
  deleteConversation,
  listConversations,
  saveLastSettings,
  loadLastSettings,
  type ConversationSummary,
} from "@/lib/storage";
import { DEFAULT_PARAMS } from "@/lib/types";
import type { ChatMessage, Conversation, ModelParams, ProviderId } from "@/lib/types";

const CLOUD_PROVIDER_IDS: ProviderId[] = PROVIDER_LIST.filter((p) => p.requiresApiKey).map(
  (p) => p.id
);

type ProviderStatusMap = Record<ProviderId, ProviderStatus & { checked: boolean }>;

function emptyStatusMap(): ProviderStatusMap {
  const map = {} as ProviderStatusMap;
  for (const p of PROVIDER_LIST) {
    map[p.id] = { checked: false, ok: true, models: [] };
  }
  return map;
}

function createConversation(provider: ProviderId, model: string, params: ModelParams): Conversation {
  const now = Date.now();
  return {
    id: crypto.randomUUID(),
    title: "",
    provider,
    model,
    params,
    messages: [],
    createdAt: now,
    updatedAt: now,
  };
}

function titleFromMessage(text: string): string {
  const clean = text.trim().replace(/\s+/g, " ");
  return clean.length > 40 ? clean.slice(0, 40) + "…" : clean;
}

export function ChatWindow() {
  const [conversation, setConversation] = useState<Conversation | null>(null);
  const [summaries, setSummaries] = useState<ConversationSummary[]>([]);
  const [input, setInput] = useState("");
  const [isGenerating, setIsGenerating] = useState(false);
  const [showSettings, setShowSettings] = useState(false);
  const [showPullDialog, setShowPullDialog] = useState(false);
  const [showApiKeysDialog, setShowApiKeysDialog] = useState(false);
  const [apiKeys, setApiKeys] = useState<Partial<Record<ProviderId, string>>>({});
  const [providerStatus, setProviderStatus] = useState<ProviderStatusMap>(emptyStatusMap());

  const abortRef = useRef<AbortController | null>(null);
  const scrollRef = useRef<HTMLDivElement | null>(null);
  // Sempre aggiornato con l'ultimo stato, utile dentro closures async (streaming, finally, ecc.)
  const conversationRef = useRef<Conversation | null>(null);
  conversationRef.current = conversation;
  const apiKeysRef = useRef<Partial<Record<ProviderId, string>>>({});
  apiKeysRef.current = apiKeys;

  const refreshProviderStatus = useCallback(async (id: ProviderId) => {
    const provider = getProvider(id);
    const status = await provider.checkStatus(apiKeysRef.current[id]);
    setProviderStatus((prev) => ({ ...prev, [id]: { checked: true, ...status } }));
    return status;
  }, []);

  const refreshAllProviders = useCallback(async () => {
    await Promise.all(PROVIDER_LIST.map((p) => refreshProviderStatus(p.id)));
  }, [refreshProviderStatus]);

  // Bootstrap: carica le chiavi salvate, verifica ogni provider, ripristina
  // ultime impostazioni e conversazioni salvate
  useEffect(() => {
    (async () => {
      const keys = await loadAllApiKeys(CLOUD_PROVIDER_IDS);
      setApiKeys(keys);
      apiKeysRef.current = keys;

      const statuses = await Promise.all(
        PROVIDER_LIST.map(async (p) => {
          const status = await p.checkStatus(keys[p.id]);
          return [p.id, status] as const;
        })
      );
      const statusMap = emptyStatusMap();
      for (const [id, status] of statuses) {
        statusMap[id] = { checked: true, ...status };
      }
      setProviderStatus(statusMap);

      const last = await loadLastSettings();
      const defaultProvider: ProviderId = last?.provider || "ollama";
      const providerModels = statusMap[defaultProvider]?.models ?? [];
      const defaultModel =
        last?.model || providerModels[0] || getProvider(defaultProvider).fallbackModels[0] || DEFAULT_MODEL;
      const defaultParams = last?.params || DEFAULT_PARAMS;

      const list = await listConversations();
      setSummaries(list);

      if (list.length > 0) {
        const loaded = await loadConversation(list[0].id);
        setConversation(loaded ?? createConversation(defaultProvider, defaultModel, defaultParams));
      } else {
        setConversation(createConversation(defaultProvider, defaultModel, defaultParams));
      }
    })();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  useEffect(() => {
    scrollRef.current?.scrollTo({ top: scrollRef.current.scrollHeight });
  }, [conversation?.messages]);

  const persist = useCallback(async (conv: Conversation) => {
    await saveConversation(conv);
    setSummaries((prev) => {
      const rest = prev.filter((s) => s.id !== conv.id);
      return [{ id: conv.id, title: conv.title, updatedAt: conv.updatedAt }, ...rest].sort(
        (a, b) => b.updatedAt - a.updatedAt
      );
    });
  }, []);

  async function handleSend() {
    const text = input.trim();
    const conv = conversationRef.current;
    if (!text || isGenerating || !conv) return;

    const userMessage: ChatMessage = { role: "user", content: text };
    const history: ChatMessage[] = [...conv.messages, userMessage];
    const isFirstMessage = conv.messages.length === 0;

    const startedConv: Conversation = {
      ...conv,
      title: isFirstMessage ? titleFromMessage(text) : conv.title,
      messages: [...history, { role: "assistant", content: "" }],
      updatedAt: Date.now(),
    };
    setConversation(startedConv);
    setInput("");
    setIsGenerating(true);

    const controller = new AbortController();
    abortRef.current = controller;

    try {
      const provider = getProvider(startedConv.provider);
      await provider.streamChat({
        apiKey: apiKeysRef.current[startedConv.provider],
        model: startedConv.model,
        messages: history,
        systemPrompt: startedConv.params.systemPrompt,
        temperature: startedConv.params.temperature,
        topP: startedConv.params.topP,
        numCtx: startedConv.params.numCtx,
        maxTokens: startedConv.params.maxTokens,
        onToken: (chunk) => {
          setConversation((prev) => {
            if (!prev) return prev;
            const next = [...prev.messages];
            const last = next[next.length - 1];
            next[next.length - 1] = { ...last, content: last.content + chunk };
            return { ...prev, messages: next };
          });
        },
        signal: controller.signal,
      });
    } catch (err) {
      const error = err as Error;
      if (error.name !== "AbortError") {
        setConversation((prev) => {
          if (!prev) return prev;
          const next = [...prev.messages];
          next[next.length - 1] = {
            role: "assistant",
            content: `⚠️ Errore: ${error.message}`,
          };
          return { ...prev, messages: next };
        });
      }
    } finally {
      setIsGenerating(false);
      abortRef.current = null;

      const finalConv = conversationRef.current;
      if (finalConv) {
        await persist({ ...finalConv, updatedAt: Date.now() });
        await saveLastSettings(finalConv.provider, finalConv.model, finalConv.params);
      }
    }
  }

  function handleStop() {
    abortRef.current?.abort();
  }

  function handleKeyDown(e: React.KeyboardEvent<HTMLTextAreaElement>) {
    if (e.key === "Enter" && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  }

  function handleNewConversation() {
    const conv = conversationRef.current;
    const provider = conv?.provider || "ollama";
    const model = conv?.model || providerStatus[provider]?.models[0] || getProvider(provider).fallbackModels[0];
    const params = conv?.params || DEFAULT_PARAMS;
    setConversation(createConversation(provider, model, params));
  }

  async function handleSelectConversation(id: string) {
    if (id === conversation?.id) return;
    const loaded = await loadConversation(id);
    if (loaded) setConversation(loaded);
  }

  async function handleDeleteConversation(id: string) {
    await deleteConversation(id);
    setSummaries((prev) => prev.filter((s) => s.id !== id));
    if (conversation?.id === id) {
      handleNewConversation();
    }
  }

  function handleProviderChange(providerId: ProviderId) {
    setConversation((prev) => {
      if (!prev) return prev;
      const models = providerStatus[providerId]?.models ?? [];
      const nextModel = models[0] || getProvider(providerId).fallbackModels[0];
      return { ...prev, provider: providerId, model: nextModel };
    });
  }

  function handleModelChange(model: string) {
    setConversation((prev) => (prev ? { ...prev, model } : prev));
  }

  function handleParamsChange(params: ModelParams) {
    setConversation((prev) => (prev ? { ...prev, params } : prev));
  }

  if (!conversation) {
    return (
      <div className="flex h-screen items-center justify-center text-sm text-muted-foreground">
        Caricamento…
      </div>
    );
  }

  const activeProvider = getProvider(conversation.provider);
  const activeStatus = providerStatus[conversation.provider];
  const activeModels = activeStatus?.models.length ? activeStatus.models : activeProvider.fallbackModels;
  const showOllamaTools = conversation.provider === "ollama";

  return (
    <div className="flex h-screen bg-background">
      <ConversationSidebar
        conversations={summaries}
        activeId={conversation.id}
        onSelect={handleSelectConversation}
        onNew={handleNewConversation}
        onDelete={handleDeleteConversation}
      />

      <div className="flex min-w-0 flex-1 flex-col">
        {/* Header */}
        <header className="flex items-center justify-between gap-2 border-b px-4 py-2.5">
          <div className="flex items-center gap-2">
            <span className="text-sm font-semibold">Ollama Chat</span>
            <ProviderSelector
              value={conversation.provider}
              onChange={handleProviderChange}
              disabled={isGenerating}
            />
            <ModelSelector
              models={activeModels}
              value={conversation.model}
              onChange={handleModelChange}
              disabled={isGenerating}
            />
          </div>
          <div className="flex items-center gap-2">
            {activeStatus?.checked && !activeStatus.ok && (
              <div className="flex items-center gap-1.5 text-xs text-destructive">
                <AlertCircle className="h-3.5 w-3.5" />
                {activeProvider.requiresApiKey
                  ? activeStatus.error || "API key mancante o non valida"
                  : "Ollama non raggiungibile su localhost:11434"}
              </div>
            )}
            {showOllamaTools && (
              <Button
                variant="outline"
                size="icon"
                onClick={() => setShowPullDialog(true)}
                title="Scarica o rimuovi modelli Ollama"
              >
                <Download className="h-4 w-4" />
              </Button>
            )}
            <Button
              variant="outline"
              size="icon"
              onClick={() => setShowApiKeysDialog(true)}
              title="API key servizi esterni"
            >
              <KeyRound className="h-4 w-4" />
            </Button>
            <Button
              variant={showSettings ? "default" : "outline"}
              size="icon"
              onClick={() => setShowSettings((v) => !v)}
              title="Parametri del modello"
            >
              <Settings2 className="h-4 w-4" />
            </Button>
          </div>
        </header>

        <div className="flex min-h-0 flex-1">
          {/* Messaggi */}
          <div
            ref={scrollRef}
            className="chat-scroll flex-1 space-y-3 overflow-y-auto px-4 py-4"
          >
            {conversation.messages.length === 0 && (
              <p className="pt-10 text-center text-sm text-muted-foreground">
                Scrivi un messaggio per iniziare la conversazione con {conversation.model} (
                {activeProvider.name}).
              </p>
            )}
            {conversation.messages.map((m, i) => (
              <MessageBubble
                key={i}
                role={m.role}
                content={m.content}
                isStreaming={
                  isGenerating &&
                  i === conversation.messages.length - 1 &&
                  m.role === "assistant"
                }
              />
            ))}
          </div>

          {/* Pannello parametri, apribile/chiudibile */}
          {showSettings && (
            <div className="chat-scroll w-72 shrink-0 overflow-y-auto border-l">
              <SettingsPanel
                provider={conversation.provider}
                params={conversation.params}
                onChange={handleParamsChange}
              />
            </div>
          )}
        </div>

        {/* Input */}
        <div className="flex items-end gap-2 border-t p-3">
          <Textarea
            value={input}
            onChange={(e) => setInput(e.target.value)}
            onKeyDown={handleKeyDown}
            placeholder="Scrivi un messaggio... (Invio per inviare, Shift+Invio per andare a capo)"
            className="max-h-40"
            rows={1}
          />
          {isGenerating ? (
            <Button variant="destructive" size="icon" onClick={handleStop} title="Interrompi">
              <Square className="h-4 w-4" />
            </Button>
          ) : (
            <Button size="icon" onClick={handleSend} disabled={!input.trim()} title="Invia">
              <Send className="h-4 w-4" />
            </Button>
          )}
        </div>
      </div>

      {showOllamaTools && (
        <PullModelDialog
          open={showPullDialog}
          onOpenChange={setShowPullDialog}
          installedModels={providerStatus.ollama?.models ?? []}
          onChanged={() => refreshProviderStatus("ollama")}
        />
      )}

      <ApiKeysDialog
        open={showApiKeysDialog}
        onOpenChange={setShowApiKeysDialog}
        onChanged={async () => {
          const keys = await loadAllApiKeys(CLOUD_PROVIDER_IDS);
          setApiKeys(keys);
          apiKeysRef.current = keys;
          await refreshAllProviders();
        }}
      />
    </div>
  );
}