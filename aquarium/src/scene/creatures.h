// Species registry + creature simulation and rendering. Portable.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../config/config.h"
#include "../engine/canvas.h"
#include "../engine/sprite.h"

typedef enum { BEH_SCHOOL = 0, BEH_WANDER, BEH_BOTTOM, BEH_SHRIMP, BEH_SNAIL } BehaviorType;

typedef enum { TAIL_FORK = 0, TAIL_ROUND, TAIL_FAN } TailType;
typedef enum {
    PAT_NONE = 0,
    PAT_NEON,     // neon tetra: blue line + red rear belly
    PAT_CARDINAL, // cardinal tetra: blue line + full red belly
    PAT_BANDS,    // 3 vertical bands (scalare)
    PAT_SPOTS,    // dots on the back (corydoras, gourami perlato)
    PAT_LATERAL,  // horizontal mid stripe
    PAT_WAVY,     // wavy horizontal lines (discus)
    PAT_BARS,     // many thin vertical bars (kuhli, mbuna)
    PAT_WEDGE,    // black triangle on the rear body (rasbora arlecchino)
    PAT_PATCHES   // large blotches (koi)
} FishPattern;
typedef enum { BODY_SLIM = 0, BODY_DISC, BODY_SHRIMP, BODY_RAMSHORN, BODY_CONE } BodyType; // last 3: invertebrates
enum { FIN_FEELERS = 1, FIN_SWEPT = 2 }; // FishShape.flags

// Parameters for the procedural fish sprite generator.
typedef struct {
    uint8_t len, h;          // body length / height (px)
    uint8_t tail_len, tail_h;
    uint8_t tail;            // TailType
    uint8_t dorsal;          // dorsal fin height
    uint8_t pattern;         // FishPattern
    uint8_t body;            // BodyType
    uint8_t anal;            // anal fin height below the belly (0 = small)
    uint8_t flags;           // FIN_*
} FishShape;

#define SPECIES_VARIANTS 4

typedef struct {
    const char *id;   // "neon_tetra"
    const char *name; // "Neon"
    const char *biome; // biome the species comes from
    BehaviorType behavior;
    uint8_t size_px;              // nominal width on the 400x240 canvas
    float speed_min, speed_max;   // px/s
    uint8_t depth_min, depth_max; // preferred band (0 = surface, 255 = bottom)
    uint8_t default_count, max_count;
    uint8_t variants;
    FishShape shape;              // fish behaviours only
    // Palettes per variant, indices 1..9 (0 unused/transparent).
    // Fish: 1 dark back edge, 2 back, 3 body, 4 belly, 5 pattern A, 6 pattern B, 7 fins, 8 eye.
    // Shrimp: 1 outline, 2 body, 3 light back, 5 bands, 6 side dots, 7 legs/antennae, 8 eye.
    // Snails: 1 shell lines, 2 shell, 3 highlight, 4 foot, 5 shell bands, 8 eye.
    uint16_t pal[SPECIES_VARIANTS][10];
} SpeciesDef;

extern const SpeciesDef SPECIES[];
extern const int SPECIES_COUNT;

int species_find(const char *id);
const SpriteSheet *species_sprite(int sp);

void creatures_init(void); // allocate pools + generate sprites (once)
void creatures_spawn(const AquariumConfig *c, uint32_t seed);
void creatures_update(float dt);
void creatures_draw(Canvas *cv, float t);
// Scare creatures near (x,y). Returns true if at least one was close enough to be "hit".
bool creatures_scare(int x, int y);
bool creatures_near(int x, int y, int radius);
int creatures_count(void);
