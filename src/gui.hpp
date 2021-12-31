/* Copyright (c) 2021 M.A.X. Port Team
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

#ifndef GUI_HPP
#define GUI_HPP

#include "unitinfo.hpp"

extern char GUI_PlayerTeamIndex;
extern char GUI_GameState;

enum : unsigned char {
    GAME_STATE_0 = 0x0,
    GAME_STATE_1 = 0x1,
    GAME_STATE_2 = 0x2,
    GAME_STATE_3_MAIN_MENU = 0x3,
    GAME_STATE_4 = 0x4,
    GAME_STATE_5 = 0x5,
    GAME_STATE_6 = 0x6,
    GAME_STATE_7_SITE_SELECT = 0x7,
    GAME_STATE_8_IN_GAME = 0x8,
    GAME_STATE_9 = 0x9,
    GAME_STATE_10 = 0xA,
    GAME_STATE_11 = 0xB,
    GAME_STATE_12 = 0xC,
    GAME_STATE_13 = 0xD,
    GAME_STATE_14 = 0xE,
    GAME_STATE_15_FATAL_ERROR = 0xF
};

bool GUI_OKCancelMenu(const char* caption, bool mode);
bool GUI_DesyncMenu();
bool GUI_SelfDestructMenu();
unsigned short GUI_TransferMenu(UnitInfo* unit);

#endif /* GUI_HPP */
