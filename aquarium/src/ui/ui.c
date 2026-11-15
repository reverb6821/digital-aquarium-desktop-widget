// Settings UI (spec §9): side panel with 7 sections + edit mode with
// draggable plants/decorations. Everything is drawn with the pixel font on the
// low-res canvas so it matches the aquarium style and works in the simulator.
#include "ui.h"
#include <stdio.h>
#include <string.h>
#include "../app/app.h"
#include "../config/storage.h"
#include "../engine/font.h"
#include "../engine/hal.h"
#include "../engine/rng.h"
#include "../scene/creatures.h"
#include "../scene/scene.h"
#include "../scene/themes.h"

// ------------------------------------------------------------------ layout

#define PANEL_X 196
#define PANEL_W (CANVAS_W - PANEL_X)
#define HEADER_H 20
#define CONTENT_Y (HEADER_H + 1)
#define CONTENT_H (CANVAS_H - CONTENT_Y)
#define PAD 6
#define ROW 24
#define BAR_Y (CANVAS_H - 24)
#define THUMB_W 80
#define THUMB_H 48

#define COL_PANEL RGB565(8, 18, 26)
#define COL_TEXT RGB565(220, 236, 236)
#define COL_DIM RGB565(130, 160, 165)
#define COL_ACCENT RGB565(60, 200, 190)
#define COL_BTN RGB565(22, 46, 58)
#define COL_BTN_HI RGB565(40, 90, 100)
#define COL_BORDER RGB565(70, 120, 130)

typedef enum { P_MAIN = 0, P_THEME, P_WATER, P_SUBSTRATE, P_PLANTS, P_CREATURES, P_ENV, P_PRESETS, P_COUNT } Page;

static const char *const PAGE_TITLES[P_COUNT] = {"Impostazioni", "Bioma", "Acqua", "Fondale", "Piante e decor",
                                                 "Creature", "Ambiente", "Preset"};
static const char *const PLANT_NAMES[PLANT_COUNT] = {"Pianta a stelo", "Vallisneria", "Cryptocoryne",
                                                     "Microsorum",     "Bolbitis",    "Anubias",
                                                     "Bucephalandra",  "Echinodorus", "Muschio di Java",
                                                     "Ninfea"};
static const char *const DECOR_NAMES[DECOR_COUNT] = {"Roccia grande", "Roccia piatta", "Roccia alta", "Sassi",
                                                     "Ardesia",       "Tronco",        "Spiderwood",  "Ramo alto",
                                                     "Radici",        "Foglie catappa"};
static const char *const SUB_NAMES[SUB_COUNT] = {"Sabbia", "Ghiaia", "Roccia scura", "Ciottoli", "Nessuno"};

// ------------------------------------------------------------------ state

typedef struct {
    bool down, pressed, released;
    int x, y, px, py; // current and press position
} UiIn;

static struct {
    bool open, editing;
    Page page;
    UiIn in;
    int active;   // id of the widget being pressed
    bool active_is_slider;
    int id;       // per-frame widget id counter
    int scroll, content_h, last_y;
    bool scrolling;
    bool in_content; // widgets currently being laid out are in the scrollable area
    // thumbnails
    uint16_t *thumb_px[THEME_COUNT];
    bool thumbs_ready;
    // edit mode
    AquariumConfig backup;
    int sel_kind, sel_index;
    bool dragging;
    int drag_off;
    uint32_t last_rebuild;
    bool drag_dirty;
    // misc
    int add_plant_type, add_decor_type;
    uint32_t toast_until;
    char toast[40];
} U;

void ui_init(void)
{
    memset(&U, 0, sizeof(U));
}

bool ui_is_open(void) { return U.open; }
bool ui_is_editing(void) { return U.editing; }

void ui_open(void)
{
    if (U.editing) return;
    U.open = true;
    U.page = P_MAIN;
    U.scroll = 0;
    U.active = 0;
}

void ui_close(void)
{
    U.open = false;
    U.active = 0;
}

static void toast(const char *msg, uint32_t now)
{
    snprintf(U.toast, sizeof(U.toast), "%s", msg);
    U.toast_until = now + 1500;
}

// ------------------------------------------------------------------ input

static void edit_gesture(const GestureEvent *e);

