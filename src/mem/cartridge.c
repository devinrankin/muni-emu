#include <stdio.h>
#include "cartridge.h"

#define CART_HEADER_ENTRY_POINT 0x0100
#define CART_HEADER_TITLE_BASE 0x0134
#define CART_HEADER_MANUFACTURER_CODE 0x013F
#define CART_HEADER_CGB_FLAG 0x0143
#define CART_HEADER_NEW_LICENSEE_CODE 0x0144
#define CART_HEADER_SGB_FLAG 0x0146
#define CART_HEADER_CartridgeYPE 0x0147
#define CART_HEADER_ROM_SIZE 0x0148
#define CART_HEADER_RAM_SIZE 0x0149
#define CART_HEADER_DEST_CODE 0x014A
#define CART_HEADER_OLD_LICENSEE_CODE 0x014B
#define CART_HEADER_MASK_ROM_VERSION 0x014C
#define CART_HEADER_HEADER_CHECKSUM 0x014D
#define CART_HEADER_GLOBAL_CHECKSUM 0x014E

#define CART_ROM_SIZE_32_KIB 0x00
#define CART_ROM_SIZE_64_KIB 0x01
#define CART_ROM_SIZE_128_KIB 0x02
#define CART_ROM_SIZE_256_KIB 0x03
#define CART_ROM_SIZE_512_KIB 0x04
#define CART_ROM_SIZE_1_MIB 0x05
#define CART_ROM_SIZE_2_MIB 0x06
#define CART_ROM_SIZE_4_MIB 0x07
#define CART_ROM_SIZE_8_MIB 0x08

#define CART_RAM_SIZE_NONE 0x00
#define CART_RAM_SIZE_8_KIB 0x02
#define CART_RAM_SIZE_32_KIB 0x03
#define CART_RAM_SIZE_128_KIB 0x04
#define CART_RAM_SIZE_64_KIB 0x05

#define CGB_FLAG_COMPATIBLE 0x80
#define CGB_FLAG_REQUIRED 0xC0

static bool cartridge_load_rom(uint8_t *rom, const char *path);
static void cartridge_parse_header(CartridgeHeader *header, const uint8_t *rom);
static uint8_t cartridge_header_get_title_len(const uint8_t *rom);
static void cartridge_header_parse_title(CartridgeHeader *header, const uint8_t* rom);
static size_t cartridge_get_ram_size(uint8_t header_value);
static MbcType cartridge_identify_mbc_type(uint8_t type);

bool cartridge_init(Cartridge *cartridge, const char* path) {
    *cartridge = (Cartridge) {0};
    if(!cartridge_load_rom(cartridge->rom, path)) {
        return false;
    }
    cartridge_parse_header(&cartridge->header, cartridge->rom);
   
    /* ROM size calculation provided here: https://gbdev.io/pandocs/The_Cartridge_Header.html#0148--rom-size */
    cartridge->rom_size = (32 * SIZE_KIB) * (1 << cartridge->header.rom_size);
    cartridge->ram_size = cartridge_get_ram_size(cartridge->header.ram_size);
    cartridge->support_mode = BIT(cartridge->header.cgb_flag, 7); // Only bit 7 of the CGB flag determines the cartridge's supported mode.

    MbcType type = cartridge_identify_mbc_type(cartridge->header.cartridge_type);
    mbc_init(&cartridge->mbc, type, cartridge->rom, cartridge->rom_size, cartridge->ram, cartridge->ram_size);
    cartridge_display_info(cartridge);

    return true;
}

void cartridge_deinit(Cartridge *cartridge) {
    free(cartridge);
}

/* Prints cartridge header info */
void cartridge_display_info(Cartridge *cartridge) {
    printf("Displaying cartridge header information: \n");
    printf("\tTitle       : %s\n", cartridge->header.title);
    printf("\tMapper Type : %2.2x\n", cartridge->header.cartridge_type);
    printf("\tROM Size    : %lu B\n", cartridge->rom_size);
    printf("\tRAM Size    : %lu B\n", cartridge->ram_size);
}

