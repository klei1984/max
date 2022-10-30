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

#include "taskattack.hpp"

#include "inifile.hpp"
#include "task_manager.hpp"
#include "units_manager.hpp"

TaskAttack::TaskAttack(SpottedUnit* spotted_unit, unsigned short task_flags)
    : Task(spotted_unit->GetTeam(), nullptr, task_flags) {
    op_state = ATTACK_STATE_INIT;
    attack_zone_reached = false;
    target_team = spotted_unit->GetUnit()->team;
    access_flags = 0;

    TaskKillUnit* task = new (std::nothrow) TaskKillUnit(this, spotted_unit, task_flags);

    primary_targets.PushBack(*task);
    secondary_targets.PushBack(*task);
}

TaskAttack::~TaskAttack() {}

bool TaskAttack::IsUnitUsable(UnitInfo& unit) {}

int TaskAttack::GetCautionLevel(UnitInfo& unit) {
    int result;

    if (unit.GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE) &&
        ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
        if (unit.shots > 0) {
            result = CAUTION_LEVEL_AVOID_REACTION_FIRE;

        } else {
            result = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
        }

    } else {
        if (unit.ammo > 0 || op_state >= ATTACK_STATE_BOLD_SEARCH) {
            result = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;

        } else {
            result = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
        }
    }

    return result;
}

int TaskAttack::GetMemoryUse() const {
    return primary_targets.GetMemorySize() - 6 + secondary_targets.GetMemorySize() - 10;
}

unsigned short TaskAttack::GetFlags() const {
    unsigned short result;

    if (secondary_targets.GetCount()) {
        result = secondary_targets[0].GetFlags();

    } else {
        result = flags;
    }

    return result;
}

char* TaskAttack::WriteStatusLog(char* buffer) const {
    TaskKillUnit* task = &*kill_unit_task;

    if (task && !task->GetSpottedUnit()) {
        task = nullptr;
    }

    if (!task && secondary_targets.GetCount() > 0) {
        task = &secondary_targets[0];
    }

    if (task && !task->GetSpottedUnit()) {
        task = nullptr;
    }

    if (task) {
        Point position;
        const char* status;

        switch (op_state) {
            case ATTACK_STATE_INIT: {
                status = "Init. attack on ";
            } break;

            case ATTACK_STATE_WAIT: {
                status = "Wait to attack ";
            } break;

            case ATTACK_STATE_GATHER_FORCES: {
                status = "Gather attack on ";
            } break;

            case ATTACK_STATE_ADVANCE: {
                status = "Adv. on ";
            } break;

            case ATTACK_STATE_ADVANCE_USING_TRANSPORT: {
                status = "Adv. w/trans. on ";
            } break;

            case ATTACK_STATE_NORMAL_SEARCH: {
                status = "Search for ";
            } break;

            case ATTACK_STATE_BOLD_SEARCH: {
                status = "Bold search for ";
            } break;

            case ATTACK_STATE_ATTACK: {
                status = "Attack ";
            } break;
        }

        position = task->DeterminePosition();

        sprintf(buffer, "%s %s at [%i,%i]", status,
                UnitsManager_BaseUnits[task->GetUnitSpotted()->unit_type].singular_name, position.x + 1,
                position.y + 1);

        if (leader) {
            char text[100];

            sprintf(text, ", leader %s %i at [%i,%i]", UnitsManager_BaseUnits[leader->unit_type].singular_name,
                    leader->unit_id, leader->grid_x + 1, leader->grid_y + 1);

            strcat(buffer, text);
        }

    } else {
        strcpy(buffer, "Completed attack task.");
    }

    return buffer;
}

Rect* TaskAttack::GetBounds(Rect* bounds) {
    if (kill_unit_task) {
        kill_unit_task->GetBounds(bounds);

    } else {
        Task::GetBounds(bounds);
    }

    return bounds;
}

unsigned char TaskAttack::GetType() const { return TaskType_TaskAttack; }

bool TaskAttack::Task_vfunc9() {}

void TaskAttack::Task_vfunc11(UnitInfo& unit) {}

void TaskAttack::Begin() {
    access_flags = MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT;

    TaskManager.AppendTask(primary_targets[0]);

    primary_targets[0].GetSpottedUnit()->SetTask(this);

    RemindTurnStart(true);
}

void TaskAttack::BeginTurn() {
    if (op_state == ATTACK_STATE_INIT || !((dos_rand() * 21) >> 15)) {
        if (EvaluateLandAttack()) {
            access_flags = MOBILE_AIR_UNIT | MOBILE_LAND_UNIT;

        } else {
            access_flags = MOBILE_AIR_UNIT | MOBILE_SEA_UNIT;
        }
    }

    AssessEnemyUnits();

    attack_zone_reached = false;

    if (secondary_targets.GetCount()) {
        ChooseFirstTarget();
        UpdateReconUnit();
        EvaluateAttackReadiness();

        if (!support_attack_task && secondary_targets.GetCount() > 5) {
            support_attack_task = new (std::nothrow) TaskSupportAttack(this);

            TaskManager.AppendTask(*support_attack_task);
        }
    }
}

