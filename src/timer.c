#include "timer.h"

#define IO_TIMER_TAC_ENABLE_BIT 3u
#define IO_TIMER_TAC_SELECT(tac) ((tac) & 0x03u)

#define IO_TIMER_TAC_SELECT_BIT_4096HZ 9u
#define IO_TIMER_TAC_SELECT_BIT_262144HZ 3u
#define IO_TIMER_TAC_SELECT_BIT_65536HZ 5u
#define IO_TIMER_TAC_SELECT_BIT_16384HZ 7u

static const uint8_t TIMER_BITS[4] = { 9, 3, 5, 7 };

static bool timer_signal(const Bus *bus);
static void timer_increment(Bus *bus);

void timer_tick(Bus *bus) {
    bool before = timer_signal(bus);
    bus->div++;
    if (before && !timer_signal(bus)) timer_increment(bus);
}

void timer_write_div(Bus *bus) {
    bool before = timer_signal(bus);
    bus->div = 0;
    if (before) timer_increment(bus);
}

void timer_write_tac(Bus *bus, uint8_t value) {
    bool before = timer_signal(bus);
    bus->tac = value & 7;
    if (before && !timer_signal(bus)) timer_increment(bus);
}

static bool timer_signal(const Bus *bus) {
    return BIT(bus->tac, 2) && BIT(bus->div, TIMER_BITS[bus->tac & 3]);
}

static void timer_increment(Bus *bus) {
    if (++bus->tima == 0) {
        bus->tima = bus->tma;
        bus_request_interrupt(bus, INTERRUPT_BIT_TIMER);
    }
}
