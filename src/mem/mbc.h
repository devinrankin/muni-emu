#ifndef MBC_H
#define MBC_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <stdbool.h>

typedef enum {
    MBC_TYPE_NONE,
    MBC_TYPE_MBC1,
    MBC_TYPE_MBC2,
    MBC_TYPE_MBC3,
    MBC_TYPE_MBC5,
    MBC_TYPE_MBC6,
    MBC_TYPE_MBC7,
    MBC_TYPE_MMM01,
    MBC_TYPE_M161,
    MBC_TYPE_HUC1,
    MBC_TYPE_HUC3,
    MBC_TYPE_OTHER
} MbcType;

typedef struct rtc {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t days_low;
    uint8_t days_high;
} Rtc;

typedef struct {
    uint8_t rom_bank_low;
    uint8_t bank_high;
    uint8_t banking_mode;
    bool ram_enabled;
} Mbc1State;

typedef struct {
    uint8_t rom_bank;
    bool ram_enabled;
} Mbc2State;

typedef struct {
    uint8_t rom_bank;
    uint8_t ram_rtc_select;
    bool ram_enabled;

    Rtc current_rtc;
    Rtc latched_rtc;
   
    bool rtc_latched;
    uint8_t rtc_latch_value;
    uint64_t last_timestamp;
} Mbc3State;

typedef struct {
    uint16_t rom_bank;
    uint8_t ram_bank;
    bool ram_enabled;
} Mbc5State;

typedef struct Mbc Mbc;

struct Mbc {
    MbcType type;

    const uint8_t *rom;
    size_t rom_size;
    size_t rom_bank_mask;
    uint8_t *ram;
    size_t ram_size;
    size_t ram_bank_mask;

    uint8_t (*read)(Mbc *mbc, uint16_t address);
    void (*write)(Mbc *mbc, uint16_t address, uint8_t value);

    union {
        Mbc1State mbc1;
        Mbc2State mbc2;
        Mbc3State mbc3;
        Mbc5State mbc5;
    } state;
};

void mbc_init(Mbc *mbc, MbcType type, const uint8_t *rom, size_t rom_size, uint8_t *ram, size_t ram_size);
void mbc_deinit(Mbc *mbc);
uint8_t mbc_read(Mbc *mbc, uint16_t address);
void mbc_write(Mbc *mbc, uint16_t address, uint8_t value);

#endif
