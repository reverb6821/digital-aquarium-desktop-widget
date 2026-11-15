# Digital Aquarium — build, flash and simulator

Firmware for the **Waveshare ESP32-S3-Touch-LCD-4.3** + Linux desktop simulator.
Overview: [README.md](README.md). Original specification (Italian): [docs/SPEC_IT.md](docs/SPEC_IT.md).
Step-by-step flashing and test guides: [docs/GUIDE_EN.md](docs/GUIDE_EN.md) · [docs/GUIDA_IT.md](docs/GUIDA_IT.md)

## Layout

```
aquarium/                 Arduino sketch (open aquarium/aquarium.ino)
├── aquarium.ino          setup(): board_init -> app_init -> FreeRTOS tasks
├── partitions.csv        nvs 24K · app 4 MB · LittleFS ~11.9 MB · coredump
└── src/
    ├── engine/   PORTABLE  RGB565 canvas, 4 bpp sprites + palettes, 5x8 font, PRNG/noise, trig, gestures, hal.h
    ├── scene/    PORTABLE  water, rays, particles, substrate, plants, decor, bubbles, food, creatures, biomes, scene
    ├── config/   PORTABLE  AquariumConfig, JSON (malloc-free tokenizer), deferred storage
    ├── ui/       PORTABLE  pixel-art settings menu + edit mode
    ├── app/      PORTABLE  glue shared by board and simulator (input -> UI/scene, fixed timestep, overlay, screen off)
    └── board/    ESP32 ONLY CH422G, GT911, RGB panel (modes A/B), HAL, tasks, bring-up/benchmark
sim/                      SDL2 simulator + headless test
assets/tools/png2c.py     indexed PNG (≤16 colours) -> C SpriteSheet array
docs/                     guides, benchmark.md, SPEC_IT.md, reference.png
```

`engine/ scene/ config/ ui/ app/` include nothing from ESP-IDF/Arduino. They reach the hardware
only through `src/engine/hal.h`, so the same code runs unchanged in the simulator.

### Scene content (where to edit)

| What | File |
|---|---|
| Biome presets (water, substrate, plants, decor, species per biome) | `src/scene/themes.c` |
| Species table, sprite generators (fish, shrimp, snails), behaviours | `src/scene/creatures.c` |
| Plant generators (stems, broad leaves, lily pads) | `src/scene/plants.c` |
| Rocks, wood, branches, roots, leaf litter | `src/scene/decor.c` |
| Plant/decor/species enums, config limits, `CONFIG_VERSION` | `src/config/config.h` |
| Menu labels and default colours for new items | `src/ui/ui.c` |

Behaviours: `SCHOOL` (boids), `WANDER`, `BOTTOM`, `SHRIMP` (walk, pick, tail-flip hop),
`SNAIL` (slow crawl, retracts when touched). Bumping `CONFIG_VERSION` discards saved configs
and presets from older versions, which is needed when plant/decor enums are renumbered.

## Firmware — Arduino IDE

You need the **esp32 by Espressif ≥ 3.1** core (tested: 3.3.12, based on ESP-IDF 5.5). No
external libraries are needed.

1. Add your user to the `dialout` group (once, then log out and in again):
   ```bash
   sudo usermod -aG dialout $USER
   ```
2. Open `aquarium/aquarium.ino` and set the **Tools** menu:
   | Option | Value |
   |---|---|
   | Board | ESP32S3 Dev Module |
   | PSRAM | **OPI PSRAM** |
   | Flash Size | **16MB (128Mb)** |
   | Partition Scheme | **Custom** (uses `aquarium/partitions.csv`) |
   | USB CDC On Boot | **Enabled** on the native USB port (`/dev/ttyACM0`), **Disabled** on the UART port (`/dev/ttyUSB0`) |
   | CPU Frequency | 240 MHz |
3. Upload. Open the serial monitor at 115200 baud. Every 5 s it prints `[perf] mode … fps=… render=…ms`.

If you change "USB CDC On Boot" compared to the previous flash, enable "Erase All Flash Before
Sketch Upload" the first time (Waveshare recommendation).

## Firmware — command line (arduino-cli)

`arduino-cli` is inside the IDE AppImage. You can also download it from
<https://arduino.github.io/arduino-cli/>.

```bash
FQBN="esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,PartitionScheme=custom,CDCOnBoot=cdc"
arduino-cli compile --fqbn "$FQBN" --warnings all aquarium
arduino-cli upload  --fqbn "$FQBN" -p /dev/ttyACM0 aquarium
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200
```

Build options: define them in `src/board/board_config.h`, or pass
`--build-property "compiler.c.extra_flags=-D…" --build-property "compiler.cpp.extra_flags=-D…"`.

| Define | Effect |
|---|---|
| `DISPLAY_MODE=2` (default) | Mode **B**: no frame buffer; 2× upscale in the bounce-buffer ISR |
| `DISPLAY_MODE=1` | Mode **A**: double 800×480 frame buffer in PSRAM |
| `BRINGUP_TEST=1` | Phase-1 test: colour bars, dithered/plain gradient, moving square, circle following the finger, corner targets, FPS log |
| `BRINGUP_STRESS=0` | (with BRINGUP_TEST) turns off the Wi-Fi scan + NVS write stress task |

## Linux simulator (SDL2)

```bash
sudo dnf install SDL2-devel cmake ninja-build gcc
cmake -S sim -B build-sim -G Ninja && cmake --build build-sim && ./build-sim/aquarium_sim
# without CMake:  make -C sim && ./build-sim/aquarium_sim
```

