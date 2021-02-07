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

#ifndef GINIT_H
#define GINIT_H

#include <string.h>
#include <stdlib.h>

#ifdef __unix__
#include <linux/limits.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
    EXIT_CODE_NO_ERROR,
    EXIT_CODE_THANKS,
    EXIT_CODE_NO_MOUSE,
    EXIT_CODE_INSUFFICIENT_MEMORY,
    EXIT_CODE_NO_SOUND_CARD,
    EXIT_CODE_SCREEN_INIT_FAILED,
    EXIT_CODE_RES_FILE_NOT_FOUND,
    EXIT_CODE_CANNOT_READ_RES_FILE,
    EXIT_CODE_RES_FILE_INVALID_ID,
    EXIT_CODE_INVALID_DIR_NAME,
    EXIT_CODE_INVALID_SCRIPT_FILE,
    EXIT_CODE_CANNOT_READ_SCRIPT_FILE,
    EXIT_CODE_CANNOT_FIND_MAX_INI,
    EXIT_CODE_CANNOT_FIND_MAX_CD,
    EXIT_CODE_HOW_TO_START_MAX,
    EXIT_CODE_VERSION_EXPIRED,
    EXIT_CODE_WRL_FILE_OPEN_ERROR
};

extern char file_path_cdrom[PATH_MAX];
extern char file_path_game_install[PATH_MAX];
extern char file_path_game_res[PATH_MAX];
extern char file_path_movie[PATH_MAX];
extern char file_path_text[PATH_MAX];
extern char file_path_flc[PATH_MAX];
extern char file_path_voice_spw[PATH_MAX];
extern char file_path_sfx_spw[PATH_MAX];
extern char file_path_msc[PATH_MAX];

extern char is_max_cd_in_use;
extern char disable_enhanced_graphics;

int ginit_change_drive_to_cdrom(char prompt_user, char restore_drive_on_error);
void ginit_init_paths(int argc, char *argv[]);
void ginit_init_resources(void);
void gexit(unsigned char error_code);

#ifdef __cplusplus
}
#endif

#endif /* GINIT_H */
