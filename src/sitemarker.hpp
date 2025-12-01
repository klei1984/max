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

#ifndef SITEMARKER_HPP
#define SITEMARKER_HPP

#include "maxfloodfill.hpp"

/**
 * \class SiteMarker
 * \brief Flood fill implementation for marking connected site regions with unique identifiers.
 *
 * Used to identify and label distinct site areas on the map. Cells with value 9 are considered unfilled/fillable, and
 * the fill operation replaces them with a unique marker value to identify the connected region.
 */
class SiteMarker : public MAXFloodFill {
    uint16_t** m_map;
    uint16_t m_marker;

public:
    /**
     * \brief Constructs a SiteMarker for the given site map.
     *
     * \param map The site map to fill (will be modified by marking regions).
     */
    SiteMarker(uint16_t** map);

    int32_t FindRunTop(Point point, int32_t upper_bound) override;
    int32_t FindRunBottom(Point point, int32_t lower_bound) override;
    int32_t FindNextFillable(Point point, int32_t lower_bound) override;
    void MarkRun(int32_t grid_x, int32_t run_top, int32_t run_bottom) override;

    /**
     * \brief Performs flood fill starting from the given point, marking cells with the specified value.
     *
     * \param point The seed point to start the fill from.
     * \param value The marker value to assign to all filled cells.
     * \return The total number of cells filled.
     */
    int32_t Fill(Point point, int32_t value);
};

#endif /* SITEMARKER_HPP */
