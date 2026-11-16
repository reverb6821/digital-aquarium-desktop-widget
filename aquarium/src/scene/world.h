// Shared world constants / state for the scene modules. Portable.
#pragma once
#include <stdint.h>
#include "../config/config.h"
#include "../engine/canvas.h"
#include "../engine/hal.h"

#define SURFACE_Y 12          // water line on the canvas
#define FG_KEY 0x0020         // colour key for the foreground cache (never drawn)

// Top y of the substrate for every canvas column (CANVAS_H if no substrate).
extern int16_t g_profile[CANVAS_W];

static inline int world_floor(int x)
{
    if (x < 0) x = 0;
    if (x >= CANVAS_W) x = CANVAS_W - 1;
    return g_profile[x];
}

// ---- water.c ----
void water_build(Canvas *bg, const AquariumConfig *c);
void water_rays_draw(Canvas *cv, const AquariumConfig *c, float t, float intensity);
void water_surface_draw(Canvas *cv, const AquariumConfig *c, float t);
void particles_init(const AquariumConfig *c, uint32_t seed);
void particles_update(float dt);
void particles_draw(Canvas *cv, float t);

// ---- substrate.c ----
void substrate_profile(const AquariumConfig *c, int16_t *profile);
void substrate_draw(Canvas *fg, const AquariumConfig *c, const int16_t *profile);

// ---- plants.c ----
void plants_build(const AquariumConfig *c);
void plants_draw(Canvas *cv, int layer, float t, int highlight_index);
// Plant index under canvas point (for edit mode), -1 if none.
int plants_hit(int x, int y);
// Canvas bounding box of a built plant (index as in the config), sway included.
void plants_bounds(int index, int *x0, int *y0, int *x1, int *y1);

// ---- decor.c ----
// Static decor (rocks, wood, shells...) into the caches; dynamic ones per frame.
void decor_draw_static(Canvas *cv, const AquariumConfig *c, int layer);
void decor_draw_dynamic(Canvas *cv, const AquariumConfig *c, int layer, float t);
int decor_hit(const AquariumConfig *c, int x, int y);
void decor_bounds(const DecorCfg *d, int *x0, int *y0, int *x1, int *y1);

// ---- bubbles.c ----
void bubbles_init(void);
void bubbles_update(const AquariumConfig *c, float dt);
void bubbles_draw(Canvas *cv);
void bubbles_burst(int x, int y, int n);

// ---- food.c ----
#define FOOD_MAX 24
typedef struct {
    float x, y, vy, age;
    uint8_t alive, resting;
} Food;
extern Food g_food[FOOD_MAX];
void food_drop(int x, int y);
void food_update(float dt);
void food_draw(Canvas *cv);
// Nearest food within radius, returns index or -1.
int food_nearest(float x, float y, float radius);
