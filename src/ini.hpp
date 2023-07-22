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

#ifndef INI_HPP
#define INI_HPP

#include <climits>

#include "enums.hpp"

enum IniParameter {
    /// DEBUG SECTION
    INI_INVALID_ID,
    INI_DEBUG,
    INI_ALL_VISIBLE,
    INI_DISABLE_FIRE,
    INI_QUICK_BUILD,
    INI_REAL_TIME,
    INI_EXCLUDE_RANGE,
    INI_PROXIMITY_RANGE,
    INI_LOG_FILE_DEBUG,
    INI_RAW_NORMAL_LOW,
    INI_RAW_NORMAL_HIGH,
    INI_RAW_CONCENTRATE_LOW,
    INI_RAW_CONCENTRATE_HIGH,
    INI_RAW_CONCENTRATE_SEPERATION,
    INI_RAW_CONCENTRATE_DIFFUSION,
    INI_FUEL_NORMAL_LOW,
    INI_FUEL_NORMAL_HIGH,
    INI_FUEL_CONCENTRATE_LOW,
    INI_FUEL_CONCENTRATE_HIGH,
    INI_FUEL_CONCENTRATE_SEPERATION,
    INI_FUEL_CONCENTRATE_DIFFUSION,
    INI_GOLD_NORMAL_LOW,
    INI_GOLD_NORMAL_HIGH,
    INI_GOLD_CONCENTRATE_LOW,
    INI_GOLD_CONCENTRATE_HIGH,
    INI_GOLD_CONCENTRATE_SEPERATION,
    INI_GOLD_CONCENTRATE_DIFFUSION,
    INI_MIXED_RESOURCE_SEPERATION,
    INI_MIN_RESOURCES,
    INI_MAX_RESOURCES,
    INI_ATTACK_FACTOR,
    INI_SHOTS_FACTOR,
    INI_RANGE_FACTOR,
    INI_ARMOR_FACTOR,
    INI_HITS_FACTOR,
    INI_SPEED_FACTOR,
    INI_SCAN_FACTOR,
    INI_COST_FACTOR,
    INI_STEAL_PERCENT,
    INI_DISABLE_PERCENT,
    INI_MAX_PERCENT,
    INI_STEP_PERCENT,
    INI_REPAIR_TURNS,
    INI_ALIEN_SEPERATION,
    INI_ALIEN_UNIT_VALUE,
    INI_RED_STRATEGY,
    INI_GREEN_STRATEGY,
    INI_BLUE_STRATEGY,
    INI_GRAY_STRATEGY,

    /// SETUP SECTION
    INI_SETUP,
    INI_PLAYER_NAME,
    INI_PLAYER_CLAN,
    INI_INTRO_MOVIE,
    INI_IPX_SOCKET,
    INI_MUSIC_LEVEL,
    INI_FX_SOUND_LEVEL,
    INI_VOICE_LEVEL,
    INI_DISABLE_MUSIC,
    INI_DISABLE_FX,
    INI_DISABLE_VOICE,
    INI_AUTO_SAVE,
    INI_GAME_FILE_NUMBER,
    INI_GAME_FILE_TYPE,
    INI_DEMO_TURNS,
    INI_ENHANCED_GRAPHICS,
    INI_LAST_CAMPAIGN,
    INI_MOVIE_PLAY,
    INI_ALT_MOVIE_RES,
    INI_LANGUAGE,

    /// OPTIONS SECTION
    INI_OPTIONS,
    INI_WORLD,
    INI_TIMER,
    INI_ENDTURN,
    INI_START_GOLD,
    INI_PLAY_MODE,
    INI_VICTORY_TYPE,
    INI_VICTORY_LIMIT,
    INI_OPPONENT,
    INI_RAW_RESOURCE,
    INI_FUEL_RESOURCE,
    INI_GOLD_RESOURCE,
    INI_ALIEN_DERELICTS,

    /// PREFERENCES SECTION
    INI_PREFERENCES,
    INI_EFFECTS,
    INI_CLICK_SCROLL,
    INI_QUICK_SCROLL,
    INI_FAST_MOVEMENT,
    INI_FOLLOW_UNIT,
    INI_AUTO_SELECT,
    INI_ENEMY_HALT,

    /// TEAMS SECTION
    INI_TEAMS,
    INI_RED_TEAM_NAME,
    INI_GREEN_TEAM_NAME,
    INI_BLUE_TEAM_NAME,
    INI_GRAY_TEAM_NAME,
    INI_RED_TEAM_PLAYER,
    INI_GREEN_TEAM_PLAYER,
    INI_BLUE_TEAM_PLAYER,
    INI_GRAY_TEAM_PLAYER,
    INI_RED_TEAM_CLAN,
    INI_GREEN_TEAM_CLAN,
    INI_BLUE_TEAM_CLAN,
    INI_GRAY_TEAM_CLAN,

    /// AUDIO SETTINGS SECTION
    INI_AUDIO_SETTINGS,
    INI_AUDIO_DEVICE_NAME,
    INI_AUDIO_DEVICE_ID,
    INI_CHANNELS_REVERSED,

    /// GRAPHICS SETTINGS SECTION
    INI_GRAPHICS_SETTINGS,
    INI_DISPLAY_INDEX,
    INI_SCREEN_MODE,
    INI_DISABLE_AR_CORRECTION,
    INI_SCALE_QUALITY,
    INI_WINDOW_WIDTH,
    INI_WINDOW_HEIGHT,
    INI_DIALOG_CENTER_MODE,

    /// NETWORK SETTINGS SECTION
    INI_NETWORK_SETTINGS,
    INI_NETWORK_TRANSPORT,
    INI_NETWORK_HOST_ADDRESS,
    INI_NETWORK_HOST_PORT,

    INI_END_DELIMITER
};

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

int inifile_save_to_file_and_free_buffer(Ini_descriptor *const pini, bool free_only = false);
int inifile_init_ini_object_from_ini_file(Ini_descriptor *const pini, const char *const inifile_path);
unsigned int inifile_hex_to_dec(const char *const hex);
int inifile_ini_seek_section(Ini_descriptor *const pini, const char *const ini_section_name);
int inifile_ini_seek_param(Ini_descriptor *const pini, const char *const ini_param_name);
int inifile_ini_get_numeric_value(Ini_descriptor *const pini, const char *const ini_param_name, int *const value);
int inifile_ini_set_numeric_value(Ini_descriptor *const pini, const int value);
int inifile_ini_set_string_value(Ini_descriptor *const pini, const char *value);
int inifile_ini_get_string(Ini_descriptor *const pini, char *const buffer, const unsigned int buffer_size,
                           const int mode, bool skip_leading_white_space = true);
int inifile_save_to_file(Ini_descriptor *const pini);
void inifile_load_from_resource(Ini_descriptor *const pini, ResourceID resource_id);
int inifile_ini_process_string_value(Ini_descriptor *const pini, char *const buffer, const unsigned int buffer_size);

int ini_get_setting(IniParameter index);
int ini_set_setting(IniParameter index, int value);

#endif /* INI_HPP */
