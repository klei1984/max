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

#include "unitpath.hpp"

#include "unitinfo.hpp"

UnitPath::UnitPath() : m_end_x(0), m_end_y(0), m_distance_x(0), m_distance_y(0), m_euclidean_distance(0) {}

UnitPath::UnitPath(int32_t target_x, int32_t target_y)
    : m_end_x(target_x), m_end_y(target_y), m_distance_x(0), m_distance_y(0), m_euclidean_distance(0) {}

UnitPath::UnitPath(int32_t distance_x, int32_t distance_y, int32_t euclidean_distance, int32_t target_x,
                   int32_t target_y)
    : m_end_x(target_x),
      m_end_y(target_y),
      m_distance_x(distance_x),
      m_distance_y(distance_y),
      m_euclidean_distance(euclidean_distance) {}

UnitPath::~UnitPath() {}

Point UnitPath::GetPosition(UnitInfo* unit) const { return Point(unit->grid_x, unit->grid_y); }

bool UnitPath::IsInPath(int32_t grid_x, int32_t grid_y) const { return false; }

void UnitPath::CancelMovement(UnitInfo* unit) {}

void UnitPath::UpdateUnitAngle(UnitInfo* unit) {}

bool UnitPath::IsEndStep() const { return false; }

void UnitPath::WritePacket(NetPacket& packet) {}

void UnitPath::ReadPacket(NetPacket& packet, int32_t steps_count) {}

void UnitPath::AppendLinearSteps(int32_t distance_x, int32_t distance_y) {}

int16_t UnitPath::GetEndX() const { return m_end_x; }

int16_t UnitPath::GetEndY() const { return m_end_y; }

int32_t UnitPath::GetDistanceX() const { return m_distance_x; }

int32_t UnitPath::GetDistanceY() const { return m_distance_y; }

int16_t UnitPath::GetEuclideanDistance() const { return m_euclidean_distance; }

void UnitPath::SetEndXY(int32_t target_x, int32_t target_y) {
    m_end_x = target_x;
    m_end_y = target_y;
}
