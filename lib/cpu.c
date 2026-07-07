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

static void fetch_data() {
    ctx.mem_dest = 0;
    ctx.dest_is_mem = false;

    switch (ctx.inst->mode) {
        case AM_IMP:
            ctx.fetched_data = 0x0;
            return;

        case AM_R:
            ctx.fetched_data = read_reg(ctx.inst->reg_1);
            return;

        case AM_R_D8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            //emu_cycles(1);
            ctx.regs.pc++;
            return; 

        case AM_D16:
        {
            u16 lo = bus_read(ctx.regs.pc);
            //emu_cycles(1);

            u16 hi = bus_read(ctx.regs.pc + 1);
            //emu_cycles(1);
            
            ctx.fetched_data = lo | (hi << 8);
            
            ctx.regs.pc += 2;
            return;
        }
    
    default:
        printf("oops, unknown addr mode %d\n", ctx.inst->mode);
        exit(-7);
        return;
    }
}

static void execute() {
    IN_PROC proc = inst_get_processor(ctx.inst->type);

    if (!proc){
        NO_IMP;
    }

    proc(&ctx);
}

static void step_log(u16 pc) {
    char *operands[10];
    if (ctx.inst->mode == AM_IMP) { 
        snprintf(operands, sizeof(operands), "(%02X)", bus_read(pc));
    } else {
        snprintf(operands, sizeof(operands), "(%02X %02X %02X)", bus_read(pc), bus_read(pc + 1), bus_read(pc + 2));
    }

    printf("%04X: %-7s %-11s A: %02X  B: %02X C: %02X F: %d%d%d%d\n",
    pc, inst_name(ctx.inst->type), operands, ctx.regs.a, ctx.regs.b, ctx.regs.c, BIT(ctx.regs.f, 7), BIT(ctx.regs.f, 6), BIT(ctx.regs.f, 5), BIT(ctx.regs.f, 4));
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