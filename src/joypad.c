#include "joypad.h"

static uint8_t joypad_lines(const Bus *bus);
static void joypad_update(Bus *bus);

void joypad_set(Bus *bus, uint8_t pressed) {
    if (pressed == bus->joyp_pressed) return;
    bus->joyp_pressed = pressed;
    joypad_update(bus);
}

uint8_t joypad_read(const Bus *bus) {
    return 0xC0 | (bus->p1 & 0x30) | joypad_lines(bus);
}

void joypad_write(Bus *bus, uint8_t value) {
    bus->p1 = value & 0x30;
    joypad_update(bus);
}

static uint8_t joypad_lines(const Bus *bus) {
    uint8_t lines = 0x0F;
    if (!(bus->p1 & 0x10)) lines &= ~(bus->joyp_pressed >> 4) & 0x0F; // D-pad select
    if (!(bus->p1 & 0x20)) lines &= ~(bus->joyp_pressed & 0x0F) & 0x0F; // Button select
    return lines;
}

static void joypad_update(Bus *bus) {
    uint8_t lines = joypad_lines(bus);
    if (bus->joyp_prev_lines & ~lines) bus_request_interrupt(bus, INTERRUPT_BIT_JOYPAD);
    bus->joyp_prev_lines = lines;
}
