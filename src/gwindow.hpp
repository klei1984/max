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

#ifndef GWINDOW_HPP
#define GWINDOW_HPP

#include "gnw.h"
#include "resource_manager.hpp"

enum {
    GWINDOW_MAIN_WINDOW,
    GWINDOW_01,
    GWINDOW_02,
    GWINDOW_03,
    GWINDOW_04,
    GWINDOW_05,
    GWINDOW_06,
    GWINDOW_07,
    GWINDOW_08,
    GWINDOW_09,
    GWINDOW_10,
    GWINDOW_11,
    GWINDOW_12,
    GWINDOW_13,
    GWINDOW_14,
    GWINDOW_15,
    GWINDOW_16,
    GWINDOW_17,
    GWINDOW_18,
    GWINDOW_19,
    GWINDOW_20,
    GWINDOW_21,
    GWINDOW_22,
    GWINDOW_23,
    GWINDOW_24,
    GWINDOW_25,
    GWINDOW_26,
    GWINDOW_27,
    GWINDOW_28,
    GWINDOW_29,
    GWINDOW_30,
    GWINDOW_31,
    GWINDOW_32,
    GWINDOW_33,
    GWINDOW_34,
    GWINDOW_35,
    GWINDOW_36,
    GWINDOW_37,
    GWINDOW_38,
    GWINDOW_39,
    GWINDOW_40,
    GWINDOW_41,
    GWINDOW_42,
    GWINDOW_43,
    GWINDOW_44,
    GWINDOW_45,
    GWINDOW_46,
    GWINDOW_47,
    GWINDOW_48,
    GWINDOW_49,
    GWINDOW_50,
    GWINDOW_51,
    GWINDOW_52,
    GWINDOW_53,
    GWINDOW_54,
    GWINDOW_55,
    GWINDOW_56,
    GWINDOW_57,
    GWINDOW_58,
    GWINDOW_COUNT
};

typedef struct {
    unsigned short ulx;
    unsigned short uly;
    unsigned short width;
    unsigned short height;
    unsigned char palette[3 * PALETTE_SIZE];
    unsigned char data[];
} ImageHeader;

typedef struct {
    unsigned short ulx;
    unsigned short uly;
    unsigned short width;
    unsigned short height;
    unsigned char data[];
} ImageHeader2;

unsigned char gwin_init(void);
WindowInfo *gwin_get_window(unsigned char id);
void gwin_clear_window(void);
void gwin_fade_in(int steps);
void gwin_fade_out(int steps);
void gwin_load_palette_from_resource(ResourceID id);
void gwin_decode_image(ImageHeader *image, unsigned char *buffer, int width, int height, int ulx);
int gwin_load_image(ResourceID id, WindowInfo *wid, short offx, short palette_from_image, int draw_to_screen,
                    int width_from_image = -1, int height_from_image = -1);
void gwin_decode_image2(ImageHeader2 *image, int ulx, int uly, int has_transparency, WindowInfo *w);
void gwin_load_image2(ResourceID id, int ulx, int uly, int has_transparency, WindowInfo *w);

#endif /* GWINDOW_HPP */
