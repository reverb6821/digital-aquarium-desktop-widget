// Creatures: data-driven species table (freshwater only: fish, shrimp, snails),
// procedural sprites and behaviours (SCHOOL boids with a spatial grid, WANDER,
// BOTTOM, SHRIMP, SNAIL).
// All sprites are generated in code: original pixel art.
#include "creatures.h"
#include <math.h>
#include <string.h>
#include "../engine/fixmath.h"
#include "../engine/rng.h"
#include "world.h"

#define C(r, g, b) RGB565(r, g, b)

// clang-format off
// Freshwater only. Shape: {len, h, tail_len, tail_h, tail, dorsal, pattern, body, anal, flags}
const SpeciesDef SPECIES[] = {
    // ---------------- Amazzonia / acque nere
    {"neon_tetra", "Neon", "amazon", BEH_SCHOOL, 13, 14, 30, 40, 150, 12, 30, 2,
     {10, 4, 3, 4, TAIL_FORK, 1, PAT_NEON, BODY_SLIM, 0, 0},
     {{0, C(48,58,70), C(90,110,125), C(150,170,185), C(200,210,215), C(40,190,255), C(235,40,55), C(150,170,185), C(10,10,14), 0},
      {0, C(48,70,60), C(90,125,110), C(150,185,170), C(200,215,210), C(80,255,170), C(255,120,40), C(150,185,170), C(10,10,14), 0}}},
    {"cardinal_tetra", "Cardinale", "blackwater", BEH_SCHOOL, 14, 14, 30, 40, 150, 14, 30, 1,
     {11, 4, 3, 4, TAIL_FORK, 1, PAT_CARDINAL, BODY_SLIM, 0, 0},
     {{0, C(50,58,72), C(95,110,128), C(150,165,180), C(200,205,212), C(30,200,255), C(230,30,45), C(160,170,185), C(10,10,14), 0}}},
    {"ember_tetra", "Pesce brace", "amazon", BEH_SCHOOL, 10, 12, 26, 30, 130, 7, 20, 2,
     {7, 4, 3, 4, TAIL_FORK, 1, PAT_NONE, BODY_SLIM, 0, 0},
     {{0, C(170,60,25), C(220,95,35), C(245,120,45), C(255,175,110), 0, 0, C(255,140,70), C(20,10,10), 0},
      {0, C(170,130,25), C(225,175,40), C(245,200,60), C(255,230,140), 0, 0, C(255,215,90), C(20,10,10), 0}}},
    {"angelfish", "Scalare", "amazon", BEH_WANDER, 29, 6, 14, 40, 170, 3, 6, 3,
     {21, 18, 8, 10, TAIL_FORK, 10, PAT_BANDS, BODY_DISC, 10, FIN_SWEPT},
     {{0, C(120,125,130), C(170,175,180), C(205,208,212), C(232,234,236), C(30,30,36), C(90,90,96), C(195,200,205), C(160,25,25), 0},
      {0, C(200,150,60), C(235,190,90), C(245,212,125), C(250,235,195), C(215,95,35), C(235,150,70), C(240,205,135), C(160,25,25), 0},
      {0, C(16,16,20), C(32,32,38), C(48,48,56), C(66,66,76), C(12,12,16), C(26,26,30), C(44,44,52), C(210,45,45), 0}}},
    {"discus", "Discus", "blackwater", BEH_WANDER, 30, 5, 12, 50, 180, 3, 6, 3,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_DISCUS, 0, 0},
     {{0, C(85,14,14), C(160,30,25), C(200,48,32), C(225,100,50), C(115,205,235), C(80,150,195), C(150,40,35), C(15,5,5), C(240,45,40)},   // turchese rosso
      {0, C(18,32,85), C(40,80,170), C(62,118,212), C(115,168,232), C(170,235,252), C(115,195,235), C(50,92,175), C(15,5,5), C(225,55,45)}, // blue diamond
      {0, C(140,55,20), C(215,115,40), C(240,150,60), C(250,200,130), C(175,40,30), C(205,75,45), C(210,110,50), C(15,5,5), C(230,40,30)}}},  // pigeon blood
    {"corydoras", "Corydoras", "amazon", BEH_BOTTOM, 13, 6, 16, 220, 255, 6, 12, 2,
     {10, 5, 3, 4, TAIL_FORK, 2, PAT_SPOTS, BODY_SLIM, 0, 0},
     {{0, C(110,95,70), C(165,145,105), C(195,175,135), C(225,215,190), 0, C(60,50,35), C(180,165,130), C(15,15,20), 0},
      {0, C(110,115,125), C(165,170,178), C(200,204,210), C(235,236,240), 0, C(30,30,36), C(180,185,190), C(15,15,20), 0}}},
    // ---------------- Sud-Est asiatico
    {"harlequin", "Rasbora arlecchino", "seasia", BEH_SCHOOL, 12, 12, 28, 40, 150, 10, 24, 1,
     {9, 5, 3, 4, TAIL_FORK, 1, PAT_WEDGE, BODY_SLIM, 0, 0},
     {{0, C(170,90,60), C(210,120,80), C(232,150,110), C(242,200,170), C(22,16,26), 0, C(230,120,80), C(10,10,14), 0}}},
    {"gourami", "Gourami", "seasia", BEH_WANDER, 18, 6, 14, 40, 160, 2, 5, 3,
     {13, 8, 4, 6, TAIL_ROUND, 3, PAT_SPOTS, BODY_SLIM, 4, FIN_FEELERS},
     {{0, C(100,90,85), C(150,140,130), C(185,175,165), C(230,130,70), 0, C(240,240,245), C(175,165,155), C(15,12,10), 0},
      {0, C(150,45,30), C(210,70,40), C(230,95,55), C(240,140,90), 0, C(90,150,235), C(200,90,60), C(15,12,10), 0},
      {0, C(35,70,140), C(60,110,190), C(85,140,210), C(140,180,230), 0, C(140,200,245), C(80,130,200), C(15,12,10), 0}}},
    {"kuhli_loach", "Pangio kuhlii", "seasia", BEH_BOTTOM, 16, 5, 12, 230, 255, 5, 12, 1,
     {15, 3, 2, 3, TAIL_ROUND, 0, PAT_BARS, BODY_SLIM, 0, 0},
     {{0, C(150,90,45), C(200,120,60), C(230,150,80), C(240,190,130), C(32,22,18), 0, C(220,140,70), C(10,10,10), 0}}},
    // ---------------- Africa occidentale (Congo)
    {"congo_tetra", "Tetra del Congo", "africa", BEH_SCHOOL, 16, 12, 26, 40, 160, 7, 16, 1,
     {11, 6, 5, 6, TAIL_FORK, 3, PAT_LATERAL, BODY_SLIM, 2, 0},
     {{0, C(70,90,140), C(110,140,190), C(170,180,195), C(230,165,95), C(240,205,95), 0, C(205,205,215), C(150,30,30), 0}}},
    {"kribensis", "Pelvicachromis", "africa", BEH_WANDER, 31, 6, 14, 150, 240, 2, 4, 1,
     {23, 10, 8, 9, TAIL_ROUND, 4, PAT_LATERAL, BODY_SLIM, 3, 0},
     {{0, C(110,90,60), C(170,150,110), C(200,185,150), C(220,80,140), C(40,30,25), 0, C(230,200,90), C(15,10,10), 0}}},
    {"shrimp", "Caridina", "seasia", BEH_SHRIMP, 12, 3, 8, 255, 255, 8, 30, 4,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_SHRIMP, 0, 0},
     {{0, C(120,20,20), C(200,40,35), C(235,90,80), 0, C(200,40,35), C(170,30,30), C(220,80,70), C(10,10,10), 0},    // red cherry
      {0, C(150,150,155), C(235,235,235), C(255,255,255), 0, C(210,30,40), C(235,235,235), C(230,220,220), C(10,10,10), 0}, // crystal red
      {0, C(80,90,85), C(150,160,150), C(190,200,190), 0, C(150,160,150), C(90,70,60), C(160,170,160), C(10,10,10), 0},  // amano
      {0, C(20,40,110), C(40,80,190), C(90,130,230), 0, C(40,80,190), C(30,60,160), C(60,100,200), C(10,10,10), 0}}},    // blue dream
    {"assassin_snail", "Lumaca killer", "seasia", BEH_SNAIL, 13, 0.8f, 2.0f, 255, 255, 2, 8, 1,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_CONE, 0, 0},
     {{0, C(30,20,15), C(75,48,26), C(120,85,50), C(170,160,130), C(232,192,64), 0, 0, C(15,15,15), 0}}},
    // ---------------- Lago Malawi
    {"mbuna", "Ciclide mbuna", "malawi", BEH_WANDER, 25, 8, 18, 60, 230, 6, 12, 3,
     {19, 9, 6, 7, TAIL_ROUND, 3, PAT_BARS, BODY_SLIM, 2, 0},
     {{0, C(200,165,20), C(240,205,40), C(250,220,60), C(252,235,130), C(240,205,40), 0, C(25,25,30), C(15,15,15), 0},
      {0, C(40,70,170), C(70,110,210), C(90,135,225), C(140,175,235), C(20,30,70), 0, C(60,90,190), C(15,15,15), 0},
      {0, C(200,90,30), C(235,125,45), C(245,145,60), C(250,190,120), C(215,105,40), 0, C(240,150,70), C(15,15,15), 0}}},
    {"aulonocara", "Ciclide pavone", "malawi", BEH_WANDER, 27, 6, 14, 40, 200, 3, 6, 2,
     {20, 9, 7, 8, TAIL_ROUND, 4, PAT_NONE, BODY_SLIM, 2, 0},
     {{0, C(35,60,160), C(60,100,215), C(80,125,230), C(150,180,240), 0, 0, C(240,185,50), C(15,15,15), 0},
      {0, C(170,50,30), C(215,75,40), C(235,100,55), C(90,130,220), 0, 0, C(80,120,210), C(15,15,15), 0}}},
    // ---------------- Laghetto
    {"koi", "Carpa koi", "pond", BEH_WANDER, 34, 6, 14, 40, 200, 3, 6, 3,
     {26, 10, 8, 8, TAIL_FORK, 2, PAT_PATCHES, BODY_SLIM, 1, 0},
     {{0, C(185,185,182), C(222,222,220), C(240,240,236), C(250,250,248), C(220,50,30), C(240,240,236), C(235,235,230), C(20,20,20), 0},
      {0, C(185,185,182), C(222,222,220), C(240,240,236), C(250,250,248), C(220,50,30), C(25,25,28), C(235,235,230), C(20,20,20), 0},
      {0, C(200,150,40), C(235,185,60), C(245,205,80), C(250,225,140), C(250,215,100), C(225,170,50), C(245,210,110), C(20,20,20), 0}}},
    {"goldfish", "Pesce rosso", "pond", BEH_WANDER, 21, 8, 18, 50, 170, 2, 6, 2,
     {14, 9, 7, 10, TAIL_FAN, 3, PAT_NONE, BODY_SLIM, 1, 0},
     {{0, C(200,100,10), C(240,140,20), C(250,170,40), C(255,210,110), 0, 0, C(250,180,70), C(25,15,10), 0},
      {0, C(200,50,35), C(230,75,50), C(240,120,90), C(250,235,225), 0, 0, C(245,215,200), C(25,15,10), 0}}},
    {"medaka", "Medaka", "pond", BEH_SCHOOL, 11, 12, 28, 20, 120, 8, 20, 3,
     {8, 3, 3, 3, TAIL_ROUND, 1, PAT_NONE, BODY_SLIM, 1, 0},
     {{0, C(210,110,40), C(240,150,60), C(248,175,90), C(252,215,160), 0, 0, C(245,170,90), C(10,10,14), 0},
      {0, C(200,200,195), C(228,228,222), C(238,238,232), C(248,248,244), 0, 0, C(235,235,228), C(10,10,14), 0},
      {0, C(110,135,165), C(150,175,200), C(180,200,220), C(220,230,240), 0, 0, C(170,190,215), C(10,10,14), 0}}},
    {"ramshorn", "Planorbarius", "pond", BEH_SNAIL, 14, 0.6f, 1.6f, 255, 255, 4, 12, 3,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_RAMSHORN, 0, 0},
     {{0, C(60,38,22), C(115,75,42), C(160,115,70), C(65,50,42), 0, 0, 0, C(10,10,10), 0},       // bruna
      {0, C(130,40,30), C(195,75,55), C(235,125,100), C(200,70,60), 0, 0, 0, C(10,10,10), 0},    // rossa
      {0, C(60,70,95), C(115,135,165), C(170,185,210), C(75,85,105), 0, 0, 0, C(10,10,10), 0}}}, // blu
};
// clang-format on
const int SPECIES_COUNT = (int)(sizeof(SPECIES) / sizeof(SPECIES[0]));

