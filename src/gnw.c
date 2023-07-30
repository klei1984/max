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

#include "gnw.h"

#include <SDL.h>

static void win_free(WinID id);
static void win_clip(GNW_Window *w, RectPtr *L, uint8_t *buf);
static void refresh_all(Rect *bound, uint8_t *buf);

static int32_t window_index[50];
static GNW_Window *window[50];
static int32_t buffering;
static int32_t bk_color;
static SetModeFunc set_mode_ptr;
static int32_t doing_refresh_all;
static ResetModeFunc reset_mode_ptr;
static uint8_t *screen_buffer;
static uint32_t window_flags;
static int32_t num_windows;

uint8_t *GNW_texture;
int32_t GNW_win_init_flag;
ColorRGB GNW_wcolor[6];

static inline int32_t GetRGBColor(int32_t r, int32_t g, int32_t b) {
    return ((r & 0x1F) << 10) | ((g & 0x1F) << 5) | ((b & 0x1F) << 0);
}

static inline int32_t GNW_IsRGBColor(int32_t color) { return (color & (GNW_TEXT_RGB_MASK & (~GNW_TEXT_COLOR_MASK))); }

static inline int32_t GNW_WinRGB2Color(int32_t color) {
    if (GNW_IsRGBColor(color)) {
        return (color & GNW_TEXT_CONTROL_MASK) | colorTable[GNW_wcolor[(color & GNW_TEXT_RGB_MASK) >> 8]];

    } else {
        return color;
    }
}

int32_t win_init(SetModeFunc set, ResetModeFunc reset, int32_t flags) {
    uint8_t *pal;
    int32_t result;

    if (GNW_win_init_flag) {
        return 4;
    }

    for (int32_t i = 0; i < 50; i++) {
        window_index[i] = -1;
    }

    if (Text_Init() == -1) {
        return 3;
    }

    reset_mode_ptr = reset;
    set_mode_ptr = set;

    if (set() == -1) {
        return 1;
    }

    if (flags & 1) {
        size_t size = (scr_size.lry - scr_size.uly + 1) * (scr_size.lrx - scr_size.ulx + 1);

        screen_buffer = (uint8_t *)malloc(size);

        if (!screen_buffer) {
            if (reset_mode_ptr) {
                reset_mode_ptr();
            } else {
                Svga_Deinit();
            }

            return 2;
        }
    }

    buffering = 0;
    doing_refresh_all = 0;

    if (!initColors()) {
        if (reset_mode_ptr) {
            reset_mode_ptr();
        } else {
            Svga_Deinit();
        }
        if (screen_buffer) {
            free(screen_buffer);
        }

        return 2;
    }

    GNW_input_init();
    GNW_intr_init();

    window[0] = (GNW_Window *)malloc(sizeof(GNW_Window));

    if (window[0]) {
        window[0]->buf = NULL;
        window[0]->id = 0;
        window[0]->flags = 0;
        window[0]->button_list = NULL;
        window[0]->last_over = NULL;
        window[0]->last_click = NULL;
        window[0]->menu = NULL;
        window[0]->tx = 0;
        window[0]->ty = 0;
        window[0]->w.ulx = scr_size.ulx;
        window[0]->w.uly = scr_size.uly;
        window[0]->w.lrx = scr_size.lrx;
        window[0]->w.lry = scr_size.lry;
        window[0]->width = scr_size.lrx - scr_size.ulx + 1;
        window[0]->length = scr_size.lry - scr_size.uly + 1;

        window_index[0] = 0;
        num_windows = 1;

        GNW_wcolor[0] = GetRGBColor(10, 10, 10);
        GNW_wcolor[1] = GetRGBColor(15, 15, 15);
        GNW_wcolor[2] = GetRGBColor(8, 8, 8);
        GNW_wcolor[3] = GetRGBColor(20, 20, 20);
        GNW_wcolor[4] = GetRGBColor(31, 31, 11);
        GNW_wcolor[5] = GetRGBColor(31, 0, 0);

        bk_color = 0;

        GNW_texture = 0;
        window_flags = flags;

        atexit(win_exit);

        GNW_win_init_flag = 1;

        result = 0;
    } else {
        if (reset_mode_ptr)
            reset_mode_ptr();
        else
            Svga_Deinit();
        if (screen_buffer) {
            free(screen_buffer);
        }

        result = 2;
    }

    return result;
}

