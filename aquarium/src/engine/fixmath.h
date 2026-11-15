// Fast trig via a precomputed table (the S3 has a single-precision FPU, but
// sinf/cosf from libm are slow). Call fm_init() once at startup.
#pragma once
#include <stdint.h>

#define FM_PI 3.14159265f
#define FM_TAU 6.28318531f
#define FM_TABLE_SIZE 512

extern float fm_sin_table[FM_TABLE_SIZE + 1];

void fm_init(void);

// Linear-interpolated table sine, any input range.
static inline float fm_sin(float x)
{
    float t = x * (FM_TABLE_SIZE / FM_TAU);
    int32_t i = (int32_t)t;
    if (t < 0 && (float)i != t) i--;
    float f = t - (float)i;
    i &= FM_TABLE_SIZE - 1;
    return fm_sin_table[i] + (fm_sin_table[i + 1] - fm_sin_table[i]) * f;
}

static inline float fm_cos(float x) { return fm_sin(x + FM_PI * 0.5f); }

static inline float fm_clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline int im_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float fm_abs(float v) { return v < 0 ? -v : v; }

// Fast inverse square root free approximation good enough for steering.
static inline float fm_sqrt(float x)
{
    if (x <= 0) return 0;
    union { float f; uint32_t i; } u = {x};
    u.i = 0x1FBD1DF5u + (u.i >> 1);
    // two Newton steps
    u.f = 0.5f * (u.f + x / u.f);
    u.f = 0.5f * (u.f + x / u.f);
    return u.f;
}
