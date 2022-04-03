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

#include "units_manager.hpp"

Cargo::Cargo() { Init(); }

void Cargo::Init() {
    gold = 0;
    raw = 0;
    fuel = 0;
    power = 0;
    life = 0;
    field_10 = 0;
}

Cargo* Cargo_GetCargo(UnitInfo* unit, Cargo* cargo) {
    cargo->Init();

    switch (UnitsManager_BaseUnits[unit->unit_type].cargo_type) {
        case MATERIALS:
            cargo->raw += unit->storage;
            break;
        case FUEL:
            cargo->fuel += unit->storage;
            break;
        case GOLD:
            cargo->gold += unit->storage;
            break;
        default:
            SDL_assert(0);
            break;
    }

    return cargo;
}

Cargo* Cargo_GetCargoCapacity(UnitInfo* unit, Cargo* cargo) {
    cargo->Init();

    switch (UnitsManager_BaseUnits[unit->unit_type].cargo_type) {
        case MATERIALS:
            cargo->raw += unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
            break;
        case FUEL:
            cargo->fuel += unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
            break;
        case GOLD:
            cargo->gold += unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
            break;
        default:
            SDL_assert(0);
            break;
    }

    return cargo;
}

Cargo& Cargo::operator+=(Cargo const& other) {
    gold += other.gold;
    raw += other.raw;
    fuel += other.fuel;
    power += other.power;
    life += other.life;
    field_10 += other.field_10;

    return *this;
}

int Cargo_GetRawConsumptionRate(ResourceID unit_type, int speed_multiplier) {
    int multiplier;
    int result;

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

int Cargo_GetFuelConsumptionRate(ResourceID unit_type) {
    int result;

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

int Cargo_GetPowerConsumptionRate(ResourceID unit_type) {
    int result;

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

int Cargo_GetLifeConsumptionRate(ResourceID unit_type) {
    int result;

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

int Cargo_GetGoldConsumptionRate(ResourceID unit_type) {
    int result;

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
