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
    {"neon_tetra", S_SP_NEON, "amazon", BEH_SCHOOL, 13, 14, 30, 40, 150, 12, 30, 2,
     {10, 4, 3, 4, TAIL_FORK, 1, PAT_NEON, BODY_SLIM, 0, 0},
     {{0, C(48,58,70), C(90,110,125), C(150,170,185), C(200,210,215), C(40,190,255), C(235,40,55), C(150,170,185), C(10,10,14), 0},
      {0, C(48,70,60), C(90,125,110), C(150,185,170), C(200,215,210), C(80,255,170), C(255,120,40), C(150,185,170), C(10,10,14), 0}}},
    {"cardinal_tetra", S_SP_CARDINAL, "blackwater", BEH_SCHOOL, 14, 14, 30, 40, 150, 14, 30, 1,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_CARDINAL, 0, 0},
     {{0, C(40,46,62), C(92,92,116), C(40,190,250), C(170,240,255), C(220,30,42), C(170,22,36), C(132,148,158), C(10,10,14), C(232,226,232), C(110,210,250), C(205,45,45), C(170,182,190)}}},
    {"ember_tetra", S_SP_EMBER, "amazon", BEH_SCHOOL, 10, 12, 26, 30, 130, 7, 20, 2,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_EMBER, 0, 0},
     {{0, C(150,40,20), C(210,70,35), C(240,98,45), C(250,150,100), C(185,55,32), 0, C(240,115,65), C(20,10,10), C(225,40,35), C(70,28,22), C(252,195,165)},   // rosso
      {0, C(160,120,25), C(220,165,40), C(240,192,58), C(250,222,135), C(195,140,35), 0, C(245,202,95), C(20,10,10), C(230,150,40), C(95,62,22), C(255,236,185)}}}, // oro
    {"rummy_nose", S_SP_RUMMY, "amazon", BEH_SCHOOL, 13, 14, 30, 50, 160, 10, 24, 1,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_RUMMY, 0, 0},
     {{0, C(112,122,122), C(168,184,180), C(198,210,206), C(224,230,228), C(228,238,234), C(222,32,32), C(186,198,194), C(10,10,14), C(244,64,52), C(18,18,22), C(246,246,246), C(168,20,26)}}},
    {"angelfish", S_SP_ANGELFISH, "amazon", BEH_WANDER, 30, 6, 14, 40, 170, 3, 6, 4,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_ANGEL, 0, 0},
     {{0, C(95,100,108), C(160,165,172), C(200,204,210), C(232,234,238), C(24,24,30), C(80,82,90), C(185,192,200), C(10,10,14), C(205,45,35)},   // selvatico argento
      {0, C(20,50,82), C(45,98,145), C(80,148,192), C(155,205,228), C(14,22,36), C(38,66,98), C(105,165,198), C(10,10,14), C(240,115,30)},     // blu (pinoy)
      {0, C(185,125,45), C(230,175,75), C(245,205,115), C(252,232,185), C(232,160,70), C(240,190,110), C(245,215,150), C(20,12,10), C(200,40,35)}, // oro
      {0, C(8,8,10), C(24,24,28), C(38,38,46), C(62,62,74), C(12,12,16), C(28,28,34), C(44,44,54), C(4,4,6), C(215,50,45)}}},                    // nero
    {"discus", S_SP_DISCUS, "blackwater", BEH_WANDER, 30, 5, 12, 50, 180, 3, 6, 3,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_DISCUS, 0, 0},
     {{0, C(85,14,14), C(160,30,25), C(200,48,32), C(225,100,50), C(115,205,235), C(80,150,195), C(150,40,35), C(15,5,5), C(240,45,40)},   // turchese rosso
      {0, C(18,32,85), C(40,80,170), C(62,118,212), C(115,168,232), C(170,235,252), C(115,195,235), C(50,92,175), C(15,5,5), C(225,55,45)}, // blue diamond
      {0, C(140,55,20), C(215,115,40), C(240,150,60), C(250,200,130), C(175,40,30), C(205,75,45), C(210,110,50), C(15,5,5), C(230,40,30)}}},  // pigeon blood
    {"corydoras", S_SP_CORYDORAS, "amazon", BEH_BOTTOM, 18, 6, 12, 220, 255, 6, 12, 3,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_CORY, 0, 0},
     {{0, C(28,40,32), C(32,74,58), C(48,110,84), C(226,124,58), C(242,128,52), C(32,74,58), C(226,112,72), C(10,10,14), C(62,36,26), C(130,180,210), C(232,176,128), C(48,110,84), C(38,90,68), C(190,90,60)},   // arancio/verde
      {0, C(52,46,32), C(96,92,58), C(112,132,88), C(216,192,152), C(152,124,78), C(96,92,58), C(182,162,122), C(10,10,14), C(72,56,36), C(150,162,150), C(202,182,142), C(112,132,88), C(92,112,72), C(150,130,96)},     // bronzo (aeneus)
      {0, C(140,136,130), C(222,218,210), C(238,234,226), C(248,246,240), C(236,232,224), C(28,28,32), C(220,214,204), C(10,10,14), C(28,28,32), C(150,150,150), C(220,214,204), C(28,28,32), C(214,210,202), C(180,176,168)}}}, // panda
    // ---------------- Sud-Est asiatico
    {"harlequin", S_SP_HARLEQUIN, "seasia", BEH_SCHOOL, 12, 12, 28, 40, 150, 10, 24, 1,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_HARLEQUIN, 0, 0},
     {{0, C(125,62,42), C(198,108,66), C(238,148,98), C(232,196,190), C(22,16,26), C(250,192,92), C(236,132,82), C(10,10,14), C(242,172,62), C(218,62,42)}}},
    {"gourami", S_SP_GOURAMI, "seasia", BEH_WANDER, 26, 6, 14, 40, 160, 2, 5, 3,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_GOURAMI, 0, 0},
     {{0, C(70,60,66), C(128,114,106), C(150,150,168), C(222,112,56), C(34,32,48), C(236,236,242), C(104,108,132), C(10,10,14), C(200,205,215), C(226,122,62), C(220,200,180)},   // perlato
      {0, C(120,40,28), C(182,62,36), C(222,92,52), C(242,142,92), C(140,40,26), C(110,165,240), C(160,62,40), C(10,10,14), C(200,205,215), C(96,145,232), C(240,190,150)},    // rosso
      {0, C(28,46,92), C(52,88,152), C(86,132,202), C(150,186,232), C(24,38,80), C(170,212,248), C(58,94,160), C(10,10,14), C(200,205,215), C(200,222,246), C(170,200,240)}}}, // blu
    {"kuhli_loach", S_SP_KUHLI, "seasia", BEH_BOTTOM, 24, 5, 12, 230, 255, 5, 12, 2,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_KUHLI, 0, 0},
     {{0, C(62,34,22), C(236,128,40), C(248,164,48), C(252,208,96), C(54,32,24), 0, C(226,196,156), C(10,10,10)},  // kuhlii
      {0, C(20,14,12), C(58,40,32), C(74,54,42), C(122,92,72), C(46,30,24), 0, C(96,76,64), C(10,10,10)}}},        // nero (oblonga)
    {"betta", S_SP_BETTA, "seasia", BEH_WANDER, 24, 4, 10, 20, 150, 1, 1, 4,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_BETTA, 0, 0},
     {{0, C(70,50,70), C(200,212,236), C(238,238,244), C(242,206,186), C(110,170,250), C(238,112,62), C(236,104,52), C(10,10,14), C(250,170,90), C(46,22,26), C(252,158,96), C(42,92,206), C(84,140,232), C(196,78,40), C(156,52,34)},  // koi
      {0, C(6,6,10), C(30,34,46), C(42,46,60), C(52,52,64), C(96,108,136), C(36,36,46), C(22,18,26), C(4,4,6), C(150,70,60), C(70,28,34), C(98,64,84), C(40,44,60), C(34,34,46), C(150,46,52), C(14,14,20)}, // crowntail nero
      {0, C(90,10,14), C(170,24,30), C(204,34,38), C(222,70,64), C(240,98,96), C(196,30,34), C(200,26,32), C(10,6,8), C(210,64,52), C(110,10,18), C(234,74,72), C(190,28,34), C(180,26,32), C(150,16,24), C(120,16,22)},          // rosso
      {0, C(14,24,70), C(28,60,160), C(40,90,210), C(70,110,200), C(112,180,255), C(30,56,150), C(36,80,200), C(6,8,14), C(80,130,230), C(16,24,80), C(92,152,250), C(34,76,190), C(30,64,170), C(26,44,140), C(18,30,90)}}},    // blu
    {"snakehead", S_SP_SNAKEHEAD, "seasia", BEH_WANDER, 40, 3, 9, 150, 235, 1, 1, 1,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_SNAKEHEAD, 0, 0},
     {{0, C(38,30,22), C(78,64,44), C(120,100,68), C(176,156,116), C(48,36,26), C(184,164,110), C(100,88,66), C(10,8,6), C(204,152,62), C(64,54,40), C(132,80,56), C(38,28,20), C(192,182,162)}}},
    // ---------------- Africa occidentale (Congo)
    {"congo_tetra", S_SP_CONGO_TETRA, "africa", BEH_SCHOOL, 16, 12, 26, 40, 160, 7, 16, 1,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_CONGO, 0, 0},
     {{0, C(40,60,112), C(72,132,202), C(165,202,212), C(222,228,236), C(236,142,62), C(246,206,92), C(122,138,165), C(15,15,20), C(206,212,222), C(236,242,252), C(122,232,232), C(122,102,192)}}},
    {"kribensis", S_SP_KRIBENSIS, "africa", BEH_WANDER, 34, 6, 14, 150, 240, 2, 4, 2,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_KRIB, 0, 0},
     {{0, C(60,52,40), C(150,135,95), C(212,204,178), C(232,226,200), C(48,40,34), C(232,70,100), C(214,198,96), C(10,10,10), C(238,214,44), C(80,130,230), C(232,70,50), C(24,20,20), C(170,90,190), C(232,180,60), C(214,196,92)},     // femmina
      {0, C(62,56,44), C(160,145,100), C(218,210,186), C(236,230,206), C(52,44,36), C(214,112,122), C(220,200,110), C(10,10,10), C(226,202,82), C(90,140,230), C(236,110,60), C(24,20,20), C(120,100,190), C(222,182,62), C(210,180,90)}}}, // maschio
    {"shrimp", S_SP_SHRIMP, "seasia", BEH_SHRIMP, 12, 3, 8, 255, 255, 8, 30, 4,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_SHRIMP, 0, 0},
     {{0, C(120,20,20), C(200,40,35), C(235,90,80), 0, C(200,40,35), C(170,30,30), C(220,80,70), C(10,10,10), 0},    // red cherry
      {0, C(150,150,155), C(235,235,235), C(255,255,255), 0, C(210,30,40), C(235,235,235), C(230,220,220), C(10,10,10), 0}, // crystal red
      {0, C(80,90,85), C(150,160,150), C(190,200,190), 0, C(150,160,150), C(90,70,60), C(160,170,160), C(10,10,10), 0},  // amano
      {0, C(20,40,110), C(40,80,190), C(90,130,230), 0, C(40,80,190), C(30,60,160), C(60,100,200), C(10,10,10), 0}}},    // blue dream
    {"assassin_snail", S_SP_ASSASSIN_SNAIL, "seasia", BEH_SNAIL, 13, 0.8f, 2.0f, 255, 255, 2, 8, 1,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_CONE, 0, 0},
     {{0, C(30,20,15), C(75,48,26), C(120,85,50), C(170,160,130), C(232,192,64), 0, 0, C(15,15,15), 0}}},
    // ---------------- Lago Malawi
    {"mbuna", S_SP_MBUNA, "malawi", BEH_WANDER, 30, 8, 18, 60, 230, 6, 12, 4,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_MBUNA, 0, 0},
     {{0, C(172,140,12), C(236,200,36), C(250,220,56), C(252,236,132), C(244,210,46), C(226,190,36), C(250,214,52), C(12,12,12), C(30,30,30), C(20,20,24), 0, C(252,232,92), C(20,20,24), C(250,226,92)},       // yellow lab
      {0, C(46,40,92), C(106,96,192), C(150,140,222), C(192,182,236), C(52,42,92), C(92,112,204), C(242,202,42), C(12,12,12), C(232,142,42), C(252,222,62), 0, C(252,232,92), C(242,202,42), C(182,172,232)}, // viola, pinne gialle
      {0, C(182,72,22), C(236,116,42), C(246,142,56), C(250,186,122), C(226,112,42), C(232,122,46), C(242,132,56), C(12,12,12), C(30,30,30), C(252,182,92), 0, C(252,222,82), C(242,132,56), C(246,152,82)},       // red zebra
      {0, C(36,56,120), C(90,140,216), C(130,176,236), C(186,210,246), C(26,36,82), C(70,110,200), C(100,150,226), C(12,12,12), C(30,30,30), C(200,226,250), 0, C(252,202,62), C(30,40,92), C(160,196,242)}}}, // blue zebra
    {"aulonocara", S_SP_AULONOCARA, "malawi", BEH_WANDER, 27, 6, 14, 40, 200, 3, 6, 2,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_PEACOCK, 0, 0},
     {{0, C(20,30,82), C(40,70,180), C(60,100,222), C(110,150,236), C(42,64,162), C(150,192,255), C(60,96,212), C(10,10,14), C(72,102,232), C(220,236,255), C(232,132,42), C(25,35,110), C(242,192,62), C(30,40,110)},   // blu
      {0, C(110,30,20), C(190,55,30), C(226,82,42), C(242,132,82), C(186,50,30), C(252,162,112), C(212,72,42), C(10,10,14), C(72,102,222), C(202,222,255), C(242,152,52), C(62,72,172), C(242,192,62), C(40,50,140)}}}, // rosso
    // ---------------- Laghetto
    {"koi", S_SP_KOI, "pond", BEH_WANDER, 34, 6, 14, 40, 200, 3, 6, 3,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_KOI, 0, 0},
     {{0, C(170,170,168), C(228,228,226), C(244,244,240), C(252,252,250), C(225,60,30), C(244,244,240), C(192,200,208), C(20,20,22), C(212,212,210), C(168,178,190), C(212,142,132), C(195,45,25)},  // kohaku
      {0, C(170,170,168), C(228,228,226), C(244,244,240), C(252,252,250), C(225,60,30), C(25,25,30), C(192,200,208), C(20,20,22), C(212,212,210), C(168,178,190), C(212,142,132), C(195,45,25)},    // sanke
      {0, C(180,130,30), C(230,180,60), C(245,205,80), C(250,225,140), C(245,205,80), C(245,205,80), C(232,186,86), C(20,20,22), C(225,175,55), C(212,160,58), C(220,170,80), C(225,175,55)}}}, // ogon
    {"goldfish", S_SP_GOLDFISH, "pond", BEH_WANDER, 21, 8, 18, 50, 170, 2, 6, 2,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_GOLDFISH, 0, 0},
     {{0, C(170,60,10), C(225,100,20), C(245,135,30), C(252,185,90), C(255,200,110), C(215,85,20), C(245,125,40), C(25,15,10), C(250,190,60), C(255,190,130), 0, C(245,135,30), C(255,200,110), C(225,100,20)}, // arancio
      {0, C(150,140,140), C(225,222,220), C(245,242,240), C(252,250,248), C(255,255,255), C(215,200,195), C(240,232,228), C(25,15,10), C(205,195,185), C(255,240,235), 0, C(225,60,40), C(245,110,80), C(190,40,30)}}}, // rosso e bianco
    {"medaka", S_SP_MEDAKA, "pond", BEH_SCHOOL, 11, 12, 28, 20, 120, 8, 20, 3,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_MEDAKA, 0, 0},
     {{0, C(190,95,35), C(235,140,55), C(248,172,90), C(252,214,160), C(215,115,45), C(255,225,180), C(245,180,110), C(10,10,14), C(250,212,150), C(230,140,70), C(255,235,200)},  // arancio (himedaka)
      {0, C(170,170,168), C(220,220,215), C(236,236,230), C(248,248,244), C(200,200,196), C(255,255,255), C(225,228,228), C(10,10,14), C(235,235,235), C(195,200,205), C(255,255,255)}, // bianco
      {0, C(120,140,165), C(185,200,220), C(215,225,238), C(240,244,250), C(160,180,205), C(250,252,255), C(175,205,225), C(10,10,14), C(230,235,240), C(120,170,210), C(245,250,255)}}}, // argento-blu
    {"white_cloud", S_SP_WHITE_CLOUD, "pond", BEH_SCHOOL, 12, 12, 28, 20, 130, 8, 20, 2,
     {0, 0, 0, 0, 0, 0, PAT_NONE, BODY_TANICHTHYS, 0, 0},
     {{0, C(40,40,72), C(82,82,146), C(132,112,156), C(212,202,214), C(236,226,206), C(184,72,62), C(222,190,92), C(10,10,14), C(222,222,232), C(226,52,42), C(222,52,42), C(20,16,22), C(242,242,242)},       // classico
      {0, C(150,92,52), C(226,162,102), C(240,182,122), C(250,216,176), C(255,232,192), C(242,172,112), C(222,162,112), C(10,10,14), C(232,232,236), C(242,242,242), C(222,62,42), C(204,92,62), C(250,250,250)}}}, // gold
    {"ramshorn", S_SP_RAMSHORN, "pond", BEH_SNAIL, 14, 0.6f, 1.6f, 255, 255, 4, 12, 3,
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
#define SPRITE_POOL 36864 // ~34 KB used at the current scales (logged at init)
static uint8_t s_sprite_mem[SPRITE_POOL];
static int s_sprite_used;
static SpriteSheet s_sprites[32];

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

// ---- design space. The hand-tuned generators below are written in "design"
// pixels and sampled at s_k output pixels per design pixel, so every sprite can
// be enlarged without blocky nearest-neighbour stretching. Output dimensions
// are SKI(design); fine patterns (pearls, rays, sheen) stay 1 output pixel.

static float s_k = 1.0f;
#define SKI(v) ((int)((float)(v) * s_k + 0.999f))

static int ox(float v) { return (int)floorf(v * s_k); } // design coord -> output px

// Fill the output pixels covering design pixel (floor(x), floor(y)).
static void dput(const SpriteSheet *ss, int fr, float x, float y, uint8_t v)
{
    const float fx = floorf(x), fy = floorf(y);
    const int xa = ox(fx), ya = ox(fy);
    int xb = ox(fx + 1.0f), yb = ox(fy + 1.0f);
    if (xb <= xa) xb = xa + 1;
    if (yb <= ya) yb = ya + 1;
    for (int yy = ya; yy < yb; yy++)
        for (int xx = xa; xx < xb; xx++) sheet_put(ss, fr, xx, yy, v);
}

// 1-output-pixel line between two design points (threads, barbels, legs).
static void dline(const SpriteSheet *ss, int fr, float xa, float ya, float xb, float yb, uint8_t v)
{
    const float len = fmaxf(fm_abs(xb - xa), fm_abs(yb - ya)) * s_k;
    const int n = (int)len + 1;
    for (int i = 0; i <= n; i++) {
        const float t = (float)i / (float)n;
        sheet_put(ss, fr, ox(xa + (xb - xa) * t), ox(ya + (yb - ya) * t), v);
    }
}

// Snails, facing right. Frames 0-2 crawling (foot ripple, tentacles), frame 3
// retracted into the shell.
static void snail_generate(SpriteSheet *ss, int body)
{
    const bool cone = body == BODY_CONE;
    const int W = cone ? 14 : 15, H = 12;
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const bool out = fr < 3;
        // foot + head
        if (out) {
            const float front = 12.0f + (fr == 1 ? 1.0f : 0.0f);
            for (int x = 2; x <= (int)front; x++) {
                dput(ss, fr, (float)x, H - 2, 4);
                dput(ss, fr, (float)x, H - 1, ((x + fr) % 3) ? 4 : 1); // ripple of the sole
            }
            const float tip = fr == 2 ? 1.0f : 0.0f;
            if (cone) { // siphon tube raised forward + short tentacles
                dput(ss, fr, front - 1, H - 3, 4);
                dput(ss, fr, front, H - 4, 4);
                dput(ss, fr, front + 1, H - 5 - tip, 4);
                dput(ss, fr, front, H - 3, 4);
                dput(ss, fr, front - 1, H - 4, 8);
            } else { // long thin tentacles with eyes at their base
                dput(ss, fr, front - 1, H - 3, 4);
                dput(ss, fr, front - 2, H - 3, 8);
                dline(ss, fr, front - 0.5f, H - 3.5f, front + 1.0f, H - 5.5f - tip, 4);
                dline(ss, fr, front - 1.5f, H - 3.5f, front - 0.5f, H - 4.5f, 4);
            }
        } else {
            for (int x = 5; x <= 8; x++) dput(ss, fr, (float)x, H - 1, 1);
        }
        // shell
        for (int y = 0; y < ss->h; y++)
            for (int x = 0; x < ss->w; x++) {
                float px = ((float)x + 0.5f) / s_k, py = ((float)y + 0.5f) / s_k;
                if (py >= (float)(H - 1)) continue;
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
    const float K = 1.3f * s_k;        // overall scale (1.0 = 24x22 sprite)
    const int TL = (int)(6.0f * s_k);  // tail length
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

// Angelfish (Pterophyllum scalare), facing right: rhombic laterally compressed
// body with a steep forehead and pointed snout, tall dorsal and anal fins swept
// back into trailing points beyond the tail, long ventral filaments, lyre tail,
// four slanted dark bands (eye, mid body, rear body, peduncle) that run into the
// fins, orange/red iris.
static void angel_generate(SpriteSheet *ss)
{
    const int W = 32, H = 46;
    const float cy = 22.5f;          // body centre line
    const float xr = 10.5f, xs = 30.5f; // peduncle .. snout tip
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    static const float FLAP[FISH_FRAMES] = {0.0f, 1.0f, 0.0f, -1.0f};
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP[fr];
        // body half-heights at column centre px (top/bottom, before the snout offset)
#define ANG_U(px) (((px) - xr) / (xs - xr)) // 0 peduncle .. 1 snout
        // tail: lyre fan, lobe tips trail at top and bottom
        for (int x = 0; x < SKI(11); x++) {
            const float tx = ((float)x + 0.5f) / s_k - 0.5f;
            float d = (11.0f - tx) / 11.0f; // 1 at the tip
            float hh = 1.8f + 5.2f * d, off = fl * d * 1.2f;
            for (int y = 0; y < ss->h; y++) {
                float dy = ((float)y + 0.5f) / s_k - (cy + off);
                float a = fm_abs(dy);
                if (a > hh + (d > 0.75f ? 2.5f * (d - 0.75f) / 0.25f : 0)) continue;
                if (d > 0.7f && a < hh * 0.55f - (d - 0.7f) * 6.0f) continue; // concave trailing edge
                uint8_t v = ((y + x / 3) % 3 == 0) ? 4 : 7;                   // fin rays
                if (tx >= 7.5f) v = 5;                                        // peduncle band
                else if (tx >= 6.5f) v = 6;
                sheet_put(ss, fr, x, y, v);
            }
        }
        for (int x = ox(2.0f); x < ss->w; x++) { // from x=2: fin streamers trail over the tail
            const float pxc = ((float)x + 0.5f) / s_k, u = ANG_U(pxc);
            if (u > 1.0f) continue;
            // body profile: deep in the middle, straight forehead to the snout
            float ht, hb;
            if (u < 0.0f) ht = hb = 2.0f;
            else if (u < 0.45f) ht = hb = 2.0f + 6.5f * fm_sqrt(u / 0.45f);
            else {
                ht = 8.5f * (1.0f - u) / 0.55f + 0.6f; // forehead
                hb = 8.5f * fm_sqrt((1.0f - u) / 0.55f); // rounder throat
            }
            const float top = cy - 1.0f * (u > 0.45f ? (u - 0.45f) / 0.55f : 0) - ht; // snout slightly high
            const float bot = cy + hb - 1.2f * (u > 0.6f ? (u - 0.6f) / 0.4f : 0);
            // dorsal fin: base u 0.05..0.62, tip trailing back above the tail
            const float dtx = 4.0f + fl * 0.6f, dty = 0.5f;
            const float dfx = xr + 0.52f * (xs - xr), dfy = cy - 8.6f;
            const float dbx = xr + 0.1f * (xs - xr), dby = cy - 4.2f;
            // anal fin: longer, tip trailing further down
            const float atx = 5.0f - fl * 0.6f, aty = (float)H - 1.0f;
            const float afx = xr + 0.48f * (xs - xr), afy = cy + 8.4f;
            const float abx = xr + 0.1f * (xs - xr), aby = cy + 4.2f;
            for (int y = 0; y < ss->h; y++) {
                const float pyc = ((float)y + 0.5f) / s_k;
                // band index: slanted (top leans back), centres at u ~0.82 (eye), 0.56, 0.24
                float bu = u + (pyc - cy) * 0.008f;
                int band = 0; // 0 none, 1 core, 2 edge
                static const float BC[3] = {0.84f, 0.56f, 0.25f}, BW[3] = {0.03f, 0.045f, 0.04f};
                for (int b = 0; b < 3; b++) {
                    float dd = fm_abs(bu - BC[b]);
                    if (dd < BW[b]) band = 1;
                    else if (dd < BW[b] + 0.03f && band == 0) band = 2;
                }
                if (pxc >= 9.0f && pyc >= top && pyc <= bot) { // body
                    float tt = (pyc - top) / (bot - top);
                    uint8_t v = tt < 0.12f ? 1 : (tt < 0.38f ? 2 : (tt < 0.78f ? 3 : 4));
                    if (v == 3 && ((x + 2 * y) % 5) == 0) v = 4; // scale sheen
                    if (u < 0.0f) v = 5;                         // peduncle band
                    else if (band == 1) v = 5;
                    else if (band == 2 && v != 1) v = 6;
                    sheet_put(ss, fr, x, y, v);
                    continue;
                }
                // fins: triangle (base front, tip, base rear) above / below the body
                bool in = false;
                uint8_t v = 7;
                if (pyc < top && pxc < dfx) {
                    // leading edge tip->front, trailing edge tip->rear base
                    float lead = dty + (dfy - dty) * fm_clamp((pxc - dtx) / (dfx - dtx), 0, 1);
                    float tq = fm_clamp((pxc - dtx) / (dbx - dtx), 0, 1);
                    float trail = pxc < dbx ? dty + (dby - dty) * powf(tq, 1.5f) : top; // concave: thin streamer
                    if (trail < lead + 1.5f) trail = lead + 1.5f;
                    if (pxc >= dtx - 0.5f && pyc >= lead && pyc <= trail + 0.5f) {
                        in = true;
                        if (pyc < lead + 1.0f && pxc > dtx + 5.0f) v = 1; // dark leading edge
                    }
                } else if (pyc > bot && pxc < afx) {
                    float lead = aty + (afy - aty) * fm_clamp((pxc - atx) / (afx - atx), 0, 1);
                    float tq = fm_clamp((pxc - atx) / (abx - atx), 0, 1);
                    float trail = pxc < abx ? aty + (aby - aty) * powf(tq, 1.5f) : bot;
                    if (trail > lead - 1.5f) trail = lead - 1.5f;
                    if (pxc >= atx - 0.5f && pyc <= lead && pyc >= trail - 0.5f) {
                        in = true;
                        if (pyc > lead - 1.0f && pxc > atx + 5.0f) v = 1;
                    }
                }
                if (!in) continue;
                if (v == 7) {
                    if (band == 1) v = 5;
                    else if (band == 2) v = 6;
                    else if (((x + y) % 4) == 0) v = 4; // translucent rays
                }
                sheet_put(ss, fr, x, y, v);
            }
        }
        // ventral filaments: two long threads from the throat, swaying
        {
            float fx0 = xr + 0.66f * (xs - xr), fy0 = cy + 6.5f;
            for (int k = 0; k < 14; k++) {
                const float s0 = fl * (float)k * 0.06f, s1 = fl * (float)(k + 1) * 0.06f;
                dline(ss, fr, fx0 - (float)k * (0.45f + s0 * 0.1f) + s0, fy0 + (float)k,
                      fx0 - (float)(k + 1) * (0.45f + s1 * 0.1f) + s1, fy0 + (float)(k + 1), 4);
                if (k < 11)
                    dline(ss, fr, fx0 + 1.0f - (float)k * 0.3f - s0, fy0 + (float)k, fx0 + 1.0f - (float)(k + 1) * 0.3f - s1,
                          fy0 + (float)(k + 1), 7);
            }
        }
        // eye: dark pupil, coloured iris, inside the eye band
        const int ex = ox(26.5f), ey = ox(cy - 1.5f);
        sheet_put(ss, fr, ex, ey, 8);
        sheet_put(ss, fr, ex - 1, ey, 9);
        sheet_put(ss, fr, ex + 1, ey, 9);
        sheet_put(ss, fr, ex, ey - 1, 9);
        sheet_put(ss, fr, ex, ey + 1, 9);
        sheet_put(ss, fr, ex + 1, ey - 1, 4); // catch-light
        // mouth + pectoral fin (beating)
        dput(ss, fr, 30, cy - 1.0f, 1);
        dput(ss, fr, 22, cy + 1, 7);
        dput(ss, fr, 21, cy + 1 + (float)(fr & 1), 7);
        dput(ss, fr, 20, cy + 2 - (float)(fr & 1), 4);
#undef ANG_U
    }
}

// ---- shared helpers for the hand-tuned fish generators (all facing right)

static const float FLAP4[FISH_FRAMES] = {0.0f, 1.0f, 0.0f, -1.0f};
#define PEARL(x, y) (((y) & 1) == 0 && (((x) + ((y) >> 1)) & 1) == 0) // staggered 2 px dot grid

// Fin fill: area `v`, pattern `ray` where (x*rx + y*ry) % rmod == 0 (rmod < 0:
// staggered pearl dots, see PEARL), and the
// polygon edges flagged in `emask` (bit i = edge p[i] -> p[i+1]) drawn in `ev`.
typedef struct {
    uint8_t v, ray;
    int8_t rx, ry, rmod;
    uint8_t ev;
    uint16_t emask;
} FinStyle;

static void poly_fill(const SpriteSheet *ss, int fr, const float *pd, int n, FinStyle st)
{
    float p[32] = {0}; // design -> output px
    if (n > 16) n = 16;
    for (int i = 0; i < 2 * n; i++) p[i] = pd[i] * s_k;
    float x0 = p[0], x1 = p[0], y0 = p[1], y1 = p[1];
    for (int i = 1; i < n; i++) {
        x0 = fminf(x0, p[2 * i]), x1 = fmaxf(x1, p[2 * i]);
        y0 = fminf(y0, p[2 * i + 1]), y1 = fmaxf(y1, p[2 * i + 1]);
    }
    for (int y = (int)floorf(y0); y <= (int)y1; y++)
        for (int x = (int)floorf(x0); x <= (int)x1; x++) {
            const float px = (float)x + 0.5f, py = (float)y + 0.5f;
            bool in = false;
            float dmin = 1e9f;
            int emin = -1;
            for (int i = 0, j = n - 1; i < n; j = i++) {
                const float xi = p[2 * i], yi = p[2 * i + 1], xj = p[2 * j], yj = p[2 * j + 1];
                if ((yi > py) != (yj > py) && px < (xj - xi) * (py - yi) / (yj - yi) + xi) in = !in;
                const float ex = xi - xj, ey = yi - yj, l2 = ex * ex + ey * ey;
                const float t = l2 > 0 ? fm_clamp(((px - xj) * ex + (py - yj) * ey) / l2, 0.0f, 1.0f) : 0.0f;
                const float dx = xj + ex * t - px, dy = yj + ey * t - py, d = dx * dx + dy * dy;
                if (d < dmin) dmin = d, emin = j;
            }
            if (!in && dmin > 0.16f) continue; // keep thin tips visible
            uint8_t v = st.v;
            if (st.ray && st.rmod > 0 && ((x * st.rx + y * st.ry) % st.rmod + st.rmod) % st.rmod == 0) v = st.ray;
            if (st.ray && st.rmod < 0 && PEARL(x, y)) v = st.ray;
            if (emin >= 0 && ((st.emask >> emin) & 1) && dmin < 0.64f) v = st.ev;
            sheet_put(ss, fr, x, y, v);
        }
}

// Body profile: half-heights above (top) and below (bot) the centre line at
// u = 0 (caudal peduncle) .. 1 (snout), linear between knots.
typedef struct {
    float u, top, bot;
} Knot;

static void prof_at(const Knot *k, int n, float u, float *top, float *bot)
{
    if (u <= k[0].u) {
        *top = k[0].top, *bot = k[0].bot;
        return;
    }
    for (int i = 1; i < n; i++)
        if (u <= k[i].u) {
            const float t = (u - k[i - 1].u) / (k[i].u - k[i - 1].u);
            *top = k[i - 1].top + (k[i].top - k[i - 1].top) * t;
            *bot = k[i - 1].bot + (k[i].bot - k[i - 1].bot) * t;
            return;
        }
    *top = k[n - 1].top, *bot = k[n - 1].bot;
}

#define XU(u) (x0 + (u) * (x1 - x0))
#define NK(k) ((int)(sizeof(k) / sizeof((k)[0])))
#define NP(p) ((int)(sizeof(p) / sizeof((p)[0]) / 2))

// One body pixel handed to a species' colouring function: u = 0 (peduncle) .. 1
// (snout), tt = 0 (back edge) .. 1 (belly edge), py = design row, x/y = output px.
typedef struct {
    float u, tt, py;
    int x, y;
} BodyPx;
typedef uint8_t (*BodyPat)(const BodyPx *b);

// Fill the knot profile around the centre line cy between x0 (peduncle) and x1
// (snout); `pat` returns the palette index of each pixel (0 = leave empty).
static void body_fill(const SpriteSheet *ss, int fr, const Knot *k, int nk, float cy, float x0, float x1, BodyPat pat)
{
    for (int x = 0; x < ss->w; x++) {
        const float u = (((float)x + 0.5f) / s_k - x0) / (x1 - x0);
        if (u < 0.0f || u > 1.0f) continue;
        float top, bot;
        prof_at(k, nk, u, &top, &bot);
        for (int y = 0; y < ss->h; y++) {
            const float py = ((float)y + 0.5f) / s_k;
            if (py < cy - top || py > cy + bot) continue;
            const BodyPx b = {u, (py - (cy - top)) / (top + bot), py, x, y};
            const uint8_t v = pat(&b);
            if (v) sheet_put(ss, fr, x, y, v);
        }
    }
}

// Eye at design point (x, y): pupil, iris ring (4 neighbours, or the full 3x3
// block when `big`), optional catch-light up-right.
static void fish_eye(const SpriteSheet *ss, int fr, float x, float y, uint8_t pupil, uint8_t ring, bool big)
{
    const int ex = ox(x), ey = ox(y);
    if (ring)
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                if (big || dx == 0 || dy == 0) sheet_put(ss, fr, ex + dx, ey + dy, ring);
    sheet_put(ss, fr, ex, ey, pupil);
}

// Corydoras: arched back over a flat belly, tall triangular dorsal with a dark
// spine, adipose fin, forked tail, sloping snout with barbels, eye high in a
// dark mask, belly fins that touch the bottom row (resting on the substrate).
static void cory_generate(SpriteSheet *ss)
{
    const int W = 19, H = 12;
    const float cy = 7.8f, x0 = 5.0f, x1 = 18.6f;
    static const Knot K[] = {{0, 1.3f, 1.2f}, {0.25f, 2.6f, 2.0f}, {0.55f, 3.8f, 2.6f},
                             {0.75f, 3.5f, 2.6f}, {0.9f, 2.2f, 2.2f}, {1.0f, 0.7f, 1.5f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 0.8f;
        const float tail[] = {x0 + 0.5f, cy - 1.2f, 0.4f, cy - 4.2f + o, 2.6f, cy + o * 0.5f, 0.4f, cy + 3.6f + o, x0 + 0.5f, cy + 1.2f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 14, 0, 1, 2, 0, 0});
        const float dors[] = {XU(0.44f), cy - 3.4f, XU(0.5f), cy - 7.6f, XU(0.58f), cy - 7.2f, XU(0.7f), cy - 3.4f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 14, 1, 0, 2, 14, 1 << 2});
        dput(ss, fr, XU(0.14f), cy - 2.6f, 7); // adipose fin
        for (int x = 0; x < ss->w; x++) {
            const float u = (((float)x + 0.5f) / s_k - x0) / (x1 - x0);
            if (u < 0.0f || u > 1.0f) continue;
            float top, bot;
            prof_at(K, NK(K), u, &top, &bot);
            for (int y = 0; y < ss->h; y++) {
                const float py = ((float)y + 0.5f) / s_k;
                if (py < cy - top || py > cy + bot) continue;
                const float tt = (py - (cy - top)) / (top + bot);
                uint8_t v = tt < 0.14f ? 1 : (tt < 0.4f ? 2 : (tt < 0.78f ? 3 : 4));
                if (v == 3 && fm_abs(tt - 0.56f) < 0.07f) v = 13;                // armour plate seam
                if (u + tt * 0.06f > 0.72f && v != 1 && tt < 0.8f) v = 5;        // head
                if (u > 0.42f && u < 0.66f && tt < 0.4f && v != 1) v = 6;         // patch under the dorsal
                if (u < 0.14f && fm_abs(tt - 0.5f) < 0.26f) v = 12;               // peduncle spot
                if (u > 0.79f && u < 0.9f && tt > 0.1f && tt < 0.62f) v = 9;      // eye mask
                sheet_put(ss, fr, x, y, v);
            }
        }
        const int ex = ox(XU(0.85f)), ey = ox(cy - 1.4f);
        sheet_put(ss, fr, ex, ey, 8);
        sheet_put(ss, fr, ex + 1, ey, 10);
        dline(ss, fr, XU(0.99f), cy + 1.0f, XU(1.0f), cy + 2.2f, 11); // barbels
        dline(ss, fr, XU(0.94f), cy + 1.6f, XU(0.93f), cy + 2.8f, 11);
        const float pect[] = {XU(0.8f), cy + 1.6f, XU(0.56f), cy + 4.0f + fl * 0.3f, XU(0.7f), cy + 3.4f};
        poly_fill(ss, fr, pect, NP(pect), (FinStyle){7, 0, 0, 0, 0, 14, 1});
        const float pelv[] = {XU(0.5f), cy + 2.2f, XU(0.34f), cy + 3.8f, XU(0.44f), cy + 3.4f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        const float anal[] = {XU(0.26f), cy + 1.4f, XU(0.1f), cy + 2.8f, XU(0.2f), cy + 2.6f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 0, 0, 0, 0, 0, 0});
    }
}

// Pearl gourami: long oval body covered in pearl dots, zigzag dark line from the
// eye to a spot on the peduncle, orange chest, long anal fin with an orange
// edge, dorsal set far back, thread-like ventral feelers trailing backwards.
static void gourami_generate(SpriteSheet *ss)
{
    const int W = 26, H = 19;
    const float cy = 8.5f, x0 = 6.0f, x1 = 25.6f;
    static const Knot K[] = {{0, 2.0f, 2.0f}, {0.2f, 3.8f, 4.2f}, {0.45f, 4.6f, 4.8f},
                             {0.7f, 4.0f, 4.0f}, {0.88f, 2.6f, 2.4f}, {1.0f, 0.9f, 0.6f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    const FinStyle dotted = {7, 6, 0, 0, -1, 0, 0};
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 1.0f;
        const float tail[] = {x0 + 0.5f, cy - 2.0f, 2.0f, cy - 5.4f + o, 0.3f, cy - 4.2f + o, 1.3f, cy + o,
                              0.3f, cy + 4.2f + o, 2.0f, cy + 5.4f + o, x0 + 0.5f, cy + 2.0f};
        poly_fill(ss, fr, tail, NP(tail), dotted);
        const float dors[] = {XU(0.44f), cy - 4.4f, XU(0.22f), cy - 7.8f, XU(0.06f), cy - 7.4f + fl * 0.4f,
                              XU(-0.04f), cy - 5.2f + fl * 0.4f, XU(0.02f), cy - 1.8f};
        poly_fill(ss, fr, dors, NP(dors), dotted);
        const float anal[] = {XU(0.72f), cy + 3.6f, XU(0.45f), cy + 7.2f, XU(0.15f), cy + 8.6f,
                              XU(-0.1f), cy + 8.4f - fl * 0.4f, XU(-0.06f), cy + 6.4f - fl * 0.4f, XU(0.02f), cy + 1.8f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 6, 0, 0, -1, 10, (1 << 0) | (1 << 1) | (1 << 2)});
        for (int x = 0; x < ss->w; x++) {
            const float u = (((float)x + 0.5f) / s_k - x0) / (x1 - x0);
            if (u < 0.0f || u > 1.0f) continue;
            float top, bot;
            prof_at(K, NK(K), u, &top, &bot);
            const float lc = cy + 0.6f + 0.5f * fm_sin(((float)x + 0.5f) / s_k * 1.3f) - (u > 0.72f ? (u - 0.72f) * 7.0f : 0.0f);
            for (int y = 0; y < ss->h; y++) {
                const float py = ((float)y + 0.5f) / s_k;
                if (py < cy - top || py > cy + bot) continue;
                const float tt = (py - (cy - top)) / (top + bot);
                uint8_t v = tt < 0.1f ? 1 : (tt < 0.42f ? 2 : (tt < 0.72f ? 3 : 4));
                if (v == 4 && u < 0.5f) v = 3;                                      // orange only on the chest
                if (v != 1 && PEARL(x, y)) v = 6;                                    // pearls
                if (fm_abs(py - lc) < 0.55f && u > 0.05f && u < 0.9f) v = 5;        // zigzag line
                if (u < 0.07f && fm_abs(tt - 0.5f) < 0.2f) v = 5;                   // peduncle spot
                sheet_put(ss, fr, x, y, v);
            }
        }
        const int ex = ox(XU(0.87f)), ey = ox(cy - 0.6f);
        sheet_put(ss, fr, ex, ey, 8);
        sheet_put(ss, fr, ex - 1, ey, 9);
        sheet_put(ss, fr, ex + 1, ey, 9);
        dput(ss, fr, W - 1, cy - 1.0f, 1); // upturned mouth
        const float pect[] = {XU(0.76f), cy + 0.5f, XU(0.62f), cy + 2.6f + fl * 0.3f, XU(0.66f), cy + 0.9f};
        poly_fill(ss, fr, pect, NP(pect), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        const float fx = XU(0.66f), fy = cy + 3.6f;
        dline(ss, fr, fx, fy, fx - 15.0f * 0.8f, fy + 15.0f * (0.42f + fl * 0.04f), 11);
    }
}

// Kribensis (Pelvicachromis pulcher, female): slender cichlid with a pointed
// head; broad dark stripe from the snout through the eye to the tail base and a
// thin one under the dorsal; cherry belly patch, yellow cheeks and throat; long
// dorsal edged red with black ocelli at the rear and a trailing tip; upper tail
// lobe spotted and red-edged, lower lobe plain; violet pelvic fins edged blue.
static uint8_t krib_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    uint8_t v = tt < 0.1f ? 1 : (tt < 0.3f ? 2 : (tt < 0.72f ? 3 : 4));
    const float ls = 0.5f - (u > 0.62f ? (u - 0.62f) * 0.6f : 0.0f); // main stripe, rising to the eye
    const float lw = 0.12f - (u > 0.85f ? (u - 0.85f) * 0.3f : 0.0f);
    const float du = (u - 0.55f) / 0.17f, dt = (tt - 0.84f) / 0.3f;
    if (du * du + dt * dt < 1.0f && tt > 0.6f) v = 6;            // cherry belly
    if (u > 0.74f && tt > ls + lw) v = 9;                        // yellow cheek and throat
    if (u > 0.84f && tt < ls - lw && v != 1) v = 2;              // dark crown
    if (fm_abs(tt - ls) < lw && u < 0.98f) v = 5;
    if (fm_abs(tt - 0.21f) < 0.06f && u > 0.04f && u < 0.8f) v = 5; // thin back stripe
    return v;
}

static void krib_generate(SpriteSheet *ss)
{
    const int W = 34, H = 19;
    const float cy = 9.2f, x0 = 8.0f, x1 = 33.6f;
    static const Knot K[] = {{0, 1.6f, 1.6f}, {0.2f, 2.9f, 2.8f}, {0.5f, 3.8f, 4.1f},
                             {0.75f, 3.5f, 3.6f}, {0.9f, 2.4f, 2.3f}, {1.0f, 0.6f, 0.9f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 1.0f;
        const float tup[] = {x0 + 0.5f, cy - 1.9f, 3.2f, cy - 4.4f + o, 0.6f, cy - 2.6f + o, 0.0f, cy + o, x0 + 0.5f, cy};
        poly_fill(ss, fr, tup, NP(tup), (FinStyle){7, 0, 0, 0, 0, 11, 1 << 1});
        const float tlo[] = {x0 + 0.5f, cy, 0.0f, cy + o, 0.6f, cy + 2.6f + o, 3.2f, cy + 4.2f + o, x0 + 0.5f, cy + 1.9f};
        poly_fill(ss, fr, tlo, NP(tlo), (FinStyle){15, 0, 0, 0, 0, 0, 0});
        dput(ss, fr, 3.2f, cy - 2.8f + o * 0.8f, 12); // tail spots, upper lobe
        dput(ss, fr, 5.4f, cy - 2.4f + o * 0.6f, 12);
        const float dors[] = {XU(0.8f), cy - 3.6f, XU(0.7f), cy - 5.4f, XU(0.22f), cy - 6.4f,
                              XU(-0.1f), cy - 7.0f + fl * 0.4f, XU(-0.02f), cy - 3.4f, XU(0.04f), cy - 1.9f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 14, 1, 0, 3, 11, (1 << 0) | (1 << 1) | (1 << 2)});
        dput(ss, fr, XU(0.22f), cy - 5.0f, 12); // dorsal ocelli
        dput(ss, fr, XU(0.1f), cy - 5.4f, 12);
        dput(ss, fr, XU(-0.02f), cy - 5.8f, 12);
        const float anal[] = {XU(0.32f), cy + 4.0f, XU(0.12f), cy + 6.6f, XU(-0.12f), cy + 6.0f - fl * 0.4f, XU(0.0f), cy + 1.9f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 0, 0, 0, 0, 10, 1 << 1});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, krib_px);
        const float pect[] = {XU(0.72f), cy + 0.8f, XU(0.58f), cy + 2.6f + fl * 0.3f, XU(0.62f), cy + 0.8f};
        poly_fill(ss, fr, pect, NP(pect), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        const float pelv[] = {XU(0.74f), cy + 3.4f, XU(0.66f), cy + 3.9f, XU(0.48f), cy + 8.9f + fl * 0.3f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){13, 0, 0, 0, 0, 10, 1 << 1});
        fish_eye(ss, fr, XU(0.88f), cy - 1.2f, 8, 14, true);
        dput(ss, fr, XU(1.0f) - 0.4f, cy + 0.2f, 1); // mouth
    }
}

