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

#include "textedit.hpp"

/* Convert key codes 128-255 to code page 437 characters */
static const short TextEdit_KeyToAsciiMap[] = {
    '\0', '\0', '\0', 159,  '\0', '\0', '\0', '\0', 94,   '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', 45,   95,   126,  '\0', '\0', '\0', '\0', '\0', '\0', 152,  '\0', 173,  155,  156,  214,  157,
    124,  213,  215,  '\0', 166,  174,  170,  '\0', '\0', '\0', 248,  241,  '\0', '\0', 216,  230,  20,   250,  '\0',
    '\0', 248,  175,  172,  171,  '\0', 168,  192,  193,  194,  195,  142,  143,  146,  128,  196,  144,  197,  198,
    217,  199,  200,  201,  '\0', 165,  202,  203,  204,  205,  153,  218,  '\0', 206,  207,  208,  154,  209,  '\0',
    225,  133,  160,  131,  210,  132,  134,  145,  135,  138,  130,  136,  137,  141,  161,  140,  139,  '\0', 164,
    149,  162,  147,  211,  148,  246,  237,  151,  163,  150,  129,  212,  '\0', '\0',
};

static_assert(sizeof(TextEdit_KeyToAsciiMap) == 128 * sizeof(const short));

TextEdit::TextEdit(WindowInfo *window, char *text, int buffer_size, int ulx, int uly, int width, int height,
                   unsigned short color, int font_num)
    : font_num(font_num),
      window(*window),
      buffer_size(buffer_size),
      approved_text(text),
      is_being_edited(false),
      cursor_blink(false),
      color(color),
      is_selected(true),
      mode(0),
      cursor_position(0),
      time_stamp(0) {
    text_font(this->font_num);
    uly += (height - text_height()) / 2;

    this->window.buffer = &window->buffer[window->width * uly + ulx];

    this->window.window.ulx += ulx;
    this->window.window.uly += uly;

    this->window.window.lrx = this->window.window.ulx + width;
    this->window.window.lry = this->window.window.uly + text_height();

    edited_text = new (std::nothrow) char[buffer_size];
    text_before_cursor = new (std::nothrow) char[buffer_size];

    strcpy(edited_text, text);
    bg_image = new (std::nothrow) Image(0, 0, width, text_height());
}

TextEdit::~TextEdit() {
    LeaveTextEditField();

    delete bg_image;
    delete[] edited_text;
    delete[] text_before_cursor;
}

void TextEdit::SetEditedText(const char *text) { strcpy(edited_text, text); }

void TextEdit::LoadBgImage() { bg_image->Copy(&window); }

void TextEdit::EnterTextEditField() {
    if (!is_being_edited) {
        time_stamp = timer_get();

        is_being_edited = true;
    }

    cursor_blink = false;
    is_selected = true;
    cursor_position = strlen(edited_text);

    DrawFullText();
}

void TextEdit::SetMode(int mode) { this->mode = mode; }

void TextEdit::LeaveTextEditField() {
    if (is_being_edited) {
        is_being_edited = false;

        if (cursor_blink) {
            cursor_blink = false;
            DrawTillCursor();
        }

        DrawFullText();
    }
}

void TextEdit::UpdateWindow(WindowInfo *window) {
    this->window.buffer = &window->buffer[this->window.window.ulx - window->window.ulx +
                                          (this->window.window.uly - window->window.uly) * window->width];
    this->window.width = window->width;
}

void TextEdit::Clear() {
    cursor_position = 0;
    bg_image->Write(&window);
    edited_text[0] = '\0';

    win_draw_rect(window.id, &window.window);
}

void TextEdit::DrawFullText(int refresh_screen) {
    int color;

    text_font(font_num);
    bg_image->Write(&window);

    if (is_being_edited && is_selected) {
        color = 0xFF;
    } else {
        color = this->color;
    }

    text_to_buf(window.buffer, edited_text, window.window.lrx - window.window.ulx, window.width, color);

    if (refresh_screen) {
        win_draw_rect(window.id, &window.window);
    }
}

void TextEdit::AcceptEditedText() { strcpy(approved_text, edited_text); }

