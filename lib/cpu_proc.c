#include <cpu.h>
#include <common.h>
#include <emu.h>
#include <bus.h>
#include <stack.h>

static bool check_cond(cpuContext *ctx) {
    bool z = CPU_FLAG_Z;
    bool c = CPU_FLAG_C;

    switch(ctx->inst->cond){
        case CND_NONE: return true;
        case CND_Z: return z;
        case CND_NZ: return !z;
        case CND_C: return c;
        case CND_NC: return !c;
    }

    return false;
}

static void set_cpu_flags(cpuContext *ctx, char z, char n, char h, char c) {
    if (z != -1) {
        BIT_SET(ctx->regs.f, 7, z);
    }
    if (n != -1) {
        BIT_SET(ctx->regs.f, 6, n);
    }
    if (h != -1) {
        BIT_SET(ctx->regs.f, 5, h);
    }
    if (c != -1) {
        BIT_SET(ctx->regs.f, 4, c);
    }

}

static void goto_addr(cpuContext *ctx, u16 addr, bool pushpc) {
    if (check_cond(ctx)) {
        if (pushpc) {
            emu_cycles(2);
            stack_push16(ctx->regs.pc); 
        }
        ctx->regs.pc = addr;
        emu_cycles(1);
    }

}

static void proc_none(cpuContext *ctx) {
    printf("NOT YET IMPLEMENTED -- proc_none\n");
    exit(-8);
}

static void proc_nop(cpuContext *ctx) {
   //uh
}

