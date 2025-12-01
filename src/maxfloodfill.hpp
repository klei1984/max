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

/**
 * \class MAXFloodFill
 * \brief Abstract base class for scanline flood fill algorithms.
 *
 * Implements an optimized scanline flood fill that processes vertical "runs" (columns of contiguous fillable cells)
 * rather than individual pixels. This approach is significantly faster than recursive or stack-based pixel-by-pixel
 * flood fill for large areas.
 *
 * Algorithm overview:
 * 1. Start at a seed point and find the vertical run containing it
 * 2. Mark all cells in the run as filled
 * 3. For adjacent columns (left and right), find new fillable runs within the Y range
 * 4. Repeat until no more runs are found
 *
 * Derived classes implement the virtual methods to define:
 * - What constitutes a "fillable" cell (the fill condition)
 * - How cells are marked as filled
 *
 * The `m_diagonal_expansion` flag controls whether runs expand diagonally (checking one cell above/below the current
 * run when scanning adjacent columns).
 */
class MAXFloodFill {
    bool m_diagonal_expansion;
    Rect m_filled_bounds;
    Rect m_search_bounds;
    int32_t m_cell_count;

public:
    /**
     * \brief Constructs a MAXFloodFill instance.
     *
     * \param search_bounds The rectangular boundary within which flood fill operates.
     * \param diagonal_expansion If true, runs can expand diagonally by checking one cell above/below.
     */
    MAXFloodFill(Rect search_bounds, bool diagonal_expansion);

    /**
     * \brief Scan upward to find the top of a fillable run.
     *
     * Starting from `point`, scans upward (decreasing Y) until a non-fillable cell is found or `upper_bound` is
     * reached.
     *
     * \param point The starting position to scan from.
     * \param upper_bound The minimum Y coordinate (exclusive) to stop scanning.
     * \return The Y coordinate of the topmost fillable cell in the run.
     */
    virtual int32_t FindRunTop(Point point, int32_t upper_bound) = 0;

    /**
     * \brief Scan downward to find the bottom of a fillable run.
     *
     * Starting from `point`, scans downward (increasing Y) until a non-fillable cell is found or `lower_bound` is
     * reached.
     *
     * \param point The starting position to scan from.
     * \param lower_bound The maximum Y coordinate (exclusive) to stop scanning.
     * \return The Y coordinate just past the bottommost fillable cell in the run.
     */
    virtual int32_t FindRunBottom(Point point, int32_t lower_bound) = 0;

    /**
     * \brief Find the next fillable cell in a column.
     *
     * Scans downward from `point` to find the next cell that satisfies the fill condition. Used to find new runs in
     * adjacent columns.
     *
     * \param point The starting position to scan from.
     * \param lower_bound The maximum Y coordinate (exclusive) to stop scanning.
     * \return The Y coordinate of the next fillable cell, or `lower_bound` if none found.
     */
    virtual int32_t FindNextFillable(Point point, int32_t lower_bound) = 0;

    /**
     * \brief Mark all cells in a vertical run as filled.
     *
     * \param grid_x The X coordinate of the column.
     * \param run_top The Y coordinate of the top of the run (inclusive).
     * \param run_bottom The Y coordinate just past the bottom of the run (exclusive).
     */
    virtual void MarkRun(int32_t grid_x, int32_t run_top, int32_t run_bottom) = 0;

    /**
     * \brief Gets the bounding rectangle of all filled cells.
     *
     * \return Pointer to the bounds rectangle (valid after Fill() completes).
     */
    Rect* GetBounds();

    /**
     * \brief Performs the flood fill starting from the given point.
     *
     * \param point The seed point to start the fill from.
     * \return The total number of cells filled.
     */
    int32_t Fill(Point point);
};

#endif /* MAXFLOODFILL_HPP */
