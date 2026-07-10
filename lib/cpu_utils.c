#include <cpu.h>

extern cpuContext ctx;

u16 reverse(u16 n) {
    return ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
}

u16 read_reg(regType reg) {
    switch(reg) {
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

bool is16Bit(regType reg) {
    return reg >= RG_AF;
}

void set_reg(regType reg, u16 val) {
    switch(reg) {
        case RG_A: ctx.regs.a = (val & 0x00FF); break;
        case RG_F: ctx.regs.f = (val & 0x00FF); break;
        case RG_B: ctx.regs.b = (val & 0x00FF); break;
        case RG_C: ctx.regs.c = (val & 0x00FF); break;
        case RG_D: ctx.regs.d = (val & 0x00FF); break;
        case RG_E: ctx.regs.e = (val & 0x00FF); break;
        case RG_H: ctx.regs.h = (val & 0x00FF); break;
        case RG_L: ctx.regs.l = (val & 0x00FF); break;

        case RG_AF: *((u16 *)&ctx.regs.a) = reverse(val); break;
        case RG_BC: *((u16 *)&ctx.regs.b) = reverse(val); break;
        case RG_DE: *((u16 *)&ctx.regs.d) = reverse(val); break;
        case RG_HL: *((u16 *)&ctx.regs.h) = reverse(val); break;

        case RG_PC: ctx.regs.pc = val; break;
        case RG_SP: ctx.regs.sp = val; break;
        case RG_NONE: break;
        default: break;
    }
}

cpuRegisters *get_cpu_regs() {
    return &ctx.regs;
}