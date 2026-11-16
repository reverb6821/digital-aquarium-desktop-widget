// Decorations: rocks, stones, wood, roots, branches and leaf litter. All are
// static and drawn into the background/foreground caches (no per-frame cost).
#include <math.h>
#include "../engine/fixmath.h"
#include "../engine/rng.h"
#include "world.h"

// Never write the foreground colour key (it would become a hole).
static inline void dpset(Canvas *cv, int x, int y, uint16_t c) { canvas_pset(cv, x, y, c == FG_KEY ? (uint16_t)(c ^ 1) : c); }
static inline void dpsetf(Canvas *cv, float x, float y, uint16_t c) { dpset(cv, (int)floorf(x + 0.5f), (int)floorf(y + 0.5f), c); }

// Base point (rotation pivot) of a decoration: on the substrate, shifted by dy.
static inline int decor_base_y(const DecorCfg *d) { return world_floor(d->x) + d->dy; }

// Rotate (x, y) by the angle whose cosine/sine are (c, s); y points down, so a
// positive angle turns clockwise on screen.
static inline void rot2(float c, float s, float x, float y, float *ox, float *oy)
{
    *ox = c * x - s * y;
    *oy = s * x + c * y;
}

static inline float deg2rad(int deg) { return (float)deg * (FM_PI / 180.0f); }

static inline int decor_scale_pct(const DecorCfg *d) { return d->scale ? d->scale : 100; }
static inline bool decor_plain(const DecorCfg *d) { return d->rot == 0 && decor_scale_pct(d) == 100; }

void decor_bounds(const DecorCfg *d, int *x0, int *y0, int *x1, int *y1)
{
    //                                       big drag seir clus slate drift spider branch roots leaves
    static const uint8_t HW[DECOR_COUNT] = {22, 24, 22, 28, 32, 32, 38, 40, 40, 28};
    static const uint8_t HH[DECOR_COUNT] = {26, 40, 40, 24, 30, 26, 60, 100, 48, 9};
    int fy = decor_base_y(d);
    int hw = HW[d->type], hh = HH[d->type];
    if (decor_plain(d)) {
        *x0 = d->x - hw;
        *x1 = d->x + hw;
        *y0 = fy - hh;
        *y1 = fy + 4;
        return;
    }
    // axis-aligned box around the scaled + rotated local box
    float k = (float)decor_scale_pct(d) / 100.0f;
    float a = deg2rad(d->rot), c = fm_cos(a) * k, s = fm_sin(a) * k;
    const float cx[4] = {(float)-hw, (float)hw, (float)hw, (float)-hw}, cy[4] = {(float)-hh, (float)-hh, 4.0f, 4.0f};
    float mnx = 1e9f, mny = 1e9f, mxx = -1e9f, mxy = -1e9f;
    for (int i = 0; i < 4; i++) {
        float ox, oy;
        rot2(c, s, cx[i], cy[i], &ox, &oy);
        if (ox < mnx) mnx = ox;
        if (ox > mxx) mxx = ox;
        if (oy < mny) mny = oy;
        if (oy > mxy) mxy = oy;
    }
    *x0 = d->x + (int)floorf(mnx);
    *x1 = d->x + (int)ceilf(mxx);
    *y0 = fy + (int)floorf(mny);
    *y1 = fy + (int)ceilf(mxy);
}

// ------------------------------------------------------------------ helpers

static float lat2(uint32_t s, int x, int y)
{
    return (float)(hash32(s ^ ((uint32_t)x * 0x9E3779B1u) ^ ((uint32_t)y * 0x85EBCA77u)) >> 8) * (1.0f / 16777216.0f);
}

// Smooth 2D value noise in [0,1).
static float vn2(uint32_t s, float x, float y)
{
    float fx0 = floorf(x), fy0 = floorf(y);
    int ix = (int)fx0, iy = (int)fy0;
    float fx = x - fx0, fy = y - fy0;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    float a = lat2(s, ix, iy), b = lat2(s, ix + 1, iy), c = lat2(s, ix, iy + 1), d = lat2(s, ix + 1, iy + 1);
    float top = a + (b - a) * fx, bot = c + (d - c) * fx;
    return top + (bot - top) * fy;
}

static inline int mirror_dir(uint32_t seed) { return (hash32(seed * 7u + 3u) & 1u) ? 1 : -1; }

// ------------------------------------------------------------------ rocks

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

