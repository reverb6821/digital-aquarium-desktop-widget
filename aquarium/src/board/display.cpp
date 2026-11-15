// RGB LCD (esp_lcd_panel_rgb) with the 400x240 -> 800x480 2x upscale.
//
// Mode B (default): no frame buffer at all. The driver's bounce-buffer ISR
// (on_bounce_empty) expands the 400x240 canvas on the fly, so the PSRAM only
// has to deliver 192 KB per refresh instead of 768 KB. Two canvases are
// swapped exactly at the start of a refresh (pos_px == 0) -> no tearing.
//
// Mode A: classic double 800x480 frame buffer in PSRAM (+ bounce buffers as
// in the Waveshare demo); the canvas is upscaled into the back buffer.
#include "display.h"
#include <Arduino.h>
#include <string.h>
#include "board_config.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
extern "C" {
#include "../engine/canvas.h"
#include "../engine/hal.h"
}

static esp_lcd_panel_handle_t s_panel;
static SemaphoreHandle_t s_frame_sem;

#if DISPLAY_MODE == DISPLAY_MODE_B
static uint16_t *s_canvas[2];
static volatile uint16_t *s_scan;    // canvas currently read by the ISR
static volatile uint16_t *s_pending; // canvas waiting to be latched
static int s_back;                   // index of the canvas we render into

// Expand canvas rows into the bounce buffer. Runs in ISR context.
static bool IRAM_ATTR on_bounce_empty(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes,
                                      void *user_ctx)
{
    (void)panel;
    (void)user_ctx;
    BaseType_t woken = pdFALSE;
    if (pos_px == 0 && s_pending) { // start of a new refresh: latch the new frame
        s_scan = s_pending;
        s_pending = NULL;
        xSemaphoreGiveFromISR(s_frame_sem, &woken);
    }
    const uint16_t *src = (const uint16_t *)s_scan;
    uint32_t *dst = (uint32_t *)bounce_buf;
    int y = pos_px / LCD_H_RES;
    const int lines = len_bytes / (LCD_H_RES * 2);
    for (int l = 0; l < lines; l++, y++) {
        uint32_t *d = dst + l * (LCD_H_RES / 2);
        if (l > 0 && (y & 1)) { // odd display line = copy of the previous one
            const uint32_t *p = d - LCD_H_RES / 2;
            for (int i = 0; i < LCD_H_RES / 2; i++) d[i] = p[i];
            continue;
        }
        const uint32_t *s = (const uint32_t *)(src + (y >> 1) * CANVAS_W);
        for (int x = 0; x < CANVAS_W / 2; x++) {
            uint32_t two = s[x];
            uint32_t a = two & 0xFFFFu, b = two >> 16;
            d[2 * x] = a | (a << 16);
            d[2 * x + 1] = b | (b << 16);
        }
    }
    return woken == pdTRUE;
}
#else
static uint16_t *s_fb[2];
static uint16_t *s_work; // 400x240 render target
static int s_cur;        // fb currently shown

static bool IRAM_ATTR on_frame_done(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *e, void *ctx)
{
    (void)panel;
    (void)e;
    (void)ctx;
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_frame_sem, &woken);
    return woken == pdTRUE;
}
#endif

const char *display_mode_name(void)
{
    return DISPLAY_MODE == DISPLAY_MODE_B ? "B (bounce-buffer upscale)" : "A (PSRAM double framebuffer)";
}

