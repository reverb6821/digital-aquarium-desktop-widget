// Board I/O: shared I2C bus (mutex-protected), CH422G expander, GT911 touch.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../engine/hal.h"

bool io_init(void);              // I2C + CH422G + touch reset sequence
void io_set_backlight(bool on);  // EXIO2
bool touch_init(void);           // probe GT911 (after io_init)
// One poll (screen coordinates). 1 = new report in *out, 0 = no new report, -1 = error.
int touch_read(HalTouch *out);
