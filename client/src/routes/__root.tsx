import { useEffect, useState } from "react";
import { createRootRoute, Outlet } from "@tanstack/react-router";
import { TanStackRouterDevtools } from "@tanstack/react-router-devtools";
import { Header } from "@/components/shared/header";
import { AiDisclosureOverlay } from "@/components/shared/ai-disclosure";
import { hasAcknowledgedDisclosure, acknowledgeDisclosure } from "@/lib/ai-disclosure"

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
    <div className="flex h-screen flex-col bg-background">
      <Header />
      <div className="min-h-0 flex-1">
        <Outlet />
      </div>
      <TanStackRouterDevtools />
      {checked && !acknowledged && <AiDisclosureOverlay onAcknowledge={handleAcknowledge} />}
    </div>
  );
};

export const Route = createRootRoute({ component: RootLayout });