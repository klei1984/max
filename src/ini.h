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

#ifndef INI_H
#define INI_H

#include <limits.h>

#ifdef __unix__
#include <linux/limits.h>
#endif

enum GAME_INI_e {
    INI_DEBUG = 0x1,
    INI_ALL_VISIBLE = 0x2,
    INI_DISABLE_FIRE = 0x3,
    INI_QUICK_BUILD = 0x4,
    INI_REAL_TIME = 0x5,
    INI_EXCLUDE_RANGE = 0x6,
    INI_PROXIMITY_RANGE = 0x7,
    INI_LOG_FILE_DEBUG = 0x8,
    INI_RAW_NORMAL_LOW = 0x9,
    INI_RAW_NORMAL_HIGH = 0xA,
    INI_RAW_CONCENTRATE_LOW = 0xB,
    INI_RAW_CONCENTRATE_HIGH = 0xC,
    INI_RAW_CONCENTRATE_SEPERATION = 0xD,
    INI_RAW_CONCENTRATE_DIFFUSION = 0xE,
    INI_FUEL_NORMAL_LOW = 0xF,
    INI_FUEL_NORMAL_HIGH = 0x10,
    INI_FUEL_CONCENTRATE_LOW = 0x11,
    INI_FUEL_CONCENTRATE_HIGH = 0x12,
    INI_FUEL_CONCENTRATE_SEPERATION = 0x13,
    INI_FUEL_CONCENTRATE_DIFFUSION = 0x14,
    INI_GOLD_NORMAL_LOW = 0x15,
    INI_GOLD_NORMAL_HIGH = 0x16,
    INI_GOLD_CONCENTRATE_LOW = 0x17,
    INI_GOLD_CONCENTRATE_HIGH = 0x18,
    INI_GOLD_CONCENTRATE_SEPERATION = 0x19,
    INI_GOLD_CONCENTRATE_DIFFUSION = 0x1A,
    INI_MIXED_RESOURCE_SEPERATION = 0x1B,
    INI_MIN_RESOURCES = 0x1C,
    INI_MAX_RESOURCES = 0x1D,
    INI_ATTACK_FACTOR = 0x1E,
    INI_SHOTS_FACTOR = 0x1F,
    INI_RANGE_FACTOR = 0x20,
    INI_ARMOR_FACTOR = 0x21,
    INI_HITS_FACTOR = 0x22,
    INI_SPEED_FACTOR = 0x23,
    INI_SCAN_FACTOR = 0x24,
    INI_STEAL_PERCENT = 0x25,
    INI_DISABLE_PERCENT = 0x26,
    INI_MAX_PERCENT = 0x27,
    INI_STEP_PERCENT = 0x28,
    INI_REPAIR_TURNS = 0x29,
    INI_UNKNOWN = 0x2A,
    INI_ALIEN_SEPERATION = 0x2B,
    INI_ALIEN_UNIT_VALUE = 0x2C,
    INI_RED_STRATEGY = 0x2D,
    INI_GREEN_STRATEGY = 0x2E,
    INI_BLUE_STRATEGY = 0x2F,
    INI_GRAY_STRATEGY = 0x30,
    INI_SETUP = 0x31,
    INI_PLAYER_NAME = 0x32,
    INI_PLAYER_CLAN = 0x33,
    INI_INTRO_MOVIE = 0x34,
    INI_IPX_SOCKET = 0x35,
    INI_MUSIC_LEVEL = 0x36,
    INI_FX_SOUND_LEVEL = 0x37,
    INI_VOICE_LEVEL = 0x38,
    INI_DISABLE_MUSIC = 0x39,
    INI_DISABLE_FX = 0x3A,
    INI_DISABLE_VOICE = 0x3B,
    INI_AUTO_SAVE = 0x3C,
    INI_GAME_FILE_NUMBER = 0x3D,
    INI_GAME_FILE_TYPE = 0x3E,
    INI_DEMO_TURNS = 0x3F,
    INI_ENHANCED_GRAPHICS = 0x40,
    INI_LAST_CAMPAIGN = 0x41,
    INI_MOVIE_PLAY = 0x42,
    INI_ALT_MOVIE_RES = 0x43,
    INI_OPTIONS = 0x44,
    INI_WORLD = 0x45,
    INI_TIMER = 0x46,
    INI_ENDTURN = 0x47,
    INI_START_GOLD = 0x48,
    INI_PLAY_MODE = 0x49,
    INI_VICTORY_TYPE = 0x4A,
    INI_VICTORY_LIMIT = 0x4B,
    INI_OPPONENT = 0x4C,
    INI_RAW_RESOURCE = 0x4D,
    INI_FUEL_RESOURCE = 0x4E,
    INI_GOLD_RESOURCE = 0x4F,
    INI_ALIEN_DERELICTS = 0x50,
    INI_PREFERENCES = 0x51,
    INI_EFFECTS = 0x52,
    INI_CLICK_SCROLL = 0x53,
    INI_QUICK_SCROLL = 0x54,
    INI_FAST_MOVEMENT = 0x55,
    INI_FOLLOW_UNIT = 0x56,
    INI_AUTO_SELECT = 0x57,
    INI_ENEMY_HALT = 0x58,
    INI_MODEM = 0x59,
    INI_MODEM_PORT = 0x5A,
    INI_MODEM_BAUD = 0x5B,
    INI_MODEM_IRQ = 0x5C,
    INI_MODEM_INIT = 0x5D,
    INI_MODEM_ANSWER = 0x5E,
    INI_MODEM_CALL = 0x5F,
    INI_MODEM_HANGUP = 0x60,
    INI_MODEM_PHONE = 0x61,
    INI_TEAMS = 0x62,
    INI_RED_TEAM_NAME = 0x63,
    INI_GREEN_TEAM_NAME = 0x64,
    INI_BLUE_TEAM_NAME = 0x65,
    INI_GRAY_TEAM_NAME = 0x66,
    INI_RED_TEAM_PLAYER = 0x67,
    INI_GREEN_TEAM_PLAYER = 0x68,
    INI_BLUE_TEAM_PLAYER = 0x69,
    INI_GRAY_TEAM_PLAYER = 0x6A,
    INI_RED_TEAM_CLAN = 0x6B,
    INI_GREEN_TEAM_CLAN = 0x6C,
    INI_BLUE_TEAM_CLAN = 0x6D,
    INI_GRAY_TEAM_CLAN = 0x6E,
    INI_DIGITAL = 0x6F,
    INI_DEVICE_NAME = 0x70,
    INI_DEVICE_IRQ = 0x71,
    INI_DEVICE_DMA = 0x72,
    INI_DEVICE_PORT = 0x73,
    INI_DEVICE_ID = 0x74,
    INI_CHANNELS_REVERSED = 0x75
};

