// Scene orchestrator.
//
// Layer order (back to front), see spec §6:
//   bg cache  = water gradient + backdrop + back static decor           (rebuilt on config change)
//   rays, particles, back plants, food, creatures, bubbles
//   fg cache  = substrate + front static decor (colour-keyed) (rebuilt on config change)
//   front plants, surface, edit highlight, brightness + fade
#include "scene.h"
#include <string.h>
#include "../engine/fixmath.h"
#include "../engine/hal.h"
#include "creatures.h"
#include "world.h"

static AquariumConfig s_cfg;
static AquariumConfig s_pending;
static Canvas s_bg, s_fg;
static Canvas s_wb;          // water gradient + backdrop, rebuilt only when they change
static uint16_t s_wb_top, s_wb_bottom;
static int s_wb_backdrop = -1;
static Canvas s_tbg, s_tfg; // thumbnail scratch (allocated lazily)
static int s_fg_top;        // first row containing foreground pixels
static float s_time;
static uint32_t s_spawn_seed = 12345;

static int s_fade_phase; // 0 none, 1 fading out, 2 fading in
static float s_fade_t;
static bool s_edit;
static int s_sel_kind, s_sel_index;

#define FADE_TIME 0.35f

static bool alloc_canvas(Canvas *c)
{
    uint16_t *px = (uint16_t *)hal_alloc_large(CANVAS_W * CANVAS_H * sizeof(uint16_t));
    if (!px) return false;
    canvas_init(c, px, CANVAS_W, CANVAS_H);
    return true;
}

bool scene_init(void)
{
    fm_init();
    if (!alloc_canvas(&s_bg) || !alloc_canvas(&s_fg)) return false;
    if (!alloc_canvas(&s_wb)) s_wb.px = NULL; // optional cache: without it the backdrop is redrawn every rebuild
    creatures_init();
    bubbles_init();
    return true;
}

static void build_static(const AquariumConfig *c, Canvas *bg, Canvas *fg, int *fg_top)
{
    substrate_profile(c, g_profile);
    if (s_wb.px && bg == &s_bg) { // live scene only: thumbnails must not evict it
        // the backdrop is the slow part of a rebuild: keep it while only plants/decor/substrate change
        if (s_wb_backdrop != c->backdrop || s_wb_top != c->water_top || s_wb_bottom != c->water_bottom) {
            water_build(&s_wb, c);
            backdrop_draw(&s_wb, c);
            s_wb_backdrop = c->backdrop, s_wb_top = c->water_top, s_wb_bottom = c->water_bottom;
        }
        canvas_copy(bg, &s_wb);
    } else {
        water_build(bg, c);
        backdrop_draw(bg, c);
    }
    decor_draw_static(bg, c, LAYER_BACK);
    canvas_fill(fg, FG_KEY);
    substrate_draw(fg, c, g_profile);
    decor_draw_static(fg, c, LAYER_FRONT);
    // first row with any foreground pixel (front decor may rise above the substrate)
    int top = CANVAS_H;
    for (int y = 0; y < CANVAS_H && top == CANVAS_H; y++) {
        const uint16_t *row = fg->px + y * CANVAS_W;
        for (int x = 0; x < CANVAS_W; x++)
            if (row[x] != FG_KEY) { top = y; break; }
    }
    *fg_top = top;
}

static void apply_now(const AquariumConfig *c, unsigned what)
{
    s_cfg = *c;
    if (what & SC_STATIC) build_static(&s_cfg, &s_bg, &s_fg, &s_fg_top);
    if (what & (SC_STATIC | SC_PLANTS)) plants_build(&s_cfg);
    if (what & SC_PARTICLES) particles_init(&s_cfg, s_cfg.substrate_seed ^ 0x1234u);
    if (what & SC_CREATURES) creatures_spawn(&s_cfg, s_spawn_seed++);
}

void scene_set_config(const AquariumConfig *c, bool fade)
{
    if (!fade) {
        apply_now(c, SC_ALL);
        bubbles_init();
        return;
    }
    s_pending = *c;
    s_fade_phase = 1;
    s_fade_t = 0;
}

void scene_config_changed(const AquariumConfig *c, unsigned what) { apply_now(c, what); }

void scene_set_edit(bool on, int sel_kind, int sel_index)
{
    s_edit = on;
    s_sel_kind = sel_kind;
    s_sel_index = sel_index;
}

int scene_pick(int x, int y, int *index)
{
    int d = decor_hit(&s_cfg, x, y);
    int p = plants_hit(x, y);
    // prefer the front-most layer
    if (d >= 0 && (p < 0 || s_cfg.decor[d].layer >= s_cfg.plants[p].layer)) {
        *index = d;
        return SEL_DECOR;
    }
    if (p >= 0) {
        *index = p;
        return SEL_PLANT;
    }
    return SEL_NONE;
}

