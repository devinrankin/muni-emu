#include "sm83.h"
#include "sm83_opcodes.h"

static uint32_t handle_interrupt(Sm83 *sm83);

void sm83_init(Sm83 *sm83, Bus* bus, GbSystemType system) {
    *sm83 = (Sm83) {
        .state = SM83_STATE_INIT,
        .halted = false,
        .total_cycles = 0,
        .ime = false,
    };

    switch (system) {
    case SYSTEM_DMG:
        sm83->registers = (Sm83Registers) {
            .pc = 0x100,
            .sp = 0xFFFE,
            .a = 0x01,
            .f = 0x00,
            .b = 0xFF,
            .c = 0x13,
            .d = 0x00,
            .e = 0xC1,
            .h = 0x84,
            .l = 0x03
        };
    case SYSTEM_CGB:
        sm83->registers = (Sm83Registers) {
            .pc = 0x100,
            .sp = 0xFFFE,
            .a = 0x11,
            .f = 0x80,
            .b = 0x00,
            .c = 0x00,
            .d = 0xFF,
            .e = 0x56,
            .h = 0x00,
            .l = 0x0D
        };
    case SYSTEM_SGB:
        sm83->registers = (Sm83Registers) {
            .pc = 0x100,
            .sp = 0xFFFE,
            .a = 0x01,
            .f = 0x00,
            .b = 0x00,
            .c = 0x14,
            .d = 0x00,
            .e = 0x00,
            .h = 0xC0,
            .l = 0x60
        };
    default:
        sm83->registers = (Sm83Registers) {0};
    }
    sm83->bus = bus;
}

/* Perform one step of the fetch, decode, execute loop. */
uint32_t sm83_step(Sm83 *sm83) {
    uint32_t cycles;
    bool enable_ime = sm83->ime_pending;
    sm83->ime_pending = false;
    uint16_t pc = sm83->registers.pc;

    uint8_t interrupt_pending = sm83->bus->interrupt_enable & sm83->bus->interrupt_flags;
    if (sm83->halted) {
        if (interrupt_pending == 0) {
            cycles = 1;
            sm83->total_cycles += cycles;
            return cycles;
        }
        sm83->halted = false;
    }

    if (sm83->ime && interrupt_pending != 0) {
        cycles = handle_interrupt(sm83);
        sm83->total_cycles += cycles;
        return cycles;
    }

    uint8_t opcode = sm83_fetch8(sm83);
    cycles = sm83_opcode_execute(sm83, opcode);

    if (enable_ime) sm83->ime = true;
    
    // trace_log(sm83, opcode, pc);
    char out[64];
    sm83_disassemble(sm83, opcode, pc, out, sizeof out);
    printf("%lu->[$%0.4X]: IR: (%0.2X) | Dism: %s\n", sm83->instr_count++, pc, opcode, out);

    sm83->total_cycles += cycles;
    return cycles;
}

/* Returns the contents of an 8-bit register, or a byte at the address pointed to by the contents of the 16-bit HL register. */
uint8_t sm83_read_r8(Sm83 *sm83, uint8_t index) {
    switch (index) {
    case 0: return sm83->registers.b;
    case 1: return sm83->registers.c;
    case 2: return sm83->registers.d;
    case 3: return sm83->registers.e;
    case 4: return sm83->registers.h;
    case 5: return sm83->registers.l;
    case 6: return bus_read(sm83->bus, BUS_CALLER_SM83, sm83_read_r16(sm83, R16_HL));
    case 7: return sm83->registers.a;
    default: NO_IMPL
    }
}

/* Stores a value into an 8-bit register, or into the address pointed to by the contents of the 16-bit HL register. */
void sm83_write_r8(Sm83 *sm83, uint8_t index, uint8_t value) {
    switch (index) {
    case 0: sm83->registers.b = value; break;
    case 1: sm83->registers.c = value; break;
    case 2: sm83->registers.d = value; break;
    case 3: sm83->registers.e = value; break;
    case 4: sm83->registers.h = value; break;
    case 5: sm83->registers.l = value; break;
    case 6: bus_write(sm83->bus, BUS_CALLER_SM83, sm83_read_r16(sm83, R16_HL), value); break;
    case 7: sm83->registers.a = value; break;
    default: NO_IMPL
    }
}

