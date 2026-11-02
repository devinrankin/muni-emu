#include "bus.h"
#include "ppu/ppu.h"
#include "mem/cartridge.h"
#include "timer.h"

#define IO_ADDR_JOYP 0xFF00
#define IO_ADDR_SB 0xFF01
#define IO_ADDR_SC 0xFF02
#define IO_ADDR_DIV 0xFF04
#define IO_ADDR_TIMA 0xFF05
#define IO_ADDR_TMA 0xFF06
#define IO_ADDR_TAC 0xFF07
#define IO_ADDR_IF 0xFF0F
#define IO_ADDR_NR10 0xFF10
#define IO_ADDR_NR11 0xFF11
#define IO_ADDR_NR12 0xFF12
#define IO_ADDR_NR13 0xFF13
#define IO_ADDR_NR14 0xFF14
#define IO_ADDR_NR21 0xFF16
#define IO_ADDR_NR22 0xFF17
#define IO_ADDR_NR23 0xFF18
#define IO_ADDR_NR24 0xFF19
#define IO_ADDR_NR30 0xFF1A
#define IO_ADDR_NR31 0xFF1B
#define IO_ADDR_NR32 0xFF1C
#define IO_ADDR_NR33 0xFF1D
#define IO_ADDR_NR34 0xFF1E
#define IO_ADDR_NR41 0xFF20
#define IO_ADDR_NR42 0xFF21
#define IO_ADDR_NR43 0xFF22
#define IO_ADDR_NR44 0xFF23
#define IO_ADDR_NR50 0xFF24
#define IO_ADDR_NR51 0xFF25
#define IO_ADDR_NR52 0xFF26
#define IO_ADDR_WAVERAM0 0xFF30
#define IO_ADDR_WAVERAM1 0xFF31
#define IO_ADDR_WAVERAM2 0xFF32
#define IO_ADDR_WAVERAM3 0xFF33
#define IO_ADDR_WAVERAM4 0xFF34
#define IO_ADDR_WAVERAM5 0xFF35
#define IO_ADDR_WAVERAM6 0xFF36
#define IO_ADDR_WAVERAM7 0xFF37
#define IO_ADDR_WAVERAM8 0xFF38
#define IO_ADDR_WAVERAM9 0xFF39
#define IO_ADDR_WAVERAMA 0xFF3A
#define IO_ADDR_WAVERAMB 0xFF3B
#define IO_ADDR_WAVERAMC 0xFF3C
#define IO_ADDR_WAVERAMD 0xFF3D
#define IO_ADDR_WAVERAME 0xFF3E
#define IO_ADDR_WAVERAMF 0xFF3F
#define IO_ADDR_LCDC 0xFF40
#define IO_ADDR_STAT 0xFF41
#define IO_ADDR_SCY 0xFF42
#define IO_ADDR_SCX 0xFF43
#define IO_ADDR_LY 0xFF44
#define IO_ADDR_LYC 0xFF45
#define IO_ADDR_BGP 0xFF47
#define IO_ADDR_OBP0 0xFF48
#define IO_ADDR_OBP1 0xFF49
#define IO_ADDR_WY 0xFF4A
#define IO_ADDR_WX 0xFF4B
#define IO_ADDR_DMA 0xFF46
#define IO_ADDR_KEY0 0xFF4C
#define IO_ADDR_KEY1 0xFF4D
#define IO_ADDR_VBK 0xFF4F
#define IO_ADDR_BANK 0xFF50
#define IO_ADDR_HDMA1 0xFF51
#define IO_ADDR_HDMA2 0xFF52
#define IO_ADDR_HDMA3 0xFF53
#define IO_ADDR_HDMA4 0xFF54
#define IO_ADDR_HDMA5 0xFF55
#define IO_ADDR_RP 0xFF56
#define IO_ADDR_BGPI 0xFF68
#define IO_ADDR_BGPD 0xFF69
#define IO_ADDR_OBPI 0xFF6A
#define IO_ADDR_OBPD 0xFF6B
#define IO_ADDR_OPRI 0xFF6C
#define IO_ADDR_SVBK 0xFF70
#define IO_ADDR_IE 0xFFFF

static uint8_t io_read(Bus* bus, BusCaller caller, uint16_t address);
static void io_write(Bus* bus, BusCaller caller, uint16_t address, uint8_t value);
static uint8_t vram_read(Bus* bus, BusCaller caller, uint16_t address);
static void vram_write(Bus* bus, BusCaller caller, uint16_t address, uint8_t value);
static uint8_t wram_read(Bus* bus, uint16_t address); 
static void wram_write(Bus* bus, uint16_t address, uint8_t value); 
static void hdma_copy_block(Bus *bus);

