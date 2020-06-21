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

#ifndef GRBUF_H
#define GRBUF_H

// void draw_line(unsigned char* buffer, int width, int x1, int y1, int x2, int y2, int color);
// void draw_box(unsigned char* buf, int full, int ulx, int uly, int lrx, int lry, int color);
// void draw_shaded_box(unsigned char* buf, int full, int ulx, int uly, int lrx, int lry, int color1, int color2);
void draw_circle(unsigned char* buf, int full, int x, int y, int r, int color);
void cscale(unsigned char* src, int ow, int ol, int full, unsigned char* dst, int nw, int nl, int full2);
void trans_cscale(unsigned char* src, int ow, int ol, int full, unsigned char* dst, int nw, int nl, int full2);
void buf_to_buf(unsigned char* src, int width, int length, int full, unsigned char* dst, int full2);
// void trans_buf_to_buf(unsigned char* src, int width, int length, int full, unsigned char* dst, int full2);
void mask_buf_to_buf(unsigned char* src, int width, int length, int full, unsigned char* msk, int full2,
                     unsigned char* dst, int full3);
void mask_trans_buf_to_buf(unsigned char* src, int width, int length, int full, unsigned char* msk, int full2,
                           unsigned char* dst, int full3);
// void buf_fill(unsigned char* buf, int width, int length, int full, int color);
// void buf_texture(unsigned char* buf, int width, int length, int full, unsigned char* texture, int tx, int ty);
// void lighten_buf(unsigned char* buf, int width, int length, int full);
void swap_color_buf(unsigned char* buf, int width, int length, int full, int c1, int c2);

#endif /* GRBUF_H */
