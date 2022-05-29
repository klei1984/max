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

PathRequest::PathRequest(UnitInfo* unit, int mode, Point point) : unit1(unit), point(point), field_14(mode) {
    field_15 = 32767;
    field_17 = 0;
    field_19 = 0;
    field_20 = 0;
    field_21 = 1;
}

PathRequest::~PathRequest() {}

UnitInfo* PathRequest::GetUnit1() const { return &*unit1; }

UnitInfo* PathRequest::GetUnit2() const { return &*unit2; }

Point PathRequest::GetPoint() const { return point; }

unsigned char PathRequest::GetField19() const { return field_19; }

int PathRequest::PathRequest_Vfunc1() { return 0; }

void PathRequest::PathRequest_Vfunc2() {
    if (unit1->hits &&
        (unit1->orders == ORDER_MOVING || unit1->orders == ORDER_MOVING_27 || unit1->orders == ORDER_ATTACKING) &&
        unit1->state == ORDER_STATE_NEW_ORDER) {
        UnitsManager_SetNewOrder(&*unit1, unit1->orders, ORDER_STATE_12);
    }

    Access_ProcessNewGroupOrder(&*unit1);
}

void PathRequest::SetField15(int value) { field_15 = value; }

void PathRequest::SetField17(int value) { field_17 = value; }

void PathRequest::SetField19(int value) { field_19 = value; }

void PathRequest::SetField20(int value) { field_20 = value; }

void PathRequest::SetField21(int value) { field_21 = value; }

void PathRequest::CreateTransport(ResourceID unit_type) {
    if (unit_type != INVALID_ID) {
        unit2 = new (std::nothrow) UnitInfo(unit_type, unit1->team, 0xFFFF);

    } else {
        unit2 = nullptr;
    }
}

unsigned char PathRequest::GetField14() const { return field_14; }

unsigned short PathRequest::GetField15() const { return field_15; }

unsigned char PathRequest::GetField20() const { return field_20; }

void PathRequest::AssignGroundPath(UnitInfo* unit, GroundPath* path) {
    /// \todo
}

void PathRequest::PathRequest_Vfunc3(GroundPath* path) {
    /// \todo
}
