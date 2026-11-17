// Scenic backdrops: distant biotope silhouettes painted into the background
// cache right after the water gradient (static, no per-frame cost). Everything
// is alpha-blended onto the gradient, so it picks up the water colour and the
// dithering; far layers are fainter than near ones to suggest depth.
#include <math.h>
#include "../engine/fixmath.h"
#include "../engine/rng.h"
#include "world.h"

// Silhouette colour: base tone pulled towards the deep water colour.
static uint16_t tint(const AquariumConfig *c, uint16_t base, uint8_t k) { return c565_lerp(base, c->water_bottom, k); }

// Vertical-ish thick stroke from (x0,y0) to (x1,y1): one blended span per row,
// width tapering from w0 to w1, optional sideways wobble.
static void stroke_v(Canvas *bg, float x0, int y0, float x1, int y1, float w0, float w1, uint16_t col, int a, float wob,
                     uint32_t seed)
{
    if (y1 < y0) {
        int ty = y0; y0 = y1; y1 = ty;
        float tx = x0; x0 = x1; x1 = tx;
        float tw = w0; w0 = w1; w1 = tw;
    }
    const int n = y1 - y0 > 0 ? y1 - y0 : 1;
    for (int y = y0; y <= y1; y++) {
        const float t = (float)(y - y0) / (float)n;
        const float xc = x0 + (x1 - x0) * t + (wob > 0 ? (noise1d(seed, (float)y * 0.05f) - 0.5f) * wob : 0.0f);
        const float w = w0 + (w1 - w0) * t;
        const int xa = (int)(xc - w * 0.5f), xb = (int)(xc + w * 0.5f);
        const int aa = y < SURFACE_Y ? a + 60 : a; // emerged parts stand out against the dark air
        canvas_blend_rect(bg, xa, y, xb - xa + 1, 1, col, (uint8_t)(aa > 255 ? 255 : aa));
    }
}

// Water colour behind the backdrop at row y (same curve as water_build, no
// dithering): opaque shapes blend against this, so overlapping strokes of one
// shape do not darken each other.
static uint16_t s_water_row[CANVAS_H];

static void water_rows(const AquariumConfig *c)
{
    for (int y = 0; y < CANVAS_H; y++) {
        if (y < SURFACE_Y) {
            s_water_row[y] = c565_scale(c->water_bottom, 90);
            continue;
        }
        const float f = (float)(y - SURFACE_Y) / (float)(CANVAS_H - SURFACE_Y), k = f * (2.0f - f);
        s_water_row[y] = c565_lerp(c->water_top, c->water_bottom, (uint8_t)(k * 255.0f));
    }
}

// ---- rock monoliths: convex polygons filled row by row, cheap shading

typedef struct {
    float x[8], y[8];
    int n;
    float rx0, ry0, rx1, ry1; // ridge line: the face right of it is in shade
    float top, h;
    uint32_t seed;
} Mono;

typedef struct {
    uint16_t lit, shade, edge;
    uint8_t alpha;
} RockStyle;

