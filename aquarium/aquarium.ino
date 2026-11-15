// Acquario Digitale — Waveshare ESP32-S3-Touch-LCD-4.3
//
// Arduino IDE settings (Tools menu), see BUILD.md:
//   Board: "ESP32S3 Dev Module"   PSRAM: "OPI PSRAM"   Flash Size: "16MB (128Mb)"
//   Partition Scheme: "Custom" (uses partitions.csv in this folder)
//   USB CDC On Boot: "Enabled" (native USB port) / "Disabled" (UART port)
//
// Code layout:
//   src/engine  portable 2D engine (canvas, sprites, font, PRNG, gestures)
//   src/scene   portable aquarium scene (water, substrate, plants, creatures...)
//   src/config  AquariumConfig + JSON + debounced storage
//   src/ui      pixel-art settings menu and edit mode
//   src/app     glue shared with the desktop simulator (sim/)
//   src/board   ESP32-only HAL: CH422G, GT911, RGB panel, tasks
#include "src/board/board_config.h"
#include "src/board/hal_esp32.h"
extern "C" {
#include "src/app/app.h"
}

void bringup_render_task(void *);

void setup()
{
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== Acquario Digitale ===");
    if (!board_init()) {
        Serial.println("board_init failed, halting");
        return;
    }
#if BRINGUP_TEST
    Serial.println("BRINGUP_TEST mode");
    board_start_tasks(bringup_render_task);
#else
    if (!app_init()) {
        Serial.println("app_init failed (out of memory?)");
        return;
    }
    board_start_tasks(NULL);
#endif
}

void loop()
{
    // All work happens in FreeRTOS tasks; the Arduino loop task just sleeps.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
