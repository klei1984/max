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

#ifndef MAXFLOODFILL_HPP
#define MAXFLOODFILL_HPP

#include "gnw.h"
#include "point.hpp"

class MAXFloodFill {
    bool mode;
    Rect bounds;
    Rect target_bounds;
    int32_t cell_count;

public:
    MAXFloodFill(Rect bounds, bool mode);

    virtual int32_t Vfunc0(Point point, int32_t uly) = 0;
    virtual int32_t Vfunc1(Point point, int32_t lry) = 0;
    virtual int32_t Vfunc2(Point point, int32_t lry) = 0;
    virtual void Vfunc3(int32_t ulx, int32_t uly, int32_t lry) = 0;

    Rect* GetBounds();
    int32_t Fill(Point point);
};

#endif /* MAXFLOODFILL_HPP */
