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

#include "access.hpp"
#include "accessmap.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "builder.hpp"
#include "buildmenu.hpp"
#include "cargo.hpp"
#include "continent.hpp"
#include "hash.hpp"
#include "inifile.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "task_manager.hpp"
#include "taskactivate.hpp"
#include "taskgetmaterials.hpp"
#include "taskmanagebuildings.hpp"
#include "taskmove.hpp"
#include "taskremovemines.hpp"
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

TaskCreateBuilding::TaskCreateBuilding(Task* task, uint16_t flags_, ResourceID unit_type_, Point site_,
                                       TaskManageBuildings* manager_)
    : TaskCreate(task, flags_, unit_type_) {
    char buffer[200];
    site = site_;
    manager = manager_;
    field_42 = false;
    op_state = CREATE_BUILDING_STATE_INITIALIZING;

    AiLog("Task Create Building: %s", WriteStatusLog(buffer));

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
    AiLog log("Task Create Building: Request Builder");

    SmartPointer<TaskObtainUnits> obtain_units_task(new (std::nothrow) TaskObtainUnits(this, site));

    op_state = CREATE_BUILDING_STATE_GETTING_BUILDER;

    obtain_units_task->AddUnit(Builder_GetBuilderType(unit_type));

    TaskManager.AppendTask(*obtain_units_task);
}

void TaskCreateBuilding::AbandonSite() {
    if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
        if (GameManager_IsActiveTurn(builder->team)) {
            builder->target_grid_x = builder->grid_x;
            builder->target_grid_y = builder->grid_y;

            op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;

            UnitsManager_SetNewOrder(&*builder, ORDER_BUILD, ORDER_STATE_BUILD_CANCEL);
        }
    }
}

bool TaskCreateBuilding::BuildRoad() {
    bool result;

    if (builder->GetUnitType() == ENGINEER && Task_IsReadyToTakeOrders(&*builder) && builder->speed == 0 &&
        (builder->grid_x != site.x || builder->grid_y != site.y)) {
        if (ini_get_setting(INI_OPPONENT) >= MASTER || builder->storage >= 26) {
            if (GameManager_IsActiveTurn(team)) {
                if (builder->storage >= 2 && Access_IsAccessible(ROAD, team, builder->grid_x, builder->grid_y,
                                                                 AccessModifier_EnemySameClassBlocks)) {
                    SmartObjectArray<ResourceID> build_list = builder->GetBuildList();
                    ResourceID unit_type_ = ROAD;

                    build_list.Clear();
                    build_list.PushBack(&unit_type_);

                    builder->SetBuildRate(1);

                    builder->target_grid_x = builder->grid_x;
                    builder->target_grid_y = builder->grid_y;

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
        AiLog log("Task Create Building: Begin Building");

        op_state = CREATE_BUILDING_STATE_BUILDING;

        if (Task_EstimateTurnsTillMissionEnd() >=
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)->GetAttribute(ATTRIB_TURNS)) {
            if (Access_IsAccessible(unit_type, team, site.x, site.y, AccessModifier_EnemySameClassBlocks)) {
                if (!parent || parent->IsNeeded()) {
                    if (builder->GetTask() == this) {
                        if (!RequestWaterPlatform()) {
                            if (!RequestMineRemoval()) {
                                if (!RequestRubbleRemoval()) {
                                    if (!Ai_IsDangerousLocation(&*builder, site, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE,
                                                                false)) {
                                        if (CheckMaterials()) {
                                            if (GameManager_IsActiveTurn(team)) {
                                                build_list.Clear();

                                                build_list.PushBack(&unit_type);

                                                if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE &&
                                                    (unit_type == MININGST ||
                                                     ((builder->storage >=
                                                       builder->GetBaseValues()->GetAttribute(ATTRIB_STORAGE) - 2) &&
                                                      !TaskManager_NeedToReserveRawMaterials(team)))) {
                                                    builder->SetBuildRate(2);

                                                } else {
                                                    builder->SetBuildRate(1);
                                                }

                                                builder->target_grid_x = site.x;
                                                builder->target_grid_y = site.y;

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
                                    log.Log("Location has rubble.");
                                }

                            } else {
                                log.Log("Location has a minefield.");
                            }

                        } else {
                            log.Log("Location needs a water platform.");
                        }

                    } else {
                        log.Log("Builder currently under another tasks's control.");

                        op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;
                    }

                } else {
                    log.Log("Parent doesn't need units.");

                    Abort();
                }

            } else {
                log.Log("Site blocked.");

                Abort();
            }

        } else {
            log.Log("Not enough time left.");

            Abort();
        }
    }
}

void TaskCreateBuilding::Abort() {
    AiLog log("Create %s at [%i,%i] aborted.", UnitsManager_BaseUnits[unit_type].singular_name, site.x + 1, site.y + 1);

    SmartPointer<Task> create_building(this);

    op_state = CREATE_BUILDING_STATE_FINISHED;

    if (manager) {
        manager->ChildComplete(this);
    }

    if (parent && parent.Get() != manager.Get()) {
        parent->ChildComplete(this);
    }

    parent = nullptr;
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
    AiLog log("Create %s at [%i,%i] finished.", UnitsManager_BaseUnits[unit_type].singular_name, site.x + 1,
              site.y + 1);

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
            TaskManager.RemindAvailable(&*builder);
        }
    }

    builder = nullptr;
    parent = nullptr;
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
                        if ((*it).team == team && ((*it).GetUnitType() == LANDMINE || (*it).GetUnitType() == SEAMINE)) {
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
                    SmartPointer<Task> remove_rubble_task(new (std::nothrow) TaskRemoveRubble(this, unit.Get(), 0x200));

                    tasks.PushBack(*remove_rubble_task);
                    TaskManager.AppendTask(*remove_rubble_task);

                    return true;
                }
            }
        }
    }

    return false;
}

