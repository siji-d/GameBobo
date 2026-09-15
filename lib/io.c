#include <io.h>
#include <timer.h>
#include <cpu.h>
#include <dma.h>
#include <lcd.h>
#include <gamepad.h>
#include <apu.h>

static u8 serial_data[2];

u8 io_read(u16 addr) {
    if (addr == 0xFF00) { //JOYPAD
        return get_gamepad_output();
    }

    if (addr == 0xFF01) { //SERIAL TRANSFER DATA
        return serial_data[0];
    } else if (addr == 0xFF02)  {//SERIAL TRANSFER CONTROL
        return serial_data[1];
    }

    if (BETWEEN(addr, 0xFF04, 0xFF07)) {
        return timer_read(addr);
    }


    if (addr == 0xFF0F) { //IF REGISTER
        return get_itr_flags();
    } 

    if (BETWEEN(addr, 0xFF10, 0xFF26)) {
        return apu_read(addr);
    }

    if (BETWEEN(addr, 0xFF30, 0xFF3F)) {
        return apu_wave_ram_read(addr);
    }

    if (BETWEEN(addr, 0xFF40, 0xFF4B)) { //LCD CONTROL, STATUS
        return lcd_read(addr);
    }

    //printf("UNSUPPORTED BUS READ at address 0x%4.4X\n", addr);
    return 0;
}

void io_write(u16 addr, u8 val) {
    if (addr == 0xFF00) { //JOYPAD
        gamepad_set_sel(val);
        return;
    }
    
    if (addr == 0xFF01) { //SERIAL TRANSFER DATA
        serial_data[0] = val;
        return;
    } else if (addr == 0xFF02)  {//SERIAL TRANSFER CONTROL
        serial_data[1] = val;
        return;
    }
    
    if (BETWEEN(addr, 0xFF04, 0xFF07)) {
        timer_write(addr, val);
        return;
    }

    if (addr == 0xFF0F) { //IF REGISTER
        set_itr_flags(val);
        return;
    }

    if (BETWEEN(addr, 0xFF10, 0xFF26)) {
        apu_write(addr, val);
        return;
    }

    if (BETWEEN(addr, 0xFF30, 0xFF3F)) {
        apu_wave_ram_write(addr, val);
        return;
    }

    if (BETWEEN(addr, 0xFF40, 0xFF4B)) { //LCD CONTROL, STATUS
        return lcd_write(addr, val);
    }


    //printf("UNSUPPORTED BUS READ at address 0x%4.4X\n", addr); 
    return;

}