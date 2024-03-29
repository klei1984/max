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

#include "text.h"

#include "gnw.h"

#define TEXT_FONT_MANAGER_COUNT 10

typedef void (*text_font_func)(int32_t);

typedef struct FontMgr {
    int32_t low_font_num;
    int32_t high_font_num;
    text_font_func text_font;
    text_to_buf_func text_to_buf;
    text_height_func text_height;
    text_width_func text_width;
    text_char_width_func text_char_width;
    text_mono_width_func text_mono_width;
    text_spacing_func text_spacing;
    text_size_func text_size;
    text_max_func text_max;
} FontMgr, *FontMgrPtr;

static int32_t load_font(int32_t n);
static void GNW_text_font(int32_t font_num);
static int32_t text_font_exists(int32_t font_num, FontMgrPtr* mgr);
static void GNW_text_to_buf(uint8_t* buf, const char* str, int32_t swidth, int32_t fullw, int32_t color);
static int32_t GNW_text_height(void);
static int32_t GNW_text_width(const char* str);
static int32_t GNW_text_char_width(char c);
static int32_t GNW_text_mono_width(char* str);
static int32_t GNW_text_spacing(void);
static int32_t GNW_text_size(char* str);
static int32_t GNW_text_max(void);

typedef struct FontInfo_s {
    int32_t width;
    int32_t offset;
} FontInfo;

typedef struct Font_s {
    int32_t num;
    int32_t height;
    int32_t spacing;
    FontInfo* info;
    uint8_t* data;
} Font;

static Font font[GNW_TEXT_FONT_COUNT];
static FontMgr font_managers[TEXT_FONT_MANAGER_COUNT];
static int32_t total_managers;
static Font* curr_font;
static int32_t curr_font_num = -1;

text_to_buf_func text_to_buf;
text_height_func text_height;
text_width_func text_width;
text_char_width_func text_char_width;
text_mono_width_func text_mono_width;
text_spacing_func text_spacing;
text_size_func text_size;
text_max_func text_max;

