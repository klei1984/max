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

#include "text.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iconv.h>

#include <map>

#include "point.hpp"
#include "resource_manager.hpp"
#include "sound_manager.hpp"

#define TEXT_FONT_MANAGER_COUNT 10
#define TEXT_FONT_META(id, height) \
    { (id), (height) }
#define TEXT_DEFAULT_GLYPH \
    { nullptr, 0, 0, 0, 0, 0, 0 }
#define TEXT_GLYPH_METRICS_SCALE (64)
#define TEXT_CACHE_ENTRIES (30)

typedef void (*text_font_func)(int);

typedef struct FontMgr {
    int low_font_num;
    int high_font_num;
    text_font_func text_font;
    text_to_buf_func text_to_buf;
    text_height_func text_height;
    text_width_func text_width;
    text_char_width_func text_char_width;
    text_mono_width_func text_mono_width;
    text_spacing_func text_spacing;
    text_size_func text_size;
} FontMgr, *FontMgrPtr;

static void Text_LoadFontTTF(int n, FT_Library library, struct Font& font);
static void Text_SetFontTTF(int font_num);
static int Text_FontExistsTTF(int font_num, FontMgrPtr* mgr);
static void Text_BlitTTF(unsigned char* buf, const char* str, int swidth, int fullw, int color);
static int Text_GetHeightTTF(void);
static int Text_GetWidthTTF(const char* str);
static int Text_GetGlyphWidthTTF(unsigned int glyph);
static int Text_GetMonospaceWidthTTF(const char* str);
static int Text_GetSpacingTTF(void);
static int Text_GetSizeTTF(const char* str);
static bool Text_FitBounds(Rect* output, Rect* bounds1, Rect* bounds2);
static bool Text_IsFitting(Rect* bounds1, Rect* bounds2);
static inline struct FontGlyph& Text_GetGlyph(Uint32 key);
static Uint32* Text_Utf8ToUcs4(const char* str);
static unsigned int Text_GetHash(const char* str);
static Uint32* Text_GetCachedString(const char* str, unsigned int& hash);
static void Text_AddCachedString(Uint32* buffer, unsigned int& hash);

struct FontGlyph {
    unsigned char* buffer;
    unsigned short width;
    unsigned short height;
    short pitch;
    short ulx;
    short uly;
    short advance;
};

struct FontMeta {
    int id;
    int height;
};

struct Font {
    int ascender;
    int max_width;
    int max_height;
    iconv_t cd;
    std::map<Uint32, struct FontGlyph> glyphs;
};

struct TextCacheEntry {
    unsigned int hash;
    Uint32* buffer;
};

struct TextCache {
    struct TextCacheEntry entries[TEXT_CACHE_ENTRIES];
    int write_index;
};

const struct FontMeta Text_FontMetas[] = {
    TEXT_FONT_META(GNW_TEXT_FONT_0, 10), TEXT_FONT_META(GNW_TEXT_FONT_1, 16), TEXT_FONT_META(GNW_TEXT_FONT_2, 9),
    TEXT_FONT_META(GNW_TEXT_FONT_3, 9),  TEXT_FONT_META(GNW_TEXT_FONT_4, 11), TEXT_FONT_META(GNW_TEXT_FONT_5, 10),
    TEXT_FONT_META(GNW_TEXT_FONT_6, 16),
};

static struct TextCache Text_StringCache;
static struct Font Text_Fonts[GNW_TEXT_FONT_COUNT];
static struct FontMgr Text_FontManagers[TEXT_FONT_MANAGER_COUNT];
static int Text_TotalManagers;
static struct Font* Text_CurrentFont;
static int Text_CurrentFontIndex = -1;

text_to_buf_func Text_Blit;
text_height_func Text_GetHeight;
text_width_func Text_GetWidth;
text_char_width_func Text_GetGlyphWidth;
text_mono_width_func Text_GetMonospaceWidth;
text_spacing_func Text_GetSpacing;
text_size_func Text_GetSize;

unsigned int Text_TypeWriter_CharacterTimeMs = 50;
unsigned int Text_TypeWriter_BeepTimeMs = 100;