int species_find(const char *id)
{
    for (int i = 0; i < SPECIES_COUNT; i++)
        if (strcmp(SPECIES[i].id, id) == 0) return i;
    return -1;
}

// ------------------------------------------------------------ sprite generator

#define FISH_FRAMES 4
#define SPRITE_POOL 16384
static uint8_t s_sprite_mem[SPRITE_POOL];
static int s_sprite_used;
static SpriteSheet s_sprites[24];

static void fish_generate(SpriteSheet *ss, const FishShape *f)
{
    const int H0 = f->h > f->tail_h ? f->h : f->tail_h;
    const int w = f->len + f->tail_len;
    const int h = H0 + f->dorsal + f->anal + 2;
    const int stride = SPRITE_STRIDE(w);
    const int bytes = stride * h * FISH_FRAMES;
    if (s_sprite_used + bytes > SPRITE_POOL) {
        hal_log("creatures: sprite pool full");
        return;
    }
    uint8_t *out = s_sprite_mem + s_sprite_used;
    s_sprite_used += bytes;
    memset(out, 0, (size_t)bytes);
    ss->w = (uint8_t)w;
    ss->h = (uint8_t)h;
    ss->frames = FISH_FRAMES;
    ss->data = out;

    const float cy = (float)f->dorsal + (float)H0 * 0.5f; // body centre line
    static const float FLAP[FISH_FRAMES] = {0.0f, 1.0f, 0.0f, -1.0f};

    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        uint8_t *fb = out + fr * stride * h;
#define PUT(px, py, v)                                                                                   \
    do {                                                                                                 \
        int _x = (px), _y = (py);                                                                        \
        if (_x >= 0 && _x < w && _y >= 0 && _y < h) {                                                    \
            uint8_t *_b = &fb[_y * stride + (_x >> 1)];                                                  \
            if (_x & 1) *_b = (uint8_t)((*_b & 0xF0) | (v));                                             \
            else *_b = (uint8_t)((*_b & 0x0F) | ((v) << 4));                                             \
        }                                                                                                \
    } while (0)
        // tail
        for (int tx = 0; tx < f->tail_len; tx++) {
            float d = (float)(f->tail_len - tx) / (float)f->tail_len; // 0 at joint .. 1 at tip
            float th = 0.8f + ((float)f->tail_h * 0.5f - 0.8f) * d;
            if (f->tail == TAIL_ROUND) th = (float)f->tail_h * 0.5f * (0.55f + 0.45f * fm_sin(d * FM_PI * 0.9f));
            float off = FLAP[fr] * d * 1.2f;
            for (int y = 0; y < h; y++) {
                float dy = (float)y + 0.5f - (cy + off);
                if (fm_abs(dy) > th) continue;
                if (f->tail == TAIL_FORK && d > 0.55f && fm_abs(dy) < th * 0.45f) continue;
                uint8_t v = 7;
                if (f->tail == TAIL_FAN && ((tx + y) & 1) && d > 0.4f) v = 3; // flowing fin streaks
                PUT(tx, y, v);
            }
        }
        // body
        for (int x = 0; x < f->len; x++) {
            float u = ((float)x + 0.5f) / (float)f->len;
            float s;
            if (f->body == BODY_DISC) {
                float k = 2.0f * u - 1.0f;
                s = fm_sqrt(1.0f - k * k > 0 ? 1.0f - k * k : 0);
                if (s < 0.35f && u < 0.5f) s = 0.35f; // caudal peduncle
            } else {
                s = u < 0.6f ? 0.5f + 0.5f * fm_sin(u / 0.6f * FM_PI * 0.5f) : fm_cos((u - 0.6f) / 0.4f * FM_PI * 0.5f);
                if (s < 0) s = 0;
                if (u >= 0.6f) s = fm_sqrt(s);
            }
            float hb = (float)f->h * 0.5f * s;
            if (hb < 0.5f) hb = 0.5f;
            int px = f->tail_len + x;
            for (int y = 0; y < h; y++) {
                float dy = (float)y + 0.5f - cy;
                if (fm_abs(dy) > hb) {
                    // dorsal / anal fins: swept (tall, peaking at the rear) or a small triangle
                    float fin = -1.0f;
                    if (f->flags & FIN_SWEPT) {
                        // u = 0 at the tail joint, 1 at the snout: peak towards the rear
                        if (u > 0.2f && u < 0.86f) fin = u > 0.36f ? (0.86f - u) / 0.5f : (u - 0.2f) / 0.16f;
                    } else if (u > 0.3f && u < 0.65f) {
                        fin = 1.0f - fm_abs(u - 0.45f) / 0.2f;
                    }
                    if (dy < 0 && f->dorsal && fin >= 0 && -dy - hb <= fin * (float)f->dorsal + 0.3f) PUT(px, y, 7);
                    if (dy > 0) {
                        if (f->flags & FIN_SWEPT) {
                            if (fin >= 0 && dy - hb <= fin * (float)f->anal + 0.3f) PUT(px, y, 7);
                        } else if (u > 0.22f && u < 0.42f + 0.04f * (float)f->anal &&
                                   dy - hb <= 1.2f + 0.6f * (float)f->anal) {
                            PUT(px, y, 7);
                        }
                    }
                    continue;
                }
                float tt = (dy + hb) / (2.0f * hb); // 0 top .. 1 bottom
                uint8_t v = tt < 0.18f ? 1 : (tt < 0.45f ? 2 : (tt < 0.8f ? 3 : 4));
                switch (f->pattern) {
                case PAT_NEON:
                    if (tt >= 0.28f && tt < 0.5f && u > 0.12f && u < 0.9f) v = 5;
                    else if (tt >= 0.5f && tt < 0.82f && u < 0.55f) v = 6;
                    break;
                case PAT_CARDINAL:
                    if (tt >= 0.25f && tt < 0.47f && u > 0.1f && u < 0.9f) v = 5;
                    else if (tt >= 0.47f && tt < 0.9f && u < 0.86f) v = 6;
                    break;
                case PAT_WAVY: {
                    float wv = tt * 7.0f + 0.7f * fm_sin(u * 14.0f);
                    if (tt > 0.08f && tt < 0.92f && u > 0.12f && u < 0.85f && ((int)wv & 1)) v = 5;
                    break;
                }
                case PAT_BARS:
                    if (tt > 0.1f && tt < 0.85f && u > 0.08f && u < 0.85f && ((x >> 1) & 1)) v = 5;
                    break;
                case PAT_WEDGE:
                    if (u > 0.22f && u < 0.6f && fm_abs(tt - 0.55f) < (u - 0.22f) / 0.38f * 0.32f) v = 5;
                    break;
                case PAT_BANDS: {
                    static const float B[3] = {0.18f, 0.5f, 0.78f};
                    for (int b = 0; b < 3; b++) {
                        float du = (u - B[b]) * (float)f->len;
                        if (fm_abs(du) < 0.75f) v = 5;
                        else if (fm_abs(du) < 1.3f) v = 6;
                    }
                    break;
                }
                case PAT_SPOTS:
                    if (tt < 0.55f && ((x * 7 + y * 3) % 5) == 0) v = 6;
                    break;
                case PAT_LATERAL:
                    if (tt >= 0.4f && tt < 0.55f && u > 0.1f && u < 0.85f) v = 5;
                    break;
                case PAT_PATCHES: // koi: 2-3 big blotches on the back (5) + small dark spots (6)
                    if (u > 0.12f && tt < 0.62f + 0.1f * fm_sin(u * 11.0f)) {
                        float blob = fm_sin(u * 10.0f + 0.8f) + 0.5f * fm_sin(u * 23.0f + (float)y);
                        if (blob > 0.15f) v = 5;
                        if (tt > 0.15f && ((x * 7 + y * 5) % 11) == 0 && u > 0.3f) v = 6;
                    }
                    break;
                default: break;
                }
                PUT(px, y, v);
            }
        }
        // gourami ventral feelers: long thin threads trailing down and back
        if (f->flags & FIN_FEELERS) {
            int fx = f->tail_len + (int)((float)f->len * 0.62f);
            float fy = cy + (float)f->h * 0.35f;
            float sw = FLAP[fr] * 0.4f;
            for (int k = 0; k < (int)((float)f->len * 0.75f); k++)
                PUT(fx - k, (int)(fy + 1.0f + (float)k * (0.3f + sw * 0.05f)), 7);
        }
        // eye (1 px)
        {
            int ex = f->tail_len + (int)((float)f->len * 0.8f);
            float u = ((float)(ex - f->tail_len) + 0.5f) / (float)f->len;
            float s = fm_cos((u - 0.6f) / 0.4f * FM_PI * 0.5f);
            float hb = (float)f->h * 0.5f * fm_sqrt(s > 0 ? s : 0);
            int ey = (int)(cy - hb * 0.3f);
            if (f->h <= 4) ey = (int)(cy - 0.5f);
            PUT(ex, ey, 8);
        }
#undef PUT
    }
}