// Mbuna (Malawi rock cichlid): robust body with a steep rounded forehead and
// thick lips, irregular dark vertical bars, long dorsal from the nape to past
// the tail base with a contrasting edge, truncate tail, egg spots on the anal
// fin, long pointed pelvic fins.
static void mbuna_generate(SpriteSheet *ss)
{
    const int W = 30, H = 20;
    const float cy = 9.0f, x0 = 6.0f, x1 = 29.4f;
    static const Knot K[] = {{0, 2.6f, 2.6f}, {0.25f, 4.4f, 4.0f}, {0.55f, 5.2f, 4.6f},
                             {0.78f, 4.8f, 4.0f}, {0.92f, 3.2f, 2.6f}, {1.0f, 1.2f, 0.8f}};
    static const float BARS[6] = {0.1f, 0.22f, 0.34f, 0.46f, 0.58f, 0.7f};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 1.0f;
        const float tail[] = {x0 + 0.5f, cy - 2.6f, 1.2f, cy - 5.2f + o, 0.3f, cy - 4.6f + o, 1.4f, cy + o,
                              0.3f, cy + 4.6f + o, 1.2f, cy + 5.2f + o, x0 + 0.5f, cy + 2.6f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 14, 0, 1, 2, 10, (1 << 1) | (1 << 4)});
        const float dors[] = {XU(0.8f), cy - 4.6f, XU(0.66f), cy - 7.6f, XU(0.2f), cy - 7.8f,
                              XU(-0.1f), cy - 6.6f + fl * 0.4f, XU(0.0f), cy - 2.6f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 0, 0, 0, 0, 10, (1 << 0) | (1 << 1) | (1 << 2)});
        const float anal[] = {XU(0.3f), cy + 3.9f, XU(0.06f), cy + 7.2f, XU(-0.08f), cy + 6.4f - fl * 0.4f, XU(0.0f), cy + 2.4f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 0, 0, 0, 0, 10, (1 << 1)});
        dput(ss, fr, XU(0.1f), cy + 4.6f, 12); // egg spots
        dput(ss, fr, XU(0.18f), cy + 4.4f, 12);
        for (int x = 0; x < ss->w; x++) {
            const float u = (((float)x + 0.5f) / s_k - x0) / (x1 - x0);
            if (u < 0.0f || u > 1.0f) continue;
            float top, bot;
            prof_at(K, NK(K), u, &top, &bot);
            for (int y = 0; y < ss->h; y++) {
                const float py = ((float)y + 0.5f) / s_k;
                if (py < cy - top || py > cy + bot) continue;
                const float tt = (py - (cy - top)) / (top + bot);
                uint8_t v = tt < 0.1f ? 1 : (tt < 0.35f ? 2 : (tt < 0.75f ? 3 : 4));
                const float bu = u + (py - cy) * 0.006f;
                for (int b = 0; b < 6; b++)
                    if (fm_abs(bu - BARS[b]) < 0.03f + 0.012f * fm_sin(py * 1.7f + (float)b) && tt < 0.88f) v = 5;
                const float oc = 0.76f - 0.08f * (tt - 0.55f) * (tt - 0.55f) * 4.0f; // opercle arc
                if (fm_abs(u - oc) < 0.025f && tt > 0.3f && tt < 0.85f) v = 6;
                sheet_put(ss, fr, x, y, v);
            }
        }
        const int ex = ox(XU(0.86f)), ey = ox(cy - 2.0f);
        sheet_put(ss, fr, ex, ey, 8);
        sheet_put(ss, fr, ex - 1, ey, 9);
        sheet_put(ss, fr, ex, ey + 1, 9);
        dput(ss, fr, W - 1, cy, 1); // thick lips
        dput(ss, fr, W - 2, cy + 1, 1);
        const float pect[] = {XU(0.74f), cy + 0.8f, XU(0.58f), cy + 3.0f + fl * 0.3f, XU(0.62f), cy + 0.8f};
        poly_fill(ss, fr, pect, NP(pect), (FinStyle){14, 0, 0, 0, 0, 0, 0});
        const float pelv[] = {XU(0.74f), cy + 3.8f, XU(0.66f), cy + 4.2f, XU(0.52f), cy + 9.8f + fl * 0.3f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){13, 0, 0, 0, 0, 10, 1 << 2});
    }
}

