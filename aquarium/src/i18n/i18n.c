#include "i18n.h"
#include <stddef.h>

// clang-format off
static const char *const STR_EN[S_COUNT] = {
    [S_PAGE_SETTINGS] = "Settings", [S_PAGE_BIOME] = "Biome", [S_PAGE_WATER] = "Water",
    [S_PAGE_SUBSTRATE] = "Substrate", [S_PAGE_PLANTS] = "Plants & decor", [S_PAGE_CREATURES] = "Creatures",
    [S_PAGE_ENV] = "Environment", [S_PAGE_PRESETS] = "Presets",

    [S_TILE_BIOME] = "Biome", [S_TILE_WATER] = "Water", [S_TILE_SUBSTRATE] = "Substrate", [S_TILE_PLANTS] = "Plants",
    [S_TILE_CREATURES] = "Creatures", [S_TILE_ENV] = "Environment", [S_TILE_PRESETS] = "Presets",
    [S_TILE_EDIT] = "Edit",

    [S_PLANT_FIRST + 0] = "Stem plant", [S_PLANT_FIRST + 1] = "Vallisneria", [S_PLANT_FIRST + 2] = "Cryptocoryne",
    [S_PLANT_FIRST + 3] = "Microsorum", [S_PLANT_FIRST + 4] = "Bolbitis", [S_PLANT_FIRST + 5] = "Anubias",
    [S_PLANT_FIRST + 6] = "Bucephalandra", [S_PLANT_FIRST + 7] = "Echinodorus", [S_PLANT_FIRST + 8] = "Java moss",
    [S_PLANT_FIRST + 9] = "Water lily",

    [S_DECOR_FIRST + 0] = "Large rock", [S_DECOR_FIRST + 1] = "Dragon stone", [S_DECOR_FIRST + 2] = "Seiryu stone",
    [S_DECOR_FIRST + 3] = "Stones", [S_DECOR_FIRST + 4] = "Slate", [S_DECOR_FIRST + 5] = "Driftwood",
    [S_DECOR_FIRST + 6] = "Spiderwood", [S_DECOR_FIRST + 7] = "Tall branch", [S_DECOR_FIRST + 8] = "Roots",
    [S_DECOR_FIRST + 9] = "Catappa leaves",

    [S_SUB_FIRST + 0] = "Sand", [S_SUB_FIRST + 1] = "Gravel", [S_SUB_FIRST + 2] = "Dark rock",
    [S_SUB_FIRST + 3] = "Pebbles", [S_SUB_FIRST + 4] = "None",

    [S_THEME_FIRST + 0] = "Amazon", [S_THEME_FIRST + 1] = "Blackwater", [S_THEME_FIRST + 2] = "Southeast Asia",
    [S_THEME_FIRST + 3] = "African Congo", [S_THEME_FIRST + 4] = "Lake Malawi", [S_THEME_FIRST + 5] = "Pond",

    [S_WP_AMAZON] = "Amazon", [S_WP_BLACKWATER] = "Blackwater (tannins)", [S_WP_SEASIA] = "Southeast Asia",
    [S_WP_CONGO] = "Congo", [S_WP_MALAWI] = "Malawi blue", [S_WP_POND] = "Pond green",

    [S_SP_NEON] = "Neon tetra", [S_SP_CARDINAL] = "Cardinal tetra", [S_SP_EMBER] = "Ember tetra",
    [S_SP_ANGELFISH] = "Angelfish", [S_SP_DISCUS] = "Discus", [S_SP_CORYDORAS] = "Corydoras",
    [S_SP_HARLEQUIN] = "Harlequin rasbora", [S_SP_GOURAMI] = "Gourami", [S_SP_KUHLI] = "Kuhli loach",
    [S_SP_CONGO_TETRA] = "Congo tetra", [S_SP_KRIBENSIS] = "Kribensis", [S_SP_SHRIMP] = "Caridina shrimp",
    [S_SP_ASSASSIN_SNAIL] = "Assassin snail", [S_SP_MBUNA] = "Mbuna cichlid", [S_SP_AULONOCARA] = "Peacock cichlid",
    [S_SP_KOI] = "Koi carp", [S_SP_GOLDFISH] = "Goldfish", [S_SP_MEDAKA] = "Medaka", [S_SP_RAMSHORN] = "Ramshorn snail", [S_SP_BETTA] = "Betta splendens",
    [S_SP_WHITE_CLOUD] = "White cloud minnow", [S_SP_RUMMY] = "Rummy-nose tetra", [S_SP_SNAKEHEAD] = "Snakehead",

    [S_CUSTOM_THEME] = "Custom", [S_CUSTOM_PALETTE] = "Custom", [S_FMT_PLANTS_DECOR] = "%d + %d decor",
    [S_FMT_ANIMALS] = "%d animals", [S_FMT_LIGHT] = "Light %d%%",
    [S_FMT_SAVED] = "%d saved", [S_MOVE_ROTATE] = "Move & rotate",

    [S_USER_PRESETS] = "USER PRESETS", [S_PRESET_LOADED] = "Preset loaded", [S_NO_PRESETS] = "No saved presets",

    [S_PALETTE] = "PALETTE", [S_SURFACE] = "SURFACE", [S_DEPTH] = "DEPTH", [S_EFFECTS] = "EFFECTS",
    [S_LIGHT_RAYS] = "Light rays", [S_PARTICLES] = "Particles", [S_ANIMATED_SURFACE] = "Animated surface",
    [S_BACKDROP] = "BACKGROUND", [S_BG_FIRST + 0] = "Open water", [S_BG_FIRST + 1] = "Rockwork",
    [S_BG_FIRST + 2] = "Rice paddy", [S_BG_FIRST + 3] = "Swamp", [S_BG_FIRST + 4] = "Flooded forest",
    [S_BG_FIRST + 5] = "Reed bed",

    [S_MATERIAL] = "MATERIAL", [S_COLOR] = "COLOR", [S_SHAPE] = "SHAPE", [S_HEIGHT] = "Height",
    [S_ROUGHNESS] = "Roughness", [S_REGEN_PROFILE] = "Regenerate profile",

    [S_FRONT_CHIP] = "fr.", [S_EDIT_MODE] = "Edit mode", [S_ADD] = "ADD", [S_PLANT_ADDED] = "Plant added",
    [S_DECOR_ADDED] = "Decoration added", [S_LIMIT_REACHED] = "Limit reached", [S_BUBBLES] = "BUBBLES",
    [S_SOURCES] = "Sources", [S_FMT_ITEMS] = "ITEMS (%d)",

    [S_POPULATION] = "Population", [S_FMT_COLOR] = "Color %d/%d", [S_ONE_COLOR] = "1 color",
    [S_TOO_MANY_SPECIES] = "Too many species", [S_CREATURE_LIMIT] = "Creature limit",

    [S_LIGHT] = "LIGHT", [S_BRIGHTNESS] = "Brightness %",
    [S_SIMULATION] = "SIMULATION", [S_SPEED] = "Speed %", [S_SCREEN] = "SCREEN", [S_MAX_FPS] = "Max FPS",
    [S_FMT_SCREEN_OFF] = "Screen off after %d min", [S_SCREEN_OFF_NEVER] = "Screen off: never",
    [S_NETWORK] = "NETWORK", [S_WIFI_NOT_SET] = "Wi-Fi/NTP: not configured", [S_LANGUAGE] = "LANGUAGE",

    [S_EMPTY_SLOT] = "Empty slot", [S_SAVE] = "Save", [S_LOAD] = "Load", [S_DELETE] = "Delete",
    [S_CUSTOM_PRESET] = "Custom", [S_PRESET_SAVED] = "Preset saved", [S_PRESET_DELETED] = "Preset deleted",

    [S_DONE] = "Done", [S_CANCEL] = "Cancel", [S_EDIT_HINT] = "Tap and drag plants/decor", [S_FRONT] = "Front",
    [S_BACK] = "Back", [S_TO_FLOOR] = "Floor",
};

