// RGB565 colour helpers. Portable, header-only.
#pragma once
#include <stdint.h>

#define RGB565(r, g, b) \
    ((uint16_t)((((unsigned)(r) & 0xF8u) << 8) | (((unsigned)(g) & 0xFCu) << 3) | ((unsigned)(b) >> 3)))

static inline uint8_t c565_r8(uint16_t c) { return (uint8_t)(((c >> 11) & 31) * 255 / 31); }
static inline uint8_t c565_g8(uint16_t c) { return (uint8_t)(((c >> 5) & 63) * 255 / 63); }
static inline uint8_t c565_b8(uint16_t c) { return (uint8_t)((c & 31) * 255 / 31); }

// Alpha blend src over dst, alpha 0..255 (quantised to 5 bits).
// Classic "spread the channels into a 32-bit word" trick: one multiply per pixel.
static inline uint16_t c565_blend(uint16_t dst, uint16_t src, uint8_t alpha)
{
    uint32_t a = ((uint32_t)alpha + 4) >> 3;
    uint32_t s = (src | ((uint32_t)src << 16)) & 0x07E0F81Fu;
    uint32_t d = (dst | ((uint32_t)dst << 16)) & 0x07E0F81Fu;
    d += ((s - d) * a) >> 5;
    d &= 0x07E0F81Fu;
    return (uint16_t)(d | (d >> 16));
}

// Linear interpolation between two colours, t = 0..255.
static inline uint16_t c565_lerp(uint16_t a, uint16_t b, uint8_t t) { return c565_blend(a, b, t); }

// Saturating additive blend (used for glow effects).
static inline uint16_t c565_add(uint16_t a, uint16_t b)
{
    uint32_t r = (uint32_t)(a >> 11) + (b >> 11);
    uint32_t g = (uint32_t)((a >> 5) & 63) + ((b >> 5) & 63);
    uint32_t bl = (uint32_t)(a & 31) + (b & 31);
    if (r > 31) r = 31;
    if (g > 63) g = 63;
    if (bl > 31) bl = 31;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

// Scale brightness, s = 0..256 (256 = unchanged).
static inline uint16_t c565_scale(uint16_t c, uint32_t s)
{
    uint32_t x = (c | ((uint32_t)c << 16)) & 0x07E0F81Fu;
    x = ((x * (s >> 3)) >> 5) & 0x07E0F81Fu;
    return (uint16_t)(x | (x >> 16));
}

// Build a colour from HSV (h 0..255, s 0..255, v 0..255).
static inline uint16_t c565_hsv(uint8_t h, uint8_t s, uint8_t v)
{
    uint8_t region = h / 43;
    uint8_t rem = (uint8_t)((h - region * 43) * 6);
    uint8_t p = (uint8_t)((v * (255 - s)) >> 8);
    uint8_t q = (uint8_t)((v * (255 - ((s * rem) >> 8))) >> 8);
    uint8_t t = (uint8_t)((v * (255 - ((s * (255 - rem)) >> 8))) >> 8);
    switch (region) {
    case 0: return RGB565(v, t, p);
    case 1: return RGB565(q, v, p);
    case 2: return RGB565(p, v, t);
    case 3: return RGB565(p, q, v);
    case 4: return RGB565(t, p, v);
    default: return RGB565(v, p, q);
    }
}
