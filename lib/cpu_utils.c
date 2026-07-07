#include <cpu.h>

extern cpuContext ctx;

u16 reverse(u16 n) {
    return ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
}

u16 read_reg(regType rt) {
    switch(rt) {
        case RG_A: return ctx.regs.a;
        case RG_F: return ctx.regs.f;
        case RG_B: return ctx.regs.b;
        case RG_C: return ctx.regs.c;
        case RG_D: return ctx.regs.d;
        case RG_E: return ctx.regs.e;
        case RG_H: return ctx.regs.h;
        case RG_L: return ctx.regs.l;

        case RG_AF: return reverse(*((u16 *)&ctx.regs.a));
        case RG_BC: return reverse(*((u16 *)&ctx.regs.b));
        case RG_DE: return reverse(*((u16 *)&ctx.regs.d));
        case RG_HL: return reverse(*((u16 *)&ctx.regs.h));

        case RG_PC: return ctx.regs.pc;
        case RG_SP: return ctx.regs.sp;
        default: return 0;
    }
}