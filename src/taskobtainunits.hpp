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

#ifndef TASKOBTAINUNITS_HPP
#define TASKOBTAINUNITS_HPP

#include "task.hpp"

class TaskObtainUnits : public Task {
    SmartObjectArray<ResourceID> units;
    Point point;
    bool init;
    bool reinit;

    bool IsValidCandidate(UnitInfo* unit, bool mode);
    void RequestUnits(ResourceID unit_type, uint16_t team, int32_t requested_amount, Point site);

public:
    TaskObtainUnits(Task* task, Point point);
    ~TaskObtainUnits();

    uint32_t CountInstancesOfUnitType(ResourceID unit_type);
    UnitInfo* FindUnit(ResourceID unit_type, bool mode);

    uint16_t GetPriority() const;
    std::string WriteStatusLog() const;
    uint8_t GetType() const;
    bool IsNeeded();
    void AddUnit(UnitInfo& unit);
    void Init();
    void BeginTurn();
    void EndTurn();
    void RemoveSelf();

    void AddUnit(ResourceID unit_type);
};

#endif /* TASKOBTAINUNITS_HPP */
