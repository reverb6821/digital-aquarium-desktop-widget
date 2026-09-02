import { storage } from "@neutralinojs/lib";

const KEY = "app-ai-disclosure-ack";

export async function hasAcknowledgedDisclosure(): Promise<boolean> {
  try {
    await storage.getData(KEY);
    return true;
  } catch {
    return false;
  }
}

export async function acknowledgeDisclosure(): Promise<void> {
  await storage.setData(KEY, "1");
}