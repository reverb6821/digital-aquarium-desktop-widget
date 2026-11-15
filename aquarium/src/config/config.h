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
    DECOR_ROCK_FLAT,
    DECOR_ROCK_TALL,    // pietra verticale (Seiryu)
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

typedef struct {
    uint8_t type;
    int16_t x;
    uint8_t layer;
    uint8_t height;
    uint16_t color;
    uint32_t seed;
} PlantCfg;

typedef struct {
    uint8_t type;
    int16_t x;
    uint8_t layer;
    uint16_t color;
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
    bool day_night_cycle;
    uint8_t sim_speed;           // 50..200 %
    uint8_t fps_cap;             // 20 | 30
    uint16_t screen_off_minutes; // 0 = never
    uint8_t brightness;          // 10..100 %, simulated (backlight is on/off only)
    uint16_t creature_limit;     // global cap, default 60
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
