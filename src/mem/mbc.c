#include <stdio.h>
#include "mbc.h"
#include "../common.h"


#define ADDR_ROM_BASE 0x0000
#define ADDR_ROM_SIZE 0x8000
#define ADDR_RAM_BASE 0xA000
#define ADDR_RAM_SIZE 0x2000

#define ADDR_ROM_BANK_BASE 0x4000
#define ADDR_ROM_BANK_SIZE 0x8000

#define ADDR_RAM_MBC2_SIZE 0x0200

#define ADDR_MBC1_RAM_ENABLE_BASE 0x0000
#define ADDR_MBC1_ROM_BANKNUM_BASE 0x2000
#define ADDR_MBC1_RAM_BANKNUM_BASE 0x4000
#define ADDR_MBC1_BANKMODE_SELECT_BASE 0x6000
#define ADDR_MBC1_REGISTER_SIZE 0x2000

#define ADDR_MBC2_RAM_ENABLE_BASE 0x0000
#define ADDR_MBC2_REGISTER_SIZE 0x4000

#define ADDR_MBC3_RTC_REG_SELECT_BASE 0xA000
#define ADDR_MBC3_RAM_TIM_ENABLE_BASE 0x0000
#define ADDR_MBC3_ROM_BANKNUM_BASE 0x2000
#define ADDR_MBC3_RAM_BANKNUM_BASE 0x4000
#define ADDR_MBC3_LATCH_CLOCKDATA_BASE 0x6000
#define ADDR_MBC3_REGISTER_SIZE 0x2000

static uint8_t mbc0_read(Mbc *mbc, uint16_t address);
static void mbc0_write(Mbc *mbc, uint16_t address, uint8_t value);

static uint8_t mbc1_read(Mbc *mbc, uint16_t address);
static void mbc1_write(Mbc *mbc, uint16_t address, uint8_t value);

static uint8_t mbc2_read(Mbc *mbc, uint16_t addr);
static void mbc2_write(Mbc *mbc, uint16_t address, uint8_t value);

static uint8_t mbc3_read(Mbc *mbc, uint16_t address);
static void mbc3_write(Mbc *mbc, uint16_t address, uint8_t value);

static uint8_t mbc5_read(Mbc *mbc, uint16_t address);
static void mbc5_write(Mbc *mbc, uint16_t address, uint8_t value);

static uint8_t mbc6_read(Mbc *mbc, uint16_t address);
static void mbc6_write(Mbc *mbc, uint16_t address, uint8_t value);

static uint8_t mbc7_read(Mbc *mbc, uint16_t address);
static void mbc7_write(Mbc *mbc, uint16_t address, uint8_t value);

static uint8_t rom_read(const Mbc *mbc, size_t offset);
static uint8_t ram_read(const Mbc *mbc, size_t offset);
static void ram_write(Mbc *mbc, size_t offset, uint8_t value);

static uint8_t rtc_read(Mbc *mbc, uint8_t register_id);
static void rtc_write(Mbc *mbc, uint8_t value);

void mbc_init(Mbc *mbc, MbcType type, const uint8_t *rom, size_t rom_size, uint8_t *ram, size_t ram_size) {
    *mbc = (Mbc){ 
        .type = type,
        .rom = rom,
        .rom_size = rom_size,
        .ram = ram,
        .ram_size = ram_size
    };

    switch(type) {
    case MBC_TYPE_NONE:
        mbc->read = mbc0_read;
        mbc->write = mbc0_write;
        break;
    case MBC_TYPE_MBC1:
        mbc->read = mbc1_read;
        mbc->write = mbc1_write;

        mbc->state.mbc1.rom_bank_low = 1;
        mbc->state.mbc1.banking_mode = 0;
        break;
    case MBC_TYPE_MBC2:
        mbc->read = mbc2_read;
        mbc->write = mbc2_write;

        mbc->state.mbc2.rom_bank = 1;
        break;
    case MBC_TYPE_MBC3:
        mbc->read = mbc3_read;
        mbc->write = mbc3_write;

        mbc->state.mbc3.rom_bank = 1;
        break;
    case MBC_TYPE_MBC5:
        mbc->read = mbc5_read;
        mbc->write = mbc5_write;

        mbc->state.mbc5.rom_bank = 1;
        break;
    case MBC_TYPE_MBC6:
        mbc->read = mbc6_read;
        mbc->write = mbc6_write;
        break;
    case MBC_TYPE_MBC7:
        mbc->read = mbc7_read;
        mbc->write = mbc7_write;
        break;
    default:
        mbc->read = mbc0_read;
        mbc->write = mbc0_write;
        break;
    }
}

void mbc_deinit(Mbc *mbc) {
    *mbc = (Mbc){0};
}

uint8_t mbc_read(Mbc *mbc, uint16_t address) {
    return mbc->read(mbc, address);
}

void mbc_write(Mbc *mbc, uint16_t address, uint8_t value) {
    mbc->write(mbc, address, value);
}

