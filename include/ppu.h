#pragma once
#include <common.h>

static const int LINES_PER_FRAME = 154;
static const int TICKS_PER_LINE = 456;
static const int YRES = 144;
static const int XRES = 160;


typedef struct {
    u8 y;
    u8 x;
    u8 tile;
    
    unsigned char f_cgb_pn : 3;
    unsigned char f_cgb_bank : 1;
    unsigned char f_pn : 1;
    unsigned char f_x_flip : 1;
    unsigned char f_y_flip : 1;
    unsigned char f_bgp : 1;

} oamSprite;

#define OAM_CGB_PN(s)   ((s).flags & 0x07)
#define OAM_CGB_BANK(s) (((s).flags >> 3) & 1)
#define OAM_PN(s)       (((s).flags >> 4) & 1)
#define OAM_X_FLIP(s)   (((s).flags >> 5) & 1)
#define OAM_Y_FLIP(s)   (((s).flags >> 6) & 1)
#define OAM_BGP(s)      (((s).flags >> 7) & 1)

typedef struct _oamLineEntry {
    oamSprite entry;
    struct _oamLineEntry *next;

} oamLineEntry;


typedef enum {
    FS_TILE,
    FS_DATA0,
    FS_DATA1,
    FS_SLEEP,
    FS_PUSH
} fetchState;

typedef struct _fifoEntry{
    struct _fifoEntry *next;
    u32 val;
} fifoEntry;

typedef struct {
    fifoEntry *head;
    fifoEntry *tail;
    u32 size;
} fifo;

typedef struct {
    fetchState fetch_state;
    fifo pixel_fifo;
    u8 line_x;
    u8 pushed_x;
    u8 fetch_x;
    u8 bgw_fetch_data[3];
    u8 fetch_entry_data[6];
    u8 map_y;
    u8 map_x;
    u8 tile_y;
    u8 fifo_x;
} pixelFifoContext;

typedef struct {
    oamSprite oam_ram[40];
    u8 vram[0x2000];

    u8 line_sprite_count;
    oamLineEntry *line_sprites; //linked list of sprites for the current line
    oamLineEntry line_entry_array[10]; //memory to use for linked list

    u8 fetched_entry_count;
    oamSprite fetched_entries[3]; //entries fetched during fifo pipeline processing

    pixelFifoContext pfc;
    u32 current_frame;
    u32 line_ticks;
    u32 *video_buffer;
    
} ppuContext;


pixelFifoContext *get_fifo_context();
ppuContext *get_ppu_context();

void ppu_init();
void ppu_tick();

void pipeline_fifo_reset();
void pipeline_proc();

void ppu_oam_write(u16 addr, u8 val);
u8 ppu_oam_read(u16 addr);

void ppu_vram_write(u16 addr, u8 val);
u8 ppu_vram_read(u16 addr);