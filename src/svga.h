/* Copyright (c) 2020 M.A.X. Port Team
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

#ifndef SVGA_H
#define SVGA_H

#include <SDL.h>
#include <SDL_surface.h>

SDL_Surface *svga_get_screen(void);

typedef void (*ScreenBlitFunc)(unsigned char *srcBuf, unsigned int srcW, unsigned int srcH, unsigned int subX,
                               unsigned int subY, unsigned int subW, unsigned int subH, unsigned int dstX,
                               unsigned int dstY);

typedef struct Rect_s {
    int ulx;
    int uly;
    int lrx;
    int lry;
} Rect;

extern ScreenBlitFunc scr_blit;
extern Rect scr_size;

#endif /* SVGA_H */
