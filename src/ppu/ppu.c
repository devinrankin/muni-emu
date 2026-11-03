#include <string.h>
#include "ppu.h"

/* DOT counts */
#define PPU_MODE_DOTS_OAM 80
#define PPU_DOTS_PER_LINE 456
#define PPU_DOTS_PER_FRAME (PPU_DOTS_PER_LINE * PPU_LY_BOTTOM) /* 70224 */

/* LY breakpoints */
#define PPU_LY_VBLANK 144
#define PPU_LY_BOTTOM 154

/* Tile Map VRAM Base Addresses */
#define PPU_ADDR_VRAM_TILEMAP0_BASE 0x9800
#define PPU_ADDR_VRAM_TILEMAP1_BASE 0x9C00

/* Tile Data VRAM Base Addresses */
#define PPU_ADDR_VRAM_TILEDATA0_BASE 0x9000
#define PPU_ADDR_VRAM_TILEDATA1_BASE 0x8000

/* LCDC bits */
#define PPU_LCDC_LCD_ENABLE_BIT 7u
#define PPU_LCDC_WINDOW_TILEMAP_AREA_BIT 6u
#define PPU_LCDC_WINDOW_ENABLE_BIT 5u
#define PPU_LCDC_BGW_TILEDATA_AREA_BIT 4u
#define PPU_LCDC_BG_TILEMAP_AREA_BIT 3u
#define PPU_LCDC_OBJ_SIZE_BIT 2u
#define PPU_LCDC_OBJ_ENABLE_BIT 1u
#define PPU_LCDC_BGW_ENABLE_BIT 0u

/* STAT bits */
#define PPU_STAT_LYC_INTERRUPT_BIT 6u
#define PPU_STAT_MODE_OAM_INTERRUPT_BIT 5u
#define PPU_STAT_MODE_VBLANK_INTERRUPT_BIT 4u
#define PPU_STAT_MODE_HBLANK_INTERRUPT_BIT 3u
#define PPU_STAT_LYC_EQ_BIT 2u

/* Tile Data Attribute bits */
#define PPU_TILEDATA_ATTR_PALETTE_MASK 0x7
#define PPU_TILEDATA_ATTR_BANK_BIT 3u
#define PPU_TILEDATA_ATTR_DMG_PALETTE_BIT 4u // OBJ tile data only
#define PPU_TILEDATA_ATTR_X_FLIP_BIT 5u
#define PPU_TILEDATA_ATTR_Y_FLIP_BIT 6u
#define PPU_TILEDATA_ATTR_PRI_BIT 7u

#define PPU_PX_PER_FETCH 8u
#define PPU_BYTES_PER_TILE 16u
#define PPU_OBJ_OFFSET_Y 16u
#define PPU_OBJ_OFFSET_X 8u

static inline uint8_t ppu_vram_read(const Ppu *ppu, uint8_t bank, uint16_t address);
static inline uint8_t stat_mode(const Ppu *ppu);
static inline void stat_mode_transition(Ppu *ppu, PpuMode mode);
static void ppu_load_dmg_palettes(Ppu *ppu);
static bool ppu_palette_locked(const Ppu *ppu);
static void ppu_mode_transition_oam(Ppu *ppu);
static void ppu_mode_transition_draw(Ppu *ppu);
static void ppu_mode_transition_hblank(Ppu *ppu);
static void ppu_mode_transition_vblank(Ppu *ppu);
static void ppu_update_stat_line(Ppu *ppu);
static void ppu_end_of_line(Ppu *ppu);
static void ppu_oam_scan(Ppu *ppu);
static void ppu_draw_step(Ppu *ppu);
static uint16_t ppu_pixel_to_rgb555(const Ppu *ppu, PpuPixel px);
static void ppu_bgw_refill(Ppu *ppu);
static void ppu_obj_refill(Ppu *ppu, const PpuObj *obj);
static PpuObj *ppu_obj_next_due(Ppu *ppu);
static PpuPixel ppu_resolve(const Ppu *ppu, PpuPixel bg, PpuPixel obj);
static inline uint16_t ppu_tile_map_base_address(const Ppu *ppu);
static inline uint16_t ppu_tile_data_address(const Ppu *ppu, uint8_t tile, uint8_t row);
static inline uint8_t ppu_row_pixel(uint8_t low, uint8_t high, uint8_t x, bool flip);


