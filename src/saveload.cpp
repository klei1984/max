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

#include "ai.hpp"
#include "game_manager.hpp"
#include "hash.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "message_manager.hpp"
#include "missionregistry.hpp"
#include "smartfile.hpp"
#include "units_manager.hpp"

extern const char *menu_team_names[];
extern const char *SaveLoadMenu_TutorialTitles[];
extern const char *SaveLoadMenu_ScenarioTitles[];
extern const char *SaveLoadMenu_CampaignTitles[];
extern const char *SaveLoadMenu_SaveFileTypes[];
extern int32_t SaveLoadMenu_SaveSlot;
extern uint16_t SaveLoadMenu_TurnTimer;
extern uint8_t SaveLoadMenu_GameState;

static void SaveLoad_TeamClearUnitList(SmartList<UnitInfo> &units, uint16_t team);
static std::filesystem::path SaveLoad_GetFilePath(const int32_t save_slot, const int32_t game_file_type,
                                                  std::string *file_name = nullptr);

void SaveLoad_TeamClearUnitList(SmartList<UnitInfo> &units, uint16_t team) {
    for (auto it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team) {
            UnitsManager_DestroyUnit(&*it);
        }
    }
}

bool SaveLoad_LoadIniOptions(const int32_t save_slot, const int32_t game_file_type) {
    SmartFileReader file;
    auto filepath = SaveLoad_GetFilePath(save_slot, game_file_type);
    bool result;

    if (file.Open(filepath.string().c_str())) {
        switch (file.GetFormat()) {
            case SmartFileFormat::V70: {
                uint8_t buffer[176];

                file.Read(buffer);

                ini_config.LoadSection(file, INI_OPTIONS, true);

                result = true;
            } break;

            case SmartFileFormat::V71: {
                result = false;
            } break;

            default: {
                result = false;
            } break;
        }

        file.Close();

    } else {
        result = false;
    }

    return result;
}

std::filesystem::path SaveLoad_GetFilePath(const int32_t save_slot, const int32_t game_file_type,
                                           std::string *file_name) {
    std::filesystem::path filepath;
    SmartString filename;

    filename.Sprintf(20, "save%i.%s", save_slot, SaveLoadMenu_SaveFileTypes[game_file_type]);

    if (file_name) {
        *file_name = filename.GetCStr();
    }

    filename.Toupper();

    if (game_file_type == GAME_TYPE_CUSTOM || game_file_type == GAME_TYPE_HOT_SEAT ||
        game_file_type == GAME_TYPE_MULTI) {
        filepath = (ResourceManager_FilePathGamePref / filename.GetCStr()).lexically_normal();

    } else {
        filepath = (ResourceManager_FilePathGameData / filename.GetCStr()).lexically_normal();
    }

    return filepath;
}

bool SaveLoad_GetSaveFileInfo(const int32_t save_slot, const int32_t game_file_type,
                              struct SaveFileInfo &save_file_header) {
    SmartFileReader file;
    std::string file_name;
    bool result;
    auto filepath = SaveLoad_GetFilePath(save_slot, game_file_type, &file_name);

    if (file.Open(filepath.string().c_str())) {
        switch (file.GetFormat()) {
            case SmartFileFormat::V70: {
                uint16_t version;
                uint8_t save_game_type;
                char save_name[30];
                uint8_t world;
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
                file.Read(world);
                file.Read(mission_index);
                file.Read(team_names);
                file.Read(team_type);
                file.Read(team_clan);
                file.Read(rng_seed);
                file.Read(opponent);
                file.Read(turn_timer_time);
                file.Read(endturn_time);
                file.Read(play_mode);

                save_file_header.version = version;
                save_file_header.save_game_type = save_game_type;
                save_file_header.save_name = save_name;

                /// \todo
                // save_file_header.mission = mission_index;
                // save_file_header.world = world;

                for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 2; ++team) {
                    save_file_header.team_names[team] = team_names[team];
                }

                for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                    save_file_header.team_type[team] = team_type[team];
                    save_file_header.team_clan[team] = team_clan[team];
                }

                save_file_header.rng_seed = rng_seed;

                result = true;
            } break;

            case SmartFileFormat::V71: {
                result = false;
            } break;

            default: {
                result = false;
            } break;
        }

        save_file_header.file_name = file_name;

        switch (game_file_type) {
            case GAME_TYPE_TRAINING: {
                save_file_header.save_name = SaveLoadMenu_TutorialTitles[save_slot - 1];
            } break;

            case GAME_TYPE_SCENARIO: {
                save_file_header.save_name = SaveLoadMenu_ScenarioTitles[save_slot - 1];
            } break;

            case GAME_TYPE_CAMPAIGN: {
                save_file_header.save_name = SaveLoadMenu_CampaignTitles[save_slot - 1];
            } break;

            case GAME_TYPE_MULTI_PLAYER_SCENARIO: {
                save_file_header.save_name = SmartString().Sprintf(100, "%s #%i", _(2954), save_slot).GetCStr();
            } break;
        }

        file.Close();

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
            result = false;
        } break;

        default: {
            result = false;
        } break;
    }

    return result;
}

