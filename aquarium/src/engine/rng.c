#include "rng.h"

static float lattice(uint32_t seed, int32_t i)
{
    return (float)(hash32((uint32_t)i * 0x9E3779B1u ^ seed) >> 8) * (1.0f / 16777216.0f);
}

float noise1d(uint32_t seed, float x)
{
    int32_t i = (int32_t)x;
    if (x < 0 && (float)i != x) i--;
    float f = x - (float)i;
    float t = f * f * (3.0f - 2.0f * f); // smoothstep
    float a = lattice(seed, i), b = lattice(seed, i + 1);
    return a + (b - a) * t;
}

float fbm1d(uint32_t seed, float x)
{
    return (noise1d(seed, x) * 0.6f + noise1d(seed + 17u, x * 2.0f) * 0.28f + noise1d(seed + 59u, x * 4.0f) * 0.12f);
}