char* TaskCreateBuilding::WriteStatusLog(char* buffer) const {
    if (unit_type != INVALID_ID) {
        sprintf(buffer, "Create a %s at [%i,%i]", UnitsManager_BaseUnits[unit_type].singular_name, site.x + 1,
                site.y + 1);

        switch (op_state) {
            case CREATE_BUILDING_STATE_INITIALIZING: {
                strcat(buffer, ": initializing");
            } break;

            case CREATE_BUILDING_STATE_WAITING_FOR_PLATFORM: {
                strcat(buffer, ": waiting for platform");
            } break;

            case CREATE_BUILDING_STATE_REMOVING_MINE: {
                strcat(buffer, ": removing mines");
            } break;

            case CREATE_BUILDING_STATE_GETTING_BUILDER: {
                strcat(buffer, ": get builder");
            } break;

            case CREATE_BUILDING_STATE_GETTING_MATERIALS: {
                strcat(buffer, ": get materials");
            } break;

            case CREATE_BUILDING_STATE_EVALUTING_SITE: {
                strcat(buffer, ": evaluating site");
            } break;

            case CREATE_BUILDING_STATE_SITE_BLOCKED: {
                strcat(buffer, ": site blocked");
            } break;

            case CREATE_BUILDING_STATE_MOVING_TO_SITE: {
                strcat(buffer, ": move to site");
            } break;

            case CREATE_BUILDING_STATE_CLEARING_SITE: {
                strcat(buffer, ": clear site");
            } break;

            case CREATE_BUILDING_STATE_BUILDING: {
                strcat(buffer, ": building");
            } break;

            case CREATE_BUILDING_STATE_MOVING_OFF_SITE: {
                strcat(buffer, ": move off site");
            } break;

            case CREATE_BUILDING_STATE_FINISHED: {
                strcat(buffer, ": finished.");
            } break;

            default: {
                strcat(buffer, ": UNKNOWN STATE!");
            } break;
        }

        if (builder && builder->GetBuildRate() > 1) {
            strcat(buffer, " at x2 rate");
        }

    } else {
        sprintf(buffer, "Create building aborted.");
    }

    return buffer;
}

