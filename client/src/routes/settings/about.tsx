import { createFileRoute } from "@tanstack/react-router";
import { SettingsAboutPage } from "@/components/settings/about-page";

export const Route = createFileRoute("/settings/about")({
  component: SettingsAboutPage,
});