int Text_Init(void) {
    FT_Library Text_Library;
    struct FontMgr manager = {GNW_TEXT_FONT_0,    GNW_TEXT_FONT_9,  Text_SetFontTTF,       Text_BlitTTF,
                              Text_GetHeightTTF,  Text_GetWidthTTF, Text_GetGlyphWidthTTF, Text_GetMonospaceWidthTTF,
                              Text_GetSpacingTTF, Text_GetSizeTTF};

    if (FT_Init_FreeType(&Text_Library)) {
        SDL_Log("Couldn't initialize font library\n");

        return -1;
    }

    memset(&Text_StringCache, 0, sizeof(Text_StringCache));

    int first_font = -1;
    int result;

    Text_TotalManagers = 0;

    for (int i = GNW_TEXT_FONT_0; i < GNW_TEXT_FONT_COUNT; ++i) {
        Text_LoadFontTTF(i, Text_Library, Text_Fonts[i]);

        if (Text_Fonts[i].glyphs.size()) {
            if (first_font == -1) {
                first_font = i;
            }
        }
    }

    if (first_font != -1) {
        result = Text_AddManager(&manager);

        if (result != -1) {
            Text_SetFont(first_font);

            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    FT_Done_FreeType(Text_Library);

    return result;
}

void Text_Exit(void) {
    for (int i = GNW_TEXT_FONT_0; i < GNW_TEXT_FONT_COUNT; ++i) {
        if (Text_Fonts[i].cd != reinterpret_cast<iconv_t>(-1)) {
            iconv_close(Text_Fonts[i].cd);
            Text_Fonts[i].cd = reinterpret_cast<iconv_t>(-1);
        }

        Text_Fonts[i].glyphs.clear();
    }

    for (auto& entry : Text_StringCache.entries) {
        delete[] entry.buffer;
        entry.buffer = nullptr;
        entry.hash = 0;
    }
}

void Text_LoadFontTTF(int n, FT_Library library, struct Font& font) {
    ResourceID id;

    switch (n) {
        case GNW_TEXT_FONT_1: {
            id = FONT_1;
        } break;

        case GNW_TEXT_FONT_2: {
            id = FONT_2;
        } break;

        case GNW_TEXT_FONT_5: {
            id = FONT_5;
        } break;

        default: {
            return;
        } break;
    }

    FT_Face font_face = nullptr;

    font.cd = iconv_open("UCS-4LE", "UTF-8");

    if (font.cd == reinterpret_cast<iconv_t>(-1)) {
        font.glyphs.clear();

        return;
    }

    unsigned int file_size = ResourceManager_GetResourceSize(id);
    unsigned char* file_base = ResourceManager_ReadResource(id);

    if (file_base) {
        if (FT_New_Memory_Face(library, file_base, file_size, 0, &font_face) == FT_Err_Ok) {
            if (FT_Set_Pixel_Sizes(font_face, Text_FontMetas[n].height, Text_FontMetas[n].height) == FT_Err_Ok) {
                FT_Select_Charmap(font_face, FT_ENCODING_UNICODE);

                FT_ULong char_code;
                FT_UInt glyph_index;
                const FT_Size_Metrics* metrics = &font_face->size->metrics;

                font.ascender =
                    (metrics->y_ppem * metrics->ascender) / (metrics->ascender + std::abs(metrics->descender));
                font.max_width = 0;
                font.max_height = metrics->y_ppem;

                char_code = FT_Get_First_Char(font_face, &glyph_index);

                while (glyph_index) {
                    if (FT_Load_Glyph(font_face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP) == FT_Err_Ok) {
                        const FT_GlyphSlot slot = font_face->glyph;
                        struct FontGlyph font_glyph = TEXT_DEFAULT_GLYPH;

                        if (FT_Render_Glyph(slot, FT_RENDER_MODE_MONO) == FT_Err_Ok) {
                            font_glyph.width = slot->metrics.width / TEXT_GLYPH_METRICS_SCALE;
                            font_glyph.height = slot->metrics.height / TEXT_GLYPH_METRICS_SCALE;
                            font_glyph.ulx = slot->metrics.horiBearingX / TEXT_GLYPH_METRICS_SCALE;
                            font_glyph.uly = font.ascender - (slot->metrics.horiBearingY / TEXT_GLYPH_METRICS_SCALE);
                            font_glyph.advance = slot->metrics.horiAdvance / TEXT_GLYPH_METRICS_SCALE;

                            font_glyph.pitch = slot->bitmap.pitch;
                            font_glyph.buffer =
                                new (std::nothrow) unsigned char[slot->bitmap.pitch * slot->bitmap.rows];

                            if (font_glyph.buffer) {
                                buf_to_buf(slot->bitmap.buffer, slot->bitmap.pitch, slot->bitmap.rows,
                                           slot->bitmap.pitch, font_glyph.buffer, slot->bitmap.pitch);
                            }

                            font.max_width = std::max<int>(font.max_width, font_glyph.width);
                            font.max_height = std::max<int>(font.max_height, font_glyph.height);
                        }

                        font.glyphs.insert({char_code, font_glyph});
                    }

                    char_code = FT_Get_Next_Char(font_face, char_code, &glyph_index);
                }

                FT_Done_Face(font_face);

            } else {
                iconv_close(font.cd);
                font.cd = reinterpret_cast<iconv_t>(-1);
                FT_Done_Face(font_face);
                font.glyphs.clear();
            }

        } else {
            iconv_close(font.cd);
            font.cd = reinterpret_cast<iconv_t>(-1);
            font.glyphs.clear();
        }

        delete[] file_base;

    } else {
        iconv_close(font.cd);
        font.cd = reinterpret_cast<iconv_t>(-1);
        font.glyphs.clear();
    }
}

int Text_AddManager(FontMgrPtr mgr) {
    int result;
    FontMgrPtr mgr_temp;

    result = -1;

    if (mgr) {
        if (Text_TotalManagers < TEXT_FONT_MANAGER_COUNT) {
            if (mgr->low_font_num < mgr->high_font_num) {
                int k = mgr->low_font_num;

                while (!Text_FontExistsTTF(k, &mgr_temp)) {
                    k++;
                    if (k > mgr->high_font_num) {
                        memcpy(&Text_FontManagers[Text_TotalManagers], mgr, sizeof(FontMgr));
                        Text_TotalManagers++;
                        result = 0;
                        break;
                    }
                }
            }
        }
    }

    return result;
}

int Text_RemoveManager(int font_num) {
    FontMgrPtr mgr;
    int result = -1;

    if (Text_FontExistsTTF(font_num, &mgr)) {
        int i = 0;

        while (i < Text_TotalManagers &&
               (mgr != &Text_FontManagers[i] || (Text_CurrentFontIndex >= Text_FontManagers[i].low_font_num &&
                                                 Text_CurrentFontIndex <= Text_FontManagers[i].high_font_num))) {
            ++i;
        }

        if (i < Text_TotalManagers - 1) {
            memmove(&Text_FontManagers[i], &Text_FontManagers[i + 1],
                    sizeof(FontMgr) * (Text_TotalManagers - i) - sizeof(FontMgr));
            --Text_TotalManagers;
            result = 0;
        }
    }

    return result;
}

void Text_SetFontTTF(int font_num) {
    if (font_num < GNW_TEXT_FONT_COUNT) {
        if (Text_Fonts[font_num].glyphs.size()) {
            Text_CurrentFont = &Text_Fonts[font_num];
        }
    }
}

int Text_GetFont(void) { return Text_CurrentFontIndex; }

void Text_SetFont(int font_num) {
    FontMgrPtr mgr;

    if (Text_FontExistsTTF(font_num, &mgr)) {
        Text_Blit = mgr->text_to_buf;
        Text_GetHeight = mgr->text_height;
        Text_GetWidth = mgr->text_width;
        Text_GetGlyphWidth = mgr->text_char_width;
        Text_GetMonospaceWidth = mgr->text_mono_width;
        Text_GetSpacing = mgr->text_spacing;
        Text_GetSize = mgr->text_size;

        Text_CurrentFontIndex = font_num;

        mgr->text_font(font_num);
    }
}

int Text_FontExistsTTF(int font_num, FontMgrPtr* mgr) {
    int result;
    int i = 0;

    while (i < Text_TotalManagers &&
           (font_num < Text_FontManagers[i].low_font_num || font_num > Text_FontManagers[i].high_font_num)) {
        ++i;
    }

    if (i < Text_TotalManagers) {
        *mgr = &Text_FontManagers[i];
        result = 1;
    } else {
        result = 0;
    }

    return result;
}

struct FontGlyph& Text_GetGlyph(Uint32 key) {
    if (!Text_CurrentFont->glyphs.contains(key)) {
        struct FontGlyph font_glyph = TEXT_DEFAULT_GLYPH;

        Text_CurrentFont->glyphs.insert({key, font_glyph});
    }

    return Text_CurrentFont->glyphs[key];
}

unsigned int Text_GetHash(const char* str) {
    uint32_t hash = 0;

    for (; *str; ++str) {
        hash += *str;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

Uint32* Text_GetCachedString(const char* str, unsigned int& hash) {
    hash = Text_GetHash(str);

    for (auto& cache : Text_StringCache.entries) {
        if (cache.hash == hash) {
            return cache.buffer;
        }
    }

    return nullptr;
}

void Text_AddCachedString(Uint32* buffer, unsigned int& hash) {
    ++Text_StringCache.write_index;

    if (Text_StringCache.write_index >= TEXT_CACHE_ENTRIES) {
        Text_StringCache.write_index = 0;
    }

    auto& entry = Text_StringCache.entries[Text_StringCache.write_index];

    delete[] entry.buffer;
    entry.buffer = buffer;
    entry.hash = hash;
}

Uint32* Text_Utf8ToUcs4(const char* str) {
    unsigned int hash;
    Uint32* result = Text_GetCachedString(str, hash);

    if (result) {
        return result;
    }

    if (Text_CurrentFont->cd != reinterpret_cast<iconv_t>(-1)) {
        unsigned int src_len = strlen(str) + 1;
        unsigned int dst_len = src_len;
        Uint32* buffer = new (std::nothrow) Uint32[dst_len];

        if (buffer) {
            char* src_str = const_cast<char*>(str);
            char* dst_str = reinterpret_cast<char*>(buffer);

            buffer[dst_len - 1] = 0;
            dst_len *= sizeof(Uint32);

            iconv(Text_CurrentFont->cd, nullptr, nullptr, nullptr, nullptr);

            if (iconv(Text_CurrentFont->cd, &src_str, &src_len, &dst_str, &dst_len) != -1) {
                Text_AddCachedString(buffer, hash);
                result = buffer;

            } else {
                delete[] buffer;
            }
        }
    }

    return result;
}

void Text_BlitTTF(unsigned char* buf, const char* str, int swidth, int fullw, int color) {
    unsigned char* data;
    unsigned char* bstart;
    unsigned char* bnext;

    bstart = buf;

    if (color & GNW_TEXT_OUTLINE) {
        color &= ~GNW_TEXT_OUTLINE;
        Text_Blit(&buf[fullw + 1], str, swidth, fullw, (color & (~GNW_TEXT_COLOR_MASK)) | colorTable[0]);
    }

    Uint32* uni_str = Text_Utf8ToUcs4(str);

    if (uni_str) {
        for (Uint32* uni_str_pos = uni_str; *uni_str_pos; ++uni_str_pos) {
            const struct FontGlyph& glyph = Text_GetGlyph(*uni_str_pos);
            const int offset = fullw * glyph.uly + glyph.ulx;

            bnext = &buf[glyph.advance];

            if (glyph.width && glyph.height) {
                if ((intptr_t)(bnext - bstart) > swidth) {
                    break;
                }

                for (int h = 0; h < glyph.height; ++h) {
                    unsigned char mask = 0x80;
                    data = &glyph.buffer[glyph.pitch * h];

                    for (int w = 0; w < glyph.width; ++w, ++buf) {
                        if (!mask) {
                            mask = 0x80;
                            ++data;
                        }

                        if (mask & *data) {
                            buf[offset] = color;
                        }

                        mask >>= 1;
                    }

                    buf += fullw - glyph.width;
                }
            }

            buf = bnext;
        }
    }

    if (color & GNW_TEXT_UNDERLINE) {
        const int width = buf - bstart;
        buf = &bstart[fullw * (Text_GetHeight() - 1)];

        for (int i = 0; i < width; i++) {
            buf[i] = color;
        }
    }
}

int Text_GetHeightTTF(void) { return Text_CurrentFont->max_height; }

int Text_GetWidthTTF(const char* str) {
    int width = 0;
    Uint32* uni_str = Text_Utf8ToUcs4(str);

    if (uni_str) {
        for (Uint32* uni_str_pos = uni_str; *uni_str_pos; ++uni_str_pos) {
            const struct FontGlyph& glyph = Text_GetGlyph(*uni_str_pos);

            width += glyph.advance;
        }
    }

    return width;
}

int Text_GetGlyphWidthTTF(Uint32 glyph) {
    const struct FontGlyph& font_glyph = Text_GetGlyph(glyph);

    return font_glyph.advance;
}

int Text_GetMonospaceWidthTTF(const char* str) {
    int count = 0;
    Uint32* uni_str = Text_Utf8ToUcs4(str);

    if (uni_str) {
        for (Uint32* uni_str_pos = uni_str; *uni_str_pos; ++uni_str_pos) {
            ++count;
        }
    }

    return count * Text_CurrentFont->max_width;
}

int Text_GetSpacingTTF(void) { return 0; }

int Text_GetSizeTTF(const char* str) { return Text_GetWidth(str) * Text_GetHeight(); }

SmartString* Text_SplitText(const char* text, int max_row_count, int width, int* row_count) {
    SmartString string;
    SmartString* string_array;
    bool flag;

    if (max_row_count && text) {
        string_array = nullptr;
        *row_count = 0;
        flag = true;

        do {
            const char* text_copy = text;
            string.Clear();

            for (; *text_copy && *text_copy != ' ' && *text_copy != '\n'; ++text_copy) {
                if (*text_copy != '\r') {
                    string += *text_copy;
                }
            }

            int string_character_count = string.GetLength();
            int string_pixel_count = Text_GetWidth(string.GetCStr());

            if (string_pixel_count > width && string_character_count >= 4 && *row_count < (max_row_count - 1)) {
                int pixels_remaining = width;

                if (!flag) {
                    pixels_remaining -= Text_GetWidth(string_array[*row_count - 1].GetCStr()) + Text_GetGlyphWidth(' ');

                    if (*row_count < (max_row_count - 1)) {
                        int segment_pixels =
                            Text_GetGlyphWidth('-') + Text_GetGlyphWidth(string[0]) + Text_GetGlyphWidth(string[1]);

                        if (pixels_remaining < segment_pixels) {
                            pixels_remaining = width;
                        }
                    }
                }

                pixels_remaining -= Text_GetGlyphWidth('-') + Text_GetWidth(string.GetCStr());

                for (int i = string_character_count - 2; i < string_character_count; ++i) {
                    pixels_remaining += Text_GetGlyphWidth(string[i]);
                }

                string_character_count -= 2;

                while (string_character_count > 1 && pixels_remaining > 0) {
                    --string_character_count;
                    pixels_remaining += Text_GetGlyphWidth(string[string_character_count]);
                }

                if (string_character_count > 1) {
                    text_copy = &text[string_character_count];
                    string[string_character_count] = '-';
                    string[string_character_count + 1] = '\0';
                }
            }

            if (*text_copy == ' ') {
                while (*text_copy == ' ') {
                    ++text_copy;
                }

                if (*text_copy == '\r') {
                    ++text_copy;
                }

                if (*text_copy == '\n') {
                    ++text_copy;
                }
            }

            if (*text == '\r') {
                ++text;
            }

            text = text_copy;

            if (*text == '\n') {
                ++text;
            }

            if ((*row_count < max_row_count) &&
                (flag || (Text_GetWidth(string_array[*row_count - 1].GetCStr()) + Text_GetGlyphWidth(' ') +
                              Text_GetWidth(string.GetCStr()) >
                          width))) {
                SmartString* new_array = new (std::nothrow) SmartString[*row_count + 1];

                for (int i = 0; i < *row_count; ++i) {
                    new_array[i] = string_array[i];
                }

                delete[] string_array;
                string_array = new_array;

                string_array[*row_count] = string;
                ++(*row_count);

            } else {
                string_array[*row_count - 1] += ' ';
                string_array[*row_count - 1] += string;
            }

            flag = *text_copy == '\n';
        } while (*text);

    } else {
        string_array = nullptr;
    }

    return string_array;
}

void Text_TextBox(unsigned char* buffer, unsigned short length, const char* text, int ulx, int uly, int width,
                  int height, int color, bool horizontal_align, bool vertical_align) {
    int font_height = Text_GetHeight();
    int row_count;
    int max_row_count;
    SmartString* string_array;

    if (color & GNW_TEXT_OUTLINE) {
        ++font_height;
    }

    max_row_count = height / font_height;

    if (max_row_count && text) {
        string_array = Text_SplitText(text, max_row_count, width, &row_count);

        if (vertical_align) {
            uly += ((height + 1) - (font_height * row_count)) / 2;
        }

        max_row_count = row_count;

        for (row_count = 0; row_count < max_row_count; ++row_count) {
            int string_width;

            if (horizontal_align) {
                string_width = Text_GetWidth(string_array[row_count].GetCStr()) + 1;

                if (string_width > width) {
                    string_width = width;
                }

                string_width = (width - string_width) / 2;
            } else {
                string_width = 0;
            }

            Text_Blit(&buffer[(row_count * font_height + uly) * length + ulx + string_width],
                      string_array[row_count].GetCStr(), width, length, color);
        }

        delete[] string_array;
    }
}

void Text_TextBox(WindowInfo* window, const char* text, int ulx, int uly, int width, int height, bool horizontal_align,
                  bool vertical_align, FontColor color) {
    int font_height = Text_GetHeight() + 1;
    int row_count;
    int max_row_count;
    SmartString* string_array;

    max_row_count = height / font_height;

    if (max_row_count && text) {
        string_array = Text_SplitText(text, max_row_count, width, &row_count);

        if (vertical_align) {
            uly += ((height + 1) - (font_height * row_count)) / 2;
        }

        max_row_count = row_count;

        for (row_count = 0; row_count < max_row_count; ++row_count) {
            Text_TextLine(window, string_array[row_count].GetCStr(), ulx, row_count * font_height + uly, width,
                          horizontal_align, color);
        }

        delete[] string_array;
    }
}

void Text_TextLine(WindowInfo* window, const char* str, int ulx, int uly, int swidth, bool horizontal_align,
                   FontColor color) {
    const int text_width = Text_GetWidth(str) + 1;
    const int text_height = Text_GetHeight() + 1;

    if (horizontal_align) {
        int spacing = (swidth - text_width) / 2;

        if (spacing > 0) {
            ulx += spacing;
        }
    }

    const int buffer_size = text_width * text_height;
    unsigned char* buffer = new (std::nothrow) unsigned char[buffer_size];
    memset(buffer, colorTable[0], buffer_size);

    Text_Blit(buffer, str, swidth, text_width, color.base);

    for (int y = text_height - 2; y >= 0; --y) {
        for (int x = 0; x < text_width; ++x) {
            if (buffer[text_width * y + x] == color.base) {
                if (buffer[text_width * (y + 1) + x + 1] == color.base) {
                    buffer[text_width * (y + 1) + x + 1] = color.outline;

                } else {
                    buffer[text_width * (y + 1) + x + 1] = color.shadow;
                }
            }
        }
    }

    trans_buf_to_buf(buffer, text_width, text_height, text_width, &window->buffer[ulx + window->width * uly],
                     window->width);

    delete[] buffer;
}

void Text_TypeWriter_TextBox(WindowInfo* window, const char* text, int ulx, int uly, int width, int alignment) {
    int width_text;
    int text_position;
    unsigned int initial_time_stamp;
    unsigned int time_stamp;
    char character[2];

    width_text = Text_GetWidth(text);

    if (alignment == 1 && width_text < width) {
        ulx += width - width_text;
    } else if (alignment == 2 && width_text < width) {
        ulx += (width - width_text) / 2;
    }

    text_position = 0;
    character[1] = '\0';
    initial_time_stamp = timer_get();
    time_stamp = timer_get();

    if (Text_TypeWriter_CharacterTimeMs > 0) {
        SoundManager.PlaySfx(MBUTT0);
    }

    while (text[text_position] && width > 0) {
        Rect bounds;
        int character_width;

        character[0] = text[text_position];
        Text_TextLine(window, character, ulx, uly, width, false, Fonts_BrightSilverColor);

        character_width = Text_GetWidth(character);
        width -= character_width;

        bounds.ulx = ulx;
        bounds.uly = uly;
        bounds.lrx = ulx + character_width;
        bounds.lry = uly + Text_GetHeight();

        ulx += character_width;
        ++text_position;

        win_draw_rect(window->id, &bounds);

        if (Text_TypeWriter_CharacterTimeMs > 0) {
            if (timer_elapsed_time(time_stamp) >= Text_TypeWriter_BeepTimeMs) {
                SoundManager.PlaySfx(MBUTT0);
                time_stamp = timer_get();
            }

            for (;;) {
                if (timer_elapsed_time(initial_time_stamp) >= (Text_TypeWriter_CharacterTimeMs * text_position)) {
                    break;
                }

                if (get_input() != -1) {
                    Text_TypeWriter_CharacterTimeMs = 0;
                }
            }
        }
    }
}

void Text_TypeWriter_TextBoxMultiLineWrapText(WindowInfo* window, const char* text, int ulx, int uly, int width,
                                              int height, int alignment) {
    int font_height;
    int max_row_count;
    int row_count;
    SmartString* strings;

    font_height = Text_GetHeight() + 1;
    max_row_count = height / font_height;

    if (max_row_count && text) {
        strings = Text_SplitText(text, max_row_count, width, &row_count);

        uly += (height + 1 - row_count * font_height) / 2;

        for (int i = 0; i < row_count; ++i) {
            Text_TypeWriter_TextBox(window, strings[i].GetCStr(), ulx, uly + i * font_height, width, alignment);
        }

        delete[] strings;
    }
}

bool Text_FitBounds(Rect* output, Rect* bounds1, Rect* bounds2) {
    if (bounds1->ulx <= bounds2->ulx) {
        output->ulx = bounds2->ulx;

    } else {
        output->ulx = bounds1->ulx;
    }
    if (bounds1->uly <= bounds2->uly) {
        output->uly = bounds2->uly;

    } else {
        output->uly = bounds1->uly;
    }

    if (bounds1->lrx >= bounds2->lrx) {
        output->lrx = bounds2->lrx;

    } else {
        output->lrx = bounds1->lrx;
    }

    if (bounds1->lry >= bounds2->lry) {
        output->lry = bounds2->lry;

    } else {
        output->lry = bounds1->lry;
    }

    return output->lrx > output->ulx && output->lry > output->uly;
}

bool Text_IsFitting(Rect* bounds1, Rect* bounds2) {
    return bounds1->ulx <= bounds2->ulx && bounds1->lrx >= bounds2->lrx && bounds1->uly <= bounds2->uly &&
           bounds1->lry >= bounds2->lry;
}

void Text_AutofitTextBox(unsigned char* buffer, unsigned short pitch, const char* text, Rect* text_area,
                         Rect* draw_area, int color, bool horizontal_align) {
    Rect render_area;

    if (Text_FitBounds(&render_area, text_area, draw_area)) {
        if (Text_IsFitting(&render_area, text_area)) {
            Text_TextBox(buffer, pitch, text, text_area->ulx, text_area->uly, text_area->lrx - text_area->ulx,
                         text_area->lry - text_area->uly, color, horizontal_align);

        } else {
            int render_width;
            int render_height;
            int text_width;
            int text_height;
            unsigned char* text_buffer;
            unsigned char* target_buffer;

            render_width = render_area.lrx - render_area.ulx;
            render_height = render_area.lry - render_area.uly;
            text_width = text_area->lrx - text_area->ulx;
            text_height = text_area->lry - text_area->uly;

            text_buffer = new (std::nothrow) unsigned char[text_width * text_height];
            buffer = &buffer[pitch * render_area.uly + render_area.ulx];
            target_buffer =
                &text_buffer[(render_area.uly - text_area->uly) * text_width + (render_area.ulx - text_area->ulx)];

            buf_to_buf(buffer, render_width, render_height, pitch, target_buffer, text_width);

            Text_TextBox(text_buffer, text_width, text, 0, 0, text_width, text_height, color, horizontal_align);

            buf_to_buf(target_buffer, render_width, render_height, text_width, buffer, pitch);

            delete[] text_buffer;
        }
    }
}
