#include "config.h"
#include <string.h>
#include "json.h"

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void config_sanitize(AquariumConfig *c)
{
    c->version = CONFIG_VERSION;
    c->theme_id[sizeof(c->theme_id) - 1] = 0;
    c->light_rays = (uint8_t)clampi(c->light_rays, 0, 5);
    c->particles = (uint8_t)clampi(c->particles, 0, 100);
    c->substrate_type = (uint8_t)clampi(c->substrate_type, 0, SUB_COUNT - 1);
    c->substrate_height = (uint8_t)clampi(c->substrate_height, 10, 60);
    c->substrate_roughness = (uint8_t)clampi(c->substrate_roughness, 0, 100);
    c->plant_count = (uint8_t)clampi(c->plant_count, 0, CFG_MAX_PLANTS);
    for (int i = 0; i < c->plant_count; i++) {
        PlantCfg *p = &c->plants[i];
        p->type = (uint8_t)clampi(p->type, 0, PLANT_COUNT - 1);
        p->x = (int16_t)clampi(p->x, 0, 399);
        p->layer = p->layer ? LAYER_FRONT : LAYER_BACK;
        p->height = (uint8_t)clampi(p->height, 6, 200);
    }
    c->decor_count = (uint8_t)clampi(c->decor_count, 0, CFG_MAX_DECOR);
    for (int i = 0; i < c->decor_count; i++) {
        DecorCfg *d = &c->decor[i];
        d->type = (uint8_t)clampi(d->type, 0, DECOR_COUNT - 1);
        d->x = (int16_t)clampi(d->x, 0, 399);
        d->layer = d->layer ? LAYER_FRONT : LAYER_BACK;
    }
    c->bubble_sources = (uint8_t)clampi(c->bubble_sources, 0, CFG_MAX_BUBBLES);
    for (int i = 0; i < c->bubble_sources; i++) {
        c->bubbles[i].x = (int16_t)clampi(c->bubbles[i].x, 0, 399);
        c->bubbles[i].rate = (uint8_t)clampi(c->bubbles[i].rate, 1, 100);
    }
    c->species_count = (uint8_t)clampi(c->species_count, 0, CFG_MAX_SPECIES);
    for (int i = 0; i < c->species_count; i++) {
        c->species[i].id[sizeof(c->species[i].id) - 1] = 0;
        c->species[i].count = (uint8_t)clampi(c->species[i].count, 0, 40);
        c->species[i].variant = (uint8_t)clampi(c->species[i].variant, 0, 3);
    }
    c->sim_speed = (uint8_t)clampi(c->sim_speed ? c->sim_speed : 100, 50, 200);
    c->fps_cap = c->fps_cap == 20 ? 20 : 30;
    c->screen_off_minutes = (uint16_t)clampi(c->screen_off_minutes, 0, 600);
    c->brightness = (uint8_t)clampi(c->brightness ? c->brightness : 100, 10, 100);
    c->creature_limit = (uint16_t)clampi(c->creature_limit ? c->creature_limit : 60, 10, 120);
}

static void color_hex(JsonWriter *w, uint16_t c)
{
    unsigned r = ((c >> 11) & 31) * 255 / 31, g = ((c >> 5) & 63) * 255 / 63, b = (c & 31) * 255 / 31;
    jw_printf(w, "\"#%02x%02x%02x\"", r, g, b);
}

size_t config_to_json(const AquariumConfig *c, const char *name, char *buf, size_t cap)
{
    JsonWriter w;
    jw_init(&w, buf, cap);
    jw_printf(&w, "{\n  \"version\": %d,\n  \"name\": ", c->version);
    jw_str(&w, name ? name : "");
    jw_raw(&w, ",\n  \"theme_id\": ");
    jw_str(&w, c->theme_id);
    jw_raw(&w, ",\n  \"water_top\": ");
    color_hex(&w, c->water_top);
    jw_raw(&w, ", \"water_bottom\": ");
    color_hex(&w, c->water_bottom);
    jw_printf(&w, ",\n  \"light_rays\": %d, \"particles\": %d, \"show_surface\": %s,\n", c->light_rays, c->particles,
              c->show_surface ? "true" : "false");
    jw_printf(&w, "  \"substrate_type\": %d, \"substrate_color\": ", c->substrate_type);
    color_hex(&w, c->substrate_color);
    jw_printf(&w, ", \"substrate_height\": %d, \"substrate_roughness\": %d, \"substrate_seed\": %lu,\n",
              c->substrate_height, c->substrate_roughness, (unsigned long)c->substrate_seed);

    jw_raw(&w, "  \"plants\": [");
    for (int i = 0; i < c->plant_count; i++) {
        const PlantCfg *p = &c->plants[i];
        jw_printf(&w, "%s\n    {\"type\": %d, \"x\": %d, \"layer\": %d, \"height\": %d, \"color\": ", i ? "," : "",
                  p->type, p->x, p->layer, p->height);
        color_hex(&w, p->color);
        jw_printf(&w, ", \"seed\": %lu}", (unsigned long)p->seed);
    }
    jw_raw(&w, "],\n  \"decor\": [");
    for (int i = 0; i < c->decor_count; i++) {
        const DecorCfg *d = &c->decor[i];
        jw_printf(&w, "%s\n    {\"type\": %d, \"x\": %d, \"layer\": %d, \"color\": ", i ? "," : "", d->type, d->x,
                  d->layer);
        color_hex(&w, d->color);
        jw_raw(&w, "}");
    }
    jw_raw(&w, "],\n  \"bubbles\": [");
    for (int i = 0; i < c->bubble_sources; i++)
        jw_printf(&w, "%s{\"x\": %d, \"rate\": %d}", i ? ", " : "", c->bubbles[i].x, c->bubbles[i].rate);
    jw_raw(&w, "],\n  \"species\": [");
    for (int i = 0; i < c->species_count; i++) {
        jw_printf(&w, "%s\n    {\"id\": ", i ? "," : "");
        jw_str(&w, c->species[i].id);
        jw_printf(&w, ", \"count\": %d, \"variant\": %d}", c->species[i].count, c->species[i].variant);
    }
    jw_printf(&w,
              "],\n  \"day_night_cycle\": %s, \"sim_speed\": %d, \"fps_cap\": %d, \"screen_off_minutes\": %d,"
              " \"brightness\": %d, \"creature_limit\": %d\n}\n",
              c->day_night_cycle ? "true" : "false", c->sim_speed, c->fps_cap, c->screen_off_minutes, c->brightness,
              c->creature_limit);
    return w.overflow ? 0 : w.len;
}

