#pragma once
#include <common.h>

typedef struct {
    bool running;
    bool paused;
    u64 ticks;
} emu_context;

int emu_run(int argc, char** argv);

emu_context *emu_get_context();