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

#ifndef TEAMUNITS_HPP
#define TEAMUNITS_HPP

#include "complex.hpp"
#include "resource_manager.hpp"
#include "smartlist.hpp"
#include "unitvalues.hpp"

struct AbstractUnit {
    AbstractUnit(unsigned int flags, unsigned short sprite, unsigned short shadows, unsigned short data,
                 unsigned short flics, unsigned short portrait, unsigned short icon, unsigned short armory_portrait,
                 unsigned short field_18, unsigned char cargo_type, unsigned char land_type, char new_gender,
                 const char* singular_name, const char* plural_name, const char* description,
                 const char* tutorial = "");

    unsigned int flags;

    unsigned short data;

    unsigned int field_6;

    unsigned short flics;
    unsigned short portrait;
    unsigned short icon;
    unsigned short armory_portrait;
    unsigned short field_18;

    unsigned char cargo_type;
    unsigned char land_type;
    unsigned char gender;

    const char* singular_name;
    const char* plural_name;
    const char* description;
    const char* tutorial;

    unsigned short sprite;
    unsigned short shadows;
};

class BaseUnit {
public:
    BaseUnit();
    ~BaseUnit();
};

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

class TeamUnits {
    unsigned short color_field_0;
    unsigned short color_field_2;
    unsigned short gold;
    SmartPointer<UnitValues> base_values[UNIT_END];
    SmartPointer<UnitValues> current_values[UNIT_END];
    SmartList<Complex> complexes;

    static int GetParam(char* string, int* offset);

public:
    TeamUnits();
    ~TeamUnits();

    void Init();

    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);
    void TextLoad(TextStructure& object);
    void TextSave(SmartTextfileWriter& file);

    int WriteComplexPacket(unsigned short complex_id, void* buffer);
    void ReadComplexPacket(void* buffer);

    unsigned short GetGold();
    void SetGold(unsigned short value);

    Complex* CreateComplex();
    Complex* GetComplex(unsigned short complex_id);
    void sub_7F3DC(unsigned short team);
    void RemoveComplex(Complex& object);
    void ClearComplexes();

    UnitValues* GetBaseUnitValues(unsigned short id);
    void SetBaseUnitValues(unsigned short id, UnitValues& object);

    UnitValues* GetCurrentUnitValues(unsigned short id);
    void SetCurrentUnitValues(unsigned short id, UnitValues& object);
};

#endif /* TEAMUNITS_HPP */
