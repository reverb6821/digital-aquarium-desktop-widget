import { Link, useRouterState } from "@tanstack/react-router";
import { Settings } from "lucide-react";
import { Button } from "@/components/ui/button";

export function Header() {
  const pathname = useRouterState({ select: (s) => s.location.pathname });
  const isSettings = pathname === "/settings";

  return (
    <header className="flex shrink-0 items-center justify-between border-b px-4 py-2.5">
      <Link to="/" className="text-sm font-semibold">
        Ollama Chat
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