/* Returns a byte from the cartridge using the member MBC's read. */
uint8_t cartridge_read(Cartridge *cartridge, uint16_t address) {
   return cartridge->mbc.read(&cartridge->mbc, address);
}

/* Writes a byte to the cartridge using the member MBC's write. */
void cartridge_write(Cartridge *cartridge, uint16_t address, uint8_t value) {
    cartridge->mbc.write(&cartridge->mbc, address, value);
}

/* Attempts to load a ROM at the given filepath. Returns false on a failure. */
static bool cartridge_load_rom(uint8_t* rom, const char* path) {
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }

    long file_size = ftell(file);

    if (file_size < 0 || (size_t)file_size > CART_MAX_ROM_SIZE) {
        fclose(file);
        return false;
    }

    rewind(file);

    size_t size = (size_t)file_size;

    if (fread(rom, 1, size, file) != size) {
        fclose(file);
        return false;
    }

    fclose(file);
    
    return true;
}

/* Parses ROM's header data and assigns values to the passed cartridge header's respective members. */
static void cartridge_parse_header(CartridgeHeader *header, const uint8_t *rom) {
    cartridge_header_parse_title(header, rom);
    header->cgb_flag = rom[CART_HEADER_CGB_FLAG];
    header->new_licensee_code = rom[CART_HEADER_NEW_LICENSEE_CODE];
    header->sgb_flag = rom[CART_HEADER_SGB_FLAG];
    header->cartridge_type = rom[CART_HEADER_CartridgeYPE]; 
    header->rom_size = rom[CART_HEADER_ROM_SIZE];
    header->ram_size = rom[CART_HEADER_RAM_SIZE];
    header->dest_code = rom[CART_HEADER_DEST_CODE];
    header->old_licensee_code = rom[CART_HEADER_OLD_LICENSEE_CODE];
    header->rom_version = rom[CART_HEADER_MASK_ROM_VERSION];
    header->checksum = rom[CART_HEADER_HEADER_CHECKSUM];
    header->global_checksum = (uint16_t)((rom[CART_HEADER_GLOBAL_CHECKSUM] << 8) | rom[CART_HEADER_GLOBAL_CHECKSUM + 1]);
}

/* Since the CGB flag address is part of the title on DMG systems, we need to check its value
 * to determine the length of the title. */
static uint8_t cartridge_header_get_title_len(const uint8_t *rom) {
    uint8_t cgb_flag = rom[CART_HEADER_CGB_FLAG];

    if (cgb_flag == CGB_FLAG_COMPATIBLE || cgb_flag == CGB_FLAG_REQUIRED) {
        return 11;
    }
    return 16;
}

/* Build the ROM title from the designated title header address space. */
static void cartridge_header_parse_title(CartridgeHeader *header, const uint8_t *rom) {
    uint8_t title_len = cartridge_header_get_title_len(rom);

    int idx = 0;
    for (uint16_t addr = CART_HEADER_TITLE_BASE; addr < CART_HEADER_TITLE_BASE + title_len - 1; addr++) {
        header->title[idx++] = rom[addr];
    }
    header->title[idx] = '\0';
}

static size_t cartridge_get_ram_size(uint8_t header_value) {
    switch (header_value) {
    case 0x00: return 0;
    case 0x02: return (8 * SIZE_KIB);
    case 0x03: return (32 * SIZE_KIB);
    case 0x04: return (128 * SIZE_KIB);
    case 0x05: return (64 * SIZE_KIB);
    default: NO_IMPL;
    }
}

static MbcType cartridge_identify_mbc_type(uint8_t type) {
    switch (type) {
    case 0x00:
    case 0x08:
    case 0x09:
        return MBC_TYPE_NONE;
    case 0x01:
    case 0x02:
    case 0x03:
        return MBC_TYPE_MBC1;
    case 0x05:
    case 0x06:
        return MBC_TYPE_MBC2;
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
        return MBC_TYPE_MBC3;
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
        return MBC_TYPE_MBC5;
    default:
        return MBC_TYPE_NONE;
    }
}
