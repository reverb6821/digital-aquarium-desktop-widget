// Minimal hardware abstraction used by the portable code (engine/, scene/, ui/,
// config/). Implemented by the board layer on the ESP32 and by sim/hal_sim.c.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Logical canvas size: the world is rendered at 400x240 and upscaled 2x.
#define CANVAS_W 400
#define CANVAS_H 240
#define SCREEN_SCALE 2

typedef struct {
    uint8_t n;          // number of active points (0..2)
    int16_t x[2], y[2]; // in SCREEN pixels (800x480)
} HalTouch;

uint32_t hal_millis(void);
// Local time of day in minutes (0..1439), or -1 if unknown (no NTP/RTC).
int hal_time_of_day_min(void);
// Latest touch sample (non-blocking).
void hal_touch_read(HalTouch *t);
// Turn the backlight on/off (EXIO2 on the Waveshare board).
void hal_backlight(bool on);
// Allocate a large buffer (PSRAM on device). Never called from the render loop.
void *hal_alloc_large(size_t bytes);
void hal_log(const char *fmt, ...);
// Global lock protecting data shared between the render task and the storage task.
void hal_lock(void);
void hal_unlock(void);

// ---- persistent storage ----
// Small binary blob (NVS on device, file in the simulator).
bool hal_blob_save(const char *key, const void *data, size_t len);
// Returns the blob size (0 if missing or larger than cap).
size_t hal_blob_load(const char *key, void *data, size_t cap);
// Text files (LittleFS on device, ./sim_data/ in the simulator).
bool hal_file_write(const char *path, const char *data, size_t len);
// Returns bytes read (0 on error); buffer is NUL-terminated.
size_t hal_file_read(const char *path, char *buf, size_t cap);
bool hal_file_remove(const char *path);

#ifdef __cplusplus
}
#endif