Rect* TaskCreateBuilding::GetBounds(Rect* bounds) {
    int32_t unit_size;

    if (UnitsManager_BaseUnits[unit_type].flags & BUILDING) {
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
    return op_state < CREATE_BUILDING_STATE_BUILDING && (!parent || parent->IsNeeded());
}

void TaskCreateBuilding::AddUnit(UnitInfo& unit) {
    if (unit_type != INVALID_ID) {
        AiLog log("Build %s at [%i,%i]: Add %s", UnitsManager_BaseUnits[unit_type].singular_name, site.x + 1,
                  site.y + 1, UnitsManager_BaseUnits[unit.GetUnitType()].singular_name);

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
                if (manager.Get() != parent.Get()) {
                    parent->AddUnit(unit);
                }

            } else {
                TaskManager.RemindAvailable(&unit);
            }

            if (manager.Get() != parent.Get()) {
                parent->ChildComplete(this);
            }

            if (builder && builder->GetTask() == this && parent && unit_type == WTRPLTFM &&
                parent->GetType() == TaskType_TaskCreateBuilding) {
                TaskCreateBuilding* create_building = dynamic_cast<TaskCreateBuilding*>(parent.Get());

                if (create_building->op_state == CREATE_BUILDING_STATE_WAITING_FOR_PLATFORM &&
                    Builder_GetBuilderType(create_building->GetUnitType()) == builder->GetUnitType()) {
                    SmartPointer<UnitInfo> backup(builder);

                    backup->RemoveTasks();

                    dynamic_cast<TaskCreateBuilding*>(parent.Get())->AddUnit(*backup);
                }
            }

            parent = nullptr;
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
                    log.Log("Task already complete.");
                }

            } else {
                log.Log("Already have builder.");
            }
        }
    }
}

