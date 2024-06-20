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
#include "localization.hpp"
#include "message_manager.hpp"
#include "remote.hpp"
#include "units_manager.hpp"

class ProductionManager {
    uint16_t team;
    SmartPointer<Complex> complex;
    Cargo prev_net_production;
    Cargo inventory;
    Cargo net_production;
    Cargo total;
    Cargo production_capacity;
    uint16_t combined_production_capacity;
    uint16_t power_station_count;
    uint16_t power_generator_count;
    uint16_t power_station_active;
    uint16_t power_generator_active;
    SmartPointer<UnitInfo> selected_unit;
    SmartObjectArray<ResourceID> units;
    bool show_messages;
    char buffer[800];

    void UpdateUnitPowerConsumption(UnitInfo* unit, uint16_t* generator_count, uint16_t* generator_active);
    void UpdatePowerConsumption(UnitInfo* unit);
    void ChangeProduction(const int32_t type, const int32_t amount);
    int32_t FreeUpProductionCapacity(const int32_t type, int32_t amount);
    int32_t SwapProduction(const int32_t type_to_increase, const int32_t type_to_reduce, int32_t amount);
    void ComposeIndustryMessage(UnitInfo* const unit, const char* format1, const char* format2, const char* material);
    bool PowerOn(ResourceID unit_type);
    void SatisfyPowerDemand(int32_t amount);
    void UpdateUnitLifeConsumption(UnitInfo* unit);
    bool ValidateAuxilaryIndustry(UnitInfo* const unit, const bool forceful_shutoff);
    bool ValidateIndustry(UnitInfo* const unit, const bool mode);
    static void ComposeResourceMessage(char* buffer, int32_t new_value, int32_t old_value, const char* format1,
                                       const char* format2);

    friend bool ProductionManager_UpdateIndustryOrders(const uint16_t team, Complex* const complex);
    friend void ProductionManager_OptimizeProduction(const uint16_t team, Complex* const complex, UnitInfo* const unit,
                                                     bool show_messages);

public:
    ProductionManager(const uint16_t team, Complex* const complex);
    ~ProductionManager();

    void OptimizePowerProduction();
    void OptimizePowerConsumption();
    int32_t OptimizeCargoProduction(const int32_t type, int32_t amount, const bool mode);
    bool CheckPowerNeed(const int32_t amount);
    void UpdateLifeConsumption();
    bool OptimizeIndustry(const bool mode);
    bool OptimizeAuxilaryIndustry(const ResourceID unit_type, const bool forceful_shutoff);
    bool OptimizeMiningIndustry();
    void DrawResourceMessage();
};

ProductionManager::ProductionManager(const uint16_t team, Complex* const complex) {
    this->team = team;
    this->complex = complex;

    show_messages = false;

    buffer[0] = '\0';

    power_generator_active = 0;
    power_generator_count = 0;
    power_station_active = 0;
    power_station_count = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if (this->complex == (*it).GetComplex()) {
            Cargo cargo;

            cargo = Cargo_GetInventory(it->Get());

            inventory += cargo;
            total += cargo;

            cargo = Cargo_GetNetProduction(it->Get(), true);

            total += cargo;

            if (cargo.raw > 0) {
                net_production.raw += cargo.raw;
            }

            if (cargo.fuel > 0) {
                net_production.fuel += cargo.fuel;
            }

            if (cargo.gold > 0) {
                net_production.gold += cargo.gold;
            }

            if (cargo.power > 0) {
                net_production.power += cargo.power;
            }

            if (cargo.life > 0) {
                net_production.life += cargo.life;
            }

            if ((*it).GetUnitType() == POWGEN && (*it).GetOrder() != ORDER_DISABLE) {
                if ((*it).GetOrder() == ORDER_POWER_ON) {
                    ++power_generator_active;
                }

                ++power_generator_count;
            }

            if ((*it).GetUnitType() == POWERSTN && (*it).GetOrder() != ORDER_DISABLE) {
                if ((*it).GetOrder() == ORDER_POWER_ON) {
                    ++power_station_active;
                }

                ++power_station_count;
            }

            if ((*it).GetUnitType() == MININGST &&
                ((*it).GetOrder() == ORDER_POWER_ON || (*it).GetOrder() == ORDER_NEW_ALLOCATE)) {
                production_capacity.raw += std::min(static_cast<int32_t>((*it).raw_mining_max), 16);
                production_capacity.fuel += std::min(static_cast<int32_t>((*it).fuel_mining_max), 16);
                production_capacity.gold += std::min(static_cast<int32_t>((*it).gold_mining_max), 16);

                combined_production_capacity = std::min(
                    static_cast<int32_t>((*it).raw_mining_max + (*it).fuel_mining_max + (*it).gold_mining_max), 16);
            }
        }
    }

    prev_net_production = net_production;
}