// Kuhli loach (Pangio kuhlii): eel-like body that wriggles (a travelling wave
// along the body across the frames), orange flanks with wide dark saddles that
// taper towards the pale belly, dark patches on the head, small dorsal and anal
// fins set far back, rounded tail, barbels.
static void kuhli_generate(SpriteSheet *ss)
{
    const int W = 27, H = 9;
    const float cy = 4.5f, x0 = 2.6f, x1 = 25.6f;
    static const Knot K[] = {{0, 0.9f, 0.9f}, {0.12f, 1.5f, 1.5f}, {0.5f, 1.8f, 1.8f},
                             {0.86f, 1.7f, 1.6f}, {0.95f, 1.3f, 1.2f}, {1.0f, 0.7f, 0.6f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float ph = (float)fr * FM_PI * 0.5f;
#define KW(u) (cy + 1.1f * fm_sin((u) * FM_PI * 2.0f - ph) * (1.0f - 0.6f * (u))) // wave travelling to the tail
        const float c0 = KW(0.0f);
        const float tail[] = {x0 + 0.5f, c0 - 0.7f, 1.2f, c0 - 1.4f, 0.4f, c0, 1.2f, c0 + 1.4f, x0 + 0.5f, c0 + 0.7f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        const float dors[] = {XU(0.28f), KW(0.28f) - 1.5f, XU(0.21f), KW(0.21f) - 2.5f, XU(0.17f), KW(0.17f) - 1.3f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        const float anal[] = {XU(0.24f), KW(0.24f) + 1.5f, XU(0.17f), KW(0.17f) + 2.4f, XU(0.13f), KW(0.13f) + 1.2f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        for (int x = 0; x < ss->w; x++) {
            const float u = (((float)x + 0.5f) / s_k - x0) / (x1 - x0);
            if (u < 0.0f || u > 1.0f) continue;
            float top, bot;
            prof_at(K, NK(K), u, &top, &bot);
            const float c = KW(u);
            for (int y = 0; y < ss->h; y++) {
                const float py = ((float)y + 0.5f) / s_k;
                if (py < c - top || py > c + bot) continue;
                const float tt = (py - (c - top)) / (top + bot);
                uint8_t v = tt < 0.12f ? 1 : (tt < 0.45f ? 2 : (tt < 0.8f ? 3 : 4));
                // 9 saddles, wider than the gaps on the back, pointed towards the belly
                const float ph2 = (u - 0.05f) / 0.097f;
                const float dd = fm_abs(ph2 - floorf(ph2 + 0.5f)) * 0.097f;
                if (u < 0.86f && tt < 0.82f && dd < 0.032f * (1.0f - tt * tt / 0.75f) + 0.006f) v = 5;
                if (u > 0.9f && tt < 0.6f && (u < 0.945f || u > 0.975f)) v = 5; // head patches
                sheet_put(ss, fr, x, y, v);
            }
        }
        sheet_put(ss, fr, ox(XU(0.955f)), ox(KW(0.955f) - 0.4f), 8); // eye
        dline(ss, fr, XU(1.0f) - 0.3f, KW(1.0f) + 0.4f, XU(1.0f) + 0.9f, KW(1.0f) + 1.4f, 7); // barbels
        dline(ss, fr, XU(0.97f), KW(0.97f) + 1.0f, XU(0.98f), KW(0.98f) + 2.0f, 7);
        dput(ss, fr, XU(0.86f), KW(0.86f) + 1.8f, 7); // pectoral fin
#undef KW
    }
}
// Cardinal tetra (Paracheirodon axelrodi): slim torpedo, iridescent blue line
// from the eye to the adipose fin, red along the whole lower half up to the tail
// base, small white belly behind the head, glassy fins, big eye ringed blue.
static uint8_t cardinal_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    if (tt < 0.12f) return 1;
    const float line = 0.36f - (u > 0.75f ? (u - 0.75f) * 0.5f : 0.0f); // rises slightly to the eye
    if (u > 0.1f && u < 0.9f && fm_abs(tt - line) < 0.13f) return tt < line - 0.04f ? 4 : 3;
    if (tt < line) return 2;
    if ((u > 0.5f && u < 0.84f && tt > 0.8f) || u > 0.93f) return 9; // white belly, silver chin
    return tt > 0.86f ? 6 : 5;
}

static void cardinal_generate(SpriteSheet *ss)
{
    const int W = 15, H = 9;
    const float cy = 4.4f, x0 = 4.0f, x1 = 14.7f;
    static const Knot K[] = {{0, 0.7f, 0.7f}, {0.15f, 1.1f, 1.1f}, {0.45f, 1.9f, 2.1f},
                             {0.7f, 1.9f, 2.0f}, {0.88f, 1.4f, 1.5f}, {1.0f, 0.4f, 0.5f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 0.6f;
        const float tail[] = {x0 + 0.5f, cy - 0.6f, 1.0f, cy - 3.0f + o, 2.4f, cy + o * 0.5f, 1.0f, cy + 3.0f + o, x0 + 0.5f, cy + 0.6f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 12, 0, 1, 2, 0, 0});
        const float base[] = {x0 + 0.6f, cy - 0.8f, x0 - 1.4f, cy - 0.9f + o * 0.3f, x0 - 1.4f, cy + 0.9f + o * 0.3f, x0 + 0.6f, cy + 0.8f};
        poly_fill(ss, fr, base, NP(base), (FinStyle){11, 0, 0, 0, 0, 0, 0}); // red tail base
        const float dors[] = {XU(0.56f), cy - 1.6f, XU(0.46f), cy - 4.1f, XU(0.34f), cy - 3.6f + fl * 0.2f, XU(0.4f), cy - 1.5f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 0, 0, 0, 0, 12, 1});
        dput(ss, fr, XU(0.1f), cy - 1.6f, 7); // adipose fin
        const float anal[] = {XU(0.46f), cy + 1.9f, XU(0.32f), cy + 3.7f, XU(0.06f), cy + 3.1f - fl * 0.2f, XU(0.1f), cy + 1.0f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 12, 1, 0, 2, 0, 0});
        const float pelv[] = {XU(0.66f), cy + 1.6f, XU(0.52f), cy + 3.0f + fl * 0.3f, XU(0.6f), cy + 1.7f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, cardinal_px);
        fish_eye(ss, fr, XU(0.86f), cy - 0.5f, 8, 10, false);
    }
}

// Ember tetra (Hyphessobrycon amandae): small, fairly deep glassy orange-red body
// with the spine showing through, tall dorsal with a dark leading edge, long
// anal fin with a pale leading edge, forked tail, big red-ringed eye.
static uint8_t ember_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    uint8_t v = tt < 0.1f ? 1 : (tt < 0.36f ? 2 : (tt < 0.74f ? 3 : 4));
    if (u > 0.06f && u < 0.78f && fm_abs(tt - 0.42f) < 0.07f) v = 5; // spine through the glassy body
    return v;
}

