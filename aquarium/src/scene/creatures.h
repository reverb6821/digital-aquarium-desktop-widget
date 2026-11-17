// Species registry + creature simulation and rendering. Portable.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../config/config.h"
#include "../engine/canvas.h"
#include "../engine/sprite.h"
#include "../i18n/i18n.h"

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
typedef enum { BODY_SLIM = 0, BODY_DISC, BODY_SHRIMP, BODY_RAMSHORN, BODY_CONE, BODY_DISCUS, BODY_ANGEL,
               BODY_CORY, BODY_GOURAMI, BODY_KRIB, BODY_MBUNA, BODY_KUHLI, BODY_CARDINAL, BODY_EMBER,
               BODY_HARLEQUIN, BODY_CONGO, BODY_PEACOCK, BODY_GOLDFISH, BODY_KOI, BODY_MEDAKA, BODY_BETTA,
               BODY_TANICHTHYS, BODY_RUMMY, BODY_SNAKEHEAD } BodyType;
// BODY_SHRIMP/RAMSHORN/CONE: invertebrates; BODY_DISCUS and later: dedicated per-species generators
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
    StrId name;       // display name, tr() it
    const char *biome; // biome the species comes from
    BehaviorType behavior;
    uint8_t size_px;              // nominal width on the 400x240 canvas
    float speed_min, speed_max;   // px/s
    uint8_t depth_min, depth_max; // preferred band (0 = surface, 255 = bottom)
    uint8_t default_count, max_count;
    uint8_t variants;
    FishShape shape;              // fish behaviours only
    // Palettes per variant, indices 1..15 (0 unused/transparent).
    // Fish: 1 dark back edge, 2 back, 3 body, 4 belly, 5 pattern A, 6 pattern B, 7 fins, 8 eye.
    // Shrimp: 1 outline, 2 body, 3 light back, 5 bands, 6 side dots, 7 legs/antennae, 8 eye.
    // Snails: 1 shell lines, 2 shell, 3 highlight, 4 foot, 5 shell bands, 8 eye.
    // Angelfish: 1 dark edge, 2 back, 3 body, 4 belly/sheen, 5 bands, 6 band edge, 7 fins, 8 pupil, 9 iris.
    // Corydoras: 1 dark edge, 2 back, 3 flank, 4 belly, 5 head, 6 dorsal patch, 7 fins, 8 pupil, 9 eye mask,
    //   10 iris, 11 barbels, 12 peduncle spot, 13 plate seam, 14 fin rays.
    // Gourami: 1 dark edge, 2 back, 3 flank, 4 chest, 5 lateral line, 6 pearls, 7 fins, 8 pupil, 9 eye ring,
    //   10 anal fin edge, 11 feelers.
    // Kribensis: 1 dark edge, 2 back, 3 flank, 4 belly, 5 stripes, 6 belly patch, 7 fins, 8 pupil, 9 cheek,
    //   10 fin edge (blue), 11 fin edge (red), 12 fin ocelli, 13 pelvic fins, 14 iris, 15 lower tail lobe.
    // Cardinal: 1 dark edge, 2 back, 3 blue line, 4 line highlight, 5 red, 6 red belly edge, 7 fins, 8 pupil,
    //   9 white belly, 10 iris, 11 red tail base, 12 fin rays.
    // Ember: 1 dark edge, 2 back, 3 body, 4 belly, 5 spine, 7 fins, 8 pupil, 9 iris, 10 dorsal edge, 11 anal edge.
    // Harlequin: 1 dark edge, 2 back, 3 flank, 4 belly, 5 wedge, 6 gold line, 7 fins, 8 pupil, 9 iris, 10 fin edge.
    // Congo: 1 dark edge, 2 back, 3 flank, 4 belly, 5 orange band, 6 band edge, 7 fins, 8 pupil, 9 iris,
    //   10 fin edge, 11 scale glitter, 12 peduncle.
    // Peacock: 1 dark edge, 2 back, 3 flank, 4 belly, 5 bars, 6 shimmer, 7 fins, 8 pupil, 9 head, 10 fin edge,
    //   11 iris, 12 dark fins/rays, 13 anal edge, 14 lips/opercle.
    // Goldfish: 1 dark edge, 2 back, 3 body, 4 belly, 5 scale sparkle, 6 fin rays, 7 fins, 8 pupil, 9 iris,
    //   10 fin tips, 12 patch, 13 patch sparkle, 14 patch back.
    // Koi: 1 dark edge, 2 back, 3 body, 4 belly, 5 red patches, 6 black spots, 7 fins, 8 pupil, 9 scale net,
    //   10 fin rays, 11 barbels/lips, 12 scale net on patches.
    // Medaka: 1 dark edge, 2 back, 3 body, 4 belly, 5 scale net, 6 lateral line, 7 fins, 8 pupil, 9 iris,
    //   10 fin rays, 11 gill shine.
    // Mbuna: 1 dark edge, 2 back, 3 flank, 4 belly, 5 bars, 6 opercle, 7 fins, 8 pupil, 9 iris, 10 fin edge,
    //   12 egg spots, 13 pelvic fins, 14 pectoral/fin rays.
    // Kuhli: 1 dark edge, 2 back, 3 flank, 4 belly, 5 saddles, 7 fins, 8 eye.
    // Betta: 1 dark edge, 2 back, 3 body, 4 belly, 5 scales, 6 head, 7 fins, 8 pupil, 9 iris, 10 fin edge,
    //   11 fin rays, 12 rear body patch, 13 fin base, 14 crown spikes / streamer tips, 15 opercle, throat, mouth.
    // White cloud: 1 dark edge, 2 back, 3 flank, 4 belly, 5 iridescent line, 6 warm band, 7 fins, 8 pupil,
    //   9 iris, 10 fin edge, 11 tail, 12 peduncle spot, 13 fin tips.
    // Rummy-nose: 1 dark edge, 2 back, 3 flank, 4 belly, 5 scale sheen, 6 red head, 7 fins, 8 pupil, 9 iris,
    //   10 black tail bars, 11 white tail, 12 dark red head top.
    // Snakehead: 1 dark edge, 2 back, 3 flank, 4 belly, 5 blotches, 6 blotch outline, 7 fins, 8 pupil, 9 iris,
    //   10 fin rays, 11 pectoral fins, 12 head streaks, 13 pelvic fins.
    // Discus: 1 outline, 2 body shade, 3 body, 4 belly/face, 5 lines, 6 dots/tail, 7 fins, 8 pupil, 9 fin edge/eye ring.
    uint16_t pal[SPECIES_VARIANTS][16]; // sprite_draw reads 16 entries
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
