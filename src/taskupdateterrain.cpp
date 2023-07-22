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

#include "taskupdateterrain.hpp"

#include "access.hpp"
#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "reminders.hpp"
#include "resource_manager.hpp"
#include "task_manager.hpp"
#include "units_manager.hpp"

TaskUpdateTerrain::TaskUpdateTerrain(unsigned short team) : Task(team, nullptr, 0) {}

TaskUpdateTerrain::~TaskUpdateTerrain() {}

char* TaskUpdateTerrain::WriteStatusLog(char* buffer) const {
    if (location.x < ResourceManager_MapSize.x) {
        sprintf(buffer, "Update Terrain [%i,%i]", location.x + 1, location.y + 1);

    } else {
        strcpy(buffer, "Update Terrain (finished)");
    }

    return buffer;
}

unsigned char TaskUpdateTerrain::GetType() const { return TaskType_TaskUpdateTerrain; }

void TaskUpdateTerrain::BeginTurn() {
    while (location.x < ResourceManager_MapSize.x) {
        if (Paths_HaveTimeToThink()) {
            AiPlayer_TerrainMap.TerrainMap_sub_690D6(location, SURFACE_TYPE_LAND);
            AiPlayer_TerrainMap.TerrainMap_sub_690D6(location, SURFACE_TYPE_WATER);

            ++location.y;

            if (location.y >= ResourceManager_MapSize.y) {
                location.y = 0;
                ++location.x;
            }

        } else {
            if (!UnitsManager_TeamInfo[GameManager_PlayerTeam].finished_turn && !IsScheduledForTurnStart()) {
                TaskManager.AppendReminder(new (std::nothrow) class RemindTurnStart(*this));
            }

            return;
        }
    }

    TaskManager.RemoveTask(*this);
}

void TaskUpdateTerrain::RemoveSelf() {
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}
