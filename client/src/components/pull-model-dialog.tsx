import { useRef, useState } from "react";
import { Download, Loader2, Trash2 } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";
import { streamPull, deleteModel } from "@/lib/ollama";
import type { OllamaPullChunk } from "@/lib/types";

// Solo suggerimenti per l'autocomplete dell'input: Ollama non espone
// un'API di ricerca del catalogo, quindi l'utente può comunque digitare
// qualsiasi nome/tag esista su https://ollama.com/library.
const SUGGESTED_MODELS = [
  "llama3.1",
  "llama3.2",
  "mistral",
  "mistral-nemo",
  "gemma2",
  "qwen2.5",
  "qwen2.5-coder",
  "phi3",
  "codellama",
  "deepseek-r1",
  "nomic-embed-text",
];

export interface PullModelDialogProps {
  open: boolean;
  onOpenChange: (open: boolean) => void;
  installedModels: string[];
  onChanged: () => void; // chiamato dopo pull/delete riusciti, per far ricaricare la lista al parent
}

interface PullProgress {
  status: string;
  pct: number | null; // null se lo step non riporta un totale (es. "verifying digest")
}

export function PullModelDialog({
  open,
  onOpenChange,
  installedModels,
  onChanged,
}: PullModelDialogProps) {
  const [modelName, setModelName] = useState("");
  const [isPulling, setIsPulling] = useState(false);
  const [progress, setProgress] = useState<PullProgress | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [deletingModel, setDeletingModel] = useState<string | null>(null);

  const abortRef = useRef<AbortController | null>(null);

  function handleProgress(chunk: OllamaPullChunk) {
    const pct =
      chunk.total && chunk.completed ? Math.round((chunk.completed / chunk.total) * 100) : null;
    setProgress({ status: chunk.status, pct });
  }

  async function handlePull() {
    const name = modelName.trim();
    if (!name || isPulling) return;

    setError(null);
    setIsPulling(true);
    setProgress({ status: "avvio del download…", pct: null });

    const controller = new AbortController();
    abortRef.current = controller;

    try {
      await streamPull(name, handleProgress, controller.signal);
      setModelName("");
      onChanged(); // ricarica la lista modelli nel parent (checkOllama)
    } catch (err) {
      const e = err as Error;
      if (e.name !== "AbortError") {
        setError(e.message);
      }
    } finally {
      setIsPulling(false);
      setProgress(null);
      abortRef.current = null;
    }
  }

  function handleAbort() {
    abortRef.current?.abort();
  }

  async function handleDelete(name: string) {
    setDeletingModel(name);
    setError(null);
    try {
      await deleteModel(name);
      onChanged();
    } catch (err) {
      setError((err as Error).message);
    } finally {
      setDeletingModel(null);
    }
  }

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent>
        <DialogHeader>
          <DialogTitle className="font-display">Gestisci modelli</DialogTitle>
          <DialogDescription>
            Scarica nuovi modelli da Ollama o rimuovi quelli che non usi più.
          </DialogDescription>
        </DialogHeader>

        {/* Pull */}
        <div className="space-y-2">
          <div className="flex gap-2">
            <Input
              value={modelName}
              onChange={(e) => setModelName(e.target.value)}
              onKeyDown={(e) => e.key === "Enter" && handlePull()}
              placeholder="es. llama3.1, qwen2.5:14b…"
              list="suggested-models"
              disabled={isPulling}
              className="font-data"
            />
            <datalist id="suggested-models">
              {SUGGESTED_MODELS.map((m) => (
                <option key={m} value={m} />
              ))}
            </datalist>
            {isPulling ? (
              <Button variant="destructive" onClick={handleAbort}>
                Annulla
              </Button>
            ) : (
              <Button onClick={handlePull} disabled={!modelName.trim()}>
                <Download className="mr-1.5 h-4 w-4" />
                Scarica
              </Button>
            )}
          </div>

          {progress && (
            <div className="space-y-1">
              <div className="flex items-center gap-1.5 font-data text-xs text-muted-foreground">
                <Loader2 className="h-3 w-3 animate-spin text-primary" />
                {progress.status}
                {progress.pct !== null && ` — ${progress.pct}%`}
              </div>
              {progress.pct !== null && (
                <div className="h-1.5 w-full overflow-hidden rounded-full bg-secondary">
                  <div
                    className="glow-primary h-full bg-primary transition-all"
                    style={{ width: `${progress.pct}%` }}
                  />
                </div>
              )}
            </div>
          )}

          {error && <p className="text-xs text-destructive">{error}</p>}
        </div>

        {/* Modelli installati */}
        <div className="mt-5">
          <h4 className="mb-1.5 font-display text-xs font-medium uppercase tracking-wide text-muted-foreground">
            Modelli installati
          </h4>
          <div className="chat-scroll max-h-48 space-y-0.5 overflow-y-auto">
            {installedModels.length === 0 && (
              <p className="py-2 text-xs text-muted-foreground">Nessun modello installato.</p>
            )}
            {installedModels.map((m) => (
              <div
                key={m}
                className="flex items-center justify-between rounded-md px-2 py-1.5 font-data text-sm hover:bg-accent/10"
              >
                <span className="truncate">{m}</span>
                <button
                  type="button"
                  className="shrink-0 rounded p-0.5 text-muted-foreground hover:text-destructive disabled:opacity-50"
                  onClick={() => handleDelete(m)}
                  disabled={deletingModel === m}
                  title="Elimina modello"
                >
                  {deletingModel === m ? (
                    <Loader2 className="h-3.5 w-3.5 animate-spin" />
                  ) : (
                    <Trash2 className="h-3.5 w-3.5" />
                  )}
                </button>
              </div>
            ))}
          </div>
        </div>
      </DialogContent>
    </Dialog>
  );
}