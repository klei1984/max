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

#ifndef PATHREQUEST_HPP
#define PATHREQUEST_HPP

#include "unitinfo.hpp"

class PathRequest : public SmartObject {
    SmartPointer<UnitInfo> unit1;
    SmartPointer<UnitInfo> unit2;
    Point point;
    unsigned char flags;
    unsigned short max_cost;
    unsigned short minimum_distance;
    unsigned char caution_level;
    bool board_transport;
    bool optimize;

    static void AssignGroundPath(UnitInfo* unit, GroundPath* path);

public:
    PathRequest(UnitInfo* unit, int mode, Point point);
    ~PathRequest();

    UnitInfo* GetUnit1() const;
    UnitInfo* GetUnit2() const;
    Point GetPoint() const;
    unsigned char GetFlags() const;
    unsigned short GetMaxCost() const;
    unsigned char GetCautionLevel() const;
    unsigned char GetBoardTransport() const;
    unsigned short GetMinimumDistance() const;

    void SetMaxCost(int value);
    void SetMinimumDistance(int value);
    void SetCautionLevel(int value);
    void SetBoardTransport(bool value);
    void SetOptimizeFlag(bool value);

    void CreateTransport(ResourceID unit_type);

    virtual int PathRequest_Vfunc1();
    virtual void Cancel();
    virtual void Finish(GroundPath* path);
};

#endif /* PATHREQUEST_HPP */
