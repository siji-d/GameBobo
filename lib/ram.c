#include <ram.h>


u16 WRAM_START = 0xC000;
u16 WRAM_SIZE = 0x2000;
u16 HRAM_START = 0xFF80;
u16 HRAM_SIZE = 0x80;

typedef struct {
    u8 wram[0x2000];
    u8 hram[0x80];

} ramContext;

static ramContext ctx;

u8 wram_read(u16 addr) {
    addr -= WRAM_START;
    return ctx.wram[addr];
}

void wram_write(u16 addr, u8 val) {
    addr -= WRAM_START;
    ctx.wram[addr] = val;
}

u8 hram_read(u16 addr) {
    addr -= HRAM_START;
    return ctx.hram[addr];
}

void hram_write(u16 addr, u8 val) {
    addr -= HRAM_START;
    ctx.hram[addr] = val;
}

