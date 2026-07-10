#include <common.h>
#include <cpu.h>
#include <bus.h>
#include <emu.h>

extern cpuContext ctx;

void fetch_data() {
    ctx.mem_dest = 0;
    ctx.dest_is_mem = false;

    if (ctx.inst == NULL) {
        return;
    }

    switch(ctx.inst->mode) {
        case AM_IMP: return;

        case AM_R:
            ctx.fetched_data = read_reg(ctx.inst->reg_1);
            return;

        case AM_R_R:
            ctx.fetched_data = read_reg(ctx.inst->reg_2);
            return;

        case AM_R_D8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;

        case AM_R_D16:
        case AM_D16: {
            u16 lo = bus_read(ctx.regs.pc);
            emu_cycles(1);

            u16 hi = bus_read(ctx.regs.pc + 1);
            emu_cycles(1);

            ctx.fetched_data = lo | (hi << 8);

            ctx.regs.pc += 2;

            return;
        }

        case AM_MR_R:
            ctx.fetched_data = read_reg(ctx.inst->reg_2);
            ctx.mem_dest = read_reg(ctx.inst->reg_1);
            ctx.dest_is_mem = true;

            if (ctx.inst->reg_1 == RG_C) {
                ctx.mem_dest |= 0xFF00;
            }

            return;

        case AM_R_MR: {
            u16 addr = read_reg(ctx.inst->reg_2);

            if (ctx.inst->reg_2 == RG_C) {
                addr |= 0xFF00;
            }

            ctx.fetched_data = bus_read(addr);
            emu_cycles(1);

        } return;

        case AM_R_HLI:
            ctx.fetched_data = bus_read(read_reg(ctx.inst->reg_2));
            emu_cycles(1);
            set_reg(RG_HL, read_reg(RG_HL) + 1);
            return;

        case AM_R_HLD:
            ctx.fetched_data = bus_read(read_reg(ctx.inst->reg_2));
            emu_cycles(1);
            set_reg(RG_HL, read_reg(RG_HL) - 1);
            return;

        case AM_HLI_R:
            ctx.fetched_data = read_reg(ctx.inst->reg_2);
            ctx.mem_dest = read_reg(ctx.inst->reg_1);
            ctx.dest_is_mem = true;
            set_reg(RG_HL, read_reg(RG_HL) + 1);
            return;

        case AM_HLD_R:
            ctx.fetched_data = read_reg(ctx.inst->reg_2);
            ctx.mem_dest = read_reg(ctx.inst->reg_1);
            ctx.dest_is_mem = true;
            set_reg(RG_HL, read_reg(RG_HL) - 1);
            return;

        case AM_R_A8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;

        case AM_A8_R:
            ctx.mem_dest = bus_read(ctx.regs.pc) | 0xFF00;
            ctx.dest_is_mem = true;
            emu_cycles(1);
            ctx.regs.pc++;
            return;

        case AM_HL_SPR:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;

        case AM_D8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++; 
            return;

        case AM_A16_R:
        case AM_D16_R: {
            u16 lo = bus_read(ctx.regs.pc);
            emu_cycles(1);

            u16 hi = bus_read(ctx.regs.pc + 1);
            emu_cycles(1);

            ctx.mem_dest = lo | (hi << 8);
            ctx.dest_is_mem = true;

            ctx.regs.pc += 2;
            ctx.fetched_data = read_reg(ctx.inst->reg_2);

        } return;

        case AM_MR_D8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            ctx.mem_dest = read_reg(ctx.inst->reg_1);
            ctx.dest_is_mem = true;
            return;

        case AM_MR:
            ctx.mem_dest = read_reg(ctx.inst->reg_1);
            ctx.dest_is_mem = true;
            ctx.fetched_data = bus_read(read_reg(ctx.inst->reg_1));
            emu_cycles(1);
            return;

        case AM_R_A16: {
            u16 lo = bus_read(ctx.regs.pc);
            emu_cycles(1);

            u16 hi = bus_read(ctx.regs.pc + 1);
            emu_cycles(1);

            u16 addr = lo | (hi << 8);

            ctx.regs.pc += 2;
            ctx.fetched_data = bus_read(addr);
            emu_cycles(1);

            return;
        }
    
    default:
        printf("oops, unknown addr mode %d\n", ctx.inst->mode);
        exit(-7);
        return;
    }
}