bool ui_gesture(const GestureEvent *e)
{
    if (!U.open && !U.editing) return false;
    switch (e->type) {
    case GE_DOWN:
        U.in.down = true;
        U.in.pressed = true;
        U.in.x = U.in.px = e->x;
        U.in.y = U.in.py = e->y;
        U.last_y = e->y;
        break;
    case GE_MOVE:
        U.in.x = e->x;
        U.in.y = e->y;
        break;
    case GE_UP:
        U.in.down = false;
        U.in.released = true;
        U.in.x = e->x;
        U.in.y = e->y;
        break;
    case GE_TAP:
        // tap outside the panel closes the menu
        if (U.open && e->x < PANEL_X) ui_close();
        break;
    default: break;
    }
    if (U.editing) edit_gesture(e);
    return true;
}

static bool inside(int x, int y, int w, int h, int px, int py) { return px >= x && py >= y && px < x + w && py < y + h; }

// Button behaviour shared by all clickable widgets. Returns true on click.
static bool clickable(int x, int y, int w, int h, bool *hover)
{
    int id = ++U.id;
    // content widgets only react to presses inside the (clipped) content area,
    // header widgets only to presses in the header
    bool zone_ok = !U.open || (U.in_content ? U.in.py >= CONTENT_Y : U.in.py < CONTENT_Y);
    if (U.in.pressed && zone_ok && inside(x, y, w, h, U.in.px, U.in.py)) {
        U.active = id;
        U.active_is_slider = false;
    }
    bool in = inside(x, y, w, h, U.in.x, U.in.y);
    *hover = U.active == id && U.in.down && in;
    if (U.in.released && U.active == id) {
        U.active = 0;
        return in && !U.scrolling;
    }
    return false;
}

// ------------------------------------------------------------------ widgets

static Canvas *g_cv;

static void frame_rect(int x, int y, int w, int h, uint16_t fill, uint16_t border)
{
    canvas_fill_rect(g_cv, x + 1, y + 1, w - 2, h - 2, fill);
    canvas_hline(g_cv, x + 1, x + w - 2, y, border);
    canvas_hline(g_cv, x + 1, x + w - 2, y + h - 1, border);
    canvas_vline(g_cv, x, y + 1, y + h - 2, border);
    canvas_vline(g_cv, x + w - 1, y + 1, y + h - 2, border);
}

static bool button(int x, int y, int w, int h, const char *label, bool selected)
{
    bool hover;
    bool click = clickable(x, y, w, h, &hover);
    frame_rect(x, y, w, h, hover ? COL_BTN_HI : (selected ? RGB565(30, 80, 80) : COL_BTN),
               selected ? COL_ACCENT : COL_BORDER);
    font_draw_centered(g_cv, x + w / 2, y + (h - FONT_HEIGHT) / 2 + 1, label, selected ? COL_ACCENT : COL_TEXT);
    return click;
}

static bool slider(int x, int y, int w, const char *label, int *v, int lo, int hi)
{
    int id = ++U.id;
    char buf[48];
    snprintf(buf, sizeof(buf), "%s: %d", label, *v);
    font_draw(g_cv, x, y, buf, COL_TEXT);
    int ty = y + 12, th = 10;
    bool changed = false;
    if (U.in.pressed && U.in.py >= CONTENT_Y && inside(x - 4, ty - 4, w + 8, th + 8, U.in.px, U.in.py)) {
        U.active = id;
        U.active_is_slider = true;
    }
    if (U.active == id && (U.in.down || U.in.released)) {
        int nv = lo + (U.in.x - x) * (hi - lo + 1) / w;
        if (nv < lo) nv = lo;
        if (nv > hi) nv = hi;
        if (nv != *v) {
            *v = nv;
            changed = true;
        }
        if (U.in.released) U.active = 0;
    }
    canvas_fill_rect(g_cv, x, ty + 4, w, 2, COL_BORDER);
    int kx = x + (hi > lo ? (*v - lo) * (w - 6) / (hi - lo) : 0);
    canvas_fill_rect(g_cv, x, ty + 4, kx - x + 3, 2, COL_ACCENT);
    frame_rect(kx, ty, 6, th, U.active == id ? COL_ACCENT : COL_TEXT, COL_ACCENT);
    return changed;
}

static bool toggle(int x, int y, int w, const char *label, bool *v)
{
    bool hover;
    bool click = clickable(x, y, w, 18, &hover);
    font_draw(g_cv, x, y + 5, label, COL_TEXT);
    int bx = x + w - 30;
    frame_rect(bx, y + 2, 28, 14, *v ? RGB565(30, 110, 100) : COL_BTN, COL_BORDER);
    frame_rect(*v ? bx + 15 : bx + 2, y + 4, 11, 10, *v ? COL_ACCENT : COL_DIM, COL_BORDER);
    if (click) *v = !*v;
    return click;
}

