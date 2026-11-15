// HAL implementation for the ESP32-S3 board + FreeRTOS tasks
// (input on core 0, storage on core 0, render on core 1).
#include "hal_esp32.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include "board_config.h"
#include "display.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "io.h"
extern "C" {
#include "../app/app.h"
#include "../config/storage.h"
#include "../engine/canvas.h"
#include "../engine/hal.h"
}

static portMUX_TYPE s_touch_mux = portMUX_INITIALIZER_UNLOCKED;
static HalTouch s_touch;
static SemaphoreHandle_t s_lock;
static Preferences s_prefs;
static bool s_fs_ok;

// ---------------------------------------------------------------- HAL (C API)

extern "C" uint32_t hal_millis(void) { return millis(); }

extern "C" int hal_time_of_day_min(void)
{
    time_t now = time(NULL);
    struct tm lt;
    localtime_r(&now, &lt);
    if (lt.tm_year + 1900 < 2024) return -1; // clock never set (no NTP yet)
    return lt.tm_hour * 60 + lt.tm_min;
}

extern "C" void hal_touch_read(HalTouch *t)
{
    portENTER_CRITICAL(&s_touch_mux);
    *t = s_touch;
    portEXIT_CRITICAL(&s_touch_mux);
}

extern "C" void hal_backlight(bool on) { io_set_backlight(on); }

extern "C" void *hal_alloc_large(size_t bytes)
{
    void *p = heap_caps_calloc(1, bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) p = calloc(1, bytes);
    return p;
}

extern "C" void hal_log(const char *fmt, ...)
{
    char buf[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Serial.println(buf);
}

extern "C" void hal_lock(void) { xSemaphoreTake(s_lock, portMAX_DELAY); }
extern "C" void hal_unlock(void) { xSemaphoreGive(s_lock); }

extern "C" bool hal_blob_save(const char *key, const void *data, size_t len)
{
    return s_prefs.putBytes(key, data, len) == len;
}

extern "C" size_t hal_blob_load(const char *key, void *data, size_t cap)
{
    size_t n = s_prefs.getBytesLength(key);
    if (n == 0 || n > cap) return 0;
    return s_prefs.getBytes(key, data, n) == n ? n : 0;
}

extern "C" bool hal_file_write(const char *path, const char *data, size_t len)
{
    if (!s_fs_ok) return false;
    File f = LittleFS.open(path, "w");
    if (!f) return false;
    bool ok = f.write((const uint8_t *)data, len) == len;
    f.close();
    return ok;
}

extern "C" size_t hal_file_read(const char *path, char *buf, size_t cap)
{
    if (!s_fs_ok || !LittleFS.exists(path)) return 0;
    File f = LittleFS.open(path, "r");
    if (!f) return 0;
    size_t n = f.read((uint8_t *)buf, cap - 1);
    buf[n] = 0;
    f.close();
    return n;
}

extern "C" bool hal_file_remove(const char *path) { return s_fs_ok && LittleFS.remove(path); }

// ---------------------------------------------------------------- tasks

static void input_task(void *)
{
    HalTouch t = {};
    for (;;) {
        int r = touch_read(&t);
        if (r > 0) {
            portENTER_CRITICAL(&s_touch_mux);
            s_touch = t;
            portEXIT_CRITICAL(&s_touch_mux);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz polling
    }
}

static void storage_task(void *)
{
    for (;;) {
        storage_poll(millis()); // debounced writes happen here, never on the render core
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

static void render_task(void *)
{
    Canvas cv;
    uint32_t frames = 0, t_stat = millis();
    float acc_render = 0, acc_present = 0;
    for (;;) {
        uint32_t t0 = micros();
        uint16_t *buf = display_begin_frame();
        canvas_init(&cv, buf, CANVAS_W, CANVAS_H);
        app_frame(&cv);
        uint32_t t1 = micros();
        display_present(buf);
        uint32_t t2 = micros();
        acc_render += (float)(t1 - t0) / 1000.0f;
        acc_present += (float)(t2 - t1) / 1000.0f;
        frames++;
        uint32_t now = millis();
        if (now - t_stat >= 5000) {
            float fps = (float)frames * 1000.0f / (float)(now - t_stat);
            app_report_stats(fps, acc_render / (float)frames);
            Serial.printf("[perf] mode %s  fps=%.1f  render=%.2fms  present-wait=%.2fms  heap int=%u psram=%u\n",
                          display_mode_name(), fps, acc_render / frames, acc_present / frames,
                          (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                          (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
            frames = 0;
            acc_render = acc_present = 0;
            t_stat = now;
        }
        // frame pacing (present already waited for the refresh)
        uint32_t spent = (micros() - t0) / 1000u, period = app_frame_period_ms();
        vTaskDelay(pdMS_TO_TICKS(spent + 1 < period ? period - spent : 1)); // always yield (watchdog)
    }
}

static bool s_lcd_ok;
static SemaphoreHandle_t s_lcd_done;

static void lcd_init_task(void *)
{
    // Created on core 0 so the LCD ISR (bounce-buffer fill) runs on core 0,
    // leaving core 1 to the renderer.
    s_lcd_ok = display_init();
    xSemaphoreGive(s_lcd_done);
    vTaskDelete(NULL);
}

bool board_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    Serial.printf("[board] PSRAM %u bytes, internal free %u\n", (unsigned)ESP.getPsramSize(),
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    io_init();
    touch_init();
    s_prefs.begin("aquarium", false);
    s_fs_ok = LittleFS.begin(true, "/littlefs", 4, "spiffs");
    if (!s_fs_ok) Serial.println("[fs] LittleFS mount failed (presets disabled)");

    s_lcd_done = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(lcd_init_task, "lcd_init", 4096, NULL, 10, NULL, 0);
    xSemaphoreTake(s_lcd_done, pdMS_TO_TICKS(5000));
    if (!s_lcd_ok) Serial.println("[lcd] init FAILED");
    return s_lcd_ok;
}

void board_start_tasks(void (*render_fn)(void *))
{
    xTaskCreatePinnedToCore(input_task, "input", 4096, NULL, INPUT_PRIO, NULL, INPUT_CORE);
    xTaskCreatePinnedToCore(storage_task, "storage", 6144, NULL, STORAGE_PRIO, NULL, STORAGE_CORE);
    xTaskCreatePinnedToCore(render_fn ? render_fn : render_task, "render", 12288, NULL, RENDER_PRIO, NULL,
                            RENDER_CORE);
}
