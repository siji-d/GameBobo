#include <ui.h>
#include <ppu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <bus.h>
#include <emu.h>

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
SDL_Surface *screen;

SDL_Window *sdlDebugWindow;
SDL_Renderer *sdlDebugRenderer;
SDL_Texture *sdlDebugTexture;
SDL_Surface *debugScreen;

static int scale = 4;
static unsigned long tile_colours[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

void delay(u32 ms) {
    SDL_Delay(ms);
}

u32 get_ticks() {
    return SDL_GetTicks();
}

void ui_init() {
    SDL_Init(SDL_INIT_VIDEO);
    printf("SDL INIT\n");
    TTF_Init();
    printf("TTF INIT\n");

    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &sdlWindow, &sdlRenderer);

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                                SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_CreateWindowAndRenderer(16 * 8 * scale, 32 * 8 * scale, 0, &sdlDebugWindow, &sdlDebugRenderer);

    debugScreen = SDL_CreateRGBSurface(0, (16 * 8 * scale) + (16 * scale), (32 * 8 * scale) + (64 * scale), 32,
                                        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    sdlDebugTexture = SDL_CreateTexture(sdlDebugRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                        (16 * 8 * scale) + (16 * scale), (32 * 8 * scale) + (64 * scale));

    int x,y;
    SDL_GetWindowPosition(sdlWindow, &x, &y);
    SDL_SetWindowPosition(sdlDebugWindow, x + SCREEN_WIDTH + 10, y + SCREEN_HEIGHT - ((32 * 8 * scale) + (64 * scale)));
}



void draw_tile(SDL_Surface *surface, u16 start, u16 tile, int x, int y) {
    SDL_Rect rct;

    for (int tileY = 0; tileY < 16; tileY += 2) {
        u8 b1 = bus_read(start + (tile * 16) + tileY);
        u8 b2 = bus_read(start + (tile * 16) + tileY + 1);

        for (int bit = 7; bit >= 0; bit--) {
            u8 hi = !!(b1 & (1 << bit)) << 1;
            u8 lo = !!(b2 & (1 << bit));

            u8 colour = hi | lo;

            rct.x = x + ((7 - bit) * scale);
            rct.y = y + (tileY / 2 * scale);
            rct.w = rct.h = scale;

            SDL_FillRect(surface, &rct, tile_colours[colour]);
        }
    }
}

void update_dbg_window() {
    int xDraw = 0;
    int yDraw = 0;
    int tileNum = 0;
    
    SDL_Rect rct;
    rct.x = 0;
    rct.y = 0;
    rct.h = debugScreen->h;
    rct.w = debugScreen->w;
    SDL_FillRect(debugScreen, &rct, 0xFF111111);

    u16 addr = 0x8000;

    //324 tiles, 24 x 16

    for (int y = 0; y < 24; y++) {
        for (int x = 0; x < 16; x++) {
            draw_tile(debugScreen, addr, tileNum, xDraw + (x * scale), yDraw + (y * scale));
            xDraw += (8 * scale);
            tileNum++;
        }

        yDraw += (8 * scale);
        xDraw = 0;
    }

    SDL_UpdateTexture(sdlDebugTexture, NULL, debugScreen->pixels, debugScreen->pitch);
	SDL_RenderClear(sdlDebugRenderer);
	SDL_RenderCopy(sdlDebugRenderer, sdlDebugTexture, NULL, NULL);
	SDL_RenderPresent(sdlDebugRenderer);
    
}

void ui_update() {
    //update_main_window();
    SDL_Rect rct;
    rct.x = rct.y = 0;
    rct.w = rct.h = 2048;

    u32 *video_buffer = get_ppu_context()->video_buffer;

    for (int line = 0; line < YRES; line++) {
        for (int x = 0; x < XRES; x++) {
            rct.x = x * scale;
            rct.y = line * scale;
            rct.w = scale;
            rct.h = scale;

            SDL_FillRect(screen, &rct, video_buffer[x + (line * XRES)]);
        } 
    }

    SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, sdlTexture , NULL, NULL);
	SDL_RenderPresent(sdlRenderer);
    
    update_dbg_window();
}

void ui_event_handler() {
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0) {
        // SDL_UpdateWindowSurface(sdlWindow)
        // SDL_UpdateWindowSurface(sdlTraceWindow)
        // SDL_UpdateWindowSurface(sdlDebugWindow)
        

        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
            emu_get_context()->die = true;
        }
    }
}