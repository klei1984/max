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

#include "inifile.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "gnw.h"
#include "localization.hpp"
#include "resource_manager.hpp"
#include "units_manager.hpp"

enum IniKeyValueType_e {
    INI_SECTION = 0x1,
    INI_NUMERIC = 0x2,
    INI_STRING = 0x4,
};

struct IniKey {
    const char *const name;
    const char *const value;
    const IniKeyValueType_e type;
};

int32_t ini_setting_victory_type;
int32_t ini_setting_victory_limit;

static const IniKey ini_keys_table[] = {
    {"DEBUG", nullptr, INI_SECTION},
    {"debug", "0", INI_NUMERIC},
    {"all_visible", "0", INI_NUMERIC},
    {"disable_fire", "0", INI_NUMERIC},
    {"quick_build", "0", INI_NUMERIC},
    {"real_time", "0", INI_NUMERIC},
    {"exclude_range", "3", INI_NUMERIC},
    {"proximity_range", "14", INI_NUMERIC},
    {"log_file_debug", "0", INI_NUMERIC},
    {"raw_normal_low", "0", INI_NUMERIC},
    {"raw_normal_high", "5", INI_NUMERIC},
    {"raw_concentrate_low", "13", INI_NUMERIC},
    {"raw_concentrate_high", "16", INI_NUMERIC},
    {"raw_concentrate_seperation", "23", INI_NUMERIC},
    {"raw_concentrate_diffusion", "6", INI_NUMERIC},
    {"fuel_normal_low", "2", INI_NUMERIC},
    {"fuel_normal_high", "3", INI_NUMERIC},
    {"fuel_concentrate_low", "12", INI_NUMERIC},
    {"fuel_concentrate_high", "16", INI_NUMERIC},
    {"fuel_concentrate_seperation", "24", INI_NUMERIC},
    {"fuel_concentrate_diffusion", "5", INI_NUMERIC},
    {"gold_normal_low", "0", INI_NUMERIC},
    {"gold_normal_high", "0", INI_NUMERIC},
    {"gold_concentrate_low", "10", INI_NUMERIC},
    {"gold_concentrate_high", "16", INI_NUMERIC},
    {"gold_concentrate_seperation", "32", INI_NUMERIC},
    {"gold_concentrate_diffusion", "5", INI_NUMERIC},
    {"mixed_resource_seperation", "40", INI_NUMERIC},
    {"min_resources", "9", INI_NUMERIC},
    {"max_resources", "20", INI_NUMERIC},
    {"attack_factor", "16", INI_NUMERIC},
    {"shots_factor", "16", INI_NUMERIC},
    {"range_factor", "8", INI_NUMERIC},
    {"armor_factor", "32", INI_NUMERIC},
    {"hits_factor", "32", INI_NUMERIC},
    {"speed_factor", "16", INI_NUMERIC},
    {"scan_factor", "8", INI_NUMERIC},
    {"cost_factor", "8", INI_NUMERIC},
    {"steal_percent", "30", INI_NUMERIC},
    {"disable_percent", "50", INI_NUMERIC},
    {"max_percent", "90", INI_NUMERIC},
    {"step_percent", "10", INI_NUMERIC},
    {"repair_turns", "10", INI_NUMERIC},
    {"alien_seperation", "60", INI_NUMERIC},
    {"alien_unit_value", "0", INI_NUMERIC},
    {"red_strategy", "random", INI_STRING},
    {"green_strategy", "random", INI_STRING},
    {"blue_strategy", "random", INI_STRING},
    {"gray_strategy", "random", INI_STRING},

    {"SETUP", nullptr, INI_SECTION},
    {"player_name", "Player 1", INI_STRING},
    {"player_clan", "0", INI_NUMERIC},
    {"intro_movie", "0", INI_NUMERIC},
    {"ipx_socket", "0x51E7", INI_NUMERIC},
    {"music_level", "100", INI_NUMERIC},
    {"fx_sound_level", "100", INI_NUMERIC},
    {"voice_level", "100", INI_NUMERIC},
    {"disable_music", "0", INI_NUMERIC},
    {"disable_fx", "0", INI_NUMERIC},
    {"disable_voice", "0", INI_NUMERIC},
    {"auto_save", "1", INI_NUMERIC},
    {"game_file_number", "1", INI_NUMERIC},
    {"demo_turns", "0", INI_NUMERIC},
    {"enhanced_graphics", "1", INI_NUMERIC},
    {"last_campaign", "1", INI_NUMERIC},
    {"movie_play", "0", INI_NUMERIC},
    {"alt_movie_res", "0", INI_NUMERIC},
    {"language", "english", INI_STRING},
    {"game_data", "", INI_STRING},
    {"cheating_computer", "3", INI_NUMERIC},

    {"OPTIONS", nullptr, INI_SECTION},
    {"world", "0", INI_NUMERIC},
    {"timer", "180", INI_NUMERIC},
    {"endturn", "45", INI_NUMERIC},
    {"start_gold", "150", INI_NUMERIC},
    {"play_mode", "1", INI_NUMERIC},
    {"victory_type", "0", INI_NUMERIC},
    {"victory_limit", "50", INI_NUMERIC},
    {"opponent", "1", INI_NUMERIC},
    {"raw_resource", "1", INI_NUMERIC},
    {"fuel_resource", "1", INI_NUMERIC},
    {"gold_resource", "1", INI_NUMERIC},
    {"alien_derelicts", "0", INI_NUMERIC},

    {"PREFERENCES", nullptr, INI_SECTION},
    {"effects", "1", INI_NUMERIC},
    {"click_scroll", "1", INI_NUMERIC},
    {"quick_scroll", "16", INI_NUMERIC},
    {"fast_movement", "1", INI_NUMERIC},
    {"follow_unit", "0", INI_NUMERIC},
    {"auto_select", "0", INI_NUMERIC},
    {"enemy_halt", "1", INI_NUMERIC},

    {"TEAMS", nullptr, INI_SECTION},
    {"red_team_name", "Player 1", INI_STRING},
    {"green_team_name", "Player 2", INI_STRING},
    {"blue_team_name", "Player 3", INI_STRING},
    {"gray_team_name", "Player 4", INI_STRING},
    {"alien_team_name", " ", INI_STRING},
    {"red_team_player", "1", INI_NUMERIC},
    {"green_team_player", "2", INI_NUMERIC},
    {"blue_team_player", "0", INI_NUMERIC},
    {"gray_team_player", "0", INI_NUMERIC},
    {"alien_team_player", "0", INI_NUMERIC},
    {"red_team_clan", "0", INI_NUMERIC},
    {"green_team_clan", "0", INI_NUMERIC},
    {"blue_team_clan", "0", INI_NUMERIC},
    {"gray_team_clan", "0", INI_NUMERIC},
    {"alien_team_clan", "0", INI_NUMERIC},

    {"AUDIO_SETTINGS", nullptr, INI_SECTION},
    {"audio_device_name", "None", INI_STRING},
    {"audio_device_id", "-1", INI_NUMERIC},
    {"channels_reversed", "0", INI_NUMERIC},

    {"GRAPHICS_SETTINGS", nullptr, INI_SECTION},
    {"display_index", "0", INI_NUMERIC},
    {"screen_mode", "2", INI_NUMERIC},
    {"disable_ar_correction", "0", INI_NUMERIC},
    {"scale_quality", "1", INI_NUMERIC},
    {"window_width", "640", INI_NUMERIC},
    {"window_height", "480", INI_NUMERIC},
    {"dialog_center_mode", "0", INI_NUMERIC},

    {"NETWORK_SETTINGS", nullptr, INI_SECTION},
    {"transport", "udp_default", INI_STRING},
    {"host_address", "127.0.0.1", INI_STRING},
    {"host_port", "31554", INI_NUMERIC},
};

