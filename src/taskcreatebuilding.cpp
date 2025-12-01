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

#include "taskcreatebuilding.hpp"

#include <format>

#include "access.hpp"
#include "accessmap.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "builder.hpp"
#include "buildmenu.hpp"
#include "cargo.hpp"
#include "continent.hpp"
#include "game_manager.hpp"
#include "hash.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "task_manager.hpp"
#include "taskactivate.hpp"
#include "taskgetmaterials.hpp"
#include "taskmanagebuildings.hpp"
#include "taskmove.hpp"
#include "taskremovemines.hpp"
#include "unit.hpp"
#include "units_manager.hpp"

enum {
    CREATE_BUILDING_STATE_INITIALIZING,
    CREATE_BUILDING_STATE_WAITING_FOR_PLATFORM,
    CREATE_BUILDING_STATE_REMOVING_MINE,
    CREATE_BUILDING_STATE_GETTING_BUILDER,
    CREATE_BUILDING_STATE_GETTING_MATERIALS,
    CREATE_BUILDING_STATE_EVALUTING_SITE,
    CREATE_BUILDING_STATE_SITE_BLOCKED,
    CREATE_BUILDING_STATE_MOVING_TO_SITE,
    CREATE_BUILDING_STATE_CLEARING_SITE,
    CREATE_BUILDING_STATE_BUILDING,
    CREATE_BUILDING_STATE_MOVING_OFF_SITE,
    CREATE_BUILDING_STATE_FINISHED,
};

static bool TaskCreateBuilding_IsMorePowerNeeded(uint16_t team);
static bool TaskCreateBuilding_IsMoreLifeNeeded(uint16_t team);
static bool TaskCreateBuilding_IsMoreFuelReservesNeeded(uint16_t team);
static int32_t TaskCreateBuilding_DetermineMapSurfaceRequirements(ResourceID unit_type, Point site);

TaskCreateBuilding::TaskCreateBuilding(Task* task, uint16_t priority_, ResourceID unit_type_, Point site_,
                                       TaskManageBuildings* manager_)
    : TaskCreate(task, priority_, unit_type_) {
    site = site_;
    manager = manager_;
    field_42 = false;
    op_state = CREATE_BUILDING_STATE_INITIALIZING;

    AILOG(log, "Task Create Building: {}", WriteStatusLog());

    SDL_assert(site.x >= 0 && site.x < ResourceManager_MapSize.x && site.y >= 0 && site.y < ResourceManager_MapSize.y);
}

TaskCreateBuilding::TaskCreateBuilding(UnitInfo* unit_, TaskManageBuildings* manager_)
    : TaskCreate(manager_, unit_), site(unit_->grid_x, unit_->grid_y) {
    if (unit_->GetOrderState() == ORDER_STATE_UNIT_READY) {
        op_state = CREATE_BUILDING_STATE_MOVING_OFF_SITE;

    } else {
        op_state = CREATE_BUILDING_STATE_BUILDING;
    }

    field_42 = false;
    manager = manager_;
}

TaskCreateBuilding::~TaskCreateBuilding() {}

int32_t TaskCreateBuilding::EstimateBuildTime() {
    int32_t result;

    if (builder && Task_vfunc28()) {
        result = builder->build_time;

    } else {
        result = 100;
    }

    return result;
}

void TaskCreateBuilding::RequestBuilder() {
    AILOG(log, "Task Create Building: Request Builder");

    const auto builder_type = Builder_GetBuilderType(unit_type);

    op_state = CREATE_BUILDING_STATE_GETTING_BUILDER;

    if (builder_type != INVALID_ID) {
        SmartPointer<TaskObtainUnits> obtain_units_task(new (std::nothrow) TaskObtainUnits(this, site));

        obtain_units_task->AddUnit(builder_type);

        TaskManager.AppendTask(*obtain_units_task);

    } else {
        RemoveSelf();
    }
}

void TaskCreateBuilding::AbandonSite() {
    if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
        if (GameManager_IsActiveTurn(builder->team)) {
            builder->move_to_grid_x = builder->grid_x;
            builder->move_to_grid_y = builder->grid_y;

            op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;

            UnitsManager_SetNewOrder(&*builder, ORDER_BUILD, ORDER_STATE_BUILD_CANCEL);
        }
    }
}

