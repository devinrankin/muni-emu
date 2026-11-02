#ifndef GUI_H
#define GUI_H

#include <stdint.h>

bool gui_init(void);
void gui_deinit(void);
bool gui_poll_events(void);
void gui_update(const uint16_t *framebuf);
void gui_frame_limit(void);
void gui_show_fps(void);

#endif