// One jagged dragon-stone spire: tapering, eroded edges, warm mottling and
// elongated vertical pits.
static void dragon_spire(Canvas *cv, int cx, int cy, int h, int bw, uint16_t col, uint32_t s, uint32_t shade)
{
    uint16_t orange = RGB565(190, 92, 48), pale = RGB565(225, 196, 150);
    float lean = ((float)(s & 15) - 7.5f) / 50.0f;
    for (int y = -h; y <= 2; y++) {
        float t = (float)(y + h) / (float)h; // 0 top .. 1 base
        if (t > 1.0f) t = 1.0f;
        float w = (float)bw * (0.1f + 0.9f * t * (2.0f - t) * 0.5f + 0.45f * t);
        float jl = (noise1d(s, (float)y * 0.45f) - 0.5f) * 4.0f + (noise1d(s + 3u, (float)y * 1.4f) - 0.5f) * 2.0f;
        float jr = (noise1d(s + 5u, (float)y * 0.45f) - 0.5f) * 4.0f + (noise1d(s + 9u, (float)y * 1.4f) - 0.5f) * 2.0f;
        int off = (int)(lean * (float)(-y));
        int x0 = (int)(-w + jl), x1 = (int)(w + jr);
        if (x1 <= x0) continue;
        for (int x = x0; x <= x1; x++) {
            float fx = 2.0f * (float)(x - x0) / (float)(x1 - x0) - 1.0f;
            uint16_t c = col;
            float tone = vn2(s + 7u, (float)x * 0.22f, (float)y * 0.12f);
            if (tone > 0.62f) c = c565_lerp(c, orange, (uint8_t)((tone - 0.62f) * 400.0f));
            else if (tone < 0.3f) c = c565_lerp(c, pale, 70);
            float light = -fx * 0.55f + (vn2(s + 11u, (float)x * 0.5f, (float)y * 0.3f) - 0.5f) * 1.1f;
            if (light > 0.38f) c = c565_lerp(c, pale, 110);
            else if (light < -0.35f) c = c565_scale(c, 165);
            float pit = vn2(s + 19u, (float)x * 0.45f, (float)y * 0.17f);
            if (pit > 0.71f) c = c565_scale(c, 70);
            else if (pit > 0.65f) c = c565_scale(c, 160);
            if (x == x0 || x == x1) c = c565_scale(c, 180);
            dpset(cv, cx + off + x, cy + y, c565_scale(c, shade));
        }
    }
}

// Dragon stone (Ohko): a cluster of craggy, pitted spires.
static void dragon_stone(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    int dir = mirror_dir(seed);
    int h2 = rng_range(&r, 22, 27), w2 = rng_range(&r, 9, 11), o2 = dir * rng_range(&r, 9, 12);
    int h1 = rng_range(&r, 34, 38), w1 = rng_range(&r, 13, 15);
    int h3 = rng_range(&r, 11, 15), w3 = rng_range(&r, 8, 10), o3 = -dir * rng_range(&r, 11, 14);
    dragon_spire(cv, cx + o2, cy, h2, w2, col, seed + 101u, 205); // behind
    dragon_spire(cv, cx, cy, h1, w1, col, seed + 202u, 256);
    dragon_spire(cv, cx + o3, cy, h3, w3, col, seed + 303u, 232); // low chunk in front
}

// Seiryu body: peaked mound made of tilted strata, each ledge jutting out by a
// different amount, with thin white calcite veins.
static void seiryu_body(Canvas *cv, int cx, int cy, int h, int bw, uint16_t col, uint32_t s, int dir)
{
    uint16_t hi = c565_lerp(col, RGB565(215, 220, 228), 85), lo = c565_scale(col, 150);
    uint16_t vein = RGB565(228, 230, 234);
    float tilt = 0.3f * (float)dir;
    int peak = dir * (int)(s % 5u);
    for (int y = -h; y <= 2; y++) {
        float t = (float)(y + h) / (float)h;
        if (t > 1.0f) t = 1.0f;
        int L = (int)floorf((float)y / 5.0f);
        // asymmetric profile: a steep cliff on one side, a stepped slope on the other
        float steep = (float)bw * (0.2f + 0.8f * fm_sqrt(fm_sqrt(t))), slope = (float)bw * (0.1f + 0.9f * t);
        float bl = dir > 0 ? slope : steep, br = dir > 0 ? steep : slope;
        float el = bl * (0.7f + 0.6f * lat2(s, L, 0)) + (noise1d(s + 1u, (float)y * 0.7f) - 0.5f) * 3.0f;
        float er = br * (0.7f + 0.6f * lat2(s, L, 1)) + (noise1d(s + 2u, (float)y * 0.7f) - 0.5f) * 3.0f;
        int off = (int)((float)peak * (1.0f - t));
        int x0 = (int)-el, x1 = (int)er;
        for (int x = x0; x <= x1; x++) {
            float q = (float)y + tilt * (float)x + 7.0f * vn2(s + 4u, (float)x * 0.1f, (float)y * 0.06f) + 60.0f;
            float k = q - 5.0f * floorf(q / 5.0f); // position inside the stratum, 0..5
            float brk = vn2(s + 6u, (float)x * 0.35f, (float)y * 0.2f); // breaks up the ledge lines
            uint16_t c = col;
            float tone = vn2(s + 9u, (float)x * 0.3f, (float)y * 0.3f);
            if (tone > 0.68f) c = c565_lerp(c, hi, 80);
            else if (tone < 0.28f) c = c565_scale(c, 210);
            if (k < 1.0f && brk > 0.3f) c = hi;
            else if (k >= 4.1f && brk < 0.75f) c = lo;
            float fx = (float)(x - x0) / (float)(x1 - x0 + 1);
            if (fx > 0.7f) c = c565_scale(c, 200);
            else if (fx < 0.25f && k < 4.1f) c = c565_lerp(c, hi, 60);
            float v = vn2(s + 23u, (float)x * 0.09f, (float)y * 0.11f);
            if (fabsf(v - 0.5f) < 0.012f) c = c565_lerp(c, vein, 190);
            if (x == x0 || x == x1) c = c565_scale(c, 175);
            dpset(cv, cx + off + x, cy + y, c);
        }
    }
}