// ---- invertebrate sprites (4 frames each, feet on the last row)

static uint8_t *sheet_alloc(SpriteSheet *ss, int w, int h)
{
    const int bytes = SPRITE_STRIDE(w) * h * FISH_FRAMES;
    if (s_sprite_used + bytes > SPRITE_POOL) {
        hal_log("creatures: sprite pool full");
        return NULL;
    }
    uint8_t *out = s_sprite_mem + s_sprite_used;
    s_sprite_used += bytes;
    memset(out, 0, (size_t)bytes);
    ss->w = (uint8_t)w;
    ss->h = (uint8_t)h;
    ss->frames = FISH_FRAMES;
    ss->data = out;
    return out;
}

static void sheet_put(const SpriteSheet *ss, int fr, int x, int y, uint8_t v)
{
    if (x < 0 || x >= ss->w || y < 0 || y >= ss->h) return;
    uint8_t *b = (uint8_t *)ss->data + (fr * ss->h + y) * SPRITE_STRIDE(ss->w) + (x >> 1);
    if (x & 1) *b = (uint8_t)((*b & 0xF0) | v);
    else *b = (uint8_t)((*b & 0x0F) | (v << 4));
}

// Shrimp in side view, facing right: arched segmented body, tail fan, rostrum,
// antennae, walking legs that alternate between frames.
static void shrimp_generate(SpriteSheet *ss)
{
    const int W = 16, H = 9;
    if (!sheet_alloc(ss, W, H)) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        float cyh = 0;
        for (int x = 2; x <= 12; x++) {
            float u = (float)(x - 2) / 10.0f;
            float cy = 5.2f - 1.6f * fm_sin(u * FM_PI);
            float hb = u < 0.6f ? 0.9f + 1.0f * u / 0.6f : 1.9f - 0.6f * (u - 0.6f) / 0.4f;
            if (x == 12) cyh = cy;
            for (int y = 0; y < H; y++) {
                float dy = (float)y + 0.5f - cy;
                if (fm_abs(dy) > hb) continue;
                uint8_t v = dy < -hb * 0.35f ? 3 : 2;
                if (dy > hb - 0.8f) v = 1;
                if (x == 3 || x == 4 || x == 7 || x == 8 || x == 11) v = 5;    // bands (crystal red)
                else if (dy > 0 && dy <= hb - 0.8f && (x & 1) && x < 10) v = 6; // side dots (amano)
                sheet_put(ss, fr, x, y, v);
            }
            // pleopods under the abdomen, walking legs under the carapace
            int base = (int)(cy + hb);
            if (x == 8 || x == 10 || x == 12) { // short walking legs, alternating stride
                int k = (x / 2 + fr) & 1;
                for (int y = base + 1; y < H; y++) sheet_put(ss, fr, x + (y == H - 1 ? (k ? 1 : 0) : 0), y, 7);
            }
        }
        // tail fan
        for (int y = 4; y <= 6; y++) sheet_put(ss, fr, 1, y, 3);
        for (int y = 3; y <= 7; y += 2) sheet_put(ss, fr, 0, y, 3);
        sheet_put(ss, fr, 0, 5, 1);
        // rostrum, eye, antennae
        int hy = (int)cyh;
        sheet_put(ss, fr, 13, hy - 1, 2);
        sheet_put(ss, fr, 14, hy - 1, 1);
        sheet_put(ss, fr, 12, hy - 1, 8);
        int wave = (fr == 1) ? -1 : (fr == 3 ? 1 : 0);
        sheet_put(ss, fr, 13, hy - 2, 7);
        sheet_put(ss, fr, 14, hy - 3 + wave, 7);
        sheet_put(ss, fr, 15, hy - 4 + wave, 7);
    }
}