ProductionManager::~ProductionManager() {}

void ProductionManager::UpdateUnitPowerConsumption(UnitInfo* unit, uint16_t* generator_count,
                                                   uint16_t* generator_active) {
    if (unit->GetOrder() != ORDER_DISABLE) {
        if (unit->GetOrder() == ORDER_POWER_ON) {
            if (*generator_active > 0) {
                --*generator_active;

            } else {
                ComposeIndustryMessage(unit, _(cdcc), _(b9d9), _(acbe));
                UnitsManager_SetNewOrder(unit, ORDER_POWER_OFF, ORDER_STATE_INIT);
            }

        } else {
            if (*generator_active == *generator_count) {
                ComposeIndustryMessage(unit, _(cfb8), _(d25b), _(07d7));
                UnitsManager_SetNewOrder(unit, ORDER_POWER_ON, ORDER_STATE_INIT);
                --*generator_active;
            }
        }

        --*generator_count;
    }
}

void ProductionManager::UpdatePowerConsumption(UnitInfo* unit) {
    if (complex == unit->GetComplex() && Cargo_GetPowerConsumptionRate(unit->GetUnitType()) < 0) {
        if (unit->GetUnitType() == POWGEN) {
            UpdateUnitPowerConsumption(unit, &power_generator_count, &power_generator_active);

        } else {
            UpdateUnitPowerConsumption(unit, &power_station_count, &power_station_active);
        }
    }
}

void ProductionManager::ChangeProduction(const int32_t type, const int32_t amount) {
    if (amount != 0) {
        switch (type) {
            case CARGO_MATERIALS: {
                net_production.raw += amount;
                total.raw += amount;
            } break;

            case CARGO_FUEL: {
                net_production.fuel += amount;
                total.fuel += amount;
            } break;

            case CARGO_GOLD: {
                net_production.gold += amount;
                total.gold += amount;
            } break;
        }
    }
}

int32_t ProductionManager::FreeUpProductionCapacity(const int32_t type, int32_t amount) {
    int32_t available = net_production.Get(type);
    int32_t result;

    if (amount > available) {
        amount = available;
    }

    available = total.Get(type);

    if (amount > available) {
        amount = available;
    }

    if (amount > 0) {
        AllocMenu_ReduceProduction(complex.Get(), type, amount);
        ChangeProduction(type, -amount);

        result = amount;

    } else {
        result = 0;
    }

    return result;
}

