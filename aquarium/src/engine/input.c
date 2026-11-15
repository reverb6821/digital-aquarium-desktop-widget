#include "input.h"
#include <string.h>

#define TAP_MAX_MS 400
#define DOUBLE_TAP_MS 320
#define LONG_PRESS_MS 1000
#define MOVE_SLOP 5     // canvas px
#define SWIPE2_MIN 40   // canvas px

static void push(GestureState *g, GestureType type, int x, int y)
{
    uint8_t next = (uint8_t)((g->qh + 1) % GESTURE_QUEUE);
    if (next == g->qt) return; // full: drop
    g->q[g->qh] = (GestureEvent){type, (int16_t)x, (int16_t)y};
    g->qh = next;
}

void gesture_init(GestureState *g)
{
    memset(g, 0, sizeof(*g));
}

static int iabs(int v) { return v < 0 ? -v : v; }

void gesture_feed(GestureState *g, const HalTouch *t, uint32_t now)
{
    if (t->n > 0) {
        int x = t->x[0] / SCREEN_SCALE, y = t->y[0] / SCREEN_SCALE;
        if (!g->down) {
            g->down = true;
            g->moved = false;
            g->long_fired = false;
            g->two_finger = false;
            g->t_down = now;
            g->x0 = g->x = (int16_t)x;
            g->y0 = g->y = (int16_t)y;
            push(g, GE_DOWN, x, y);
        } else if (x != g->x || y != g->y) {
            g->x = (int16_t)x;
            g->y = (int16_t)y;
            if (iabs(x - g->x0) > MOVE_SLOP || iabs(y - g->y0) > MOVE_SLOP) g->moved = true;
            push(g, GE_MOVE, x, y);
        }
        if (t->n >= 2) {
            int cx = (t->x[0] + t->x[1]) / (2 * SCREEN_SCALE);
            int cy = (t->y[0] + t->y[1]) / (2 * SCREEN_SCALE);
            if (!g->two_finger) {
                g->two_finger = true;
                g->cx0 = (int16_t)cx;
                g->cy0 = (int16_t)cy;
            }
            g->cx = (int16_t)cx;
            g->cy = (int16_t)cy;
        }
        if (!g->moved && !g->long_fired && !g->two_finger && now - g->t_down >= LONG_PRESS_MS) {
            g->long_fired = true;
            push(g, GE_LONG_PRESS, g->x, g->y);
        }
        return;
    }

    if (!g->down) return;
    g->down = false;
    push(g, GE_UP, g->x, g->y);

    if (g->two_finger) {
        int dx = g->cx - g->cx0, dy = g->cy - g->cy0;
        if (iabs(dx) >= SWIPE2_MIN && iabs(dx) > iabs(dy)) push(g, dx < 0 ? GE_SWIPE2_LEFT : GE_SWIPE2_RIGHT, g->cx, g->cy);
        return;
    }
    if (g->long_fired || g->moved || now - g->t_down > TAP_MAX_MS) return;

    if (g->last_tap_t && now - g->last_tap_t < DOUBLE_TAP_MS && iabs(g->x - g->last_tap_x) < 16 &&
        iabs(g->y - g->last_tap_y) < 16) {
        push(g, GE_DOUBLE_TAP, g->x, g->y);
        g->last_tap_t = 0;
        return;
    }
    push(g, GE_TAP, g->x, g->y);
    g->last_tap_t = now ? now : 1;
    g->last_tap_x = g->x;
    g->last_tap_y = g->y;
}

bool gesture_pop(GestureState *g, GestureEvent *out)
{
    if (g->qt == g->qh) return false;
    *out = g->q[g->qt];
    g->qt = (uint8_t)((g->qt + 1) % GESTURE_QUEUE);
    return true;
}