/* Returns the contents of a 16-bit register, or the stack pointer. */
uint16_t sm83_read_r16(Sm83 *sm83, uint8_t pair) {
    switch (pair) {
    case 0: return (uint16_t)(sm83->registers.b << 8) | (uint16_t)sm83->registers.c;
    case 1: return (uint16_t)(sm83->registers.d << 8) | (uint16_t)sm83->registers.e;
    case 2: return (uint16_t)(sm83->registers.h << 8) | (uint16_t)sm83->registers.l;
    case 3: return sm83->registers.sp;
    default: NO_IMPL
    }
}

uint16_t sm83_read_r16stk(Sm83 *sm83, uint8_t pair) {
    switch (pair) {
    case 0: return (uint16_t)(sm83->registers.b << 8) | (uint16_t)sm83->registers.c;
    case 1: return (uint16_t)(sm83->registers.d << 8) | (uint16_t)sm83->registers.e;
    case 2: return (uint16_t)(sm83->registers.h << 8) | (uint16_t)sm83->registers.l;
    case 3: return (uint16_t)(sm83->registers.a << 8) | (uint16_t)sm83->registers.f;
    default: NO_IMPL
    }
}

uint16_t sm83_read_r16mem(Sm83 *sm83, uint8_t pair) {
    switch (pair) {
    case 0: return (uint16_t)(sm83->registers.b << 8) | (uint16_t)sm83->registers.c;
    case 1: return (uint16_t)(sm83->registers.d << 8) | (uint16_t)sm83->registers.e;
    case 2: {
        uint16_t value = (uint16_t)(sm83->registers.h << 8) | (uint16_t)sm83->registers.l;
        sm83_write_r16(sm83, R16_HL, sm83_read_r16(sm83, R16_HL) + 1);
        return value; 
    }
    case 3: {
        uint16_t value = (uint16_t)(sm83->registers.h << 8) | (uint16_t)sm83->registers.l;
        sm83_write_r16(sm83, R16_HL, sm83_read_r16(sm83, R16_HL) - 1);
        return value; 
    }
    default: NO_IMPL
    }
}

/* Stores a value into a 16-bit regsiter, or into the stack pointer. */
void sm83_write_r16(Sm83 *sm83, uint8_t pair, uint16_t value) {
    switch (pair) {
    case 0: 
        sm83->registers.b = (uint8_t)(value >> 8);
        sm83->registers.c = (uint8_t)(value & 0xFF);
        break;
    case 1:
        sm83->registers.d = (uint8_t)(value >> 8);
        sm83->registers.e = (uint8_t)(value & 0xFF);
        break;
    case 2: 
        sm83->registers.h = (uint8_t)(value >> 8);
        sm83->registers.l = (uint8_t)(value & 0xFF);
        break;
    case 3: 
        sm83->registers.sp = value;
        break;
    default: NO_IMPL
    }
}

void sm83_write_r16stk(Sm83 *sm83, uint8_t pair, uint16_t value) {
    switch (pair) {
    case 0: 
        sm83->registers.b = (uint8_t)(value >> 8);
        sm83->registers.c = (uint8_t)(value & 0xFF);
        break;
    case 1:
        sm83->registers.d = (uint8_t)(value >> 8);
        sm83->registers.e = (uint8_t)(value & 0xFF);
        break;
    case 2: 
        sm83->registers.h = (uint8_t)(value >> 8);
        sm83->registers.l = (uint8_t)(value & 0xFF);
        break;
    case 3: 
        sm83->registers.a = (uint8_t)(value >> 8);
        sm83_update_flags(sm83, FLAG_N_MASK | FLAG_Z_MASK | FLAG_H_MASK | FLAG_C_MASK, (uint8_t)(value & 0xFF));
        break;
    default: NO_IMPL
    }
}

void sm83_write_r16mem(Sm83 *sm83, uint8_t pair, uint16_t value) {
    switch (pair) {
    case 0: 
        sm83->registers.b = (uint8_t)(value >> 8);
        sm83->registers.c = (uint8_t)(value & 0xFF);
        break;
    case 1:
        sm83->registers.d = (uint8_t)(value >> 8);
        sm83->registers.e = (uint8_t)(value & 0xFF);
        break;
    case 2:
        value++;
        sm83->registers.h = (uint8_t)(value >> 8);
        sm83->registers.l = (uint8_t)(value & 0xFF);
        break;
    case 3:
        value--;
        sm83->registers.h = (uint8_t)(value >> 8);
        sm83->registers.l = (uint8_t)(value & 0xFF);
        break;
    default: NO_IMPL
    }
}

/* Returns one of the Z, N, H, or C flags.
 * Compatible masks: SM83_FLAG_Z_MASK, SM83_FLAG_N_MASK, SM83_FLAG_H_MASK, SM83_FLAG_C_MASK. */
