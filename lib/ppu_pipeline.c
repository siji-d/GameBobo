#include <ppu.h>
#include <lcd.h>
#include <bus.h>

bool window_visible() {
    return LCDC_WIN_ENABLE && (get_lcd_context()->win_x >= 0) && (get_lcd_context()->win_x <= 166) && 
            (get_lcd_context()->win_y >= 0) && (get_lcd_context()->win_y < YRES);
}

void pixel_fifo_push(u32 val) {
    fifoEntry *next = malloc(sizeof(fifoEntry));
    next->next = NULL;
    next->val = val;

    if (!get_ppu_context()->pfc.pixel_fifo.head) {
        get_ppu_context()->pfc.pixel_fifo.head = get_ppu_context()->pfc.pixel_fifo.tail = next;
    } else {
        get_ppu_context()->pfc.pixel_fifo.tail->next = next;
        get_ppu_context()->pfc.pixel_fifo.tail = next;
    }

    get_ppu_context()->pfc.pixel_fifo.size++;
}

u32 fetch_sprite_pixels(int bit, u32 colour, u8 bg_colour) {
    for (int i = 0; i < get_ppu_context()->fetched_entry_count; i++) {
        int sp_x = (get_ppu_context()->fetched_entries[i].x - 8) + (get_lcd_context()->scx % 8);

        if (sp_x + 8 < get_ppu_context()->pfc.fifo_x) {
            continue;
        }

        int offset = get_ppu_context()->pfc.fifo_x - sp_x;

        if (offset < 0 || offset > 7) {
            //Out of bounds...
            continue;
        }

        bit = (7 - offset);

        if (get_ppu_context()->fetched_entries[i].f_x_flip) {
            bit = offset;
        }

        u8 lo = !!(get_ppu_context()->pfc.fetch_entry_data[i * 2] & (1 << bit));
        u8 hi = !!(get_ppu_context()->pfc.fetch_entry_data[(i * 2) + 1] & (1 << bit)) << 1;

        bool bg_priority = get_ppu_context()->fetched_entries[i].f_bgp;

        if (!(hi | lo)) {
            continue;
        }

        if (!bg_priority || bg_colour == 0) {
            colour = (get_ppu_context()->fetched_entries[i].f_pn) ? 
                    get_lcd_context()->sp2_colours[hi|lo] : get_lcd_context()->sp1_colours[hi|lo];
            
            if (hi | lo) {
                break;
            } 
        }



    }

    return colour;
}

u32 pixel_fifo_pop() {

    if (get_ppu_context()->pfc.pixel_fifo.size <= 0) {
        printf("balls in pioxel fifos lmao\n");
        exit(-29);
    }

    fifoEntry *entry = get_ppu_context()->pfc.pixel_fifo.head;

    get_ppu_context()->pfc.pixel_fifo.head = entry->next;
    get_ppu_context()->pfc.pixel_fifo.size--;

    u32 val = entry->val;
    free(entry);
    return val;
}

bool pipeline_fifo_add() {
    if (get_ppu_context()->pfc.pixel_fifo.size > 8) {
        return false;
    }

    int x = get_ppu_context()->pfc.fetch_x - (8 - (get_lcd_context()->scx % 8));

    for (int bit = 7; bit >= 0; bit--) {
        u8 hi = !!(get_ppu_context()->pfc.bgw_fetch_data[2] & (1 << bit)) << 1;
        u8 lo = !!(get_ppu_context()->pfc.bgw_fetch_data[1] & (1 << bit));
        u32 colour = get_lcd_context()->bg_colours[hi | lo];

        if (!LCDC_BGW_ENABLE) {
            colour = get_lcd_context()->bg_colours[0]; 
        }

        if (LCDC_OBJ_ENABLE) {
            colour = fetch_sprite_pixels(bit, colour, hi | lo);
        }

        
        
        if (x >= 0) {
            pixel_fifo_push(colour); 
            get_ppu_context()->pfc.fifo_x++;
        }
    }
    return true;
}

void pipeline_load_sprite_tile() {
    oamLineEntry *le = get_ppu_context()->line_sprites;

    while(le) {
        int sp_x = (le->entry.x - 8) + (get_lcd_context()->scx % 8);

        if ((sp_x >= get_ppu_context()->pfc.fetch_x && sp_x < get_ppu_context()->pfc.fetch_x + 8 )||
            ((sp_x + 8) >= get_ppu_context()->pfc.fetch_x && (sp_x + 8) < get_ppu_context()->pfc.fetch_x + 8 )) {

            get_ppu_context()->fetched_entries[get_ppu_context()->fetched_entry_count++] = le->entry;
        }
        
        le = le->next;

        if (!le || get_ppu_context()->fetched_entry_count >= 3) {
            break;
        }

    }

}

