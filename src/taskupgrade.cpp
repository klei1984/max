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

#include "taskupgrade.hpp"

#include "aiplayer.hpp"
#include "units_manager.hpp"

TaskUpgrade::TaskUpgrade(UnitInfo* unit) : TaskRepair(unit) {}

TaskUpgrade::~TaskUpgrade() {}

int TaskUpgrade::GetMemoryUse() const { return 4; }

char* TaskUpgrade::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Upgrade unit");

    return buffer;
}

unsigned char TaskUpgrade::GetType() const { return TaskType_TaskUpgrade; }

void TaskUpgrade::SelectOperator() {}

int TaskUpgrade::GetTurnsToComplete() {
    int result;

    if (AiPlayer_Teams[team].IsUpgradeNeeded(&*target_unit)) {
        result = target_unit->GetNormalRateBuildCost() / 4;

    } else {
        result = 1;
    }

    return result;
}

bool TaskUpgrade::IsInPerfectCondition() {
    bool result;

    if (AiPlayer_Teams[team].IsUpgradeNeeded(&*target_unit)) {
        result = false;

    } else {
        if (target_unit->ammo < target_unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO)) {
            result = false;

        } else {
            result = true;
        }
    }

    return result;
}

void TaskUpgrade::CreateUnit() {
    ResourceID unit_type;

    unit_type = GetRepairShopType();

    if (unit_type != INVALID_ID) {
        CreateUnitIfNeeded(unit_type);
    }
}

void TaskUpgrade::IssueOrder() {
    operator_unit->SetParent(&*target_unit);

    if (AiPlayer_Teams[team].IsUpgradeNeeded(&*target_unit)) {
        UnitsManager_SetNewOrder(&*operator_unit, ORDER_UPGRADE, ORDER_STATE_0);

    } else if (target_unit->ammo < target_unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO)) {
        UnitsManager_SetNewOrder(&*operator_unit, ORDER_RELOAD, ORDER_STATE_0);
    }
}
