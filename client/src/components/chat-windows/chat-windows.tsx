import { useEffect, useRef, useState } from "react";
import { Send, Square } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Textarea } from "@/components/ui/textarea";
import { MessageBubble } from "@/components/chat-windows/message-bubble";
import { useChat } from "@/components/chat-windows/chat-context";

/**
 * Contenuto della rotta "/": solo lo storico messaggi + la barra di input.
 * Sidebar conversazioni e header (selettore modello, stato provider, azioni)
 * sono stati spostati in AppShell/ChatProvider, montati a livello di root,
 * così restano visibili anche navigando su /settings.
 */
export function ChatWindow() {
  const { conversation, isGenerating, modelOptions, activeProvider, sendMessage, stopGeneration } =
    useChat();
  const [input, setInput] = useState("");
  const scrollRef = useRef<HTMLDivElement | null>(null);

  useEffect(() => {
    scrollRef.current?.scrollTo({ top: scrollRef.current.scrollHeight });
  }, [conversation?.messages]);

  if (!conversation) {
    return (
      <div className="flex flex-1 items-center justify-center text-sm text-muted-foreground">
        Caricamento…
      </div>
    );
  }

  async function handleSend() {
    const text = input.trim();
    if (!text || isGenerating) return;
    setInput("");
    await sendMessage(text);
  }

  function handleKeyDown(e: React.KeyboardEvent<HTMLTextAreaElement>) {
    if (e.key === "Enter" && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  }

  return (
    <>
      {/* Messaggi — colonna di lettura centrata: senza un tetto di larghezza il
          testo si stirerebbe su tutto lo schermo nelle finestre molto larghe. */}
      <div ref={scrollRef} className="chat-scroll flex-1 overflow-y-auto px-4 py-4">
        <div className="mx-auto flex w-full max-w-3xl flex-col gap-3">
          {conversation.messages.length === 0 && (
            <p className="pt-10 text-center text-sm text-muted-foreground">
              {modelOptions.length === 0
                ? "Configura un provider (Ollama in locale, o una API key in Impostazioni) per iniziare."
                : `Scrivi un messaggio per iniziare la conversazione con ${conversation.model} (${activeProvider.name}).`}
            </p>
          )}
          {conversation.messages.map((m, i) => (
            <MessageBubble
              key={i}
              role={m.role}
              content={m.content}
              isStreaming={
                isGenerating && i === conversation.messages.length - 1 && m.role === "assistant"
              }
            />
          ))}
        </div>
      </div>

      {/* Input */}
      <div className="border-t border-sidebar-border p-3">
        <div className="mx-auto flex w-full max-w-3xl items-end gap-2">
          <Textarea
            value={input}
            onChange={(e) => setInput(e.target.value)}
            onKeyDown={handleKeyDown}
            placeholder="Scrivi un messaggio... (Invio per inviare, Shift+Invio per andare a capo)"
            aria-label="Messaggio"
            className="max-h-40"
            rows={1}
          />
          {isGenerating ? (
            <Button
              variant="destructive"
              size="icon"
              onClick={stopGeneration}
              title="Interrompi"
              aria-label="Interrompi generazione"
            >
              <Square className="h-4 w-4" />
            </Button>
          ) : (
            <Button
              size="icon"
              onClick={handleSend}
              disabled={!input.trim() || modelOptions.length === 0}
              title="Invia"
              aria-label="Invia messaggio"
            >
              <Send className="h-4 w-4" />
            </Button>
          )}
        </div>
      </div>
      <p className="mx-auto max-w-3xl px-3 pb-2 text-center font-data text-[10px] text-muted-foreground">
        Le risposte sono generate da un'IA e possono contenere errori — verifica le
        informazioni importanti.
      </p>
    </>
  );
}