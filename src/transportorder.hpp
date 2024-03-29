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

#ifndef TRANSPORTORDER_HPP
#define TRANSPORTORDER_HPP

#include "paths.hpp"
#include "taskpathrequest.hpp"

class TransportOrder : public SmartObject {
    uint8_t transport_category;
    ResourceID unit_type;
    SmartPointer<GroundPath> path;
    Point position;

public:
    TransportOrder(UnitInfo* unit, ResourceID unit_type, GroundPath* path);
    ~TransportOrder();

    ResourceID GetUnitType() const;
    uint8_t GetTransportCategory() const;
    static int32_t DetermineCategory(ResourceID unit_type);
    Point MatchClosestPosition(Point target, Point current);
    Point MatchStartPosition(Point target);
    void UsePrecalculatedPath(TaskPathRequest* request);
};

#endif /* TRANSPORTORDER_HPP */
