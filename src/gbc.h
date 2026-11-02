#ifndef GBC_H
#define GBC_H

#include "common.h"
#include "sm83/sm83.h"
#include "ppu/ppu.h"
#include "mem/cartridge.h"
#include "bus.h"

typedef struct Gbc {
    sm83_t sm83;
    ppu_t ppu;
    bus_t bus;
    cartridge_t cartridge;

    GbSystemType system;
} Gbc;

bool gbc_init(Gbc *gbc, GbSystemType system, const char* rom_path);
void gbc_step(Gbc *gbc);

#endif