void bus_init(Bus* bus, Cartridge* cartridge, Ppu* ppu, GbSystemType system) {
/* https://gbdev.io/pandocs/Power_Up_Sequence.html#hardware-registers 
 * Currently using CGB boot values. */
    *bus = (Bus) {
        .p1 = 0xCF,
        .sc = 0x7E,
        .div = 0xAB,
        .tima = 0x00,
        .tma = 0x00,
        .tac = 0xF8,
        .interrupt_flags = 0xE1,
        .lcdc = 0x91,
        .stat = 0x85,
        .scy = 0x00,
        .scx = 0x00,
        .ly = 0x00,
        .lyc = 0x00,
        .dma = 0xFF,
        .bgp = 0xFC,
        .wy = 0x00,
        .wx = 0x00,
        .key0 = 0x04, // temporary until I can pass around support mode (cgb flag is probably the easiest way)
        .interrupt_enable = 0x00
    };
    bus->cartridge = cartridge;
    bus->ppu = ppu;
}

/* Returns a byte from a pre-defined block of memory pointed to by the specified address. */
uint8_t bus_read(Bus* bus, BusCaller caller, uint16_t address) {
    if (IN_RANGE(address, ADDR_ROM0_BASE, ADDR_ROMX_SIZE) || IN_RANGE(address, ADDR_EXRAM_BASE, ADDR_EXRAM_SIZE)) {
        return cartridge_read(bus->cartridge, address);
    }

    if (IN_RANGE(address, ADDR_VRAM_BASE, ADDR_VRAM_SIZE)) {
        uint8_t value = vram_read(bus, caller, address);

        return value; 
    }

    if (IN_RANGE(address, ADDR_WRAM0_BASE, ADDR_WRAMX_SIZE)) {
        return wram_read(bus, address);
    }

    if (IN_RANGE(address, ADDR_ERAM_BASE, ADDR_ERAM_SIZE)) {
        return wram_read(bus, address - (ADDR_ERAM_BASE - ADDR_WRAM0_BASE));
    }

    if (IN_RANGE(address, ADDR_OAM_BASE, ADDR_OAM_SIZE)) {
        return bus->oam[address - ADDR_OAM_BASE];
    }

    if (IN_RANGE(address, ADDR_NU_BASE, ADDR_NU_SIZE)) {
        return 0xFF;
    }

    if (IN_RANGE(address, ADDR_IO_BASE, ADDR_IO_SIZE)) {
        return io_read(bus, caller, address); 
    }

    if (IN_RANGE(address, ADDR_HRAM_BASE, ADDR_HRAM_SIZE)) {
        return bus->hram[address - ADDR_HRAM_BASE];
    }

    return bus->interrupt_enable;
}

/* Attempts to write a byte to a pre-defined block of memory pointed to by the specified address. */
void bus_write(Bus* bus, BusCaller caller, uint16_t address, uint8_t value) {
    // if (address == 0xCFA7) fprintf(stderr, "[$CFA7] write val=%0.2X\n", value);
    if (IN_RANGE(address, ADDR_ROM0_BASE, ADDR_ROMX_SIZE) || IN_RANGE(address, ADDR_EXRAM_BASE, ADDR_EXRAM_SIZE)) {
        cartridge_write(bus->cartridge, address, value);
        return;
    }

    if (IN_RANGE(address, ADDR_VRAM_BASE, ADDR_VRAM_SIZE)) {
        vram_write(bus, caller, address, value);
        return;
    }

    if (IN_RANGE(address, ADDR_WRAM0_BASE, ADDR_WRAMX_SIZE)) {
        wram_write(bus, address, value);
        return;
    }

    if (IN_RANGE(address, ADDR_ERAM_BASE, ADDR_ERAM_SIZE)) {
        wram_write(bus, address - (ADDR_ERAM_BASE - ADDR_WRAM0_BASE), value);
        return;
    }
 
    if (IN_RANGE(address, ADDR_OAM_BASE, ADDR_OAM_SIZE)) {
        bus->oam[address - ADDR_OAM_BASE] = value;
        return;
    }
 
    if (IN_RANGE(address, ADDR_NU_BASE, ADDR_NU_SIZE)) {
        return;
    }

    if (IN_RANGE(address, ADDR_IO_BASE, ADDR_IO_SIZE)) {
        io_write(bus, caller, address, value);
        return;
    }

    if (IN_RANGE(address, ADDR_HRAM_BASE, ADDR_HRAM_SIZE)) {
        bus->hram[address - ADDR_HRAM_BASE] = value;
        return;
    }

    bus->interrupt_enable = value;
}

