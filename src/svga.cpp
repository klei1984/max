/* Copyright (c) 2020 M.A.X. Port Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "svga.h"

#include "color.hpp"
#include "ini.hpp"

const bool SVGA_NO_TEXTURE_UPDATE = true;

static SDL_Window *sdlWindow;
static SDL_Renderer *sdlRenderer;
static SDL_Surface *sdlWindowSurface;
static SDL_Surface *sdlPaletteSurface;
static SDL_Texture *sdlTexture;
static unsigned int Svga_RenderTimer;
static int sdl_win_init_flag;
static bool Svga_PaletteChanged;

static int Svga_ScreenWidth;
static int Svga_ScreenHeight;
static int Svga_ScreenMode;
static int Svga_ScaleQuality;

Rect scr_size;
ScreenBlitFunc scr_blit;

int Svga_Init(void) {
    Uint32 flags = 0;

    if (sdl_win_init_flag) {
        return 0;
    }

    Svga_ScreenWidth = ini_get_setting(INI_WINDOW_WIDTH);
    Svga_ScreenHeight = ini_get_setting(INI_WINDOW_HEIGHT);
    Svga_ScreenMode = ini_get_setting(INI_SCREEN_MODE);
    Svga_ScaleQuality = ini_get_setting(INI_SCALE_QUALITY);

    if (Svga_ScreenWidth < 640) {
        Svga_ScreenWidth = 640;
    }

    if (Svga_ScreenHeight < 480) {
        Svga_ScreenHeight = 480;
    }

    switch (Svga_ScreenMode) {
        case WINDOW_MODE_WINDOWED: {
            flags |= 0;
        } break;

        case WINDOW_MODE_FULLSCREEN: {
            flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;
        } break;

        case WINDOW_MODE_BORDERLESS: {
            flags |=
                SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;
        } break;

        default: {
            SDL_Log("Unsupported screen mode: %i\n", Svga_ScreenMode);
        } break;
    }

    if ((sdlWindow = SDL_CreateWindow("M.A.X.: Mechanized Assault & Exploration", SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED, Svga_ScreenWidth, Svga_ScreenHeight, flags)) == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s\n", SDL_GetError());
    }

    if ((sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE)) ==
        NULL) {
        SDL_Log("SDL_CreateRenderer failed: %s\n", SDL_GetError());
    }

    switch (Svga_ScaleQuality) {
        case 0: {
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
        } break;

        case 1: {
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

        } break;

        case 2: {
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
        } break;

        default:
            SDL_Log("Unsupported scale quality: %i\n", Svga_ScaleQuality);
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
            break;
    }

    SDL_RenderSetLogicalSize(sdlRenderer, Svga_ScreenWidth, Svga_ScreenHeight);

    sdlWindowSurface =
        SDL_CreateRGBSurfaceWithFormat(0, Svga_ScreenWidth, Svga_ScreenHeight, 32, SDL_PIXELFORMAT_ARGB8888);
    sdlPaletteSurface =
        SDL_CreateRGBSurfaceWithFormat(0, Svga_ScreenWidth, Svga_ScreenHeight, 8, SDL_PIXELFORMAT_INDEX8);
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Svga_ScreenWidth,
                                   Svga_ScreenHeight);

    scr_blit = &Svga_Blit;

    scr_size.lrx = Svga_ScreenWidth - 1;
    scr_size.lry = Svga_ScreenHeight - 1;
    scr_size.ulx = 0;
    scr_size.uly = 0;

    if (0 != SDL_RenderClear(sdlRenderer)) {
        SDL_Log("SDL_RenderClear failed: %s\n", SDL_GetError());
    }

    SDL_RenderPresent(sdlRenderer);

    if (sdlWindowSurface && sdlTexture && sdlPaletteSurface) {
        sdl_win_init_flag = 1;
        return 0;
    } else {
        sdl_win_init_flag = 0;
        return -1;
    }
}

void Svga_Deinit(void) {
    if (sdl_win_init_flag) {
        /// \todo Clean Up
    }
}

void Svga_Blit(unsigned char *srcBuf, unsigned int srcW, unsigned int srcH, unsigned int subX, unsigned int subY,
               unsigned int subW, unsigned int subH, unsigned int dstX, unsigned int dstY) {
    SDL_assert(sdlPaletteSurface && sdlPaletteSurface->format &&
               sdlPaletteSurface->format->BytesPerPixel == sizeof(unsigned char));

    {
        unsigned char *target_pixels =
            &((unsigned char *)sdlPaletteSurface->pixels)[dstX + sdlPaletteSurface->pitch * dstY];
        unsigned char *source_pixels = &srcBuf[subX + srcW * subY];

        for (unsigned int h = 0; h < subH; ++h) {
            SDL_memcpy(target_pixels, source_pixels, subW);
            source_pixels += srcW;
            target_pixels += sdlPaletteSurface->pitch;
        }
    }

    SDL_Rect bounds = {static_cast<int>(dstX), static_cast<int>(dstY), static_cast<int>(subW), static_cast<int>(subH)};

    if (Svga_PaletteChanged) {
        Svga_PaletteChanged = false;

        bounds.x = 0;
        bounds.y = 0;
        bounds.w = sdlPaletteSurface->w;
        bounds.h = sdlPaletteSurface->h;
    }

    /* Blit 8-bit palette surface onto the window surface that's closer to the texture's format */
    if (SDL_LowerBlit(sdlPaletteSurface, &bounds, sdlWindowSurface, &bounds) != 0) {
        SDL_Log("SDL_BlitSurface failed: %s\n", SDL_GetError());
    }

    if (SVGA_NO_TEXTURE_UPDATE) {
        Uint32 *source_pixels = &((Uint32 *)sdlWindowSurface->pixels)[bounds.x + sdlWindowSurface->w * bounds.y];
        void *target_pixels = nullptr;
        int target_pitch = 0;

        if (SDL_LockTexture(sdlTexture, &bounds, &target_pixels, &target_pitch)) {
            SDL_Log("SDL_LockTexture failed: %s\n", SDL_GetError());

        } else {
            for (unsigned int h = 0; h < bounds.h; ++h) {
                SDL_memcpy(target_pixels, source_pixels, bounds.w * sizeof(Uint32));
                source_pixels += sdlWindowSurface->w;
                target_pixels = &(static_cast<Uint32 *>(target_pixels)[target_pitch / sizeof(Uint32)]);
            }

            SDL_UnlockTexture(sdlTexture);
        }

    } else {
        if (SDL_UpdateTexture(sdlTexture, &bounds,
                              &((Uint32 *)sdlWindowSurface->pixels)[bounds.x + sdlPaletteSurface->pitch * bounds.y],
                              sdlWindowSurface->pitch) != 0) {
            SDL_Log("SDL_UpdateTexture failed: %s\n", SDL_GetError());
        }
    }

    /* Make the modified texture visible by rendering it */
    if (SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL) != 0) {
        SDL_Log("SDL_RenderCopy failed: %s\n", SDL_GetError());
    }

    SDL_RenderPresent(sdlRenderer);
}

int Svga_WarpMouse(int window_x, int window_y) {
    int result;

    if (SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_INPUT_FOCUS) {
        SDL_WarpMouseInWindow(sdlWindow, window_x, window_y);
        result = true;

    } else {
        result = false;
    }

    return result;
}

void Svga_SetPaletteColor(int i, unsigned char r, unsigned char g, unsigned char b) {
    SDL_Color color;

    color.r = r;
    color.g = g;
    color.b = b;
    color.a = 0;

    if (SDL_SetPaletteColors(sdlPaletteSurface->format->palette, &color, i, 1)) {
        SDL_Log("SDL_SetPaletteColors failed: %s\n", SDL_GetError());
    }

    Svga_PaletteChanged = true;

    if ((i == PALETTE_SIZE - 1) || (timer_elapsed_time(Svga_RenderTimer) > TIMER_FPS_TO_MS(60))) {
        Svga_RenderTimer = timer_get();

        unsigned char srcBuf;

        Svga_Blit(&srcBuf, 0, 0, 0, 0, 0, 0, 0, 0);
    }
}

int Svga_GetScreenWidth(void) { return Svga_ScreenWidth; }

int Svga_GetScreenHeight(void) { return Svga_ScreenHeight; }