static bool swatches(int x, int y, uint16_t *col)
{
    bool changed = false;
    for (int i = 0; i < SWATCH_COUNT; i++) {
        int sx = x + (i % 8) * 23, sy = y + (i / 8) * 18;
        bool hover;
        if (clickable(sx, sy, 21, 16, &hover)) {
            *col = SWATCHES[i];
            changed = true;
        }
        canvas_fill_rect(g_cv, sx + 1, sy + 1, 19, 14, SWATCHES[i]);
        if (*col == SWATCHES[i] || hover) canvas_rect(g_cv, sx, sy, 21, 16, COL_TEXT);
    }
    return changed;
}

// ------------------------------------------------------------------ pages

static void nav(Page p)
{
    U.page = p;
    U.scroll = 0;
}

static int page_main(int x, int y, int w)
{
    for (int p = P_THEME; p < P_COUNT; p++, y += ROW)
        if (button(x, y, w, ROW - 4, PAGE_TITLES[p], false)) nav((Page)p);
    return y;
}

static void make_thumbs(void)
{
    for (int i = 0; i < THEME_COUNT; i++) {
        if (!U.thumb_px[i]) U.thumb_px[i] = (uint16_t *)hal_alloc_large(THUMB_W * THUMB_H * 2);
        if (!U.thumb_px[i]) return;
        AquariumConfig c = *app_cfg();
        theme_apply(&c, (ThemeId)i);
        Canvas t;
        canvas_init(&t, U.thumb_px[i], THUMB_W, THUMB_H);
        scene_render_thumbnail(&c, &t);
    }
    U.thumbs_ready = true;
}

static int page_theme(int x, int y, int w, uint32_t now)
{
    if (!U.thumbs_ready) make_thumbs();
    int cur = theme_index(app_cfg()->theme_id);
    for (int i = 0; i < THEME_COUNT; i++) {
        int tx = x + (i % 2) * (THUMB_W + 12), ty = y + (i / 2) * (THUMB_H + 16);
        bool hover;
        if (clickable(tx, ty, THUMB_W + 4, THUMB_H + 14, &hover)) app_set_theme(i);
        if (U.thumb_px[i])
            for (int r = 0; r < THUMB_H; r++)
                for (int c = 0; c < THUMB_W; c++)
                    canvas_pset(g_cv, tx + 2 + c, ty + 2 + r, U.thumb_px[i][r * THUMB_W + c]);
        canvas_rect(g_cv, tx, ty, THUMB_W + 4, THUMB_H + 4, i == cur ? COL_ACCENT : (hover ? COL_TEXT : COL_BORDER));
        font_draw(g_cv, tx + 2, ty + THUMB_H + 5, THEME_NAMES[i], i == cur ? COL_ACCENT : COL_TEXT);
    }
    y += ((THEME_COUNT + 1) / 2) * (THUMB_H + 16) + 4;
    font_draw(g_cv, x, y, "Preset utente:", COL_DIM);
    y += 12;
    bool any = false;
    for (int s = 0; s < CFG_MAX_USER_PRESETS; s++) {
        const char *n = storage_preset_name(s);
        if (!n[0]) continue;
        any = true;
        if (button(x, y, w, ROW - 4, n, false)) {
            AquariumConfig c = *app_cfg();
            if (storage_preset_load(s, &c)) app_load_config(&c);
            toast("Preset caricato", now);
        }
        y += ROW;
    }
    if (!any) {
        font_draw(g_cv, x, y, "(nessuno)", COL_DIM);
        y += 12;
    }
    return y;
}