void SaveLoad_Save(const std::filesystem::path &filepath, const char *const save_name, const uint32_t rng_seed) {
    SmartFileWriter file;
    if (file.Open(filepath.string().c_str())) {
        uint16_t version;
        uint8_t save_game_type;
        char local_save_name[30];
        uint8_t world;
        uint16_t mission_index;
        char team_names[4][30];
        uint8_t team_type[5];
        uint8_t team_clan[5];
        int8_t opponent;
        uint16_t turn_timer_time;
        uint16_t endturn_time;
        int8_t play_mode;
        uint16_t game_state;
        const uint32_t map_cell_count{static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

        version = static_cast<uint16_t>(SmartFileFormat::V70);
        save_game_type = ini_get_setting(INI_GAME_FILE_TYPE);

        SDL_utf8strlcpy(local_save_name, save_name, sizeof(local_save_name));

        world = ini_get_setting(INI_WORLD);
        mission_index = GameManager_GameFileNumber;
        opponent = ini_get_setting(INI_OPPONENT);
        turn_timer_time = ini_get_setting(INI_TIMER);
        endturn_time = ini_get_setting(INI_ENDTURN);
        play_mode = ini_get_setting(INI_PLAY_MODE);

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if ((save_game_type == GAME_TYPE_TRAINING || save_game_type == GAME_TYPE_SCENARIO ||
                 save_game_type == GAME_TYPE_CAMPAIGN) &&
                team != PLAYER_TEAM_RED && UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER) {
                UnitsManager_TeamInfo[team].team_type = TEAM_TYPE_COMPUTER;
            }

            if (save_game_type == GAME_TYPE_DEMO && UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                UnitsManager_TeamInfo[team].team_type = TEAM_TYPE_COMPUTER;
            }

            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                ini_config.GetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), team_names[team],
                                          sizeof(team_names[team]));
                if (!strlen(team_names[team])) {
                    strcpy(team_names[team], menu_team_names[team]);
                }
            }

            team_type[team] = UnitsManager_TeamInfo[team].team_type;
            team_clan[team] = UnitsManager_TeamInfo[team].team_clan;
        }

        file.Write(version);
        file.Write(save_game_type);
        file.Write(local_save_name);
        file.Write(world);
        file.Write(mission_index);
        file.Write(team_names);
        file.Write(team_type);
        file.Write(team_clan);
        file.Write(rng_seed);
        file.Write(opponent);
        file.Write(turn_timer_time);
        file.Write(endturn_time);
        file.Write(play_mode);

        ini_config.SaveSection(file, INI_OPTIONS);

        file.Write(ResourceManager_MapSurfaceMap, map_cell_count * sizeof(uint8_t));
        file.Write(ResourceManager_CargoMap, map_cell_count * sizeof(uint16_t));

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            CTInfo *team_info;
            uint16_t unit_id;

            team_info = &UnitsManager_TeamInfo[team];

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

        file.Write(GameManager_ActiveTurnTeam);
        file.Write(GameManager_PlayerTeam);
        file.Write(GameManager_TurnCounter);

        game_state = GameManager_GameState;

        file.Write(game_state);

        uint16_t timer_value = GameManager_TurnTimerValue;

        file.Write(timer_value);

        ini_config.SaveSection(file, INI_PREFERENCES);

        ResourceManager_TeamUnitsRed.FileSave(file);
        ResourceManager_TeamUnitsGreen.FileSave(file);
        ResourceManager_TeamUnitsBlue.FileSave(file);
        ResourceManager_TeamUnitsGray.FileSave(file);

        SmartList_UnitInfo_FileSave(UnitsManager_GroundCoverUnits, file);
        SmartList_UnitInfo_FileSave(UnitsManager_MobileLandSeaUnits, file);
        SmartList_UnitInfo_FileSave(UnitsManager_StationaryUnits, file);
        SmartList_UnitInfo_FileSave(UnitsManager_MobileAirUnits, file);
        SmartList_UnitInfo_FileSave(UnitsManager_ParticleUnits, file);

        Hash_UnitHash.FileSave(file);
        Hash_MapHash.FileSave(file);

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                file.Write(UnitsManager_TeamInfo[team].heat_map_complete, map_cell_count);
                file.Write(UnitsManager_TeamInfo[team].heat_map_stealth_sea, map_cell_count);
                file.Write(UnitsManager_TeamInfo[team].heat_map_stealth_land, map_cell_count);
            }
        }

        MessageManager_SaveMessageLogs(file);

        Ai_FileSave(file);

        file.Close();
    }
}

