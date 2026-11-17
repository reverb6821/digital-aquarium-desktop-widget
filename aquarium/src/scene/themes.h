// Built-in read-only biome presets (freshwater only).
#pragma once
#include "../config/config.h"
#include "../i18n/i18n.h"

typedef enum {
    THEME_AMAZON = 0, // Amazzonico
    THEME_BLACKWATER, // Acque nere
    THEME_SEASIA,     // Sud-Est asiatico
    THEME_AFRICA,     // Africa occidentale (Congo)
    THEME_MALAWI,     // Lago Malawi
    THEME_POND,       // Laghetto
    THEME_COUNT
} ThemeId;

extern const char *const THEME_IDS[THEME_COUNT];   // "amazon", ...
// Localised display name of a biome.
const char *theme_name(int id);

// Fill *c with the theme preset (environment fields are preserved).
void theme_apply(AquariumConfig *c, ThemeId id);
// Index of c->theme_id, or -1 for "custom".
int theme_index(const char *theme_id);
// A full default config (Amazzonico).
void config_defaults(AquariumConfig *c);

// Water palettes offered in the UI.
typedef struct {
    StrId name; // tr() it for display
    uint16_t top, bottom;
} WaterPalette;
#define WATER_PALETTE_COUNT 6
extern const WaterPalette WATER_PALETTES[WATER_PALETTE_COUNT];

// Colour swatches used by the UI colour picker.
#define SWATCH_COUNT 16
extern const uint16_t SWATCHES[SWATCH_COUNT];