static int page_water(int x, int y, int w)
{
    AquariumConfig *c = app_cfg();
    font_draw(g_cv, x, y, "Palette:", COL_DIM);
    y += 11;
    for (int i = 0; i < WATER_PALETTE_COUNT; i++, y += ROW - 2) {
        bool sel = c->water_top == WATER_PALETTES[i].top && c->water_bottom == WATER_PALETTES[i].bottom;
        if (button(x, y, w, ROW - 6, WATER_PALETTES[i].name, sel)) {
            c->water_top = WATER_PALETTES[i].top;
            c->water_bottom = WATER_PALETTES[i].bottom;
            snprintf(c->theme_id, sizeof(c->theme_id), "custom");
            app_config_changed(SC_STATIC);
        }
    }
    font_draw(g_cv, x, y + 2, "Colore superiore:", COL_DIM);
    y += 12;
    if (swatches(x, y, &c->water_top)) app_config_changed(SC_STATIC);
    y += 38;
    font_draw(g_cv, x, y, "Colore inferiore:", COL_DIM);
    y += 10;
    uint16_t dark = c->water_bottom;
    if (swatches(x, y, &dark)) {
        c->water_bottom = c565_scale(dark, 70); // bottom stays dark
        app_config_changed(SC_STATIC);
    }
    y += 40;
    int v = c->light_rays;
    if (slider(x, y, w - 4, "Raggi di luce", &v, 0, 5)) {
        c->light_rays = (uint8_t)v;
        app_config_changed(0);
    }
    y += 28;
    v = c->particles;
    if (slider(x, y, w - 4, "Particelle", &v, 0, 100)) {
        c->particles = (uint8_t)v;
        app_config_changed(SC_PARTICLES);
    }
    y += 30;
    if (toggle(x, y, w, "Superficie", &c->show_surface)) app_config_changed(0);
    return y + 22;
}

static int page_substrate(int x, int y, int w)
{
    AquariumConfig *c = app_cfg();
    for (int i = 0; i < SUB_COUNT; i++) {
        int bw = (w - 4) / 2;
        int bx = x + (i % 2) * (bw + 4), by = y + (i / 2) * (ROW - 2);
        if (button(bx, by, bw, ROW - 6, SUB_NAMES[i], c->substrate_type == i)) {
            c->substrate_type = (uint8_t)i;
            app_config_changed(SC_STATIC);
        }
    }
    y += 3 * (ROW - 2) + 2;
    font_draw(g_cv, x, y, "Colore:", COL_DIM);
    y += 10;
    uint16_t col = c->substrate_color;
    if (swatches(x, y, &col)) {
        c->substrate_color = c565_scale(col, 120);
        app_config_changed(SC_STATIC);
    }
    y += 40;
    int v = c->substrate_height;
    if (slider(x, y, w - 4, "Altezza", &v, 10, 60)) {
        c->substrate_height = (uint8_t)v;
        app_config_changed(SC_STATIC);
    }
    y += 28;
    v = c->substrate_roughness;
    if (slider(x, y, w - 4, "Rugosita'", &v, 0, 100)) {
        c->substrate_roughness = (uint8_t)v;
        app_config_changed(SC_STATIC);
    }
    y += 30;
    if (button(x, y, w, ROW - 4, "Rigenera", false)) {
        c->substrate_seed = hash32(c->substrate_seed + hal_millis());
        app_config_changed(SC_STATIC);
    }
    return y + ROW;
}

static void enter_edit(void)
{
    U.backup = *app_cfg();
    U.open = false;
    U.editing = true;
    U.sel_kind = SEL_NONE;
    U.sel_index = -1;
    scene_set_edit(true, SEL_NONE, -1);
}

