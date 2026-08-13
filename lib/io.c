#include <io.h>
#include <timer.h>
#include <cpu.h>

static u8 serial_data[2];

u8 io_read(u16 addr) {
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

    printf("UNSUPPORTED BUS READ at address 0x%4.4X\n", addr);
    return 0;
}

void io_write(u16 addr, u8 val) {
    
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

    printf("UNSUPPORTED BUS READ at address 0x%4.4X\n", addr); 
    return;

}