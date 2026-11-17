#include "app.h"
#include <stdio.h>
#include "../config/storage.h"
#include "../engine/font.h"
#include "../engine/hal.h"
#include "../engine/input.h"
#include "../i18n/i18n.h"
#include "../scene/scene.h"
#include "../scene/themes.h"
#include "../ui/ui.h"

#define SIM_DT (1.0f / 30.0f) // fixed simulation step

static AquariumConfig s_cfg;
static GestureState s_gest;
static bool s_overlay;
static float s_fps, s_frame_ms;
static uint32_t s_last_ms, s_last_touch_ms;
static float s_acc;
static bool s_screen_on = true;
static bool s_wake_swallow; // the touch that wakes the screen is not a tap

AquariumConfig *app_cfg(void) { return &s_cfg; }

bool app_init(void)
{
    if (!scene_init()) {
        hal_log("scene_init: out of memory");
        return false;
    }
    storage_init();
    config_defaults(&s_cfg);
    if (storage_load_active(&s_cfg)) hal_log("config: restored (theme %s)", s_cfg.theme_id);
    else hal_log("config: defaults");
    config_sanitize(&s_cfg);
    i18n_set_lang(s_cfg.language);
    scene_set_config(&s_cfg, false);
    gesture_init(&s_gest);
    ui_init();
    s_last_ms = s_last_touch_ms = hal_millis();
    hal_backlight(true);
    return true;
}

uint32_t app_frame_period_ms(void) { return 1000u / (s_cfg.fps_cap ? s_cfg.fps_cap : 30); }

void app_report_stats(float fps, float frame_ms)
{
    s_fps = fps;
    s_frame_ms = frame_ms;
}

void app_config_changed(unsigned flags)
{
    config_sanitize(&s_cfg);
    i18n_set_lang(s_cfg.language);
    if (flags) scene_config_changed(&s_cfg, flags);
    storage_mark_dirty(&s_cfg, hal_millis());
}

void app_load_config(const AquariumConfig *c)
{
    // keep the environment settings: they are not part of a scene preset
    AquariumConfig n = *c;
    n.sim_speed = s_cfg.sim_speed;
    n.fps_cap = s_cfg.fps_cap;
    n.screen_off_minutes = s_cfg.screen_off_minutes;
    n.brightness = s_cfg.brightness;
    n.creature_limit = s_cfg.creature_limit;
    n.language = s_cfg.language;
    config_sanitize(&n);
    s_cfg = n;
    scene_set_config(&s_cfg, true);
    storage_mark_dirty(&s_cfg, hal_millis());
}

void app_set_theme(int theme)
{
    AquariumConfig n = s_cfg;
    theme_apply(&n, (ThemeId)((theme + THEME_COUNT) % THEME_COUNT));
    app_load_config(&n);
}

static void cycle_theme(int dir)
{
    int cur = theme_index(s_cfg.theme_id);
    if (cur < 0) cur = 0;
    app_set_theme(cur + dir);
}

void app_key(char key)
{
    if (key >= '1' && key < '1' + THEME_COUNT) app_set_theme(key - '1');
    else if (key == 'f') s_overlay = !s_overlay;
    else if (key == 'm') {
        if (ui_is_open()) ui_close();
        else ui_open();
    }
}

static void handle_gesture(const GestureEvent *e)
{
    if (ui_gesture(e)) return; // the UI consumed it
    switch (e->type) {
    case GE_TAP: scene_tap(e->x, e->y); break;
    case GE_LONG_PRESS: ui_open(); break;
    case GE_DOUBLE_TAP: s_overlay = !s_overlay; break;
    case GE_SWIPE2_LEFT: cycle_theme(+1); break;
    case GE_SWIPE2_RIGHT: cycle_theme(-1); break;
    default: break;
    }
}

static void draw_overlay(Canvas *cv)
{
    char buf[64];
    int tod = hal_time_of_day_min();
    if (tod >= 0) snprintf(buf, sizeof(buf), "%02d:%02d", tod / 60, tod % 60);
    else snprintf(buf, sizeof(buf), "--:--");
    canvas_blend_rect(cv, 2, 2, 140, 22, RGB565(0, 0, 0), 150);
    font_draw(cv, 5, 4, buf, RGB565(220, 240, 240));
    snprintf(buf, sizeof(buf), "%.1f fps %.1fms n=%d", (double)s_fps, (double)s_frame_ms, scene_creature_count());
    font_draw(cv, 5, 14, buf, RGB565(150, 220, 200));
}

void app_frame(Canvas *cv)
{
    uint32_t now = hal_millis();
    HalTouch t;
    hal_touch_read(&t);

    // screen-off timer + wake on touch
    if (t.n > 0) {
        s_last_touch_ms = now;
        if (!s_screen_on) {
            s_screen_on = true;
            s_wake_swallow = true;
            hal_backlight(true);
        }
    }
    if (s_wake_swallow) {
        if (t.n == 0) s_wake_swallow = false;
        t.n = 0; // don't feed the waking touch to the gesture recogniser
    }
    if (s_screen_on && s_cfg.screen_off_minutes && !ui_is_open() &&
        now - s_last_touch_ms > (uint32_t)s_cfg.screen_off_minutes * 60000u) {
        s_screen_on = false;
        hal_backlight(false);
    }

    storage_hold(ui_is_open() || ui_is_editing(), now);
    gesture_feed(&s_gest, &t, now);
    GestureEvent e;
    while (gesture_pop(&s_gest, &e)) handle_gesture(&e);

    // fixed-timestep simulation (max 4 steps to avoid a spiral of death)
    float dt = (float)(now - s_last_ms) / 1000.0f;
    s_last_ms = now;
    if (dt > 0.25f) dt = 0.25f;
    s_acc += dt;
    int steps = 0;
    while (s_acc >= SIM_DT && steps < 4) {
        scene_update(SIM_DT);
        s_acc -= SIM_DT;
        steps++;
    }
    if (steps == 4) s_acc = 0;

    scene_render(cv);
    ui_draw(cv, now);
    if (s_overlay) draw_overlay(cv);
}
