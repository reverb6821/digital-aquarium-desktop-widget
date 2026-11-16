# Acquario Digitale — Specifica di progetto per Claude Code

> **Destinatario:** Claude Code. Questo documento descrive cosa costruire, su quale hardware, con quali vincoli e in quale ordine. Leggilo tutto prima di scrivere codice. Dove un dato è marcato **[DA VERIFICARE]**, controllalo sugli esempi ufficiali Waveshare o sul datasheet prima di usarlo.

---

## 1. Obiettivo

Realizzare un **acquario digitale animato in stile pixel-art** che gira su una **Waveshare ESP32-S3-Touch-LCD-4.3** (800×480, touch capacitivo). L'utente deve poter **personalizzare l'acquario dal touch screen**: bioma, sfondo, fondale, piante, decorazioni, pesci e altri parametri. **Solo acqua dolce.**

### Riferimento visivo

L'utente ha fornito uno screenshot di riferimento (da salvare in `docs/reference.png`, se presente). Lo stile è:

- **Pixel-art a bassa risoluzione** scalata (pixel visibili, palette limitata, nessun anti-aliasing).
- Acqua **blu/verde molto scura**, con leggero gradiente verticale (più chiaro in alto).
- **Superficie dell'acqua** in alto: una linea ondulata sottile, chiara, animata.
- **Raggi di luce** (god rays) diagonali, semi-trasparenti, che si muovono lentamente.
- **Colonna di bolle** che sale da un punto del fondale, bolle disegnate come piccoli cerchi vuoti.
- **Fondale** scuro con texture (roccia/ghiaia) e un profilo collinare irregolare.
- **Piante** d'acqua dolce reali (Cryptocoryne, Microsorum, Bolbitis, Anubias, Echinodorus, vallisneria, muschio...) e hardscape di rocce e legni.
- **Particelle** in sospensione (puntini bianchi tenui) che derivano lentamente.
- Creature con comportamenti diversi: banchi che si muovono insieme, pesci solitari, pesci da fondo.

### Biomi preset (solo acqua dolce, tutti con asset **originali**)

| Bioma | Ambiente | Piante / hardscape | Pesci |
|---|---|---|---|
| **Amazzonico** | Acqua verde-teal limpida, sabbia, raggi di luce | Echinodorus, vallisneria, piante a stelo (Rotala), muschio; tronco, spiderwood, ramo, foglie | Neon, pesce brace, scalari, corydoras |
| **Acque nere** | Acqua color tè (tannini), luce tenue, sabbia scura | Intrico di rami alti, radici, spiderwood, foglie di catappa; pochi Echinodorus e muschio | Cardinali, discus, corydoras |
| **Sud-Est asiatico** | Ruscello del Borneo, ghiaia | Prato di Cryptocoryne, Microsorum, Bucephalandra, vallisneria; rocce, sassi, legni | Rasbora arlecchino, gourami, Pangio kuhlii, caridine (red cherry, crystal red, amano), lumache killer |
| **Africa (Congo)** | Acqua bruno-verde, sabbia | Bolbitis heudelotii, Anubias, vallisneria; radici, ardesia, tronco, ramo | Tetra del Congo, Pelvicachromis |
| **Lago Malawi** | Blu limpido, sabbia chiara, molta luce | Cumuli di rocce: grandi, alte, ardesia, sassi; pochissime piante | Ciclidi mbuna (3 colori), ciclide pavone |
| **Laghetto** | Acqua verde, ciottoli | Ninfee con foglie galleggianti, Egeria, vallisneria; rocce e radici | Carpe koi (kohaku, sanke, ogon), pesci rossi, medaka, Planorbarius |

> ⚠️ **Non usare personaggi protetti da copyright** (es. Pokémon, Nemo, ecc.) né sprite presi da giochi o app esistenti. Tutti gli sprite devono essere originali: disegnati proceduralmente nel codice o creati ex novo come pixel-art.

---

## 2. Hardware

Fonte: <https://docs.waveshare.com/ESP32-S3-Touch-LCD-4.3>

