import { useEffect, useState } from "react";
import { Loader2, Check, AlertCircle } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";
import { PROVIDER_LIST } from "@/lib/providers";
import { saveApiKey, loadApiKey } from "@/lib/api-keys";
import type { ProviderId } from "@/lib/types";

export interface ApiKeysDialogProps {
  open: boolean;
  onOpenChange: (open: boolean) => void;
  // Il parent ricarica lo stato (modelli disponibili) dei provider dopo un salvataggio
  onChanged: () => void;
}

type CheckState = "idle" | "checking" | "ok" | "error";

export function ApiKeysDialog({ open, onOpenChange, onChanged }: ApiKeysDialogProps) {
  const cloudProviders = PROVIDER_LIST.filter((p) => p.requiresApiKey);

  const [values, setValues] = useState<Partial<Record<ProviderId, string>>>({});
  const [checkState, setCheckState] = useState<Partial<Record<ProviderId, CheckState>>>({});
  const [checkError, setCheckError] = useState<Partial<Record<ProviderId, string>>>({});

  // Ogni volta che il dialog si apre, ricarica i valori salvati
  useEffect(() => {
    if (!open) return;
    (async () => {
      const loaded: Partial<Record<ProviderId, string>> = {};
      for (const p of cloudProviders) {
        const key = await loadApiKey(p.id);
        if (key) loaded[p.id] = key;
      }
      setValues(loaded);
    })();
    // cloudProviders è stabile (deriva da un array costante), niente da aggiungere qui
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [open]);

  async function handleSave(providerId: ProviderId) {
    const provider = cloudProviders.find((p) => p.id === providerId);
    if (!provider) return;
    const key = (values[providerId] || "").trim();

    await saveApiKey(providerId, key);
    onChanged();

    if (!key) {
      setCheckState((s) => ({ ...s, [providerId]: "idle" }));
      return;
    }

    setCheckState((s) => ({ ...s, [providerId]: "checking" }));
    const status = await provider.checkStatus(key);
    setCheckState((s) => ({ ...s, [providerId]: status.ok ? "ok" : "error" }));
    setCheckError((s) => ({ ...s, [providerId]: status.error || "" }));
  }

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent>
        <DialogHeader>
          <DialogTitle>API key servizi esterni</DialogTitle>
          <DialogDescription>
            Salvate in locale, in chiaro (non cifrate) — stesso approccio di molti client
            desktop di LLM. Non condividere il file dell'app se ti preoccupa.
          </DialogDescription>
        </DialogHeader>

        <div className="space-y-4">
          {cloudProviders.map((provider) => {
            const state = checkState[provider.id] ?? "idle";
            return (
              <div key={provider.id}>
                <label className="mb-1 flex items-center justify-between text-xs font-medium text-muted-foreground">
                  {provider.name}
                  {state === "checking" && <Loader2 className="h-3 w-3 animate-spin" />}
                  {state === "ok" && <Check className="h-3 w-3 text-green-600" />}
                  {state === "error" && <AlertCircle className="h-3 w-3 text-destructive" />}
                </label>
                <div className="flex gap-2">
                  <Input
                    type="password"
                    autoComplete="off"
                    value={values[provider.id] ?? ""}
                    onChange={(e) =>
                      setValues((v) => ({ ...v, [provider.id]: e.target.value }))
                    }
                    placeholder="sk-..."
                  />
                  <Button size="sm" variant="outline" onClick={() => handleSave(provider.id)}>
                    Salva
                  </Button>
                </div>
                {state === "error" && checkError[provider.id] && (
                  <p className="mt-1 text-xs text-destructive">⚠️ {checkError[provider.id]}</p>
                )}
              </div>
            );
          })}
        </div>
      </DialogContent>
    </Dialog>
  );
}