bool TaskCreateBuilding::BuildRoad() {
    bool result;

    if (builder->GetUnitType() == ENGINEER && Task_IsReadyToTakeOrders(&*builder) && builder->speed == 0 &&
        (builder->grid_x != site.x || builder->grid_y != site.y)) {
        if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= MASTER || builder->storage >= 26) {
            if (GameManager_IsActiveTurn(m_team)) {
                if (builder->storage >= 2 && Access_IsAccessible(ROAD, m_team, builder->grid_x, builder->grid_y,
                                                                 AccessModifier_EnemySameClassBlocks)) {
                    SmartObjectArray<ResourceID> build_list = builder->GetBuildList();
                    ResourceID unit_type_ = ROAD;

                    build_list.Clear();
                    build_list.PushBack(&unit_type_);

                    builder->SetBuildRate(1);

                    builder->move_to_grid_x = builder->grid_x;
                    builder->move_to_grid_y = builder->grid_y;

                    builder->path = nullptr;

                    if (Remote_IsNetworkGame) {
                        Remote_SendNetPacket_38(&*builder);
                    }

                    builder->BuildOrder();

                    result = true;

                } else {
                    result = false;
                }

            } else {
                result = false;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskCreateBuilding::BeginBuilding() {
    SmartObjectArray<ResourceID> build_list = builder->GetBuildList();

    if (!build_list.GetCount()) {
        AILOG(log, "Task Create Building: Begin Building");

        op_state = CREATE_BUILDING_STATE_BUILDING;

        if (Task_EstimateTurnsTillMissionEnd() >=
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[m_team], unit_type)->GetAttribute(ATTRIB_TURNS)) {
            if (Access_IsAccessible(unit_type, m_team, site.x, site.y, AccessModifier_EnemySameClassBlocks)) {
                if (!m_parent || m_parent->IsNeeded()) {
                    if (builder->GetTask() == this) {
                        if (!RequestWaterPlatform()) {
                            if (!RequestMineRemoval()) {
                                if (!RequestRubbleRemoval()) {
                                    if (!Ai_IsDangerousLocation(&*builder, site, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE,
                                                                false)) {
                                        if (CheckMaterials()) {
                                            if (GameManager_IsActiveTurn(m_team)) {
                                                build_list.Clear();

                                                build_list.PushBack(&unit_type);

                                                if (ResourceManager_GetSettings()->GetNumericValue("opponent") >=
                                                        OPPONENT_TYPE_AVERAGE &&
                                                    (unit_type == MININGST ||
                                                     ((builder->storage >=
                                                       builder->GetBaseValues()->GetAttribute(ATTRIB_STORAGE) - 2) &&
                                                      !TaskManager_NeedToReserveRawMaterials(m_team)))) {
                                                    builder->SetBuildRate(2);

                                                } else {
                                                    builder->SetBuildRate(1);
                                                }

                                                builder->move_to_grid_x = site.x;
                                                builder->move_to_grid_y = site.y;

                                                builder->path = nullptr;

                                                if (Remote_IsNetworkGame) {
                                                    Remote_SendNetPacket_38(&*builder);
                                                }

                                                builder->BuildOrder();

                                                RemindTurnStart(true);
                                            }
                                        }

                                    } else {
                                        Task_RetreatFromDanger(this, &*builder, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE);

                                        op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;
                                    }

                                } else {
                                    AILOG_LOG(log, "Location has rubble.");
                                }

                            } else {
                                AILOG_LOG(log, "Location has a minefield.");
                            }

                        } else {
                            AILOG_LOG(log, "Location needs a water platform.");
                        }

                    } else {
                        AILOG_LOG(log, "Builder currently under another tasks's control.");

                        op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;
                    }

                } else {
                    AILOG_LOG(log, "Parent doesn't need units.");

                    Abort();
                }

            } else {
                AILOG_LOG(log, "Site blocked.");

                Abort();
            }

        } else {
            AILOG_LOG(log, "Not enough time left.");

            Abort();
        }
    }
}

void TaskCreateBuilding::Abort() {
    AILOG(log, "Create {} at [{},{}] aborted.", ResourceManager_GetUnit(unit_type).GetSingularName().data(), site.x + 1,
          site.y + 1);

    SmartPointer<Task> create_building(this);

    op_state = CREATE_BUILDING_STATE_FINISHED;

    if (manager) {
        manager->ChildComplete(this);
    }

    if (m_parent && m_parent.Get() != manager.Get()) {
        m_parent->ChildComplete(this);
    }

    m_parent = nullptr;
    manager = nullptr;
    building = nullptr;

    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        (*it).RemoveSelf();
    }

    tasks.Clear();

    Finish();

    unit_type = INVALID_ID;
}

void TaskCreateBuilding::Finish() {
    AILOG(log, "Create {} at [{},{}] finished.", ResourceManager_GetUnit(unit_type).GetSingularName().data(),
          site.x + 1, site.y + 1);

    // prevent early destruction of object
    SmartPointer<Task> create_building_task(this);

    if (building && building->GetOrder() != ORDER_IDLE && building->GetOrder() != ORDER_AWAIT_SCALING) {
        AddUnit(*building);
    }

    if (builder) {
        if (building) {
            Activate();

            return;
        }

        builder->RemoveTask(this);

        if (!builder->GetTask()) {
            TaskManager.ClearUnitTasksAndRemindAvailable(&*builder);
        }
    }

    builder = nullptr;
    m_parent = nullptr;
    manager = nullptr;
    zone = nullptr;

    TaskManager.RemoveTask(*this);
}

bool TaskCreateBuilding::RequestMineRemoval() {
    bool result;

    if (!tasks.GetCount()) {
        Rect bounds;

        GetBounds(&bounds);

        for (int32_t x = bounds.ulx; x < bounds.lrx; ++x) {
            for (int32_t y = bounds.uly; y < bounds.lry; ++y) {
                const auto units = Hash_MapHash[Point(x, y)];
                SmartPointer<UnitInfo> unit;

                if (units) {
                    // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                        if ((*it).team == m_team &&
                            ((*it).GetUnitType() == LANDMINE || (*it).GetUnitType() == SEAMINE)) {
                            unit = *it;
                            break;
                        }
                    }
                }

                if (unit) {
                    SmartPointer<Task> remove_mines_task(new (std::nothrow) TaskRemoveMines(this, unit.Get()));

                    tasks.PushBack(*remove_mines_task);
                    TaskManager.AppendTask(*remove_mines_task);
                }
            }
        }

        return tasks.GetCount() > 0;

    } else {
        result = false;
    }

    return result;
}

bool TaskCreateBuilding::RequestRubbleRemoval() {
    if (!tasks.GetCount()) {
        Rect bounds;

        GetBounds(&bounds);

        for (int32_t x = bounds.ulx; x < bounds.lrx; ++x) {
            for (int32_t y = bounds.uly; y < bounds.lry; ++y) {
                const auto units = Hash_MapHash[Point(x, y)];
                SmartPointer<UnitInfo> unit;

                if (units) {
                    // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                        if ((*it).GetUnitType() == SMLRUBLE || (*it).GetUnitType() == LRGRUBLE) {
                            unit = *it;
                            break;
                        }
                    }
                }

                if (unit) {
                    SmartPointer<Task> remove_rubble_task(
                        new (std::nothrow) TaskRemoveRubble(this, unit.Get(), TASK_PRIORITY_REMOVE_RUBBLE));

                    tasks.PushBack(*remove_rubble_task);
                    TaskManager.AppendTask(*remove_rubble_task);

                    return true;
                }
            }
        }
    }

    return false;
}

