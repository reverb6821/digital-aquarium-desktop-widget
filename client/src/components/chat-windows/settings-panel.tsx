import { Textarea } from "@/components/ui/textarea";
import { Input } from "@/components/ui/input";
import type { ModelParams, ProviderId } from "@/lib/types";

export interface SettingsPanelProps {
  provider: ProviderId;
  params: ModelParams;
  onChange: (params: ModelParams) => void;
}

export function SettingsPanel({ provider, params, onChange }: SettingsPanelProps) {
  function update<K extends keyof ModelParams>(key: K, value: ModelParams[K]) {
    onChange({ ...params, [key]: value });
  }

  return (
    <div className="space-y-5 p-4">
      <h3 className="font-display text-xs font-medium uppercase tracking-wide text-muted-foreground">
        Parametri
      </h3>

      <div>
        <label className="mb-1 flex items-center justify-between text-xs font-medium text-muted-foreground">
          Temperature
          <span className="font-data tabular-nums text-foreground">
            {params.temperature.toFixed(2)}
          </span>
        </label>
        <input
          type="range"
          min={0}
          max={2}
          step={0.05}
          value={params.temperature}
          onChange={(e) => update("temperature", Number(e.target.value))}
          className="w-full accent-primary"
        />
      </div>

      <div>
        <label className="mb-1 flex items-center justify-between text-xs font-medium text-muted-foreground">
          Top P
          <span className="font-data tabular-nums text-foreground">
            {params.topP.toFixed(2)}
          </span>
        </label>
        <input
          type="range"
          min={0}
          max={1}
          step={0.05}
          value={params.topP}
          onChange={(e) => update("topP", Number(e.target.value))}
          className="w-full accent-primary"
        />
      </div>

      {provider === "ollama" && (
        <div>
          <label className="mb-1 block text-xs font-medium text-muted-foreground">
            Context window (num_ctx)
          </label>
          <Input
            type="number"
            min={512}
            step={512}
            className="font-data"
            value={params.numCtx}
            onChange={(e) => update("numCtx", Number(e.target.value) || 0)}
          />
        </div>
      )}

      {provider !== "ollama" && (
        <div>
          <label className="mb-1 block text-xs font-medium text-muted-foreground">
            Max tokens di output
          </label>
          <Input
            type="number"
            min={1}
            step={128}
            className="font-data"
            value={params.maxTokens}
            onChange={(e) => update("maxTokens", Number(e.target.value) || 0)}
          />
        </div>
      )}

      <div>
        <label className="mb-1 block text-xs font-medium text-muted-foreground">
          System prompt
        </label>
        <Textarea
          value={params.systemPrompt}
          onChange={(e) => update("systemPrompt", e.target.value)}
          placeholder="Es: rispondi sempre in italiano, in modo conciso."
          rows={5}
        />
      </div>
    </div>
  );
}