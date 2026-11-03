#ifndef SM83_H
#define SM83_H

#include <stdint.h>
#include <stdbool.h>
#include "../common.h"
#include "../bus.h"

#define R8_A 7
#define R8_C 1
#define R8_HLMEM 6

#define R16_HL 2
#define R16_SP 3

#define R16MEM_HLI 2
#define R16MEM_HLD 3

#define FLAG_Z_MASK (1u << 7)
#define FLAG_N_MASK (1u << 6)
#define FLAG_H_MASK (1u << 5)
#define FLAG_C_MASK (1u << 4)

typedef struct Sm83 Sm83;

typedef unsigned (*Sm83OpcodeHandler)(Sm83 *sm83, uint8_t opcode);

typedef enum {
    SM83_STATE_INIT,
    SM83_STATE_RESET,
    SM83_STATE_RUNNING,
    SM83_STATE_HALTED,
    SM83_STATE_STOPPED
} Sm83State;

typedef struct {
    uint8_t b; uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint8_t a;
    uint8_t f;
    uint16_t sp;
    uint16_t pc;
} Sm83Registers;

typedef struct Sm83 {
    Sm83State state;
    Sm83Registers registers;

    Bus* bus;

    uint64_t instr_count;
    uint64_t total_cycles;

    bool halted;
    bool stopped;
    bool ime_pending;
    bool ime;
} Sm83;

void sm83_init(Sm83 *sm83, Bus* bus, GbSystemType system);
void sm83_deinit(Sm83 *sm83);
uint32_t sm83_step(Sm83 *sm83);

bool sm83_get_flag(Sm83 *sm83, uint8_t flag);
void sm83_update_flags(Sm83 *sm83, uint8_t mask, uint8_t values);

uint8_t sm83_read_r8(Sm83 *sm83, uint8_t index);
void sm83_write_r8(Sm83 *sm83, uint8_t index, uint8_t value);

uint16_t sm83_read_r16(Sm83 *sm83, uint8_t pair);
void sm83_write_r16(Sm83 *sm83, uint8_t pair, uint16_t value);

uint16_t sm83_read_r16stk(Sm83 *sm83, uint8_t pair);
void sm83_write_r16stk(Sm83 *sm83, uint8_t pair, uint16_t value);

uint16_t sm83_read_r16mem(Sm83 *sm83, uint8_t pair);
void sm83_write_r16mem(Sm83 *sm83, uint8_t pair, uint16_t value);

uint16_t sm83_stack_pop16(Sm83 *sm83);
void sm83_stack_push16(Sm83 *sm83, uint16_t value);

uint8_t sm83_fetch8(Sm83 *sm83);
uint16_t sm83_fetch16(Sm83 *sm83);

uint8_t sm83_peek8(const Sm83 *sm83, uint16_t address);
uint16_t sm83_peek16(const Sm83 *sm83, uint16_t address);

uint16_t sm83_read_pc(const Sm83 *sm83);

#endif