static int page_plants(int x, int y, int w, uint32_t now)
{
    AquariumConfig *c = app_cfg();
    if (button(x, y, w, ROW - 4, "Modalita' modifica", true)) {
        enter_edit();
        return y;
    }
    y += ROW + 2;
    // add plant
    char buf[40];
    if (button(x, y, 22, ROW - 6, "<", false)) U.add_plant_type = (U.add_plant_type + PLANT_COUNT - 1) % PLANT_COUNT;
    snprintf(buf, sizeof(buf), "+ %s", PLANT_NAMES[U.add_plant_type]);
    if (button(x + 24, y, w - 48, ROW - 6, buf, false)) {
        if (c->plant_count < CFG_MAX_PLANTS) {
            static const uint8_t H[PLANT_COUNT] = {70, 100, 26, 50, 55, 22, 10, 60, 7, 200};
            static const uint16_t COLS[PLANT_COUNT] = {
                RGB565(70, 165, 80), RGB565(80, 160, 90),  RGB565(95, 115, 60), RGB565(35, 105, 60),
                RGB565(40, 110, 70), RGB565(35, 105, 60),  RGB565(60, 100, 95), RGB565(70, 165, 80),
                RGB565(60, 140, 70), RGB565(70, 150, 70)};
            PlantCfg *p = &c->plants[c->plant_count++];
            p->type = (uint8_t)U.add_plant_type;
            p->x = (int16_t)(40 + (hash32(now) % 320));
            p->layer = LAYER_BACK;
            p->height = H[U.add_plant_type];
            p->color = COLS[U.add_plant_type];
            p->seed = hash32(now ^ 0x5151u);
            app_config_changed(SC_PLANTS);
        } else toast("Massimo raggiunto", now);
    }
    if (button(x + w - 22, y, 22, ROW - 6, ">", false)) U.add_plant_type = (U.add_plant_type + 1) % PLANT_COUNT;
    y += ROW;
    if (button(x, y, 22, ROW - 6, "<", false)) U.add_decor_type = (U.add_decor_type + DECOR_COUNT - 1) % DECOR_COUNT;
    snprintf(buf, sizeof(buf), "+ %s", DECOR_NAMES[U.add_decor_type]);
    if (button(x + 24, y, w - 48, ROW - 6, buf, false)) {
        if (c->decor_count < CFG_MAX_DECOR) {
            static const uint16_t COLS[DECOR_COUNT] = {
                RGB565(70, 72, 78),  RGB565(70, 72, 78),   RGB565(95, 98, 102), RGB565(120, 105, 90),
                RGB565(80, 78, 74),  RGB565(110, 80, 50),  RGB565(150, 120, 85), RGB565(110, 80, 50),
                RGB565(70, 48, 30),  RGB565(120, 70, 35)};
            DecorCfg *d = &c->decor[c->decor_count++];
            d->type = (uint8_t)U.add_decor_type;
            d->x = (int16_t)(40 + (hash32(now * 3) % 320));
            d->layer = LAYER_BACK;
            d->color = COLS[U.add_decor_type];
            app_config_changed(SC_STATIC);
        } else toast("Massimo raggiunto", now);
    }
    if (button(x + w - 22, y, 22, ROW - 6, ">", false)) U.add_decor_type = (U.add_decor_type + 1) % DECOR_COUNT;
    y += ROW;
    // bubbles
    int v = c->bubble_sources;
    if (slider(x, y, w - 4, "Sorgenti bolle", &v, 0, CFG_MAX_BUBBLES)) {
        while (c->bubble_sources < v) {
            BubbleCfg *b = &c->bubbles[c->bubble_sources++];
            b->x = (int16_t)(30 + (hash32(now + (uint32_t)v) % 340));
            b->rate = 40;
        }
        c->bubble_sources = (uint8_t)v;
        app_config_changed(0);
    }
    y += 30;
    font_draw(g_cv, x, y, "Elementi (tocca x per eliminare):", COL_DIM);
    y += 12;
    for (int i = 0; i < c->plant_count; i++, y += 18) {
        snprintf(buf, sizeof(buf), "%s%s", PLANT_NAMES[c->plants[i].type], c->plants[i].layer ? " (av.)" : "");
        font_draw(g_cv, x, y + 4, buf, COL_TEXT);
        if (button(x + w - 18, y, 18, 16, "x", false)) {
            memmove(&c->plants[i], &c->plants[i + 1], sizeof(PlantCfg) * (size_t)(c->plant_count - i - 1));
            c->plant_count--;
            app_config_changed(SC_PLANTS);
            break;
        }
    }
    for (int i = 0; i < c->decor_count; i++, y += 18) {
        snprintf(buf, sizeof(buf), "%s%s", DECOR_NAMES[c->decor[i].type], c->decor[i].layer ? " (av.)" : "");
        font_draw(g_cv, x, y + 4, buf, COL_TEXT);
        if (button(x + w - 18, y, 18, 16, "x", false)) {
            memmove(&c->decor[i], &c->decor[i + 1], sizeof(DecorCfg) * (size_t)(c->decor_count - i - 1));
            c->decor_count--;
            app_config_changed(SC_STATIC);
            break;
        }
    }
    return y + 4;
}

// Total count of a species (a preset may list it more than once, one entry per
// colour variant); *slot = first entry, -1 if absent.
static int species_count_of(const AquariumConfig *c, const char *id, int *slot)
{
    int n = 0;
    *slot = -1;
    for (int i = 0; i < c->species_count; i++)
        if (strcmp(c->species[i].id, id) == 0) {
            if (*slot < 0) *slot = i;
            n += c->species[i].count;
        }
    return n;
}

