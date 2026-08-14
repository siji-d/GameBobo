#include <ppu_sm.h>
#include <ppu.h>
#include <cpu.h>
#include <interrupts.h>
#include <lcd.h>

static u32 target_frame_time = 1000 / 60; //ms per frame
static long prev_frame_time = 0;
static long start_timer = 0;
static long frame_count = 0;

void inc_ly() {
    get_lcd_context()->ly++;

    if (get_lcd_context()->ly == get_lcd_context()->lyc) {
        STAT_LYC_SET(1);

        if (STAT_ITR(SS_LYC)) {
            cpu_request_interrupt(IT_LCD_STAT); 
        }
    } else {
        STAT_LYC_SET(0);
    }
}

void ppu_mode_hblank() {
    if (get_ppu_context()->line_ticks >= TICKS_PER_LINE) {
        inc_ly();

        if (get_lcd_context()->ly >= YRES) {
            STAT_MODE_SET(MODE_VBLANK);
            cpu_request_interrupt(IT_VBLANK);

            if (STAT_ITR(SS_VBLANK)){
                cpu_request_interrupt(IT_LCD_STAT);
            }

            get_ppu_context()->current_frame++;

            //calc fps?
            u32 end = get_ticks();
            u32 frame_time = end - prev_frame_time;

            if (frame_time < target_frame_time) {
                delay((target_frame_time - frame_time));
            }

            if (end - start_timer >= 1000) {
                u32 fps = frame_count;
                start_timer = end;
                frame_count = 0;

                //printf("FPS: %d\n", fps);
            }
            //printf("current frame: %ld\n", frame_count);
            frame_count++;
            prev_frame_time = get_ticks();


        } else {
            STAT_MODE_SET(MODE_OAM);
        }

        get_ppu_context()->line_ticks = 0;
    }
    
}

void ppu_mode_vblank() {
    if (get_ppu_context()->line_ticks >= TICKS_PER_LINE) {
        inc_ly();
        if (get_lcd_context()->ly >= LINES_PER_FRAME) {
            STAT_MODE_SET(MODE_OAM);
            get_lcd_context()->ly = 0;

        }
        get_ppu_context()->line_ticks = 0;
    }

}

void ppu_mode_oam() {
    if (get_ppu_context()->line_ticks >= 80) {
        STAT_MODE_SET(MODE_TRANSFER);

    }

}

void ppu_mode_transfer() {
    if (get_ppu_context()->line_ticks >= 80 + 172) {
        STAT_MODE_SET(MODE_HBLANK);
    }


}