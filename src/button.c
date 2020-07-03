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

#include "button.h"

#include "game.h"

static GNW_ButtonPtr button_create(WinID id, int ulx, int uly, int width, int length, int on_value, int off_value,
                                   int p_value, int r_value, int flags, char* up, char* down, char* hover);
static int button_under_mouse(GNW_ButtonPtr b, Rect* r);
static int button_check_group(GNW_ButtonPtr b);
static void button_draw(GNW_ButtonPtr b, GNW_Window* w, char* image, int draw, Rect* bound);

static WinID last_button_winID = -1;
static GNW_ButtonGroup btn_grp[64];

ButtonID win_register_button(WinID id, int ulx, int uly, int width, int length, int on_value, int off_value,
                             int p_value, int r_value, char* up, char* down, char* hover, int flags) {
    ButtonID result;
    GNW_Window* w;

    w = GNW_find(id);

    flags = flags | 0x10000;

    if (GNW_win_init_flag && w) {
        if (!up && (down || hover)) {
            result = -1;
        } else {
            GNW_ButtonPtr b;

            b = button_create(id, ulx, uly, width, length, on_value, off_value, p_value, r_value, flags, up, down,
                              hover);

            if (b) {
                button_draw(b, w, b->up, 0, NULL);

                result = b->id;
            } else {
                result = -1;
            }
        }
    } else {
        result = -1;
    }

    return result;
}