static int page_creatures(int x, int y, int w, uint32_t now)
{
    AquariumConfig *c = app_cfg();
    char buf[48];
    int total = 0;
    for (int i = 0; i < c->species_count; i++) total += c->species[i].count;
    snprintf(buf, sizeof(buf), "Totale %d / limite %d", total, c->creature_limit);
    font_draw(g_cv, x, y, buf, total > c->creature_limit ? RGB565(255, 140, 100) : COL_DIM);
    y += 12;
    for (int s = 0; s < SPECIES_COUNT; s++, y += 30) {
        const SpeciesDef *d = &SPECIES[s];
        int slot;
        int n = species_count_of(c, d->id, &slot);
        font_draw(g_cv, x, y, d->name, n ? COL_TEXT : COL_DIM);
        int by = y + 10;
        bool minus = button(x, by, 20, 16, "-", false);
        snprintf(buf, sizeof(buf), "%d", n);
        font_draw_centered(g_cv, x + 34, by + 4, buf, COL_TEXT);
        bool plus = button(x + 48, by, 20, 16, "+", false);
        int variant = slot >= 0 ? c->species[slot].variant : 0;
        snprintf(buf, sizeof(buf), "Colore %d/%d", variant + 1, d->variants);
        bool var = button(x + 74, by, w - 74, 16, buf, false);
        if (!(minus || plus || var)) continue;
        if (slot < 0) {
            if (minus || c->species_count >= CFG_MAX_SPECIES) {
                if (!minus) toast("Troppe specie", now);
                continue;
            }
            slot = c->species_count++;
            snprintf(c->species[slot].id, sizeof(c->species[slot].id), "%s", d->id);
            c->species[slot].count = 0;
            c->species[slot].variant = 0;
        }
        SpeciesCfg *sc = &c->species[slot];
        if (minus && sc->count > 0) sc->count--;
        if (plus && sc->count < d->max_count) {
            if (total < c->creature_limit) sc->count++;
            else toast("Limite creature", now);
        }
        if (var) sc->variant = (uint8_t)((sc->variant + 1) % d->variants);
        if (sc->count == 0 && !var) { // drop empty entries
            memmove(sc, sc + 1, sizeof(SpeciesCfg) * (size_t)(c->species_count - slot - 1));
            c->species_count--;
        }
        app_config_changed(SC_CREATURES);
    }
    return y;
}

static int page_env(int x, int y, int w)
{
    AquariumConfig *c = app_cfg();
    if (toggle(x, y, w, "Ciclo giorno/notte", &c->day_night_cycle)) app_config_changed(0);
    y += 24;
    int v = c->sim_speed;
    if (slider(x, y, w - 4, "Velocita' %", &v, 50, 200)) {
        c->sim_speed = (uint8_t)v;
        app_config_changed(0);
    }
    y += 30;
    v = c->brightness;
    if (slider(x, y, w - 4, "Luminosita' %", &v, 10, 100)) {
        c->brightness = (uint8_t)v;
        app_config_changed(0);
    }
    y += 30;
    font_draw(g_cv, x, y, "FPS massimi:", COL_TEXT);
    if (button(x + w - 60, y - 4, 28, 16, "20", c->fps_cap == 20)) {
        c->fps_cap = 20;
        app_config_changed(0);
    }
    if (button(x + w - 30, y - 4, 28, 16, "30", c->fps_cap == 30)) {
        c->fps_cap = 30;
        app_config_changed(0);
    }
    y += 20;
    static const uint16_t OFF[] = {0, 5, 10, 30, 60, 120};
    int idx = 0;
    for (int i = 0; i < 6; i++)
        if (OFF[i] == c->screen_off_minutes) idx = i;
    char buf[40];
    if (c->screen_off_minutes) snprintf(buf, sizeof(buf), "Spegni dopo %d min", c->screen_off_minutes);
    else snprintf(buf, sizeof(buf), "Spegnimento: mai");
    if (button(x, y, w, ROW - 6, buf, false)) {
        c->screen_off_minutes = OFF[(idx + 1) % 6];
        app_config_changed(0);
    }
    y += ROW;
    v = c->creature_limit;
    if (slider(x, y, w - 4, "Limite creature", &v, 10, 120)) {
        c->creature_limit = (uint16_t)v;
        app_config_changed(SC_CREATURES);
    }
    y += 30;
    font_draw(g_cv, x, y, "Wi-Fi/NTP: non configurato", COL_DIM);
    return y + 14;
}

