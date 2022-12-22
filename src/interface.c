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

#include "interface.h"

#include "gnw.h"

typedef struct tm_item_s {
    TOCKS created;
    WinID id;
    int location;
} tm_item;

typedef struct tm_location_item_s {
    int taken;
    int y;
} tm_location_item;

static WinID create_pull_down(char** list, int num, int ulx, int uly, int fcolor, int bcolor, Rect* r);
static int process_pull_down(WinID id, Rect* r, char** list, int num, int fcolor, int bcolor, GNW_Menu* m, int num_pd);
static void win_debug_delete(ButtonID bid, int button_value);
static int find_first_letter(int c, char** list, int num);
static int calc_max_field_chars_wcursor(int min, int max);
static int get_num_i(WinID id, int* value, int max_chars_wcursor, char clear, char allow_negative, int x, int y);
static void tm_watch_msgs(void);
static void tm_kill_msg(void);
static void tm_kill_out_of_order(int queue_index);
static void tm_click_response(ButtonID b_id, int b_value);
static int tm_index_active(int queue_index);

int win_list_select(char* title, char** list, int num, SelectFunc select_func, int ulx, int uly, int color) {
    return win_list_select_at(title, list, num, select_func, ulx, uly, color, 0);
}

static tm_location_item tm_location[5];
static int tm_text_x;
static int tm_h;
static tm_item tm_queue[5];
static TOCKS tm_persistence;
static int tm_kill;
static int scr_center_x;
static int tm_add;
static int tm_text_y;
static int curry;
static int currx;
static int tm_watch_active;

static WinID wd = -1;

int win_list_select_at(char* title, char** list, int num, SelectFunc select_func, int ulx, int uly, int color,
                       int start) {
    /* not implemented yet as M.A.X. does not use it */
    SDL_assert(0);

    return -1;
}

int win_get_str(char* str, int limit, char* title, int x, int y) {
    unsigned char* buf;
    int retval;
    int width;
    int length;
    WinID id;

    if (GNW_win_init_flag) {
        width = text_width(title) + 12;

        if (((limit + 1) * text_max()) > width) {
            width = text_max() * (limit + 1);
        }

        width += 16;

        if (width < 160) {
            width = 160;
        }

        length = 5 * text_height() + 16;

        id = win_add(x, y, width, length, 256, 20);

        if (id == -1) {
            retval = -1;
        } else {
            win_border(id);

            buf = GNW_find(id)->buf;

            buf_fill(&buf[(text_height() + 14) * width + 14], width - 28, text_height() + 2, width,
                     colorTable[GNW_wcolor[0]]);

            text_to_buf(&buf[8 * width + 8], title, width, width, colorTable[GNW_wcolor[4]]);

            draw_shaded_box(buf, width, 14, text_height() + 14, width - 14, 2 * text_height() + 16,
                            colorTable[GNW_wcolor[2]], colorTable[GNW_wcolor[1]]);

            win_register_text_button(id, width / 2 - 72, length - 8 - text_height() - 6, -1, -1, -1, 13, "Done", 0);

            win_register_text_button(id, width / 2 + 8, length - 8 - text_height() - 6, -1, -1, -1, 27, "Cancel", 0);

            win_draw(id);

            retval = win_input_str(id, str, limit, 16, text_height() + 16, colorTable[GNW_wcolor[3]],
                                   colorTable[GNW_wcolor[0]]);

            win_delete(id);
        }
    } else {
        retval = -1;
    }

    return retval;
}

int win_output(char* title, char** list, int num, int ulx, int uly, int color, char* extra_button) {
    /* not implemented yet as M.A.X. does not use it */
    SDL_assert(0);

    return -1;
}

int win_yes_no(char* question, int ulx, int uly, int color) {
    /* not implemented yet as M.A.X. does not use it */
    SDL_assert(0);

    return -1;
}

int win_msg(char* msg, int ulx, int uly, int color) {
    /* not implemented yet as M.A.X. does not use it */
    SDL_assert(0);

    return -1;
}

int win_pull_down(char** list, int num, int ulx, int uly, int color) {
    Rect r;
    int result;
    WinID id;

    if (GNW_win_init_flag) {
        id = create_pull_down(list, num, ulx, uly, color, colorTable[GNW_wcolor[0]], &r);

        if (id == -1) {
            result = -1;
        } else {
            result = process_pull_down(id, &r, list, num, color, colorTable[GNW_wcolor[0]], 0, -1);
        }
    } else {
        result = -1;
    }

    return result;
}

WinID create_pull_down(char** list, int num, int ulx, int uly, int fcolor, int bcolor, Rect* r) {
    WinID result;
    int width;
    int length;
    WinID id;

    length = num * text_height() + 16;
    width = win_width_needed(list, num) + 4;

    if (length >= 2 && width >= 2) {
        id = win_add(ulx, uly, width, length, bcolor, 20);

        if (id == -1) {
            result = -1;
        } else {
            win_text(id, list, num, width - 4, 2, 8, fcolor);

            win_box(id, 0, 0, width - 1, length - 1, colorTable[0]);
            win_box(id, 1, 1, width - 2, length - 2, fcolor);

            win_draw(id);

            win_get_rect(id, r);

            result = id;
        }
    } else {
        result = -1;
    }

    return result;
}

