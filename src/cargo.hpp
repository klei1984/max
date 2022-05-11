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

#ifndef CARGO_HPP
#define CARGO_HPP

#include "enums.hpp"

class UnitInfo;

struct Cargo {
    Cargo();

    short gold;
    short raw;
    short fuel;
    short power;
    short life;
    short field_10;

    void Init();

    friend Cargo* Cargo_GetCargo(UnitInfo* unit, Cargo* cargo);
    friend Cargo* Cargo_GetCargoCapacity(UnitInfo* unit, Cargo* cargo);
    friend Cargo* Cargo_GetCargoDemand(UnitInfo* unit, Cargo* cargo, bool current_order);

    Cargo& operator+=(Cargo const& other);
};

Cargo* Cargo_GetCargo(UnitInfo* unit, Cargo* cargo);
Cargo* Cargo_GetCargoCapacity(UnitInfo* unit, Cargo* cargo);
Cargo* Cargo_GetCargoDemand(UnitInfo* unit, Cargo* cargo, bool current_order = false);

int Cargo_GetRawConsumptionRate(ResourceID unit_type, int speed_multiplier);
int Cargo_GetFuelConsumptionRate(ResourceID unit_type);
int Cargo_GetPowerConsumptionRate(ResourceID unit_type);
int Cargo_GetLifeConsumptionRate(ResourceID unit_type);
int Cargo_GetGoldConsumptionRate(ResourceID unit_type);
void Cargo_UpdateResourceLevels(UnitInfo* unit, int factor);

#endif /* CARGO_HPP */