int32_t win_reinit(SetModeFunc set) {
    int32_t result;

    result = 0;
    if (GNW_win_init_flag) {
        if (set() == -1) {
            result = 1;
        } else {
            for (int32_t i = 1; i < num_windows; i++) {
                if ((scr_size.lrx - scr_size.ulx + 1) < window[i]->width ||
                    (scr_size.lry - scr_size.uly + 1) < window[i]->length) {
                    result = 6;
                    break;
                }
            }

            if (!result && (window_flags & 1)) {
                size_t size = (scr_size.lry - scr_size.uly + 1) * (scr_size.lrx - scr_size.ulx + 1);
                uint8_t *buffer = (uint8_t *)malloc(size);

                if (buffer) {
                    free(screen_buffer);
                    screen_buffer = buffer;
                } else {
                    result = 2;
                }
            }

            if (result) {
                set_mode_ptr();
            } else {
                set_mode_ptr = set;
            }

            setSystemPalette(getSystemPalette());

            window[0]->w = scr_size;
            window[0]->width = scr_size.lrx - scr_size.ulx + 1;
            window[0]->length = scr_size.lry - scr_size.uly + 1;

            {
                GNW_Window *win;

                win = GNW_find(0);
                if (GNW_win_init_flag && win) {
                    GNW_win_refresh(win, &win->w, NULL);
                }
            }

            for (int32_t i = 1; i < num_windows; i++) {
                win_move(window[i]->id, window[i]->w.ulx, window[i]->w.uly);
            }
        }
    } else {
        result = 5;
    }

    return result;
}

int32_t win_active(void) { return GNW_win_init_flag; }

void win_exit(void) {
    static int32_t insideWinExit = 0;

    if (!insideWinExit) {
        insideWinExit = 1;

        if (GNW_win_init_flag) {
            GNW_intr_exit();

            for (int32_t i = num_windows - 1; i >= 0; i--) {
                win_free(window[i]->id);
            }

            if (GNW_texture) {
                free(GNW_texture);
            }

            if (screen_buffer) {
                free(screen_buffer);
            }

            if (reset_mode_ptr) {
                reset_mode_ptr();
            }

            GNW_input_exit();
            GNW_rect_exit();
            Text_Exit();
            colorsClose();

            GNW_win_init_flag = 0;
        }

        insideWinExit = 0;
    }
}

