// Decorations: rocks, stones, wood, roots, branches and leaf litter. All are
// static and drawn into the background/foreground caches (no per-frame cost).
#include <math.h>
#include "../engine/fixmath.h"
#include "../engine/rng.h"
#include "world.h"

// Never write the foreground colour key (it would become a hole).
static inline void dpset(Canvas *cv, int x, int y, uint16_t c) { canvas_pset(cv, x, y, c == FG_KEY ? (uint16_t)(c ^ 1) : c); }

void decor_bounds(const DecorCfg *d, int *x0, int *y0, int *x1, int *y1)
{
    //                                         big flat tall clus slate drift spider branch roots leaves
    static const uint8_t HW[DECOR_COUNT] = {22, 26, 13, 24, 30, 30, 28, 30, 30, 24};
    static const uint8_t HH[DECOR_COUNT] = {26, 12, 46, 10, 32, 22, 62, 110, 40, 4};
    int fy = world_floor(d->x);
    *x0 = d->x - HW[d->type];
    *x1 = d->x + HW[d->type];
    *y0 = fy - HH[d->type];
    *y1 = fy + 4;
}

// Irregular boulder: ellipse with noise-modulated radius + top-left lighting.
static void rock(Canvas *cv, int cx, int cy, int rx, int ry, uint16_t col, uint32_t seed)
{
    uint16_t hi = c565_lerp(col, RGB565(200, 210, 220), 70);
    uint16_t lo = c565_scale(col, 150);
    for (int y = -ry; y <= ry / 3; y++) {
        for (int x = -rx; x <= rx; x++) {
            float ang = (float)x / (float)rx;
            float nr = 0.85f + 0.3f * noise1d(seed, (ang + 1.0f) * 3.0f + (y < 0 ? 0 : 7.0f));
            float fx = (float)x / ((float)rx * nr), fy = (float)y / (float)ry;
            float d = fx * fx + fy * fy;
            if (d > 1.0f) continue;
            uint16_t c = col;
            float light = -fx * 0.5f - fy * 0.8f;
            if (light > 0.55f) c = hi;
            else if (light < -0.15f) c = lo;
            if (d > 0.82f) c = c565_scale(c, 190);
            if ((hash32((uint32_t)(x * 31 + y * 17) ^ seed) & 15) == 0) c = c565_scale(c, 210);
            dpset(cv, cx + x, cy + y, c);
        }
    }
}

static void driftwood(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    uint16_t dark = c565_scale(col, 150), light = c565_lerp(col, RGB565(220, 190, 150), 60);
    // main log lying diagonally
    for (int i = 0; i < 3; i++) {
        float ang = (i == 0) ? -0.25f : -1.0f - 0.5f * (float)i + rng_frange(&r, -0.2f, 0.2f);
        float len = i == 0 ? 56.0f : rng_frange(&r, 16, 24);
        float x = (float)cx - (i == 0 ? 28.0f : -(float)(i * 10 - 14)), y = (float)cy - (i == 0 ? 0.0f : 6.0f);
        float th = i == 0 ? 3.5f : 1.8f;
        for (float d = 0; d < len; d += 0.7f) {
            float w = th * (1.0f - d / len * 0.6f);
            for (float k = -w; k <= w; k += 0.7f) {
                int px = (int)(x + fm_cos(ang) * d - fm_sin(ang) * k);
                int py = (int)(y + fm_sin(ang) * d + fm_cos(ang) * k);
                uint16_t c = k < -w * 0.4f ? light : (k > w * 0.4f ? dark : col);
                if (((int)(d * 1.3f) + (int)k) % 5 == 0) c = dark;
                dpset(cv, px, py, c);
            }
        }
    }
}


// Tall upright stone (Seiryu style): tapering jagged slab with light veins.
static void rock_tall(Canvas *cv, int cx, int cy, int hgt, int bw, uint16_t col, uint32_t seed)
{
    uint16_t hi = c565_lerp(col, RGB565(210, 215, 220), 80), lo = c565_scale(col, 150);
    uint16_t vein = c565_lerp(col, RGB565(235, 235, 230), 120);
    float lean = ((float)(seed & 15) - 7.5f) / 30.0f;
    for (int y = -hgt; y <= 2; y++) {
        float t = (float)(y + hgt) / (float)hgt; // 0 top .. 1 base
        float w = (float)bw * (0.25f + 0.75f * fm_sqrt(t));
        float jag = 1.5f * noise1d(seed, (float)y * 0.35f) - 0.75f;
        float jag2 = 1.5f * noise1d(seed ^ 0x55u, (float)y * 0.3f) - 0.75f;
        int off = (int)(lean * (float)(-y));
        int x0 = (int)(-w + jag), x1 = (int)(w + jag2);
        for (int x = x0; x <= x1; x++) {
            float fx = (float)(x - x0) / (float)(x1 - x0 + 1);
            uint16_t c = fx < 0.3f ? hi : (fx > 0.72f ? lo : col);
            if (((x + y * 2 + (int)(seed & 7)) % 11) == 0 && t > 0.15f) c = vein;
            if (x == x0 || x == x1) c = c565_scale(c, 200);
            dpset(cv, cx + off + x, cy + y, c);
        }
    }
}

