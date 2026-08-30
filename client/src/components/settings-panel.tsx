import { Textarea } from "@/components/ui/textarea";
import { Input } from "@/components/ui/input";
import type { ModelParams } from "@/lib/types";

export interface SettingsPanelProps {
  params: ModelParams;
  onChange: (params: ModelParams) => void;
}

export function SettingsPanel({ params, onChange }: SettingsPanelProps) {
  function update<K extends keyof ModelParams>(key: K, value: ModelParams[K]) {
    onChange({ ...params, [key]: value });
  }

  return (
    <div className="space-y-5 p-4">
      <h3 className="text-xs font-semibold uppercase tracking-wide text-muted-foreground">
        Parametri
      </h3>

      <div>
        <label className="mb-1 flex items-center justify-between text-xs font-medium text-muted-foreground">
          Temperature
          <span className="tabular-nums">{params.temperature.toFixed(2)}</span>
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
          <span className="tabular-nums">{params.topP.toFixed(2)}</span>
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

      <div>
        <label className="mb-1 block text-xs font-medium text-muted-foreground">
          Context window (num_ctx)
        </label>
        <Input
          type="number"
          min={512}
          step={512}
          value={params.numCtx}
          onChange={(e) => update("numCtx", Number(e.target.value) || 0)}
        />
      </div>

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