| Componente | Dettaglio |
|---|---|
| MCU | ESP32-S3 (modulo **ESP32-S3-WROOM-1-N16R8**), dual-core LX7 @ 240 MHz |
| Memoria | 512 KB SRAM interna, **16 MB Flash**, **8 MB PSRAM** (Octal) |
| Wireless | Wi-Fi 2.4 GHz b/g/n, Bluetooth 5 LE |
| Display | IPS 4.3", **800×480**, RGB565, interfaccia **RGB parallela** |
| Retroilluminazione | Abilitata da **EXIO2** del CH422G (on/off, **niente PWM** di serie) |
| Touch | Capacitivo 5 punti, I2C. Controller **GT911** [DA VERIFICARE], indirizzo 0x5D o 0x14 [DA VERIFICARE] |
| Pin touch | SDA **GPIO8**, SCL **GPIO9**, IRQ **GPIO4**, RST su **EXIO1** |
| IO expander | **CH422G** su I2C (GPIO8/9): EXIO1 = TP_RST, EXIO2 = backlight, EXIO4 = SD_CS, EXIO5 = USB/CAN select |
| MicroSD | SPI: MOSI **GPIO11**, SCK **GPIO12**, MISO **GPIO13**, CS su **EXIO4** (attivo basso) |
| Alimentazione | USB-C 5V; batteria Li-ion 3.7V opzionale (PH2.0) |

### Pinout LCD (GPIO → segnale)

```
GPIO0  G3     GPIO1  R3     GPIO2  R4     GPIO3  VSYNC
GPIO5  DE     GPIO7  PCLK   GPIO10 B7     GPIO14 B3
GPIO17 B6     GPIO18 B5     GPIO21 G7     GPIO38 B4
GPIO39 G2     GPIO40 R7     GPIO41 R6     GPIO42 R5
GPIO45 G4     GPIO46 HSYNC  GPIO47 G6     GPIO48 G5
```

Ordine dati RGB565 per `esp_lcd_rgb_panel_config_t.data_gpio_nums` (B3..B7, G2..G7, R3..R7):

```c
// B0..B4
14, 38, 18, 17, 10,
// G0..G5
39, 0, 45, 48, 47, 21,
// R0..R4
1, 2, 42, 41, 40
```

### Timing RGB di partenza [DA VERIFICARE con la demo ESP-IDF Waveshare]

```c
.pclk_hz = 16 * 1000 * 1000,
.h_res = 800, .v_res = 480,
.hsync_pulse_width = 4, .hsync_back_porch = 8, .hsync_front_porch = 8,
.vsync_pulse_width = 4, .vsync_back_porch = 8, .vsync_front_porch = 8,
.flags.pclk_active_neg = 1,
```

---

## 3. Stack tecnico

| Scelta | Decisione | Motivo |
|---|---|---|
| Framework | **ESP-IDF v5.3+** (C, CMake) | Controllo fine su RGB panel, bounce buffer, PSRAM, task e core |
| Driver display | `esp_lcd_panel_rgb` | Nativo IDF |
| Driver touch | `esp_lcd_touch_gt911` (component registry) | |
| IO expander | Componente CH422G dal registry se disponibile, altrimenti driver minimale I2C | Il CH422G usa "indirizzi-comando" (es. 0x24 config, 0x38 scrittura IO) [DA VERIFICARE su datasheet] |
| Rendering acquario | **Motore 2D custom** (non LVGL) | Serve un compositing di sprite veloce a framerate costante |
| UI impostazioni | **LVGL v9** sopra lo stesso framebuffer, attiva solo quando il menu è aperto | Widget pronti (liste, slider, color picker) |
| Persistenza | **NVS** per la config attiva + **JSON su SD/LittleFS** per preset e asset | |
| Simulatore desktop | **SDL2** su Linux (target secondario, vedi §11) | Iterazione rapida senza flashare |

Ambiente di sviluppo dell'utente: **Nobara Linux (Fedora) con GNOME**. Le istruzioni per installare dipendenze devono usare `dnf` (es. `sudo dnf install SDL2-devel cmake ninja-build python3-pip`), e il flash via `/dev/ttyACM0` o `/dev/ttyUSB0` (l'utente deve essere nel gruppo `dialout`).

