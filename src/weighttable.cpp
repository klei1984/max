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

#include <algorithm>

#include "builder.hpp"
#include "gnw.h"
#include "randomizer.hpp"

WeightTable& WeightTable::operator=(WeightTable const& other) {
    if (this != &other) {
        m_entries = other.m_entries;
        m_team = other.m_team;
    }

    return *this;
}

WeightTable& WeightTable::operator+=(WeightTable const& other) {
    for (const auto& entry : other.m_entries) {
        // Find existing entry with same unit_type
        auto it = std::find_if(m_entries.begin(), m_entries.end(),
                               [&entry](const UnitWeight& e) { return e.unit_type == entry.unit_type; });

        if (it != m_entries.end()) {
            it->weight += entry.weight;
        } else {
            m_entries.push_back(entry);
        }
    }

    return *this;
}

bool WeightTable::Add(ResourceID unit_type, uint16_t weight) {
    if (Builder_IsBuildable(m_team, unit_type)) {
        m_entries.emplace_back(unit_type, weight);
        return true;
    }

    return false;
}

void WeightTable::PushBack(const UnitWeight& entry) { m_entries.push_back(entry); }

void WeightTable::Disable(uint32_t index) {
    if (index < m_entries.size()) {
        if (index < m_entries.size() - 1) {
            std::swap(m_entries[index], m_entries.back());
        }

        m_entries.pop_back();
    }
}

void WeightTable::Compact() {
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                                   [](const UnitWeight& e) { return e.weight == 0 || e.unit_type == INVALID_ID; }),
                    m_entries.end());
}

void WeightTable::Clear() { m_entries.clear(); }

ResourceID WeightTable::RollUnitType() const {
    int32_t total_weight = 0;

    for (const auto& entry : m_entries) {
        total_weight += entry.weight;
    }

    if (total_weight > 0) {
        int32_t roll = Randomizer_Generate(total_weight) + 1;

        for (const auto& entry : m_entries) {
            roll -= entry.weight;

            if (roll <= 0) {
                return entry.unit_type;
            }
        }
    }

    return INVALID_ID;
}

int32_t WeightTable::GetWeight(ResourceID unit_type) const {
    for (const auto& entry : m_entries) {
        if (entry.unit_type == unit_type) {
            return entry.weight;
        }
    }

    return 0;
}