void TaskCreateBuilding::Begin() {
    AiLog log("Task Create Building: Begin");

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

    if (op_state < CREATE_BUILDING_STATE_BUILDING && parent && !parent->IsNeeded()) {
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
                AiLog log("Task Create %s at [%i,%i]: evaluating site", UnitsManager_BaseUnits[unit_type].singular_name,
                          site.x + 1, site.y + 1);

                FindBuildSite();
            } break;

            case CREATE_BUILDING_STATE_SITE_BLOCKED: {
                if (IsInitNeeded()) {
                    AiLog log("Task Create %s at [%i,%i]: site blocked",
                              UnitsManager_BaseUnits[unit_type].singular_name, site.x + 1, site.y + 1);

                    ChangeInitNeededFlag(false);

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
            if (Access_GetDistance(&*builder, site) > Access_GetDistance(&unit_, site)) {
                AiLog log("Task Create Building: Exchange unit.");

                TaskManager.RemindAvailable(&*builder);

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

    AiLog log("Task Create Building: Move Finished.");

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
                                if (UnitsManager_BaseUnits[unit_type].flags & BUILDING) {
                                    Rect bounds;

                                    op_state = CREATE_BUILDING_STATE_CLEARING_SITE;

                                    rect_init(&bounds, site.x, site.y, site.x + 2, site.y + 2);

                                    zone = new (std::nothrow) Zone(&*builder, this, &bounds);

                                    AiPlayer_Teams[team].ClearZone(&*zone);

                                    result = true;

                                } else {
                                    BeginBuilding();

                                    result = true;
                                }

                            } else {
                                log.Log("Site blocked.");

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
                            log.Log("Materials now available.");

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
    AiLog log("Create %s at [%i,%i]: remove %s.", UnitsManager_BaseUnits[unit_type].singular_name, site.x + 1,
              site.y + 1, UnitsManager_BaseUnits[unit.GetUnitType()].singular_name);

    if (builder == unit) {
        builder = nullptr;
        zone = nullptr;
    }
}

void TaskCreateBuilding::EventZoneCleared(Zone* zone_, bool status) {
    AiLog log("Task Create Building: zone cleared.");

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
                log.Log("Zone could not be cleared.");

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
    AiLog log("Task Create Building: Choose new site for %s.", UnitsManager_BaseUnits[unit_type].singular_name);

    if (builder->GetTask() == this) {
        Point new_site;

        op_state = CREATE_BUILDING_STATE_EVALUTING_SITE;

        if (manager && unit_type != CNCT_4W && unit_type != WTRPLTFM) {
            SDL_assert(manager->GetType() == TaskType_TaskManageBuildings);

            if (manager->ChangeSite(this, new_site)) {
                if (unit_type != INVALID_ID) {
                    site = new_site;

                    log.Log("New site for %s is [%i, %i].", UnitsManager_BaseUnits[unit_type].singular_name, site.x + 1,
                            site.y + 1);

                    SDL_assert(site.x > 0 && site.y > 0);

                    op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;

                    if (builder) {
                        if (!RequestWaterPlatform() && !RequestMineRemoval() && !RequestRubbleRemoval() &&
                            !IsScheduledForTurnEnd()) {
                            TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
                        }

                    } else {
                        log.Log("Requirements for site pre-empted builder.");
                    }

                } else {
                    log.Log("Site blocked and no alternative found.");
                }

            } else {
                log.Log("Site blocked and no alternative found.");

                Abort();
            }

        } else {
            log.Log("Site no longer valid for connector/platform, or no manager.");

            Abort();
        }

    } else {
        log.Log("Builder pre-empted by another task.");
    }
}

bool TaskCreateBuilding::RequestWaterPlatform() {
    bool result;

    if (!tasks.GetCount() && TaskCreateBuilding_DetermineMapSurfaceRequirements(unit_type, site) == 2) {
        Rect bounds;
        Point position;

        AiLog("Requesting water platform.");

        GetBounds(&bounds);

        op_state = CREATE_BUILDING_STATE_MOVING_TO_SITE;

        SDL_assert(site.x > 0 && site.y > 0);

        if (builder) {
            op_state = CREATE_BUILDING_STATE_WAITING_FOR_PLATFORM;
            builder->RemoveTasks();
        }

        if (UnitsManager_BaseUnits[unit_type].flags & BUILDING) {
            BuildBridges();
        }

        BuildBoardwalks();

        for (position.x = bounds.ulx; position.x < bounds.lrx; ++position.x) {
            for (position.y = bounds.uly; position.y < bounds.lry; ++position.y) {
                if (TaskCreateBuilding_DetermineMapSurfaceRequirements(WTRPLTFM, position) == 1) {
                    field_42 = true;

                    SmartPointer<TaskCreateBuilding> create_building_task(
                        new (std::nothrow) TaskCreateBuilding(this, GetFlags() - 0x80, WTRPLTFM, position, nullptr));

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
        if (Cargo_GetRawConsumptionRate(unit_type, 1) > 0 && Task_GetReadyUnitsCount(team, unit_type) > 0 &&
            TaskManager_NeedToReserveRawMaterials(team)) {
            op_state = CREATE_BUILDING_STATE_GETTING_MATERIALS;

            result = false;

        } else if (Cargo_GetPowerConsumptionRate(unit_type) > 0 && Task_GetReadyUnitsCount(team, unit_type) > 0 &&
                   (TaskCreateBuilding_IsMorePowerNeeded(team) ||
                    (unit_type != MININGST && TaskCreateBuilding_IsMoreFuelReservesNeeded(team)))) {
            op_state = CREATE_BUILDING_STATE_GETTING_MATERIALS;

            result = false;

        } else if (Cargo_GetLifeConsumptionRate(unit_type) > 0 && Task_GetReadyUnitsCount(team, unit_type) > 0 &&
                   TaskCreateBuilding_IsMoreLifeNeeded(team)) {
            op_state = CREATE_BUILDING_STATE_GETTING_MATERIALS;

            result = false;

        } else {
            int32_t resource_demand =
                BuildMenu_GetTurnsToBuild(unit_type, team) * Cargo_GetRawConsumptionRate(builder->GetUnitType(), 1);

            if (TaskCreateBuilding_DetermineMapSurfaceRequirements(unit_type, site) == 2 &&
                Builder_GetBuilderType(WTRPLTFM) == builder->GetUnitType()) {
                resource_demand +=
                    BuildMenu_GetTurnsToBuild(WTRPLTFM, team) * Cargo_GetRawConsumptionRate(builder->GetUnitType(), 1);
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
        unit_type != DOCK && (UnitsManager_BaseUnits[unit_type].flags & BUILDING)) {
        Point position;
        AccessMap map;
        Rect bounds;
        int32_t range_limit;

        AiLog log("Create boardwalks around unit.");

        MarkBridgeAreas(map.GetMap());

        PopulateMap(map.GetMap());

        GetBounds(&bounds);

        position.x = bounds.ulx - 1;
        position.y = bounds.lry;

        range_limit = bounds.lrx - bounds.ulx;

        for (int32_t direction = 0; direction < 8; direction += 2) {
            for (int32_t range = 0; range < range_limit; ++range) {
                position += Paths_8DirPointsArray[direction];

                if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                    position.y < ResourceManager_MapSize.y) {
                    if (map.GetMapColumn(position.x)[position.y] == 1) {
                        log.Log("Create boardwalk at [%i,%i].", position.x + 1, position.y + 1);

                        SmartPointer<TaskCreateBuilding> create_building_task =
                            new (std::nothrow) TaskCreateBuilding(this, 0x1C80, BRIDGE, position, &*manager);

                        if (manager) {
                            manager->AddCreateOrder(&*create_building_task);

                        } else {
                            log.Log("No building manager!");

                            TaskManager.AppendTask(*create_building_task);
                        }
                    }
                }
            }

            position += Paths_8DirPointsArray[direction];
        }
    }
}

void TaskCreateBuilding::BuildBridges() {
    AccessMap map;
    bool spot_found;
    Point position;

    AiLog log("Building bridges.");

    MarkBridgeAreas(map.GetMap());

    spot_found = false;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == MININGST) {
            position.x = (*it).grid_x - 1;
            position.y = (*it).grid_y + 2;

            for (int32_t direction = 0; direction < 8 && !spot_found; direction += 2) {
                for (int32_t range = 0; range < 3 && !spot_found; ++range) {
                    position += Paths_8DirPointsArray[direction];

                    if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                        position.y < ResourceManager_MapSize.y) {
                        if (map.GetMapColumn(position.x)[position.y] == 2) {
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
        PopulateMap(map.GetMap());

        SmartPointer<Continent> continent(new (std::nothrow) Continent(map.GetMap(), 3, position, 0));

        if (!FindBridgePath(map.GetMap(), 3)) {
            FindBridgePath(map.GetMap(), 4);
        }

    } else {
        log.Log("No starting point found.");
    }
}

void TaskCreateBuilding::MarkBridgeAreas(uint8_t** map) {
    int16_t** damage_potential_map;

    AiLog log("Marking bridge areas.");

    for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            switch (ResourceManager_MapSurfaceMap[y * ResourceManager_MapSize.x + x]) {
                case SURFACE_TYPE_LAND: {
                    map[x][y] = 2;
                } break;

                case SURFACE_TYPE_WATER:
                case SURFACE_TYPE_COAST: {
                    map[x][y] = 1;
                } break;

                default: {
                    map[x][y] = 0;
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
                    map[x][y] = 4;
                }
            }
        }
    }

    damage_potential_map = AiPlayer_Teams[team].GetDamagePotentialMap(ENGINEER, CAUTION_LEVEL_AVOID_ALL_DAMAGE, true);

    for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
        for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
            if (damage_potential_map[x][y] > 0) {
                map[x][y] = 0;
            }
        }
    }

    for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
         ++it) {
        if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskCreateBuilding) {
            Rect bounds;
            TaskCreateBuilding* create_building_task = dynamic_cast<TaskCreateBuilding*>(&*it);

            create_building_task->GetBounds(&bounds);

            if (create_building_task->GetUnitType() != BRIDGE && create_building_task->GetUnitType() != WTRPLTFM &&
                create_building_task->GetUnitType() != CNCT_4W) {
                for (int32_t x = bounds.ulx; x < bounds.lrx; ++x) {
                    for (int32_t y = bounds.uly; y < bounds.lry; ++y) {
                        map[x][y] = 4;
                    }
                }
            }
        }
    }
}

void TaskCreateBuilding::PopulateMap(uint8_t** map) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
         it != UnitsManager_GroundCoverUnits.End(); ++it) {
        if ((*it).GetUnitType() == BRIDGE && (*it).IsVisibleToTeam(team)) {
            map[(*it).grid_x][(*it).grid_y] = 2;
        }
    }

    for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
         ++it) {
        if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskCreateBuilding) {
            Rect bounds;
            TaskCreateBuilding* create_building_task = dynamic_cast<TaskCreateBuilding*>(&*it);

            create_building_task->GetBounds(&bounds);

            if (create_building_task->GetUnitType() == BRIDGE) {
                if (map[bounds.ulx][bounds.uly] == 1) {
                    map[bounds.ulx][bounds.uly] = 2;
                }
            }
        }
    }

    for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            if ((ResourceManager_CargoMap[y * ResourceManager_MapSize.x + x] & 0x1F) >= 8) {
                if (map[x][y] == 1) {
                    map[x][y] = 0;
                }
            }
        }
    }
}

