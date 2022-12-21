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

#ifndef RECT_H
#define RECT_H

typedef struct Rect_s {
    int ulx;
    int uly;
    int lrx;
    int lry;
} Rect;

typedef struct rectdata* RectPtr;

struct rectdata {
    Rect r;
    RectPtr next;
};

void GNW_rect_exit(void);
void rect_clip_list(RectPtr* pCur, Rect* bound);
RectPtr rect_clip(Rect* b, Rect* t);
RectPtr rect_malloc(void);
void rect_free(RectPtr ptr);
void rect_min_bound(Rect* r1, Rect* r2, Rect* min_bound);
int rect_inside_bound(Rect* r1, Rect* bound, Rect* r2);
Rect* rect_init(Rect* r, int ulx, int uly, int lrx, int lry);
int rect_get_width(Rect* r);
int rect_get_height(Rect* r);

#endif /* RECT_H */