void bus_request_interrupt(Bus* bus, uint8_t interrupt_bit) {
    BIT_SET(bus->interrupt_flags, interrupt_bit);
}

void bus_dma_step(Bus *bus) {
    if (!bus->dma_active) return;
    if(bus->dma_delay) { bus->dma_delay--; return; }

    uint16_t src = bus->dma_src + bus->dma_index;
    if (IN_RANGE(src, ADDR_ERAM_BASE, ADDR_ERAM_SIZE)) src -= ADDR_ERAM_BASE - ADDR_WRAM0_BASE;
    bus->oam[bus->dma_index] = bus_read(bus, BUS_CALLER_DMA, src);

    if (++bus->dma_index == 160) bus->dma_active = false;
}

void bus_hdma_hblank(Bus *bus) {
    if (bus->hdma_active && bus->hdma_hblank) hdma_copy_block(bus);
}

static uint8_t io_read(Bus *bus, BusCaller caller, uint16_t address) {
    (void)caller;
    switch (address) {
    case IO_ADDR_JOYP: return bus->p1;
    case IO_ADDR_DIV: return (uint8_t)(bus->div >> 8);
    case IO_ADDR_TIMA: return bus->tima;
    case IO_ADDR_TMA: return bus->tma;
    case IO_ADDR_TAC: return bus->tac;
    case IO_ADDR_IF: return bus->interrupt_flags;
    case IO_ADDR_NR10 ... IO_ADDR_WAVERAMF: return 0xFF;
    case IO_ADDR_LCDC: return bus->lcdc;
    case IO_ADDR_STAT: return bus->stat;
    case IO_ADDR_SCY: return bus->scy;
    case IO_ADDR_SCX: return bus->scx;
    case IO_ADDR_LY: return bus->ly;
    case IO_ADDR_LYC: return bus->lyc;
    case IO_ADDR_BGP: return bus->bgp;
    case IO_ADDR_OBP0: return bus->obp0;
    case IO_ADDR_OBP1: return bus->obp1;
    case IO_ADDR_WX: return bus->wx;
    case IO_ADDR_WY: return bus->wy;
    case IO_ADDR_KEY0: return bus->key0;
    case IO_ADDR_KEY1: return bus->key1;
    case IO_ADDR_VBK: return bus->vbk;
    case IO_ADDR_HDMA1: return 0xFF;
    case IO_ADDR_HDMA2: return 0xFF; 
    case IO_ADDR_HDMA3: return 0xFF; 
    case IO_ADDR_HDMA4: return 0xFF; 
    case IO_ADDR_HDMA5: return bus->hdma_active ? (bus->hdma_len & 0x7F) : 0xFF;
    case IO_ADDR_RP: return bus->rp;
    case IO_ADDR_BGPI: return bus->bgpi;
    case IO_ADDR_BGPD: return ppu_palette_read(bus->ppu, false);
    case IO_ADDR_OBPI: return bus->obpi;
    case IO_ADDR_OBPD: return ppu_palette_read(bus->ppu, true);
    case IO_ADDR_SVBK: return bus->svbk;
    case IO_ADDR_IE: return bus->interrupt_enable;
    default: return 0xFF;
    }
}

