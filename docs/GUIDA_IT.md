# Guida: flash e test dell'Acquario Digitale

Scheda: **Waveshare ESP32-S3-Touch-LCD-4.3** · PC: **Nobara/Fedora Linux** · IDE: **Arduino IDE 2**

> English version: [GUIDE_EN.md](GUIDE_EN.md)

---

## 1. Preparazione (una sola volta)

### 1.1 Permessi sulla porta seriale

```bash
sudo usermod -aG dialout $USER
```

Poi fai **logout e login** (o riavvia il PC). Verifica con `groups`: deve comparire `dialout`.

### 1.2 Core ESP32 nell'Arduino IDE

In **Strumenti → Scheda → Gestore schede** cerca `esp32` (di Espressif Systems) e installa la
versione **3.1 o superiore**. Sul tuo PC è già installata la 3.3.12. Non servono altre librerie.

### 1.3 Collegare la scheda

Collega la scheda con un cavo USB-C **dati** (non solo ricarica). Controlla quale porta compare:

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

- `/dev/ttyACM0` → porta USB nativa dell'ESP32-S3 → in seguito usa **USB CDC On Boot: Enabled**
- `/dev/ttyUSB0` → porta tramite convertitore UART → usa **USB CDC On Boot: Disabled**

Se non compare nessuna porta, prova un altro cavo. Puoi anche avviare la scheda in modalità
download: tieni premuto **BOOT**, premi e rilascia **RESET**, poi rilascia **BOOT**.

---

## 2. Flash con Arduino IDE

1. **File → Apri** → `digital_aquarium_esp/aquarium/aquarium.ino`
   (la cartella deve chiamarsi `aquarium` come il file `.ino`).
2. Imposta il menu **Strumenti**:

   | Opzione | Valore |
   |---|---|
   | Scheda | **ESP32S3 Dev Module** |
   | Porta | `/dev/ttyACM0` (o `/dev/ttyUSB0`) |
   | PSRAM | **OPI PSRAM** ← obbligatorio |
   | Flash Size | **16MB (128Mb)** |
   | Partition Scheme | **Custom** (usa `aquarium/partitions.csv`) |
   | USB CDC On Boot | **Enabled** (ttyACM) / **Disabled** (ttyUSB) |
   | CPU Frequency | 240MHz (WiFi) |
   | Upload Speed | 921600 (se fallisce: 460800) |

3. Premi **Carica** (freccia →). La compilazione richiede circa 30–60 s la prima volta.
4. Apri **Strumenti → Monitor seriale** a **115200 baud**.

> Se cambi "USB CDC On Boot" rispetto all'ultimo flash, la prima volta attiva anche
> **Strumenti → Erase All Flash Before Sketch Upload: Enabled**, poi rimettilo su Disabled.
> Attenzione: cancella anche configurazione e preset salvati.

### Primo avvio

Al primo avvio la scheda **formatta la partizione LittleFS** (~12 MB): può restare a schermo nero
per **10–30 secondi**. Succede una volta sola.

Aggiornando da un firmware precedente alla versione con i biomi d'acqua dolce, l'acquario salvato e
i preset non vengono migrati: si riparte dal bioma Amazzonico.

Nel monitor seriale dovresti vedere qualcosa di simile:

```
=== Acquario Digitale ===
[board] PSRAM 8388608 bytes, internal free ...
[touch] GT911 id '911 ' at 0x5D, resolution 800x480
[lcd] mode B (bounce-buffer upscale), bounce 20 lines
config: defaults
[perf] mode B (bounce-buffer upscale)  fps=30.0  render=..ms  present-wait=..ms  heap int=... psram=...
```

---

## 3. Flash da terminale (alternativa, arduino-cli)

