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

#include "weighttable.hpp"

#include "builder.hpp"
#include "gnw.h"

UnitWeight::UnitWeight() : unit_type(INVALID_ID), weight(0) {}

UnitWeight::UnitWeight(ResourceID unit_type_, unsigned short weight_) {
    if (Builder_IsBuildable(unit_type_)) {
        unit_type = unit_type_;
        weight = weight_;

    } else {
        unit_type = INVALID_ID;
        weight = 0;
    }
}

WeightTable::WeightTable() {}

WeightTable::~WeightTable() {}

WeightTable::WeightTable(const WeightTable& other, bool deep_copy) : weight_table(other.weight_table, deep_copy) {}

WeightTable& WeightTable::operator=(WeightTable const& other) {
    weight_table = other.weight_table;
    return *this;
}

WeightTable& WeightTable::operator+=(WeightTable const& other) {
    ResourceID unit_type;
    int index;

    for (int i = 0; i < other.weight_table.GetCount(); ++i) {
        unit_type = other.weight_table[i]->unit_type;

        for (index = 0; index < weight_table.GetCount() && unit_type != weight_table[i]->unit_type; ++index) {
            ;
        }

        if (index == weight_table.GetCount()) {
            UnitWeight unit_weight(unit_type, 0);

            weight_table.PushBack(&unit_weight);
        }

        weight_table[index]->weight += other.weight_table[i]->weight;
    }

    return *this;
}

UnitWeight& WeightTable::operator[](unsigned short position) { return *weight_table[position]; }

int WeightTable::GetCount() const { return weight_table.GetCount(); }

ResourceID WeightTable::RollUnitType() const {
    int weight = 0;
    ResourceID unit_type = INVALID_ID;

    for (int i = 0; i < weight_table.GetCount(); ++i) {
        weight += weight_table[i]->weight;
    }

    if (weight > 0) {
        weight = ((dos_rand() * weight) >> 15) + 1;

        for (int i = 0; i < weight_table.GetCount(); ++i) {
            weight -= weight_table[i]->weight;

            if (weight <= 0) {
                unit_type = weight_table[i]->unit_type;
                break;
            }
        }
    }

    return unit_type;
}

void WeightTable::PushBack(UnitWeight& object) { weight_table.PushBack(&object); }

void WeightTable::Clear() { weight_table.Clear(); }

int WeightTable::GetWeight(ResourceID unit_type) const {
    int weight = 0;

    for (int i = 0; i < weight_table.GetCount(); ++i) {
        if (unit_type == weight_table[i]->unit_type) {
            weight = weight_table[i]->weight;
            break;
        }
    }

    return weight;
}
