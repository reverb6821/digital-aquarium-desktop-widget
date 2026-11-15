#include "themes.h"
#include <stdio.h>
#include <string.h>
#include "../engine/color.h"

const char *const THEME_IDS[THEME_COUNT] = {"amazon", "blackwater", "seasia", "africa", "malawi", "pond"};
const char *const THEME_NAMES[THEME_COUNT] = {"Amazzonico", "Acque nere", "Sud-Est Asia",
                                              "Africa Congo", "Lago Malawi", "Laghetto"};

const WaterPalette WATER_PALETTES[WATER_PALETTE_COUNT] = {
    {"Amazzonico", RGB565(24, 58, 60), RGB565(5, 16, 18)},
    {"Acque nere (tannini)", RGB565(70, 46, 20), RGB565(14, 8, 3)},
    {"Sud-Est Asia", RGB565(28, 66, 58), RGB565(6, 18, 16)},
    {"Congo", RGB565(46, 56, 36), RGB565(10, 14, 8)},
    {"Blu Malawi", RGB565(30, 80, 120), RGB565(6, 20, 44)},
    {"Verde laghetto", RGB565(40, 70, 40), RGB565(9, 20, 10)},
};

const uint16_t SWATCHES[SWATCH_COUNT] = {
    RGB565(40, 150, 110), RGB565(30, 110, 70),  RGB565(70, 170, 90),  RGB565(40, 200, 210),
    RGB565(110, 140, 60), RGB565(150, 110, 60), RGB565(220, 40, 150), RGB565(240, 120, 40),
    RGB565(230, 80, 80),  RGB565(160, 90, 220), RGB565(240, 220, 200), RGB565(120, 120, 130),
    RGB565(60, 60, 70),   RGB565(200, 160, 110), RGB565(90, 60, 40),  RGB565(255, 200, 60),
};

static void add_plant(AquariumConfig *c, PlantType type, int x, int layer, int h, uint16_t col, uint32_t seed)
{
    if (c->plant_count >= CFG_MAX_PLANTS) return;
    c->plants[c->plant_count++] = (PlantCfg){(uint8_t)type, (int16_t)x, (uint8_t)layer, (uint8_t)h, col, seed};
}

static void add_decor(AquariumConfig *c, DecorType type, int x, int layer, uint16_t col)
{
    if (c->decor_count >= CFG_MAX_DECOR) return;
    c->decor[c->decor_count++] = (DecorCfg){(uint8_t)type, (int16_t)x, (uint8_t)layer, col};
}

static void add_species(AquariumConfig *c, const char *id, int count, int variant)
{
    if (c->species_count >= CFG_MAX_SPECIES) return;
    SpeciesCfg *s = &c->species[c->species_count++];
    snprintf(s->id, sizeof(s->id), "%s", id);
    s->count = (uint8_t)count;
    s->variant = (uint8_t)variant;
}

static void add_bubbles(AquariumConfig *c, int x, int rate)
{
    if (c->bubble_sources >= CFG_MAX_BUBBLES) return;
    c->bubbles[c->bubble_sources++] = (BubbleCfg){(int16_t)x, (uint8_t)rate};
}

static void base_env(AquariumConfig *c)
{
    c->version = CONFIG_VERSION;
    c->show_surface = true;
    c->plant_count = c->decor_count = c->bubble_sources = c->species_count = 0;
}

static void water(AquariumConfig *c, uint16_t top, uint16_t bottom, int rays, int particles)
{
    c->water_top = top;
    c->water_bottom = bottom;
    c->light_rays = (uint8_t)rays;
    c->particles = (uint8_t)particles;
}

static void substrate(AquariumConfig *c, SubstrateType type, uint16_t col, int height, int rough, uint32_t seed)
{
    c->substrate_type = (uint8_t)type;
    c->substrate_color = col;
    c->substrate_height = (uint8_t)height;
    c->substrate_roughness = (uint8_t)rough;
    c->substrate_seed = seed;
}

// Plant colours
#define G_BRIGHT RGB565(70, 165, 80)   // Echinodorus, stem plants
#define G_MID RGB565(50, 140, 75)
#define G_DARK RGB565(35, 105, 60)     // Microsorum, Anubias
#define G_BOLB RGB565(40, 110, 70)     // Bolbitis (translucent dark green)
#define G_CRYPT RGB565(95, 115, 60)    // Cryptocoryne wendtii (olive/bronze)
#define G_CRYPT_R RGB565(130, 80, 60)  // Cryptocoryne "brown/red"
#define G_BUCE RGB565(60, 100, 95)     // Bucephalandra (blue-green)
#define G_MOSS RGB565(60, 140, 70)
#define G_VALLIS RGB565(80, 160, 90)
#define G_LILY RGB565(70, 150, 70)
#define G_ROTALA_R RGB565(190, 90, 70) // Rotala rotundifolia (pink-red tips)

