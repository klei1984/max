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

#include "pathfill.hpp"

#include "resource_manager.hpp"

PathFill::PathFill(unsigned char** map)
    : MAXFloodFill({0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y}, true), map(map) {}

int PathFill::Vfunc0(Point point, int uly) {
    for (; point.y > uly; --point.y) {
        unsigned char value;

        value = map[point.x][point.y - 1];

        if (!(value & 0x1F) || (value & 0x20)) {
            break;
        }
    }

    return point.y;
}

int PathFill::Vfunc1(Point point, int lry) {
    for (; point.y < lry; ++point.y) {
        unsigned char value;

        value = map[point.x][point.y];

        if (!(value & 0x1F) || (value & 0x20)) {
            break;
        }
    }

    return point.y;
}

int PathFill::Vfunc2(Point point, int lry) {
    for (; point.y < lry; ++point.y) {
        unsigned char value;

        value = map[point.x][point.y];

        if ((value & 0x1F) && !(value & 0x20)) {
            break;
        }
    }

    return point.y;
}

void PathFill::Vfunc3(int ulx, int uly, int lry) {
    for (; uly < lry; ++uly) {
        map[ulx][uly] |= 0x20;
    }
}
