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

struct CTInfo;

struct BaseUnitDataFile {
    char image_base;
    char image_count;
    char turret_image_base;
    char turret_image_count;
    char firing_image_base;
    char firing_image_count;
    char connector_image_base;
    char connector_image_count;
    PathStep angle_offsets[8];
};

static_assert(sizeof(struct BaseUnitDataFile) == 24, "The structure needs to be packed.");

struct AbstractUnit {
    AbstractUnit(unsigned int flags, ResourceID sprite, ResourceID shadows, ResourceID data, ResourceID flics,
                 ResourceID portrait, ResourceID icon, ResourceID armory_portrait, ResourceID field_18,
                 unsigned char cargo_type, unsigned char land_type, char new_gender, const char* singular_name,
                 const char* plural_name, const char* description, const char* tutorial = "");

    unsigned int flags;

    ResourceID data;

    unsigned int field_6;

    ResourceID flics;
    ResourceID portrait;
    ResourceID icon;
    ResourceID armory_portrait;
    ResourceID field_18;

    unsigned char land_type;
    unsigned char cargo_type;
    unsigned char gender;

    const char* singular_name;
    const char* plural_name;
    const char* description;
    const char* tutorial;

    ResourceID sprite;
    ResourceID shadows;
};

struct BaseUnit {
    BaseUnit();
    void Init(AbstractUnit* unit);

    unsigned int flags;
    ResourceID data;
    unsigned char* data_buffer;
    ResourceID flics;
    ResourceID portrait;
    ResourceID icon;
    ResourceID armory_portrait;
    ResourceID field_18;
    unsigned char land_type;
    unsigned char cargo_type;
    unsigned char gender;
    const char* singular_name;
    const char* plural_name;
    const char* description;
    const char* tutorial;
    unsigned char* sprite;
    unsigned char* shadows;
    char* field_47;
};

class TeamUnits {
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

    unsigned short hash_team_id;
    ColorIndex* color_index_table;
};

int TeamUnits_GetUpgradeCost(unsigned short team, ResourceID unit_type, int attribute);
int TeamUnits_UpgradeOffsetFactor(unsigned short team, ResourceID unit_type, int attribute);

#endif /* TEAMUNITS_HPP */
