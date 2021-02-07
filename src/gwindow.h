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

#ifndef GWINDOW_H
#define GWINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "interface.h"

typedef struct {
    unsigned short field_0;
    unsigned short field_2;
    unsigned short width;
    unsigned short height;
    unsigned char palette[3 * PALETTE_SIZE];
    unsigned char data[];
} ImageHeader;

typedef struct {
    unsigned short field_0;
    unsigned short field_2;
    unsigned short width;
    unsigned short height;
    unsigned char data[];
} ImageHeader2;

unsigned char gwin_init(void);
Window *gwin_get_window(unsigned char id);
void gwin_clear_window(void);
void gwin_fade_in(int steps);
void gwin_fade_out(int steps);
void gwin_load_palette_from_resource(GAME_RESOURCE id);
void gwin_decode_image(ImageHeader *image, unsigned char *buffer, int width, int height, int ulx);
int gwin_load_image(GAME_RESOURCE id, Window *wid, short offx, short palette_from_image, int draw_to_screen,
                    int width_from_image, int height_from_image);
void gwin_decode_image2(ImageHeader2 *image, int ulx, int uly, int has_transparency, Window *w);
void gwin_load_image2(GAME_RESOURCE id, int ulx, int uly, int has_transparency, Window *w);

#ifdef __cplusplus
}
#endif

#endif /* GWINDOW_H */