static uint8_t mbc0_read(Mbc *mbc, uint16_t address) {
    if (IN_RANGE(address, ADDR_ROM_BASE, ADDR_ROM_SIZE)) return rom_read(mbc, address);
    if (IN_RANGE(address, ADDR_RAM_BASE, ADDR_RAM_SIZE)) return ram_read(mbc, address - ADDR_RAM_BASE);
    return 0xFF;
}

static void mbc0_write(Mbc *mbc, uint16_t address, uint8_t value) {
    (void)mbc;
    (void)address;
    (void)value;
}

static uint8_t mbc1_read(Mbc *mbc, uint16_t address) {
    Mbc1State *state = &mbc->state.mbc1;

    size_t bank;
    size_t offset;

    if (IN_RANGE(address, ADDR_ROM_BASE, ADDR_ROM_BANK_SIZE)) {
        if (state->banking_mode == 0) bank = 0;
        else bank = (size_t)(state->bank_high & 0x03) << 5;

        offset = bank * ADDR_ROM_BANK_SIZE + address;
        return rom_read(mbc, offset);
    }

    if (IN_RANGE(address, ADDR_ROM_BANK_BASE, ADDR_ROM_BANK_SIZE)) {
        bank = ((size_t)(state->bank_high & 0x03) << 5) | (state->rom_bank_low & 0x1F);
        if ((bank & 0x1F) == 0) bank = 1;

        offset = bank * ADDR_ROM_BANK_SIZE + (address - ADDR_ROM_BANK_SIZE);
        return rom_read(mbc, offset);
    }

    if (IN_RANGE(address, ADDR_RAM_BASE, ADDR_RAM_SIZE)) {
        if (!state->ram_enabled) return 0xFF;
        if (state->banking_mode == 0) bank = 0;
        else bank = state->bank_high & 0x03;
        
        offset = bank * ADDR_RAM_SIZE + (address - ADDR_RAM_BASE);
        return ram_read(mbc, offset);
    }
    return 0xFF;
}

static void mbc1_write(Mbc *mbc, uint16_t address, uint8_t value) {
    Mbc1State *state = &mbc->state.mbc1;
    
    if (IN_RANGE(address, ADDR_MBC1_RAM_ENABLE_BASE, ADDR_MBC1_REGISTER_SIZE)) { state->ram_enabled = ((value & 0x0F) == 0x0A); return; }
    if (IN_RANGE(address, ADDR_MBC1_ROM_BANKNUM_BASE, ADDR_MBC1_REGISTER_SIZE)) { state->rom_bank_low = (value & 0x1F); return; }
    if (IN_RANGE(address, ADDR_MBC1_RAM_BANKNUM_BASE, ADDR_MBC1_REGISTER_SIZE)) { state->bank_high = value & 0x03; return; }
    if (IN_RANGE(address, ADDR_MBC1_BANKMODE_SELECT_BASE, ADDR_MBC1_REGISTER_SIZE)) { state->banking_mode = value & 0x01; return; }
    if (IN_RANGE(address, ADDR_RAM_BASE, ADDR_RAM_SIZE)) {
        if(!state->ram_enabled) return;
        size_t bank = 0;
        if(state->banking_mode == 1) bank = state->bank_high & 0x03;
        size_t offset = bank * ADDR_RAM_SIZE + (address - ADDR_RAM_BASE);

        ram_write(mbc, offset, value);
        return;
    }
}

static uint8_t mbc2_read(Mbc *mbc, uint16_t address) {
    Mbc2State *state = &mbc->state.mbc2;

    size_t bank;
    size_t offset;

    if (IN_RANGE(address, ADDR_ROM_BASE, ADDR_ROM_BANK_SIZE)) {
        offset = address;
        return rom_read(mbc, offset);
    }

    if (IN_RANGE(address, ADDR_ROM_BANK_BASE, ADDR_ROM_BANK_SIZE)) {
        bank = state->rom_bank & 0x0F;
        if(bank == 0) bank = 1;

        offset = bank * ADDR_ROM_BANK_SIZE + (address - 0x4000);
        return rom_read(mbc, offset);
    }

    if (IN_RANGE(address, ADDR_RAM_BASE, ADDR_RAM_SIZE)) {
        if(!state->ram_enabled) return 0xFF;
        offset = (address - ADDR_RAM_BASE) & 0x01FF;
        return 0xF0 | (ram_read(mbc, offset) & 0x0F);
    }

    return 0xFF;
}

static void mbc2_write(Mbc *mbc, uint16_t address, uint8_t value) {
    Mbc2State *state = &mbc->state.mbc2;

    if (IN_RANGE(address, ADDR_MBC2_RAM_ENABLE_BASE, ADDR_MBC2_REGISTER_SIZE)) {
        /* Masking the least significant bit of the upper address byte */
        if ((address & 0x0100) == 0) state->ram_enabled = ((value & 0x0F) == 0x0A); // Ram is enabled if and only if the lower 4 bits of value are $A
        else state->rom_bank = value & 0x0F;
        return;
    }

    if (IN_RANGE(address, ADDR_RAM_BASE, ADDR_RAM_SIZE)) {
        if(!state->ram_enabled) return;
        size_t offset = (address - ADDR_RAM_BASE) & 0x01FF;
        ram_write(mbc, offset, value & 0x0F);
    }
}

