// Gesture recogniser: turns raw touch samples into tap / double tap /
// long press / drag / two-finger swipe events. Coordinates are CANVAS pixels.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "hal.h"

typedef enum {
    GE_NONE = 0,
    GE_DOWN,        // finger down
    GE_MOVE,        // finger moved (while down)
    GE_UP,          // finger released
    GE_TAP,
    GE_DOUBLE_TAP,
    GE_LONG_PRESS,
    GE_SWIPE2_LEFT,
    GE_SWIPE2_RIGHT,
} GestureType;

typedef struct {
    GestureType type;
    int16_t x, y;
} GestureEvent;

#define GESTURE_QUEUE 16

typedef struct {
    bool down;
    bool moved;
    bool long_fired;
    bool two_finger;
    uint32_t t_down;
    int16_t x0, y0, x, y;
    int16_t cx0, cy0, cx, cy; // two-finger centroid
    uint32_t last_tap_t;
    int16_t last_tap_x, last_tap_y;
    GestureEvent q[GESTURE_QUEUE];
    uint8_t qh, qt;
} GestureState;

void gesture_init(GestureState *g);
// Feed a raw sample (screen coordinates) taken at time `now`.
void gesture_feed(GestureState *g, const HalTouch *t, uint32_t now);
bool gesture_pop(GestureState *g, GestureEvent *out);
