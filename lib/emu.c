#include <emu.h>
#include <pthread.h>
#include <unistd.h>
#include <cart.h>
#include <cpu.h>
#include <ui.h>
#include <timer.h>
#include <dma.h>
#include <ppu.h> 

static emuContext ctx;

emuContext *emu_get_context() {
    return &ctx;
}

void *cpu_run(void *p) {// MAIN CPU THREAD
    timer_init();
    cpu_init();
    ppu_init();

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
            return 0;
        }

        
    }

    return 0;

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

    ui_init();
    pthread_t t1;

    if (pthread_create(&t1, NULL, cpu_run, NULL)) {
        fprintf(stderr, "Failed to start main thread\n");
        return -1;
    }

    u32 prev_frame = 0;

    while(!ctx.die) {
        usleep(1000);
        ui_event_handler();

        //printf("PREV: %u, ------ CURR: %u\n", prev_frame, get_ppu_context()->current_frame);

        if (prev_frame != get_ppu_context()->current_frame) {
            ui_update();
        }
        

        //ui_update();
        prev_frame = get_ppu_context()->current_frame;
    }
    
    return 0;
}

void emu_cycles(int cycles) {
    //spend some cycles for accuracy and sync

    for (int i = 0; i < cycles; i++) {
        for (int j = 0; j < 4; j++) {
            ctx.ticks++;
            timer_tick();
            ppu_tick();
        }

        dma_tick();
    }



}