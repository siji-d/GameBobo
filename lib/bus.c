#include <bus.h>
#include <cart.h>
#include <ram.h>
#include <cpu.h>

// 0x0000 - 0x3FFF : ROM BANK 0
// 0x4000 - 0x7FFF : ROM BANK 1-N (Switchable)
// 0x8000 - 0x97FF : VRAM - CHR RAM
// 0x9800 - 0x9BFF : VRAM - BG MAP 1
// 0x9C00 - 0x9FFF : VRAM - BG MAP 2
// 0xA000 - 0xBFFF : EXTERNAL CART RAM (EXT-RAM)
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
        //ROM Data
        return cart_read(addr);
        
    } else if (addr < 0xA000u) {
        //VRAM
        printf("UNSUPPORTED BUS READ at address 0x%4.4X\n", addr);
        NO_IMP;

    } else if (addr < 0xC000u) {
        //EXTERNAL CART RAM
        return cart_read(addr);
        //NO_IMP;

    } else if (addr < 0xE000u) {
        //WORK RAM
        return wram_read(addr);
        
    } else if (addr < 0xFE00u) {
        //RESERVED ECHO RAM
        printf("UNSUPPORTED BUS READ at address 0x%4.4X\n", addr);
        NO_IMP;

    } else if (addr < 0xFEA0u) {
        //OBJECT ATTRIBUTE MEMORY
        printf("UNSUPPORTED BUS READ at address 0x%4.4X\n", addr);
        NO_IMP;
        
    } else if (addr < 0xFF00u) {
        //RESERVED - UNUSABLE
        printf("UNSUPPORTED BUS READ at address 0x%4.4X\n", addr);
        NO_IMP;
        
    } else if (addr == 0xFF0Fu) {
        //CPU IF REGISTER
        return get_if_register();

    } else if (addr < 0xFF80u) {
        //IO REGISTERS
        printf("UNSUPPORTED BUS READ at address 0x%4.4X\n", addr);
        //NO_IMP;
    
    } else if (addr < 0xFFFFu) {
        //HIGH RAM
        return hram_read(addr);
        
    } else if (addr == 0xFFFFu) {
        //CPU IE REGISTER
        return get_ie_register();
        
    } else {
        printf("idek how you did that dawg, unsupported read at 0x%4.4X\n", addr);
        exit(-1);
    }
}

void bus_write(u16 addr, u8 val) {
    if (addr < 0x8000u){
        //ROM Data
        cart_write(addr, val);
        
    } else if (addr < 0xA000u) {
        //VRAM
        printf("UNSUPPORTED BUS WRITE at address 0x%4.4X\n", addr);
        NO_IMP;

    } else if (addr < 0xC000u) {
        //EXTERNAL CART RAM
        cart_write(addr, val);
        //NO_IMP;

    } else if (addr < 0xE000u) {
        //WORK RAM
        wram_write(addr, val);
        
    } else if (addr < 0xFE00u) {
        //RESERVED ECHO RAM
        printf("UNSUPPORTED BUS WRITE at address 0x%4.4X\n", addr);
        NO_IMP;

    } else if (addr < 0xFEA0u) {
        //OBJECT ATTRIBUTE MEMORY
        printf("UNSUPPORTED BUS WRITE at address 0x%4.4X\n", addr);
        NO_IMP;
        
    } else if (addr < 0xFF00u) {
        //RESERVED - UNUSABLE
        printf("UNSUPPORTED BUS WRITE at address 0x%4.4X\n", addr);
        NO_IMP;
        
    }  else if (addr == 0xFF0Fu) {
        set_if_register(val);
        
    } else if (addr < 0xFF80u) {
        //IO REGISTERS
        printf("UNSUPPORTED (i/o reg) BUS WRITE at address 0x%4.4X\n", addr);
        //NO_IMP;

    } else if (addr < 0xFFFFu) {
        //HIGH RAM
        hram_write(addr, val);
        
    } else if (addr == 0xFFFFu) {
        //CPU IE REGISTER
        set_ie_register(val);
        
    } else {
        printf("idek how you did that dawg, unsupported write at 0x%4.4X\n", addr);
        exit(-1);
    }
}

u16 bus_read16(u16 addr) {
    u16 hi = bus_read(addr);
    u16 lo = bus_read(addr + 1);

    return (lo | hi << 8);
}
 
void bus_write16(u16 addr, u16 val) {
    bus_write(addr, (val & 0x00FF));
    bus_write(addr + 1, (val >> 8) & 0xFF);
}