std::string TaskCreateBuilding::WriteStatusLog() const {
    if (unit_type != INVALID_ID) {
        std::string result =
            std::format("Create a {} at [{},{}]", ResourceManager_GetUnit(unit_type).GetSingularName().data(),
                        site.x + 1, site.y + 1);

        switch (op_state) {
            case CREATE_BUILDING_STATE_INITIALIZING: {
                result += ": initializing";
            } break;

            case CREATE_BUILDING_STATE_WAITING_FOR_PLATFORM: {
                result += ": waiting for platform";
            } break;

            case CREATE_BUILDING_STATE_REMOVING_MINE: {
                result += ": removing mines";
            } break;

            case CREATE_BUILDING_STATE_GETTING_BUILDER: {
                result += ": get builder";
            } break;

            case CREATE_BUILDING_STATE_GETTING_MATERIALS: {
                result += ": get materials";
            } break;

            case CREATE_BUILDING_STATE_EVALUTING_SITE: {
                result += ": evaluating site";
            } break;

            case CREATE_BUILDING_STATE_SITE_BLOCKED: {
                result += ": site blocked";
            } break;

            case CREATE_BUILDING_STATE_MOVING_TO_SITE: {
                result += ": move to site";
            } break;

            case CREATE_BUILDING_STATE_CLEARING_SITE: {
                result += ": clear site";
            } break;

            case CREATE_BUILDING_STATE_BUILDING: {
                result += ": building";
            } break;

            case CREATE_BUILDING_STATE_MOVING_OFF_SITE: {
                result += ": move off site";
            } break;

            case CREATE_BUILDING_STATE_FINISHED: {
                result += ": finished.";
            } break;

            default: {
                result += ": UNKNOWN STATE!";
            } break;
        }

        if (builder && builder->GetBuildRate() > 1) {
            result += " at x2 rate";
        }

        return result;

    } else {
        return "Create building aborted.";
    }
}

Rect* TaskCreateBuilding::GetBounds(Rect* bounds) {
    int32_t unit_size;

    if (ResourceManager_GetUnit(unit_type).GetFlags() & BUILDING) {
        unit_size = 2;

    } else {
        unit_size = 1;
    }

    bounds->ulx = site.x;
    bounds->uly = site.y;
    bounds->lrx = site.x + unit_size;
    bounds->lry = site.y + unit_size;

    return bounds;
}

uint8_t TaskCreateBuilding::GetType() const { return TaskType_TaskCreateBuilding; }

bool TaskCreateBuilding::IsNeeded() {
    return op_state < CREATE_BUILDING_STATE_BUILDING && (!m_parent || m_parent->IsNeeded());
}

void TaskCreateBuilding::AddUnit(UnitInfo& unit) {
    if (unit_type != INVALID_ID) {
        AILOG(log, "Build {} at [{},{}]: Add {}", ResourceManager_GetUnit(unit_type).GetSingularName().data(),
              site.x + 1, site.y + 1, ResourceManager_GetUnit(unit.GetUnitType()).GetSingularName().data());

        if (unit.GetUnitType() == unit_type && unit.grid_x == site.x && unit.grid_y == site.y &&
            unit.GetOrder() != ORDER_IDLE) {
            SmartPointer<Task> create_building_task(this);

            op_state = CREATE_BUILDING_STATE_FINISHED;

            unit.RemoveTasks();

            if (manager) {
                manager->ChildComplete(this);
                manager->AddUnit(unit);
            }

            if (unit_type == WTRPLTFM || unit_type == BRIDGE) {
                if (manager.Get() != m_parent.Get()) {
                    m_parent->AddUnit(unit);
                }

            } else {
                TaskManager.ClearUnitTasksAndRemindAvailable(&unit);
            }

            if (manager.Get() != m_parent.Get()) {
                m_parent->ChildComplete(this);
            }

            if (builder && builder->GetTask() == this && m_parent && unit_type == WTRPLTFM &&
                m_parent->GetType() == TaskType_TaskCreateBuilding) {
                TaskCreateBuilding* create_building = dynamic_cast<TaskCreateBuilding*>(m_parent.Get());

                if (create_building->op_state == CREATE_BUILDING_STATE_WAITING_FOR_PLATFORM &&
                    Builder_GetBuilderType(create_building->GetUnitType()) == builder->GetUnitType()) {
                    SmartPointer<UnitInfo> backup(builder);

                    backup->RemoveTasks();

                    dynamic_cast<TaskCreateBuilding*>(m_parent.Get())->AddUnit(*backup);
                }
            }

            m_parent = nullptr;
            manager = nullptr;
            building = nullptr;

            tasks.Clear();

            Finish();

        } else if (unit.GetUnitType() == Builder_GetBuilderType(unit_type)) {
            if (!builder) {
                if (op_state < CREATE_BUILDING_STATE_BUILDING) {
                    op_state = CREATE_BUILDING_STATE_GETTING_MATERIALS;
                    builder = unit;
                    builder->AddTask(this);
                    Task_RemindMoveFinished(&*builder);

                } else {
                    AILOG_LOG(log, "Task already complete.");
                }

            } else {
                AILOG_LOG(log, "Already have builder.");
            }
        }
    }
}

void TaskCreateBuilding::Init() {
    AILOG(log, "Task Create Building: Begin");

    if (builder) {
        builder->AddTask(this);
        if (builder->GetOrderState() == ORDER_STATE_UNIT_READY) {
            Activate();
        }

    } else {
        RemindTurnStart(true);
    }
}

