#include <SDL3/SDL.h>
#include <stdio.h>
#include "gui.h"
#include "common.h"

#define FRAME_NS 16742706ull

#define DEFAULT_SCREEN_WIDTH 1024
#define DEFAULT_SCREEN_HEIGHT 768

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static uint32_t pixels[LCD_XRES * LCD_YRES];
static uint64_t next_frame;

static inline uint32_t rgb555_to_argb(uint16_t c) {
    uint32_t r = c & 0x1F, g = (c >> 5) & 0x1F, b = (c >> 10) & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 3) | (g >> 2);
    b = (b << 3) | (b >> 2);
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

bool gui_init(void) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) goto fail;

    if (!SDL_CreateWindowAndRenderer(MUNI_PROJECT_NAME, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE, &window, &renderer))
        goto fail;

    SDL_SetRenderVSync(renderer, 0);
    SDL_SetRenderLogicalPresentation(renderer, LCD_XRES, LCD_YRES, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, LCD_XRES, LCD_YRES);
    if (!texture) goto fail;

    /* TODO: allow user to change scale mode */
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    return true;

fail:
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error initializing application: %s\n", SDL_GetError());
    gui_deinit();
    return false;
}

void gui_deinit() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool gui_poll_events() {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) return false;
        }
    }

    return true;
}

void gui_update(const uint16_t *framebuf) {
    for (int i = 0; i < LCD_XRES * LCD_YRES; i++) {
        pixels[i] = rgb555_to_argb(framebuf[i]);
    }

    SDL_UpdateTexture(texture, NULL, pixels, LCD_XRES * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void gui_frame_limit(void) {
    uint64_t now = SDL_GetTicksNS();

    if (next_frame == 0) {
        next_frame = now;
    }

    next_frame += FRAME_NS;

    if (now < next_frame) {
        SDL_DelayPrecise(next_frame - now);
    } else if (now - next_frame > 5 * FRAME_NS) {
        next_frame = now;
    }
}
void gui_show_fps(void) {
    static uint64_t last = 0;
    static int frames = 0;

    uint64_t now = SDL_GetTicksNS();
    frames++;

    if (now - last >= 1000000000ull) {
        char title[64];
        double fps = frames * 1e9 / (double)(now - last);
        snprintf(title, sizeof title, "%s - %.1f fps (%.0f%%)", MUNI_PROJECT_NAME, fps, fps / 59.7275 * 100.0);
        SDL_SetWindowTitle(window, title);
        frames = 0;
        last = now;
    }
}
