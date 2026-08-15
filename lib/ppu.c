#include <ppu.h>
#include <lcd.h>
#include <string.h>
#include <ppu_sm.h>

static ppuContext ctx;

ppuContext *get_ppu_context() {
    return &ctx;
}

void ppu_init() {
    ctx.current_frame = 0;
    ctx.line_ticks = 0;
    ctx.video_buffer = malloc(YRES * XRES * sizeof(u32));

    ctx.pfc.line_x = 0;
    ctx.pfc.pushed_x = 0;
    ctx.pfc.fetch_x = 0;
    ctx.pfc.pixel_fifo.size = 0;
    ctx.pfc.pixel_fifo.head = ctx.pfc.pixel_fifo.tail = NULL;
    ctx.pfc.fetch_state = FS_TILE;

    ctx.line_sprites = 0;
    ctx.fetched_entry_count = 0;


    lcd_init();
    STAT_MODE_SET(MODE_OAM);
    //printf("LCDS MODE: %d\n", STAT_MODE);

    memset(ctx.oam_ram, 0, sizeof(ctx.oam_ram));
    memset(ctx.video_buffer, 0, YRES * XRES * sizeof(u32));

}

void ppu_tick() {
    ctx.line_ticks++;
    //printf("PPU TICK: %d\n", ctx.line_ticks);
    //printf("LCDS MODE: %d\n", STAT_MODE);

    switch (STAT_MODE) {
        case MODE_HBLANK:
            ppu_mode_hblank();
            break;

        case MODE_VBLANK:
            ppu_mode_vblank();
            break;

        case MODE_OAM:
            ppu_mode_oam();
            break;

        case MODE_TRANSFER:
            ppu_mode_transfer();
            break;
    }
}

void ppu_oam_write(u16 addr, u8 val){
    if (addr >= 0xFE00) {
        addr -= 0xFE00;
    }

    u8 *p = (u8 *)ctx.oam_ram;
    p[addr] = val;

}

u8 ppu_oam_read(u16 addr){
    if (addr >= 0xFE00) {
        addr -= 0xFE00;
    }

    u8 *p = (u8 *)ctx.oam_ram;
    return p[addr];
}


void ppu_vram_write(u16 addr, u8 val){
    ctx.vram[addr - 0x8000] = val;
}

u8 ppu_vram_read(u16 addr){
    return ctx.vram[addr - 0x8000];
}