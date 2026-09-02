import { createFileRoute } from "@tanstack/react-router";
import { ChatWindow } from "@/components/chat-windows";

export const Route = createFileRoute("/")({
  component: ChatWindow,
});