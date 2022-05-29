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

#include "searcher.hpp"

#include "resource_manager.hpp"

Searcher::Searcher(Point point1, Point point2, unsigned char mode) : mode(mode) {
    Point map_size;
    PathSquare square;

    matrix1 = new (std::nothrow) unsigned short*[ResourceManager_MapSize.x];
    matrix2 = new (std::nothrow) unsigned char*[ResourceManager_MapSize.x];

    for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
        matrix1[i] = new (std::nothrow) unsigned short[ResourceManager_MapSize.y];
        matrix2[i] = new (std::nothrow) unsigned char[ResourceManager_MapSize.y];

        memset(matrix2[i], 0xFF, ResourceManager_MapSize.y);

        for (int j = 0; j < ResourceManager_MapSize.y; ++j) {
            matrix1[i][j] = 0x3FFF;
        }
    }

    field_12 = 0;

    map_size.x = ResourceManager_MapSize.x - point1.x;
    map_size.y = ResourceManager_MapSize.y - point1.y;

    if (map_size.x < point1.x) {
        map_size.x = point1.x;
    }

    if (map_size.y < point1.y) {
        map_size.y = point1.y;
    }

    {
        int array_size;

        if (map_size.x <= map_size.y) {
            array_size = map_size.y * 2 + map_size.x;

        } else {
            array_size = map_size.x * 2 + map_size.y;
        }

        array = new (std::nothrow) unsigned short[array_size];

        for (int i = 0; i < array_size; ++i) {
            array[i] = 0x7FFF;
        }

        array[0] = 0;
        matrix1[point1.x][point1.y] = 0;

        square.point.x = point1.x;
        square.point.y = point1.y;
        square.weight = 0;

        squares.Append(&square);
        point = point2;
    }
}

Searcher::~Searcher() {
    for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
        delete[] matrix1[i];
        delete[] matrix2[i];
    }

    delete[] matrix1;
    delete[] matrix2;
    delete[] array;
}
