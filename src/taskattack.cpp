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

#include "access.hpp"
#include "aiplayer.hpp"
#include "continent.hpp"
#include "inifile.hpp"
#include "task_manager.hpp"
#include "taskrepair.hpp"
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

bool TaskAttack::IsUnitUsable(UnitInfo& unit) { return !recon_unit && IsReconUnitUsable(&unit); }

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

    if (task && !task->GetUnitSpotted()) {
        task = nullptr;
    }

    if (!task && secondary_targets.GetCount() > 0) {
        task = &secondary_targets[0];
    }

    if (task && !task->GetUnitSpotted()) {
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

bool TaskAttack::Task_vfunc9() { return primary_targets.GetCount() > 0; }

void TaskAttack::AddUnit(UnitInfo& unit) {
    int unit_index = managed_unit_types->Find(&unit.unit_type);

    if (unit_index >= 0) {
        managed_unit_types.Remove(unit_index);
    }

    if (!recon_unit && IsReconUnitUsable(&unit)) {
        recon_unit = unit;
        unit.PushFrontTask1List(this);

        if (!GetField7()) {
            TaskManager.AppendReminder(new (std::nothrow) class RemindTurnStart(*this));
        }

        Task_RemindMoveFinished(&unit, true);

        recon_unit->point.x = 0;
        recon_unit->point.y = 0;

    } else {
        TaskManager.RemindAvailable(&unit);
    }
}

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
            if (!kill_unit_task || !kill_unit_task->GetUnitSpotted()) {
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
        for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnitsListIterator(); it2 != nullptr; ++it2) {
            if ((*it2).speed > 0 && Task_IsReadyToTakeOrders(&*it2) && (*it2).GetTask() == this) {
                Task_RemindMoveFinished(&*it2, true);
            }
        }
    }
}

