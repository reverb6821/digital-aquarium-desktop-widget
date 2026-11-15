// RGB565 software canvas: clipped primitives used by the whole renderer.
// Portable C11, no platform dependencies.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "color.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t *px;   // row-major pixels, stride == w
    int w, h;
    // Clip rectangle, inclusive-exclusive [cx0, cx1) x [cy0, cy1)
    int cx0, cy0, cx1, cy1;
} Canvas;

void canvas_init(Canvas *c, uint16_t *px, int w, int h);
void canvas_reset_clip(Canvas *c);
void canvas_set_clip(Canvas *c, int x, int y, int w, int h);

static inline bool canvas_in(const Canvas *c, int x, int y)
{
    return x >= c->cx0 && y >= c->cy0 && x < c->cx1 && y < c->cy1;
}

static inline void canvas_pset(Canvas *c, int x, int y, uint16_t col)
{
    if (canvas_in(c, x, y)) c->px[y * c->w + x] = col;
}

static inline void canvas_pblend(Canvas *c, int x, int y, uint16_t col, uint8_t a)
{
    if (canvas_in(c, x, y)) {
        uint16_t *p = &c->px[y * c->w + x];
        *p = c565_blend(*p, col, a);
    }
}

static inline void canvas_padd(Canvas *c, int x, int y, uint16_t col)
{
    if (canvas_in(c, x, y)) {
        uint16_t *p = &c->px[y * c->w + x];
        *p = c565_add(*p, col);
    }
}

void canvas_fill(Canvas *c, uint16_t col);
void canvas_fill_rect(Canvas *c, int x, int y, int w, int h, uint16_t col);
void canvas_blend_rect(Canvas *c, int x, int y, int w, int h, uint16_t col, uint8_t a);
void canvas_rect(Canvas *c, int x, int y, int w, int h, uint16_t col);
void canvas_hline(Canvas *c, int x0, int x1, int y, uint16_t col);
void canvas_vline(Canvas *c, int x, int y0, int y1, uint16_t col);
void canvas_line(Canvas *c, int x0, int y0, int x1, int y1, uint16_t col);
void canvas_line_blend(Canvas *c, int x0, int y0, int x1, int y1, uint16_t col, uint8_t a);
void canvas_circle(Canvas *c, int cx, int cy, int r, uint16_t col);
void canvas_circle_blend(Canvas *c, int cx, int cy, int r, uint16_t col, uint8_t a);
void canvas_fill_circle(Canvas *c, int cx, int cy, int r, uint16_t col);
void canvas_fill_ellipse(Canvas *c, int cx, int cy, int rx, int ry, uint16_t col);
void canvas_fill_ellipse_blend(Canvas *c, int cx, int cy, int rx, int ry, uint16_t col, uint8_t a);
// Soft additive glow dot (radius 0..3).
void canvas_glow(Canvas *c, int x, int y, int r, uint16_t col);

// Copy a full canvas (same size) — memcpy fast path.
void canvas_copy(Canvas *dst, const Canvas *src);
// Copy src onto dst skipping pixels equal to key, starting from row y0.
void canvas_blit_keyed(Canvas *dst, const Canvas *src, uint16_t key, int y0);
// Multiply every pixel by s (0..256) — used for night filter and fades.
void canvas_darken(Canvas *c, uint32_t s);
// Downsample src by integer factor f into dst (box filter on 565).
void canvas_downsample(Canvas *dst, const Canvas *src, int f);
// 2x nearest-neighbour upscale of src (w x h) into dst (2w x 2h).
void canvas_upscale2x(uint16_t *dst, const uint16_t *src, int w, int h);

#ifdef __cplusplus
}
#endif
