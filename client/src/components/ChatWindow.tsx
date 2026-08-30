import { useEffect, useRef, useState } from "react";
import { Send, Square, AlertCircle } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Textarea } from "@/components/ui/textarea";
import { MessageBubble } from "@/components/MessageBubble";
import { streamChat, checkOllama, MODEL } from "@/lib/ollama";
import type { ChatMessage, OllamaStatus } from "@/lib/types";

export function ChatWindow() {
  const [messages, setMessages] = useState<ChatMessage[]>([]);
  const [input, setInput] = useState("");
  const [isGenerating, setIsGenerating] = useState(false);
  const [ollamaStatus, setOllamaStatus] = useState<OllamaStatus>({
    checked: false,
    ok: true,
    models: [],
  });

  const abortRef = useRef<AbortController | null>(null);
  const scrollRef = useRef<HTMLDivElement | null>(null);

  // Controllo iniziale: Ollama è raggiungibile?
  useEffect(() => {
    checkOllama().then(({ ok, models }) => {
      setOllamaStatus({ checked: true, ok, models });
    });
  }, []);

  // Auto-scroll verso il basso a ogni nuovo contenuto
  useEffect(() => {
    scrollRef.current?.scrollTo({ top: scrollRef.current.scrollHeight });
  }, [messages]);

  async function handleSend() {
    const text = input.trim();
    if (!text || isGenerating) return;

    const userMessage: ChatMessage = { role: "user", content: text };
    const history: ChatMessage[] = [...messages, userMessage];

    setMessages([...history, { role: "assistant", content: "" }]);
    setInput("");
    setIsGenerating(true);

    const controller = new AbortController();
    abortRef.current = controller;

    try {
      await streamChat(
        history,
        (chunk) => {
          setMessages((prev) => {
            const next = [...prev];
            const last = next[next.length - 1];
            next[next.length - 1] = { ...last, content: last.content + chunk };
            return next;
          });
        },
        controller.signal
      );
    } catch (err) {
      const error = err as Error;
      if (error.name !== "AbortError") {
        setMessages((prev) => {
          const next = [...prev];
          next[next.length - 1] = {
            role: "assistant",
            content: `⚠️ Errore: ${error.message}`,
          };
          return next;
        });
      }
    } finally {
      setIsGenerating(false);
      abortRef.current = null;
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

  return (
    <div className="flex h-screen flex-col bg-background">
      {/* Header */}
      <header className="flex items-center justify-between border-b px-4 py-2.5">
        <div className="flex items-center gap-2">
          <span className="text-sm font-semibold">Ollama Chat</span>
          <span className="rounded-full bg-secondary px-2 py-0.5 text-xs text-secondary-foreground">
            {MODEL}
          </span>
        </div>
        {ollamaStatus.checked && !ollamaStatus.ok && (
          <div className="flex items-center gap-1.5 text-xs text-destructive">
            <AlertCircle className="h-3.5 w-3.5" />
            Ollama non raggiungibile su localhost:11434
          </div>
        )}
      </header>

      {/* Messaggi */}
      <div
        ref={scrollRef}
        className="chat-scroll flex-1 space-y-3 overflow-y-auto px-4 py-4"
      >
        {messages.length === 0 && (
          <p className="pt-10 text-center text-sm text-muted-foreground">
            Scrivi un messaggio per iniziare la conversazione con {MODEL}.
          </p>
        )}
        {messages.map((m, i) => (
          <MessageBubble
            key={i}
            role={m.role}
            content={m.content}
            isStreaming={isGenerating && i === messages.length - 1 && m.role === "assistant"}
          />
        ))}
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
  );
}