static void proc_ld(cpuContext *ctx) {
    if (ctx->dest_is_mem) {

        //check if we're writing to a 16 or 8-bit register
        if (is16Bit(ctx->inst->reg_2)) {
            emu_cycles(1);
            bus_write16(ctx->mem_dest, ctx->fetched_data);
        } else {
            emu_cycles(1);
            bus_write(ctx->mem_dest, ctx->fetched_data);
        }
       
        return;
    }

    if (ctx->inst->mode == AM_HL_SPR) {
        u8 hflag = (read_reg(ctx->inst->reg_2) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
        u8 cflag = (read_reg(ctx->inst->reg_2) & 0xFF) + (ctx->fetched_data & 0xFF) >= 0x100;

        set_cpu_flags(ctx, 0, 0, hflag, cflag);
        set_reg(ctx->inst->reg_1, read_reg(ctx->inst->reg_2) + (char)ctx->fetched_data);
        return;

    }
    
    set_reg(ctx->inst->reg_1, ctx->fetched_data); 
}

static void proc_inc(cpuContext *ctx) {
    u16 val = read_reg(ctx->inst->reg_1) + 1;;
    
    if (is16Bit(ctx->inst->reg_1)) {
        emu_cycles(1);
    } 

    if (ctx->dest_is_mem && ctx->inst->reg_1 == RG_HL) {
        val = bus_read(read_reg(RG_HL)) + 1; 
        val &= 0xFF;
        bus_write(read_reg(RG_HL), val);
    } else {
        set_reg(ctx->inst->reg_1, val);
        val = read_reg(ctx->inst->reg_1);
    }

    if ((ctx->opcode & 0x3) != 0x3) {
        set_cpu_flags(ctx, val == 0, 0, (val & 0x0F) == 0, -1);
    }

}

static void proc_dec(cpuContext *ctx) {
    u16 val = ctx->inst->reg_1 - 1;;
    
    if (is16Bit(ctx->inst->reg_1)) {
        emu_cycles(1);
    } 

    if (ctx->dest_is_mem && ctx->inst->reg_1 == RG_HL) {
        val = bus_read(read_reg(RG_HL)) - 1;
        bus_write(read_reg(RG_HL), val);
    } else {
        set_reg(ctx->inst->reg_1, val);
        val = read_reg(ctx->inst->reg_1);
    }

    if ((ctx->opcode & 0xB) != 0xB) {
        set_cpu_flags(ctx, val == 0, 1, (val & 0x0F) == 0x0F, -1);
    }
}

static void proc_add(cpuContext *ctx) {
    u32 val = read_reg(ctx->inst->reg_1) + ctx->fetched_data;


    if (is16Bit(ctx->inst->reg_1)) {
        emu_cycles(1);
    }
    
    if (ctx->inst->reg_1 == RG_SP) {
        val = read_reg(RG_SP) + (char)ctx->fetched_data;
    }
    
    int z = (val & 0xFF) == 0;
    int h =  (read_reg(ctx->inst->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
    int c = (int)(read_reg(ctx->inst->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) >= 0x100;

    if (is16Bit(ctx->inst->reg_1)) {
        if (ctx->inst->reg_1 == RG_SP) {
            z = 0;
        } else {
            z = -1;
            h =  (read_reg(ctx->inst->reg_1) & 0xFFF) + (ctx->fetched_data & 0xFFF) >= 0x1000;
            c = ((u32)(read_reg(ctx->inst->reg_1))) + ((u32)(ctx->fetched_data)) >= 0x10000;
        }
        
    }

    set_reg(ctx->inst->reg_1, val & 0xFFFF);
    set_cpu_flags(ctx, z, 0, h, c);
} 

static void proc_jr(cpuContext *ctx) {
    char rel = (char)(ctx->fetched_data & 0xFF);
    u16 dest = ctx->regs.pc + rel;
    goto_addr(ctx, dest, false);
}

static void proc_adc(cpuContext *ctx) { 
    u16 a = ctx->regs.a;
    u16 f = ctx->fetched_data;
    u16 c = CPU_FLAG_C;

    ctx->regs.a = (a + f + c) & 0xFF;

    set_cpu_flags(ctx, ctx->regs.a == 0, 0, ((a & 0xF) + (f & 0xF) + c) >= 0x100, ( a + f + c ) >= 0x100); 
}

static void proc_sub(cpuContext *ctx) { 
    u8 val = read_reg(ctx->inst->reg_1) - ctx->fetched_data;
    
    int z = val == 0;
    int h = ((int)read_reg(ctx->inst->reg_1) & 0xF) - ((int)ctx->fetched_data & 0xF) < 0;
    int c = (int)read_reg(ctx->inst->reg_1) - (int)ctx->fetched_data < 0;

    set_reg(ctx->inst->reg_1, val);
    set_cpu_flags(ctx, z, 1, h, c);
}

static void proc_sbc(cpuContext *ctx) { 
    u16 val = read_reg(ctx->inst->reg_1) + CPU_FLAG_C;
    
    int z = read_reg(ctx->inst->reg_1) - val == 0;
    int h = ((int)read_reg(ctx->inst->reg_1) & 0xF) - ((int)ctx->fetched_data & 0xF) - (int)(CPU_FLAG_C) < 0;
    int c = (int)read_reg(ctx->inst->reg_1) - (int)ctx->fetched_data  - (int)(CPU_FLAG_C) < 0;

    set_reg(ctx->inst->reg_1, read_reg(ctx->inst->reg_1) - val );
    set_cpu_flags(ctx, z, 1, h, c);
}

static void proc_pop(cpuContext *ctx) {
    u16 lo = stack_pop();
    emu_cycles(1);
    u16 val = lo | (stack_pop() << 8);
    emu_cycles(1);
    
    if (ctx->inst->reg_1 == RG_AF) {
        set_reg(ctx->inst->reg_1, val & 0xFFF0);
    } else {
        set_reg(ctx->inst->reg_1, val);
    }
    
}

static void proc_jp(cpuContext *ctx) {
    goto_addr(ctx, ctx->fetched_data, false);
}

static void proc_push(cpuContext *ctx) {
    u16 hi = ((read_reg(ctx->inst->reg_1) >> 8) & 0xFF);
    emu_cycles(1);
    stack_push(hi);
    
    u16 lo = (read_reg(ctx->inst->reg_1) & 0xFF);
    emu_cycles(1);
    stack_push(lo);
    
    emu_cycles(1); 
}

static void proc_ret(cpuContext *ctx) {
    if (ctx->inst->cond != CND_NONE) {
        emu_cycles(1);
    }

    if (check_cond(ctx)) {
        u16 lo = stack_pop();
        emu_cycles(1);
        u16 hi = stack_pop();
        emu_cycles(1);

        u16 val = (hi << 8) | lo;
        ctx->regs.pc = val;
        emu_cycles(1);
    }
}


static void proc_call(cpuContext *ctx) {
    goto_addr(ctx, ctx->fetched_data, true);
}

static void proc_reti(cpuContext *ctx) {
    ctx->ime_flag = true;
    proc_ret(ctx);   
}

static void proc_xor(cpuContext *ctx) {
    ctx->regs.a ^= (ctx->fetched_data & 0x00FF);
    set_cpu_flags(ctx, ctx->regs.a, 0, 0, 0);
}

static void proc_ldh(cpuContext *ctx) {
    if (ctx->dest_is_mem) {
        bus_write(ctx->mem_dest, ctx->regs.a);
    } else {
        set_reg(RG_A, (0xFF00 | bus_read(ctx->fetched_data)));
    }
}

static void proc_di(cpuContext *ctx) {
    ctx->ime_flag = false;
}

static void proc_rst(cpuContext *ctx) {
    goto_addr(ctx, ctx->inst->param, true);
}


IN_PROC processors[] = {
    [IN_NONE] = proc_none,
    [IN_NOP] = proc_nop,
    [IN_LD] = proc_ld,
    [IN_INC] = proc_inc,
    [IN_DEC] = proc_dec,
    // IN_RLCA
    [IN_ADD] = proc_add,
    // IN_RRCA
    // IN_STOP
    // IN_RLA
    [IN_JR] = proc_jr,
    // IN_RRA
    // IN_DAA
    // IN_CPL
    // IN_SCF
    // IN_CCF
    // IN_HALT
    [IN_ADC] = proc_adc,
    [IN_SUB] = proc_sub,
    [IN_SBC] = proc_sbc,
    // IN_AND
    [IN_XOR] = proc_xor,
    // IN_OR
    // IN_CP
    [IN_POP] = proc_pop,
    [IN_JP] = proc_jp,
    [IN_PUSH] = proc_push,
    [IN_RET] = proc_ret,
    // IN_CB
    [IN_CALL] = proc_call,
    [IN_RETI] = proc_reti,
    [IN_LDH] = proc_ldh,
    // IN_JPHL
    [IN_DI] = proc_di,
    // IN_EI
    [IN_RST] = proc_rst,
    // IN_ERR

    // //CB instructions
    // IN_RLC 
    // IN_RRC
    // IN_RL 
    // IN_RR
    // IN_SLA 
    // IN_SRA
    // IN_SWAP 
    // IN_SRL
    // IN_BIT 
    // IN_RES 
    // IN_SET
};

IN_PROC inst_get_processor(inType type) {
    return processors[type];
};
