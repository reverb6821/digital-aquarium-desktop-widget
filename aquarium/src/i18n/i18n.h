// UI translations: one string table per language, indexed by StrId. Portable.
// English is the default and the fallback for any string missing in another table.
// The pixel font is ASCII only (0x20..0x7E): no accented letters in translations.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { LANG_EN = 0, LANG_IT, LANG_COUNT } Lang;

typedef enum {
    // Page titles, same order as the UI Page enum
    S_PAGE_SETTINGS = 0,
    S_PAGE_BIOME,
    S_PAGE_WATER,
    S_PAGE_SUBSTRATE,
    S_PAGE_PLANTS,
    S_PAGE_CREATURES,
    S_PAGE_ENV,
    S_PAGE_PRESETS,

    // Dashboard tile titles (8, in tile order)
    S_TILE_BIOME,
    S_TILE_WATER,
    S_TILE_SUBSTRATE,
    S_TILE_PLANTS,
    S_TILE_CREATURES,
    S_TILE_ENV,
    S_TILE_PRESETS,
    S_TILE_EDIT,

    // PlantType order
    S_PLANT_FIRST,
    S_PLANT_LAST = S_PLANT_FIRST + 9,
    // DecorType order
    S_DECOR_FIRST,
    S_DECOR_LAST = S_DECOR_FIRST + 9,
    // SubstrateType order
    S_SUB_FIRST,
    S_SUB_LAST = S_SUB_FIRST + 4,
    // ThemeId order
    S_THEME_FIRST,
    S_THEME_LAST = S_THEME_FIRST + 5,

    // Water palettes (WATER_PALETTES order)
    S_WP_AMAZON,
    S_WP_BLACKWATER,
    S_WP_SEASIA,
    S_WP_CONGO,
    S_WP_MALAWI,
    S_WP_POND,

    // Species common names
    S_SP_NEON,
    S_SP_CARDINAL,
    S_SP_EMBER,
    S_SP_ANGELFISH,
    S_SP_DISCUS,
    S_SP_CORYDORAS,
    S_SP_HARLEQUIN,
    S_SP_GOURAMI,
    S_SP_KUHLI,
    S_SP_CONGO_TETRA,
    S_SP_KRIBENSIS,
    S_SP_SHRIMP,
    S_SP_ASSASSIN_SNAIL,
    S_SP_MBUNA,
    S_SP_AULONOCARA,
    S_SP_KOI,
    S_SP_GOLDFISH,
    S_SP_MEDAKA,
    S_SP_RAMSHORN,
    S_SP_BETTA,
    S_SP_WHITE_CLOUD,
    S_SP_RUMMY,
    S_SP_SNAKEHEAD,

    // Dashboard subtitles
    S_CUSTOM_THEME,
    S_CUSTOM_PALETTE,
    S_FMT_PLANTS_DECOR, // %d + %d
    S_FMT_ANIMALS,      // %d
    S_FMT_LIGHT, // %d (brightness %)
    S_FMT_SAVED, // %d
    S_MOVE_ROTATE,

    // Biome page
    S_USER_PRESETS,
    S_PRESET_LOADED,
    S_NO_PRESETS,

    // Water page
    S_PALETTE,
    S_SURFACE,
    S_DEPTH,
    S_EFFECTS,
    S_LIGHT_RAYS,
    S_PARTICLES,
    S_ANIMATED_SURFACE,
    S_BACKDROP,
    S_BG_FIRST,
    S_BG_LAST = S_BG_FIRST + 5,

    // Substrate page
    S_MATERIAL,
    S_COLOR,
    S_SHAPE,
    S_HEIGHT,
    S_ROUGHNESS,
    S_REGEN_PROFILE,

    // Plants page
    S_FRONT_CHIP,
    S_EDIT_MODE,
    S_ADD,
    S_PLANT_ADDED,
    S_DECOR_ADDED,
    S_LIMIT_REACHED,
    S_BUBBLES,
    S_SOURCES,
    S_FMT_ITEMS, // %d

    // Creatures page
    S_POPULATION,
    S_FMT_COLOR, // %d/%d
    S_ONE_COLOR,
    S_TOO_MANY_SPECIES,
    S_CREATURE_LIMIT,

    // Environment page
    S_LIGHT,
    S_BRIGHTNESS,
    S_SIMULATION,
    S_SPEED,
    S_SCREEN,
    S_MAX_FPS,
    S_FMT_SCREEN_OFF, // %d
    S_SCREEN_OFF_NEVER,
    S_NETWORK,
    S_WIFI_NOT_SET,
    S_LANGUAGE,

    // Presets page
    S_EMPTY_SLOT,
    S_SAVE,
    S_LOAD,
    S_DELETE,
    S_CUSTOM_PRESET,
    S_PRESET_SAVED,
    S_PRESET_DELETED,

    // Edit bar
    S_DONE,
    S_CANCEL,
    S_EDIT_HINT,
    S_FRONT,
    S_BACK,
    S_TO_FLOOR,

    S_COUNT
} StrId;

void i18n_set_lang(int lang); // out-of-range values select English
Lang i18n_lang(void);
const char *tr(StrId id);
const char *lang_code(Lang l); // short label for the language picker ("EN", "IT")

#ifdef __cplusplus
}
#endif