Installa `arduino-cli` (usa gli stessi core e toolchain già scaricati dall'IDE):

```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
```

Poi, dalla cartella del progetto:

```bash
cd ~/Development/digital_aquarium_esp
FQBN="esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,PartitionScheme=custom,CDCOnBoot=cdc"
arduino-cli compile --fqbn "$FQBN" --warnings all aquarium
arduino-cli upload  --fqbn "$FQBN" -p /dev/ttyACM0 aquarium
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200
```

Con la porta UART (`/dev/ttyUSB0`) sostituisci `CDCOnBoot=cdc` con `CDCOnBoot=default`.

---

## 4. Test hardware (Fase 1: bring-up)

Lo scopo è verificare display, touch e stabilità **prima** dell'acquario.

1. Apri `aquarium/src/board/board_config.h` e imposta:
   ```c
   #define BRINGUP_TEST 1
   ```
2. Carica lo sketch. Compare una sequenza di schermate. **Tocca lo schermo** per passare alla
   successiva; senza tocchi cambia da sola ogni 8 s.

| Schermata | Cosa controllare | Esito atteso |
|---|---|---|
| **Barre colore** | Ordine da sinistra: bianco, giallo, ciano, verde, magenta, rosso, blu, nero | Colori giusti e nell'ordine giusto (se rosso e blu sono invertiti → segnalalo) |
| **Gradiente** | In alto: gradiente con dithering. In basso: gradiente senza dithering | In alto più liscio, in basso con bande visibili |
| **Touch** | Quadrato arancione che scorre, cerchi bianchi nei 4 angoli | Toccando, un cerchio ciano segue il dito; le coordinate mostrate vanno da circa 0,0 a 799,479 |

3. **Test del drift**: lascia girare 2–3 minuti. In questa modalità un task di stress esegue
   scansioni Wi-Fi e scritture su flash (NVS) di continuo. L'immagine **non deve mai scorrere
   o spostarsi**. In basso a sinistra vedi modo e FPS.
4. **Touch agli angoli**: tocca i 4 cerchi negli angoli. Il cerchio ciano deve centrarli
   (errore massimo di pochi pixel). Prova anche 2 dita: compaiono `P0` e `P1`.

### Benchmark modo A e modo B

Ripeti il test due volte e annota i valori in `docs/benchmark.md`:

| Prova | Impostazione in `board_config.h` |
|---|---|
| Modo B (default) | `#define DISPLAY_MODE DISPLAY_MODE_B` |
| Modo A | `#define DISPLAY_MODE DISPLAY_MODE_A` |

Dal monitor seriale copia le righe `[bench] mode ... fps=... draw=...ms` e annota se c'è drift.

Alla fine **rimetti** `BRINGUP_TEST 0` e `DISPLAY_MODE_B` (salvo che il modo A risulti migliore).

---

## 5. Test dell'acquario (Fasi 2–5)

Con `BRINGUP_TEST 0` carica lo sketch e segui la checklist:

| # | Azione | Esito atteso |
|---|---|---|
| 1 | Guarda la scena all'avvio | Bioma "Amazzonico": raggi di luce, bolle, Echinodorus e vallisneria che ondeggiano, legni, banco di neon, pesci brace, scalari, corydoras |
| 2 | **Doppio tap** | Overlay in alto a sinistra con ora e FPS (≥ 25–30 fps) |
| 3 | **Tap sull'acqua** lontano dai pesci | Cadono granelli di cibo, i pesci vicini li inseguono e li mangiano |
| 4 | **Tap vicino a un pesce** | Il pesce scappa, il banco si disperde e poi si ricompatta |
| 5 | **Swipe orizzontale con 2 dita** | Dissolvenza e cambio bioma (Acque nere → Sud-Est asiatico → Africa → Malawi → Laghetto → ...) |
| 5b | Bioma Sud-Est Asia (tasto/swipe) | Le caridine camminano e becchettano sul fondo e scattano via se tocchi vicino; le lumache killer strisciano lente e si ritirano nel guscio al tocco. Bioma Laghetto: lumache Planorbarius |
| 6 | **Tieni premuto 1 s** | Si apre il menu a destra |
| 7 | Menu → **Bioma** | 6 anteprime, tocca per cambiare bioma |
| 8 | Menu → **Acqua / Fondale** | Le modifiche (colori, raggi, altezza, "Rigenera") si vedono subito |
| 9 | Menu → **Creature** | `+`/`−` aggiunge o toglie pesci; "Colore" cambia variante; limite totale 60 |
| 10 | Menu → **Piante e decor → Modalita' modifica** | Tocca e trascina una pianta o decorazione in orizzontale e in verticale (vicino al fondo si aggancia di nuovo); prova Dietro/Davanti, Colore (tocca il quadratino), **-15/+15** (ruota), **- / +** (dimensione, 50-200%), **Fondo** (torna sul fondo e dritta), Elimina, poi **Fatto** |
| 11 | Menu → **Ambiente** | Giorno/notte, velocità, luminosità, FPS 20/30, spegnimento schermo |
| 12 | Menu → **Preset → Salva**, cambia bioma, poi **Carica** | Torna la scena salvata |
| 13 | Fai una modifica, **chiudi il menu**, aspetta **3 s**, premi **RESET** sulla scheda | L'acquario riparte esattamente come l'avevi lasciato (con il menu aperto non si salva nulla) |
| 14 | Ambiente → "Spegni dopo 5 min", attendi | La retroilluminazione si spegne; il primo tocco la riaccende (quel tocco non dà cibo) |
| 15 | Creature → porta il totale a 60 | Gli FPS nell'overlay restano ≥ 25 |
| 16 | Menu aperto | L'acquario continua ad animarsi dietro, ≥ 15 fps |

Le righe `[perf]` nel monitor seriale riportano ogni 5 s FPS, tempo di render e memoria libera.

---

## 6. Simulatore sul PC (senza scheda)

```bash
sudo dnf install SDL2-devel cmake ninja-build gcc
cd ~/Development/digital_aquarium_esp
cmake -S sim -B build-sim -G Ninja && cmake --build build-sim
./build-sim/aquarium_sim
```

| Input | Azione |
|---|---|
| Click | tap |
| Tieni premuto il tasto sinistro 1 s | menu |
| Doppio click | overlay FPS |
| Trascina col tasto destro | swipe a 2 dita (cambio bioma) |
| `1`–`6` | bioma · `F` overlay · `M` menu · `S` screenshot PNG · `Esc` esci |

Test automatico senza finestra (non richiede SDL):

```bash
./build-sim/aquarium_headless /tmp/aquarium_test
```

Deve terminare con `ALL OK` e salvare gli screenshot in `/tmp/aquarium_test/`.

---

## 7. Problemi comuni

| Sintomo | Soluzione |
|---|---|
| `Permission denied: /dev/ttyACM0` | Non sei nel gruppo `dialout` o non hai rifatto il login (§1.1) |
| Nessuna porta seriale | Cavo solo-ricarica; prova la modalità download (BOOT + RESET) |
| Upload si blocca su `Connecting...` | Tieni premuto BOOT, premi RESET, rilascia BOOT, riprova |
| Schermo nero, seriale ok, `[lcd] init FAILED` | PSRAM non impostata su **OPI PSRAM** |
| Schermo nero per ~20 s al primo avvio | Normale: formattazione LittleFS |
| Monitor seriale vuoto dopo l'avvio | "USB CDC On Boot" sbagliato per la porta usata, **oppure** problema EXIO5 (sotto) |
| La seriale USB funziona durante il flash ma sparisce dopo l'avvio | EXIO5 (selettore USB/CAN, **da verificare**): in `board_config.h` cambia `EXIO_DEFAULT` aggiungendo `\| EXIO_USB_SEL` e segnalalo |
| `[touch] GT911 not found` | Premi RESET; se persiste, invia il log completo di avvio |
| Il tocco risponde ma in un punto sbagliato | Annota le coordinate agli angoli nel bring-up e segnalale (possibile asse invertito) |
| Immagine stirata/spostata per un attimo ~2 s dopo la chiusura del menu | Noto: è il momento in cui la config viene scritta su flash (vedi BUILD.md → Persistence). Segnalalo solo se succede in altri momenti |
| Immagine che scorre/trema (drift) in altri momenti | Annota modo, FPS e quando succede; prova l'altro `DISPLAY_MODE` |
| Colori sbagliati (rosso/blu invertiti) | Segnalalo con una foto delle barre colore |
| Riportare la scheda allo stato iniziale (cancella config e preset) | Vedi sotto |

Per cancellare completamente la flash:

```bash
~/.arduino15/packages/esp32/tools/esptool_py/5.3.1/esptool --chip esp32s3 -p /dev/ttyACM0 erase-flash
```

Poi ricarica lo sketch.

---

## 8. Cosa inviarmi dopo il test

- Le righe `[bench]` e `[perf]` dei modi A e B, oppure la tabella `docs/benchmark.md` compilata.
- Se c'è stato drift e in quale modo.
- La porta usata (`ttyACM0` o `ttyUSB0`) e se la seriale resta attiva dopo l'avvio (per EXIO5).
- Le coordinate touch lette sui 4 angoli.
- Eventuali foto dello schermo se qualcosa è diverso dalle attese.
