import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useRef,
  useState,
} from "react";
import { useRouterState } from "@tanstack/react-router";
import { type ModelOption } from "@/components/shared/unified-model-selector";
import { DEFAULT_MODEL } from "@/lib/ollama";
import { PROVIDER_LIST, getProvider, type ProviderStatus } from "@/lib/providers";
import { loadAllApiKeys } from "@/lib/api-keys";
import { loadDefaultParams } from "@/lib/default-params";
import {
  saveConversation,
  loadConversation,
  deleteConversation as deleteConversationFromStorage,
  listConversations,
  saveLastSettings,
  loadLastSettings,
  type ConversationSummary,
} from "@/lib/storage";
import { DEFAULT_PARAMS } from "@/lib/types";
import type { ChatMessage, Conversation, ModelParams, ProviderId } from "@/lib/types";

const CLOUD_PROVIDER_IDS: ProviderId[] = PROVIDER_LIST.filter((p) => p.requiresApiKey).map(
  (p) => p.id
);

type ProviderStatusMap = Record<ProviderId, ProviderStatus & { checked: boolean }>;

function emptyStatusMap(): ProviderStatusMap {
  const map = {} as ProviderStatusMap;
  for (const p of PROVIDER_LIST) {
    map[p.id] = { checked: false, ok: true, models: [] };
  }
  return map;
}

function createNewConversation(
  provider: ProviderId,
  model: string,
  params: ModelParams
): Conversation {
  const now = Date.now();
  return {
    id: crypto.randomUUID(),
    title: "",
    provider,
    model,
    params,
    messages: [],
    createdAt: now,
    updatedAt: now,
  };
}

function titleFromMessage(text: string): string {
  const clean = text.trim().replace(/\s+/g, " ");
  return clean.length > 40 ? clean.slice(0, 40) + "…" : clean;
}

/** Modelli realmente selezionabili ora: Ollama se raggiungibile, i cloud solo se hanno una chiave valida. */
function buildModelOptions(statusMap: ProviderStatusMap): ModelOption[] {
  const options: ModelOption[] = [];
  for (const provider of PROVIDER_LIST) {
    const status = statusMap[provider.id];
    if (!status?.ok) continue;
    for (const model of status.models) {
      options.push({ provider: provider.id, model });
    }
  }
  return options;
}

interface ChatContextValue {
  conversation: Conversation | null;
  summaries: ConversationSummary[];
  isGenerating: boolean;
  apiKeys: Partial<Record<ProviderId, string>>;
  providerStatus: ProviderStatusMap;
  modelOptions: ModelOption[];
  activeProvider: ReturnType<typeof getProvider>;
  activeStatus?: ProviderStatus & { checked: boolean };
  showOllamaTools: boolean;
  sendMessage: (text: string) => Promise<void>;
  stopGeneration: () => void;
  newConversation: () => void;
  selectConversation: (id: string) => Promise<void>;
  deleteConversation: (id: string) => Promise<void>;
  changeModel: (option: ModelOption) => void;
  refreshProviderStatus: (id: ProviderId) => Promise<ProviderStatus>;
}

const ChatContext = createContext<ChatContextValue | null>(null);

/**
 * Provider unico per tutto lo stato della chat: conversazione attiva, elenco
 * conversazioni, provider/modelli disponibili, generazione in corso.
 * Va montato una sola volta in __root.tsx, sopra <Outlet />, così sidebar e
 * header restano visibili (e sincronizzati) su ogni rotta.
 */
