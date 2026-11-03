#ifndef PPU_FIFO_H
#define PPU_FIFO_H

#include <stdint.h>
#include <stdbool.h>
#include "../common.h"

#define FIFO_CAPACITY 16u

typedef struct PpuPixel {
    uint8_t color;
    uint8_t palette;
    bool priority;
    bool is_obj;
    uint8_t oam_index; // OBJ only
} PpuPixel;

typedef struct PpuFifo {
    PpuPixel pixels[FIFO_CAPACITY];
    uint8_t head;
    uint8_t count;
} PpuFifo;

void ppu_fifo_clear(PpuFifo *fifo);
bool ppu_fifo_push(PpuFifo *fifo, PpuPixel px);
bool ppu_fifo_pop(PpuFifo *fifo, PpuPixel *px);
uint8_t ppu_fifo_count(const PpuFifo *fifo);
bool ppu_fifo_empty(const PpuFifo *fifo);

#endif
