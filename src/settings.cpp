/* Copyright (c) 2025 M.A.X. Port Team
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

#include "settings.hpp"

#include <SDL3/SDL.h>

#include <fstream>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include "resource_manager.hpp"
#include "utf8.hpp"

using json = nlohmann::ordered_json;
using validator = nlohmann::json_schema::json_validator;

static const std::vector<std::pair<std::string, Settings::SettingDefinition>> setting_defaults = {
    {"debug", {0, "DEBUG"}},
    {"all_visible", {0, "DEBUG"}},
    {"disable_fire", {0, "DEBUG"}},
    {"quick_build", {0, "DEBUG"}},
    {"real_time", {0, "DEBUG"}},
    {"exclude_range", {3, "DEBUG"}},
    {"proximity_range", {14, "DEBUG"}},
    {"log_file_debug", {0, "DEBUG"}},
    {"raw_normal_low", {0, "DEBUG"}},
    {"raw_normal_high", {5, "DEBUG"}},
    {"raw_concentrate_low", {13, "DEBUG"}},
    {"raw_concentrate_high", {16, "DEBUG"}},
    {"raw_concentrate_seperation", {23, "DEBUG"}},
    {"raw_concentrate_diffusion", {6, "DEBUG"}},
    {"fuel_normal_low", {2, "DEBUG"}},
    {"fuel_normal_high", {3, "DEBUG"}},
    {"fuel_concentrate_low", {12, "DEBUG"}},
    {"fuel_concentrate_high", {16, "DEBUG"}},
    {"fuel_concentrate_seperation", {24, "DEBUG"}},
    {"fuel_concentrate_diffusion", {5, "DEBUG"}},
    {"gold_normal_low", {0, "DEBUG"}},
    {"gold_normal_high", {0, "DEBUG"}},
    {"gold_concentrate_low", {10, "DEBUG"}},
    {"gold_concentrate_high", {16, "DEBUG"}},
    {"gold_concentrate_seperation", {32, "DEBUG"}},
    {"gold_concentrate_diffusion", {5, "DEBUG"}},
    {"mixed_resource_seperation", {40, "DEBUG"}},
    {"min_resources", {9, "DEBUG"}},
    {"max_resources", {20, "DEBUG"}},
    {"attack_factor", {16, "DEBUG"}},
    {"shots_factor", {16, "DEBUG"}},
    {"range_factor", {8, "DEBUG"}},
    {"armor_factor", {32, "DEBUG"}},
    {"hits_factor", {32, "DEBUG"}},
    {"speed_factor", {16, "DEBUG"}},
    {"scan_factor", {8, "DEBUG"}},
    {"cost_factor", {8, "DEBUG"}},
    {"steal_percent", {30, "DEBUG"}},
    {"disable_percent", {50, "DEBUG"}},
    {"max_percent", {90, "DEBUG"}},
    {"step_percent", {10, "DEBUG"}},
    {"repair_turns", {10, "DEBUG"}},
    {"alien_seperation", {60, "DEBUG"}},
    {"alien_unit_value", {0, "DEBUG"}},
    {"red_strategy", {std::string("random"), "DEBUG"}},
    {"green_strategy", {std::string("random"), "DEBUG"}},
    {"blue_strategy", {std::string("random"), "DEBUG"}},
    {"gray_strategy", {std::string("random"), "DEBUG"}},
    {"player_name", {std::string("Player 1"), "SETUP"}},
    {"player_clan", {0, "SETUP"}},
    {"intro_movie", {1, "SETUP"}},
    {"ipx_socket", {0x51E7, "SETUP"}},
    {"music_level", {100, "SETUP"}},
    {"fx_sound_level", {100, "SETUP"}},
    {"voice_level", {100, "SETUP"}},
    {"disable_music", {0, "SETUP"}},
    {"disable_fx", {0, "SETUP"}},
    {"disable_voice", {0, "SETUP"}},
    {"disable_movie", {0, "SETUP"}},
    {"auto_save", {1, "SETUP"}},
    {"game_file_number", {1, "SETUP"}},
    {"demo_turns", {5, "SETUP"}},
    {"enhanced_graphics", {1, "SETUP"}},
    {"last_campaign", {1, "SETUP"}},
    {"language", {std::string("en-US"), "SETUP"}},
    {"voiceover", {std::string("en-US"), "SETUP"}},
    {"game_data", {std::string("."), "SETUP"}},
    {"cheating_computer", {3, "SETUP"}},
    {"mouse_wheel_sensitivity", {3, "SETUP"}},
    {"disable_subtitles", {0, "SETUP"}},
    {"subtitle_text_color", {static_cast<int32_t>(0xFFFFFFFF), "SETUP"}},
    {"subtitle_outline_color", {static_cast<int32_t>(0xFF000000), "SETUP"}},
    {"subtitle_bg_color", {0x00000000, "SETUP"}},
    {"world", {0, "OPTIONS"}},
    {"timer", {180, "OPTIONS"}},
    {"endturn", {45, "OPTIONS"}},
    {"start_gold", {150, "OPTIONS"}},
    {"play_mode", {1, "OPTIONS"}},
    {"victory_type", {0, "OPTIONS"}},
    {"victory_limit", {50, "OPTIONS"}},
    {"opponent", {1, "OPTIONS"}},
    {"raw_resource", {1, "OPTIONS"}},
    {"fuel_resource", {1, "OPTIONS"}},
    {"gold_resource", {1, "OPTIONS"}},
    {"alien_derelicts", {0, "OPTIONS"}},
    {"effects", {1, "PREFERENCES"}},
    {"click_scroll", {1, "PREFERENCES"}},
    {"quick_scroll", {16, "PREFERENCES"}},
    {"fast_movement", {1, "PREFERENCES"}},
    {"follow_unit", {0, "PREFERENCES"}},
    {"auto_select", {0, "PREFERENCES"}},
    {"enemy_halt", {1, "PREFERENCES"}},
    {"red_team_name", {std::string("Player 1"), "TEAMS"}},
    {"green_team_name", {std::string("Player 2"), "TEAMS"}},
    {"blue_team_name", {std::string("Player 3"), "TEAMS"}},
    {"gray_team_name", {std::string("Player 4"), "TEAMS"}},
    {"alien_team_name", {std::string(" "), "TEAMS"}},
    {"red_team_player", {1, "TEAMS"}},
    {"green_team_player", {2, "TEAMS"}},
    {"blue_team_player", {0, "TEAMS"}},
    {"gray_team_player", {0, "TEAMS"}},
    {"alien_team_player", {0, "TEAMS"}},
    {"red_team_clan", {0, "TEAMS"}},
    {"green_team_clan", {0, "TEAMS"}},
    {"blue_team_clan", {0, "TEAMS"}},
    {"gray_team_clan", {0, "TEAMS"}},
    {"alien_team_clan", {0, "TEAMS"}},
    {"audio_device_name", {std::string("None"), "AUDIO_SETTINGS"}},
    {"audio_device_id", {-1, "AUDIO_SETTINGS"}},
    {"channels_reversed", {0, "AUDIO_SETTINGS"}},
    {"display_index", {0, "GRAPHICS_SETTINGS"}},
    {"screen_mode", {2, "GRAPHICS_SETTINGS"}},
    {"disable_ar_correction", {0, "GRAPHICS_SETTINGS"}},
    {"scale_quality", {1, "GRAPHICS_SETTINGS"}},
    {"window_width", {640, "GRAPHICS_SETTINGS"}},
    {"window_height", {480, "GRAPHICS_SETTINGS"}},
    {"dialog_center_mode", {0, "GRAPHICS_SETTINGS"}},
    {"transport", {std::string("udp_default"), "NETWORK_SETTINGS"}},
    {"host_address", {std::string("127.0.0.1"), "NETWORK_SETTINGS"}},
    {"host_port", {31554, "NETWORK_SETTINGS"}},
};

Settings::Settings()
    : m_loaded_settings(std::make_unique<std::unordered_map<std::string, SettingValue>>()),
      m_setting_definitions(
          std::make_unique<std::vector<std::pair<std::string, SettingDefinition>>>(setting_defaults)) {}

Settings::~Settings() {}

[[nodiscard]] std::string Settings::LoadSchema() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_SCHES);
    uint8_t* file_base = ResourceManager_ReadResource(SC_SCHES);

    SDL_assert(file_size && file_base);

    for (size_t i = 0; i < file_size; ++i) {
        file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
    }

    std::string schema(reinterpret_cast<const char*>(file_base), file_size);

    delete[] file_base;

    return schema;
}

[[nodiscard]] bool Settings::LoadScript(const std::string& script) {
    bool result{false};

    m_loaded_settings->clear();

    try {
        json jscript = json::parse(script);

        try {
            validator validator;
            json jschema = json::parse(LoadSchema());

            validator.set_root_schema(jschema);
            validator.validate(jscript);

        } catch (const std::exception& e) {
            SDL_Log("Warning: Settings validation failed: %s\n", e.what());
        }

        const auto& settings_object = jscript["settings"];

        for (const auto& [section_name, section_data] : settings_object.items()) {
            if (section_data.is_object()) {
                for (const auto& [key, value] : section_data.items()) {
                    auto it = std::find_if(m_setting_definitions->begin(), m_setting_definitions->end(),
                                           [&key](const auto& pair) { return pair.first == key; });

                    if (it != m_setting_definitions->end()) {
                        if (std::holds_alternative<int32_t>(it->second.default_value) && value.is_number_integer()) {
                            (*m_loaded_settings)[key] = value.get<int32_t>();

                        } else if (std::holds_alternative<std::string>(it->second.default_value) && value.is_string()) {
                            (*m_loaded_settings)[key] = value.get<std::string>();
                        }
                    }
                }
            }
        }

        result = true;

        ValidateNames();

    } catch (const json::parse_error& e) {
        SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());

    } catch (const std::exception& e) {
        SDL_Log("\n%s\n", (std::string("JSON error: ") + e.what()).c_str());
    }

    return result;
}

[[nodiscard]] bool Settings::Load() {
    m_filepath = ResourceManager_FilePathGamePref / "settings.json";

    std::ifstream fs(m_filepath.string().c_str());
    bool result{false};

    if (fs) {
        std::string contents((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
        result = LoadScript(contents);

    } else {
        SDL_Log("Settings file not found at %s, using defaults\n", m_filepath.string().c_str());

        result = true;
    }

    return result;
}

[[nodiscard]] bool Settings::Save() {
    std::ifstream infile(m_filepath.string().c_str());
    json jscript;
    bool file_existed = false;

    if (infile) {
        std::string contents((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());

        infile.close();

        try {
            jscript = json::parse(contents);
            file_existed = true;

        } catch (const json::parse_error&) {
            return false;
        }

    } else {
        jscript = json::object();
        jscript["$schema"] = "http://json-schema.org/draft-07/schema#";
        jscript["settings"] = json::object();
    }

    if (!jscript.contains("settings") || !jscript["settings"].is_object()) {
        return false;
    }

    auto& settings_obj = jscript["settings"];

    if (file_existed) {
        // Update existing settings only
        for (auto& [section_name, section_data] : settings_obj.items()) {
            if (section_data.is_object()) {
                for (auto& [key, value] : section_data.items()) {
                    auto it = m_loaded_settings->find(key);

                    if (it != m_loaded_settings->end()) {
                        if (std::holds_alternative<int32_t>(it->second)) {
                            value = std::get<int32_t>(it->second);

                        } else if (std::holds_alternative<std::string>(it->second)) {
                            value = std::get<std::string>(it->second);
                        }
                    }
                }
            }
        }

    } else {
        // Dump all defaults organized by section
        for (const auto& [key, def] : *m_setting_definitions) {
            if (!settings_obj.contains(def.section)) {
                settings_obj[def.section] = json::object();
            }

            auto it = m_loaded_settings->find(key);

            if (it != m_loaded_settings->end()) {
                // Use loaded value if available
                if (std::holds_alternative<int32_t>(it->second)) {
                    settings_obj[def.section][key] = std::get<int32_t>(it->second);

                } else if (std::holds_alternative<std::string>(it->second)) {
                    settings_obj[def.section][key] = std::get<std::string>(it->second);
                }

            } else {
                // Use default value
                if (std::holds_alternative<int32_t>(def.default_value)) {
                    settings_obj[def.section][key] = std::get<int32_t>(def.default_value);

                } else if (std::holds_alternative<std::string>(def.default_value)) {
                    settings_obj[def.section][key] = std::get<std::string>(def.default_value);
                }
            }
        }
    }

    // Ensure the directory exists before writing the file
    std::error_code ec;
    std::filesystem::create_directories(m_filepath.parent_path(), ec);

    std::ofstream outfile(m_filepath.string());

    if (outfile) {
        outfile << jscript.dump(1, '\t') << std::endl;

        return true;

    } else {
        return false;
    }
}

[[nodiscard]] int32_t Settings::GetNumericValue(const std::string& key, int32_t default_value) const {
    auto it = m_loaded_settings->find(key);

    if (it != m_loaded_settings->end() && std::holds_alternative<int32_t>(it->second)) {
        return std::get<int32_t>(it->second);
    }

    auto default_it = std::find_if(m_setting_definitions->begin(), m_setting_definitions->end(),
                                   [&key](const auto& pair) { return pair.first == key; });

    if (default_it != m_setting_definitions->end() &&
        std::holds_alternative<int32_t>(default_it->second.default_value)) {
        return std::get<int32_t>(default_it->second.default_value);
    }

    return default_value;
}

[[nodiscard]] std::string Settings::GetStringValue(const std::string& key, const std::string& default_value) const {
    auto it = m_loaded_settings->find(key);

    if (it != m_loaded_settings->end() && std::holds_alternative<std::string>(it->second)) {
        return std::get<std::string>(it->second);
    }

    auto default_it = std::find_if(m_setting_definitions->begin(), m_setting_definitions->end(),
                                   [&key](const auto& pair) { return pair.first == key; });

    if (default_it != m_setting_definitions->end() &&
        std::holds_alternative<std::string>(default_it->second.default_value)) {
        return std::get<std::string>(default_it->second.default_value);
    }

    return default_value;
}

void Settings::SetNumericValue(const std::string& key, int32_t value) {
    auto it = std::find_if(m_setting_definitions->begin(), m_setting_definitions->end(),
                           [&key](const auto& pair) { return pair.first == key; });

    if (it != m_setting_definitions->end() && std::holds_alternative<int32_t>(it->second.default_value)) {
        (*m_loaded_settings)[key] = value;
    }
}

void Settings::SetStringValue(const std::string& key, const std::string& value) {
    auto it = std::find_if(m_setting_definitions->begin(), m_setting_definitions->end(),
                           [&key](const auto& pair) { return pair.first == key; });

    if (it != m_setting_definitions->end() && std::holds_alternative<std::string>(it->second.default_value)) {
        (*m_loaded_settings)[key] = value;
    }
}

void Settings::ValidateNames() {
    static const struct {
        const char* key;
        uint32_t default_name_key;
    } name_settings[] = {
        {"player_name", 0x0cb7},    {"red_team_name", 0xa18e},  {"green_team_name", 0xf3cd},
        {"blue_team_name", 0x2b0e}, {"gray_team_name", 0x7e3b},
    };

    for (const auto& setting : name_settings) {
        auto it = m_loaded_settings->find(setting.key);

        if (it != m_loaded_settings->end() && std::holds_alternative<std::string>(it->second)) {
            const auto& value = std::get<std::string>(it->second);

            if (utf8_is_blank(value)) {
                const auto& default_name = ResourceManager_GetLanguageEntry(setting.default_name_key);

                (*m_loaded_settings)[setting.key] = default_name;
            }
        }
    }
}