void TaskCreateBuilding::BeginTurn() {
    if (op_state == CREATE_BUILDING_STATE_SITE_BLOCKED) {
        op_state = CREATE_BUILDING_STATE_EVALUTING_SITE;
    }

    if (op_state < CREATE_BUILDING_STATE_BUILDING && m_parent && !m_parent->IsNeeded()) {
        Abort();

    } else {
        EndTurn();
    }
}

void TaskCreateBuilding::ChildComplete(Task* task) { tasks.Remove(*task); }

void TaskCreateBuilding::EndTurn() {
    if (builder) {
        switch (op_state) {
            case CREATE_BUILDING_STATE_EVALUTING_SITE: {
                AILOG(log, "Task Create {} at [{},{}]: evaluating site",
                      ResourceManager_GetUnit(unit_type).GetSingularName().data(), site.x + 1, site.y + 1);

                FindBuildSite();
            } break;

            case CREATE_BUILDING_STATE_SITE_BLOCKED: {
                if (IsProcessingNeeded()) {
                    AILOG(log, "Task Create {} at [{},{}]: site blocked",
                          ResourceManager_GetUnit(unit_type).GetSingularName().data(), site.x + 1, site.y + 1);

                    SetProcessingNeeded(false);

                    FindBuildSite();
                }
            } break;

            case CREATE_BUILDING_STATE_MOVING_TO_SITE: {
                if (!BuildRoad() && builder->GetOrder() == ORDER_AWAIT && builder->GetTask() == this) {
                    Execute(*builder);
                }
            } break;

            case CREATE_BUILDING_STATE_BUILDING: {
                if (builder->GetTask() == this) {
                    if (builder->GetOrder() == ORDER_AWAIT) {
                        op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;

                        RemindTurnEnd(true);

                    } else if (builder->GetOrderState() == ORDER_STATE_UNIT_READY && builder->GetTask() == this) {
                        Activate();

                    } else if (builder->GetOrder() == ORDER_BUILD &&
                               builder->GetOrderState() == ORDER_STATE_BUILD_IN_PROGRESS &&
                               Ai_IsDangerousLocation(&*builder, site, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, false)) {
                        AbandonSite();
                    }
                }
            } break;

            case CREATE_BUILDING_STATE_FINISHED: {
                if (builder->GetOrder() == ORDER_AWAIT && builder->GetTask() == this) {
                    Finish();
                }
            } break;

            default: {
                Execute(*builder);
            } break;
        }

    } else if (unit_type != INVALID_ID && op_state != CREATE_BUILDING_STATE_GETTING_BUILDER &&
               op_state != CREATE_BUILDING_STATE_BUILDING && !tasks.GetCount()) {
        RequestBuilder();
    }
}

