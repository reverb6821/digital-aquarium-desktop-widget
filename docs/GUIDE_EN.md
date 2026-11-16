# Guide: flashing and testing the Digital Aquarium

Board: **Waveshare ESP32-S3-Touch-LCD-4.3** · PC: **Nobara/Fedora Linux** · IDE: **Arduino IDE 2**

> Versione italiana: [GUIDA_IT.md](GUIDA_IT.md)

---

## 1. One-time setup

### 1.1 Serial port permissions

```bash
sudo usermod -aG dialout $USER
```

Then **log out and back in** (or reboot). Check with `groups`: `dialout` must be listed.

### 1.2 ESP32 core in Arduino IDE

In **Tools → Board → Boards Manager**, search for `esp32` (by Espressif Systems) and install
version **3.1 or newer**. Version 3.3.12 is already installed on your PC. No other libraries are needed.

### 1.3 Connect the board

Use a USB-C **data** cable (not a charge-only one). Check which port appears:

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

- `/dev/ttyACM0` → native ESP32-S3 USB → use **USB CDC On Boot: Enabled** later
- `/dev/ttyUSB0` → USB-UART bridge → use **USB CDC On Boot: Disabled**

If no port appears, try another cable. You can also start the board in download mode: hold
**BOOT**, press and release **RESET**, then release **BOOT**.

---

## 2. Flashing with Arduino IDE

1. **File → Open** → `digital_aquarium_esp/aquarium/aquarium.ino`
   (the folder must be named `aquarium`, like the `.ino` file).
2. Set the **Tools** menu:

   | Option | Value |
   |---|---|
   | Board | **ESP32S3 Dev Module** |
   | Port | `/dev/ttyACM0` (or `/dev/ttyUSB0`) |
   | PSRAM | **OPI PSRAM** ← required |
   | Flash Size | **16MB (128Mb)** |
   | Partition Scheme | **Custom** (uses `aquarium/partitions.csv`) |
   | USB CDC On Boot | **Enabled** (ttyACM) / **Disabled** (ttyUSB) |
   | CPU Frequency | 240MHz (WiFi) |
   | Upload Speed | 921600 (if it fails: 460800) |

3. Click **Upload** (→ arrow). The first build takes about 30–60 s.
4. Open **Tools → Serial Monitor** at **115200 baud**.

> If you change "USB CDC On Boot" compared to the previous flash, also set
> **Tools → Erase All Flash Before Sketch Upload: Enabled** the first time, then set it back to Disabled.
> Warning: this also erases the saved configuration and presets.

### First boot

On first boot the board **formats the LittleFS partition** (~12 MB): the screen can stay black
for **10–30 seconds**. This happens only once.

After updating from firmware older than the freshwater-biomes version, the saved tank and
presets are not migrated: the aquarium starts again from the Amazzonico biome.

The serial monitor should show something like:

```
=== Acquario Digitale ===
[board] PSRAM 8388608 bytes, internal free ...
[touch] GT911 id '911 ' at 0x5D, resolution 800x480
[lcd] mode B (bounce-buffer upscale), bounce 20 lines
config: defaults
[perf] mode B (bounce-buffer upscale)  fps=30.0  render=..ms  present-wait=..ms  heap int=... psram=...
```

---

## 3. Flashing from the terminal (alternative, arduino-cli)

Install `arduino-cli` (it reuses the cores and toolchains already downloaded by the IDE):

```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
```

Then, from the project folder:

```bash
cd ~/Development/digital_aquarium_esp
FQBN="esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,PartitionScheme=custom,CDCOnBoot=cdc"
arduino-cli compile --fqbn "$FQBN" --warnings all aquarium
arduino-cli upload  --fqbn "$FQBN" -p /dev/ttyACM0 aquarium
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200
```

With the UART port (`/dev/ttyUSB0`), replace `CDCOnBoot=cdc` with `CDCOnBoot=default`.

---

## 4. Hardware test (Phase 1: bring-up)

The goal is to check the display, touch and stability **before** running the aquarium.

1. Open `aquarium/src/board/board_config.h` and set:
   ```c
   #define BRINGUP_TEST 1
   ```
2. Upload the sketch. A sequence of test screens appears. **Tap the screen** to go to the next
   one; without taps it changes by itself every 8 s.

| Screen | What to check | Expected result |
|---|---|---|
| **Colour bars** | Order from the left: white, yellow, cyan, green, magenta, red, blue, black | Correct colours in the correct order (if red and blue are swapped → report it) |
| **Gradient** | Top: dithered gradient. Bottom: gradient without dithering | Top smoother, bottom with visible bands |
| **Touch** | Orange square moving, white circles in the 4 corners | When you touch, a cyan circle follows your finger; the coordinates shown range from about 0,0 to 799,479 |

3. **Drift test**: let it run for 2–3 minutes. In this mode a stress task keeps running Wi-Fi
   scans and flash (NVS) writes. The image must **never scroll or shift**. Mode and FPS are
   shown at the bottom left.
4. **Corner touch**: touch the 4 corner circles. The cyan circle must land on them (at most a
   few pixels off). Also try 2 fingers: `P0` and `P1` appear.

### Benchmark: mode A vs mode B

Run the test twice and record the values in `docs/benchmark.md`:

| Run | Setting in `board_config.h` |
|---|---|
| Mode B (default) | `#define DISPLAY_MODE DISPLAY_MODE_B` |
| Mode A | `#define DISPLAY_MODE DISPLAY_MODE_A` |

From the serial monitor, copy the `[bench] mode ... fps=... draw=...ms` lines and note whether there is any drift.

When done, **set back** `BRINGUP_TEST 0` and `DISPLAY_MODE_B` (unless mode A turns out better).