static void ember_generate(SpriteSheet *ss)
{
    const int W = 12, H = 10;
    const float cy = 5.0f, x0 = 3.4f, x1 = 11.7f;
    static const Knot K[] = {{0, 0.7f, 0.7f}, {0.2f, 1.4f, 1.5f}, {0.5f, 2.3f, 2.5f},
                             {0.72f, 2.2f, 2.2f}, {0.88f, 1.6f, 1.5f}, {1.0f, 0.5f, 0.6f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 0.6f;
        const float tail[] = {x0 + 0.5f, cy - 0.6f, 0.8f, cy - 3.1f + o, 2.0f, cy + o * 0.5f, 0.8f, cy + 3.1f + o, x0 + 0.5f, cy + 0.6f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        const float dors[] = {XU(0.58f), cy - 2.2f, XU(0.5f), cy - 4.6f, XU(0.36f), cy - 4.0f + fl * 0.2f, XU(0.4f), cy - 2.1f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 0, 0, 0, 0, 10, 1});
        dput(ss, fr, XU(0.1f), cy - 1.4f, 7); // adipose fin
        const float anal[] = {XU(0.5f), cy + 2.4f, XU(0.34f), cy + 4.7f, XU(0.02f), cy + 3.4f - fl * 0.2f, XU(0.06f), cy + 1.2f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 0, 0, 0, 0, 11, 1});
        const float pelv[] = {XU(0.66f), cy + 2.2f, XU(0.54f), cy + 3.8f + fl * 0.3f, XU(0.6f), cy + 2.2f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, ember_px);
        fish_eye(ss, fr, XU(0.85f), cy - 0.6f, 8, 9, false);
    }
}

