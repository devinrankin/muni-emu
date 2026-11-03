#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include "common.h"
#include "bus.h"

void timer_tick(Bus *bus);
void timer_write_div(Bus *bus);
void timer_write_tac(Bus *bus, uint8_t value);

#endif
