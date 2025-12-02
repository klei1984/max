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

#include "taskobtainunits.hpp"

#include "access.hpp"
#include "ailog.hpp"
#include "builder.hpp"
#include "task_manager.hpp"
#include "taskcreateunit.hpp"
#include "unit.hpp"
#include "units_manager.hpp"

TaskObtainUnits::TaskObtainUnits(Task* task, Point point)
    : Task(task->GetTeam(), task, task->GetPriority()), point(point), init(true), reinit(true) {}

TaskObtainUnits::~TaskObtainUnits() {}

uint32_t TaskObtainUnits::CountInstancesOfUnitType(ResourceID unit_type) {
    uint32_t count = 0;

    for (uint32_t i = 0; i < units->GetCount(); ++i) {
        if (*units[i] == unit_type) {
            ++count;
        }
    }

    return count;
}

bool TaskObtainUnits::IsValidCandidate(UnitInfo* unit, bool mode) {
    bool result;

    if (m_team != unit->team || unit->hits <= 0) {
        result = false;

    } else if (unit->GetOrder() != ORDER_AWAIT && unit->GetOrder() != ORDER_SENTRY &&
               (unit->GetOrder() != ORDER_MOVE || unit->GetOrderState() != ORDER_STATE_EXECUTING_ORDER) &&
               (unit->GetOrder() != ORDER_MOVE_TO_UNIT || unit->GetOrderState() != ORDER_STATE_EXECUTING_ORDER) &&
               (unit->GetOrder() != ORDER_MOVE_TO_ATTACK || unit->GetOrderState() != ORDER_STATE_EXECUTING_ORDER)) {
        result = false;

    } else {
        Task* task = unit->GetTask();

        if (task) {
            if (mode) {
                result = false;

            } else if (!task->IsUnitTransferable(*unit)) {
                result = false;

            } else {
                result = task->ComparePriority(GetPriority()) > 0;
            }

        } else {
            result = true;
        }
    }

    return result;
}

UnitInfo* TaskObtainUnits::FindUnit(ResourceID unit_type, bool mode) {
    SmartList<UnitInfo>* list;
    int32_t speed{0};
    int32_t best_speed{0};
    UnitInfo* selected_unit{nullptr};
    bool is_unit_available{false};

    if (ResourceManager_GetUnit(unit_type).GetFlags() & STATIONARY) {
        list = &UnitsManager_StationaryUnits;

    } else if (ResourceManager_GetUnit(unit_type).GetFlags() & MOBILE_AIR_UNIT) {
        list = &UnitsManager_MobileAirUnits;

    } else {
        list = &UnitsManager_MobileLandSeaUnits;
    }

    AILOG(log, "Obtain Unit: Find {} ", ResourceManager_GetUnit(unit_type).GetSingularName().data());

    for (SmartList<UnitInfo>::Iterator unit = list->Begin(); unit != list->End(); ++unit) {
        if ((*unit).GetUnitType() == unit_type) {
            if ((*unit).GetOrder() == ORDER_BUILD &&
                ((*unit).flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT))) {
                speed = (*unit).build_time * (*unit).GetBaseValues()->GetAttribute(ATTRIB_SPEED) +
                        Access_GetApproximateDistance(point.x - (*unit).grid_x, point.y - (*unit).grid_y);

                if (selected_unit == nullptr || (best_speed > speed)) {
                    is_unit_available = false;
                    selected_unit = unit->Get();
                    best_speed = speed;
                }

            } else {
                if (IsValidCandidate(unit->Get(), mode)) {
                    speed = Access_GetApproximateDistance(point.x - (*unit).grid_x, point.y - (*unit).grid_y);

                    if (selected_unit == nullptr || (best_speed > speed)) {
                        is_unit_available = true;
                        selected_unit = unit->Get();
                        best_speed = speed;
                    }
                }
            }
        }
    }

    if (selected_unit) {
        if (!is_unit_available) {
            AILOG_LOG(log, "{} at [{},{}] has {} turns left to build",
                      ResourceManager_GetUnit(selected_unit->GetUnitType()).GetSingularName().data(),
                      selected_unit->grid_x + 1, selected_unit->grid_y + 1, selected_unit->build_time);

            selected_unit = nullptr;

        } else {
            AILOG_LOG(log, "found.");
        }

    } else {
        AILOG_LOG(log, "not found.");
    }

    return selected_unit;
}

uint16_t TaskObtainUnits::GetPriority() const {
    uint16_t flags;

    if (m_parent != nullptr) {
        flags = m_parent->GetPriority();
    } else {
        flags = TASK_PRIORITY_MOVE;
    }

    return flags;
}

std::string TaskObtainUnits::WriteStatusLog() const {
    std::string result = "Obtain units: ";

    if (units->GetCount() == 0) {
        result += "(finished)";
    }

    for (uint32_t i = 0; i < units->GetCount() && i < 3; ++i) {
        result += ResourceManager_GetUnit(*units[i]).GetSingularName().data();
        if ((i + 1) < units->GetCount() && i < 2) {
            result += ", ";
        }
    }

    if (units->GetCount() > 3) {
        result += "...";
    }

    return result;
}

uint8_t TaskObtainUnits::GetType() const { return TaskType_TaskObtainUnits; }

