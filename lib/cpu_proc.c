#include <cpu.h>
#include <common.h>
#include <emu.h>

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

static void proc_none(cpuContext *ctx) {
    printf("NOT YET IMPLEMENTED -- proc_none\n");
    exit(-8);
}

static void proc_nop(cpuContext *ctx) {
   //uh
}

static void proc_ld(cpuContext *ctx) {
    //TO DO frr...
}


static void proc_jp(cpuContext *ctx) {
    if (check_cond(ctx)) {
        ctx->regs.pc = ctx->fetched_data;
        //emu_cycles(1);
    }
}

static void proc_xor(cpuContext *ctx) {
    ctx->regs.a ^= (ctx->fetched_data & 0x00FF);
    set_cpu_flags(ctx, ctx->regs.a, 0, 0, 0);
}

static void proc_di(cpuContext *ctx) {
    ctx->interrupts_enabled = false;
}

IN_PROC processors[] = {
    [IN_NONE] = proc_none,
    [IN_NOP] = proc_nop,
    [IN_LD] = proc_ld,
    // IN_INC
    // IN_DEC
    // IN_RLCA
    // IN_ADD
    // IN_RRCA
    // IN_STOP
    // IN_RLA
    // IN_JR
    // IN_RRA
    // IN_DAA
    // IN_CPL
    // IN_SCF
    // IN_CCF
    // IN_HALT
    // IN_ADC
    // IN_SUB
    // IN_SBC
    // IN_AND
    [IN_XOR] = proc_xor,
    // IN_OR
    // IN_CP
    // IN_POP
    [IN_JP] = proc_jp,
    // IN_PUSH
    // IN_RET
    // IN_CB
    // IN_CALL
    // IN_RETI
    // IN_LDH
    // IN_JPHL
    [IN_DI] = proc_di,
    // IN_EI
    // IN_RST
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