bool TaskCreateBuilding::ExchangeOperator(UnitInfo& unit_) {
    bool result;

    if (builder && builder->GetUnitType() == unit_.GetUnitType()) {
        if (op_state < CREATE_BUILDING_STATE_BUILDING) {
            if (Access_GetSquaredDistance(&*builder, site) > Access_GetSquaredDistance(&unit_, site)) {
                AILOG(log, "Task Create Building: Exchange unit.");

                TaskManager.ClearUnitTasksAndRemindAvailable(&*builder);

                if (!builder) {
                    AddUnit(unit_);
                }

                result = true;

            } else {
                result = false;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskCreateBuilding::Execute(UnitInfo& unit) {
    bool result;

    AILOG(log, "Task Create Building: Move Finished.");

    if (builder == unit && unit.IsReadyForOrders(this)) {
        if (Task_RetreatFromDanger(this, &unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
            result = true;

        } else {
            switch (op_state) {
                case CREATE_BUILDING_STATE_MOVING_TO_SITE: {
                    if (BuildRoad()) {
                        result = true;

                    } else if (CheckMaterials()) {
                        if (builder->grid_x == site.x && builder->grid_y == site.y) {
                            if (TaskCreateBuilding_DetermineMapSurfaceRequirements(unit_type, site)) {
                                if (ResourceManager_GetUnit(unit_type).GetFlags() & BUILDING) {
                                    Rect bounds;

                                    op_state = CREATE_BUILDING_STATE_CLEARING_SITE;

                                    rect_init(&bounds, site.x, site.y, site.x + 2, site.y + 2);

                                    zone = new (std::nothrow) Zone(&*builder, this, &bounds);

                                    AiPlayer_Teams[m_team].ClearZone(&*zone);

                                    result = true;

                                } else {
                                    BeginBuilding();

                                    result = true;
                                }

                            } else {
                                AILOG_LOG(log, "Site blocked.");

                                Abort();

                                result = false;
                            }

                        } else {
                            int32_t minimum_distance;
                            Point line_distance;

                            if (tasks.GetCount()) {
                                // do we wait for rubble removal even though we want to build a connector?
                                if (tasks[0].GetType() == TaskType_TaskRemoveRubble && unit_type == CNCT_4W) {
                                    minimum_distance = 0;

                                } else {
                                    minimum_distance = 8;
                                }

                            } else {
                                minimum_distance = 0;
                            }

                            op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;

                            line_distance.x = builder->grid_x - site.x;
                            line_distance.y = builder->grid_y - site.y;

                            if (line_distance.x * line_distance.x + line_distance.y * line_distance.y >
                                minimum_distance) {
                                SmartPointer<TaskMove> move_task(new (std::nothrow) TaskMove(
                                    &*builder, this, minimum_distance, CAUTION_LEVEL_AVOID_ALL_DAMAGE, site,
                                    &MoveFinishedCallback));

                                TaskManager.AppendTask(*move_task);

                                result = true;

                            } else {
                                result = false;
                            }
                        }

                    } else {
                        result = true;
                    }
                } break;

                case CREATE_BUILDING_STATE_GETTING_MATERIALS: {
                    if (CheckMaterials()) {
                        if (manager && !Task_vfunc29()) {
                            AILOG_LOG(log, "Materials now available.");

                            FindBuildSite();

                            result = true;

                        } else if (RequestWaterPlatform() || RequestMineRemoval() || RequestRubbleRemoval()) {
                            result = true;

                        } else {
                            op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;

                            Execute(unit);

                            result = true;
                        }

                    } else {
                        result = true;
                    }
                } break;

                case CREATE_BUILDING_STATE_MOVING_OFF_SITE:
                case CREATE_BUILDING_STATE_FINISHED: {
                    if (builder->GetOrder() == ORDER_AWAIT) {
                        Finish();
                    }

                    result = true;
                } break;

                default: {
                    result = true;
                } break;
            }
        }

    } else {
        result = false;
    }

    return result;
}

void TaskCreateBuilding::RemoveSelf() { Abort(); }

void TaskCreateBuilding::RemoveUnit(UnitInfo& unit) {
    AILOG(log, "Create {} at [{},{}]: remove {}.", ResourceManager_GetUnit(unit_type).GetSingularName().data(),
          site.x + 1, site.y + 1, ResourceManager_GetUnit(unit.GetUnitType()).GetSingularName().data());

    if (builder == unit) {
        builder = nullptr;
        zone = nullptr;
    }
}

void TaskCreateBuilding::EventZoneCleared(Zone* zone_, bool status) {
    AILOG(log, "Task Create Building: zone cleared.");

    if (zone == zone_) {
        zone = nullptr;

        if (op_state == CREATE_BUILDING_STATE_CLEARING_SITE && builder) {
            if (status) {
                if (builder->grid_x == site.x && builder->grid_y == site.y) {
                    BeginBuilding();

                } else {
                    op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;

                    SmartPointer<TaskMove> move_task(new (std::nothrow) TaskMove(
                        &*builder, this, 0, CAUTION_LEVEL_AVOID_ALL_DAMAGE, site, &MoveFinishedCallback));

                    TaskManager.AppendTask(*move_task);
                }

            } else {
                AILOG_LOG(log, "Zone could not be cleared.");

                FindBuildSite();
            }
        }
    }
}

bool TaskCreateBuilding::Task_vfunc28() { return op_state >= CREATE_BUILDING_STATE_BUILDING; }

void TaskCreateBuilding::Activate() {
    if (!building && builder->GetOrder() == ORDER_BUILD) {
        building = builder->GetParent();

        if (building->GetTask() != this) {
            building->AddTask(this);
        }
    }

    op_state = CREATE_BUILDING_STATE_MOVING_OFF_SITE;

    SmartPointer<TaskActivate> activate_task(new (std::nothrow) TaskActivate(&*builder, this, &*building));

    TaskManager.AppendTask(*activate_task);
}

void TaskCreateBuilding::FindBuildSite() {
    AILOG(log, "Task Create Building: Choose new site for {}.",
          ResourceManager_GetUnit(unit_type).GetSingularName().data());

    if (builder->GetTask() == this) {
        Point new_site;

        op_state = CREATE_BUILDING_STATE_EVALUTING_SITE;

        if (manager && unit_type != CNCT_4W && unit_type != WTRPLTFM) {
            SDL_assert(manager->GetType() == TaskType_TaskManageBuildings);

            if (manager->ChangeSite(this, new_site)) {
                if (unit_type != INVALID_ID) {
                    site = new_site;

                    AILOG_LOG(log, "New site for {} is [{}, {}].",
                              ResourceManager_GetUnit(unit_type).GetSingularName().data(), site.x + 1, site.y + 1);

                    SDL_assert(site.x > 0 && site.y > 0);

                    op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;

                    if (builder) {
                        if (!RequestWaterPlatform() && !RequestMineRemoval() && !RequestRubbleRemoval() &&
                            !IsScheduledForTurnEnd()) {
                            TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
                        }

                    } else {
                        AILOG_LOG(log, "Requirements for site pre-empted builder.");
                    }

                } else {
                    AILOG_LOG(log, "Site blocked and no alternative found.");
                }

            } else {
                AILOG_LOG(log, "Site blocked and no alternative found.");

                Abort();
            }

        } else {
            AILOG_LOG(log, "Site no longer valid for connector/platform, or no manager.");

            Abort();
        }

    } else {
        AILOG_LOG(log, "Builder pre-empted by another task.");
    }
}

bool TaskCreateBuilding::RequestWaterPlatform() {
    bool result;

    if (!tasks.GetCount() && TaskCreateBuilding_DetermineMapSurfaceRequirements(unit_type, site) == 2) {
        Rect bounds;
        Point position;

        AILOG(log, "Requesting water platform.");

        GetBounds(&bounds);

        op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;

        SDL_assert(site.x > 0 && site.y > 0);

        if (builder) {
            op_state = CREATE_BUILDING_STATE_WAITING_FOR_PLATFORM;
            builder->RemoveTasks();
        }

        if (ResourceManager_GetUnit(unit_type).GetFlags() & BUILDING) {
            BuildBridges();
        }

        BuildBoardwalks();

        for (position.x = bounds.ulx; position.x < bounds.lrx; ++position.x) {
            for (position.y = bounds.uly; position.y < bounds.lry; ++position.y) {
                if (TaskCreateBuilding_DetermineMapSurfaceRequirements(WTRPLTFM, position) == 1) {
                    field_42 = true;

                    SmartPointer<TaskCreateBuilding> create_building_task(new (std::nothrow) TaskCreateBuilding(
                        this, GetPriority() - TASK_PRIORITY_ADJUST_MINOR, WTRPLTFM, position, nullptr));

                    tasks.PushBack(*create_building_task);
                    TaskManager.AppendTask(*create_building_task);
                }
            }
        }

        result = true;

    } else {
        BuildBoardwalks();

        result = false;
    }

    return result;
}

bool TaskCreateBuilding::CheckMaterials() {
    bool result;

    if (builder) {
        if (Cargo_GetRawConsumptionRate(unit_type, 1) > 0 && Task_GetReadyUnitsCount(m_team, unit_type) > 0 &&
            TaskManager_NeedToReserveRawMaterials(m_team)) {
            op_state = CREATE_BUILDING_STATE_GETTING_MATERIALS;

            result = false;

        } else if (Cargo_GetPowerConsumptionRate(unit_type) > 0 && Task_GetReadyUnitsCount(m_team, unit_type) > 0 &&
                   (TaskCreateBuilding_IsMorePowerNeeded(m_team) ||
                    (unit_type != MININGST && TaskCreateBuilding_IsMoreFuelReservesNeeded(m_team)))) {
            op_state = CREATE_BUILDING_STATE_GETTING_MATERIALS;

            result = false;

        } else if (Cargo_GetLifeConsumptionRate(unit_type) > 0 && Task_GetReadyUnitsCount(m_team, unit_type) > 0 &&
                   TaskCreateBuilding_IsMoreLifeNeeded(m_team)) {
            op_state = CREATE_BUILDING_STATE_GETTING_MATERIALS;

            result = false;

        } else {
            int32_t resource_demand =
                BuildMenu_GetTurnsToBuild(unit_type, m_team) * Cargo_GetRawConsumptionRate(builder->GetUnitType(), 1);

            if (TaskCreateBuilding_DetermineMapSurfaceRequirements(unit_type, site) == 2 &&
                Builder_GetBuilderType(WTRPLTFM) == builder->GetUnitType()) {
                resource_demand += BuildMenu_GetTurnsToBuild(WTRPLTFM, m_team) *
                                   Cargo_GetRawConsumptionRate(builder->GetUnitType(), 1);
            }

            if (builder->storage >= resource_demand) {
                result = true;

            } else {
                op_state = CREATE_BUILDING_STATE_GETTING_MATERIALS;

                SmartPointer<TaskGetMaterials> get_materials_task(
                    new (std::nothrow) TaskGetMaterials(this, &*builder, resource_demand));

                TaskManager.AppendTask(*get_materials_task);

                result = false;
            }
        }

    } else {
        result = false;
    }

    return result;
}

void TaskCreateBuilding::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    TaskCreateBuilding* search_task = dynamic_cast<TaskCreateBuilding*>(task);

    if (result == TASKMOVE_RESULT_SUCCESS) {
        Task_RemindMoveFinished(unit);

    } else if (result == TASKMOVE_RESULT_BLOCKED) {
        search_task->op_state = CREATE_BUILDING_STATE_SITE_BLOCKED;

        for (SmartList<Task>::Iterator it = search_task->tasks.Begin(); it != search_task->tasks.End(); ++it) {
            (*it).RemoveSelf();
        }

        search_task->tasks.Clear();
    }
}

void TaskCreateBuilding::BuildBoardwalks() {
    if (unit_type != WTRPLTFM && unit_type != BRIDGE && unit_type != CNCT_4W && unit_type != SHIPYARD &&
        unit_type != DOCK && (ResourceManager_GetUnit(unit_type).GetFlags() & BUILDING)) {
        Point position;
        AccessMap map;
        Rect bounds;
        int32_t range_limit;

        AILOG(log, "Create boardwalks around unit.");

        MarkBridgeAreas(map);

        PopulateMap(map);

        GetBounds(&bounds);

        position.x = bounds.ulx - 1;
        position.y = bounds.lry;

        range_limit = bounds.lrx - bounds.ulx;

        for (int32_t direction = 0; direction < 8; direction += 2) {
            for (int32_t range = 0; range < range_limit; ++range) {
                position += DIRECTION_OFFSETS[direction];

                if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                    position.y < ResourceManager_MapSize.y) {
                    if (map(position.x, position.y) == 1) {
                        AILOG_LOG(log, "Create boardwalk at [{},{}].", position.x + 1, position.y + 1);

                        SmartPointer<TaskCreateBuilding> create_building_task = new (std::nothrow) TaskCreateBuilding(
                            this, TASK_PRIORITY_BRIDGE_BASE | TASK_PRIORITY_ADJUST_MINOR, BRIDGE, position, &*manager);

                        if (manager) {
                            manager->AddCreateOrder(&*create_building_task);

                        } else {
                            AILOG_LOG(log, "No building manager!");

                            TaskManager.AppendTask(*create_building_task);
                        }
                    }
                }
            }

            position += DIRECTION_OFFSETS[direction];
        }
    }
}

void TaskCreateBuilding::BuildBridges() {
    AccessMap map;
    bool spot_found;
    Point position;

    AILOG(log, "Building bridges.");

    MarkBridgeAreas(map);

    spot_found = false;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == m_team && (*it).GetUnitType() == MININGST) {
            position.x = (*it).grid_x - 1;
            position.y = (*it).grid_y + 2;

            for (int32_t direction = 0; direction < 8 && !spot_found; direction += 2) {
                for (int32_t range = 0; range < 3 && !spot_found; ++range) {
                    position += DIRECTION_OFFSETS[direction];
                    if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                        position.y < ResourceManager_MapSize.y) {
                        if (map(position.x, position.y) == 2) {
                            spot_found = true;
                        }
                    }
                }
            }

            if (spot_found) {
                break;
            }
        }
    }

    if (spot_found) {
        PopulateMap(map);

        SmartPointer<Continent> continent(new (std::nothrow) Continent(map, 3, position, 0));

        if (!FindBridgePath(map, 3)) {
            FindBridgePath(map, 4);
        }

    } else {
        AILOG_LOG(log, "No starting point found.");
    }
}

