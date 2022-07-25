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

#include "pathrequest.hpp"

#include "access.hpp"
#include "units_manager.hpp"

PathRequest::PathRequest(UnitInfo* unit, int mode, Point point) : unit1(unit), point(point), flags(mode) {
    max_cost = 32767;
    minimum_distance = 0;
    caution_level = 0;
    board_transport = 0;
    optimize = 1;
}

PathRequest::~PathRequest() {}

UnitInfo* PathRequest::GetUnit1() const { return &*unit1; }

UnitInfo* PathRequest::GetUnit2() const { return &*unit2; }

Point PathRequest::GetPoint() const { return point; }

unsigned char PathRequest::GetCautionLevel() const { return caution_level; }

int PathRequest::PathRequest_Vfunc1() { return 0; }

void PathRequest::Cancel() {
    if (unit1->hits &&
        (unit1->orders == ORDER_MOVE || unit1->orders == ORDER_MOVE_TO_UNIT || unit1->orders == ORDER_MOVE_TO_ATTACK) &&
        unit1->state == ORDER_STATE_NEW_ORDER) {
        UnitsManager_SetNewOrder(&*unit1, unit1->orders, ORDER_STATE_12);
    }

    Access_ProcessNewGroupOrder(&*unit1);
}

void PathRequest::SetMaxCost(int value) { max_cost = value; }

void PathRequest::SetMinimumDistance(int value) { minimum_distance = value; }

void PathRequest::SetCautionLevel(int value) { caution_level = value; }

void PathRequest::SetBoardTransport(bool value) { board_transport = value; }

void PathRequest::SetOptimizeFlag(bool value) { optimize = value; }

void PathRequest::CreateTransport(ResourceID unit_type) {
    if (unit_type != INVALID_ID) {
        unit2 = new (std::nothrow) UnitInfo(unit_type, unit1->team, 0xFFFF);

    } else {
        unit2 = nullptr;
    }
}

unsigned char PathRequest::GetFlags() const { return flags; }

unsigned short PathRequest::GetMaxCost() const { return max_cost; }

unsigned char PathRequest::GetBoardTransport() const { return board_transport; }

void PathRequest::AssignGroundPath(UnitInfo* unit, GroundPath* path) {
    /// \todo
}

void PathRequest::Finish(GroundPath* path) {
    /// \todo
}