// Seiryu stone: tall craggy piece with a smaller chunk leaning on one side.
static void seiryu_stone(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    int dir = mirror_dir(seed);
    int h = rng_range(&r, 35, 39), bw = rng_range(&r, 13, 15);
    seiryu_body(cv, cx + dir * rng_range(&r, 10, 13), cy, h * 45 / 100, bw * 7 / 10, c565_scale(col, 225), seed + 77u, -dir);
    seiryu_body(cv, cx, cy, h, bw, col, seed, dir);
}

// Smooth, tilted, porous river stone sitting half-buried at (cx, cy).
static void pebble(Canvas *cv, int cx, int cy, int rx, int ry, float ang, uint16_t col, uint32_t seed)
{
    uint16_t hi = c565_lerp(col, RGB565(240, 232, 215), 90), lo = c565_scale(col, 165);
    float ca = fm_cos(ang), sa = fm_sin(ang);
    int R = (rx > ry ? rx : ry) + 1;
    int ccy = cy - ry * 3 / 4;
    for (int dy = -R; dy <= R; dy++)
        for (int dx = -R; dx <= R; dx++) {
            float u = ((float)dx * ca + (float)dy * sa) / (float)rx;
            float v = (-(float)dx * sa + (float)dy * ca) / (float)ry;
            float d = u * u + v * v;
            if (d > 1.0f) continue;
            float light = -((float)dx / (float)R) * 0.55f - ((float)dy / (float)R) * 0.85f;
            uint16_t c = light > 0.45f ? hi : (light > 0.05f ? col : (light > -0.4f ? c565_scale(col, 210) : lo));
            if (d > 0.84f) c = c565_scale(c, 200);
            uint32_t hs = hash32((uint32_t)(dx * 73 + dy * 151) ^ seed);
            if ((hs & 15) == 0) c = c565_scale(c, 175);      // pores
            else if ((hs & 31) == 1) c = c565_lerp(c, hi, 120);
            dpset(cv, cx + dx, ccy + dy, c);
        }
}

// Rounded river stones of different size leaning on each other, with small
// coloured pebbles scattered around their base.
static void rock_cluster(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    int n = rng_range(&r, 3, 4);
    static const int OX[2][4] = {{-11, 9, -22, 21}, {-14, 3, 17, -24}};
    const int *ox = OX[seed & 1];
    for (int i = 0; i < n; i++) { // biggest first, the others in front
        int ry = (i == 0 ? 13 : 11 - i * 2) + rng_range(&r, -1, 1);
        int rx = ry * rng_range(&r, 70, 90) / 100;
        float ang = (ox[i] > 0 ? -1.0f : 1.0f) * rng_frange(&r, 0.1f, 0.45f);
        uint16_t c = c565_lerp(col, (i & 1) ? RGB565(205, 195, 175) : RGB565(150, 135, 115), (uint8_t)rng_range(&r, 20, 110));
        pebble(cv, cx + ox[i] + rng_range(&r, -2, 2), cy, rx, ry, ang, c, seed + (uint32_t)i * 131u);
    }
    static const uint16_t SMALL[4] = {RGB565(125, 122, 118), RGB565(135, 98, 72), RGB565(185, 165, 135), RGB565(95, 92, 90)};
    int m = rng_range(&r, 10, 14);
    for (int i = 0; i < m; i++) {
        int rr = rng_range(&r, 1, 2);
        pebble(cv, cx + rng_range(&r, -27, 27), cy + rng_range(&r, 0, 2), rr + 1, rr, rng_frange(&r, -0.6f, 0.6f),
               SMALL[rng_range(&r, 0, 3)], seed + 999u + (uint32_t)i);
    }
}

