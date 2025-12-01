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

#ifndef PATHFILL_HPP
#define PATHFILL_HPP

#include "accessmap.hpp"
#include "maxfloodfill.hpp"

/**
 * \class PathFill
 * \brief Flood fill implementation for marking connected pathable regions.
 *
 * Used by the pathfinding system to identify contiguous areas that a unit can traverse. Cells are considered fillable
 * if they have a non-zero traversal cost (bits 0-4) and have not been visited (bit 5 clear). The fill marks visited
 * cells by setting bit 5.
 *
 * Map cell format:
 * - Bits 0-4: Traversal cost (0 = impassable, 1-31 = passable with cost)
 * - Bit 5 (0x20): Visited/filled marker
 */
class PathFill : public MAXFloodFill {
    AccessMap& m_map;

public:
    /**
     * \brief Constructs a PathFill for the given access map.
     *
     * \param map The access map to fill (will be modified by setting visited bits).
     */
    explicit PathFill(AccessMap& map);

    int32_t FindRunTop(Point point, int32_t upper_bound) override;
    int32_t FindRunBottom(Point point, int32_t lower_bound) override;
    int32_t FindNextFillable(Point point, int32_t lower_bound) override;
    void MarkRun(int32_t grid_x, int32_t run_top, int32_t run_bottom) override;
};

#endif /* PATHFILL_HPP */