| Input | Action |
|---|---|
| click | tap (food / scare) |
| hold 1 s | settings menu |
| double click | time + FPS overlay |
| drag | drag (menu, edit mode) |
| drag with the right button | 2-finger swipe (next/previous biome) |
| `F` / `S` / `1-6` / `M` / `Esc` | FPS overlay / PNG screenshot / biome / menu / quit |

Data is stored in `./sim_data/`: the active config (`cfgj.bin`, JSON) and the presets (JSON).

**Headless test.** It needs nothing but gcc. It checks the menu, edit mode, persistence, JSON,
swipe, every biome hotkey and the creature limit, and it saves screenshots:

```bash
cmake --build build-sim --target aquarium_headless && ./build-sim/aquarium_headless /tmp
# or: make -C sim ../build-sim/aquarium_headless && ./build-sim/aquarium_headless /tmp
```

## Gestures on the board

| Gesture | Action |
|---|---|
| Tap on the water | food drops; nearby fish and shrimp chase and eat it |
| Tap near a fish | the fish flees (a school scatters, then regroups); shrimp tail-flip away; snails retract |
| Long press 1 s | opens the menu (Bioma, Acqua, Fondale, Piante e decor, Creature, Ambiente, Preset) |
| Double tap | time / FPS overlay |
| 2-finger horizontal swipe | next / previous biome |

Edit mode (menu → Piante e decor → "Modalita' modifica"): tap a plant or decoration and drag it.
The bottom bar has Fatto (done), Annulla (cancel), Dietro/Davanti (back/front), Colore (colour)
and Elimina (delete).

The on-screen UI labels are in Italian.

## Persistence

- The active config is stored as **one JSON record in NVS** (key `cfgj`). User presets are JSON
  files in LittleFS (`/presetN.json`).
- Writes are deferred and run in a low-priority task on core 0. While the menu or edit mode is
  open, **nothing is written**. Everything pending is written together 2 s after the menu
  closes. If the JSON did not change since the last save, nothing is written at all.
- The reason is the display (see below): every flash write causes a short glitch.

## Differences from the specification

- **Arduino instead of plain ESP-IDF.** This was the user's choice (Arduino IDE already
  installed). The esp32 3.3.12 core is ESP-IDF 5.5.5, precompiled. The code uses
  `esp_lcd_panel_rgb` directly (bounce buffers, `no_fb`, `on_bounce_empty`), FreeRTOS, NVS
  (`Preferences`) and LittleFS.
  - **The `sdkconfig` is fixed.** Already enabled: `SPIRAM_MODE_OCT`, `SPIRAM_SPEED_80M`,
    `LCD_RGB_RESTART_IN_VSYNC`, `FREERTOS_HZ=1000`.
  - **Cannot be changed:** 32 KB data cache with 32 B lines (spec: 64/64), and
    `SPIRAM_FETCH_INSTRUCTIONS/RODATA` (missing). This is why mode B is the default: it cuts
    PSRAM traffic by 4×.
  - **`LCD_RGB_ISR_IRAM_SAFE` is also missing.** During every flash write the cache is
    disabled, the bounce-buffer ISR does not run, and the screen shows stretched or shifted
    lines until the next VSYNC. Hence the write policy above: at most one short glitch, about
    2 s after the menu closes. Removing it completely needs ESP-IDF with
    `CONFIG_LCD_RGB_ISR_IRAM_SAFE=y` (or Arduino as an IDF component).
- **Own pixel-art UI instead of LVGL.** The widgets are immediate-mode and drawn on the 400×240
  canvas. They look like the rest of the aquarium, need no heavy dependency, and work the same
  in the simulator (tested automatically). Every option from §8/§9 of the spec is there; the
  Wi-Fi keyboard is not.
- **Folder layout**: `aquarium/src/{engine,scene,config,ui,app,board}` instead of
  `components/`, because the Arduino sketch only compiles code under `src/`.
- **No dirty rectangles.** The canvas is only 96 000 pixels. Redrawing everything from the
  caches is simpler and takes constant time (the spec allows this fallback).
- **Everything is procedural.** Fish, shrimp and snail sprites are generated at boot as 4 bpp
  indexed sprites with palette swap (4 frames each). Plants with broad leaves, rocks and wood
  are drawn procedurally. All the art is original. `png2c.py` is ready for hand-drawn PNG
  sprites.
- **Freshwater only.** The marine/abyss themes of the original spec were replaced by six
  freshwater biomes (Amazzonico, Acque nere, Sud-Est Asia, Africa Congo, Lago Malawi,
  Laghetto).
- **Phase 6**:
  - done: scheduled screen-off (EXIO2) with wake on touch, and the day/night cycle (real time
    if set, otherwise simulated: 1 real minute = 1 hour);
  - **not done**: Wi-Fi/NTP, web page, assets from microSD, OTA.
- The "Pokémon" theme in the reference screenshot was not implemented (protected characters).

## To verify on the hardware

1. **Bring-up** (`BRINGUP_TEST=1`):
   - the colour bars are correct (RGB order);
   - the gradient renders properly;
   - the image does not drift while the Wi-Fi + NVS stress task runs;
   - touch is accurate on the 4 corner circles.
2. **Benchmark**: run mode B, then mode A, and fill in the table in `docs/benchmark.md`. Check
   memory and FPS with the new procedural plants (larger static buffers: 12 KB sprite pool,
   ~32 KB plant nodes).
3. **EXIO5 (USB/CAN select)** [TO VERIFY]: the firmware copies the final state of the Waveshare
   demo (`0x1E`, EXIO5 low). If the USB serial port disappears after boot, change
   `EXIO_DEFAULT` in `board_config.h`.
4. **Reboot**: close the menu and wait about 3 s. After a reset, the aquarium must come back as
   you left it.
