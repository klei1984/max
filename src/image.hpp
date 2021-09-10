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

extern "C" {
#include "gnw.h"
}

class Image {
    char *data;
    short ulx;
    short uly;
    short width;
    short height;
    bool allocated;

public:
    Image(short ulx, short uly, short with, short height);
    Image(unsigned short id, short ulx, short uly);
    ~Image();

    char *GetData() const;
    short GetULX() const;
    short GetULY() const;
    short GetWidth() const;
    short GetHeight() const;

    void Allocate();
    Rect GetBounds() const;
    void Copy(Window *w);
    void Copy(const Image &other);
    void Blend(const Image &other);
    void Write(Window *w) const;
    void Write(Window *w, Rect *r) const;
    void Write(Window *w, int ulx, int uly) const;
    void Draw(WinID wid) const;
};

#endif /* IMAGE_HPP */
