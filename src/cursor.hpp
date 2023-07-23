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

#ifndef CURSOR_HPP
#define CURSOR_HPP

#include "unitinfo.hpp"

enum : uint8_t {
    CURSOR_HIDDEN,
    CURSOR_HAND,
    CURSOR_ARROW_N,
    CURSOR_ARROW_NE,
    CURSOR_ARROW_E,
    CURSOR_ARROW_SE,
    CURSOR_ARROW_S,
    CURSOR_ARROW_SW,
    CURSOR_ARROW_W,
    CURSOR_ARROW_NW,
    CURSOR_MAP,
    CURSOR_MINI,
    CURSOR_REPAIR,
    CURSOR_TRANSFER,
    CURSOR_FUEL,
    CURSOR_RELOAD,
    CURSOR_LOAD,
    CURSOR_FRIEND,
    CURSOR_ENEMY,
    CURSOR_FAR_TARGET,
    CURSOR_UNIT_GO,
    CURSOR_UNIT_NO_GO,
    CURSOR_WAY,
    CURSOR_GROUP,
    CURSOR_ACTIVATE,
    CURSOR_MAP2,
    CURSOR_STEAL,
    CURSOR_DISABLE,
    CURSOR_PATH,
    CURSOR_HELP
};

void Cursor_Init();
uint8_t Cursor_GetCursor();
uint8_t Cursor_GetDefaultWindowCursor(uint8_t window_index);
void Cursor_SetCursor(uint8_t cursor_index);
void Cursor_DrawAttackPowerCursor(UnitInfo* selected_unit, UnitInfo* target_unit, uint8_t cursor_index);
void Cursor_DrawStealthActionChanceCursor(int32_t experience_level, uint8_t cursor_index);

#endif /* CURSOR_HPP */
