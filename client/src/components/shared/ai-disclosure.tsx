import { Bot } from "lucide-react";
import { Button } from "@/components/ui/button";

export interface AiDisclosureOverlayProps {
  onAcknowledge: () => void;
}

export function AiDisclosureOverlay({ onAcknowledge }: AiDisclosureOverlayProps) {
  return (
    <div className="fixed inset-0 z-[100] flex items-center justify-center bg-background p-4">
      <div className="w-full max-w-md rounded-lg border p-6">
        <div className="mb-4 flex items-center gap-2">
          <Bot className="h-5 w-5" />
          <h1 className="text-sm font-semibold">Prima di iniziare</h1>
        </div>

        <div className="space-y-3 text-xs text-muted-foreground">
          <p>
            Questa applicazione è un <strong>client per modelli di intelligenza
            artificiale</strong> (Ollama in locale, oppure Claude, ChatGPT, Gemini o
            DeepSeek tramite la tua API key). Ogni risposta che ricevi è generata da
            un'IA, non da una persona.
          </p>
          <p>
            Le risposte possono contenere <strong>errori, imprecisioni o
            informazioni inventate</strong> ("allucinazioni"). Non affidarti a
            questo strumento come unica fonte per decisioni mediche, legali,
            finanziarie o di sicurezza.
          </p>
          <p>
            <strong>Ollama (locale)</strong>: i tuoi messaggi restano sul tuo
            computer. <strong>Provider cloud</strong> (Claude/ChatGPT/Gemini/
            DeepSeek): i messaggi che invii vengono trasmessi ai server di quel
            provider e trattati secondo le sue condizioni d'uso e informativa
            privacy.
          </p>
          <p>
            Conversazioni, impostazioni e API key sono salvate solo in locale sul
            tuo dispositivo.
          </p>
        </div>

        <Button className="mt-5 w-full" onClick={onAcknowledge}>
          Ho capito, continua
        </Button>
      </div>
    </div>
  );
}