#ifndef OPCODES_H
#define OPCODES_H

#include <stdio.h>
#include <stdint.h>
#include "../common.h"

#define OPCODE_X(op) (((op) >> 6) & 0x03u)
#define OPCODE_Y(op) (((op) >> 3) & 0x07u)
#define OPCODE_Z(op) ((op) & 0x07u)
#define OPCODE_P(op) (((op) >> 4) & 0x03u)
#define OPCODE_Q(op) (((op) >> 3) & 0x01u)

#define OPCODE_COND(op) (OPCODE_Y(op) & 0x03);

typedef struct Sm83 Sm83;

typedef enum {
    OP_NOP,
    OP_LD,
    OP_LDH,

    OP_INC,
    OP_DEC,

    OP_ADD,
    OP_ADC,
    OP_SUB,
    OP_SBC,
    OP_AND,
    OP_XOR,
    OP_OR,
    OP_CP,

    OP_DAA,
    OP_CPL,
    OP_SCF,
    OP_CCF,

    OP_RLCA,
    OP_RRCA,
    OP_RLA,
    OP_RRA,

    OP_RLC,
    OP_RRC,
    OP_RL,
    OP_RR,
    OP_SLA,
    OP_SRA,
    OP_SWAP,
    OP_SRL,

    OP_BIT,
    OP_RES,
    OP_SET,

    OP_JP,
    OP_JR,
    OP_CALL,
    OP_RET,
    OP_RETI,
    OP_RST,

    OP_PUSH,
    OP_POP,

    OP_HALT,
    OP_STOP,
    OP_DI,
    OP_EI,
    OP_CB
} Sm83Operation;

typedef enum {
    OPERAND_NONE,
    OPERAND_R8,
    OPERAND_R16,
    OPERAND_MEM_R8,
    OPERAND_MEM_R16,
    OPERAND_HLI,
    OPERAND_HLD,
    OPERAND_IMM8,
    OPERAND_IMM16,
    OPERAND_MEM_HIGH_IMM8,
    OPERAND_MEM_IMM16,
    OPERAND_MEM_HIGH_C,
    OPERAND_SIGNED_IMM8,
    OPERAND_COND,
    OPERAND_BIT
} Sm83DisasmOperandKind;



typedef struct {
    Sm83DisasmOperandKind kind;
    uint8_t value;
} Sm83DisasmOperand;

typedef unsigned (*Sm83OpcodeHandler)(Sm83 *sm83, uint8_t opcode);

typedef struct {
    Sm83OpcodeHandler handler;
    uint8_t length;
    Sm83Operation operation;
    Sm83DisasmOperand dst;
    Sm83DisasmOperand src;
} Sm83OpcodeDescriptor;

inline unsigned sm83_opcode_execute(Sm83 *sm83, uint8_t opcode);
void sm83_disassemble(Sm83 *sm83, uint8_t opcode, uint16_t pc, char *out, size_t out_size);
#endif
