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

#ifndef SEARCHER_HPP
#define SEARCHER_HPP

#include "point.hpp"
#include "smartobjectarray.hpp"

class GroundPath;

struct PathSquare {
    Point point;
    uint16_t cost;
};

class Searcher {
    static int32_t Searcher_MarkerColor;

    uint16_t **costs_map;
    uint8_t **directions_map;
    uint16_t *distance_vector;
    int16_t line_distance_max;
    int32_t line_distance_limit;
    ObjectArray<PathSquare> squares;
    Point destination;
    uint8_t mode;

    void EvaluateSquare(Point point, int32_t cost, int32_t direction, Searcher *searcher);
    void UpdateCost(Point point1, Point point2, int32_t cost);

public:
    Searcher(Point point1, Point point2, uint8_t mode);
    ~Searcher();

    void Process(Point point, bool mode_flag);
    bool ForwardSearch(Searcher *backward_searcher);
    bool BackwardSearch(Searcher *forward_searcher);
    SmartPointer<GroundPath> DeterminePath(Point point, int32_t max_cost);
};

#endif /* SEARCHER_HPP */
