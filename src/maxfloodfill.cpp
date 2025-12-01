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

#include "maxfloodfill.hpp"

#include "ailog.hpp"
#include "smartobjectarray.hpp"

struct FloodRun {
    int16_t grid_x;
    int16_t run_top;
    int16_t run_bottom;
};

MAXFloodFill::MAXFloodFill(Rect search_bounds, bool diagonal_expansion)
    : m_diagonal_expansion(diagonal_expansion), m_search_bounds(search_bounds), m_cell_count(0) {}

Rect* MAXFloodFill::GetBounds() { return &m_filled_bounds; }

int32_t MAXFloodFill::Fill(Point point) {
    FloodRun run;
    ObjectArray<FloodRun> runs;
    int32_t runs_count;
    int32_t max_runs = 0;
    uint64_t time_stamp = timer_get();

    m_cell_count = 0;

    m_filled_bounds.ulx = point.x;
    m_filled_bounds.uly = point.y;
    m_filled_bounds.lrx = point.x;
    m_filled_bounds.lry = point.y;

    run.run_top = FindRunTop(point, m_search_bounds.uly);
    run.run_bottom = FindRunBottom(point, m_search_bounds.lry);
    run.grid_x = point.x;

    m_cell_count += run.run_bottom - run.run_top;

    MarkRun(point.x, run.run_top, run.run_bottom);

    runs.Append(&run);

    while (runs.GetCount()) {
        runs_count = runs.GetCount();

        if (runs_count > max_runs) {
            max_runs = runs_count;
        }

        run = *runs[runs_count - 1];

        runs.Remove(runs_count - 1);

        if (run.run_top < m_filled_bounds.uly) {
            m_filled_bounds.uly = run.run_top;
        }

        if (run.grid_x < m_filled_bounds.ulx) {
            m_filled_bounds.ulx = run.grid_x;
        }

        if (run.grid_x >= m_filled_bounds.lrx) {
            m_filled_bounds.lrx = run.grid_x;
        }

        if (run.run_bottom > m_filled_bounds.lry) {
            m_filled_bounds.lry = run.run_bottom;
        }

        if (m_diagonal_expansion) {
            run.run_top = std::max(m_search_bounds.uly, run.run_top - 1);
            run.run_bottom = std::min(m_search_bounds.lry, run.run_bottom + 1);
        }

        if (run.grid_x > m_search_bounds.ulx) {
            point.x = run.grid_x - 1;
            point.y = run.run_top;

            while (point.y < run.run_bottom) {
                point.y = FindNextFillable(point, run.run_bottom);

                if (point.y < run.run_bottom) {
                    FloodRun inner_run;

                    inner_run.grid_x = point.x;

                    inner_run.run_top = FindRunTop(point, m_search_bounds.uly);
                    inner_run.run_bottom = FindRunBottom(point, m_search_bounds.lry);
                    point.y = inner_run.run_bottom;

                    m_cell_count += inner_run.run_bottom - inner_run.run_top;

                    MarkRun(inner_run.grid_x, inner_run.run_top, inner_run.run_bottom);

                    runs.Append(&inner_run);
                }
            }
        }

        if (run.grid_x < m_search_bounds.lrx - 1) {
            point.x = run.grid_x + 1;
            point.y = run.run_top;

            while (point.y < run.run_bottom) {
                point.y = FindNextFillable(point, run.run_bottom);

                if (point.y < run.run_bottom) {
                    FloodRun inner_run;

                    inner_run.grid_x = point.x;

                    inner_run.run_top = FindRunTop(point, m_search_bounds.uly);
                    inner_run.run_bottom = FindRunBottom(point, m_search_bounds.lry);
                    point.y = inner_run.run_bottom;

                    m_cell_count += inner_run.run_bottom - inner_run.run_top;

                    MarkRun(inner_run.grid_x, inner_run.run_top, inner_run.run_bottom);

                    runs.Append(&inner_run);
                }
            }
        }
    }

    ++m_filled_bounds.lrx;

    uint64_t elapsed_time = timer_elapsed_time(time_stamp);

    if (elapsed_time > 10 || max_runs > 10) {
        AILOG(log, "Flood fill, {} msecs, {} max depth.", elapsed_time, max_runs);
    }

    return m_cell_count;
}
