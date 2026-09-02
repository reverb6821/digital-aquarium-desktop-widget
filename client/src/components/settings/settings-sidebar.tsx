import { Link, useRouterState } from "@tanstack/react-router";
import { cn } from "@/lib/utils";

function navClass(active: boolean): string {
  return cn(
    "block rounded-md px-2.5 py-1.5 text-sm",
    active ? "bg-accent text-accent-foreground" : "text-muted-foreground hover:bg-accent/60"
  );
}

export function SettingsSidebar() {
  const pathname = useRouterState({ select: (s) => s.location.pathname });

  return (
    <aside className="w-56 shrink-0 border-r p-2">
      <nav className="space-y-0.5">
        <Link to="/settings" className={navClass(pathname === "/settings")}>
          Provider e API key
        </Link>
        {/* <Link to="/settings/parameters" className={navClass(pathname === "/settings/parameters")}>
          Parametri modello
        </Link> */}
        <Link to="/settings/about" className={navClass(pathname === "/settings/about")}>
          Informazioni IA
        </Link>
      </nav>
    </aside>
  );
}