export function ChatProvider({ children }: { children: React.ReactNode }) {
  const [conversation, setConversation] = useState<Conversation | null>(null);
  const [summaries, setSummaries] = useState<ConversationSummary[]>([]);
  const [isGenerating, setIsGenerating] = useState(false);
  const [apiKeys, setApiKeys] = useState<Partial<Record<ProviderId, string>>>({});
  const [providerStatus, setProviderStatus] = useState<ProviderStatusMap>(emptyStatusMap());

  const abortRef = useRef<AbortController | null>(null);
  const conversationRef = useRef<Conversation | null>(null);
  conversationRef.current = conversation;
  const apiKeysRef = useRef<Partial<Record<ProviderId, string>>>({});
  apiKeysRef.current = apiKeys;

  const refreshProviderStatus = useCallback(async (id: ProviderId) => {
    const provider = getProvider(id);
    const status = await provider.checkStatus(apiKeysRef.current[id]);
    setProviderStatus((prev) => ({ ...prev, [id]: { checked: true, ...status } }));
    return status;
  }, []);

  const refreshAllProviderStatuses = useCallback(async () => {
    const keys = await loadAllApiKeys(CLOUD_PROVIDER_IDS);
    setApiKeys(keys);
    apiKeysRef.current = keys;

    const statuses = await Promise.all(
      PROVIDER_LIST.map(async (p) => {
        const status = await p.checkStatus(keys[p.id]);
        return [p.id, status] as const;
      })
    );
    const statusMap = emptyStatusMap();
    for (const [id, status] of statuses) {
      statusMap[id] = { checked: true, ...status };
    }
    setProviderStatus(statusMap);
    return statusMap;
  }, []);

  // Bootstrap iniziale: carica le chiavi salvate, verifica ogni provider,
  // ripristina ultime impostazioni e conversazioni salvate.
  useEffect(() => {
    (async () => {
      const statusMap = await refreshAllProviderStatuses();

      const last = await loadLastSettings();
      const options = buildModelOptions(statusMap);
      const lastStillValid =
        last && options.some((o) => o.provider === last.provider && o.model === last.model);

      const defaultProvider: ProviderId = lastStillValid
        ? last!.provider
        : options[0]?.provider || "ollama";
      const defaultModel = lastStillValid
        ? last!.model
        : options[0]?.model || getProvider(defaultProvider).fallbackModels[0] || DEFAULT_MODEL;
      const defaultParams = last?.params || (await loadDefaultParams()) || DEFAULT_PARAMS;

      const list = await listConversations();
      setSummaries(list);

      if (list.length > 0) {
        const loaded = await loadConversation(list[0].id);
        setConversation(loaded ?? createNewConversation(defaultProvider, defaultModel, defaultParams));
      } else {
        setConversation(createNewConversation(defaultProvider, defaultModel, defaultParams));
      }
    })();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // Sidebar e header ora vivono a livello di root e non vengono più smontati
  // quando si naviga verso /settings e si torna indietro (prima il bootstrap
  // si rieseguiva al remount di ChatWindow). Per non perdere quel
  // comportamento — es. una API key appena salvata in Impostazioni — si
  // rifanno i controlli quando si rientra su "/".
  const pathname = useRouterState({ select: (s) => s.location.pathname });
  const isFirstPathRender = useRef(true);
  useEffect(() => {
    if (isFirstPathRender.current) {
      isFirstPathRender.current = false;
      return;
    }
    if (pathname === "/") {
      refreshAllProviderStatuses();
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [pathname]);

  const persist = useCallback(async (conv: Conversation) => {
    await saveConversation(conv);
    setSummaries((prev) => {
      const rest = prev.filter((s) => s.id !== conv.id);
      return [{ id: conv.id, title: conv.title, updatedAt: conv.updatedAt }, ...rest].sort(
        (a, b) => b.updatedAt - a.updatedAt
      );
    });
  }, []);

  const sendMessage = useCallback(
    async (text: string) => {
      const conv = conversationRef.current;
      if (!text || !conv) return;

      const userMessage: ChatMessage = { role: "user", content: text };
      const history: ChatMessage[] = [...conv.messages, userMessage];
      const isFirstMessage = conv.messages.length === 0;

      const startedConv: Conversation = {
        ...conv,
        title: isFirstMessage ? titleFromMessage(text) : conv.title,
        messages: [...history, { role: "assistant", content: "" }],
        updatedAt: Date.now(),
      };
      setConversation(startedConv);
      setIsGenerating(true);

      const controller = new AbortController();
      abortRef.current = controller;

      try {
        const provider = getProvider(startedConv.provider);
        await provider.streamChat({
          apiKey: apiKeysRef.current[startedConv.provider],
          model: startedConv.model,
          messages: history,
          systemPrompt: startedConv.params.systemPrompt,
          temperature: startedConv.params.temperature,
          topP: startedConv.params.topP,
          numCtx: startedConv.params.numCtx,
          maxTokens: startedConv.params.maxTokens,
          onToken: (chunk) => {
            setConversation((prev) => {
              if (!prev) return prev;
              const next = [...prev.messages];
              const last = next[next.length - 1];
              next[next.length - 1] = { ...last, content: last.content + chunk };
              return { ...prev, messages: next };
            });
          },
          signal: controller.signal,
        });
      } catch (err) {
        const error = err as Error;
        if (error.name !== "AbortError") {
          setConversation((prev) => {
            if (!prev) return prev;
            const next = [...prev.messages];
            next[next.length - 1] = {
              role: "assistant",
              content: `⚠️ Errore: ${error.message}`,
            };
            return { ...prev, messages: next };
          });
        }
      } finally {
        setIsGenerating(false);
        abortRef.current = null;

        const finalConv = conversationRef.current;
        if (finalConv) {
          try {
            await persist({ ...finalConv, updatedAt: Date.now() });
            await saveLastSettings(finalConv.provider, finalConv.model, finalConv.params);
          } catch (persistErr) {
            // Prima non arrivava a schermo: se il salvataggio fallisce (storage, permessi,
            // dato corrotto...) ora almeno lo vediamo in console invece di sparire nel nulla.
            console.error("Errore nel salvataggio della conversazione:", persistErr);
          }
        }
      }
    },
    [persist]
  );

  const stopGeneration = useCallback(() => {
    abortRef.current?.abort();
  }, []);

  const newConversation = useCallback(() => {
    const conv = conversationRef.current;
    const provider = conv?.provider || "ollama";
    const model =
      conv?.model || providerStatus[provider]?.models[0] || getProvider(provider).fallbackModels[0];
    const params = conv?.params || DEFAULT_PARAMS;
    setConversation(createNewConversation(provider, model, params));
  }, [providerStatus]);

  const selectConversation = useCallback(async (id: string) => {
    if (id === conversationRef.current?.id) return;
    const loaded = await loadConversation(id);
    if (loaded) setConversation(loaded);
  }, []);

  const deleteConversation = useCallback(
    async (id: string) => {
      await deleteConversationFromStorage(id);
      setSummaries((prev) => prev.filter((s) => s.id !== id));
      if (conversationRef.current?.id === id) {
        newConversation();
      }
    },
    [newConversation]
  );

  const changeModel = useCallback((option: ModelOption) => {
    setConversation((prev) =>
      prev ? { ...prev, provider: option.provider, model: option.model } : prev
    );
  }, []);

  const activeProviderId = conversation?.provider ?? "ollama";
  const activeProvider = getProvider(activeProviderId);
  const activeStatus = providerStatus[activeProviderId];
  const modelOptions = useMemo(() => buildModelOptions(providerStatus), [providerStatus]);
  const showOllamaTools = activeProviderId === "ollama";

  const value: ChatContextValue = {
    conversation,
    summaries,
    isGenerating,
    apiKeys,
    providerStatus,
    modelOptions,
    activeProvider,
    activeStatus,
    showOllamaTools,
    sendMessage,
    stopGeneration,
    newConversation,
    selectConversation,
    deleteConversation,
    changeModel,
    refreshProviderStatus,
  };

  return <ChatContext.Provider value={value}>{children}</ChatContext.Provider>;
}

export function useChat(): ChatContextValue {
  const ctx = useContext(ChatContext);
  if (!ctx) {
    throw new Error("useChat deve essere usato dentro un <ChatProvider>");
  }
  return ctx;
}