static void mono_fill(Canvas *bg, const Mono *m, const RockStyle *st)
{
    float y0 = m->y[0], y1 = m->y[0];
    for (int i = 1; i < m->n; i++) y0 = fminf(y0, m->y[i]), y1 = fmaxf(y1, m->y[i]);
    const int ya = y0 < (float)SURFACE_Y ? SURFACE_Y : (int)y0, yb = y1 >= (float)CANVAS_H ? CANVAS_H - 1 : (int)y1;
    for (int y = ya; y <= yb; y++) {
        const float py = (float)y + 0.5f;
        float xl = 1e9f, xr = -1e9f;
        for (int i = 0, j = m->n - 1; i < m->n; j = i++) { // convex: the row crosses two edges
            if ((m->y[i] > py) == (m->y[j] > py)) continue;
            const float x = m->x[j] + (m->x[i] - m->x[j]) * (py - m->y[j]) / (m->y[i] - m->y[j]);
            xl = fminf(xl, x), xr = fmaxf(xr, x);
        }
        if (xr < xl) continue;
        // rough silhouette: both sides wander a few px from row to row
        xl += (noise1d(m->seed, py * 0.18f) - 0.5f) * 6.0f;
        xr += (noise1d(m->seed ^ 0x77u, py * 0.18f) - 0.5f) * 6.0f;
        if (xr < xl) continue;
        const int xa = xl < 0.0f ? 0 : (int)xl, xb = xr >= (float)CANVAS_W ? CANVAS_W - 1 : (int)xr;
        const float t = (py - m->ry0) / (m->ry1 - m->ry0), ridge = m->rx0 + (m->rx1 - m->rx0) * t;
        const float light = 0.95f - 0.35f * fm_clamp((py - m->top) / m->h, 0.0f, 1.0f); // brighter towards the top
        uint16_t *row = bg->px + y * CANVAS_W;
        for (int x = xa; x <= xb; x++) {
            float v = light * ((float)x > ridge ? 0.62f : 1.0f);
            const uint32_t px = ((uint32_t)x * 83492791u ^ (uint32_t)y * 2654435761u) * 0x9E3779B1u;
            v += (float)((int)(px >> 27) - 16) * (1.0f / 200.0f);                                  // grain
            v += 0.07f * fm_sin((float)y * 0.45f + (float)x * 0.12f + fm_sin((float)x * 0.07f) * 3.0f); // wavy strata
            if ((px >> 24) < 10) v -= 0.3f;                                                         // dark granite speckles
            uint16_t col = c565_lerp(st->shade, st->lit, (uint8_t)(fm_clamp(v, 0.0f, 1.0f) * 255.0f));
            if ((float)x - xl < 1.5f || xr - (float)x < 1.5f) col = st->edge; // outline
            row[x] = c565_blend(row[x], col, st->alpha);
        }
    }
}

// A leaning slab standing on the bottom: base centre bx, base width bw, height h,
// lean (horizontal shift of the top per unit of height), top width as a fraction.
static void monolith(Canvas *bg, Rng *r, const RockStyle *st, float bx, float bw, float h, float lean, float topw)
{
    const float base = (float)CANVAS_H + 4.0f, ty = base - h, tx = bx + lean * h, tw = bw * topw;
    Mono m;
    m.n = 8;
    const float sh = rng_frange(r, 0.45f, 0.65f); // shoulder height
    const float rt = tw * 0.25f, dr = rng_frange(r, 0.0f, 10.0f), dl = rng_frange(r, 0.0f, 14.0f);
    m.x[0] = bx - bw * 0.5f, m.y[0] = base;
    m.x[1] = bx + bw * 0.5f, m.y[1] = base;
    m.x[2] = bx + bw * rng_frange(r, 0.45f, 0.6f) + lean * h * sh, m.y[2] = base - h * sh;
    m.x[3] = tx + tw * 0.5f, m.y[3] = ty + dr + rt;       // rounded top corners
    m.x[4] = tx + tw * 0.5f - rt, m.y[4] = ty + dr;
    m.x[5] = tx - tw * 0.5f + rt, m.y[5] = ty + dl;
    m.x[6] = tx - tw * 0.5f, m.y[6] = ty + dl + rt;
    m.x[7] = bx - bw * rng_frange(r, 0.45f, 0.6f) + lean * h * (sh - 0.1f), m.y[7] = base - h * (sh - 0.1f);
    m.rx0 = bx + bw * rng_frange(r, 0.0f, 0.3f), m.ry0 = base;
    m.rx1 = tx + tw * rng_frange(r, -0.1f, 0.2f), m.ry1 = ty;
    m.top = ty, m.h = h;
    m.seed = rng_next(r);
    mono_fill(bg, &m, st);
}

// ---- driftwood: recursive forking branches, amber rim light on the left

typedef struct {
    uint16_t wood, rim, dark;
    uint8_t alpha;
} WoodStyle;

