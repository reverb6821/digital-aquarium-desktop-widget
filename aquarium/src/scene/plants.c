// Procedural plants: each plant is a small tree of nodes generated from a
// seed (simple L-system-like recursion). Drawn every frame with a sway that
// grows with height, so no cache is needed (plants are thin and cheap).
// Broad leaves (Cryptocoryne, Anubias, Microsorum...) are chains of K_WIDE
// nodes with a per-node half-width, filled with short spans.
#include <string.h>
#include "../engine/fixmath.h"
#include "../engine/rng.h"
#include "world.h"

enum { K_STEM = 0, K_LEAF, K_WIDE, K_PAD };

typedef struct {
    int16_t rx, ry;  // rest position relative to the plant base
    int16_t parent;  // index in pool, -1 for root
    uint8_t kind;
    uint8_t shade;   // 0..255 (0 = base colour darkened, 255 = light tip)
    uint8_t w;       // K_WIDE: half-width in 1/4 px; K_PAD: radius in px
} PNode;

typedef struct {
    uint16_t first, count;
    int16_t bx, by, height, halfw;
    uint8_t layer, type;
    uint16_t color;
    float amp, speed, phase;
} Plant;

#define NODE_POOL 3200
static PNode s_nodes[NODE_POOL];
static int s_nnodes;
static Plant s_plants[CFG_MAX_PLANTS];
static int s_nplants;

static int add_node_w(int parent, float x, float y, int kind, int shade, int w)
{
    if (s_nnodes >= NODE_POOL) return -1;
    s_nodes[s_nnodes] = (PNode){(int16_t)x,       (int16_t)y, (int16_t)parent, (uint8_t)kind,
                                (uint8_t)im_clamp(shade, 0, 255), (uint8_t)im_clamp(w, 0, 255)};
    return s_nnodes++;
}

static int add_node(int parent, float x, float y, int kind, int shade)
{
    return add_node_w(parent, x, y, kind, shade, 0);
}

static int add_wide(int parent, float x, float y, float halfw, int shade)
{
    return add_node_w(parent, x, y, K_WIDE, shade, (int)(halfw * 4.0f + 0.5f));
}

// One broad leaf from (x,y) along `ang`. Profile: 0 at the base, widest at
// `peak` (0..1 along the blade), pointed tip. `wavy` ripples the edge.
static int leaf(Rng *r, int parent, float x, float y, float ang, float len, float maxw, float peak, float curve,
                float wavy, int shade0, int h)
{
    const float step = 2.0f;
    int node = add_wide(parent, x, y, 0.5f, shade0);
    if (node < 0) return parent;
    float ph = rng_frange(r, 0, FM_TAU);
    for (float d = step; d <= len; d += step) {
        float u = d / len;
        float prof = u < peak ? fm_sin(u / peak * FM_PI * 0.5f) : fm_cos((u - peak) / (1.0f - peak) * FM_PI * 0.5f);
        if (prof < 0) prof = 0;
        float w = maxw * prof * (1.0f + wavy * fm_sin(d * 1.3f + ph));
        ang += curve;
        x += fm_cos(ang) * step;
        y += fm_sin(ang) * step;
        int n = add_wide(node, x, y, w, shade0 + (int)(-y * 120.0f / (float)h) + (int)(u * 60));
        if (n < 0) break;
        node = n;
    }
    return node;
}

// Grow a curving stem from (x,y) along `ang` for `len` px in steps; returns tip node.
static int grow(Rng *r, int parent, float *x, float *y, float ang, float len, float curve, int kind, int h)
{
    float step = 3.0f;
    int node = parent;
    for (float d = 0; d < len; d += step) {
        ang += curve + rng_frange(r, -0.08f, 0.08f);
        *x += fm_cos(ang) * step;
        *y += fm_sin(ang) * step;
        node = add_node(node, *x, *y, kind, (int)(-*y * 255.0f / (float)h));
        if (node < 0) return parent;
    }
    return node;
}

