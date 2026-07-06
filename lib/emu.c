#include <emu.h>
#include <stdio.h>
#include <cart.h>
#include <cpu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

static emuContext ctx;

emuContext *emu_get_context() {
    return &ctx;
}

void delay(u32 ms) {
    SDL_Delay(ms);
}

int emu_run(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom_file>\n", argv[0]);
        return -1;
    }

    if (!cart_load(argv[1])) { 
        fprintf(stderr, "Failed to load ROM file: %s\n", argv[1]);
        return -2;
    }

    printf("Cart Loaded...\n");

    SDL_Init(SDL_INIT_VIDEO);
    printf("SDL init\n");
    TTF_Init();
    printf("TTF init\n");

    cpu_init();
    
    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;

    while(ctx.running) {
        if (ctx.paused) {
            delay(10);
            continue;
        }

        if (!cpu_step()) {
            printf("CPU halted\n");
            return -3;
        }

        ctx.ticks++;
    }
    
    return 0;
}