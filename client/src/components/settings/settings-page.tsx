import { useEffect, useState } from "react";
import { Loader2, Check, AlertCircle } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { PROVIDER_LIST, type ProviderStatus } from "@/lib/providers";
import { saveApiKey, loadApiKey } from "@/lib/api-keys";
import type { ProviderId } from "@/lib/types";

type CheckState = "idle" | "checking" | "ok" | "error";

export function SettingsPage() {
  // Ollama non ha bisogno di chiave: questa pagina gestisce solo i provider cloud.
  const cloudProviders = PROVIDER_LIST.filter((p) => p.requiresApiKey);

  const [values, setValues] = useState<Partial<Record<ProviderId, string>>>({});
  const [checkState, setCheckState] = useState<Partial<Record<ProviderId, CheckState>>>({});
  const [checkError, setCheckError] = useState<Partial<Record<ProviderId, string>>>({});
  const [modelCount, setModelCount] = useState<Partial<Record<ProviderId, number>>>({});

  useEffect(() => {
    (async () => {
      const loadedValues: Partial<Record<ProviderId, string>> = {};
      const loadedState: Partial<Record<ProviderId, CheckState>> = {};
      const loadedCount: Partial<Record<ProviderId, number>> = {};

      for (const p of cloudProviders) {
        const key = await loadApiKey(p.id);
        if (!key) continue;
        loadedValues[p.id] = key;
        const status = await p.checkStatus(key);
        loadedState[p.id] = status.ok ? "ok" : "error";
        loadedCount[p.id] = status.models.length;
      }

      setValues(loadedValues);
      setCheckState(loadedState);
      setModelCount(loadedCount);
    })();
    // cloudProviders deriva da un array costante (PROVIDER_LIST), stabile tra i render
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  async function handleSave(providerId: ProviderId) {
    const provider = cloudProviders.find((p) => p.id === providerId);
    if (!provider) return;
    const key = (values[providerId] || "").trim();

    await saveApiKey(providerId, key);

    if (!key) {
      setCheckState((s) => ({ ...s, [providerId]: "idle" }));
      setModelCount((s) => ({ ...s, [providerId]: undefined }));
      return;
    }

    setCheckState((s) => ({ ...s, [providerId]: "checking" }));
    const status: ProviderStatus = await provider.checkStatus(key);
    setCheckState((s) => ({ ...s, [providerId]: status.ok ? "ok" : "error" }));
    setCheckError((s) => ({ ...s, [providerId]: status.error || "" }));
    setModelCount((s) => ({ ...s, [providerId]: status.models.length }));
  }

  return (
    <div className="mx-auto h-full max-w-2xl overflow-y-auto p-6">
      <h1 className="mb-1 text-sm font-semibold">Provider e API key</h1>
      <p className="mb-6 text-xs text-muted-foreground">
        Le chiavi sono salvate in locale, in chiaro (non cifrate) — stesso approccio di
        molti client desktop di LLM. Una volta salvata una chiave valida, i modelli di
        quel provider compaiono nel selettore modello della chat.
      </p>

      <div className="space-y-4">
        {cloudProviders.map((provider) => {
          const state = checkState[provider.id] ?? "idle";
          return (
            <div key={provider.id} className="rounded-lg border p-4">
              <div className="mb-2 flex items-center justify-between">
                <h2 className="text-sm font-medium">{provider.name}</h2>
                <div className="flex items-center gap-1.5 text-xs">
                  {state === "checking" && (
                    <span className="flex items-center gap-1 text-muted-foreground">
                      <Loader2 className="h-3 w-3 animate-spin" /> verifica…
                    </span>
                  )}
                  {state === "ok" && (
                    <span className="flex items-center gap-1 text-green-600">
                      <Check className="h-3 w-3" />
                      {modelCount[provider.id] ?? 0} modelli disponibili
                    </span>
                  )}
                  {state === "error" && (
                    <span className="flex items-center gap-1 text-destructive">
                      <AlertCircle className="h-3 w-3" /> non valida
                    </span>
                  )}
                </div>
              </div>

              <div className="flex gap-2">
                <Input
                  type="password"
                  autoComplete="off"
                  value={values[provider.id] ?? ""}
                  onChange={(e) => setValues((v) => ({ ...v, [provider.id]: e.target.value }))}
                  placeholder="sk-..."
                />
                <Button size="sm" variant="outline" onClick={() => handleSave(provider.id)}>
                  Salva
                </Button>
              </div>

              {state === "error" && checkError[provider.id] && (
                <p className="mt-1.5 text-xs text-destructive">⚠️ {checkError[provider.id]}</p>
              )}
            </div>
          );
        })}
      </div>
    </div>
  );
}