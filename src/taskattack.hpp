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

#ifndef TASKATTACK_HPP
#define TASKATTACK_HPP

#include "spottedunit.hpp"
#include "taskkillunit.hpp"
#include "tasksupportattack.hpp"

enum {
    ATTACK_STATE_INIT = 0x0,
    ATTACK_STATE_WAIT = 0x1,
    ATTACK_STATE_GATHER_FORCES = 0x2,
    ATTACK_STATE_ADVANCE = 0x3,
    ATTACK_STATE_ADVANCE_USING_TRANSPORT = 0x4,
    ATTACK_STATE_NORMAL_SEARCH = 0x5,
    ATTACK_STATE_BOLD_SEARCH = 0x6,
    ATTACK_STATE_ATTACK = 0x7,
};

class TaskAttack : public Task {
    uint16_t target_team;
    uint8_t op_state;
    SmartList<TaskKillUnit> primary_targets;
    SmartList<TaskKillUnit> secondary_targets;
    SmartPointer<TaskKillUnit> kill_unit_task;
    SmartPointer<TaskSupportAttack> support_attack_task;
    SmartPointer<UnitInfo> leader;
    SmartPointer<Task> leader_task;
    uint32_t access_flags;
    SmartPointer<UnitInfo> recon_unit;
    SmartObjectArray<ResourceID> managed_unit_types;
    bool attack_zone_reached;

    bool EvaluateLandAttack();
    void Finish();
    bool RequestReconUnit(ResourceID unit_type, int32_t safe_distance);
    bool FindReconUnit(ResourceID unit_type, int32_t safe_distance);
    bool IsReconUnitUsable(UnitInfo* unit);
    bool IsWithinAttackRange(UnitInfo* unit, Point unit_position);
    void GetSafeDistances(int32_t* safe_distance_air, int32_t* safe_distance_ground);
    void UpdateReconUnit();
    void ChooseFirstTarget();
    void CopyTargets(TaskAttack* other);
    bool IsTargetGroupInSight();
    bool IsThereTimeToPrepare();
    bool MoveUnit(Task* task, UnitInfo* unit, Point site, int32_t caution_level);
    Point FindClosestDirectRoute(UnitInfo* unit, int32_t caution_level);
    bool MoveReconUnit(int32_t caution_level);
    bool IsAttackUnderControl();
    bool PlanMoveForReconUnit();
    bool IsViableLeader(UnitInfo* unit);
    Point GetLeaderDestination();
    void FindNewSiteForUnit(UnitInfo* unit);
    bool MoveUnits();
    bool IsAnyTargetInRange();
    bool IsReconUnitAvailable();
    void EvaluateAttackReadiness();
    bool IsDefenderDangerous(SpottedUnit* spotted_unit);
    void RemoveTask(TaskKillUnit* task);
    void AssessEnemyUnits();

public:
    TaskAttack(SpottedUnit* spotted_unit, uint16_t task_priority);
    ~TaskAttack();

    bool IsUnitUsable(UnitInfo& unit);
    int32_t GetCautionLevel(UnitInfo& unit);
    uint16_t GetPriority() const;
    char* WriteStatusLog(char* buffer) const;
    Rect* GetBounds(Rect* bounds);
    uint8_t GetType() const;
    bool IsNeeded();
    void AddUnit(UnitInfo& unit);
    void Init();
    void BeginTurn();
    void ChildComplete(Task* task);
    void EndTurn();
    bool ExchangeOperator(UnitInfo& unit);
    bool Execute(UnitInfo& unit);
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);

    UnitInfo* DetermineLeader();
    bool MoveCombatUnit(Task* task, UnitInfo* unit);
    int32_t GetHighestScan();
    bool IsDestinationReached(UnitInfo* unit);
    uint32_t GetAccessFlags() const;
    bool IsExecutionPhase();
};

#endif /* TASKATTACK_HPP */
