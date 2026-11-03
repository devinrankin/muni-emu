#include "ppu_fifo.h"

void ppu_fifo_clear(PpuFifo *fifo) {
    fifo->head = 0;
    fifo->count = 0;
}

bool ppu_fifo_push(PpuFifo *fifo, PpuPixel px) {
    if (fifo->count >= FIFO_CAPACITY) {
        return false;
    }

    int tail = (fifo->head + fifo->count) % FIFO_CAPACITY;
    fifo->pixels[tail] = px;
    fifo->count++;

    return true;
}

bool ppu_fifo_pop(PpuFifo *fifo, PpuPixel *px) {
    if (fifo->count == 0) {
        return false;
    }

    *px = fifo->pixels[fifo->head];
    fifo->head = (fifo->head + 1) % FIFO_CAPACITY;
    fifo->count--;

    return true;
}

uint8_t ppu_fifo_count(const PpuFifo *fifo) {
    return fifo->count;
}

bool ppu_fifo_empty(const PpuFifo *fifo) {
    return !fifo->count;
}