void ppu_init(Ppu *ppu, Bus *bus) {
    *ppu = (Ppu) {
        .bus = bus,
        .lcd_on = false,
        .mode_x = 0,
        .line_dots = 0,
        .frame_ready = false,
        .discard_px = 0,
        .y_cond = false,
        .window_active = false,
        .window_line = 0,
    };
    ppu->cgb = !BIT(ppu->bus->key0, 2u);
    if (!ppu->cgb) {
        ppu_load_dmg_palettes(ppu);
    }
}

void ppu_step(Ppu *ppu) {
    if (!BIT(ppu->bus->lcdc, PPU_LCDC_LCD_ENABLE_BIT)) {
        if (ppu->lcd_on) {
            ppu->lcd_on = false;
            ppu->line_dots = 0;
            ppu->bus->ly = 0;
            ppu->stat_line = false;
            ppu->off_dots = 0;
            ppu_mode_transition_hblank(ppu);
            for (int i = 0; i < LCD_XRES * LCD_YRES; i++) ppu->framebuf[i] = 0x7FFF;
        }

        if (++ppu->off_dots >= PPU_DOTS_PER_FRAME) {
            ppu->off_dots = 0;
            ppu->frame_ready = true;
        }
        return;
    }

    if (!ppu->lcd_on) {
        ppu->lcd_on = true;
        ppu_mode_transition_oam(ppu);
    }

    ppu->line_dots++;

    switch (stat_mode(ppu)) {
    case PPU_MODE_OAM:
        if (ppu->line_dots == PPU_MODE_DOTS_OAM) {
            ppu_oam_scan(ppu);
            ppu_mode_transition_draw(ppu);
        }
        break;
    case PPU_MODE_DRAW: {
        const bool window_enabled = ppu->y_cond && BIT(ppu->bus->lcdc, PPU_LCDC_WINDOW_ENABLE_BIT);
        int16_t window_left = (int16_t)ppu->bus->wx - 7;

        if (!ppu->window_active && window_enabled && window_left < LCD_XRES && (int16_t)ppu->mode_x >= (window_left < 0 ? 0 : window_left)) {
            ppu->window_active = true;
            ppu->bg_tx = 0;
            ppu->discard_px = 0;
            ppu->stall = 6;
            ppu_fifo_clear(&ppu->bgw_fifo);
        }
        ppu_draw_step(ppu);
        if (ppu->mode_x >= LCD_XRES) {
            ppu_mode_transition_hblank(ppu); 
            bus_hdma_hblank(ppu->bus);
        }
        break;
    }
    case PPU_MODE_HBLANK: 
    case PPU_MODE_VBLANK: 
        if(ppu->line_dots == PPU_DOTS_PER_LINE) ppu_end_of_line(ppu);
        break;
    }
    ppu_update_stat_line(ppu);
}

uint8_t ppu_palette_read(const Ppu *ppu, bool obj) {
    if (ppu_palette_locked(ppu)) return 0xFF;

    uint8_t idx = (obj ? ppu->bus->obpi : ppu->bus->bgpi) & 0x3F;
    return (obj ? ppu->obj_palettes : ppu->bgw_palettes)[idx];
}

void ppu_palette_write(Ppu *ppu, bool obj, uint8_t value) {
    uint8_t *pi = obj ? &ppu->bus->obpi : &ppu->bus->bgpi;
    uint8_t *ram = obj ? ppu->obj_palettes : ppu->bgw_palettes;
    uint8_t idx = *pi & 0x3F;

    if (!ppu_palette_locked(ppu)) ram[idx] = value;
    if (BIT(*pi, 7)) *pi = (*pi & 0x80) | ((idx + 1) & 0x3F);
}

static inline uint8_t ppu_vram_read(const Ppu *ppu, uint8_t bank, uint16_t address) {
    return ppu->bus->vram[BIT(bank, 0u) * ADDR_VRAM_SIZE + (address - ADDR_VRAM_BASE)];
}

static inline uint8_t stat_mode(const Ppu *ppu) {
    return ppu->bus->stat & 0x3;
}
static inline void stat_mode_transition(Ppu *ppu, PpuMode mode) {
    ppu->bus->stat = (ppu->bus->stat & 0xFC) | (mode);
}

