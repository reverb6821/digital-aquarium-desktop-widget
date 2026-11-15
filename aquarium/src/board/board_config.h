// Waveshare ESP32-S3-Touch-LCD-4.3 — pins and timings.
// Verified against the official Waveshare demo (ESP-IDF/08_lvgl_Porting and
// Arduino/09_lvgl_Porting of the Waveshare ESP32-S3-Touch-LCD-4.3 demo package, https://docs.waveshare.com/ESP32-S3-Touch-LCD-4.3).
#pragma once

// ---------------------------------------------------------------- build options
// Rendering strategy (spec §4):
//   DISPLAY_MODE_A: double 800x480 framebuffer in PSRAM, canvas upscaled into it.
//   DISPLAY_MODE_B: no framebuffer, the bounce-buffer ISR upscales the 400x240
//                   canvas on the fly (much less PSRAM traffic). Default.
#define DISPLAY_MODE_A 1
#define DISPLAY_MODE_B 2
#ifndef DISPLAY_MODE
#define DISPLAY_MODE DISPLAY_MODE_B
#endif

// Set to 1 to boot into the Phase-1 bring-up test (colour bars, gradient,
// moving square, circle following the finger, FPS log) instead of the aquarium.
#ifndef BRINGUP_TEST
#define BRINGUP_TEST 0
#endif

// Bounce buffer height in display lines (must divide 480).
#define BOUNCE_LINES 20

// ---------------------------------------------------------------- LCD (RGB565)
#define LCD_H_RES 800
#define LCD_V_RES 480
#define LCD_PCLK_HZ (16 * 1000 * 1000) // demo value; refresh = 16e6/(820*500) ~= 39 Hz
#define LCD_HSYNC_PULSE 4
#define LCD_HSYNC_BP 8
#define LCD_HSYNC_FP 8
#define LCD_VSYNC_PULSE 4
#define LCD_VSYNC_BP 8
#define LCD_VSYNC_FP 8
#define LCD_PCLK_ACTIVE_NEG 1

#define LCD_PIN_HSYNC 46
#define LCD_PIN_VSYNC 3
#define LCD_PIN_DE 5
#define LCD_PIN_PCLK 7
// data_gpio_nums order: B0..B4, G0..G5, R0..R4 (B3..B7, G2..G7, R3..R7 on the panel)
#define LCD_DATA_PINS {14, 38, 18, 17, 10, 39, 0, 45, 48, 47, 21, 1, 2, 42, 41, 40}

// ---------------------------------------------------------------- I2C bus
#define I2C_PIN_SDA 8
#define I2C_PIN_SCL 9
#define I2C_FREQ_HZ 400000

// ---------------------------------------------------------------- CH422G IO expander
// The CH422G has no register map: each "address" is a command.
#define CH422G_ADDR_MODE 0x24 // write 0x01 = IO0..7 push-pull outputs
#define CH422G_ADDR_OUT 0x38  // write IO0..7 output levels
#define EXIO_TP_RST (1u << 1) // touch reset
#define EXIO_BL (1u << 2)     // backlight enable (on/off only)
#define EXIO_LCD_RST (1u << 3)
#define EXIO_SD_CS (1u << 4)  // SD card chip select (active low)
#define EXIO_USB_SEL (1u << 5)
// Normal running state = what the Waveshare demo writes after init (0x1E):
// TP_RST high, BL on, LCD_RST high, SD_CS high (deselected), EXIO5 low.
// [DA VERIFICARE] EXIO5 level for "USB mode": the demo leaves it low (0x1E)
// after init but sets it high during the touch reset (0x2C/0x2E).
#define EXIO_DEFAULT (EXIO_TP_RST | EXIO_BL | EXIO_LCD_RST | EXIO_SD_CS)

// ---------------------------------------------------------------- GT911 touch
#define TOUCH_PIN_INT 4
#define GT911_ADDR 0x5D     // INT held low during reset -> 0x5D
#define GT911_ADDR_ALT 0x14 // fallback

// ---------------------------------------------------------------- microSD (SPI)
#define SD_PIN_MOSI 11
#define SD_PIN_SCK 12
#define SD_PIN_MISO 13

// ---------------------------------------------------------------- tasks
#define RENDER_CORE 1
#define RENDER_PRIO 5
#define INPUT_CORE 0
#define INPUT_PRIO 4
#define STORAGE_CORE 0
#define STORAGE_PRIO 1
