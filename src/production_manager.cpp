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

#include "access.hpp"
#include "allocmenu.hpp"
#include "message_manager.hpp"
#include "remote.hpp"
#include "units_manager.hpp"

ProductionManager::ProductionManager(unsigned short team, Complex* complex) {
    this->team = team;
    this->complex = complex;

    is_player_team = false;

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

            if ((*it).unit_type == MININGST && ((*it).orders == ORDER_POWER_ON || (*it).orders == ORDER_NEW_ALLOCATE)) {
                cargo_mining_capacity.raw += std::min(static_cast<int>((*it).raw_mining_max), 16);
                cargo_mining_capacity.fuel += std::min(static_cast<int>((*it).fuel_mining_max), 16);
                cargo_mining_capacity.gold += std::min(static_cast<int>((*it).gold_mining_max), 16);

                total_resource_mining_capacity = std::min(
                    static_cast<int>((*it).raw_mining_max + (*it).fuel_mining_max + (*it).gold_mining_max), 16);
            }
        }
    }

    cargo1 = cargo_resource_reserves;
}

ProductionManager::~ProductionManager() {}

void ProductionManager::UpdateUnitPowerConsumption(UnitInfo* unit, unsigned short* generator_count,
                                                   unsigned short* generator_active) {
    if (unit->orders != ORDER_DISABLE) {
        if (unit->orders == ORDER_POWER_ON) {
            if (*generator_active > 0) {
                --*generator_active;

            } else {
                ComposeIndustryMessage(unit, "Cannot turn %s on, %s needed.\n", "More %s needed, %s turned off.\n",
                                       "fuel");
                UnitsManager_SetNewOrder(unit, ORDER_POWER_OFF, ORDER_STATE_0);
            }

        } else {
            if (*generator_active == *generator_count) {
                ComposeIndustryMessage(unit, "Cannot turn %s off, other buildings need %s.\n",
                                       "More %s needed, %s turned on.\n", "power");
                UnitsManager_SetNewOrder(unit, ORDER_POWER_ON, ORDER_STATE_0);
                --*generator_active;
            }
        }

        --*generator_count;
    }
}

void ProductionManager::UpdatePowerConsumption(UnitInfo* unit) {
    if (complex == unit->GetComplex() && Cargo_GetPowerConsumptionRate(unit->unit_type) < 0) {
        if (unit->unit_type == POWGEN) {
            UpdateUnitPowerConsumption(unit, &power_generator_count, &power_generator_active);

        } else {
            UpdateUnitPowerConsumption(unit, &power_station_count, &power_station_active);
        }
    }
}

void ProductionManager::AddCargo(int type, int amount) {
    if (amount != 0) {
        switch (type) {
            case CARGO_MATERIALS: {
                cargo_resource_reserves.raw += amount;
                cargo4.raw += amount;
            } break;

            case CARGO_FUEL: {
                cargo_resource_reserves.fuel += amount;
                cargo4.fuel += amount;
            } break;

            case CARGO_GOLD: {
                cargo_resource_reserves.gold += amount;
                cargo4.gold += amount;
            } break;
        }
    }
}

int ProductionManager::SatisfyCargoDemand(int type, int amount) {
    int available = cargo_resource_reserves.Get(type);
    int result;

    if (amount > available) {
        amount = available;
    }

    available = cargo4.Get(type);

    if (amount > available) {
        amount = available;
    }

    if (amount > 0) {
        AllocMenu_AdjustForDemands(&*complex, type, amount);
        AddCargo(type, -amount);

        result = amount;

    } else {
        result = 0;
    }

    return result;
}

