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

#ifndef TASKACTIVATE_HPP
#define TASKACTIVATE_HPP

#include "task.hpp"

class TaskActivate : public Task {
    SmartPointer<UnitInfo> unit_to_activate;
    SmartPointer<UnitInfo> unit_parent;
    SmartPointer<Zone> zone;

    void Activate();

public:
    TaskActivate(UnitInfo* unit, Task* task, UnitInfo* parent);
    ~TaskActivate();

    bool IsUnitTransferable(UnitInfo& unit);
    char* WriteStatusLog(char* buffer) const;
    Rect* GetBounds(Rect* bounds);
    uint8_t GetType() const;
    void AddUnit(UnitInfo& unit);
    void Init();
    void EndTurn();
    bool Execute(UnitInfo& unit);
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);
    void EventZoneCleared(Zone* zone, bool status);

    [[nodiscard]] const UnitInfo* GetContainer() const noexcept;
    [[nodiscard]] const UnitInfo* GetPayload() const noexcept;
};

#endif /* TASKACTIVATE_HPP */
