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

#include "saveload.hpp"

#include <format>

#include "ai.hpp"
#include "game_manager.hpp"
#include "hash.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "message_manager.hpp"
#include "missionmanager.hpp"
#include "missionregistry.hpp"
#include "remote.hpp"
#include "smartfile.hpp"
#include "units_manager.hpp"

/// \todo
extern const char *menu_team_names[];
extern uint16_t SaveLoadMenu_TurnTimer;
extern uint8_t SaveLoadMenu_GameState;

static void SaveLoad_TeamClearUnitList(SmartList<UnitInfo> &units, uint16_t team);
static std::filesystem::path SaveLoad_GetFilePath(const MissionCategory mission_category, const int32_t save_slot,
                                                  std::string *file_name = nullptr);
static bool SaveLoad_GetSaveFileInfoV70(SmartFileReader &file, struct SaveFileInfo &save_file_info,
                                        const bool load_options);
static bool SaveLoad_GetSaveFileInfoV71(SmartFileReader &file, struct SaveFileInfo &save_file_info,
                                        const bool load_options);
static bool SaveLoad_LoadFormatV70(SmartFileReader &file, const MissionCategory mission_category, bool is_remote_game,
                                   bool ini_load_mode);
static bool SaveLoad_LoadFormatV71(SmartFileReader &file, const MissionCategory mission_category, bool is_remote_game,
                                   bool ini_load_mode);
static std::string SaveLoad_TranslateMissionIndexToHashKey(const uint32_t save_file_type, const uint32_t index);
static uint32_t SaveLoad_TranslateHashKeyToWorldIndex(const std::string hash);
static std::string SaveLoad_TranslateWorldIndexToHashKey(const uint32_t index);
MissionCategory SaveLoad_TranslateSaveFileCategory(const uint32_t save_file_type);

void SaveLoad_TeamClearUnitList(SmartList<UnitInfo> &units, uint16_t team) {
    for (auto it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team) {
            UnitsManager_DestroyUnit(&*it);
        }
    }
}

std::filesystem::path SaveLoad_GetFilePath(const MissionCategory mission_category, const int32_t save_slot,
                                           std::string *file_name) {
    std::filesystem::path filepath;
    const auto filename = SaveLoad_GetSaveFileName(mission_category, save_slot);

    if (file_name) {
        *file_name = filename;
    }

    if (mission_category == MISSION_CATEGORY_CUSTOM || mission_category == MISSION_CATEGORY_HOT_SEAT ||
        mission_category == MISSION_CATEGORY_MULTI) {
        filepath = (ResourceManager_FilePathGamePref / filename.c_str()).lexically_normal();

    } else {
        filepath = (ResourceManager_FilePathGameData / filename.c_str()).lexically_normal();
    }

    return filepath;
}

bool SaveLoad_GetSaveFileInfoV70(SmartFileReader &file, struct SaveFileInfo &save_file_info, const bool load_options) {
    std::string mission_hash;
    uint32_t save_file_category;
    uint32_t rng_seed;
    uint16_t version;
    uint16_t mission_index;
    char save_name[30];
    char team_names[4][30];
    uint8_t save_game_type;
    uint8_t world_index;
    uint8_t team_type[5];
    uint8_t team_clan[5];

    bool result;

    try {
        file.Read(version);

        if (version != static_cast<uint32_t>(SmartFileFormat::V70)) {
            throw std::runtime_error(std::format("Invalid version: {}", version));
        }

        file.Read(save_game_type);

        save_file_category = SaveLoad_TranslateSaveFileCategory(save_game_type);

        if (save_file_category >= MISSION_CATEGORY_COUNT) {
            throw std::runtime_error(std::format("Invalid save file category: {}", save_file_category));
        }

        file.Read(save_name);
        file.Read(world_index);
        file.Read(mission_index);
        file.Read(team_names);
        file.Read(team_type);
        file.Read(team_clan);
        file.Read(rng_seed);

        if (load_options) {
            uint8_t play_mode;
            uint8_t opponent;
            uint16_t turn_timer_time;
            uint16_t endturn_time;

            file.Read(play_mode);
            file.Read(opponent);
            file.Read(turn_timer_time);
            file.Read(endturn_time);

            ini_config.LoadSection(file, INI_OPTIONS, true);
        }

        save_file_info.version = version;
        save_file_info.save_file_category = save_file_category;
        save_file_info.save_name = save_name;
        save_file_info.mission = SaveLoad_TranslateMissionIndexToHashKey(save_game_type, mission_index - 1);
        save_file_info.world = SaveLoad_TranslateWorldIndexToHashKey(world_index);

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 2; ++team) {
            save_file_info.team_names[team] = team_names[team];
        }

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            save_file_info.team_type[team] = team_type[team];
            save_file_info.team_clan[team] = team_clan[team];
        }

        save_file_info.random_seed = rng_seed;

        result = true;

    } catch (const std::exception &e) {
        result = false;

        SDL_Log("\n%s\n", (std::format("Save Format V71 error: {}.", e.what()).c_str()));
    };

    return result;
}

bool SaveLoad_GetSaveFileInfoV71(SmartFileReader &file, struct SaveFileInfo &save_file_info, const bool load_options) {
    bool result;

    uint32_t version;
    uint32_t save_file_category;
    std::vector<uint8_t> binary_script;
    std::string save_name;
    std::string world;
    std::string team_names[PLAYER_TEAM_MAX];
    uint32_t team_type[PLAYER_TEAM_MAX];
    uint32_t team_clan[PLAYER_TEAM_MAX];
    uint32_t random_seed;

    try {
        file.Read(version);

        if (version != static_cast<uint32_t>(SmartFileFormat::V71)) {
            throw std::runtime_error(std::format("Invalid version: {}", version));
        }

        file.Read(save_file_category);

        if (save_file_category >= MISSION_CATEGORY_COUNT) {
            throw std::runtime_error(std::format("Invalid save file category: {}", save_file_category));
        }

        file.Read(binary_script);
        file.Read(save_name);
        file.Read(world);

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            file.Read(team_names[team]);
        }

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            file.Read(team_type[team]);
        }

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            file.Read(team_clan[team]);
        }

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            uint32_t difficulty_level;

            file.Read(difficulty_level);
        }

        file.Read(random_seed);

        if (load_options) {
            uint32_t turn_timer_time;
            uint32_t endturn_time;
            uint32_t game_play_mode;

            file.Read(turn_timer_time);
            file.Read(endturn_time);
            file.Read(game_play_mode);

            ini_config.LoadSection(file, INI_OPTIONS, true);
        }

        save_file_info.version = version;
        save_file_info.script = binary_script;
        save_file_info.save_file_category = save_file_category;
        save_file_info.save_name = save_name;
        save_file_info.world = world;

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            save_file_info.team_names[team] = team_names[team];
            save_file_info.team_type[team] = team_type[team];
            save_file_info.team_clan[team] = team_clan[team];
        }

        save_file_info.random_seed = random_seed;

        result = true;

    } catch (const std::exception &e) {
        result = false;

        SDL_Log("\n%s\n", (std::format("Save Format V71 error: {}.", e.what()).c_str()));
    };

    return result;
}

