import { storage } from "@neutralinojs/lib";
import type { ModelParams } from "./types";

const KEY = "app-default-params";

export async function saveDefaultParams(params: ModelParams): Promise<void> {
  await storage.setData(KEY, JSON.stringify(params));
}

export async function loadDefaultParams(): Promise<ModelParams | null> {
  try {
    const raw = await storage.getData(KEY);
    return JSON.parse(raw) as ModelParams;
  } catch {
    return null;
  }
}