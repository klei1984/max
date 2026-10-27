/* Copyright (c) 2026 M.A.X. Port Team
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

#include "attack_census.hpp"

#if !defined(NDEBUG)

#include <format>
#include <map>
#include <string>
#include <string_view>

#include "ailog.hpp"
#include "task_manager.hpp"
#include "taskattack.hpp"
#include "units_manager.hpp"

struct AttackCensus_Counters {
    uint32_t assaults;
    uint32_t kill_task_ends;
};

struct AttackCensus_Share {
    int32_t units{0};
    int32_t value{0};
    std::map<std::string_view, int32_t> tasks;
};

static AttackCensus_Counters AttackCensus_TeamCounters[PLAYER_TEAM_MAX];

/* Adds the armed mobile units of a team to the shares of the tasks at the roots of their task chains.
 */
static void AttackCensus_AddUnits(SmartList<UnitInfo>& units, uint16_t team,
                                  std::map<std::string_view, AttackCensus_Share>& shares, AttackCensus_Share& total) {
    for (auto it = units.Begin(), it_end = units.End(); it != it_end; ++it) {
        if ((*it).team == team && (*it).hits > 0 && (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0) {
            Task* task = (*it).GetTask();
            Task* root = task;

            while (root && root->GetParent()) {
                root = root->GetParent();
            }

            const std::string_view root_name = root ? Task_GetName(root) : std::string_view("None");
            const std::string_view task_name = task ? Task_GetName(task) : std::string_view("None");
            const int32_t value = (*it).GetNormalRateBuildCost();
            AttackCensus_Share& share = shares[root_name];

            ++share.units;
            share.value += value;
            ++share.tasks[task_name];

            ++total.units;
            total.value += value;
        }
    }
}

void AttackCensus_Log(uint16_t team) {
    if (!AiLog_IsEnabled()) {
        return;
    }

    std::map<std::string_view, AttackCensus_Share> shares;
    AttackCensus_Share total;
    int32_t attack_count = 0;

    AILOG(log, "Attack census of team {}.", team);

    AttackCensus_AddUnits(UnitsManager_MobileLandSeaUnits, team, shares, total);
    AttackCensus_AddUnits(UnitsManager_MobileAirUnits, team, shares, total);

    AILOG_LOG(log, "Military units={} value={}", total.units, total.value);

    for (const auto& [root_name, share] : shares) {
        std::string tasks;

        for (const auto& [task_name, count] : share.tasks) {
            tasks += std::format("{}{}={}", tasks.empty() ? "" : " ", task_name, count);
        }

        AILOG_LOG(log, "Owner {} units={} value={} tasks {}", root_name, share.units, share.value, tasks);
    }

    for (auto it = TaskManager.GetTaskList().Begin(), it_end = TaskManager.GetTaskList().End(); it != it_end; ++it) {
        if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskAttack) {
            ++attack_count;
        }
    }

    AILOG_LOG(log, "Attacks {}", attack_count);

    for (auto it = TaskManager.GetTaskList().Begin(), it_end = TaskManager.GetTaskList().End(); it != it_end; ++it) {
        if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskAttack) {
            AILOG_LOG(log, "Attack {}", dynamic_cast<TaskAttack*>(it->Get())->GetCensus());
        }
    }

    AILOG_LOG(log, "Events assaults={} kill_tasks_ended={}", AttackCensus_TeamCounters[team].assaults,
              AttackCensus_TeamCounters[team].kill_task_ends);

    AttackCensus_TeamCounters[team] = {};
}

void AttackCensus_CountAssault(uint16_t team) { ++AttackCensus_TeamCounters[team].assaults; }

void AttackCensus_CountKillTaskEnd(uint16_t team) { ++AttackCensus_TeamCounters[team].kill_task_ends; }

#endif /* !defined(NDEBUG) */
