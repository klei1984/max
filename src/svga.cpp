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

#include "gnw.h"
#include "input.h"
#include "resource_manager.hpp"
#include "settings.hpp"

#define SVGA_DEFAULT_WIDTH (640)
#define SVGA_DEFAULT_HEIGHT (480)
#define SVGA_DEFAULT_REFRESH_RATE (30)

static Uint32 Svga_SetupDisplayMode(SDL_Rect* bounds);
static void Svga_CorrectAspectRatio(SDL_DisplayMode* display_mode);
static void Svga_RefreshSystemPalette(bool force);

static const bool SVGA_NO_TEXTURE_UPDATE = true;

static SDL_Window* sdlWindow;
static SDL_Renderer* sdlRenderer;
static SDL_Surface* sdlWindowSurface;
static SDL_Surface* sdlPaletteSurface;
static SDL_Texture* sdlTexture;
static uint64_t Svga_RenderTimer;
static uint64_t Svga_BackgroundTimer;
static int32_t sdl_win_init_flag;
static bool Svga_PaletteChanged;
static bool Svga_RenderDirty;

static int32_t Svga_ScreenWidth;
static int32_t Svga_ScreenHeight;
static int32_t Svga_ScreenMode;
static int32_t Svga_ScaleQuality;
static int32_t Svga_DisplayIndex;
static SDL_DisplayID Svga_DisplayID;
static int32_t Svga_DisplayRefreshRate;
static SDL_PixelFormat Svga_DisplayPixelFormat;

Rect scr_size;
ScreenBlitFunc scr_blit;

