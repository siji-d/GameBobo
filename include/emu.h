#pragma once
#include <common.h>

typedef struct {
    bool running;
    bool paused;
    u64 ticks;
} emuContext;

int emu_run(int argc, char** argv);

emuContext *emu_get_context();