// Snails, facing right. Frames 0-2 crawling (foot ripple, tentacles), frame 3
// retracted into the shell.
static void snail_generate(SpriteSheet *ss, int body)
{
    const bool cone = body == BODY_CONE;
    const int W = cone ? 14 : 15, H = 12;
    if (!sheet_alloc(ss, W, H)) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const bool out = fr < 3;
        // foot + head
        if (out) {
            int front = 12 + (fr == 1 ? 1 : 0);
            for (int x = 2; x <= front; x++) {
                sheet_put(ss, fr, x, H - 2, 4);
                sheet_put(ss, fr, x, H - 1, ((x + fr) % 3) ? 4 : 1); // ripple of the sole
            }
            int tip = fr == 2 ? 1 : 0;
            if (cone) { // siphon tube raised forward + short tentacles
                sheet_put(ss, fr, front - 1, H - 3, 4);
                sheet_put(ss, fr, front, H - 4, 4);
                sheet_put(ss, fr, front + 1, H - 5 - tip, 4);
                sheet_put(ss, fr, front, H - 3, 4);
                sheet_put(ss, fr, front - 1, H - 4, 8);
            } else { // long thin tentacles with eyes at their base
                sheet_put(ss, fr, front - 1, H - 3, 4);
                sheet_put(ss, fr, front - 2, H - 3, 8);
                for (int k = 1; k <= 3; k++) sheet_put(ss, fr, front - 1 + (k + 1) / 2, H - 3 - k - (k == 3 ? tip : 0), 4);
                for (int k = 1; k <= 2; k++) sheet_put(ss, fr, front - 2 + k / 2, H - 3 - k, 4);
            }
        } else {
            for (int x = 5; x <= 8; x++) sheet_put(ss, fr, x, H - 1, 1);
        }
        // shell
        for (int y = 0; y < H - 1; y++)
            for (int x = 0; x < W; x++) {
                float px = (float)x + 0.5f, py = (float)y + 0.5f;
                uint8_t v = 0;
                if (!cone) { // Planorbarius: flat planispiral disc seen from the side
                    float dx = px - 6.5f, dy = py - 5.6f;
                    float r = fm_sqrt(dx * dx + dy * dy);
                    if (r > 4.9f) continue;
                    float a = atan2f(dy, dx);
                    float sp = r / 1.55f + a / FM_TAU;
                    v = (sp - (float)(int)sp) < 0.3f ? 1 : 2;
                    if (dx < -0.5f && dy < -0.5f && r > 2.4f && r < 4.2f && v == 2) v = 3;
                    if (r > 4.25f || r < 0.8f) v = 1;
                } else { // Clea helena: tall cone, yellow spiral band on dark brown
                    const float bx = 7.0f, by = 8.8f, ax = 1.6f, ay = 0.4f;
                    float lx = ax - bx, ly = ay - by, L = fm_sqrt(lx * lx + ly * ly);
                    float ux = lx / L, uy = ly / L;
                    float vx = px - bx, vy = py - by;
                    float t = (vx * ux + vy * uy) / L;
                    float d = fm_abs(vx * uy - vy * ux);
                    float hw;
                    if (t >= 0 && t <= 1) hw = 3.3f * powf(1.0f - t, 0.85f) + 0.4f;
                    else if (t < 0 && t > -0.22f) hw = 3.7f * fm_sqrt(1.0f - (t / 0.22f) * (t / 0.22f));
                    else continue;
                    if (d > hw) continue;
                    float band = t * L / 1.5f + d * 0.2f; // whorls, slightly slanted
                    v = (band - (float)(int)band) < 0.4f ? 5 : 2;
                    if (d > hw - 0.7f) v = 1;
                    else if (v == 2 && vx * uy - vy * ux < -hw * 0.4f) v = 3; // lit side
                }
                sheet_put(ss, fr, x, y, v);
            }
    }
}

