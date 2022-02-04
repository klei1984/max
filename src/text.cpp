/* Copyright (c) 2021 M.A.X. Port Team
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

#include <new>

#include "point.hpp"
#include "sound_manager.hpp"

unsigned int Text_TypeWriter_CharacterTimeMs = 50;
unsigned int Text_TypeWriter_BeepTimeMs = 100;

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
            int string_pixel_count = text_width(string.GetCStr());

            if (string_pixel_count > width && string_character_count >= 4 && *row_count < (max_row_count - 1)) {
                int pixels_remaining = width;

                if (!flag) {
                    pixels_remaining -= text_width(string_array[*row_count].GetCStr()) + text_char_width(' ');

                    if (*row_count < (max_row_count - 1)) {
                        int segment_pixels =
                            text_char_width('-') + text_char_width(string[0]) + text_char_width(string[1]);

                        if (pixels_remaining < segment_pixels) {
                            pixels_remaining = width;
                        }
                    }
                }

                pixels_remaining -= text_char_width('-') + text_width(string.GetCStr());

                for (int i = string_character_count - 2; i < string_character_count; ++i) {
                    pixels_remaining += text_char_width(string[i]);
                }

                string_character_count -= 2;

                while (string_character_count > 1 && pixels_remaining > 0) {
                    --string_character_count;
                    pixels_remaining += text_char_width(string[string_character_count]);
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

            if ((*row_count < max_row_count) && (flag || (text_width(string_array[*row_count - 1].GetCStr()) +
                                                              text_char_width(' ') + text_width(string.GetCStr()) >
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
    int font_height = text_height();
    int row_count;
    int max_row_count;
    SmartString* string_array;

    if (color & 0x10000) {
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
                string_width = text_width(string_array[row_count].GetCStr());

                if (string_width > width) {
                    string_width = width;
                }

                string_width = (width - string_width) / 2;
            } else {
                string_width = 0;
            }

            text_to_buf(&buffer[(row_count * font_height + uly) * length + ulx + string_width],
                        string_array[row_count].GetCStr(), width, length, color);
        }

        delete[] string_array;
    }
}

void Text_TextBox(WindowInfo* window, const char* text, int ulx, int uly, int width, int height, bool horizontal_align,
                  bool vertical_align, FontColor color) {
    int font_height = text_height() + 1;
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

void Text_TextLine(WindowInfo* window, const char* text, int ulx, int uly, int width, bool horizontal_align,
                   FontColor color) {
    int font_height = text_height() + 1;
    int font_width = text_max() + 1;
    int buffer_size;
    Color* buffer;
    Point point;

    if (horizontal_align) {
        int spacing = (width - text_width(const_cast<char*>(text))) / 2;

        if (spacing > 0) {
            ulx += spacing;
        }
    }

    buffer_size = font_height * font_width;
    buffer = new (std::nothrow) Color[buffer_size];

    for (int i = 0; text[i] && width > 0; ++i) {
        char character[2];
        int character_width;

        character[0] = text[i];
        character[1] = '\0';

        character_width = text_char_width(character[0]);

        width -= character_width;

        if (width >= 0) {
            memset(buffer, 0, buffer_size);
            text_to_buf(buffer, character, font_width, font_width, color.base);

            for (point.y = text_height() - 1; point.y >= 0; --point.y) {
                Color* address_1;
                Color* address_2;

                address_1 = &buffer[font_width * point.y];
                address_2 = &address_1[font_width + 1];

                for (point.x = 0; point.x < character_width; ++point.x) {
                    if (address_1[point.x] == color.base) {
                        if (address_2[point.x] == color.base) {
                            address_2[point.x] = color.outline;
                        } else {
                            address_2[point.x] = color.shadow;
                        }
                    }
                }
            }

            trans_buf_to_buf(buffer, character_width + 1, font_height, font_width,
                             &window->buffer[ulx + window->width * uly], window->width);
            ulx += character_width;
        }
    }

    delete[] buffer;
}

void Text_TypeWriter_TextBox(WindowInfo* window, char* text, int ulx, int uly, int width, int alignment) {
    int width_text;
    int text_position;
    unsigned int initial_time_stamp;
    unsigned int time_stamp;
    char character[2];

    width_text = text_width(text);

    if (alignment == 1 && width_text < width) {
        ulx += width - width_text;
    } else if (alignment == 2 && width_text < width) {
        ulx += (width - width_text) / 2;
    }

    text_position = 0;
    character[1] = '\0';
    initial_time_stamp = timer_get_stamp32();
    time_stamp = timer_get_stamp32();

    if (Text_TypeWriter_CharacterTimeMs > 0) {
        soundmgr.PlaySfx(MBUTT0);
    }

    while (text[text_position] && width > 0) {
        Rect bounds;
        int character_width;

        character[0] = text[text_position];
        Text_TextLine(window, character, ulx, uly, width, false, Fonts_BrightSilverColor);

        character_width = text_width(character);
        width -= character_width;

        bounds.ulx = ulx;
        bounds.uly = uly;
        bounds.lrx = ulx + character_width;
        bounds.lry = uly + text_height();

        ulx += character_width;
        ++text_position;

        win_draw_rect(window->id, &bounds);

        if (Text_TypeWriter_CharacterTimeMs > 0) {
            if (timer_elapsed_time_ms(time_stamp) >= Text_TypeWriter_BeepTimeMs) {
                soundmgr.PlaySfx(MBUTT0);
                time_stamp = timer_get_stamp32();
            }

            for (;;) {
                if (timer_elapsed_time_ms(initial_time_stamp) >= (Text_TypeWriter_CharacterTimeMs * text_position)) {
                    break;
                }

                if (get_input() != -1) {
                    Text_TypeWriter_CharacterTimeMs = 0;
                }
            }
        }
    }
}

void Text_TypeWriter_TextBoxMultiLineWrapText(WindowInfo* window, char* text, int ulx, int uly, int width, int height,
                                              int alignment) {
    int font_height;
    int max_row_count;
    int row_count;
    SmartString* strings;

    font_height = text_height() + 1;
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
