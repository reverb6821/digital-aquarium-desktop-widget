// Settings UI (spec §9): side panel with 7 sections + edit mode with
// draggable plants/decorations. Everything is drawn with the pixel font on the
// low-res canvas so it matches the aquarium style and works in the simulator.
//
// Look: "deep sea" glass panel - opaque abyss gradient with rising bubbles, an
// animated wave under the header and a light sweep along the edge; rounded
// cards and pills, bubble slider knobs, iOS-style switches and colour pixel-art
// icons. The panel is opaque on purpose: it never reads the scene behind it,
// which keeps the PSRAM traffic (and the bounce-buffer ISR) relaxed.
#include "ui.h"
#include <stdio.h>
#include <string.h>
#include "../app/app.h"
#include "../config/storage.h"
#include "../engine/fixmath.h"
#include "../engine/font.h"
#include "../engine/hal.h"
#include "../engine/rng.h"
#include "../engine/sprite.h"
#include "../i18n/i18n.h"
#include "../scene/creatures.h"
#include "../scene/scene.h"
#include "../scene/themes.h"
#include "../scene/world.h"

// ------------------------------------------------------------------ layout

#define PANEL_X 196
#define PANEL_W (CANVAS_W - PANEL_X)
#define HEADER_H 24
#define CONTENT_Y (HEADER_H + 1)
#define CONTENT_H (CANVAS_H - CONTENT_Y)
#define PAD 6
#define ROW 24
#define BAR_Y (CANVAS_H - 24)
#define THUMB_W 80
#define THUMB_H 48
#define TILE_H 46
#define TILE_GAP 6
#define OPEN_MS 140 // slide-in time of the panel

// "deep sea" palette
#define COL_BG_TOP RGB565(12, 40, 56)
#define COL_BG_BOT RGB565(3, 12, 22)
#define COL_HEADER RGB565(14, 50, 66)
#define COL_CARD RGB565(16, 50, 64)
#define COL_CARD_HI RGB565(30, 82, 96)
#define COL_CARD_EDGE RGB565(36, 84, 98)
#define COL_PRESS RGB565(36, 104, 114)
#define COL_TRACK RGB565(6, 24, 34)
#define COL_TEXT RGB565(226, 242, 240)
#define COL_DIM RGB565(120, 160, 168)
#define COL_LABEL RGB565(86, 176, 172)
#define COL_ACCENT RGB565(84, 226, 204)
#define COL_ACCENT_DK RGB565(30, 120, 116)
#define COL_ON_ACCENT RGB565(4, 34, 40)
#define COL_CORAL RGB565(255, 138, 100)
#define COL_PANEL COL_BG_BOT // edit bar / toast background

typedef enum { P_MAIN = 0, P_THEME, P_WATER, P_SUBSTRATE, P_PLANTS, P_CREATURES, P_ENV, P_PRESETS, P_COUNT } Page;

_Static_assert(S_PAGE_PRESETS - S_PAGE_SETTINGS + 1 == P_COUNT, "page title strings out of sync");
_Static_assert(S_PLANT_LAST - S_PLANT_FIRST + 1 == PLANT_COUNT, "plant name strings out of sync");
_Static_assert(S_DECOR_LAST - S_DECOR_FIRST + 1 == DECOR_COUNT, "decor name strings out of sync");
_Static_assert(S_SUB_LAST - S_SUB_FIRST + 1 == SUB_COUNT, "substrate name strings out of sync");
_Static_assert(S_BG_LAST - S_BG_FIRST + 1 == BACKDROP_COUNT, "backdrop name strings out of sync");

static const char *page_title(Page p) { return tr((StrId)(S_PAGE_SETTINGS + p)); }
static const char *plant_name(int t) { return tr((StrId)(S_PLANT_FIRST + t)); }
static const char *decor_name(int t) { return tr((StrId)(S_DECOR_FIRST + t)); }
static const char *sub_name(int t) { return tr((StrId)(S_SUB_FIRST + t)); }

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
    uint32_t open_at; // ms when the panel was opened (slide-in)
    bool open_fresh;  // open_at must be taken on the next draw
    // thumbnails
    uint16_t *thumb_px[THEME_COUNT];
    bool thumbs_ready;
    // edit mode
    AquariumConfig backup;
    int sel_kind, sel_index;
    bool dragging;
    int drag_off, drag_y0, drag_dy0;
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
    U.open_fresh = true;
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

// ------------------------------------------------------------------ drawing helpers

static Canvas *g_cv;
static uint32_t g_now;

static inline float fmodf_pos(float a, float m)
{
    float r = a - m * (float)(int)(a / m);
    return r < 0 ? r + m : r;
}

// Filled rectangle with 2 px rounded corners.
static void rrect(int x, int y, int w, int h, uint16_t col)
{
    for (int r = 0; r < h; r++) {
        int e = r < h - 1 - r ? r : h - 1 - r; // distance from the nearest horizontal edge
        int in = e == 0 ? 2 : (e == 1 ? 1 : 0);
        if (h < 5 && in > 1) in = 1;
        canvas_hline(g_cv, x + in, x + w - 1 - in, y + r, col);
    }
}

// Rounded card: edge colour, fill, 1 px gloss on top and a darker bottom lip.
static void card(int x, int y, int w, int h, uint16_t fill, uint16_t edge)
{
    rrect(x, y, w, h, edge);
    rrect(x + 1, y + 1, w - 2, h - 2, fill);
    canvas_hline(g_cv, x + 3, x + w - 4, y + 1, c565_lerp(fill, RGB565(255, 255, 255), 40));
    canvas_hline(g_cv, x + 3, x + w - 4, y + h - 2, c565_scale(fill, 200));
}

static void text_bold(int x, int y, const char *s, uint16_t col)
{
    font_draw(g_cv, x, y, s, col);
    font_draw(g_cv, x + 1, y, s, col);
}

static void text_right(int xr, int y, const char *s, uint16_t col) { font_draw(g_cv, xr - font_width(s), y, s, col); }