// Discus, facing right: round body ringed by dorsal/anal fins that widen towards
// the rear (red edge, undulating between frames), wavy turquoise lines (dotted
// on the belly), striped face, ringed red eye, short fan tail.
static void discus_generate(SpriteSheet *ss)
{
    const float K = 1.3f; // overall scale (1.0 = 24x22 sprite)
    const int TL = 6;     // tail length
    const float rx = 7.6f * K, ry = 7.4f * K;
    const float cx = (float)TL + rx + 1.5f;
    const int W = (int)(cx + rx + 2.0f), H = (int)(2.0f * ry * 1.48f + 2.0f);
    const float cy = (float)H * 0.5f;
    if (!sheet_alloc(ss, W, H)) return;
    static const float FLAP[FISH_FRAMES] = {0.0f, 1.0f, 0.0f, -1.0f};
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        // tail (behind the fins)
        for (int tx = 0; tx < TL; tx++) {
            float d = ((float)TL - (float)tx) / (float)TL; // 1 at the tip
            float hh = (1.2f + 3.4f * d) * K, off = FLAP[fr] * d * 1.3f * K;
            for (int y = 0; y < H; y++) {
                float dy = (float)y + 0.5f - (cy + off);
                if (fm_abs(dy) > hh) continue;
                uint8_t v = (y & 1) ? 6 : 7;
                if (fm_abs(dy) > hh - 0.8f && tx < 2) v = 9;
                sheet_put(ss, fr, tx, y, v);
            }
        }
        for (int y = 0; y < H; y++)
            for (int x = 3; x < W; x++) {
                float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy;
                const float xs = (float)x / K, ys = (float)y / K; // pattern space (same density at any scale)
                float e = fm_sqrt((dx * dx) / (rx * rx) + (dy * dy) / (ry * ry));
                if (e > 1.0f) { // fin ring
                    float a = fm_abs(atan2f(dy, dx)); // 0 = snout, pi = tail
                    float a0 = dy < 0 ? 0.75f : 1.1f; // dorsal starts higher on the head
                    if (a < a0 || a > 3.0f) continue;
                    float p = a < 2.55f ? (a - a0) / (2.55f - a0) : (3.0f - a) / 0.45f; // 0..1, widest at the rear
                    float ramp = fm_clamp((a - a0) / 0.35f, 0.0f, 1.0f);              // soft start on the head
                    float t = ramp * (0.16f + 0.3f * p) + 0.05f * fm_sin(a * 7.0f + (float)fr * FM_PI * 0.5f);
                    if (e > 1.0f + t) continue;
                    uint8_t v = 7;
                    float st = e * 6.0f + a * 3.0f;                       // streaks parallel to the edge
                    if (st - (float)(int)st < 0.3f) v = a > 1.9f ? 6 : 5; // turquoise striations
                    if (e > 1.0f + t - 0.12f) v = 9;             // red edge
                    sheet_put(ss, fr, x, y, v);
                    continue;
                }
                uint8_t v = 3;
                if (dy < 0 && e > 0.72f) v = 2;                     // darker back rim
                if (dy * 0.8f + dx * 0.45f > ry * 0.4f) v = 4;      // warm belly / throat
                if (dx > rx * 0.5f) { // face: curved horizontal stripes
                    if (fm_abs(fm_sin(ys * 0.9f + 1.3f * fm_sin(xs * 0.8f))) < 0.4f) v = 5;
                } else { // wavy vertical lines, solid above, dotted on the belly
                    if (fm_abs(fm_sin(xs * 1.05f + 1.6f * fm_sin(ys * 0.55f))) < 0.5f)
                        v = dy < ry * 0.3f ? 5 : (((x + y) & 1) ? 6 : v);
                }
                if (e > 1.0f - 0.45f / rx) v = 1; // crisp 1 px outline
                sheet_put(ss, fr, x, y, v);
            }
        // eye: dark pupil ringed red
        const int ex = (int)(cx + rx * 0.52f), ey = (int)(cy - 1.6f * K);
        sheet_put(ss, fr, ex, ey, 8);
        sheet_put(ss, fr, ex + 1, ey - 1, 4); // catch-light
        sheet_put(ss, fr, ex - 1, ey, 9);
        sheet_put(ss, fr, ex + 1, ey, 9);
        sheet_put(ss, fr, ex, ey - 1, 9);
        sheet_put(ss, fr, ex, ey + 1, 9);
        // mouth
        sheet_put(ss, fr, (int)(cx + rx), (int)cy + 1, 1);
        // pectoral fin, beating
        sheet_put(ss, fr, ex - 2, ey + 3, 7);
        sheet_put(ss, fr, ex - 3, ey + 3 + (fr & 1), 7);
        sheet_put(ss, fr, ex - 4, ey + 4 - (fr & 1), 7);
    }
}

static int s_sprite_of[64];

const SpriteSheet *species_sprite(int sp)
{
    if (sp < 0 || sp >= SPECIES_COUNT || s_sprite_of[sp] < 0) return NULL;
    return &s_sprites[s_sprite_of[sp]];
}

// ------------------------------------------------------------ creature state

#define MAX_CREATURES 120

typedef struct {
    uint8_t sp, variant, frame;
    int8_t face;
    float x, y, vx, vy;
    float z;          // depth 0 (near) .. 1 (far)
    float anim;       // animation accumulator
    float tx, ty;     // wander target
    float timer;      // state timer
    float flee;       // remaining scare time
    float speed;      // cruising speed
    uint8_t state;    // behaviour sub-state
    uint32_t seed;
} Creature;

static Creature *s_cr; // allocated once (PSRAM on device)
static int s_n;
static uint8_t s_order[MAX_CREATURES];
static Rng s_rng;

// School travel targets per species
static float s_school_tx[64], s_school_ty[64], s_school_timer[64];

// Spatial grid for boids
#define CELL 24
#define GW ((CANVAS_W + CELL - 1) / CELL)
#define GH ((CANVAS_H + CELL - 1) / CELL)
static int16_t s_cell_head[GW * GH];
static int16_t s_cell_next[MAX_CREATURES];