static void ppu_load_dmg_palettes(Ppu *ppu) {
    static const uint16_t gray[4] = { 0x7FFF, 0x56B5, 0x294A, 0x0000 };

    for (int p = 0; p < 8; p++) {
        for (int c = 0; c < 4; c++) {
            int off = p * 8 + c * 2;
            ppu->bgw_palettes[off] = ppu->obj_palettes[off] = gray[c] & 0xFF;
            ppu->bgw_palettes[off + 1] = ppu->obj_palettes[off + 1] = gray[c] >> 8;
        }
    }
}

static bool ppu_palette_locked(const Ppu *ppu) {
    return BIT(ppu->bus->lcdc, PPU_LCDC_LCD_ENABLE_BIT) && stat_mode(ppu) == PPU_MODE_DRAW;
}


static void ppu_mode_transition_oam(Ppu *ppu) {
    stat_mode_transition(ppu, PPU_MODE_OAM);

    ppu->mode_x = 0;
    ppu->window_active = false;
    ppu->obj_count = 0;
    if (ppu->bus->ly == ppu->bus->wy) ppu->y_cond = true;
}

static void ppu_mode_transition_draw(Ppu *ppu) {
    stat_mode_transition(ppu, PPU_MODE_DRAW);

    ppu->mode_x = 0;
    ppu->bg_tx = 0;
    ppu->stall = 12;
    ppu->discard_px = ppu->bus->scx & 7;
    ppu->window_active = false;

    memset(ppu->obj_line, 0, sizeof ppu->obj_line);
    ppu_fifo_clear(&ppu->bgw_fifo);
}

static void ppu_mode_transition_hblank(Ppu *ppu) {
    stat_mode_transition(ppu, PPU_MODE_HBLANK);
}

static void ppu_mode_transition_vblank(Ppu *ppu) {
    stat_mode_transition(ppu, PPU_MODE_VBLANK);

    ppu->frame_ready = true;
    bus_request_interrupt(ppu->bus, INTERRUPT_BIT_VBLANK);
}

static void ppu_end_of_line(Ppu *ppu) {
    ppu->line_dots = 0;
    if (ppu->window_active) ppu->window_line++;

    uint8_t ly = ++ppu->bus->ly;
    if (ly == PPU_LY_VBLANK) {
        ppu_mode_transition_vblank(ppu);
    } else if (ly == PPU_LY_BOTTOM) {
        ppu->bus->ly = 0;
        ppu->window_line = 0;
        ppu->y_cond = false;
        ppu_mode_transition_oam(ppu);
    } else if (ly < PPU_LY_VBLANK) {
        ppu_mode_transition_oam(ppu);
    }
}

static void ppu_update_stat_line(Ppu *ppu) {
    bool lyc_eq = ppu->bus->ly == ppu->bus->lyc;
    const uint8_t lyc_mask = 1u << PPU_STAT_LYC_EQ_BIT;
    ppu->bus->stat = (ppu->bus->stat & ~lyc_mask) | (lyc_eq ? lyc_mask : 0);

    uint8_t mode = ppu->bus->stat & 3;
    bool line = (ppu->bus->ly == ppu->bus->lyc && BIT(ppu->bus->stat, PPU_STAT_LYC_INTERRUPT_BIT)) ||
        (mode == PPU_MODE_VBLANK && BIT(ppu->bus->stat, PPU_STAT_MODE_VBLANK_INTERRUPT_BIT)) ||
        (mode == PPU_MODE_HBLANK && BIT(ppu->bus->stat, PPU_STAT_MODE_HBLANK_INTERRUPT_BIT)) ||
        (mode == PPU_MODE_OAM && BIT(ppu->bus->stat, PPU_STAT_MODE_OAM_INTERRUPT_BIT));
    if (line && !ppu->stat_line) {
        bus_request_interrupt(ppu->bus, INTERRUPT_BIT_STAT);
    }
    ppu->stat_line = line;
}