bool SaveLoad_GetSaveFileInfo(const MissionCategory mission_category, const int32_t save_slot,
                              struct SaveFileInfo &save_file_info, const bool load_options) {
    auto filepath = SaveLoad_GetFilePath(mission_category, save_slot);

    return SaveLoad_GetSaveFileInfo(filepath, save_file_info, load_options);
}

bool SaveLoad_GetSaveFileInfo(const std::filesystem::path &filepath, struct SaveFileInfo &save_file_info,
                              const bool load_options) {
    SmartFileReader file;
    bool result;

    if (file.Open(filepath.string())) {
        switch (file.GetFormat()) {
            case SmartFileFormat::V70: {
                result = SaveLoad_GetSaveFileInfoV70(file, save_file_info, load_options);
            } break;

            case SmartFileFormat::V71: {
                result = SaveLoad_GetSaveFileInfoV71(file, save_file_info, load_options);
            } break;

            default: {
                result = false;
            } break;
        }

        file.Close();

        save_file_info.file_name = filepath.filename().string();
        save_file_info.file_path = filepath;

    } else {
        result = false;
    }

    return result;
}

[[nodiscard]] bool SaveLoad_IsSaveFileFormatSupported(const uint32_t format_version) {
    bool result;

    switch (format_version) {
        case static_cast<uint32_t>(SmartFileFormat::V70): {
            result = true;
        } break;

        case static_cast<uint32_t>(SmartFileFormat::V71): {
            result = true;
        } break;

        default: {
            result = false;
        } break;
    }

    return result;
}

bool SaveLoad_Save(const std::filesystem::path &filepath, const char *const save_name, const uint32_t rng_seed) {
    bool result{false};
    const auto mission = ResourceManager_GetMissionManager().get()->GetMission();
    const auto mission_category = mission->GetCategory();
    SmartFileWriter file;
    const uint32_t map_cell_count{static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

    if (file.Open(filepath.string())) {
        bool error{false};

        // save file format version
        {
            uint32_t version = static_cast<uint32_t>(SmartFileFormat::LATEST);

            SDL_assert(version >= static_cast<uint32_t>(SmartFileFormat::V70));

            error |= !file.Write(version);
        }

        // save file category
        {
            uint32_t save_file_category = mission_category;

            error |= !file.Write(save_file_category);
        }

        // mission
        {
            auto script = mission->GetBinaryScript();

            error |= !file.Write(*script);
        }

        // save file title
        {
            std::string local_save_name = save_name;

            error |= !file.Write(local_save_name);
        }

        // world hash
        {
            uint32_t world_index = ini_get_setting(INI_WORLD);
            std::string world = SaveLoad_TranslateWorldIndexToHashKey(world_index);

            file.Write(world);
        }

        // team names
        {
            for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
                std::string ini_team_name;

                if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                    char team_name[30];

                    if (!ini_config.GetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), team_name,
                                                   sizeof(team_name))) {
                        team_name[0] = '\0';
                    }

                    if (0 == strlen(team_name)) {
                        SDL_utf8strlcpy(team_name, menu_team_names[team], sizeof(team_name));
                    }

                    ini_team_name = team_name;
                }

                error |= !file.Write(ini_team_name);
            }
        }

        // team types
        {
            for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
                uint32_t team_type;

                if ((mission_category == MISSION_CATEGORY_TRAINING || mission_category == MISSION_CATEGORY_SCENARIO ||
                     mission_category == MISSION_CATEGORY_CAMPAIGN) &&
                    team != PLAYER_TEAM_RED && UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER) {
                    SDL_assert(0);  /// \todo Fix broken missions
                    UnitsManager_TeamInfo[team].team_type = TEAM_TYPE_COMPUTER;
                }

                if (mission_category == MISSION_CATEGORY_DEMO &&
                    UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                    SDL_assert(0);  /// \todo Fix broken missions
                    UnitsManager_TeamInfo[team].team_type = TEAM_TYPE_COMPUTER;
                }

                team_type = UnitsManager_TeamInfo[team].team_type;

                error |= !file.Write(team_type);
            }
        }

        // team clans
        {
            for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
                uint32_t team_clan = UnitsManager_TeamInfo[team].team_clan;

                error |= !file.Write(team_clan);
            }
        }

        // team difficulty levels
        {
            for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
                uint32_t difficulty_level = ini_get_setting(INI_OPPONENT);

                switch (difficulty_level) {
                    case OPPONENT_TYPE_CLUELESS:
                    case OPPONENT_TYPE_APPRENTICE:
                    case OPPONENT_TYPE_AVERAGE:
                    case OPPONENT_TYPE_EXPERT:
                    case OPPONENT_TYPE_MASTER:
                    case OPPONENT_TYPE_GOD: {
                    } break;

                    default: {
                        difficulty_level = OPPONENT_TYPE_EXPERT;
                    } break;
                }

                error |= !file.Write(difficulty_level);
            }
        }

        // random generator seed
        {
            uint32_t random_seed = rng_seed;

            error |= !file.Write(random_seed);
        }

        // turn timer time setting
        {
            uint32_t turn_timer_time = ini_get_setting(INI_TIMER);

            error |= !file.Write(turn_timer_time);
        }

        // end-turn time setting
        {
            uint32_t endturn_time = ini_get_setting(INI_ENDTURN);

            error |= !file.Write(endturn_time);
        }

        // game play mode setting
        {
            uint32_t game_play_mode = ini_get_setting(INI_PLAY_MODE);

            switch (game_play_mode) {
                case PLAY_MODE_TURN_BASED:
                case PLAY_MODE_SIMULTANEOUS_MOVES: {
                } break;

                default: {
                    game_play_mode = PLAY_MODE_TURN_BASED;
                } break;
            }

            error |= !file.Write(game_play_mode);
        }

        // INI sections
        {
            ini_config.SaveSection(file, INI_OPTIONS);
            ini_config.SaveSection(file, INI_PREFERENCES);
        }

        // surface map (pass table)
        {
            SDL_assert(ResourceManager_MapSurfaceMap != nullptr);

            error |= !file.Write(ResourceManager_MapSurfaceMap, map_cell_count * sizeof(uint8_t));
        }

        // cargo map (survey map)
        {
            SDL_assert(ResourceManager_CargoMap != nullptr);

            error |= !file.Write(ResourceManager_CargoMap, map_cell_count * sizeof(uint16_t));
        }

        // team info
        {
            for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
                const CTInfo *const team_info = &UnitsManager_TeamInfo[team];
                uint16_t unit_id;

                file.Write(team_info->markers);
                file.Write(team_info->team_type);
                file.Write(team_info->finished_turn);
                file.Write(team_info->team_clan);
                file.Write(team_info->research_topics);
                file.Write(team_info->team_points);
                file.Write(team_info->number_of_objects_created);
                file.Write(team_info->unit_counters);
                file.Write(team_info->screen_locations);
                file.Write(team_info->score_graph, sizeof(team_info->score_graph));

                if (team_info->selected_unit != nullptr) {
                    unit_id = team_info->selected_unit->GetId();
                } else {
                    unit_id = 0xFFFF;
                }

                file.Write(unit_id);
                file.Write(team_info->zoom_level);
                file.Write(team_info->camera_position.x);
                file.Write(team_info->camera_position.y);
                file.Write(team_info->display_button_range);
                file.Write(team_info->display_button_scan);
                file.Write(team_info->display_button_status);
                file.Write(team_info->display_button_colors);
                file.Write(team_info->display_button_hits);
                file.Write(team_info->display_button_ammo);
                file.Write(team_info->display_button_names);
                file.Write(team_info->display_button_minimap_2x);
                file.Write(team_info->display_button_minimap_tnt);
                file.Write(team_info->display_button_grid);
                file.Write(team_info->display_button_survey);
                file.Write(team_info->stats_factories_built);
                file.Write(team_info->stats_mines_built);
                file.Write(team_info->stats_buildings_built);
                file.Write(team_info->stats_units_built);
                file.Write(team_info->casualties);
                file.Write(team_info->stats_gold_spent_on_upgrades);
            }
        }

        // active team index
        {
            uint32_t active_turn_team = GameManager_ActiveTurnTeam;

            SDL_assert(active_turn_team < PLAYER_TEAM_MAX);

            error |= !file.Write(active_turn_team);
        }

        // player team
        {
            uint32_t player_team = GameManager_PlayerTeam;

            SDL_assert(player_team < PLAYER_TEAM_MAX);

            error |= !file.Write(player_team);
        }

        // turn counter value
        {
            uint32_t turn_counter = GameManager_TurnCounter;

            error |= !file.Write(turn_counter);
        }

        // turn timer value
        {
            uint32_t turn_timer = GameManager_TurnTimerValue;

            error |= !file.Write(turn_timer);
        }

        // game state machine state
        {
            uint32_t game_state = GameManager_GameState;

            error |= !file.Write(game_state);
        }

        // team units
        {
            ResourceManager_TeamUnitsRed.FileSave(file);
            ResourceManager_TeamUnitsGreen.FileSave(file);
            ResourceManager_TeamUnitsBlue.FileSave(file);
            ResourceManager_TeamUnitsGray.FileSave(file);
        }

        // units
        {
            SmartList_UnitInfo_FileSave(UnitsManager_GroundCoverUnits, file);
            SmartList_UnitInfo_FileSave(UnitsManager_MobileLandSeaUnits, file);
            SmartList_UnitInfo_FileSave(UnitsManager_StationaryUnits, file);
            SmartList_UnitInfo_FileSave(UnitsManager_MobileAirUnits, file);
            SmartList_UnitInfo_FileSave(UnitsManager_ParticleUnits, file);
        }

        // unit hash-map
        {
            Hash_UnitHash.FileSave(file);
        }

        // map hash-map
        {
            Hash_MapHash.FileSave(file);
        }

        // heat maps
        {
            for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
                if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                    SDL_assert(UnitsManager_TeamInfo[team].heat_map_complete != nullptr);
                    SDL_assert(UnitsManager_TeamInfo[team].heat_map_stealth_sea != nullptr);
                    SDL_assert(UnitsManager_TeamInfo[team].heat_map_stealth_land != nullptr);

                    file.Write(UnitsManager_TeamInfo[team].heat_map_complete, map_cell_count);
                    file.Write(UnitsManager_TeamInfo[team].heat_map_stealth_sea, map_cell_count);
                    file.Write(UnitsManager_TeamInfo[team].heat_map_stealth_land, map_cell_count);
                }
            }
        }

        // message logs
        {
            MessageManager_SaveMessageLogs(file);
        }

        // computer ai
        {
            Ai_FileSave(file);
        }

        file.Close();

        result = (error == false);
    }

    return result;
}