// One slate slab: thin tilted plate with pointed, chipped ends.
static void slate_slab(Canvas *cv, float cx, float cy, float len, float th, float ang, uint16_t col, uint32_t s)
{
    uint16_t hi = c565_lerp(col, RGB565(150, 160, 178), 110), lo = c565_scale(col, 105), gap = c565_scale(col, 55);
    float ca = fm_cos(ang), sa = fm_sin(ang);
    float half = len * 0.5f;
    float tipl = 2.0f + 4.0f * lat2(s, 0, 0), tipr = 2.0f + 4.0f * lat2(s, 1, 0);
    for (float u = -half; u <= half; u += 0.5f) {
        float e = u < 0 ? (u + half) / tipl : (half - u) / tipr;
        if (e > 1.0f) e = 1.0f;
        float top = (noise1d(s + 3u, u * 0.4f) - 0.5f) * 1.2f;
        float vth = th * e + top;
        for (float v = top; v <= vth; v += 0.5f) {
            uint16_t c = col;
            float g = vn2(s + 5u, u * 0.14f, v * 0.9f);
            if (g > 0.7f) c = c565_lerp(c, hi, 70);
            else if (g < 0.25f) c = c565_scale(c, 200);
            if (v < top + 1.0f) c = hi;
            else if (v > vth - 1.0f) c = vth - top > 2.0f ? gap : lo;
            dpsetf(cv, cx + u * ca - v * sa, cy + u * sa + v * ca, c);
        }
    }
}

// Stacked slate: irregular pile of tilted plates plus a couple leaning on it.
static void rock_slate(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    int dir = mirror_dir(seed);
    int layers = rng_range(&r, 7, 9);
    float y = (float)cy;
    for (int l = 0; l < layers; l++) {
        float th = rng_frange(&r, 3.0f, 4.5f);
        float len = rng_frange(&r, 30.0f, 44.0f) * (1.0f - (float)l * 0.075f);
        float off = rng_frange(&r, -7.0f, 7.0f) * (1.0f - (float)l * 0.06f);
        float ang = rng_frange(&r, -0.2f, 0.2f);
        slate_slab(cv, (float)cx + off, y - th, len, th, ang, col, seed + (uint32_t)l * 37u);
        y -= th * 0.8f + rng_frange(&r, 0.0f, 1.0f);
    }
    float lx = (float)(cx + dir * rng_range(&r, 14, 18));
    slate_slab(cv, lx, (float)cy - 6.0f, rng_frange(&r, 16, 20), 3.5f, (float)dir * rng_frange(&r, 0.45f, 0.6f),
               c565_lerp(col, RGB565(110, 118, 130), 40), seed + 13u); // leaning on the pile
}

// ------------------------------------------------------------------ wood

typedef struct {
    uint16_t col, hi, lo, crack;
    float grain;   // crack density along the fibres (0 = smooth)
    uint8_t patchy; // dark bark patches
} WoodPal;

static WoodPal wood_pal(uint16_t col, uint16_t light, float grain, int patchy)
{
    WoodPal p = {col, c565_lerp(col, light, 100), c565_scale(col, 150), c565_scale(col, 95), grain, (uint8_t)patchy};
    return p;
}

// Tapered wooden tube along a quadratic Bezier, lit from the top-left, with
// fibre cracks running along its length.
static void wood_stroke(Canvas *cv, float x0, float y0, float x1, float y1, float x2, float y2, float th0, float th1,
                        const WoodPal *p, uint32_t seed)
{
    float len = fm_sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)) + fm_sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    int n = (int)(len * 1.8f) + 2;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / (float)n, it = 1.0f - t;
        float px = it * it * x0 + 2 * it * t * x1 + t * t * x2;
        float py = it * it * y0 + 2 * it * t * y1 + t * t * y2;
        float dx = 2 * it * (x1 - x0) + 2 * t * (x2 - x1), dy = 2 * it * (y1 - y0) + 2 * t * (y2 - y1);
        float dl = fm_sqrt(dx * dx + dy * dy);
        if (dl < 1e-4f) continue;
        float nx = -dy / dl, ny = dx / dl;
        float s = t * len;
        float w = th0 + (th1 - th0) * t;
        if (w < 0.5f) w = 0.5f;
        float knot = (lat2(seed + 5u, (int)(s * 0.25f), 0) > 0.9f) ? 0.6f : 0.0f; // occasional bump
        float wl = w + knot;
        int patch = p->patchy && vn2(seed + 99u, s * 0.15f, 0.5f) < 0.22f;
        for (float k = -wl; k <= wl; k += 0.5f) {
            float f = k / wl;
            float lit = -(nx * f) * 0.45f - (ny * f) * 0.9f;
            uint16_t c = lit > 0.35f ? p->hi : (lit < -0.45f ? p->lo : p->col);
            if (p->grain > 0) {
                float g = vn2(seed, s * 0.07f, (f + 1.0f) * 3.0f);
                if (g < p->grain) c = p->crack;
                else if (g > 1.0f - p->grain * 0.8f) c = c565_lerp(c, p->hi, 120);
            }
            if (patch && f > -0.2f && f < 0.6f) c = c565_scale(c, 140);
            dpsetf(cv, px + nx * k, py + ny * k, c);
        }
    }
}

