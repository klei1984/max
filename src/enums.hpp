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

#ifndef ENUMS_HPP
#define ENUMS_HPP

#include "sortedenum.hpp"

enum {
    GROUND_COVER = 0x1,
    EXPLODING = 0x2,
    ANIMATED = 0x4,
    CONNECTOR_UNIT = 0x8,
    BUILDING = 0x10,
    MISSILE_UNIT = 0x20,
    MOBILE_AIR_UNIT = 0x40,
    MOBILE_SEA_UNIT = 0x80,
    MOBILE_LAND_UNIT = 0x100,
    STATIONARY = 0x200,
    HASH_1 = 0x400,
    HASH_2 = 0x800,
    HASH_3 = 0x1000,
    HASH_4 = 0x2000,
    UPGRADABLE = 0x4000,
    HOVERING = 0x10000,
    HAS_FIRING_SPRITE = 0x20000,
    FIRES_MISSILES = 0x40000,
    CONSTRUCTOR_UNIT = 0x80000,
    ELECTRONIC_UNIT = 0x200000,
    SELECTABLE = 0x400000,
    STANDALONE = 0x800000,
    REQUIRES_SLAB = 0x1000000,
    TURRET_SPRITE = 0x2000000,
    SENTRY_UNIT = 0x4000000,
    SPINNING_TURRET = 0x8000000,
    REGENERATING_UNIT = 0x10000000
};

enum : unsigned char { LAND = 0x1, WATER = 0x2 };

enum : unsigned char { NO_CARGO = 0x0, MATERIALS = 0x1, FUEL = 0x2, GOLD = 0x3 };

extern SortedEnum Enums_UnitType;
extern SortedEnum Enums_ResearchTopic;
extern SortedEnum Enums_Sound;
extern SortedEnum Enums_Orders;
extern SortedEnum Enums_States;
extern SortedEnum Enums_LayingState;
extern SortedEnum Enums_Cursor;
extern SortedEnum Enums_EngineWeaponComm;

#endif /* ENUMS_HPP */