bool SaveLoad_LoadFormatV70(SmartFileReader &file, const MissionCategory mission_category, bool is_remote_game,
                            bool ini_load_mode) {
    bool result;
    bool save_load_flag;
    uint16_t version;
    uint8_t save_game_type;
    char save_name[30];
    uint8_t world_index;
    uint16_t mission_index;
    char team_names[4][30];
    uint8_t team_type[5];
    uint8_t team_clan[5];
    uint32_t rng_seed;
    int8_t opponent;
    uint16_t turn_timer_time;
    uint16_t endturn_time;
    int8_t play_mode;

    file.Read(version);
    file.Read(save_game_type);
    file.Read(save_name);
    file.Read(world_index);
    file.Read(mission_index);
    file.Read(team_names);
    file.Read(team_type);
    file.Read(team_clan);
    file.Read(rng_seed);
    file.Read(opponent);
    file.Read(turn_timer_time);
    file.Read(endturn_time);
    file.Read(play_mode);

    int32_t backup_start_gold;
    int32_t backup_raw_resource;
    int32_t backup_fuel_resource;
    int32_t backup_gold_resource;
    int32_t backup_alien_derelicts;
    uint16_t selected_unit_ids[PLAYER_TEAM_MAX - 1];
    uint16_t game_state;

    if (!is_remote_game) {
        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), team_names[team]);
        }
    }

    ResourceManager_InitInGameAssets(world_index);

    opponent = ini_get_setting(INI_OPPONENT);
    turn_timer_time = ini_get_setting(INI_TIMER);
    endturn_time = ini_get_setting(INI_ENDTURN);
    play_mode = ini_get_setting(INI_PLAY_MODE);
    backup_start_gold = ini_get_setting(INI_START_GOLD);
    backup_raw_resource = ini_get_setting(INI_RAW_RESOURCE);
    backup_fuel_resource = ini_get_setting(INI_FUEL_RESOURCE);
    backup_gold_resource = ini_get_setting(INI_GOLD_RESOURCE);
    backup_alien_derelicts = ini_get_setting(INI_ALIEN_DERELICTS);

    ini_config.LoadSection(file, INI_OPTIONS, ini_load_mode);

    if (mission_category == MISSION_CATEGORY_TRAINING || mission_category == MISSION_CATEGORY_CAMPAIGN ||
        mission_category == MISSION_CATEGORY_SCENARIO || mission_category == MISSION_CATEGORY_MULTI_PLAYER_SCENARIO) {
        ini_set_setting(INI_OPPONENT, opponent);
        ini_set_setting(INI_TIMER, turn_timer_time);
        ini_set_setting(INI_ENDTURN, endturn_time);
        ini_set_setting(INI_PLAY_MODE, play_mode);
        ini_set_setting(INI_START_GOLD, backup_start_gold);
        ini_set_setting(INI_RAW_RESOURCE, backup_raw_resource);
        ini_set_setting(INI_FUEL_RESOURCE, backup_fuel_resource);
        ini_set_setting(INI_GOLD_RESOURCE, backup_gold_resource);
        ini_set_setting(INI_ALIEN_DERELICTS, backup_alien_derelicts);
    }

    GameManager_PlayMode = ini_get_setting(INI_PLAY_MODE);

    const uint32_t map_cell_count{static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

    file.Read(ResourceManager_MapSurfaceMap, map_cell_count * sizeof(uint8_t));
    file.Read(ResourceManager_CargoMap, map_cell_count * sizeof(uint16_t));

    ResourceManager_InitTeamInfo();

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        CTInfo *team_info;

        team_info = &UnitsManager_TeamInfo[team];

        file.Read(team_info->markers);
        file.Read(team_info->team_type);
        file.Read(team_info->finished_turn);
        file.Read(team_info->team_clan);
        file.Read(team_info->research_topics);
        file.Read(team_info->team_points);
        file.Read(team_info->number_of_objects_created);
        file.Read(team_info->unit_counters);
        file.Read(team_info->screen_locations);
        file.Read(team_info->score_graph, sizeof(team_info->score_graph));
        file.Read(selected_unit_ids[team]);
        file.Read(team_info->zoom_level);
        file.Read(team_info->camera_position.x);
        file.Read(team_info->camera_position.y);
        file.Read(team_info->display_button_range);
        file.Read(team_info->display_button_scan);
        file.Read(team_info->display_button_status);
        file.Read(team_info->display_button_colors);
        file.Read(team_info->display_button_hits);
        file.Read(team_info->display_button_ammo);
        file.Read(team_info->display_button_names);
        file.Read(team_info->display_button_minimap_2x);
        file.Read(team_info->display_button_minimap_tnt);
        file.Read(team_info->display_button_grid);
        file.Read(team_info->display_button_survey);
        file.Read(team_info->stats_factories_built);
        file.Read(team_info->stats_mines_built);
        file.Read(team_info->stats_buildings_built);
        file.Read(team_info->stats_units_built);
        file.Read(team_info->casualties);
        file.Read(team_info->stats_gold_spent_on_upgrades);
    }

    file.Read(GameManager_ActiveTurnTeam);
    file.Read(GameManager_PlayerTeam);
    file.Read(GameManager_TurnCounter);
    file.Read(game_state);

    SaveLoadMenu_GameState = game_state;

    file.Read(SaveLoadMenu_TurnTimer);

    ini_config.LoadSection(file, INI_PREFERENCES, true);

    ResourceManager_TeamUnitsRed.FileLoad(file);
    ResourceManager_TeamUnitsGreen.FileLoad(file);
    ResourceManager_TeamUnitsBlue.FileLoad(file);
    ResourceManager_TeamUnitsGray.FileLoad(file);

    UnitsManager_InitPopupMenus();

    SmartList_UnitInfo_FileLoad(UnitsManager_GroundCoverUnits, file);
    SmartList_UnitInfo_FileLoad(UnitsManager_MobileLandSeaUnits, file);
    SmartList_UnitInfo_FileLoad(UnitsManager_StationaryUnits, file);
    SmartList_UnitInfo_FileLoad(UnitsManager_MobileAirUnits, file);
    SmartList_UnitInfo_FileLoad(UnitsManager_ParticleUnits, file);

    Hash_UnitHash.FileLoad(file);
    Hash_MapHash.FileLoad(file);

    UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_units = &ResourceManager_TeamUnitsRed;
    UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_units = &ResourceManager_TeamUnitsGreen;
    UnitsManager_TeamInfo[PLAYER_TEAM_BLUE].team_units = &ResourceManager_TeamUnitsBlue;
    UnitsManager_TeamInfo[PLAYER_TEAM_GRAY].team_units = &ResourceManager_TeamUnitsGray;

    save_load_flag = true;

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        CTInfo *team_info;
        char team_name[30];

        team_info = &UnitsManager_TeamInfo[team];

        if (team_info->team_type != TEAM_TYPE_NONE) {
            if (is_remote_game) {
                team_info->team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team));

                if (team_info->team_type == TEAM_TYPE_PLAYER) {
                    GameManager_PlayerTeam = team;
                    ini_config.GetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), team_name,
                                              sizeof(team_name));
                    ini_config.SetStringValue(INI_PLAYER_NAME, team_name);
                }

            } else if (mission_category == MISSION_CATEGORY_MULTI_PLAYER_SCENARIO) {
                team_info->team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team));

                if (save_load_flag && team_info->team_type == TEAM_TYPE_PLAYER) {
                    GameManager_PlayerTeam = team;
                    save_load_flag = false;
                }

            } else {
                switch (team_info->team_type) {
                    case TEAM_TYPE_PLAYER: {
                        if (mission_category != MISSION_CATEGORY_HOT_SEAT) {
                            ini_config.GetStringValue(INI_PLAYER_NAME, team_name, sizeof(team_name));
                            ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), team_name);
                        }
                    } break;

                    case TEAM_TYPE_COMPUTER: {
                        ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team),
                                                  menu_team_names[team]);
                    } break;

                    case TEAM_TYPE_REMOTE: {
                        team_info->team_type = TEAM_TYPE_PLAYER;
                    } break;
                }
            }

            ResourceManager_InitHeatMaps(team);

            if (team_info->team_type != TEAM_TYPE_NONE) {
                file.Read(team_info->heat_map_complete, map_cell_count);
                file.Read(team_info->heat_map_stealth_sea, map_cell_count);
                file.Read(team_info->heat_map_stealth_land, map_cell_count);

            } else {
                char *temp_buffer;

                temp_buffer = new (std::nothrow) char[map_cell_count];

                file.Read(temp_buffer, map_cell_count);
                file.Read(temp_buffer, map_cell_count);
                file.Read(temp_buffer, map_cell_count);

                delete[] temp_buffer;

                SaveLoad_TeamClearUnitList(UnitsManager_GroundCoverUnits, team);
                SaveLoad_TeamClearUnitList(UnitsManager_MobileLandSeaUnits, team);
                SaveLoad_TeamClearUnitList(UnitsManager_StationaryUnits, team);
                SaveLoad_TeamClearUnitList(UnitsManager_MobileAirUnits, team);
                SaveLoad_TeamClearUnitList(UnitsManager_ParticleUnits, team);

                if (team == UnitsManager_DelayedReactionsTeam) {
                    ++UnitsManager_DelayedReactionsTeam;
                }
            }

        } else {
            ResourceManager_InitHeatMaps(team);
        }

        ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team), team_info->team_type);
    }

    ResourceManager_InitHeatMaps(PLAYER_TEAM_ALIEN);

    MessageManager_LoadMessageLogs(file);

    if (mission_category == MISSION_CATEGORY_MULTI_PLAYER_SCENARIO) {
        Ai_Init();

    } else {
        Ai_FileLoad(file);
    }

    file.Close();

    GameManager_SelectedUnit = nullptr;

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (selected_unit_ids[team] != 0xFFFF) {
            UnitsManager_TeamInfo[team].selected_unit = Hash_UnitHash[selected_unit_ids[team]];

        } else {
            UnitsManager_TeamInfo[team].selected_unit = nullptr;
        }
    }

    result = true;

    return result;
}

