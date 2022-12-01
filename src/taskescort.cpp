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

#include "taskescort.hpp"

#include "access.hpp"
#include "aiattack.hpp"
#include "aiplayer.hpp"
#include "inifile.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "taskobtainunits.hpp"
#include "taskreload.hpp"
#include "taskrepair.hpp"
#include "tasktransport.hpp"
#include "taskupgrade.hpp"
#include "transportermap.hpp"
#include "units_manager.hpp"

bool TaskEscort::IssueOrders(UnitInfo* unit) {
    bool result;

    if (Task_IsReadyToTakeOrders(&*target) || target->orders == ORDER_MOVE_TO_ATTACK) {
        Point target_location(target->grid_x, target->grid_y);
        Point unit_location(unit->grid_x, unit->grid_y);
        Point position;
        unsigned char** info_map = AiPlayer_Teams[team].GetInfoMap();
        short** damage_potential_map =
            AiPlayer_Teams[team].GetDamagePotentialMap(unit, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, 0x00);
        Rect bounds;
        ResourceID escort_type = INVALID_ID;

        rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

        if (Task_GetReadyUnitsCount(team, AIRTRANS) > 0) {
            escort_type = AIRTRANS;

        } else if (Task_GetReadyUnitsCount(team, SEATRANS) > 0) {
            escort_type = SEATRANS;

        } else if ((unit->unit_type == COMMANDO || unit->unit_type == INFANTRY) &&
                   Task_GetReadyUnitsCount(team, CLNTRANS) > 0) {
            escort_type = CLNTRANS;

        } else if (Task_GetReadyUnitsCount(team, AIRPLT) > 0) {
            escort_type = AIRTRANS;

        } else if (Task_GetReadyUnitsCount(team, SHIPYARD) > 0) {
            escort_type = SEATRANS;
        }

        if (damage_potential_map) {
            TransporterMap transporter_map(unit, 0x01, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, escort_type);
            int distance;
            int minimum_distance = TaskManager_GetDistance(unit_location, target_location) / 2;
            int unit_hits = unit->hits;

            for (int range = 1; range < minimum_distance; ++range) {
                position.x = target_location.x - range;
                position.y = target_location.y + range;

                for (int direction = 0; direction < 8; direction += 2) {
                    for (int index = 0; index < range * 2; ++index) {
                        position += Paths_8DirPointsArray[direction];

                        if (Access_IsInsideBounds(&bounds, &position) &&
                            damage_potential_map[position.x][position.y] < unit_hits) {
                            distance =
                                (TaskManager_GetDistance(unit->grid_x - position.x, unit->grid_y - position.y) / 4) +
                                TaskManager_GetDistance(position, target_location) / 2;

                            if (distance < minimum_distance && !(info_map[position.x][position.y] & 8) &&
                                transporter_map.Search(position) &&
                                Access_IsAccessible(unit->unit_type, team, position.x, position.y, 0x02)) {
                                unit_location = position;
                                minimum_distance = distance;
                            }
                        }
                    }
                }
            }

            if (unit_location.x == unit->grid_x && unit_location.y == unit->grid_y) {
                result = false;

            } else {
                unit->point = unit_location;

                SmartPointer<TaskMove> move_task(new (std::nothrow)
                                                     TaskMove(unit, this, 0, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE,
                                                              unit_location, &TaskTransport_MoveFinishedCallback));

                move_task->SetField68(true);

                TaskManager.AppendTask(*move_task);

                result = true;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskEscort::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    if (result != TASKMOVE_RESULT_SUCCESS || !AiAttack_EvaluateAssault(unit, task, &MoveFinishedCallback)) {
        if (unit->IsReadyForOrders(task)) {
            dynamic_cast<TaskEscort*>(task)->IssueOrders(unit);
        }
    }
}

TaskEscort::TaskEscort(UnitInfo* unit, ResourceID unit_type_) : Task(unit->team, nullptr, 0x0F00) {
    target = unit;
    unit_type = unit_type_;
    is_escort_requested = 0;
}

TaskEscort::~TaskEscort() {}

bool TaskEscort::Task_vfunc1(UnitInfo& unit) { return (!target || target->hits == 0 || escort != unit); }

int TaskEscort::GetMemoryUse() const { return 4; }

char* TaskEscort::WriteStatusLog(char* buffer) const {
    if (target) {
        sprintf(buffer, "Escort %s at [%i,%i]", UnitsManager_BaseUnits[target->unit_type].singular_name,
                target->grid_x + 1, target->grid_y + 1);

    } else {
        strcpy(buffer, "Completed escort task");
    }

    return buffer;
}

unsigned char TaskEscort::GetType() const { return TaskType_TaskEscort; }

void TaskEscort::AddUnit(UnitInfo& unit) {
    if (!escort && unit_type == unit.unit_type && target) {
        escort = unit;
        escort->PushFrontTask1List(this);
        escort->AddReminders(true);
        is_escort_requested = 0;
    }
}

void TaskEscort::Begin() {
    target->PushBackTask2List(this);
    RemindTurnStart(true);
}

void TaskEscort::EndTurn() {
    if (target) {
        if (target->hits > 0 && target->team == team) {
            if (escort) {
                if (escort->IsReadyForOrders(this)) {
                    if (escort->ammo < escort->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS)) {
                        SmartPointer<Task> reload_task(new (std::nothrow) TaskReload(&*escort));

                        TaskManager.AppendTask(*reload_task);

                    } else if (escort->hits < escort->GetBaseValues()->GetAttribute(ATTRIB_HITS) / 2 &&
                               ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_APPRENTICE) {
                        SmartPointer<Task> repair_task(new (std::nothrow) TaskRepair(&*escort));

                        TaskManager.AppendTask(*repair_task);

                    } else if (!(escort->flags & REGENERATING_UNIT) &&
                               ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_EXPERT &&
                               AiPlayer_Teams[escort->team].IsUpgradeNeeded(&*escort)) {
                        SmartPointer<Task> upgrade_task(new (std::nothrow) TaskUpgrade(&*escort));

                        TaskManager.AppendTask(*upgrade_task);

                    } else if (escort->speed > 0) {
                        Task_RemindMoveFinished(&*escort, true);
                    }
                }

            } else {
                if (!is_escort_requested) {
                    if (unit_type == FIGHTER || unit_type == SP_FLAK || unit_type == FASTBOAT) {
                        bool teams[PLAYER_TEAM_MAX];
                        SmartList<UnitInfo>::Iterator it;

                        AiAttack_GetTargetTeams(team, teams);

                        for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End();
                             ++it) {
                            if ((*it).team != team && teams[(*it).team] && (*it).unit_type == AIRPLT) {
                                break;
                            }
                        }

                        if (!it) {
                            for (it = UnitsManager_MobileAirUnits.Begin(); it != UnitsManager_MobileAirUnits.End();
                                 ++it) {
                                if ((*it).team != team && teams[(*it).team]) {
                                    break;
                                }
                            }
                        }

                        if (!it) {
                            return;
                        }
                    }

                    SmartPointer<TaskObtainUnits> obtain_unit_task(
                        new (std::nothrow) TaskObtainUnits(this, Point(target->grid_x, target->grid_y)));

                    obtain_unit_task->AddUnit(unit_type);

                    TaskManager.AppendTask(*obtain_unit_task);

                    is_escort_requested = true;
                }
            }

        } else {
            RemoveSelf();
        }
    }
}

bool TaskEscort::Task_vfunc17(UnitInfo& unit) {
    bool result;

    if (target) {
        if (target->hits > 0) {
            if (escort == unit) {
                if (unit.speed > 0 && unit.IsReadyForOrders(this)) {
                    if (Task_RetreatFromDanger(this, &unit, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE)) {
                        result = true;

                    } else if (AiAttack_EvaluateAssault(&unit, this, &MoveFinishedCallback)) {
                        result = true;

                    } else {
                        result = IssueOrders(&unit);
                    }

                } else {
                    result = false;
                }

            } else {
                result = false;
            }

        } else {
            RemoveSelf();

            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskEscort::RemoveSelf() {
    if (escort) {
        escort->RemoveTask(this, false);

        Task_RemoveMovementTasks(&*escort);
    }

    if (target) {
        target->RemoveFromTask2List(this);
    }

    target = nullptr;
    escort = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskEscort::RemoveUnit(UnitInfo& unit) {
    if (escort == unit) {
        escort = nullptr;
    }
}