// Pixel-art icons, 12x12. Letters pick colours from icon_col().
enum { IC_BIOME = 0, IC_WATER, IC_SUBSTRATE, IC_PLANTS, IC_CREATURES, IC_ENV, IC_PRESET, IC_EDIT, IC_COUNT };
static const char *const ICONS[IC_COUNT][12] = {
    {// aquarium tank with a plant and a fish
     "kkkkkkkkkkkk", "kbbbbbbbbbbk", "kbbbbbbbgbbk", "kbbobbbbgbbk", "kboooobggbbk", "kbbobbbgbgbk",
     "kbbbbbbgggbk", "kbbbbbggbbbk", "kssssssgssk.", "kSsSsSsSsSsk", "kkkkkkkkkkkk", "............"},
    {// water drop
     ".....b......", ".....bb.....", "....bbbb....", "....bbbbb...", "...bbbbbbb..", "..bwbbbbbbb.",
     "..bwbbbbbbb.", "..bbwbbbbBb.", "..bbbbbbbBb.", "...bbbbbBb..", "....BBBBB...", "............"},
    {// sand dune with pebbles
     "............", "............", "............", "............", "......rrr...", ".rr..rrRrr..",
     "rrRr.rRRrr..", "ssssssssssss", "sSssSsssSsss", "SsSSsSSsSsSS", "SSSSSSSSSSSS", "............"},
    {// sprig of leaves
     "..........g.", "........ggg.", ".g.....gggG.", "ggg...ggGg..", "gGgg..gGg...", ".gGgggGg....",
     "..ggGGg.....", "....Gg......", "....G.......", "....G.......", "...sSs......", "..ssSss....."},
    {// fish
     "............", "......oo....", ".....oooo...", ".o..oooooo..", ".oooooooowoo", ".oooooookooo",
     ".ooooooooooo", ".o..yoooooo.", ".....yyyy...", "......yy....", "............", "............"},
    {// sun over waves
     ".....y......", "..y..y..y...", "...yyyyy....", "..yyyyyyy...", "yyyyyyyyyyy.", "..yyyyyyy...",
     "...yyyyy....", "..y..y..y...", "............", ".b..b..b..b.", "b.bb.bb.bb.b", "............"},
    {// floppy disk
     ".kkkkkkkkk..", ".kwwwwwwwkk.", ".kwwwwwwwkbk", ".kkkkkkkkkbk", ".kbbbbbbbbbk", ".kbkkkkkkbbk",
     ".kbkwwwwkbbk", ".kbkwwwwkbbk", ".kbkwwwwkbbk", ".kkkkkkkkkkk", "............", "............"},
    {// move cross
     ".....a......", "....aaa.....", "...a.a.a....", ".....a......", ".a...a...a..", "aaaaaaaaaaa.",
     ".a...a...a..", ".....a......", "...a.a.a....", "....aaa.....", ".....a......", "............"},
};

static uint16_t icon_col(char ch)
{
    switch (ch) {
    case 'k': return RGB565(120, 190, 200);
    case 'b': return RGB565(60, 150, 230);
    case 'B': return RGB565(30, 90, 170);
    case 'w': return RGB565(235, 250, 255);
    case 'g': return RGB565(90, 210, 110);
    case 'G': return RGB565(40, 140, 70);
    case 'o': return RGB565(255, 140, 60);
    case 'y': return RGB565(255, 215, 80);
    case 's': return RGB565(225, 200, 150);
    case 'S': return RGB565(180, 150, 100);
    case 'r': return RGB565(150, 155, 165);
    case 'R': return RGB565(105, 110, 120);
    case 'a': return COL_ACCENT;
    default: return 0;
    }
}

static void icon(int x, int y, int id)
{
    for (int r = 0; r < 12; r++) {
        const char *row = ICONS[id][r];
        for (int c = 0; c < 12 && row[c]; c++)
            if (row[c] != '.') canvas_pset(g_cv, x + c, y + r, icon_col(row[c]));
    }
}

// Draw a species sprite scaled down (nearest) to fit a box, centred.
static void sprite_fit(const SpriteSheet *s, const uint16_t *pal, int bx, int by, int bw, int bh)
{
    if (!s || !s->data) return;
    int f = 1;
    while (s->w / f > bw || s->h / f > bh) f++;
    int w = s->w / f, h = s->h / f;
    int ox = bx + (bw - w) / 2, oy = by + (bh - h) / 2;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            uint8_t i = sprite_get(s, 0, x * f, y * f);
            if (i) canvas_pset(g_cv, ox + x, oy + y, pal[i]);
        }
}

// ------------------------------------------------------------------ widgets

static bool button(int x, int y, int w, int h, const char *label, bool selected)
{
    bool hover;
    bool click = clickable(x, y, w, h, &hover);
    if (selected) {
        card(x, y, w, h, hover ? COL_TEXT : COL_ACCENT, COL_ACCENT_DK);
        font_draw_centered(g_cv, x + w / 2, y + (h - FONT_HEIGHT) / 2 + 1, label, COL_ON_ACCENT);
        font_draw_centered(g_cv, x + w / 2 + 1, y + (h - FONT_HEIGHT) / 2 + 1, label, COL_ON_ACCENT);
    } else {
        card(x, y, w, h, hover ? COL_PRESS : COL_CARD, hover ? COL_ACCENT : COL_CARD_EDGE);
        font_draw_centered(g_cv, x + w / 2, y + (h - FONT_HEIGHT) / 2 + 1, label, COL_TEXT);
    }
    return click;
}

// Round icon button (header back/close).
static bool round_button(int x, int y, int w, int h, char glyph)
{
    bool hover;
    bool click = clickable(x, y, w, h, &hover);
    int cx = x + w / 2, cy = y + h / 2;
    canvas_fill_circle(g_cv, cx, cy, 8, hover ? COL_ACCENT : COL_CARD_HI);
    canvas_circle(g_cv, cx, cy, 8, hover ? COL_TEXT : COL_CARD_EDGE);
    uint16_t c = hover ? COL_ON_ACCENT : COL_TEXT;
    if (glyph == 'x') { // crisp cross
        for (int i = -3; i <= 3; i++) {
            canvas_pset(g_cv, cx + i, cy + i, c);
            canvas_pset(g_cv, cx + i, cy - i, c);
        }
    } else { // chevron
        for (int i = 0; i <= 3; i++) {
            canvas_pset(g_cv, cx - 1 + i, cy - i, c);
            canvas_pset(g_cv, cx - 1 + i, cy + i, c);
            canvas_pset(g_cv, cx + i, cy - i, c);
            canvas_pset(g_cv, cx + i, cy + i, c);
        }
    }
    return click;
}

// Section label in small caps-style accent with a hairline to the right.
static int section(int x, int y, int w, const char *label)
{
    font_draw(g_cv, x, y, label, COL_LABEL);
    int lx = x + font_width(label) + 4;
    if (lx < x + w) canvas_hline(g_cv, lx, x + w - 1, y + 4, COL_CARD_EDGE);
    return y + 13;
}

