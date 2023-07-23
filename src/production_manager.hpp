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
    uint16_t team;
    SmartPointer<Complex> complex;
    Cargo cargo1;
    Cargo cargo2;
    Cargo cargo_resource_reserves;
    Cargo cargo4;
    Cargo cargo_mining_capacity;
    uint16_t total_resource_mining_capacity;
    SmartPointer<UnitInfo> selected_unit;
    SmartObjectArray<ResourceID> units;
    bool is_player_team;
    char buffer[800];
    uint16_t power_station_count;
    uint16_t power_generator_count;
    uint16_t power_station_active;
    uint16_t power_generator_active;

    void UpdateUnitPowerConsumption(UnitInfo* unit, uint16_t* generator_count, uint16_t* generator_active);
    void UpdatePowerConsumption(UnitInfo* unit);
    void AddCargo(int32_t type, int32_t amount);
    int32_t SatisfyCargoDemand(int32_t type, int32_t amount);
    int32_t BalanceMining(int32_t type1, int32_t type2, int32_t amount);
    void ComposeIndustryMessage(UnitInfo* unit, const char* format1, const char* format2, const char* material);
    bool PowerOn(ResourceID unit_type);
    void SatisfyPowerDemand(int32_t amount);
    void UpdateUnitWorkerConsumption(UnitInfo* unit);
    bool SatisfyIndustryPower(UnitInfo* unit, bool mode);
    static void ComposeResourceMessage(char* buffer, int32_t new_value, int32_t old_value, const char* format1,
                                       const char* format2);

    friend bool ProductionManager_ManageFactories(uint16_t team, Complex* complex);
    friend void ProductionManager_ManageMining(uint16_t team, Complex* complex, UnitInfo* unit,
                                               bool is_player_team);

public:
    ProductionManager(uint16_t team, Complex* complex);
    ~ProductionManager();

    void CheckGenerators();
    void ManagePower();
    int32_t CheckCargoNeed(int32_t type, int32_t amount, bool mode);
    bool CheckPowerNeed(int32_t amount);
    void UpdateLifeConsumption();
    bool CheckIndustry(UnitInfo* unit, bool mode);
    bool OptimizeIndustry(bool mode);
    bool OptimizeMiningIndustry();
    bool OptimizeAuxilaryIndustry(ResourceID unit_type, bool mode);
    void DrawResourceMessage();
};

bool ProductionManager_ManageFactories(uint16_t team, Complex* complex);
void ProductionManager_ManageMining(uint16_t team, Complex* complex, UnitInfo* unit, bool is_player_team);

#endif /* PRODUCTION_MANAGER_HPP */