static void ppu_oam_scan(Ppu *ppu) {
    const uint8_t ly = ppu->bus->ly;
    const uint8_t height = BIT(ppu->bus->lcdc, PPU_LCDC_OBJ_SIZE_BIT) ? 16 : 8;
    ppu->obj_order = ppu->cgb && !BIT(ppu->bus->opri, 0);

    ppu->obj_count = 0;
    ppu->obj_next = 0;

    for (uint8_t i = 0; i < 40 && ppu->obj_count < OAM_BUFFER_CAPACITY; i++) {
        const uint8_t *e = &ppu->bus->oam[i * 4];
        int row = ly + 16 - e[0];
        if (row < 0 || row >= height) continue;

        PpuObj obj = { 
            .y = e[0],
            .x = e[1],
            .tile = e[2],
            .attr = e[3],
            .oam_index = i
        };

        uint8_t j = ppu->obj_count++;
        while (j > 0 && ppu->obj_buffer[j - 1].x > obj.x) {
            ppu->obj_buffer[j] = ppu->obj_buffer[j - 1];
            j--;
        }
        ppu->obj_buffer[j] = obj;
    }

    while (ppu->obj_next < ppu->obj_count && ppu->obj_buffer[ppu->obj_next].x == 0) {
        ppu->obj_next++;
    }
}

static void ppu_draw_step(Ppu *ppu) {
    if (ppu->stall) {
        ppu->stall--;
        return;
    }

    PpuObj *o = ppu_obj_next_due(ppu);
    if (o) {
        ppu_obj_refill(ppu, o);
        return;
    }

    if (ppu_fifo_empty(&ppu->bgw_fifo)) ppu_bgw_refill(ppu);

    PpuPixel bg;
    ppu_fifo_pop(&ppu->bgw_fifo, &bg);
    if (ppu->discard_px) {
        ppu->discard_px--;
        return;
    }

    PpuPixel out = ppu_resolve(ppu, bg, ppu->obj_line[ppu->mode_x]);
    ppu->framebuf[ppu->bus->ly * LCD_XRES + ppu->mode_x] = ppu_pixel_to_rgb555(ppu, out);
    ppu->mode_x++;
}

static uint16_t ppu_pixel_to_rgb555(const Ppu *ppu, PpuPixel px) {
    uint8_t color = px.color & 3;
    uint8_t palette = px.palette & 7;

    if (!ppu->cgb) {
        uint8_t reg;
        if (px.is_obj) {
        palette &= 1;
        reg = palette ? ppu->bus->obp1 : ppu->bus->obp0;
        } else {
            palette = 0;
            reg = ppu->bus->bgp;
        }
        color = (reg >> (color * 2)) & 3;
    }

    const uint8_t *ram = px.is_obj ? ppu->obj_palettes : ppu->bgw_palettes;
    uint8_t offset = (uint8_t)(palette * 8 + color * 2);

    return (uint16_t)((ram[offset] | (ram[offset + 1] << 8)) & 0x7FFF);
}

static void ppu_bgw_refill(Ppu *ppu) {
    uint8_t tx, ty, row;

    if (ppu->window_active) {
        tx = ppu->bg_tx & 0x1F;
        ty = ppu->window_line >> 3;
        row = ppu->window_line & 7;
    } else {
        uint8_t y = (uint8_t)(ppu->bus->scy + ppu->bus->ly);
        tx = (uint8_t)(((ppu->bus->scx >> 3) + ppu->bg_tx) & 0x1F);
        ty = y >> 3;
        row = y & 7;
    }
    ppu->bg_tx++;

    uint16_t map = ppu_tile_map_base_address(ppu) + ty * 32u + tx;
    uint8_t tile = ppu_vram_read(ppu, 0, map);
    uint8_t attr = ppu->cgb ? ppu_vram_read(ppu, 1, map) : 0;
    uint8_t bank = ppu->cgb ? BIT(attr, PPU_TILEDATA_ATTR_BANK_BIT) : 0;

    if (BIT(attr, PPU_TILEDATA_ATTR_Y_FLIP_BIT)) row = 7 - row;

    uint16_t address = ppu_tile_data_address(ppu, tile, row);
    uint8_t low = ppu_vram_read(ppu, bank, address);
    uint8_t high = ppu_vram_read(ppu, bank, address + 1);

    bool flip_x = BIT(attr, PPU_TILEDATA_ATTR_X_FLIP_BIT);
    for (uint8_t x = 0; x < PPU_PX_PER_FETCH; x++) {
        ppu_fifo_push(&ppu->bgw_fifo, (PpuPixel) {
            .color = ppu_row_pixel(low, high, x, flip_x),
            .palette = attr & PPU_TILEDATA_ATTR_PALETTE_MASK,
            .priority = BIT(attr, PPU_TILEDATA_ATTR_PRI_BIT)
        });
    }
}