bool TaskAttack::Task_vfunc16(UnitInfo& unit) {
    bool result;

    if (recon_unit && kill_unit_task) {
        Point position = kill_unit_task->DeterminePosition();

        if (Access_GetDistance(&unit, position) < Access_GetDistance(&*recon_unit, position) &&
            IsReconUnitUsable(&unit)) {
            SmartPointer<UnitInfo> copy(recon_unit);

            if (leader == recon_unit) {
                leader->RemoveFromTask2List(this);
                leader = unit;
                leader->PushBackTask2List(this);
            }

            recon_unit = unit;

            unit.PushFrontTask1List(this);

            unit.point.x = 0;
            unit.point.y = 0;

            Task_RemindMoveFinished(&unit);

            TaskManager.RemindAvailable(&*copy);

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::Task_vfunc17(UnitInfo& unit) {
    bool result;

    if (secondary_targets.GetCount() > 0) {
        if (unit.speed > 0 && unit.IsReadyForOrders(this)) {
            if ((unit.hits < unit.GetBaseValues()->GetAttribute(ATTRIB_HITS) / 4) ||
                ((leader == unit || recon_unit == unit) &&
                 (unit.hits < unit.GetBaseValues()->GetAttribute(ATTRIB_HITS) / 2))) {
                unit.ClearFromTaskLists();

                SmartPointer<Task> repair_task(new (std::nothrow) TaskRepair(&unit));

                TaskManager.AppendTask(*repair_task);

                result = true;

            } else {
                result = MoveCombatUnit(this, &unit);
            }

        } else {
            result = false;
        }

    } else {
        RemoveUnit(unit);
        unit.RemoveTask(this);

        result = false;
    }

    return result;
}

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

bool TaskAttack::IsDestinationReached(UnitInfo* unit) {
    return unit->grid_x == unit->point.x && unit->grid_y == unit->point.y;
}

UnitInfo* TaskAttack::DetermineLeader() {}

bool TaskAttack::EvaluateLandAttack() {
    UnitInfo* unit = nullptr;
    bool result;

    for (int i = 0; !unit && i < primary_targets.GetCount(); ++i) {
        unit = primary_targets[i].GetUnitSpotted();
    }

    if (unit) {
        WeightTable weight_table;
        bool sea_unit_present = false;
        unsigned short task_flags = GetFlags();

        weight_table = AiPlayer_Teams[team].GetExtendedWeightTable(unit, 0x03);

        for (int i = 0; !unit && i < weight_table.GetCount(); ++i) {
            if (weight_table[i].weight > 0 &&
                !(UnitsManager_BaseUnits[weight_table[i].unit_type].flags & MOBILE_LAND_UNIT) &&
                (UnitsManager_BaseUnits[weight_table[i].unit_type].flags & MOBILE_SEA_UNIT)) {
                sea_unit_present = true;
            }
        }

        if (sea_unit_present) {
            AccessMap access_map;
            bool stationary_unit_present = false;
            Point position;
            Rect bounds;
            int worth_of_land_units;
            int worth_of_sea_units;

            rect_init(&bounds, 0, 0, 0, 0);

            for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
                for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
                    if (ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * y + x] == SURFACE_TYPE_LAND) {
                        access_map.GetMapColumn(x)[y] = 2;
                    }
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
                 it != UnitsManager_GroundCoverUnits.End(); ++it) {
                if ((*it).unit_type != BRIDGE && (*it).IsVisibleToTeam(team)) {
                    access_map.GetMapColumn((*it).grid_x)[(*it).grid_y] = 2;
                }
            }

            for (SmartList<TaskKillUnit>::Iterator it = primary_targets.Begin(); it != primary_targets.End(); ++it) {
                SpottedUnit* spotted_unit = (*it).GetSpottedUnit();

                if (spotted_unit) {
                    position = spotted_unit->GetLastPosition();

                    if (access_map.GetMapColumn(position.x)[position.y] == 2) {
                        SmartPointer<Continent> continent(new (std::nothrow)
                                                              Continent(access_map.GetMap(), 3, position, 0));
                        continent->GetBounds(bounds);
                    }
                }
            }

            worth_of_land_units = 0;
            worth_of_sea_units = 0;

            weight_table = AiPlayer_Teams[team].GetExtendedWeightTable(unit, 0x01);

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == team && (*it).hits > 0 && (*it).ammo > 0 &&
                    (*it).ammo >= (*it).GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) &&
                    !((*it).flags & MOBILE_AIR_UNIT) &&
                    (((*it).flags & MOBILE_SEA_UNIT) || access_map.GetMapColumn((*it).grid_x)[(*it).grid_y] == 3) &&
                    ((*it).orders == ORDER_AWAIT || (*it).orders == ORDER_SENTRY || (*it).orders == ORDER_MOVE ||
                     (*it).orders == ORDER_MOVE_TO_UNIT)) {
                    bool is_relevant = false;

                    if ((*it).GetTask()) {
                        if ((*it).GetTask()->DeterminePriority(task_flags) > 0) {
                            is_relevant = (*it).GetTask()->Task_vfunc1(*it);
                        }

                    } else {
                        is_relevant = true;
                    }

                    if (is_relevant && weight_table.GetWeight((*it).unit_type)) {
                        if ((*it).flags & MOBILE_SEA_UNIT) {
                            worth_of_sea_units += (*it).GetBaseValues()->GetAttribute(ATTRIB_TURNS);

                        } else {
                            worth_of_land_units += (*it).GetBaseValues()->GetAttribute(ATTRIB_TURNS);
                        }
                    }
                }
            }

            if (worth_of_sea_units > 0 || worth_of_land_units > 0) {
                result = worth_of_land_units > worth_of_sea_units;

            } else {
                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if ((*it).team == team && (*it).unit_type == LANDPLT) {
                        stationary_unit_present = true;

                        if (access_map.GetMapColumn((*it).grid_x)[(*it).grid_y] == 3) {
                            return true;
                        }
                    }
                }

                if (!stationary_unit_present) {
                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                         it != UnitsManager_StationaryUnits.End(); ++it) {
                        if ((*it).team == team && access_map.GetMapColumn((*it).grid_x)[(*it).grid_y] == 3) {
                            return true;
                        }
                    }
                }

                result = false;
            }

        } else {
            result = true;
        }

    } else {
        result = true;
    }

    return result;
}

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

