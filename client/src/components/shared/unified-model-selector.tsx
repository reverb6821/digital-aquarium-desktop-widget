import {
  Select,
  SelectContent,
  SelectGroup,
  SelectLabel,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { PROVIDER_LIST } from "@/lib/providers";
import type { ProviderId } from "@/lib/types";

export interface ModelOption {
  provider: ProviderId;
  model: string;
}

export interface UnifiedModelSelectorProps {
  options: ModelOption[];
  value: ModelOption;
  onChange: (option: ModelOption) => void;
  disabled?: boolean;
}

// Il valore della Select è una stringa unica: codifichiamo provider+modello insieme
// (due provider diversi potrebbero teoricamente avere un modello con lo stesso nome).
function encode(o: ModelOption): string {
  return `${o.provider}::${o.model}`;
}

function decode(key: string): ModelOption {
  const [provider, ...rest] = key.split("::");
  return { provider: provider as ProviderId, model: rest.join("::") };
}

export function UnifiedModelSelector({
  options,
  value,
  onChange,
  disabled,
}: UnifiedModelSelectorProps) {
  const grouped = PROVIDER_LIST.map((p) => ({
    provider: p,
    models: options.filter((o) => o.provider === p.id),
  })).filter((g) => g.models.length > 0);

  return (
    <Select
      items={options.map((o) => ({ value: encode(o), label: o.model }))}
      value={encode(value)}
      onValueChange={(v) => {
        if (v) onChange(decode(v));
      }}
      disabled={disabled || options.length === 0}
    >
      <SelectTrigger aria-label="Modello" className="min-w-[12rem]">
        <SelectValue placeholder="Nessun modello disponibile" />
      </SelectTrigger>
      <SelectContent>
        {grouped.map((g) => (
          <SelectGroup key={g.provider.id}>
            <SelectLabel>{g.provider.name}</SelectLabel>
            {g.models.map((o) => (
              <SelectItem key={encode(o)} value={encode(o)}>
                {o.model}
              </SelectItem>
            ))}
          </SelectGroup>
        ))}
      </SelectContent>
    </Select>
  );
}