bool TaskObtainUnits::IsNeeded() {
    bool result;

    if (units->GetCount() > 0 && m_parent != nullptr) {
        result = m_parent->IsNeeded();
    } else {
        result = false;
    }

    return result;
}

void TaskObtainUnits::AddUnit(UnitInfo& unit) {
    auto unit_type{unit.GetUnitType()};

    int32_t index = units->Find(&unit_type);

    AILOG(log, "Obtain Units: Add {} {}.", ResourceManager_GetUnit(unit.GetUnitType()).GetSingularName().data(),
          unit.unit_id);

    if (CountInstancesOfUnitType(unit.GetUnitType())) {
        units->Remove(index);
        m_parent->AddUnit(unit);

        if (units->GetCount() == 0) {
            if (m_parent != nullptr) {
                m_parent->ChildComplete(this);
            }

            m_parent = nullptr;
            TaskManager.RemoveTask(*this);
        }
    }
}

void TaskObtainUnits::Init() {
    if (reinit) {
        RemindTurnEnd(true);

        init = true;
        reinit = false;
    }
}

void TaskObtainUnits::BeginTurn() {
    AILOG(log, "Obtain Unit: Begin Turn");

    reinit = true;
    EndTurn();
}

void TaskObtainUnits::EndTurn() {
    if (m_parent == nullptr || !m_parent->IsNeeded()) {
        units.Clear();
    }

    if (init) {
        for (int32_t i = units.GetCount() - 1; i >= 0; --i) {
            UnitInfo* unit = FindUnit(*units[i], false);

            if (unit) {
                Task* task;

                units.Remove(i);
                task = unit->GetTask();

                if (task) {
                    unit->RemoveTasks();

                    if (unit->GetOrder() != ORDER_AWAIT) {
                        UnitsManager_SetNewOrder(unit, ORDER_AWAIT, ORDER_STATE_CLEAR_PATH);
                    }
                }

                m_parent->AddUnit(*unit);
            }
        }

        bool IsUnitTypeRequested[UNIT_END];

        memset(IsUnitTypeRequested, false, sizeof(IsUnitTypeRequested));

        for (uint32_t i = 0; i < units.GetCount(); ++i) {
            if (ResourceManager_GetUnit(*units[i]).GetFlags() & STATIONARY) {
                TaskManager.CreateBuilding(*units[i], m_team, point, this);
            } else if (*units[i] == CONSTRCT || *units[i] == ENGINEER) {
                TaskManager.CreateUnit(*units[i], m_team, point, this);
            } else if (!IsUnitTypeRequested[*units[i]]) {
                IsUnitTypeRequested[*units[i]] = true;

                RequestUnits(*units[i], m_team, CountInstancesOfUnitType(*units[i]), point);
            }
        }

        init = false;
    }

    if (!units.GetCount()) {
        if (m_parent != nullptr) {
            m_parent->ChildComplete(this);
        }

        m_parent = nullptr;
        TaskManager.RemoveTask(*this);
    }
}

void TaskObtainUnits::RequestUnits(ResourceID unit_type, uint16_t team, int32_t requested_amount, Point site) {
    uint16_t task_priority = this->GetPriority();
    uint16_t task_team = this->GetTeam();
    auto& tasks = TaskManager.GetTaskList();

    AILOG(log, "Task: Request {}.", ResourceManager_GetUnit(unit_type).GetSingularName().data());

    if (Task_EstimateTurnsTillMissionEnd() >=
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[task_team], unit_type)->GetAttribute(ATTRIB_TURNS)) {
        uint32_t unit_counters[UNIT_END];
        const auto builder_type = Builder_GetBuilderType(m_team, unit_type);
        int64_t builder_count = 0;
        int64_t buildable_count;
        uint32_t unit_count = 0;

        memset(unit_counters, 0, sizeof(unit_counters));

        for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
            if ((*it).GetTeam() == this->GetTeam() && (*it).GetType() == TaskType_TaskCreateUnit) {
                if ((*it).ComparePriority(task_priority + TASK_PRIORITY_ADJUST_MAJOR) <= 0) {
                    ++unit_counters[dynamic_cast<TaskCreateUnit*>(it->Get())->GetUnitType()];
                }
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == task_team && (*it).GetUnitType() == builder_type) {
                ++builder_count;
            }
        }

        for (const auto unit : Builder_GetBuildableUnits(m_team, builder_type)) {
            unit_count += unit_counters[unit];
        }

        buildable_count = (builder_count * 2 + 1) - unit_count;

        if (buildable_count > requested_amount) {
            buildable_count = requested_amount;
        }

        for (int64_t i = 0; i < buildable_count; ++i) {
            SmartPointer<TaskCreateUnit> create_unit_task(new (std::nothrow) TaskCreateUnit(unit_type, this, site));

            TaskManager.AppendTask(*create_unit_task);
        }
    }
}

void TaskObtainUnits::RemoveSelf() {
    AILOG(log, "Obtain Unit: Parent Complete");

    units.Clear();
    m_parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskObtainUnits::AddUnit(ResourceID unit_type) {
    SDL_assert(unit_type != INVALID_ID);

    units.PushBack(&unit_type);
}