int32_t GNW_text_init(void) {
    FontMgr GNW_font_mgr = {GNW_TEXT_FONT_0,  GNW_TEXT_FONT_9, GNW_text_font,       GNW_text_to_buf,
                            GNW_text_height,  GNW_text_width,  GNW_text_char_width, GNW_text_mono_width,
                            GNW_text_spacing, GNW_text_size,   GNW_text_max};
    int32_t first_font = -1;
    int32_t result;

    for (int32_t i = GNW_TEXT_FONT_0; i < GNW_TEXT_FONT_COUNT; i++) {
        if (load_font(i) == -1) {
            font[i].num = 0;
        } else if (first_font == -1) {
            first_font = i;
        }
    }

    if (first_font != -1) {
        result = text_add_manager(&GNW_font_mgr);
        if (result != -1) {
            text_font(first_font);
            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

void GNW_text_exit(void) {
    for (int32_t i = GNW_TEXT_FONT_0; i < GNW_TEXT_FONT_COUNT; i++) {
        if (font[i].num) {
            free(font[i].info);
            free(font[i].data);
        }
    }
}

int32_t load_font(int32_t n) {
    char str[13];
    DB_FILE fp;
    int32_t result;

    sprintf((char*)&str, "FONT%d.FON", n);

    fp = db_fopen(str, "rb");

    if (fp) {
        if (db_fread(&font[n], sizeof(Font), 1, fp) == 1) {
            font[n].info = (FontInfo*)malloc(sizeof(FontInfo) * font[n].num);

            if (font[n].info) {
                int32_t nelem = db_fread(font[n].info, sizeof(FontInfo), font[n].num, fp);
                if (nelem == font[n].num) {
                    int32_t last;
                    int32_t size;

                    last = nelem - 1;
                    size = font[n].height * ((font[n].info[last].width + 7) >> 3) + font[n].info[last].offset;
                    font[n].data = (uint8_t*)malloc(size);
                    if (font[n].data) {
                        if (db_fread(font[n].data, sizeof(uint8_t), size, fp) == size) {
                            db_fclose(fp);
                            result = 0;
                        } else {
                            db_fclose(fp);
                            free(font[n].info);
                            free(font[n].data);
                            result = -1;
                        }
                    } else {
                        db_fclose(fp);
                        free(font[n].info);
                        result = -1;
                    }
                } else {
                    db_fclose(fp);
                    free(font[n].info);
                    result = -1;
                }
            } else {
                db_fclose(fp);
                result = -1;
            }
        } else {
            db_fclose(fp);
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int32_t text_add_manager(FontMgrPtr mgr) {
    int32_t result;
    FontMgrPtr mgr_temp;

    result = -1;

    if (mgr) {
        if (total_managers < TEXT_FONT_MANAGER_COUNT) {
            if (mgr->low_font_num < mgr->high_font_num) {
                int32_t k = mgr->low_font_num;

                while (!text_font_exists(k, &mgr_temp)) {
                    k++;
                    if (k > mgr->high_font_num) {
                        memcpy(&font_managers[total_managers], mgr, sizeof(FontMgr));
                        total_managers++;
                        result = 0;
                        break;
                    }
                }
            }
        }
    }

    return result;
}

int32_t text_remove_manager(int32_t font_num) {
    FontMgrPtr mgr;
    int32_t result = -1;

    if (text_font_exists(font_num, &mgr)) {
        int32_t i = 0;

        while (i < total_managers && (mgr != &font_managers[i] || (curr_font_num >= font_managers[i].low_font_num &&
                                                                   curr_font_num <= font_managers[i].high_font_num))) {
            i++;
        }

        if (i < total_managers - 1) {
            memmove(&font_managers[i], &font_managers[i + 1], sizeof(FontMgr) * (total_managers - i) - sizeof(FontMgr));
            total_managers--;
            result = 0;
        }
    }

    return result;
}

void GNW_text_font(int32_t font_num) {
    if (font_num < GNW_TEXT_FONT_COUNT) {
        if (font[font_num].num) {
            curr_font = &font[font_num];
        }
    }
}

int32_t text_curr(void) { return curr_font_num; }

void text_font(int32_t font_num) {
    FontMgrPtr mgr;

    if (text_font_exists(font_num, &mgr)) {
        text_to_buf = mgr->text_to_buf;
        text_height = mgr->text_height;
        text_width = mgr->text_width;
        text_char_width = mgr->text_char_width;
        text_mono_width = mgr->text_mono_width;
        text_spacing = mgr->text_spacing;
        text_size = mgr->text_size;
        text_max = mgr->text_max;

        curr_font_num = font_num;

        mgr->text_font(font_num);
    }
}

int32_t text_font_exists(int32_t font_num, FontMgrPtr* mgr) {
    int32_t result;
    int32_t i = 0;

    while (i < total_managers &&
           (font_num < font_managers[i].low_font_num || font_num > font_managers[i].high_font_num)) {
        i++;
    }

    if (i < total_managers) {
        *mgr = &font_managers[i];
        result = 1;
    } else {
        result = 0;
    }

    return result;
}

void GNW_text_to_buf(uint8_t* buf, const char* str, int32_t swidth, int32_t fullw, int32_t color) {
    uint8_t* data;
    uint8_t* bstart;
    uint8_t* bnext;
    int32_t tmax;
    int32_t width;

    bstart = buf;

    if (color & GNW_TEXT_OUTLINE) {
        color &= ~GNW_TEXT_OUTLINE;
        text_to_buf(&buf[fullw + 1], str, swidth, fullw, (color & (~GNW_TEXT_COLOR_MASK)) | colorTable[0]);
    }

    if (color & GNW_TEXT_MONOSPACE) {
        tmax = text_max();
    }

    while (*str) {
        if (*str < curr_font->num) {
            width = curr_font->info[*str].width;

            if (color & GNW_TEXT_MONOSPACE) {
                bnext = &buf[tmax];
                buf += (tmax - curr_font->spacing - width) / 2;
            } else {
                bnext = (&buf[width] + curr_font->spacing);
            }

            if ((int32_t)(bnext - bstart) > swidth) {
                break;
            }

            data = &curr_font->data[curr_font->info[*str].offset];

            for (int32_t h = 0; h < curr_font->height; h++) {
                uint8_t mask = 0x80;

                for (int32_t w = 0; w < width; w++, buf++) {
                    if (!mask) {
                        mask = 0x80;
                        data++;
                    }

                    if (mask & *data) {
                        *buf = color;
                    }

                    mask >>= 1;
                }

                data++;
                buf += fullw - width;
            }
            buf = bnext;
        }
        str++;
    }

    if (color & GNW_TEXT_UNDERLINE) {
        width = buf - bstart;
        buf = &bstart[fullw * (curr_font->height - 1)];

        for (int32_t i = 0; i < width; i++) {
            buf[i] = color;
        }
    }
}

int32_t GNW_text_height(void) { return curr_font->height; }

int32_t GNW_text_width(const char* str) {
    int32_t len;
    int32_t i;

    len = 0;

    while (*str) {
        i = *str;
        if (i < curr_font->num) {
            len += curr_font->spacing + curr_font->info[i].width;
        }
        str++;
    }

    return len;
}

int32_t GNW_text_char_width(char c) { return curr_font->info[c].width; }

int32_t GNW_text_mono_width(char* str) { return strlen(str) * text_max(); }

int32_t GNW_text_spacing(void) { return curr_font->spacing; }

int32_t GNW_text_size(char* str) { return text_width(str) * text_height(); }

int32_t GNW_text_max(void) {
    int32_t len;

    len = 0;
    for (int32_t i = 0; i < curr_font->num; i++) {
        if (len < curr_font->info[i].width) {
            len = curr_font->info[i].width;
        }
    }

    return len + curr_font->spacing;
}