bool TaskAttack::RequestReconUnit(ResourceID unit_type, int safe_distance) {
    bool result;

    if (UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)->GetAttribute(ATTRIB_SCAN) >=
        safe_distance) {
        if (recon_unit) {
            if (recon_unit->unit_type == unit_type) {
                return true;
            }

            TaskManager.RemindAvailable(&*recon_unit);

            recon_unit = nullptr;
        }

        if (managed_unit_types->Find(&unit_type) >= 0) {
            result = true;

        } else {
            SmartPointer<TaskObtainUnits> obtain_units_task(new (std::nothrow)
                                                                TaskObtainUnits(this, DeterminePosition()));

            obtain_units_task->AddUnit(unit_type);

            managed_unit_types.PushBack(&unit_type);

            TaskManager.AppendTask(*obtain_units_task);

            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::FindReconUnit(ResourceID unit_type, int safe_distance) {
    bool result;

    if (UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)->GetAttribute(ATTRIB_SCAN) >=
        safe_distance) {
        SmartList<UnitInfo>::Iterator it;
        UnitInfo* new_recon_unit = nullptr;
        unsigned short task_flags = GetFlags();

        if (UnitsManager_BaseUnits[unit_type].flags & MOBILE_AIR_UNIT) {
            it = UnitsManager_MobileAirUnits.Begin();

        } else {
            it = UnitsManager_MobileLandSeaUnits.Begin();
        }

        for (; it; ++it) {
            if ((*it).team == team && (*it).unit_type == unit_type &&
                (*it).GetBaseValues()->GetAttribute(ATTRIB_SCAN) >= safe_distance &&
                (!(*it).GetTask() || (*it).GetTask()->DeterminePriority(task_flags) > 0)) {
                if (!new_recon_unit || (new_recon_unit->GetTask() && !(*it).GetTask())) {
                    new_recon_unit = &*it;
                }
            }
        }

        if (new_recon_unit) {
            if (recon_unit) {
                TaskManager.RemindAvailable(&*recon_unit);
            }

            new_recon_unit->ClearFromTaskLists();
            new_recon_unit->PushFrontTask1List(this);
            recon_unit = new_recon_unit;

            recon_unit->point.x = 0;
            recon_unit->point.y = 0;

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::IsReconUnitUsable(UnitInfo* unit) {
    bool result;

    if (unit->unit_type == AWAC || unit->unit_type == SCOUT || unit->unit_type == SCANNER ||
        unit->unit_type == FASTBOAT) {
        int safe_distance_ground;
        int safe_distance_air;

        GetSafeDistances(&safe_distance_air, &safe_distance_ground);

        if (unit->unit_type == AWAC) {
            result = unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN) >= safe_distance_air;

        } else {
            result = unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN) >= safe_distance_ground;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::IsWithinAttackRange(UnitInfo* unit, Point unit_position) {
    bool result;

    if (unit && unit->ammo > 0) {
        int distance = unit->GetAttackRange() - 3;

        distance = distance * distance;

        result = Access_GetDistance(unit, unit_position) <= distance;

    } else {
        result = false;
    }

    return result;
}

void TaskAttack::GetSafeDistances(int* safe_distance_air, int* safe_distance_ground) {
    *safe_distance_air = 0;
    *safe_distance_ground = 0;

    if (kill_unit_task) {
        UnitInfo* unit = kill_unit_task->GetUnitSpotted();
        Point unit_position(unit->grid_x, unit->grid_y);
        int safe_air_distance;
        int safe_ground_distance;

        for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
            unit = (*it).GetUnitSpotted();

            if (unit) {
                UnitValues* unit_values = unit->GetBaseValues();

                if (unit_values->GetAttribute(ATTRIB_ATTACK) > 0) {
                    if (&*it == &*kill_unit_task || IsWithinAttackRange(unit, unit_position)) {
                        if (Access_IsValidAttackTargetType(unit->unit_type, AWAC)) {
                            *safe_distance_air =
                                std::max(*safe_distance_air, unit_values->GetAttribute(ATTRIB_RANGE) + 1);

                        } else {
                            *safe_distance_ground =
                                std::max(*safe_distance_ground, unit_values->GetAttribute(ATTRIB_RANGE) + 1);
                        }
                    }
                }
            }
        }

        safe_air_distance =
            *safe_distance_air -
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], AWAC)->GetAttribute(ATTRIB_SCAN);

        if (access_flags & MOBILE_LAND_UNIT) {
            safe_ground_distance =
                *safe_distance_ground -
                UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], SCANNER)->GetAttribute(ATTRIB_SCAN);

        } else {
            safe_ground_distance =
                *safe_distance_ground -
                UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], FASTBOAT)->GetAttribute(ATTRIB_SCAN);
        }

        if (safe_air_distance > 0 && safe_ground_distance > 0) {
            int safe_distane = std::min(safe_air_distance, safe_ground_distance);

            *safe_distance_air -= safe_distane;
            *safe_distance_ground -= safe_distane;
        }
    }
}

