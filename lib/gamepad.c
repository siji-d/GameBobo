#include <gamepad.h>
#include <string.h>

static gamepadContext ctx = {0};

bool gamepad_button_sel() {
    return ctx.button_sel;
}

bool gamepad_dir_sel() {
    return ctx.dir_sel;
} 

void gamepage_init();

void gamepad_set_sel(u8 val) {
    ctx.button_sel = val & 0x20;
    ctx.dir_sel = val & 0x10;

}

gamepadState *get_gamepad_state() {
    return &ctx.controller;
}

u8 get_gamepad_output() {
    u8 output = 0xCF;

    if (!gamepad_button_sel()) {
        if (get_gamepad_state()->start) {
            output &= ~(1 << 3); 
        }
        if (get_gamepad_state()->select) {
            output &= ~(1 << 2); 
        }
        if (get_gamepad_state()->a) {
            output &= ~(1); 
        }
        if (get_gamepad_state()->b) {
            output &= ~(1 << 1); 
        }
    }

    if (!gamepad_dir_sel()) {
        if (get_gamepad_state()->up) {
            output &= ~(1 << 2); 
        }
        if (get_gamepad_state()->down) {
            output &= ~(1 << 3); 
        }
        if (get_gamepad_state()->left) {
            output &= ~(1 << 1); 
        }
        if (get_gamepad_state()->right) {
            output &= ~(1);
        }
    }

    return output;
}