bool display_init(void)
{
    s_frame_sem = xSemaphoreCreateBinary();
    esp_lcd_rgb_panel_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.clk_src = LCD_CLK_SRC_DEFAULT;
    cfg.timings.pclk_hz = LCD_PCLK_HZ;
    cfg.timings.h_res = LCD_H_RES;
    cfg.timings.v_res = LCD_V_RES;
    cfg.timings.hsync_pulse_width = LCD_HSYNC_PULSE;
    cfg.timings.hsync_back_porch = LCD_HSYNC_BP;
    cfg.timings.hsync_front_porch = LCD_HSYNC_FP;
    cfg.timings.vsync_pulse_width = LCD_VSYNC_PULSE;
    cfg.timings.vsync_back_porch = LCD_VSYNC_BP;
    cfg.timings.vsync_front_porch = LCD_VSYNC_FP;
    cfg.timings.flags.pclk_active_neg = LCD_PCLK_ACTIVE_NEG;
    cfg.data_width = 16;
    cfg.bits_per_pixel = 16;
    cfg.hsync_gpio_num = LCD_PIN_HSYNC;
    cfg.vsync_gpio_num = LCD_PIN_VSYNC;
    cfg.de_gpio_num = LCD_PIN_DE;
    cfg.pclk_gpio_num = LCD_PIN_PCLK;
    cfg.disp_gpio_num = -1;
    const int pins[16] = LCD_DATA_PINS;
    for (int i = 0; i < 16; i++) cfg.data_gpio_nums[i] = pins[i];
    cfg.dma_burst_size = 64;
    cfg.bounce_buffer_size_px = LCD_H_RES * BOUNCE_LINES;

#if DISPLAY_MODE == DISPLAY_MODE_B
    for (int i = 0; i < 2; i++) {
        s_canvas[i] = (uint16_t *)heap_caps_aligned_calloc(64, CANVAS_W * CANVAS_H, 2, MALLOC_CAP_SPIRAM);
        if (!s_canvas[i]) return false;
    }
    s_scan = s_canvas[0];
    s_back = 1;
    cfg.num_fbs = 0;
    cfg.flags.no_fb = 1;
#else
    cfg.num_fbs = 2;
    cfg.flags.fb_in_psram = 1;
    s_work = (uint16_t *)heap_caps_calloc(CANVAS_W * CANVAS_H, 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!s_work) s_work = (uint16_t *)heap_caps_calloc(CANVAS_W * CANVAS_H, 2, MALLOC_CAP_SPIRAM);
    if (!s_work) return false;
#endif

    esp_err_t err = esp_lcd_new_rgb_panel(&cfg, &s_panel);
    if (err != ESP_OK) {
        Serial.printf("[lcd] esp_lcd_new_rgb_panel failed: %s\n", esp_err_to_name(err));
        return false;
    }
    esp_lcd_rgb_panel_event_callbacks_t cbs;
    memset(&cbs, 0, sizeof(cbs));
#if DISPLAY_MODE == DISPLAY_MODE_B
    cbs.on_bounce_empty = on_bounce_empty;
#else
    cbs.on_frame_buf_complete = on_frame_done;
#endif
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(s_panel, &cbs, NULL));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
#if DISPLAY_MODE == DISPLAY_MODE_A
    void *fb0 = NULL, *fb1 = NULL;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(s_panel, 2, &fb0, &fb1));
    s_fb[0] = (uint16_t *)fb0;
    s_fb[1] = (uint16_t *)fb1;
    s_cur = 0;
#endif
    Serial.printf("[lcd] mode %s, bounce %d lines\n", display_mode_name(), BOUNCE_LINES);
    return true;
}

uint16_t *display_begin_frame(void)
{
#if DISPLAY_MODE == DISPLAY_MODE_B
    return s_canvas[s_back];
#else
    return s_work;
#endif
}

void display_present(uint16_t *canvas)
{
#if DISPLAY_MODE == DISPLAY_MODE_B
    (void)canvas;
    xSemaphoreTake(s_frame_sem, 0); // clear a stale token
    s_pending = s_canvas[s_back];
    // wait until the ISR latched it (one refresh is ~26 ms)
    if (xSemaphoreTake(s_frame_sem, pdMS_TO_TICKS(100)) == pdTRUE) s_back ^= 1;
    else s_pending = NULL; // panel stalled: keep rendering into the same buffer
#else
    int next = s_cur ^ 1;
    canvas_upscale2x(s_fb[next], canvas, CANVAS_W, CANVAS_H);
    xSemaphoreTake(s_frame_sem, 0);
    // passing one of the driver's own frame buffers = switch, no copy
    esp_lcd_panel_draw_bitmap(s_panel, 0, 0, LCD_H_RES, LCD_V_RES, s_fb[next]);
    xSemaphoreTake(s_frame_sem, pdMS_TO_TICKS(100));
    s_cur = next;
#endif
}