static int page_presets(int x, int y, int w, uint32_t now)
{
    for (int s = 0; s < CFG_MAX_USER_PRESETS; s++, y += 36) {
        const char *n = storage_preset_name(s);
        char buf[32];
        snprintf(buf, sizeof(buf), "%d. %s", s + 1, n[0] ? n : "(vuoto)");
        font_draw(g_cv, x, y, buf, n[0] ? COL_TEXT : COL_DIM);
        int bw = (w - 8) / 3, by = y + 10;
        if (button(x, by, bw, 18, "Salva", false)) {
            char name[PRESET_NAME_LEN];
            int ti = theme_index(app_cfg()->theme_id);
            snprintf(name, sizeof(name), "%s %d", ti >= 0 ? THEME_NAMES[ti] : "Custom", s + 1);
            storage_request_preset_save(s, app_cfg(), name);
            toast("Preset salvato", now);
        }
        if (n[0] && button(x + bw + 4, by, bw, 18, "Carica", false)) {
            AquariumConfig c = *app_cfg();
            if (storage_preset_load(s, &c)) app_load_config(&c);
            toast("Preset caricato", now);
        }
        if (n[0] && button(x + 2 * (bw + 4), by, bw, 18, "Elimina", false)) {
            storage_request_preset_delete(s);
            toast("Preset eliminato", now);
        }
    }
    return y;
}

// ------------------------------------------------------------------ edit mode

static void edit_select(int kind, int index)
{
    U.sel_kind = kind;
    U.sel_index = index;
    scene_set_edit(true, kind, index);
}

static int16_t *sel_x(void)
{
    AquariumConfig *c = app_cfg();
    if (U.sel_kind == SEL_PLANT && U.sel_index < c->plant_count) return &c->plants[U.sel_index].x;
    if (U.sel_kind == SEL_DECOR && U.sel_index < c->decor_count) return &c->decor[U.sel_index].x;
    return NULL;
}

static void edit_gesture(const GestureEvent *e)
{
    if (e->y >= BAR_Y) return; // the bottom bar is handled as widgets
    if (e->type == GE_DOWN) {
        int idx;
        int kind = scene_pick(e->x, e->y, &idx);
        if (kind != SEL_NONE) edit_select(kind, idx);
        int16_t *px = sel_x();
        U.dragging = kind != SEL_NONE && px;
        if (U.dragging) U.drag_off = *px - e->x;
    } else if (e->type == GE_MOVE && U.dragging) {
        int16_t *px = sel_x();
        if (!px) return;
        int nx = e->x + U.drag_off;
        *px = (int16_t)(nx < 4 ? 4 : (nx > CANVAS_W - 5 ? CANVAS_W - 5 : nx));
        U.drag_dirty = true;
    } else if (e->type == GE_UP && U.dragging) {
        U.dragging = false;
        U.drag_dirty = true;
        U.last_rebuild = 0; // force a final rebuild
    }
}

static void edit_draw(uint32_t now)
{
    AquariumConfig *c = app_cfg();
    // throttled live rebuild while dragging (static caches are not free)
    if (U.drag_dirty && (now - U.last_rebuild > 120 || !U.dragging)) {
        app_config_changed(U.sel_kind == SEL_DECOR ? SC_STATIC : SC_PLANTS);
        U.drag_dirty = false;
        U.last_rebuild = now;
    }
    canvas_blend_rect(g_cv, 0, BAR_Y, CANVAS_W, CANVAS_H - BAR_Y, COL_PANEL, 220);
    canvas_hline(g_cv, 0, CANVAS_W - 1, BAR_Y, COL_BORDER);
    int y = BAR_Y + 3, h = 18;
    if (button(4, y, 56, h, "Fatto", true)) {
        U.editing = false;
        scene_set_edit(false, SEL_NONE, -1);
        app_config_changed(0);
    }
    if (button(64, y, 64, h, "Annulla", false)) {
        *c = U.backup;
        U.editing = false;
        scene_set_edit(false, SEL_NONE, -1);
        app_config_changed(SC_STATIC | SC_PLANTS);
    }
    bool has = sel_x() != NULL;
    if (!has) {
        font_draw(g_cv, 140, y + 5, "Tocca e trascina piante/decor", COL_DIM);
        return;
    }
    uint8_t *layer = U.sel_kind == SEL_PLANT ? &c->plants[U.sel_index].layer : &c->decor[U.sel_index].layer;
    uint16_t *color = U.sel_kind == SEL_PLANT ? &c->plants[U.sel_index].color : &c->decor[U.sel_index].color;
    unsigned flag = U.sel_kind == SEL_PLANT ? SC_PLANTS : SC_STATIC;
    if (button(140, y, 76, h, *layer ? "Davanti" : "Dietro", false)) {
        *layer = !*layer;
        app_config_changed(SC_STATIC | SC_PLANTS);
    }
    if (button(220, y, 64, h, "Colore", false)) {
        int cur = 0;
        for (int i = 0; i < SWATCH_COUNT; i++)
            if (SWATCHES[i] == *color) cur = i;
        *color = SWATCHES[(cur + 1) % SWATCH_COUNT];
        app_config_changed(flag);
    }
    canvas_fill_rect(g_cv, 286, y + 3, 10, 12, *color);
    if (button(300, y, 70, h, "Elimina", false)) {
        if (U.sel_kind == SEL_PLANT) {
            memmove(&c->plants[U.sel_index], &c->plants[U.sel_index + 1],
                    sizeof(PlantCfg) * (size_t)(c->plant_count - U.sel_index - 1));
            c->plant_count--;
        } else {
            memmove(&c->decor[U.sel_index], &c->decor[U.sel_index + 1],
                    sizeof(DecorCfg) * (size_t)(c->decor_count - U.sel_index - 1));
            c->decor_count--;
        }
        edit_select(SEL_NONE, -1);
        app_config_changed(SC_STATIC | SC_PLANTS);
    }
}

