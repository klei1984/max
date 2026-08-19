/* Copyright (c) 2026 M.A.X. Port Team
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

#include "crash_context.hpp"

#include <cstring>
#include <string>

#include "enums.hpp"
#include "game_manager.hpp"
#include "mission.hpp"
#include "missionmanager.hpp"
#include "resource_manager.hpp"
#include "resourcetable.hpp"
#include "saveload.hpp"
#include "saveloadmenu.hpp"
#include "settings.hpp"

namespace {

void CrashContext_CopyText(char* destination, const size_t size, const char* text) {
    if (!text) {
        destination[0] = '\0';

        return;
    }

    std::strncpy(destination, text, size - 1u);
    destination[size - 1u] = '\0';
}

const char* CrashContext_GetPhase(const uint8_t game_state) {
    const char* phase;

    switch (game_state) {
        case GAME_STATE_3_MAIN_MENU: {
            phase = "main menu";
        } break;

        case GAME_STATE_6_GAME_SETUP: {
            phase = "game setup";
        } break;

        case GAME_STATE_7_SITE_SELECT:
        case GAME_STATE_13_SITE_SELECTED:
        case GAME_STATE_14_EXIT_SITE_SELECT: {
            phase = "landing site selection";
        } break;

        case GAME_STATE_8_IN_GAME: {
            phase = "in game";
        } break;

        case GAME_STATE_9_END_TURN: {
            phase = "end of turn";
        } break;

        case GAME_STATE_10_LOAD_GAME: {
            phase = "loading a save";
        } break;

        case GAME_STATE_11_TURN_ACTIVE: {
            phase = "turn in progress";
        } break;

        case GAME_STATE_12_DEPLOYING_UNITS: {
            phase = "deploying units";
        } break;

        case GAME_STATE_15_FATAL_ERROR: {
            phase = "fatal error handling";
        } break;

        default: {
            phase = "startup";
        } break;
    }

    return phase;
}

const char* CrashContext_GetMissionKind(const MissionCategory category) {
    const char* kind;

    switch (category) {
        case MISSION_CATEGORY_CUSTOM: {
            kind = "custom";
        } break;

        case MISSION_CATEGORY_TRAINING: {
            kind = "training";
        } break;

        case MISSION_CATEGORY_CAMPAIGN: {
            kind = "campaign";
        } break;

        case MISSION_CATEGORY_HOT_SEAT: {
            kind = "hot seat";
        } break;

        case MISSION_CATEGORY_MULTI: {
            kind = "multiplayer";
        } break;

        case MISSION_CATEGORY_DEMO: {
            kind = "demo";
        } break;

        case MISSION_CATEGORY_SCENARIO: {
            kind = "scenario";
        } break;

        case MISSION_CATEGORY_MULTI_PLAYER_SCENARIO: {
            kind = "multiplayer scenario";
        } break;

        default: {
            kind = "";
        } break;
    }

    return kind;
}

void CrashContext_FillMission(CrashContext& context) {
    try {
        const auto mission_manager = ResourceManager_GetMissionManager();

        if (!mission_manager) {
            return;
        }

        const auto mission = mission_manager->GetMission();

        if (!mission) {
            return;
        }

        const auto category = mission->GetCategory();

        CrashContext_CopyText(context.mission_kind, sizeof(context.mission_kind),
                              CrashContext_GetMissionKind(category));

        const auto title = mission->GetTitle();

        CrashContext_CopyText(context.mission, sizeof(context.mission), title.c_str());

        const auto hashes = mission->GetMissionHashes();

        if (!hashes.empty()) {
            CrashContext_CopyText(context.mission_hash, sizeof(context.mission_hash), hashes.front().c_str());
        }

        if (context.save_slot > 0) {
            const auto save_file = SaveLoad_GetSaveFileName(category, static_cast<uint32_t>(context.save_slot));

            CrashContext_CopyText(context.save_file, sizeof(context.save_file), save_file.c_str());
        }

    } catch (...) {
        ;  // nothing to do
    }
}

void CrashContext_Fill(CrashContext& context) {
    context.game_state = GameManager_GameState;
    context.turn = GameManager_TurnCounter;
    context.play_mode = GameManager_PlayMode;
    context.player_team = GameManager_PlayerTeam;
    context.active_team = GameManager_ActiveTurnTeam;
    context.human_players = GameManager_HumanPlayerCount;
    context.save_slot = SaveLoadMenu_SaveSlot;
    context.victory_type = ini_setting_victory_type;
    context.victory_limit = ini_setting_victory_limit;
    context.real_time = GameManager_RealTime;
    context.cheater = GameManager_IsCheater;
    context.world_index = -1;

    CrashContext_CopyText(context.phase, sizeof(context.phase), CrashContext_GetPhase(GameManager_GameState));

    try {
        const auto settings = ResourceManager_GetSettings();

        if (settings) {
            context.world_index = settings->GetNumericValue("world");
            context.opponent = settings->GetNumericValue("opponent");
        }

    } catch (...) {
        ;  // nothing to do
    }

    CrashContext_FillMission(context);

    context.valid = true;
}

}  // namespace

void CrashContext_Register() { CrashReporter_SetContextProvider(&CrashContext_Fill); }
