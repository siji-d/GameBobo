#include <stack.h>
#include <cpu.h>
#include <bus.h>

void stack_push(u8 val) {
    get_cpu_regs()->sp--;
    bus_write(get_cpu_regs()->sp, val);
}

void stack_push16(u16 val) {
    stack_push((val >> 8) & 0x00FF);
    stack_push(val & 0x00FF);
}

u8 stack_pop() {
    return bus_read(get_cpu_regs()->sp++);
}

u16 stack_pop16() {
    u16 lo = stack_pop();
    return lo | (stack_pop() << 8);
}