WinID win_add(int32_t ulx, int32_t uly, int32_t width, int32_t length, int32_t color, int32_t flags) {
    WinID result;
    int32_t i;
    int32_t j;
    WinID id;
    GNW_Window *w;

    if (GNW_win_init_flag && (num_windows != 50) && ((scr_size.lrx - scr_size.ulx + 1) >= width) &&
        ((scr_size.lry - scr_size.uly + 1) >= length)) {
        w = (GNW_Window *)malloc(sizeof(GNW_Window));

        if (w) {
            window[num_windows] = w;

            w->buf = (uint8_t *)malloc(length * width);

            if (w->buf) {
                for (id = 1; GNW_find(id); id++) {
                    ;
                }

                w->id = id;

                if (flags & 1) {
                    flags |= window_flags;
                }

                w->flags = flags;
                w->width = width;
                w->length = length;
                w->tx = dos_rand() & 0xFFFE;
                w->ty = dos_rand() & 0xFFFE;

                if (color == 0x100) {
                    if (!GNW_texture) {
                        color = colorTable[GNW_wcolor[0]];
                    }
                } else {
                    color = GNW_WinRGB2Color(color);
                }

                w->color = color;
                w->button_list = NULL;
                w->last_over = NULL;
                w->last_click = NULL;
                w->menu = NULL;
                w->trans_b2b = trans_buf_to_buf;

                window_index[id] = num_windows++;

                win_fill(id, 0, 0, width, length, color);

                w->flags |= 0x08;

                win_move(id, ulx, uly);

                w->flags = flags;

                if (!(flags & 0x04)) {
                    for (i = num_windows - 2; (i > 0) && (window[i]->flags & 4); i--) {
                        ;
                    }

                    if (num_windows - 2 != i) {
                        i++;
                        for (j = num_windows - 1; j > i; j--) {
                            window[j] = window[j - 1];
                            window_index[window[j]->id] = j;
                        }

                        window[i] = w;
                        window_index[id] = i;
                    }
                }

                result = id;
            } else {
                free(w);
                result = -1;
            }
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

void win_delete(WinID id) {
    Rect r;
    GNW_Window *w;
    int32_t win_num;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        r = w->w;

        win_free(id);
        win_num = window_index[id];
        window_index[id] = -1;
        num_windows--;

        for (int32_t i = win_num; i < num_windows; i++) {
            window[i] = window[i + 1];
            window_index[window[i]->id] = i;
        }

        win_refresh_all(&r);
    }
}

void win_free(WinID id) {
    GNW_Window *w;
    GNW_ButtonPtr bp;
    GNW_ButtonPtr np;

    w = GNW_find(id);

    if (w) {
        if (w->buf) free(w->buf);
        if (w->menu) free(w->menu);

        for (bp = w->button_list; bp; bp = np) {
            np = bp->next;
            GNW_delete_button(bp);
        }

        free(w);
    }
}

void win_buffering(int32_t state) {
    if (screen_buffer) {
        buffering = state;
    }
}

void win_border(WinID id) {
    if (GNW_win_init_flag) {
        GNW_Window *w;

        w = GNW_find(id);

        if (w) {
            lighten_buf(w->buf + 5, w->width - 10, 5, w->width);
            lighten_buf(w->buf, 5, w->length, w->width);
            lighten_buf(&w->buf[w->width - 5], 5, w->length, w->width);
            lighten_buf(&w->buf[w->width * (w->length - 5) + 5], w->width - 10, 5, w->width);

            draw_box(w->buf, w->width, 0, 0, w->width - 1, w->length - 1, colorTable[0]);

            draw_shaded_box(w->buf, w->width, 1, 1, w->width - 2, w->length - 2, colorTable[GNW_wcolor[1]],
                            colorTable[GNW_wcolor[2]]);

            draw_shaded_box(w->buf, w->width, 5, 5, w->width - 6, w->length - 6, colorTable[GNW_wcolor[2]],
                            colorTable[GNW_wcolor[1]]);
        }
    }
}

void win_set_bk_color(int32_t color) {
    GNW_Window *w;

    if (GNW_win_init_flag) {
        bk_color = color;

        w = GNW_find(0);

        if (GNW_win_init_flag) {
            if (w) {
                GNW_win_refresh(w, &w->w, NULL);
            }
        }
    }
}

void win_print(WinID id, char *str, int32_t field_width, int32_t x, int32_t y, int32_t color) {
    GNW_Window *w;
    uint8_t *buf;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        if (!field_width) {
            if (color & GNW_TEXT_MONOSPACE) {
                field_width = Text_GetMonospaceWidth(str);
            } else {
                field_width = Text_GetWidth(str);
            }
        }

        if (field_width + x > w->width) {
            if (!(color & GNW_TEXT_ALLOW_TRUNCATED)) {
                return;
            }

            field_width = w->width - x;
        }

        buf = &w->buf[x + w->width * y];

        if ((Text_GetHeight() + y) <= w->length) {
            if (!(color & GNW_TEXT_FILL_WINDOW)) {
                if (w->color == 0x100 && GNW_texture) {
                    buf_texture(buf, field_width, Text_GetHeight(), w->width, GNW_texture, w->tx + x, w->ty + y);
                } else {
                    buf_fill(buf, field_width, Text_GetHeight(), w->width, w->color);
                }
            }

            color = GNW_WinRGB2Color(color);

            Text_Blit(buf, str, field_width, w->width, color);

            if (color & GNW_TEXT_REFRESH_WINDOW) {
                Rect r;

                r.ulx = w->w.ulx + x;
                r.uly = w->w.uly + y;
                r.lrx = field_width + r.ulx;
                r.lry = Text_GetHeight() + r.uly;

                GNW_win_refresh(w, &r, 0);
            }
        }
    }
}

void win_text(WinID id, char **list, int32_t num, int32_t field_width, int32_t x, int32_t y, int32_t color) {
    int32_t i;
    int32_t height;
    int32_t full;
    uint8_t *buf;
    GNW_Window *w;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        full = w->width;
        buf = &w->buf[x + full * y];
        height = Text_GetHeight();
        i = 0;

        while (i < num) {
            if (*list[i]) {
                win_print(id, list[i], field_width, x, y, color);
            } else if (field_width) {
                draw_line(buf, full, 0, height / 2, field_width - 1, height / 2, colorTable[GNW_wcolor[2]]);
                draw_line(buf, full, 0, height / 2 + 1, field_width - 1, height / 2 + 1, colorTable[GNW_wcolor[1]]);
            }

            i++;

            buf += full * height;
            y += height;
        }
    }
}