int ProductionManager::BalanceMining(int type1, int type2, int amount) {
    int result;

    if (type1 != type2 && amount > 0) {
        switch (type1) {
            case CARGO_MATERIALS: {
                amount = std::min(amount, static_cast<int>(cargo_mining_capacity.raw - cargo_resource_reserves.raw));
            } break;

            case CARGO_FUEL: {
                amount = std::min(amount, static_cast<int>(cargo_mining_capacity.fuel - cargo_resource_reserves.fuel));
            } break;

            case CARGO_GOLD: {
                amount = std::min(amount, static_cast<int>(cargo_mining_capacity.gold - cargo_resource_reserves.gold));
            } break;
        }

        switch (type2) {
            case CARGO_MATERIALS: {
                amount = std::min(amount, static_cast<int>(cargo_resource_reserves.raw));
            } break;

            case CARGO_FUEL: {
                amount = std::min(amount, static_cast<int>(cargo_resource_reserves.fuel));
            } break;

            case CARGO_GOLD: {
                amount = std::min(amount, static_cast<int>(cargo_resource_reserves.gold));
            } break;
        }

        if (amount > 0) {
            int mining_total;

            amount = SatisfyCargoDemand(type2, amount);
            mining_total = AllocMenu_Optimize(&*complex, type1, amount, type1);
            AddCargo(type1, mining_total);

            if (mining_total < amount) {
                AllocMenu_Optimize(&*complex, type2, amount - mining_total, type2);
                AddCargo(type2, amount - mining_total);

                amount = mining_total;
            }

            result = amount;

        } else {
            result = 0;
        }

    } else {
        result = 0;
    }

    return result;
}

void ProductionManager::ComposeIndustryMessage(UnitInfo* unit, const char* format1, const char* format2,
                                               const char* material) {
    if (is_player_team && units->Find(&unit->unit_type) == -1) {
        char text[300];

        units.PushBack(&unit->unit_type);

        if (selected_unit == unit) {
            sprintf(text, format1, UnitsManager_BaseUnits[unit->unit_type].singular_name, material);

        } else {
            if (buffer[0] == '\0') {
                strcpy(buffer, "Adjustments made:\n");
            }

            sprintf(text, format2, material, UnitsManager_BaseUnits[unit->unit_type].singular_name);
        }

        strcat(buffer, text);
    }
}

