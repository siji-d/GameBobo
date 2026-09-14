#pragma once
#include <common.h>

typedef struct {
    bool start;
    bool select;
    bool a;
    bool b;
    bool up;
    bool down;
    bool left;
    bool right;

} gamepadState;

typedef struct {
    bool button_sel;
    bool dir_sel;
    gamepadState controller;
} gamepadContext;

void gamepage_init();
bool gamepad_button_sel();
bool gamepad_dir_sel(); 
void gamepad_set_sel(u8 val);

gamepadState *get_gamepad_state();
u8 get_gamepad_output();