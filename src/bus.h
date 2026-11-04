#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include "mem/cartridge.h"

/*
     * Game Boy Color Memory Layout:
     *  $0000 - $3FFF ROM (bank 0)
     *  $4000 - $7FFF ROM (bank 1..n)
     *  $8000 - $9FFF VRAM (bank 0 non-CGB, bank 0-1 CGB)
     *  $A000 - $BFFF RAM (external from cartridge)
     *  $C000 - $CFFF WRAM (bank 0)
     *  $D000 - $DFFF WRAM (bank 1-7, CGB-only)
     *  $E000 - $FDFF Echo RAM
     *  $FE00 - $FE9F OAM (Object Attribute Memory)
     *  $FEA0 - $FEFF Not Usable
     *  $FF00 - $FF7F I/O Registers
     *  $FF80 - $FFFE HRAM (High RAM)
     *  $FFFF - $FFFF IE (Interrupt Enable Register)
*/

#define ADDR_ROM0_BASE 0x0000
#define ADDR_ROM0_SIZE 0x4000

/* Rom Banks 1-X */
#define ADDR_ROMX_BASE 0x4000
#define ADDR_ROMX_SIZE 0x4000

#define ADDR_VRAM_BASE 0x8000
#define ADDR_VRAM_SIZE 0x2000

/* External (Cartridge) RAM */
#define ADDR_EXRAM_BASE 0xA000
#define ADDR_EXRAM_SIZE 0x2000

#define ADDR_WRAM0_BASE 0xC000
#define ADDR_WRAM0_SIZE 0x1000

/* WRAM Banks 1-7 */
#define ADDR_WRAMX_BASE 0xD000
#define ADDR_WRAMX_SIZE 0x1000

#define ADDR_WRAM_BASE 0xC000
#define ADDR_WRAM_SIZE 0x2000

/* Echo RAM (mirror of C000 - DDFF) */
#define ADDR_ERAM_BASE 0xE000
#define ADDR_ERAM_SIZE 0x1E00

/* Object Attribute Memory */
#define ADDR_OAM_BASE 0xFE00
#define ADDR_OAM_SIZE 0x00A0

/* Not usable */
#define ADDR_NU_BASE 0xFEA0
#define ADDR_NU_SIZE 0x0060

#define ADDR_IO_BASE 0xFF00
#define ADDR_IO_SIZE 0x0080

#define ADDR_HRAM_BASE 0xFF80
#define ADDR_HRAM_SIZE 0x007F

/* Interrupt Enable register */
#define ADDR_IE 0xFFFF

#define INTERRUPT_BIT_JOYPAD 4u
#define INTERRUPT_BIT_SERIAL 3u
#define INTERRUPT_BIT_TIMER 2u
#define INTERRUPT_BIT_STAT 1u
#define INTERRUPT_BIT_VBLANK 0u

#define INTERRUPT_ADDR_JOYPAD 0x60
#define INTERRUPT_ADDR_SERIAL 0x58
#define INTERRUPT_ADDR_TIMER 0x50
#define INTERRUPT_ADDR_STAT 0x48
#define INTERRUPT_ADDR_VBLANK 0x40

#define IO_NR_COUNT 23
#define IO_WAVRAM_SIZE 0x0F

typedef enum {
    BUS_CALLER_NONE,
    BUS_CALLER_SM83,
    BUS_CALLER_PPU,
    BUS_CALLER_DMA
} BusCaller;

typedef struct Ppu Ppu;

