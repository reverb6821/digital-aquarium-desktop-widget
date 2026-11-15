// HAL implementation for the desktop simulator (SDL and headless builds).
#include "../aquarium/src/engine/hal.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

HalTouch g_sim_touch;             // written by the SDL front-end
uint32_t g_sim_fake_ms;           // headless: manual clock
int g_sim_use_fake_clock;

uint32_t hal_millis(void)
{
    if (g_sim_use_fake_clock) return g_sim_fake_ms;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

int hal_time_of_day_min(void)
{
    time_t now = time(NULL);
    struct tm lt;
    localtime_r(&now, &lt);
    return lt.tm_hour * 60 + lt.tm_min;
}

void hal_touch_read(HalTouch *t) { *t = g_sim_touch; }

int g_sim_backlight = 1;
void hal_backlight(bool on) { g_sim_backlight = on; }

void *hal_alloc_large(size_t bytes) { return calloc(1, bytes); }

void hal_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

void hal_lock(void) {}
void hal_unlock(void) {}

#define DATA_DIR "sim_data"

static void make_path(char *out, size_t cap, const char *name)
{
    mkdir(DATA_DIR, 0755);
    snprintf(out, cap, DATA_DIR "/%s", name[0] == '/' ? name + 1 : name);
}

bool hal_blob_save(const char *key, const void *data, size_t len)
{
    char path[256];
    char name[64];
    snprintf(name, sizeof(name), "%s.bin", key);
    make_path(path, sizeof(path), name);
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    bool ok = fwrite(data, 1, len, f) == len;
    fclose(f);
    return ok;
}

size_t hal_blob_load(const char *key, void *data, size_t cap)
{
    char path[256];
    char name[64];
    snprintf(name, sizeof(name), "%s.bin", key);
    make_path(path, sizeof(path), name);
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    size_t got = (n > 0 && (size_t)n <= cap) ? fread(data, 1, (size_t)n, f) : 0;
    fclose(f);
    return got == (size_t)n ? got : 0;
}

bool hal_file_write(const char *p, const char *data, size_t len)
{
    char path[256];
    make_path(path, sizeof(path), p);
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    bool ok = fwrite(data, 1, len, f) == len;
    fclose(f);
    return ok;
}

size_t hal_file_read(const char *p, char *buf, size_t cap)
{
    char path[256];
    make_path(path, sizeof(path), p);
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    size_t n = fread(buf, 1, cap - 1, f);
    buf[n] = 0;
    fclose(f);
    return n;
}

bool hal_file_remove(const char *p)
{
    char path[256];
    make_path(path, sizeof(path), p);
    return remove(path) == 0;
}
