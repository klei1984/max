/* Copyright (c) 2022 M.A.X. Port Team
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

#include "winloss.hpp"

#include <utility>

#include "game_manager.hpp"
#include "missionmanager.hpp"
#include "units_manager.hpp"

static int32_t WinLoss_GetWorthOfAssets(uint16_t team, SmartList<UnitInfo>* units);
static int32_t WinLoss_GetTotalWorthOfAssets(uint16_t team);

int32_t WinLoss_GetWorthOfAssets(uint16_t team, SmartList<UnitInfo>* units) {
    int32_t sum_build_turns_value_of_team;

    sum_build_turns_value_of_team = 0;

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team) {
            sum_build_turns_value_of_team += (*it).GetNormalRateBuildCost();
        }
    }

    return sum_build_turns_value_of_team;
}

int32_t WinLoss_GetTotalWorthOfAssets(uint16_t team) {
    return WinLoss_GetWorthOfAssets(team, &UnitsManager_MobileLandSeaUnits) +
           WinLoss_GetWorthOfAssets(team, &UnitsManager_MobileAirUnits) +
           WinLoss_GetWorthOfAssets(team, &UnitsManager_StationaryUnits);
}

WinLoss_Status WinLoss_EvaluateStatus(const int32_t turn_counter) {
    WinLoss_Status status = {0};
    uint16_t teams_in_play[PLAYER_TEAM_MAX] = {0};
    const auto& winloss_handler = ResourceManager_GetMissionManager()->GetWinLossHandler();
    const auto mission_category = ResourceManager_GetMissionManager()->GetMission()->GetCategory();

    SDL_assert(winloss_handler);

    status.mission_type = mission_category;

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        status.team_type[team] = UnitsManager_TeamInfo[team].team_type;
    }

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            WinLossState team_status;

            if (winloss_handler->TestWinLossConditions(team, team_status)) {
                status.team_status[team] = team_status;

            } else {
                SDL_assert(0);

                status.team_status[team] = VICTORY_STATE_GENERIC;
            }

            ++status.teams_total;

            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_COMPUTER) {
                ++status.teams_non_computers;
            }

            if (status.team_status[team] != VICTORY_STATE_LOST &&
                UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED) {
                if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_COMPUTER) {
                    ++status.teams_humans_in_play;
                }

                teams_in_play[status.teams_in_play] = team;
                ++status.teams_in_play;
            }
        }
    }

    for (int32_t i = 0; i < status.teams_in_play - 1; ++i) {
        for (int32_t j = i + 1; j < status.teams_in_play; ++j) {
            if (UnitsManager_TeamInfo[teams_in_play[i]].team_points <
                UnitsManager_TeamInfo[teams_in_play[j]].team_points) {
                std::swap(teams_in_play[i], teams_in_play[j]);

            } else if (UnitsManager_TeamInfo[teams_in_play[j]].team_points ==
                       UnitsManager_TeamInfo[teams_in_play[i]].team_points) {
                if (WinLoss_GetTotalWorthOfAssets(teams_in_play[i]) - WinLoss_GetTotalWorthOfAssets(teams_in_play[j]) <
                    0) {
                    std::swap(teams_in_play[i], teams_in_play[j]);
                }
            }
        }
    }

    for (int32_t i = 0; i < status.teams_in_play; ++i) {
        status.team_rank[teams_in_play[i]] = i + 1;
    }

    return status;
}
