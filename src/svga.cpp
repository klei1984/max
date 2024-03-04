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

#include "ailog.hpp"
#include "gnw.h"
#include "ini.hpp"

#define SVGA_DEFAULT_WIDTH (640)
#define SVGA_DEFAULT_HEIGHT (480)
#define SVGA_DEFAULT_REFRESH_RATE (60)

static Uint32 Svga_SetupDisplayMode(SDL_Rect *bounds);
static void Svga_CorrectAspectRatio(SDL_DisplayMode *display_mode);
static void Svga_RefreshSystemPalette(bool force);

static const bool SVGA_NO_TEXTURE_UPDATE = true;

static SDL_Window *sdlWindow;
static SDL_Renderer *sdlRenderer;
static SDL_Surface *sdlWindowSurface;
static SDL_Surface *sdlPaletteSurface;
static SDL_Texture *sdlTexture;
static uint32_t Svga_RenderTimer;
static int32_t sdl_win_init_flag;
static bool Svga_PaletteChanged;

static int32_t Svga_ScreenWidth;
static int32_t Svga_ScreenHeight;
static int32_t Svga_ScreenMode;
static int32_t Svga_ScaleQuality;
static int32_t Svga_DisplayIndex;
static int32_t Svga_DisplayRefreshRate;
static Uint32 Svga_DisplayPixelFormat;

Rect scr_size;
ScreenBlitFunc scr_blit;

Uint32 Svga_SetupDisplayMode(SDL_Rect *bounds) {
    Uint32 flags = 0uL;
    SDL_DisplayMode display_mode;

    Svga_DisplayIndex = ini_get_setting(INI_DISPLAY_INDEX);

    if (SDL_GetCurrentDisplayMode(Svga_DisplayIndex, &display_mode)) {
        AiLog log("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());

        display_mode.refresh_rate = SVGA_DEFAULT_REFRESH_RATE;
        display_mode.format = SDL_PIXELFORMAT_RGB888;
        display_mode.w = SVGA_DEFAULT_WIDTH;
        display_mode.h = SVGA_DEFAULT_HEIGHT;
    }

    Svga_DisplayRefreshRate = display_mode.refresh_rate;
    Svga_DisplayPixelFormat = display_mode.format;

    Svga_ScreenMode = ini_get_setting(INI_SCREEN_MODE);
    Svga_ScaleQuality = ini_get_setting(INI_SCALE_QUALITY);
    Svga_ScreenWidth = ini_get_setting(INI_WINDOW_WIDTH);
    Svga_ScreenHeight = ini_get_setting(INI_WINDOW_HEIGHT);

    Svga_CorrectAspectRatio(&display_mode);

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
            AiLog log("Unsupported screen mode: %i\n", Svga_ScreenMode);
        } break;
    }

    if (Svga_ScreenMode == WINDOW_MODE_WINDOWED) {
        *bounds = {0, 0, Svga_ScreenWidth, Svga_ScreenHeight};

    } else {
        if (SDL_GetDisplayBounds(Svga_DisplayIndex, bounds)) {
            AiLog log("SDL_GetDisplayBounds failed: %s\n", SDL_GetError());
            *bounds = {0, 0, Svga_ScreenWidth, Svga_ScreenHeight};
        }
    }

    return flags;
}

void Svga_CorrectAspectRatio(SDL_DisplayMode *display_mode) {
    const double minimum_aspect_ratio = static_cast<double>(SVGA_DEFAULT_WIDTH) / SVGA_DEFAULT_HEIGHT;
    const double display_aspect_ratio = static_cast<double>(display_mode->w) / display_mode->h;
    double user_aspect_ratio = static_cast<double>(Svga_ScreenWidth) / Svga_ScreenHeight;

    if (Svga_ScreenHeight < SVGA_DEFAULT_HEIGHT) {
        Svga_ScreenHeight = SVGA_DEFAULT_HEIGHT;
    }

    if (user_aspect_ratio < minimum_aspect_ratio) {
        user_aspect_ratio = minimum_aspect_ratio;
    }

    if (!ini_get_setting(INI_DISABLE_AR_CORRECTION)) {
        user_aspect_ratio = display_aspect_ratio;
    }

    Svga_ScreenWidth = static_cast<double>(Svga_ScreenHeight) * user_aspect_ratio + .9;
}