void TaskCreateBuilding::MarkBridgeAreas(AccessMap& map) {
    int16_t** damage_potential_map;

    AILOG(log, "Marking bridge areas.");

    for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            switch (ResourceManager_MapSurfaceMap[y * ResourceManager_MapSize.x + x]) {
                case SURFACE_TYPE_LAND: {
                    map(x, y) = 2;
                } break;

                case SURFACE_TYPE_WATER:
                case SURFACE_TYPE_COAST: {
                    map(x, y) = 1;
                } break;

                default: {
                    map(x, y) = 0;
                } break;
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetUnitType() != CNCT_4W) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            for (int32_t x = bounds.ulx; x < bounds.lrx; ++x) {
                for (int32_t y = bounds.uly; y < bounds.lry; ++y) {
                    map(x, y) = 4;
                }
            }
        }
    }

    damage_potential_map = AiPlayer_Teams[m_team].GetDamagePotentialMap(ENGINEER, CAUTION_LEVEL_AVOID_ALL_DAMAGE, true);

    for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
        for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
            if (damage_potential_map[x][y] > 0) {
                map(x, y) = 0;
            }
        }
    }

    for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
         ++it) {
        if ((*it).GetTeam() == m_team && (*it).GetType() == TaskType_TaskCreateBuilding) {
            Rect bounds;
            TaskCreateBuilding* create_building_task = dynamic_cast<TaskCreateBuilding*>(&*it);

            create_building_task->GetBounds(&bounds);

            if (create_building_task->GetUnitType() != BRIDGE && create_building_task->GetUnitType() != WTRPLTFM &&
                create_building_task->GetUnitType() != CNCT_4W) {
                for (int32_t x = bounds.ulx; x < bounds.lrx; ++x) {
                    for (int32_t y = bounds.uly; y < bounds.lry; ++y) {
                        map(x, y) = 4;
                    }
                }
            }
        }
    }
}

