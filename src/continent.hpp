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

#ifndef CONTINENT_HPP
#define CONTINENT_HPP

#include "gnw.h"
#include "point.hpp"
#include "smartpointer.hpp"

class Continent : public SmartObject {
    bool is_isolated;
    unsigned short filler;
    unsigned short continent_size;
    Rect bounds;
    Point point;
    unsigned char **map;
    unsigned char field_35;

    bool IsDangerousProximity(int grid_x, int grid_y, unsigned short team, int proximity_range);
    bool IsViableSite(bool test_proximity, unsigned short team, Point site);

public:
    Continent(unsigned char **map, unsigned short filler, Point point, unsigned char value = 1);
    ~Continent();

    Rect GetBounds() const;
    Point GetCenter() const;
    bool IsIsolated() const;
    unsigned short GetContinentSize() const;
    unsigned short GetFiller() const;

    bool IsCloseProximity() const;
    bool IsViableContinent(bool test_proximity, unsigned short team);
    void SelectLandingSite(unsigned short team, int strategy);
    void TestIsolated();
};

#endif /* CONTINENT_HPP */
