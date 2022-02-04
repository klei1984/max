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

#include <stdlib.h>
#include <string.h>

#include "gnw.h"

void draw_line(unsigned char* buffer, int width, int x1, int y1, int x2, int y2, int color) {
    int delta_x;
    int delta_y;
    int offset_x;
    int offset_y;
    int accum;

    delta_x = 2 * abs(x2 - x1);

    if ((x2 - x1) >= 0) {
        offset_x = 1;
    } else {
        offset_x = -1;
    }

    delta_y = 2 * abs(y2 - y1);

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

void draw_box(unsigned char* buf, int full, int ulx, int uly, int lrx, int lry, int color) {
    draw_line(buf, full, ulx, uly, lrx, uly, color);
    draw_line(buf, full, ulx, lry, lrx, lry, color);
    draw_line(buf, full, ulx, uly, ulx, lry, color);
    draw_line(buf, full, lrx, uly, lrx, lry, color);
}

void draw_shaded_box(unsigned char* buf, int full, int ulx, int uly, int lrx, int lry, int color1, int color2) {
    draw_line(buf, full, ulx, uly, lrx, uly, color1);
    draw_line(buf, full, ulx, lry, lrx, lry, color2);
    draw_line(buf, full, ulx, uly, ulx, lry, color1);
    draw_line(buf, full, lrx, uly, lrx, lry, color2);
}

void draw_circle(unsigned char* buf, int full, int x, int y, int r, int color) {
    int x1;
    int y1;
    int d1;

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

void cscale(unsigned char* src, int ow, int ol, int full, unsigned char* dst, int nw, int nl, int full2) {
    char* src_addr;
    char* dst_addr;

    unsigned int sx_preshift;
    unsigned int ex_preshift;

    unsigned char pixel;

    int destx;
    int desty;

    int srcx;
    int srcy;

    int my;
    int mx;

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

void trans_cscale(unsigned char* src, int ow, int ol, int full, unsigned char* dst, int nw, int nl, int full2) {
    char* src_addr;
    char* dst_addr;

    unsigned int sx_preshift;
    unsigned int ex_preshift;

    unsigned char pixel;

    int destx;
    int desty;

    int srcx;
    int srcy;

    int my;
    int mx;

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

void buf_to_buf(unsigned char* src, int width, int length, int full, unsigned char* dst, int full2) {
    for (int i = 0; i < length; i++) {
        memcpy(dst, src, width);

        src += full;
        dst += full2;
    }
}

void trans_buf_to_buf(unsigned char* src, int width, int length, int full, unsigned char* dst, int full2) {
    for (int i = 0; i < length; i++) {
        for (int j = 0; j < width; j++) {
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

void mask_buf_to_buf(unsigned char* src, int width, int length, int full, unsigned char* msk, int full2,
                     unsigned char* dst, int full3) {
    for (int i = 0; i < length; i++) {
        for (int j = 0; j < width; j++) {
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

void mask_trans_buf_to_buf(unsigned char* src, int width, int length, int full, unsigned char* msk, int full2,
                           unsigned char* dst, int full3) {
    for (int i = 0; i < length; i++) {
        for (int j = 0; j < width; j++) {
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

void buf_fill(unsigned char* buf, int width, int length, int full, int color) {
    color &= 0xFFFF00FF;
    color |= (color & 0xFF) << 8;
    color = color | (color << 16);

    for (int i = 0; i < length; i++) {
        memset(buf, color, width);
        buf += full;
    }
}

void buf_texture(unsigned char* buf, int width, int length, int full, unsigned char* texture, int tx, int ty) {
    unsigned int n;
    int x;
    int y;
    int oldtx;
    char* tex;
    short ysize;
    short xsize;

    xsize = ((short*)texture)[0];
    texture += sizeof(short);

    ysize = ((short*)texture)[0];
    texture += sizeof(short);

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

void lighten_buf(unsigned char* buf, int width, int length, int full) {
    for (unsigned int y = 0; y < length; y++) {
        for (unsigned int x = 0; x < width; x++) {
            *buf = intensityColorTable[*buf][147];
            buf++;
        }

        buf += full - width;
    }
}

void swap_color_buf(unsigned char* buf, int width, int length, int full, int c1, int c2) {
    for (unsigned int y = 0; y < length; y++) {
        for (unsigned int x = 0; x < width; x++) {
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