// Harlequin rasbora (Trigonostigma heteromorpha): deep copper-orange body, black
// wedge with a vertical front edge under the dorsal tapering to a point at the
// tail base, a gold line along its top edge, dark mark behind the gill, red-edged
// forked tail, pelvic fins with a black leading edge.
static uint8_t harlequin_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    uint8_t v = tt < 0.1f ? 1 : (tt < 0.34f ? 2 : (tt < 0.72f ? 3 : 4));
    if (u > 0.02f && u < 0.57f) {
        const float k = fm_clamp((u - 0.02f) / 0.54f, 0.0f, 1.0f);
        const float half = 0.04f + 0.3f * k, wc = 0.52f + 0.02f * k;
        if (fm_abs(tt - wc) < half) return 5;
        if (u > 0.12f && tt < wc - half && tt > wc - half - 0.14f) return 6; // gold line above the wedge
    }
    if (u > 0.75f && u < 0.8f && tt > 0.3f && tt < 0.7f) return 5; // gill mark
    return v;
}

static void harlequin_generate(SpriteSheet *ss)
{
    const int W = 13, H = 10;
    const float cy = 4.9f, x0 = 3.6f, x1 = 12.7f;
    static const Knot K[] = {{0, 0.8f, 0.8f}, {0.15f, 1.3f, 1.2f}, {0.45f, 2.5f, 2.6f},
                             {0.7f, 2.4f, 2.3f}, {0.88f, 1.7f, 1.5f}, {1.0f, 0.5f, 0.5f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 0.6f;
        const float tail[] = {x0 + 0.5f, cy - 0.7f, 0.8f, cy - 3.3f + o, 2.2f, cy + o * 0.5f, 0.8f, cy + 3.3f + o, x0 + 0.5f, cy + 0.7f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 0, 0, 0, 0, 10, (1 << 0) | (1 << 3)});
        const float dors[] = {XU(0.6f), cy - 2.5f, XU(0.52f), cy - 4.8f, XU(0.38f), cy - 4.2f + fl * 0.2f, XU(0.42f), cy - 2.3f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 0, 0, 0, 0, 10, 1 << 1});
        const float anal[] = {XU(0.5f), cy + 2.5f, XU(0.38f), cy + 4.4f, XU(0.2f), cy + 3.8f - fl * 0.2f, XU(0.24f), cy + 1.9f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        const float pelv[] = {XU(0.66f), cy + 2.2f, XU(0.56f), cy + 4.1f + fl * 0.3f, XU(0.62f), cy + 2.2f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){7, 0, 0, 0, 0, 5, 1});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, harlequin_px);
        fish_eye(ss, fr, XU(0.86f), cy - 0.5f, 8, 9, false);
    }
}

// Congo tetra (Phenacogrammus interruptus, male): long deep body, blue back, a
// broad orange band edged gold from the head to the rear body, silver-turquoise
// scales below, violet peduncle; tail with a feathery middle filament, long
// trailing dorsal and anal fins with white edges.
static uint8_t congo_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    if (tt < 0.08f) return 1;
    if (u < 0.12f) return tt > 0.85f ? 4 : 12;                       // violet peduncle
    if (u > 0.86f) return tt < 0.42f ? 5 : (tt < 0.8f ? 3 : 4);      // orange forehead, silver cheek
    if (tt < 0.28f) return 2;                                        // blue back
    if (u > 0.2f && tt < 0.34f) return 6;                            // gold edge of the band
    if (u > 0.2f && tt < 0.5f) return 5;                             // orange band
    if (tt < 0.84f) return PEARL(b->x, b->y) ? 11 : 3;               // glittering scales
    return 4;
}

static void congo_generate(SpriteSheet *ss)
{
    const int W = 18, H = 13;
    const float cy = 6.4f, x0 = 5.0f, x1 = 17.7f;
    static const Knot K[] = {{0, 0.9f, 0.9f}, {0.15f, 1.6f, 1.6f}, {0.45f, 2.7f, 2.8f},
                             {0.72f, 2.6f, 2.5f}, {0.9f, 1.9f, 1.7f}, {1.0f, 0.6f, 0.6f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 0.8f;
        const float tail[] = {x0 + 0.5f, cy - 0.9f, 1.8f, cy - 4.4f + o, 0.3f, cy - 3.8f + o, 1.8f, cy - 0.8f + o * 0.6f,
                              0.0f, cy + o * 0.6f, 1.8f, cy + 0.8f + o * 0.6f, 0.3f, cy + 3.8f + o, 1.8f, cy + 4.4f + o,
                              x0 + 0.5f, cy + 0.9f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 0, 0, 0, 0, 10, (1 << 0) | (1 << 3) | (1 << 4) | (1 << 7)});
        const float dors[] = {XU(0.58f), cy - 2.7f, XU(0.5f), cy - 5.0f, XU(0.22f), cy - 5.4f + fl * 0.3f, XU(0.38f), cy - 2.6f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 0, 0, 0, 0, 10, (1 << 0) | (1 << 1)});
        const float anal[] = {XU(0.54f), cy + 2.6f, XU(0.4f), cy + 4.6f, XU(0.0f), cy + 5.0f - fl * 0.3f, XU(0.08f), cy + 1.5f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 0, 0, 0, 0, 10, 1 << 1});
        const float pelv[] = {XU(0.66f), cy + 2.4f, XU(0.54f), cy + 4.4f + fl * 0.3f, XU(0.6f), cy + 2.4f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, congo_px);
        fish_eye(ss, fr, XU(0.87f), cy - 0.6f, 8, 9, false);
    }
}