void TaskCreateBuilding::PopulateMap(AccessMap& map) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
         it != UnitsManager_GroundCoverUnits.End(); ++it) {
        if ((*it).GetUnitType() == BRIDGE && (*it).IsVisibleToTeam(m_team)) {
            map((*it).grid_x, (*it).grid_y) = 2;
        }
    }

    for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
         ++it) {
        if ((*it).GetTeam() == m_team && (*it).GetType() == TaskType_TaskCreateBuilding) {
            Rect bounds;
            TaskCreateBuilding* create_building_task = dynamic_cast<TaskCreateBuilding*>(&*it);

            create_building_task->GetBounds(&bounds);

            if (create_building_task->GetUnitType() == BRIDGE) {
                if (map(bounds.ulx, bounds.uly) == 1) {
                    map(bounds.ulx, bounds.uly) = 2;
                }
            }
        }
    }

    for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            if ((ResourceManager_CargoMap[y * ResourceManager_MapSize.x + x] & 0x1F) >= 8) {
                if (map(x, y) == 1) {
                    map(x, y) = 0;
                }
            }
        }
    }
}

bool TaskCreateBuilding::FindBridgePath(AccessMap& map, int32_t value) {
    Point position;
    Point start_position;
    bool is_found = false;
    uint16_t bridge_priority;
    uint16_t bridge_count;
    Rect bounds;
    int32_t range_limit;
    int32_t direction2;

    AILOG(log, "Find bridge path.");

    GetBounds(&bounds);

    range_limit = bounds.lrx - bounds.ulx + 1;

    position.x = bounds.ulx - 1;
    position.y = bounds.lry;

    bridge_count = SHRT_MAX;

    for (int32_t direction = 0; direction < 8; direction += 2) {
        for (int32_t range = 0; range < range_limit; ++range) {
            position += DIRECTION_OFFSETS[direction];

            if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                position.y < ResourceManager_MapSize.y) {
                if (map(position.x, position.y) == value) {
                    AILOG_LOG(log, "Unit is already connected.");

                    return true;

                } else if (SearchPathStep(map, position, &direction2, &bridge_count, value)) {
                    start_position = position;

                    is_found = true;

                    AILOG_LOG(log, "Found: start point [{},{}], {} bridges.", position.x + 1, position.y + 1,
                              bridge_count);
                }
            }
        }
    }

    bridge_priority = GetPriority() - TASK_PRIORITY_ADJUST_BRIDGE;

    if (unit_type != LIGHTPLT && unit_type != LANDPLT && unit_type != DEPOT && unit_type != TRAINHAL) {
        bridge_priority = TASK_PRIORITY_BRIDGE_BASE;
    }

    if (is_found) {
        SmartPointer<TaskCreateBuilding> create_building_task;

        position = start_position;

        while (bridge_count > 0) {
            if (map(position.x, position.y) == 1) {
                AILOG_LOG(log, "Create bridge at [{},{}].", position.x + 1, position.y + 1);

                create_building_task = new (std::nothrow)
                    TaskCreateBuilding(this, bridge_priority + bridge_count, BRIDGE, position, &*manager);

                tasks.PushBack(*create_building_task);

                if (manager) {
                    manager->AddCreateOrder(&*create_building_task);

                } else {
                    AILOG_LOG(log, "No building manager!");

                    TaskManager.AppendTask(*create_building_task);
                }
            }

            position += DIRECTION_OFFSETS[direction2];

            SearchPathStep(map, position, &direction2, &bridge_count, value);
        }

    } else {
        AILOG_LOG(log, "No connection found.");
    }

    return is_found;
}

