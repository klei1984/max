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

#include "sitemarker.hpp"

#include "resource_manager.hpp"

SiteMarker::SiteMarker(uint16_t** map)
    : MAXFloodFill({0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y}, false), m_map(map), m_marker(0) {}

int32_t SiteMarker::FindRunTop(Point point, int32_t upper_bound) {
    for (; point.y > upper_bound; --point.y) {
        if (m_map[point.x][point.y - 1] != 9) {
            break;
        }
    }

    return point.y;
}

int32_t SiteMarker::FindRunBottom(Point point, int32_t lower_bound) {
    for (; point.y < lower_bound; ++point.y) {
        if (m_map[point.x][point.y] != 9) {
            break;
        }
    }

    return point.y;
}

int32_t SiteMarker::FindNextFillable(Point point, int32_t lower_bound) {
    for (; point.y < lower_bound; ++point.y) {
        if (m_map[point.x][point.y] == 9) {
            break;
        }
    }

    return point.y;
}

void SiteMarker::MarkRun(int32_t grid_x, int32_t run_top, int32_t run_bottom) {
    for (; run_top < run_bottom; ++run_top) {
        m_map[grid_x][run_top] = m_marker;
    }
}

int32_t SiteMarker::Fill(Point point, int32_t value) {
    m_marker = value;

    return MAXFloodFill::Fill(point);
}
