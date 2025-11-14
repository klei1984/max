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

#ifndef TASKKILLUNIT_HPP
#define TASKKILLUNIT_HPP

#include "spottedunit.hpp"
#include "task.hpp"
#include "transportermap.hpp"
#include "weighttable.hpp"

class TaskAttack;

class TaskKillUnit : public Task {
    uint16_t unit_requests;
    SmartPointer<SpottedUnit> spotted_unit;
    SmartList<UnitInfo> units;
    uint16_t required_damage;
    uint16_t projected_damage;
    uint16_t hits;
    SmartPointer<UnitInfo> managed_unit;
    WeightTable weight_table;
    bool seek_target;

    static int32_t GetProjectedDamage(UnitInfo* attacker, UnitInfo* target);
    static void MoveFinishedCallback(Task* task, UnitInfo* unit, char result);

    void FindVaildTypes();
    bool GetNewUnits();
    UnitInfo* FindClosestCombatUnit(SmartList<UnitInfo>* units, UnitInfo* unit, int32_t* distance, TransporterMap* map);
    bool GiveOrdersToUnit(UnitInfo* unit);

public:
    TaskKillUnit(TaskAttack* task_attack, SpottedUnit* spotted_unit, uint16_t priority);
    ~TaskKillUnit();

    bool IsUnitUsable(UnitInfo& unit);
    uint16_t GetPriority() const;
    char* WriteStatusLog(char* buffer) const;
    Rect* GetBounds(Rect* bounds);
    uint8_t GetType() const;
    bool IsNeeded();
    void AddUnit(UnitInfo& unit);
    void BeginTurn();
    void ChildComplete(Task* task);
    void EndTurn();
    bool Execute(UnitInfo& unit);
    void RemoveSelf();
    bool CheckReactions();
    void RemoveUnit(UnitInfo& unit);
    void EventUnitDestroyed(UnitInfo& unit);
    void EventEnemyUnitSpotted(UnitInfo& unit);

    int32_t GetTotalProjectedDamage();
    bool MoveUnits();
    SpottedUnit* GetSpottedUnit() const;
    UnitInfo* GetUnitSpotted() const;
    SmartList<UnitInfo>& GetUnits();
    uint16_t GetRequiredDamage() const;
    uint16_t GetProjectedDamage() const;
};

#endif /* TASKKILLUNIT_HPP */
