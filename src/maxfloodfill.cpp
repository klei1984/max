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
    short grid_x;
    short uly;
    short lry;
};

MAXFloodFill::MAXFloodFill(Rect bounds, bool mode) : mode(mode), target_bounds(bounds), cell_count(0) {}

Rect* MAXFloodFill::GetBounds() { return &bounds; }

int MAXFloodFill::Fill(Point point) {
    FloodRun run;
    ObjectArray<FloodRun> runs;
    int runs_count;
    int max_runs = 0;
    unsigned int time_stamp = timer_get();

    cell_count = 0;

    bounds.ulx = point.x;
    bounds.uly = point.y;
    bounds.lrx = point.x;
    bounds.lry = point.y;

    run.uly = Vfunc0(point, target_bounds.uly);
    run.lry = Vfunc1(point, target_bounds.lry);
    run.grid_x = point.x;

    cell_count += run.lry - run.uly;

    Vfunc3(point.x, run.uly, run.lry);

    runs.Append(&run);

    while (runs.GetCount()) {
        runs_count = runs.GetCount();

        if (runs_count > max_runs) {
            max_runs = runs_count;
        }

        run = *runs[runs_count - 1];

        runs.Remove(runs_count - 1);

        if (run.uly < bounds.uly) {
            bounds.uly = run.uly;
        }

        if (run.grid_x < bounds.ulx) {
            bounds.ulx = run.grid_x;
        }

        if (run.grid_x >= bounds.lrx) {
            bounds.lrx = run.grid_x;
        }

        if (run.lry > bounds.lry) {
            bounds.lry = run.lry;
        }

        if (mode) {
            run.uly = std::max(target_bounds.uly, run.uly - 1);
            run.lry = std::min(target_bounds.lry, run.lry + 1);
        }

        if (run.grid_x > target_bounds.ulx) {
            point.x = run.grid_x - 1;
            point.y = run.uly;

            while (point.y < run.lry) {
                point.y = Vfunc2(point, run.lry);

                if (point.y < run.lry) {
                    FloodRun inner_run;

                    inner_run.grid_x = point.x;

                    inner_run.uly = Vfunc0(point, target_bounds.uly);
                    inner_run.lry = Vfunc1(point, target_bounds.lry);
                    point.y = inner_run.lry;

                    cell_count += inner_run.lry - inner_run.uly;

                    Vfunc3(inner_run.grid_x, inner_run.uly, inner_run.lry);

                    runs.Append(&inner_run);
                }
            }
        }

        if (run.grid_x < target_bounds.lrx - 1) {
            point.x = run.grid_x + 1;
            point.y = run.uly;

            while (point.y < run.lry) {
                point.y = Vfunc2(point, run.lry);

                if (point.y < run.lry) {
                    FloodRun inner_run;

                    inner_run.grid_x = point.x;

                    inner_run.uly = Vfunc0(point, target_bounds.uly);
                    inner_run.lry = Vfunc1(point, target_bounds.lry);
                    point.y = inner_run.lry;

                    cell_count += inner_run.lry - inner_run.uly;

                    Vfunc3(inner_run.grid_x, inner_run.uly, inner_run.lry);

                    runs.Append(&inner_run);
                }
            }
        }
    }

    ++bounds.lrx;

    int elapsed_time = timer_elapsed_time(time_stamp);

    if (elapsed_time > 10 || max_runs > 10) {
        AiLog log("Flood fill, %i msecs, %i max depth.", elapsed_time, max_runs);
    }

    return cell_count;
}
