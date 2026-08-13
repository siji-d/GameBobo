#include <interrupts.h>
#include <stack.h>

/*      INTERRUPT VECTORS      */
u16 VBLANK_START = 0x40;
u16 LCD_STAT_START = 0x48;
u16 TIMER_START = 0x50;
u16 SERIAL_START = 0x58;
u16 JOYPAD_START = 0x60;
/*      INTERRUPT VECTORS      */

void itr_handle(cpuContext *ctx, u16 addr) {
    stack_push16(ctx->regs.pc);
    ctx->regs.pc = addr;
}

bool itr_check(cpuContext *ctx, u16 addr, interruptType itr) {
    if (ctx->itr_flags & itr && ctx->ie_register & itr) {
        itr_handle(ctx, addr);
        ctx->itr_flags &= ~itr;
        ctx->halted = false;
        ctx->ime_flag = false;

        return true;
    }

    return false;
}

void cpu_handle_interrupt(cpuContext *ctx) {
    if (itr_check(ctx, VBLANK_START, IT_VBLANK)) {
    } else if (itr_check(ctx, LCD_STAT_START, IT_LCD_STAT)) {

    } else if (itr_check(ctx, TIMER_START, IT_TIMER)) {
        
    } else if (itr_check(ctx, SERIAL_START, IT_SERIAL)) {
        
    } else if (itr_check(ctx, JOYPAD_START, IT_JOYPAD)) {
        
    }
}