// Hardscape colours
#define WOOD RGB565(110, 80, 50)
#define WOOD_DARK RGB565(70, 48, 30)
#define WOOD_PALE RGB565(150, 120, 85)
#define STONE RGB565(70, 72, 78)
#define STONE_SEIRYU RGB565(95, 98, 102)
#define STONE_WARM RGB565(120, 105, 90)
#define LEAF_BROWN RGB565(120, 70, 35)

void theme_apply(AquariumConfig *c, ThemeId id)
{
    base_env(c);
    snprintf(c->theme_id, sizeof(c->theme_id), "%s", THEME_IDS[id]);
    switch (id) {
    case THEME_AMAZON:
    default: // Rio Negro tributary, planted: swords, vallisneria, stem plants, wood
        water(c, RGB565(24, 58, 60), RGB565(5, 16, 18), 4, 40);
        substrate(c, SUB_SAND, RGB565(150, 125, 90), 48, 40, 1234);
        add_decor(c, DECOR_DRIFTWOOD, 120, LAYER_BACK, WOOD);
        add_decor(c, DECOR_SPIDERWOOD, 300, LAYER_BACK, WOOD_PALE);
        add_decor(c, DECOR_ROCK_BIG, 60, LAYER_BACK, STONE_WARM);
        add_decor(c, DECOR_ROCK_CLUSTER, 220, LAYER_FRONT, STONE_WARM);
        add_decor(c, DECOR_BRANCH, 350, LAYER_BACK, WOOD);
        add_decor(c, DECOR_LEAVES, 170, LAYER_FRONT, LEAF_BROWN);
        add_plant(c, PLANT_VALLIS, 20, LAYER_BACK, 110, G_VALLIS, 11);
        add_plant(c, PLANT_ECHINO, 246, LAYER_BACK, 80, G_BRIGHT, 23);
        add_plant(c, PLANT_VALLIS, 380, LAYER_BACK, 100, G_VALLIS, 12);
        add_plant(c, PLANT_STEM, 90, LAYER_BACK, 90, G_ROTALA_R, 31);
        add_plant(c, PLANT_ECHINO, 160, LAYER_BACK, 60, G_MID, 41);
        add_plant(c, PLANT_STEM, 330, LAYER_BACK, 70, G_BRIGHT, 32);
        add_plant(c, PLANT_ECHINO, 40, LAYER_FRONT, 40, G_BRIGHT, 43);
        add_plant(c, PLANT_MOSS, 125, LAYER_BACK, 7, G_MOSS, 3);
        add_plant(c, PLANT_MOSS, 280, LAYER_FRONT, 6, G_MOSS, 4);
        add_plant(c, PLANT_ECHINO, 370, LAYER_FRONT, 36, G_MID, 44);
        add_bubbles(c, 312, 45);
        add_species(c, "neon_tetra", 14, 0);
        add_species(c, "ember_tetra", 7, 0);
        add_species(c, "angelfish", 3, 0);
        add_species(c, "corydoras", 6, 0);
        break;
    case THEME_BLACKWATER: // tea-coloured water, tangle of branches and roots, leaf litter
        water(c, RGB565(70, 46, 20), RGB565(14, 8, 3), 2, 65);
        substrate(c, SUB_SAND, RGB565(90, 66, 40), 44, 30, 4321);
        add_decor(c, DECOR_BRANCH, 70, LAYER_BACK, WOOD_DARK);
        add_decor(c, DECOR_BRANCH, 300, LAYER_BACK, WOOD);
        add_decor(c, DECOR_ROOTS, 190, LAYER_BACK, WOOD_DARK);
        add_decor(c, DECOR_SPIDERWOOD, 360, LAYER_BACK, WOOD);
        add_decor(c, DECOR_DRIFTWOOD, 250, LAYER_FRONT, WOOD_DARK);
        add_decor(c, DECOR_SPIDERWOOD, 30, LAYER_FRONT, WOOD);
        add_decor(c, DECOR_BRANCH, 140, LAYER_FRONT, WOOD);
        add_decor(c, DECOR_LEAVES, 110, LAYER_FRONT, LEAF_BROWN);
        add_decor(c, DECOR_LEAVES, 220, LAYER_FRONT, RGB565(140, 85, 40));
        add_decor(c, DECOR_LEAVES, 340, LAYER_FRONT, LEAF_BROWN);
        add_plant(c, PLANT_ECHINO, 120, LAYER_BACK, 50, G_MID, 5);
        add_plant(c, PLANT_ECHINO, 246, LAYER_BACK, 70, G_MID, 23);
        add_plant(c, PLANT_MOSS, 195, LAYER_BACK, 6, G_MOSS, 7);
        add_plant(c, PLANT_MOSS, 300, LAYER_BACK, 6, G_MOSS, 8);
        add_bubbles(c, 340, 20);
        add_species(c, "cardinal_tetra", 16, 0);
        add_species(c, "discus", 4, 0);
        add_species(c, "corydoras", 5, 1);
        break;
    case THEME_SEASIA: // Borneo stream: Cryptocoryne carpet, Java fern and Buce on rocks/wood
        water(c, RGB565(28, 66, 58), RGB565(6, 18, 16), 3, 35);
        substrate(c, SUB_GRAVEL, RGB565(80, 74, 66), 50, 55, 99);
        add_decor(c, DECOR_ROCK_BIG, 70, LAYER_BACK, STONE);
        add_decor(c, DECOR_ROCK_FLAT, 290, LAYER_BACK, STONE);
        add_decor(c, DECOR_DRIFTWOOD, 190, LAYER_BACK, WOOD);
        add_decor(c, DECOR_SPIDERWOOD, 350, LAYER_BACK, WOOD_PALE);
        add_decor(c, DECOR_ROCK_CLUSTER, 140, LAYER_FRONT, STONE);
        add_decor(c, DECOR_ROCK_CLUSTER, 330, LAYER_FRONT, STONE_WARM);
        add_plant(c, PLANT_VALLIS, 25, LAYER_BACK, 105, G_VALLIS, 1);
        add_plant(c, PLANT_MICROSORUM, 246, LAYER_BACK, 60, G_DARK, 23);
        add_plant(c, PLANT_MICROSORUM, 75, LAYER_BACK, 46, G_DARK, 2);
        add_plant(c, PLANT_VALLIS, 385, LAYER_BACK, 95, G_VALLIS, 3);
        add_plant(c, PLANT_CRYPT, 110, LAYER_FRONT, 28, G_CRYPT, 4);
        add_plant(c, PLANT_CRYPT, 170, LAYER_FRONT, 22, G_CRYPT_R, 5);
        add_plant(c, PLANT_CRYPT, 230, LAYER_FRONT, 30, G_CRYPT, 6);
        add_plant(c, PLANT_CRYPT, 300, LAYER_FRONT, 24, G_CRYPT_R, 7);
        add_plant(c, PLANT_CRYPT, 360, LAYER_FRONT, 26, G_CRYPT, 8);
        add_plant(c, PLANT_BUCE, 290, LAYER_BACK, 10, G_BUCE, 9);
        add_plant(c, PLANT_BUCE, 60, LAYER_FRONT, 9, G_CRYPT_R, 10);
        add_plant(c, PLANT_MOSS, 195, LAYER_BACK, 7, G_MOSS, 11);
        add_bubbles(c, 320, 35);
        add_species(c, "harlequin", 12, 0);
        add_species(c, "gourami", 2, 0);
        add_species(c, "gourami", 1, 1);
        add_species(c, "kuhli_loach", 5, 0);
        add_species(c, "shrimp", 6, 0);
        add_species(c, "shrimp", 4, 1);
        add_species(c, "shrimp", 3, 2);
        add_species(c, "assassin_snail", 2, 0);
        break;
    case THEME_AFRICA: // Congo river bank: Bolbitis and Anubias on wood and slate
        water(c, RGB565(46, 56, 36), RGB565(10, 14, 8), 3, 45);
        substrate(c, SUB_SAND, RGB565(120, 100, 75), 46, 45, 777);
        add_decor(c, DECOR_ROOTS, 90, LAYER_BACK, WOOD);
        add_decor(c, DECOR_ROCK_SLATE, 300, LAYER_BACK, RGB565(80, 78, 74));
        add_decor(c, DECOR_DRIFTWOOD, 210, LAYER_BACK, WOOD_DARK);
        add_decor(c, DECOR_BRANCH, 370, LAYER_BACK, WOOD);
        add_decor(c, DECOR_ROCK_CLUSTER, 160, LAYER_FRONT, STONE_WARM);
        add_decor(c, DECOR_LEAVES, 260, LAYER_FRONT, LEAF_BROWN);
        add_plant(c, PLANT_BOLBITIS, 60, LAYER_BACK, 60, G_BOLB, 1);
        add_plant(c, PLANT_BOLBITIS, 246, LAYER_BACK, 70, G_BOLB, 23);
        add_plant(c, PLANT_VALLIS, 385, LAYER_BACK, 110, G_VALLIS, 2);
        add_plant(c, PLANT_BOLBITIS, 330, LAYER_BACK, 45, G_BOLB, 3);
        add_plant(c, PLANT_ANUBIAS, 120, LAYER_FRONT, 24, G_DARK, 4);
        add_plant(c, PLANT_ANUBIAS, 300, LAYER_FRONT, 20, G_DARK, 5);
        add_plant(c, PLANT_ANUBIAS, 200, LAYER_BACK, 18, G_DARK, 6);
        add_plant(c, PLANT_MOSS, 160, LAYER_BACK, 6, G_MOSS, 7);
        add_bubbles(c, 40, 40);
        add_species(c, "congo_tetra", 8, 0);
        add_species(c, "kribensis", 2, 0);
        break;
    case THEME_MALAWI: // rocky lake shore: piles of stone, very few plants
        water(c, RGB565(30, 80, 120), RGB565(6, 20, 44), 5, 25);
        substrate(c, SUB_SAND, RGB565(200, 185, 150), 44, 30, 2024);
        add_decor(c, DECOR_ROCK_BIG, 50, LAYER_BACK, STONE);
        add_decor(c, DECOR_ROCK_TALL, 105, LAYER_BACK, STONE_SEIRYU);
        add_decor(c, DECOR_ROCK_SLATE, 160, LAYER_BACK, RGB565(90, 88, 84));
        add_decor(c, DECOR_ROCK_BIG, 230, LAYER_BACK, STONE_WARM);
        add_decor(c, DECOR_ROCK_TALL, 285, LAYER_BACK, STONE);
        add_decor(c, DECOR_ROCK_BIG, 350, LAYER_BACK, STONE_SEIRYU);
        add_decor(c, DECOR_ROCK_FLAT, 200, LAYER_FRONT, STONE);
        add_decor(c, DECOR_ROCK_CLUSTER, 70, LAYER_FRONT, STONE_WARM);
        add_decor(c, DECOR_ROCK_CLUSTER, 320, LAYER_FRONT, STONE);
        add_decor(c, DECOR_ROCK_TALL, 375, LAYER_FRONT, STONE_SEIRYU);
        add_plant(c, PLANT_VALLIS, 20, LAYER_BACK, 120, G_VALLIS, 1);
        add_plant(c, PLANT_VALLIS, 246, LAYER_BACK, 100, G_VALLIS, 23);
        add_plant(c, PLANT_ANUBIAS, 160, LAYER_FRONT, 16, G_DARK, 2);
        add_bubbles(c, 380, 50);
        add_species(c, "mbuna", 4, 0);
        add_species(c, "mbuna", 3, 1);
        add_species(c, "mbuna", 2, 2);
        add_species(c, "aulonocara", 3, 0);
        break;
    case THEME_POND: // garden pond: green water, pebbles, water lilies
        water(c, RGB565(40, 70, 40), RGB565(9, 20, 10), 3, 75);
        substrate(c, SUB_PEBBLES, RGB565(90, 84, 72), 40, 50, 5150);
        add_decor(c, DECOR_ROCK_BIG, 40, LAYER_BACK, STONE_WARM);
        add_decor(c, DECOR_ROCK_FLAT, 330, LAYER_BACK, STONE);
        add_decor(c, DECOR_ROOTS, 370, LAYER_BACK, WOOD_DARK);
        add_decor(c, DECOR_ROCK_CLUSTER, 150, LAYER_FRONT, STONE_WARM);
        add_decor(c, DECOR_ROCK_CLUSTER, 280, LAYER_FRONT, STONE);
        add_plant(c, PLANT_LILY, 90, LAYER_BACK, 200, G_LILY, 1);
        add_plant(c, PLANT_LILY, 246, LAYER_BACK, 200, G_LILY, 23);
        add_plant(c, PLANT_STEM, 190, LAYER_BACK, 100, G_BRIGHT, 2);
        add_plant(c, PLANT_VALLIS, 15, LAYER_BACK, 110, G_VALLIS, 3);
        add_plant(c, PLANT_LILY, 345, LAYER_BACK, 200, RGB565(90, 140, 60), 4);
        add_plant(c, PLANT_STEM, 300, LAYER_FRONT, 60, G_MID, 5);
        add_plant(c, PLANT_MOSS, 220, LAYER_FRONT, 7, G_MOSS, 6);
        add_species(c, "koi", 2, 0);
        add_species(c, "koi", 1, 1);
        add_species(c, "koi", 1, 2);
        add_species(c, "goldfish", 2, 0);
        add_species(c, "medaka", 10, 0);
        add_species(c, "ramshorn", 3, 0);
        add_species(c, "ramshorn", 2, 1);
        break;
    }
    config_sanitize(c);
}

int theme_index(const char *theme_id)
{
    for (int i = 0; i < THEME_COUNT; i++)
        if (strcmp(theme_id, THEME_IDS[i]) == 0) return i;
    return -1;
}

void config_defaults(AquariumConfig *c)
{
    memset(c, 0, sizeof(*c));
    c->day_night_cycle = false;
    c->sim_speed = 100;
    c->fps_cap = 30;
    c->screen_off_minutes = 0;
    c->brightness = 100;
    c->creature_limit = 60;
    theme_apply(c, THEME_AMAZON);
}