void win_line(WinID id, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color) {
    GNW_Window *w;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        color = GNW_WinRGB2Color(color);

        draw_line(w->buf, w->width, x1, y1, x2, y2, color);
    }
}

void win_box(WinID id, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, int32_t color) {
    GNW_Window *w;
    int32_t v7;
    int32_t ulya;
    int32_t lrxa;
    int32_t lrya;

    ulya = uly;
    lrxa = lrx;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        color = GNW_WinRGB2Color(color);

        if (lrx < ulx) {
            v7 = ulx ^ lrx;
            ulx ^= v7;
            lrxa = ulx ^ v7;
        }

        if (lry < uly) {
            lrya = uly ^ lry;
            ulya = lrya ^ uly;
            lry = lrya ^ uly ^ lrya;
        }

        draw_box(w->buf, w->width, ulx, ulya, lrxa, lry, color);
    }
}

void win_shaded_box(WinID id, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, int32_t color1, int32_t color2) {
    GNW_Window *w;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        color1 = GNW_WinRGB2Color(color1);
        color2 = GNW_WinRGB2Color(color2);

        draw_shaded_box(w->buf, w->width, ulx, uly, lrx, lry, color1, color2);
    }
}

void win_fill(WinID id, int32_t ulx, int32_t uly, int32_t width, int32_t length, int32_t color) {
    GNW_Window *w;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        if (color == 0x100) {
            if (GNW_texture) {
                buf_texture(&w->buf[w->width * uly + ulx], width, length, w->width, GNW_texture, ulx + w->tx,
                            uly + w->ty);
            } else {
                color = colorTable[GNW_wcolor[0]];
            }
        } else {
            color = GNW_WinRGB2Color(color);
        }

        if (color < 0x100) {
            buf_fill(&w->buf[w->width * uly + ulx], width, length, w->width, color);
        }
    }
}