static const char *const STR_IT[S_COUNT] = {
    [S_PAGE_SETTINGS] = "Impostazioni", [S_PAGE_BIOME] = "Bioma", [S_PAGE_WATER] = "Acqua",
    [S_PAGE_SUBSTRATE] = "Fondale", [S_PAGE_PLANTS] = "Piante e decor", [S_PAGE_CREATURES] = "Creature",
    [S_PAGE_ENV] = "Ambiente", [S_PAGE_PRESETS] = "Preset",

    [S_TILE_BIOME] = "Bioma", [S_TILE_WATER] = "Acqua", [S_TILE_SUBSTRATE] = "Fondale", [S_TILE_PLANTS] = "Piante",
    [S_TILE_CREATURES] = "Creature", [S_TILE_ENV] = "Ambiente", [S_TILE_PRESETS] = "Preset",
    [S_TILE_EDIT] = "Modifica",

    [S_PLANT_FIRST + 0] = "Pianta a stelo", [S_PLANT_FIRST + 1] = "Vallisneria", [S_PLANT_FIRST + 2] = "Cryptocoryne",
    [S_PLANT_FIRST + 3] = "Microsorum", [S_PLANT_FIRST + 4] = "Bolbitis", [S_PLANT_FIRST + 5] = "Anubias",
    [S_PLANT_FIRST + 6] = "Bucephalandra", [S_PLANT_FIRST + 7] = "Echinodorus", [S_PLANT_FIRST + 8] = "Muschio di Java",
    [S_PLANT_FIRST + 9] = "Ninfea",

    [S_DECOR_FIRST + 0] = "Roccia grande", [S_DECOR_FIRST + 1] = "Dragon stone", [S_DECOR_FIRST + 2] = "Seiryu stone",
    [S_DECOR_FIRST + 3] = "Sassi", [S_DECOR_FIRST + 4] = "Ardesia", [S_DECOR_FIRST + 5] = "Tronco",
    [S_DECOR_FIRST + 6] = "Spiderwood", [S_DECOR_FIRST + 7] = "Ramo alto", [S_DECOR_FIRST + 8] = "Radici",
    [S_DECOR_FIRST + 9] = "Foglie catappa",

    [S_SUB_FIRST + 0] = "Sabbia", [S_SUB_FIRST + 1] = "Ghiaia", [S_SUB_FIRST + 2] = "Roccia scura",
    [S_SUB_FIRST + 3] = "Ciottoli", [S_SUB_FIRST + 4] = "Nessuno",

    [S_THEME_FIRST + 0] = "Amazzonico", [S_THEME_FIRST + 1] = "Acque nere", [S_THEME_FIRST + 2] = "Sud-Est Asia",
    [S_THEME_FIRST + 3] = "Africa Congo", [S_THEME_FIRST + 4] = "Lago Malawi", [S_THEME_FIRST + 5] = "Laghetto",

    [S_WP_AMAZON] = "Amazzonico", [S_WP_BLACKWATER] = "Acque nere (tannini)", [S_WP_SEASIA] = "Sud-Est Asia",
    [S_WP_CONGO] = "Congo", [S_WP_MALAWI] = "Blu Malawi", [S_WP_POND] = "Verde laghetto",

    [S_SP_NEON] = "Neon", [S_SP_CARDINAL] = "Cardinale", [S_SP_EMBER] = "Pesce brace",
    [S_SP_ANGELFISH] = "Scalare", [S_SP_DISCUS] = "Discus", [S_SP_CORYDORAS] = "Corydoras",
    [S_SP_HARLEQUIN] = "Rasbora arlecchino", [S_SP_GOURAMI] = "Gourami", [S_SP_KUHLI] = "Pangio kuhlii",
    [S_SP_CONGO_TETRA] = "Tetra del Congo", [S_SP_KRIBENSIS] = "Pelvicachromis", [S_SP_SHRIMP] = "Caridina",
    [S_SP_ASSASSIN_SNAIL] = "Lumaca killer", [S_SP_MBUNA] = "Ciclide mbuna", [S_SP_AULONOCARA] = "Ciclide pavone",
    [S_SP_KOI] = "Carpa koi", [S_SP_GOLDFISH] = "Pesce rosso", [S_SP_MEDAKA] = "Medaka",
    [S_SP_RAMSHORN] = "Planorbarius", [S_SP_BETTA] = "Betta splendens",
    [S_SP_WHITE_CLOUD] = "Tanichthys", [S_SP_RUMMY] = "Tetra rummy nose", [S_SP_SNAKEHEAD] = "Pesce testa di serpente",

    [S_CUSTOM_THEME] = "Personale", [S_CUSTOM_PALETTE] = "Personalizzata", [S_FMT_PLANTS_DECOR] = "%d + %d decor",
    [S_FMT_ANIMALS] = "%d animali", [S_FMT_LIGHT] = "Luce %d%%",
    [S_FMT_SAVED] = "%d salvati", [S_MOVE_ROTATE] = "Sposta e ruota",

    [S_USER_PRESETS] = "PRESET UTENTE", [S_PRESET_LOADED] = "Preset caricato",
    [S_NO_PRESETS] = "Nessun preset salvato",

    [S_PALETTE] = "PALETTE", [S_SURFACE] = "SUPERFICIE", [S_DEPTH] = "PROFONDITA'", [S_EFFECTS] = "EFFETTI",
    [S_LIGHT_RAYS] = "Raggi di luce", [S_PARTICLES] = "Particelle", [S_ANIMATED_SURFACE] = "Superficie animata",
    [S_BACKDROP] = "SFONDO", [S_BG_FIRST + 0] = "Acqua libera", [S_BG_FIRST + 1] = "Rocciata",
    [S_BG_FIRST + 2] = "Risaia", [S_BG_FIRST + 3] = "Palude", [S_BG_FIRST + 4] = "Bosco allagato",
    [S_BG_FIRST + 5] = "Canneto",

    [S_MATERIAL] = "MATERIALE", [S_COLOR] = "COLORE", [S_SHAPE] = "FORMA", [S_HEIGHT] = "Altezza",
    [S_ROUGHNESS] = "Rugosita'", [S_REGEN_PROFILE] = "Rigenera profilo",

    [S_FRONT_CHIP] = "av.", [S_EDIT_MODE] = "Modalita' modifica", [S_ADD] = "AGGIUNGI",
    [S_PLANT_ADDED] = "Pianta aggiunta", [S_DECOR_ADDED] = "Decorazione aggiunta",
    [S_LIMIT_REACHED] = "Massimo raggiunto", [S_BUBBLES] = "BOLLE", [S_SOURCES] = "Sorgenti",
    [S_FMT_ITEMS] = "ELEMENTI (%d)",

    [S_POPULATION] = "Popolazione", [S_FMT_COLOR] = "Colore %d/%d", [S_ONE_COLOR] = "1 colore",
    [S_TOO_MANY_SPECIES] = "Troppe specie", [S_CREATURE_LIMIT] = "Limite creature",

    [S_LIGHT] = "LUCE", [S_BRIGHTNESS] = "Luminosita' %",
    [S_SIMULATION] = "SIMULAZIONE", [S_SPEED] = "Velocita' %", [S_SCREEN] = "SCHERMO", [S_MAX_FPS] = "FPS massimi",
    [S_FMT_SCREEN_OFF] = "Spegni dopo %d min", [S_SCREEN_OFF_NEVER] = "Spegnimento: mai",
    [S_NETWORK] = "RETE", [S_WIFI_NOT_SET] = "Wi-Fi/NTP: non configurato", [S_LANGUAGE] = "LINGUA",

    [S_EMPTY_SLOT] = "Slot libero", [S_SAVE] = "Salva", [S_LOAD] = "Carica", [S_DELETE] = "Elimina",
    [S_CUSTOM_PRESET] = "Custom", [S_PRESET_SAVED] = "Preset salvato", [S_PRESET_DELETED] = "Preset eliminato",

    [S_DONE] = "Fatto", [S_CANCEL] = "Annulla", [S_EDIT_HINT] = "Tocca e trascina piante/decor",
    [S_FRONT] = "Davanti", [S_BACK] = "Dietro", [S_TO_FLOOR] = "Fondo",
};
// clang-format on

static const char *const *const TABLES[LANG_COUNT] = {STR_EN, STR_IT};
static const char *const CODES[LANG_COUNT] = {"EN", "IT"};

static Lang s_lang = LANG_EN;

void i18n_set_lang(int lang) { s_lang = lang >= 0 && lang < LANG_COUNT ? (Lang)lang : LANG_EN; }

Lang i18n_lang(void) { return s_lang; }

const char *tr(StrId id)
{
    if ((unsigned)id >= S_COUNT) return "?";
    const char *s = TABLES[s_lang][id];
    if (!s) s = STR_EN[id];
    return s ? s : "?";
}

const char *lang_code(Lang l) { return (unsigned)l < LANG_COUNT ? CODES[l] : "?"; }
