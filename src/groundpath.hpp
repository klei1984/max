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

#ifndef GROUNDPATH_HPP
#define GROUNDPATH_HPP

#include "smartobjectarray.hpp"
#include "unitpath.hpp"

class GroundPath : public UnitPath {
    uint32_t m_step_index;
    SmartObjectArray<PathStep> m_steps;

public:
    GroundPath();
    GroundPath(int32_t target_x, int32_t target_y);
    ~GroundPath();

    static FileObject* Allocate() noexcept;

    uint32_t GetTypeIndex() const;
    void FileLoad(SmartFileReader& file) noexcept;
    void FileSave(SmartFileWriter& file) noexcept;
    Point GetPosition(UnitInfo* unit) const;
    bool IsInPath(int32_t grid_x, int32_t grid_y) const;
    void CancelMovement(UnitInfo* unit);
    int32_t GetMovementCost(UnitInfo* unit);
    bool Execute(UnitInfo* unit);
    void UpdateUnitAngle(UnitInfo* unit);
    void Draw(UnitInfo* unit, WindowInfo* window);
    bool IsEndStep() const;
    void WritePacket(NetPacket& packet);
    void ReadPacket(NetPacket& packet, int32_t steps_count);
    void AppendLinearSteps(int32_t distance_x, int32_t distance_y);

    void AddStep(int32_t step_x, int32_t step_y);
    SmartObjectArray<PathStep> GetSteps();
    uint32_t GetStepIndex() const;
};

#endif /* GROUNDPATH_HPP */
