#ifndef PPU_H
#define PPU_H

#include "../common.h"
#include "../bus.h"
#include "ppu_fifo.h"

#define PPU_NUM_CGB_PALETTES 8u
#define PPU_COLORS_PER_PALETTE 4u
#define PPU_BYTES_PER_COLOR 2u

#define OAM_BUFFER_CAPACITY 10u

typedef enum {
    PPU_MODE_HBLANK,
    PPU_MODE_VBLANK,
    PPU_MODE_OAM,
    PPU_MODE_DRAW
} PpuMode;

typedef struct {
    uint8_t y;
    uint8_t x;
    uint8_t tile;
    uint8_t attr;
    uint8_t oam_index;
} PpuObj;

typedef struct Ppu {
    bool lcd_on;
    bool cgb;
    uint32_t off_dots;
    uint16_t mode_x;
    uint16_t line_dots;
    bool stat_line;
    uint8_t stall;

    uint16_t framebuf[LCD_XRES * LCD_YRES];
    bool frame_ready;

    bool obj_order;
    uint8_t obj_count;
    uint8_t obj_next;
    PpuObj obj_buffer[OAM_BUFFER_CAPACITY];
    uint8_t discard_px;

    bool y_cond;
    bool window_active;
    uint8_t window_line;

    PpuFifo bgw_fifo;
    PpuPixel obj_line[LCD_XRES];
    uint8_t bg_tx;
    uint8_t bgw_palettes[PPU_NUM_CGB_PALETTES * PPU_COLORS_PER_PALETTE * PPU_BYTES_PER_COLOR];
    uint8_t obj_palettes[PPU_NUM_CGB_PALETTES * PPU_COLORS_PER_PALETTE * PPU_BYTES_PER_COLOR];

    Bus *bus; // for reading VRAM and OAM
} Ppu;

void ppu_init(Ppu *ppu, Bus *bus);
void ppu_step(Ppu *ppu);

uint8_t ppu_palette_read(const Ppu *ppu, bool obj);
void ppu_palette_write(Ppu *ppu, bool obj, uint8_t value);

void dump_framebuf(const Ppu *ppu);

#endif
