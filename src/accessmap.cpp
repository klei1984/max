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

#include "accessmap.hpp"

#include "resource_manager.hpp"

AccessMap::AccessMap() {
    size = ResourceManager_MapSize.x;

    map = new (std::nothrow) unsigned char*[ResourceManager_MapSize.x];

    for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
        map[i] = new (std::nothrow) unsigned char[ResourceManager_MapSize.y];

        memset(map[i], 0, ResourceManager_MapSize.y);
    }
}

AccessMap::~AccessMap() {
    for (int i = 0; i < size; ++i) {
        delete[] map[i];
    }

    delete[] map;
}

unsigned char** AccessMap::GetMap() const { return map; }

unsigned char* AccessMap::GetMapColumn(int index) const { return map[index]; }