static void wood_disc(Canvas *bg, const WoodStyle *ws, float x, float y, float r, float nx, float ny)
{
    const int ri = (int)ceilf(r);
    for (int py = -ri; py <= ri; py++)
        for (int px = -ri; px <= ri; px++) {
            if ((float)(px * px + py * py) > r * r + 0.5f) continue;
            const int X = (int)x + px, Y = (int)y + py;
            if (X < 0 || X >= CANVAS_W || Y < 0 || Y >= CANVAS_H) continue;
            const float t = ((float)px * nx + (float)py * ny) / fmaxf(r, 1.0f); // -1 left edge .. 1 right edge
            const uint16_t col = t < -0.5f ? ws->rim : (t > 0.45f ? ws->dark : ws->wood);
            bg->px[Y * CANVAS_W + X] = c565_blend(s_water_row[Y], col, ws->alpha);
        }
}

static void branch(Canvas *bg, const AquariumConfig *c, const WoodStyle *ws, Rng *r, float x, float y, float ang, float len, float w,
                   int depth)
{
    float done = 0.0f;
    while (done < len && y > -10.0f && w > 0.6f) {
        ang += rng_frange(r, -0.07f, 0.07f);
        const float dx = fm_sin(ang), dy = -fm_cos(ang);
        const float step = fmaxf(1.0f, w * 0.4f);
        wood_disc(bg, ws, x, y, w * 0.5f, -dy, dx);
        x += dx * step, y += dy * step, done += step;
        w *= 1.0f - 0.0025f * step; // slow taper
        if (depth < 2 && rng_range(r, 0, 400) < (int)step && done > len * 0.25f) // side shoot
            branch(bg, c, ws, r, x, y, ang + (rng_range(r, 0, 1) ? 1.0f : -1.0f) * rng_frange(r, 0.5f, 1.0f), len * rng_frange(r, 0.3f, 0.5f),
                   w * 0.55f, depth + 1);
    }
    if (depth < 2 && y > 0.0f)
        for (int k = 0; k < 2; k++)
            branch(bg, c, ws, r, x, y, ang + (k ? 1.0f : -1.0f) * rng_frange(r, 0.25f, 0.6f), len * rng_frange(r, 0.45f, 0.7f), w * 0.65f,
                   depth + 1);
}

// ---------------------------------------------------------------- biotopes

// Rockwork (Malawi): tall pale rock monoliths leaning against each other, deep
// blue water showing in the gaps between them, low slabs piled at the bottom.
static void draw_rocks(Canvas *bg, const AquariumConfig *c, Rng *r)
{
    const RockStyle far = {tint(c, RGB565(170, 180, 200), 120), tint(c, RGB565(60, 70, 96), 110), tint(c, RGB565(30, 36, 52), 100), 150};
    const RockStyle near = {tint(c, RGB565(226, 230, 236), 25), tint(c, RGB565(104, 114, 136), 30), tint(c, RGB565(40, 46, 60), 30), 235};
    for (int i = 0; i < 3; i++) // far monoliths in the gaps
        monolith(bg, r, &far, rng_frange(r, 60, 340), rng_frange(r, 50, 70), rng_frange(r, 110, 150), rng_frange(r, -0.2f, 0.2f),
                 rng_frange(r, 0.5f, 0.8f));
    float x = rng_frange(r, -10, 20);
    for (int i = 0; x < CANVAS_W + 30; i++) {
        const float bw = rng_frange(r, 60, 104), lean = (i & 1 ? 1.0f : -1.0f) * rng_frange(r, 0.08f, 0.3f);
        monolith(bg, r, &near, x, bw, rng_frange(r, 150, 240), lean, rng_frange(r, 0.5f, 0.85f));
        x += bw * rng_frange(r, 0.8f, 1.2f);
    }
    for (int i = 0; i < 3; i++) // low slabs at the bottom
        monolith(bg, r, &near, rng_frange(r, 40, 360), rng_frange(r, 60, 100), rng_frange(r, 55, 80), rng_frange(r, -0.6f, 0.6f),
                 rng_frange(r, 0.6f, 0.9f));
}

