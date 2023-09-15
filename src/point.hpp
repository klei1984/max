/* Copyright (c) 2021 M.A.X. Port Team
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

#ifndef POINT_HPP
#define POINT_HPP

#include <cstdint>

struct PathStep {
    int8_t x;
    int8_t y;
};

struct Point {
    int16_t x;
    int16_t y;

    Point() : x(0), y(0) {}
    Point(int32_t x, int32_t y) : x(x), y(y) {}
    Point(const Point& other) : x(other.x), y(other.y) {}

    Point& operator=(const Point& other) {
        x = other.x;
        y = other.y;

        return *this;
    }

    Point& operator+=(const Point& other) {
        x += other.x;
        y += other.y;

        return *this;
    }

    Point& operator-=(const Point& other) {
        x -= other.x;
        y -= other.y;

        return *this;
    }
};

inline bool operator==(const Point& lhs, const Point& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }

inline bool operator!=(const Point& lhs, const Point& rhs) { return !(lhs == rhs); }

inline Point operator+(Point lhs, const Point& rhs) {
    lhs += rhs;
    return lhs;
}

inline Point operator-(Point lhs, const Point& rhs) {
    lhs -= rhs;
    return lhs;
}

#endif /* POINT_HPP */