// Point and tangent angle on a quadratic Bezier.
static void bez_at(float x0, float y0, float x1, float y1, float x2, float y2, float t, float *x, float *y, float *a)
{
    float it = 1.0f - t;
    *x = it * it * x0 + 2 * it * t * x1 + t * t * x2;
    *y = it * it * y0 + 2 * it * t * y1 + t * t * y2;
    *a = atan2f(2 * it * (y1 - y0) + 2 * t * (y2 - y1), 2 * it * (x1 - x0) + 2 * t * (x2 - x1));
}

// Curved tapering limb that may fork; forks always grow upwards.
static void wood_limb(Canvas *cv, Rng *r, const WoodPal *p, float x, float y, float ang, float len, float th, int depth,
                      float curl, uint32_t seed)
{
    float bend = rng_frange(r, -curl, curl);
    float ca = fm_cos(ang), sa = fm_sin(ang);
    float ex = x + ca * len, ey = y + sa * len;
    float mx = (x + ex) * 0.5f - sa * len * bend * 0.5f, my = (y + ey) * 0.5f + ca * len * bend * 0.5f;
    float th1 = th * 0.5f;
    wood_stroke(cv, x, y, mx, my, ex, ey, th, th1 < 0.5f ? 0.5f : th1, p, seed);
    if (depth <= 0 || th < 0.7f) return;
    int nb = rng_range(r, 1, 2);
    for (int i = 0; i < nb; i++) {
        float t = rng_frange(r, 0.4f, 0.85f), bx, by, ta;
        bez_at(x, y, mx, my, ex, ey, t, &bx, &by, &ta);
        float a = ta + ((i & 1) ? 1.0f : -1.0f) * rng_frange(r, 0.35f, 0.8f);
        if (fm_sin(a) > 0.05f) a = -a; // keep it above the ground
        wood_limb(cv, r, p, bx, by, a, len * rng_frange(r, 0.3f, 0.55f), th * (1.0f - t * 0.5f) * 0.65f, depth - 1, curl,
                  seed + (uint32_t)i * 17u + 1u);
    }
}

// Weathered driftwood log lying on the bottom, arching up towards one end,
// with a broken side branch and a splintered thick end.
static void driftwood(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    int dir = mirror_dir(seed);
    WoodPal p = wood_pal(col, RGB565(230, 222, 205), 0.24f, 0);
    float x0 = (float)(cx - dir * 30), y0 = (float)cy - 3.0f;
    float x2 = (float)(cx + dir * 29), y2 = (float)cy - rng_frange(&r, 14, 19);
    float x1 = (float)(cx - dir * 2), y1 = (float)cy - rng_frange(&r, 17, 22);
    float bx, by, ba;
    bez_at(x0, y0, x1, y1, x2, y2, rng_frange(&r, 0.38f, 0.5f), &bx, &by, &ba);
    wood_limb(cv, &r, &p, bx, by - 2.0f, -FM_PI / 2 - (float)dir * rng_frange(&r, 0.3f, 0.6f), rng_frange(&r, 10, 14),
              2.6f, 0, 0.2f, seed + 7u); // broken branch (behind)
    wood_stroke(cv, x0, y0, x1, y1, x2, y2, 6.0f, 3.2f, &p, seed);
    for (int i = 0; i < 4; i++) { // splinters on the thick end
        float a = (dir > 0 ? FM_PI : 0.0f) + rng_frange(&r, -0.45f, 0.45f);
        float sy = y0 + rng_frange(&r, -4.0f, 4.0f);
        float l = rng_frange(&r, 3.0f, 6.0f);
        wood_stroke(cv, x0, sy, x0 + fm_cos(a) * l * 0.5f, sy + fm_sin(a) * l * 0.5f, x0 + fm_cos(a) * l, sy + fm_sin(a) * l, 1.1f,
                    0.5f, &p, seed + 50u + (uint32_t)i);
    }
}

