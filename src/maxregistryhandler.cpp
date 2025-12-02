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

#include "maxregistryhandler.hpp"

#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "resource_manager.hpp"
#include "scripter.hpp"
#include "settings.hpp"
#include "units_manager.hpp"

namespace MaxRegistryHandler {}

void MaxRegistryHandler::Init() {
    Reset();

    Scripter::MaxRegistryRegister("RedTeamPoints", []() -> std::variant<bool, int64_t, double, std::string> {
        return UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_points;
    });

    Scripter::MaxRegistryRegister("GreenTeamPoints", []() -> std::variant<bool, int64_t, double, std::string> {
        return UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_points;
    });

    Scripter::MaxRegistryRegister("BlueTeamPoints", []() -> std::variant<bool, int64_t, double, std::string> {
        return UnitsManager_TeamInfo[PLAYER_TEAM_BLUE].team_points;
    });

    Scripter::MaxRegistryRegister("GrayTeamPoints", []() -> std::variant<bool, int64_t, double, std::string> {
        return UnitsManager_TeamInfo[PLAYER_TEAM_GRAY].team_points;
    });

    Scripter::MaxRegistryRegister("TeamScoreVictoryLimit", []() -> std::variant<bool, int64_t, double, std::string> {
        const auto victory_type = ini_setting_victory_type;
        const auto victory_limit = ini_setting_victory_limit;

        return static_cast<int64_t>((victory_type == VICTORY_TYPE_SCORE) ? victory_limit : -1);
    });

    Scripter::MaxRegistryRegister("TeamTurnVictoryLimit", []() -> std::variant<bool, int64_t, double, std::string> {
        const auto victory_type = ini_setting_victory_type;
        const auto victory_limit = ini_setting_victory_limit;

        return static_cast<int64_t>((victory_type == VICTORY_TYPE_DURATION) ? victory_limit : -1);
    });

    Scripter::MaxRegistryRegister("TeamTurnValue", []() -> std::variant<bool, int64_t, double, std::string> {
        return static_cast<int64_t>(GameManager_TurnCounter);
    });

    // Team types
    Scripter::MaxRegistryRegister("RedTeamType", []() -> std::variant<bool, int64_t, double, std::string> {
        return static_cast<int64_t>(UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_type);
    });

    Scripter::MaxRegistryRegister("GreenTeamType", []() -> std::variant<bool, int64_t, double, std::string> {
        return static_cast<int64_t>(UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_type);
    });

    Scripter::MaxRegistryRegister("BlueTeamType", []() -> std::variant<bool, int64_t, double, std::string> {
        return static_cast<int64_t>(UnitsManager_TeamInfo[PLAYER_TEAM_BLUE].team_type);
    });

    Scripter::MaxRegistryRegister("GrayTeamType", []() -> std::variant<bool, int64_t, double, std::string> {
        return static_cast<int64_t>(UnitsManager_TeamInfo[PLAYER_TEAM_GRAY].team_type);
    });

    // Team clans
    Scripter::MaxRegistryRegister("RedTeamClan", []() -> std::variant<bool, int64_t, double, std::string> {
        return static_cast<int64_t>(UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_clan);
    });

    Scripter::MaxRegistryRegister("GreenTeamClan", []() -> std::variant<bool, int64_t, double, std::string> {
        return static_cast<int64_t>(UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_clan);
    });

    Scripter::MaxRegistryRegister("BlueTeamClan", []() -> std::variant<bool, int64_t, double, std::string> {
        return static_cast<int64_t>(UnitsManager_TeamInfo[PLAYER_TEAM_BLUE].team_clan);
    });

    Scripter::MaxRegistryRegister("GrayTeamClan", []() -> std::variant<bool, int64_t, double, std::string> {
        return static_cast<int64_t>(UnitsManager_TeamInfo[PLAYER_TEAM_GRAY].team_clan);
    });

    // Team strategies (human players get AI_STRATEGY_RANDOM)
    Scripter::MaxRegistryRegister("RedTeamStrategy", []() -> std::variant<bool, int64_t, double, std::string> {
        if (UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_type == TEAM_TYPE_COMPUTER) {
            return static_cast<int64_t>(AiPlayer_Teams[PLAYER_TEAM_RED].GetStrategy());
        }
        return static_cast<int64_t>(AI_STRATEGY_RANDOM);
    });

    Scripter::MaxRegistryRegister("GreenTeamStrategy", []() -> std::variant<bool, int64_t, double, std::string> {
        if (UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_type == TEAM_TYPE_COMPUTER) {
            return static_cast<int64_t>(AiPlayer_Teams[PLAYER_TEAM_GREEN].GetStrategy());
        }
        return static_cast<int64_t>(AI_STRATEGY_RANDOM);
    });

    Scripter::MaxRegistryRegister("BlueTeamStrategy", []() -> std::variant<bool, int64_t, double, std::string> {
        if (UnitsManager_TeamInfo[PLAYER_TEAM_BLUE].team_type == TEAM_TYPE_COMPUTER) {
            return static_cast<int64_t>(AiPlayer_Teams[PLAYER_TEAM_BLUE].GetStrategy());
        }
        return static_cast<int64_t>(AI_STRATEGY_RANDOM);
    });

    Scripter::MaxRegistryRegister("GrayTeamStrategy", []() -> std::variant<bool, int64_t, double, std::string> {
        if (UnitsManager_TeamInfo[PLAYER_TEAM_GRAY].team_type == TEAM_TYPE_COMPUTER) {
            return static_cast<int64_t>(AiPlayer_Teams[PLAYER_TEAM_GRAY].GetStrategy());
        }
        return static_cast<int64_t>(AI_STRATEGY_RANDOM);
    });

    // Team difficulty levels (human players get OPPONENT_TYPE_GOD)
    Scripter::MaxRegistryRegister("RedTeamDifficulty", []() -> std::variant<bool, int64_t, double, std::string> {
        if (UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_type == TEAM_TYPE_COMPUTER) {
            return static_cast<int64_t>(ResourceManager_GetSettings()->GetNumericValue("opponent"));
        }
        return static_cast<int64_t>(OPPONENT_TYPE_GOD);
    });

    Scripter::MaxRegistryRegister("GreenTeamDifficulty", []() -> std::variant<bool, int64_t, double, std::string> {
        if (UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_type == TEAM_TYPE_COMPUTER) {
            return static_cast<int64_t>(ResourceManager_GetSettings()->GetNumericValue("opponent"));
        }
        return static_cast<int64_t>(OPPONENT_TYPE_GOD);
    });

    Scripter::MaxRegistryRegister("BlueTeamDifficulty", []() -> std::variant<bool, int64_t, double, std::string> {
        if (UnitsManager_TeamInfo[PLAYER_TEAM_BLUE].team_type == TEAM_TYPE_COMPUTER) {
            return static_cast<int64_t>(ResourceManager_GetSettings()->GetNumericValue("opponent"));
        }
        return static_cast<int64_t>(OPPONENT_TYPE_GOD);
    });

    Scripter::MaxRegistryRegister("GrayTeamDifficulty", []() -> std::variant<bool, int64_t, double, std::string> {
        if (UnitsManager_TeamInfo[PLAYER_TEAM_GRAY].team_type == TEAM_TYPE_COMPUTER) {
            return static_cast<int64_t>(ResourceManager_GetSettings()->GetNumericValue("opponent"));
        }
        return static_cast<int64_t>(OPPONENT_TYPE_GOD);
    });

    Update();
}

void MaxRegistryHandler::LoadMission(const Mission& mission) {}

void MaxRegistryHandler::LoadCampaign(const Campaign& campaign) {}

void MaxRegistryHandler::Reset() { Scripter::MaxRegistryReset(); }

void MaxRegistryHandler::Update() { Scripter::MaxRegistryUpdate(); }