void creatures_init(void)
{
    if (!s_cr) s_cr = (Creature *)hal_alloc_large(sizeof(Creature) * MAX_CREATURES);
    if (s_sprite_used == 0) {
        int ns = 0;
        for (int i = 0; i < SPECIES_COUNT && i < 64; i++) {
            s_sprite_of[i] = -1;
            if (ns < (int)(sizeof(s_sprites) / sizeof(s_sprites[0]))) {
                const FishShape *f = &SPECIES[i].shape;
                if (f->body == BODY_SHRIMP) shrimp_generate(&s_sprites[ns]);
                else if (f->body == BODY_DISCUS) discus_generate(&s_sprites[ns]);
                else if (f->body == BODY_RAMSHORN || f->body == BODY_CONE) snail_generate(&s_sprites[ns], f->body);
                else fish_generate(&s_sprites[ns], f);
                s_sprite_of[i] = ns++;
            }
        }
    }
    s_n = 0;
}

static float band_y(int sp, float f)
{
    // map depth (0..255) to canvas y between surface and the substrate
    float top = (float)SURFACE_Y + 10.0f;
    float bottom = (float)world_floor(CANVAS_W / 2) - 10.0f;
    float d = (float)SPECIES[sp].depth_min + ((float)SPECIES[sp].depth_max - (float)SPECIES[sp].depth_min) * f;
    return top + (bottom - top) * d / 255.0f;
}

static void pick_target(Creature *c)
{
    c->tx = rng_frange(&s_rng, 16, CANVAS_W - 16);
    c->ty = band_y(c->sp, rng_f01(&s_rng));
    float fl = (float)world_floor((int)c->tx) - 8.0f;
    if (c->ty > fl) c->ty = fl;
}

static void spawn_one(int sp, int variant)
{
    if (s_n >= MAX_CREATURES || !s_cr) return;
    const SpeciesDef *d = &SPECIES[sp];
    Creature *c = &s_cr[s_n++];
    memset(c, 0, sizeof(*c));
    c->sp = (uint8_t)sp;
    c->variant = (uint8_t)(variant % d->variants);
    c->seed = rng_next(&s_rng);
    c->x = rng_frange(&s_rng, 20, CANVAS_W - 20);
    c->y = band_y(sp, rng_f01(&s_rng));
    c->speed = rng_frange(&s_rng, d->speed_min, d->speed_max);
    float a = rng_frange(&s_rng, 0, FM_TAU);
    c->vx = fm_cos(a) * c->speed;
    c->vy = fm_sin(a) * c->speed * 0.3f;
    c->face = c->vx >= 0 ? 1 : -1;
    c->z = rng_f01(&s_rng);
    c->anim = rng_frange(&s_rng, 0, 4);
    c->timer = rng_frange(&s_rng, 1, 6);
    pick_target(c);
    if (d->behavior == BEH_BOTTOM || d->behavior == BEH_SHRIMP || d->behavior == BEH_SNAIL) {
        c->y = (float)world_floor((int)c->x) - 3.0f;
        c->z = rng_frange(&s_rng, 0, 0.4f);
        c->tx = c->ty = 0; // shrimp: hop velocity / height offset
    }
}

void creatures_spawn(const AquariumConfig *cfg, uint32_t seed)
{
    rng_seed(&s_rng, seed);
    s_n = 0;
    int limit = cfg->creature_limit < MAX_CREATURES ? cfg->creature_limit : MAX_CREATURES;
    for (int i = 0; i < cfg->species_count; i++) {
        int sp = species_find(cfg->species[i].id);
        if (sp < 0) continue;
        int n = cfg->species[i].count;
        if (n > SPECIES[sp].max_count) n = SPECIES[sp].max_count;
        for (int k = 0; k < n && s_n < limit; k++) spawn_one(sp, cfg->species[i].variant);
    }
    for (int i = 0; i < SPECIES_COUNT && i < 64; i++) {
        s_school_tx[i] = rng_frange(&s_rng, 60, CANVAS_W - 60);
        s_school_ty[i] = band_y(i, 0.5f);
        s_school_timer[i] = 0;
    }
}

int creatures_count(void) { return s_n; }

// ------------------------------------------------------------ behaviours

static void limit_speed(Creature *c, float lo, float hi)
{
    float sp = fm_sqrt(c->vx * c->vx + c->vy * c->vy);
    if (sp > hi && sp > 0) {
        c->vx *= hi / sp;
        c->vy *= hi / sp;
    } else if (sp < lo) {
        if (sp < 0.01f) {
            c->vx = lo * (float)c->face;
            c->vy = 0;
        } else {
            c->vx *= lo / sp;
            c->vy *= lo / sp;
        }
    }
}

// Soft forces keeping creatures inside the tank; returns the floor y used.
static float bounds_force(const Creature *c, float *ax, float *ay, float margin_bottom)
{
    const float m = 14.0f;
    float fl = (float)world_floor((int)c->x) - margin_bottom;
    if (c->x < m) *ax += (m - c->x) * 3.0f;
    if (c->x > CANVAS_W - m) *ax -= (c->x - (CANVAS_W - m)) * 3.0f;
    if (c->y < SURFACE_Y + 10) *ay += ((float)SURFACE_Y + 10 - c->y) * 3.0f;
    if (c->y > fl) *ay -= (c->y - fl) * 4.0f;
    return fl;
}

static void hard_clamp(Creature *c, float floor_margin)
{
    if (c->x < 2) { c->x = 2; if (c->vx < 0) c->vx = -c->vx; }
    if (c->x > CANVAS_W - 3) { c->x = CANVAS_W - 3; if (c->vx > 0) c->vx = -c->vx; }
    if (c->y < SURFACE_Y + 4) { c->y = SURFACE_Y + 4; if (c->vy < 0) c->vy = 0; }
    float fl = (float)world_floor((int)c->x) - floor_margin;
    if (c->y > fl) { c->y = fl; if (c->vy > 0) c->vy = 0; }
}

static void seek_food(Creature *c, float *ax, float *ay, bool resting_only)
{
    int f = food_nearest(c->x, c->y, 90.0f);
    if (f < 0 || (resting_only && !g_food[f].resting)) return;
    float dx = g_food[f].x - c->x, dy = g_food[f].y - c->y;
    float d = fm_sqrt(dx * dx + dy * dy);
    if (d < 3.0f) {
        g_food[f].alive = 0; // eaten
        return;
    }
    *ax += dx / d * 60.0f;
    *ay += dy / d * 60.0f;
}