void win_show(WinID id) {
    GNW_Window *w;
    int32_t i;

    w = GNW_find(id);

    i = window_index[w->id];

    if (GNW_win_init_flag && w) {
        if (w->flags & 0x08) {
            w->flags &= ~0x08;

            if ((num_windows - 1) == i) {
                GNW_win_refresh(w, &w->w, NULL);
            }
        }

        if (num_windows - 1 > i && !(w->flags & 2)) {
            while ((num_windows - 1) > i && ((w->flags & 4) || !(window[i + 1]->flags & 4))) {
                window[i] = window[i + 1];
                window_index[window[i]->id] = i;
                i++;
            }

            window[i] = w;
            window_index[w->id] = i;

            GNW_win_refresh(w, &w->w, NULL);
        }
    }
}

void win_hide(WinID id) {
    GNW_Window *w;

    w = GNW_find(id);

    if (GNW_win_init_flag && w && !(w->flags & 8)) {
        w->flags |= 8u;
        win_refresh_all(&w->w);
    }
}

void win_move(WinID id, int32_t ulx, int32_t uly) {
    Rect r;
    GNW_Window *w;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        r = w->w;

        if (ulx < 0) {
            ulx = 0;
        }

        if (uly < 0) {
            uly = 0;
        }

        if (w->flags & 0x01) {
            ulx += 2;
        }

        if ((w->width + ulx - 1) > scr_size.lrx) {
            ulx = scr_size.lrx - w->width + 1;
        }

        if ((w->length + uly - 1) > scr_size.lry) {
            uly = scr_size.lry - w->length + 1;
        }

        if (w->flags & 0x01) {
            ulx &= ~0x03;
        }

        w->w.ulx = ulx;
        w->w.uly = uly;
        w->w.lrx = w->width + ulx - 1;
        w->w.lry = w->length + uly - 1;

        if (!(w->flags & 0x08)) {
            GNW_win_refresh(w, &w->w, NULL);
            win_refresh_all(&r);
        }
    }
}

void win_draw(WinID id) {
    if (GNW_win_init_flag) {
        GNW_Window *w;

        w = GNW_find(id);

        if (w) {
            GNW_win_refresh(w, &w->w, NULL);
        }
    }
}

void win_draw_rect(WinID id, Rect *bound) {
    Rect r;
    GNW_Window *w;

    w = GNW_find(id);

    if (GNW_win_init_flag) {
        if (w) {
            r.ulx = w->w.ulx + bound->ulx;
            r.uly = w->w.uly + bound->uly;
            r.lrx = w->w.ulx + bound->lrx;
            r.lry = w->w.uly + bound->lry;

            GNW_win_refresh(w, &r, NULL);
        }
    }
}