// Bubble knob.
static void knob(int cx, int cy, bool active)
{
    int r = active ? 5 : 4;
    canvas_fill_circle(g_cv, cx, cy, r, active ? COL_ACCENT : COL_TEXT);
    canvas_circle(g_cv, cx, cy, r, COL_ACCENT_DK);
    canvas_pset(g_cv, cx - 1, cy - 2, RGB565(255, 255, 255)); // highlight
    canvas_pset(g_cv, cx - 2, cy - 1, RGB565(255, 255, 255));
}

static bool slider(int x, int y, int w, const char *label, int *v, int lo, int hi)
{
    int id = ++U.id;
    char buf[16];
    font_draw(g_cv, x, y, label, COL_TEXT);
    snprintf(buf, sizeof(buf), "%d", *v);
    int vw = font_width(buf) + 8;
    rrect(x + w - vw, y - 2, vw, 11, COL_TRACK);
    text_right(x + w - 4, y, buf, COL_ACCENT);
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
    int cy = ty + 5;
    int kx = x + 4 + (hi > lo ? (*v - lo) * (w - 8) / (hi - lo) : 0);
    rrect(x, cy - 2, w, 5, COL_TRACK);
    if (kx > x + 1) rrect(x, cy - 2, kx - x + 1, 5, COL_ACCENT_DK);
    canvas_hline(g_cv, x + 1, kx, cy - 1, COL_ACCENT);
    knob(kx, cy, U.active == id);
    return changed;
}

static bool toggle(int x, int y, int w, const char *label, bool *v)
{
    bool hover;
    bool click = clickable(x, y, w, 18, &hover);
    font_draw(g_cv, x, y + 5, label, COL_TEXT);
    int bx = x + w - 28, by = y + 2;
    rrect(bx, by, 28, 14, *v ? COL_ACCENT_DK : COL_TRACK);
    rrect(bx + 1, by + 1, 26, 12, *v ? COL_ACCENT : RGB565(30, 52, 62));
    int kx = *v ? bx + 21 : bx + 7;
    canvas_fill_circle(g_cv, kx, by + 7, 5, hover ? COL_ACCENT_DK : COL_TEXT);
    canvas_pset(g_cv, kx - 1, by + 4, RGB565(255, 255, 255));
    if (click) *v = !*v;
    return click;
}

static bool swatches(int x, int y, uint16_t *col)
{
    bool changed = false;
    for (int i = 0; i < SWATCH_COUNT; i++) {
        int sx = x + (i % 8) * 24, sy = y + (i / 8) * 18;
        bool hover;
        if (clickable(sx, sy, 22, 16, &hover)) {
            *col = SWATCHES[i];
            changed = true;
        }
        bool sel = *col == SWATCHES[i];
        if (sel || hover) rrect(sx, sy, 22, 16, sel ? COL_TEXT : COL_ACCENT);
        rrect(sx + 2, sy + 2, 18, 12, SWATCHES[i]);
        canvas_hline(g_cv, sx + 4, sx + 17, sy + 3, c565_lerp(SWATCHES[i], RGB565(255, 255, 255), 70));
    }
    return changed;
}

// ------------------------------------------------------------------ pages

static void nav(Page p)
{
    U.page = p;
    U.scroll = 0;
}

static void enter_edit(void);

static const char *water_palette_name(const AquariumConfig *c)
{
    for (int i = 0; i < WATER_PALETTE_COUNT; i++)
        if (c->water_top == WATER_PALETTES[i].top && c->water_bottom == WATER_PALETTES[i].bottom)
            return tr(WATER_PALETTES[i].name);
    return tr(S_CUSTOM_PALETTE);
}

// Dashboard: 2x4 grid of tiles with icon, title and the current state.
static int page_main(int x, int y, int w)
{
    const AquariumConfig *c = app_cfg();
    char sub[8][28];
    int ti = theme_index(c->theme_id);
    snprintf(sub[0], sizeof(sub[0]), "%s", ti >= 0 ? theme_name(ti) : tr(S_CUSTOM_THEME));
    snprintf(sub[1], sizeof(sub[1]), "%.15s", water_palette_name(c));
    snprintf(sub[2], sizeof(sub[2]), "%s", sub_name(c->substrate_type));
    snprintf(sub[3], sizeof(sub[3]), tr(S_FMT_PLANTS_DECOR), c->plant_count, c->decor_count);
    int fish = 0;
    for (int i = 0; i < c->species_count; i++) fish += c->species[i].count;
    snprintf(sub[4], sizeof(sub[4]), tr(S_FMT_ANIMALS), fish);
    snprintf(sub[5], sizeof(sub[5]), tr(S_FMT_LIGHT), c->brightness);
    int saved = 0;
    for (int s = 0; s < CFG_MAX_USER_PRESETS; s++) saved += storage_preset_name(s)[0] != 0;
    snprintf(sub[6], sizeof(sub[6]), tr(S_FMT_SAVED), saved);
    snprintf(sub[7], sizeof(sub[7]), "%s", tr(S_MOVE_ROTATE));
    int tw = (w - TILE_GAP) / 2;
    for (int i = 0; i < 8; i++) {
        int tx = x + (i % 2) * (tw + TILE_GAP), ty = y + (i / 2) * (TILE_H + TILE_GAP);
        bool hover;
        bool click = clickable(tx, ty, tw, TILE_H, &hover);
        bool cta = i == 7;
        card(tx, ty, tw, TILE_H, hover ? COL_PRESS : (cta ? RGB565(18, 70, 74) : COL_CARD),
             hover || cta ? COL_ACCENT_DK : COL_CARD_EDGE);
        // icon in a soft circle
        canvas_fill_circle(g_cv, tx + 13, ty + 13, 9, hover ? COL_ACCENT_DK : RGB565(10, 34, 46));
        icon(tx + 7, ty + 7, i);
        text_bold(tx + 6, ty + 25, tr((StrId)(S_TILE_BIOME + i)), COL_TEXT);
        // subtitle, clipped to the tile
        char s[16];
        snprintf(s, sizeof(s), "%.14s", sub[i]);
        font_draw(g_cv, tx + 6, ty + 35, s, cta ? COL_ACCENT : COL_DIM);
        if (click) {
            if (cta) enter_edit();
            else nav((Page)(P_THEME + i));
        }
        if (cta && !U.open) return y; // edit mode started
    }
    return y + 4 * (TILE_H + TILE_GAP) - TILE_GAP; // fits without scrolling
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
    int cw = (w - TILE_GAP) / 2, ch = THUMB_H + 18;
    for (int i = 0; i < THEME_COUNT; i++) {
        int tx = x + (i % 2) * (cw + TILE_GAP), ty = y + (i / 2) * (ch + TILE_GAP);
        bool hover;
        if (clickable(tx, ty, cw, ch, &hover)) app_set_theme(i);
        bool sel = i == cur;
        card(tx, ty, cw, ch, hover ? COL_PRESS : COL_CARD, sel ? COL_ACCENT : (hover ? COL_ACCENT_DK : COL_CARD_EDGE));
        int px = tx + (cw - THUMB_W) / 2, py = ty + 3;
        if (U.thumb_px[i])
            for (int r = 0; r < THUMB_H; r++) {
                // rounded thumbnail corners
                int e = r < THUMB_H - 1 - r ? r : THUMB_H - 1 - r;
                int in = e == 0 ? 2 : (e == 1 ? 1 : 0);
                for (int c = in; c < THUMB_W - in; c++) canvas_pset(g_cv, px + c, py + r, U.thumb_px[i][r * THUMB_W + c]);
            }
        font_draw(g_cv, tx + 6, ty + THUMB_H + 7, theme_name(i), sel ? COL_ACCENT : COL_TEXT);
        if (sel) { // check badge
            canvas_fill_circle(g_cv, px + THUMB_W - 6, py + 6, 5, COL_ACCENT);
            canvas_line(g_cv, px + THUMB_W - 9, py + 6, px + THUMB_W - 7, py + 8, COL_ON_ACCENT);
            canvas_line(g_cv, px + THUMB_W - 7, py + 8, px + THUMB_W - 3, py + 4, COL_ON_ACCENT);
        }
    }
    y += ((THEME_COUNT + 1) / 2) * (ch + TILE_GAP) + 2;
    y = section(x, y, w, tr(S_USER_PRESETS));
    bool any = false;
    for (int s = 0; s < CFG_MAX_USER_PRESETS; s++) {
        const char *n = storage_preset_name(s);
        if (!n[0]) continue;
        any = true;
        if (button(x, y, w, ROW - 4, n, false)) {
            AquariumConfig c = *app_cfg();
            if (storage_preset_load(s, &c)) app_load_config(&c);
            toast(tr(S_PRESET_LOADED), now);
        }
        y += ROW;
    }
    if (!any) {
        font_draw(g_cv, x, y, tr(S_NO_PRESETS), COL_DIM);
        y += 12;
    }
    return y;
}

