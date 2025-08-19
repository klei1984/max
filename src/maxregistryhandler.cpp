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

#include "game_manager.hpp"
#include "inifile.hpp"
#include "scripter.hpp"
#include "units_manager.hpp"

namespace MaxRegistryHandler {}

void MaxRegistryHandler::Init() {
    Reset();

    Scripter::MaxRegistryRegister("RedTeamPoints", []() -> std::variant<bool, size_t, double, std::string> {
        return UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_points;
    });

    Scripter::MaxRegistryRegister("GreenTeamPoints", []() -> std::variant<bool, size_t, double, std::string> {
        return UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_points;
    });

    Scripter::MaxRegistryRegister("BlueTeamPoints", []() -> std::variant<bool, size_t, double, std::string> {
        return UnitsManager_TeamInfo[PLAYER_TEAM_BLUE].team_points;
    });

    Scripter::MaxRegistryRegister("GrayTeamPoints", []() -> std::variant<bool, size_t, double, std::string> {
        return UnitsManager_TeamInfo[PLAYER_TEAM_GRAY].team_points;
    });

    Scripter::MaxRegistryRegister("TeamScoreVictoryLimit", []() -> std::variant<bool, size_t, double, std::string> {
        const auto victory_type = ini_setting_victory_type;
        const auto victory_limit = ini_setting_victory_limit;

        return static_cast<size_t>((victory_type == VICTORY_TYPE_SCORE) ? victory_limit : -1);
    });

    Scripter::MaxRegistryRegister("TeamTurnVictoryLimit", []() -> std::variant<bool, size_t, double, std::string> {
        const auto victory_type = ini_setting_victory_type;
        const auto victory_limit = ini_setting_victory_limit;

        return static_cast<size_t>((victory_type == VICTORY_TYPE_DURATION) ? victory_limit : -1);
    });

    Scripter::MaxRegistryRegister("TeamTurnValue", []() -> std::variant<bool, size_t, double, std::string> {
        return static_cast<size_t>(GameManager_TurnCounter);
    });

    Update();
}

void MaxRegistryHandler::LoadMission(const Mission& mission) {}

void MaxRegistryHandler::LoadCampaign(const Campaign& campaign) {}

void MaxRegistryHandler::Reset() { Scripter::MaxRegistryReset(); }

void MaxRegistryHandler::Update() { Scripter::MaxRegistryUpdate(); }
