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

#include "grbuf.h"

#include <SDL3/SDL.h>
#include <string.h>

#include "gnw.h"

void draw_line(uint8_t* buffer, int32_t width, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color) {
    int32_t delta_x;
    int32_t delta_y;
    int32_t offset_x;
    int32_t offset_y;
    int32_t accum;

    delta_x = 2 * SDL_abs(x2 - x1);

    if ((x2 - x1) >= 0) {
        offset_x = 1;
    } else {
        offset_x = -1;
    }

    delta_y = 2 * SDL_abs(y2 - y1);

    if ((y2 - y1) >= 0) {
        offset_y = 1;
    } else {
        offset_y = -1;
    }

    if (delta_x <= delta_y) {
        accum = delta_x - (delta_y >> 1);

        for (;;) {
            buffer[x1 + y1 * width] = color;

            if (y1 == y2) {
                break;
            }

            if (accum >= 0) {
                accum -= delta_y;
                x1 += offset_x;
            }

            accum += delta_x;
            y1 += offset_y;
        }
    } else {
        accum = delta_y - (delta_x >> 1);

        for (;;) {
            buffer[x1 + y1 * width] = color;

            if (x1 == x2) {
                break;
            }

            if (accum >= 0) {
                accum -= delta_x;
                y1 += offset_y;
            }

            accum += delta_y;
            x1 += offset_x;
        }
    }
}

void draw_box(uint8_t* buf, int32_t full, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, int32_t color) {
    draw_line(buf, full, ulx, uly, lrx, uly, color);
    draw_line(buf, full, ulx, lry, lrx, lry, color);
    draw_line(buf, full, ulx, uly, ulx, lry, color);
    draw_line(buf, full, lrx, uly, lrx, lry, color);
}

void draw_shaded_box(uint8_t* buf, int32_t full, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, int32_t color1,
                     int32_t color2) {
    draw_line(buf, full, ulx, uly, lrx, uly, color1);
    draw_line(buf, full, ulx, lry, lrx, lry, color2);
    draw_line(buf, full, ulx, uly, ulx, lry, color1);
    draw_line(buf, full, lrx, uly, lrx, lry, color2);
}

void draw_circle(uint8_t* buf, int32_t full, int32_t x, int32_t y, int32_t r, int32_t color) {
    int32_t x1;
    int32_t y1;
    int32_t d1;

    x1 = 0;
    y1 = r;
    d1 = 2 * (1 - r);

    buf[full * (r + y) + x] = color;
    buf[full * (y - r) + x] = color;
    buf[full * (y - r) + x] = color;
    buf[full * (r + y) + x] = color;

    while (y1 >= 0) {
        if (d1 >= 0) {
            if (d1 <= 0) {
                d1 += 2 * ++x1 - 2 * --y1 + 2;
            } else if (2 * d1 - 2 * x1 - 1 <= 0) {
                d1 += 2 * ++x1 - 2 * --y1 + 2;
            } else {
                d1 += 1 - 2 * --y1;
            }
        } else if (2 * d1 + 2 * y1 - 1 <= 0) {
            d1 += 2 * ++x1 + 1;
        } else {
            d1 += 2 * ++x1 - 2 * --y1 + 2;
        }

        buf[x1 + x + full * (y1 + y)] = color;
        buf[x1 + x + full * (y - y1)] = color;
        buf[full * (y - y1) + x - x1] = color;
        buf[full * (y1 + y) + x - x1] = color;
    }
}

void cscale(uint8_t* src, int32_t ow, int32_t ol, int32_t full, uint8_t* dst, int32_t nw, int32_t nl, int32_t full2) {
    uint8_t* src_addr;
    uint8_t* dst_addr;

    uint32_t sx_preshift;
    uint32_t ex_preshift;

    uint8_t pixel;

    int32_t destx;
    int32_t desty;

    int32_t srcx;
    int32_t srcy;

    int32_t my;
    int32_t mx;

    my = (nl << 16) / ol;
    mx = (nw << 16) / ow;

    for (srcy = 0; srcy < ol; srcy++) {
        sx_preshift = 0;
        ex_preshift = (nw << 16) / ow;
        src_addr = &src[srcy * full];

        for (srcx = 0; srcx < ow; srcx++, src_addr++) {
            pixel = *src_addr;

            for (desty = my * srcy >> 16; desty < ((my + my * srcy) >> 16); desty++) {
                dst_addr = &dst[desty * full2] + (sx_preshift >> 16);

                for (destx = sx_preshift >> 16; destx < (ex_preshift >> 16); destx++) {
                    *dst_addr++ = pixel;
                }
            }

            sx_preshift += mx;
            ex_preshift += mx;
        }
    }
}