void scene_update(float dt)
{
    if (s_fade_phase) {
        s_fade_t += dt;
        if (s_fade_phase == 1 && s_fade_t >= FADE_TIME) {
            apply_now(&s_pending, SC_ALL);
            bubbles_init();
            s_fade_phase = 2;
            s_fade_t = 0;
        } else if (s_fade_phase == 2 && s_fade_t >= FADE_TIME) {
            s_fade_phase = 0;
        }
    }
    float sdt = dt * (float)s_cfg.sim_speed / 100.0f;
    if (s_edit) sdt *= 0.2f; // creatures slow down while editing
    s_time += sdt;
    particles_update(sdt);
    bubbles_update(&s_cfg, sdt);
    food_update(sdt);
    creatures_update(sdt);
}

static void draw_selection(Canvas *cv)
{
    int x0, y0, x1, y1;
    if (s_sel_kind == SEL_DECOR && s_sel_index < s_cfg.decor_count) {
        decor_bounds(&s_cfg.decor[s_sel_index], &x0, &y0, &x1, &y1);
    } else if (s_sel_kind == SEL_PLANT && s_sel_index < s_cfg.plant_count) {
        plants_bounds(s_sel_index, &x0, &y0, &x1, &y1);
    } else {
        return;
    }
    // dashed outline
    uint16_t col = RGB565(255, 240, 120);
    for (int x = x0; x <= x1; x++)
        if (((x + (int)(s_time * 20)) & 3) < 2) {
            canvas_pset(cv, x, y0, col);
            canvas_pset(cv, x, y1, col);
        }
    for (int y = y0; y <= y1; y++)
        if (((y + (int)(s_time * 20)) & 3) < 2) {
            canvas_pset(cv, x0, y, col);
            canvas_pset(cv, x1, y, col);
        }
}

void scene_render(Canvas *out)
{
    const float t = s_time;
    canvas_copy(out, &s_bg);
    water_rays_draw(out, &s_cfg, t, 1.0f);
    particles_draw(out, t);
    plants_draw(out, LAYER_BACK, t, s_sel_kind == SEL_PLANT ? s_sel_index : -1);
    decor_draw_dynamic(out, &s_cfg, LAYER_BACK, t);
    food_draw(out);
    creatures_draw(out, t);
    bubbles_draw(out);
    canvas_blit_keyed(out, &s_fg, FG_KEY, s_fg_top);
    plants_draw(out, LAYER_FRONT, t, s_sel_kind == SEL_PLANT ? s_sel_index : -1);
    decor_draw_dynamic(out, &s_cfg, LAYER_FRONT, t);
    water_surface_draw(out, &s_cfg, t);
    if (s_edit) draw_selection(out);

    float k = (float)s_cfg.brightness / 100.0f;
    if (s_fade_phase == 1) k *= 1.0f - s_fade_t / FADE_TIME;
    else if (s_fade_phase == 2) k *= s_fade_t / FADE_TIME;
    if (k < 0.995f) canvas_darken(out, (uint32_t)(fm_clamp(k, 0, 1) * 256.0f));
}

void scene_tap(int x, int y)
{
    if (creatures_scare(x, y)) return; // tapped on/near a fish: it flees
    if (y > world_floor(x)) return;    // tapped the substrate: nothing
    food_drop(x, y);
}

int scene_creature_count(void) { return creatures_count(); }

void scene_render_thumbnail(const AquariumConfig *c, Canvas *thumb)
{
    if (!s_tbg.px && (!alloc_canvas(&s_tbg) || !alloc_canvas(&s_tfg))) return;
    static int16_t saved[CANVAS_W];
    memcpy(saved, g_profile, sizeof(saved));
    int top;
    build_static(c, &s_tbg, &s_tfg, &top);
    plants_build(c);
    water_rays_draw(&s_tbg, c, 0, 1.0f);
    plants_draw(&s_tbg, LAYER_BACK, 0, -1);
    decor_draw_dynamic(&s_tbg, c, LAYER_BACK, 0);
    canvas_blit_keyed(&s_tbg, &s_tfg, FG_KEY, top);
    plants_draw(&s_tbg, LAYER_FRONT, 0, -1);
    decor_draw_dynamic(&s_tbg, c, LAYER_FRONT, 0);
    water_surface_draw(&s_tbg, c, 0);
    canvas_downsample(thumb, &s_tbg, CANVAS_W / thumb->w);
    // restore the live scene state
    memcpy(g_profile, saved, sizeof(saved));
    plants_build(&s_cfg);
}
