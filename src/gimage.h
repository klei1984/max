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

#ifndef GIMAGE_H
#define GIMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gnw.h"
typedef unsigned short GAME_RESOURCE;
//#include "resrcmgr.h"

struct __attribute__((packed)) GImage {
    unsigned char *buffer;
    short ulx;
    short uly;
    short width;
    short height;
    char allocated;
};

typedef struct GImage GImage;

unsigned char *gimage_get_buffer(GImage *image);
int gimage_get_ulx(GImage *image);
int gimage_get_uly(GImage *image);
int gimage_get_height(GImage *image);
int gimage_get_width(GImage *image);
GImage *gimage_alloc_ex(GImage *image, int ulx, int uly, int width, int height);
GImage *gimage_init(GImage *image, GAME_RESOURCE resource_id, int ulx, int uly);
GImage *gimage_delete(GImage *image);
void gimage_alloc(GImage *image);
void gimage_copy_from_window(GImage *image, WindowInfo *w);
void gimage_copy_from_image(GImage *dst, GImage *src);
void gimage_copy_content(GImage *dst, GImage *src);
void gimage_copy_to_window(GImage *image, WindowInfo *w);
void gimage_copy_rect_to_window(GImage *image, WindowInfo *w, Rect *r);
void gimage_copy_offset_to_window(GImage *image, WindowInfo *w, int ulx, int uly);
void gimage_get_image_rect(GImage *image, Rect *r);
void gimage_draw_rect(GImage *image, WinID wid);

#ifdef __cplusplus
}
#endif

#endif /* GIMAGE_H */
