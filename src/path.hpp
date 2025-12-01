/* Copyright (c) 2025 M.A.X. Port Team
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

#ifndef PATH_HPP
#define PATH_HPP

#include "point.hpp"

/**
 * \brief 8-direction indices for grid navigation.
 *
 * Direction values correspond to clockwise compass directions starting from NORTH.
 * Each direction maps to an offset in DIRECTION_OFFSETS.
 */
enum Direction : uint8_t {
    DIRECTION_NORTH = 0,
    DIRECTION_NORTH_EAST = 1,
    DIRECTION_EAST = 2,
    DIRECTION_SOUTH_EAST = 3,
    DIRECTION_SOUTH = 4,
    DIRECTION_SOUTH_WEST = 5,
    DIRECTION_WEST = 6,
    DIRECTION_NORTH_WEST = 7,
    DIRECTION_COUNT = 8
};

/**
 * \brief Offset values for each of the 8 grid directions.
 *
 * Index with Direction enum values to get (dx, dy) offsets for movement.
 * Order: N, NE, E, SE, S, SW, W, NW (clockwise from north).
 */
inline constexpr Point DIRECTION_OFFSETS[DIRECTION_COUNT] = {{0, -1}, {1, -1}, {1, 0},  {1, 1},
                                                             {0, 1},  {-1, 1}, {-1, 0}, {-1, -1}};

#endif /* PATH_HPP */
