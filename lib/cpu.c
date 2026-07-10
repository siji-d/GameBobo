#include <cpu.h>
#include <bus.h>
#include <emu.h>

cpuContext ctx = {0};

void cpu_init() {
    ctx.regs.pc = 0x100;
    ctx.regs.a = 0x01;
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
    char operands[11];
    if (ctx.inst->mode == AM_IMP) { 
        snprintf(operands, sizeof(operands), "(%02X)", bus_read(pc));
    } else {
        snprintf(operands, sizeof(operands), "(%02X %02X %02X)", bus_read(pc), bus_read(pc + 1), bus_read(pc + 2));
    }

    printf("%04X: %-7s %-11s A: 0x%02X BC: 0x%02X%02X DE: 0x%02X%02X, HL: 0x%02X%02X, F: 0b%d%d%d%d, SP: 0x%04X\n",
        pc, inst_name(ctx.inst->type), operands, ctx.regs.a, ctx.regs.b, ctx.regs.c, ctx.regs.d, ctx.regs.e, ctx.regs.h, ctx.regs.l,
        BIT(ctx.regs.f, 7), BIT(ctx.regs.f, 6), BIT(ctx.regs.f, 5), BIT(ctx.regs.f, 4), ctx.regs.sp);
}

bool cpu_step() {
    if (!ctx.halted) {

        u16 current_pc = ctx.regs.pc;
        fetch_instruction();
        fetch_data();

        step_log(current_pc);

        execute();

        // if (ctx.regs.pc >= 0x200) {
        //     printf("reached 0x200, stopping early :P\n");
        //     exit(-9);
        // }
    }
    
    return true;
}

u8 get_ie_register(){
    return ctx.ie_register;
}

void set_ie_register(u8 val) {
    ctx.ie_register = val;
}

u8 get_if_register() {
    return ctx.if_register;
}

void set_if_register(u8 val) {
    ctx.if_register = val;
}
