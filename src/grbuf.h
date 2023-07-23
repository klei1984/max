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

#include <stdint.h>

void draw_line(uint8_t* buffer, int32_t width, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color);
void draw_box(uint8_t* buf, int32_t full, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, int32_t color);
void draw_shaded_box(uint8_t* buf, int32_t full, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, int32_t color1,
                     int32_t color2);
void draw_circle(uint8_t* buf, int32_t full, int32_t x, int32_t y, int32_t r, int32_t color);
void cscale(uint8_t* src, int32_t ow, int32_t ol, int32_t full, uint8_t* dst, int32_t nw, int32_t nl, int32_t full2);
void trans_cscale(uint8_t* src, int32_t ow, int32_t ol, int32_t full, uint8_t* dst, int32_t nw, int32_t nl,
                  int32_t full2);
void buf_to_buf(uint8_t* src, int32_t width, int32_t length, int32_t full, uint8_t* dst, int32_t full2);
void trans_buf_to_buf(uint8_t* src, int32_t width, int32_t length, int32_t full, uint8_t* dst, int32_t full2);
void mask_buf_to_buf(uint8_t* src, int32_t width, int32_t length, int32_t full, uint8_t* msk, int32_t full2,
                     uint8_t* dst, int32_t full3);
void mask_trans_buf_to_buf(uint8_t* src, int32_t width, int32_t length, int32_t full, uint8_t* msk, int32_t full2,
                           uint8_t* dst, int32_t full3);
void buf_fill(uint8_t* buf, int32_t width, int32_t length, int32_t full, int32_t color);
void buf_texture(uint8_t* buf, int32_t width, int32_t length, int32_t full, uint8_t* texture, int32_t tx, int32_t ty);
void lighten_buf(uint8_t* buf, int32_t width, int32_t length, int32_t full);
void swap_color_buf(uint8_t* buf, int32_t width, int32_t length, int32_t full, int32_t c1, int32_t c2);

#endif /* GRBUF_H */