void GNW_win_refresh(GNW_Window *w, Rect *bound, uint8_t *scr_buf) {
    Rect m;
    uint8_t *buf;
    Rect *r;
    RectPtr L;
    RectPtr N;
    RectPtr rp;
    uint32_t srcW;
    uint32_t srcH;
    int32_t full2;

    if (!(w->flags & 8)) {
        if ((w->flags & 0x20) && buffering && !doing_refresh_all) {
            refresh_all(bound, screen_buffer);

            srcW = bound->lrx - bound->ulx + 1;
            srcH = bound->lry - bound->uly + 1;

            if (!mouse_hidden() && mouse_in(bound->ulx, bound->uly, bound->lrx, bound->lry)) {
                mouse_show();
                mouse_get_rect(&m);

                for (L = rect_clip(bound, &m); L; L = N) {
                    N = L->next;

                    GNW_button_refresh(w, (Rect *)L);

                    scr_blit(&screen_buffer[L->r.ulx - bound->ulx + (L->r.uly - bound->uly) * srcW], srcW,
                             L->r.lrx - L->r.uly + 1, 0, 0, L->r.lrx - L->r.ulx + 1, L->r.lry - L->r.uly + 1, L->r.ulx,
                             L->r.uly);
                    rect_free(L);
                }
            } else {
                GNW_button_refresh(w, bound);
                scr_blit(screen_buffer, srcW, srcH, 0, 0, srcW, srcH, bound->ulx, bound->uly);
            }
        } else {
            rp = rect_malloc();

            if (rp) {
                rp->next = 0;
                r = &w->w;

                if (w->w.ulx >= bound->ulx) {
                    rp->r.ulx = r->ulx;
                } else {
                    rp->r.ulx = bound->ulx;
                }

                if (r->uly >= bound->uly) {
                    rp->r.uly = r->uly;
                } else {
                    rp->r.uly = bound->uly;
                }

                if (r->lrx <= bound->lrx) {
                    rp->r.lrx = r->lrx;
                } else {
                    rp->r.lrx = bound->lrx;
                }

                if (r->lry <= bound->lry) {
                    rp->r.lry = r->lry;
                } else {
                    rp->r.lry = bound->lry;
                }

                if (rp->r.ulx <= rp->r.lrx && rp->r.uly <= rp->r.lry) {
                    if (scr_buf) {
                        full2 = bound->lrx - bound->ulx + 1;
                    }

                    win_clip(w, &rp, scr_buf);

                    if (w->id) {
                        for (L = rp; L; L = N) {
                            N = L->next;

                            GNW_button_refresh(w, (Rect *)L);

                            if (scr_buf) {
                                if (buffering && (w->flags & 0x20)) {
                                    w->trans_b2b(&w->buf[L->r.ulx - w->w.ulx + (L->r.uly - w->w.uly) * w->width],
                                                 L->r.lrx - L->r.ulx + 1, L->r.lry - L->r.uly + 1, w->width,
                                                 &scr_buf[full2 * (L->r.uly - bound->uly) + L->r.ulx - bound->ulx],
                                                 full2);
                                } else {
                                    buf_to_buf(
                                        &w->buf[L->r.ulx - w->w.ulx + (L->r.uly - w->w.uly) * w->width],
                                        L->r.lrx - L->r.ulx + 1, L->r.lry - L->r.uly + 1, w->width,
                                        (uint8_t *)&scr_buf[full2 * (L->r.uly - bound->uly) + L->r.ulx - bound->ulx],
                                        full2);
                                }
                            } else if (buffering) {
                                if (w->flags & 0x20) {
                                    w->trans_b2b(
                                        &w->buf[L->r.ulx - w->w.ulx + (L->r.uly - w->w.uly) * w->width],
                                        L->r.lrx - L->r.ulx + 1, L->r.lry - L->r.uly + 1, w->width,
                                        &screen_buffer[L->r.uly * (scr_size.lrx - scr_size.ulx + 1) + L->r.ulx],
                                        scr_size.lrx - scr_size.ulx + 1);
                                } else {
                                    buf_to_buf(&w->buf[L->r.ulx - w->w.ulx + (L->r.uly - w->w.uly) * w->width],
                                               L->r.lrx - L->r.ulx + 1, L->r.lry - L->r.uly + 1, w->width,
                                               &screen_buffer[L->r.uly * (scr_size.lrx - scr_size.ulx + 1) + L->r.ulx],
                                               scr_size.lrx - scr_size.ulx + 1);
                                }
                            } else {
                                scr_blit(&w->buf[L->r.ulx - w->w.ulx + (L->r.uly - w->w.uly) * w->width], w->width,
                                         L->r.lry - L->r.uly + 1, 0, 0, L->r.lrx - L->r.ulx + 1,
                                         L->r.lry - L->r.uly + 1, L->r.ulx, L->r.uly);
                            }
                        }
                    } else {
                        for (L = rp; L; L = N) {
                            N = L->next;

                            srcW = L->r.lrx - L->r.ulx + 1;
                            srcH = L->r.lry - L->r.uly + 1;

                            buf = (uint8_t *)malloc(srcH * srcW);

                            if (buf) {
                                buf_fill(buf, srcW, srcH, srcW, bk_color);

                                if (scr_buf) {
                                    buf_to_buf(
                                        buf, srcW, srcH, srcW,
                                        (uint8_t *)&scr_buf[full2 * (L->r.uly - bound->uly) + L->r.ulx - bound->ulx],
                                        full2);
                                } else if (buffering) {
                                    buf_to_buf(buf, srcW, srcH, srcW,
                                               &screen_buffer[L->r.uly * (scr_size.lrx - scr_size.ulx + 1) + L->r.ulx],
                                               scr_size.lrx - scr_size.ulx + 1);
                                } else {
                                    scr_blit(buf, srcW, srcH, 0, 0, srcW, srcH, L->r.ulx, L->r.uly);
                                }

                                free(buf);
                            }
                        }
                    }

                    for (L = rp; L; L = N) {
                        N = L->next;

                        if (buffering && !scr_buf) {
                            scr_blit(&screen_buffer[L->r.ulx + (scr_size.lrx - scr_size.ulx + 1) * L->r.uly],
                                     scr_size.lrx - scr_size.ulx + 1, L->r.lry - L->r.uly + 1, 0, 0,
                                     L->r.lrx - L->r.ulx + 1, L->r.lry - L->r.uly + 1, L->r.ulx, L->r.uly);
                        }

                        rect_free(L);
                    }

                    if (!doing_refresh_all && !scr_buf && !mouse_hidden() &&
                        mouse_in(bound->ulx, bound->uly, bound->lrx, bound->lry)) {
                        mouse_show();
                    }
                } else {
                    rect_free(rp);
                }
            }
        }
    }
}

