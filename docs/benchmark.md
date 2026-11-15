# Rendering benchmark (spec §4)

## Method

1. Build with `BRINGUP_TEST=1`. The Wi-Fi + NVS write stress task is on by default. Try
   `DISPLAY_MODE=2` (B), then `DISPLAY_MODE=1` (A). Watch the screen for 2–3 minutes: any
   horizontal scrolling or shifting of the image counts as **drift**.
2. Build the normal aquarium (`BRINGUP_TEST=0`) in both modes. From the serial monitor, write
   down the `[perf]` lines (5 s averages) in two cases:
   - the Amazzonico biome;
   - the stress test (menu → Creature → raise the total to 60).

Panel refresh with the demo timing: 16 MHz / (820 × 500) ≈ **39 Hz**.
PSRAM traffic of the scan-out alone: mode A ≈ 768 KB × 39 = **30 MB/s**; mode B ≈ 192 KB × 39 = **7.5 MB/s**.

Known artefact: a short glitch (stretched/shifted lines) whenever the flash is written. In normal
use this happens only about 2 s after the menu closes, and only if the config changed. See
BUILD.md → Persistence. Do not count it as drift.

## Results on the board (to fill in)

| Mode | Scene | FPS | render ms | present-wait ms | Drift with Wi-Fi + flash | Notes |
|---|---|---|---|---|---|---|
| B | bring-up |  |  |  |  |  |
| B | Amazzonico (30 creatures) |  |  |  |  |  |
| B | stress, 60 creatures |  |  |  |  |  |
| B | menu open |  |  |  |  |  |
| A | bring-up |  |  |  |  |  |
| A | Amazzonico (30 creatures) |  |  |  |  |  |
| A | stress, 60 creatures |  |  |  |  |  |

Final choice: **mode B** is the default. It has less PSRAM traffic, no 768 KB frame buffer,
and the canvas swap is synchronised to the start of the refresh in the ISR. To be confirmed
with the numbers above.

## Simulator reference (x86-64 host, -O2)

`aquarium_headless`: average `app_frame` ≈ 0.17 ms (scene + UI, 30–60 creatures). This says
nothing about timing on the ESP32-S3; use it only to compare versions for regressions.
