#ifndef ALU_H
#define ALU_H

#include <stdint.h>
#include "../common.h"

typedef struct Sm83 Sm83;

typedef enum {
    ALU_ADD,
    ALU_ADC,
    ALU_SUB,
    ALU_SBC,
    ALU_AND,
    ALU_XOR,
    ALU_OR,
    ALU_CP,
} AluOperation;

uint8_t alu_execute(Sm83 *sm83, AluOperation operation, uint8_t lhs, uint8_t rhs);

uint16_t alu_add16(Sm83 *sm83, uint16_t lhs, uint16_t rhs);
uint16_t alu_add_sp_e8(Sm83 *sm83, uint16_t sp, int8_t offset);

#endif
