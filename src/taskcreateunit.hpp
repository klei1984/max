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

#ifndef TASKCREATEUNIT_HPP
#define TASKCREATEUNIT_HPP

#include "taskcreate.hpp"

class TaskCreateUnit : public TaskCreate {
    Point site;
    uint8_t op_state;

    void WaitForMaterials();
    bool IsUnitStillNeeded();

public:
    TaskCreateUnit(ResourceID unit_type, Task* task, Point site);
    TaskCreateUnit(UnitInfo* unit, Task* task);
    ~TaskCreateUnit();

    uint16_t GetPriority() const;
    std::string WriteStatusLog() const;
    uint8_t GetType() const;

    void AddUnit(UnitInfo& unit);
    void Init();
    void BeginTurn();
    void EndTurn();
    bool Execute(UnitInfo& unit);
    void RemoveUnit(UnitInfo& unit);
    void EventZoneCleared(Zone* zone, bool status);

    bool Task_vfunc28();
    bool Task_vfunc29();
};

#endif /* TASKCREATEUNIT_HPP */
