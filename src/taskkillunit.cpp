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

#include "taskkillunit.hpp"

#include "access.hpp"
#include "aiattack.hpp"
#include "units_manager.hpp"

TaskKillUnit::TaskKillUnit(TaskAttack* task_attack, SpottedUnit* spotted_unit_, unsigned short flags_)
    : Task(spotted_unit->GetTeam(), task_attack, flags_) {
    UnitInfo* target = spotted_unit_->GetUnit();
    spotted_unit = spotted_unit_;
    field_19 = 0;
    field_49 = 1;
    hits = target->hits;

    if (target->GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0 && target->team != PLAYER_TEAM_ALIEN) {
        required_damage = hits;
        hits *= 2;

    } else {
        required_damage = 1;
    }

    projected_damage = 0;
}

TaskKillUnit::~TaskKillUnit() {}

int TaskKillUnit::GetProjectedDamage(UnitInfo* unit, UnitInfo* threat) {
    int damage_potential =
        AiAttack_GetAttackPotential(unit, threat) * unit->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS);

    if (threat->GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0) {
        if ((threat->GetBaseValues()->GetAttribute(ATTRIB_SPEED) == 0 &&
             unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE) > threat->GetBaseValues()->GetAttribute(ATTRIB_RANGE)) ||
            !Access_IsValidAttackTargetType(threat->unit_type, unit->unit_type)) {
            damage_potential += threat->hits - 1;
        }
    }

    return damage_potential;
}

bool TaskKillUnit::IsUnitUsable(UnitInfo& unit_) {
    bool result;

    if (spotted_unit && unit_.GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) > 0 &&
        unit_.ammo >= unit_.GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) && unit != unit_) {
        if (weight_table.GetWeight(unit_.unit_type)) {
            result = hits > projected_damage;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

int TaskKillUnit::GetMemoryUse() const {}

unsigned short TaskKillUnit::GetFlags() const {}

char* TaskKillUnit::WriteStatusLog(char* buffer) const {
    if (spotted_unit && spotted_unit->GetUnit()) {
        char unit_name[50];

        spotted_unit->GetUnit()->GetDisplayName(unit_name);

        sprintf(buffer, "Kill %s at [%i,%i]", UnitsManager_BaseUnits[spotted_unit->GetUnit()->unit_type].singular_name,
                spotted_unit->GetLastPositionX() + 1, spotted_unit->GetLastPositionY() + 1);

        if (required_damage > projected_damage) {
            strcat(buffer,
                   SmartString().Sprintf(40, " (%i points needed)", required_damage - projected_damage).GetCStr());

        } else {
            strcat(buffer, " (ready)");
        }

    } else {
        strcpy(buffer, "Completed Kill Unit task.");
    }

    return buffer;
}

Rect* TaskKillUnit::GetBounds(Rect* bounds) {}

unsigned char TaskKillUnit::GetType() const {}

bool TaskKillUnit::Task_vfunc9() {}

void TaskKillUnit::Task_vfunc11(UnitInfo& unit) {}

void TaskKillUnit::BeginTurn() {}

void TaskKillUnit::ChildComplete(Task* task) {}

void TaskKillUnit::EndTurn() {}

bool TaskKillUnit::Task_vfunc17(UnitInfo& unit) {}

void TaskKillUnit::RemoveSelf() {}

bool TaskKillUnit::Task_vfunc19() {}

void TaskKillUnit::RemoveUnit(UnitInfo& unit) {}

void TaskKillUnit::Task_vfunc23(UnitInfo& unit) {}

void TaskKillUnit::Task_vfunc25(UnitInfo& unit) {}
