// AquariumConfig: everything the user can customise. Portable.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_VERSION 2
#define CFG_MAX_PLANTS 28
#define CFG_MAX_DECOR 20
#define CFG_MAX_BUBBLES 4
#define CFG_MAX_SPECIES 12
#define CFG_MAX_USER_PRESETS 8

// Scenic backdrop painted behind everything (distant biotope silhouettes).
typedef enum { BG_OPEN = 0, BG_ROCKS, BG_RICE, BG_SWAMP, BG_FOREST, BG_REEDS, BACKDROP_COUNT } Backdrop;

typedef enum { SUB_SAND = 0, SUB_GRAVEL, SUB_DARK_ROCK, SUB_PEBBLES, SUB_NONE, SUB_COUNT } SubstrateType;

typedef enum {
    PLANT_STEM = 0,    // pianta a stelo ramificata (Rotala, Egeria)
    PLANT_VALLIS,      // vallisneria
    PLANT_CRYPT,       // Cryptocoryne (rosetta di foglie ondulate)
    PLANT_MICROSORUM,  // Microsorum pteropus (felce di Java)
    PLANT_BOLBITIS,    // Bolbitis heudelotii (felce africana)
    PLANT_ANUBIAS,     // Anubias
    PLANT_BUCE,        // Bucephalandra
    PLANT_ECHINO,      // Echinodorus (spada amazzonica)
    PLANT_MOSS,        // muschio di Java
    PLANT_LILY,        // ninfea (foglie galleggianti)
    PLANT_COUNT
} PlantType;

typedef enum {
    DECOR_ROCK_BIG = 0,
    DECOR_DRAGON_STONE, // dragon stone (Ohko): guglie porose color ruggine
    DECOR_SEIRYU_STONE, // seiryu stone: pietra grigia a strati con venature bianche
    DECOR_ROCK_CLUSTER, // gruppo di sassi
    DECOR_ROCK_SLATE,   // lastre di ardesia impilate
    DECOR_DRIFTWOOD,    // tronco sdraiato
    DECOR_SPIDERWOOD,   // radici sottili ramificate
    DECOR_BRANCH,       // ramo alto verticale
    DECOR_ROOTS,        // radici ad arco (mangrovia)
    DECOR_LEAVES,       // foglie di catappa sul fondo
    DECOR_COUNT
} DecorType;

enum { LAYER_BACK = 0, LAYER_FRONT = 1 };

// Limits for the placement offsets (edit mode).
#define CFG_DY_MIN (-200)
#define CFG_DY_MAX 12
#define CFG_SCALE_MIN 50
#define CFG_SCALE_MAX 200
#define CFG_SCALE_STEP 10

// Normalise an angle in degrees to -180..179.
static inline int16_t cfg_norm_rot(int a)
{
    a %= 360;
    if (a < -180) a += 360;
    if (a >= 180) a -= 360;
    return (int16_t)a;
}

typedef struct {
    uint8_t type;
    int16_t x;
    uint8_t layer;
    uint8_t height;
    uint16_t color;
    uint32_t seed;
    int16_t dy;  // vertical offset of the base from the substrate (px, negative = up)
    int16_t rot; // rotation around the base, degrees (positive = clockwise)
    uint8_t scale; // size in percent (CFG_SCALE_MIN..CFG_SCALE_MAX, 100 = original; 0 is read as 100)
} PlantCfg;

typedef struct {
    uint8_t type;
    int16_t x;
    uint8_t layer;
    uint16_t color;
    int16_t dy;  // vertical offset of the base from the substrate (px, negative = up)
    int16_t rot; // rotation around the base, degrees (positive = clockwise)
    uint8_t scale; // size in percent (CFG_SCALE_MIN..CFG_SCALE_MAX, 100 = original; 0 is read as 100)
} DecorCfg;

typedef struct {
    int16_t x;
    uint8_t rate; // 1..100
} BubbleCfg;

typedef struct {
    char id[20];
    uint8_t count;
    uint8_t variant;
} SpeciesCfg;

typedef struct {
    uint8_t version;
    char theme_id[16]; // biome id ("amazon", "blackwater", ...) | "custom"

    // Water / background
    uint16_t water_top, water_bottom; // RGB565
    uint8_t light_rays;               // 0..5
    uint8_t particles;                // density 0..100
    bool show_surface;
    uint8_t backdrop; // Backdrop

    // Substrate
    uint8_t substrate_type;
    uint16_t substrate_color;
    uint8_t substrate_height;    // 10..60 canvas px
    uint8_t substrate_roughness; // 0..100
    uint32_t substrate_seed;

    uint8_t plant_count;
    PlantCfg plants[CFG_MAX_PLANTS];
    uint8_t decor_count;
    DecorCfg decor[CFG_MAX_DECOR];

    uint8_t bubble_sources;
    BubbleCfg bubbles[CFG_MAX_BUBBLES];

    uint8_t species_count;
    SpeciesCfg species[CFG_MAX_SPECIES];

    // Environment
    uint8_t sim_speed;           // 50..200 %
    uint8_t fps_cap;             // 20 | 30
    uint16_t screen_off_minutes; // 0 = never
    uint8_t brightness;          // 10..100 %, simulated (backlight is on/off only)
    uint16_t creature_limit;     // global cap, default 40
    uint8_t language;            // Lang (i18n.h), 0 = English
} AquariumConfig;

// Clamp every field to a valid range (after load/migration or UI edits).
void config_sanitize(AquariumConfig *c);

// JSON (de)serialisation. Returns bytes written (0 on overflow).
size_t config_to_json(const AquariumConfig *c, const char *name, char *buf, size_t cap);
// Parses JSON into *c (fields missing in JSON keep their current value).
// Optionally copies the preset name into name (cap name_cap).
bool config_from_json(AquariumConfig *c, const char *json, char *name, size_t name_cap);

#ifdef __cplusplus
}
#endif