// Spiderwood: gnarled knot near the ground with long, thin, sinuous roots
// spreading up and to one side.
static void spiderwood(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    int dir = mirror_dir(seed);
    WoodPal p = wood_pal(col, RGB565(238, 212, 172), 0.12f, 0);
    float kx = (float)(cx + dir * 12), ky = (float)cy - 3.0f;
    int n = rng_range(&r, 6, 8);
    for (int i = 0; i < n; i++) {
        float f = (float)i / (float)(n - 1);
        float a = -FM_PI / 2 + (float)dir * (0.85f - 2.3f * f) + rng_frange(&r, -0.12f, 0.12f);
        float len = rng_frange(&r, 36.0f, 56.0f) * (1.0f - 0.3f * f * f);
        wood_limb(cv, &r, &p, kx + rng_frange(&r, -3, 3), ky + rng_frange(&r, -2, 1), a, len, 1.7f, 1, 0.7f,
                  seed + (uint32_t)i * 29u);
    }
    for (int i = 0; i < 2; i++) // roots lying on the substrate
        wood_limb(cv, &r, &p, kx, ky + 2.0f, (dir > 0 ? FM_PI : 0.0f) - (float)dir * rng_frange(&r, 0.02f, 0.15f),
                  rng_frange(&r, 18, 28), 1.5f, 0, 0.3f, seed + 400u + (uint32_t)i);
    for (int i = 0; i < 5; i++) { // knot
        float a = rng_frange(&r, -FM_PI, 0.3f);
        float l = rng_frange(&r, 4.0f, 7.0f);
        wood_stroke(cv, kx, ky, kx + fm_cos(a) * l * 0.5f, ky + fm_sin(a) * l * 0.4f, kx + fm_cos(a) * l, ky + fm_sin(a) * l * 0.6f,
                    3.2f, 2.0f, &p, seed + 600u + (uint32_t)i);
    }
}

// Tall branch: stump on the ground with several long limbs leaning the same
// way towards the surface, short roots splayed at the base.
static void branch_tall(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    int dir = mirror_dir(seed);
    WoodPal p = wood_pal(col, RGB565(215, 170, 125), 0.12f, 1);
    float sx = (float)(cx + dir * 14), sy = (float)cy;
    for (int i = 0; i < 4; i++) { // roots
        float a = (dir > 0 ? 0.0f : FM_PI) + (float)dir * rng_frange(&r, -0.3f, 0.6f);
        wood_limb(cv, &r, &p, sx, sy - 3.0f, a, rng_frange(&r, 9, 15), 1.4f, 0, 0.4f, seed + 900u + (uint32_t)i);
    }
    int n = rng_range(&r, 3, 4);
    for (int i = 0; i < n; i++) {
        float a = -FM_PI / 2 - (float)dir * (0.15f + 0.3f * (float)i + rng_frange(&r, -0.08f, 0.08f));
        float len = rng_frange(&r, 70.0f, 98.0f) * (1.0f - 0.12f * (float)i);
        wood_limb(cv, &r, &p, sx + rng_frange(&r, -2, 2), sy - 6.0f, a, len, 2.2f, 1, 0.22f, seed + (uint32_t)i * 41u);
    }
    for (int i = 0; i < 2; i++) // long thin low limbs
        wood_limb(cv, &r, &p, sx, sy - 5.0f, -FM_PI / 2 - (float)dir * rng_frange(&r, 0.95f, 1.15f), rng_frange(&r, 24, 32),
                  1.2f, 1, 0.35f, seed + 700u + (uint32_t)i);
    wood_stroke(cv, sx + (float)dir * 3.0f, sy + 1.0f, sx, sy - 4.0f, sx - (float)dir * 2.0f, sy - 9.0f, 5.0f, 3.5f, &p,
                seed + 1u); // stump
}

