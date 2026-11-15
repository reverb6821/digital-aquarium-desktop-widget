// Board bring-up and FreeRTOS task startup (ESP32 only).
#pragma once
#include <stdbool.h>

// I2C, CH422G, touch reset, NVS, LittleFS, RGB panel (ISR on core 0).
bool board_init(void);
// Start input (core 0), storage (core 0) and render (core 1) tasks.
// render_fn = NULL -> the aquarium render loop.
void board_start_tasks(void (*render_fn)(void *));