// Rice paddy (SE Asia): clumps of rice stems rising through shallow, silty water
// and out of the surface, leaves arching over at the top.
static void rice_clump(Canvas *bg, Rng *r, int bx, int n, int base, int top_lo, int top_hi, float w, uint16_t col, int a)
{
    for (int i = 0; i < n; i++) {
        const float sx = (float)bx + rng_frange(r, -4.0f, 4.0f);
        const float lean = rng_frange(r, -14.0f, 14.0f);
        const int top = rng_range(r, top_lo, top_hi);
        stroke_v(bg, sx, base, sx + lean, top, w, fmaxf(1.0f, w - 1.0f), col, a, 0, 0);
        // leaf tip arching outwards
        const float tx = sx + lean, dir = lean < 0 ? -1.0f : 1.0f;
        canvas_line_blend(bg, (int)tx, top, (int)(tx + dir * rng_frange(r, 5, 11)), top + rng_range(r, 4, 9), col,
                          (uint8_t)(top < SURFACE_Y ? a + 60 : a));
    }
}

static void draw_rice(Canvas *bg, const AquariumConfig *c, Rng *r)
{
    // silt haze over the lower water
    const uint16_t silt = tint(c, RGB565(110, 90, 50), 70);
    for (int y = 120; y < CANVAS_H; y++) canvas_blend_rect(bg, 0, y, CANVAS_W, 1, silt, (uint8_t)((y - 120) * 70 / (CANVAS_H - 120)));
    const uint16_t far = tint(c, RGB565(90, 140, 60), 80), near = tint(c, RGB565(110, 165, 70), 40);
    for (int x = rng_range(r, 0, 12); x < CANVAS_W + 10; x += rng_range(r, 28, 40))
        rice_clump(bg, r, x, rng_range(r, 4, 6), 210, 16, 50, 1.0f, far, 38);
    for (int x = rng_range(r, 10, 30); x < CANVAS_W + 10; x += rng_range(r, 70, 100))
        rice_clump(bg, r, x, rng_range(r, 5, 8), 225, 0, 20, 2.0f, near, 72);
}

// Swamp (blackwater): amber, tea-coloured light and a tangle of tall driftwood
// roots rising from the bottom, forking and leaving
// the frame at the top, lit amber on one side.
static void draw_swamp(Canvas *bg, const AquariumConfig *c, Rng *r)
{
    for (int y = SURFACE_Y; y < CANVAS_H; y++) // warm glow from the surface
        canvas_blend_rect(bg, 0, y, CANVAS_W, 1, RGB565(220, 120, 40), (uint8_t)(40 * (CANVAS_H - y) / (CANVAS_H - SURFACE_Y)));
    const WoodStyle far = {tint(c, RGB565(70, 34, 16), 70), tint(c, RGB565(140, 72, 32), 70), tint(c, RGB565(34, 16, 8), 60), 150};
    const WoodStyle near = {RGB565(62, 28, 14), RGB565(176, 86, 34), RGB565(28, 12, 6), 230};
    for (int x = rng_range(r, 10, 40); x < CANVAS_W; x += rng_range(r, 50, 80))
        branch(bg, c, &far, r, (float)x, (float)CANVAS_H, rng_frange(r, -0.35f, 0.35f), rng_frange(r, 150, 210), rng_frange(r, 6, 9), 1);
    for (int x = rng_range(r, 20, 60); x < CANVAS_W; x += rng_range(r, 90, 140))
        branch(bg, c, &near, r, (float)x, (float)CANVAS_H, rng_frange(r, -0.45f, 0.45f), rng_frange(r, 190, 260), rng_frange(r, 13, 18), 0);
}

// Flooded forest (igapo): tree trunks standing in the water, the near ones with
// flared buttress roots, thin aerial roots dangling from above.
static void trunk(Canvas *bg, float x, float w, uint16_t col, uint16_t bark, int a, bool buttress, uint32_t seed)
{
    for (int y = 0; y < CANVAS_H; y++) {
        float ww = w + (noise1d(seed, (float)y * 0.04f) - 0.5f) * w * 0.25f;
        if (buttress && y > 160) ww += (float)((y - 160) * (y - 160)) / 55.0f;
        const float xc = x + (noise1d(seed ^ 0x55u, (float)y * 0.02f) - 0.5f) * 6.0f;
        const int xa = (int)(xc - ww * 0.5f), xb = (int)(xc + ww * 0.5f);
        const int aa = y < SURFACE_Y ? a + 50 : a;
        canvas_blend_rect(bg, xa, y, xb - xa + 1, 1, col, (uint8_t)(aa > 255 ? 255 : aa));
        // bark furrows
        for (int k = 1; k < 3; k++) canvas_pblend(bg, xa + (xb - xa) * k / 3, y, bark, (uint8_t)(aa / 2));
    }
}

