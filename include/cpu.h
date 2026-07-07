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
    cpuRegisters regs;
    u16 fetched_data;
    u16 mem_dest;
    u8 opcode;
    instruction *inst;

    bool dest_is_mem;
    bool halted;
    bool step_mode;
} cpuContext;

void cpu_init();

bool cpu_step();

u16 read_reg(regType reg);

