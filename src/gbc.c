#include "gbc.h"
#include "timer.h"

bool gbc_init(Gbc *gbc, GbSystemType system, const char* rom_path) {
    *gbc = (Gbc){0};

    if (!cartridge_init(&gbc->cartridge, rom_path)) {
        return false;
    }
    bus_init(&gbc->bus, &gbc->cartridge, &gbc->ppu, system);
    sm83_init(&gbc->sm83, &gbc->bus, system);
    ppu_init(&gbc->ppu, &gbc->bus);

    return true;
}

void gbc_step(Gbc *gbc) {
    uint32_t mcycles = sm83_step(&gbc->sm83);

    uint32_t dots = mcycles * BIT(gbc->bus.key1, 7) ? 2 : 4;
    for (uint32_t i = 0; i < dots; i++) {
        ppu_step(&gbc->ppu);
    }
}