void pipeline_load_window_tile() {
    if (!window_visible()) {
        return;
    }
    u8 win_y = get_lcd_context()->win_y;

    if ((get_ppu_context()->pfc.fetch_x + 7 >= get_lcd_context()->win_x) && 
        (get_ppu_context()->pfc.fetch_x + 7 < get_lcd_context()->win_x + YRES + 14)) {
    
        if (get_lcd_context()->ly >= win_y && get_lcd_context()->ly < win_y + XRES) {
            u8 w_tile_y = get_ppu_context()->window_line / 8;

            get_ppu_context()->pfc.bgw_fetch_data[0] = bus_read(LCDC_WIN_MAP_AREA +
                ((get_ppu_context()->pfc.fetch_x + 7 - get_lcd_context()->win_x) / 8) +
                (w_tile_y * 32));
            
            if (LCDC_BGW_DATA_AREA == 0x8800) {
                get_ppu_context()->pfc.bgw_fetch_data[0] += 128;
            }
        }

    }
}

void pipeline_load_sprite_data(u8 offset) {
    int y = get_lcd_context()->ly;
    u8 spr_height = LCDC_OBJ_HEIGHT;

    for (int i = 0; i < get_ppu_context()->fetched_entry_count; i++) {
        u8 ty = ((y + 16) - get_ppu_context()->fetched_entries[i].y) * 2;

        if (get_ppu_context()->fetched_entries[i].f_y_flip) {
            ty = ((spr_height * 2) - 2) - ty;
        }

        u8 tile_index = get_ppu_context()->fetched_entries[i].tile;

        if (spr_height == 16) {
            tile_index &= ~(1);
        }

        get_ppu_context()->pfc.fetch_entry_data[(i * 2) + offset] = bus_read(0x8000 + (tile_index * 16) + ty + offset);
    }
}

void pipeline_fetch() {
    switch(get_ppu_context()->pfc.fetch_state) {
        case FS_TILE: {
            get_ppu_context()->fetched_entry_count = 0;

            if (LCDC_BGW_ENABLE) {
                get_ppu_context()->pfc.bgw_fetch_data[0] = bus_read(LCDC_BG_MAP_AREA + (get_ppu_context()->pfc.map_x / 8) +
                                                                    ((get_ppu_context()->pfc.map_y / 8) * 32));
                
                if (LCDC_BGW_DATA_AREA == 0x8800) {
                    get_ppu_context()->pfc.bgw_fetch_data[0] += 128;
                }

                pipeline_load_window_tile();
            }

            if (LCDC_OBJ_ENABLE && get_ppu_context()->line_sprites) {
                pipeline_load_sprite_tile();
            }

            get_ppu_context()->pfc.fetch_state = FS_DATA0;
            get_ppu_context()->pfc.fetch_x += 8;
               
        } break;

        case FS_DATA0: {
            get_ppu_context()->pfc.bgw_fetch_data[1] = bus_read(LCDC_BGW_DATA_AREA + (get_ppu_context()->pfc.bgw_fetch_data[0] * 16) 
                                                                    + get_ppu_context()->pfc.tile_y);

            pipeline_load_sprite_data(0);
            get_ppu_context()->pfc.fetch_state = FS_DATA1;

        } break;

        case FS_DATA1: {
            get_ppu_context()->pfc.bgw_fetch_data[2] = bus_read(LCDC_BGW_DATA_AREA + (get_ppu_context()->pfc.bgw_fetch_data[0] * 16) 
                                                                    + get_ppu_context()->pfc.tile_y + 1);

            pipeline_load_sprite_data(1);
            get_ppu_context()->pfc.fetch_state = FS_SLEEP;

        } break;
 
        case FS_SLEEP: {
            get_ppu_context()->pfc.fetch_state = FS_PUSH; 
        } break;

        case FS_PUSH: {
            if (pipeline_fifo_add()) {
                get_ppu_context()->pfc.fetch_state = FS_TILE;
            }
        } break;

    }
} 

void pipeline_fifo_reset() {
    while (get_ppu_context()->pfc.pixel_fifo.size) {
        pixel_fifo_pop();
    }

    get_ppu_context()->pfc.pixel_fifo.head = 0;
}

void pipeline_push_pixel() {
    if (get_ppu_context()->pfc.pixel_fifo.size > 8) {
        u32 pixel_data = pixel_fifo_pop();

        if (get_ppu_context()->pfc.line_x >= (get_lcd_context()->scx % 8)) {
            get_ppu_context()->video_buffer[get_ppu_context()->pfc.pushed_x + (get_lcd_context()->ly * XRES)] = pixel_data;
            
            get_ppu_context()->pfc.pushed_x++;
        
        }

        get_ppu_context()->pfc.line_x++;

    }
}

void pipeline_proc() {
    get_ppu_context()->pfc.map_y = (get_lcd_context()->ly + get_lcd_context()->scy);
    get_ppu_context()->pfc.map_x = (get_ppu_context()->pfc.fetch_x + get_lcd_context()->scx);
    get_ppu_context()->pfc.tile_y = ((get_lcd_context()->ly + get_lcd_context()->scy) % 8) * 2;

    if (!(get_ppu_context()->line_ticks & 1)) {
        pipeline_fetch();
    }

    pipeline_push_pixel();

}