// Peacock cichlid (Aulonocara): robust Malawi cichlid with a sloping forehead,
// shimmering scales and faint dark bars, a differently coloured head with a dark
// opercle line, long dorsal edged pale, dark anal fin edged yellow, long pelvic
// fins, rounded tail, large amber eye.
static uint8_t peacock_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    uint8_t v = tt < 0.08f ? 1 : (tt < 0.3f ? 2 : (tt < 0.74f ? 3 : 4));
    if (v == 1) return v;
    if (u > 0.74f) { // head
        const float oc = 0.76f - 0.32f * (tt - 0.55f) * (tt - 0.55f); // opercle arc
        if (fm_abs(u - oc) < 0.02f && tt > 0.3f && tt < 0.9f) return 14;
        return (u > 0.96f && tt > 0.45f) ? 14 : 9;                   // lips
    }
    const float bu = u + (tt - 0.5f) * 0.03f;
    for (int k = 0; k < 7; k++)
        if (fm_abs(bu - (0.1f + 0.09f * (float)k)) < 0.022f && tt > 0.12f && tt < 0.86f) return 5;
    if (tt > 0.15f && tt < 0.85f && ((b->x + 2 * b->y) % 5) == 0) v = 6; // scale shimmer
    return v;
}

static void peacock_generate(SpriteSheet *ss)
{
    const int W = 27, H = 18;
    const float cy = 8.2f, x0 = 6.0f, x1 = 26.6f;
    static const Knot K[] = {{0, 1.9f, 1.9f}, {0.2f, 3.1f, 3.0f}, {0.5f, 4.3f, 3.9f},
                             {0.74f, 4.1f, 3.6f}, {0.9f, 3.0f, 2.5f}, {1.0f, 1.0f, 0.9f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 1.0f;
        const float tail[] = {x0 + 0.5f, cy - 1.9f, 1.4f, cy - 4.7f + o, 0.2f, cy - 3.8f + o, 0.5f, cy + o,
                              0.2f, cy + 3.8f + o, 1.4f, cy + 4.7f + o, x0 + 0.5f, cy + 1.9f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 12, 0, 1, 3, 10, 1 << 0});
        const float dors[] = {XU(0.78f), cy - 4.1f, XU(0.64f), cy - 7.3f, XU(0.2f), cy - 7.7f,
                              XU(-0.14f), cy - 6.2f + fl * 0.4f, XU(0.0f), cy - 1.9f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 12, 1, 0, 3, 10, (1 << 0) | (1 << 1) | (1 << 2)});
        const float anal[] = {XU(0.36f), cy + 3.7f, XU(0.14f), cy + 7.0f, XU(-0.1f), cy + 6.4f - fl * 0.4f, XU(0.0f), cy + 1.9f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){12, 0, 0, 0, 0, 13, (1 << 1) | (1 << 2)});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, peacock_px);
        const float pect[] = {XU(0.72f), cy + 0.8f, XU(0.56f), cy + 3.0f + fl * 0.3f, XU(0.6f), cy + 0.8f};
        poly_fill(ss, fr, pect, NP(pect), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        const float pelv[] = {XU(0.68f), cy + 3.4f, XU(0.6f), cy + 3.7f, XU(0.44f), cy + 8.6f + fl * 0.3f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){12, 0, 0, 0, 0, 10, 1 << 2});
        fish_eye(ss, fr, XU(0.85f), cy - 1.5f, 8, 11, true);
    }
}

// Goldfish (Carassius auratus, veiltail): deep egg-shaped body with a blunt
// head, scale sparkle, tall dorsal, long flowing double tail that waves between
// frames, long pelvic fins, gold-ringed eye. Index 12-14 = colour patches (red
// on the red & white variant, same as the body on the plain orange one).
static uint8_t goldfish_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    uint8_t v = tt < 0.08f ? 1 : (tt < 0.3f ? 2 : (tt < 0.75f ? 3 : 4));
    if (v == 1) return v;
    const bool patch = (u > 0.78f && tt < 0.62f) ||                                       // red cap
                       (u > 0.28f && u < 0.62f && tt < 0.5f + 0.12f * fm_sin(u * 19.0f)); // saddle
    const bool sparkle = (v == 2 || v == 3) && u < 0.82f && PEARL(b->x, b->y);
    if (patch) return sparkle ? 13 : (v == 2 ? 14 : 12);
    return sparkle ? 5 : v;
}

static void goldfish_generate(SpriteSheet *ss)
{
    const int W = 26, H = 21;
    const float cy = 9.8f, x0 = 10.0f, x1 = 25.6f;
    static const Knot K[] = {{0, 1.6f, 1.6f}, {0.2f, 3.4f, 3.6f}, {0.45f, 4.8f, 5.0f},
                             {0.7f, 4.5f, 4.4f}, {0.88f, 3.3f, 3.0f}, {1.0f, 1.4f, 1.0f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 1.0f;
        const float tail[] = {x0 + 0.5f, cy - 1.5f, 5.6f, cy - 4.6f + o * 0.5f, 1.0f, cy - 8.4f + o * 1.2f,
                              0.2f, cy - 6.6f + o * 1.4f, 2.4f, cy - 3.6f + o * 1.1f, 3.6f, cy + o * 0.8f,
                              2.4f, cy + 3.6f + o * 1.1f, 0.2f, cy + 6.6f + o * 1.4f, 1.0f, cy + 8.4f + o * 1.2f,
                              5.6f, cy + 4.6f + o * 0.5f, x0 + 0.5f, cy + 1.5f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 6, 1, 1, 3, 10, (1 << 1) | (1 << 2) | (1 << 7) | (1 << 8)});
        const float dors[] = {XU(0.66f), cy - 4.6f, XU(0.54f), cy - 9.4f, XU(0.3f), cy - 9.0f + fl * 0.3f, XU(0.06f), cy - 3.0f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 6, 1, 0, 3, 10, 1 << 1});
        const float anal[] = {XU(0.26f), cy + 4.2f, XU(0.08f), cy + 7.8f, XU(-0.12f), cy + 7.4f - fl * 0.4f, XU(0.0f), cy + 2.4f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 6, 1, 0, 3, 0, 0});
        const float pelv[] = {XU(0.66f), cy + 4.4f, XU(0.58f), cy + 4.8f, XU(0.48f), cy + 10.4f + fl * 0.3f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){7, 0, 0, 0, 0, 10, 1 << 1});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, goldfish_px);
        const float pect[] = {XU(0.8f), cy + 1.2f, XU(0.62f), cy + 4.0f + fl * 0.3f, XU(0.68f), cy + 1.2f};
        poly_fill(ss, fr, pect, NP(pect), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        fish_eye(ss, fr, XU(0.86f), cy - 1.0f, 8, 9, true);
        dput(ss, fr, XU(1.0f) - 0.4f, cy + 0.2f, 1); // mouth
    }
}

// Koi (Cyprinus rubrofuscus): long heavy torpedo, scale net, big red patches on
// the back plus a red head cap (kohaku), black sumi spots (sanke), large forked
// tail and long dorsal with fine rays, big pectoral fins, barbels.
static uint8_t koi_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    const uint8_t v = tt < 0.08f ? 1 : (tt < 0.3f ? 2 : (tt < 0.78f ? 3 : 4));
    if (v == 1) return v;
    const bool scale = PEARL(b->x, b->y) && tt < 0.8f && u < 0.86f;
    const float edge = 0.6f + 0.14f * fm_sin(u * 13.0f + 1.0f);
    const bool patch = (u > 0.87f && tt < 0.5f) ||
                       (u > 0.1f && u < 0.83f && tt < edge && fm_sin(u * 17.0f - 1.2f) + 0.4f * fm_sin(u * 29.0f + tt * 5.0f) > 0.1f);
    if (patch) return scale ? 12 : 5;
    if (u > 0.2f && u < 0.8f && tt > 0.15f && tt < 0.55f && fm_sin(u * 41.0f + 0.5f) > 0.7f && fm_sin(tt * 15.0f + u * 9.0f) > 0.3f)
        return 6; // sumi
    return scale ? 9 : v;
}

static void koi_generate(SpriteSheet *ss)
{
    const int W = 34, H = 16;
    const float cy = 7.4f, x0 = 10.0f, x1 = 33.6f;
    static const Knot K[] = {{0, 1.3f, 1.3f}, {0.2f, 2.6f, 2.4f}, {0.5f, 3.6f, 3.3f},
                             {0.75f, 3.4f, 3.0f}, {0.9f, 2.6f, 2.3f}, {1.0f, 1.3f, 1.2f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 0.8f;
        const float tail[] = {x0 + 0.5f, cy - 1.3f, 5.0f, cy - 3.2f + o * 0.5f, 0.6f, cy - 5.2f + o * 1.2f, 3.0f, cy + o * 0.8f,
                              0.6f, cy + 5.2f + o * 1.2f, 5.0f, cy + 3.2f + o * 0.5f, x0 + 0.5f, cy + 1.3f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 10, 1, 2, 4, 0, 0});
        const float dors[] = {XU(0.62f), cy - 3.5f, XU(0.54f), cy - 6.4f, XU(0.2f), cy - 5.2f + fl * 0.3f, XU(0.1f), cy - 2.2f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 10, 1, 0, 3, 0, 0});
        const float anal[] = {XU(0.22f), cy + 2.3f, XU(0.12f), cy + 4.8f - fl * 0.3f, XU(0.04f), cy + 1.6f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        const float pelv[] = {XU(0.5f), cy + 3.0f, XU(0.38f), cy + 5.8f + fl * 0.3f, XU(0.46f), cy + 3.0f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, koi_px);
        const float pect[] = {XU(0.8f), cy + 1.4f, XU(0.62f), cy + 4.6f + fl * 0.4f, XU(0.72f), cy + 5.2f + fl * 0.4f, XU(0.78f), cy + 2.6f};
        poly_fill(ss, fr, pect, NP(pect), (FinStyle){7, 10, 1, 0, 3, 0, 0});
        dline(ss, fr, XU(0.99f), cy + 0.9f, XU(1.0f) + 0.2f, cy + 2.6f, 11); // barbels
        dline(ss, fr, XU(0.95f), cy + 1.4f, XU(0.93f), cy + 2.8f, 11);
        dput(ss, fr, XU(1.0f) - 0.3f, cy + 0.6f, 11); // lips
        fish_eye(ss, fr, XU(0.88f), cy - 0.4f, 8, 0, false);
    }
}

// Medaka (Oryzias latipes): small, straight flat back, upturned mouth, big
// silver-ringed eye, scale net on the back, silver lateral line, small dorsal
// set far back, long rayed anal fin, broad truncate rayed tail.
static uint8_t medaka_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    uint8_t v = tt < 0.1f ? 1 : (tt < 0.4f ? 2 : (tt < 0.74f ? 3 : 4));
    if (v == 2 && u < 0.85f && PEARL(b->x, b->y)) v = 5;                 // scale net
    if (u > 0.08f && u < 0.7f && fm_abs(tt - 0.5f) < 0.07f) v = 6;         // lateral line
    if (u > 0.7f && u < 0.78f && tt > 0.4f && tt < 0.85f) v = 11;          // gill shine
    return v;
}