ButtonID win_register_text_button(WinID id, int ulx, int uly, int on_value, int off_value, int p_value, int r_value,
                                  char* name, int flags) {
    ButtonID result;
    int width;
    int length;
    GNW_Window* w;
    char* up;
    char* down;
    GNW_ButtonPtr b;

    w = GNW_find(id);

    if (GNW_win_init_flag && w) {
        width = text_width(name) + 16;
        length = text_height() + 6;

        up = (char*)mem_malloc(length * width);

        if (up) {
            down = (char*)mem_malloc(length * width);

            if (down) {
                if (w->color == 0x100 && GNW_texture) {
                    buf_texture(up, width, length, width, GNW_texture, ulx + w->tx + 1, uly + w->ty + 1);
                    buf_texture(down, width, length, width, GNW_texture, ulx + w->tx, uly + w->ty);
                } else {
                    buf_fill(up, width, length, width, w->color);
                    buf_fill(down, width, length, width, w->color);
                }

                lighten_buf(up, width, length, width);

                text_to_buf(&up[3 * width + 8], name, width, width, colorTable[GNW_wcolor[3]] | 0x10000);

                draw_shaded_box(up, width, 2, 2, width - 3, length - 3, colorTable[GNW_wcolor[1]],
                                colorTable[GNW_wcolor[2]]);

                draw_shaded_box(up, width, 1, 1, width - 2, length - 2, colorTable[GNW_wcolor[1]],
                                colorTable[GNW_wcolor[2]]);

                draw_box(up, width, 0, 0, width - 1, length - 1, colorTable[0]);

                text_to_buf(&down[4 * width + 9], name, width, width, colorTable[GNW_wcolor[3]] | 0x10000);

                draw_shaded_box(down, width, 2, 2, width - 3, length - 3, colorTable[GNW_wcolor[2]],
                                colorTable[GNW_wcolor[1]]);

                draw_shaded_box(down, width, 1, 1, width - 2, length - 2, colorTable[GNW_wcolor[2]],
                                colorTable[GNW_wcolor[1]]);

                draw_box(down, width, 0, 0, width - 1, length - 1, colorTable[0]);

                b = button_create(id, ulx, uly, width, length, on_value, off_value, p_value, r_value, flags, up, down,
                                  NULL);
                if (b) {
                    button_draw(b, w, b->up, 0, NULL);

                    result = b->id;
                } else {
                    mem_free(up);
                    mem_free(down);

                    result = -1;
                }
            } else {
                mem_free(up);

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

int win_register_button_disable(ButtonID bid, char* disabled_up, char* disabled_down, char* disabled_hover) {
    int result;
    GNW_ButtonPtr b;

    if (GNW_win_init_flag) {
        b = GNW_find_button(bid, 0);

        if (b) {
            b->dis_up = disabled_up;
            b->dis_down = disabled_down;
            b->dis_hover = disabled_hover;

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int win_register_button_image(ButtonID bid, char* up, char* down, char* hover, int draw) {
    int result;
    GNW_ButtonPtr b;
    GNW_Window* w;

    if (GNW_win_init_flag) {
        if (!up && (down || hover)) {
            result = -1;
        } else {
            b = GNW_find_button(bid, &w);

            if (b) {
                if (b->flags & 0x10000) {
                    if (b->last_image == b->up) {
                        b->last_image = up;
                    } else if (b->last_image == b->down) {
                        b->last_image = down;
                    } else if (b->last_image == b->hover) {
                        b->last_image = hover;
                    }

                    b->up = up;
                    b->down = down;
                    b->hover = hover;

                    button_draw(b, w, b->last_image, draw, NULL);

                    result = 0;
                } else {
                    result = -1;
                }
            } else {
                result = -1;
            }
        }
    } else {
        result = -1;
    }

    return result;
}

int win_register_button_func(ButtonID bid, ButtonFunc on_func, ButtonFunc off_func, ButtonFunc p_func,
                             ButtonFunc r_func) {
    int result;
    GNW_ButtonPtr b;

    if (GNW_win_init_flag) {
        b = GNW_find_button(bid, NULL);

        if (b) {
            b->on_func = on_func;
            b->off_func = off_func;
            b->p_func = p_func;
            b->r_func = r_func;

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int win_register_right_button(ButtonID bid, int p_value, int r_value, ButtonFunc p_func, ButtonFunc r_func) {
    int result;
    GNW_ButtonPtr b;

    if (GNW_win_init_flag) {
        b = GNW_find_button(bid, NULL);
        if (b) {
            b->rp_value = p_value;
            b->rr_value = r_value;
            b->rp_func = p_func;
            b->rr_func = r_func;

            if (p_value != -1 || r_value != -1 || p_func || r_func) {
                b->flags |= 0x080000;
            } else {
                b->flags &= 0xFFF7FFFF;
            }

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int win_register_button_mask(ButtonID bid, char* mask) {
    int result;
    GNW_ButtonPtr b;

    if (GNW_win_init_flag) {
        b = GNW_find_button(bid, NULL);

        if (b) {
            b->mask = mask;

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

GNW_ButtonPtr button_create(WinID id, int ulx, int uly, int width, int length, int on_value, int off_value, int p_value,
                            int r_value, int flags, char* up, char* down, char* hover) {
    int v13;
    int v14;
    GNW_ButtonPtr result;
    GNW_ButtonPtr b;
    GNW_Window* w;

    w = GNW_find(id);

    if (w) {
        if ((ulx >= 0) && (uly >= 0) && ((width + ulx) <= win_width(id)) && ((length + uly) <= win_height(id))) {
            b = (GNW_ButtonPtr)mem_malloc(sizeof(struct GNW_buttondata));

            if (b) {
                if (!(flags & 0x01)) {
                    if (flags & 0x02) {
                        flags &= 0xFFFFFFFD;
                    }

                    if (flags & 0x04) {
                        flags &= 0xFFFFFFFB;
                    }
                }

                b->id = button_new_id();
                b->flags = flags;

                b->b.ulx = ulx;
                b->b.uly = uly;
                b->b.lrx = width + ulx - 1;
                b->b.lry = length + uly - 1;

                b->on_value = on_value;
                b->off_value = off_value;

                b->p_value = p_value;
                b->r_value = r_value;

                b->rp_value = -1;
                b->rr_value = -1;

                b->up = up;
                b->down = down;
                b->hover = hover;

                b->dis_up = NULL;
                b->dis_down = NULL;
                b->dis_hover = NULL;

                b->last_image = NULL;
                b->mask = NULL;

                b->on_func = NULL;
                b->off_func = NULL;

                b->p_func = NULL;
                b->r_func = NULL;

                b->rp_func = NULL;
                b->rr_func = NULL;

                b->group = NULL;

                b->prev = NULL;
                b->next = w->button_list;

                if (b->next) {
                    b->next->prev = b;
                }

                w->button_list = b;

                result = b;
            } else {
                result = NULL;
            }
        } else {
            result = NULL;
        }
    } else {
        result = NULL;
    }

    return result;
}

int win_button_down(ButtonID bid) {
    int result;
    GNW_ButtonPtr b;

    if (GNW_win_init_flag) {
        b = GNW_find_button(bid, NULL);

        if (b) {
            result = (b->flags & 0x01) && (b->flags & 0x20000);
        } else {
            result = 0;
        }
    } else {
        result = 0;
    }

    return result;
}

int GNW_check_buttons(GNW_Window* w, int* press) {
    Rect s;
    int result;
    int mbuttons;
    GNW_ButtonPtr b;
    GNW_ButtonPtr start;
    GNW_Window* ow;
    ButtonFunc func;

    b = w->last_over;
    start = w->button_list;
    func = NULL;

    if (w->flags & 8) {
        return -1;
    }

    if (b) {
        s = b->b;

        s.ulx += w->w.ulx;
        s.uly += w->w.uly;
        s.lrx += w->w.ulx;
        s.lry += w->w.uly;
    } else if (w->last_click) {
        s = w->last_click->b;

        s.ulx += w->w.ulx;
        s.uly += w->w.uly;
        s.lrx += w->w.ulx;
        s.lry += w->w.uly;
    }

    *press = -1;

    if (!mouse_click_in(w->w.ulx, w->w.uly, w->w.lrx, w->w.lry)) {
        if (b) {
            *press = b->off_value;

            if ((b->flags & 1) && (b->flags & 0x020000)) {
                button_draw(b, w, b->down, 1, NULL);
            } else {
                button_draw(b, w, b->up, 1, NULL);
            }

            w->last_over = NULL;
        }

        if (*press == -1) {
            if (b && !(b->flags & 8) && b->off_func) {
                b->off_func(b->id, *press);
            }

            result = -1;
        } else {
            last_button_winID = w->id;
            if (b->flags & 8) {
                *press = -1;
            } else if (b->off_func) {
                b->off_func(b->id, *press);

                if (!(b->flags & 0x40)) {
                    *press = -1;
                }
            }

            result = 0;
        }

        return result;
    }

    mbuttons = mouse_get_buttons();

    if (!(w->flags & 0x40) && (mbuttons & 1)) {
        win_show(w->id);
    } else if (!mbuttons) {
        w->last_click = NULL;
    }

    if (b) {
        if (!button_under_mouse(b, &s)) {
            if (!(b->flags & 8)) {
                *press = b->off_value;
            }

            if ((b->flags & 1) && (b->flags & 0x020000)) {
                button_draw(b, w, b->down, 1, NULL);
            } else {
                button_draw(b, w, b->up, 1, NULL);
            }

            w->last_over = NULL;
            last_button_winID = w->id;

            if (!(b->flags & 8)) {
                if (b->off_func) {
                    b->off_func(b->id, *press);

                    if (!(b->flags & 0x40)) {
                        *press = -1;
                    }
                }
            }

            return 0;
        }

        start = b;

    } else if (w->last_click && button_under_mouse(w->last_click, &s)) {
        start = w->last_click;

        if (!(start->flags & 8)) {
            *press = start->on_value;
        }

        if ((start->flags & 1) && (start->flags & 0x020000)) {
            button_draw(start, w, start->down, 1, NULL);
        } else {
            button_draw(start, w, start->up, 1, NULL);
        }

        w->last_over = start;
        last_button_winID = w->id;

        if (!(start->flags & 8)) {
            if (start->on_func) {
                start->on_func(start->id, *press);
                if (!(start->flags & 0x40)) *press = -1;
            }
        }

        return 0;
    }

    if (last_button_winID == -1 || last_button_winID == w->id || (ow = GNW_find(last_button_winID)) == 0 ||
        (last_button_winID = -1, b = ow->last_over, !ow) || !b) {
        for (b = start;; b = b->next) {
            if (!b) {
                if ((w->flags & 0x80) && (mbuttons & 3) && !(mbuttons & 0xC)) {
                    win_drag(w->id);
                }

                last_button_winID = w->id;

                return 0;
            }

            if (!(b->flags & 8)) {
                s = b->b;

                s.ulx += w->w.ulx;
                s.uly += w->w.uly;
                s.lrx += w->w.ulx;
                s.lry += w->w.uly;

                if (button_under_mouse(b, &s)) {
                    break;
                }
            }
        }

        if (!(b->flags & 8)) {
            if (mbuttons & 3) {
                if ((mbuttons & 2) && !(b->flags & 0x080000)) {
                    b = NULL;

                    if ((w->flags & 0x80) && (mbuttons & 3) && !(mbuttons & 0xC)) {
                        win_drag(w->id);
                    }

                    last_button_winID = w->id;

                    return 0;
                }

                if (b == w->last_over || b == w->last_click) {
                    w->last_over = b;
                    w->last_click = b;

                    if (b->flags & 1) {
                        if (b->flags & 2) {
                            if (b->flags & 0x020000) {
                                if (!(b->flags & 4)) {
                                    if (b->group) {
                                        b->group->curr_checked--;
                                    }

                                    if (mbuttons & 1) {
                                        *press = b->r_value;
                                        func = b->r_func;
                                    } else {
                                        *press = b->rr_value;
                                        func = b->rr_func;
                                    }

                                    b->flags &= 0xFFFDFFFF;
                                }
                            } else {
                                if (button_check_group(b) == -1) {
                                    b = NULL;

                                    if ((w->flags & 0x80) && (mbuttons & 3) && !(mbuttons & 0xC)) {
                                        win_drag(w->id);
                                    }

                                    last_button_winID = w->id;

                                    return 0;
                                }

                                if (mbuttons & 1) {
                                    *press = b->p_value;
                                    func = b->p_func;
                                } else {
                                    *press = b->rp_value;
                                    func = b->rp_func;
                                }

                                b->flags |= 0x020000;
                            }
                        }
                    } else {
                        if (button_check_group(b) == -1) {
                            b = NULL;

                            if ((w->flags & 0x80) && (mbuttons & 3) && !(mbuttons & 0xC)) {
                                win_drag(w->id);
                            }

                            last_button_winID = w->id;

                            return 0;
                        }

                        if (mbuttons & 1) {
                            *press = b->p_value;
                            func = b->p_func;
                        } else {
                            *press = b->rp_value;
                            func = b->rp_func;
                        }
                    }

                    button_draw(b, w, b->down, 1, NULL);
                }

                if (b) {
                    if ((b->flags & 0x10) && (mbuttons & 3) && !(mbuttons & 0xC)) {
                        win_drag(w->id);
                        button_draw(b, w, b->up, 1, NULL);
                    }

                } else if ((w->flags & 0x80) && (mbuttons & 3) && !(mbuttons & 0xC)) {
                    win_drag(w->id);
                }

                last_button_winID = w->id;

                if (b) {
                    if (func) {
                        func(b->id, *press);
                        if (!(b->flags & 0x40)) {
                            *press = -1;
                        }
                    }
                }

                return 0;
            }

            if ((b == w->last_click) && (mbuttons & 0x30)) {
                w->last_over = b;
                w->last_click = NULL;

                if (b->flags & 1) {
                    if (!(b->flags & 2)) {
                        if (b->flags & 0x020000) {
                            if (!(b->flags & 4)) {
                                if (b->group) {
                                    b->group->curr_checked--;
                                }

                                if (mbuttons & 0x10) {
                                    *press = b->r_value;
                                    func = b->r_func;
                                } else {
                                    *press = b->rr_value;
                                    func = b->rr_func;
                                }

                                b->flags &= 0xFFFDFFFF;
                            }
                        } else {
                            if (button_check_group(b) == -1) {
                                button_draw(b, w, b->up, 1, NULL);
                                b = NULL;

                                if ((w->flags & 0x80) && (mbuttons & 3) && !(mbuttons & 0xC)) {
                                    win_drag(w->id);
                                }

                                last_button_winID = w->id;

                                return 0;
                            }

                            if (mbuttons & 0x10) {
                                *press = b->p_value;
                                func = b->p_func;
                            } else {
                                *press = b->rp_value;
                                func = b->rp_func;
                            }

                            b->flags |= 0x020000;
                        }
                    }
                } else {
                    if ((b->flags & 0x020000) && b->group) {
                        b->group->curr_checked--;
                    }

                    if (mbuttons & 0x10) {
                        *press = b->r_value;
                        func = b->r_func;
                    } else {
                        *press = b->rr_value;
                        func = b->rr_func;
                    }
                }

                if (b->hover) {
                    button_draw(b, w, b->hover, 1, NULL);
                } else {
                    button_draw(b, w, b->up, 1, NULL);
                }

                if (b) {
                    if ((b->flags & 0x10) && (mbuttons & 3) && !(mbuttons & 0xC)) {
                        win_drag(w->id);
                        button_draw(b, w, b->up, 1, NULL);
                    }
                } else if ((w->flags & 0x80) && (mbuttons & 3) && !(mbuttons & 0xC)) {
                    win_drag(w->id);
                }

                last_button_winID = w->id;

                if (b) {
                    if (func) {
                        func(b->id, *press);
                        if (!(b->flags & 0x40)) *press = -1;
                    }
                }

                return 0;
            }
        }

        if (!w->last_over && !mbuttons) {
            w->last_over = b;
            if (!(b->flags & 8)) {
                *press = b->on_value;
                func = b->on_func;
            }

            button_draw(b, w, b->hover, 1, NULL);
        }

        if (b) {
            if ((b->flags & 0x10) && (mbuttons & 3) && !(mbuttons & 0xC)) {
                win_drag(w->id);
                button_draw(b, w, b->up, 1, NULL);
            }
        } else if ((w->flags & 0x80) && (mbuttons & 3) && !(mbuttons & 0xC)) {
            win_drag(w->id);
        }

        last_button_winID = w->id;

        if (b) {
            if (func) {
                func(b->id, *press);

                if (!(b->flags & 0x40)) {
                    *press = -1;
                }
            }
        }

        return 0;
    }

    if (!(b->flags & 8)) {
        *press = b->off_value;
    }

    if ((b->flags & 1) && (b->flags & 0x020000)) {
        button_draw(b, ow, b->down, 1, NULL);
    } else {
        button_draw(b, ow, b->up, 1, NULL);
    }

    ow->last_over = NULL;
    ow->last_click = NULL;

    if (!(b->flags & 8)) {
        if (b->off_func) {
            b->off_func(b->id, *press);
            if (!(b->flags & 0x40)) {
                *press = -1;
            }
        }
    }

    return 0;
}

int button_under_mouse(GNW_ButtonPtr b, Rect* r) {
    int result;
    int x;
    int y;

    result = mouse_click_in(r->ulx, r->uly, r->lrx, r->lry);

    if (result && b->mask) {
        mouse_get_position(&x, &y);

        x -= r->ulx;
        y -= r->uly;

        result = b->mask[x + y * (b->b.lrx - b->b.ulx + 1)];
    }

    return result;
}

WinID win_button_winID(ButtonID bid) {
    WinID result;
    GNW_Window* w;

    if (GNW_win_init_flag) {
        if (GNW_find_button(bid, &w))
            result = w->id;
        else
            result = -1;
    } else {
        result = -1;
    }

    return result;
}

WinID win_last_button_winID(void) { return last_button_winID; }

int win_delete_button(ButtonID bid) {
    int result;
    GNW_ButtonPtr b;
    GNW_Window* w;

    if (GNW_win_init_flag) {
        b = GNW_find_button(bid, &w);

        if (b) {
            if (b->prev) {
                b->prev->next = b->next;
            } else {
                w->button_list = b->next;
            }

            if (b->next) {
                b->next->prev = b->prev;
            }

            win_fill(w->id, b->b.ulx, b->b.uly, b->b.lrx - b->b.ulx + 1, b->b.lry - b->b.uly + 1, w->color);

            if (w->last_over == b) {
                w->last_over = NULL;
            }

            if (w->last_click == b) {
                w->last_click = NULL;
            }

            GNW_delete_button(b);

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

void GNW_delete_button(GNW_ButtonPtr b) {
    if (!(b->flags & 0x10000)) {
        if (b->up) {
            mem_free(b->up);
        }

        if (b->down) {
            mem_free(b->down);
        }

        if (b->hover) {
            mem_free(b->hover);
        }

        if (b->dis_up) {
            mem_free(b->dis_up);
        }

        if (b->dis_down) {
            mem_free(b->dis_down);
        }

        if (b->dis_hover) {
            mem_free(b->dis_hover);
        }
    }

    if (b->group) {
        for (int i = 0; i < b->group->num_buttons; i++) {
            if (b->group->list[i] == b) {
                for (int j = i + 1; j < b->group->num_buttons; j++) {
                    b->group->list[j - 1] = b->group->list[j];
                }

                b->group->num_buttons--;

                mem_free(b);
                return;
            }
        }
    } else {
        mem_free(b);
    }
}

void win_delete_button_win(ButtonID bid, int button_value) {
    GNW_Window* w;

    if (GNW_find_button(bid, &w)) {
        win_delete(w->id);
        GNW_add_input_buffer(button_value);
    }
}

ButtonID button_new_id(void) {
    ButtonID id;

    for (id = 1; GNW_find_button(id, NULL); id++) {
        ;
    }

    return id;
}

int win_enable_button(ButtonID bid) {
    int result;
    GNW_ButtonPtr b;
    GNW_Window* w;

    if (GNW_win_init_flag) {
        b = GNW_find_button(bid, &w);

        if (b) {
            if (b->flags & 0x08) {
                b->flags &= 0xFFFFFFF7;
                button_draw(b, w, b->last_image, 1, NULL);
            }

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int win_disable_button(ButtonID bid) {
    int result;
    GNW_ButtonPtr b;
    GNW_Window* w;

    if (GNW_win_init_flag) {
        b = GNW_find_button(bid, &w);

        if (b) {
            if (!(b->flags & 8)) {
                b->flags |= 8u;
                button_draw(b, w, b->last_image, 1, NULL);
                if (w->last_over == b) {
                    if (b->off_value != -1) {
                        GNW_add_input_buffer(b->off_value);
                    }

                    w->last_over = NULL;
                }
            }

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int win_set_button_rest_state(ButtonID bid, int rest_down, int flags) {
    int result;
    GNW_ButtonPtr b;
    GNW_Window* w;
    int input;

    input = -1;

    if (GNW_win_init_flag) {
        b = GNW_find_button(bid, &w);

        if (b) {
            if (b->flags & 0x01) {
                if (b->flags & 0x020000) {
                    if (!rest_down) {
                        b->flags &= 0xFFFDFFFF;

                        if (!(flags & 0x02)) {
                            button_draw(b, w, b->up, 1, NULL);
                        }

                        input = b->r_value;

                        if (b->group) {
                            b->group->curr_checked--;
                        }
                    }
                } else if (rest_down) {
                    b->flags |= 0x020000;

                    if (!(flags & 0x02)) {
                        button_draw(b, w, b->down, 1, NULL);
                    }

                    input = b->p_value;

                    if (b->group) {
                        b->group->curr_checked++;
                    }
                }

                if ((input != -1) && (flags & 0x01)) {
                    GNW_add_input_buffer(input);
                }
            }

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int win_group_check_buttons(int num_buttons, ButtonID* button_list, int max_checked, CheckButtonFunc func) {
    int result;
    int i;
    int j;
    GNW_ButtonPtr b;

    if (GNW_win_init_flag) {
        if (num_buttons <= 64) {
            for (i = 0;; i++) {
                if (i >= 64) {
                    return -1;
                }

                if (!btn_grp[i].num_buttons) {
                    break;
                }
            }

            btn_grp[i].curr_checked = 0;

            for (j = 0; j < num_buttons; ++j) {
                b = GNW_find_button(button_list[j], NULL);

                if (!b) {
                    return -1;
                }

                btn_grp[i].list[j] = b;
                b->group = &btn_grp[i];

                if (b->flags & 0x020000) {
                    btn_grp[i].curr_checked++;
                }
            }

            btn_grp[i].max_checked = max_checked;
            btn_grp[i].func = func;
            btn_grp[i].num_buttons = num_buttons;

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int win_group_radio_buttons(int num_buttons, ButtonID* button_list) {
    int result;
    GNW_ButtonPtr b;

    if (GNW_win_init_flag) {
        if (win_group_check_buttons(num_buttons, button_list, 1, NULL) == -1) {
            result = -1;

        } else {
            b = GNW_find_button(*button_list, NULL);

            for (int i = 0; i < b->group->num_buttons; i++) {
                b->group->list[i]->flags |= 0x40000;
            }

            result = 0;
        }
    } else {
        result = -1;
    }

    return result;
}

int button_check_group(GNW_ButtonPtr b) {
    GNW_Window* w;

    if (!b->group) {
        return 0;
    }

    if (b->flags & 0x040000) {
        if (b->group->curr_checked > 0) {
            for (int i = 0; i < b->group->num_buttons; i++) {
                b = b->group->list[i];

                if (b->flags & 0x020000) {
                    b->flags &= 0xFFFDFFFF;

                    GNW_find_button(b->id, &w);
                    button_draw(b, w, b->up, 1, NULL);

                    if (b->r_func) {
                        b->r_func(b->id, b->r_value);
                    }
                }
            }
        }

        if (!(b->flags & 0x020000)) {
            b->group->curr_checked++;
        }

        return 0;
    }

    if (b->group->curr_checked < b->group->max_checked) {
        if (!(b->flags & 0x020000)) {
            b->group->curr_checked++;
        }

        return 0;
    }

    if (b->group->func) {
        b->group->func(b->id);
    }

    return -1;
}

void button_draw(GNW_ButtonPtr b, GNW_Window* w, char* image, int draw, Rect* bound) {
    Rect s;
    Rect r;
    int bwidth;

    if (image) {
        s = b->b;

        s.ulx += w->w.ulx;
        s.uly += w->w.uly;
        s.lrx += w->w.ulx;
        s.lry += w->w.uly;

        if (bound) {
            if (rect_inside_bound(&s, bound, &s) == -1) {
                return;
            }

            r.ulx = s.ulx - w->w.ulx;
            r.uly = s.uly - w->w.uly;
            r.lrx = s.lrx - w->w.ulx;
            r.lry = s.lry - w->w.uly;
        } else {
            r = b->b;
        }

        if ((image == b->up) && (b->flags & 0x20000)) {
            image = b->down;
        }

        if (b->flags & 8) {
            if (image == b->up) {
                image = b->dis_up;
            } else if (image == b->down) {
                image = b->dis_down;
            } else if (image == b->hover) {
                image = b->dis_hover;
            }
        } else if (image == b->dis_up) {
            image = b->up;
        } else if (image == b->dis_down) {
            image = b->down;
        } else if (image == b->dis_hover) {
            image = b->hover;
        }

        if (image) {
            if (!draw) {
                bwidth = b->b.lrx - b->b.ulx + 1;

                if (b->flags & 0x20) {
                    trans_buf_to_buf((unsigned char*)&image[bwidth * (r.uly - b->b.uly) + r.ulx - b->b.ulx],
                                     r.lrx - r.ulx + 1, r.lry - r.uly + 1, bwidth, &w->buf[r.ulx + w->width * r.uly],
                                     w->width);
                } else {
                    buf_to_buf((unsigned char*)&image[bwidth * (r.uly - b->b.uly) + r.ulx - b->b.ulx],
                               r.lrx - r.ulx + 1, r.lry - r.uly + 1, bwidth, &w->buf[r.ulx + w->width * r.uly],
                               w->width);
                }
            }

            b->last_image = image;

            if (draw) {
                image = NULL;
                GNW_win_refresh(w, &s, NULL);
            }
        }
    }
}

void GNW_button_refresh(GNW_Window* w, Rect* r) {
    GNW_ButtonPtr b;

    for (b = w->button_list; b && b->next; b = b->next) {
        ;
    }

    while (b) {
        button_draw(b, w, b->last_image, 0, r);
        b = b->prev;
    }
}

int win_button_press_and_release(ButtonID bid) {
    int result;
    GNW_ButtonPtr b;
    GNW_Window* w;

    if (GNW_win_init_flag) {
        b = GNW_find_button(bid, &w);

        if (b) {
            button_draw(b, w, b->down, 1, NULL);

            if (b->p_func) {
                b->p_func(bid, b->p_value);

                if (b->flags & 0x40) {
                    GNW_add_input_buffer(b->p_value);
                }
            } else if (b->p_value != -1) {
                GNW_add_input_buffer(b->p_value);
            }

            button_draw(b, w, b->up, 1, NULL);

            if (b->r_func) {
                b->r_func(bid, b->r_value);

                if (b->flags & 0x40) {
                    GNW_add_input_buffer(b->r_value);
                }
            } else if (b->r_value != -1) {
                GNW_add_input_buffer(b->r_value);
            }

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}
