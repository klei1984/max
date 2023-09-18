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
    int16_t gold;
    int16_t raw;
    int16_t fuel;
    int16_t power;
    int16_t life;
    int16_t free_capacity;

    Cargo() noexcept { Init(); }
    ~Cargo() noexcept {}

    inline void Init() noexcept {
        gold = 0;
        raw = 0;
        fuel = 0;
        power = 0;
        life = 0;
        free_capacity = 0;
    }

    inline Cargo& operator+=(const Cargo& other) noexcept {
        gold += other.gold;
        raw += other.raw;
        fuel += other.fuel;
        power += other.power;
        life += other.life;
        free_capacity += other.free_capacity;
        return *this;
    }

    inline Cargo& operator-=(const Cargo& other) noexcept {
        gold -= other.gold;
        raw -= other.raw;
        fuel -= other.fuel;
        power -= other.power;
        life -= other.life;
        free_capacity -= other.free_capacity;
        return *this;
    }

    [[nodiscard]] inline int32_t Get(int32_t type) const noexcept {
        int32_t result;
        switch (type) {
            case CARGO_MATERIALS: {
                result = raw;
            } break;
            case CARGO_FUEL: {
                result = fuel;
            } break;
            case CARGO_GOLD: {
                result = gold;
            } break;
            default: {
                result = 0;
            } break;
        }
        return result;
    }
};

Cargo Cargo_GetInventory(UnitInfo* const unit);
Cargo Cargo_GetCargoCapacity(UnitInfo* const unit);
Cargo Cargo_GetNetProduction(UnitInfo* const unit, const bool current_order = false);

int32_t Cargo_GetRawConsumptionRate(const ResourceID unit_type, const int32_t speed_multiplier);
int32_t Cargo_GetFuelConsumptionRate(const ResourceID unit_type);
int32_t Cargo_GetPowerConsumptionRate(const ResourceID unit_type);
int32_t Cargo_GetLifeConsumptionRate(const ResourceID unit_type);
int32_t Cargo_GetGoldConsumptionRate(const ResourceID unit_type);
void Cargo_UpdateResourceLevels(UnitInfo* const unit, const int32_t factor);

#endif /* CARGO_HPP */
