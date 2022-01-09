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

#include "ginit.h"

#include "inifile.hpp"
#include "resource_manager.hpp"
#include "soundmgr.hpp"

extern "C" {
#include "gnw.h"
}

/// \todo Fix includes and dependencies
extern "C" {
typedef ResourceID GAME_RESOURCE;
typedef void GameResourceMeta;
#define RESRCMGR_H
#include "gwindow.h"
#include "mvelib32.h"
#include "screendump.h"
#include "units.h"
#include "wrappers.h"
}

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
char disable_enhanced_graphics;

const char *const error_code_lut[] = {"",
                                      "\nThanks for playing M.A.X.!\n\n",
                                      "\nNo mouse driver found.\n\n",
                                      "\nNot enough memory for buffers.\n\n",
                                      "\nSound card not found.\n\n",
                                      "\nScreen Init failed.\n\n",
                                      "\nUnable to find game resource file.\n\n",
                                      "\nError reading resource file.\n\n",
                                      "\nInvalid resource file ID.\n\n",
                                      "\nInvalid Directory name.\n\n",
                                      "\nInvalid script file.\n\n",
                                      "\nError reading script file.\n\n",
                                      "\nM.A.X. INI not found.\n\n",
                                      "\nPlease insert the M.A.X. CD and try again.\n\n",
                                      "\nTo play M.A.X. type MC\n\n",
                                      "\nThis Version has Expired.\n\n",
                                      "\nToo many files open or .WRL file not found.\n\n"};

static void check_available_extended_memory(void);
static void check_available_disk_space(void);
static void ginit_init(void);
static void check_mouse_driver(void);
static void menu_draw_exit_logos(void);

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
                    SDL_Log("\nPlease insert the M.A.X. CD and try again.\n");
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

void ginit_init_resources(void) {
    check_available_extended_memory();
    check_available_disk_space();
    ginit_init();
    check_mouse_driver();
}

void check_available_extended_memory(void) {
    int memory;

    memory = SDL_GetSystemRAM();
    if (memory < 6L) {
        SDL_Log("\nNot enough memory available to run M.A.X.\nAmount Needed: %i MB, Amount found: %i MB\n\n", 6,
                memory);
        exit(1);
    }
}

void check_available_disk_space(void) {
    /// \todo    unsigned long long available_disk_space;
    //    int pressed_key;
    //
    //    char *pref_path = NULL;
    //    char *base_path = NULL;
    //
    //    pref_path = SDL_GetPrefPath("Interplay", "MAX");
    //    base_path = SDL_GetBasePath();
    //
    //    if (!base_path) {
    //        SDL_Log("SDL_GetBasePath failed: %s\n", SDL_GetError());
    //    }
    //
    //    available_disk_space = 0;
    //    \todo Portable way to determine free space in user profile specific directory
    //
    //    if (available_disk_space < 250000ULL) {
    //        printf("\n\n");
    //        printf("The Drive %c has only %llu bytes available.  You may have trouble saving games...\n", drive,
    //               available_disk_space);
    //        printf("\nPress ESC to exit, any other key to continue...");
    //        printf("\n\n");
    //
    //        do {
    //            pressed_key = getch();
    //        } while (pressed_key <= 0);
    //
    //        if (pressed_key == 27 /* ESC */) {
    //            exit(1);
    //        }
    //    }
}

void ginit_init(void) {
    unsigned char error_code;

    if (gwin_init()) {
        gexit(EXIT_CODE_SCREEN_INIT_FAILED);
    }

    error_code = ResourceManager_Init();

    if (error_code) {
        gexit(error_code);
    }

    ini_config.Init();
    ini_clans.Init();

    disable_enhanced_graphics = get_dpmi_physical_memory() < 13312000L;
    ini_set_setting(INI_ENHANCED_GRAPHICS, !disable_enhanced_graphics);

    timer_init();
    soundmgr.Init();
    timer_ch2_setup();

    register_pause(-1, NULL);
    register_screendump(302, screendump_pcx);
}

void check_mouse_driver(void) {
    if (!mouse_query_exist()) {
        gexit(EXIT_CODE_NO_MOUSE);
    }
}

void gexit(unsigned char error_code) {
    soundmgr.FreeAllSamples();

    if ((error_code == 0) || (error_code == 1)) {
        menu_draw_exit_logos();
    }

    soundmgr.Deinit();

    timer_close();

    win_exit();

    reset_mode();

    SDL_Log("%s", error_code_lut[error_code]);

    exit(0);
}

void menu_draw_exit_logos(void) {
    mouse_hide();
    menu_display_logo(OEMONE, 60000);
    menu_display_logo(OEMTWO, 60000);
}