Uint32 Svga_SetupDisplayMode(SDL_Rect* bounds) {
    Uint32 flags = 0uL;
    int display_count = 0;
    auto settings = ResourceManager_GetSettings();

    Svga_DisplayIndex = settings->GetNumericValue("display_index");

    SDL_DisplayID* displays = SDL_GetDisplays(&display_count);

    if (displays && Svga_DisplayIndex < display_count) {
        Svga_DisplayID = displays[Svga_DisplayIndex];

    } else {
        Svga_DisplayID = SDL_GetPrimaryDisplay();
    }

    SDL_free(displays);

    const SDL_DisplayMode* detected_display_mode = SDL_GetCurrentDisplayMode(Svga_DisplayID);
    SDL_DisplayMode display_mode;

    if (detected_display_mode) {
        display_mode = *detected_display_mode;

    } else {
        SDL_Log("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());

        display_mode.refresh_rate = SVGA_DEFAULT_REFRESH_RATE;
        display_mode.format = SDL_PIXELFORMAT_XRGB8888;
        display_mode.w = SVGA_DEFAULT_WIDTH;
        display_mode.h = SVGA_DEFAULT_HEIGHT;
    }

    Svga_DisplayRefreshRate = detected_display_mode->refresh_rate;
    Svga_DisplayPixelFormat = detected_display_mode->format;

    Svga_ScreenMode = settings->GetNumericValue("screen_mode");
    Svga_ScaleQuality = settings->GetNumericValue("scale_quality");
    Svga_ScreenWidth = settings->GetNumericValue("window_width");
    Svga_ScreenHeight = settings->GetNumericValue("window_height");

    Svga_CorrectAspectRatio(&display_mode);

    switch (Svga_ScreenMode) {
        case WINDOW_MODE_WINDOWED: {
            flags |= 0;

        } break;

        case WINDOW_MODE_FULLSCREEN: {
            flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;
        } break;

        case WINDOW_MODE_BORDERLESS: {
            flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;
        } break;

        default: {
            SDL_Log("Unsupported screen mode: %d\n", Svga_ScreenMode);
        } break;
    }

    if (Svga_ScreenMode == WINDOW_MODE_WINDOWED) {
        *bounds = {0, 0, Svga_ScreenWidth, Svga_ScreenHeight};

    } else {
        if (!SDL_GetDisplayBounds(Svga_DisplayID, bounds)) {
            SDL_Log("SDL_GetDisplayBounds failed: %s\n", SDL_GetError());

            *bounds = {0, 0, Svga_ScreenWidth, Svga_ScreenHeight};
        }
    }

    return flags;
}

void Svga_CorrectAspectRatio(SDL_DisplayMode* display_mode) {
    const double minimum_aspect_ratio = static_cast<double>(SVGA_DEFAULT_WIDTH) / SVGA_DEFAULT_HEIGHT;
    const double display_aspect_ratio = static_cast<double>(display_mode->w) / display_mode->h;
    double user_aspect_ratio = static_cast<double>(Svga_ScreenWidth) / Svga_ScreenHeight;

    if (Svga_ScreenHeight < SVGA_DEFAULT_HEIGHT) {
        Svga_ScreenHeight = SVGA_DEFAULT_HEIGHT;
    }

    if (user_aspect_ratio < minimum_aspect_ratio) {
        user_aspect_ratio = minimum_aspect_ratio;
    }

    if (!ResourceManager_GetSettings()->GetNumericValue("disable_ar_correction")) {
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

    if ((sdlWindow = SDL_CreateWindow("M.A.X.: Mechanized Assault & Exploration", bounds.w, bounds.h, flags)) ==
        nullptr) {
        SDL_Log("SDL_CreateWindow failed: %s\n", SDL_GetError());
    }

    if ((sdlRenderer = SDL_CreateRenderer(sdlWindow, nullptr)) == nullptr) {
        SDL_Log("SDL_CreateRenderer failed: %s\n", SDL_GetError());
    }

    if (!SDL_SetRenderLogicalPresentation(sdlRenderer, Svga_ScreenWidth, Svga_ScreenHeight,
                                          SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
        SDL_Log("SDL_SetRenderLogicalPresentation failed: %s\n", SDL_GetError());
    }

    sdlWindowSurface = SDL_CreateSurface(Svga_ScreenWidth, Svga_ScreenHeight, Svga_DisplayPixelFormat);
    sdlPaletteSurface = SDL_CreateSurface(Svga_ScreenWidth, Svga_ScreenHeight, SDL_PIXELFORMAT_INDEX8);

    SDL_Palette* palette = SDL_CreatePalette(PALETTE_SIZE);
    if (palette) {
        SDL_SetSurfacePalette(sdlPaletteSurface, palette);
        SDL_DestroyPalette(palette);
    }

    sdlTexture = SDL_CreateTexture(sdlRenderer, Svga_DisplayPixelFormat, SDL_TEXTUREACCESS_STREAMING, Svga_ScreenWidth,
                                   Svga_ScreenHeight);

    switch (Svga_ScaleQuality) {
        case 0: {
            SDL_SetTextureScaleMode(sdlTexture, SDL_SCALEMODE_NEAREST);
        } break;

        case 1: {
            SDL_SetTextureScaleMode(sdlTexture, SDL_SCALEMODE_LINEAR);
        } break;

        case 2: {
            // SDL_SetTextureScaleMode(sdlTexture, SDL_SCALEMODE_PIXELART);
        } break;

        default: {
            // Linear/best quality will be set per-texture
        } break;
    }

    scr_blit = &Svga_Blit;

    scr_size.lrx = Svga_ScreenWidth - 1;
    scr_size.lry = Svga_ScreenHeight - 1;
    scr_size.ulx = 0;
    scr_size.uly = 0;

    if (!SDL_RenderClear(sdlRenderer)) {
        SDL_Log("SDL_RenderClear failed: %s\n", SDL_GetError());
    }

    if (!SDL_RenderPresent(sdlRenderer)) {
        SDL_Log("SDL_RenderPresent failed: %s\n", SDL_GetError());
    }

    if (sdlWindowSurface && sdlTexture && sdlPaletteSurface) {
        sdl_win_init_flag = 1;
        Svga_BackgroundTimer = timer_get();

        return 0;

    } else {
        sdl_win_init_flag = 0;

        return -1;
    }
}

void Svga_Deinit(void) {
    if (sdl_win_init_flag) {
        remove_bk_process(Svga_BackgroundProcess);

        if (sdlTexture) {
            SDL_DestroyTexture(sdlTexture);
            sdlTexture = nullptr;
        }

        if (sdlPaletteSurface) {
            SDL_DestroySurface(sdlPaletteSurface);
            sdlPaletteSurface = nullptr;
        }

        if (sdlWindowSurface) {
            SDL_DestroySurface(sdlWindowSurface);
            sdlWindowSurface = nullptr;
        }

        if (sdlRenderer) {
            SDL_DestroyRenderer(sdlRenderer);
            sdlRenderer = nullptr;
        }

        if (sdlWindow) {
            SDL_DestroyWindow(sdlWindow);
            sdlWindow = nullptr;
        }

        sdl_win_init_flag = 0;
    }
}

// Stage 1: Copy INDEX8 buffer to palette surface.
static inline void svga_stage_index8_to_palette(uint8_t* srcBuf, uint32_t srcW, uint32_t srcH, uint32_t subX,
                                                uint32_t subY, uint32_t subW, uint32_t subH, uint32_t dstX,
                                                uint32_t dstY) {
    buf_to_buf(&srcBuf[subX + srcW * subY], subW, subH, srcW,
               &((uint8_t*)sdlPaletteSurface->pixels)[dstX + sdlPaletteSurface->pitch * dstY],
               sdlPaletteSurface->pitch);
}

// Stage 2: Convert palette surface to RGB surface. Bounds may be extended if palette changed.
static inline void svga_stage_palette_to_rgb(SDL_Rect* bounds) {
    if (Svga_PaletteChanged) {
        Svga_PaletteChanged = false;

        bounds->x = 0;
        bounds->y = 0;
        bounds->w = sdlPaletteSurface->w;
        bounds->h = sdlPaletteSurface->h;
    }

    if (!SDL_BlitSurfaceUnchecked(sdlPaletteSurface, bounds, sdlWindowSurface, bounds)) {
        SDL_Log("SDL_BlitSurface failed: %s\n", SDL_GetError());
    }
}

// Stage 3: Copy RGB surface region to texture.
static inline void svga_stage_rgb_to_texture(const SDL_Rect* bounds) {
    if (SVGA_NO_TEXTURE_UPDATE) {
        Uint32* source_pixels = &((Uint32*)sdlWindowSurface->pixels)[bounds->x + sdlWindowSurface->w * bounds->y];
        void* target_pixels = nullptr;
        int32_t target_pitch = 0;

        if (SDL_LockTexture(sdlTexture, bounds, &target_pixels, &target_pitch)) {
            for (int32_t h = 0; h < bounds->h; ++h) {
                SDL_memcpy(target_pixels, source_pixels, bounds->w * sizeof(Uint32));
                source_pixels += sdlWindowSurface->w;
                target_pixels = &(static_cast<Uint32*>(target_pixels)[target_pitch / sizeof(Uint32)]);
            }

            SDL_UnlockTexture(sdlTexture);

        } else {
            SDL_Log("SDL_LockTexture failed: %s\n", SDL_GetError());
        }

    } else {
        if (!SDL_UpdateTexture(sdlTexture, bounds,
                               &((Uint32*)sdlWindowSurface->pixels)[bounds->x + sdlPaletteSurface->pitch * bounds->y],
                               sdlWindowSurface->pitch)) {
            SDL_Log("SDL_UpdateTexture failed: %s\n", SDL_GetError());
        }
    }
}

// Stage 4: Render texture to screen.
static inline void svga_stage_render(void) {
    if (!SDL_RenderTexture(sdlRenderer, sdlTexture, nullptr, nullptr)) {
        SDL_Log("SDL_RenderTexture failed: %s\n", SDL_GetError());
    }

    Svga_RenderDirty = true;
}

void Svga_Blit(uint8_t* srcBuf, uint32_t srcW, uint32_t srcH, uint32_t subX, uint32_t subY, uint32_t subW,
               uint32_t subH, uint32_t dstX, uint32_t dstY) {
    SDL_assert(sdlPaletteSurface && SDL_BYTESPERPIXEL(sdlPaletteSurface->format) == sizeof(uint8_t));

    svga_stage_index8_to_palette(srcBuf, srcW, srcH, subX, subY, subW, subH, dstX, dstY);

    SDL_Rect bounds = {static_cast<int32_t>(dstX), static_cast<int32_t>(dstY), static_cast<int32_t>(subW),
                       static_cast<int32_t>(subH)};

    svga_stage_palette_to_rgb(&bounds);
    svga_stage_rgb_to_texture(&bounds);
    svga_stage_render();
}

void Svga_BlitIndex8ToRGB(uint8_t* srcBuf, uint32_t srcW, uint32_t srcH, uint32_t subX, uint32_t subY, uint32_t subW,
                          uint32_t subH, uint32_t dstX, uint32_t dstY) {
    SDL_assert(sdlPaletteSurface && SDL_BYTESPERPIXEL(sdlPaletteSurface->format) == sizeof(uint8_t));

    svga_stage_index8_to_palette(srcBuf, srcW, srcH, subX, subY, subW, subH, dstX, dstY);

    SDL_Rect bounds = {static_cast<int32_t>(dstX), static_cast<int32_t>(dstY), static_cast<int32_t>(subW),
                       static_cast<int32_t>(subH)};

    svga_stage_palette_to_rgb(&bounds);
}

void Svga_BlitRGBA(uint32_t* srcBuf, uint32_t srcW, uint32_t srcH, uint32_t subX, uint32_t subY, uint32_t subW,
                   uint32_t subH, uint32_t dstX, uint32_t dstY, uint32_t finalizeX, uint32_t finalizeY,
                   uint32_t finalizeW, uint32_t finalizeH) {
    SDL_assert(sdlWindowSurface);

    uint32_t* src = &srcBuf[subX + srcW * subY];
    uint32_t* dst = &((uint32_t*)sdlWindowSurface->pixels)[dstX + sdlWindowSurface->w * dstY];
    const uint32_t dst_pitch = sdlWindowSurface->w;

    for (uint32_t y = 0; y < subH; ++y) {
        for (uint32_t x = 0; x < subW; ++x) {
            uint32_t pixel = src[x];
            uint8_t alpha = (pixel >> 24) & 0xFF;

            if (alpha != 0) {
                dst[x] = pixel;
            }
        }

        src += srcW;
        dst += dst_pitch;
    }

    // Finalize the full frame region (not just the overlay region)
    SDL_Rect bounds = {static_cast<int32_t>(finalizeX), static_cast<int32_t>(finalizeY),
                       static_cast<int32_t>(finalizeW), static_cast<int32_t>(finalizeH)};

    svga_stage_rgb_to_texture(&bounds);
    svga_stage_render();
}

void Svga_BlitFinalize(uint32_t dstX, uint32_t dstY, uint32_t width, uint32_t height) {
    SDL_Rect bounds = {static_cast<int32_t>(dstX), static_cast<int32_t>(dstY), static_cast<int32_t>(width),
                       static_cast<int32_t>(height)};

    svga_stage_rgb_to_texture(&bounds);
    svga_stage_render();
}

void Svga_BackgroundProcess(void) {
    uint32_t elapsed = timer_elapsed_time(Svga_BackgroundTimer);
    bool should_present = Svga_RenderDirty || elapsed >= TIMER_FPS_TO_MS(SVGA_DEFAULT_REFRESH_RATE);

    if (should_present) {
        Svga_BackgroundTimer = timer_get();
        Svga_RenderDirty = false;

        // Ensure the current texture state is rendered before presenting
        if (!SDL_RenderTexture(sdlRenderer, sdlTexture, nullptr, nullptr)) {
            SDL_Log("SDL_RenderTexture failed: %s\n", SDL_GetError());
        }

        if (!SDL_RenderPresent(sdlRenderer)) {
            SDL_Log("SDL_RenderPresent failed: %s\n", SDL_GetError());
        }
    }
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
    if (force || (timer_elapsed_time(Svga_RenderTimer) >= TIMER_FPS_TO_MS(SVGA_DEFAULT_REFRESH_RATE))) {
        Svga_RenderTimer = timer_get();
        Svga_PaletteChanged = true;

        uint8_t srcBuf;

        Svga_Blit(&srcBuf, 0, 0, 0, 0, 0, 0, 0, 0);
    }
}

void Svga_SetPaletteColor(int32_t index, SDL_Color* color) {
    auto palette = SDL_GetSurfacePalette(sdlPaletteSurface);

    if (!palette || !SDL_SetPaletteColors(palette, color, index, 1)) {
        SDL_Log("SDL_SetPaletteColors failed: %s\n", SDL_GetError());
    }

    Svga_RefreshSystemPalette(index == PALETTE_SIZE - 1);
}

void Svga_SetPalette(SDL_Palette* palette) {
    if (!SDL_SetSurfacePalette(sdlPaletteSurface, palette)) {
        SDL_Log("SDL_SetSurfacePalette failed: %s\n", SDL_GetError());
    }

    Svga_RefreshSystemPalette(true);
}

SDL_Palette* Svga_GetPalette(void) { return SDL_GetSurfacePalette(sdlPaletteSurface); }

int32_t Svga_GetScreenWidth(void) { return Svga_ScreenWidth; }

int32_t Svga_GetScreenHeight(void) { return Svga_ScreenHeight; }

int32_t Svga_GetScreenRefreshRate(void) { return Svga_DisplayRefreshRate; }

bool Svga_IsFullscreen(void) { return (sdlWindow && (SDL_GetWindowFlags(sdlWindow) & (SDL_WINDOW_FULLSCREEN))); }

bool Svga_GetWindowFlags(uint32_t* flags) {
    bool result{false};

    if (flags && sdlWindow) {
        *flags = SDL_GetWindowFlags(sdlWindow);

        result = true;
    }

    return result;
}

SDL_Window* Svga_GetWindow(void) { return sdlWindow; }

bool Svga_GetPaletteSurfaceData(uint8_t* buffer, int32_t buffer_size, uint8_t* palette_out) {
    if (!sdlPaletteSurface || !buffer) {
        return false;
    }

    const int32_t required_size = sdlPaletteSurface->w * sdlPaletteSurface->h;

    if (buffer_size < required_size) {
        return false;
    }

    // Copy the palette surface pixels (indexed format)
    const uint8_t* src = static_cast<uint8_t*>(sdlPaletteSurface->pixels);

    for (int32_t y = 0; y < sdlPaletteSurface->h; ++y) {
        SDL_memcpy(&buffer[y * sdlPaletteSurface->w], &src[y * sdlPaletteSurface->pitch], sdlPaletteSurface->w);
    }

    // Copy the palette if requested
    if (palette_out) {
        SDL_Palette* palette = SDL_GetSurfacePalette(sdlPaletteSurface);

        if (palette && palette->colors) {
            for (int32_t i = 0; i < PALETTE_SIZE && i < palette->ncolors; ++i) {
                // Convert from SDL_Color (0-255) to the game's 6-bit palette format (0-63)
                palette_out[PALETTE_STRIDE * i + 0] = palette->colors[i].r / 4;
                palette_out[PALETTE_STRIDE * i + 1] = palette->colors[i].g / 4;
                palette_out[PALETTE_STRIDE * i + 2] = palette->colors[i].b / 4;
            }
        }
    }

    return true;
}

void Svga_ToggleFullscreen(void) {
    if (sdlWindow) {
        const int32_t initial_mode = ResourceManager_GetSettings()->GetNumericValue("screen_mode");
        int32_t new_mode;
        SDL_Rect bounds;

        if (initial_mode == WINDOW_MODE_FULLSCREEN) {
            new_mode = (Svga_ScreenMode == WINDOW_MODE_FULLSCREEN) ? WINDOW_MODE_WINDOWED : WINDOW_MODE_FULLSCREEN;

        } else {
            new_mode = (Svga_ScreenMode == WINDOW_MODE_WINDOWED) ? WINDOW_MODE_BORDERLESS : WINDOW_MODE_WINDOWED;
        }

        if (!SDL_GetDisplayBounds(Svga_DisplayID, &bounds)) {
            SDL_Log("SDL_GetDisplayBounds failed: %s\n", SDL_GetError());
            return;
        }

        switch (new_mode) {
            case WINDOW_MODE_WINDOWED: {
                if (!SDL_SetWindowFullscreen(sdlWindow, false)) {
                    SDL_Log("SDL_SetWindowFullscreen(false) failed: %s\n", SDL_GetError());
                    return;
                }

                // Ensure window has a border (might have been borderless)
                if (!SDL_SetWindowBordered(sdlWindow, true)) {
                    SDL_Log("SDL_SetWindowBordered failed: %s\n", SDL_GetError());
                }

                // Set window size to logical resolution (aspect ratio corrected)
                if (!SDL_SetWindowSize(sdlWindow, Svga_ScreenWidth, Svga_ScreenHeight)) {
                    SDL_Log("SDL_SetWindowSize failed: %s\n", SDL_GetError());
                }

                // Center the window on the display
                if (!SDL_SetWindowPosition(sdlWindow, SDL_WINDOWPOS_CENTERED_DISPLAY(Svga_DisplayIndex),
                                           SDL_WINDOWPOS_CENTERED_DISPLAY(Svga_DisplayIndex))) {
                    SDL_Log("SDL_SetWindowPosition failed: %s\n", SDL_GetError());
                }
            } break;

            case WINDOW_MODE_FULLSCREEN: {
                // Set window size to desktop resolution before going fullscreen
                if (!SDL_SetWindowSize(sdlWindow, bounds.w, bounds.h)) {
                    SDL_Log("SDL_SetWindowSize failed: %s\n", SDL_GetError());
                }

                // Enter exclusive fullscreen mode
                if (!SDL_SetWindowFullscreen(sdlWindow, true)) {
                    SDL_Log("SDL_SetWindowFullscreen(true) failed: %s\n", SDL_GetError());
                    return;
                }
            } break;

            case WINDOW_MODE_BORDERLESS: {
                // Set window size to desktop resolution before going fullscreen
                if (!SDL_SetWindowSize(sdlWindow, bounds.w, bounds.h)) {
                    SDL_Log("SDL_SetWindowSize failed: %s\n", SDL_GetError());
                }

                // Enter borderless fullscreen mode (uses SDL_WINDOW_FULLSCREEN in SDL3, but with borderless behavior)
                if (!SDL_SetWindowFullscreen(sdlWindow, true)) {
                    SDL_Log("SDL_SetWindowFullscreen(true) failed: %s\n", SDL_GetError());
                    return;
                }
            } break;

            default: {
                SDL_Log("Svga_ToggleFullscreen: Unknown mode %d\n", new_mode);
                return;
            }
        }

        Svga_ScreenMode = new_mode;

        // Update mouse mode for fullscreen state
        mouse_set_fullscreen_mode(new_mode != WINDOW_MODE_WINDOWED);

        // Force a full screen refresh to ensure proper display
        Svga_RefreshSystemPalette(true);
    }
}
