import { Link, useRouterState } from "@tanstack/react-router";
import { Settings } from "lucide-react";
import { Button } from "@/components/ui/button";

export function Header() {
  const pathname = useRouterState({ select: (s) => s.location.pathname });
  const isSettings = pathname === "/settings";

  return (
    <header className="flex shrink-0 items-center justify-between border-b border-border bg-card px-4 py-2.5">
      <Link to="/" className="flex items-center gap-2">
        <span className="relative flex h-2 w-2">
          <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-primary opacity-60" />
          <span className="relative inline-flex h-2 w-2 rounded-full bg-primary" />
        </span>
        <span className="font-display text-sm font-medium tracking-tight text-foreground">
          Ollama Chat
        </span>
        <span className="font-data text-[10px] text-muted-foreground">
          nucleo online
        </span>
      </Link>
      <Link to="/settings">
        <Button
          variant={isSettings ? "default" : "outline"}
          size="icon"
          title="Impostazioni: provider e API key"
        >
          <Settings className="h-4 w-4" />
        </Button>
      </Link>
    </header>
  );
}