// Palette row: colour preview of the gradient + name.
static bool palette_row(int x, int y, int w, const WaterPalette *p, bool sel)
{
    bool hover;
    bool click = clickable(x, y, w, 18, &hover);
    card(x, y, w, 18, hover ? COL_PRESS : COL_CARD, sel ? COL_ACCENT : (hover ? COL_ACCENT_DK : COL_CARD_EDGE));
    for (int r = 0; r < 12; r++) canvas_hline(g_cv, x + 4, x + 21, y + 3 + r, c565_lerp(p->top, p->bottom, (uint8_t)(r * 21)));
    canvas_rect(g_cv, x + 4, y + 3, 18, 12, COL_CARD_EDGE);
    font_draw(g_cv, x + 28, y + 6, tr(p->name), sel ? COL_ACCENT : COL_TEXT);
    if (sel) canvas_fill_circle(g_cv, x + w - 9, y + 9, 3, COL_ACCENT);
    return click;
}

static int page_water(int x, int y, int w)
{
    AquariumConfig *c = app_cfg();
    y = section(x, y, w, tr(S_BACKDROP));
    int bw = (w - 4) / 2;
    for (int i = 0; i < BACKDROP_COUNT; i++) {
        int bx = x + (i % 2) * (bw + 4), by = y + (i / 2) * (ROW - 2);
        if (button(bx, by, bw, ROW - 6, tr((StrId)(S_BG_FIRST + i)), c->backdrop == i) && c->backdrop != i) {
            c->backdrop = (uint8_t)i;
            app_config_changed(SC_STATIC);
        }
    }
    y += ((BACKDROP_COUNT + 1) / 2) * (ROW - 2) + 2;
    y = section(x, y, w, tr(S_PALETTE));
    for (int i = 0; i < WATER_PALETTE_COUNT; i++, y += 21) {
        bool sel = c->water_top == WATER_PALETTES[i].top && c->water_bottom == WATER_PALETTES[i].bottom;
        if (palette_row(x, y, w, &WATER_PALETTES[i], sel)) {
            c->water_top = WATER_PALETTES[i].top;
            c->water_bottom = WATER_PALETTES[i].bottom;
            snprintf(c->theme_id, sizeof(c->theme_id), "custom");
            app_config_changed(SC_STATIC);
        }
    }
    y = section(x, y + 3, w, tr(S_SURFACE));
    if (swatches(x, y, &c->water_top)) app_config_changed(SC_STATIC);
    y += 38;
    y = section(x, y, w, tr(S_DEPTH));
    uint16_t dark = c->water_bottom;
    if (swatches(x, y, &dark)) {
        c->water_bottom = c565_scale(dark, 70); // bottom stays dark
        app_config_changed(SC_STATIC);
    }
    y += 38;
    y = section(x, y, w, tr(S_EFFECTS));
    int v = c->light_rays;
    if (slider(x, y + 2, w, tr(S_LIGHT_RAYS), &v, 0, 5)) {
        c->light_rays = (uint8_t)v;
        app_config_changed(0);
    }
    y += 30;
    v = c->particles;
    if (slider(x, y + 2, w, tr(S_PARTICLES), &v, 0, 100)) {
        c->particles = (uint8_t)v;
        app_config_changed(SC_PARTICLES);
    }
    y += 30;
    if (toggle(x, y, w, tr(S_ANIMATED_SURFACE), &c->show_surface)) app_config_changed(0);
    return y + 22;
}

