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

#ifndef THREATMAP_HPP
#define THREATMAP_HPP

#include "unitinfo.hpp"

class ThreatMap {
    Point dimension;

    void Deinit();

public:
    ThreatMap();
    ~ThreatMap();

    void Init();

    static uint16_t GetRiskLevel(ResourceID unit_type);
    static uint16_t GetRiskLevel(UnitInfo *unit);
    void SetRiskLevel(uint8_t risk_level);

    void Update(int32_t armor);

    int16_t team;
    uint16_t id;
    uint8_t risk_level;
    uint8_t caution_level;
    bool for_attacking;
    int16_t armor;
    int16_t **damage_potential_map;
    int16_t **shots_map;
};

#endif /* THREATMAP_HPP */
