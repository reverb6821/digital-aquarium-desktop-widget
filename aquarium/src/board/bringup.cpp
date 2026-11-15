// Phase-1 bring-up test (enable with BRINGUP_TEST=1 in board_config.h).
// Patterns (tap to advance, auto every 8 s):
//   0 colour bars   1 gradient: dithered (top) vs raw (bottom)
//   2 moving square + circle following the finger + corner targets
// With BRINGUP_STRESS=1 a core-0 task also runs Wi-Fi scans and NVS writes,
// to check that the image never drifts under bus/flash load.
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include "board_config.h"
#include "display.h"
#include "esp_heap_caps.h"
extern "C" {
#include "../engine/canvas.h"
#include "../engine/font.h"
#include "../engine/hal.h"
}

#ifndef BRINGUP_STRESS
#define BRINGUP_STRESS 1
#endif

static void stress_task(void *)
{
    Preferences p;
    p.begin("bench", false);
    WiFi.mode(WIFI_STA);
    uint32_t n = 0;
    for (;;) {
        WiFi.scanNetworks(true); // async scan: radio + core 0 busy
        for (int i = 0; i < 5; i++) {
            p.putUInt("n", n++); // flash write -> cache disabled for a moment
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        WiFi.scanDelete();
    }
}

static void draw_pattern(Canvas *c, int pat, uint32_t t, const HalTouch *tp)
{
    char buf[64];
    switch (pat) {
    case 0: {
        static const uint16_t bars[8] = {RGB565(255, 255, 255), RGB565(255, 255, 0), RGB565(0, 255, 255),
                                         RGB565(0, 255, 0),     RGB565(255, 0, 255), RGB565(255, 0, 0),
                                         RGB565(0, 0, 255),     RGB565(0, 0, 0)};
        for (int i = 0; i < 8; i++) canvas_fill_rect(c, i * CANVAS_W / 8, 0, CANVAS_W / 8, CANVAS_H, bars[i]);
        break;
    }
    case 1: {
        static const uint8_t B[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
        for (int y = 0; y < CANVAS_H; y++)
            for (int x = 0; x < CANVAS_W; x++) {
                // dark water gradient, left->right; top half dithered, bottom raw
                uint32_t g = (uint32_t)x * 60 * 16 / CANVAS_W; // 0..60 in 1/16 steps
                uint32_t q = y < CANVAS_H / 2 ? (g + B[y & 3][x & 3]) >> 4 : g >> 4;
                c->px[y * CANVAS_W + x] = RGB565(q * 2 / 3, q * 4 / 3 + 8, q * 2);
            }
        font_draw(c, 4, 4, "dithered", RGB565(255, 255, 255));
        font_draw(c, 4, CANVAS_H / 2 + 4, "raw (banding)", RGB565(255, 255, 255));
        break;
    }
    default: {
        canvas_fill(c, RGB565(8, 20, 30));
        int sx = (int)((t / 8) % (CANVAS_W - 30));
        canvas_fill_rect(c, sx, 100, 30, 30, RGB565(240, 140, 40));
        // corner targets for touch accuracy
        const int cx[4] = {6, CANVAS_W - 7, 6, CANVAS_W - 7}, cy[4] = {6, 6, CANVAS_H - 7, CANVAS_H - 7};
        for (int i = 0; i < 4; i++) canvas_circle(c, cx[i], cy[i], 4, RGB565(255, 255, 255));
        for (int i = 0; i < tp->n; i++) {
            int x = tp->x[i] / 2, y = tp->y[i] / 2;
            canvas_circle(c, x, y, 12, RGB565(60, 230, 220));
            canvas_hline(c, x - 4, x + 4, y, RGB565(255, 255, 255));
            canvas_vline(c, x, y - 4, y + 4, RGB565(255, 255, 255));
            snprintf(buf, sizeof(buf), "P%d %d,%d", i, tp->x[i], tp->y[i]);
            font_draw(c, 120, 30 + i * 10, buf, RGB565(255, 255, 255));
        }
        break;
    }
    }
}

void bringup_render_task(void *)
{
#if BRINGUP_STRESS
    xTaskCreatePinnedToCore(stress_task, "stress", 6144, NULL, 2, NULL, 0);
#endif
    Canvas c;
    int pat = 0;
    bool was_down = false;
    uint32_t t_pat = millis(), t_stat = millis(), frames = 0;
    float render_ms = 0, fps = 0;
    char buf[80];
    for (;;) {
        uint32_t t0 = micros();
        HalTouch tp;
        hal_touch_read(&tp);
        if (was_down && tp.n == 0) { // tap -> next pattern
            pat = (pat + 1) % 3;
            t_pat = millis();
        }
        was_down = tp.n > 0;
        if (millis() - t_pat > 8000 && pat != 2) {
            pat = (pat + 1) % 3;
            t_pat = millis();
        }
        uint16_t *fb = display_begin_frame();
        canvas_init(&c, fb, CANVAS_W, CANVAS_H);
        draw_pattern(&c, pat, millis(), &tp);
        snprintf(buf, sizeof(buf), "%s %.1ffps %.1fms", DISPLAY_MODE == DISPLAY_MODE_B ? "B" : "A", fps, render_ms);
        canvas_fill_rect(&c, 0, CANVAS_H - 10, font_width(buf) + 4, 10, 0);
        font_draw(&c, 2, CANVAS_H - 9, buf, RGB565(255, 255, 0));
        uint32_t t1 = micros();
        display_present(fb);
        render_ms = render_ms * 0.9f + (float)(t1 - t0) / 1000.0f * 0.1f;
        frames++;
        if (millis() - t_stat >= 2000) {
            fps = (float)frames * 1000.0f / (float)(millis() - t_stat);
            Serial.printf("[bench] mode %s pattern %d fps=%.1f draw=%.2fms psram_free=%u\n", display_mode_name(), pat,
                          fps, render_ms, (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
            frames = 0;
            t_stat = millis();
        }
        vTaskDelay(1);
    }
}
