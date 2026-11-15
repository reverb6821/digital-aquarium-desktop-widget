// I2C devices on GPIO8/9: CH422G IO expander + GT911 capacitive touch.
// The bus is shared by two tasks (touch polling on core 0, backlight changes
// from the render task), so every transaction takes a mutex.
#include "io.h"
#include <Arduino.h>
#include <Wire.h>
#include "board_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t s_i2c_mtx;
static uint8_t s_exio = EXIO_DEFAULT;
static uint8_t s_gt_addr;
static uint16_t s_gt_xmax = LCD_H_RES, s_gt_ymax = LCD_V_RES;

static bool i2c_lock(void) { return xSemaphoreTake(s_i2c_mtx, pdMS_TO_TICKS(50)) == pdTRUE; }
static void i2c_unlock(void) { xSemaphoreGive(s_i2c_mtx); }

static bool ch422g_write(uint8_t cmd_addr, uint8_t v)
{
    if (!i2c_lock()) return false;
    Wire.beginTransmission(cmd_addr);
    Wire.write(v);
    bool ok = Wire.endTransmission() == 0;
    i2c_unlock();
    return ok;
}

bool io_init(void)
{
    s_i2c_mtx = xSemaphoreCreateMutex();
    Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL, I2C_FREQ_HZ);

    // Touch reset sequence, replicated from the Waveshare demo
    // (waveshare_esp32_s3_touch_reset): INT low while RST rises -> GT911 @ 0x5D.
    bool ok = ch422g_write(CH422G_ADDR_MODE, 0x01); // all IO as outputs
    ch422g_write(CH422G_ADDR_OUT, 0x2C);            // TP_RST low, BL on, EXIO3/5 high
    delay(100);
    pinMode(TOUCH_PIN_INT, OUTPUT);
    digitalWrite(TOUCH_PIN_INT, LOW);
    delay(100);
    ch422g_write(CH422G_ADDR_OUT, 0x2E); // TP_RST high
    delay(200);
    pinMode(TOUCH_PIN_INT, INPUT); // release INT (GT911 drives it from now on)

    s_exio = EXIO_DEFAULT;
    ok = ok && ch422g_write(CH422G_ADDR_OUT, s_exio);
    if (!ok) Serial.println("[io] CH422G not responding");
    return ok;
}

void io_set_backlight(bool on)
{
    uint8_t v = on ? (uint8_t)(s_exio | EXIO_BL) : (uint8_t)(s_exio & ~EXIO_BL);
    if (v == s_exio) return;
    s_exio = v;
    ch422g_write(CH422G_ADDR_OUT, s_exio);
}

// ---------------------------------------------------------------- GT911

static bool gt_read(uint16_t reg, uint8_t *buf, size_t len)
{
    if (!i2c_lock()) return false;
    Wire.beginTransmission(s_gt_addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)reg);
    bool ok = Wire.endTransmission(false) == 0;
    if (ok) {
        size_t n = Wire.requestFrom((uint16_t)s_gt_addr, (uint8_t)len, true);
        ok = n == len;
        for (size_t i = 0; i < n; i++) buf[i] = (uint8_t)Wire.read();
    }
    i2c_unlock();
    return ok;
}

static bool gt_write8(uint16_t reg, uint8_t v)
{
    if (!i2c_lock()) return false;
    Wire.beginTransmission(s_gt_addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)reg);
    Wire.write(v);
    bool ok = Wire.endTransmission() == 0;
    i2c_unlock();
    return ok;
}

bool touch_init(void)
{
    const uint8_t addrs[2] = {GT911_ADDR, GT911_ADDR_ALT};
    uint8_t id[4] = {0};
    for (int i = 0; i < 2; i++) {
        s_gt_addr = addrs[i];
        if (gt_read(0x8140, id, 4)) break;
        s_gt_addr = 0;
    }
    if (!s_gt_addr) {
        Serial.println("[touch] GT911 not found at 0x5D/0x14");
        return false;
    }
    uint8_t res[4];
    if (gt_read(0x8048, res, 4)) {
        uint16_t xm = (uint16_t)(res[0] | (res[1] << 8)), ym = (uint16_t)(res[2] | (res[3] << 8));
        if (xm >= 100 && ym >= 100) {
            s_gt_xmax = xm;
            s_gt_ymax = ym;
        }
    }
    Serial.printf("[touch] GT911 id '%c%c%c%c' at 0x%02X, resolution %ux%u\n", id[0], id[1], id[2], id[3] ? id[3] : ' ',
                  s_gt_addr, s_gt_xmax, s_gt_ymax);
    return true;
}

int touch_read(HalTouch *out)
{
    if (!s_gt_addr) return -1;
    uint8_t status;
    if (!gt_read(0x814E, &status, 1)) return -1;
    if (!(status & 0x80)) return 0; // no new report: caller keeps the previous state
    out->n = 0;
    uint8_t n = status & 0x0F;
    if (n > 5) n = 0;
    uint8_t pts[16];
    uint8_t take = n > 2 ? 2 : n;
    if (take && !gt_read(0x814F, pts, (size_t)take * 8)) take = 0;
    gt_write8(0x814E, 0); // ack
    for (uint8_t i = 0; i < take; i++) {
        uint16_t x = (uint16_t)(pts[i * 8 + 1] | (pts[i * 8 + 2] << 8));
        uint16_t y = (uint16_t)(pts[i * 8 + 3] | (pts[i * 8 + 4] << 8));
        // scale if the controller is configured for a different resolution
        if (s_gt_xmax != LCD_H_RES) x = (uint16_t)((uint32_t)x * LCD_H_RES / s_gt_xmax);
        if (s_gt_ymax != LCD_V_RES) y = (uint16_t)((uint32_t)y * LCD_V_RES / s_gt_ymax);
        out->x[i] = (int16_t)(x < LCD_H_RES ? x : LCD_H_RES - 1);
        out->y[i] = (int16_t)(y < LCD_V_RES ? y : LCD_V_RES - 1);
    }
    out->n = take;
    return 1;
}
