// Headless end-to-end test of the portable app (no SDL): scripted touches
// drive the menu/edit mode, screenshots are written as PNG, and config
// persistence + JSON round-trip are checked. Exit code != 0 on failure.
//   ./aquarium_headless [outdir]
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../aquarium/src/app/app.h"
#include "../aquarium/src/config/storage.h"
#include "../aquarium/src/engine/hal.h"
#include "../aquarium/src/scene/scene.h"
#include "../aquarium/src/scene/themes.h"
#include "png_write.h"

extern HalTouch g_sim_touch;
extern uint32_t g_sim_fake_ms;
extern int g_sim_use_fake_clock;

static uint16_t px[CANVAS_W * CANVAS_H];
static Canvas cv;
static const char *outdir = ".";
static int failures;
static double total_ms;
static int total_frames;

static void frames(int n)
{
    for (int i = 0; i < n; i++) {
        g_sim_fake_ms += 33;
        clock_t t0 = clock();
        app_frame(&cv);
        total_ms += (double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC;
        total_frames++;
        storage_poll(hal_millis());
    }
}

// canvas coordinates -> screen touch
static void down(int x, int y)
{
    g_sim_touch.n = 1;
    g_sim_touch.x[0] = (int16_t)(x * 2);
    g_sim_touch.y[0] = (int16_t)(y * 2);
}
static void up(void) { g_sim_touch.n = 0; }
static void tap(int x, int y)
{
    down(x, y);
    frames(2);
    up();
    frames(12); // > double-tap window
}

static void shot(const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.png", outdir, name);
    png_write_rgb565(path, px, CANVAS_W, CANVAS_H, 2);
    printf("  screenshot %s\n", path);
}

static void check(bool ok, const char *what)
{
    printf("  [%s] %s\n", ok ? " OK " : "FAIL", what);
    if (!ok) failures++;
}

int main(int argc, char **argv)
{
    if (argc > 1) outdir = argv[1];
    g_sim_use_fake_clock = 1;
    g_sim_fake_ms = 1000;
    remove("sim_data/cfg.bin");
    remove("sim_data/active.json");
    canvas_init(&cv, px, CANVAS_W, CANVAS_H);
    check(app_init(), "app_init");
    frames(90);
    shot("01_scene");

    // JSON round trip
    {
        static char buf[8192];
        size_t n = config_to_json(app_cfg(), "test", buf, sizeof(buf));
        AquariumConfig c;
        memset(&c, 0, sizeof(c));
        char name[32];
        bool ok = n > 0 && config_from_json(&c, buf, name, sizeof(name));
        check(ok && strcmp(name, "test") == 0 && c.plant_count == app_cfg()->plant_count &&
                  c.species_count == app_cfg()->species_count && c.water_top == app_cfg()->water_top &&
                  c.substrate_seed == app_cfg()->substrate_seed,
              "JSON round-trip");
    }

    // feeding
    tap(100, 60);
    frames(30);
    shot("02_food");

    // long press opens the menu
    down(100, 100);
    frames(40);
    up();
    frames(3);
    shot("03_menu");
    static const char *pages[] = {"theme", "water", "substrate", "plants", "creatures", "env", "presets"};
    for (int p = 0; p < 7; p++) {
        tap(260, 21 + 4 + p * 24 + 8); // page buttons
        frames(2);
        char name[64];
        snprintf(name, sizeof(name), "04_page_%s", pages[p]);
        shot(name);
        tap(207, 10); // back
    }

    // change biome to blackwater from the biome page (thumbnail #2)
    tap(260, 21 + 4 + 0 * 24 + 8);
    tap(330, 50);
    frames(40); // fade out/in
    check(strcmp(app_cfg()->theme_id, "blackwater") == 0, "theme switch from menu (blackwater)");
    tap(388, 10); // close
    frames(30);
    shot("05_blackwater");

    // edit mode: drag the tall plant (index 1, x=246) to x~200
    down(100, 100);
    frames(40);
    up();
    frames(3);
    tap(260, 21 + 4 + 3 * 24 + 8); // "Piante e decor"
    tap(300, 33);                  // "Modalita' modifica"
    int16_t before = app_cfg()->plants[1].x;
    int fy = 150;
    down(246, fy);
    frames(3);
    for (int x = 246; x >= 200; x -= 4) {
        g_sim_touch.x[0] = (int16_t)(x * 2);
        frames(1);
    }
    frames(3);
    shot("06_edit_drag");
    up();
    frames(3);
    check(before == 246 && app_cfg()->plants[1].x < 215, "edit mode drag moves plant");
    tap(30, 226); // "Fatto"
    frames(5);
    int16_t moved = app_cfg()->plants[1].x;

    // double tap overlay
    down(150, 120); frames(1); up(); frames(2);
    down(150, 120); frames(1); up(); frames(5);
    shot("07_overlay");
    down(150, 120); frames(1); up(); frames(2);
    down(150, 120); frames(1); up(); frames(12);

    // persistence: wait for the debounced save, then "reboot"
    frames(90);
    check(app_init(), "re-init (reboot)");
    check(strcmp(app_cfg()->theme_id, "blackwater") == 0 && app_cfg()->plants[1].x == moved, "config restored after reboot");

    // two-finger swipe -> next biome (seasia)
    g_sim_touch.n = 2;
    for (int i = 0; i < 12; i++) {
        int cx = 250 - i * 8;
        g_sim_touch.x[0] = (int16_t)((cx - 20) * 2);
        g_sim_touch.x[1] = (int16_t)((cx + 20) * 2);
        g_sim_touch.y[0] = g_sim_touch.y[1] = 240;
        frames(1);
    }
    up();
    frames(60);
    check(strcmp(app_cfg()->theme_id, "seasia") == 0, "two-finger swipe -> next theme");
    shot("08_seasia");
    for (int b = 0; b < THEME_COUNT; b++) { // every biome preset
        app_key((char)('1' + b));
        frames(60);
        check(strcmp(app_cfg()->theme_id, THEME_IDS[b]) == 0, "biome hotkey");
        char name[64];
        snprintf(name, sizeof(name), "09_biome_%d_%s", b + 1, THEME_IDS[b]);
        shot(name);
    }

    // stress: creature limit 60 with many fish
    {
        AquariumConfig *c = app_cfg();
        theme_apply(c, THEME_AMAZON);
        c->species[0].count = 30;
        c->species[1].count = 20;
        c->species[2].count = 6;
        c->species[3].count = 12;
        app_config_changed(SC_ALL);
        frames(300);
        printf("  creatures in stress test: %d\n", scene_creature_count());
        check(scene_creature_count() == 60, "global creature limit (60)");
        shot("10_stress");
    }
    printf("  avg app_frame on host: %.3f ms\n", total_ms / total_frames);
    printf(failures ? "FAILED (%d)\n" : "ALL OK\n", failures);
    return failures ? 1 : 0;
}