static void update_school(Creature *c, int idx, float dt)
{
    const SpeciesDef *d = &SPECIES[c->sp];
    float ax = 0, ay = 0;
    float cx = 0, cy = 0, avx = 0, avy = 0, sx = 0, sy = 0;
    int n = 0;
    int gx = (int)c->x / CELL, gy = (int)c->y / CELL;
    for (int yy = gy - 1; yy <= gy + 1; yy++) {
        if (yy < 0 || yy >= GH) continue;
        for (int xx = gx - 1; xx <= gx + 1; xx++) {
            if (xx < 0 || xx >= GW) continue;
            for (int j = s_cell_head[yy * GW + xx]; j >= 0; j = s_cell_next[j]) {
                if (j == idx || s_cr[j].sp != c->sp) continue;
                float dx = s_cr[j].x - c->x, dy = s_cr[j].y - c->y;
                float d2 = dx * dx + dy * dy;
                if (d2 > 24.0f * 24.0f) continue;
                cx += s_cr[j].x;
                cy += s_cr[j].y;
                avx += s_cr[j].vx;
                avy += s_cr[j].vy;
                n++;
                // separation in "fish space": bodies are long, so horizontal
                // neighbours need more room than vertical ones
                float ex = dx * 0.7f, ey = dy * 1.6f, e2 = ex * ex + ey * ey;
                if (e2 < 12.0f * 12.0f && e2 > 0.01f) {
                    sx -= ex / e2;
                    sy -= ey / e2 * 0.6f;
                }
            }
        }
    }
    if (n > 0) {
        float inv = 1.0f / (float)n;
        ax += (cx * inv - c->x) * 0.6f;   // cohesion
        ay += (cy * inv - c->y) * 1.0f;
        ax += (avx * inv - c->vx) * 1.2f; // alignment
        ay += (avy * inv - c->vy) * 1.2f;
    }
    ax += sx * 320.0f; // separation
    ay += sy * 320.0f;
    // species travel goal keeps the school exploring the tank
    ax += (s_school_tx[c->sp] - c->x) * 0.08f;
    ay += (s_school_ty[c->sp] - c->y) * 0.05f;
    // a little individual noise
    ax += fm_sin(c->anim * 0.7f + (float)(c->seed & 63)) * 6.0f;
    ay += fm_cos(c->anim * 0.5f + (float)(c->seed & 31)) * 3.0f;
    seek_food(c, &ax, &ay, false);
    bounds_force(c, &ax, &ay, 8.0f);
    c->vx += ax * dt;
    c->vy += ay * dt;
    c->vy *= 1.0f - 0.8f * dt; // fish prefer horizontal motion
    float hi = d->speed_max * (c->flee > 0 ? 2.6f : 1.0f);
    limit_speed(c, d->speed_min * 0.6f, hi);
}

static void update_wander(Creature *c, float dt)
{
    const SpeciesDef *d = &SPECIES[c->sp];
    float ax = 0, ay = 0;
    c->timer -= dt;
    if (c->timer <= 0) {
        if (c->state == 0 && rng_range(&s_rng, 0, 3) == 0) {
            c->state = 1; // pause
            c->timer = rng_frange(&s_rng, 1.0f, 2.5f);
        } else {
            c->state = 0;
            pick_target(c);
            c->timer = rng_frange(&s_rng, 4.0f, 10.0f);
        }
    }
    float dx = c->tx - c->x, dy = c->ty - c->y;
    float dist = fm_sqrt(dx * dx + dy * dy);
    if (dist < 6.0f) pick_target(c);
    float want = c->state == 1 ? d->speed_min * 0.15f : c->speed;
    if (dist > 0.1f) {
        ax += (dx / dist * want - c->vx) * 1.4f;
        ay += (dy / dist * want * 0.6f - c->vy) * 1.4f;
    }
    seek_food(c, &ax, &ay, false);
    bounds_force(c, &ax, &ay, 10.0f);
    c->vx += ax * dt;
    c->vy += ay * dt;
    float hi = d->speed_max * (c->flee > 0 ? 2.8f : 1.0f);
    limit_speed(c, 0.0f, hi);
}

static void update_bottom(Creature *c, float dt)
{
    const SpriteSheet *ss = species_sprite(c->sp);
    c->timer -= dt;
    if (c->timer <= 0) {
        if (c->state == 0) {
            c->state = 1; // stop and "eat"
            c->timer = rng_frange(&s_rng, 1.0f, 4.0f);
        } else {
            c->state = 0;
            c->timer = rng_frange(&s_rng, 0.6f, 2.5f);
            if (rng_range(&s_rng, 0, 2) == 0) c->face = (int8_t)-c->face;
        }
    }
    int f = food_nearest(c->x, c->y, 80.0f);
    if (f >= 0 && g_food[f].resting) {
        c->state = 0;
        c->face = g_food[f].x > c->x ? 1 : -1;
        if (fm_abs(g_food[f].x - c->x) < 3) g_food[f].alive = 0;
    }
    float target = c->state == 0 ? (float)c->face * c->speed * (c->flee > 0 ? 3.0f : 1.0f) : 0.0f;
    c->vx += (target - c->vx) * 4.0f * dt;
    if (c->x < 12 && c->face < 0) c->face = 1;
    if (c->x > CANVAS_W - 12 && c->face > 0) c->face = -1;
    float fy = (float)world_floor((int)c->x) - (float)(ss ? ss->h : 6) * 0.5f + 1.0f;
    if (c->state == 1) fy += fm_sin(c->anim * 4.0f) * 0.6f; // pecking
    c->vy = (fy - c->y) * 6.0f;
}

static float floor_y(const Creature *c)
{
    const SpriteSheet *ss = species_sprite(c->sp);
    return (float)world_floor((int)c->x) - (float)(ss ? ss->h : 6) * 0.5f + 1.0f;
}

// Shrimp: walk and pick at the substrate; now and then (or when scared) a
// tail-flip hop backwards, then a slow sink. tx = hop velocity, ty = height offset.
static void update_shrimp(Creature *c, float dt)
{
    c->timer -= dt;
    if (c->state == 2) { // hopping
        c->tx += 45.0f * dt;
        c->ty += c->tx * dt;
        if (c->ty >= 0) {
            c->ty = c->tx = 0;
            c->state = 1;
            c->timer = rng_frange(&s_rng, 0.5f, 2.0f);
        }
        c->vx *= 1.0f - 1.5f * dt;
    } else {
        if (c->timer <= 0) {
            int r = rng_range(&s_rng, 0, 9);
            if (r == 0) { // spontaneous swim
                c->state = 2;
                c->tx = -rng_frange(&s_rng, 18, 30);
                c->vx = (float)c->face * c->speed * 3.0f;
            } else {
                c->state = c->state == 0 ? 1 : 0;
                if (rng_range(&s_rng, 0, 2) == 0) c->face = (int8_t)-c->face;
            }
            c->timer = rng_frange(&s_rng, 0.6f, 3.0f);
        }
        int f = food_nearest(c->x, c->y, 70.0f);
        if (f >= 0 && g_food[f].resting) {
            c->state = 0;
            c->face = g_food[f].x > c->x ? 1 : -1;
            if (fm_abs(g_food[f].x - c->x) < 3) g_food[f].alive = 0;
        }
        float target = c->state == 0 ? (float)c->face * c->speed : 0.0f;
        c->vx += (target - c->vx) * 5.0f * dt;
    }
    if (c->x < 10 && c->vx < 0) { c->face = 1; c->vx = -c->vx; }
    if (c->x > CANVAS_W - 10 && c->vx > 0) { c->face = -1; c->vx = -c->vx; }
    float fy = floor_y(c) + c->ty;
    if (c->state == 1) fy += fm_sin(c->anim * 5.0f) * 0.4f; // picking
    c->vy = (fy - c->y) * 8.0f;
}