void win_refresh_all(Rect *bound) {
    if (GNW_win_init_flag) {
        refresh_all(bound, NULL);
    }
}

void win_clip(GNW_Window *w, RectPtr *L, uint8_t *buf) {
    for (int32_t i = window_index[w->id] + 1; (i < num_windows) && *L; i++) {
        if (window[i]->flags & 8) {
            continue;
        }

        if (buffering && (window[i]->flags & 0x20)) {
            if (doing_refresh_all) {
                continue;
            }

            GNW_win_refresh(window[i], &window[i]->w, NULL);
        }

        rect_clip_list(L, &window[i]->w);
    }

    if ((buf == screen_buffer || !buf) && !mouse_hidden()) {
        Rect m;

        mouse_get_rect(&m);
        rect_clip_list(L, &m);
    }
}

void win_drag(WinID id) {
    Rect r;
    int32_t x;
    int32_t y;
    int32_t dx;
    int32_t dy;
    RectPtr L;
    RectPtr N;
    GNW_Window *w;

    dx = 0;
    dy = 0;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        win_show(id);

        r = w->w;

        GNW_do_bk_process();

        mouse_info();

        while (!((mouse_get_buttons() & 0x30))) {
            if (dx || dy) {
                w->w.ulx += dx;
                w->w.uly += dy;
                w->w.lrx += dx;
                w->w.lry += dy;

                GNW_win_refresh(w, &w->w, NULL);

                for (L = rect_clip(&r, &w->w); L; L = N) {
                    N = L->next;

                    win_refresh_all((Rect *)L);
                    rect_free(L);
                }

                r = w->w;
            }

            mouse_get_position(&x, &y);

            GNW_do_bk_process();

            mouse_info();

            dx = x;
            dy = y;

            mouse_get_position(&x, &y);

            dx = x - dx;
            dy = y - dy;

            if ((w->w.ulx + dx) < scr_size.ulx) {
                dx = scr_size.ulx - w->w.ulx;
            }

            if ((w->w.lrx + dx) > scr_size.lrx) {
                dx = scr_size.lrx - w->w.lrx;
            }

            if ((w->w.uly + dy) < scr_size.uly) {
                dy = scr_size.uly - w->w.uly;
            }

            if ((w->w.lry + dy) > scr_size.lry) {
                dy = scr_size.lry - w->w.lry;
            }
        }

        if ((w->flags & 0x100) && (w->w.ulx & 3)) {
            win_move(w->id, w->w.ulx, w->w.uly);
        }
    }
}

