import { useEffect, useState } from "react";
import { Loader2, Check, AlertCircle } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { PROVIDER_LIST } from "@/lib/providers";
import { saveApiKey, loadApiKey } from "@/lib/api-keys";
import type { ProviderId } from "@/lib/types";

type CheckState = "idle" | "checking" | "ok" | "error";

export function SettingsProvidersPage() {
  const cloudProviders = PROVIDER_LIST.filter((p) => p.requiresApiKey);

  const [selectedId, setSelectedId] = useState<ProviderId>(cloudProviders[0]?.id ?? "anthropic");
  const [keyValue, setKeyValue] = useState("");
  const [checkState, setCheckState] = useState<CheckState>("idle");
  const [checkError, setCheckError] = useState("");
  const [modelCount, setModelCount] = useState<number | null>(null);
  const [savedProviders, setSavedProviders] = useState<Set<ProviderId>>(new Set());

  const selectedProvider = cloudProviders.find((p) => p.id === selectedId) ?? cloudProviders[0];

  // Al cambio del provider selezionato: ricarica la sua chiave (se già salvata) e la verifica
  useEffect(() => {
    if (!selectedProvider) return;
    (async () => {
      setCheckState("idle");
      setModelCount(null);
      const key = await loadApiKey(selectedId);
      setKeyValue(key ?? "");
      if (key) {
        setCheckState("checking");
        const status = await selectedProvider.checkStatus(key);
        setCheckState(status.ok ? "ok" : "error");
        setCheckError(status.error || "");
        setModelCount(status.models.length);
      }
    })();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [selectedId]);

  // All'avvio: quali provider hanno già una chiave salvata (pallino verde nel dropdown)
  useEffect(() => {
    (async () => {
      const saved = new Set<ProviderId>();
      for (const p of cloudProviders) {
        const key = await loadApiKey(p.id);
        if (key) saved.add(p.id);
      }
      setSavedProviders(saved);
    })();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  async function handleSave() {
    if (!selectedProvider) return;
    const key = keyValue.trim();

    try {
      await saveApiKey(selectedId, key);
    } catch (err) {
      setCheckState("error");
      setCheckError((err as Error).message);
      return;
    }

    if (!key) {
      setCheckState("idle");
      setModelCount(null);
      setSavedProviders((prev) => {
        const next = new Set(prev);
        next.delete(selectedId);
        return next;
      });
      return;
    }

    setCheckState("checking");
    const status = await selectedProvider.checkStatus(key);
    setCheckState(status.ok ? "ok" : "error");
    setCheckError(status.error || "");
    setModelCount(status.models.length);
    setSavedProviders((prev) => {
      const next = new Set(prev);
      if (status.ok) next.add(selectedId);
      else next.delete(selectedId);
      return next;
    });
  }

  return (
    <div className="mx-auto max-w-xl p-6">
      <h1 className="mb-1 text-sm font-semibold">Provider e API key</h1>
      <p className="mb-6 text-xs text-muted-foreground">
        Le chiavi sono salvate in locale, in chiaro (non cifrate) — stesso approccio di
        molti client desktop di LLM. Una volta salvata una chiave valida, i modelli di
        quel provider compaiono nel selettore modello della chat.
      </p>

      <div className="space-y-4">
        <div>
          <label className="mb-1 block text-xs font-medium text-muted-foreground">Provider</label>
          <Select
            items={cloudProviders.map((p) => ({ value: p.id, label: p.name }))}
            value={selectedId}
            onValueChange={(v) => {
              if (v) setSelectedId(v as ProviderId);
            }}
          >
            <SelectTrigger className="w-full">
              <SelectValue />
            </SelectTrigger>
            <SelectContent>
              {cloudProviders.map((p) => (
                <SelectItem key={p.id} value={p.id}>
                  <span className="flex items-center gap-2">
                    {p.name}
                    {savedProviders.has(p.id) && (
                      <span className="h-1.5 w-1.5 rounded-full bg-green-600" />
                    )}
                  </span>
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
        </div>

        <div>
          <label className="mb-1 flex items-center justify-between text-xs font-medium text-muted-foreground">
            API key
            <span className="flex items-center gap-1.5">
              {checkState === "checking" && <Loader2 className="h-3 w-3 animate-spin" />}
              {checkState === "ok" && (
                <span className="flex items-center gap-1 text-green-600">
                  <Check className="h-3 w-3" />
                  {modelCount ?? 0} modelli
                </span>
              )}
              {checkState === "error" && (
                <span className="flex items-center gap-1 text-destructive">
                  <AlertCircle className="h-3 w-3" /> non valida
                </span>
              )}
            </span>
          </label>
          <div className="flex gap-2">
            <Input
              type="password"
              autoComplete="off"
              value={keyValue}
              onChange={(e) => setKeyValue(e.target.value)}
              placeholder="sk-..."
            />
            <Button size="sm" variant="outline" onClick={handleSave}>
              Salva
            </Button>
          </div>
          {checkState === "error" && checkError && (
            <p className="mt-1.5 text-xs text-destructive">⚠️ {checkError}</p>
          )}
        </div>
      </div>
    </div>
  );
}