bool SaveLoad_LoadFormatV71(SmartFileReader &file, const MissionCategory mission_category, bool is_remote_game,
                            bool ini_load_mode) {
    bool result;
    bool save_load_flag;
    uint32_t version;
    uint32_t save_file_category;
    std::vector<uint8_t> binary_script;
    std::string save_name;
    std::string world;
    std::string team_names[PLAYER_TEAM_MAX];
    uint32_t team_types[PLAYER_TEAM_MAX];
    uint32_t team_clans[PLAYER_TEAM_MAX];
    uint32_t difficulty_levels[PLAYER_TEAM_MAX];
    uint32_t random_seed;
    uint32_t turn_timer_time;
    uint32_t endturn_time;
    uint32_t game_play_mode;

    file.Read(version);

    SDL_assert(version == static_cast<uint32_t>(SmartFileFormat::V71));

    file.Read(save_file_category);
    file.Read(binary_script);
    file.Read(save_name);
    file.Read(world);

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        file.Read(team_names[team]);
    }

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        file.Read(team_types[team]);
    }

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        file.Read(team_clans[team]);
    }

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        file.Read(difficulty_levels[team]);
    }

    file.Read(random_seed);
    file.Read(turn_timer_time);
    file.Read(endturn_time);
    file.Read(game_play_mode);

    int32_t opponent;
    int32_t play_mode;
    int32_t backup_start_gold;
    int32_t backup_raw_resource;
    int32_t backup_fuel_resource;
    int32_t backup_gold_resource;
    int32_t backup_alien_derelicts;
    uint16_t selected_unit_ids[PLAYER_TEAM_MAX - 1];

    if (!is_remote_game) {
        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), team_names[team].c_str());
        }
    }

    ResourceManager_InitInGameAssets(SaveLoad_TranslateHashKeyToWorldIndex(world));

    opponent = ini_get_setting(INI_OPPONENT);
    turn_timer_time = ini_get_setting(INI_TIMER);
    endturn_time = ini_get_setting(INI_ENDTURN);
    play_mode = ini_get_setting(INI_PLAY_MODE);
    backup_start_gold = ini_get_setting(INI_START_GOLD);
    backup_raw_resource = ini_get_setting(INI_RAW_RESOURCE);
    backup_fuel_resource = ini_get_setting(INI_FUEL_RESOURCE);
    backup_gold_resource = ini_get_setting(INI_GOLD_RESOURCE);
    backup_alien_derelicts = ini_get_setting(INI_ALIEN_DERELICTS);

    ini_config.LoadSection(file, INI_OPTIONS, ini_load_mode);
    ini_config.LoadSection(file, INI_PREFERENCES, true);

    if (mission_category == MISSION_CATEGORY_TRAINING || mission_category == MISSION_CATEGORY_CAMPAIGN ||
        mission_category == MISSION_CATEGORY_SCENARIO || mission_category == MISSION_CATEGORY_MULTI_PLAYER_SCENARIO) {
        ini_set_setting(INI_OPPONENT, opponent);
        ini_set_setting(INI_TIMER, turn_timer_time);
        ini_set_setting(INI_ENDTURN, endturn_time);
        ini_set_setting(INI_PLAY_MODE, play_mode);
        ini_set_setting(INI_START_GOLD, backup_start_gold);
        ini_set_setting(INI_RAW_RESOURCE, backup_raw_resource);
        ini_set_setting(INI_FUEL_RESOURCE, backup_fuel_resource);
        ini_set_setting(INI_GOLD_RESOURCE, backup_gold_resource);
        ini_set_setting(INI_ALIEN_DERELICTS, backup_alien_derelicts);
    }

    GameManager_PlayMode = ini_get_setting(INI_PLAY_MODE);

    const uint32_t map_cell_count{static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

    file.Read(ResourceManager_MapSurfaceMap, map_cell_count * sizeof(uint8_t));
    file.Read(ResourceManager_CargoMap, map_cell_count * sizeof(uint16_t));

    ResourceManager_InitTeamInfo();

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        CTInfo *team_info;

        team_info = &UnitsManager_TeamInfo[team];

        file.Read(team_info->markers);
        file.Read(team_info->team_type);
        file.Read(team_info->finished_turn);
        file.Read(team_info->team_clan);
        file.Read(team_info->research_topics);
        file.Read(team_info->team_points);
        file.Read(team_info->number_of_objects_created);
        file.Read(team_info->unit_counters);
        file.Read(team_info->screen_locations);
        file.Read(team_info->score_graph, sizeof(team_info->score_graph));
        file.Read(selected_unit_ids[team]);
        file.Read(team_info->zoom_level);
        file.Read(team_info->camera_position.x);
        file.Read(team_info->camera_position.y);
        file.Read(team_info->display_button_range);
        file.Read(team_info->display_button_scan);
        file.Read(team_info->display_button_status);
        file.Read(team_info->display_button_colors);
        file.Read(team_info->display_button_hits);
        file.Read(team_info->display_button_ammo);
        file.Read(team_info->display_button_names);
        file.Read(team_info->display_button_minimap_2x);
        file.Read(team_info->display_button_minimap_tnt);
        file.Read(team_info->display_button_grid);
        file.Read(team_info->display_button_survey);
        file.Read(team_info->stats_factories_built);
        file.Read(team_info->stats_mines_built);
        file.Read(team_info->stats_buildings_built);
        file.Read(team_info->stats_units_built);
        file.Read(team_info->casualties);
        file.Read(team_info->stats_gold_spent_on_upgrades);
    }

    {
        uint32_t active_turn_team;

        file.Read(active_turn_team);

        GameManager_ActiveTurnTeam = active_turn_team;
    }

    {
        uint32_t player_team;

        file.Read(player_team);

        GameManager_PlayerTeam = player_team;
    }

    {
        uint32_t turn_counter;

        file.Read(turn_counter);

        GameManager_TurnCounter = turn_counter;
    }

    {
        uint32_t turn_timer;

        file.Read(turn_timer);

        SaveLoadMenu_TurnTimer = turn_timer;
    }

    {
        uint32_t game_state;

        file.Read(game_state);

        SaveLoadMenu_GameState = game_state;
    }

    ResourceManager_TeamUnitsRed.FileLoad(file);
    ResourceManager_TeamUnitsGreen.FileLoad(file);
    ResourceManager_TeamUnitsBlue.FileLoad(file);
    ResourceManager_TeamUnitsGray.FileLoad(file);

    UnitsManager_InitPopupMenus();

    SmartList_UnitInfo_FileLoad(UnitsManager_GroundCoverUnits, file);
    SmartList_UnitInfo_FileLoad(UnitsManager_MobileLandSeaUnits, file);
    SmartList_UnitInfo_FileLoad(UnitsManager_StationaryUnits, file);
    SmartList_UnitInfo_FileLoad(UnitsManager_MobileAirUnits, file);
    SmartList_UnitInfo_FileLoad(UnitsManager_ParticleUnits, file);

    Hash_UnitHash.FileLoad(file);
    Hash_MapHash.FileLoad(file);

    UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_units = &ResourceManager_TeamUnitsRed;
    UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_units = &ResourceManager_TeamUnitsGreen;
    UnitsManager_TeamInfo[PLAYER_TEAM_BLUE].team_units = &ResourceManager_TeamUnitsBlue;
    UnitsManager_TeamInfo[PLAYER_TEAM_GRAY].team_units = &ResourceManager_TeamUnitsGray;

    save_load_flag = true;

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        CTInfo *team_info;
        char team_name[30];

        team_info = &UnitsManager_TeamInfo[team];

        if (team_info->team_type != TEAM_TYPE_NONE) {
            if (is_remote_game) {
                team_info->team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team));

                if (team_info->team_type == TEAM_TYPE_PLAYER) {
                    GameManager_PlayerTeam = team;
                    ini_config.GetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), team_name,
                                              sizeof(team_name));
                    ini_config.SetStringValue(INI_PLAYER_NAME, team_name);
                }

            } else if (mission_category == MISSION_CATEGORY_MULTI_PLAYER_SCENARIO) {
                team_info->team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team));

                if (save_load_flag && team_info->team_type == TEAM_TYPE_PLAYER) {
                    GameManager_PlayerTeam = team;
                    save_load_flag = false;
                }

            } else {
                switch (team_info->team_type) {
                    case TEAM_TYPE_PLAYER: {
                        if (mission_category != MISSION_CATEGORY_HOT_SEAT) {
                            ini_config.GetStringValue(INI_PLAYER_NAME, team_name, sizeof(team_name));
                            ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), team_name);
                        }
                    } break;

                    case TEAM_TYPE_COMPUTER: {
                        ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team),
                                                  menu_team_names[team]);
                    } break;

                    case TEAM_TYPE_REMOTE: {
                        team_info->team_type = TEAM_TYPE_PLAYER;
                    } break;
                }
            }

            ResourceManager_InitHeatMaps(team);

            if (team_info->team_type != TEAM_TYPE_NONE) {
                file.Read(team_info->heat_map_complete, map_cell_count);
                file.Read(team_info->heat_map_stealth_sea, map_cell_count);
                file.Read(team_info->heat_map_stealth_land, map_cell_count);

            } else {
                char *temp_buffer;

                temp_buffer = new (std::nothrow) char[map_cell_count];

                file.Read(temp_buffer, map_cell_count);
                file.Read(temp_buffer, map_cell_count);
                file.Read(temp_buffer, map_cell_count);

                delete[] temp_buffer;

                SaveLoad_TeamClearUnitList(UnitsManager_GroundCoverUnits, team);
                SaveLoad_TeamClearUnitList(UnitsManager_MobileLandSeaUnits, team);
                SaveLoad_TeamClearUnitList(UnitsManager_StationaryUnits, team);
                SaveLoad_TeamClearUnitList(UnitsManager_MobileAirUnits, team);
                SaveLoad_TeamClearUnitList(UnitsManager_ParticleUnits, team);

                if (team == UnitsManager_DelayedReactionsTeam) {
                    ++UnitsManager_DelayedReactionsTeam;
                }
            }

        } else {
            ResourceManager_InitHeatMaps(team);
        }

        ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team), team_info->team_type);
    }

    ResourceManager_InitHeatMaps(PLAYER_TEAM_ALIEN);

    MessageManager_LoadMessageLogs(file);

    if (mission_category == MISSION_CATEGORY_MULTI_PLAYER_SCENARIO) {
        Ai_Init();

    } else {
        Ai_FileLoad(file);
    }

    file.Close();

    GameManager_SelectedUnit = nullptr;

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (selected_unit_ids[team] != 0xFFFF) {
            UnitsManager_TeamInfo[team].selected_unit = Hash_UnitHash[selected_unit_ids[team]];

        } else {
            UnitsManager_TeamInfo[team].selected_unit = nullptr;
        }
    }

    result = true;

    return result;
}