static void medaka_generate(SpriteSheet *ss)
{
    const int W = 12, H = 8;
    const float cy = 3.8f, x0 = 3.2f, x1 = 11.8f;
    static const Knot K[] = {{0, 0.8f, 0.8f}, {0.2f, 1.3f, 1.2f}, {0.5f, 1.5f, 1.9f},
                             {0.75f, 1.5f, 1.8f}, {0.9f, 1.3f, 1.3f}, {1.0f, 0.9f, 0.3f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 0.6f;
        const float tail[] = {x0 + 0.5f, cy - 0.7f, 0.4f, cy - 2.6f + o, 0.1f, cy + o * 0.5f, 0.4f, cy + 2.6f + o, x0 + 0.5f, cy + 0.7f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 10, 0, 1, 2, 0, 0});
        const float dors[] = {XU(0.26f), cy - 1.2f, XU(0.2f), cy - 3.2f, XU(0.06f), cy - 2.6f + fl * 0.2f, XU(0.08f), cy - 0.9f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 10, 1, 0, 2, 0, 0});
        const float anal[] = {XU(0.56f), cy + 1.7f, XU(0.46f), cy + 3.7f, XU(0.04f), cy + 3.0f - fl * 0.2f, XU(0.04f), cy + 0.9f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 10, 1, 0, 2, 0, 0});
        const float pelv[] = {XU(0.7f), cy + 1.6f, XU(0.62f), cy + 3.4f + fl * 0.3f, XU(0.66f), cy + 1.6f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, medaka_px);
        fish_eye(ss, fr, XU(0.86f), cy - 0.3f, 8, 9, true);
    }
}
// Betta splendens (male): slender body with an upturned mouth, huge flowing
// fins - a round fan tail, a long dorsal set back over the peduncle and a deep
// anal fin running from the belly to the tail - all with rays radiating from the
// base and short spikes past the membrane (crowntail; on the koi variant the
// spikes share the dark fin edge colour, so they read as a ragged margin).
// Long thin pelvic streamers. Body: scale sparkle everywhere, a colour patch on
// the rear half (blue on the koi), a distinct head colour and a dark opercle.
static uint8_t betta_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    uint8_t v = tt < 0.08f ? 1 : (tt < 0.35f ? 2 : (tt < 0.78f ? 3 : 4));
    if (v == 1) return v;
    if (u > 0.76f && u < 0.8f && tt > 0.25f && tt < 0.92f) return 15;              // opercle
    if (u > 0.78f) return v == 4 ? 15 : 6;                                          // head, throat
    const bool patch = u < 0.42f + 0.1f * fm_sin(tt * 7.0f + 1.0f) && tt < 0.86f; // rear patch
    if (u < 0.74f && PEARL(b->x, b->y)) return 5;                                  // scales
    return patch ? 12 : v;
}

// Fan of rays from (bx, by) to each point of pts, then a spike continuing each
// ray `spike` design px past the edge.
static void betta_rays(const SpriteSheet *ss, int fr, float bx, float by, const float *pts, int n, float spike)
{
    for (int i = 0; i < n; i++) {
        const float px = pts[2 * i], py = pts[2 * i + 1];
        const float dx = px - bx, dy = py - by, l = sqrtf(dx * dx + dy * dy);
        if (l < 0.5f) continue;
        dline(ss, fr, bx + dx * 0.25f, by + dy * 0.25f, px - dx / l * 0.6f, py - dy / l * 0.6f, 11);
        dline(ss, fr, px, py, px + dx / l * spike, py + dy / l * spike, 14);
    }
}

static void betta_generate(SpriteSheet *ss)
{
    const int W = 31, H = 25;
    const float cy = 10.6f, x0 = 11.5f, x1 = 30.6f;
    static const Knot K[] = {{0, 1.6f, 1.6f}, {0.25f, 2.9f, 2.9f}, {0.5f, 3.6f, 3.5f},
                             {0.72f, 3.6f, 3.2f}, {0.88f, 2.8f, 2.4f}, {1.0f, 1.3f, 0.6f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr];
        // round fan tail: arc around the peduncle, drooping a little, tips wave most
        float tail[2 * 13], arc[2 * 11];
        const float tx = x0 + 0.4f, ty = cy + 0.8f;
        tail[0] = x0 + 0.5f, tail[1] = cy - 1.2f;
        for (int i = 0; i < 11; i++) {
            const float a = FM_PI * (0.58f + 0.84f * (float)i / 10.0f); // upper-left .. lower-left
            const float r = 10.2f + 0.6f * fm_sin((float)i * 1.7f);
            const float px = tx + r * fm_cos(a), py = ty - r * fm_sin(a) + fl * 1.3f * (1.0f - fm_abs((float)i - 5.0f) / 8.0f);
            arc[2 * i] = px, arc[2 * i + 1] = py;
            tail[2 + 2 * i] = px, tail[3 + 2 * i] = py;
        }
        tail[24] = x0 + 0.5f, tail[25] = cy + 1.2f;
        poly_fill(ss, fr, tail, 13, (FinStyle){7, 0, 0, 0, 0, 10, 0x7FE}); // edge along the arc
        betta_rays(ss, fr, tx, ty, arc, 11, 1.4f);
        // long dorsal swept back over the tail
        const float dors[] = {XU(0.58f), cy - 2.9f, XU(0.42f), cy - 7.6f, XU(0.12f), cy - 9.4f + fl * 0.5f,
                              x0 - 3.6f, cy - 8.0f + fl * 0.8f, x0 - 2.0f, cy - 4.2f + fl * 0.6f, XU(0.04f), cy - 1.3f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 0, 0, 0, 0, 10, (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4)});
        const float dtip[] = {XU(0.42f), cy - 7.6f, XU(0.26f), cy - 8.8f + fl * 0.3f, XU(0.12f), cy - 9.4f + fl * 0.5f,
                              x0 - 1.0f, cy - 9.0f + fl * 0.7f, x0 - 3.6f, cy - 8.0f + fl * 0.8f};
        betta_rays(ss, fr, XU(0.36f), cy - 2.2f, dtip, NP(dtip), 1.2f);
        // deep anal fin from the belly to under the tail
        const float anal[] = {XU(0.74f), cy + 2.5f, XU(0.56f), cy + 8.0f, XU(0.26f), cy + 11.6f - fl * 0.5f,
                              x0 - 2.4f, cy + 12.2f - fl * 0.8f, x0 - 1.4f, cy + 6.0f - fl * 0.6f, XU(0.04f), cy + 1.3f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 0, 0, 0, 0, 10, (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4)});
        const float atip[] = {XU(0.56f), cy + 8.0f, XU(0.42f), cy + 10.2f - fl * 0.3f, XU(0.26f), cy + 11.6f - fl * 0.5f,
                              XU(0.08f), cy + 12.2f - fl * 0.7f, x0 - 2.4f, cy + 12.2f - fl * 0.8f};
        betta_rays(ss, fr, XU(0.48f), cy + 2.4f, atip, NP(atip), 1.2f);
        // fin bases blend into the body colour
        const float abase[] = {XU(0.7f), cy + 2.4f, XU(0.5f), cy + 4.4f, XU(0.1f), cy + 4.2f, XU(0.04f), cy + 1.3f};
        poly_fill(ss, fr, abase, NP(abase), (FinStyle){13, 0, 0, 0, 0, 0, 0});
        const float tbase[] = {x0 + 0.5f, cy - 1.4f, x0 - 2.0f, cy - 2.4f, x0 - 3.0f, cy + 0.8f, x0 - 2.0f, cy + 3.8f, x0 + 0.5f, cy + 1.4f};
        poly_fill(ss, fr, tbase, NP(tbase), (FinStyle){13, 0, 0, 0, 0, 0, 0});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, betta_px);
        // small clear pectoral fin, long pelvic streamers
        const float pect[] = {XU(0.74f), cy + 0.4f, XU(0.6f), cy + 2.4f + fl * 0.3f, XU(0.66f), cy + 0.6f};
        poly_fill(ss, fr, pect, NP(pect), (FinStyle){13, 0, 0, 0, 0, 0, 0});
        const float vx = XU(0.72f), vy = cy + 2.4f;
        dline(ss, fr, vx, vy, vx - 3.0f, vy + 5.6f + fl * 0.3f, 7);
        dline(ss, fr, vx + 0.6f, vy, vx - 2.2f, vy + 5.8f + fl * 0.3f, 7);
        dput(ss, fr, vx - 3.0f, vy + 5.6f + fl * 0.3f, 14);
        fish_eye(ss, fr, XU(0.88f), cy - 0.6f, 8, 9, false);
        dput(ss, fr, x1 - 0.4f, cy - 0.8f, 15); // upturned mouth
    }
}

// White cloud minnow (Tanichthys albonubes): slim, straight back, iridescent
// lateral line from the gill to the tail with a warm band under its rear half,
// black spot on the peduncle, red forked tail, dorsal set far back and a long
// anal fin, both edged (red on the classic, white on the gold variant).
static uint8_t tanichthys_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    uint8_t v = tt < 0.1f ? 1 : (tt < 0.38f ? 2 : (tt < 0.74f ? 3 : 4));
    if (v == 1) return v;
    if (u < 0.07f && fm_abs(tt - 0.5f) < 0.2f) return 12;               // peduncle spot
    if (fm_abs(tt - 0.42f) < 0.08f && u > 0.06f && u < 0.84f) return 5;  // iridescent line
    if (tt > 0.5f && tt < 0.68f && u < 0.62f) return 6;                  // warm band
    return v;
}

static void tanichthys_generate(SpriteSheet *ss)
{
    const int W = 16, H = 10;
    const float cy = 4.8f, x0 = 4.2f, x1 = 15.8f;
    static const Knot K[] = {{0, 0.8f, 0.8f}, {0.2f, 1.2f, 1.2f}, {0.5f, 1.6f, 1.7f},
                             {0.75f, 1.5f, 1.6f}, {0.9f, 1.2f, 1.1f}, {1.0f, 0.6f, 0.4f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 0.6f;
        const float tail[] = {x0 + 0.5f, cy - 0.7f, 0.6f, cy - 3.2f + o, 2.0f, cy + o * 0.5f, 0.6f, cy + 3.2f + o, x0 + 0.5f, cy + 0.7f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){11, 0, 0, 0, 0, 0, 0});
        const float dors[] = {XU(0.42f), cy - 1.5f, XU(0.36f), cy - 3.3f, XU(0.2f), cy - 3.1f + fl * 0.2f, XU(0.2f), cy - 1.3f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 0, 0, 0, 0, 10, (1 << 1) | (1 << 2)});
        dput(ss, fr, XU(0.35f), cy - 3.2f, 13); // pale fin tip
        const float anal[] = {XU(0.52f), cy + 1.6f, XU(0.44f), cy + 3.2f, XU(0.14f), cy + 2.9f - fl * 0.2f, XU(0.12f), cy + 1.2f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 0, 0, 0, 0, 10, (1 << 1) | (1 << 2)});
        dput(ss, fr, XU(0.43f), cy + 3.1f, 13);
        const float pelv[] = {XU(0.7f), cy + 1.5f, XU(0.62f), cy + 3.2f + fl * 0.3f, XU(0.66f), cy + 1.5f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, tanichthys_px);
        fish_eye(ss, fr, XU(0.86f), cy - 0.4f, 8, 9, true);
        dput(ss, fr, x1 - 0.3f, cy - 0.5f, 1); // upturned mouth
    }
}

// Rummy-nose tetra (Hemigrammus bleheri): slim, translucent silver body, blood
// red head and eye, black bar on the peduncle running into the tail, a black
// patch on each tail lobe with white between them, small clear fins, adipose.
static uint8_t rummy_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    uint8_t v = tt < 0.1f ? 1 : (tt < 0.38f ? 2 : (tt < 0.75f ? 3 : 4));
    if (u > 0.76f || (u > 0.64f && tt < 0.62f - (0.76f - u) * 3.0f)) return v == 1 || tt < 0.25f ? 12 : 6; // red head
    if (v == 1) return v;
    if (u < 0.1f && fm_abs(tt - 0.5f) < 0.2f) return 10;                 // peduncle bar
    if (v == 2 && u < 0.7f && PEARL(b->x, b->y)) return 5;               // scale sheen
    return v;
}

