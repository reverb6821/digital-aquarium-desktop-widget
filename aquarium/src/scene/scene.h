// Scene orchestrator: owns the layer caches and draws the layers in order.
#pragma once
#include <stdbool.h>
#include "../config/config.h"
#include "../engine/canvas.h"

// What changed in the config (bit mask for scene_config_changed).
enum {
    SC_STATIC = 1,    // water / substrate / static decor -> rebuild caches
    SC_PLANTS = 2,    // plant list or substrate profile -> rebuild plants
    SC_CREATURES = 4, // species list -> respawn
    SC_PARTICLES = 8,
    SC_ALL = 15,
};

// Edit-mode selection kinds.
enum { SEL_NONE = 0, SEL_PLANT, SEL_DECOR };

bool scene_init(void);
// Apply a whole new config (theme/preset change). With fade: dissolve via black.
void scene_set_config(const AquariumConfig *c, bool fade);
// Apply an incremental change (live preview from the UI).
void scene_config_changed(const AquariumConfig *c, unsigned what);
void scene_update(float dt);
void scene_render(Canvas *out);
// Tap on the aquarium: scare nearby fish or drop food.
void scene_tap(int x, int y);
void scene_set_edit(bool on, int sel_kind, int sel_index);
// Pick an editable object at canvas point; returns SEL_* and fills *index.
int scene_pick(int x, int y, int *index);
float scene_daylight(void);
int scene_creature_count(void);
// Render a 1/4-scale preview (100x60) of a config's static scene into thumb.
void scene_render_thumbnail(const AquariumConfig *c, Canvas *thumb);
