import { useEffect, useState } from "react";
import { createRootRoute, Outlet } from "@tanstack/react-router";
import { TanStackRouterDevtools } from "@tanstack/react-router-devtools";
import { AiDisclosureOverlay } from "@/components/shared/ai-disclosure";
import { hasAcknowledgedDisclosure, acknowledgeDisclosure } from "@/lib/ai-disclosure";
import { ChatProvider } from "@/components/chat-windows/chat-context";
import { AppShell } from "@/components/chat-windows/app-shell";

const RootLayout = () => {
  const [checked, setChecked] = useState(false);
  const [acknowledged, setAcknowledged] = useState(false);

  useEffect(() => {
    (async () => {
      setAcknowledged(await hasAcknowledgedDisclosure());
      setChecked(true);
    })();
  }, []);

  async function handleAcknowledge() {
    await acknowledgeDisclosure();
    setAcknowledged(true);
  }

  return (
    <div className="h-svh">
      <ChatProvider>
        <AppShell>
          <Outlet />
        </AppShell>
      </ChatProvider>
      <TanStackRouterDevtools />
      {checked && !acknowledged && <AiDisclosureOverlay onAcknowledge={handleAcknowledge} />}
    </div>
  );
};

export const Route = createRootRoute({ component: RootLayout });