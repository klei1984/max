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

#include "taskreload.hpp"

#include "access.hpp"
#include "ailog.hpp"
#include "task_manager.hpp"
#include "units_manager.hpp"

TaskReload::TaskReload(UnitInfo* unit) : TaskRepair(unit) {}

TaskReload::~TaskReload() {}

char* TaskReload::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Reload unit with ammunition.");

    return buffer;
}

uint8_t TaskReload::GetType() const { return TaskType_TaskReload; }

void TaskReload::SelectOperator() {
    if (!(target_unit->flags & MOBILE_AIR_UNIT)) {
        ResourceID unit_type = INVALID_ID;
        ResourceID repair_shop_type;
        UnitInfo* unit = nullptr;
        int32_t distance;
        int32_t minimum_distance{INT32_MAX};

        repair_shop_type = GetRepairShopType();

        if (repair_shop_type == DOCK) {
            unit_type = CARGOSHP;

        } else if (repair_shop_type == DEPOT) {
            unit_type = SPLYTRCK;

        } else {
            Point site;
            Rect bounds;
            int32_t range;
            int32_t surface_type;

            site.x = target_unit->grid_x - 1;
            site.y = target_unit->grid_y + 1;

            rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

            if (target_unit->flags & BUILDING) {
                range = 3;

            } else {
                range = 2;
            }

            for (int32_t direction = 0; direction < 8; direction += 2) {
                for (int32_t i = 0; i < range; ++i) {
                    site += Paths_8DirPointsArray[direction];

                    if (Access_IsInsideBounds(&bounds, &site)) {
                        surface_type = Access_GetModifiedSurfaceType(site.x, site.y);

                        if (surface_type == SURFACE_TYPE_LAND) {
                            unit_type = SPLYTRCK;

                        } else if (surface_type == SURFACE_TYPE_WATER && unit_type == INVALID_ID) {
                            unit_type = CARGOSHP;
                        }
                    }
                }
            }

            if (unit_type == INVALID_ID) {
                return;
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
             it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
            if ((*it).team == team && (*it).GetUnitType() == unit_type && (*it).hits > 0 &&
                ((*it).GetOrder() == ORDER_AWAIT || ((*it).GetOrder() == ORDER_MOVE && (*it).speed == 0)) &&
                target_unit != (*it)) {
                if ((*it).GetTask() == nullptr || (*it).GetTask()->DeterminePriority(flags) > 0) {
                    distance = TaskManager_GetDistance(&*it, &*target_unit);

                    if (unit == nullptr || distance < minimum_distance) {
                        unit = &*it;
                        minimum_distance = distance;
                    }
                }
            }
        }

        operator_unit = unit;
    }
}

int32_t TaskReload::GetTurnsToComplete() { return 1; }

bool TaskReload::IsInPerfectCondition() {
    return target_unit->ammo == target_unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO);
}

void TaskReload::CreateUnit() {
    ResourceID unit_type;

    unit_type = GetRepairShopType();

    if (unit_type != INVALID_ID) {
        CreateUnitIfNeeded(unit_type);
    }

    if (unit_type == DOCK) {
        CreateUnitIfNeeded(CARGOSHP);
    }

    if (unit_type == DEPOT || unit_type == BARRACKS || unit_type == INVALID_ID) {
        CreateUnitIfNeeded(SPLYTRCK);
    }
}

void TaskReload::IssueOrder() {
    AILOG(log, "Reloading {} from {}.", UnitsManager_BaseUnits[target_unit->GetUnitType()].GetSingularName(),
          UnitsManager_BaseUnits[operator_unit->GetUnitType()].GetSingularName());

    operator_unit->SetParent(&*target_unit);
    UnitsManager_SetNewOrder(&*operator_unit, ORDER_RELOAD, ORDER_STATE_INIT);
}
