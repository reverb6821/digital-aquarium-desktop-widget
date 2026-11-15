#include "canvas.h"
#include <stdlib.h>
#include <string.h>

void canvas_init(Canvas *c, uint16_t *px, int w, int h)
{
    c->px = px;
    c->w = w;
    c->h = h;
    canvas_reset_clip(c);
}

void canvas_reset_clip(Canvas *c)
{
    c->cx0 = 0;
    c->cy0 = 0;
    c->cx1 = c->w;
    c->cy1 = c->h;
}

void canvas_set_clip(Canvas *c, int x, int y, int w, int h)
{
    c->cx0 = x < 0 ? 0 : x;
    c->cy0 = y < 0 ? 0 : y;
    c->cx1 = x + w > c->w ? c->w : x + w;
    c->cy1 = y + h > c->h ? c->h : y + h;
}

void canvas_fill(Canvas *c, uint16_t col)
{
    canvas_fill_rect(c, 0, 0, c->w, c->h, col);
}

void canvas_fill_rect(Canvas *c, int x, int y, int w, int h, uint16_t col)
{
    int x0 = x < c->cx0 ? c->cx0 : x;
    int y0 = y < c->cy0 ? c->cy0 : y;
    int x1 = x + w > c->cx1 ? c->cx1 : x + w;
    int y1 = y + h > c->cy1 ? c->cy1 : y + h;
    for (int yy = y0; yy < y1; yy++) {
        uint16_t *p = &c->px[yy * c->w + x0];
        for (int xx = x0; xx < x1; xx++) *p++ = col;
    }
}

void canvas_blend_rect(Canvas *c, int x, int y, int w, int h, uint16_t col, uint8_t a)
{
    int x0 = x < c->cx0 ? c->cx0 : x;
    int y0 = y < c->cy0 ? c->cy0 : y;
    int x1 = x + w > c->cx1 ? c->cx1 : x + w;
    int y1 = y + h > c->cy1 ? c->cy1 : y + h;
    for (int yy = y0; yy < y1; yy++) {
        uint16_t *p = &c->px[yy * c->w + x0];
        for (int xx = x0; xx < x1; xx++, p++) *p = c565_blend(*p, col, a);
    }
}

void canvas_rect(Canvas *c, int x, int y, int w, int h, uint16_t col)
{
    canvas_hline(c, x, x + w - 1, y, col);
    canvas_hline(c, x, x + w - 1, y + h - 1, col);
    canvas_vline(c, x, y, y + h - 1, col);
    canvas_vline(c, x + w - 1, y, y + h - 1, col);
}

void canvas_hline(Canvas *c, int x0, int x1, int y, uint16_t col)
{
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    canvas_fill_rect(c, x0, y, x1 - x0 + 1, 1, col);
}

void canvas_vline(Canvas *c, int x, int y0, int y1, uint16_t col)
{
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    canvas_fill_rect(c, x, y0, 1, y1 - y0 + 1, col);
}

// Bresenham; `a == 255` means opaque.
static void line_impl(Canvas *c, int x0, int y0, int x1, int y1, uint16_t col, uint8_t a)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        if (a == 255) canvas_pset(c, x0, y0, col);
        else canvas_pblend(c, x0, y0, col, a);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void canvas_line(Canvas *c, int x0, int y0, int x1, int y1, uint16_t col)
{
    line_impl(c, x0, y0, x1, y1, col, 255);
}

void canvas_line_blend(Canvas *c, int x0, int y0, int x1, int y1, uint16_t col, uint8_t a)
{
    line_impl(c, x0, y0, x1, y1, col, a);
}

