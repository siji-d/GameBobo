#include <bus.h>
#include <cart.h>

// 0x0000 - 0x3FFF : ROM BANK 0
// 0x4000 - 0x7FFF : ROM BANK 1-N (Switchable)
// 0x8000 - 0x97FF : VRAM - CHR RAM
// 0x9800 - 0x9BFF : VRAM - BG MAP 1
// 0x9C00 - 0x9FFF : VRAM - BG MAP 2
// 0xA000 - 0xBFFF : EXTERNAL CART RAM (RAM)
// 0xC000 - 0xCFFF : WORK RAM (WRAM BANK 0) 
// 0xD000 - 0xDFFF : WORK RAM (WRAM BANK 1-7, Switchable in CGB mode only)
// 0xE000 - 0xFDFF : ECHO RAM (Mirrors C000 - DDFF, Reserved)
// 0xFE00 - 0xFE9F : OBJECT ATTRIBUTE MEMORY (OAM)
// 0xFEA0 - 0xFEFF : RESERVED - UNUSABLE
// 0xFF00 - 0xFF7F : I/O REGISTERS
// 0xFF80 - 0XFFFE : HIGH RAM (HRAM)
// 0xFFFF - 0xFFFF : INTERRUPT ENABLE REGISTER



u8 bus_read(u16 addr) {
    if (addr < 0x8000u){
        return cart_read(addr);
    }

    printf("bus read...\n");
    NO_IMP;
}

void bus_write(u16 addr, u8 val) {
    if (addr < 0x8000u){
        cart_write(addr, val);
        return;
    }

    printf("bus write...\n");
    NO_IMP;
}