static void build_plant(Plant *p, const PlantCfg *pc)
{
    Rng r;
    rng_seed(&r, pc->seed * 2654435761u + pc->type);
    p->first = (uint16_t)s_nnodes;
    p->bx = pc->x;
    p->by = (int16_t)(world_floor(pc->x) + 2);
    p->height = pc->height;
    p->layer = pc->layer;
    p->type = pc->type;
    p->color = pc->color;
    p->phase = rng_frange(&r, 0, FM_TAU);
    p->speed = rng_frange(&r, 0.7f, 1.1f);
    p->amp = 2.0f;
    const int h = pc->height;
    int root = add_node(-1, 0, 0, K_STEM, 0);

    switch (pc->type) {
    case PLANT_STEM: { // bunch of upright stems with opposite leaf pairs (Rotala, Egeria)
        int stems = rng_range(&r, 4, 7);
        for (int st = 0; st < stems; st++) {
            float x = rng_frange(&r, -6, 6), y = 0;
            float ang = -FM_PI / 2 + rng_frange(&r, -0.2f, 0.2f);
            float len = (float)h * rng_frange(&r, 0.6f, 1.0f);
            int prev = root;
            int k = 0;
            for (float d = 0; d < len; d += 3.0f, k++) {
                ang += rng_frange(&r, -0.06f, 0.06f);
                x += fm_cos(ang) * 3.0f;
                y += fm_sin(ang) * 3.0f;
                int sh = (int)(d * 255 / len);
                int n = add_node(prev, x, y, K_STEM, sh);
                if (n < 0) break;
                prev = n;
                if (d < 4.0f) continue;
                float ll = 2.0f + 1.5f * (1.0f - d / len); // leaves shrink towards the tip
                for (int side = -1; side <= 1; side += 2) {
                    float la = ang + (float)side * (1.15f + ((k & 1) ? 0.15f : 0.0f));
                    add_node(n, x + fm_cos(la) * ll, y + fm_sin(la) * ll - 0.5f, K_STEM, sh + 30);
                }
            }
        }
        p->amp = 2.2f;
        break;
    }
    case PLANT_VALLIS: {
        int blades = rng_range(&r, 5, 8);
        for (int b = 0; b < blades; b++) {
            float x = rng_frange(&r, -4, 4), y = 0;
            float ang = -FM_PI / 2 + rng_frange(&r, -0.25f, 0.25f);
            grow(&r, root, &x, &y, ang, (float)h * rng_frange(&r, 0.55f, 1.0f), rng_frange(&r, -0.04f, 0.04f), K_STEM, h);
        }
        p->amp = 5.0f;
        break;
    }
    case PLANT_CRYPT: { // rosette of wavy lanceolate leaves on short petioles
        int n = rng_range(&r, 6, 9);
        for (int i = 0; i < n; i++) {
            float a = -FM_PI / 2 + ((float)i / (float)(n - 1) - 0.5f) * 1.9f + rng_frange(&r, -0.12f, 0.12f);
            float x = 0, y = 0;
            int pt = grow(&r, root, &x, &y, a, (float)h * rng_frange(&r, 0.15f, 0.3f), 0, K_STEM, h);
            float out = a < -FM_PI / 2 ? -0.03f : 0.03f;
            leaf(&r, pt, x, y, a, (float)h * rng_frange(&r, 0.55f, 0.8f), rng_frange(&r, 1.8f, 2.6f), 0.4f, out, 0.25f,
                 60, h);
        }
        p->amp = 1.4f;
        break;
    }
    case PLANT_MICROSORUM: { // Java fern: long narrow arching leaves from a creeping rhizome
        int n = rng_range(&r, 6, 9);
        float x = -6, y = 0;
        int rh = grow(&r, root, &x, &y, 0.0f, 12.0f, 0, K_STEM, h);
        (void)rh;
        for (int i = 0; i < n; i++) {
            float bx = -6.0f + 12.0f * (float)i / (float)(n - 1);
            float a = -FM_PI / 2 + rng_frange(&r, -0.55f, 0.55f) + bx * 0.03f;
            float curve = (a < -FM_PI / 2 ? -1.0f : 1.0f) * rng_frange(&r, 0.01f, 0.035f);
            leaf(&r, root, bx, 0, a, (float)h * rng_frange(&r, 0.6f, 1.0f), rng_frange(&r, 1.6f, 2.4f), 0.45f, curve,
                 0.0f, 30, h);
        }
        p->amp = 1.8f;
        break;
    }
    case PLANT_BOLBITIS: { // fronds with alternate lobed leaflets
        int fronds = rng_range(&r, 4, 6);
        for (int f = 0; f < fronds; f++) {
            float a = -FM_PI / 2 + ((float)f / (float)(fronds - 1) - 0.5f) * 1.4f + rng_frange(&r, -0.1f, 0.1f);
            float x = rng_frange(&r, -4, 4), y = 0;
            float len = (float)h * rng_frange(&r, 0.7f, 1.0f);
            float curve = (a < -FM_PI / 2 ? -1.0f : 1.0f) * 0.02f;
            int prev = root;
            int k = 0;
            for (float d = 0; d < len; d += 3.0f, k++) {
                a += curve;
                x += fm_cos(a) * 3.0f;
                y += fm_sin(a) * 3.0f;
                int n = add_node(prev, x, y, K_STEM, (int)(d * 200 / len));
                if (n < 0) break;
                prev = n;
                if (d > len * 0.25f && (k & 1)) {
                    float side = ((k >> 1) & 1) ? 1.0f : -1.0f;
                    float ll = (len - d) * 0.25f + 4.0f;
                    leaf(&r, n, x, y, a + side * 0.85f, ll, 1.6f, 0.45f, -side * 0.06f, 0.45f, 40, h);
                }
            }
            leaf(&r, prev, x, y, a, 7.0f, 1.8f, 0.4f, 0, 0.4f, 70, h); // terminal leaflet
        }
        p->amp = 1.6f;
        break;
    }
    case PLANT_ANUBIAS:
    case PLANT_BUCE: { // broad oval leaves on petioles (Bucephalandra: smaller)
        bool buce = pc->type == PLANT_BUCE;
        int n = rng_range(&r, buce ? 6 : 5, buce ? 10 : 8);
        for (int i = 0; i < n; i++) {
            float a = -FM_PI / 2 + rng_frange(&r, -1.1f, 1.1f);
            float x = rng_frange(&r, -5, 5), y = 0;
            int pt = grow(&r, root, &x, &y, a, (float)h * rng_frange(&r, 0.25f, 0.55f), 0, K_STEM, h);
            float la = a + rng_frange(&r, -0.4f, 0.4f);
            if (buce) leaf(&r, pt, x, y, la, rng_frange(&r, 4, 6), rng_frange(&r, 1.2f, 1.7f), 0.5f, 0, 0.1f, 80, h);
            else leaf(&r, pt, x, y, la, (float)h * rng_frange(&r, 0.35f, 0.5f), rng_frange(&r, 2.4f, 3.2f), 0.5f, 0,
                      0.0f, 50, h);
        }
        p->amp = buce ? 0.6f : 0.9f;
        break;
    }
    case PLANT_ECHINO: { // Amazon sword: long petioles arching out, big pointed leaves
        int n = rng_range(&r, 8, 12);
        for (int i = 0; i < n; i++) {
            float a = -FM_PI / 2 + rng_frange(&r, -0.9f, 0.9f);
            float out = a < -FM_PI / 2 ? -0.05f : 0.05f;
            float x = 0, y = 0;
            int pt = grow(&r, root, &x, &y, a, (float)h * rng_frange(&r, 0.3f, 0.5f), out * 0.5f, K_STEM, h);
            leaf(&r, pt, x, y, a + out * 4.0f, (float)h * rng_frange(&r, 0.4f, 0.6f), rng_frange(&r, 2.2f, 3.0f), 0.35f,
                 out, 0.0f, 90, h);
        }
        p->amp = 2.4f;
        break;
    }
    case PLANT_MOSS: {
        for (int i = 0; i < 50; i++) {
            float ax = rng_frange(&r, -1, 1), ay = rng_frange(&r, 0, 1);
            if (ax * ax + ay * ay > 1) continue;
            add_node(root, ax * (float)h * 1.8f, -ay * (float)h, K_LEAF, (int)(ay * 200) + rng_range(&r, 0, 55));
        }
        p->amp = 0.3f;
        break;
    }
    case PLANT_LILY:
    default: { // water lily: thin stems up to floating pads, a few submerged leaves
        float top = (float)(SURFACE_Y + 2 - p->by); // pads float at the water line
        float reach = (float)h > -top ? -top : (float)h;
        int stems = rng_range(&r, 2, 4);
        for (int s = 0; s < stems; s++) {
            float x = rng_frange(&r, -3, 3), y = 0;
            float ang = -FM_PI / 2 + rng_frange(&r, -0.2f, 0.2f);
            int prev = root;
            float len = reach * rng_frange(&r, 0.85f, 1.0f);
            for (float d = 0; d < len; d += 4.0f) {
                ang += rng_frange(&r, -0.04f, 0.04f);
                x += fm_cos(ang) * 4.0f;
                y += fm_sin(ang) * 4.0f;
                int n = add_node(prev, x, y, K_STEM, (int)(d * 120 / len));
                if (n < 0) break;
                prev = n;
            }
            add_node_w(prev, x, y, K_PAD, 200, rng_range(&r, 5, 8));
        }
        for (int i = 0; i < 3; i++) {
            float a = -FM_PI / 2 + rng_frange(&r, -0.9f, 0.9f);
            float x = 0, y = 0;
            int pt = grow(&r, root, &x, &y, a, rng_frange(&r, 6, 12), 0, K_STEM, h);
            leaf(&r, pt, x, y, a, rng_frange(&r, 6, 9), 3.0f, 0.5f, 0, 0.0f, 40, h);
        }
        p->amp = 1.0f;
        break;
    }
    }
    p->count = (uint16_t)(s_nnodes - p->first);
    int maxw = 4;
    for (int i = p->first; i < p->first + p->count; i++) {
        int ax = s_nodes[i].rx < 0 ? -s_nodes[i].rx : s_nodes[i].rx;
        if (ax > maxw) maxw = ax;
    }
    p->halfw = (int16_t)maxw;
}

