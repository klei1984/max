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
class NetPacket;

class TeamUnits {
    uint32_t gold;
    SmartPointer<UnitValues> base_values[UNIT_END];
    SmartPointer<UnitValues> current_values[UNIT_END];
    SmartList<Complex> complexes;

public:
    TeamUnits();
    ~TeamUnits();

    void Init();

    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);

    void WriteComplexPacket(uint16_t complex_id, NetPacket& packet);
    void ReadComplexPacket(NetPacket& packet);

    uint32_t GetGold();
    void SetGold(uint32_t value);

    Complex* CreateComplex();
    SmartList<Complex>& GetComplexes();
    Complex* GetComplex(uint16_t complex_id);
    void OptimizeComplexes(uint16_t team);
    void RemoveComplex(Complex& object);
    void ClearComplexes();

    UnitValues* GetBaseUnitValues(uint16_t id);
    void SetBaseUnitValues(uint16_t id, UnitValues& object);

    UnitValues* GetCurrentUnitValues(uint16_t id);
    void SetCurrentUnitValues(uint16_t id, UnitValues& object);

    uint16_t hash_team_id;
    ColorIndex* color_index_table;
};

int32_t TeamUnits_GetUpgradeCost(uint16_t team, ResourceID unit_type, int32_t attribute);
int32_t TeamUnits_UpgradeOffsetFactor(uint16_t team, ResourceID unit_type, int32_t attribute);

extern TeamUnits ResourceManager_TeamUnitsRed;
extern TeamUnits ResourceManager_TeamUnitsGreen;
extern TeamUnits ResourceManager_TeamUnitsBlue;
extern TeamUnits ResourceManager_TeamUnitsGray;

#endif /* TEAMUNITS_HPP */