void TaskAttack::ChildComplete(Task* task) {
    if (task->GetType() == TaskType_TaskKillUnit) {
        secondary_targets.Remove(*dynamic_cast<TaskKillUnit*>(task));

        if (kill_unit_task == dynamic_cast<TaskKillUnit*>(task)) {
            kill_unit_task = nullptr;
        }

        if (secondary_targets.GetCount() > 0) {
            if (!kill_unit_task || !kill_unit_task->GetSpottedUnit()) {
                ChooseFirstTarget();
            }

        } else {
            Finish();
        }
    }
}

void TaskAttack::EndTurn() {
    EvaluateAttackReadiness();

    if (recon_unit && recon_unit->speed > 0) {
        Task_RemindMoveFinished(&*recon_unit, true);
    }

    if (support_attack_task) {
        for (SmartList<UnitInfo>::Iterator it = support_attack_task->GetUnitsListIterator(); it != nullptr; ++it) {
            if ((*it).speed > 0 && Task_IsReadyToTakeOrders(&*it) && (*it).GetTask() == this) {
                Task_RemindMoveFinished(&*it, true);
            }
        }
    }

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnitsListIterator(); it != nullptr; ++it) {
            if ((*it2).speed > 0 && Task_IsReadyToTakeOrders(&*it2) && (*it2).GetTask() == this) {
                Task_RemindMoveFinished(&*it2, true);
            }
        }
    }
}

bool TaskAttack::Task_vfunc16(UnitInfo& unit) {}

bool TaskAttack::Task_vfunc17(UnitInfo& unit) {}

void TaskAttack::RemoveSelf() {
    if (support_attack_task) {
        support_attack_task->RemoveSelf();
    }

    if (leader) {
        leader->RemoveFromTask2List(this);
        TaskManager.RemindAvailable(&*leader);
    }

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        (*it).GetSpottedUnit()->SetTask(nullptr);
        (*it).RemoveSelf();
    }

    primary_targets.Clear();
    secondary_targets.Clear();
    kill_unit_task = nullptr;
    parent = nullptr;
    support_attack_task = nullptr;
    leader = nullptr;
    backup_task = nullptr;
    recon_unit = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskAttack::RemoveUnit(UnitInfo& unit) {}

unsigned int TaskAttack::GetAccessFlags() const { return access_flags; }

int TaskAttack::GetHighestScan() {}

bool TaskAttack::MoveCombatUnit(Task* task, UnitInfo* unit) {}

bool TaskAttack::IsDestinationReached(UnitInfo* unit) {}

UnitInfo* TaskAttack::DetermineLeader() {}

bool TaskAttack::EvaluateLandAttack() {}

void TaskAttack::Finish() {
    if (support_attack_task) {
        support_attack_task->RemoveSelf();
        support_attack_task = nullptr;
    }

    if (recon_unit) {
        TaskManager.RemindAvailable(&*recon_unit);
    }

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        (*it).GetSpottedUnit()->SetTask(nullptr);
        (*it).RemoveSelf();
    }

    primary_targets.Clear();
    secondary_targets.Clear();

    TaskManager.RemoveTask(*this);
}

bool TaskAttack::RequestReconUnit(ResourceID unit_type, int safe_distance) {}

bool TaskAttack::FindReconUnit(ResourceID unit_type, int safe_distance) {}

bool TaskAttack::IsReconUnitUsable(UnitInfo* unit) {}

bool TaskAttack::IsWithinAttackRange(Point unit_position) {}

void TaskAttack::GetSafeDistances(int* safe_distance_air, int* safe_distance_ground) {}

void TaskAttack::UpdateReconUnit() {}

void TaskAttack::ChooseFirstTarget() {}

void TaskAttack::CopyTargets(TaskAttack* other) {}

bool TaskAttack::IsTargetGroupInSight() {}

bool TaskAttack::IsThereTimeToPrepare() {}

bool TaskAttack::MoveUnit(Task* task, UnitInfo* unit, Point site, int caution_level) {}

Point TaskAttack::FindClosestDirectRoute(UnitInfo* unit, int caution_level) {}

bool TaskAttack::MoveReconUnit(int caution_level) {}

bool TaskAttack::IsAttackUnderControl() {}

bool TaskAttack::PlanMoveForReconUnit() {}

bool TaskAttack::IsViableLeader(UnitInfo* unit) {}

Point TaskAttack::GetLeaderDestination() {}

void TaskAttack::FindNewSiteForUnit(UnitInfo* unit) {}

bool TaskAttack::MoveUnits() {}

bool TaskAttack::IsAnyTargetInRange() {}

bool TaskAttack::IsReconUnitAvailable() {}

void TaskAttack::EvaluateAttackReadiness() {}

bool TaskAttack::IsDefenderDangerous(SpottedUnit* spotted_unit) {}

void TaskAttack::RemoveTask(TaskKillUnit* task) {}

void TaskAttack::AssessEnemyUnits() {}

bool TaskAttack::IsExecutionPhase() { return op_state >= ATTACK_STATE_ADVANCE; }
