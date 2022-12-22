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

#include "ini.hpp"

static SDL_Window *sdlWindow;
static SDL_Renderer *sdlRenderer;
static SDL_Surface *sdlWindowSurface;
static SDL_Surface *sdlPaletteSurface;
static SDL_Texture *sdlTexture;

static int sdl_win_init_flag;

Rect scr_size;
ScreenBlitFunc scr_blit;

SDL_Surface *svga_get_screen(void) { return sdlPaletteSurface; }

int init_mode_640_480(void) { return init_vesa_mode(0x101, 640, 480, 0); }

int init_vesa_mode(int mode, int width, int height, int half) {
    Uint32 flags;
    int screen_mode;
    int scale_quality;

    if (sdl_win_init_flag) {
        return 0;
    }

    screen_mode = ini_get_setting(INI_SCREEN_MODE);

    flags = 0;

    switch (screen_mode) {
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
            SDL_Log("Unsupported screen mode: %i\n", screen_mode);
        } break;
    }

    if ((sdlWindow = SDL_CreateWindow("M.A.X.: Mechanized Assault & Exploration", SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED, width, height, flags)) == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s\n", SDL_GetError());
    }

    sdlRenderer = SDL_CreateSoftwareRenderer(SDL_GetWindowSurface(sdlWindow));

    if ((sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE)) ==
        NULL) {
        SDL_Log("SDL_CreateRenderer failed: %s\n", SDL_GetError());
    }

    scale_quality = ini_get_setting(INI_SCALE_QUALITY);

    switch (scale_quality) {
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
            SDL_Log("Unsupported scale quality: %i\n", scale_quality);
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
            break;
    }

    SDL_RenderSetLogicalSize(sdlRenderer, width, height);

    sdlWindowSurface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
    sdlPaletteSurface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 8, SDL_PIXELFORMAT_INDEX8);
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    scr_blit = &vesa_screen_blit;

    scr_size.lrx = width - 1;
    scr_size.lry = height - 1;
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

void get_start_mode(void) {}

void reset_mode(void) {}

void vesa_screen_blit(unsigned char *srcBuf, unsigned int srcW, unsigned int srcH, unsigned int subX, unsigned int subY,
                      unsigned int subW, unsigned int subH, unsigned int dstX, unsigned int dstY) {
    SDL_assert(sdlPaletteSurface && sdlPaletteSurface->format &&
               sdlPaletteSurface->format->BytesPerPixel == sizeof(unsigned char));

    {
        unsigned char *target_pixels =
            &((unsigned char *)sdlPaletteSurface->pixels)[dstX + sdlPaletteSurface->w * dstY];
        unsigned char *source_pixels = &srcBuf[subX + srcW * subY];

        for (unsigned int h = 0; h < subH; ++h) {
            memcpy(target_pixels, source_pixels, subW);
            source_pixels += srcW;
            target_pixels += sdlPaletteSurface->w;
        }
    }

    svga_render();
}

void svga_render(void) {
    /* Blit 8-bit palette surface onto the window surface that's closer to the texture's format */
    if (SDL_BlitSurface(sdlPaletteSurface, NULL, sdlWindowSurface, NULL) != 0) {
        SDL_Log("SDL_BlitSurface failed: %s\n", SDL_GetError());
    }

    if (SDL_UpdateTexture(sdlTexture, NULL, sdlWindowSurface->pixels, sdlWindowSurface->pitch) != 0) {
        SDL_Log("SDL_UpdateTexture failed: %s\n", SDL_GetError());
    }

    /* Make the modified texture visible by rendering it */
    if (SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL) != 0) {
        SDL_Log("SDL_RenderCopy failed: %s\n", SDL_GetError());
    }

    SDL_RenderPresent(sdlRenderer);
}

int svga_warp_mouse(int window_x, int window_y) {
    int result;

    if (SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_INPUT_FOCUS) {
        SDL_WarpMouseInWindow(sdlWindow, window_x, window_y);
        result = true;

    } else {
        result = false;
    }

    return result;
}