// Snail: very slow crawl with long pauses; scared -> retracts for a while.
static void update_snail(Creature *c, float dt)
{
    c->timer -= dt;
    if (c->timer <= 0) {
        if (c->state == 0) {
            c->state = 1;
            c->timer = rng_frange(&s_rng, 3.0f, 9.0f);
        } else {
            c->state = 0;
            c->timer = rng_frange(&s_rng, 6.0f, 15.0f);
            if (rng_range(&s_rng, 0, 2) == 0) c->face = (int8_t)-c->face;
        }
    }
    int f = food_nearest(c->x, c->y, 60.0f);
    if (f >= 0 && g_food[f].resting && c->flee <= 0) {
        c->state = 0;
        c->face = g_food[f].x > c->x ? 1 : -1;
        if (fm_abs(g_food[f].x - c->x) < 3) g_food[f].alive = 0;
    }
    if (c->x < 10) c->face = 1;
    if (c->x > CANVAS_W - 10) c->face = -1;
    c->vx = (c->state == 0 && c->flee <= 0) ? (float)c->face * c->speed : 0.0f;
    c->vy = (floor_y(c) - c->y) * 4.0f;
}

void creatures_update(float dt)
{
    if (!s_cr) return;
    // school goals
    for (int i = 0; i < SPECIES_COUNT && i < 64; i++) {
        s_school_timer[i] -= dt;
        if (s_school_timer[i] <= 0) {
            s_school_timer[i] = rng_frange(&s_rng, 5, 12);
            s_school_tx[i] = rng_frange(&s_rng, 50, CANVAS_W - 50);
            s_school_ty[i] = band_y(i, rng_f01(&s_rng));
        }
    }
    // spatial grid
    for (int i = 0; i < GW * GH; i++) s_cell_head[i] = -1;
    for (int i = 0; i < s_n; i++) {
        int gx = im_clamp((int)s_cr[i].x / CELL, 0, GW - 1), gy = im_clamp((int)s_cr[i].y / CELL, 0, GH - 1);
        s_cell_next[i] = s_cell_head[gy * GW + gx];
        s_cell_head[gy * GW + gx] = (int16_t)i;
    }
    for (int i = 0; i < s_n; i++) {
        Creature *c = &s_cr[i];
        const SpeciesDef *d = &SPECIES[c->sp];
        if (c->flee > 0) c->flee -= dt;
        switch (d->behavior) {
        case BEH_SCHOOL: update_school(c, i, dt); break;
        case BEH_WANDER: update_wander(c, dt); break;
        case BEH_BOTTOM: update_bottom(c, dt); break;
        case BEH_SHRIMP: update_shrimp(c, dt); break;
        case BEH_SNAIL: update_snail(c, dt); break;
        }
        c->x += c->vx * dt;
        c->y += c->vy * dt;
        bool on_floor = d->behavior == BEH_BOTTOM || d->behavior == BEH_SHRIMP || d->behavior == BEH_SNAIL;
        hard_clamp(c, on_floor ? 1.0f : 4.0f);
        // facing with hysteresis (sprite flip); hopping shrimp swim backwards, snails turn only on purpose
        if (d->behavior != BEH_SNAIL && !(d->behavior == BEH_SHRIMP && c->state == 2)) {
            if (c->vx > 1.0f) c->face = 1;
            else if (c->vx < -1.0f) c->face = -1;
        }
        float spd = fm_abs(c->vx) + fm_abs(c->vy);
        c->anim += dt * (2.0f + spd * 0.25f);
    }
}

// ------------------------------------------------------------ rendering

void creatures_draw(Canvas *cv, float t)
{
    (void)t;
    if (!s_cr) return;
    // order by depth: far (high z) first
    for (int i = 0; i < s_n; i++) s_order[i] = (uint8_t)i;
    for (int i = 1; i < s_n; i++) {
        uint8_t v = s_order[i];
        int j = i - 1;
        while (j >= 0 && s_cr[s_order[j]].z < s_cr[v].z) {
            s_order[j + 1] = s_order[j];
            j--;
        }
        s_order[j + 1] = v;
    }
    for (int k = 0; k < s_n; k++) {
        const Creature *c = &s_cr[s_order[k]];
        const SpeciesDef *d = &SPECIES[c->sp];
        const uint16_t *pal = d->pal[c->variant];
        uint32_t br = (uint32_t)(256.0f - c->z * 70.0f);
        const SpriteSheet *ss = species_sprite(c->sp);
        if (!ss) continue;
        int frame = ((int)c->anim) & 3;
        if ((d->behavior == BEH_BOTTOM || d->behavior == BEH_SHRIMP) && c->state == 1) frame = 0;
        if (d->behavior == BEH_SNAIL) frame = c->flee > 0 ? 3 : (c->state == 0 ? ((int)(c->anim * 0.5f)) % 3 : 0);
        sprite_draw(cv, ss, frame, (int)c->x - ss->w / 2, (int)c->y - ss->h / 2, pal, c->face < 0 ? SPRITE_FLIP_X : 0,
                    br);
    }
}

bool creatures_near(int x, int y, int radius)
{
    for (int i = 0; i < s_n; i++) {
        float dx = s_cr[i].x - (float)x, dy = s_cr[i].y - (float)y;
        if (dx * dx + dy * dy <= (float)(radius * radius)) return true;
    }
    return false;
}

bool creatures_scare(int x, int y)
{
    bool hit = false;
    for (int i = 0; i < s_n; i++) {
        Creature *c = &s_cr[i];
        float dx = c->x - (float)x, dy = c->y - (float)y;
        float d2 = dx * dx + dy * dy;
        BehaviorType b = SPECIES[c->sp].behavior;
        float radius = b == BEH_SCHOOL ? 60.0f : (b == BEH_SNAIL ? 20.0f : 45.0f);
        if (d2 > radius * radius) continue;
        if (d2 < 14.0f * 14.0f) hit = true;
        if (b == BEH_SNAIL) { // retract into the shell
            c->flee = rng_frange(&s_rng, 3.0f, 6.0f);
            c->vx = 0;
            continue;
        }
        if (b == BEH_SHRIMP) { // tail flip away from the touch
            c->state = 2;
            c->tx = -rng_frange(&s_rng, 25, 40);
            c->face = dx >= 0 ? -1 : 1; // head towards the danger, body darts away
            c->vx = (dx >= 0 ? 1.0f : -1.0f) * 45.0f;
            c->flee = 1.0f;
            continue;
        }
        float d = fm_sqrt(d2) + 0.5f;
        float boost = SPECIES[c->sp].speed_max * 2.2f;
        c->vx += dx / d * boost;
        c->vy += dy / d * boost * 0.6f;
        c->flee = 1.4f;
        if (SPECIES[c->sp].behavior == BEH_BOTTOM) c->face = dx >= 0 ? 1 : -1;
        if (SPECIES[c->sp].behavior == BEH_WANDER) pick_target(c);
    }
    return hit;
}
