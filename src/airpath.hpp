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

#ifndef AIRPATH_HPP
#define AIRPATH_HPP

#include "unitpath.hpp"

class AirPath : public UnitPath {
    int16_t m_length;
    char m_angle;
    int16_t m_start_x;
    int16_t m_start_y;
    int32_t m_step_x;
    int32_t m_step_y;
    int32_t m_delta_x;
    int32_t m_delta_y;

public:
    AirPath();
    AirPath(UnitInfo* unit, int32_t distance_x, int32_t distance_y, int32_t euclidean_distance, int32_t target_x,
            int32_t target_y);
    ~AirPath();

    static FileObject* Allocate() noexcept;

    uint32_t GetTypeIndex() const;
    void FileLoad(SmartFileReader& file) noexcept;
    void FileSave(SmartFileWriter& file) noexcept;
    Point GetPosition(UnitInfo* unit) const;
    void CancelMovement(UnitInfo* unit);
    int32_t GetMovementCost(UnitInfo* unit);
    bool Execute(UnitInfo* unit);
    void Draw(UnitInfo* unit, WindowInfo* window);
};

#endif /* AIRPATH_HPP */
