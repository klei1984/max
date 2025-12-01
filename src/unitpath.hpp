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

#ifndef UNITPATH_HPP
#define UNITPATH_HPP

#include "gnw.h"
#include "point.hpp"
#include "smartfile.hpp"

class UnitInfo;
class NetPacket;

class UnitPath : public FileObject {
protected:
    int16_t m_end_x;
    int16_t m_end_y;
    int32_t m_distance_x;
    int32_t m_distance_y;
    int16_t m_euclidean_distance;

public:
    UnitPath();
    UnitPath(int32_t target_x, int32_t target_y);
    UnitPath(int32_t distance_x, int32_t distance_y, int32_t euclidean_distance, int32_t target_x, int32_t target_y);
    virtual ~UnitPath();

    virtual uint32_t GetTypeIndex() const = 0;
    virtual void FileLoad(SmartFileReader& file) noexcept = 0;
    virtual void FileSave(SmartFileWriter& file) noexcept = 0;
    virtual Point GetPosition(UnitInfo* unit) const;
    virtual bool IsInPath(int32_t grid_x, int32_t grid_y) const;
    virtual void CancelMovement(UnitInfo* unit);
    virtual int32_t GetMovementCost(UnitInfo* unit) = 0;
    virtual bool Execute(UnitInfo* unit) = 0;
    virtual void UpdateUnitAngle(UnitInfo* unit);
    virtual void Draw(UnitInfo* unit, WindowInfo* window) = 0;
    virtual bool IsEndStep() const;
    virtual void WritePacket(NetPacket& packet);
    virtual void ReadPacket(NetPacket& packet, int32_t steps_count);
    virtual void AppendLinearSteps(int32_t distance_x, int32_t distance_y);

    int16_t GetEndX() const;
    int16_t GetEndY() const;
    int32_t GetDistanceX() const;
    int32_t GetDistanceY() const;
    int16_t GetEuclideanDistance() const;
    void SetEndXY(int32_t target_x, int32_t target_y);
};

#endif /* UNITPATH_HPP */
