#include "sprite.h"
#include <string.h>

void sprite_draw(Canvas *c, const SpriteSheet *s, int frame, int x, int y, const uint16_t pal[16], int flags,
                 uint32_t bright)
{
    uint16_t p[16];
    for (int i = 0; i < 16; i++) p[i] = bright >= 256 ? pal[i] : c565_scale(pal[i], bright);
    const int flip = flags & SPRITE_FLIP_X;
    for (int sy = 0; sy < s->h; sy++) {
        int dy = y + sy;
        if (dy < c->cy0 || dy >= c->cy1) continue;
        uint16_t *drow = c->px + dy * c->w;
        for (int sx = 0; sx < s->w; sx++) {
            int dx = x + (flip ? s->w - 1 - sx : sx);
            if (dx < c->cx0 || dx >= c->cx1) continue;
            uint8_t idx = sprite_get(s, frame, sx, sy);
            if (idx) drow[dx] = p[idx];
        }
    }
}

static uint8_t hexval(char ch)
{
    if (ch >= '1' && ch <= '9') return (uint8_t)(ch - '0');
    if (ch >= 'a' && ch <= 'f') return (uint8_t)(ch - 'a' + 10);
    return 0;
}

void sprite_pack_ascii(uint8_t *out, int w, int h, int frames, const char *const *rows)
{
    const int stride = SPRITE_STRIDE(w);
    memset(out, 0, (size_t)SPRITE_BYTES(w, h, frames));
    for (int r = 0; r < h * frames; r++) {
        const char *src = rows[r];
        size_t len = strlen(src);
        for (int x = 0; x < w && (size_t)x < len; x++) {
            uint8_t v = hexval(src[x]);
            if (x & 1) out[r * stride + (x >> 1)] |= v;
            else out[r * stride + (x >> 1)] |= (uint8_t)(v << 4);
        }
    }
}