bool TaskCreateBuilding::SearchPathStep(AccessMap& map, Point position, int32_t* direction, uint16_t* best_unit_count,
                                        int32_t value) {
    Point site1;
    Point site2;
    Point site3;
    bool result = false;
    uint16_t unit_count;
    bool is_found;

    for (int32_t local_direction = 0; local_direction < 8; local_direction += 2) {
        unit_count = 0;
        is_found = false;

        site1 = position;
        site2 = position;
        site3 = position;

        site2 += DIRECTION_OFFSETS[(local_direction + 2) & 7];
        site3 += DIRECTION_OFFSETS[(local_direction + 6) & 7];

        if (site2.x < 0 || site2.x >= ResourceManager_MapSize.x || site2.y < 0 ||
            site2.y >= ResourceManager_MapSize.x) {
            site2 = site1;
        }

        if (site3.x < 0 || site3.x >= ResourceManager_MapSize.x || site3.y < 0 ||
            site3.y >= ResourceManager_MapSize.x) {
            site3 = site1;
        }

        while (site1.x >= 0 && site1.x < ResourceManager_MapSize.x && site1.y >= 0 &&
               site1.y < ResourceManager_MapSize.y && unit_count < *best_unit_count) {
            if (map(site1.x, site1.y) == value || map(site2.x, site2.y) == value || map(site3.x, site3.y) == value) {
                is_found = true;
                break;
            }

            if (map(site1.x, site1.y) == 1) {
                ++unit_count;
            }

            if (map(site1.x, site1.y) == 0) {
                break;
            }

            site1 += DIRECTION_OFFSETS[local_direction];
            site2 += DIRECTION_OFFSETS[local_direction];
            site3 += DIRECTION_OFFSETS[local_direction];
        }

        if (is_found) {
            result = true;

            *direction = local_direction;
            *best_unit_count = unit_count;
        }
    }

    return result;
}

bool TaskCreateBuilding::Task_vfunc29() {
    bool result;

    if (unit_type == CNCT_4W || unit_type == WTRPLTFM || unit_type == BRIDGE || unit_type == MININGST) {
        result = true;

    } else if (op_state == CREATE_BUILDING_STATE_SITE_BLOCKED || op_state == CREATE_BUILDING_STATE_EVALUTING_SITE) {
        result = false;

    } else if (op_state >= CREATE_BUILDING_STATE_MOVING_TO_SITE || tasks.GetCount() || field_42) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

bool TaskCreateBuilding_IsMorePowerNeeded(uint16_t team) {
    bool result;
    int32_t consumption_rate = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team) {
            consumption_rate += Cargo_GetPowerConsumptionRate((*it).GetUnitType());
        }
    }

    if (consumption_rate > 0) {
        for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
             ++it) {
            if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskCreateBuilding) {
                TaskCreateBuilding* create_building_Task = dynamic_cast<TaskCreateBuilding*>(&*it);

                if (create_building_Task->Task_vfunc28()) {
                    consumption_rate += Cargo_GetPowerConsumptionRate(create_building_Task->GetUnitType());
                }
            }
        }

        result = consumption_rate > 0;

    } else {
        result = false;
    }

    return result;
}

bool TaskCreateBuilding_IsMoreLifeNeeded(uint16_t team) {
    bool result;
    int32_t consumption_rate = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team) {
            consumption_rate += Cargo_GetLifeConsumptionRate((*it).GetUnitType());
        }
    }

    if (consumption_rate > 0) {
        for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
             ++it) {
            if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskCreateBuilding) {
                TaskCreateBuilding* create_building_Task = dynamic_cast<TaskCreateBuilding*>(&*it);

                if (create_building_Task->Task_vfunc28()) {
                    consumption_rate += Cargo_GetLifeConsumptionRate(create_building_Task->GetUnitType());
                }
            }
        }

        result = consumption_rate > 0;

    } else {
        result = false;
    }

    return result;
}

bool TaskCreateBuilding_IsMoreFuelReservesNeeded(uint16_t team) {
    int32_t fuel_reserves = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == FDUMP) {
            fuel_reserves += (*it).storage;
        }
    }

    return fuel_reserves < 10;
}

int32_t TaskCreateBuilding_DetermineMapSurfaceRequirements(ResourceID unit_type, Point site) {
    Rect bounds;
    bool water_present;
    bool land_present;
    bool coast_present;
    int32_t result;
    const auto& base_unit = ResourceManager_GetUnit(unit_type);

    rect_init(&bounds, site.x, site.y, site.x + 1, site.y + 1);

    water_present = false;
    land_present = false;
    coast_present = false;

    if (base_unit.GetFlags() & BUILDING) {
        ++bounds.lrx;
        ++bounds.lry;
    }

    if (bounds.ulx >= 0 && bounds.uly >= 0 && bounds.lrx <= ResourceManager_MapSize.x &&
        bounds.lry <= ResourceManager_MapSize.y) {
        for (int32_t x = bounds.ulx; x < bounds.lrx; ++x) {
            for (int32_t y = bounds.uly; y < bounds.lry; ++y) {
                switch (Access_GetModifiedSurfaceType(x, y)) {
                    case SURFACE_TYPE_LAND: {
                        land_present = true;
                    } break;

                    case SURFACE_TYPE_WATER: {
                        water_present = true;
                    } break;

                    case SURFACE_TYPE_COAST: {
                        coast_present = true;
                    } break;

                    default: {
                        return 0;
                    } break;
                }
            }
        }

        if (!(base_unit.GetLandType() & SURFACE_TYPE_LAND) && land_present) {
            result = 0;

        } else if (!(base_unit.GetLandType() & SURFACE_TYPE_WATER) && water_present) {
            result = 2;

        } else if (!(base_unit.GetLandType() & SURFACE_TYPE_COAST) && coast_present) {
            result = 2;

        } else {
            result = 1;
        }

    } else {
        result = 0;
    }

    return result;
}
