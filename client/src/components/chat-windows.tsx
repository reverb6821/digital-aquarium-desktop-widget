import { useCallback, useEffect, useRef, useState } from "react";
import { Send, Square, AlertCircle, Settings2, Download } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Textarea } from "@/components/ui/textarea";
import { MessageBubble } from "@/components/message-bubble";
import { ModelSelector } from "@/components/model-selector";
import { SettingsPanel } from "@/components/settings-panel";
import { ConversationSidebar } from "@/components/conversation-sidebar";
import { PullModelDialog } from "@/components/pull-model-dialog";
import { streamChat, checkOllama, DEFAULT_MODEL } from "@/lib/ollama";
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
import type { ChatMessage, Conversation, ModelParams, OllamaStatus } from "@/lib/types";

function createConversation(model: string, params: ModelParams): Conversation {
  const now = Date.now();
  return {
    id: crypto.randomUUID(),
    title: "",
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
  const [ollamaStatus, setOllamaStatus] = useState<OllamaStatus>({
    checked: false,
    ok: true,
    models: [],
  });

  const abortRef = useRef<AbortController | null>(null);
  const scrollRef = useRef<HTMLDivElement | null>(null);
  // Sempre aggiornato con l'ultimo stato, utile dentro closures async (streaming, finally, ecc.)
  const conversationRef = useRef<Conversation | null>(null);
  conversationRef.current = conversation;

  const refreshOllamaStatus = useCallback(async () => {
    const status = await checkOllama();
    setOllamaStatus({ checked: true, ...status });
    return status;
  }, []);

  // Bootstrap: verifica Ollama, ripristina ultime impostazioni e conversazioni salvate
  useEffect(() => {
    (async () => {
      const status = await refreshOllamaStatus();

      const last = await loadLastSettings();
      const defaultModel = last?.model || status.models[0] || DEFAULT_MODEL;
      const defaultParams = last?.params || DEFAULT_PARAMS;

      const list = await listConversations();
      setSummaries(list);

      if (list.length > 0) {
        const loaded = await loadConversation(list[0].id);
        setConversation(loaded ?? createConversation(defaultModel, defaultParams));
      } else {
        setConversation(createConversation(defaultModel, defaultParams));
      }
    })();
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
      await streamChat({
        model: startedConv.model,
        messages: history,
        systemPrompt: startedConv.params.systemPrompt,
        temperature: startedConv.params.temperature,
        topP: startedConv.params.topP,
        numCtx: startedConv.params.numCtx,
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

      // conversationRef ha già l'ultimo contenuto streammato (via setConversation sopra)
      const finalConv = conversationRef.current;
      if (finalConv) {
        await persist({ ...finalConv, updatedAt: Date.now() });
        await saveLastSettings(finalConv.model, finalConv.params);
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
    const model = conv?.model || ollamaStatus.models[0] || DEFAULT_MODEL;
    const params = conv?.params || DEFAULT_PARAMS;
    setConversation(createConversation(model, params));
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
            <ModelSelector
              models={ollamaStatus.models}
              value={conversation.model}
              onChange={handleModelChange}
              disabled={isGenerating}
            />
          </div>
          <div className="flex items-center gap-2">
            {ollamaStatus.checked && !ollamaStatus.ok && (
              <div className="flex items-center gap-1.5 text-xs text-destructive">
                <AlertCircle className="h-3.5 w-3.5" />
                Ollama non raggiungibile su localhost:11434
              </div>
            )}
            <Button
              variant="outline"
              size="icon"
              onClick={() => setShowPullDialog(true)}
              title="Scarica o rimuovi modelli"
            >
              <Download className="h-4 w-4" />
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
                Scrivi un messaggio per iniziare la conversazione con {conversation.model}.
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
              <SettingsPanel params={conversation.params} onChange={handleParamsChange} />
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

      <PullModelDialog
        open={showPullDialog}
        onOpenChange={setShowPullDialog}
        installedModels={ollamaStatus.models}
        onChanged={refreshOllamaStatus}
      />
    </div>
  );
}