static const int32_t ini_keys_table_size = sizeof(ini_keys_table) / sizeof(struct IniKey);

static const char *const clan_ini_section_name_lut[] = {"Clan ?", "Clan A", "Clan B", "Clan C", "Clan D",
                                                        "Clan E", "Clan F", "Clan G", "Clan H"};

static const int32_t ini_clans_table_size = sizeof(clan_ini_section_name_lut) / sizeof(char *);

static_assert(ini_keys_table_size == INI_END_DELIMITER,
              "INI enumerator list and configuration ini parameters table size do not match");

static_assert(ini_clans_table_size == 9, "M.A.X. v1.04 has 9 clans");

IniSettings ini_config;
IniClans ini_clans;

IniSettings::IniSettings() {
    ini.buffer = nullptr;

    items = new (std::nothrow) uint32_t[ini_keys_table_size];

    memset(items, 0, sizeof(int32_t) * ini_keys_table_size);
}

IniSettings::~IniSettings() {
    Destroy();

    delete[] items;
}

void IniSettings::Destroy() { inifile_save_to_file_and_free_buffer(&ini); }

void IniSettings::Init() {
    int32_t v1;
    int32_t value;
    char format_string[50];
    FILE *fp;
    int32_t index;

    auto filepath{ResourceManager_FilePathGamePref / "settings.ini"};

    fp = fopen(filepath.string().c_str(), "rt");

    if (!fp) {
        SDL_Log("%s", _(f3e3));
        fp = fopen(filepath.string().c_str(), "wt");

        if (!fp) {
            SDL_Log("%s", _(db0c));
            ResourceManager_ExitGame(EXIT_CODE_CANNOT_FIND_MAX_INI);
        }

        for (index = 0; index < ini_keys_table_size; ++index) {
            if (ini_keys_table[index].type & INI_SECTION) {
                if (index) {
                    strcpy(format_string, "\n[");
                } else {
                    strcpy(format_string, "[");
                }

                strcat(format_string, ini_keys_table[index].name);
                strcat(format_string, "]");
            } else {
                strcpy(format_string, ini_keys_table[index].name);
                strcat(format_string, "=");
                strcat(format_string, ini_keys_table[index].value);
            }

            strcat(format_string, "\n");
            fprintf(fp, "%s", format_string);
        }
    }

    fclose(fp);

    if (!inifile_init_ini_object_from_ini_file(&ini, filepath.string().c_str())) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_FIND_MAX_INI);
    }

    for (index = 0; index < ini_keys_table_size; index++) {
        if (ini_keys_table[index].type & INI_SECTION) {
            inifile_ini_seek_section(&ini, ini_keys_table[index].name);
        } else if (ini_keys_table[index].type & INI_NUMERIC) {
            if (inifile_ini_get_numeric_value(&ini, ini_keys_table[index].name, &value)) {
                SetNumericValue(static_cast<IniParameter>(index), value);
            } else {
                if (!strncmp(ini_keys_table[index].value, "0x", 2)) {
                    v1 = inifile_hex_to_dec(ini_keys_table[index].value + 2);
                } else {
                    v1 = strtol(ini_keys_table[index].value, nullptr, 10);
                }

                SetNumericValue(static_cast<IniParameter>(index), v1);
            }
        }
    }
}