// Low pile of rounded pebbles.
static void rock_cluster(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    int n = rng_range(&r, 4, 6);
    for (int i = 0; i < n; i++) {
        int x = cx + rng_range(&r, -18, 18), rx = rng_range(&r, 4, 8), ry = rng_range(&r, 4, 7);
        uint16_t c = c565_lerp(col, RGB565(150, 140, 125), (uint8_t)rng_range(&r, 0, 90));
        rock(cv, x, cy + ry / 3, rx, ry, c, seed + (uint32_t)i * 101u);
    }
}

// Stacked slate slabs with dark gaps between layers.
static void rock_slate(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    uint16_t hi = c565_lerp(col, RGB565(200, 205, 210), 60), lo = c565_scale(col, 140);
    int y = cy, w = 26;
    int layers = rng_range(&r, 5, 7);
    int off = 0;
    for (int l = 0; l < layers; l++) {
        int th = rng_range(&r, 3, 5);
        off += rng_range(&r, -4, 4);
        for (int k = 0; k < th; k++)
            for (int x = -w; x <= w; x++) {
                if ((x == -w || x == w) && k == 0) continue;
                uint16_t c = k == 0 ? hi : (k == th - 1 ? lo : col);
                if (((x * 5 + k * 3 + l) & 15) == 0) c = lo;
                dpset(cv, cx + off + x, y - k, c);
            }
        y -= th;
        dpset(cv, cx + off - w + 1, y + 1, c565_scale(col, 90)); // shadowed gap
        w -= rng_range(&r, 1, 4);
        if (w < 8) break;
    }
}

// Tapered wooden limb; returns the end point.
static void limb(Canvas *cv, float x, float y, float ang, float len, float th0, float th1, uint16_t col, float *ex,
                 float *ey)
{
    uint16_t dark = c565_scale(col, 145), light = c565_lerp(col, RGB565(225, 195, 155), 70);
    float ca = fm_cos(ang), sa = fm_sin(ang);
    for (float d = 0; d < len; d += 0.6f) {
        float w = th0 + (th1 - th0) * d / len;
        float px = x + ca * d, py = y + sa * d;
        for (float k = -w; k <= w; k += 0.6f) {
            uint16_t c = k < -w * 0.35f ? light : (k > w * 0.35f ? dark : col);
            if (((int)(d * 0.9f) + (int)(k * 2.0f)) % 7 == 0) c = dark; // bark grain
            dpset(cv, (int)(px - sa * k), (int)(py + ca * k), c);
        }
    }
    *ex = x + ca * len;
    *ey = y + sa * len;
}

// Recursive branch: forks into two thinner limbs.
static void branch_rec(Canvas *cv, Rng *r, float x, float y, float ang, float len, float th, int depth, uint16_t col)
{
    float ex, ey;
    float bend = rng_frange(r, -0.25f, 0.25f);
    limb(cv, x, y, ang + bend * 0.5f, len, th, th * 0.65f, col, &ex, &ey);
    if (depth <= 0 || th < 0.6f) return;
    int nb = rng_range(r, 1, 2) + (depth > 1 ? 1 : 0);
    for (int i = 0; i < nb; i++) {
        float a = ang + bend + rng_frange(r, -0.75f, 0.75f);
        if (a > -0.3f) a = -0.3f; // keep growing upwards
        if (a < -FM_PI + 0.3f) a = -FM_PI + 0.3f;
        branch_rec(cv, r, ex, ey, a, len * rng_frange(r, 0.5f, 0.75f), th * 0.6f, depth - 1, col);
    }
}

// Spiderwood: many thin twisted roots radiating up from a small base.
static void spiderwood(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    int n = rng_range(&r, 5, 8);
    for (int i = 0; i < n; i++) {
        float a = -FM_PI / 2 + ((float)i / (float)(n - 1) - 0.5f) * 2.0f + rng_frange(&r, -0.15f, 0.15f);
        branch_rec(cv, &r, (float)cx + rng_frange(&r, -4, 4), (float)cy, a, rng_frange(&r, 16, 26), 1.3f, 3, col);
    }
}

// Tall upright branch reaching towards the surface.
static void branch_tall(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    float a = -FM_PI / 2 + rng_frange(&r, -0.35f, 0.35f);
    float x = (float)cx, y = (float)cy;
    float th = 3.2f;
    for (int seg = 0; seg < 4; seg++) { // crooked trunk with side branches
        float ex, ey, len = rng_frange(&r, 20, 28);
        limb(cv, x, y, a, len, th, th * 0.8f, col, &ex, &ey);
        float side = (seg & 1) ? 1.0f : -1.0f;
        if (seg > 0)
            branch_rec(cv, &r, ex, ey, a + side * rng_frange(&r, 0.5f, 0.9f), len * 0.7f, th * 0.5f, 2, col);
        x = ex;
        y = ey;
        th *= 0.78f;
        a += rng_frange(&r, -0.3f, 0.3f);
        if (a > -0.6f) a = -0.6f;
        if (a < -FM_PI + 0.6f) a = -FM_PI + 0.6f;
    }
}

