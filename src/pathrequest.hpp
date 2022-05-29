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
    unsigned char field_14;
    unsigned short field_15;
    unsigned short field_17;
    unsigned char field_19;
    unsigned char field_20;
    unsigned char field_21;

    static void AssignGroundPath(UnitInfo* unit, GroundPath* path);

public:
    PathRequest(UnitInfo* unit, int mode, Point point);
    ~PathRequest();

    UnitInfo* GetUnit1() const;
    UnitInfo* GetUnit2() const;
    Point GetPoint() const;
    unsigned char GetField14() const;
    unsigned short GetField15() const;
    unsigned char GetField19() const;
    unsigned char GetField20() const;

    void SetField15(int value);
    void SetField17(int value);
    void SetField19(int value);
    void SetField20(int value);
    void SetField21(int value);

    void CreateTransport(ResourceID unit_type);

    virtual int PathRequest_Vfunc1();
    virtual void PathRequest_Vfunc2();
    virtual void PathRequest_Vfunc3(GroundPath* path);
};

#endif /* PATHREQUEST_HPP */