static void io_write(Bus* bus, BusCaller caller, uint16_t address, uint8_t value) {
    (void)caller;
    switch(address) {
    case IO_ADDR_JOYP: bus->p1 = value; break;
    case IO_ADDR_SB: bus->sb = value; break;
    case IO_ADDR_SC: bus->sc = value; break;
    case IO_ADDR_DIV: timer_write_div(bus); break;
    case IO_ADDR_TIMA: bus->tima = value; break;
    case IO_ADDR_TMA: bus->tma = value; break;
    case IO_ADDR_TAC: timer_write_tac(bus, value); break;
    case IO_ADDR_IF: bus->interrupt_flags = value; break;
    case IO_ADDR_NR10 ... IO_ADDR_WAVERAMF: break;
    case IO_ADDR_LCDC: bus->lcdc = value; break;
    case IO_ADDR_STAT: bus->stat = (bus->stat & 0x07) | (value & 0x78); break;
    case IO_ADDR_SCY: bus->scy = value; break;
    case IO_ADDR_SCX: bus->scx = value; break;
    case IO_ADDR_LY: break;
    case IO_ADDR_LYC: bus->lyc = value; break;
    case IO_ADDR_DMA:
        bus->dma = value;
        bus->dma_src = (uint16_t)value << 8;
        bus->dma_index = 0;
        bus->dma_delay = 1;
        bus->dma_active = true;
        break;
    case IO_ADDR_BGP: bus->bgp = value; break;
    case IO_ADDR_OBP0: bus->obp0 = value; break;
    case IO_ADDR_OBP1: bus->obp1 = value; break;
    case IO_ADDR_WY: bus->wy = value; break;
    case IO_ADDR_WX: bus->wx = value; break;
    case IO_ADDR_KEY0: break;
    case IO_ADDR_KEY1: bus->key1 = value; break;
    case IO_ADDR_VBK: bus->vbk = value; break;
    case IO_ADDR_HDMA1: bus->hdma1 = value; break;
    case IO_ADDR_HDMA2: bus->hdma2 = value; break;
    case IO_ADDR_HDMA3: bus->hdma3 = value; break;
    case IO_ADDR_HDMA4: bus->hdma4 = value; break;
    case IO_ADDR_HDMA5:
        if (bus->hdma_active && !BIT(value, 7)) {
            bus->hdma_active = false;
            break;
        }
        bus->hdma_len = value & 0x7F;
        if (BIT(value, 7)) {
            bus->hdma_active = true;
            bus->hdma_hblank = true;
        } else {
            bus->hdma_src = ((uint16_t)bus->hdma1 << 8) | (bus->hdma2 & 0xF0);
            bus->hdma_dst = 0x8000 | (((uint16_t)bus->hdma3 & 0x1F) << 8) | (bus->hdma4 & 0xF0);
            int blocks = (value & 0x7F) + 1;
            for (int i = 0; i <= blocks; i++) hdma_copy_block(bus);
            bus->hdma_len = 0x7F;
        }
        break;
    case IO_ADDR_RP: bus->rp = value; break;
    case IO_ADDR_BGPI: bus->bgpi = value; break;
    case IO_ADDR_BGPD: ppu_palette_write(bus->ppu, false, value); break;
    case IO_ADDR_OBPI: bus->obpi = value; break;
    case IO_ADDR_OBPD: ppu_palette_write(bus->ppu, true, value); break;
    case IO_ADDR_SVBK: bus->svbk = value; break;
    case IO_ADDR_IE: bus->interrupt_enable = value; break;
    default: break;
    }
}

/* Returns the byte in VRAM pointed to by the specified address. */
static uint8_t vram_read(Bus* bus, BusCaller caller, uint16_t address) {
    uint8_t ppu_mode = bus->stat & 3;
    if (caller == BUS_CALLER_SM83 && ppu_mode == 3) {
        return 0xFF;
    }

    uint16_t offset = BIT(bus->vbk, 0) * ADDR_VRAM_SIZE + (address - ADDR_VRAM_BASE);

    return bus->vram[offset];
}

/* Writes a byte into VRAM at the specified address. */
static void vram_write(Bus* bus, BusCaller caller, uint16_t address, uint8_t value) {
    uint8_t mode = bus->stat & 0x03;
    bool blocked = caller == BUS_CALLER_SM83 && mode == 3;

    if (blocked) return;
    
    uint16_t offset = BIT(bus->vbk, 0) * ADDR_VRAM_SIZE + (address - ADDR_VRAM_BASE);

    bus->vram[offset] = value;
}

/* Returns the byte in WRAM pointed to by the specified address. */
static uint8_t wram_read(Bus* bus, uint16_t address) {
    uint16_t offset;

    if (IN_RANGE(address, ADDR_WRAM0_BASE, ADDR_WRAM0_SIZE)) {
        offset = address - ADDR_WRAM0_BASE;
    } else {
        uint8_t bank = bus->svbk & 7;
        if (bank == 0) bank = 1;
        offset = bank * ADDR_WRAMX_SIZE + (address - ADDR_WRAMX_BASE);
    }

    return bus->wram[offset];
}

/* Writes a byte into WRAM at the specified address. */
static void wram_write(Bus* bus, uint16_t address, uint8_t value) {
    uint16_t offset;

    if(IN_RANGE(address, ADDR_WRAM0_BASE, ADDR_WRAM0_SIZE)) {
        offset = address - ADDR_WRAM0_BASE;
    } else {
        uint8_t bank = bus->svbk & 7;
        if (bank == 0) bank = 1;
        offset = bank * ADDR_WRAMX_SIZE + (address - ADDR_WRAMX_BASE);
    }

    bus->wram[offset] = value;
}

static void hdma_copy_block(Bus *bus) {
    for (int i = 0; i < 16; i++) {
        uint8_t b = bus_read(bus, BUS_CALLER_DMA, bus->hdma_src++);
        bus->vram[BIT(bus->vbk, 0) * ADDR_VRAM_SIZE + ((bus->hdma_dst++ - ADDR_VRAM_BASE) & 0x1FFF)] = b;
    }
    if (bus->hdma_len-- == 0) bus->hdma_active = false;
}