std::string SaveLoad_TranslateMissionIndexToHashKey(const uint32_t save_file_type, const uint32_t index) {
    const auto mission_category = SaveLoad_TranslateSaveFileCategory(save_file_type);

    constexpr std::string_view TrainingMissions[] = {
        "24c19a1201ad24cfdc0c4cfc158596a505284992e57b34d65f2ddc2b243ce549",
        "964a248d6ba1e4595366712f11f7a7800474225a61f73117e7f5cfcd7642eb08",
        "09f2722bf6b01c7b5f3e2be43ce9affeb232e18621df662c6d789c15f2ef6f9d",
        "28b444c0e8d92f9fd89894b5527d15c08249075d8107865dd72261cd049d2ac4",
        "0e9619e645bb16ea93e231c1bbdc54839f1eefebad1e23dba01ae29597ccf355",
        "708d7744c99175ec2680861433632b8640fc4e139a142ad609c362edee51651c",
        "575b555bff26baeafda70a3cf9226e8a5103ce4f8ff5928377d5b2e5cfa1b9c2",
        "dbea01e3013b81bacafa1c834b2b26b05fc8cc225b4d88f2687812eb488e7c11",
        "7694150b1e309b863437607ee4b04299b719356f1b4d41cfda8e28b26b256d43",
        "166c5137eca626b85916f39b10b6fdc3fd9b5ff2d7fd5e5544c6396c851b4371",
        "935fb3c0948490480170ccf2c9c6660ae2f833a4518dc08972c0c2523355a9a7",
        "fd2db0fdd8a98bb3016c16fec90b1f680e78a83910e345b4e0162c45a35386cf",
        "ab169e63d7d0f03f2b2c640d9fcbc1dfb94810c9e378556d3468d153c7659ba9",
        "49a8abab703cef9101b6733668d8b0ccb6d67faac02c38a5bf4dc7532960563b",
        "9ac9193e3a41e2e37cf5314ccc429a4559e433155891727aacbb2860fa0adf4c",
    };

    constexpr std::string_view CampaignMissions[] = {
        "e25f962bf3dcf478335b3163e755f1d394c64fb1fe9653862bc3d38ae1b8d556",
        "4fd41542e802db515bc92a1afc1a0676ac7052e20f7ce2890f6b4fa748789506",
        "c5fb0253b5116b0b5dc7ab5edc197ff798b6cfc355b961c4ea98864fabc569ad",
        "14e710e0a9c3c5ee017bb7f3b492f996892cf27e9943566858339216d0b955b8",
        "04fb40113d068f81eae2b9701545c4fe049635fbe382ab61cce0209db2a81d10",
        "3c5d41a8ced0b0a9561560a235038ed6d5534e13e90ad510299b04f37b78ca1c",
        "1d9da84128776d1bc264f26f1381d19c16dead2c0af9bba9dfe9433853a8f00e",
        "0891d72f39acbf972d72168060e99fdcd5dd5b571d2fee711b2b21f98ae67327",
        "df4dbce57e7486956c51a74743c86059c900b4442f5de55db97b836d90205004",
    };

    constexpr std::string_view ScenarioMissions[] = {
        "c8985d85117e72a1f7e821e220374cfa504bd1f955c5811d3be1be1f8e20646f",
        "ea5d400404c3321ff1b34b56d028ac488b7dbb055811d254e86e27ccd10cadbf",
        "89d813b5fbe05dbb0c2c4e625de70a962460047389b322f7d64e85a85d606506",
        "920f0327b807a9a52981b698c994ccab9582a803c2916ee1ecec3c603cf0ae79",
        "62d4ca2ca15324cb16f4d49a6beecf8b1fda2fe6f28519f5c63077a6f2834f2f",
        "e11c5dd4f456c3c9f8ed1f657b4c530bafba3cc34e21842b0aa3728d3b58c377",
        "fae6b52ce64bf2248b08ebdc3614210fe1c7e0a1e9219a0131cb36a92e049eca",
        "1aa8d83146e5a31ebc8efeed4f9e63f9ab48b326cd7ea3bb4c74eb696a5e8b0d",
        "83c1492d9c0125a217aaf7ba10d9d47a4a1429c0a50fe5ea6828054c1be9d23e",
        "fd20cfe181b1da9871da3134c97f6476d49b0f589b0f3a5abd3ceed0a5fedb48",
        "b1d99c48a3f308abd1ed4bded276c5b24e5da0f00300795ed20445f98a182388",
        "68a5a75a9de28b428e20a2f8dfd9e471c1cacbd745da5742c3169fc83dad2e08",
        "1e6fab79997f536c0c8de6230a445c0c4545f7bf6a22cb06a49a2d189dd443dd",
        "da5d002ab0dcc133c163678592e4b459fe6275feea8e84edc481f97e424a447a",
        "a8a27b3913511e67ceac3f0da093e1d123cbc776a55340e996a49b2ff2ee3714",
        "5e24064600898e7d044da7f418d3c895f8255a81542f90b8c6a335b49c441d58",
        "659fe1ab6d696a5dacaf4572d5180158e1707965045a82c5a40f9fe13668cbbd",
        "f115e93d4a52ba9a28939a651d85914ada0ca020153058929724c5ce80fb64bb",
        "a223762f13ced61a5cd325bab4d72da5ffcb8d26c145042f2e9579a660ed3390",
        "d7e88e8026c9709fc48d704361f90ced93b6ae85fb03b99b3dca398c1c4863cc",
        "a8e8ee9909371170eb44069995f52ec3f44a42db658b6bc9c1cb0db56cad2bec",
        "fbc1f269cadfe18655c0166a48fe27493bfb91fa0ee3b6036950598ad91b950d",
        "c312c771a8c560f3a51bf3db5f63877f7c2d58bbe76bed8bf7d0446129a3298c",
        "b2b391e19d8e399ac5f907b159119ffd13ceb5e8afda127b763216dbae2d3070",
    };

    constexpr std::string_view MultiScenarioMissions[] = {
        "2e5e7be790f4a03d85fe3943220e13040894fb7ef94455e425f10d0a5fb3388d",
        "9613d3557d29385acccf006a0e0dfba50ab5cdf7e40e385e9b68b56057f3b69c",
        "f57997258f167cd8843a469c87ca8d0b0cb9a205d96c4a9e0ad861eeac8b3c17",
        "d4c0610ceb8370f2d16d8d1c412a6b5efa98f3b8f7f7527b60c97fd74d0ed09f",
        "613bd95aea9ec08cd1540da55a47b1b4f3b0baaa85da75425a45bced30963924",
        "f5b11ca96f41062a8e2b5b9f7be0e591fd8deacaac82219a8769dec57828945a",
        "6ff33d83005c12bbf77c85001082c4bdce7eb0a88a9d50ed4942bba4c99eb61b",
        "87984122cc9d8096b40a4fba1479f285cc6eb4288852dd3a01a1b304b87cde58",
        "5396100cf35ca46ba76bd89f00510e353d563651904ea09c9675d5a3b809589f",
        "a29856db9a28c9f64dca45cad767ebc31669e7b39b85ef9feb7a06ff303bf948",
        "77465bebe8cefdb3329c802fa55ac42ce531a99a201d20adce84c0db29446955",
        "9e57ebf688b754f89417feabfe79539596487a11d05771834766aa85d53265fa",
    };

    std::string result;

    switch (mission_category) {
        case MISSION_CATEGORY_TRAINING: {
            if (index < std::size(TrainingMissions)) {
                result = TrainingMissions[index];

            } else {
                result = "";
            }
        } break;

        case MISSION_CATEGORY_CAMPAIGN: {
            if (index < std::size(CampaignMissions)) {
                result = CampaignMissions[index];

            } else {
                result = "";
            }
        } break;

        case MISSION_CATEGORY_SCENARIO: {
            if (index < std::size(ScenarioMissions)) {
                result = ScenarioMissions[index];

            } else {
                result = "";
            }
        } break;

        case MISSION_CATEGORY_MULTI_PLAYER_SCENARIO: {
            if (index < std::size(MultiScenarioMissions)) {
                result = MultiScenarioMissions[index];

            } else {
                result = "";
            }
        } break;

        default: {
            result = "";
        } break;
    }

    return result;
}

