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
    ini_debug = 0x1,
    ini_all_visible = 0x2,
    ini_disable_fire = 0x3,
    ini_quick_build = 0x4,
    ini_real_time = 0x5,
    ini_exclude_range = 0x6,
    ini_proximity_range = 0x7,
    ini_log_file_debug = 0x8,
    ini_raw_normal_low = 0x9,
    ini_raw_normal_high = 0xA,
    ini_raw_concentrate_low = 0xB,
    ini_raw_concentrate_high = 0xC,
    ini_raw_concentrate_seperation = 0xD,
    ini_raw_concentrate_diffusion = 0xE,
    ini_fuel_normal_low = 0xF,
    ini_fuel_normal_high = 0x10,
    ini_fuel_concentrate_low = 0x11,
    ini_fuel_concentrate_high = 0x12,
    ini_fuel_concentrate_seperation = 0x13,
    ini_fuel_concentrate_diffusion = 0x14,
    ini_gold_normal_low = 0x15,
    ini_gold_normal_high = 0x16,
    ini_gold_concentrate_low = 0x17,
    ini_gold_concentrate_high = 0x18,
    ini_gold_concentrate_seperation = 0x19,
    ini_gold_concentrate_diffusion = 0x1A,
    ini_mixed_resource_seperation = 0x1B,
    ini_min_resources = 0x1C,
    ini_max_resources = 0x1D,
    ini_attack_factor = 0x1E,
    ini_shots_factor = 0x1F,
    ini_range_factor = 0x20,
    ini_armor_factor = 0x21,
    ini_hits_factor = 0x22,
    ini_speed_factor = 0x23,
    ini_scan_factor = 0x24,
    ini_steal_percent = 0x25,
    ini_disable_percent = 0x26,
    ini_max_percent = 0x27,
    ini_step_percent = 0x28,
    ini_repair_turns = 0x29,
    ini_unknown = 0x2A,
    ini_alien_seperation = 0x2B,
    ini_alien_unit_value = 0x2C,
    ini_red_strategy = 0x2D,
    ini_green_strategy = 0x2E,
    ini_blue_strategy = 0x2F,
    ini_gray_strategy = 0x30,
    ini_SETUP = 0x31,
    ini_player_name = 0x32,
    ini_player_clan = 0x33,
    ini_intro_movie = 0x34,
    ini_ipx_socket = 0x35,
    ini_music_level = 0x36,
    ini_fx_sound_level = 0x37,
    ini_voice_level = 0x38,
    ini_disable_music = 0x39,
    ini_disable_fx = 0x3A,
    ini_disable_voice = 0x3B,
    ini_auto_save = 0x3C,
    ini_game_file_number = 0x3D,
    ini_game_file_type = 0x3E,
    ini_demo_turns = 0x3F,
    ini_enhanced_graphics = 0x40,
    ini_last_campaign = 0x41,
    ini_movie_play = 0x42,
    ini_alt_movie_res = 0x43,
    ini_OPTIONS = 0x44,
    ini_world = 0x45,
    ini_timer = 0x46,
    ini_endturn = 0x47,
    ini_start_gold = 0x48,
    ini_play_mode = 0x49,
    ini_victory_type = 0x4A,
    ini_victory_limit = 0x4B,
    ini_opponent = 0x4C,
    ini_raw_resource = 0x4D,
    ini_fuel_resource = 0x4E,
    ini_gold_resource = 0x4F,
    ini_alien_derelicts = 0x50,
    ini_PREFERENCES = 0x51,
    ini_effects = 0x52,
    ini_click_scroll = 0x53,
    ini_quick_scroll = 0x54,
    ini_fast_movement = 0x55,
    ini_follow_unit = 0x56,
    ini_auto_select = 0x57,
    ini_enemy_halt = 0x58,
    ini_MODEM = 0x59,
    ini_modem_port = 0x5A,
    ini_modem_baud = 0x5B,
    ini_modem_irq = 0x5C,
    ini_modem_init = 0x5D,
    ini_modem_answer = 0x5E,
    ini_modem_call = 0x5F,
    ini_modem_hangup = 0x60,
    ini_modem_phone = 0x61,
    ini_TEAMS = 0x62,
    ini_red_team_name = 0x63,
    ini_green_team_name = 0x64,
    ini_blue_team_name = 0x65,
    ini_gray_team_name = 0x66,
    ini_red_team_player = 0x67,
    ini_green_team_player = 0x68,
    ini_blue_team_player = 0x69,
    ini_gray_team_player = 0x6A,
    ini_red_team_clan = 0x6B,
    ini_green_team_clan = 0x6C,
    ini_blue_team_clan = 0x6D,
    ini_gray_team_clan = 0x6E,
    ini_DIGITAL = 0x6F,
    ini_Device_Name = 0x70,
    ini_Device_IRQ = 0x71,
    ini_Device_DMA = 0x72,
    ini_Device_Port = 0x73,
    ini_Device_ID = 0x74,
    ini_Channels_Reversed = 0x75
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