int32_t ProductionManager::SwapProduction(const int32_t type_to_increase, const int32_t type_to_reduce,
                                          int32_t amount) {
    int32_t result;

    if (type_to_increase != type_to_reduce && amount > 0) {
        switch (type_to_increase) {
            case CARGO_MATERIALS: {
                amount = std::min(amount, static_cast<int32_t>(production_capacity.raw - net_production.raw));
            } break;

            case CARGO_FUEL: {
                amount = std::min(amount, static_cast<int32_t>(production_capacity.fuel - net_production.fuel));
            } break;

            case CARGO_GOLD: {
                amount = std::min(amount, static_cast<int32_t>(production_capacity.gold - net_production.gold));
            } break;
        }

        switch (type_to_reduce) {
            case CARGO_MATERIALS: {
                amount = std::min(amount, static_cast<int32_t>(net_production.raw));
            } break;

            case CARGO_FUEL: {
                amount = std::min(amount, static_cast<int32_t>(net_production.fuel));
            } break;

            case CARGO_GOLD: {
                amount = std::min(amount, static_cast<int32_t>(net_production.gold));
            } break;
        }

        if (amount > 0) {
            amount = FreeUpProductionCapacity(type_to_reduce, amount);

            const int32_t supplied_amount =
                AllocMenu_Optimize(complex.Get(), type_to_increase, amount, type_to_increase);

            ChangeProduction(type_to_increase, supplied_amount);

            if (supplied_amount < amount) {
                AllocMenu_Optimize(complex.Get(), type_to_reduce, amount - supplied_amount, type_to_reduce);
                ChangeProduction(type_to_reduce, amount - supplied_amount);

                amount = supplied_amount;
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

void ProductionManager::ComposeIndustryMessage(UnitInfo* const unit, const char* format1, const char* format2,
                                               const char* material) {
    auto unit_type{unit->GetUnitType()};

    if (show_messages && units->Find(&unit_type) == -1) {
        char text[300];

        units.PushBack(&unit_type);

        if (selected_unit == unit) {
            sprintf(text, format1, UnitsManager_BaseUnits[unit->GetUnitType()].singular_name, material);

        } else {
            if (buffer[0] == '\0') {
                strcpy(buffer, _(b441));
            }

            sprintf(text, format2, material, UnitsManager_BaseUnits[unit->GetUnitType()].singular_name);
        }

        strcat(buffer, text);
    }
}

bool ProductionManager::PowerOn(ResourceID unit_type) {
    bool result;

    if (Cargo_GetFuelConsumptionRate(unit_type) > total.fuel) {
        if (units->Find(&unit_type) == -1) {
            char text[300];

            units.PushBack(&unit_type);

            sprintf(text, _(0aea), UnitsManager_BaseUnits[unit_type].singular_name, _(ff10));
            strcat(buffer, text);
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

void ProductionManager::SatisfyPowerDemand(int32_t amount) {
    OptimizePowerProduction();

    if (show_messages && total.power < amount) {
        if (power_generator_count <= power_generator_active || !PowerOn(POWGEN)) {
            if (power_station_count > power_station_active) {
                PowerOn(POWERSTN);
            }
        }
    }
}

void ProductionManager::UpdateUnitLifeConsumption(UnitInfo* unit) {
    if (complex == unit->GetComplex() && Cargo_GetLifeConsumptionRate(unit->GetUnitType()) < 0 &&
        unit->GetOrder() != ORDER_DISABLE && unit->GetOrder() != ORDER_POWER_ON &&
        unit->GetOrderState() != ORDER_STATE_INIT &&
        CheckPowerNeed(Cargo_GetPowerConsumptionRate(unit->GetUnitType()))) {
        int32_t life_consumption_rate = Cargo_GetLifeConsumptionRate(unit->GetUnitType());

        total.life -= life_consumption_rate;
        net_production.life -= life_consumption_rate;

        ComposeIndustryMessage(unit, _(5fd6), _(0a4b), _(ed57));

        UnitsManager_SetNewOrder(unit, ORDER_POWER_ON, ORDER_STATE_INIT);
    }
}

bool ProductionManager::ValidateAuxilaryIndustry(UnitInfo* const unit, const bool forceful_shutoff) {
    bool result;

    if (complex == unit->GetComplex() && unit->GetOrder() != ORDER_POWER_OFF && unit->GetOrder() != ORDER_DISABLE &&
        unit->GetOrder() != ORDER_IDLE) {
        Cargo production;

        production = Cargo_GetNetProduction(unit, true);

        if (production.raw < 0 || production.fuel < 0 || production.gold < 0 || production.power < 0 ||
            production.life < 0) {
            if (forceful_shutoff || (production.raw < 0 && total.raw < 0) || (production.fuel < 0 && total.fuel < 0) ||
                (production.gold < 0 && total.gold < 0)) {
                bool power_needed = false;
                const char* material;

                if (production.raw < 0 && total.raw < 0) {
                    material = _(9c91);

                } else if (production.fuel < 0 && total.fuel < 0) {
                    material = _(2162);

                } else if (production.gold < 0 && total.gold < 0) {
                    material = _(24d7);

                } else if (production.power < 0 && (total.power < 0 || total.fuel < 0)) {
                    material = _(f814);
                    power_needed = true;

                } else if (production.life < 0 && total.life < 0) {
                    material = _(73f7);

                } else if (production.raw < 0) {
                    material = _(2acf);

                } else if (production.fuel < 0) {
                    material = _(19b2);

                } else if (production.gold < 0) {
                    material = _(f260);

                } else if (production.power < 0) {
                    material = _(7a37);
                    power_needed = true;

                } else {
                    material = "debugging :P";
                }

                ComposeIndustryMessage(unit, _(961d), _(b215), material);

                total -= production;

                if (power_needed) {
                    SatisfyPowerDemand(-production.power);
                }

                UnitsManager_SetNewOrder(unit, ORDER_POWER_OFF, ORDER_STATE_INIT);

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

void ProductionManager::ComposeResourceMessage(char* buffer_, int32_t new_value, int32_t old_value, const char* format1,
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

void ProductionManager::OptimizePowerProduction() {
    const int32_t power_generator_fuel_consumption_rate = Cargo_GetFuelConsumptionRate(POWGEN);
    const int32_t power_generator_power_consumption_rate = -Cargo_GetPowerConsumptionRate(POWGEN);
    const int32_t power_station_fuel_consumption_rate = Cargo_GetFuelConsumptionRate(POWERSTN);
    const int32_t power_station_power_consumption_rate = -Cargo_GetPowerConsumptionRate(POWERSTN);

    total.fuel += power_generator_fuel_consumption_rate * power_generator_active +
                  power_station_fuel_consumption_rate * power_station_active;

    total.power -= net_production.power;

    int32_t power = -total.power;

    int32_t count = (power_generator_power_consumption_rate * power_station_fuel_consumption_rate) /
                    power_generator_fuel_consumption_rate;

    power_station_active =
        (power + power_station_power_consumption_rate - count) / power_station_power_consumption_rate;

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
        int32_t count2 = (power + power_station_power_consumption_rate - 1) / power_station_power_consumption_rate;

        if (count2 > power_station_count - power_station_active) {
            count2 = power_station_count - power_station_active;
        }

        power_station_active += count2;

        power = -total.power - power_station_active * power_station_power_consumption_rate;

        if (power < 0) {
            power = 0;
        }

        power_generator_active =
            (power + power_generator_power_consumption_rate - 1) / power_generator_power_consumption_rate;

        if (power_generator_active > power_generator_count) {
            power_generator_active = power_generator_count;
        }
    }

    net_production.power = power_station_active * power_station_power_consumption_rate +
                           power_generator_active * power_generator_power_consumption_rate;

    total.power += net_production.power;

    total.fuel -= power_station_active * power_station_fuel_consumption_rate +
                  power_generator_active * power_generator_fuel_consumption_rate;
}

void ProductionManager::OptimizePowerConsumption() {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if (selected_unit != it->Get()) {
            UpdatePowerConsumption(it->Get());
        }
    }

    if (selected_unit) {
        UpdatePowerConsumption(selected_unit.Get());
    }
}

int32_t ProductionManager::OptimizeCargoProduction(const int32_t type, int32_t amount, const bool mode) {
    int32_t result;

    if (amount > 0) {
        int32_t free_capacity{0};
        int32_t reduced_gold_mining{0};
        int32_t reduced_raw_mining{0};
        int32_t reduced_fuel_mining{0};

        switch (type) {
            case CARGO_MATERIALS: {
                free_capacity = production_capacity.raw - net_production.raw;
            } break;

            case CARGO_FUEL: {
                free_capacity = production_capacity.fuel - net_production.fuel;
            } break;

            case CARGO_GOLD: {
                free_capacity = production_capacity.gold - net_production.gold;
            } break;

            default: {
                SDL_assert(0);
            } break;
        }

        if (free_capacity > 0 &&
            combined_production_capacity < (net_production.raw + net_production.fuel + net_production.gold)) {
            free_capacity = AllocMenu_Optimize(complex.Get(), type, amount, type);

        } else {
            free_capacity = 0;
        }

        ChangeProduction(type, free_capacity);

        amount -= free_capacity;

        reduced_gold_mining = SwapProduction(type, CARGO_GOLD, amount);

        amount -= reduced_gold_mining;

        reduced_raw_mining = SwapProduction(type, CARGO_MATERIALS, amount);

        amount -= reduced_raw_mining;

        reduced_fuel_mining = SwapProduction(type, CARGO_FUEL, amount);

        amount -= reduced_fuel_mining;

        free_capacity += reduced_raw_mining + reduced_fuel_mining + reduced_gold_mining;

        if (free_capacity == 0 || (amount != 0 && !mode)) {
            AllocMenu_ReduceProduction(complex.Get(), type, free_capacity);
            ChangeProduction(type, -free_capacity);

            AllocMenu_Optimize(complex.Get(), CARGO_GOLD, reduced_gold_mining, CARGO_GOLD);
            AllocMenu_Optimize(complex.Get(), CARGO_MATERIALS, reduced_raw_mining, CARGO_MATERIALS);
            AllocMenu_Optimize(complex.Get(), CARGO_FUEL, reduced_fuel_mining, CARGO_FUEL);

            ChangeProduction(CARGO_GOLD, reduced_gold_mining);
            ChangeProduction(CARGO_MATERIALS, reduced_raw_mining);
            ChangeProduction(CARGO_FUEL, reduced_fuel_mining);

            result = 0;

        } else {
            result = free_capacity;
        }

    } else {
        result = 1;
    }

    return result;
}

bool ProductionManager::CheckPowerNeed(const int32_t amount) {
    bool result;
    total.power -= amount;

    OptimizePowerProduction();

    if (total.fuel < 0) {
        OptimizeCargoProduction(CARGO_FUEL, -total.fuel, false);
    }

    if (total.fuel >= 0 && total.power >= 0) {
        result = true;

    } else {
        total.power += amount;

        OptimizePowerProduction();

        result = false;
    }

    return result;
}

void ProductionManager::UpdateLifeConsumption() {
    if (selected_unit) {
        UpdateUnitLifeConsumption(selected_unit.Get());
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End() && total.life < 0; ++it) {
        UpdateUnitLifeConsumption(it->Get());
    }
}

bool ProductionManager::ValidateIndustry(UnitInfo* const unit, const bool mode) {
    bool result;

    if (complex == unit->GetComplex() && unit->GetOrder() == ORDER_BUILD && unit->GetOrderState() != ORDER_STATE_13 &&
        unit->GetOrderState() != ORDER_STATE_46 && unit->GetOrderState() != ORDER_STATE_UNIT_READY &&
        Cargo_GetRawConsumptionRate(unit->GetUnitType(), 1) > 0 && (mode || total.raw < 0)) {
        Cargo cargo = Cargo_GetNetProduction(unit, true);

        if (cargo.life < 0 && total.life < 0) {
            ComposeIndustryMessage(unit, _(d139), _(7516), _(0579));

        } else if (mode) {
            ComposeIndustryMessage(unit, _(87e8), _(9665), _(57f8));

        } else {
            ComposeIndustryMessage(unit, _(5f9a), _(ac9d), _(61e6));
        }

        total -= cargo;

        if (mode) {
            SatisfyPowerDemand(Cargo_GetPowerConsumptionRate(unit->GetUnitType()));
        }

        UnitsManager_SetNewOrder(unit, ORDER_BUILD, ORDER_STATE_46);

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool ProductionManager::OptimizeIndustry(const bool mode) {
    if (selected_unit && ValidateIndustry(selected_unit.Get(), mode)) {
        return true;

    } else {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if (ValidateIndustry(it->Get(), mode)) {
                return true;
            }
        }
    }

    return false;
}

bool ProductionManager::OptimizeMiningIndustry() {
    SmartPointer<UnitInfo> mine;
    Cargo minimum_demand;
    bool result;

    if (selected_unit && selected_unit->GetOrder() != ORDER_POWER_OFF && selected_unit->GetOrder() != ORDER_DISABLE &&
        selected_unit->GetOrder() != ORDER_IDLE && selected_unit->GetUnitType() == MININGST) {
        mine = selected_unit;

        minimum_demand = Cargo_GetNetProduction(selected_unit.Get(), true);

    } else {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if (complex == (*it).GetComplex() && (*it).GetOrder() != ORDER_POWER_OFF &&
                (*it).GetOrder() != ORDER_DISABLE && (*it).GetOrder() != ORDER_IDLE &&
                (*it).GetUnitType() == MININGST) {
                Cargo demand = Cargo_GetNetProduction(it->Get(), true);

                if (!mine || demand.fuel < minimum_demand.fuel) {
                    mine = it->Get();
                    minimum_demand = demand;
                }
            }
        }
    }

    if (mine) {
        net_production.raw -= mine->raw_mining;
        net_production.fuel -= mine->fuel_mining;
        net_production.gold -= mine->gold_mining;

        total -= minimum_demand;

        UnitsManager_SetNewOrder(mine.Get(), ORDER_POWER_OFF, ORDER_STATE_INIT);

        ComposeIndustryMessage(mine.Get(), _(2ff7), _(b8fe), _(5f24));

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool ProductionManager::OptimizeAuxilaryIndustry(const ResourceID unit_type, const bool forceful_shutoff) {
    if (selected_unit && selected_unit->GetUnitType() == unit_type &&
        ValidateAuxilaryIndustry(selected_unit.Get(), forceful_shutoff)) {
        return true;
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetUnitType() == unit_type && ValidateAuxilaryIndustry(it->Get(), forceful_shutoff)) {
            return true;
        }
    }

    return false;
}

void ProductionManager::DrawResourceMessage() {
    if (buffer[0] == '\0') {
        strcpy(buffer, _(3226));
    }

    ComposeResourceMessage(buffer, net_production.raw, prev_net_production.raw, _(3d58), _(61fa));

    ComposeResourceMessage(buffer, net_production.fuel, prev_net_production.fuel, _(f957), _(1dfe));

    ComposeResourceMessage(buffer, net_production.gold, prev_net_production.gold, _(1226), _(17c7));

    MessageManager_DrawMessage(buffer, 1, 0, false, true);
}

bool ProductionManager_UpdateIndustryOrders(const uint16_t team, Complex* const complex) {
    ProductionManager manager(team, complex);
    bool is_found = false;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End() && manager.total.raw > 0; ++it) {
        if ((*it).GetComplex() == complex && (*it).GetOrder() == ORDER_HALT_BUILDING_2 && (*it).storage == 0 &&
            Cargo_GetRawConsumptionRate((*it).GetUnitType(), (*it).GetMaxAllowedBuildRate()) <= manager.total.raw &&
            Cargo_GetLifeConsumptionRate((*it).GetUnitType()) <= manager.total.life &&
            manager.CheckPowerNeed(Cargo_GetPowerConsumptionRate((*it).GetUnitType()))) {
            manager.total.raw -= Cargo_GetRawConsumptionRate((*it).GetUnitType(), (*it).GetMaxAllowedBuildRate());
            manager.total.life -= Cargo_GetLifeConsumptionRate((*it).GetUnitType());

            (*it).BuildOrder();

            is_found = true;
        }
    }

    manager.OptimizePowerConsumption();

    return is_found;
}

void ProductionManager_OptimizeProduction(const uint16_t team, Complex* const complex, UnitInfo* const unit,
                                          bool show_messages) {
    if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_REMOTE) {
        Access_UpdateResourcesTotal(complex);

        ProductionManager manager(team, complex);

        if (manager.total.raw >= 0 && manager.total.fuel >= 0 && manager.total.gold >= 0 && manager.total.power >= 0 &&
            manager.total.life >= 0) {
            show_messages = false;
        }

        manager.selected_unit = unit;
        manager.show_messages = show_messages;

        manager.OptimizePowerProduction();

        while (manager.total.raw < 0 || manager.total.fuel < 0 || manager.total.power < 0 || manager.total.life < 0 ||
               manager.total.gold < 0) {
            manager.CheckPowerNeed(0);

            if (manager.total.raw < 0) {
                manager.OptimizeCargoProduction(CARGO_MATERIALS, -manager.total.raw, false);
            }

            if (manager.total.fuel < 0) {
                manager.OptimizeCargoProduction(CARGO_FUEL, -manager.total.fuel, false);
            }

            if (manager.total.gold < 0) {
                manager.OptimizeCargoProduction(CARGO_GOLD, -manager.total.gold, false);
            }

            bool flag = false;

            if ((manager.total.raw < 0 || manager.total.fuel < 0 || manager.total.gold < 0) &&
                (manager.OptimizeIndustry(false) || manager.OptimizeAuxilaryIndustry(COMMTWR, false))) {
                flag = true;
            }

            if (!flag) {
                if (manager.total.life < 0) {
                    manager.UpdateLifeConsumption();
                }

                if (manager.total.raw < 0 || manager.total.fuel < 0 || manager.total.power < 0 ||
                    manager.total.life < 0 || manager.total.gold < 0) {
                    if ((!manager.selected_unit || !manager.ValidateIndustry(manager.selected_unit.Get(), true)) &&
                        !manager.OptimizeAuxilaryIndustry(RESEARCH, true) &&
                        !manager.OptimizeAuxilaryIndustry(GREENHSE, true) &&
                        !manager.OptimizeAuxilaryIndustry(COMMTWR, true) && !manager.OptimizeIndustry(true) &&
                        !manager.OptimizeMiningIndustry()) {
                        return;
                    }
                }
            }
        }

        manager.OptimizePowerProduction();
        manager.OptimizePowerConsumption();

        Access_UpdateResourcesTotal(complex);

        if (Remote_IsNetworkGame && GameManager_PlayerTeam == team) {
            Remote_SendNetPacket_11(team, complex);
        }

        if (show_messages) {
            manager.DrawResourceMessage();
        }
    }
}
