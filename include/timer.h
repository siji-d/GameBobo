#pragma once
#include <common.h>

typedef struct {
    u16 div;
    u8 tima;
    u8 tma;
    u8 tac;
} timerContext;

void timer_init();
void timer_tick();

void timer_write(u16 addr, u8 val);
u8 timer_read(u16 addr);

timerContext *get_timer_context();