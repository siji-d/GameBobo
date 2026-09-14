#include <lcd.h>
#include <ppu.h>
#include <dma.h>

static lcdContext ctx;
static unsigned long default_colours[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

lcdContext *get_lcd_context() {
    return &ctx;
}

void lcd_init() {
    ctx.lcdc = 0x91;
    ctx.scx = 0;
    ctx.scy = 0;
    ctx.ly = 0;
    ctx.lyc = 0;
    ctx.bg_palette = 0xFC;
    ctx.obj_palette[0] = 0xFF;
    ctx.obj_palette[1] = 0xFF;
    ctx.win_y = 0;
    ctx.win_x = 0; 

    for (int i = 0; i < 4; i++) {
        ctx.bg_colours[i] = default_colours[i];
        ctx.sp1_colours[i] = default_colours[i];
        ctx.sp2_colours[i] = default_colours[i];
    }

}

void update_palette(u8 val, u8 pal) {
    u32 *p_colours = ctx.bg_colours;

    switch (pal) {
        case 1:
            p_colours = ctx.sp1_colours;
            break;

        case 2:
            p_colours = ctx.sp2_colours;
            break;
    }

    p_colours[0] = default_colours[val & 0b11];
    p_colours[1] = default_colours[val >> 2 & 0b11];
    p_colours[2] = default_colours[val >> 4 & 0b11];
    p_colours[3] = default_colours[val >> 6 & 0b11]; 
}

u8 lcd_read(u16 addr) {
    u8 offset = (addr - 0xFF40);
    u8 *p = (u8 *)&ctx;

    return p[offset];
}

void lcd_write(u16 addr, u8 val) {
    u8 offset = (addr - 0xFF40);
    u8 *p = (u8 *)&ctx;

    p[offset] = val;

    if (offset == 6) { // 0xFF46 - DMA
        dma_init(val);
    }
    
    if (addr == 0xFF47) {
        update_palette(val, 0);
    } else if (addr == 0xFF48) {
        update_palette(val & 0xFC, 1);
    } else if (addr == 0xFF49) {
        update_palette(val & 0xFC, 2);
    }
}