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

#include "spottedunit.hpp"

SpottedUnit::SpottedUnit(UnitInfo* unit_, unsigned short team_) {
    unit = unit_;

    team = team_;

    visible_to_team = unit_->IsVisibleToTeam(team_);

    last_position.x = unit_->grid_x;
    last_position.y = unit_->grid_y;
}

SpottedUnit::SpottedUnit(SmartFileReader& file) { FileLoad(file); }

SpottedUnit::~SpottedUnit() {}

void SpottedUnit::FileLoad(SmartFileReader& file) {
    static_assert(sizeof(SpottedUnit::visible_to_team) == sizeof(unsigned char));

    unit = dynamic_cast<UnitInfo*>(file.ReadObject());
    file.Read(team);
    file.Read(visible_to_team);
    file.Read(last_position);
}

void SpottedUnit::FileSave(SmartFileWriter& file) {
    file.WriteObject(&*unit);
    file.Write(team);
    file.Write(visible_to_team);
    file.Write(last_position);
}

Task* SpottedUnit::GetTask() const { return &*task; }

void SpottedUnit::SetTask(Task* task_) { task = task_; }

Point SpottedUnit::GetLastPosition() {
    UpdatePositionIfVisible();

    return last_position;
}

int SpottedUnit::GetLastPositionX() {
    UpdatePositionIfVisible();

    return last_position.x;
}

int SpottedUnit::GetLastPositionY() {
    UpdatePositionIfVisible();

    return last_position.y;
}

void SpottedUnit::SetPosition(Point position) { last_position = position; }

void SpottedUnit::UpdatePosition() {
    last_position.x = unit->grid_x;
    last_position.y = unit->grid_y;
}

void SpottedUnit::UpdatePositionIfVisible() {
    if (visible_to_team || unit->IsVisibleToTeam(team)) {
        last_position.x = unit->grid_x;
        last_position.y = unit->grid_y;

        visible_to_team = unit->IsVisibleToTeam(team);
    }
}

UnitInfo* SpottedUnit::GetUnit() const { return &*unit; }

unsigned short SpottedUnit::GetTeam() const { return team; }
