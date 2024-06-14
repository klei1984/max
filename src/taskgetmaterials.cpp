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

#include "taskgetmaterials.hpp"

#include "ailog.hpp"
#include "game_manager.hpp"
#include "task_manager.hpp"
#include "units_manager.hpp"

TaskGetMaterials::TaskGetMaterials(Task* task, UnitInfo* unit, uint16_t materials_needed_)
    : TaskGetResource(task, *unit), materials_needed(materials_needed_) {}

TaskGetMaterials::~TaskGetMaterials() {}

uint16_t TaskGetMaterials::GetFlags() const {
    uint16_t result;

    if (parent) {
        result = parent->GetFlags();

    } else {
        result = flags;
    }

    return result;
}

char* TaskGetMaterials::WriteStatusLog(char* buffer) const {
    sprintf(buffer, "Get %i Materials", materials_needed);

    if (requestor) {
        if (requestor->storage < materials_needed) {
            char text[50];

            sprintf(text, ": %i on hand, ", requestor->storage);
            strcat(buffer, text);

            if (source) {
                int32_t remaining_demand = materials_needed - requestor->storage;

                if (remaining_demand > source->storage) {
                    remaining_demand = source->storage;
                }

                sprintf(text, "get %i from ", remaining_demand);
                strcat(buffer, text);
                strcat(buffer, UnitsManager_BaseUnits[source->GetUnitType()].singular_name);

            } else {
                strcat(buffer, "waiting for source.");
            }

        } else {
            strcat(buffer, ": already on hand.");
        }

    } else {
        strcat(buffer, ": finished.");
    }

    return buffer;
}

uint8_t TaskGetMaterials::GetType() const { return TaskType_TaskGetMaterials; }

void TaskGetMaterials::EndTurn() {
    if (requestor) {
        if (materials_needed > requestor->storage) {
            if (source && requestor->GetTask() == this && source->storage == 0) {
                ReleaseSource();
            }

            TaskGetResource::EndTurn();

        } else {
            EventCargoTransfer(*requestor);
        }

    } else {
        ReleaseSource();

        parent = nullptr;

        TaskManager.RemoveTask(*this);
    }
}

void TaskGetMaterials::EventCargoTransfer(UnitInfo& unit) {
    AiLog log("Task Get Materials: Transfer Finished.");

    if (requestor == unit) {
        SmartPointer<Task> get_materials_task = this;

        if (requestor->storage >= materials_needed) {
            requestor->RemoveTask(this);
            parent = nullptr;
            requestor = nullptr;
        }

        ReleaseSource();

        if (!requestor) {
            parent = nullptr;

            TaskManager.RemoveTask(*this);
        }
    }
}

void TaskGetMaterials::DoTransfer() {
    AiLog log("Task Get Materials: DoTransfer.");

    if (source->orders != ORDER_TRANSFER) {
        if (source->flags & STATIONARY) {
            Cargo materials;
            Cargo capacity;

            source->GetComplex()->GetCargoInfo(materials, capacity);

            if (materials.raw >= requestor->GetBaseValues()->GetAttribute(ATTRIB_STORAGE) * 2 ||
                materials.raw == capacity.raw) {
                source->target_grid_x = requestor->GetBaseValues()->GetAttribute(ATTRIB_STORAGE) - requestor->storage;

            } else {
                source->target_grid_x = materials_needed - requestor->storage;
            }

            if (source->target_grid_x > materials.raw) {
                source->target_grid_x = materials.raw;
            }

        } else {
            source->target_grid_x = materials_needed - requestor->storage;

            if (source->target_grid_x > source->storage) {
                source->target_grid_x = source->storage;
            }
        }

        source->SetParent(requestor.Get());

        SDL_assert(GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team);

        log.Log("Order %i materials from %s at [%i,%i] for %s at [%i,%i] holding %i materials.", source->target_grid_x,
                UnitsManager_BaseUnits[source->GetUnitType()].singular_name, source->grid_x + 1, source->grid_y + 1,
                UnitsManager_BaseUnits[requestor->GetUnitType()].singular_name, requestor->grid_x + 1,
                requestor->grid_y + 1, requestor->storage);

        UnitsManager_SetNewOrder(source.Get(), ORDER_TRANSFER, ORDER_STATE_INIT);

    } else {
        log.Log("Transfer delayed - unit's orders are TRANSFER.");
    }
}

UnitInfo* TaskGetMaterials::FindBuilding() {
    UnitInfo* building{nullptr};
    int32_t minimum_distance{INT32_MAX};
    AiLog log("Task Get Materials: Find Building.");

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).storage > 0 && (*it).hits > 0 &&
            UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type == CARGO_TYPE_RAW) {
            UnitInfo* candidate_building = FindClosestBuilding((*it).GetComplex());

            const int32_t distance = TaskManager_GetDistance(candidate_building, it->Get());

            if (!building || distance < minimum_distance) {
                minimum_distance = distance;
                building = candidate_building;
                source = *it;
            }
        }
    }

    return building;
}

void TaskGetMaterials::FindTruck() {
    UnitInfo* truck{nullptr};
    int32_t minimum_distance{INT32_MAX};
    AiLog log("Task Get Materials: Find Truck.");

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type == CARGO_TYPE_RAW &&
            (*it).storage > 0 && (*it).hits > 0 &&
            ((*it).orders == ORDER_AWAIT || ((*it).orders == ORDER_MOVE && (*it).speed == 0)) && requestor != *it) {
            bool candidate_found;

            if ((*it).GetTask()) {
                if ((*it).GetTask()->DeterminePriority(flags) <= 0) {
                    if ((*it).GetTask()->DeterminePriority(flags) == 0 &&
                        (*it).GetTask()->GetType() == TaskType_TaskGetMaterials &&
                        (*it).GetTask()->GetId() > this->id) {
                        candidate_found = true;

                    } else {
                        candidate_found = false;
                    }

                } else {
                    candidate_found = true;
                }

            } else {
                candidate_found = true;
            }

            if (candidate_found) {
                const int32_t distance = TaskManager_GetDistance(it->Get(), requestor.Get());

                if (!truck || distance < minimum_distance) {
                    minimum_distance = distance;
                    truck = it->Get();
                }
            }
        }
    }

    supplier = truck;
}
