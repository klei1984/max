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

#include "image.hpp"

#include <SDL.h>

#include <algorithm>
#include <new>

#include "resource_manager.hpp"

Image::Image(int16_t ulx, int16_t uly, int16_t width, int16_t height)
    : ulx(ulx), uly(uly), width(width), height(height) {
    data = new (std::nothrow) uint8_t[width * height];
    allocated = true;
}

Image::Image(ResourceID id, int16_t ulx, int16_t uly) {
    struct ImageSimpleHeader* sprite;

    sprite = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(id));
    SDL_assert(sprite);

    if (ulx < 0) {
        this->ulx = sprite->ulx;
    } else {
        this->ulx = ulx;
    }

    if (uly < 0) {
        this->uly = sprite->uly;
    } else {
        this->uly = uly;
    }

    width = sprite->width;
    height = sprite->height;

    data = &sprite->transparent_color;

    allocated = false;
}

Image::~Image() {
    if (allocated) {
        delete[] data;
    }
}

uint8_t* Image::GetData() const { return data; }

int16_t Image::GetULX() const { return ulx; }

int16_t Image::GetULY() const { return uly; }

int16_t Image::GetWidth() const { return width; }

int16_t Image::GetHeight() const { return height; }

void Image::Allocate() {
    uint8_t* buffer;

    if (!allocated) {
        buffer = new (std::nothrow) uint8_t[width * height];

        memcpy(buffer, data, width * height);
        data = buffer;

        allocated = true;
    }

    SDL_assert(allocated && data);
}

Rect Image::GetBounds() const { return {ulx, uly, width + ulx, height + uly}; }

void Image::Copy(WindowInfo* w) {
    Allocate();
    buf_to_buf(&w->buffer[ulx + w->width * uly], width, height, w->width, data, width);
}

void Image::Copy(const Image& other) {
    Allocate();
    buf_to_buf(other.GetData(), std::min(width, other.width), std::min(height, other.height), other.width, data, width);
}

void Image::Blend(const Image& other) {
    uint8_t* source_buffer;
    uint8_t* destination_buffer;
    int16_t min_width;
    int16_t width_offset;
    int16_t height_offset;
    char transparent_pixel;

    Allocate();
    source_buffer = other.GetData();

    transparent_pixel = *source_buffer;
    destination_buffer = data;

    min_width = std::min(width, other.width);
    height_offset = std::min(height, other.height);

    while (--height_offset >= 0) {
        width_offset = min_width;
        while (--width_offset >= 0) {
            if (*source_buffer != transparent_pixel) {
                *destination_buffer = *source_buffer;
            }

            source_buffer++;
            destination_buffer++;
        }

        source_buffer += other.width - min_width;
        destination_buffer += width - min_width;
    }
}

void Image::Write(WindowInfo* w) const {
    buf_to_buf(data, width, height, width, &w->buffer[ulx + w->width * uly], w->width);
}

void Image::Write(WindowInfo* w, Rect* r) const {
    buf_to_buf(&data[r->ulx + r->uly * width], r->lrx - r->ulx, r->lry - r->uly, width,
               &w->buffer[ulx + r->ulx + w->width * (r->uly + uly)], w->width);
}

void Image::Write(WindowInfo* w, int32_t ulx, int32_t uly) const {
    buf_to_buf(data, width, height, width, &w->buffer[ulx + w->width * uly], w->width);
}

void Image::Draw(WinID wid) const {
    Rect r = GetBounds();
    win_draw_rect(wid, &r);
}