/// \todo Remove
uint32_t SaveLoad_TranslateHashKeyToWorldIndex(const std::string hash) {
    constexpr std::string_view Worlds[] = {
        "c3f5c757d02197efab583091287a046b4b8991f2f0634281a7dd300d5fb365b0",
        "e90592f415271319fa53e8ec3605b4ff6abfc55915e938a91eedfea3d3ea9172",
        "a625d409ca1d5cd24cdeed165813e5b79c538b123491791016372492f5106801",
        "a6367393db48e1621d5ac247c4c4b2aac07885317f3ca40d0f12b81a28596f2e",
        "11a9255e65af53b6438803d0aef4ca96ed9d57575da3cf13779838ccda25a6e9",
        "d86feaba2d0af91065030a7aad2e206c3e2d26f780074ad436a84a5f7de226ee",
        "1c7fd2dbf663bd59e44b805d4eefe01679e25ffea8eb68337adfca5ec6eba7c6",
        "f18045f9dd315bb2c91dce05efb196924f9fcdd64a873482b997c432af1a9b12",
        "91b3ed80b33560e0e14c61120a6bb9ef7d2dfa44a159c130ea78f944cb5b9c64",
        "3749ceabc5b53aa3e7126d8751479725786d93fc800cf4b3e59b32d22dd301bd",
        "2646e4c177bccf19e88a78365f18fda48e06c0f8e0aa62ebd442b6ab2de646e1",
        "f2a2b010c280a8a73f0a7e6af549eb318b7b981463c366a7511a4725a138d393",
        "fad64e38a8c405afd9dfe8a905e2b4d167c671d02f4422500f300175b839c321",
        "4663efd15a19949ffd6fb0f2b7a23475494d54c51c670f500ec5c0b5c0a95645",
        "fa3da4a51d99e9a69e28de3aadb984789a8f79016a1c286971fc2e8340e0b6b0",
        "dbcfc4495334640776e6a0b1776651f904583aa87f193a43436e6b1f04635241",
        "315a11f6a4cc9cf6066a9cba9f84fb2dde7df9ad3b3025779d06268f8938dcc8",
        "f1fcedbc7be8571ffad83133711a8d267ffa07d05b207dc8e0e9353e05e34204",
        "2a1e65b4e9d95c6edc3f6da12d85c1d801df860fac562bc60f612c6fc2087ad8",
        "ef341cff54196289740ea3156423897b0a4135741d8214f8c1104cba3ba7f37a",
        "5877fc3c584558330683909b0ec153712e25f3337e975b590a0cacca3d8b4c7a",
        "5a6ac1733af876ed603d076f03106ac0a46977d43dc29d8c2885684ab3269b30",
        "33771f1cd7b3b321dca8667976f3e1fcadbf9c2c978e509fda1819291f0cf80c",
        "ddcba1014d11165e219d17396457a640376b22ca099525264fc7f3a509a138e9",
    };

    for (uint32_t i = 0; i < std::size(Worlds); ++i) {
        if (Worlds[i] == hash) {
            return i;
        }
    }

    return 0;
}