void trans_cscale(uint8_t* src, int32_t ow, int32_t ol, int32_t full, uint8_t* dst, int32_t nw, int32_t nl,
                  int32_t full2) {
    uint8_t* src_addr;
    uint8_t* dst_addr;

    uint32_t sx_preshift;
    uint32_t ex_preshift;

    uint8_t pixel;

    int32_t destx;
    int32_t desty;

    int32_t srcx;
    int32_t srcy;

    int32_t my;
    int32_t mx;

    my = (nl << 16) / ol;
    mx = (nw << 16) / ow;

    for (srcy = 0; srcy < ol; srcy++) {
        sx_preshift = 0;
        ex_preshift = (nw << 16) / ow;
        src_addr = &src[srcy * full];

        for (srcx = 0; srcx < ow; srcx++, src_addr++) {
            pixel = *src_addr;

            if (*src_addr) {
                for (desty = my * srcy >> 16; desty < ((my + my * srcy) >> 16); desty++) {
                    dst_addr = &dst[desty * full2] + (sx_preshift >> 16);

                    for (destx = sx_preshift >> 16; destx < (ex_preshift >> 16); destx++) {
                        *dst_addr++ = pixel;
                    }
                }
            }

            sx_preshift += mx;
            ex_preshift += mx;
        }
    }
}

void buf_to_buf(uint8_t* src, int32_t width, int32_t length, int32_t full, uint8_t* dst, int32_t full2) {
    for (int32_t i = 0; i < length; i++) {
        memcpy(dst, src, width);

        src += full;
        dst += full2;
    }
}

void trans_buf_to_buf(uint8_t* src, int32_t width, int32_t length, int32_t full, uint8_t* dst, int32_t full2) {
    for (int32_t i = 0; i < length; i++) {
        for (int32_t j = 0; j < width; j++) {
            if (*src) {
                *dst = *src;
            }

            src++;
            dst++;
        }

        src += full - width;
        dst += full2 - width;
    }
}

void mask_buf_to_buf(uint8_t* src, int32_t width, int32_t length, int32_t full, uint8_t* msk, int32_t full2,
                     uint8_t* dst, int32_t full3) {
    for (int32_t i = 0; i < length; i++) {
        for (int32_t j = 0; j < width; j++) {
            if (*msk) {
                if (*src) {
                    *dst = *src;
                }
            }

            src++;
            msk++;
            dst++;
        }

        src += full - width;
        msk += full2 - width;
        dst += full3 - width;
    }
}

void mask_trans_buf_to_buf(uint8_t* src, int32_t width, int32_t length, int32_t full, uint8_t* msk, int32_t full2,
                           uint8_t* dst, int32_t full3) {
    for (int32_t i = 0; i < length; i++) {
        for (int32_t j = 0; j < width; j++) {
            if (*msk) {
                *dst = *src;
            }

            src++;
            msk++;
            dst++;
        }

        src += full - width;
        msk += full2 - width;
        dst += full3 - width;
    }
}

void buf_fill(uint8_t* buf, int32_t width, int32_t length, int32_t full, int32_t color) {
    color &= 0xFFFF00FF;
    color |= (color & 0xFF) << 8;
    color = color | (color << 16);

    for (int32_t i = 0; i < length; i++) {
        memset(buf, color, width);
        buf += full;
    }
}

void buf_texture(uint8_t* buf, int32_t width, int32_t length, int32_t full, uint8_t* texture, int32_t tx, int32_t ty) {
    uint32_t n;
    int32_t x;
    int32_t y;
    int32_t oldtx;
    uint8_t* tex;
    int16_t ysize;
    int16_t xsize;

    xsize = ((int16_t*)texture)[0];
    texture += sizeof(int16_t);

    ysize = ((int16_t*)texture)[0];
    texture += sizeof(int16_t);

    tx = tx % xsize;
    oldtx = tx;
    ty = ty % ysize;
    tex = &texture[ty * xsize];

    for (y = 0; y < length; y++) {
        for (x = 0; x < width; x += n) {
            n = xsize - tx;

            if (width - x < n) {
                n = width - x;
            }

            memcpy(&buf[x], &tex[tx], n);

            tx += n;

            if (xsize == tx) {
                tx = 0;
            }
        }

        buf += full;
        tx = oldtx;

        if (ysize == ++ty) {
            ty = 0;
            tex = texture;
        } else {
            tex += xsize;
        }
    }
}

void lighten_buf(uint8_t* buf, int32_t width, int32_t length, int32_t full) {
    for (uint32_t y = 0; y < length; y++) {
        for (uint32_t x = 0; x < width; x++) {
            *buf = Color_ColorIntensity(0x12600, *buf);
            buf++;
        }

        buf += full - width;
    }
}

void swap_color_buf(uint8_t* buf, int32_t width, int32_t length, int32_t full, int32_t c1, int32_t c2) {
    for (uint32_t y = 0; y < length; y++) {
        for (uint32_t x = 0; x < width; x++) {
            if (*buf == c1) {
                *buf = c2;
            } else if (*buf == c2) {
                *buf = c1;
            }

            buf++;
        }

        buf += full - width;
    }
}
