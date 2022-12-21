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

#include "zonewalker.hpp"

#include "resource_manager.hpp"

ZoneWalker::ZoneWalker(Point position, int range_) {
    range = range_;
    start = position;
    distance = range * range;
    limit.y = start.y + range;

    if (limit.y > ResourceManager_MapSize.y - 1) {
        limit.y = ResourceManager_MapSize.y - 1;
    }

    InitXY();
}

void ZoneWalker::InitXY() {
    current.y = start.y - range;

    if (current.y < 0) {
        current.y = 0;
    }

    InitX();
}

void ZoneWalker::InitX() {
    int distance_x;
    int distance_y;
    int index_x;

    distance_y = (current.y - start.y) * (current.y - start.y);

    for (index_x = start.x - range; index_x <= start.x; ++index_x) {
        distance_x = (index_x - start.x) * (index_x - start.x);

        if (distance_x + distance_y <= distance) {
            break;
        }
    }

    current.x = index_x;
    limit.x = start.x * 2 - index_x;

    if (current.x < 0) {
        current.x = 0;
    }

    if (limit.x > ResourceManager_MapSize.x - 1) {
        limit.x = ResourceManager_MapSize.x - 1;
    }
}

int ZoneWalker::GetGridX() const { return current.x; }

int ZoneWalker::GetGridY() const { return current.y; }

Point* ZoneWalker::GetCurrentLocation() { return &current; }

bool ZoneWalker::FindNext() {
    bool result;

    ++current.x;

    if (current.x <= limit.x) {
        result = true;

    } else {
        ++current.y;

        if (current.y <= limit.y) {
            InitX();

            result = true;

        } else {
            result = false;
        }
    }

    return result;
}