void plants_build(const AquariumConfig *c)
{
    s_nnodes = 0;
    s_nplants = 0;
    for (int i = 0; i < c->plant_count; i++) build_plant(&s_plants[s_nplants++], &c->plants[i]);
}

static inline float sway(const Plant *p, int ry, float t)
{
    float hf = (float)(-ry) / (float)(p->height > 0 ? p->height : 1);
    if (hf < 0) hf = 0;
    return fm_sin(t * p->speed + p->phase + (float)ry * 0.05f) * p->amp * hf * hf;
}

// Filled tapered segment (w0/w1 half-widths in 1/4 px): spans perpendicular to
// the dominant axis, dark on one side of the midrib, light midrib.
static void leaf_segment(Canvas *cv, int x0, int y0, int w0, int x1, int y1, int w1, uint16_t col, uint16_t dark)
{
    int dx = x1 - x0, dy = y1 - y0;
    int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
    int n = adx > ady ? adx : ady;
    if (n == 0) n = 1;
    const uint16_t mid = c565_lerp(col, RGB565(210, 240, 190), 60);
    for (int i = 0; i <= n; i++) {
        int cx = x0 + dx * i / n, cy = y0 + dy * i / n;
        int hw = (w0 + (w1 - w0) * i / n + 2) / 4;
        if (ady >= adx) { // mostly vertical: horizontal span
            for (int k = -hw; k <= hw; k++)
                canvas_pset(cv, cx + k, cy, k == 0 && hw > 0 ? mid : ((k > 0) == (dy < 0) ? dark : col));
        } else {
            for (int k = -hw; k <= hw; k++)
                canvas_pset(cv, cx, cy + k, k == 0 && hw > 0 ? mid : (k > 0 ? dark : col));
        }
    }
}

