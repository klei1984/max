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

#include "cargo.hpp"

#include "settings.hpp"
#include "unit.hpp"
#include "units_manager.hpp"

static void Cargo_ApplyUnitConsumption(const ResourceID unit_type, const int32_t build_speed_multiplier, Cargo& cargo);

Cargo Cargo_GetInventory(UnitInfo* const unit) {
    Cargo cargo;

    switch (ResourceManager_GetUnit(unit->GetUnitType()).GetCargoType()) {
        case Unit::CARGO_TYPE_RAW: {
            cargo.raw += unit->storage;
        } break;

        case Unit::CARGO_TYPE_FUEL: {
            cargo.fuel += unit->storage;
        } break;

        case Unit::CARGO_TYPE_GOLD: {
            cargo.gold += unit->storage;
        } break;
    }

    return cargo;
}

Cargo Cargo_GetCargoCapacity(UnitInfo* const unit) {
    Cargo cargo;

    switch (ResourceManager_GetUnit(unit->GetUnitType()).GetCargoType()) {
        case Unit::CARGO_TYPE_RAW: {
            cargo.raw += unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
        } break;

        case Unit::CARGO_TYPE_FUEL: {
            cargo.fuel += unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
        } break;

        case Unit::CARGO_TYPE_GOLD: {
            cargo.gold += unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
        } break;
    }

    return cargo;
}

Cargo Cargo_GetNetProduction(UnitInfo* const unit, const bool current_order) {
    uint8_t orders;
    int32_t difficulty_factor;
    int32_t opponent;
    Cargo cargo;

    orders = unit->GetOrder();

    if (unit->GetOrderState() == ORDER_STATE_INIT && !current_order) {
        orders = unit->GetPriorOrder();
    }

    switch (unit->GetUnitType()) {
        case SHIPYARD:
        case LIGHTPLT:
        case LANDPLT:
        case TRAINHAL:
        case AIRPLT: {
            if (orders == ORDER_BUILD && unit->GetOrderState() != ORDER_STATE_UNIT_READY) {
                Cargo_ApplyUnitConsumption(unit->GetUnitType(), unit->GetMaxAllowedBuildRate(), cargo);
            }
        } break;

        case COMMTWR:
        case POWERSTN:
        case POWGEN:
        case HABITAT:
        case RESEARCH:
        case GREENHSE: {
            if (orders == ORDER_POWER_ON) {
                Cargo_ApplyUnitConsumption(unit->GetUnitType(), 1, cargo);
            }

        } break;

        case MININGST: {
            if (orders == ORDER_POWER_ON || orders == ORDER_NEW_ALLOCATE) {
                difficulty_factor = 4;

                if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) {
                    opponent = ResourceManager_GetSettings()->GetNumericValue("opponent");

                    if (opponent == OPPONENT_TYPE_MASTER) {
                        difficulty_factor = 5;
                    } else if (opponent == OPPONENT_TYPE_GOD) {
                        difficulty_factor = 6;
                    }
                }

                cargo.gold += (unit->gold_mining * difficulty_factor) / 4;
                cargo.raw += (unit->raw_mining * difficulty_factor) / 4;
                cargo.fuel += (unit->fuel_mining * difficulty_factor) / 4;
                cargo.free_capacity += 16 - unit->total_mining;

                Cargo_ApplyUnitConsumption(unit->GetUnitType(), 1, cargo);
            }
        } break;
    }

    return cargo;
}

void Cargo_ApplyUnitConsumption(ResourceID const unit_type, const int32_t build_speed_multiplier, Cargo& cargo) {
    cargo.raw -= Cargo_GetRawConsumptionRate(unit_type, build_speed_multiplier);
    cargo.fuel -= Cargo_GetFuelConsumptionRate(unit_type);
    cargo.power -= Cargo_GetPowerConsumptionRate(unit_type);
    cargo.life -= Cargo_GetLifeConsumptionRate(unit_type);
    cargo.gold -= Cargo_GetGoldConsumptionRate(unit_type);
}

int32_t Cargo_GetRawConsumptionRate(const ResourceID unit_type, const int32_t speed_multiplier) {
    int32_t multiplier;
    int32_t result;

    switch (speed_multiplier) {
        case 2: {
            multiplier = 4;
        } break;

        case 4: {
            multiplier = 12;
        } break;

        default: {
            multiplier = speed_multiplier;
        } break;
    }

    switch (unit_type) {
        case TRAINHAL: {
            result = multiplier;
        } break;

        case ENGINEER:
        case CONSTRCT: {
            result = multiplier * 2;
        } break;

        case SHIPYARD:
        case LANDPLT:
        case AIRPLT:
        case LIGHTPLT: {
            result = multiplier * 3;
        } break;

        default: {
            result = 0;
        } break;
    }

    return result;
}

int32_t Cargo_GetFuelConsumptionRate(const ResourceID unit_type) {
    int32_t result;

    switch (unit_type) {
        case POWERSTN: {
            result = 6;
        } break;

        case POWGEN: {
            result = 2;
        } break;

        default: {
            result = 0;
        } break;
    }

    return result;
}

int32_t Cargo_GetPowerConsumptionRate(const ResourceID unit_type) {
    int32_t result;

    switch (unit_type) {
        case SHIPYARD:
        case LANDPLT:
        case AIRPLT:
        case LIGHTPLT:
        case TRAINHAL:
        case GREENHSE:
        case RESEARCH:
        case COMMTWR:
        case MININGST: {
            result = 1;
        } break;

        case POWERSTN: {
            result = -6;
        } break;

        case POWGEN: {
            result = -1;
        } break;

        default: {
            result = 0;
        } break;
    }

    return result;
}

int32_t Cargo_GetLifeConsumptionRate(const ResourceID unit_type) {
    int32_t result;

    switch (unit_type) {
        case HABITAT: {
            result = -3;
        } break;

        case TRAINHAL:
        case GREENHSE:
        case RESEARCH: {
            result = 1;
        } break;

        default: {
            result = 0;
        } break;
    }

    return result;
}

int32_t Cargo_GetGoldConsumptionRate(const ResourceID unit_type) {
    int32_t result;

    switch (unit_type) {
        case COMMTWR: {
            result = 5;
        } break;

        default: {
            result = 0;
        } break;
    }

    return result;
}

void Cargo_UpdateResourceLevels(UnitInfo* const unit, const int32_t factor) {
    SmartPointer<Complex> complex = unit->GetComplex();

    complex->material += unit->GetRawConsumptionRate() * factor;
    complex->fuel += Cargo_GetFuelConsumptionRate(unit->GetUnitType()) * factor;
    complex->power += Cargo_GetPowerConsumptionRate(unit->GetUnitType()) * factor;
    complex->workers += Cargo_GetLifeConsumptionRate(unit->GetUnitType()) * factor;
    complex->gold += Cargo_GetGoldConsumptionRate(unit->GetUnitType()) * factor;
}
