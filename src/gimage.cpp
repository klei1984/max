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

#include "gimage.h"

#include <algorithm>

#include "SDL_assert.h"

struct SpriteHeader {
    unsigned short width;
    unsigned short height;
    unsigned short ulx;
    unsigned short uly;
    unsigned char data[];
};

static_assert(sizeof(GImage) == 13, "The structure needs to be packed.");

unsigned char *gimage_get_buffer(GImage *image) { return image->buffer; }

int gimage_get_ulx(GImage *image) { return image->ulx; }

int gimage_get_uly(GImage *image) { return image->uly; }

int gimage_get_height(GImage *image) { return image->height; }

int gimage_get_width(GImage *image) { return image->width; }

GImage *gimage_alloc_ex(GImage *image, int ulx, int uly, int width, int height) {
    image->ulx = ulx;
    image->uly = uly;
    image->width = width;
    image->height = height;
    image->buffer = new (std::nothrow) unsigned char[image->width * image->height];
    SDL_assert(image->buffer);

    image->allocated = 1;

    return image;
}

GImage *gimage_init(GImage *image, GAME_RESOURCE id, int ulx, int uly) {
    struct SpriteHeader *sprite;

    sprite = (SpriteHeader *)load_game_resource(id);
    if (!sprite) {
        SDL_assert(sprite != NULL);
    }

    if (ulx < 0) {
        image->ulx = sprite->ulx;
    } else {
        image->ulx = ulx;
    }

    if (uly < 0) {
        image->uly = sprite->uly;
    } else {
        image->uly = uly;
    }

    image->width = sprite->width;
    image->height = sprite->height;

    image->buffer = sprite->data;

    image->allocated = 0;

    return image;
}

GImage *gimage_delete(GImage *image) {
    if (image->allocated) {
        delete[] image->buffer;
    }

    return image;
}

void gimage_alloc(GImage *image) {
    unsigned char *buffer;

    if (!image->allocated) {
        buffer = new (std::nothrow) unsigned char[image->width * image->height];
        SDL_assert(buffer);

        memcpy(buffer, image->buffer, image->width * image->height);
        image->buffer = buffer;

        image->allocated = 1;
    }
}

void gimage_copy_from_window(GImage *image, WindowInfo *w) {
    gimage_alloc(image);
    buf_to_buf(&w->buffer[image->ulx + w->width * image->uly], image->width, image->height, w->width, image->buffer,
               image->width);
}

void gimage_copy_from_image(GImage *dst, GImage *src) {
    gimage_alloc(dst);
    buf_to_buf(gimage_get_buffer(src), std::min(dst->width, src->width), std::min(dst->height, src->height), src->width,
               dst->buffer, dst->width);
}

void gimage_copy_content(GImage *dst, GImage *src) {
    unsigned char *image_data;
    unsigned char *buffer;
    short min_width;
    short width_offset;
    short height_offset;
    char transparent_pixel;

    gimage_alloc(dst);
    image_data = gimage_get_buffer(src);

    transparent_pixel = *image_data;
    buffer = dst->buffer;

    min_width = std::min(dst->width, src->width);
    height_offset = std::min(dst->height, src->height);

    while (--height_offset >= 0) {
        width_offset = min_width;
        while (--width_offset >= 0) {
            if (*image_data != transparent_pixel) {
                *buffer = *image_data;
            }

            image_data++;
            buffer++;
        }

        image_data += src->width - min_width;
        buffer += dst->width - min_width;
    }
}

void gimage_copy_to_window(GImage *image, WindowInfo *w) {
    buf_to_buf(image->buffer, image->width, image->height, image->width,
               &w->buffer[image->ulx + w->width * image->uly], w->width);
}

void gimage_copy_rect_to_window(GImage *image, WindowInfo *w, Rect *r) {
    buf_to_buf(&image->buffer[r->ulx + r->uly * image->width], r->lrx - r->ulx, r->lry - r->uly, image->width,
               &w->buffer[image->ulx + r->ulx + w->width * (r->uly + image->uly)], w->width);
}

void gimage_copy_offset_to_window(GImage *image, WindowInfo *w, int ulx, int uly) {
    buf_to_buf(image->buffer, image->width, image->height, image->width, &w->buffer[ulx + w->width * uly],
               w->width);
}

void gimage_get_image_rect(GImage *image, Rect *r) {
    r->ulx = image->ulx;
    r->uly = image->uly;
    r->lrx = image->width + image->ulx;
    r->lry = image->height + image->uly;
}

void gimage_draw_rect(GImage *image, WinID wid) {
    Rect r;

    gimage_get_image_rect(image, &r);
    win_draw_rect(wid, &r);
}