// Midpoint circle outline.
static void circle_impl(Canvas *c, int cx, int cy, int r, uint16_t col, uint8_t a)
{
    if (r <= 0) {
        if (a == 255) canvas_pset(c, cx, cy, col);
        else canvas_pblend(c, cx, cy, col, a);
        return;
    }
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        const int pts[8][2] = {
            {cx + x, cy + y}, {cx + y, cy + x}, {cx - y, cy + x}, {cx - x, cy + y},
            {cx - x, cy - y}, {cx - y, cy - x}, {cx + y, cy - x}, {cx + x, cy - y},
        };
        for (int i = 0; i < 8; i++) {
            // Avoid double-blending the same pixel on the diagonals/axes.
            if (a != 255 && i > 0 && pts[i][0] == pts[i - 1][0] && pts[i][1] == pts[i - 1][1]) continue;
            if (a == 255) canvas_pset(c, pts[i][0], pts[i][1], col);
            else canvas_pblend(c, pts[i][0], pts[i][1], col, a);
        }
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void canvas_circle(Canvas *c, int cx, int cy, int r, uint16_t col) { circle_impl(c, cx, cy, r, col, 255); }
void canvas_circle_blend(Canvas *c, int cx, int cy, int r, uint16_t col, uint8_t a) { circle_impl(c, cx, cy, r, col, a); }

void canvas_fill_circle(Canvas *c, int cx, int cy, int r, uint16_t col)
{
    canvas_fill_ellipse(c, cx, cy, r, r, col);
}

static void ellipse_impl(Canvas *c, int cx, int cy, int rx, int ry, uint16_t col, uint8_t a)
{
    if (rx <= 0 || ry <= 0) return;
    long rx2 = (long)rx * rx, ry2 = (long)ry * ry;
    for (int y = -ry; y <= ry; y++) {
        // half-width at this row: rx * sqrt(1 - y^2/ry^2), integer search
        long lim = rx2 * ry2 - (long)y * y * rx2;
        int hw = 0;
        while ((long)(hw + 1) * (hw + 1) * ry2 <= lim) hw++;
        if (a == 255) canvas_fill_rect(c, cx - hw, cy + y, 2 * hw + 1, 1, col);
        else canvas_blend_rect(c, cx - hw, cy + y, 2 * hw + 1, 1, col, a);
    }
}

void canvas_fill_ellipse(Canvas *c, int cx, int cy, int rx, int ry, uint16_t col)
{
    ellipse_impl(c, cx, cy, rx, ry, col, 255);
}

void canvas_fill_ellipse_blend(Canvas *c, int cx, int cy, int rx, int ry, uint16_t col, uint8_t a)
{
    ellipse_impl(c, cx, cy, rx, ry, col, a);
}

void canvas_glow(Canvas *c, int x, int y, int r, uint16_t col)
{
    canvas_padd(c, x, y, col);
    if (r <= 0) return;
    uint16_t half = c565_scale(col, 110);
    canvas_padd(c, x - 1, y, half);
    canvas_padd(c, x + 1, y, half);
    canvas_padd(c, x, y - 1, half);
    canvas_padd(c, x, y + 1, half);
    if (r <= 1) return;
    uint16_t q = c565_scale(col, 48);
    canvas_padd(c, x - 1, y - 1, q);
    canvas_padd(c, x + 1, y - 1, q);
    canvas_padd(c, x - 1, y + 1, q);
    canvas_padd(c, x + 1, y + 1, q);
    canvas_padd(c, x - 2, y, q);
    canvas_padd(c, x + 2, y, q);
    canvas_padd(c, x, y - 2, q);
    canvas_padd(c, x, y + 2, q);
}

void canvas_copy(Canvas *dst, const Canvas *src)
{
    memcpy(dst->px, src->px, (size_t)src->w * (size_t)src->h * sizeof(uint16_t));
}

void canvas_blit_keyed(Canvas *dst, const Canvas *src, uint16_t key, int y0)
{
    if (y0 < 0) y0 = 0;
    const int n = src->w * src->h;
    const uint16_t *s = src->px + y0 * src->w;
    uint16_t *d = dst->px + y0 * dst->w;
    for (int i = y0 * src->w; i < n; i++, s++, d++) {
        if (*s != key) *d = *s;
    }
}

void canvas_darken(Canvas *c, uint32_t s)
{
    if (s >= 256) return;
    const int n = c->w * c->h;
    uint16_t *p = c->px;
    for (int i = 0; i < n; i++, p++) *p = c565_scale(*p, s);
}

void canvas_downsample(Canvas *dst, const Canvas *src, int f)
{
    const int n = f * f;
    for (int y = 0; y < dst->h; y++) {
        for (int x = 0; x < dst->w; x++) {
            uint32_t r = 0, g = 0, b = 0;
            for (int j = 0; j < f; j++) {
                const uint16_t *p = &src->px[(y * f + j) * src->w + x * f];
                for (int i = 0; i < f; i++) {
                    r += p[i] >> 11;
                    g += (p[i] >> 5) & 63;
                    b += p[i] & 31;
                }
            }
            dst->px[y * dst->w + x] = (uint16_t)(((r / n) << 11) | ((g / n) << 5) | (b / n));
        }
    }
}

void canvas_upscale2x(uint16_t *dst, const uint16_t *src, int w, int h)
{
    const int dw = w * 2;
    for (int y = 0; y < h; y++) {
        const uint16_t *s = src + y * w;
        uint32_t *d = (uint32_t *)(dst + (2 * y) * dw);
        for (int x = 0; x < w; x++) {
            uint32_t p = s[x];
            d[x] = p | (p << 16);
        }
        memcpy(dst + (2 * y + 1) * dw, dst + (2 * y) * dw, (size_t)dw * sizeof(uint16_t));
    }
}
