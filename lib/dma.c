#include <dma.h>
#include <ppu.h>
#include <bus.h>

static dmaContext ctx;

void dma_init(u8 start){
    ctx.active = true;
    ctx.byte = 0x0;
    ctx.delay = 2;
    ctx.val = start;
}

void dma_tick() {
    if (!ctx.active) { return; }

    if (ctx.delay) {
        ctx.delay--;
        return;
    }

    ppu_oam_write(ctx.byte, bus_read((ctx.val * 0x100) + ctx.byte));
    ctx.byte++;

    ctx.active = ctx.byte < 0xA0;

}

bool dma_transferring() {
    return ctx.active;
}