typedef struct {
    Cartridge* cartridge;
    Ppu *ppu;

    uint8_t vram[ADDR_VRAM_SIZE * 2u];
    uint8_t wram[ADDR_WRAM0_SIZE * 8u];

    uint8_t oam[ADDR_OAM_SIZE];

    uint8_t hram[ADDR_HRAM_SIZE];

    uint8_t vram_bank;
    uint8_t wram_bank;

    /* OAM DMA */
    bool dma_active;
    uint8_t dma_index;
    uint16_t dma_src;
    uint8_t dma_delay;

    /* VRAM DMA */
    bool hdma_active;
    bool hdma_hblank;
    uint16_t hdma_src;
    uint16_t hdma_dst;
    int8_t hdma_len;

    /* Joypad input */
    uint8_t joyp_pressed;
    uint8_t joyp_prev_lines;

/* IO registers */
    uint8_t p1;                     /* $FF00 */ // Joypad state
    uint8_t sb;                     /* $FF01 */ // Serial transfer data
    uint8_t sc;                     /* $FF02 */ // Serial transfer control
    /* Timer registers */
    uint16_t div;                   /* $FF03 */ // Note: The timer is actually an unsigned 16-bit integer, but the register only uses the lower 8-bits.
    uint8_t tima;                   /* $FF04 */
    uint8_t tma;                    /* $FF05 */
    uint8_t tac;                    /* $FF06 */
    /* Interrupt flags register */
    uint8_t interrupt_flags;        /* $FF0F */
    /* Audio registers */
    uint8_t nr[IO_NR_COUNT];        /* $FF10 - $FF26 */
    uint8_t wavram[IO_WAVRAM_SIZE]; /* $FF30 - $FF3F */
    /* LCD registers */
    uint8_t lcdc;                   /* $FF40 */ // LCD control
    uint8_t stat;                   /* $FF41 */ // LCD status
    uint8_t scy;                    /* $FF42 */ // Background viewport Y position
    uint8_t scx;                    /* $FF43 */ // Background viewport X position
    uint8_t ly;                     /* $FF44 */ // LCD Y coordinate
    uint8_t lyc;                    /* $FF45 */ // LCD Y coordinate compare
    uint8_t dma;                    /* $FF46 */ // OAM DMA transfer
    uint8_t bgp;                    /* $FF47 */ // Background palette data
    uint8_t obp0;                   /* $FF48 */ // Object palette 0 data 
    uint8_t obp1;                   /* $FF49 */ // Object palette 1 data
    uint8_t wy;                     /* $FF4A */ // Window Y position
    uint8_t wx;                     /* $FF4B */ // Window X position
    /* CGB Registers */
    uint8_t key0;                   /* $FF4C */ // CGB boot-mode control: boot-ROM use only; sets CGB boot-mode (for DMG compatibility).
    uint8_t key1;                   /* $FF4D */ // Speed-switch control: Controls the speed of the system. Bit 7 reports current speed (0 = normal, 1 = double); bit 0 arms a switch to STOP.
    uint8_t vbk;                    /* $FF4F */ // VRAM bank select: Selects bank 0 or 1 mapped at 8000-9FFF.
    uint8_t hdma1;                  /* $FF51 */ // VRAM DMA source (high)
    uint8_t hdma2;                  /* $FF52 */ // VRAM DMA source (low)
    uint8_t hdma3;                  /* $FF53 */ // VRAM DMA destination (high)
    uint8_t hdma4;                  /* $FF54 */ // VRAM DMA destination (low)
    uint8_t hdma5;                  /* $FF55 */ // VRAM DMA length/mode/start B7:mode (0=General-Purpose, 1=HBlank) B6-0:length. Any write starts the transfer.
    uint8_t rp;                     /* $FF56 */ // Infrared communications port
    uint8_t bgpi;                   /* $FF68 */ // Background palette index
    uint8_t bgpd;                   /* $FF69 */ // Background palette data
    uint8_t obpi;                   /* $FF6A */ // Object palette index
    uint8_t obpd;                   /* $FF6B */ // Object palette data
    uint8_t opri;                   /* $FF6C */ // Object priority mode
    uint8_t svbk;                   /* $FF70 */ // WRAM bank select: Selects bank 1-7 mapped at D000-DFFF. Writing 0 will select bank 1.
    /* Interrupt enable register */
    uint8_t interrupt_enable;       /* $FFFF */
} Bus;

void bus_init(Bus* bus, Cartridge* cartridge, Ppu *ppu, GbSystemType system);
uint8_t bus_read(Bus* bus, BusCaller caller, uint16_t address);
void bus_write(Bus* bus, BusCaller caller, uint16_t address, uint8_t value);
void bus_request_interrupt(Bus* bus, uint8_t interrupt_bit);
void bus_dma_step(Bus *bus);
void bus_hdma_hblank(Bus *bus);

#endif
