#pragma once
#include <stdbool.h>
#include <stdint.h>

// Write an RGB565 buffer as PNG, upscaled by an integer factor.
bool png_write_rgb565(const char *path, const uint16_t *px, int w, int h, int scale);