// Mangrove-like stump with roots arching into the substrate on both sides.
static void roots(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    float topx = (float)cx + rng_frange(&r, -4, 4), topy = (float)cy - rng_frange(&r, 30, 38);
    float ex, ey;
    limb(cv, topx, topy, -FM_PI / 2 + rng_frange(&r, -0.2f, 0.2f), 8, 3.0f, 2.2f, col, &ex, &ey); // broken top
    int n = rng_range(&r, 5, 7);
    for (int i = 0; i < n; i++) {
        float side = (i & 1) ? 1.0f : -1.0f;
        float spread = rng_frange(&r, 8, 28) * side;
        // quadratic arch from the stump to the ground
        float px = topx, py = topy;
        float gx = (float)cx + spread, gy = (float)cy + 2;
        float mx = topx + spread * 0.8f, my = topy + rng_frange(&r, 2, 10);
        for (int k = 1; k <= 10; k++) {
            float t = (float)k / 10.0f, it = 1.0f - t;
            float qx = it * it * topx + 2 * it * t * mx + t * t * gx;
            float qy = it * it * topy + 2 * it * t * my + t * t * gy;
            float dx = qx - px, dy = qy - py;
            float len = fm_sqrt(dx * dx + dy * dy);
            float ang = len > 0.01f ? atan2f(dy, dx) : 0;
            limb(cv, px, py, ang, len + 0.5f, 2.2f - t * 1.2f, 2.0f - t * 1.2f, col, &ex, &ey);
            px = qx;
            py = qy;
        }
    }
}

// Catappa leaves lying on the substrate (blackwater).
static void leaves(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    int n = rng_range(&r, 3, 5);
    for (int i = 0; i < n; i++) {
        int lx = cx + rng_range(&r, -18, 18), ly = cy + rng_range(&r, -1, 1);
        int rx = rng_range(&r, 4, 7);
        uint16_t c = c565_lerp(col, RGB565(60, 35, 20), (uint8_t)rng_range(&r, 0, 140));
        uint16_t vein = c565_scale(c, 160);
        int tilt = rng_range(&r, -1, 1);
        for (int x = -rx; x <= rx; x++) {
            float f = (float)x / (float)rx;
            int hh = (int)(2.0f * fm_sqrt(1.0f - f * f) + 0.3f);
            int yo = tilt * x / rx;
            for (int y = -hh; y <= 0; y++) dpset(cv, lx + x, ly + y + yo, y == -hh / 2 ? vein : c);
        }
        dpset(cv, lx - rx - 1, ly, vein); // stalk
    }
}

static void draw_one(Canvas *cv, const DecorCfg *d, int idx)
{
    int fy = world_floor(d->x);
    uint32_t seed = (uint32_t)(idx * 977 + d->x * 13 + d->type);
    switch (d->type) {
    case DECOR_ROCK_BIG: rock(cv, d->x, fy + 4, 20, 24, d->color, seed); break;
    case DECOR_ROCK_FLAT: rock(cv, d->x, fy + 3, 24, 11, d->color, seed); break;
    case DECOR_ROCK_TALL: rock_tall(cv, d->x, fy + 2, 42, 12, d->color, seed); break;
    case DECOR_ROCK_CLUSTER: rock_cluster(cv, d->x, fy + 1, d->color, seed); break;
    case DECOR_ROCK_SLATE: rock_slate(cv, d->x, fy + 2, d->color, seed); break;
    case DECOR_DRIFTWOOD: driftwood(cv, d->x, fy - 2, d->color, seed); break;
    case DECOR_SPIDERWOOD: spiderwood(cv, d->x, fy + 1, d->color, seed); break;
    case DECOR_BRANCH: branch_tall(cv, d->x, fy + 2, d->color, seed); break;
    case DECOR_ROOTS: roots(cv, d->x, fy, d->color, seed); break;
    case DECOR_LEAVES: leaves(cv, d->x, fy + 1, d->color, seed); break;
    default: break;
    }
}

void decor_draw_static(Canvas *cv, const AquariumConfig *c, int layer)
{
    for (int i = 0; i < c->decor_count; i++) {
        const DecorCfg *d = &c->decor[i];
        if (d->layer == layer) draw_one(cv, d, i);
    }
}

void decor_draw_dynamic(Canvas *cv, const AquariumConfig *c, int layer, float t)
{
    // no animated decor at the moment (kept as a hook for the render pipeline)
    (void)cv;
    (void)c;
    (void)layer;
    (void)t;
}

int decor_hit(const AquariumConfig *c, int x, int y)
{
    for (int i = c->decor_count - 1; i >= 0; i--) {
        int x0, y0, x1, y1;
        decor_bounds(&c->decor[i], &x0, &y0, &x1, &y1);
        if (x >= x0 && x <= x1 && y >= y0 && y <= y1) return i;
    }
    return -1;
}
