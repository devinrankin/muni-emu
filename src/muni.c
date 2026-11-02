#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "gbc.h"
#include "gui.h"

int main(int argc, char* argv[]) {
    (void)argc;
    bool running = true;
    
    Gbc gbc;
    gbc_init(&gbc, SYSTEM_CGB, argv[1]);
    gui_init();

    while(running) {
        gbc_step(&gbc);
        if (gbc.ppu.frame_ready) {
            gbc.ppu.frame_ready = false;
            gui_update(gbc.ppu.framebuf);
            gui_show_fps();
            running = gui_poll_events();
            gui_frame_limit();
        }
    }

    return 0;
}