void plants_draw(Canvas *cv, int layer, float t, int highlight)
{
    for (int pi = 0; pi < s_nplants; pi++) {
        const Plant *p = &s_plants[pi];
        if (p->layer != layer) continue;
        const uint16_t dark = c565_scale(p->color, 150);
        const uint16_t light = c565_lerp(p->color, RGB565(220, 255, 230), 90);
        const bool hl = pi == highlight;
        for (int i = p->first; i < p->first + p->count; i++) {
            const PNode *n = &s_nodes[i];
            int x = p->bx + n->rx + (int)sway(p, n->ry, t);
            int y = p->by + n->ry;
            uint16_t col = n->shade < 128 ? c565_lerp(dark, p->color, (uint8_t)(n->shade * 2))
                                          : c565_lerp(p->color, light, (uint8_t)((n->shade - 128) * 2));
            if (hl) col = c565_lerp(col, RGB565(255, 255, 160), 140);
            switch (n->kind) {
            case K_LEAF: canvas_pset(cv, x, y, col); break;
            case K_PAD: { // floating leaf seen edge-on: flat ellipse with a notch
                int rw = n->w;
                canvas_hline(cv, x - rw, x + rw, y, col);
                canvas_hline(cv, x - rw + 2, x + rw - 2, y - 1, c565_lerp(col, light, 120));
                canvas_hline(cv, x - rw + 1, x + rw - 1, y + 1, dark);
                canvas_pset(cv, x + rw / 3, y - 1, col); // notch
                break;
            }
            case K_WIDE:
                if (n->parent >= 0) {
                    const PNode *q = &s_nodes[n->parent];
                    int px = p->bx + q->rx + (int)sway(p, q->ry, t), py = p->by + q->ry;
                    leaf_segment(cv, px, py, q->kind == K_WIDE ? q->w : 0, x, y, n->w, col, dark);
                }
                break;
            default:
                if (n->parent >= 0) {
                    const PNode *q = &s_nodes[n->parent];
                    int px = p->bx + q->rx + (int)sway(p, q->ry, t), py = p->by + q->ry;
                    canvas_line(cv, px, py, x, y, col);
                }
                break;
            }
        }
    }
}

int plants_hit(int x, int y)
{
    for (int i = s_nplants - 1; i >= 0; i--) {
        const Plant *p = &s_plants[i];
        int hw = p->halfw + 3;
        if (x >= p->bx - hw && x <= p->bx + hw && y <= p->by + 3 && y >= p->by - p->height - 4) return i;
    }
    return -1;
}
