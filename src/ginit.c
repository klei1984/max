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

#include "game.h"

char file_path_cdrom[PATH_MAX];
char file_path_game_install[PATH_MAX];
char file_path_game_res[PATH_MAX];
char file_path_movie[PATH_MAX];
char file_path_text[PATH_MAX];
char file_path_flc[PATH_MAX];
char file_path_voice_spw[PATH_MAX];
char file_path_sfx_spw[PATH_MAX];
char file_path_msc[PATH_MAX];

char is_max_cd_in_use;

int ginit_change_drive_to_cdrom(char prompt_user, char restore_drive_on_error) {
    unsigned int total;
    unsigned int drive;
    unsigned int cdrom_drive;
    unsigned int new_drive;

    cdrom_drive = file_path_cdrom[0] - '@';

    dos_getdrive(&drive);
    dos_setdrive(cdrom_drive, &total);
    dos_getdrive(&new_drive);

    while (new_drive != cdrom_drive || chdir(file_path_cdrom)) {
        if (!prompt_user || !draw_yes_no_popup("\nPlease insert the M.A.X. CD and try again.\n", 1)) {
            dos_setdrive(drive, &total);

            return 0;
        }
    }

    if (restore_drive_on_error) {
        dos_setdrive(drive, &total);
    }

    return 1;
}

void ginit_init_paths(int argc, char *argv[]) {
    char dst[PATH_MAX];
    short i;

    if ((*argv)[1] == ':') {
        strcpy(dst, *argv);
        i = strlen(dst);

        while (--i != -1) {
            if (dst[i] == '\\') {
                dst[i + 1] = '\0';

                break;
            }
        }

        if (i < 1) dst[2] = 0;
    } else {
        dst[0] = 0;
    }

    file_path_cdrom[0] = '\0';
    file_path_game_install[0] = '\0';
    file_path_game_res[0] = '\0';
    file_path_voice_spw[0] = '\0';
    file_path_movie[0] = '\0';
    file_path_text[0] = '\0';
    file_path_flc[0] = '\0';
    file_path_sfx_spw[0] = '\0';
    file_path_msc[0] = '\0';

    is_max_cd_in_use = 0;

    for (i = 0; i < argc; i++) {
        strlwr(argv[i]);

        if (strstr(argv[i], "-s") || strstr(argv[i], "-m") || strstr(argv[i], "-l") || strstr(argv[i], "-f")) {
            strcpy(file_path_cdrom, argv[i] + 2);
            strupr(file_path_cdrom);

            if (strstr(argv[i], "-s") || strstr(argv[i], "-m") || strstr(argv[i], "-l")) {
                if (!ginit_change_drive_to_cdrom(0, 0)) {
                    printf("\nPlease insert the M.A.X. CD and try again.\n");
                    exit(1);
                }

                is_max_cd_in_use = 1;

                strcpy(file_path_voice_spw, file_path_cdrom);
                strcpy(file_path_movie, file_path_cdrom);
                strcpy(file_path_text, file_path_cdrom);
                strcpy(file_path_flc, file_path_cdrom);
                strcpy(file_path_sfx_spw, file_path_cdrom);
                strcpy(file_path_msc, file_path_cdrom);

                strcat(file_path_voice_spw, "\\");
                strcat(file_path_movie, "\\");
                strcat(file_path_text, "\\");
                strcat(file_path_flc, "\\");
                strcat(file_path_sfx_spw, "\\");
                strcat(file_path_msc, "\\");
            }

            strcpy(file_path_game_install, dst);
            strcpy(file_path_game_res, file_path_game_install);

            if (strstr(argv[i], "-m") || strstr(argv[i], "-l") || strstr(argv[i], "-f")) {
                strcpy(file_path_voice_spw, file_path_game_install);
                strcpy(file_path_flc, file_path_game_install);
                strcpy(file_path_sfx_spw, file_path_game_install);

                if (strstr(argv[i], "-l") || strstr(argv[i], "-f")) {
                    strcpy(file_path_msc, file_path_game_install);
                }

                if (strstr(argv[i], "-f")) {
                    strcpy(file_path_movie, file_path_game_install);
                    strcpy(file_path_text, file_path_game_install);
                }
            }
        }
    }
}
