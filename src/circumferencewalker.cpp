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

#include "circumferencewalker.hpp"

#include "resource_manager.hpp"

CircumferenceWalker::CircumferenceWalker(Point position, int range_) {
    start = position;
    range = range_;
    distance = range * range;

    InitXY();
}

bool CircumferenceWalker::InitXY() {
    bool result;

    offset.x = 0;
    offset.y = -range;

    grid_x = &offset.x;
    grid_y = &offset.y;

    factor_x = 1;
    factor_y = 1;
    direction = 0;

    current.x = start.x + offset.x;
    current.y = start.y + offset.y;

    if (current.x >= 0 && current.x < ResourceManager_MapSize.x && current.y >= 0 &&
        current.y < ResourceManager_MapSize.y) {
        result = true;

    } else {
        result = FindNext();
    }

    return result;
}

bool CircumferenceWalker::InitDirection() {
    int factor1;
    int factor2;

    while (direction < 8) {
        if (direction & 1) {
            if (*grid_x != 0) {
                return true;
            }

        } else {
            if (labs(*grid_x) < labs(*grid_y)) {
                return true;
            }
        }

        ++direction;

        if ((direction + 2) & 4) {
            factor1 = -1;

        } else {
            factor1 = 1;
        }

        if (direction & 4) {
            factor2 = -1;

        } else {
            factor2 = 1;
        }

        if ((direction + 1) & 2) {
            grid_x = &offset.y;
            grid_y = &offset.x;
            factor_x = factor2;
            factor_y = factor1;

        } else {
            grid_x = &offset.x;
            grid_y = &offset.y;
            factor_x = factor1;
            factor_y = factor2;
        }
    }

    return false;
}

int CircumferenceWalker::GetGridX() const { return current.x; }

int CircumferenceWalker::GetGridY() const { return current.y; }

const Point* CircumferenceWalker::GetGridXY() const { return &current; }

bool CircumferenceWalker::FindNext() {
    int value1;
    int value2;
    int limit;

    while (InitDirection()) {
        *grid_x += factor_x;

        value1 = (distance - ((*grid_x) * (*grid_x))) * 4;
        value2 = (*grid_y) * (*grid_y) * 4;

        limit = factor_y * 4 * (*grid_y) + value2 + 1;

        if (limit <= value2) {
            if (value1 <= limit) {
                *grid_y += factor_y;
            }

        } else {
            if (value1 > limit) {
                *grid_y += factor_y;
            }
        }

        current.x = start.x + offset.x;
        current.y = start.y + offset.y;

        if (current.x >= 0 && current.x < ResourceManager_MapSize.x && current.y >= 0 &&
            current.y < ResourceManager_MapSize.y) {
            return true;
        }
    }

    return false;
}