void win_get_mouse_buf(uint8_t *buf) {
    Rect m;

    mouse_get_rect(&m);
    refresh_all(&m, buf);
}

void refresh_all(Rect *bound, uint8_t *buf) {
    doing_refresh_all = 1;

    for (int32_t i = 0; i < num_windows; i++) {
        GNW_win_refresh(window[i], bound, buf);
    }

    doing_refresh_all = 0;

    if (!buf && !mouse_hidden()) {
        if (mouse_in(bound->ulx, bound->uly, bound->lrx, bound->lry)) {
            mouse_show();
        }
    }
}

GNW_Window *GNW_find(WinID id) {
    GNW_Window *w;

    if (id == -1) {
        w = NULL;
    } else {
        if (window_index[id] == -1) {
            w = NULL;
        } else {
            w = window[window_index[id]];
        }
    }

    return w;
}

uint8_t *win_get_buf(WinID id) {
    uint8_t *buf;
    GNW_Window *w;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        buf = w->buf;
    } else {
        buf = NULL;
    }

    return buf;
}

WinID win_get_top_win(int32_t x, int32_t y) {
    int32_t i;

    for (i = num_windows - 1; i >= 1; i--) {
        if ((x >= window[i]->w.ulx) && (x <= window[i]->w.lrx) && (y >= window[i]->w.uly) && (y <= window[i]->w.lry)) {
            break;
        }
    }

    return window[i]->id;
}

int32_t win_width(WinID id) {
    GNW_Window *w;
    int32_t result;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        result = w->width;
    } else {
        result = -1;
    }

    return result;
}

int32_t win_height(WinID id) {
    GNW_Window *w;
    int32_t result;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        result = w->length;
    } else {
        result = -1;
    }

    return result;
}

int32_t win_get_rect(WinID id, Rect *r) {
    GNW_Window *w;
    int32_t result;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        *r = w->w;
        result = 0;
    } else {
        result = -1;
    }

    return result;
}

int32_t win_check_all_buttons(void) {
    int32_t result;
    intptr_t press;

    press = -1;

    if (GNW_win_init_flag) {
        for (int32_t i = num_windows - 1; i >= 1; i--) {
            if (!GNW_check_buttons(window[i], &press)) {
                break;
            }

            if (window[i]->flags & 0x10) {
                break;
            }
        }

        result = press;
    } else {
        result = -1;
    }

    return result;
}

GNW_ButtonPtr GNW_find_button(ButtonID id, GNW_Window **w) {
    GNW_ButtonPtr b;

    for (int32_t i = 0; i < num_windows; i++) {
        for (b = window[i]->button_list; b; b = b->next) {
            if (b->id == id) {
                if (w) {
                    *w = window[i];
                }

                return b;
            }
        }
    }

    return NULL;
}

int32_t GNW_check_menu_bars(int32_t input) {
    int32_t result;

    if (GNW_win_init_flag) {
        for (int32_t i = num_windows - 1; i >= 1; i--) {
            if (window[i]->menu) {
                for (int32_t j = 0; j < window[i]->menu->num_pds; j++) {
                    if (window[i]->menu->pd[j].value == input) {
                        input = GNW_process_menu(window[i]->menu, j);
                        break;
                    }
                }
            }

            if (window[i]->flags & 0x10) {
                break;
            }
        }

        result = input;
    } else {
        result = -1;
    }

    return result;
}

void win_set_trans_b2b(WinID id, Trans_b2b trans_b2b) {
    GNW_Window *w;

    w = GNW_find(id);

    if (GNW_win_init_flag && w && (w->flags & 0x20)) {
        if (trans_b2b) {
            w->trans_b2b = trans_b2b;
        } else {
            w->trans_b2b = trans_buf_to_buf;
        }
    }
}