---

## 4. Strategia di rendering (punto critico)

L'RGB panel legge **continuamente** il framebuffer dalla PSRAM, quindi la banda PSRAM è la risorsa più scarsa. Se viene saturata compare il **"drift"/scorrimento dell'immagine**.

### Approccio richiesto: canvas a bassa risoluzione + upscale 2×

1. Il mondo dell'acquario è renderizzato in un **canvas logico 400×240 RGB565** (192 KB).
2. Il canvas viene **scalato 2× (nearest neighbor)** verso il display 800×480.
3. Vantaggi: 4× meno pixel da comporre, look pixel-art identico al riferimento, coordinate touch = `touch / 2`.

### Due implementazioni da confrontare (fai un benchmark nella Fase 1)

- **A — Framebuffer in PSRAM:** double buffer 800×480 in PSRAM (2 × 768 KB), il render task compone il canvas 400×240 e poi fa l'upscale nel back buffer, poi `esp_lcd_panel_draw_bitmap` / swap su VSYNC.
- **B — Bounce buffer con upscale "al volo":** nessun framebuffer full-size; si usa `bounce_buffer_size_px` e la callback `on_bounce_empty` per riempire il bounce buffer leggendo dal canvas 400×240 (in SRAM interna se c'è spazio, altrimenti PSRAM) e duplicando pixel e righe. Molto meno traffico PSRAM.

Scegli quella con framerate stabile e **zero drift**, anche con Wi-Fi attivo e scritture su flash. Documenta i numeri misurati in `docs/benchmark.md`.

### Configurazione PSRAM / sdkconfig (punto di partenza)

```
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_FETCH_INSTRUCTIONS=y      # evita drift durante scritture flash
CONFIG_SPIRAM_RODATA=y
CONFIG_ESP32S3_DATA_CACHE_64KB=y
CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y
CONFIG_LCD_RGB_RESTART_IN_VSYNC=y
CONFIG_FREERTOS_HZ=1000
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
```

### Budget di prestazioni

- **Target: 30 FPS**, minimo accettabile 20 FPS stabili.
- Render su **core 1**; Wi-Fi, touch, I/O su **core 0**.
- Lo **sfondo statico** (gradiente acqua + fondale + piante ferme) va pre-renderizzato in un layer cache e rigenerato **solo** quando cambia la configurazione.
- Usare **dirty rectangles** quando conviene (sprite piccoli su sfondo cache), con fallback a ridisegno completo se i dirty rect superano ~40% del canvas.
- Niente `malloc` nel loop di render: pool pre-allocati.
- Matematica in **fixed-point** o `float` (l'S3 ha FPU single precision); tabelle precalcolate per `sin/cos`.

---

## 5. Architettura software

```
aquarium/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv                 # app 4MB, storage LittleFS ~10MB, nvs
├── main/
│   └── main.c                     # init hardware, avvio task
├── components/
│   ├── board/                     # HAL specifica della Waveshare 4.3
│   │   ├── ch422g.c/.h            # IO expander
│   │   ├── display.c/.h           # RGB panel, backlight, vsync
│   │   ├── touch.c/.h             # GT911 + gesture (tap, long press, drag)
│   │   └── sdcard.c/.h
│   ├── engine/                    # PORTABILE: nessuna dipendenza da ESP-IDF
│   │   ├── canvas.c/.h            # buffer RGB565, blit, blend, clip
│   │   ├── sprite.c/.h            # sprite, animazioni a frame, flip orizzontale
│   │   ├── palette.c/.h           # sprite indicizzati 4/8 bit + palette swap
│   │   ├── rng.c/.h               # PRNG deterministico con seed
│   │   └── fixmath.h
│   ├── aquarium/                  # PORTABILE: logica della scena
│   │   ├── scene.c/.h             # layer, ordine di disegno
│   │   ├── water.c/.h             # gradiente, superficie, god rays, particelle
│   │   ├── substrate.c/.h         # generazione fondale procedurale
│   │   ├── plants.c/.h            # piante procedurali (L-system semplice) + ondeggiamento
│   │   ├── decor.c/.h             # rocce, sassi, ardesia, legni, rami, radici, foglie
│   │   ├── bubbles.c/.h
│   │   ├── creatures.c/.h         # registry specie
│   │   ├── behavior.c/.h          # boids, wander, bottom-dweller
│   │   ├── food.c/.h              # cibo che affonda + attrazione pesci
│   │   ├── daynight.c/.h          # ciclo giorno/notte
│   │   └── themes.c/.h            # preset dei 6 biomi d'acqua dolce
│   ├── config/
│   │   ├── config.c/.h            # struct AquariumConfig + (de)serializzazione JSON
│   │   └── storage.c/.h           # NVS + file preset
│   ├── ui/                        # LVGL: menu impostazioni ed edit mode
│   └── webcfg/                    # (Fase 6) server HTTP per configurare da browser
├── assets/
│   ├── src/                       # PNG sorgente pixel-art originali
│   └── tools/png2c.py             # converte PNG → array C indicizzati
├── sim/                           # simulatore SDL2 per Linux
│   ├── CMakeLists.txt
│   └── main_sdl.c                 # mouse = touch, finestra 800×480
└── docs/
    ├── reference.png
    └── benchmark.md
```

**Regola fondamentale:** `engine/` e `aquarium/` non devono includere header ESP-IDF. Comunicano con l'hardware solo tramite un'interfaccia HAL minima (`hal_present(canvas)`, `hal_millis()`, `hal_touch_read()`), così lo stesso codice gira nel simulatore SDL.

### Task FreeRTOS

| Task | Core | Priorità | Compito |
|---|---|---|---|
| `render_task` | 1 | alta | update simulazione (dt fisso) + composizione + present |
| `input_task` | 0 | media | polling/IRQ touch, riconoscimento gesture, coda eventi |
| `storage_task` | 0 | bassa | salvataggi config differiti (debounce 2 s) per non disturbare il render |
| `wifi/web` | 0 | bassa | solo Fase 6 |

---

## 6. Scena: layer (dal fondo al primo piano)

1. **Acqua** — gradiente verticale (2–3 colori dal tema) + dithering ordinato per evitare banding in RGB565.
2. **God rays** — 2–5 fasce diagonali semitrasparenti, intensità e posizione animate lentamente con `sin`.
3. **Sfondo lontano** — silhouette di rocce/piante in colore attenuato (parallasse lieve, opzionale).
4. **Particelle** — 30–80 puntini che derivano lentamente.
5. **Piante e decorazioni dietro**.
6. **Creature** — ordinate per profondità (z), con scala/luminosità ridotta per quelle "lontane".
7. **Bolle**.
8. **Fondale** — profilo collinare procedurale (rumore 1D) + texture (ghiaia, sabbia, roccia scura con pattern a cerchi come nel riferimento).
9. **Piante e decorazioni davanti**.
10. **Superficie acqua** — linea ondulata animata in alto.
11. **Overlay** — vignettatura opzionale, filtro notte, effetti bioluminescenza (additive blend).
12. **UI** (solo quando aperta).

---

## 7. Creature e comportamenti

Ogni specie è definita da dati, non da codice dedicato:

```c
typedef struct {
    const char *id;            // "neon_tetra"
    const char *name;          // "Neon"
    const SpriteSheet *sprite; // frame di nuoto (2–4 frame)
    BehaviorType behavior;     // SCHOOL, WANDER, BOTTOM, SHRIMP, SNAIL
    uint8_t size_px;           // sul canvas 400×240
    float speed_min, speed_max;
    uint8_t depth_min, depth_max; // fascia verticale preferita (0=superficie, 255=fondo)
    uint8_t default_count;
    uint8_t max_count;
    uint16_t palette_variants; // per variazioni di colore
} SpeciesDef;
```

### Comportamenti

- **SCHOOL (banco)** — boids: separazione, allineamento, coesione, solo tra individui della stessa specie; evitare bordi con forza morbida; uso di griglia spaziale per tenere il costo O(n).
- **WANDER (solitario)** — target casuale nella sua fascia di profondità, curve morbide, pause occasionali, flip dello sprite quando cambia direzione.
- **BOTTOM (da fondo)** — segue il profilo del fondale, movimento lento a scatti, si ferma a "mangiare".
- **SHRIMP (caridina)** — cammina e becchetta sul fondo; ogni tanto (o se spaventata) scatto all'indietro con colpo di coda e lenta ricaduta.
- **SNAIL (lumaca)** — strisciata lentissima con lunghe pause; al tocco si ritira nel guscio per qualche secondo.

### Interazioni

- **Tap sull'acqua** → cade cibo nel punto toccato; i pesci entro un raggio si dirigono verso il cibo, lo "mangiano" (sparisce).
- **Tap sul vetro vicino a un pesce** → il pesce si spaventa e scappa (il banco si disperde e si ricompatta).
- Limite globale di creature configurabile (default 60) per rispettare il budget FPS.

---

## 8. Personalizzazione

### Oggetto di configurazione

```c
typedef struct {
    uint8_t  version;
    char     theme_id[16];          // "amazon" | "blackwater" | "seasia" | "africa" | "malawi" | "pond" | "custom"

    // Acqua / sfondo
    uint16_t water_top, water_bottom;   // RGB565
    uint8_t  light_rays;            // 0..5
    uint8_t  particles;             // densità 0..100
    bool     show_surface;

    // Fondale
    uint8_t  substrate_type;        // SAND, GRAVEL, DARK_ROCK, PEBBLES, NONE
    uint16_t substrate_color;
    uint8_t  substrate_height;      // 10..60 px sul canvas
    uint8_t  substrate_roughness;   // 0..100
    uint32_t substrate_seed;

    // Piante e decorazioni (posizionabili)
    uint8_t  plant_count;
    struct { uint8_t type; int16_t x; uint8_t layer; uint8_t height; uint16_t color; uint32_t seed; } plants[24];
    uint8_t  decor_count;
    struct { uint8_t type; int16_t x; uint8_t layer; uint16_t color; } decor[16];

    // Bolle
    uint8_t  bubble_sources;
    struct { int16_t x; uint8_t rate; } bubbles[4];

    // Creature
    uint8_t  species_count;
    struct { char id[20]; uint8_t count; uint8_t variant; } species[12];

    // Ambiente
    bool     day_night_cycle;       // usa ora NTP se Wi-Fi disponibile, altrimenti ciclo simulato
    uint8_t  sim_speed;             // 50..200 %
    uint8_t  fps_cap;               // 20 | 30
    uint16_t screen_off_minutes;    // 0 = mai (spegne backlight via EXIO2)
} AquariumConfig;
```

- Serializzazione **JSON** (cJSON incluso in IDF) per preset su LittleFS/SD; copia binaria compatta in **NVS** per l'avvio rapido.
- Campo `version` per migrazioni future.
- **Preset**: i 6 biomi sono preset di sola lettura; l'utente può salvare fino a 8 preset personali.

### Catalogo elementi selezionabili (minimo)

- **Piante:** pianta a stelo (Rotala/Egeria), vallisneria, Cryptocoryne, Microsorum, Bolbitis heudelotii, Anubias, Bucephalandra, Echinodorus, muschio di Java, ninfea.
- **Decorazioni:** roccia grande, dragon stone, Seiryu stone, gruppo di sassi, ardesia impilata, tronco, spiderwood, ramo alto, radici ad arco, foglie di catappa.
- **Fondali:** sabbia, ghiaia, roccia scura con pattern, ciottoli, nessuno.
- **Sfondi acqua:** palette per bioma (amazzonico, acque nere, Sud-Est asiatico, Congo, blu Malawi, verde laghetto) + colori custom.
- **Creature (solo acqua dolce):** neon, cardinale, pesce brace, scalare, discus, corydoras, rasbora arlecchino, gourami, Pangio kuhlii, tetra del Congo, Pelvicachromis, ciclide mbuna, ciclide pavone, carpa koi, pesce rosso, medaka; invertebrati: caridina (red cherry, crystal red, amano, blue dream), Planorbarius (bruna, rossa, blu), lumaca killer (Clea helena).

---

## 9. Interfaccia touch

### Gesture

| Gesto | Azione |
|---|---|
| Tap | Dà da mangiare / spaventa pesci |
| **Long press (1 s)** | Apre il menu impostazioni |
| Doppio tap | Mostra/nasconde un piccolo overlay (ora, FPS se debug attivo) |
| Swipe orizzontale a 2 dita | Bioma successivo/precedente |

### Menu (LVGL)

Pannello laterale semitrasparente (l'acquario continua ad animarsi dietro, eventualmente a FPS ridotto):

1. **Bioma** — griglia di anteprime dei 6 biomi + preset utente (thumbnail renderizzate on-device a 1/4).
2. **Acqua** — colori gradiente, raggi di luce, particelle, superficie.
3. **Fondale** — tipo, colore, altezza, rugosità, pulsante "rigenera" (nuovo seed).
4. **Piante e decorazioni** — elenco con aggiunta/rimozione; pulsante **"Modalità modifica"**.
5. **Creature** — lista specie con contatore +/− e scelta variante.
6. **Ambiente** — ciclo giorno/notte, velocità, spegnimento schermo, Wi-Fi.
7. **Preset** — salva/carica/elimina.

### Modalità modifica

- Il menu si chiude, compare una barra in basso con "Fatto" / "Annulla".
- Le piante/decorazioni diventano **trascinabili** in orizzontale e in verticale (offset `dy` sopra il fondale; entro 3 px si riaggancia al profilo del fondale) e **ruotabili** a passi di 15° attorno al punto di base (`rot`); tap per selezionare, pulsanti per layer davanti/dietro, ruota colore, -15/+15, "Fondo" (azzera offset e rotazione), -/+ dimensione di piante e arredi (`scale` 50-200%, passi del 10%; la ninfea si allarga solo in orizzontale), elimina.
- Le creature vengono nascoste o rallentate durante l'editing.

Le modifiche si applicano **in anteprima live**; il salvataggio su storage avviene con debounce.

---

## 10. Asset

- **Preferenza: generazione procedurale** di piante, fondale, bolle, particelle, raggi, rocce e legni (meno memoria, più varietà via seed).
- Per i pesci: sprite pixel-art **originali** piccoli (8–24 px di larghezza sul canvas), 2–4 frame, formato **indicizzato 4 bit + palette** per permettere varianti di colore con palette swap.
- `assets/tools/png2c.py`: converte PNG indicizzati in array C (`const` in flash) con struct `SpriteSheet`. Deve rifiutare PNG con più di 16 colori e riportare l'errore.
- Se lo spazio flash lo consente gli asset restano in flash; un pacchetto opzionale di asset extra può essere caricato da **microSD** (`/sdcard/aquarium/`).
- Se generi tu gli sprite come array di pixel nel codice, mantieni uno stile coerente con il riferimento (contorni morbidi, 3–5 tonalità per pesce, occhio di 1 px).

---

## 11. Simulatore desktop (Linux / SDL2)

- Target CMake separato in `sim/` che compila `engine/`, `aquarium/`, `config/` (con storage su file locale) e una HAL SDL.
- Finestra 800×480, il mouse simula il touch (click = tap, tenere premuto = long press, trascinare = drag).
- Tasto `F` mostra FPS e tempo di frame; tasto `S` salva screenshot PNG; tasti `1–6` cambiano bioma.
- Build:
  ```bash
  sudo dnf install SDL2-devel cmake ninja-build gcc
  cmake -S sim -B build-sim -G Ninja && cmake --build build-sim && ./build-sim/aquarium_sim
  ```
- LVGL può girare anche nel simulatore con il suo driver SDL (opzionale, utile per la Fase 5).

---

## 12. Fasi di sviluppo e criteri di accettazione

Procedi **una fase alla volta**. Alla fine di ogni fase: compila, verifica i criteri, fai commit, e riassumi cosa è stato fatto e cosa va testato sull'hardware dall'utente.

### Fase 0 — Setup
- Progetto ESP-IDF, `sdkconfig.defaults`, `partitions.csv`, struttura cartelle, README con comandi di build/flash per Fedora/Nobara.
- ✅ `idf.py build` compila senza warning rilevanti.

### Fase 1 — Bring-up hardware
- Driver CH422G, backlight on, RGB panel, touch GT911 (con corretta sequenza di reset via EXIO1).
- Test pattern: barre colore, gradiente, quadrato che si muove, cerchio che segue il dito.
- Benchmark approcci A e B (§4) → `docs/benchmark.md`.
- ✅ Immagine stabile senza drift, touch preciso agli angoli, FPS misurati.

### Fase 2 — Motore e scena statica + simulatore
- Canvas, blit, sprite, palette, PRNG. Simulatore SDL funzionante.
- Acqua con gradiente e dithering, fondale procedurale, piante procedurali, superficie, raggi, particelle, bolle.
- ✅ Scena "Amazzonico" senza pesci visivamente vicina al riferimento, ≥30 FPS su device.

### Fase 3 — Creature
- Registry specie, comportamenti SCHOOL, WANDER, BOTTOM.
- Popolare i biomi Amazzonico e Acque nere.
- Cibo e spavento con il tocco.
- ✅ 40+ creature a ≥25 FPS; banchi credibili; nessuna creatura bloccata ai bordi.

### Fase 4 — Temi e configurazione
- Biomi Sud-Est asiatico, Africa (Congo), Lago Malawi, Laghetto.
- `AquariumConfig`, JSON, NVS, preset, cambio tema a caldo (rigenerazione del layer cache senza glitch visibili, eventualmente con dissolvenza).
- ✅ Riavviando la scheda si ritrova l'acquario come lasciato.

### Fase 5 — UI di personalizzazione
- LVGL, menu completo §9, modalità modifica con drag delle piante.
- ✅ Tutte le opzioni di §8 modificabili dal touch con anteprima live; il menu non fa scendere l'acquario sotto 15 FPS.

### Fase 6 — Extra (opzionali, in ordine di priorità)
1. Ciclo giorno/notte con NTP (Wi-Fi configurabile dal menu; tastiera LVGL).
2. Spegnimento schermo programmato (EXIO2) e risveglio al tocco.
3. Pagina web di configurazione servita dall'ESP32 (stesso JSON della config).
4. Asset extra da microSD.
5. Aggiornamento OTA.

---

## 13. Rischi noti e indicazioni

- **Drift dell'immagine RGB:** quasi sempre banda PSRAM satura o scritture flash con cache disabilitata. Mitigazioni: bounce buffer, `SPIRAM_FETCH_INSTRUCTIONS`/`RODATA`, restart in VSYNC, salvataggi rari e differiti.
- **Bus I2C condiviso** tra touch e CH422G: proteggere con mutex; non leggere il touch durante la sequenza di reset.
- **GT911**: l'indirizzo I2C dipende dallo stato del pin INT durante il reset; replicare la sequenza della demo Waveshare.
- **Backlight solo on/off**: la "luminosità" va simulata scurendo il rendering (filtro notte), non con PWM.
- **RGB565 e gradienti scuri**: usare dithering ordinato (Bayer 4×4) per evitare bande visibili.
- **Watchdog**: il render task deve cedere (`vTaskDelay`/attesa VSYNC) ogni frame.
- **USB/CAN** condividono GPIO19/20 tramite EXIO5: lasciare EXIO5 in modalità USB.

---

## 14. Regole di lavoro per Claude Code

- Prima di iniziare, cerca le **demo ufficiali Waveshare ESP-IDF** per questa scheda e usale come riferimento per timing, init del touch e CH422G; segnala ogni differenza con questa specifica.
- Codice **C11** pulito, moderno, commentato dove la logica non è ovvia; nessun warning con `-Wall -Wextra`.
- Mantieni `engine/` e `aquarium/` portabili e testabili nel simulatore.
- Non aggiungere dipendenze pesanti senza chiedere.
- Se un dato hardware non è verificabile, **chiedi all'utente** invece di indovinare.
- Ad ogni fase, indica all'utente i comandi esatti per compilare e flashare:
  ```bash
  . $HOME/esp/esp-idf/export.sh
  idf.py set-target esp32s3
  idf.py build flash monitor -p /dev/ttyACM0
  ```