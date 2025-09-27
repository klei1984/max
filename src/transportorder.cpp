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

#include "transportorder.hpp"

#include "access.hpp"
#include "task_manager.hpp"
#include "units_manager.hpp"

TransportOrder::TransportOrder(UnitInfo* unit, ResourceID unit_type_, GroundPath* path_) {
    transport_category = DetermineCategory(unit->GetUnitType());
    position.x = unit->grid_x;
    position.y = unit->grid_y;
    path = path_;
    unit_type = unit_type_;
}

TransportOrder::~TransportOrder() {}

ResourceID TransportOrder::GetUnitType() const { return unit_type; }

uint8_t TransportOrder::GetTransportCategory() const { return transport_category; }

int32_t TransportOrder::DetermineCategory(ResourceID unit_type_) {
    int32_t result;

    if (UnitsManager_BaseUnits[unit_type_].flags & MOBILE_AIR_UNIT) {
        result = 0;

    } else if (unit_type_ == SURVEYOR) {
        result = 4;

    } else if (unit_type_ == SUBMARNE) {
        result = 5;

    } else if (unit_type_ == COMMANDO || unit_type_ == INFANTRY) {
        result = 6;

    } else if (!(UnitsManager_BaseUnits[unit_type_].land_type & SURFACE_TYPE_WATER)) {
        result = 1;

    } else if (UnitsManager_BaseUnits[unit_type_].flags & MOBILE_LAND_UNIT) {
        result = 3;

    } else {
        result = 2;
    }

    return result;
}

Point TransportOrder::MatchClosestPosition(Point end_position, Point start_position) {
    SmartObjectArray<PathStep> steps = path->GetSteps();
    Point path_position = position;
    Point closest_position;
    uint32_t index;
    int32_t distance;
    int32_t minimum_distance;

    for (index = 0; path_position != start_position; ++index) {
        path_position.x += steps[index]->x;
        path_position.y += steps[index]->y;
    }

    closest_position = path_position;
    minimum_distance = TaskManager_GetDistance(closest_position, end_position);

    for (; index < steps->GetCount(); ++index) {
        path_position.x += steps[index]->x;
        path_position.y += steps[index]->y;

        distance = TaskManager_GetDistance(path_position, end_position);

        if (distance < minimum_distance) {
            minimum_distance = distance;
            closest_position = path_position;

            if (distance == 0) {
                break;
            }
        }
    }

    return closest_position;
}

Point TransportOrder::MatchStartPosition(Point target) { return MatchClosestPosition(target, position); }

void TransportOrder::UsePrecalculatedPath(TaskPathRequest* request) {
    request->CreateTransport(unit_type);
    request->Complete(position, &*path);
}
