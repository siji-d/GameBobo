#include <cpu.h>
#include <bus.h>
#include <emu.h>

cpuContext ctx = {0};

void cpu_init() {
    ctx.regs.pc = 0x100u;
}

static void fetch_instruction() {
    ctx.opcode = bus_read(ctx.regs.pc++);
    ctx.inst = instruction_by_opcode(ctx.opcode);

    if (ctx.inst->inst == IN_NONE) {
        printf("unkown instruction %02X, exiting \n", ctx.opcode);
        exit(-8);
    }
}

static void fetch_data() {
    ctx.mem_dest = 0;
    ctx.dest_is_mem = false;

    switch (ctx.inst->mode) {
        case AM_IMP:
            printf("Implied Addressing Mode\n");
            ctx.fetched_data = 0x0u;
            return;

        case AM_R:
            printf("Load Register\n"); 
            ctx.fetched_data = read_reg(ctx.inst->reg_1);
            return;

        case AM_R_D8:
            printf("direct u8\n");
            ctx.fetched_data = bus_read(ctx.regs.pc);
            //emu_cycles(1);
            ctx.regs.pc++;
            return; 

        case AM_D16:
        {
            printf("direct u16\n");
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
    printf("\t\tNot really.....\n");
}

bool cpu_step() {
    if (!ctx.halted) {

        u16 current_pc = ctx.regs.pc;
        fetch_instruction();
        fetch_data();

        printf("EXECUTING:  %02X %4.4X, PC:  %4.4X\n", ctx.opcode, ctx.fetched_data, current_pc);
        execute();
        printf("successful step\n");

        // if (ctx.regs.pc >= 0x200) {
        //     printf("reached 0x200, stopping early :P\n");
        //     exit(-9);
        // }
    }
    
    return true;
}