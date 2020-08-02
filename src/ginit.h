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

#ifdef __unix__
#include <linux/limits.h>
#endif

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

int ginit_change_drive_to_cdrom(char prompt_user, char restore_drive_on_error);
void ginit_init_paths(int argc, char *argv[]);

#endif /* GINIT_H */
