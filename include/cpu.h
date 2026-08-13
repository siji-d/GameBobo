#pragma once
#include <common.h>
#include <instructions.h> 

typedef struct {
    u8 a;
    u8 f;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
    u16 sp;
    u16 pc;
} cpuRegisters;

typedef struct {
    //registers
    cpuRegisters regs;

    //current state
    u16 fetched_data;
    u16 mem_dest;
    u8 opcode;
    instruction *inst; 
    bool dest_is_mem;
    bool ime_flag;
    bool enabling_ime;
    u8 itr_flags;
    u8 ie_register;

    //emu stuff
    bool halted;
    bool step_mode;

    
} cpuContext;

void cpu_init();

bool cpu_step();

u16 read_reg(regType reg);
void set_reg(regType reg, u16 val);

u8 read_reg8(regType reg);
void set_reg8 (regType reg, u8 val);

u8 get_ie_register();
void set_ie_register(u8 val);

u8 get_itr_flags();
void set_itr_flags(u8 flags);

void inst_to_str(cpuContext *ctx, char *str);

typedef void (*IN_PROC)(cpuContext *);

IN_PROC inst_get_processor(inType type);

cpuRegisters *get_cpu_regs();



void fetch_data();

bool is16Bit(regType reg);

#define CPU_FLAG_Z BIT(ctx->regs.f, 7)
#define CPU_FLAG_N BIT(ctx->regs.f, 6)
#define CPU_FLAG_H BIT(ctx->regs.f, 5)
#define CPU_FLAG_C BIT(ctx->regs.f, 4)