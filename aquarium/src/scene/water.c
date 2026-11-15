// Water: dithered gradient (cached), god rays, surface line, drifting particles.
#include <string.h>
#include "../engine/fixmath.h"
#include "../engine/rng.h"
#include "world.h"

static const uint8_t BAYER4[4][4] = {
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5},
};

// Quantise an 8.8 fixed-point channel to `bits` with ordered dithering.
static inline uint32_t dither_q(uint32_t v88, int bits, uint8_t th)
{
    // v88: channel 0..255 in 8.8 fixed point
    uint32_t maxv = (1u << bits) - 1;
    uint32_t scaled = (v88 * maxv) / 255; // still 8.8
    uint32_t q = (scaled + ((uint32_t)th << 4)) >> 8;
    return q > maxv ? maxv : q;
}

void water_build(Canvas *bg, const AquariumConfig *c)
{
    const int h = bg->h, w = bg->w;
    uint32_t tr = c565_r8(c->water_top), tg = c565_g8(c->water_top), tb = c565_b8(c->water_top);
    uint32_t br = c565_r8(c->water_bottom), bgc = c565_g8(c->water_bottom), bb = c565_b8(c->water_bottom);

    // Air above the surface: almost black, slightly tinted.
    uint16_t air = c565_scale(c->water_bottom, 90);
    canvas_fill_rect(bg, 0, 0, w, SURFACE_Y, air);

    for (int y = SURFACE_Y; y < h; y++) {
        // Non-linear gradient: brighter near the surface, long dark tail.
        float f = (float)(y - SURFACE_Y) / (float)(h - SURFACE_Y);
        float k = f * (2.0f - f); // ease-out
        uint32_t t = (uint32_t)(k * 256.0f);
        uint32_t r88 = tr * (256 - t) + br * t;
        uint32_t g88 = tg * (256 - t) + bgc * t;
        uint32_t b88 = tb * (256 - t) + bb * t;
        uint16_t *row = bg->px + y * w;
        for (int x = 0; x < w; x++) {
            uint8_t th = BAYER4[y & 3][x & 3];
            row[x] = (uint16_t)((dither_q(r88, 5, th) << 11) | (dither_q(g88, 6, th) << 5) | dither_q(b88, 5, th));
        }
    }

    // Faint static specks for texture (very subtle, like the reference).
    Rng r;
    rng_seed(&r, 0xBEEF);
    for (int i = 0; i < 220; i++) {
        int x = rng_range(&r, 0, w - 1), y = rng_range(&r, SURFACE_Y + 2, h - 1);
        canvas_pblend(bg, x, y, RGB565(140, 190, 200), 18);
    }
}

void water_rays_draw(Canvas *cv, const AquariumConfig *c, float t, float intensity)
{
    if (!c->light_rays || intensity <= 0.01f) return;
    uint16_t col = c565_lerp(c->water_top, RGB565(150, 205, 215), 150);
    const int n = c->light_rays;
    const int bottom = cv->h - c->substrate_height / 2;
    for (int i = 0; i < n; i++) {
        float ph = (float)i * 2.17f;
        float x0 = (float)((i * 2 + 1) * CANVAS_W) / (float)(2 * n) - 70.0f + fm_sin(t * 0.07f + ph) * 22.0f;
        float slope = 0.30f + 0.05f * fm_sin(ph);
        float w0 = 14.0f + 7.0f * (float)((i * 7) % 3);
        float a0 = (46.0f + 16.0f * fm_sin(t * 0.23f + ph * 1.7f)) * intensity;
        if (a0 < 2) continue;
        for (int y = SURFACE_Y + 1; y < bottom; y++) {
            float d = (float)(y - SURFACE_Y) / (float)(bottom - SURFACE_Y);
            float a = a0 * (1.0f - d);
            if (a < 2.5f) break;
            float xc = x0 + (float)(y - SURFACE_Y) * slope;
            float ww = w0 + (float)(y - SURFACE_Y) * 0.12f;
            int xa = (int)(xc - ww * 0.5f), xb = (int)(xc + ww * 0.5f);
            uint8_t ac = (uint8_t)a, ae = (uint8_t)(a * 0.5f);
            canvas_blend_rect(cv, xa - 3, y, 3, 1, col, ae);
            canvas_blend_rect(cv, xa, y, xb - xa, 1, col, ac);
            canvas_blend_rect(cv, xb, y, 3, 1, col, ae);
        }
    }
}