static int page_substrate(int x, int y, int w)
{
    AquariumConfig *c = app_cfg();
    y = section(x, y, w, tr(S_MATERIAL));
    int bw = (w - 4) / 2;
    for (int i = 0; i < SUB_COUNT; i++) {
        int bx = x + (i % 2) * (bw + 4), by = y + (i / 2) * (ROW - 2);
        if (button(bx, by, bw, ROW - 6, sub_name(i), c->substrate_type == i)) {
            c->substrate_type = (uint8_t)i;
            app_config_changed(SC_STATIC);
        }
    }
    y += 3 * (ROW - 2) + 2;
    y = section(x, y, w, tr(S_COLOR));
    uint16_t col = c->substrate_color;
    if (swatches(x, y, &col)) {
        c->substrate_color = c565_scale(col, 120);
        app_config_changed(SC_STATIC);
    }
    y += 38;
    y = section(x, y, w, tr(S_SHAPE));
    int v = c->substrate_height;
    if (slider(x, y + 2, w, tr(S_HEIGHT), &v, 10, 60)) {
        c->substrate_height = (uint8_t)v;
        app_config_changed(SC_STATIC);
    }
    y += 30;
    v = c->substrate_roughness;
    if (slider(x, y + 2, w, tr(S_ROUGHNESS), &v, 0, 100)) {
        c->substrate_roughness = (uint8_t)v;
        app_config_changed(SC_STATIC);
    }
    y += 30;
    if (button(x, y, w, ROW - 4, tr(S_REGEN_PROFILE), false)) {
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

// "<  + name  >" picker row; returns 1 on add, and steps *type with the arrows.
static bool add_picker(int x, int y, int w, int *type, int count, const char *(*name)(int))
{
    char buf[40];
    if (button(x, y, 22, ROW - 6, "<", false)) *type = (*type + count - 1) % count;
    snprintf(buf, sizeof(buf), "+ %s", name(*type));
    bool add = button(x + 25, y, w - 50, ROW - 6, buf, false);
    if (button(x + w - 22, y, 22, ROW - 6, ">", false)) *type = (*type + 1) % count;
    return add;
}

// List row with name, optional "front" chip and a round delete button.
static bool item_row(int x, int y, int w, const char *name, bool front, int stripe)
{
    if (stripe) rrect(x, y, w, 17, RGB565(10, 32, 44));
    font_draw(g_cv, x + 5, y + 5, name, COL_TEXT);
    if (front) {
        int cx = x + w - 52;
        rrect(cx, y + 3, 30, 11, COL_ACCENT_DK);
        font_draw(g_cv, cx + 4, y + 5, tr(S_FRONT_CHIP), COL_TEXT);
    }
    return round_button(x + w - 19, y, 18, 17, 'x');
}

static int page_plants(int x, int y, int w, uint32_t now)
{
    AquariumConfig *c = app_cfg();
    if (button(x, y, w, ROW - 4, tr(S_EDIT_MODE), true)) {
        enter_edit();
        return y;
    }
    y += ROW + 2;
    y = section(x, y, w, tr(S_ADD));
    if (add_picker(x, y, w, &U.add_plant_type, PLANT_COUNT, plant_name)) {
        if (c->plant_count < CFG_MAX_PLANTS) {
            static const uint8_t H[PLANT_COUNT] = {70, 100, 26, 50, 55, 22, 10, 60, 7, 200};
            static const uint16_t COLS[PLANT_COUNT] = {
                RGB565(70, 165, 80), RGB565(80, 160, 90),  RGB565(95, 115, 60), RGB565(35, 105, 60),
                RGB565(70, 145, 75), RGB565(35, 105, 60),  RGB565(60, 100, 95), RGB565(70, 165, 80),
                RGB565(60, 140, 70), RGB565(70, 150, 70)};
            PlantCfg *p = &c->plants[c->plant_count++];
            p->type = (uint8_t)U.add_plant_type;
            p->x = (int16_t)(40 + (hash32(now) % 320));
            p->layer = LAYER_BACK;
            p->height = H[U.add_plant_type];
            p->color = COLS[U.add_plant_type];
            p->seed = hash32(now ^ 0x5151u);
            p->dy = 0;
            p->rot = 0;
            p->scale = 100;
            app_config_changed(SC_PLANTS);
            toast(tr(S_PLANT_ADDED), now);
        } else toast(tr(S_LIMIT_REACHED), now);
    }
    y += ROW;
    if (add_picker(x, y, w, &U.add_decor_type, DECOR_COUNT, decor_name)) {
        if (c->decor_count < CFG_MAX_DECOR) {
            static const uint16_t COLS[DECOR_COUNT] = {
                RGB565(70, 72, 78),  RGB565(150, 108, 68), RGB565(95, 98, 102), RGB565(120, 105, 90),
                RGB565(80, 78, 74),  RGB565(110, 80, 50),  RGB565(150, 120, 85), RGB565(110, 80, 50),
                RGB565(70, 48, 30),  RGB565(120, 70, 35)};
            DecorCfg *d = &c->decor[c->decor_count++];
            d->type = (uint8_t)U.add_decor_type;
            d->x = (int16_t)(40 + (hash32(now * 3) % 320));
            d->layer = LAYER_BACK;
            d->color = COLS[U.add_decor_type];
            d->dy = 0;
            d->rot = 0;
            d->scale = 100;
            app_config_changed(SC_STATIC);
            toast(tr(S_DECOR_ADDED), now);
        } else toast(tr(S_LIMIT_REACHED), now);
    }
    y += ROW + 2;
    y = section(x, y, w, tr(S_BUBBLES));
    int v = c->bubble_sources;
    if (slider(x, y + 2, w, tr(S_SOURCES), &v, 0, CFG_MAX_BUBBLES)) {
        while (c->bubble_sources < v) {
            BubbleCfg *b = &c->bubbles[c->bubble_sources++];
            b->x = (int16_t)(30 + (hash32(now + (uint32_t)v) % 340));
            b->rate = 40;
        }
        c->bubble_sources = (uint8_t)v;
        app_config_changed(0);
    }
    y += 32;
    char title[32];
    snprintf(title, sizeof(title), tr(S_FMT_ITEMS), c->plant_count + c->decor_count);
    y = section(x, y, w, title);
    int k = 0;
    for (int i = 0; i < c->plant_count; i++, y += 18, k++)
        if (item_row(x, y, w, plant_name(c->plants[i].type), c->plants[i].layer, k & 1)) {
            memmove(&c->plants[i], &c->plants[i + 1], sizeof(PlantCfg) * (size_t)(c->plant_count - i - 1));
            c->plant_count--;
            app_config_changed(SC_PLANTS);
            break;
        }
    for (int i = 0; i < c->decor_count; i++, y += 18, k++)
        if (item_row(x, y, w, decor_name(c->decor[i].type), c->decor[i].layer, k & 1)) {
            memmove(&c->decor[i], &c->decor[i + 1], sizeof(DecorCfg) * (size_t)(c->decor_count - i - 1));
            c->decor_count--;
            app_config_changed(SC_STATIC);
            break;
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
    // population gauge
    bool over = total > c->creature_limit;
    snprintf(buf, sizeof(buf), "%d / %d", total, c->creature_limit);
    font_draw(g_cv, x, y, tr(S_POPULATION), COL_TEXT);
    text_right(x + w, y, buf, over ? COL_CORAL : COL_ACCENT);
    rrect(x, y + 11, w, 6, COL_TRACK);
    int fw = c->creature_limit ? (total > c->creature_limit ? w : total * w / c->creature_limit) : 0;
    if (fw > 2) rrect(x, y + 11, fw, 6, over ? COL_CORAL : COL_ACCENT_DK);
    y += 24;
    for (int s = 0; s < SPECIES_COUNT; s++, y += 36) {
        const SpeciesDef *d = &SPECIES[s];
        int slot;
        int n = species_count_of(c, d->id, &slot);
        int variant = slot >= 0 ? c->species[slot].variant : 0;
        card(x, y, w, 33, COL_CARD, n ? COL_ACCENT_DK : COL_CARD_EDGE);
        sprite_fit(species_sprite(s), d->pal[variant], x + 3, y + 3, 30, 27);
        font_draw(g_cv, x + 36, y + 4, tr(d->name), n ? COL_TEXT : COL_DIM);
        int by = y + 15;
        bool minus = button(x + 36, by, 18, 15, "-", false);
        snprintf(buf, sizeof(buf), "%d", n);
        font_draw_centered(g_cv, x + 66, by + 4, buf, n ? COL_ACCENT : COL_DIM);
        bool plus = button(x + 78, by, 18, 15, "+", false);
        snprintf(buf, sizeof(buf), tr(S_FMT_COLOR), variant + 1, d->variants);
        bool var = d->variants > 1 && button(x + 100, by, w - 104, 15, buf, false);
        if (d->variants <= 1) font_draw(g_cv, x + 104, by + 4, tr(S_ONE_COLOR), COL_DIM);
        if (!(minus || plus || var)) continue;
        if (slot < 0) {
            if (minus || c->species_count >= CFG_MAX_SPECIES) {
                if (!minus) toast(tr(S_TOO_MANY_SPECIES), now);
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
            else toast(tr(S_CREATURE_LIMIT), now);
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
    y = section(x, y, w, tr(S_LIGHT));
    int v = c->brightness;
    if (slider(x, y + 2, w, tr(S_BRIGHTNESS), &v, 10, 100)) {
        c->brightness = (uint8_t)v;
        app_config_changed(0);
    }
    y += 32;
    y = section(x, y, w, tr(S_SIMULATION));
    v = c->sim_speed;
    if (slider(x, y + 2, w, tr(S_SPEED), &v, 50, 200)) {
        c->sim_speed = (uint8_t)v;
        app_config_changed(0);
    }
    y += 30;
    v = c->creature_limit;
    if (slider(x, y + 2, w, tr(S_CREATURE_LIMIT), &v, 10, 120)) {
        c->creature_limit = (uint16_t)v;
        app_config_changed(SC_CREATURES);
    }
    y += 32;
    y = section(x, y, w, tr(S_SCREEN));
    font_draw(g_cv, x, y + 5, tr(S_MAX_FPS), COL_TEXT);
    // segmented control
    if (button(x + w - 58, y, 29, 17, "20", c->fps_cap == 20)) {
        c->fps_cap = 20;
        app_config_changed(0);
    }
    if (button(x + w - 29, y, 29, 17, "30", c->fps_cap == 30)) {
        c->fps_cap = 30;
        app_config_changed(0);
    }
    y += 22;
    static const uint16_t OFF[] = {0, 5, 10, 30, 60, 120};
    int idx = 0;
    for (int i = 0; i < 6; i++)
        if (OFF[i] == c->screen_off_minutes) idx = i;
    char buf[40];
    if (c->screen_off_minutes) snprintf(buf, sizeof(buf), tr(S_FMT_SCREEN_OFF), c->screen_off_minutes);
    else snprintf(buf, sizeof(buf), "%s", tr(S_SCREEN_OFF_NEVER));
    if (button(x, y, w, ROW - 6, buf, false)) {
        c->screen_off_minutes = OFF[(idx + 1) % 6];
        app_config_changed(0);
    }
    y += ROW + 2;
    y = section(x, y, w, tr(S_LANGUAGE));
    // one segment per language, each labelled with its own code
    int lw = (w - 4 * (LANG_COUNT - 1)) / LANG_COUNT;
    for (int l = 0; l < LANG_COUNT; l++)
        if (button(x + l * (lw + 4), y, lw, 17, lang_code((Lang)l), c->language == l) && c->language != l) {
            c->language = (uint8_t)l;
            app_config_changed(0);
        }
    y += 24;
    y = section(x, y, w, tr(S_NETWORK));
    font_draw(g_cv, x, y, tr(S_WIFI_NOT_SET), COL_DIM);
    return y + 14;
}

static int page_presets(int x, int y, int w, uint32_t now)
{
    for (int s = 0; s < CFG_MAX_USER_PRESETS; s++, y += 42) {
        const char *n = storage_preset_name(s);
        card(x, y, w, 38, COL_CARD, n[0] ? COL_ACCENT_DK : COL_CARD_EDGE);
        char num[4];
        snprintf(num, sizeof(num), "%d", s + 1);
        canvas_fill_circle(g_cv, x + 10, y + 9, 6, n[0] ? COL_ACCENT : COL_TRACK);
        font_draw_centered(g_cv, x + 10, y + 6, num, n[0] ? COL_ON_ACCENT : COL_DIM);
        font_draw(g_cv, x + 21, y + 6, n[0] ? n : tr(S_EMPTY_SLOT), n[0] ? COL_TEXT : COL_DIM);
        int bw = (w - 16) / 3, by = y + 18;
        if (button(x + 4, by, bw, 16, tr(S_SAVE), !n[0])) {
            char name[PRESET_NAME_LEN];
            int ti = theme_index(app_cfg()->theme_id);
            snprintf(name, sizeof(name), "%s %d", ti >= 0 ? theme_name(ti) : tr(S_CUSTOM_PRESET), s + 1);
            storage_request_preset_save(s, app_cfg(), name);
            toast(tr(S_PRESET_SAVED), now);
        }
        if (n[0] && button(x + 8 + bw, by, bw, 16, tr(S_LOAD), false)) {
            AquariumConfig c = *app_cfg();
            if (storage_preset_load(s, &c)) app_load_config(&c);
            toast(tr(S_PRESET_LOADED), now);
        }
        if (n[0] && button(x + 12 + 2 * bw, by, bw, 16, tr(S_DELETE), false)) {
            storage_request_preset_delete(s);
            toast(tr(S_PRESET_DELETED), now);
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

// Placement fields of the selected object (x, vertical offset, rotation, size); x is NULL if none.
typedef struct {
    int16_t *x, *dy, *rot;
    uint8_t *scale;
} Placement;

static Placement sel_place(void)
{
    AquariumConfig *c = app_cfg();
    if (U.sel_kind == SEL_PLANT && U.sel_index < c->plant_count) {
        PlantCfg *p = &c->plants[U.sel_index];
        return (Placement){&p->x, &p->dy, &p->rot, &p->scale};
    }
    if (U.sel_kind == SEL_DECOR && U.sel_index < c->decor_count) {
        DecorCfg *d = &c->decor[U.sel_index];
        return (Placement){&d->x, &d->dy, &d->rot, &d->scale};
    }
    return (Placement){NULL, NULL, NULL, NULL};
}

#define DY_SNAP 3 // within this many px of the substrate the object snaps back onto it

static void edit_gesture(const GestureEvent *e)
{
    if (e->y >= BAR_Y) return; // the bottom bar is handled as widgets
    if (e->type == GE_DOWN) {
        int idx;
        int kind = scene_pick(e->x, e->y, &idx);
        if (kind != SEL_NONE) edit_select(kind, idx);
        Placement pl = sel_place();
        U.dragging = kind != SEL_NONE && pl.x;
        if (U.dragging) {
            U.drag_off = *pl.x - e->x;
            U.drag_y0 = e->y;
            U.drag_dy0 = *pl.dy;
        }
    } else if (e->type == GE_MOVE && U.dragging) {
        Placement pl = sel_place();
        if (!pl.x) return;
        int nx = e->x + U.drag_off;
        nx = nx < 4 ? 4 : (nx > CANVAS_W - 5 ? CANVAS_W - 5 : nx);
        *pl.x = (int16_t)nx;
        // vertical finger motion changes the height above the substrate; moving sideways
        // keeps it (objects resting on the bottom follow its profile), small offsets snap to 0
        int fl = world_floor(nx);
        int dy = U.drag_dy0 + (e->y - U.drag_y0);
        int lo = SURFACE_Y + 6 - fl;
        if (lo < CFG_DY_MIN) lo = CFG_DY_MIN;
        if (dy < lo) dy = lo;
        if (dy > CFG_DY_MAX) dy = CFG_DY_MAX;
        if (dy >= -DY_SNAP && dy <= DY_SNAP) dy = 0;
        *pl.dy = (int16_t)dy;
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
    // glass bar: opaque gradient + accent hairline
    for (int y = BAR_Y; y < CANVAS_H; y++)
        canvas_hline(g_cv, 0, CANVAS_W - 1, y, c565_lerp(COL_HEADER, COL_BG_BOT, (uint8_t)((y - BAR_Y) * 255 / 24)));
    canvas_hline(g_cv, 0, CANVAS_W - 1, BAR_Y, COL_ACCENT_DK);
    int y = BAR_Y + 3, h = 18;
    if (button(4, y, 36, h, tr(S_DONE), true)) {
        U.editing = false;
        scene_set_edit(false, SEL_NONE, -1);
        app_config_changed(0);
    }
    if (button(43, y, 46, h, tr(S_CANCEL), false)) {
        *c = U.backup;
        U.editing = false;
        scene_set_edit(false, SEL_NONE, -1);
        app_config_changed(SC_STATIC | SC_PLANTS);
    }
    Placement pl = sel_place();
    if (!pl.x) {
        icon(102, y + 3, IC_EDIT);
        font_draw(g_cv, 118, y + 5, tr(S_EDIT_HINT), COL_DIM);
        return;
    }
    bool decor = U.sel_kind == SEL_DECOR;
    uint8_t *layer = decor ? &c->decor[U.sel_index].layer : &c->plants[U.sel_index].layer;
    uint16_t *color = decor ? &c->decor[U.sel_index].color : &c->plants[U.sel_index].color;
    unsigned flag = decor ? SC_STATIC : SC_PLANTS;
    if (button(93, y, 46, h, tr(*layer ? S_FRONT : S_BACK), false)) {
        *layer = !*layer;
        app_config_changed(SC_STATIC | SC_PLANTS);
    }
    // colour: the swatch itself is the button (tap = next colour)
    {
        bool hover;
        if (clickable(142, y, 18, h, &hover)) {
            int cur = 0;
            for (int i = 0; i < SWATCH_COUNT; i++)
                if (SWATCHES[i] == *color) cur = i;
            *color = SWATCHES[(cur + 1) % SWATCH_COUNT];
            app_config_changed(flag);
        }
        rrect(142, y, 18, h, hover ? COL_ACCENT : COL_TEXT);
        rrect(144, y + 2, 14, h - 4, *color);
        canvas_hline(g_cv, 146, 155, y + 3, c565_lerp(*color, RGB565(255, 255, 255), 80));
    }
    // rotation in 15 degree steps around the base point
    if (button(164, y, 24, h, "-15", false)) {
        *pl.rot = cfg_norm_rot(*pl.rot - 15);
        app_config_changed(flag);
    }
    if (button(190, y, 24, h, "+15", false)) {
        *pl.rot = cfg_norm_rot(*pl.rot + 15);
        app_config_changed(flag);
    }
    // back onto the substrate, upright
    if (button(218, y, 34, h, tr(S_TO_FLOOR), false) && (*pl.dy || *pl.rot)) {
        *pl.dy = 0;
        *pl.rot = 0;
        app_config_changed(flag);
    }
    // size, in 10 % steps (plants and decorations)
    char info[16];
    int sc = *pl.scale ? *pl.scale : 100;
    if (button(256, y, 17, h, "-", false) && sc > CFG_SCALE_MIN) {
        *pl.scale = (uint8_t)(sc - CFG_SCALE_STEP);
        app_config_changed(flag);
    }
    snprintf(info, sizeof(info), "%d%%", sc);
    rrect(275, y + 3, 30, 12, COL_TRACK);
    font_draw_centered(g_cv, 290, y + 5, info, sc != 100 ? COL_ACCENT : COL_DIM);
    if (button(307, y, 17, h, "+", false) && sc < CFG_SCALE_MAX) {
        *pl.scale = (uint8_t)(sc + CFG_SCALE_STEP);
        app_config_changed(flag);
    }
    snprintf(info, sizeof(info), "%d", *pl.rot);
    rrect(327, y + 3, 18, 12, COL_TRACK);
    font_draw_centered(g_cv, 336, y + 5, info, *pl.rot ? COL_ACCENT : COL_DIM);
    if (button(348, y, 48, h, tr(S_DELETE), false)) {
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

// ------------------------------------------------------------------ panel chrome

// Opaque abyss gradient + a few faint bubbles rising behind the content.
static void panel_background(int px)
{
    static uint16_t rows[CANVAS_H];
    static bool ready;
    if (!ready) {
        for (int y = 0; y < CANVAS_H; y++) rows[y] = c565_lerp(COL_BG_TOP, COL_BG_BOT, (uint8_t)(y * 255 / (CANVAS_H - 1)));
        ready = true;
    }
    for (int y = 0; y < CANVAS_H; y++) canvas_hline(g_cv, px, CANVAS_W - 1, y, rows[y]);
    float t = (float)g_now * 0.001f;
    for (int i = 0; i < 7; i++) {
        uint32_t h = hash32(0xB0B1u + (uint32_t)i * 977u);
        float speed = 10.0f + (float)(h % 14);
        float fy = (float)CANVAS_H + 8.0f - fmodf_pos(t * speed + (float)(h >> 8 & 255), (float)CANVAS_H + 16.0f);
        int bx = px + 8 + (int)((h >> 16) % (uint32_t)(PANEL_W - 16)) + (int)(fm_sin(t * 1.3f + (float)i) * 2.0f);
        int r = 1 + (int)(h % 3);
        uint16_t c = c565_lerp(rows[fy < 0 ? 0 : (fy >= CANVAS_H ? CANVAS_H - 1 : (int)fy)], COL_ACCENT, 50);
        canvas_circle(g_cv, bx, (int)fy, r, c);
    }
}

// Soft shadow over the scene + a light sweep running down the panel edge.
static void panel_edge(int px)
{
    static const uint8_t SH[4] = {30, 55, 85, 120};
    for (int i = 0; i < 4; i++) canvas_blend_rect(g_cv, px - 4 + i, 0, 1, CANVAS_H, RGB565(0, 0, 0), SH[i]);
    float sweep = fmodf_pos((float)g_now * 0.09f, (float)CANVAS_H + 120.0f) - 60.0f;
    for (int y = 0; y < CANVAS_H; y++) {
        float d = ((float)y - sweep) / 40.0f;
        float k = 1.0f - d * d;
        uint16_t c = k > 0 ? c565_lerp(COL_ACCENT_DK, COL_ACCENT, (uint8_t)(k * 255.0f)) : COL_ACCENT_DK;
        canvas_pset(g_cv, px, y, c);
    }
}

// Header band with an animated wave along its bottom edge.
static void panel_header(int px)
{
    for (int y = 0; y < HEADER_H; y++)
        canvas_hline(g_cv, px + 1, CANVAS_W - 1, y, c565_lerp(COL_HEADER, COL_BG_TOP, (uint8_t)(y * 255 / HEADER_H)));
    float t = (float)g_now * 0.004f;
    for (int x = px + 1; x < CANVAS_W; x++) {
        int wy = HEADER_H - 2 + (int)(fm_sin((float)x * 0.18f + t) * 1.3f + 0.5f);
        canvas_pset(g_cv, x, wy, COL_ACCENT_DK);
        canvas_pset(g_cv, x, wy + 1, c565_lerp(COL_ACCENT_DK, COL_BG_TOP, 140));
    }
}

// ------------------------------------------------------------------ frame

void ui_draw(Canvas *cv, uint32_t now)
{
    g_cv = cv;
    g_now = now;
    U.id = 0;
    if (U.editing) {
        edit_draw(now);
    } else if (U.open) {
        if (U.open_fresh) {
            U.open_at = now;
            U.open_fresh = false;
        }
        // slide-in with ease-out (input uses the same offset, so taps stay consistent)
        uint32_t el = now - U.open_at;
        float k = el >= OPEN_MS ? 1.0f : (float)el / (float)OPEN_MS;
        k = 1.0f - (1.0f - k) * (1.0f - k) * (1.0f - k);
        int slide = (int)((1.0f - k) * (float)PANEL_W);
        int px = PANEL_X + slide;

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

        panel_edge(px);
        panel_background(px + 1);
        panel_header(px);
        int hx = px + PAD;
        if (U.page != P_MAIN) {
            if (round_button(hx - 4, 2, 20, 20, '<')) nav(P_MAIN);
            hx += 20;
        }
        static const int PAGE_ICON[P_COUNT] = {IC_ENV, IC_BIOME, IC_WATER, IC_SUBSTRATE, IC_PLANTS, IC_CREATURES, IC_ENV,
                                               IC_PRESET};
        if (U.page != P_MAIN) icon(hx, 6, PAGE_ICON[U.page]), hx += 16;
        text_bold(hx, 8, page_title(U.page), COL_TEXT);
        if (round_button(CANVAS_W - 22, 2, 20, 20, 'x')) ui_close();

        // content (clipped)
        canvas_set_clip(cv, px + 1, CONTENT_Y, CANVAS_W - px - 1, CONTENT_H);
        U.in_content = true;
        int x = px + PAD, w = PANEL_W - 2 * PAD, y0 = CONTENT_Y + 6 - U.scroll, y = y0;
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
        // scrollbar pill
        if (U.open && U.content_h > CONTENT_H) {
            int bh = CONTENT_H * CONTENT_H / U.content_h;
            if (bh < 12) bh = 12;
            int by = CONTENT_Y + U.scroll * (CONTENT_H - bh) / (U.content_h - CONTENT_H);
            rrect(CANVAS_W - 4, CONTENT_Y + 1, 3, CONTENT_H - 2, RGB565(8, 26, 36));
            rrect(CANVAS_W - 4, by, 3, bh, U.scrolling ? COL_ACCENT : COL_ACCENT_DK);
        }
    }
    if (now < U.toast_until) { // pill toast with an accent dot
        int tw = font_width(U.toast) + 26;
        int tx = (CANVAS_W - tw) / 2, ty = CANVAS_H - 44;
        card(tx, ty, tw, 18, COL_BG_BOT, COL_ACCENT_DK);
        canvas_fill_circle(g_cv, tx + 10, ty + 9, 3, COL_ACCENT);
        font_draw(cv, tx + 18, ty + 6, U.toast, COL_TEXT);
    }
    if (U.in.released) {
        U.scrolling = false;
        U.active_is_slider = false;
    }
    U.in.pressed = false;
    U.in.released = false;
}