void IniSettings::Save() { inifile_save_to_file(&ini); }

const char *IniSettings::SeekToSection(IniParameter param) {
    int32_t index = param;

    do {
        if (ini_keys_table[index].type & INI_SECTION) {
            break;
        }

        --index;
    } while (index);

    return ini_keys_table[index].name;
}

int32_t IniSettings::SetNumericValue(IniParameter param, int32_t value) {
    uint32_t old_value;

    if (param >= ini_keys_table_size) {
        return 0;
    }

    old_value = items[param];
    items[param] = value;

    if (inifile_ini_seek_section(&ini, IniSettings::SeekToSection(param)) &&
        inifile_ini_seek_param(&ini, ini_keys_table[param].name)) {
        inifile_ini_set_numeric_value(&ini, value);
    }

    return old_value;
}

int32_t IniSettings::GetNumericValue(IniParameter param) {
    int32_t value;

    if (param >= ini_keys_table_size) {
        value = 0;
    } else {
        value = items[param];
    }

    return value;
}

int32_t IniSettings::SetStringValue(IniParameter param, const char *value) {
    int32_t result;

    if (inifile_ini_seek_section(&ini, IniSettings::SeekToSection(param)) &&
        inifile_ini_seek_param(&ini, ini_keys_table[param].name)) {
        result = inifile_ini_set_string_value(&ini, value);
    } else {
        result = 0;
    }
    return result;
}

