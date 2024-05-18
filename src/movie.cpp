/* Copyright (c) 2022 M.A.X. Port Team
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

#include "movie.hpp"

#include "cursor.hpp"
#include "gnw.h"
#include "inifile.hpp"
#include "mvelib32.h"
#include "resource_manager.hpp"
#include "smartstring.hpp"
#include "sound_manager.hpp"
#include "window_manager.hpp"

static void* mve_cb_alloc(size_t size);
static void mve_cb_free(void* p);
static int32_t mve_cb_read(FILE* handle, void* buf, size_t count);
static int32_t mve_cb_ctl(void);
static void movie_cb_show_frame(uint8_t* buffer, int32_t bufw, int32_t bufh, int32_t sx, int32_t sy, int32_t w,
                                int32_t h, int32_t dstx, int32_t dsty);
static void movie_cb_set_palette(uint8_t* p, int32_t start, int32_t count);
static void movie_init_palette(void);
static int32_t movie_run(ResourceID resource_id);

static uint32_t movie_music_level;

static uint8_t* gfx_buf;

void* mve_cb_alloc(size_t size) { return malloc(size); }

void mve_cb_free(void* p) { free(p); }

int32_t mve_cb_read(FILE* handle, void* buf, size_t count) { return fread(buf, 1, count, handle); }

int32_t mve_cb_ctl(void) {
    int32_t input;
    int32_t result;

    input = get_input();

    if (input > 0 || (mouse_get_buttons() & (MOUSE_PRESS_LEFT | MOUSE_PRESS_RIGHT))) {
        result = 1;

    } else {
        result = 0;
    }

    return result;
}

void movie_cb_show_frame(uint8_t* buffer, int32_t bufw, int32_t bufh, int32_t sx, int32_t sy, int32_t w, int32_t h,
                         int32_t dstx, int32_t dsty) {
    const int32_t window_width = Svga_GetScreenWidth();
    const int32_t window_height = Svga_GetScreenHeight();

    const float scale_w = static_cast<float>(window_width) / bufw;
    const float scale_h = static_cast<float>(window_height) / bufh;
    const float scale = (scale_h >= scale_w) ? scale_w : scale_h;

    int32_t frame_width = scale * bufw;
    int32_t frame_height = scale * bufh;

    frame_width = std::min(frame_width, window_width);
    frame_height = std::min(frame_height, window_height);

    const int32_t frame_offset_x = (window_width - frame_width) / 2;
    const int32_t frame_offset_y = (window_height - frame_height) / 2;

    cscale(buffer, bufw, bufh, bufw, gfx_buf, frame_width, frame_height, window_width);

    Svga_Blit(gfx_buf, window_width, window_height, 0, 0, frame_width, frame_height, frame_offset_x, frame_offset_y);
}

void movie_cb_set_palette(uint8_t* p, int32_t start, int32_t count) {
    for (int32_t i = start; i < (start + count); i++) {
        SDL_Color color;

        color.r = p[PALETTE_STRIDE * i + 0] * 4;
        color.g = p[PALETTE_STRIDE * i + 1] * 4;
        color.b = p[PALETTE_STRIDE * i + 2] * 4;
        color.a = 0;

        Svga_SetPaletteColor(i, &color);
    }
}

static void movie_init_palette(void) {
    SDL_Color color;

    for (int32_t i = 0; i < PALETTE_SIZE; i++) {
        color.r = 0;
        color.g = 0;
        color.b = 0;
        color.a = 0;

        Svga_SetPaletteColor(i, &color);
    }

    color.r = 50;
    color.g = 50;
    color.b = 50;
    color.a = 0;

    Svga_SetPaletteColor(PALETTE_SIZE - 1, &color);
}

int32_t movie_run(ResourceID resource_id) {
    int32_t result;
    uint8_t* palette;

    SoundManager_FreeMusic();

    WindowManager_ClearWindow();

    auto fp{ResourceManager_OpenFileResource(resource_id, ResourceType_Movie)};

    if (fp) {
        palette = Color_GetSystemPalette();
        Cursor_SetCursor(CURSOR_HIDDEN);
        mouse_show();

        MVE_memCallbacks(mve_cb_alloc, mve_cb_free);
        MVE_ioCallbacks(mve_cb_read);
        MVE_rmCallbacks(mve_cb_ctl);
        MVE_sfCallbacks(movie_cb_show_frame);

        movie_init_palette();
        MVE_palCallbacks(movie_cb_set_palette);

        movie_music_level = (32767 * ini_get_setting(INI_MUSIC_LEVEL)) / 100;

        if (ini_get_setting(INI_DISABLE_MUSIC)) {
            movie_music_level = 0;
        }

        MVE_sndVolume(movie_music_level);

        if (ini_get_setting(INI_MOVIE_PLAY)) {
            MVE_rmFastMode(1);
        }

        const int32_t gfx_buf_size = Svga_GetScreenWidth() * Svga_GetScreenHeight();

        gfx_buf = (uint8_t*)malloc(Svga_GetScreenWidth() * Svga_GetScreenHeight());

        memset(gfx_buf, 0, gfx_buf_size);

        MVE_sfSVGA(Svga_GetScreenWidth(), Svga_GetScreenHeight(), Svga_GetScreenWidth(), 0, nullptr, 0, 0, nullptr, 0);

        if (!MVE_rmPrepMovie(fp, -1, -1, 0)) {
            int32_t aborted = 0;
            int32_t frame_index = 0;

            for (; (result = MVE_rmStepMovie()) == 0 && !aborted;) {
                if (mve_cb_ctl()) {
                    aborted = 1;
                    result = 1;
                }

                ++frame_index;
            }

            MVE_rmEndMovie();
        }

        free(gfx_buf);
        gfx_buf = NULL;

        MVE_ReleaseMem();

        Color_SetColorPalette(palette);
        fclose(fp);

        result = 0;

    } else {
        result = 1;
    }

    if (!result || result == 3) {
        win_reinit(Svga_Init);
        WindowManager_ClearWindow();

        result = 0;
    }

    return result;
}

int32_t Movie_PlayOemLogo(void) { return movie_run(LOGOFLIC); }

int32_t Movie_PlayIntro(void) { return movie_run(INTROFLC); }

int32_t Movie_Play(ResourceID resource_id) {
    WindowManager_ClearWindow();
    return movie_run(resource_id);
}
