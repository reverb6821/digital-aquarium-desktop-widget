// Indexed 4-bit sprites with palette swap. Index 0 is always transparent.
#pragma once
#include <stdint.h>
#include "canvas.h"

typedef struct {
    uint8_t w, h, frames;
    const uint8_t *data; // packed 4bpp, high nibble = left pixel; frames stacked vertically
} SpriteSheet;

#define SPRITE_STRIDE(w) (((w) + 1) / 2)
#define SPRITE_BYTES(w, h, frames) (SPRITE_STRIDE(w) * (h) * (frames))

enum { SPRITE_FLIP_X = 1 };

// Draws `frame` at (x,y) top-left. `bright` 0..256 scales the palette (depth fade).
void sprite_draw(Canvas *c, const SpriteSheet *s, int frame, int x, int y, const uint16_t pal[16], int flags,
                 uint32_t bright);

// Pack an ASCII-art description into `out` (size SPRITE_BYTES). Each row string
// has `w` chars: '.' or ' ' = 0, '1'..'9' = 1..9, 'a'..'f' = 10..15.
// `rows` holds h*frames strings (frame 0 rows first).
void sprite_pack_ascii(uint8_t *out, int w, int h, int frames, const char *const *rows);

static inline uint8_t sprite_get(const SpriteSheet *s, int frame, int x, int y)
{
    const uint8_t *row = s->data + (frame * s->h + y) * SPRITE_STRIDE(s->w);
    uint8_t b = row[x >> 1];
    return (x & 1) ? (b & 15) : (b >> 4);
}
