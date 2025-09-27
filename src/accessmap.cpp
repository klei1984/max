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
    size_x = static_cast<uint32_t>(ResourceManager_MapSize.x);
    uint32_t size_y = static_cast<uint32_t>(ResourceManager_MapSize.y);

    map = new (std::nothrow) uint8_t*[size_x];

    for (uint32_t i = 0; i < size_x; ++i) {
        map[i] = new (std::nothrow) uint8_t[size_y];

        SDL_memset(map[i], 0, size_y);
    }
}

AccessMap::~AccessMap() {
    for (uint32_t i = 0; i < size_x; ++i) {
        delete[] map[i];
    }

    delete[] map;
}

uint8_t** AccessMap::GetMap() const { return map; }

uint8_t* AccessMap::GetMapColumn(const uint32_t index) const {
    SDL_assert(index < size_x);

    return (index < size_x) ? map[index] : nullptr;
}