static void rummy_generate(SpriteSheet *ss)
{
    const int W = 15, H = 9;
    const float cy = 4.4f, x0 = 4.0f, x1 = 14.8f;
    static const Knot K[] = {{0, 0.7f, 0.7f}, {0.2f, 1.1f, 1.1f}, {0.5f, 1.5f, 1.6f},
                             {0.75f, 1.5f, 1.5f}, {0.9f, 1.2f, 1.0f}, {1.0f, 0.6f, 0.4f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 0.6f;
        const float tail[] = {x0 + 0.5f, cy - 0.6f, 0.4f, cy - 3.1f + o, 2.0f, cy + o * 0.4f, 0.4f, cy + 3.1f + o, x0 + 0.5f, cy + 0.6f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        const float white[] = {x0 + 0.5f, cy - 0.7f, 1.4f, cy - 2.2f + o * 0.7f, 1.8f, cy + o * 0.4f, 1.4f, cy + 2.2f + o * 0.7f, x0 + 0.5f, cy + 0.7f};
        poly_fill(ss, fr, white, NP(white), (FinStyle){11, 0, 0, 0, 0, 0, 0});
        const float up[] = {2.9f, cy - 1.0f + o * 0.3f, 1.0f, cy - 2.5f + o * 0.8f, 0.8f, cy - 1.9f + o * 0.8f, 2.4f, cy - 0.7f + o * 0.3f};
        poly_fill(ss, fr, up, NP(up), (FinStyle){10, 0, 0, 0, 0, 0, 0});
        const float lo[] = {2.9f, cy + 1.0f + o * 0.3f, 1.0f, cy + 2.5f + o * 0.8f, 0.8f, cy + 1.9f + o * 0.8f, 2.4f, cy + 0.7f + o * 0.3f};
        poly_fill(ss, fr, lo, NP(lo), (FinStyle){10, 0, 0, 0, 0, 0, 0});
        dline(ss, fr, x0 + 0.5f, cy, 1.9f, cy + o * 0.4f, 10); // centre bar
        const float dors[] = {XU(0.52f), cy - 1.4f, XU(0.44f), cy - 3.4f + fl * 0.1f, XU(0.34f), cy - 1.3f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        const float anal[] = {XU(0.42f), cy + 1.5f, XU(0.32f), cy + 2.9f - fl * 0.1f, XU(0.08f), cy + 2.2f, XU(0.1f), cy + 1.0f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 0, 0, 0, 0, 0, 0});
        dput(ss, fr, XU(0.14f), cy - 1.4f, 7); // adipose fin
        body_fill(ss, fr, K, NK(K), cy, x0, x1, rummy_px);
        fish_eye(ss, fr, XU(0.88f), cy - 0.3f, 8, 9, true);
    }
}

// Snakehead (Channa argus): long cylindrical body, flat pointed head with a wide
// mouth, a dorsal running along almost the whole back and a long anal fin, round
// rayed tail; brown with dark blotches outlined in tan, dark streaks behind the
// eye, rusty pectoral fins, pale pelvic fins.
static uint8_t snakehead_px(const BodyPx *b)
{
    const float u = b->u, tt = b->tt;
    const uint8_t v = tt < 0.08f ? 1 : (tt < 0.35f ? 2 : (tt < 0.76f ? 3 : 4));
    if (v == 1) return v;
    if (u > 0.82f) {
        if (fm_abs(tt - 0.46f) < 0.07f || fm_abs(tt - 0.7f) < 0.06f) return 12; // head streaks
        return v;
    }
    if (tt > 0.16f && tt < 0.86f) {
        const float n = fm_sin(u * 26.0f + tt * 3.0f) + 0.8f * fm_sin(u * 15.0f - tt * 7.0f + 1.3f);
        if (n > 0.55f) return 5;  // dark blotch
        if (n > 0.25f) return 6;  // tan outline
    }
    return v;
}

static void snakehead_generate(SpriteSheet *ss)
{
    const int W = 42, H = 13;
    const float cy = 6.2f, x0 = 7.0f, x1 = 41.6f;
    static const Knot K[] = {{0, 1.4f, 1.4f}, {0.1f, 2.0f, 1.9f}, {0.3f, 2.5f, 2.4f}, {0.6f, 2.7f, 2.6f},
                             {0.8f, 2.5f, 2.4f}, {0.92f, 1.9f, 1.8f}, {1.0f, 0.9f, 0.8f}};
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        const float fl = FLAP4[fr], o = fl * 0.7f;
        const float tail[] = {x0 + 0.5f, cy - 1.3f, 5.0f, cy - 2.6f + o * 0.4f, 3.2f, cy - 3.4f + o * 0.7f, 1.4f, cy - 3.0f + o,
                              0.4f, cy - 1.6f + o, 0.2f, cy + o, 0.4f, cy + 1.6f + o, 1.4f, cy + 3.0f + o,
                              3.2f, cy + 3.4f + o * 0.7f, 5.0f, cy + 2.6f + o * 0.4f, x0 + 0.5f, cy + 1.3f};
        poly_fill(ss, fr, tail, NP(tail), (FinStyle){7, 10, 0, 1, 2, 0, 0});
        const float dors[] = {XU(0.72f), cy - 2.4f, XU(0.66f), cy - 4.3f, XU(0.1f), cy - 4.9f + fl * 0.3f,
                              XU(0.02f), cy - 3.4f + fl * 0.3f, XU(0.02f), cy - 1.3f};
        poly_fill(ss, fr, dors, NP(dors), (FinStyle){7, 10, 1, 0, 2, 0, 0});
        const float anal[] = {XU(0.45f), cy + 2.4f, XU(0.4f), cy + 4.4f, XU(0.06f), cy + 4.6f - fl * 0.3f, XU(0.02f), cy + 1.3f};
        poly_fill(ss, fr, anal, NP(anal), (FinStyle){7, 10, 1, 0, 2, 0, 0});
        const float pelv[] = {XU(0.7f), cy + 2.3f, XU(0.65f), cy + 4.6f + fl * 0.2f, XU(0.61f), cy + 2.4f};
        poly_fill(ss, fr, pelv, NP(pelv), (FinStyle){13, 0, 0, 0, 0, 0, 0});
        body_fill(ss, fr, K, NK(K), cy, x0, x1, snakehead_px);
        const float pect[] = {XU(0.79f), cy - 0.2f, XU(0.68f), cy + 0.6f + fl * 0.3f, XU(0.68f), cy + 1.8f + fl * 0.3f, XU(0.78f), cy + 1.4f};
        poly_fill(ss, fr, pect, NP(pect), (FinStyle){11, 0, 0, 0, 0, 0, 0});
        fish_eye(ss, fr, XU(0.93f), cy - 0.9f, 8, 9, false);
        dline(ss, fr, XU(0.94f), cy + 0.5f, x1, cy + 0.3f, 1); // wide mouth
    }
}

// Caridina shrimp in side view, facing right: bulky carapace with a forward
// rostrum and stalked eye, six abdominal segments tapering to a tail fan, long
// antennae sweeping forward, thin walking legs under the carapace (alternating
// stride, feet on the last row), swimmerets paddling under the abdomen.
// Bands (index 5) sit behind the eye, on the first abdominal segments and near
// the tail, like a crystal / bee shrimp.
static void shrimp_generate(SpriteSheet *ss)
{
    const int W = 18, H = 9;
    if (!sheet_alloc(ss, SKI(W), SKI(H))) return;
#define SHR_CY(x) (4.9f - 0.6f * fm_sin(fm_clamp(((x) - 2.0f) / 12.5f, 0.0f, 1.0f) * FM_PI))
#define SHR_HB(x) ((x) < 9.0f ? 0.9f + 1.2f * fm_clamp(((x) - 2.4f) / 6.6f, 0.0f, 1.0f) \
                              : ((x) < 13.4f ? 2.3f : 2.3f - 1.0f * ((x) - 13.4f)))
    static const float SEG[5] = {3.5f, 4.7f, 5.9f, 7.1f, 9.4f}; // segment joints, last = carapace edge
    for (int fr = 0; fr < FISH_FRAMES; fr++) {
        // tail fan (uropods + telson), fanned slightly upwards
        const float fan[] = {2.9f, SHR_CY(2.6f) - 0.7f, 0.6f, SHR_CY(2.6f) - 2.0f, 0.0f, SHR_CY(2.6f) - 0.6f,
                             0.6f, SHR_CY(2.6f) + 0.9f, 2.9f, SHR_CY(2.6f) + 0.7f};
        poly_fill(ss, fr, fan, NP(fan), (FinStyle){3, 0, 0, 0, 0, 2, (1 << 3) | (1 << 4)});
        // body
        for (int x = 0; x < ss->w; x++) {
            const float xc = ((float)x + 0.5f) / s_k;
            if (xc < 2.4f || xc > 14.6f) continue;
            const float cy = SHR_CY(xc), hb = SHR_HB(xc);
            bool joint = false;
            for (int s = 0; s < 5; s++) joint |= ox(SEG[s]) == x;
            for (int y = 0; y < ss->h; y++) {
                const float dy = ((float)y + 0.5f) / s_k - cy;
                if (fm_abs(dy) > hb) continue;
                uint8_t v = dy < -hb * 0.35f ? 3 : 2;
                if (dy > hb - 0.5f) v = 1;                                                   // belly edge
                if ((xc > 3.6f && xc < 4.6f) || (xc > 7.0f && xc < 9.3f) || (xc > 12.0f && xc < 13.0f)) v = 5; // bands
                else if (dy > 0.2f && dy < hb - 0.5f && ((int)xc & 1) && xc < 12.0f) v = 6;  // side dots
                if (joint && dy < -hb * 0.2f && v != 5) v = 1;                               // segment joints
                sheet_put(ss, fr, x, y, v);
            }
        }
        // walking legs under the carapace: thigh down-forward, shin down, alternating stride
        for (int i = 0; i < 5; i++) {
            const float lx = 9.6f + (float)i * 0.95f, base = SHR_CY(lx) + SHR_HB(lx) - 0.1f;
            const float st = ((i + fr) & 1) ? 0.7f : -0.2f;
            dline(ss, fr, lx, base, lx + 0.5f, base + 0.6f, 7);
            dline(ss, fr, lx + 0.5f, base + 0.6f, lx + st, (float)H - 0.5f, 7);
            sheet_put(ss, fr, ox(lx + st), ss->h - 1, 7); // foot
        }
        // swimmerets: short paddles under the abdomen, beating in a wave
        for (int i = 0; i < 5; i++) {
            const float sx = 3.9f + (float)i * 1.15f, base = SHR_CY(sx) + SHR_HB(sx) - 0.2f;
            const float sw = ((i + fr) & 1) ? 0.5f : -0.3f;
            dline(ss, fr, sx, base, sx + sw, base + 0.9f, 7);
        }
        // rostrum, stalked eye, antennae
        const float hy = SHR_CY(14.0f);
        dline(ss, fr, 14.2f, hy - 0.9f, 16.4f, hy - 1.5f, 2);
        dput(ss, fr, 14.0f, hy - 1.0f, 1);
        sheet_put(ss, fr, ox(14.0f) + 1, ox(hy - 1.0f), 8);
        const float wave = (fr == 1) ? -0.6f : (fr == 3 ? 0.6f : 0.0f);
        dline(ss, fr, 14.6f, hy - 0.4f, 16.2f, hy - 2.6f + wave, 7); // long antennae, forward and up
        dline(ss, fr, 16.2f, hy - 2.6f + wave, 17.9f, hy - 4.0f + wave * 1.5f, 7);
        dline(ss, fr, 14.6f, hy + 0.2f, 17.9f, hy - 0.2f - wave, 7); // second pair, straight ahead
    }
#undef SHR_CY
#undef SHR_HB
}

#undef XU
#undef NK
#undef NP
#undef PEARL

// Output scale of a species' sprite: everything a bit larger than the design
// size, small species more so they stay readable on the 400x240 canvas.
static float sprite_scale(const SpeciesDef *d)
{
    const float t = fm_clamp(((float)d->size_px - 14.0f) / 12.0f, 0.0f, 1.0f);
    return 1.5f - 0.2f * t;
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
                s_k = sprite_scale(&SPECIES[i]);
                if (f->body == BODY_SHRIMP) shrimp_generate(&s_sprites[ns]);
                else if (f->body == BODY_DISCUS) discus_generate(&s_sprites[ns]);
                else if (f->body == BODY_ANGEL) angel_generate(&s_sprites[ns]);
                else if (f->body == BODY_CORY) cory_generate(&s_sprites[ns]);
                else if (f->body == BODY_GOURAMI) gourami_generate(&s_sprites[ns]);
                else if (f->body == BODY_KRIB) krib_generate(&s_sprites[ns]);
                else if (f->body == BODY_MBUNA) mbuna_generate(&s_sprites[ns]);
                else if (f->body == BODY_KUHLI) kuhli_generate(&s_sprites[ns]);
                else if (f->body == BODY_CARDINAL) cardinal_generate(&s_sprites[ns]);
                else if (f->body == BODY_EMBER) ember_generate(&s_sprites[ns]);
                else if (f->body == BODY_HARLEQUIN) harlequin_generate(&s_sprites[ns]);
                else if (f->body == BODY_CONGO) congo_generate(&s_sprites[ns]);
                else if (f->body == BODY_PEACOCK) peacock_generate(&s_sprites[ns]);
                else if (f->body == BODY_GOLDFISH) goldfish_generate(&s_sprites[ns]);
                else if (f->body == BODY_KOI) koi_generate(&s_sprites[ns]);
                else if (f->body == BODY_MEDAKA) medaka_generate(&s_sprites[ns]);
                else if (f->body == BODY_BETTA) betta_generate(&s_sprites[ns]);
                else if (f->body == BODY_TANICHTHYS) tanichthys_generate(&s_sprites[ns]);
                else if (f->body == BODY_RUMMY) rummy_generate(&s_sprites[ns]);
                else if (f->body == BODY_SNAKEHEAD) snakehead_generate(&s_sprites[ns]);
                else if (f->body == BODY_RAMSHORN || f->body == BODY_CONE) snail_generate(&s_sprites[ns], f->body);
                else {
                    FishShape sf = *f; // generic generator: scale the shape parameters
                    sf.len = (uint8_t)((float)f->len * s_k + 0.5f), sf.h = (uint8_t)((float)f->h * s_k + 0.5f);
                    sf.tail_len = (uint8_t)((float)f->tail_len * s_k + 0.5f);
                    sf.tail_h = (uint8_t)((float)f->tail_h * s_k + 0.5f);
                    sf.dorsal = (uint8_t)((float)f->dorsal * s_k + 0.5f), sf.anal = (uint8_t)((float)f->anal * s_k + 0.5f);
                    fish_generate(&s_sprites[ns], &sf);
                }
                s_sprite_of[i] = ns++;
            }
        }
        hal_log("creatures: %d sprites, %d/%d bytes", ns, s_sprite_used, SPRITE_POOL);
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
