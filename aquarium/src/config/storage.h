// Persistence: active config (JSON in one NVS record) + user presets (JSON files).
// Writes are deferred and executed by storage_poll(), which on the device runs
// in a low-priority task on core 0 so the render loop never blocks on flash.
//
// Every flash write disables the CPU cache; with the Arduino core the RGB LCD
// bounce-buffer ISR is not IRAM-safe, so the picture glitches (stretched /
// shifted lines) until the next VSYNC. Hence: no writes while the menu or the
// edit mode is open (storage_hold), no write if nothing changed, one record.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PRESET_NAME_LEN 20

void storage_init(void);
// Loads the last active config. Returns false if nothing valid is stored.
bool storage_load_active(AquariumConfig *c);
// Schedule a save of the active config (debounced).
void storage_mark_dirty(const AquariumConfig *c, uint32_t now);
// Schedule a user preset save into slot 0..CFG_MAX_USER_PRESETS-1.
void storage_request_preset_save(int slot, const AquariumConfig *c, const char *name);
void storage_request_preset_delete(int slot);
// Synchronous preset load. Returns false if the slot is empty/invalid.
bool storage_preset_load(int slot, AquariumConfig *c);
// Cached preset name ("" if empty slot).
const char *storage_preset_name(int slot);
// While held (menu / edit mode open) nothing is written; on release the
// pending writes run after the debounce. Call every frame.
void storage_hold(bool hold, uint32_t now);
// Execute pending writes whose debounce expired. Call periodically.
void storage_poll(uint32_t now);
// Flush everything immediately (e.g. before deep sleep).
void storage_flush(void);

#ifdef __cplusplus
}
#endif