void TextEdit::DrawTillCursor() {
    if (cursor_blink) {
        Rect bounds;

        strncpy(text_before_cursor, edited_text, cursor_position);

        text_before_cursor[cursor_position] = '\0';

        text_font(font_num);

        bounds.ulx = text_width(text_before_cursor);
        bounds.lrx = bounds.ulx + text_width("|");
        bounds.uly = 0;
        bounds.lry = text_height();

        text_to_buf(&window.buffer[bounds.ulx], "|", bounds.lrx - bounds.ulx, window.width, color);

        bounds.ulx += window.window.ulx;
        bounds.lrx += window.window.ulx;
        bounds.uly += window.window.uly;
        bounds.lry += window.window.uly;

        win_draw_rect(window.id, &bounds);
    } else {
        DrawFullText();
    }
}

void TextEdit::SetCursorPosition(int position) {
    is_selected = false;
    cursor_blink = false;
    int text_size;

    DrawTillCursor();

    if (position < 0) {
        position = 0;
    }

    text_size = strlen(edited_text);

    if (position > text_size) {
        position = text_size;
    }

    cursor_position = position;
    cursor_blink = true;

    DrawTillCursor();
}

void TextEdit::Delete() {
    if (is_selected) {
        Clear();
    } else {
        int text_size;

        text_size = strlen(edited_text);

        if (cursor_position != text_size) {
            cursor_blink = false;
            DrawTillCursor();

            strncpy(text_before_cursor, edited_text, cursor_position);
            text_before_cursor[cursor_position] = '\0';

            strcat(text_before_cursor, &edited_text[cursor_position + 1]);

            strcpy(edited_text, text_before_cursor);

            DrawFullText();

            cursor_blink = true;
            DrawTillCursor();
        }
    }
}

void TextEdit::Backspace() {
    if (is_selected) {
        Clear();
    } else if (cursor_position) {
        --cursor_position;
        Delete();
    }
}

void TextEdit::InsertCharacter(char character) {
    if (is_selected) {
        Clear();

        is_selected = false;
    }

    if ((mode != 1 || isdigit(character)) && (mode != 2 || isxdigit(character))) {
        if (strlen(edited_text) < (buffer_size - 1)) {
            cursor_blink = false;
            DrawTillCursor();
            strncpy(text_before_cursor, edited_text, cursor_position);
            text_before_cursor[cursor_position] = character;
            text_before_cursor[cursor_position + 1] = '\0';

            strcat(text_before_cursor, &edited_text[cursor_position]);

            if ((text_width(text_before_cursor) + text_width("|")) <= (window.window.lrx - window.window.ulx)) {
                strcpy(edited_text, text_before_cursor);

                DrawFullText();
                ++cursor_position;

                cursor_blink = false;
                DrawTillCursor();
            }
        }
    }
}

int TextEdit::ProcessKeyPress(int key) {
    int result;

    if (is_being_edited) {
        if (timer_elapsed_time(time_stamp) >= 500) {
            cursor_blink = !cursor_blink;
            DrawTillCursor();
            time_stamp = timer_get();
        }

        switch (key) {
            case GNW_KB_KEY_HOME:
            case GNW_KB_KEY_PAGEUP: {
                SetCursorPosition(0);
                result = true;

            } break;

            case GNW_KB_KEY_END:
            case GNW_KB_KEY_PAGEDOWN: {
                SetCursorPosition(strlen(edited_text));
                result = true;

            } break;

            case GNW_KB_KEY_RIGHT:
            case GNW_KB_KEY_DOWN: {
                SetCursorPosition(cursor_position + 1);
                result = true;

            } break;

            case GNW_KB_KEY_INSERT: {
                result = true;

            } break;

            case GNW_KB_KEY_DELETE: {
                Delete();
                result = true;

            } break;

            case GNW_KB_KEY_LEFT:
            case GNW_KB_KEY_UP: {
                SetCursorPosition(cursor_position - 1);
                result = true;

            } break;

            case GNW_KB_KEY_CTRL_X: {
                Clear();
                result = true;

            } break;

            case GNW_KB_KEY_ESCAPE: {
                strcpy(edited_text, approved_text);
                LeaveTextEditField();
                result = true;

            } break;

            case GNW_KB_KEY_BACKSPACE: {
                Backspace();
                result = true;

            } break;

            case GNW_KB_KEY_RETURN: {
                strcpy(approved_text, edited_text);
                LeaveTextEditField();
                result = true;

            } break;

            default: {
                if (key < 0) {
                    result = false;

                } else {
                    if (key >= 128 && key <= 255) {
                        key = TextEdit_KeyToAsciiMap[key - 128];
                    }

                    if (key >= 32 && key <= 255) {
                        InsertCharacter(key);
                        result = true;

                    } else {
                        result = false;
                    }
                }

            } break;
        }
    } else {
        result = false;
    }

    return result;
}
