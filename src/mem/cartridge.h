#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "mbc.h"
#include "../common.h"

#define CART_MAX_ROM_SIZE (8 * SIZE_MIB)
#define CART_MAX_RAM_SIZE (32 * SIZE_KIB)

#define CART_HEADER_TITLE_LEN 16

typedef enum {
    CARTRIDGE_SUPPORT_DMG,
    CARTRIDGE_SUPPORT_CGB
} CartridgeSupport;

typedef struct {
    char title[CART_HEADER_TITLE_LEN];
    uint8_t cgb_flag;
    uint8_t new_licensee_code;
    uint8_t sgb_flag;
    uint8_t cartridge_type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t dest_code;
    uint8_t old_licensee_code;
    uint8_t rom_version;
    uint8_t checksum;
    uint16_t global_checksum;
} CartridgeHeader;

typedef struct cartridge {
    CartridgeHeader header;
    Mbc mbc;

    uint8_t rom[CART_MAX_ROM_SIZE];
    uint8_t ram[CART_MAX_RAM_SIZE];
    
    size_t rom_size;
    size_t ram_size;

    CartridgeSupport support_mode;
} Cartridge;

bool cartridge_init(Cartridge *cartridge, const char* path);
void cartridge_deinit(Cartridge *cartridge);
void cartridge_display_info(Cartridge *cartridge);

uint8_t cartridge_read(Cartridge *cartridge, uint16_t address);
void cartridge_write(Cartridge *cartridge, uint16_t address, uint8_t value);

#endif
