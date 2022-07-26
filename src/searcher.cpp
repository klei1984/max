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

#include "game_manager.hpp"
#include "gfx.hpp"
#include "paths.hpp"
#include "resource_manager.hpp"
#include "window_manager.hpp"

static void Searcher_DrawMarker(int angle, int grid_x, int grid_y, int color);
static int Searcher_EvaluateCost(Point point1, Point point2, bool mode);

void Searcher_DrawMarker(int angle, int grid_x, int grid_y, int color) {
    WindowInfo* window;
    int pixel_x;
    int pixel_y;
    Rect bounds;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    pixel_x = grid_x * 64 + 32;
    pixel_y = grid_y * 64 + 32;

    if (pixel_x < GameManager_MapWindowDrawBounds.lrx && pixel_x > GameManager_MapWindowDrawBounds.ulx &&
        pixel_y < GameManager_MapWindowDrawBounds.lry && pixel_y > GameManager_MapWindowDrawBounds.uly) {
    }

    grid_x = (pixel_x << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
    grid_y = (pixel_y << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

    Paths_DrawMarker(window, angle, grid_x, grid_y, color);

    bounds.ulx = window->window.ulx + grid_x - (Gfx_ZoomLevel / 2);
    bounds.uly = window->window.uly + grid_y - (Gfx_ZoomLevel / 2);
    bounds.lrx = bounds.ulx + Gfx_ZoomLevel;
    bounds.lry = bounds.uly + Gfx_ZoomLevel;

    win_draw_rect(window->id, &bounds);
}

int Searcher_EvaluateCost(Point point1, Point point2, bool mode) {
    unsigned char value1;
    unsigned char value2;
    int result;

    value2 = PathsManager_AccessMap[point2.x][point2.y];

    ++Paths_EvaluatorCallCount;

    if (mode) {
        value1 = PathsManager_AccessMap[point1.x][point1.y];

        if ((value2 & 0x40) && (value1 & 0x80)) {
            result = 0;

        } else if ((value1 & 0x40) && (value2 & 0x80)) {
            result = 0;

        } else {
            result = value2 & 0x1F;
        }

    } else {
        result = value2 & 0x1F;
    }

    return result;
}

Searcher::Searcher(Point point1, Point point2, unsigned char mode) : mode(mode) {
    Point map_size;
    PathSquare square;

    costs_map = new (std::nothrow) unsigned short*[ResourceManager_MapSize.x];
    directions_map = new (std::nothrow) unsigned char*[ResourceManager_MapSize.x];

    for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
        costs_map[i] = new (std::nothrow) unsigned short[ResourceManager_MapSize.y];
        directions_map[i] = new (std::nothrow) unsigned char[ResourceManager_MapSize.y];

        memset(directions_map[i], 0xFF, ResourceManager_MapSize.y);

        for (int j = 0; j < ResourceManager_MapSize.y; ++j) {
            costs_map[i][j] = 0x3FFF;
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
        costs_map[point1.x][point1.y] = 0;

        square.point.x = point1.x;
        square.point.y = point1.y;
        square.weight = 0;

        squares.Append(&square);
        destination = point2;
    }
}

Searcher::~Searcher() {
    for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
        delete[] costs_map[i];
        delete[] directions_map[i];
    }

    delete[] costs_map;
    delete[] directions_map;
    delete[] array;
}
