import { createFileRoute } from "@tanstack/react-router";
import { SettingsProvidersPage } from "@/components/settings/settings-provider-page";

export const Route = createFileRoute("/settings/")({
  component: SettingsProvidersPage,
});