void TaskAttack::UpdateReconUnit() {
    if (kill_unit_task) {
        int safe_air_distance;
        int safe_ground_distance;

        GetSafeDistances(&safe_air_distance, &safe_ground_distance);

        if (recon_unit) {
            if (recon_unit->unit_type == AWAC) {
                if (recon_unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN) >= safe_air_distance) {
                    return;
                }

            } else {
                if (recon_unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN) >= safe_ground_distance) {
                    return;
                }
            }
        }

        if (access_flags & MOBILE_SEA_UNIT) {
            if (!FindReconUnit(FASTBOAT, safe_ground_distance) && !FindReconUnit(AWAC, safe_air_distance)) {
                RequestReconUnit(FASTBOAT, safe_ground_distance);
                RequestReconUnit(AWAC, safe_air_distance);
            }

        } else if (access_flags & MOBILE_LAND_UNIT) {
            if (!FindReconUnit(SCOUT, safe_ground_distance) && !FindReconUnit(SCANNER, safe_ground_distance) &&
                !FindReconUnit(AWAC, safe_air_distance) && !RequestReconUnit(SCOUT, safe_ground_distance)) {
                RequestReconUnit(SCANNER, safe_ground_distance);
                RequestReconUnit(AWAC, safe_air_distance);
            }

        } else {
            if (!FindReconUnit(AWAC, safe_air_distance)) {
                RequestReconUnit(AWAC, 0);
            }
        }
    }
}

void TaskAttack::ChooseFirstTarget() {
    if (secondary_targets.GetCount() > 0) {
        kill_unit_task = secondary_targets[0];

        if (attack_zone_reached && leader) {
            if (leader->speed == leader->GetBaseValues()->GetAttribute(ATTRIB_SPEED)) {
                TaskKillUnit* task = &*kill_unit_task;
                secondary_targets.Remove(*kill_unit_task);
                secondary_targets.PushBack(*kill_unit_task);
                kill_unit_task = secondary_targets[0];

                if (kill_unit_task != task) {
                    attack_zone_reached = false;
                }
            }
        }
    }
}

void TaskAttack::CopyTargets(TaskAttack* other) {
    for (SmartList<TaskKillUnit>::Iterator it = other->primary_targets.Begin(); it != other->primary_targets.End();
         ++it) {
        primary_targets.PushBack(*it);
    }

    for (SmartList<TaskKillUnit>::Iterator it = other->secondary_targets.Begin(); it != other->secondary_targets.End();
         ++it) {
        (*it).SetParent(this);
        (*it).GetSpottedUnit()->SetTask(this);
        secondary_targets.PushBack(*it);
    }
}

bool TaskAttack::IsTargetGroupInSight() {
    bool result;

    if (secondary_targets.GetCount() > 0) {
        unsigned short unit_team = secondary_targets[0].GetUnitSpotted()->team;

        for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
            for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnitsListIterator(); it2 != nullptr; ++it2) {
                if ((*it2).IsVisibleToTeam(unit_team)) {
                    return true;
                }
            }
        }

        result = false;

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::IsThereTimeToPrepare() {
    bool result;

    if (op_state <= ATTACK_STATE_GATHER_FORCES) {
        if (IsTargetGroupInSight()) {
            if (kill_unit_task && kill_unit_task->GetUnitSpotted()) {
                result = !kill_unit_task->GetUnitSpotted()->IsVisibleToTeam(team);

            } else {
                result = false;
            }

        } else {
            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

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

void TaskAttack::RemoveTask(TaskKillUnit* task) {
    primary_targets.Remove(*task);
    secondary_targets.Remove(*task);
    task->RemoveSelf();
}

void TaskAttack::AssessEnemyUnits() {}

bool TaskAttack::IsExecutionPhase() { return op_state >= ATTACK_STATE_ADVANCE; }