void water_surface_draw(Canvas *cv, const AquariumConfig *c, float t)
{
    if (!c->show_surface) return;
    uint16_t line = c565_lerp(c->water_top, RGB565(170, 220, 230), 170);
    uint16_t under = c565_lerp(c->water_top, RGB565(120, 180, 200), 70);
    int prev = -1;
    for (int x = 0; x < cv->w; x++) {
        float fx = (float)x;
        float y = (float)SURFACE_Y + fm_sin(fx * 0.075f + t * 1.4f) * 1.1f + fm_sin(fx * 0.19f - t * 0.9f) * 0.6f;
        int yi = (int)(y + 0.5f) - 1;
        canvas_pblend(cv, x, yi, line, 200);
        if (prev >= 0 && (yi - prev > 1 || prev - yi > 1)) canvas_pblend(cv, x, (prev + yi) / 2, line, 120);
        canvas_pblend(cv, x, yi + 1, under, 90);
        // Caustic sparkle along the line.
        if (((x * 13 + (int)(t * 6.0f)) % 41) == 0) canvas_pblend(cv, x, yi, RGB565(230, 250, 255), 220);
        prev = yi;
    }
}

// ---------------------------------------------------------------- particles

#define PARTICLE_MAX 80
typedef struct {
    float x, y, vx, vy, ph;
    uint8_t bright;
} Particle;

static Particle s_part[PARTICLE_MAX];
static int s_npart;
static Rng s_prng;

void particles_init(const AquariumConfig *c, uint32_t seed)
{
    rng_seed(&s_prng, seed);
    s_npart = c->particles * PARTICLE_MAX / 100;
    for (int i = 0; i < s_npart; i++) {
        Particle *p = &s_part[i];
        p->x = rng_frange(&s_prng, 0, CANVAS_W);
        p->y = rng_frange(&s_prng, SURFACE_Y + 4, CANVAS_H - 10);
        p->vx = rng_frange(&s_prng, -1.5f, 1.5f);
        p->vy = rng_frange(&s_prng, -0.6f, 1.2f);
        p->ph = rng_frange(&s_prng, 0, FM_TAU);
        p->bright = (uint8_t)rng_range(&s_prng, 40, 130);
    }
}

void particles_update(float dt)
{
    for (int i = 0; i < s_npart; i++) {
        Particle *p = &s_part[i];
        p->ph += dt * 0.7f;
        p->x += (p->vx + fm_sin(p->ph) * 1.2f) * dt;
        p->y += (p->vy + fm_cos(p->ph * 0.8f) * 0.6f) * dt;
        if (p->x < 0) p->x += CANVAS_W;
        if (p->x >= CANVAS_W) p->x -= CANVAS_W;
        if (p->y < SURFACE_Y + 3 || p->y > world_floor((int)p->x) - 1) {
            p->y = rng_frange(&s_prng, SURFACE_Y + 4, CANVAS_H * 0.6f);
            p->x = rng_frange(&s_prng, 0, CANVAS_W);
        }
    }
}

void particles_draw(Canvas *cv, float t)
{
    for (int i = 0; i < s_npart; i++) {
        const Particle *p = &s_part[i];
        float tw = 0.6f + 0.4f * fm_sin(t * 1.3f + p->ph * 3.0f);
        canvas_pblend(cv, (int)p->x, (int)p->y, RGB565(200, 230, 235), (uint8_t)(p->bright * tw));
    }
}