bool sm83_get_flag(Sm83 *sm83, uint8_t flag) {
    return (sm83->registers.f & flag) != 0;
}

/* Given a mask of affected flags and their new values, updates the flag register. 
 * Example usage: sm83_update_flags(sm83, SM83_FLAG_N_MASK | SM83_FLAG_C_MASK, n | c) */
void sm83_update_flags(Sm83 *sm83, uint8_t mask, uint8_t values) {
    sm83->registers.f = (sm83->registers.f & (uint8_t)~mask) | (values & mask);

    /* Ensures the lower 4 bits of F to 0 should the above arithmetic set them. */
    sm83->registers.f &= FLAG_Z_MASK | FLAG_N_MASK | FLAG_H_MASK | FLAG_C_MASK;
}

uint16_t sm83_stack_pop16(Sm83 *sm83) {
    uint8_t low = bus_read(sm83->bus, BUS_CALLER_SM83, sm83->registers.sp);
    sm83->registers.sp++;
    uint8_t high = bus_read(sm83->bus, BUS_CALLER_SM83, sm83->registers.sp);
    sm83->registers.sp++;

    return ((uint16_t)high << 8) | low;
}

void sm83_stack_push16(Sm83 *sm83, uint16_t value) {
    uint8_t high = (uint8_t)(value >> 8);
    uint8_t low = (uint8_t)(value & 0xFF);
    
    sm83->registers.sp--;
    bus_write(sm83->bus, BUS_CALLER_SM83, sm83->registers.sp, high);

    sm83->registers.sp--;
    bus_write(sm83->bus, BUS_CALLER_SM83, sm83->registers.sp, low);
}

/* Fetches the next byte in ROM and increments the program counter. */
uint8_t sm83_fetch8(Sm83 *sm83) {
    uint8_t value = bus_read(sm83->bus, BUS_CALLER_SM83, sm83->registers.pc++);

    return value;
}

/* Fetches the next 2 bytes in ROM and increments the program counter twice. */
uint16_t sm83_fetch16(Sm83 *sm83) {
    uint8_t low = sm83_fetch8(sm83);
    uint8_t high = sm83_fetch8(sm83);

    return ((uint16_t)high << 8) | low;
}

uint8_t sm83_peek8(const Sm83 *sm83, uint16_t address) {
    return bus_read(sm83->bus, BUS_CALLER_SM83, address);
}

uint16_t sm83_peek16(const Sm83 *sm83, uint16_t address) {
    uint8_t low = sm83_peek8(sm83, address);
    uint8_t high = sm83_peek8(sm83, address + 1);

    return ((uint16_t)high << 8) | low;
}

uint16_t sm83_read_pc(const Sm83 *sm83) {
    return sm83->registers.pc;
}

static uint32_t handle_interrupt(Sm83 *sm83) {
    uint8_t interrupt_pending = sm83->bus->interrupt_flags & sm83->bus->interrupt_enable & 0x1F;
    uint8_t* interrupt_flags = &sm83->bus->interrupt_flags;
    uint16_t vector = 0;

    if (interrupt_pending == 0) {
        return 0;
    }

    if (BIT(interrupt_pending, INTERRUPT_BIT_VBLANK)) {
        BIT_CLEAR(*interrupt_flags, INTERRUPT_BIT_VBLANK);
        vector = INTERRUPT_ADDR_VBLANK;
    } else if (BIT(interrupt_pending, INTERRUPT_BIT_STAT)) {
        BIT_CLEAR(*interrupt_flags, INTERRUPT_BIT_STAT);
        vector = INTERRUPT_ADDR_STAT;
    } else if (BIT(interrupt_pending, INTERRUPT_BIT_TIMER)) {
        BIT_CLEAR(*interrupt_flags, INTERRUPT_BIT_TIMER);
        vector = INTERRUPT_ADDR_TIMER;
    } else if (BIT(interrupt_pending, INTERRUPT_BIT_SERIAL)) {
        BIT_CLEAR(*interrupt_flags, INTERRUPT_BIT_SERIAL);
        vector = INTERRUPT_ADDR_SERIAL;
    } else if (BIT(interrupt_pending, INTERRUPT_BIT_JOYPAD)) {
        BIT_CLEAR(*interrupt_flags, INTERRUPT_BIT_JOYPAD);
        vector = INTERRUPT_ADDR_JOYPAD;
    }
       
    sm83->ime = false;
    sm83->halted = false;

    sm83_stack_push16(sm83, sm83->registers.pc);
    sm83->registers.pc = vector;

    return 5;
}
