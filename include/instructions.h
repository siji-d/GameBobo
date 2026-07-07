#pragma once
#include <common.h>

typedef enum {
    IN_NONE,
    IN_NOP,
    IN_LD,
    IN_INC,
    IN_DEC,
    IN_RLCA,
    IN_ADD,
    IN_RRCA,
    IN_STOP,
    IN_RLA,
    IN_JR,
    IN_RRA,
    IN_DAA,
    IN_CPL,
    IN_SCF,
    IN_CCF,
    IN_HALT,
    IN_ADC,
    IN_SUB,
    IN_SBC,
    IN_AND,
    IN_XOR,
    IN_OR,
    IN_CP,
    IN_POP,
    IN_JP,
    IN_PUSH,
    IN_RET,
    IN_CB,
    IN_CALL,
    IN_RETI,
    IN_LDH,
    IN_JPHL,
    IN_DI,
    IN_EI,
    IN_RST,
    IN_ERR,
    //CB instructions
    IN_RLC, 
    IN_RRC,
    IN_RL, 
    IN_RR,
    IN_SLA, 
    IN_SRA,
    IN_SWAP, 
    IN_SRL,
    IN_BIT, 
    IN_RES, 
    IN_SET
} inType;

typedef enum {
    AM_IMP,
    AM_R_D16,
    AM_R_R,
    AM_MR_R,
    AM_R,
    AM_R_D8,
    AM_R_MR,
    AM_R_HLI,
    AM_R_HLD,
    AM_HLI_R,
    AM_HLD_R,
    AM_R_A8,
    AM_A8_R,
    AM_HL_SPR,
    AM_D16,
    AM_D8,
    AM_D16_R,
    AM_MR_D8,
    AM_MR,
    AM_A16_R,
    AM_R_A16
} addrMode;

typedef enum {
    RG_NONE,
    RG_A, RG_F,
    RG_B, RG_C,
    RG_D, RG_E,
    RG_H, RG_L,

    RG_AF, RG_BC,
    RG_DE, RG_HL,
    RG_SP, RG_PC
} regType;

typedef enum {
    CND_NONE, CND_NZ, CND_Z, CND_NC, CND_C
} condType;

typedef struct {
    inType type;
    addrMode mode;
    regType reg_1;
    regType reg_2;
    condType cond;
    u8 param;
    //int cycles; possibly might be used to synch stuff later, 
} instruction;

instruction *instruction_by_opcode(u8 opcode);

char *inst_name(inType t);