std::string SaveLoad_TranslateWorldIndexToHashKey(const uint32_t index) {
    constexpr std::string_view Worlds[] = {
        "c3f5c757d02197efab583091287a046b4b8991f2f0634281a7dd300d5fb365b0",
        "e90592f415271319fa53e8ec3605b4ff6abfc55915e938a91eedfea3d3ea9172",
        "a625d409ca1d5cd24cdeed165813e5b79c538b123491791016372492f5106801",
        "a6367393db48e1621d5ac247c4c4b2aac07885317f3ca40d0f12b81a28596f2e",
        "11a9255e65af53b6438803d0aef4ca96ed9d57575da3cf13779838ccda25a6e9",
        "d86feaba2d0af91065030a7aad2e206c3e2d26f780074ad436a84a5f7de226ee",
        "1c7fd2dbf663bd59e44b805d4eefe01679e25ffea8eb68337adfca5ec6eba7c6",
        "f18045f9dd315bb2c91dce05efb196924f9fcdd64a873482b997c432af1a9b12",
        "91b3ed80b33560e0e14c61120a6bb9ef7d2dfa44a159c130ea78f944cb5b9c64",
        "3749ceabc5b53aa3e7126d8751479725786d93fc800cf4b3e59b32d22dd301bd",
        "2646e4c177bccf19e88a78365f18fda48e06c0f8e0aa62ebd442b6ab2de646e1",
        "f2a2b010c280a8a73f0a7e6af549eb318b7b981463c366a7511a4725a138d393",
        "fad64e38a8c405afd9dfe8a905e2b4d167c671d02f4422500f300175b839c321",
        "4663efd15a19949ffd6fb0f2b7a23475494d54c51c670f500ec5c0b5c0a95645",
        "fa3da4a51d99e9a69e28de3aadb984789a8f79016a1c286971fc2e8340e0b6b0",
        "dbcfc4495334640776e6a0b1776651f904583aa87f193a43436e6b1f04635241",
        "315a11f6a4cc9cf6066a9cba9f84fb2dde7df9ad3b3025779d06268f8938dcc8",
        "f1fcedbc7be8571ffad83133711a8d267ffa07d05b207dc8e0e9353e05e34204",
        "2a1e65b4e9d95c6edc3f6da12d85c1d801df860fac562bc60f612c6fc2087ad8",
        "ef341cff54196289740ea3156423897b0a4135741d8214f8c1104cba3ba7f37a",
        "5877fc3c584558330683909b0ec153712e25f3337e975b590a0cacca3d8b4c7a",
        "5a6ac1733af876ed603d076f03106ac0a46977d43dc29d8c2885684ab3269b30",
        "33771f1cd7b3b321dca8667976f3e1fcadbf9c2c978e509fda1819291f0cf80c",
        "ddcba1014d11165e219d17396457a640376b22ca099525264fc7f3a509a138e9",
    };

    std::string result;

    if (index < std::size(Worlds)) {
        result = Worlds[index];

    } else {
        result = "";
    }

    return result;
}

