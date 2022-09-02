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

#include "taskclearzone.hpp"

#include "aiplayer.hpp"
#include "task_manager.hpp"
#include "units_manager.hpp"

void TaskClearZone::PathFindResultCallback(Task* task, PathRequest* request, Point point, UnitPath* path) {}

void TaskClearZone::PathFindCancelCallback(Task* task, PathRequest* request) {}

TaskClearZone::TaskClearZone(unsigned short team, unsigned int flags_)
    : Task(team, nullptr, 0), unit_flags(flags_), state(CLEARZONE_STATE_WAITING) {}

TaskClearZone::~TaskClearZone() {}

int TaskClearZone::GetMemoryUse() const {
    return 4 + zones.GetCount() * (sizeof(Zone) + 4) + zone_squares.GetCount() * sizeof(ZoneSquare) +
           (points1.GetCount() + points2.GetCount()) * sizeof(Point);
}

char* TaskClearZone::WriteStatusLog(char* buffer) const {
    if (unit_flags == MOBILE_AIR_UNIT) {
        strcpy(buffer, "Clear air zones: ");

    } else {
        strcpy(buffer, "Clear land / sea zones: ");
    }

    switch (state) {
        case CLEARZONE_STATE_WAITING: {
            strcat(buffer, "waiting");
        } break;

        case CLEARZONE_STATE_EXAMINING_ZONES: {
            strcat(buffer, "examining zones");
        } break;

        case CLEARZONE_STATE_SEARCHING_MAP: {
            strcat(buffer, "searching map");
        } break;

        case CLEARZONE_STATE_WAITING_FOR_PATH: {
            strcat(buffer, "waiting for path");
        } break;

        case CLEARZONE_STATE_MOVING_UNIT: {
            strcat(buffer, "moving ");
            strcat(buffer, UnitsManager_BaseUnits[moving_unit->unit_type].singular_name);
        } break;
    }

    return buffer;
}

unsigned char TaskClearZone::GetType() const { return TaskType_TaskClearZone; }

bool TaskClearZone::Task_vfunc10() {}

void TaskClearZone::Begin() { RemindTurnEnd(); }

void TaskClearZone::BeginTurn() { EndTurn(); }

void TaskClearZone::EndTurn() {}

bool TaskClearZone::Task_vfunc17(UnitInfo& unit) {
    if (moving_unit == unit) {
        EndTurn();
    }

    return true;
}

void TaskClearZone::RemoveSelf() {
    zones.Release();

    if (moving_unit != nullptr) {
        moving_unit->RemoveTask(this);
        moving_unit = nullptr;
    }

    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskClearZone::RemoveUnit(UnitInfo& unit) {
    if (moving_unit == unit) {
        moving_unit = nullptr;
        state = CLEARZONE_STATE_WAITING;
    }
}

void TaskClearZone::AddZone(Zone* zone) {
    unsigned char** map;

    map = AiPlayer_Teams[team].GetInfoMap();

    zones.Insert(*zone);

    if (map) {
        for (int i = 0; i < zone->points.GetCount(); ++i) {
            map[zone->points[i]->x][zone->points[i]->y] |= 0x08;
        }
    }

    RemindTurnEnd();
}
