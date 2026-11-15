// Desktop simulator (SDL2). Mouse = touch.
//   left click        tap            hold 1 s     long press (menu)
//   left drag         drag           right drag   two-finger swipe (theme)
//   F overlay (FPS)   S screenshot   1-6 biomes   M menu   Esc quit
#include <SDL.h>
#include <stdio.h>
#include <time.h>
#include "../aquarium/src/app/app.h"
#include "../aquarium/src/config/storage.h"
#include "../aquarium/src/engine/hal.h"
#include "png_write.h"

extern HalTouch g_sim_touch;
extern int g_sim_backlight;

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // nearest neighbour = pixel art
    SDL_Window *win = SDL_CreateWindow("Acquario digitale (simulatore)", SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, CANVAS_W * SCREEN_SCALE, CANVAS_H * SCREEN_SCALE, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *tex =
        SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, CANVAS_W, CANVAS_H);
    if (!win || !ren || !tex) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }

    static uint16_t px[CANVAS_W * CANVAS_H];
    Canvas cv;
    canvas_init(&cv, px, CANVAS_W, CANVAS_H);
    if (!app_init()) return 1;

    bool running = true, right = false;
    uint32_t fps_t0 = SDL_GetTicks(), frames = 0;
    float frame_ms = 0;
    while (running) {
        uint32_t t0 = SDL_GetTicks();
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN: {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_ESCAPE) running = false;
                else if (k == SDLK_s) {
                    char name[64];
                    snprintf(name, sizeof(name), "screenshot_%ld.png", (long)time(NULL));
                    if (png_write_rgb565(name, px, CANVAS_W, CANVAS_H, SCREEN_SCALE)) printf("saved %s\n", name);
                } else if (k >= SDLK_1 && k <= SDLK_6) app_key((char)('1' + (k - SDLK_1)));
                else if (k == SDLK_f) app_key('f');
                else if (k == SDLK_m) app_key('m');
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
                right = ev.button.button == SDL_BUTTON_RIGHT;
                g_sim_touch.n = right ? 2 : 1;
                g_sim_touch.x[0] = (int16_t)ev.button.x;
                g_sim_touch.y[0] = (int16_t)ev.button.y;
                g_sim_touch.x[1] = (int16_t)(ev.button.x + 60);
                g_sim_touch.y[1] = (int16_t)ev.button.y;
                break;
            case SDL_MOUSEMOTION:
                if (g_sim_touch.n) {
                    g_sim_touch.x[0] = (int16_t)ev.motion.x;
                    g_sim_touch.y[0] = (int16_t)ev.motion.y;
                    g_sim_touch.x[1] = (int16_t)(ev.motion.x + 60);
                    g_sim_touch.y[1] = (int16_t)ev.motion.y;
                }
                break;
            case SDL_MOUSEBUTTONUP: g_sim_touch.n = 0; break;
            default: break;
            }
        }

        uint32_t r0 = SDL_GetTicks();
        app_frame(&cv);
        frame_ms = frame_ms * 0.9f + (float)(SDL_GetTicks() - r0) * 0.1f;
        storage_poll(hal_millis());

        if (g_sim_backlight) SDL_UpdateTexture(tex, NULL, px, CANVAS_W * 2);
        else {
            static uint16_t black[CANVAS_W * CANVAS_H];
            SDL_UpdateTexture(tex, NULL, black, CANVAS_W * 2);
        }
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);

        frames++;
        uint32_t now = SDL_GetTicks();
        if (now - fps_t0 >= 1000) {
            app_report_stats((float)frames * 1000.0f / (float)(now - fps_t0), frame_ms);
            frames = 0;
            fps_t0 = now;
        }
        uint32_t period = app_frame_period_ms(), spent = SDL_GetTicks() - t0;
        if (spent < period) SDL_Delay(period - spent);
    }
    storage_flush();
    SDL_Quit();
    return 0;
}