bool SaveLoad_LoadFormatV70(SmartFileReader &file, int32_t save_slot, bool is_remote_game, bool ini_load_mode,
                            int32_t game_file_type) {
    bool result;
    bool save_load_flag;
    uint16_t version;
    uint8_t save_game_type;
    char save_name[30];
    uint8_t world;
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
    file.Read(world);
    file.Read(mission_index);
    file.Read(team_names);
    file.Read(team_type);
    file.Read(team_clan);
    file.Read(rng_seed);
    file.Read(opponent);
    file.Read(turn_timer_time);
    file.Read(endturn_time);
    file.Read(play_mode);

    if (save_game_type != GAME_TYPE_CAMPAIGN) {
        SaveLoadMenu_SaveSlot = save_slot;
    }

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

    GameManager_GameFileNumber = mission_index;

    ResourceManager_InitInGameAssets(world);

    ini_set_setting(INI_GAME_FILE_TYPE, save_game_type);

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

    if (game_file_type == GAME_TYPE_TRAINING || game_file_type == GAME_TYPE_CAMPAIGN ||
        game_file_type == GAME_TYPE_SCENARIO || game_file_type == GAME_TYPE_MULTI_PLAYER_SCENARIO) {
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

            } else if (game_file_type == GAME_TYPE_MULTI_PLAYER_SCENARIO) {
                team_info->team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team));

                if (save_load_flag && team_info->team_type == TEAM_TYPE_PLAYER) {
                    GameManager_PlayerTeam = team;
                    save_load_flag = false;
                }

            } else {
                switch (team_info->team_type) {
                    case TEAM_TYPE_PLAYER: {
                        if (game_file_type != GAME_TYPE_HOT_SEAT) {
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

    if (game_file_type == GAME_TYPE_MULTI_PLAYER_SCENARIO) {
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

bool SaveLoad_Load(const std::filesystem::path &filepath, int32_t save_slot, int32_t game_file_type, bool ini_load_mode,
                   bool is_remote_game) {
    bool result;
    SmartFileReader file;

    if (file.Open(filepath.string().c_str())) {
        switch (file.GetFormat()) {
            case SmartFileFormat::V70: {
                result = SaveLoad_LoadFormatV70(file, save_slot, is_remote_game, ini_load_mode, game_file_type);
            } break;

            case SmartFileFormat::V71: {
                result = false;
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
