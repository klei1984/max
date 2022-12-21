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

#ifndef PRODUCTION_MANAGER_HPP
#define PRODUCTION_MANAGER_HPP

#include "unitinfo.hpp"

class ProductionManager {
    unsigned short team;
    SmartPointer<Complex> complex;
    Cargo cargo1;
    Cargo cargo2;
    Cargo cargo_resource_reserves;
    Cargo cargo4;
    Cargo cargo_mining_capacity;
    unsigned short total_resource_mining_capacity;
    SmartPointer<UnitInfo> selected_unit;
    SmartObjectArray<ResourceID> units;
    bool is_player_team;
    char buffer[800];
    unsigned short power_station_count;
    unsigned short power_generator_count;
    unsigned short power_station_active;
    unsigned short power_generator_active;

    void UpdateUnitPowerConsumption(UnitInfo* unit, unsigned short* generator_count, unsigned short* generator_active);
    void UpdatePowerConsumption(UnitInfo* unit);
    void AddCargo(int type, int amount);
    int SatisfyCargoDemand(int type, int amount);
    int BalanceMining(int type1, int type2, int amount);
    void ComposeIndustryMessage(UnitInfo* unit, const char* format1, const char* format2, const char* material);
    bool PowerOn(ResourceID unit_type);
    void SatisfyPowerDemand(int amount);
    void UpdateUnitWorkerConsumption(UnitInfo* unit);
    bool SatisfyIndustryPower(UnitInfo* unit, bool mode);
    static void ComposeResourceMessage(char* buffer, int new_value, int old_value, const char* format1,
                                       const char* format2);

    friend bool ProductionManager_ManageFactories(unsigned short team, Complex* complex);
    friend void ProductionManager_ManageMining(unsigned short team, Complex* complex, UnitInfo* unit,
                                               bool is_player_team);

public:
    ProductionManager(unsigned short team, Complex* complex);
    ~ProductionManager();

    void CheckGenerators();
    void ManagePower();
    int CheckCargoNeed(int type, int amount, bool mode);
    bool CheckPowerNeed(int amount);
    void UpdateLifeConsumption();
    bool CheckIndustry(UnitInfo* unit, bool mode);
    bool OptimizeIndustry(bool mode);
    bool OptimizeMiningIndustry();
    bool OptimizeAuxilaryIndustry(ResourceID unit_type, bool mode);
    void DrawResourceMessage();
};

bool ProductionManager_ManageFactories(unsigned short team, Complex* complex);
void ProductionManager_ManageMining(unsigned short team, Complex* complex, UnitInfo* unit, bool is_player_team);

#endif /* PRODUCTION_MANAGER_HPP */