#define MAX_TOKENS 640

bool config_from_json(AquariumConfig *c, const char *js, char *name, size_t name_cap)
{
    static JsonTok t[MAX_TOKENS]; // static: keeps the stack small (called from a low-priority task)
    int n = json_parse(js, t, MAX_TOKENS);
    if (n <= 0 || t[0].type != JT_OBJ) return false;

#define GI(field, key) c->field = (__typeof__(c->field))json_int(js, t, json_get(js, t, 0, key), (long)c->field)
#define GB(field, key) c->field = json_bool(js, t, json_get(js, t, 0, key), c->field)
    // Version 1 used a different plant/decor numbering (marine themes): not migratable.
    if (json_int(js, t, json_get(js, t, 0, "version"), 0) < CONFIG_VERSION) return false;
    if (name) json_str(js, t, json_get(js, t, 0, "name"), name, name_cap);
    int ti = json_get(js, t, 0, "theme_id");
    if (ti >= 0) json_str(js, t, ti, c->theme_id, sizeof(c->theme_id));
    GI(water_top, "water_top");
    GI(water_bottom, "water_bottom");
    GI(light_rays, "light_rays");
    GI(particles, "particles");
    GB(show_surface, "show_surface");
    GI(substrate_type, "substrate_type");
    GI(substrate_color, "substrate_color");
    GI(substrate_height, "substrate_height");
    GI(substrate_roughness, "substrate_roughness");
    c->substrate_seed = (uint32_t)json_int(js, t, json_get(js, t, 0, "substrate_seed"), (long)c->substrate_seed);
    GB(day_night_cycle, "day_night_cycle");
    GI(sim_speed, "sim_speed");
    GI(fps_cap, "fps_cap");
    GI(screen_off_minutes, "screen_off_minutes");
    GI(brightness, "brightness");
    GI(creature_limit, "creature_limit");
#undef GI
#undef GB

    int arr = json_get(js, t, 0, "plants");
    if (arr >= 0 && t[arr].type == JT_ARR) {
        c->plant_count = 0;
        for (int i = 0; i < t[arr].size && c->plant_count < CFG_MAX_PLANTS; i++) {
            int o = json_at(t, arr, i);
            PlantCfg *p = &c->plants[c->plant_count++];
            p->type = (uint8_t)json_int(js, t, json_get(js, t, o, "type"), 0);
            p->x = (int16_t)json_int(js, t, json_get(js, t, o, "x"), 200);
            p->layer = (uint8_t)json_int(js, t, json_get(js, t, o, "layer"), 0);
            p->height = (uint8_t)json_int(js, t, json_get(js, t, o, "height"), 60);
            p->color = (uint16_t)json_int(js, t, json_get(js, t, o, "color"), 0x2C67);
            p->seed = (uint32_t)json_int(js, t, json_get(js, t, o, "seed"), 1);
        }
    }
    arr = json_get(js, t, 0, "decor");
    if (arr >= 0 && t[arr].type == JT_ARR) {
        c->decor_count = 0;
        for (int i = 0; i < t[arr].size && c->decor_count < CFG_MAX_DECOR; i++) {
            int o = json_at(t, arr, i);
            DecorCfg *d = &c->decor[c->decor_count++];
            d->type = (uint8_t)json_int(js, t, json_get(js, t, o, "type"), 0);
            d->x = (int16_t)json_int(js, t, json_get(js, t, o, "x"), 200);
            d->layer = (uint8_t)json_int(js, t, json_get(js, t, o, "layer"), 0);
            d->color = (uint16_t)json_int(js, t, json_get(js, t, o, "color"), 0x8410);
        }
    }
    arr = json_get(js, t, 0, "bubbles");
    if (arr >= 0 && t[arr].type == JT_ARR) {
        c->bubble_sources = 0;
        for (int i = 0; i < t[arr].size && c->bubble_sources < CFG_MAX_BUBBLES; i++) {
            int o = json_at(t, arr, i);
            BubbleCfg *b = &c->bubbles[c->bubble_sources++];
            b->x = (int16_t)json_int(js, t, json_get(js, t, o, "x"), 300);
            b->rate = (uint8_t)json_int(js, t, json_get(js, t, o, "rate"), 40);
        }
    }
    arr = json_get(js, t, 0, "species");
    if (arr >= 0 && t[arr].type == JT_ARR) {
        c->species_count = 0;
        for (int i = 0; i < t[arr].size && c->species_count < CFG_MAX_SPECIES; i++) {
            int o = json_at(t, arr, i);
            SpeciesCfg *s = &c->species[c->species_count++];
            json_str(js, t, json_get(js, t, o, "id"), s->id, sizeof(s->id));
            s->count = (uint8_t)json_int(js, t, json_get(js, t, o, "count"), 1);
            s->variant = (uint8_t)json_int(js, t, json_get(js, t, o, "variant"), 0);
        }
    }
    config_sanitize(c);
    return true;
}
