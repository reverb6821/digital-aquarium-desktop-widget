import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";

export interface ModelSelectorProps {
  models: string[];
  value: string;
  onChange: (model: string) => void;
  disabled?: boolean;
}

export function ModelSelector({ models, value, onChange, disabled }: ModelSelectorProps) {
  // Finché la lista non arriva ancora da `ollama list`, mostriamo comunque
  // il modello corrente come unica opzione, per non lasciare il trigger vuoto.
  const items = models.length > 0 ? models : [value];

  return (
    <Select
      items={items.map((m) => ({ value: m, label: m }))}
      value={value}
      onValueChange={(v) => {
        if (v) onChange(v);
      }}
      disabled={disabled || models.length === 0}
    >
      <SelectTrigger aria-label="Modello">
        <SelectValue placeholder="Modello" />
      </SelectTrigger>
      <SelectContent>
        {items.map((m) => (
          <SelectItem key={m} value={m}>
            {m}
          </SelectItem>
        ))}
      </SelectContent>
    </Select>
  );
}