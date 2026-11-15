// Procedural substrate: hilly 1D-noise profile + textures (sand, gravel,
// dark rock with ring pattern, pebbles). Rendered once into the fg cache.
#include "../engine/fixmath.h"
#include "../engine/rng.h"
#include "world.h"

int16_t g_profile[CANVAS_W];

void substrate_profile(const AquariumConfig *c, int16_t *profile)
{
    if (c->substrate_type == SUB_NONE) {
        for (int x = 0; x < CANVAS_W; x++) profile[x] = CANVAS_H;
        return;
    }
    const float rough = (float)c->substrate_roughness / 100.0f;
    const float base = (float)(CANVAS_H - c->substrate_height);
    const float amp = (float)c->substrate_height * (0.35f + 0.75f * rough);
    for (int x = 0; x < CANVAS_W; x++) {
        float n = fbm1d(c->substrate_seed, (float)x / 70.0f) - 0.5f;
        float fine = (noise1d(c->substrate_seed ^ 0x55u, (float)x / 9.0f) - 0.5f) * 3.0f * rough;
        int y = (int)(base + n * amp * 1.6f + fine);
        profile[x] = (int16_t)im_clamp(y, SURFACE_Y + 40, CANVAS_H - 4);
    }
}

static inline void put(Canvas *fg, int x, int y, uint16_t col)
{
    if (col == FG_KEY) col ^= 1; // never write the colour key
    canvas_pset(fg, x, y, col);
}

static inline bool in_ground(const int16_t *profile, int x, int y, int margin)
{
    return x >= 0 && x < CANVAS_W && y >= profile[x] + margin && y < CANVAS_H;
}

void substrate_draw(Canvas *fg, const AquariumConfig *c, const int16_t *profile)
{
    if (c->substrate_type == SUB_NONE) return;
    const uint16_t base = c->substrate_color;
    const uint16_t light = c565_lerp(base, RGB565(170, 190, 200), 70);
    const uint16_t dark = c565_scale(base, 150);
    const uint32_t seed = c->substrate_seed;

    // 1) base fill with depth shading and per-pixel noise
    for (int x = 0; x < CANVAS_W; x++) {
        for (int y = profile[x]; y < CANVAS_H; y++) {
            int d = y - profile[x];
            uint32_t h = hash32((uint32_t)(x * 7919 + y * 104729) ^ seed);
            uint16_t col = c565_scale(base, (uint32_t)(256 - (d > 40 ? 40 : d) * 2));
            switch (c->substrate_type) {
            case SUB_SAND: {
                int n = (int)(h & 15) - 7;
                col = c565_scale(base, (uint32_t)(240 + n * 3 - (d > 30 ? 30 : d)));
                if (((y + (int)(fm_sin((float)x * 0.12f) * 2.0f)) % 7) == 0 && d > 2 && (h & 1)) col = c565_scale(col, 238);
                break;
            }
            case SUB_GRAVEL: {
                uint32_t g = hash32((uint32_t)((x >> 1) * 31 + (y >> 1) * 977) ^ seed);
                static const uint32_t shades[4] = {170, 215, 250, 300};
                col = c565_scale(base, shades[g & 3] - (uint32_t)(d > 30 ? 30 : d));
                if ((h & 31) == 0) col = light;
                break;
            }
            case SUB_PEBBLES: col = c565_scale(base, 150 + (h & 15)); break;
            default: // dark rock
                if ((h & 7) == 0) col = c565_scale(col, 230);
                break;
            }
            put(fg, x, y, col);
        }
    }

    Rng r;
    rng_seed(&r, seed ^ 0xA5A5u);

    // 2) type-specific overlays
    if (c->substrate_type == SUB_DARK_ROCK) {
        // Ring pattern (like the reference): light outlines of overlapping circles.
        uint16_t ring = c565_lerp(base, RGB565(110, 140, 160), 50);
        uint16_t ring2 = c565_lerp(base, RGB565(90, 120, 140), 28);
        for (int i = 0; i < 110; i++) {
            int cx = rng_range(&r, -6, CANVAS_W + 6);
            int top = profile[im_clamp(cx, 0, CANVAS_W - 1)];
            int cy = rng_range(&r, top + 6, CANVAS_H + 4);
            int rad = rng_range(&r, 4, 9);
            int x = rad, y = 0, err = 1 - rad;
            uint16_t col = (i & 1) ? ring : ring2;
            while (x >= y) {
                const int pts[8][2] = {{cx + x, cy + y}, {cx + y, cy + x}, {cx - y, cy + x}, {cx - x, cy + y},
                                       {cx - x, cy - y}, {cx - y, cy - x}, {cx + y, cy - x}, {cx + x, cy - y}};
                for (int k = 0; k < 8; k++)
                    if (in_ground(profile, pts[k][0], pts[k][1], 3)) put(fg, pts[k][0], pts[k][1], col);
                y++;
                if (err < 0) err += 2 * y + 1;
                else { x--; err += 2 * (y - x) + 1; }
            }
        }
    } else if (c->substrate_type == SUB_PEBBLES) {
        for (int i = 0; i < 260; i++) {
            int cx = rng_range(&r, 0, CANVAS_W - 1);
            int cy = rng_range(&r, profile[cx] + 1, CANVAS_H + 2);
            int rx = rng_range(&r, 2, 4), ry = rng_range(&r, 1, 3);
            uint16_t col = c565_scale(base, (uint32_t)rng_range(&r, 190, 330));
            for (int yy = -ry; yy <= ry; yy++)
                for (int xx = -rx; xx <= rx; xx++) {
                    if (xx * xx * ry * ry + yy * yy * rx * rx > rx * rx * ry * ry) continue;
                    if (!in_ground(profile, cx + xx, cy + yy, 0)) continue;
                    uint16_t cc = (yy == -ry + 1 && xx < 0) ? c565_lerp(col, RGB565(255, 255, 255), 50)
                                  : (yy == ry) ? c565_scale(col, 170) : col;
                    put(fg, cx + xx, cy + yy, cc);
                }
        }
    } else if (c->substrate_type == SUB_GRAVEL || c->substrate_type == SUB_SAND) {
        for (int i = 0; i < 90; i++) {
            int x = rng_range(&r, 0, CANVAS_W - 1);
            int y = rng_range(&r, profile[x] + 2, CANVAS_H - 1);
            put(fg, x, y, c->substrate_type == SUB_SAND ? light : dark);
        }
    }

    // 3) highlighted top edge
    for (int x = 0; x < CANVAS_W; x++) {
        put(fg, x, profile[x], light);
        if ((hash32((uint32_t)x ^ seed) & 3) == 0) put(fg, x, profile[x] + 1, c565_lerp(base, light, 128));
    }
}
