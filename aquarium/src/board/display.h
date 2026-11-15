// RGB panel driver with two strategies (see board_config.h DISPLAY_MODE).
#pragma once
#include <stdbool.h>
#include <stdint.h>

bool display_init(void);
// Canvas buffer (400x240 RGB565) the next frame must be rendered into.
uint16_t *display_begin_frame(void);
// Hand the rendered canvas to the panel. Blocks until it is safe to render the
// next frame (the new frame has been latched by the scan-out).
void display_present(uint16_t *canvas);
const char *display_mode_name(void);
