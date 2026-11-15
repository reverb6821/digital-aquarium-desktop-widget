// Tiny 5x8 bitmap font for the pixel UI and debug overlay.
#pragma once
#include "canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FONT_ADVANCE 6
#define FONT_HEIGHT 8

int font_draw_char(Canvas *c, int x, int y, char ch, uint16_t col);
int font_draw(Canvas *c, int x, int y, const char *s, uint16_t col);
int font_draw_centered(Canvas *c, int cx, int y, const char *s, uint16_t col);
int font_width(const char *s);

#ifdef __cplusplus
}
#endif