bool TaskCreateBuilding::FindBridgePath(uint8_t** map, int32_t value) {
    Point position;
    Point start_position;
    bool is_found = false;
    uint16_t flags1;
    uint16_t bridge_count;
    Rect bounds;
    int32_t range_limit;
    int32_t direction2;

    AiLog log("Find bridge path.");

    GetBounds(&bounds);

    range_limit = bounds.lrx - bounds.ulx + 1;

    position.x = bounds.ulx - 1;
    position.y = bounds.lry;

    bridge_count = SHRT_MAX;

    for (int32_t direction = 0; direction < 8; direction += 2) {
        for (int32_t range = 0; range < range_limit; ++range) {
            position += Paths_8DirPointsArray[direction];

            if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                position.y < ResourceManager_MapSize.y) {
                if (map[position.x][position.y] == value) {
                    log.Log("Unit is already connected.");

                    return true;

                } else if (SearchPathStep(map, position, &direction2, &bridge_count, value)) {
                    start_position = position;

                    is_found = true;

                    log.Log("Found: start point [%i,%i], %i bridges.", position.x + 1, position.y + 1, bridge_count);
                }
            }
        }
    }

    flags1 = GetFlags() - 200;

    if (unit_type != LIGHTPLT && unit_type != LANDPLT && unit_type != DEPOT && unit_type != TRAINHAL) {
        flags1 = 0x1C00;
    }

    if (is_found) {
        SmartPointer<TaskCreateBuilding> create_building_task;

        position = start_position;

        while (bridge_count > 0) {
            if (map[position.x][position.y] == 1) {
                log.Log("Create bridge at [%i,%i].", position.x + 1, position.y + 1);

                create_building_task =
                    new (std::nothrow) TaskCreateBuilding(this, flags1 + bridge_count, BRIDGE, position, &*manager);

                tasks.PushBack(*create_building_task);

                if (manager) {
                    manager->AddCreateOrder(&*create_building_task);

                } else {
                    log.Log("No building manager!");

                    TaskManager.AppendTask(*create_building_task);
                }
            }

            position += Paths_8DirPointsArray[direction2];

            SearchPathStep(map, position, &direction2, &bridge_count, value);
        }

    } else {
        log.Log("No connection found.");
    }

    return is_found;
}

