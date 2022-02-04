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
#include "sound_manager.hpp"
#include "window_manager.hpp"

static void* mve_cb_alloc(size_t size);
static void mve_cb_free(void* p);
static int mve_cb_read(FILE* handle, void* buf, size_t count);
static int mve_cb_ctl(void);
static void movie_cb_show_frame(unsigned char* buffer, int bufw, int bufh, int sx, int sy, int w, int h, int dstx,
                                int dsty);
static void movie_cb_set_palette(unsigned char* p, int start, int count);
static void movie_init_palette(void);
static int movie_run(ResourceID resource_id, int mode);

static unsigned int movie_music_level;
static SDL_Palette* movie_palette;

void* mve_cb_alloc(size_t size) { return malloc(size); }

void mve_cb_free(void* p) { free(p); }

int mve_cb_read(FILE* handle, void* buf, size_t count) { return fread(buf, 1, count, handle); }

int mve_cb_ctl(void) {
    int input;
    int result;

    input = get_input();

    if (input > 0 || (mouse_get_buttons() & (MOUSE_PRESS_LEFT | MOUSE_PRESS_RIGHT))) {
        result = 1;
    } else {
        result = 0;
    }

    return result;
}

void movie_cb_show_frame(unsigned char* buffer, int bufw, int bufh, int sx, int sy, int w, int h, int dstx, int dsty) {
    update_system_palette(movie_palette, 0);
    vesa_screen_blit(buffer, bufw, bufh, sx, sy, w, h, dstx, dsty);
}

void movie_cb_set_palette(unsigned char* p, int start, int count) {
    SDL_assert(movie_palette);

    for (int n = start; n < (start + count); n++) {
        movie_palette->colors[n].r = p[3 * n + 0] * 4;
        movie_palette->colors[n].g = p[3 * n + 1] * 4;
        movie_palette->colors[n].b = p[3 * n + 2] * 4;
        movie_palette->colors[n].a = 0;
    }
}

static void movie_init_palette(void) {
    if (NULL == movie_palette) {
        movie_palette = SDL_AllocPalette(PALETTE_SIZE);
        SDL_assert(movie_palette);
    }

    SDL_Color default_color = {0, 0, 0, 0};
    SDL_Color subtitle_color = {50, 50, 50, 0};

    for (int n = 0; n < PALETTE_SIZE; n++) {
        movie_palette->colors[n] = default_color;
    }

    movie_palette->colors[PALETTE_SIZE - 1] = subtitle_color;
}

int movie_run(ResourceID resource_id, int mode) {
    FILE* fp;
    char result;
    char path[PATH_MAX];
    char* file_name;
    unsigned char* palette;

    soundmgr.FreeMusic();

    WindowManager_ClearWindow();

    file_name = reinterpret_cast<char*>(ResourceManager_ReadResource(resource_id));
    if (file_name) {
        path[0] = '\0';

        if (mode) {
            strcpy(path, ResourceManager_FilePathMovie);
        }

        strcat(path, strupr(file_name));

        fp = fopen(path, "rb");

        if (fp) {
            palette = getSystemPalette();
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

            MVE_sfSVGA(640, 480, 640, 0, NULL, 0, 0, NULL, 0);

            if (!MVE_rmUnprotect() && !MVE_rmPrepMovie(fp, -1, -1, 0)) {
                int aborted = 0;
                int frame_index = 0;

                for (; (result = MVE_rmStepMovie()) == 0 && !aborted;) {
                    if (mve_cb_ctl()) {
                        aborted = 1;
                        result = 1;
                    }

                    frame_index++;

                    SDL_Delay(33);
                }

                MVE_rmEndMovie();
            }

            MVE_ReleaseMem();

            memcpy(getColorPalette(), palette, 3 * PALETTE_SIZE);
            setColorPalette(getColorPalette());
            fclose(fp);

            result = 0;

        } else {
            result = 1;
        }

        if (!result || result == 3) {
            win_reinit(init_mode_640_480);
            WindowManager_ClearWindow();

            result = 0;
        }

        delete[] file_name;

    } else {
        result = 0;
    }

    return result;
}

int Movie_PlayOemLogo(void) { return movie_run(LOGOFLIC, 0); }

int Movie_PlayIntro(void) { return movie_run(INTROFLC, 1); }

int Movie_Play(ResourceID resource_id) {
    WindowManager_ClearWindow();
    return movie_run(resource_id, 0);
}