int32_t Svga_Init(void) {
    if (sdl_win_init_flag) {
        return 0;
    }

    SDL_Rect bounds;
    Uint32 flags = Svga_SetupDisplayMode(&bounds);

    if ((sdlWindow = SDL_CreateWindow(
             "M.A.X.: Mechanized Assault & Exploration", SDL_WINDOWPOS_CENTERED_DISPLAY(Svga_DisplayIndex),
             SDL_WINDOWPOS_CENTERED_DISPLAY(Svga_DisplayIndex), bounds.w, bounds.h, flags)) == nullptr) {
        AiLog log("SDL_CreateWindow failed: %s\n", SDL_GetError());
    }

    if ((sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE)) ==
        nullptr) {
        AiLog log("SDL_CreateRenderer failed: %s\n", SDL_GetError());
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
            AiLog log("Unsupported scale quality: %i\n", Svga_ScaleQuality);
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
            break;
    }

    SDL_RenderSetLogicalSize(sdlRenderer, Svga_ScreenWidth, Svga_ScreenHeight);

    sdlWindowSurface =
        SDL_CreateRGBSurfaceWithFormat(0, Svga_ScreenWidth, Svga_ScreenHeight, 32, Svga_DisplayPixelFormat);
    sdlPaletteSurface =
        SDL_CreateRGBSurfaceWithFormat(0, Svga_ScreenWidth, Svga_ScreenHeight, 8, SDL_PIXELFORMAT_INDEX8);
    sdlTexture = SDL_CreateTexture(sdlRenderer, Svga_DisplayPixelFormat, SDL_TEXTUREACCESS_STREAMING, Svga_ScreenWidth,
                                   Svga_ScreenHeight);

    scr_blit = &Svga_Blit;

    scr_size.lrx = Svga_ScreenWidth - 1;
    scr_size.lry = Svga_ScreenHeight - 1;
    scr_size.ulx = 0;
    scr_size.uly = 0;

    if (0 != SDL_RenderClear(sdlRenderer)) {
        AiLog log("SDL_RenderClear failed: %s\n", SDL_GetError());
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

void Svga_Blit(uint8_t *srcBuf, uint32_t srcW, uint32_t srcH, uint32_t subX, uint32_t subY, uint32_t subW,
               uint32_t subH, uint32_t dstX, uint32_t dstY) {
    SDL_assert(sdlPaletteSurface && sdlPaletteSurface->format &&
               sdlPaletteSurface->format->BytesPerPixel == sizeof(uint8_t));

    {
        buf_to_buf(&srcBuf[subX + srcW * subY], subW, subH, srcW,
                   &((uint8_t *)sdlPaletteSurface->pixels)[dstX + sdlPaletteSurface->pitch * dstY],
                   sdlPaletteSurface->pitch);
    }

    SDL_Rect bounds = {static_cast<int32_t>(dstX), static_cast<int32_t>(dstY), static_cast<int32_t>(subW),
                       static_cast<int32_t>(subH)};

    if (Svga_PaletteChanged) {
        Svga_PaletteChanged = false;

        bounds.x = 0;
        bounds.y = 0;
        bounds.w = sdlPaletteSurface->w;
        bounds.h = sdlPaletteSurface->h;
    }

    /* Blit 8-bit palette surface onto the window surface that's closer to the texture's format */
    if (SDL_LowerBlit(sdlPaletteSurface, &bounds, sdlWindowSurface, &bounds) != 0) {
        AiLog log("SDL_BlitSurface failed: %s\n", SDL_GetError());
    }

    if (SVGA_NO_TEXTURE_UPDATE) {
        Uint32 *source_pixels = &((Uint32 *)sdlWindowSurface->pixels)[bounds.x + sdlWindowSurface->w * bounds.y];
        void *target_pixels = nullptr;
        int32_t target_pitch = 0;

        if (SDL_LockTexture(sdlTexture, &bounds, &target_pixels, &target_pitch)) {
            AiLog log("SDL_LockTexture failed: %s\n", SDL_GetError());

        } else {
            for (uint32_t h = 0; h < bounds.h; ++h) {
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
            AiLog log("SDL_UpdateTexture failed: %s\n", SDL_GetError());
        }
    }

    /* Make the modified texture visible by rendering it */
    if (SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, nullptr) != 0) {
        AiLog log("SDL_RenderCopy failed: %s\n", SDL_GetError());
    }

    SDL_RenderPresent(sdlRenderer);
}

int32_t Svga_WarpMouse(int32_t window_x, int32_t window_y) {
    int32_t result;

    if (mouse_get_lock() == MOUSE_LOCK_LOCKED) {
        SDL_WarpMouseInWindow(sdlWindow, window_x, window_y);
        result = true;

    } else {
        result = false;
    }

    return result;
}

void Svga_RefreshSystemPalette(bool force) {
    if (force || (timer_elapsed_time(Svga_RenderTimer) >= TIMER_FPS_TO_MS(Svga_DisplayRefreshRate))) {
        Svga_RenderTimer = timer_get();
        Svga_PaletteChanged = true;

        uint8_t srcBuf;

        Svga_Blit(&srcBuf, 0, 0, 0, 0, 0, 0, 0, 0);
    }
}

void Svga_SetPaletteColor(int32_t index, SDL_Color *color) {
    if (SDL_SetPaletteColors(sdlPaletteSurface->format->palette, color, index, 1)) {
        AiLog log("SDL_SetPaletteColors failed: %s\n", SDL_GetError());
    }

    Svga_RefreshSystemPalette(index == PALETTE_SIZE - 1);
}

void Svga_SetPalette(SDL_Palette *palette) {
    if (SDL_SetSurfacePalette(sdlPaletteSurface, palette)) {
        AiLog log("SDL_SetSurfacePalette failed: %s\n", SDL_GetError());
    }

    Svga_RefreshSystemPalette(true);
}

SDL_Palette *Svga_GetPalette(void) { return sdlPaletteSurface->format->palette; }

int32_t Svga_GetScreenWidth(void) { return Svga_ScreenWidth; }

int32_t Svga_GetScreenHeight(void) { return Svga_ScreenHeight; }

int32_t Svga_GetScreenRefreshRate(void) { return Svga_DisplayRefreshRate; }

bool Svga_IsFullscreen(void) { return (sdlWindow && (SDL_GetWindowFlags(sdlWindow) & (SDL_WINDOW_FULLSCREEN))); }

bool Svga_GetWindowFlags(uint32_t *flags) {
    bool result{false};

    if (flags && sdlWindow) {
        *flags = SDL_GetWindowFlags(sdlWindow);

        result = true;
    }

    return result;
}
