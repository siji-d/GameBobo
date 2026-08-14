#pragma once
#include <common.h>

typedef struct {
    //REGISTERS
    u8 lcdc;
    u8 stat;
    u8 scy;
    u8 scx;
    u8 ly;
    u8 lyc;
    u8 dma;
    u8 bg_palette;
    u8 obj_palette[2];

    u8 win_y;
    u8 win_x;

    u32 bg_colours[4];
    u32 sp1_colours[4];
    u32 sp2_colours[4];

} lcdContext;

typedef enum {
    MODE_HBLANK,
    MODE_VBLANK,
    MODE_OAM,
    MODE_TRANSFER

} lcdMode;

typedef enum {
    SS_HBLANK = (1 << 3),
    SS_VBLANK = (1 << 4),
    SS_OAM = (1 << 5),
    SS_LYC = (1 << 6),
} statSrc;

lcdContext *get_lcd_context();

void lcd_init();
u8 lcd_read(u16 addr);
void lcd_write(u16 addr, u8 val);

#define LCDC_BGW_ENABLE (BIT(get_lcd_context()->lcdc, 0))
#define LCDC_OBJ_ENABLE (BIT(get_lcd_context()->lcdc, 1))
#define LCDC_OBJ_HEIGHT (BIT(get_lcd_context()->lcdc, 2) ? 16 : 8)
#define LCDC_BG_MAP_AREA (BIT(get_lcd_context()->lcdc, 3) ? 0x9C00 : 0x9800)
#define LCDC_BGW_DATA_AREA (BIT(get_lcd_context()->lcdc, 4) ? 0x8000 : 0x8800)
#define LCDC_WIN_ENABLE (BIT(get_lcd_context()->lcdc, 5))
#define LCDC_WIN_MAP_AREA (BIT(get_lcd_context()->lcdc, 6) ? 0x9C00 : 0x9800)
#define LCDC_LCD_ENABLE (BIT(get_lcd_context()->lcdc, 7))

#define STAT_MODE ((lcdMode)(get_lcd_context()->stat & 0b11))
#define STAT_MODE_SET(mode) { get_lcd_context()->stat &= ~0b11; get_lcd_context()->stat |= mode; }

#define STAT_LYC BIT(get_lcd_context()->stat, 2)
#define STAT_LYC_SET(b) BIT_SET(get_lcd_context()->stat, 2, b)

#define STAT_ITR(src) get_lcd_context()->stat & src

