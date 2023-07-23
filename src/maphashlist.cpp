/* Copyright (c) 2021 M.A.X. Port Team
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

#include "maphashlist.hpp"

MapHashList::MapHashList(UnitInfo& unit, uint16_t team) : unit(unit), team(team) {
    visible = unit.IsVisibleToTeam(team);
    point.x = unit.grid_x;
    point.y = unit.grid_y;
}

MapHashList::MapHashList(SmartFileReader& file) {
    unit = dynamic_cast<UnitInfo*>(file.ReadObject());
    file.Read(team);
    file.Read(visible);
    file.Read(point);
}

MapHashList::~MapHashList() {}

void MapHashList::FileSave(SmartFileWriter& file) {
    file.WriteObject(&*unit);
    file.Write(team);
    file.Write(visible);
    file.Write(point);
}

Task* MapHashList::GetTask() { return &*task; }

void MapHashList::SetTask(Task* task) { this->task = task; }

Point MapHashList::GetPosition() {
    UpdateVisibility();
    return Point(point);
}

void MapHashList::SetPosition(Point point) { this->point = point; }

int16_t MapHashList::GetPositionX() {
    UpdateVisibility();
    return point.x;
}

int16_t MapHashList::GetPositionY() {
    UpdateVisibility();
    return point.y;
}

void MapHashList::UpdatePosition() {
    point.x = unit->grid_x;
    point.y = unit->grid_y;
}

void MapHashList::UpdateVisibility() {
    point.x = unit->grid_x;
    point.y = unit->grid_y;
}