// ------------------------------------------------------------------ frame

void ui_draw(Canvas *cv, uint32_t now)
{
    g_cv = cv;
    U.id = 0;
    if (U.editing) {
        edit_draw(now);
    } else if (U.open) {
        // drag-to-scroll inside the content area
        if (U.in.down && !U.active_is_slider && U.in.px >= PANEL_X && U.in.py > CONTENT_Y) {
            if (!U.scrolling && (U.in.y - U.in.py > 6 || U.in.py - U.in.y > 6)) {
                U.scrolling = true;
                U.active = 0;
            }
            if (U.scrolling) U.scroll -= U.in.y - U.last_y;
        }
        U.last_y = U.in.y;
        int max_scroll = U.content_h - CONTENT_H;
        if (U.scroll > max_scroll) U.scroll = max_scroll;
        if (U.scroll < 0) U.scroll = 0;

        canvas_blend_rect(cv, PANEL_X, 0, PANEL_W, CANVAS_H, COL_PANEL, 215);
        canvas_vline(cv, PANEL_X, 0, CANVAS_H - 1, COL_BORDER);
        // header
        canvas_blend_rect(cv, PANEL_X + 1, 0, PANEL_W - 1, HEADER_H, RGB565(20, 50, 60), 230);
        int hx = PANEL_X + PAD;
        if (U.page != P_MAIN) {
            if (button(hx - 2, 2, 20, 16, "<", false)) nav(P_MAIN);
            hx += 22;
        }
        font_draw(cv, hx, 6, PAGE_TITLES[U.page], COL_ACCENT);
        if (button(CANVAS_W - 22, 2, 20, 16, "x", false)) ui_close();

        // content (clipped)
        canvas_set_clip(cv, PANEL_X + 1, CONTENT_Y, PANEL_W - 1, CONTENT_H);
        U.in_content = true;
        int x = PANEL_X + PAD, w = PANEL_W - 2 * PAD, y0 = CONTENT_Y + 4 - U.scroll, y = y0;
        switch (U.page) {
        case P_MAIN: y = page_main(x, y, w); break;
        case P_THEME: y = page_theme(x, y, w, now); break;
        case P_WATER: y = page_water(x, y, w); break;
        case P_SUBSTRATE: y = page_substrate(x, y, w); break;
        case P_PLANTS: y = page_plants(x, y, w, now); break;
        case P_CREATURES: y = page_creatures(x, y, w, now); break;
        case P_ENV: y = page_env(x, y, w); break;
        case P_PRESETS: y = page_presets(x, y, w, now); break;
        default: break;
        }
        U.content_h = y - y0 + 8;
        canvas_reset_clip(cv);
        U.in_content = false;
        // scrollbar
        if (U.content_h > CONTENT_H) {
            int bh = CONTENT_H * CONTENT_H / U.content_h;
            int by = CONTENT_Y + U.scroll * (CONTENT_H - bh) / (U.content_h - CONTENT_H);
            canvas_fill_rect(cv, CANVAS_W - 3, by, 2, bh, COL_BORDER);
        }
    }
    if (now < U.toast_until) {
        int tw = font_width(U.toast) + 12;
        canvas_blend_rect(cv, (CANVAS_W - tw) / 2, 100, tw, 16, RGB565(0, 0, 0), 200);
        font_draw_centered(cv, CANVAS_W / 2, 104, U.toast, COL_TEXT);
    }
    if (U.in.released) {
        U.scrolling = false;
        U.active_is_slider = false;
    }
    U.in.pressed = false;
    U.in.released = false;
}
