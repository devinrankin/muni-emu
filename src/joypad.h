#ifndef JOYPAD_H
#define JOYPAD_H

#include <stdint.h>
#include "bus.h"

/* P14 D-pad Selected */
#define JOYP_RIGHT 0x10
#define JOYP_LEFT 0x20
#define JOYP_UP 0x40
#define JOYP_DOWN 0x80

/* P15 Buttons Selected */
#define JOYP_A 0x01
#define JOYP_B 0x02
#define JOYP_SELECT 0x04
#define JOYP_START 0x08

void joypad_set(Bus *bus, uint8_t pressed);
uint8_t joypad_read(const Bus *bus);
void joypad_write(Bus *bus, uint8_t value);

#endif
