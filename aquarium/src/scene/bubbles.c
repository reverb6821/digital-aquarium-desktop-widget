// Bubble columns: hollow circles rising with a wobble, popping at the surface.
#include "../engine/fixmath.h"
#include "../engine/rng.h"
#include "world.h"

#define BUBBLE_MAX 96

typedef struct {
    float x, y, vy, ph, x0;
    uint8_t r, alive;
} Bubble;

static Bubble s_b[BUBBLE_MAX];
static float s_acc[CFG_MAX_BUBBLES];
static Rng s_rng;

void bubbles_init(void)
{
    rng_seed(&s_rng, 0xB0B1u);
    for (int i = 0; i < BUBBLE_MAX; i++) s_b[i].alive = 0;
    for (int i = 0; i < CFG_MAX_BUBBLES; i++) s_acc[i] = 0;
}

static void spawn(float x, float y, int r)
{
    for (int i = 0; i < BUBBLE_MAX; i++) {
        if (s_b[i].alive) continue;
        Bubble *b = &s_b[i];
        b->alive = 1;
        b->x = b->x0 = x;
        b->y = y;
        b->r = (uint8_t)r;
        b->vy = -(14.0f + 6.0f * (float)r + rng_frange(&s_rng, 0, 6));
        b->ph = rng_frange(&s_rng, 0, FM_TAU);
        return;
    }
}

void bubbles_burst(int x, int y, int n)
{
    for (int i = 0; i < n; i++) spawn((float)x + rng_frange(&s_rng, -3, 3), (float)y + rng_frange(&s_rng, -2, 2), rng_range(&s_rng, 0, 1));
}

void bubbles_update(const AquariumConfig *c, float dt)
{
    for (int s = 0; s < c->bubble_sources; s++) {
        s_acc[s] += dt * (float)c->bubbles[s].rate * 0.12f; // rate 100 -> 12 bubbles/s
        while (s_acc[s] >= 1.0f) {
            s_acc[s] -= 1.0f;
            int x = c->bubbles[s].x;
            spawn((float)x + rng_frange(&s_rng, -1.5f, 1.5f), (float)world_floor(x) - 1, rng_range(&s_rng, 0, 4) == 0 ? 2 : rng_range(&s_rng, 0, 1));
        }
    }
    for (int i = 0; i < BUBBLE_MAX; i++) {
        Bubble *b = &s_b[i];
        if (!b->alive) continue;
        b->ph += dt * 4.0f;
        b->y += b->vy * dt;
        b->x = b->x0 + fm_sin(b->ph) * 1.4f;
        if (b->y <= (float)SURFACE_Y + 1) b->alive = 0;
    }
}

void bubbles_draw(Canvas *cv)
{
    const uint16_t col = RGB565(190, 225, 235);
    for (int i = 0; i < BUBBLE_MAX; i++) {
        const Bubble *b = &s_b[i];
        if (!b->alive) continue;
        int x = (int)b->x, y = (int)b->y;
        if (b->r == 0) canvas_pblend(cv, x, y, col, 170);
        else {
            canvas_circle_blend(cv, x, y, b->r, col, 150);
            canvas_pblend(cv, x - 1, y - b->r + 1 + (b->r > 1), RGB565(255, 255, 255), 140);
        }
    }
}

// ---------------------------------------------------------------- food

Food g_food[FOOD_MAX];

void food_drop(int x, int y)
{
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < FOOD_MAX; i++) {
            if (g_food[i].alive) continue;
            Food *f = &g_food[i];
            f->alive = 1;
            f->resting = 0;
            f->x = (float)x + rng_frange(&s_rng, -4, 4);
            f->y = (float)(y < SURFACE_Y + 2 ? SURFACE_Y + 2 : y) + rng_frange(&s_rng, -2, 2);
            f->vy = rng_frange(&s_rng, 6, 11);
            f->age = 0;
            break;
        }
    }
}

void food_update(float dt)
{
    for (int i = 0; i < FOOD_MAX; i++) {
        Food *f = &g_food[i];
        if (!f->alive) continue;
        f->age += dt;
        if (f->age > 25.0f) { f->alive = 0; continue; }
        if (f->resting) continue;
        f->y += f->vy * dt;
        f->x += fm_sin(f->age * 2.0f + (float)i) * 3.0f * dt;
        float fl = (float)world_floor((int)f->x) - 1;
        if (f->y >= fl) { f->y = fl; f->resting = 1; }
    }
}

void food_draw(Canvas *cv)
{
    for (int i = 0; i < FOOD_MAX; i++) {
        const Food *f = &g_food[i];
        if (!f->alive) continue;
        uint8_t a = f->age > 22.0f ? (uint8_t)(255 * (25.0f - f->age) / 3.0f) : 255;
        canvas_pblend(cv, (int)f->x, (int)f->y, RGB565(210, 150, 80), a);
    }
}

int food_nearest(float x, float y, float radius)
{
    int best = -1;
    float bd = radius * radius;
    for (int i = 0; i < FOOD_MAX; i++) {
        if (!g_food[i].alive) continue;
        float dx = g_food[i].x - x, dy = g_food[i].y - y, d = dx * dx + dy * dy;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