static void draw_forest(Canvas *bg, const AquariumConfig *c, Rng *r)
{
    const uint16_t far = tint(c, RGB565(70, 62, 50), 90), near = tint(c, RGB565(58, 48, 38), 50);
    const uint16_t bark = tint(c, RGB565(30, 26, 22), 50);
    for (int x = rng_range(r, 0, 30); x < CANVAS_W + 10; x += rng_range(r, 46, 80))
        trunk(bg, (float)x, rng_frange(r, 7, 13), far, bark, 50, false, rng_next(r));
    for (int x = rng_range(r, 30, 90); x < CANVAS_W; x += rng_range(r, 120, 170)) {
        trunk(bg, (float)x, rng_frange(r, 18, 28), near, bark, 100, true, rng_next(r));
        for (int k = 0; k < 3; k++) // aerial roots dropping from the crown
            stroke_v(bg, (float)(x + rng_range(r, -40, 40)), 0, (float)(x + rng_range(r, -46, 46)), rng_range(r, 90, 180), 1.0f, 1.0f,
                     near, 60, 4.0f, rng_next(r));
    }
}

// Reed bed (pond): thin straight reeds rising out of the water, a few long leaf
// blades and cattail heads above the surface.
static void draw_reeds(Canvas *bg, const AquariumConfig *c, Rng *r)
{
    const uint16_t far = tint(c, RGB565(110, 130, 70), 80), near = tint(c, RGB565(120, 150, 70), 40);
    const uint16_t head = RGB565(90, 60, 34);
    for (int i = 0; i < 46; i++) {
        const float x = (float)rng_range(r, 0, CANVAS_W);
        stroke_v(bg, x, CANVAS_H, x + rng_frange(r, -8, 8), rng_range(r, 0, 30), 1.0f, 1.0f, far, 48, 0, 0);
    }
    for (int i = 0; i < 15; i++) {
        const float x = (float)rng_range(r, 0, CANVAS_W), lean = rng_frange(r, -10, 10);
        const int top = rng_range(r, 0, 8);
        stroke_v(bg, x, CANVAS_H, x + lean, top, 2.0f, 1.0f, near, 100, 0, 0);
        if (rng_range(r, 0, 2) == 0) { // leaf blade peeling off the stalk
            const int ly = rng_range(r, 70, 170);
            const float lx = x + lean * (float)(CANVAS_H - ly) / (float)(CANVAS_H - top);
            const float dir = rng_range(r, 0, 1) ? 1.0f : -1.0f;
            canvas_line_blend(bg, (int)lx, ly, (int)(lx + dir * rng_frange(r, 14, 26)), ly - rng_range(r, 40, 70), near, 90);
        }
        if (top <= 4 && rng_range(r, 0, 1)) canvas_fill_ellipse(bg, (int)(x + lean), top + 4, 1, 3, head); // cattail
    }
}

void backdrop_draw(Canvas *bg, const AquariumConfig *c)
{
    Rng r;
    rng_seed(&r, 0xBAC0u + c->backdrop * 7919u);
    water_rows(c);
    switch (c->backdrop) {
    case BG_ROCKS: draw_rocks(bg, c, &r); break;
    case BG_RICE: draw_rice(bg, c, &r); break;
    case BG_SWAMP: draw_swamp(bg, c, &r); break;
    case BG_FOREST: draw_forest(bg, c, &r); break;
    case BG_REEDS: draw_reeds(bg, c, &r); break;
    default: break; // BG_OPEN: plain water
    }
}