static void ppu_obj_refill(Ppu *ppu, const PpuObj *o) {
    ppu->obj_next++;
    ppu->stall = 6;

    const uint8_t height = BIT(ppu->bus->lcdc, PPU_LCDC_OBJ_SIZE_BIT) ? 16 : 8;
    uint8_t row = (uint8_t)(ppu->bus->ly + 16 - o->y);
    if (BIT(o->attr, PPU_TILEDATA_ATTR_Y_FLIP_BIT)) row = height - 1 - row;

    uint8_t tile = o->tile;
    if (height == 16) tile = (tile & 0xFE) | (row >> 3);

    uint16_t address = PPU_ADDR_VRAM_TILEDATA1_BASE + tile * PPU_BYTES_PER_TILE + (row & 7) * 2;
    uint8_t bank = ppu->cgb ? BIT(o->attr, PPU_TILEDATA_ATTR_BANK_BIT) : 0;
    uint8_t low = ppu_vram_read(ppu, bank, address);
    uint8_t high = ppu_vram_read(ppu, bank, address + 1);

    bool flip_x = BIT(o->attr, PPU_TILEDATA_ATTR_X_FLIP_BIT);
    uint8_t palette = ppu->cgb ? (o->attr & PPU_TILEDATA_ATTR_PALETTE_MASK) : BIT(o->attr, PPU_TILEDATA_ATTR_DMG_PALETTE_BIT);

    int left = (int)o->x - PPU_OBJ_OFFSET_X;
    for (int i = 0; i < 8; i++) {
        int px = left + i;
        if (px < 0 || px >= LCD_XRES) continue;

        uint8_t color = ppu_row_pixel(low, high, i, flip_x);
        if (color == 0) continue;

        PpuPixel *slot = &ppu->obj_line[px];
        if (slot->color == 0 || (ppu->obj_order && o->oam_index < slot->oam_index)) {
            *slot = (PpuPixel) {
                .color = color,
                .palette = palette,
                .priority = BIT(o->attr, PPU_TILEDATA_ATTR_PRI_BIT),
                .is_obj = true,
                .oam_index = o->oam_index
            };
        }
    }
}

static PpuObj *ppu_obj_next_due(Ppu *ppu) {
    if (!BIT(ppu->bus->lcdc, PPU_LCDC_OBJ_ENABLE_BIT) || ppu->obj_next >= ppu->obj_count) return NULL;
    PpuObj *obj = &ppu->obj_buffer[ppu->obj_next];
    return ((int)obj->x - 8 <= (int)ppu->mode_x) ? obj : NULL;
}

static PpuPixel ppu_resolve(const Ppu *ppu, PpuPixel bg, PpuPixel obj) {
    bool bgw_en = BIT(ppu->bus->lcdc, PPU_LCDC_BGW_ENABLE_BIT);

    if (!ppu->cgb) {
        if (!bgw_en) bg.color = 0;
        if (obj.color == 0) return bg;
        return (obj.priority && bg.color) ? bg : obj;
    }

    if (obj.color == 0) return bg;
    if (!bgw_en) return obj;
    return (bg.color && (obj.priority || bg.priority)) ? bg : obj;
}

static inline uint16_t ppu_tile_map_base_address(const Ppu *ppu) {
    return BIT(ppu->bus->lcdc, ppu->window_active ? PPU_LCDC_WINDOW_TILEMAP_AREA_BIT : PPU_LCDC_BG_TILEMAP_AREA_BIT) 
        ? PPU_ADDR_VRAM_TILEMAP1_BASE : PPU_ADDR_VRAM_TILEMAP0_BASE;
}

static inline uint16_t ppu_tile_data_address(const Ppu *ppu, uint8_t tile, uint8_t row) {
    int index = BIT(ppu->bus->lcdc, PPU_LCDC_BGW_TILEDATA_AREA_BIT) ? tile : (int8_t)tile;
    int base = BIT(ppu->bus->lcdc, PPU_LCDC_BGW_TILEDATA_AREA_BIT) ? PPU_ADDR_VRAM_TILEDATA1_BASE : PPU_ADDR_VRAM_TILEDATA0_BASE;
    return (uint16_t)(base + index * (int)PPU_BYTES_PER_TILE + row * 2);
}

static inline uint8_t ppu_row_pixel(uint8_t low, uint8_t high, uint8_t x, bool flip) {
    uint8_t bit = flip ? x : 7 - x;
    return (uint8_t)((BIT(high, bit) << 1) | BIT(low, bit));
}