static uint8_t mbc3_read(Mbc *mbc, uint16_t address) {
    Mbc3State *state = &mbc->state.mbc3;
    
    size_t bank;
    size_t offset;

    if (IN_RANGE(address, ADDR_ROM_BASE, ADDR_ROM_BANK_SIZE)) {
        return rom_read(mbc, address);
    }

    if (IN_RANGE(address, ADDR_ROM_BANK_BASE, ADDR_ROM_BANK_SIZE)) {
        bank = state->rom_bank & 0x7F;
        if(bank == 0) bank = 1;

        offset = bank * ADDR_ROM_BANK_SIZE + (address - ADDR_ROM_BANK_SIZE);
        return rom_read(mbc, offset);
    }

    if (IN_RANGE(address, ADDR_RAM_BASE, ADDR_RAM_SIZE)) {
        if(!state->ram_enabled) return 0xFF;
        switch (state->ram_rtc_select) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
            offset = (size_t)state->ram_rtc_select * ADDR_RAM_SIZE + (address - ADDR_RAM_BASE);
            return ram_read(mbc, offset);
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
            return rtc_read(mbc, state->ram_rtc_select);
        default:
            return 0xFF;
        }
    }
    return 0xFF;
}

static void mbc3_write(Mbc *mbc, uint16_t address, uint8_t value) {
    Mbc3State *state = &mbc->state.mbc3;
    
    if (IN_RANGE(address, ADDR_MBC3_RAM_TIM_ENABLE_BASE, ADDR_MBC3_REGISTER_SIZE)) { state->ram_enabled = ((value & 0x0F) == 0x0A); return; }
    if (IN_RANGE(address, ADDR_MBC3_ROM_BANKNUM_BASE, ADDR_MBC3_REGISTER_SIZE)) { state->rom_bank = value & 0x7F; return; }
    if (IN_RANGE(address, ADDR_MBC3_RTC_REG_SELECT_BASE, ADDR_MBC3_REGISTER_SIZE)) { state->ram_rtc_select = value; return; }
    if (IN_RANGE(address, ADDR_MBC3_LATCH_CLOCKDATA_BASE, ADDR_MBC3_REGISTER_SIZE)) { /* TODO: implement RTC latch */ return; }
    if (IN_RANGE(address, ADDR_RAM_BASE, ADDR_RAM_SIZE)) {
        if(!state->ram_enabled) return;
        switch(state->ram_rtc_select) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03: {
            size_t offset = (size_t)state->ram_rtc_select * ADDR_RAM_SIZE + (address - ADDR_RAM_BASE);
            ram_write(mbc, offset, value); return;
        }
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
            rtc_write(mbc, value); return;
        default:
            return;
        }
    }
}

static uint8_t mbc5_read(Mbc *mbc, uint16_t address) {
    (void)mbc;
    (void)address;
    return 0;
}

static void mbc5_write(Mbc *mbc, uint16_t address, uint8_t value) {
    (void)mbc;
    (void)address;
    (void)value;   
}

static uint8_t mbc6_read(Mbc *mbc, uint16_t address) {
    (void)mbc;
    (void)address;
    return 0;
}

static void mbc6_write(Mbc *mbc, uint16_t address, uint8_t value) {
    (void)mbc;
    (void)address;
    (void)value;
}

static uint8_t mbc7_read(Mbc *mbc, uint16_t address) {
    (void)mbc;
    (void)address;
    return 0;
}

static void mbc7_write(Mbc *mbc, uint16_t address, uint8_t value) {
    (void)mbc;
    (void)address;
    (void)value;
}

static uint8_t rom_read(const Mbc *mbc, size_t offset) {
    if (offset >= mbc->rom_size) return 0xFF;
    return mbc->rom[offset];
}

static uint8_t ram_read(const Mbc *mbc, size_t offset) {
    if (mbc->ram == NULL || offset >= mbc->ram_size) return 0xFF;
    return mbc->ram[offset];
}

static void ram_write(Mbc *mbc, size_t offset, uint8_t value) {
    if (mbc->ram == NULL || offset >= mbc->ram_size) return;
    mbc->ram[offset] = value;
}

static uint8_t rtc_read(Mbc *mbc, uint8_t register_id) {
    Mbc3State *state = &mbc->state.mbc3;
    const Rtc* rtc = state->rtc_latched ? &state->latched_rtc : &state->current_rtc;

    switch(register_id) {
    case 0x08: return rtc->seconds;
    case 0x09: return rtc->minutes;
    case 0x0A: return rtc->hours;
    case 0x0B: return rtc->days_low;
    case 0x0C: return rtc->days_high;
    default: return 0xFF;
    }
}

static void rtc_write(Mbc *mbc, uint8_t value) {
    Mbc3State *state = &mbc->state.mbc3;

    if (state->rtc_latch_value == 0x00 && value == 0x01) state->latched_rtc = state->current_rtc;
    state->rtc_latch_value = value;
}