bool TaskCreateBuilding::SearchPathStep(uint8_t** map, Point position, int32_t* direction, uint16_t* best_unit_count,
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

        site2 += Paths_8DirPointsArray[(local_direction + 2) & 7];
        site3 += Paths_8DirPointsArray[(local_direction + 6) & 7];

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
            if (map[site1.x][site1.y] == value || map[site2.x][site2.y] == value || map[site3.x][site3.y] == value) {
                is_found = true;
                break;
            }

            if (map[site1.x][site1.y] == 1) {
                ++unit_count;
            }

            if (map[site1.x][site1.y] == 0) {
                break;
            }

            site1 += Paths_8DirPointsArray[local_direction];
            site2 += Paths_8DirPointsArray[local_direction];
            site3 += Paths_8DirPointsArray[local_direction];
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
    BaseUnit* base_unit;
    bool water_present;
    bool land_present;
    bool coast_present;
    int32_t result;

    rect_init(&bounds, site.x, site.y, site.x + 1, site.y + 1);

    base_unit = &UnitsManager_BaseUnits[unit_type];

    water_present = false;
    land_present = false;
    coast_present = false;

    if (base_unit->flags & BUILDING) {
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

        if (!(base_unit->land_type & SURFACE_TYPE_LAND) && land_present) {
            result = 0;

        } else if (!(base_unit->land_type & SURFACE_TYPE_WATER) && water_present) {
            result = 2;

        } else if (!(base_unit->land_type & SURFACE_TYPE_COAST) && coast_present) {
            result = 2;

        } else {
            result = 1;
        }

    } else {
        result = 0;
    }

    return result;
}
