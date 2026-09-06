import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { PROVIDER_LIST } from "@/lib/providers";
import type { ProviderId } from "@/lib/types";

export interface ProviderSelectorProps {
  value: ProviderId;
  onChange: (id: ProviderId) => void;
  disabled?: boolean;
}

export function ProviderSelector({ value, onChange, disabled }: ProviderSelectorProps) {
  return (
    <Select
      items={PROVIDER_LIST.map((p) => ({ value: p.id, label: p.name }))}
      value={value}
      onValueChange={(v) => {
        if (v) onChange(v as ProviderId);
      }}
      disabled={disabled}
    >
      <SelectTrigger aria-label="Provider" className="font-display text-xs">
        <SelectValue placeholder="Provider" />
      </SelectTrigger>
      <SelectContent>
        {PROVIDER_LIST.map((p) => (
          <SelectItem key={p.id} value={p.id} className="font-display text-xs">
            {p.name}
          </SelectItem>
        ))}
      </SelectContent>
    </Select>
  );
}