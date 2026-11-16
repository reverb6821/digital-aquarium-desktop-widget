// Procedural plants: each plant is a small tree of nodes generated from a
// seed (simple L-system-like recursion). Drawn every frame with a sway that
// grows with height, so no cache is needed (plants are thin and cheap).
// Broad leaves (Cryptocoryne, Anubias, Microsorum...) are chains of K_WIDE
// nodes with a per-node half-width, filled as shaded quads (narrow blades
// use cheaper spans).
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
    int16_t bx, by, height;
    int16_t x0, y0, x1, y1; // bounding box of the nodes, relative to the base
    uint8_t layer, type;
    uint16_t color;
    float amp, speed, phase;
} Plant;

#define NODE_POOL 4096
static PNode s_nodes[NODE_POOL];
static float s_sx[NODE_POOL]; // per-frame swayed x of every node (parents come before children)
static int s_nnodes;
static Plant s_plants[CFG_MAX_PLANTS];
static int s_nplants;

static int add_node_w(int parent, float x, float y, int kind, int shade, int w)
{
    if (s_nnodes >= NODE_POOL) return -1;
    if (y > 1.0f) y = 1.0f; // never grow below the substrate
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
static int leaf_s(Rng *r, int parent, float x, float y, float ang, float len, float maxw, float peak, float curve,
                  float wavy, int shade0, int h, float step)
{
    int node = add_wide(parent, x, y, 0.5f, shade0);
    if (node < 0) return parent;
    float ph = rng_frange(r, 0, FM_TAU);
    for (float d = step; d <= len + 0.01f; d += step) {
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

static int leaf(Rng *r, int parent, float x, float y, float ang, float len, float maxw, float peak, float curve,
                float wavy, int shade0, int h)
{
    // short or wavy blades need fine steps; long smooth ones get away with fewer segments
    float step = (len > 10.0f && wavy < 0.15f) ? 3.0f : 2.0f;
    return leaf_s(r, parent, x, y, ang, len, maxw, peak, curve, wavy, shade0, h, step);
}

// Long ribbon of constant width with a short pointed tip (Vallisneria).
static void ribbon(Rng *r, int parent, float x, float y, float ang, float len, float w, float curve, int h)
{
    const float step = 4.0f;
    int node = add_wide(parent, x, y, w * 0.6f, 0);
    for (float d = step; d <= len && node >= 0; d += step) {
        float u = d / len;
        float ww = u > 0.85f ? w * (1.0f - u) / 0.15f + 0.3f : w;
        ang += curve + rng_frange(r, -0.01f, 0.01f);
        x += fm_cos(ang) * step;
        y += fm_sin(ang) * step;
        node = add_wide(node, x, y, ww, (int)(-y * 230.0f / (float)h));
    }
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

static inline bool reddish(uint16_t c) { return c565_r8(c) > c565_g8(c); }

static void build_plant(Plant *p, const PlantCfg *pc)
{
    Rng r;
    rng_seed(&r, pc->seed * 2654435761u + pc->type);
    p->first = (uint16_t)s_nnodes;
    p->bx = pc->x;
    p->by = (int16_t)(world_floor(pc->x) + 2 + pc->dy);
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
    case PLANT_STEM:
        if (reddish(pc->color)) { // Rotala rotundifolia: rounded bush of fine stems, tiny leaves, pink tips
            int stems = rng_range(&r, 8, 10);
            for (int st = 0; st < stems; st++) {
                float f = (float)st / (float)(stems - 1) * 2.0f - 1.0f; // -1..1 across the bush
                float x = f * 9.0f + rng_frange(&r, -1.5f, 1.5f), y = 0;
                float ang = -FM_PI / 2 + f * 0.75f + rng_frange(&r, -0.1f, 0.1f);
                float len = (float)h * (1.0f - 0.45f * f * f) * rng_frange(&r, 0.8f, 1.0f);
                int prev = root;
                int k = 0;
                for (float d = 0; d < len; d += 3.0f, k++) {
                    ang += -f * 0.03f + rng_frange(&r, -0.05f, 0.05f); // bend back upwards
                    x += fm_cos(ang) * 3.0f;
                    y += fm_sin(ang) * 3.0f;
                    int sh = (int)(d * 255 / len);
                    int n = add_node(prev, x, y, K_STEM, sh);
                    if (n < 0) break;
                    prev = n;
                    if (d < 3.0f) continue;
                    float u = d / len;
                    float ll = 3.2f - 1.2f * u;
                    float spread = 1.0f - 0.45f * u; // leaves point upwards near the tip
                    for (int side = -1; side <= 1; side += 2) {
                        float la = ang + (float)side * (spread + ((k & 1) ? 0.2f : 0.0f));
                        add_node(n, x + fm_cos(la) * ll, y + fm_sin(la) * ll, K_STEM, sh + 25);
                    }
                }
                for (int i = 0; i < 3; i++) { // terminal tuft
                    float la = ang + (float)(i - 1) * 0.45f;
                    add_node(prev, x + fm_cos(la) * 2.5f, y + fm_sin(la) * 2.5f, K_STEM, 255);
                }
            }
            p->amp = 1.6f;
        } else { // Egeria densa: upright bottlebrush stems with dense whorls of narrow leaves
            int stems = rng_range(&r, 4, 5);
            for (int st = 0; st < stems; st++) {
                float x = rng_frange(&r, -6, 6), y = 0;
                float ang = -FM_PI / 2 + x * 0.025f + rng_frange(&r, -0.1f, 0.1f);
                float len = (float)h * rng_frange(&r, 0.6f, 1.0f);
                int prev = root;
                int k = 0;
                for (float d = 0; d < len; d += 3.0f, k++) {
                    ang += rng_frange(&r, -0.05f, 0.05f);
                    x += fm_cos(ang) * 3.0f;
                    y += fm_sin(ang) * 3.0f;
                    int sh = (int)(d * 230 / len);
                    int n = add_node(prev, x, y, K_STEM, sh);
                    if (n < 0) break;
                    prev = n;
                    if (d < 3.0f) continue;
                    float u = d / len;
                    float ll = u > 0.82f ? 2.5f + (1.0f - u) * 8.0f : 4.8f - u * 0.8f;
                    float spread = u > 0.82f ? 0.55f : 1.05f;
                    for (int side = -1; side <= 1; side += 2) {
                        float la = ang + (float)side * (spread + ((k & 1) ? 0.25f : 0.0f));
                        add_node(n, x + fm_cos(la) * ll, y + fm_sin(la) * ll, K_STEM, sh + 20);
                    }
                    float fa = ang + ((k & 1) ? 0.4f : -0.4f); // leaf of the whorl facing the viewer
                    add_node(n, x + fm_cos(fa) * 2.5f, y + fm_sin(fa) * 2.5f, K_STEM, sh + 50);
                }
            }
            p->amp = 2.0f;
        }
        break;
    case PLANT_VALLIS: { // dense clump of upright ribbon blades
        int blades = rng_range(&r, 9, 12);
        for (int b = 0; b < blades; b++) {
            float x = rng_frange(&r, -7, 7);
            float ang = -FM_PI / 2 + x * 0.012f + rng_frange(&r, -0.12f, 0.12f);
            ribbon(&r, root, x, 0, ang, (float)h * rng_frange(&r, 0.55f, 1.0f), rng_frange(&r, 0.7f, 1.1f),
                   rng_frange(&r, -0.008f, 0.008f), h);
        }
        p->amp = 4.0f;
        break;
    }
    case PLANT_CRYPT: { // Cryptocoryne wendtii: lanceolate wavy blades on long petioles
        int n = rng_range(&r, 7, 10);
        for (int i = 0; i < n; i++) {
            float a = -FM_PI / 2 + ((float)i / (float)(n - 1) - 0.5f) * 1.7f + rng_frange(&r, -0.12f, 0.12f);
            float x = rng_frange(&r, -2, 2), y = 0;
            int pt = grow(&r, root, &x, &y, a, (float)h * rng_frange(&r, 0.35f, 0.6f), 0, K_STEM, h);
            float out = a < -FM_PI / 2 ? -0.035f : 0.035f;
            leaf(&r, pt, x, y, a + out * 3.0f, (float)h * rng_frange(&r, 0.5f, 0.72f), 0.085f * (float)h + 0.6f, 0.35f,
                 out, 0.12f, 50, h);
        }
        p->amp = 1.4f;
        break;
    }
    case PLANT_MICROSORUM: { // Java fern: long lanceolate leaves from a creeping rhizome
        int n = rng_range(&r, 8, 11);
        float x = -8, y = 0;
        grow(&r, root, &x, &y, 0.0f, 16.0f, 0, K_STEM, h);
        for (int i = 0; i < n; i++) {
            float bx = -8.0f + 16.0f * (float)i / (float)(n - 1);
            float a = -FM_PI / 2 + rng_frange(&r, -0.6f, 0.6f) + bx * 0.03f;
            float curve = (a < -FM_PI / 2 ? -1.0f : 1.0f) * rng_frange(&r, 0.008f, 0.03f);
            leaf_s(&r, root, bx, 0, a, (float)h * rng_frange(&r, 0.55f, 1.0f), 0.055f * (float)h + 1.0f, 0.45f, curve,
                   0.04f, 40, h, 3.0f);
        }
        p->amp = 1.8f;
        break;
    }
    case PLANT_BOLBITIS: { // bushy fronds with alternate narrow, deeply lobed leaflets
        int fronds = rng_range(&r, 7, 9);
        for (int f = 0; f < fronds; f++) {
            float a = -FM_PI / 2 + ((float)f / (float)(fronds - 1) - 0.5f) * 2.0f + rng_frange(&r, -0.12f, 0.12f);
            float x = rng_frange(&r, -4, 4), y = 0;
            float len = (float)h * rng_frange(&r, 0.6f, 1.0f);
            float curve = (a < -FM_PI / 2 ? -1.0f : 1.0f) * rng_frange(&r, 0.02f, 0.04f);
            int prev = root;
            int k = 0;
            for (float d = 0; d < len; d += 3.0f, k++) {
                a += curve;
                x += fm_cos(a) * 3.0f;
                y += fm_sin(a) * 3.0f;
                int n = add_node(prev, x, y, K_STEM, (int)(d * 200 / len));
                if (n < 0) break;
                prev = n;
                if (d > len * 0.15f) {
                    float side = (k & 1) ? 1.0f : -1.0f;
                    float ll = (len - d) * 0.2f + 4.0f;
                    leaf(&r, n, x, y, a + side * 0.85f, ll, 1.4f, 0.4f, -side * 0.05f, 0.55f, 70, h);
                }
            }
            leaf(&r, prev, x, y, a, 8.0f, 1.3f, 0.4f, 0, 0.6f, 80, h); // terminal leaflet
        }
        p->amp = 1.6f;
        break;
    }
    case PLANT_ANUBIAS: { // broad, glossy, heart-shaped leaves held out on short petioles
        int n = rng_range(&r, 5, 8);
        for (int i = 0; i < n; i++) {
            float f = (float)i / (float)(n - 1) * 2.0f - 1.0f;
            float a = -FM_PI / 2 + f * 0.95f + rng_frange(&r, -0.15f, 0.15f);
            float x = rng_frange(&r, -4, 4), y = 0;
            int pt = grow(&r, root, &x, &y, a, (float)h * rng_frange(&r, 0.2f, 0.45f), 0, K_STEM, h);
            float out = f < 0 ? -1.0f : 1.0f;
            float len = (float)h * rng_frange(&r, 0.6f, 0.8f);
            leaf(&r, pt, x, y, a + out * rng_frange(&r, 0.05f, 0.3f), len, len * 0.38f, 0.33f, out * 0.03f, 0.0f, 40, h);
        }
        p->amp = 0.9f;
        break;
    }
    case PLANT_BUCE: { // Bucephalandra: compact clump of small wavy oval leaves on a rhizome
        int n = rng_range(&r, 12, 16);
        for (int i = 0; i < n; i++) {
            float a = -FM_PI / 2 + rng_frange(&r, -1.2f, 1.2f);
            float x = rng_frange(&r, -9, 9), y = 0;
            int pt = grow(&r, root, &x, &y, a, (float)h * rng_frange(&r, 0.2f, 0.8f), 0, K_STEM, h);
            float len = 0.5f * (float)h + rng_frange(&r, 3.5f, 5.0f);
            leaf(&r, pt, x, y, a + rng_frange(&r, -0.4f, 0.4f), len, len * 0.32f, 0.5f, 0, 0.25f, 60, h);
        }
        p->amp = 0.6f;
        break;
    }
    case PLANT_ECHINO: { // Amazon sword: rosette of long lanceolate leaves, slightly wavy
        int n = rng_range(&r, 10, 13);
        for (int i = 0; i < n; i++) {
            float a = -FM_PI / 2 + rng_frange(&r, -0.85f, 0.85f);
            float out = a < -FM_PI / 2 ? -0.025f : 0.025f;
            float x = 0, y = 0;
            int pt = grow(&r, root, &x, &y, a, (float)h * rng_frange(&r, 0.15f, 0.3f), out * 0.5f, K_STEM, h);
            leaf_s(&r, pt, x, y, a + out * 3.0f, (float)h * rng_frange(&r, 0.5f, 0.72f), 0.05f * (float)h + 0.8f, 0.35f,
                   out, 0.1f, 70, h, 3.0f);
        }
        p->amp = 2.2f;
        break;
    }
    case PLANT_MOSS: { // dense low cushion of fine, tangled branching strands
        float W = (float)h * 2.2f;
        for (int i = 0; i < 70; i++) {
            float ax = rng_frange(&r, -1, 1);
            float hm = (float)h * fm_sqrt(1.0f - ax * ax);
            float x = ax * W, y = -rng_frange(&r, 0, hm * 0.7f);
            int sh = (int)(-y * 200.0f / (float)h);
            int prev = add_node(root, x, y, K_LEAF, sh);
            float ang = -FM_PI / 2 + rng_frange(&r, -1.3f, 1.3f);
            int segs = rng_range(&r, 2, 3);
            for (int s = 0; s < segs && prev >= 0; s++) {
                ang += rng_frange(&r, -0.7f, 0.7f);
                float l = rng_frange(&r, 1.5f, 3.0f);
                x += fm_cos(ang) * l;
                y += fm_sin(ang) * l;
                if (y > 0) y = 0;
                prev = add_node(prev, x, y, K_STEM, sh + 25 * (s + 1) + rng_range(&r, 0, 30));
            }
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
    int pct = pc->scale ? pc->scale : 100;
    if (pct != 100) { // resize around the base: positions, leaf widths, sway
        float k = (float)pct / 100.0f;
        // water lily: only sideways, the pads must stay on the water line
        float ky = pc->type == PLANT_LILY ? 1.0f : k;
        for (int i = p->first; i < p->first + p->count; i++) {
            PNode *n = &s_nodes[i];
            float x = (float)n->rx * k, y = (float)n->ry * ky;
            n->rx = (int16_t)(x < 0 ? x - 0.5f : x + 0.5f);
            n->ry = (int16_t)(y < 0 ? y - 0.5f : y + 0.5f);
            if (n->kind == K_WIDE || n->kind == K_PAD) {
                int w = (int)((float)n->w * k + 0.5f);
                n->w = (uint8_t)(w < 1 ? 1 : (w > 255 ? 255 : w));
            }
        }
        p->height = (int16_t)((float)p->height * ky + 0.5f);
        p->amp *= k;
    }
    if (pc->rot) { // rotate the whole plant around its base (sway stays horizontal)
        float a = (float)pc->rot * (FM_PI / 180.0f), c = fm_cos(a), sn = fm_sin(a);
        for (int i = p->first; i < p->first + p->count; i++) {
            float x = s_nodes[i].rx, y = s_nodes[i].ry;
            float rx = c * x - sn * y, ry = sn * x + c * y;
            s_nodes[i].rx = (int16_t)(rx < 0 ? rx - 0.5f : rx + 0.5f);
            s_nodes[i].ry = (int16_t)(ry < 0 ? ry - 0.5f : ry + 0.5f);
        }
    }
    int x0 = -4, x1 = 4, y0 = -4, y1 = 0;
    for (int i = p->first; i < p->first + p->count; i++) {
        const PNode *n = &s_nodes[i];
        int w = n->kind == K_WIDE ? n->w / 4 + 1 : (n->kind == K_PAD ? n->w : 0);
        if (n->rx - w < x0) x0 = n->rx - w;
        if (n->rx + w > x1) x1 = n->rx + w;
        if (n->ry - w < y0) y0 = n->ry - w;
        if (n->ry + w > y1) y1 = n->ry + w;
    }
    p->x0 = (int16_t)x0;
    p->x1 = (int16_t)x1;
    p->y0 = (int16_t)y0;
    p->y1 = (int16_t)y1;
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

// Fast floor for the small coordinates used here (floorf is a libm call).
static inline int ifloor(float v) { return (int)(v + 4096.0f) - 4096; }

// Broad leaf segment between two nodes (half-widths in px), filled as one
// convex quad by scanlines: pixels on the upper side of the axis get the leaf
// colour, the lower side is shaded; wide blades get a light midrib along the axis. Rows that
// only graze a very thin quad still get one pixel so ribbons never break up.
static void leaf_segment(Canvas *cv, float x0, float y0, float w0, float x1, float y1, float w1, uint16_t col, uint16_t dark)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = fm_sqrt(dx * dx + dy * dy);
    if (len < 0.01f) return;
    if (w0 < 0.8f && w1 < 0.8f) { // hairline: a plain line is enough
        canvas_line(cv, ifloor(x0 + 0.5f), ifloor(y0 + 0.5f), ifloor(x1 + 0.5f), ifloor(y1 + 0.5f), col);
        return;
    }
    float il = 1.0f / len;
    float nx = -dy * il, ny = dx * il;
    if (ny > 0) { nx = -nx; ny = -ny; } // n points up: that side catches the light
    if (w0 <= 1.6f && w1 <= 1.6f) { // narrow blade: short spans across the major axis (cheap)
        float adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
        int n = (int)(adx > ady ? adx : ady) + 1;
        float inv = 1.0f / (float)n;
        bool vert = ady >= adx;
        for (int i = 0; i <= n; i++) {
            float t = (float)i * inv;
            float cx = x0 + dx * t, cy = y0 + dy * t, w = w0 + (w1 - w0) * t;
            if (vert) {
                int yi = ifloor(cy + 0.5f);
                int xs = ifloor(cx - w + 0.5f), xe = ifloor(cx + w + 0.5f);
                for (int x = xs; x <= xe; x++) canvas_pset(cv, x, yi, ((float)x - cx) * nx >= 0 ? col : dark);
            } else {
                int xi = ifloor(cx + 0.5f);
                int ys = ifloor(cy - w + 0.5f), ye = ifloor(cy + w + 0.5f);
                for (int y = ys; y <= ye; y++) canvas_pset(cv, xi, y, (float)y <= cy ? col : dark);
            }
        }
        return;
    }
    // extend half a pixel along the axis so consecutive segments overlap
    float ax = x0 - dx * il * 0.5f, ay = y0 - dy * il * 0.5f, bx = x1 + dx * il * 0.5f, by = y1 + dy * il * 0.5f;
    float qx[4] = {ax + nx * w0, bx + nx * w1, bx - nx * w1, ax - nx * w0};
    float qy[4] = {ay + ny * w0, by + ny * w1, by - ny * w1, ay - ny * w0};
    float ymin = qy[0], ymax = qy[0];
    float inv[4];
    for (int i = 0; i < 4; i++) {
        int j = (i + 1) & 3;
        float ey = qy[j] - qy[i];
        inv[i] = (ey > 1e-5f || ey < -1e-5f) ? (qx[j] - qx[i]) / ey : 0.0f;
        if (qy[i] < ymin) ymin = qy[i];
        if (qy[i] > ymax) ymax = qy[i];
    }
    const bool rib = w0 + w1 > 2.4f; // light midrib on wide blades
    const uint16_t mid = rib ? c565_lerp(col, RGB565(215, 240, 190), 70) : col;
    int yi0 = ifloor(ymin), yi1 = ifloor(ymax);
    if (yi0 < cv->cy0) yi0 = cv->cy0;
    if (yi1 >= cv->cy1) yi1 = cv->cy1 - 1;
    // signed distance from the axis along n: >= 0 on the lit side
    float sx = nx, sy = ny;
    for (int yi = yi0; yi <= yi1; yi++) {
        float yc = (float)yi + 0.5f, xl = 1e9f, xr = -1e9f;
        for (int i = 0; i < 4; i++) {
            int j = (i + 1) & 3;
            float ya = qy[i], yb = qy[j];
            if ((ya <= yc && yb > yc) || (yb <= yc && ya > yc)) {
                float x = qx[i] + (yc - ya) * inv[i];
                if (x < xl) xl = x;
                if (x > xr) xr = x;
            }
        }
        if (xl > xr) continue;
        int xs = ifloor(xl + 0.5f), xe = ifloor(xr - 0.5f);
        if (xe < xs) xs = xe = ifloor((xl + xr) * 0.5f);
        if (xs < cv->cx0) xs = cv->cx0;
        if (xe >= cv->cx1) xe = cv->cx1 - 1;
        float side = ((float)xs + 0.5f - ax) * sx + (yc - ay) * sy;
        uint16_t *row = cv->px + yi * cv->w;
        if (rib) {
            for (int x = xs; x <= xe; x++, side += sx) row[x] = side >= 0.5f ? col : (side > -0.5f ? mid : dark);
        } else {
            for (int x = xs; x <= xe; x++, side += sx) row[x] = side >= 0 ? col : dark;
        }
    }
}

void plants_draw(Canvas *cv, int layer, float t, int highlight)
{
    for (int pi = 0; pi < s_nplants; pi++) {
        const Plant *p = &s_plants[pi];
        if (p->layer != layer) continue;
        // Rotala-like red stems: green at the base, warm towards the tips
        const uint16_t dark = (p->type == PLANT_STEM && reddish(p->color)) ? c565_lerp(p->color, RGB565(70, 120, 55), 190)
                                                                           : c565_scale(p->color, 150);
        const uint16_t light = c565_lerp(p->color, RGB565(220, 255, 230), 90);
        const bool hl = pi == highlight;
        for (int i = p->first; i < p->first + p->count; i++) {
            const PNode *n = &s_nodes[i];
            float sx = (float)(p->bx + n->rx) + sway(p, n->ry, t);
            s_sx[i] = sx;
            int x = (int)sx;
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
                    float fx = sx, fy = (float)y;
                    float px = s_sx[n->parent], py = (float)(p->by + q->ry);
                    float w0 = q->kind == K_WIDE ? (float)q->w * 0.25f : 0.0f;
                    leaf_segment(cv, px, py, w0, fx, fy, (float)n->w * 0.25f, col, c565_lerp(col, dark, 150));
                }
                break;
            default:
                if (n->parent >= 0) {
                    const PNode *q = &s_nodes[n->parent];
                    canvas_line(cv, (int)s_sx[n->parent], p->by + q->ry, x, y, col);
                }
                break;
            }
        }
    }
}

void plants_bounds(int index, int *x0, int *y0, int *x1, int *y1)
{
    const Plant *p = &s_plants[index];
    int sw = (int)(p->amp + 0.99f); // room for the sway
    *x0 = p->bx + p->x0 - sw - 1;
    *x1 = p->bx + p->x1 + sw + 1;
    *y0 = p->by + p->y0 - 1;
    *y1 = p->by + p->y1 + 1;
}

int plants_hit(int x, int y)
{
    for (int i = s_nplants - 1; i >= 0; i--) {
        int x0, y0, x1, y1;
        plants_bounds(i, &x0, &y0, &x1, &y1);
        if (x >= x0 - 2 && x <= x1 + 2 && y >= y0 - 2 && y <= y1 + 2) return i;
    }
    return -1;
}