int32_t IniSettings::GetStringValue(IniParameter param, char *buffer, int32_t buffer_size) {
    int32_t result;

    if (inifile_ini_seek_section(&ini, IniSettings::SeekToSection(param)) &&
        inifile_ini_seek_param(&ini, ini_keys_table[param].name)) {
        result = inifile_ini_get_string(&ini, buffer, buffer_size, 1);
    } else {
        result = 0;
    }

    return result;
}

void IniSettings::SaveSection(SmartFileWriter &file, IniParameter section) {
    char buffer[30];
    int32_t victory_type;
    int32_t backup_victory_limit;
    int32_t index = section;

    backup_victory_limit = ini_get_setting(INI_VICTORY_LIMIT);
    victory_type = ini_get_setting(INI_VICTORY_TYPE);

    ini_set_setting(INI_VICTORY_LIMIT, ini_setting_victory_limit);
    ini_set_setting(INI_VICTORY_TYPE, ini_setting_victory_type);

    while (!(ini_keys_table[++index].type & INI_SECTION) && index < ini_keys_table_size) {
        if (ini_keys_table[index].type & INI_NUMERIC) {
            int32_t ini_param;

            ini_param = GetNumericValue(static_cast<IniParameter>(index));

            file.Write(ini_param);
        } else {
            GetStringValue(static_cast<IniParameter>(index), buffer, sizeof(buffer));
            file.Write(buffer, sizeof(buffer));
        }
    }

    ini_set_setting(INI_VICTORY_LIMIT, backup_victory_limit);
    ini_set_setting(INI_VICTORY_TYPE, victory_type);
}

void IniSettings::LoadSection(SmartFileReader &file, IniParameter section, char mode) {
    char buffer[30];
    int32_t value;
    int32_t victory_type;
    int32_t backup_victory_limit;
    int32_t index = section;

    backup_victory_limit = ini_get_setting(INI_VICTORY_LIMIT);
    victory_type = ini_get_setting(INI_VICTORY_TYPE);

    while (!(ini_keys_table[++index].type & INI_SECTION) && index < ini_keys_table_size) {
        if (ini_keys_table[index].type & INI_NUMERIC) {
            file.Read(value);
            if (mode) {
                SetNumericValue(static_cast<IniParameter>(index), value);
            }
        } else {
            file.Read(buffer, sizeof(buffer));
            if (mode) {
                SetStringValue(static_cast<IniParameter>(index), buffer);
            }
        }
    }

    if (mode) {
        if (section == INI_OPTIONS) {
            ini_setting_victory_type = ini_get_setting(INI_VICTORY_TYPE);
            ini_setting_victory_limit = ini_get_setting(INI_VICTORY_LIMIT);

            ini_set_setting(INI_VICTORY_LIMIT, backup_victory_limit);
            ini_set_setting(INI_VICTORY_TYPE, victory_type);
        }
    }
}

int32_t ini_get_setting(IniParameter index) { return ini_config.GetNumericValue(static_cast<IniParameter>(index)); }

int32_t ini_set_setting(IniParameter param, int32_t value) { return ini_config.SetNumericValue(param, value); }

IniClans::IniClans() { ini.buffer = nullptr; }

IniClans::~IniClans() { inifile_save_to_file_and_free_buffer(&ini); }

void IniClans::Init() { inifile_load_from_resource(&ini, CLANATRB); }

int32_t IniClans::SeekUnit(int32_t clan, int32_t unit) {
    return inifile_ini_seek_section(&ini, clan_ini_section_name_lut[clan]) &&
           inifile_ini_seek_param(&ini, UnitsManager_BaseUnits[unit].singular_name);
}

