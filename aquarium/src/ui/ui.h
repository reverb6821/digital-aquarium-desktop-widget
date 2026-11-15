// Pixel-art settings menu + edit mode, drawn on the 400x240 canvas.
// Immediate-mode widgets; portable (runs identically in the simulator).
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../engine/canvas.h"
#include "../engine/input.h"

void ui_init(void);
void ui_open(void);
void ui_close(void);
bool ui_is_open(void);
bool ui_is_editing(void);
// Returns true if the event was consumed by the UI.
bool ui_gesture(const GestureEvent *e);
// Process interaction and draw (call once per frame after the scene).
void ui_draw(Canvas *cv, uint32_t now);
