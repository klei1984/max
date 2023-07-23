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
    SmartPointer<UnitInfo> transporter;
    Point point;
    uint8_t flags;
    uint16_t max_cost;
    uint16_t minimum_distance;
    uint8_t caution_level;
    bool board_transport;
    bool optimize;

    static void AssignGroundPath(UnitInfo* unit, GroundPath* path);

protected:
    SmartPointer<UnitInfo> client;

public:
    PathRequest(UnitInfo* unit, int32_t mode, Point point);
    ~PathRequest();

    UnitInfo* GetClient() const;
    UnitInfo* GetTransporter() const;
    Point GetDestination() const;
    uint8_t GetFlags() const;
    uint16_t GetMaxCost() const;
    uint8_t GetCautionLevel() const;
    uint8_t GetBoardTransport() const;
    uint16_t GetMinimumDistance() const;

    void SetMaxCost(int32_t value);
    void SetMinimumDistance(int32_t value);
    void SetCautionLevel(int32_t value);
    void SetBoardTransport(bool value);
    void SetOptimizeFlag(bool value);

    void CreateTransport(ResourceID unit_type);

    virtual bool PathRequest_Vfunc1();
    virtual void Cancel();
    virtual void Finish(GroundPath* path);
};

extern const char* PathRequest_CautionLevels[];

#endif /* PATHREQUEST_HPP */
