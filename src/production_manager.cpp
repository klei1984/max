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

#include "production_manager.hpp"

#include "units_manager.hpp"

bool ProductionManager_OptimizeBuilding(unsigned short team, Complex* complex) {
    /// \todo
}

void ProductionManager_OptimizeMining(unsigned short team, Complex* complex, UnitInfo* unit, bool is_player_team) {
    /// \todo
}

ProductionManager::ProductionManager(unsigned short team, Complex* complex) {
    this->team = team;
    this->complex = complex;

    mode = false;

    buffer[0] = '\0';

    power_generator_active = 0;
    power_generator_count = 0;
    power_station_active = 0;
    power_station_count = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if (this->complex == (*it).GetComplex()) {
            Cargo cargo;

            cargo2 += *Cargo_GetCargo(&(*it), &cargo);
            cargo4 += cargo;
            cargo4 += *Cargo_GetCargoDemand(&(*it), &cargo, true);

            if (cargo.raw > 0) {
                cargo_resource_reserves.raw += cargo.raw;
            }

            if (cargo.fuel > 0) {
                cargo_resource_reserves.fuel += cargo.fuel;
            }

            if (cargo.gold > 0) {
                cargo_resource_reserves.gold += cargo.gold;
            }

            if (cargo.power > 0) {
                cargo_resource_reserves.power += cargo.power;
            }

            if (cargo.life > 0) {
                cargo_resource_reserves.life += cargo.life;
            }

            if ((*it).unit_type == POWGEN && (*it).orders != ORDER_DISABLE) {
                if ((*it).orders == ORDER_POWER_ON) {
                    ++power_generator_active;
                }

                ++power_generator_count;
            }

            if ((*it).unit_type == POWERSTN && (*it).orders != ORDER_DISABLE) {
                if ((*it).orders == ORDER_POWER_ON) {
                    ++power_station_active;
                }

                ++power_station_count;
            }

            if ((*it).unit_type == MININGST &&
                ((*it).orders == ORDER_POWER_ON || (*it).orders == ORDER_NEW_ALLOCATE)) {
                cargo_mining_capacity.raw = std::min(static_cast<int>((*it).raw_mining_max), 16);
                cargo_mining_capacity.fuel = std::min(static_cast<int>((*it).fuel_mining_max), 16);
                cargo_mining_capacity.gold = std::min(static_cast<int>((*it).gold_mining_max), 16);

                total_resource_mining_capacity = std::min(
                    static_cast<int>((*it).raw_mining_max + (*it).fuel_mining_max + (*it).gold_mining_max), 16);
            }
        }
    }

    cargo1 = cargo_resource_reserves;
}

ProductionManager::~ProductionManager() {}