---

## 5. Aquarium test (Phases 2–5)

With `BRINGUP_TEST 0`, upload the sketch and go through the checklist:

| # | Action | Expected result |
|---|---|---|
| 1 | Look at the scene after boot | "Amazzonico" (Amazon) biome: light rays, bubbles, swaying Echinodorus and vallisneria, driftwood, neon school, ember tetras, angelfish, corydoras |
| 2 | **Double tap** | Top-left overlay with time and FPS (≥ 25–30 fps) |
| 3 | **Tap on the water**, away from fish | Food pellets sink; nearby fish chase and eat them |
| 4 | **Tap next to a fish** | The fish flees; the school scatters and then regroups |
| 5 | **Horizontal swipe with 2 fingers** | Fade and biome change (Blackwater → South-East Asia → Africa → Malawi → Pond → ...) |
| 5b | South-East Asia biome (key/swipe) | Shrimp walk and pick at the bottom and tail-flip away when you tap near them; the assassin snails crawl slowly and pull into the shell when touched. Pond biome: Planorbarius snails |
| 6 | **Hold for 1 s** | The menu opens on the right |
| 7 | Menu → **Bioma** (Biome) | 6 previews; tap one to switch biome |
| 8 | Menu → **Acqua / Fondale** (Water / Substrate) | Changes (colours, rays, height, "Rigenera" = regenerate) show immediately |
| 9 | Menu → **Creature** | `+`/`−` adds or removes fish; "Colore" changes the colour variant; total limit 60 |
| 10 | Menu → **Piante e decor → Modalita' modifica** (edit mode) | Tap and drag a plant or decoration sideways and up/down (it snaps back onto the bottom when close to it); try Dietro/Davanti (back/front), Colore (tap the swatch), **-15/+15** (rotate), **- / +** (size, 50-200%), **Fondo** (back on the bottom, upright), Elimina (delete), then **Fatto** (done) |
| 11 | Menu → **Ambiente** (Environment) | Day/night, speed, brightness, 20/30 FPS, screen-off timer |
| 12 | Menu → **Preset → Salva** (save), change biome, then **Carica** (load) | The saved scene comes back |
| 13 | Make a change, **close the menu**, wait **3 s**, press **RESET** on the board | The aquarium restarts exactly as you left it (nothing is saved while the menu is open) |
| 14 | Ambiente → "Spegni dopo 5 min" (screen off after 5 min), wait | The backlight turns off; the first touch turns it back on (that touch does not drop food) |
| 15 | Creature → raise the total to 60 | Overlay FPS stays ≥ 25 |
| 16 | Menu open | The aquarium keeps animating behind it at ≥ 15 fps |

Every 5 s, the `[perf]` lines in the serial monitor report FPS, render time and free memory.

---

## 6. PC simulator (no board needed)

```bash
sudo dnf install SDL2-devel cmake ninja-build gcc
cd ~/Development/digital_aquarium_esp
cmake -S sim -B build-sim -G Ninja && cmake --build build-sim
./build-sim/aquarium_sim
```

| Input | Action |
|---|---|
| Click | tap |
| Hold the left button for 1 s | menu |
| Double click | FPS overlay |
| Drag with the right button | 2-finger swipe (biome change) |
| `1`–`6` | biome · `F` overlay · `M` menu · `S` PNG screenshot · `Esc` quit |

Automatic test without a window (no SDL needed):

```bash
./build-sim/aquarium_headless /tmp/aquarium_test
```

It must end with `ALL OK` and save screenshots in `/tmp/aquarium_test/`.

---

## 7. Troubleshooting

| Symptom | Fix |
|---|---|
| `Permission denied: /dev/ttyACM0` | You are not in the `dialout` group, or you didn't log in again (§1.1) |
| No serial port | Charge-only cable; try download mode (BOOT + RESET) |
| Upload stuck at `Connecting...` | Hold BOOT, press RESET, release BOOT, try again |
| Black screen, serial OK, `[lcd] init FAILED` | PSRAM is not set to **OPI PSRAM** |
| Black screen for ~20 s on first boot | Normal: LittleFS is being formatted |
| Serial monitor empty after boot | Wrong "USB CDC On Boot" for the port in use, **or** the EXIO5 issue (below) |
| USB serial works while flashing but disappears after boot | EXIO5 (USB/CAN selector, **not yet verified**): in `board_config.h`, add `\| EXIO_USB_SEL` to `EXIO_DEFAULT` and report it |
| `[touch] GT911 not found` | Press RESET; if it persists, send the full boot log |
| Touch responds but at the wrong position | Note the corner coordinates from the bring-up test and report them (an axis may be flipped) |
| Image stretched/shifted for a moment about 2 s after closing the menu | Known: that is the moment the config is written to flash (see BUILD.md → Persistence). Report it only if it happens at other times |
| Image scrolls/jitters (drift) at other times | Note the mode, FPS and when it happens; try the other `DISPLAY_MODE` |
| Wrong colours (red/blue swapped) | Report it with a photo of the colour bars |
| Reset the board to its initial state (erases config and presets) | See below |

To erase the whole flash:

```bash
~/.arduino15/packages/esp32/tools/esptool_py/5.3.1/esptool --chip esp32s3 -p /dev/ttyACM0 erase-flash
```

Then upload the sketch again.

---

## 8. What to send back after testing

- The `[bench]` and `[perf]` lines for modes A and B, or the filled-in `docs/benchmark.md` table.
- Whether any drift happened, and in which mode.
- The port used (`ttyACM0` or `ttyUSB0`) and whether the serial port stays active after boot (for EXIO5).
- The touch coordinates read at the 4 corners.
- Photos of the screen if anything looks different from what is expected.