MissionCategory SaveLoad_TranslateSaveFileCategory(const uint32_t save_file_type) {
    enum GameType : uint8_t {
        GAME_TYPE_CUSTOM = 0x0,
        GAME_TYPE_TRAINING = 0x1,
        GAME_TYPE_CAMPAIGN = 0x2,
        GAME_TYPE_HOT_SEAT = 0x3,
        GAME_TYPE_MULTI = 0x4,
        GAME_TYPE_DEMO = 0x5,
        GAME_TYPE_DEBUG = 0x6,
        GAME_TYPE_TEXT = 0x7,
        GAME_TYPE_SCENARIO = 0x8,
        GAME_TYPE_MULTI_PLAYER_SCENARIO = 0x9
    };

    MissionCategory mission_category;

    switch (save_file_type) {
        case GAME_TYPE_CUSTOM: {
            mission_category = MISSION_CATEGORY_CUSTOM;
        } break;

        case GAME_TYPE_TRAINING: {
            mission_category = MISSION_CATEGORY_TRAINING;
        } break;

        case GAME_TYPE_CAMPAIGN: {
            mission_category = MISSION_CATEGORY_CAMPAIGN;
        } break;

        case GAME_TYPE_HOT_SEAT: {
            mission_category = MISSION_CATEGORY_HOT_SEAT;
        } break;

        case GAME_TYPE_MULTI: {
            mission_category = MISSION_CATEGORY_MULTI;
        } break;

        case GAME_TYPE_DEMO: {
            mission_category = MISSION_CATEGORY_DEMO;
        } break;

        case GAME_TYPE_SCENARIO: {
            mission_category = MISSION_CATEGORY_SCENARIO;
        } break;

        case GAME_TYPE_MULTI_PLAYER_SCENARIO: {
            mission_category = MISSION_CATEGORY_MULTI_PLAYER_SCENARIO;
        } break;

        default: {
            mission_category = MISSION_CATEGORY_COUNT;
        } break;
    }

    return mission_category;
}

bool SaveLoad_Load(const std::filesystem::path &filepath, const MissionCategory mission_category, bool ini_load_mode,
                   bool is_remote_game) {
    bool result;
    SmartFileReader file;

    if (file.Open(filepath.string())) {
        switch (file.GetFormat()) {
            case SmartFileFormat::V70: {
                result = SaveLoad_LoadFormatV70(file, mission_category, is_remote_game, ini_load_mode);
            } break;

            case SmartFileFormat::V71: {
                result = SaveLoad_LoadFormatV71(file, mission_category, is_remote_game, ini_load_mode);
            } break;

            default: {
                MessageManager_DrawMessage(_(61ef), 2, 1, true);

                result = false;
            } break;
        }

    } else {
        result = false;
    }

    return result;
}

std::string SaveLoad_GetSaveFileName(const MissionCategory mission_category, const uint32_t save_slot) {
    const char *extension;

    switch (mission_category) {
        case MISSION_CATEGORY_CUSTOM: {
            extension = "dta";
        } break;

        case MISSION_CATEGORY_TRAINING: {
            extension = "tra";
        } break;

        case MISSION_CATEGORY_CAMPAIGN: {
            extension = "cam";
        } break;

        case MISSION_CATEGORY_HOT_SEAT: {
            extension = "hot";
        } break;

        case MISSION_CATEGORY_MULTI: {
            extension = "mlt";
        } break;

        case MISSION_CATEGORY_DEMO: {
            extension = "dmo";
        } break;

        case MISSION_CATEGORY_SCENARIO: {
            extension = "sce";
        } break;

        case MISSION_CATEGORY_MULTI_PLAYER_SCENARIO: {
            extension = "mps";
        } break;

        default: {
            extension = "dta";

            SDL_assert(0);
        } break;
    }

    return std::format("save{}.{}", save_slot, extension);
}

[[nodiscard]] MissionCategory SaveLoad_GetSaveFileCategory(const MissionCategory mission_category) {
    MissionCategory result;

    switch (mission_category) {
        case MISSION_CATEGORY_CUSTOM: {
            result = MISSION_CATEGORY_CUSTOM;
        } break;

        case MISSION_CATEGORY_TRAINING: {
            result = MISSION_CATEGORY_CUSTOM;
        } break;

        case MISSION_CATEGORY_CAMPAIGN: {
            result = MISSION_CATEGORY_CUSTOM;
        } break;

        case MISSION_CATEGORY_HOT_SEAT: {
            result = MISSION_CATEGORY_HOT_SEAT;
        } break;

        case MISSION_CATEGORY_MULTI: {
            result = MISSION_CATEGORY_MULTI;
        } break;

        case MISSION_CATEGORY_DEMO: {
            result = MISSION_CATEGORY_CUSTOM;
        } break;

        case MISSION_CATEGORY_SCENARIO: {
            result = MISSION_CATEGORY_CUSTOM;
        } break;

        case MISSION_CATEGORY_MULTI_PLAYER_SCENARIO: {
            if (Remote_IsNetworkGame) {
                result = MISSION_CATEGORY_MULTI;

            } else if (GameManager_HumanPlayerCount > 0) {
                result = MISSION_CATEGORY_HOT_SEAT;

            } else {
                result = MISSION_CATEGORY_CUSTOM;
            }
        } break;

        default: {
            result = MISSION_CATEGORY_CUSTOM;

            SDL_assert(0);
        } break;
    }

    return result;
}