bool ProductionManager::PowerOn(ResourceID unit_type) {
    bool result;

    if (Cargo_GetFuelConsumptionRate(unit_type) > cargo4.fuel) {
        if (units->Find(&unit_type) == -1) {
            char text[300];

            units.PushBack(&unit_type);

            sprintf(text, "Cannot turn %s on, %s needed.\n", UnitsManager_BaseUnits[unit_type].singular_name, "fuel");
            strcat(buffer, text);
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

void ProductionManager::SatisfyPowerDemand(int amount) {
    CheckGenerators();

    if (is_player_team && cargo4.power < amount) {
        if (power_generator_count <= power_generator_active || !PowerOn(POWGEN)) {
            if (power_station_count > power_station_active) {
                PowerOn(POWERSTN);
            }
        }
    }
}

void ProductionManager::UpdateUnitWorkerConsumption(UnitInfo* unit) {
    if (complex == unit->GetComplex() && Cargo_GetLifeConsumptionRate(unit->unit_type) < 0 &&
        unit->orders != ORDER_DISABLE && unit->orders != ORDER_POWER_ON && unit->state != ORDER_STATE_0 &&
        CheckPowerNeed(Cargo_GetPowerConsumptionRate(unit->unit_type))) {
        int life_consumption_rate = Cargo_GetLifeConsumptionRate(unit->unit_type);

        cargo4.life -= life_consumption_rate;
        cargo_resource_reserves.life -= life_consumption_rate;

        ComposeIndustryMessage(unit, "Cannot turn %s off, other buildings need %s.\n",
                               "More %s needed, %s turned on.\n", "workers");

        UnitsManager_SetNewOrder(unit, ORDER_POWER_ON, ORDER_STATE_0);
    }
}

bool ProductionManager::SatisfyIndustryPower(UnitInfo* unit, bool mode) {
    bool result;

    if (complex == unit->GetComplex() && unit->orders != ORDER_POWER_OFF && unit->orders != ORDER_DISABLE &&
        unit->orders != ORDER_IDLE) {
        Cargo demand;

        Cargo_GetCargoDemand(unit, &demand, true);

        if (demand.raw < 0 || demand.fuel < 0 || demand.gold < 0 || demand.power < 0 || demand.life < 0) {
            if (mode || (demand.raw < 0 && cargo4.raw < 0) || (demand.fuel < 0 && cargo4.fuel < 0) ||
                (demand.gold < 0 && cargo4.gold < 0)) {
                bool power_needed = false;
                const char* material;

                if (demand.raw < 0 && cargo4.raw < 0) {
                    material = "raw material";

                } else if (demand.fuel < 0 && cargo4.fuel < 0) {
                    material = "fuel";

                } else if (demand.gold < 0 && cargo4.gold < 0) {
                    material = "gold";

                } else if (demand.power < 0 && (cargo4.power < 0 || cargo4.fuel < 0)) {
                    material = "power";
                    power_needed = true;

                } else if (demand.life < 0 && cargo4.life < 0) {
                    material = "workers";

                } else if (demand.raw < 0) {
                    material = "raw material";

                } else if (demand.fuel < 0) {
                    material = "fuel";

                } else if (demand.gold < 0) {
                    material = "gold";

                } else if (demand.power < 0) {
                    material = "power";
                    power_needed = true;

                } else {
                    material = "debugging :P";
                }

                ComposeIndustryMessage(unit, "Cannot turn %s on, %s needed.\n", "More %s needed, %s turned off.\n",
                                       material);

                cargo4 -= demand;

                if (power_needed) {
                    SatisfyPowerDemand(-demand.power);
                }

                UnitsManager_SetNewOrder(unit, ORDER_POWER_OFF, ORDER_STATE_0);

                result = true;

            } else {
                result = false;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void ProductionManager::ComposeResourceMessage(char* buffer_, int new_value, int old_value, const char* format1,
                                               const char* format2) {
    char text[300];

    if (new_value >= old_value) {
        if (new_value > old_value) {
            sprintf(text, format2, new_value);
            strcat(buffer_, text);
        }

    } else {
        sprintf(text, format1, new_value);
        strcat(buffer_, text);
    }
}

void ProductionManager::CheckGenerators() {
    int power_generator_fuel_consumption_rate = Cargo_GetFuelConsumptionRate(POWGEN);
    int power_generator_power_consumption_rate = -Cargo_GetPowerConsumptionRate(POWGEN);
    int power_station_fuel_consumption_rate = Cargo_GetFuelConsumptionRate(POWERSTN);
    int power_station_power_consumption_rate = -Cargo_GetPowerConsumptionRate(POWERSTN);
    int power;
    int count;

    cargo4.fuel += power_generator_fuel_consumption_rate * power_generator_active +
                   power_station_fuel_consumption_rate * power_station_active;

    cargo4.power -= cargo_resource_reserves.power;

    power = -cargo4.power;

    count = (power_generator_power_consumption_rate * power_station_fuel_consumption_rate) /
            power_generator_fuel_consumption_rate;

    power_station_active =
        (power + power_station_power_consumption_rate - count) % power_station_power_consumption_rate;

    if (power_station_active > power_station_count) {
        power_station_active = power_station_count;
    }

    power -= power_station_active * power_station_power_consumption_rate;

    if (power < 0) {
        power = 0;
    }

    power_generator_active =
        (power + power_generator_power_consumption_rate - 1) / power_generator_power_consumption_rate;

    if (power_generator_active > power_generator_count) {
        power_generator_active = power_generator_count;
    }

    power -= power_generator_active * power_generator_power_consumption_rate;

    if (power > 0) {
        int count2;

        count2 = (power + power_station_power_consumption_rate - 1) / power_station_power_consumption_rate;

        if (count2 > power_station_count - power_station_active) {
            count2 = power_station_count - power_station_active;
        }

        power_station_active = count2;

        power = -cargo4.power - power_station_active * power_station_power_consumption_rate;

        if (power < 0) {
            power = 0;
        }

        power_generator_active =
            (power + power_generator_power_consumption_rate - 1) / power_generator_power_consumption_rate;

        if (power_generator_active > power_generator_count) {
            power_generator_active = power_generator_count;
        }
    }

    cargo_resource_reserves.power = power_station_active * power_station_power_consumption_rate +
                                    power_generator_active * power_generator_power_consumption_rate;

    cargo4.power += cargo_resource_reserves.power;

    cargo4.fuel -= power_station_active * power_station_fuel_consumption_rate +
                   power_generator_active * power_generator_fuel_consumption_rate;
}

void ProductionManager::ManagePower() {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if (selected_unit != &*it) {
            UpdatePowerConsumption(&*it);
        }
    }

    if (selected_unit) {
        UpdatePowerConsumption(&*selected_unit);
    }
}

int ProductionManager::CheckCargoNeed(int type, int amount, bool mode) {
    int result;

    if (amount > 0) {
        int mining_total;
        int mining_gold;
        int mining_raw;
        int mining_fuel;

        switch (type) {
            case CARGO_MATERIALS: {
                mining_total = cargo_mining_capacity.raw - cargo_resource_reserves.raw;
            } break;

            case CARGO_FUEL: {
                mining_total = cargo_mining_capacity.fuel - cargo_resource_reserves.fuel;
            } break;

            case CARGO_GOLD: {
                mining_total = cargo_mining_capacity.gold - cargo_resource_reserves.gold;
            } break;
        }

        if (mining_total > 0 &&
            total_resource_mining_capacity <
                (cargo_resource_reserves.raw + cargo_resource_reserves.fuel + cargo_resource_reserves.gold)) {
            mining_total = AllocMenu_Optimize(&*complex, type, amount, type);

        } else {
            mining_total = 0;
        }

        AddCargo(type, mining_total);

        amount -= mining_total;

        mining_gold = BalanceMining(type, CARGO_GOLD, amount);

        amount -= mining_gold;

        mining_raw = BalanceMining(type, CARGO_MATERIALS, amount);

        amount -= mining_raw;

        mining_fuel = BalanceMining(type, CARGO_FUEL, amount);

        amount -= mining_fuel;

        mining_total += mining_raw + mining_fuel + mining_gold;

        if (mining_total == 0 || (amount != 0 && !mode)) {
            AllocMenu_AdjustForDemands(&*complex, type, mining_total);
            AddCargo(type, -mining_total);

            AllocMenu_Optimize(&*complex, CARGO_GOLD, mining_gold, CARGO_GOLD);
            AllocMenu_Optimize(&*complex, CARGO_MATERIALS, mining_raw, CARGO_MATERIALS);
            AllocMenu_Optimize(&*complex, CARGO_FUEL, mining_fuel, CARGO_FUEL);

            AddCargo(CARGO_GOLD, mining_gold);
            AddCargo(CARGO_MATERIALS, mining_raw);
            AddCargo(CARGO_FUEL, mining_fuel);

            result = 0;

        } else {
            result = mining_total;
        }

    } else {
        result = 1;
    }

    return result;
}

bool ProductionManager::CheckPowerNeed(int amount) {
    bool result;
    cargo4.power -= amount;

    CheckGenerators();

    if (cargo4.fuel < 0) {
        CheckCargoNeed(CARGO_FUEL, -cargo4.fuel, false);
    }

    if (cargo4.fuel >= 0 && cargo4.power >= 0) {
        result = true;

    } else {
        cargo4.power += amount;

        CheckGenerators();

        result = false;
    }

    return result;
}

void ProductionManager::UpdateLifeConsumption() {
    if (selected_unit) {
        UpdateUnitWorkerConsumption(&*selected_unit);
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End() && cargo4.life < 0; ++it) {
        UpdateUnitWorkerConsumption(&*it);
    }
}

bool ProductionManager::CheckIndustry(UnitInfo* unit, bool mode) {
    bool result;

    if (complex == unit->GetComplex() && unit->orders == ORDER_BUILD && unit->orders != ORDER_STATE_13 &&
        unit->orders != ORDER_STATE_46 && Cargo_GetRawConsumptionRate(unit->unit_type, 1) > 0 &&
        (mode || cargo4.raw < 0)) {
        Cargo cargo;

        Cargo_GetCargoDemand(unit, &cargo, true);

        if (cargo.life < 0 && cargo4.life < 0) {
            ComposeIndustryMessage(unit, "Cannot turn %s on, %s needed.\n", "More %s needed, %s turned off.\n",
                                   "workers");

        } else if (mode) {
            ComposeIndustryMessage(unit, "Cannot turn %s on, %s needed.\n", "More %s needed, %s turned off.\n",
                                   "power");

        } else {
            ComposeIndustryMessage(unit, "Cannot turn %s on, %s needed.\n", "More %s needed, %s turned off.\n",
                                   "raw material");
        }

        cargo4 -= cargo;

        if (mode) {
            SatisfyPowerDemand(Cargo_GetPowerConsumptionRate(unit->unit_type));
        }

        UnitsManager_SetNewOrder(unit, ORDER_BUILD, ORDER_STATE_46);

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool ProductionManager::OptimizeIndustry(bool mode) {
    if (selected_unit && CheckIndustry(&*selected_unit, mode)) {
        return true;

    } else {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if (CheckIndustry(&*it, mode)) {
                return true;
            }
        }
    }

    return false;
}

bool ProductionManager::OptimizeMiningIndustry() {
    SmartPointer<UnitInfo> mine;
    Cargo minimum_demand;
    Cargo demand;
    bool result;

    if (selected_unit && selected_unit->orders != ORDER_POWER_OFF && selected_unit->orders != ORDER_DISABLE &&
        selected_unit->orders != ORDER_IDLE && selected_unit->unit_type == MININGST) {
        mine = selected_unit;

        Cargo_GetCargoDemand(&*selected_unit, &minimum_demand, true);

    } else {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if (complex == (*it).GetComplex() && (*it).orders != ORDER_POWER_OFF && (*it).orders != ORDER_DISABLE &&
                (*it).orders != ORDER_IDLE && (*it).unit_type == MININGST) {
                Cargo_GetCargoDemand(&*it, &demand, true);

                if (!mine || demand.fuel < minimum_demand.fuel) {
                    mine = &*it;
                    minimum_demand = demand;
                }
            }
        }
    }

    if (mine) {
        cargo_resource_reserves.raw -= mine->raw_mining;
        cargo_resource_reserves.fuel -= mine->fuel_mining;
        cargo_resource_reserves.gold -= mine->gold_mining;

        cargo4 -= minimum_demand;

        UnitsManager_SetNewOrder(&*mine, ORDER_POWER_OFF, ORDER_STATE_0);

        ComposeIndustryMessage(&*mine, "Cannot turn %s on, %s needed.\n", "More %s needed, %s turned off.\n", "power");

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool ProductionManager::OptimizeAuxilaryIndustry(ResourceID unit_type, bool mode) {
    if (selected_unit && selected_unit->unit_type == unit_type && SatisfyIndustryPower(&*selected_unit, mode)) {
        return true;
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).unit_type == unit_type && SatisfyIndustryPower(&*it, mode)) {
            return true;
        }
    }

    return false;
}

void ProductionManager::DrawResourceMessage() {
    if (buffer[0] == '\0') {
        strcpy(buffer, "Adjustments made:\n");
    }

    ComposeResourceMessage(buffer, cargo_resource_reserves.raw, cargo1.raw, "Raw material mining reduced to %i.\n",
                           "Raw material mining increased to %i.\n");

    ComposeResourceMessage(buffer, cargo_resource_reserves.fuel, cargo1.fuel, "Fuel mining reduced to %i.\n",
                           "Fuel mining increased to %i.\n");

    ComposeResourceMessage(buffer, cargo_resource_reserves.gold, cargo1.gold, "Gold mining reduced to %i.\n",
                           "Gold mining increased to %i.\n");

    MessageManager_DrawMessage(buffer, 1, 0, false, true);
}

bool ProductionManager_ManageFactories(unsigned short team, Complex* complex) {
    ProductionManager manager(team, complex);
    bool is_found = false;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End() && manager.cargo4.raw > 0; ++it) {
        if ((*it).GetComplex() == complex && (*it).orders == ORDER_HALT_BUILDING_2 && (*it).storage == 0 &&
            Cargo_GetRawConsumptionRate((*it).unit_type, (*it).GetMaxAllowedBuildRate()) <= manager.cargo4.raw &&
            Cargo_GetLifeConsumptionRate((*it).unit_type) <= manager.cargo4.life &&
            manager.CheckPowerNeed(Cargo_GetPowerConsumptionRate((*it).unit_type))) {
            manager.cargo4.raw -= Cargo_GetRawConsumptionRate((*it).unit_type, (*it).GetMaxAllowedBuildRate());
            manager.cargo4.life -= Cargo_GetLifeConsumptionRate((*it).unit_type);

            (*it).BuildOrder();

            is_found = true;
        }
    }

    manager.ManagePower();

    return is_found;
}

void ProductionManager_ManageMining(unsigned short team, Complex* complex, UnitInfo* unit, bool is_player_team) {
    if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_REMOTE) {
        Access_UpdateResourcesTotal(complex);

        ProductionManager manager(team, complex);

        if (manager.cargo4.raw >= 0 && manager.cargo4.fuel >= 0 && manager.cargo4.gold >= 0 &&
            manager.cargo4.power >= 0 && manager.cargo4.life >= 0) {
            is_player_team = false;
        }

        manager.selected_unit = unit;
        manager.is_player_team = is_player_team;

        manager.CheckGenerators();

        while (manager.cargo4.raw < 0 || manager.cargo4.fuel < 0 || manager.cargo4.power < 0 ||
               manager.cargo4.life < 0 || manager.cargo4.gold < 0) {
            manager.CheckPowerNeed(0);

            if (manager.cargo4.raw < 0) {
                manager.CheckCargoNeed(CARGO_MATERIALS, -manager.cargo4.raw, false);
            }

            if (manager.cargo4.fuel < 0) {
                manager.CheckCargoNeed(CARGO_FUEL, -manager.cargo4.fuel, false);
            }

            if (manager.cargo4.gold < 0) {
                manager.CheckCargoNeed(CARGO_GOLD, -manager.cargo4.gold, false);
            }

            bool flag = false;

            if ((manager.cargo4.raw < 0 || manager.cargo4.fuel < 0 || manager.cargo4.gold < 0) &&
                (manager.OptimizeIndustry(false) || manager.OptimizeAuxilaryIndustry(COMMTWR, false))) {
                flag = true;
            }

            if (!flag) {
                if (manager.cargo4.life < 0) {
                    manager.UpdateLifeConsumption();
                }

                if (manager.cargo4.raw < 0 || manager.cargo4.fuel < 0 || manager.cargo4.power < 0 ||
                    manager.cargo4.life < 0 || manager.cargo4.gold < 0) {
                    if ((!manager.selected_unit || !manager.CheckIndustry(&*manager.selected_unit, true)) &&
                        !manager.OptimizeAuxilaryIndustry(RESEARCH, true) &&
                        !manager.OptimizeAuxilaryIndustry(GREENHSE, true) &&
                        !manager.OptimizeAuxilaryIndustry(COMMTWR, true) && !manager.OptimizeIndustry(true) &&
                        !manager.OptimizeMiningIndustry()) {
                        return;
                    }
                }
            }
        }

        manager.CheckGenerators();
        manager.ManagePower();

        Access_UpdateResourcesTotal(complex);

        if (Remote_IsNetworkGame && GameManager_PlayerTeam == team) {
            Remote_SendNetPacket_11(team, complex);
        }

        if (is_player_team) {
            manager.DrawResourceMessage();
        }
    }
}
