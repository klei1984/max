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

#include "taskfindmines.hpp"

#include "aiplayer.hpp"
#include "resource_manager.hpp"
#include "task_manager.hpp"

TaskFindMines::TaskFindMines(unsigned short team_, Point point_) : TaskAbstractSearch(team_, nullptr, 0x2000, point_) {}

TaskFindMines::~TaskFindMines() {}

int TaskFindMines::GetMemoryUse() const { return units.GetMemorySize() - 6; }

char* TaskFindMines::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Look for enemy land/sea mines");

    return buffer;
}

unsigned char TaskFindMines::GetType() const { return TaskType_TaskFindMines; }

void TaskFindMines::BeginTurn() {
    SmartList<UnitInfo>::Iterator unit = units.Begin();

    if (unit && requestors == 0) {
        unsigned short** damage_potential_map =
            AiPlayer_Teams[team].GetDamagePotentialMap(&*unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE, 1);
        unsigned char** mine_map = AiPlayer_Teams[team].GetMineMap();
        int valuable_sites = 0;

        if (damage_potential_map && mine_map) {
            for (int index_x = 0; index_x < ResourceManager_MapSize.x; ++index_x) {
                for (int index_y = 0; index_y < ResourceManager_MapSize.y; ++index_y) {
                    if (mine_map[index_x][index_y] > 0 && damage_potential_map[index_x][index_y] == 0) {
                        ++valuable_sites;
                    }
                }
            }

            valuable_sites = sqrt(valuable_sites) * 0.25;

            if (units.GetCount() < valuable_sites) {
                ++requestors;

                ObtainUnit();
            }
        }
    }

    TaskAbstractSearch::BeginTurn();
}

bool TaskFindMines::Task_vfunc17(UnitInfo& unit) {
    bool result;

    if (unit.IsReadyForOrders(this) && unit.speed) {
        FindDestination(unit, 1);

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool TaskFindMines::IsVisited(UnitInfo& unit, Point site) { return AiPlayer_Teams[team].GetMineMapEntry(site) == 0; }

void TaskFindMines::ObtainUnit() {
    SmartPointer<TaskObtainUnits> obtain_units_task = new (std::nothrow) TaskObtainUnits(this, point);

    obtain_units_task->AddUnit(SURVEYOR);

    TaskManager.AppendTask(*obtain_units_task);
}
