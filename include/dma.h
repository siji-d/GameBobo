#pragma once
#include <common.h>

typedef struct {
    bool active;
    u8 byte;
    u8 val;
    u8 delay;

} dmaContext;



void dma_init(u8 start);
void dma_tick();

bool dma_transferring();