int process_pull_down(WinID id, Rect* r, char** list, int num, int fcolor, int bcolor, GNW_Menu* m, int num_pd) {
    /* not implemented yet as M.A.X. does not use it */
    SDL_assert(0);

    return -1;
}

int win_debug(char* str) {
    /* not implemented yet as M.A.X. does not use it */
    SDL_assert(0);

    return -1;
}

void win_debug_delete(ButtonID bid, int button_value) {
    /* not implemented yet as M.A.X. does not use it */
    SDL_assert(0);
}

int win_register_menu_bar(WinID wid, int ulx, int uly, int width, int length, int fore_color, int back_color) {
    int result;
    GNW_Window* w;

    w = GNW_find(wid);
    if (GNW_win_init_flag && w && !w->menu && (width + ulx <= w->width) && (length + uly <= w->length)) {
        w->menu = (GNW_Menu*)malloc(sizeof(GNW_Menu));

        if (w->menu) {
            w->menu->wid = wid;

            w->menu->m.ulx = ulx;
            w->menu->m.uly = uly;
            w->menu->m.lrx = width + ulx - 1;
            w->menu->m.lry = length + uly - 1;

            w->menu->num_pds = 0;

            w->menu->fcolor = fore_color;
            w->menu->bcolor = back_color;

            win_fill(wid, ulx, uly, width, length, back_color);

            win_box(wid, ulx, uly, width + ulx - 1, length + uly - 1, fore_color);

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int win_register_menu_pulldown(WinID wid, int offx, char* name, int value, int num, char** list, int fore_color,
                               int back_color) {
    int result;
    GNW_Window* w;
    int i;
    int x;
    int y;

    w = GNW_find(wid);
    if (GNW_win_init_flag && w && w->menu && w->menu->num_pds != 15) {
        i = w->menu->num_pds;

        x = w->menu->m.ulx + offx;
        y = (w->menu->m.lry + w->menu->m.uly - text_height()) / 2;

        if (win_register_button(wid, x, y, text_width(name), text_height(), -1, -1, value, -1, 0, 0, 0, 0) == -1) {
            result = -1;
        } else {
            win_print(wid, name, 0, x, y, w->menu->fcolor | 0x2000000);

            w->menu->pd[i].r.ulx = x;
            w->menu->pd[i].r.uly = y;
            w->menu->pd[i].r.lrx = text_width(name) + x - 1;
            w->menu->pd[i].r.lry = text_height() + y - 1;
            w->menu->pd[i].value = value;
            w->menu->pd[i].num_list = num;
            w->menu->pd[i].list = list;
            w->menu->pd[i].fcolor = fore_color;
            w->menu->pd[i].bcolor = back_color;

            w->menu->num_pds++;

            result = 0;
        }
    } else {
        result = -1;
    }

    return result;
}

void win_delete_menu_bar(WinID wid) {
    GNW_Window* w;

    w = GNW_find(wid);

    if (GNW_win_init_flag && w) {
        if (w->menu) {
            win_fill(wid, w->menu->m.ulx, w->menu->m.uly, w->menu->m.lrx - w->menu->m.ulx + 1,
                     w->menu->m.lry - w->menu->m.uly + 1, w->color);

            free(w->menu);
            w->menu = NULL;
        }
    }
}

int GNW_process_menu(GNW_Menu* m, int num_pd) {
    static GNW_Menu* curr_menu = NULL;

    Rect r;
    WinID wid;
    int i;
    int result;
    GNW_PD* pd;

    if (curr_menu) {
        result = -1;
    } else {
        curr_menu = m;

        do {
            pd = &m->pd[num_pd];

            wid = create_pull_down(pd->list, pd->num_list, pd->r.ulx, m->m.lry + 1, pd->fcolor, pd->bcolor, &r);

            if (wid == -1) {
                curr_menu = NULL;

                return -1;
            }

            i = process_pull_down(wid, &r, pd->list, pd->num_list, pd->fcolor, pd->bcolor, m, num_pd);

            if (i < -1) {
                num_pd = -2 - i;
            }

        } while (i < -1);

        if (i != -1) {
            flush_input_buffer();
            GNW_add_input_buffer(i);

            i = m->pd[num_pd].value;
        }

        curr_menu = NULL;
        result = i;
    }

    return result;
}

int find_first_letter(int c, char** list, int num) {
    if (c >= 'A' && c <= 'Z') {
        c += ' ';
    }

    for (int i = 0; i < num; i++) {
        if ((*list[i] == c) || (*list[i] == c - ' ')) {
            return i;
        }
    }

    return -1;
}

int win_width_needed(char** list, int num) {
    char** lc;
    int i;
    int j;
    int width;

    width = 0;
    lc = list;

    for (i = 0; i < num; i++) {
        j = text_width(lc[i]);

        if (j > width) {
            width = j;
        }
    }

    return width;
}

int win_input_str(WinID id, char* str, int limit, int x, int y, int text_color, int back_color) {
    /* not implemented yet as M.A.X. does not use it */
    SDL_assert(0);

    return -1;
}

int win_get_num_i(int* value, int min, int max, int clear, char* title, int x, int y) {
    /* not implemented yet as M.A.X. does not use it */
    SDL_assert(0);

    return -1;
}

int calc_max_field_chars_wcursor(int min, int max) {
    int result;
    char* str_num;
    int len_min;
    int len_max;
    int r;

    str_num = (char*)malloc(17);

    if (str_num) {
        sprintf(str_num, "%d", min);
        len_min = strlen(str_num);

        sprintf(str_num, "%d", max);
        len_max = strlen(str_num);

        free(str_num);

        if (len_max <= len_min) {
            r = len_min;
        } else {
            r = len_max;
        }

        result = r + 1;
    } else {
        result = -1;
    }

    return result;
}

int get_num_i(WinID id, int* value, int max_chars_wcursor, char clear, char allow_negative, int x, int y) {
    /* not implemented yet as M.A.X. does not use it */
    SDL_assert(0);

    return -1;
}

void GNW_intr_init(void) {
    int i;
    int y_first;
    int y_inc;

    tm_add = 0;
    tm_kill = -1;
    tm_persistence = 3000;

    scr_center_x = scr_size.lrx >> 1;
    y_first = scr_size.lry >> 2;
    y_inc = scr_size.lry >> 3;

    if (scr_size.lry >= 479) {
        tm_text_x = 16;
        tm_text_y = 16;
    } else {
        tm_text_x = 10;
        tm_text_y = 10;
    }

    tm_h = 2 * tm_text_y + text_height();

    for (i = 0; i < 5; i++) {
        tm_location[i].taken = 0;
        tm_location[i].y = y_first + i * y_inc;
    }
}

void win_timed_msg_defaults(TOCKS persistence) { tm_persistence = persistence; }

void GNW_intr_exit(void) {
    remove_bk_process(tm_watch_msgs);
    while (tm_kill != -1) {
        tm_kill_msg();
    }
}

int win_timed_msg(char* msg, int color) {
    /* not implemented yet as M.A.X. does not use it */
    SDL_assert(0);

    return -1;
}

void tm_watch_msgs(void) {
    if (!tm_watch_active) {
        tm_watch_active = 1;

        while (tm_kill != -1 && elapsed_time(tm_queue[tm_kill].created) >= tm_persistence) {
            tm_kill_msg();
        }

        tm_watch_active = 0;
    }
}

void tm_kill_msg(void) {
    if (tm_kill != -1) {
        win_delete(tm_queue[tm_kill].id);

        tm_location[tm_queue[tm_kill].location].taken = 0;

        tm_kill++;

        if (tm_kill == 5) {
            tm_kill = 0;
        }

        if (tm_kill == tm_add) {
            tm_add = 0;
            tm_kill = -1;

            remove_bk_process(tm_watch_msgs);
        }
    }
}

void tm_kill_out_of_order(int queue_index) {
    int copy_over;
    int copy_from;

    if (tm_kill != -1 && tm_index_active(queue_index)) {
        win_delete(tm_queue[queue_index].id);

        tm_location[tm_queue[queue_index].location].taken = 0;

        for (copy_over = queue_index; copy_over != tm_kill; copy_over = copy_from) {
            copy_from = copy_over - 1;
            if ((copy_over - 1) < 0) {
                copy_from = 4;
            }

            memcpy(&tm_queue[copy_over], &tm_queue[copy_from], sizeof(tm_item));
        }

        tm_kill++;

        if (tm_kill == 5) {
            tm_kill = 0;
        }

        if (tm_kill == tm_add) {
            tm_add = 0;
            tm_kill = -1;

            remove_bk_process(tm_watch_msgs);
        }
    }
}

void tm_click_response(ButtonID b_id, int b_value) {
    int queue_index;
    WinID w_id;

    if (tm_kill != -1) {
        queue_index = tm_kill;

        w_id = win_button_winID(b_id);

        while (tm_queue[queue_index].id != w_id) {
            queue_index++;

            if (queue_index == 5) {
                queue_index = 0;
            }

            if (queue_index == tm_kill || !tm_index_active(queue_index)) {
                return;
            }
        }

        tm_kill_out_of_order(queue_index);
    }
}

int tm_index_active(int queue_index) {
    if (tm_kill == tm_add) {
        return 1;
    }

    if (tm_kill >= tm_add) {
        if (queue_index >= tm_add && queue_index < tm_kill) {
            return 0;
        }

        return 1;
    }

    if (queue_index >= tm_kill && queue_index < tm_add) {
        return 1;
    }

    return 0;
}