int32_t IniClans::GetNextUnitUpgrade(int16_t *attrib_id, int16_t *value) {
    const char *cstr;

    if (!inifile_ini_process_string_value(&ini, buffer, sizeof(buffer))) {
        return 0;
    }

    cstr = buffer;

    if (!strlen(cstr)) {
        return 0;
    }

    *value = strtol(cstr, nullptr, 10);

    while (*cstr != ' ') {
        ++cstr;
    }

    ++cstr;

    for (*attrib_id = 0; *attrib_id < ATTRIB_COUNT; ++(*attrib_id)) {
        int32_t result;

        switch (*attrib_id) {
            case ATTRIB_ATTACK: {
                result = stricmp(_(fca3), cstr);
            } break;

            case ATTRIB_ROUNDS: {
                result = stricmp(_(206c), cstr);
            } break;

            case ATTRIB_RANGE: {
                result = stricmp(_(2269), cstr);
            } break;

            case ATTRIB_ARMOR: {
                result = stricmp(_(d81e), cstr);
            } break;

            case ATTRIB_HITS: {
                result = stricmp(_(62f5), cstr);
            } break;

            case ATTRIB_SPEED: {
                result = stricmp(_(bbcc), cstr);
            } break;

            case ATTRIB_SCAN: {
                result = stricmp(_(59ad), cstr);
            } break;

            case ATTRIB_TURNS: {
                result = stricmp(_(6976), cstr);
            } break;

            case ATTRIB_AMMO: {
                result = stricmp(_(24d8), cstr);
            } break;

            case ATTRIB_MOVE_AND_FIRE: {
                result = stricmp(_(4027), cstr);
            } break;

            case ATTRIB_FUEL: {
                result = stricmp("fuel", cstr);
            } break;

            case ATTRIB_STORAGE: {
                result = stricmp(_(49a2), cstr);
            } break;

            case ATTRIB_ATTACK_RADIUS: {
                result = stricmp(_(4a91), cstr);
            } break;

            case ATTRIB_AGENT_ADJUST: {
                result = stricmp(_(e9d8), cstr);
            } break;
        }

        if (!result) {
            break;
        }
    }

    SDL_assert(*attrib_id < ATTRIB_COUNT);

    return 1;
}

void IniClans::GetStringValue(char *buffer, int32_t buffer_size) {
    inifile_ini_get_string(&ini, buffer, buffer_size, 0);
}

int32_t IniClans::GetClanGold(int32_t clan) {
    int32_t result;

    if (inifile_ini_seek_section(&ini, clan_ini_section_name_lut[clan]) && inifile_ini_seek_param(&ini, "Gold") &&
        inifile_ini_process_string_value(&ini, buffer, sizeof(buffer))) {
        result = strtol(buffer, nullptr, 10);
    } else {
        result = 0;
    }

    return result;
}

void IniClans::GetClanText(int32_t clan, char *buffer, int32_t buffer_size) {
    if (!inifile_ini_seek_section(&ini, clan_ini_section_name_lut[clan]) || !inifile_ini_seek_param(&ini, "Text") ||
        !inifile_ini_process_string_value(&ini, buffer, buffer_size)) {
        strcpy(buffer, _(7f53));
    }
}

void IniClans::GetClanName(int32_t clan, char *buffer, int32_t buffer_size) {
    if (inifile_ini_seek_section(&ini, clan_ini_section_name_lut[clan])) {
        if (inifile_ini_seek_param(&ini, "Name")) {
            inifile_ini_process_string_value(&ini, buffer, buffer_size);
        }
    }
}

IniSoundVolumes::IniSoundVolumes() { ini.buffer = nullptr; }

IniSoundVolumes::~IniSoundVolumes() { inifile_save_to_file_and_free_buffer(&ini); }

void IniSoundVolumes::Init() { inifile_load_from_resource(&ini, SOUNDVOL); }

int32_t IniSoundVolumes::GetUnitVolume(ResourceID id) {
    int32_t value;

    if (!this->ini.buffer) {
        return 0x7FFF;
    }

    if (inifile_ini_seek_section(&ini, "Unit Volumes")) {
        if (ResourceManager_GetResourceFileID(id) != -1) {
            if (inifile_ini_get_numeric_value(&ini, ResourceManager_GetResourceID(id), &value)) {
                return 0x7FFF * value / 100u;
            }
        }
    }

    return 0x7FFF;
}
