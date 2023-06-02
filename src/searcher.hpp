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
    unsigned short cost;
};

class Searcher {
    static int Searcher_MarkerColor;

    unsigned short **costs_map;
    unsigned char **directions_map;
    unsigned short *distance_vector;
    short line_distance_max;
    int line_distance_limit;
    ObjectArray<PathSquare> squares;
    Point destination;
    unsigned char mode;

    void EvaluateSquare(Point point, int cost, int direction, Searcher *searcher);
    void UpdateCost(Point point1, Point point2, int cost);

public:
    Searcher(Point point1, Point point2, unsigned char mode);
    ~Searcher();

    void Process(Point point, bool mode_flag);
    bool ForwardSearch(Searcher *backward_searcher);
    bool BackwardSearch(Searcher *forward_searcher);
    SmartPointer<GroundPath> DeterminePath(Point point, int max_cost);
};

#endif /* SEARCHER_HPP */