// Arching roots: bundle of intertwined strands forming a bridge between two
// feet that splay into the substrate, with twiggy branches on top.
static void roots(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    WoodPal p = wood_pal(col, RGB565(195, 125, 85), 0.08f, 0);
    float x0 = (float)cx - 30.0f, y0 = (float)cy + 1.0f, x2 = (float)cx + 30.0f, y2 = (float)cy + 1.0f;
    float x1 = (float)cx + rng_frange(&r, -6, 6), y1 = (float)cy - rng_frange(&r, 58.0f, 64.0f);
    int n = 7;
    for (int j = 0; j < n; j++) {
        float o = ((float)j - (float)(n - 1) * 0.5f) * 1.1f;
        float ph = rng_frange(&r, 0, FM_TAU), fr = rng_frange(&r, 9.0f, 16.0f);
        float th = rng_frange(&r, 0.8f, 1.2f);
        float qx = 0, qy = 0;
        int steps = 40;
        for (int i = 0; i <= steps; i++) {
            float t = (float)i / (float)steps, bx, by, a;
            bez_at(x0, y0, x1, y1, x2, y2, t, &bx, &by, &a);
            float sp = 1.0f + 3.5f * ((t < 0.18f ? 0.18f - t : 0.0f) + (t > 0.82f ? t - 0.82f : 0.0f)) / 0.18f;
            float off = (o + fm_sin(t * fr + ph) * 1.4f) * sp;
            float px = bx - fm_sin(a) * off, py = by + fm_cos(a) * off;
            if (py > (float)cy + 2.0f) py = (float)cy + 2.0f;
            if (i > 0)
                wood_stroke(cv, qx, qy, (qx + px) * 0.5f, (qy + py) * 0.5f, px, py, th, th, &p, seed + (uint32_t)j * 13u);
            qx = px;
            qy = py;
        }
    }
    int tw = rng_range(&r, 4, 6);
    for (int i = 0; i < tw; i++) {
        float t = rng_frange(&r, 0.2f, 0.8f), bx, by, a;
        bez_at(x0, y0, x1, y1, x2, y2, t, &bx, &by, &a);
        float ang = -FM_PI / 2 + (t - 0.5f) * 2.4f + rng_frange(&r, -0.3f, 0.3f);
        wood_limb(cv, &r, &p, bx, by - 2.0f, ang, rng_frange(&r, 10, 20), 1.2f, 2, 0.5f, seed + 300u + (uint32_t)i);
    }
}

// Catappa leaves lying on the substrate: big oval leaves with a midrib, side
// veins and dried, mottled tones.
static void leaves(Canvas *cv, int cx, int cy, uint16_t col, uint32_t seed)
{
    Rng r;
    rng_seed(&r, seed);
    int n = rng_range(&r, 3, 4);
    for (int i = 0; i < n; i++) {
        int lx = cx + rng_range(&r, -16, 16), ly = cy + rng_range(&r, -1, 1);
        int rx = rng_range(&r, 9, 12), ry = rng_range(&r, 3, 5);
        float ang = rng_frange(&r, -0.22f, 0.22f) + ((hash32(seed + (uint32_t)i) & 1) ? FM_PI : 0.0f);
        uint16_t base = (i & 1) ? c565_lerp(col, RGB565(210, 165, 100), (uint8_t)rng_range(&r, 40, 150))
                                : c565_lerp(col, RGB565(95, 52, 26), (uint8_t)rng_range(&r, 0, 110));
        uint16_t vein = c565_scale(base, 150);
        float ca = fm_cos(ang), sa = fm_sin(ang);
        int R = rx + 1;
        int lcy = ly - ry / 2;
        uint32_t ls = seed + (uint32_t)i * 71u;
        for (int dy = -R; dy <= R; dy++)
            for (int dx = -R; dx <= R; dx++) {
                float u = (float)dx * ca + (float)dy * sa, v = -(float)dx * sa + (float)dy * ca;
                float un = u / (float)rx;
                if (un < -1.0f || un > 1.0f) continue;
                float half = (float)ry * fm_sqrt(1.0f - un * un) * (un > 0 ? 1.0f - 0.35f * un : 1.0f) + 0.3f;
                if (fabsf(v) > half) continue;
                uint16_t c = base;
                float m = vn2(ls, (float)dx * 0.4f, (float)dy * 0.7f);
                if (m > 0.66f) c = c565_lerp(c, RGB565(215, 175, 115), 70);
                else if (m < 0.3f) c = c565_scale(c, 200);
                if (dy < 0) c = c565_lerp(c, RGB565(225, 190, 130), 30);
                float sv = u * 0.6f - fabsf(v) * 1.3f + 30.0f;
                if (ry >= 3 && sv - 3.0f * floorf(sv / 3.0f) < 0.5f) c = c565_scale(c, 205);
                if (fabsf(v) < 0.5f) c = vein;
                else if (fabsf(v) > half - 0.8f) c = c565_scale(c, 185);
                dpset(cv, lx + dx, lcy + dy, c);
            }
        for (int k = 1; k <= 3; k++) dpsetf(cv, (float)lx - ca * (float)(rx + k), (float)lcy - sa * (float)(rx + k), vein); // stalk
    }
}

