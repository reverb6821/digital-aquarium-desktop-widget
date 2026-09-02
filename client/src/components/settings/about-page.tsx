export function SettingsAboutPage() {
  return (
    <div className="mx-auto max-w-xl space-y-5 p-6">
      <div>
        <h1 className="text-sm font-semibold">Informazioni sull'IA e trasparenza</h1>
        <p className="mt-1 text-xs text-muted-foreground">
          Redatte per rispettare lo spirito degli obblighi di trasparenza del
          Regolamento (UE) 2024/1689 (AI Act) per i sistemi che interagiscono con
          persone fisiche — non costituiscono consulenza legale.
        </p>
      </div>

      <section className="space-y-1.5">
        <h2 className="text-xs font-semibold uppercase tracking-wide text-muted-foreground">
          Stai parlando con un'IA
        </h2>
        <p className="text-xs">
          Ogni risposta in questa chat è generata da un modello linguistico (LLM),
          non da una persona. I modelli possono commettere errori, essere non
          aggiornati o generare informazioni plausibili ma false.
        </p>
      </section>

      <section className="space-y-1.5">
        <h2 className="text-xs font-semibold uppercase tracking-wide text-muted-foreground">
          Dove vanno i tuoi dati
        </h2>
        <ul className="space-y-1.5 text-xs">
          <li>
            <strong>Ollama (locale)</strong>: tutto resta sul tuo computer, nessun
            dato esce.
          </li>
          <li>
            <strong>Claude (Anthropic)</strong>: i messaggi vengono inviati ai
            server Anthropic.{" "}
            <a
              className="underline"
              href="https://www.anthropic.com/legal/privacy"
              target="_blank"
              rel="noreferrer"
            >
              Informativa privacy
            </a>
          </li>
          <li>
            <strong>ChatGPT (OpenAI)</strong>: i messaggi vengono inviati ai
            server OpenAI.{" "}
            <a
              className="underline"
              href="https://openai.com/policies/privacy-policy"
              target="_blank"
              rel="noreferrer"
            >
              Informativa privacy
            </a>
          </li>
          <li>
            <strong>Gemini (Google)</strong>: i messaggi vengono inviati ai
            server Google.{" "}
            <a
              className="underline"
              href="https://policies.google.com/privacy"
              target="_blank"
              rel="noreferrer"
            >
              Informativa privacy
            </a>
          </li>
          <li>
            <strong>DeepSeek</strong>: i messaggi vengono inviati ai server
            DeepSeek.{" "}
            <a
              className="underline"
              href="https://www.deepseek.com/privacy"
              target="_blank"
              rel="noreferrer"
            >
              Informativa privacy
            </a>
          </li>
        </ul>
      </section>

      <section className="space-y-1.5">
        <h2 className="text-xs font-semibold uppercase tracking-wide text-muted-foreground">
          Cosa salviamo in locale
        </h2>
        <p className="text-xs">
          Conversazioni, impostazioni e API key restano sul tuo dispositivo — non
          vengono inviate a server nostri (non ne abbiamo). Le API key sono
          protette dal meccanismo di sicurezza nativo del tuo sistema operativo
          (Keychain su macOS, Secret Service su Linux, DPAPI su Windows).
        </p>
      </section>

      <section className="space-y-1.5">
        <h2 className="text-xs font-semibold uppercase tracking-wide text-muted-foreground">
          Limiti d'uso consigliati
        </h2>
        <p className="text-xs">
          Non affidarti alle risposte di questa chat come unica base per decisioni
          mediche, legali, finanziarie o di sicurezza. Verifica sempre le
          informazioni importanti con fonti affidabili.
        </p>
      </section>
    </div>
  );
}