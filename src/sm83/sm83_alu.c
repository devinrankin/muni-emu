#include "sm83_alu.h"
#include "sm83.h"

static uint8_t alu_add(Sm83 *sm83, uint8_t lhs, uint8_t rhs, uint8_t carry_in);
static uint8_t alu_sub(Sm83 *sm83, uint8_t lhs, uint8_t rhs, uint8_t carry_in);
static uint8_t alu_and(Sm83 *sm83, uint8_t lhs, uint8_t rhs);
static uint8_t alu_or(Sm83 *sm83, uint8_t lhs, uint8_t rhs);
static uint8_t alu_xor(Sm83 *sm83, uint8_t lhs, uint8_t rhs);

uint8_t alu_execute(Sm83 *sm83, AluOperation operation, uint8_t lhs, uint8_t rhs) {
    switch (operation) {
    case ALU_ADD:
        return alu_add(sm83, lhs, rhs, 0);
    case ALU_ADC:
        return alu_add(sm83, lhs, rhs, 1);
    case ALU_SUB:
        return alu_sub(sm83, lhs, rhs, 0);
    case ALU_SBC:
        return alu_sub(sm83, lhs, rhs, 1);
    case ALU_AND:
        return alu_and(sm83, lhs, rhs);
    case ALU_XOR:
        return alu_xor(sm83, lhs, rhs);
    case ALU_OR:
        return alu_or(sm83, lhs, rhs);
    case ALU_CP:
        return alu_sub(sm83, lhs, rhs, 0);
    }
}

uint16_t alu_add16(Sm83 *sm83, uint16_t lhs, uint16_t rhs) {
    uint32_t sum = (uint32_t)lhs + (uint32_t)rhs;
    uint16_t result = (uint16_t)sum;

    uint8_t flags = 0;

    if (((lhs & 0x0FFF) + (rhs & 0x0FFF)) > 0x0FFF) flags |= FLAG_H_MASK;
    if (sum > 0xFFFF) flags |= FLAG_C_MASK;

    sm83_update_flags(sm83, FLAG_N_MASK | FLAG_H_MASK | FLAG_C_MASK, flags);

    return result;
}

uint16_t alu_add_sp_e8(Sm83 *sm83, uint16_t sp, int8_t offset) {
    uint8_t immediate = (uint8_t)offset;

    uint16_t result = (uint16_t)((int32_t)sp + offset);

    uint8_t flags = 0;

    if (((sp & 0x000F) + (immediate & 0x0F)) > 0x0F) flags |= FLAG_H_MASK;
    if (((sp & 0x00FF) + immediate) > 0xFF) flags |= FLAG_C_MASK;

    sm83_update_flags(sm83, FLAG_Z_MASK | FLAG_N_MASK | FLAG_H_MASK | FLAG_C_MASK, flags);

    return result;
}

static uint8_t alu_add(Sm83 *sm83, uint8_t lhs, uint8_t rhs, uint8_t carry_in) {
    uint16_t sum = lhs + rhs + carry_in;
    uint8_t result = (uint8_t)sum;

    uint8_t flags = 0;

    if(result == 0) flags |= FLAG_Z_MASK;
    if((lhs & 0x0F) + (rhs & 0x0F) + carry_in > 0x0F) flags |= FLAG_H_MASK;
    if(sum > 0xFF) flags |= FLAG_C_MASK;

    sm83_update_flags(sm83, FLAG_Z_MASK | FLAG_N_MASK | FLAG_H_MASK | FLAG_C_MASK, flags);
    return (uint8_t)result;
}

static uint8_t alu_sub(Sm83 *sm83, uint8_t lhs, uint8_t rhs, uint8_t carry_in) {
    uint8_t result = (uint8_t)(lhs - rhs - carry_in);

    uint8_t flags = 0;

    if(result == 0) flags |= FLAG_Z_MASK;
    flags |= FLAG_N_MASK;
    if((lhs & 0x0F) < (rhs & 0x0F) + carry_in) flags |= FLAG_H_MASK;
    if((uint16_t)lhs < (uint16_t)rhs + carry_in) flags |= FLAG_C_MASK;

    sm83_update_flags(sm83, FLAG_Z_MASK | FLAG_N_MASK | FLAG_H_MASK | FLAG_C_MASK, flags);
    return result;
}

static uint8_t alu_and(Sm83 *sm83, uint8_t lhs, uint8_t rhs) {
    uint8_t result = lhs & rhs;

    uint8_t flags = 0;

    if(result == 0) flags |= FLAG_Z_MASK;
    flags |= FLAG_H_MASK;

    sm83_update_flags(sm83, FLAG_Z_MASK | FLAG_N_MASK | FLAG_H_MASK | FLAG_C_MASK, flags);
    return result;
}

static uint8_t alu_or(Sm83 *sm83, uint8_t lhs, uint8_t rhs) {
    uint8_t result = lhs | rhs;

    uint8_t flags = 0;

    if(result == 0) flags |= FLAG_Z_MASK;

    sm83_update_flags(sm83, FLAG_Z_MASK, flags);
    return result;
}

static uint8_t alu_xor(Sm83 *sm83, uint8_t lhs, uint8_t rhs) {
    uint8_t result = lhs ^ rhs;

    uint8_t flags = 0;
    
    if(result == 0) flags |= FLAG_Z_MASK;

    sm83_update_flags(sm83, FLAG_Z_MASK | FLAG_N_MASK | FLAG_H_MASK | FLAG_C_MASK, flags);
    return result;
}