static void draw_shape(Canvas *cv, int type, int x, int fy, uint16_t color, uint32_t seed)
{
    switch (type) {
    case DECOR_ROCK_BIG: rock(cv, x, fy + 4, 20, 24, color, seed); break;
    case DECOR_DRAGON_STONE: dragon_stone(cv, x, fy + 3, color, seed); break;
    case DECOR_SEIRYU_STONE: seiryu_stone(cv, x, fy + 3, color, seed); break;
    case DECOR_ROCK_CLUSTER: rock_cluster(cv, x, fy + 2, color, seed); break;
    case DECOR_ROCK_SLATE: rock_slate(cv, x, fy + 2, color, seed); break;
    case DECOR_DRIFTWOOD: driftwood(cv, x, fy + 1, color, seed); break;
    case DECOR_SPIDERWOOD: spiderwood(cv, x, fy + 1, color, seed); break;
    case DECOR_BRANCH: branch_tall(cv, x, fy + 2, color, seed); break;
    case DECOR_ROOTS: roots(cv, x, fy + 1, color, seed); break;
    case DECOR_LEAVES: leaves(cv, x, fy + 1, color, seed); break;
    default: break;
    }
}

// Scratch sprite for rotated/scaled decorations: the shape is drawn upright at
// its original size around a pivot, then copied into the cache scaled and
// rotated (nearest neighbour). FG_KEY marks empty pixels (dpset never writes it).
#define SCR_W 128
#define SCR_H 136
#define SCR_PX 64  // pivot (base point) inside the scratch sprite
#define SCR_PY 124
static uint16_t *s_scr;

static void draw_transformed(Canvas *cv, const DecorCfg *d, int fy, uint32_t seed)
{
    if (!s_scr) s_scr = (uint16_t *)hal_alloc_large(SCR_W * SCR_H * sizeof(uint16_t));
    if (!s_scr) { // no memory: draw it plain rather than not at all
        draw_shape(cv, d->type, d->x, fy, d->color, seed);
        return;
    }
    Canvas sc;
    canvas_init(&sc, s_scr, SCR_W, SCR_H);
    canvas_fill(&sc, FG_KEY);
    draw_shape(&sc, d->type, SCR_PX, SCR_PY, d->color, seed);
    float k = (float)decor_scale_pct(d) / 100.0f;
    float a = deg2rad(d->rot), c = fm_cos(a), s = fm_sin(a);
    float ck = c * k, sk = s * k;          // forward: source -> destination
    float ic = c / k, is = s / k;          // inverse: destination -> source
    // destination box: the transformed scratch rectangle, clipped to the canvas
    const float qx[4] = {-SCR_PX, SCR_W - SCR_PX, SCR_W - SCR_PX, -SCR_PX};
    const float qy[4] = {-SCR_PY, -SCR_PY, SCR_H - SCR_PY, SCR_H - SCR_PY};
    float mnx = 1e9f, mny = 1e9f, mxx = -1e9f, mxy = -1e9f;
    for (int i = 0; i < 4; i++) {
        float ox, oy;
        rot2(ck, sk, qx[i], qy[i], &ox, &oy);
        if (ox < mnx) mnx = ox;
        if (ox > mxx) mxx = ox;
        if (oy < mny) mny = oy;
        if (oy > mxy) mxy = oy;
    }
    int ox0 = d->x + (int)floorf(mnx), ox1 = d->x + (int)ceilf(mxx);
    int oy0 = fy + (int)floorf(mny), oy1 = fy + (int)ceilf(mxy);
    if (ox0 < cv->cx0) ox0 = cv->cx0;
    if (oy0 < cv->cy0) oy0 = cv->cy0;
    if (ox1 >= cv->cx1) ox1 = cv->cx1 - 1;
    if (oy1 >= cv->cy1) oy1 = cv->cy1 - 1;
    for (int oy = oy0; oy <= oy1; oy++) {
        float ry = (float)(oy - fy);
        for (int ox = ox0; ox <= ox1; ox++) {
            float rx = (float)(ox - d->x);
            // inverse transform: destination -> source
            int sx = (int)floorf(ic * rx + is * ry + 0.5f) + SCR_PX;
            int sy = (int)floorf(-is * rx + ic * ry + 0.5f) + SCR_PY;
            if (sx < 0 || sy < 0 || sx >= SCR_W || sy >= SCR_H) continue;
            uint16_t v = s_scr[sy * SCR_W + sx];
            if (v != FG_KEY) cv->px[oy * cv->w + ox] = v;
        }
    }
}

static void draw_one(Canvas *cv, const DecorCfg *d, int idx)
{
    int fy = decor_base_y(d);
    uint32_t seed = (uint32_t)(idx * 977 + d->x * 13 + d->type);
    if (decor_plain(d)) draw_shape(cv, d->type, d->x, fy, d->color, seed);
    else draw_transformed(cv, d, fy, seed);
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
