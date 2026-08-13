#pragma once
#include <cpu.h>
#include <bus.h>
#include <emu.h>
#include <dbg.h>
#include <timer.h>
#include <interrupts.h>


cpuContext ctx = {0};

void cpu_init() {
    ctx.regs.pc = 0x100;
    ctx.regs.sp = 0xFFFE;
    *((short *)&ctx.regs.a) = 0xB001;
    *((short *)&ctx.regs.b) = 0x1300;
    *((short *)&ctx.regs.d) = 0xD800;
    *((short *)&ctx.regs.h) = 0x4D01;
    ctx.ie_register = 0;
    ctx.itr_flags = 0;
    ctx.ime_flag = false;
    ctx.enabling_ime = false;

    get_timer_context()->div = 0xABCC;
}


static void fetch_instruction() {
    ctx.opcode = bus_read(ctx.regs.pc++);
    ctx.inst = instruction_by_opcode(ctx.opcode);
}


static void execute() {
    IN_PROC proc = inst_get_processor(ctx.inst->type);

    if (!proc){
        NO_IMP;
    }

    proc(&ctx);
}

static void step_log(u16 pc) {
    char operands[16];
    char flags[16];

    sprintf(flags, "%c%c%c%c", 
        (BIT(ctx.regs.f, 7) ? 'Z' : '-'),
        (BIT(ctx.regs.f, 6) ? 'N' : '-'),
        (BIT(ctx.regs.f, 5) ? 'H' : '-'),
        (BIT(ctx.regs.f, 4) ? 'C' : '-'));

    char inst[16];
    inst_to_str(&ctx, inst);
    
    if (ctx.inst->mode == AM_IMP) { 
        snprintf(operands, sizeof(operands), "(%02X)", bus_read(pc));
    } else {
        snprintf(operands, sizeof(operands), "(%02X %02X %02X)", bus_read(pc), bus_read(pc + 1), bus_read(pc + 2));
    }

    printf("%08llX  %04X: %-12s %-11s A: %02X BC: %02X%02X DE: %02X%02X, HL: %02X%02X, FLAGS: %4s, SP: 0x%04X\n",
        emu_get_context()->ticks, pc, inst, operands, ctx.regs.a, ctx.regs.b, ctx.regs.c, ctx.regs.d, ctx.regs.e, ctx.regs.h, ctx.regs.l,
       flags, ctx.regs.sp);

       dbg_update();
       dbg_print();
}

bool cpu_step() {
    if (!ctx.halted) {

        u16 current_pc = ctx.regs.pc;
        fetch_instruction();
        emu_cycles(1);
        fetch_data();

        step_log(current_pc);

        execute();

        // if (emu_get_context()->ticks >= 0x639) {
        //     printf("reached 0x639 ticks, stopping early :P\n");
        //     exit(-9);
        // }
    } else {
        emu_cycles(1);
        
        if (ctx.itr_flags) {
            ctx.halted = false;
        }
    } 

    if (ctx.ime_flag) {
        cpu_handle_interrupt(&ctx);
        ctx.enabling_ime = false;
    }
    
    if (ctx.enabling_ime) {
        ctx.ime_flag = true;

    }

    return true;
}

u8 get_ie_register() {
    return ctx.ie_register;
}

void set_ie_register(u8 val) {
    ctx.ie_register = val;
}

void cpu_request_interrupt(interruptType t) {
    ctx.itr_flags |= t;
}