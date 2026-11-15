// Deterministic PRNG (xorshift32) + hash-based 1D value noise. Portable.
#pragma once
#include <stdint.h>

typedef struct {
    uint32_t s;
} Rng;

static inline void rng_seed(Rng *r, uint32_t seed) { r->s = seed ? seed : 0x9E3779B9u; }

static inline uint32_t rng_next(Rng *r)
{
    uint32_t x = r->s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return r->s = x;
}

// Integer in [lo, hi] inclusive.
static inline int rng_range(Rng *r, int lo, int hi)
{
    if (hi <= lo) return lo;
    return lo + (int)(rng_next(r) % (uint32_t)(hi - lo + 1));
}

// Float in [0, 1).
static inline float rng_f01(Rng *r) { return (float)(rng_next(r) >> 8) * (1.0f / 16777216.0f); }

static inline float rng_frange(Rng *r, float lo, float hi) { return lo + (hi - lo) * rng_f01(r); }

static inline uint32_t hash32(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x;
}

// Smooth 1D value noise in [0,1). `x` in lattice units.
float noise1d(uint32_t seed, float x);
// Fractal (3 octaves) value noise in [0,1).
float fbm1d(uint32_t seed, float x);
