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

#ifndef WINLOSS_HPP
#define WINLOSS_HPP

#include <cstdint>

#include "enums.hpp"

enum WinLossState {
    VICTORY_STATE_GENERIC = 0,
    VICTORY_STATE_PENDING = 1,
    VICTORY_STATE_WON = 2,
    VICTORY_STATE_LOST = 3
};

struct WinLoss_Status {
    int32_t mission_type;
    int32_t team_type[PLAYER_TEAM_MAX];
    int32_t team_status[PLAYER_TEAM_MAX];
    int32_t team_rank[PLAYER_TEAM_MAX];
    int32_t teams_total;
    int32_t teams_in_play;
    int32_t teams_humans_in_play;
    int32_t teams_non_computers;
};

WinLoss_Status WinLoss_EvaluateStatus(const int32_t turn_counter);

#endif /* WINLOSS_HPP */
