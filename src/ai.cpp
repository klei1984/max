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

#include "ai.hpp"

#include "ai_player.hpp"
#include "units_manager.hpp"

bool Ai_SetupStrategy(unsigned short team) {
    /// \todo

    return false;
}

void Ai_SetInfoMapPoint(Point point, unsigned short team) {
    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        AiPlayer_Teams[team].SetInfoMapPoint(point);
    }
}

void Ai_MarkMineMapPoint(Point point, unsigned short team) {
    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        AiPlayer_Teams[team].MarkMineMapPoint(point);
    }
}

void Ai_UpdateMineMap(Point point, unsigned short team) {
    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        AiPlayer_Teams[team].UpdateMineMap(point);
    }
}

void Ai_SetTasksPendingFlag(const char* event) {
    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
            AiPlayer_Teams[team].ChangeTasksPendingFlag(true);
        }
    }
}
