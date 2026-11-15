// Application glue shared by the device firmware and the simulator:
// config ownership, gestures -> UI / scene, fixed-step simulation, overlays,
// screen-off timer. Portable.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../config/config.h"
#include "../engine/canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

bool app_init(void);
// One frame: poll input, update with a fixed timestep, render into cv (400x240).
void app_frame(Canvas *cv);
// Frame period requested by the config (ms).
uint32_t app_frame_period_ms(void);
// Statistics reported by the platform (shown in the overlay).
void app_report_stats(float fps, float frame_ms);
// Simulator keyboard shortcuts: '1'..'4' theme, 'f' overlay, 'm' menu.
void app_key(char key);

// ---- used by the UI ----
AquariumConfig *app_cfg(void);
// The config was edited in place: refresh the scene (SC_* flags) and schedule a save.
void app_config_changed(unsigned flags);
// Replace the whole config (theme/preset) with a fade.
void app_load_config(const AquariumConfig *c);
void app_set_theme(int theme);

#ifdef __cplusplus
}
#endif