typedef enum GAME_INI_e GAME_INI;

struct Ini_descriptor {
    unsigned char unknown_1;
    unsigned char flags;
    unsigned char unknown_2[2];
    char ini_file_path[PATH_MAX];
    char *buffer;
    unsigned int file_size;
    unsigned int buffer_size;
    char *current_address;
    char *section_start_address;
    char *key_start_address;
    char *value_start_address;
    char *next_value_address;
    char *param_start_address;
};

typedef struct Ini_descriptor Ini_descriptor;

int inifile_save_to_file_and_free_buffer(Ini_descriptor *const pini);
int inifile_init_ini_object_from_ini_file(Ini_descriptor *const pini, const char *const inifile_path);
unsigned int inifile_hex_to_dec(const char *const hex);
int inifile_ini_seek_section(Ini_descriptor *const pini, const char *const ini_section_name);
int inifile_ini_seek_param(Ini_descriptor *const pini, const char *const ini_param_name);
int inifile_ini_get_numeric_value(Ini_descriptor *const pini, const char *const ini_param_name, int *const value);
int inifile_ini_set_numeric_value(Ini_descriptor *const pini, const int value);
int inifile_ini_set_string_value(Ini_descriptor *const pini, char *value);
int inifile_ini_get_string(Ini_descriptor *const pini, char *const buffer, const unsigned int buffer_size,
                           const int mode);
int inifile_save_to_file(Ini_descriptor *const pini);
void inifile_load_from_resource(Ini_descriptor *const pini, const unsigned short resource_id);
int inifile_ini_process_string_value(Ini_descriptor *const pini, char *const buffer, const unsigned int buffer_size);

int ini_get_setting(GAME_INI index);
int ini_set_setting(GAME_INI index, int value);

#endif /* INI_H */
