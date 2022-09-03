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

#include "game_manager.hpp"
#include "task_manager.hpp"
#include "units_manager.hpp"

TaskGetMaterials::TaskGetMaterials(Task* task, UnitInfo* unit, unsigned short materials_needed_)
    : TaskGetResource(task, *unit), materials_needed(materials_needed_) {}

TaskGetMaterials::~TaskGetMaterials() {}

int TaskGetMaterials::GetMemoryUse() const { return 4; }

unsigned short TaskGetMaterials::GetFlags() const {
    unsigned short result;

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
                int remaining_demand = materials_needed - requestor->storage;

                if (remaining_demand > source->storage) {
                    remaining_demand = source->storage;
                }

                sprintf(text, "get %i from ", remaining_demand);
                strcat(buffer, text);
                strcat(buffer, UnitsManager_BaseUnits[source->unit_type].singular_name);

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

unsigned char TaskGetMaterials::GetType() const { return TaskType_TaskGetMaterials; }

void TaskGetMaterials::EndTurn() {
    if (requestor) {
        if (materials_needed > requestor->storage) {
            if (source && requestor->GetTask1ListFront() == this && source->storage == 0) {
                ReleaseSource();
            }

            TaskGetResource::EndTurn();

        } else {
            Task_vfunc22(*requestor);
        }

    } else {
        ReleaseSource();

        parent = nullptr;

        TaskManager.RemoveTask(*this);
    }
}

void TaskGetMaterials::Task_vfunc22(UnitInfo& unit) {
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

        source->SetParent(&*requestor);

        SDL_assert(GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team);

        UnitsManager_SetNewOrder(&*source, ORDER_TRANSFER, ORDER_STATE_0);
    }
}

UnitInfo* TaskGetMaterials::FindBuilding() {
    UnitInfo* building = nullptr;
    int distance;
    int minimum_distance;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).storage > 0 && (*it).hits > 0 &&
            UnitsManager_BaseUnits[(*it).unit_type].cargo_type == CARGO_TYPE_RAW) {
            UnitInfo* candidate_building = FindClosestBuilding((*it).GetComplex());

            distance = TaskManager_GetDistance(candidate_building, &*it);

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
    UnitInfo* truck = nullptr;
    int distance;
    int minimum_distance;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && UnitsManager_BaseUnits[(*it).unit_type].cargo_type == CARGO_TYPE_RAW &&
            (*it).storage > 0 && (*it).hits > 0 &&
            ((*it).orders == ORDER_AWAIT || ((*it).orders == ORDER_MOVE && !(*it).speed)) && requestor != *it) {
            bool candidate_found;

            if ((*it).GetTask1ListFront()) {
                if ((*it).GetTask1ListFront()->DeterminePriority(flags) <= 0) {
                    if ((*it).GetTask1ListFront()->DeterminePriority(flags) == 0 &&
                        (*it).GetTask1ListFront()->GetType() == TaskType_TaskGetMaterials &&
                        (*it).GetTask1ListFront()->GetId() > this->id) {
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
                distance = TaskManager_GetDistance(&*it, &*requestor);

                if (!truck || distance < minimum_distance) {
                    minimum_distance = distance;
                    truck = &*it;
                }
            }
        }
    }

    supplier = truck;
}
