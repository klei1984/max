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

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "resource_manager.hpp"

class Image {
    uint8_t* data;
    int16_t ulx;
    int16_t uly;
    int16_t width;
    int16_t height;
    bool allocated;

public:
    Image(int16_t ulx, int16_t uly, int16_t width, int16_t height);
    Image(ResourceID id, int16_t ulx, int16_t uly);
    ~Image();

    uint8_t* GetData() const;
    int16_t GetULX() const;
    int16_t GetULY() const;
    int16_t GetWidth() const;
    int16_t GetHeight() const;

    void Allocate();
    Rect GetBounds() const;
    void Copy(WindowInfo* w);
    void Copy(const Image& other);
    void Blend(const Image& other);
    void Write(WindowInfo* w) const;
    void Write(WindowInfo* w, Rect* r) const;
    void Write(WindowInfo* w, int32_t ulx, int32_t uly) const;
    void Draw(WinID wid) const;
};

#endif /* IMAGE_HPP */
