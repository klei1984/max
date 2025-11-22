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

#include <cctype>
#include <cstring>
#include <new>

#include "input.h"
#include "utf8.hpp"

/* Active TextEdit instance for UTF-8 input routing */
static TextEdit* g_active_text_edit = nullptr;

/* Static callback wrapper for UTF-8 input */
static int32_t TextEdit_UTF8InputCallback(const char* utf8_text) {
    return (g_active_text_edit) ? g_active_text_edit->ProcessUTF8Input(utf8_text) : 0;
}

TextEdit::TextEdit(WindowInfo* window, char* text, int32_t buffer_size, int32_t ulx, int32_t uly, int32_t width,
                   int32_t height, uint16_t color, int32_t font_num)
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
    Text_SetFont(this->font_num);

    uly += (height - Text_GetHeight()) / 2;

    this->window.buffer = &window->buffer[window->width * uly + ulx];

    this->window.window.ulx += ulx;
    this->window.window.uly += uly;

    this->window.window.lrx = this->window.window.ulx + width;
    this->window.window.lry = this->window.window.uly + Text_GetHeight();

    edited_text = new (std::nothrow) char[buffer_size];
    text_before_cursor = new (std::nothrow) char[buffer_size];

    strcpy(edited_text, text);

    bg_image = new (std::nothrow) Image(0, 0, width, Text_GetHeight());
}

TextEdit::~TextEdit() {
    LeaveTextEditField();

    delete bg_image;
    delete[] edited_text;
    delete[] text_before_cursor;
}

void TextEdit::SetEditedText(const char* text) { SDL_utf8strlcpy(edited_text, text, buffer_size); }

void TextEdit::LoadBgImage() { bg_image->Copy(&window); }

void TextEdit::EnterTextEditField() {
    if (!is_being_edited) {
        time_stamp = timer_get();

        is_being_edited = true;

        /* Register this TextEdit instance for UTF-8 input */
        g_active_text_edit = this;

        GNW_register_utf8_input(TextEdit_UTF8InputCallback);
    }

    cursor_blink = false;
    is_selected = true;
    cursor_position = strlen(edited_text);

    DrawFullText();
}

void TextEdit::SetMode(int32_t mode) { this->mode = mode; }

void TextEdit::LeaveTextEditField() {
    if (is_being_edited) {
        is_being_edited = false;

        /* Unregister UTF-8 input callback */
        if (g_active_text_edit == this) {
            g_active_text_edit = nullptr;

            GNW_unregister_utf8_input();
        }

        if (cursor_blink) {
            cursor_blink = false;

            DrawTillCursor();
        }

        DrawFullText();
    }
}

void TextEdit::UpdateWindow(WindowInfo* window) {
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

void TextEdit::DrawFullText(int32_t refresh_screen) {
    int32_t color;

    Text_SetFont(font_num);

    bg_image->Write(&window);

    if (is_being_edited && is_selected) {
        color = 0xFF;

    } else {
        color = this->color;
    }

    Text_Blit(window.buffer, edited_text, window.window.lrx - window.window.ulx, window.width, color);

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

        Text_SetFont(font_num);

        bounds.ulx = Text_GetWidth(text_before_cursor);
        bounds.lrx = bounds.ulx + Text_GetWidth("|");
        bounds.uly = 0;
        bounds.lry = Text_GetHeight();

        Text_Blit(&window.buffer[bounds.ulx], "|", bounds.lrx - bounds.ulx, window.width, color);

        bounds.ulx += window.window.ulx;
        bounds.lrx += window.window.ulx;
        bounds.uly += window.window.uly;
        bounds.lry += window.window.uly;

        win_draw_rect(window.id, &bounds);
    } else {
        DrawFullText();
    }
}

void TextEdit::SetCursorPosition(int32_t position) {
    is_selected = false;
    cursor_blink = false;

    DrawTillCursor();

    if (position < 0) {
        position = 0;
    }

    /* Note: cursor_position is stored as byte offset, not codepoint index */
    size_t text_byte_size = strlen(edited_text);

    if (static_cast<size_t>(position) > text_byte_size) {
        position = text_byte_size;
    }

    cursor_position = position;
    cursor_blink = true;

    DrawTillCursor();
}

void TextEdit::Delete() {
    if (is_selected) {
        Clear();

    } else {
        size_t text_size = strlen(edited_text);

        if (cursor_position < text_size) {
            cursor_blink = false;

            DrawTillCursor();

            size_t next_char_offset = utf8_next_char_offset(edited_text, cursor_position);

            strncpy(text_before_cursor, edited_text, cursor_position);

            text_before_cursor[cursor_position] = '\0';

            strcat(text_before_cursor, &edited_text[next_char_offset]);

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
        cursor_position = utf8_prev_char_offset(edited_text, cursor_position);

        Delete();
    }
}

void TextEdit::InsertCharacter(char character) {
    if (is_selected) {
        Clear();

        is_selected = false;
    }

    if ((mode != TEXTEDIT_MODE_INT || isdigit(character)) && (mode != TEXTEDIT_MODE_HEX || isxdigit(character))) {
        if (strlen(edited_text) < static_cast<uint32_t>(buffer_size - 1)) {
            cursor_blink = false;

            DrawTillCursor();

            strncpy(text_before_cursor, edited_text, cursor_position);

            text_before_cursor[cursor_position] = character;
            text_before_cursor[cursor_position + 1] = '\0';

            strcat(text_before_cursor, &edited_text[cursor_position]);

            if ((Text_GetWidth(text_before_cursor) + Text_GetWidth("|")) <= (window.window.lrx - window.window.ulx)) {
                strcpy(edited_text, text_before_cursor);

                DrawFullText();

                ++cursor_position;

                cursor_blink = false;

                DrawTillCursor();
            }
        }
    }
}

void TextEdit::InsertUTF8Text(const char* utf8_text) {
    if (!utf8_text || !*utf8_text) {
        return;
    }

    if (is_selected) {
        Clear();
        is_selected = false;
    }

    /* Calculate UTF-8 string length in bytes */
    size_t utf8_len = strlen(utf8_text);
    size_t current_len = strlen(edited_text);

    /* Check if we have space (accounting for null terminator) */
    if (current_len + utf8_len < static_cast<size_t>(buffer_size - 1)) {
        cursor_blink = false;
        DrawTillCursor();

        /* Build new string: text before cursor + utf8 text + text after cursor */
        strncpy(text_before_cursor, edited_text, cursor_position);
        text_before_cursor[cursor_position] = '\0';
        strcat(text_before_cursor, utf8_text);
        strcat(text_before_cursor, &edited_text[cursor_position]);

        /* Check if text fits in visible area */
        if ((Text_GetWidth(text_before_cursor) + Text_GetWidth("|")) <= (window.window.lrx - window.window.ulx)) {
            strcpy(edited_text, text_before_cursor);
            DrawFullText();

            /* Move cursor past the inserted UTF-8 text */
            cursor_position += utf8_len;

            cursor_blink = false;
            DrawTillCursor();
        }
    }
}

int32_t TextEdit::ProcessKeyPress(int32_t key) {
    int32_t result;

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
                size_t next_pos = utf8_next_char_offset(edited_text, cursor_position);

                SetCursorPosition(next_pos);

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
                size_t prev_pos = utf8_prev_char_offset(edited_text, cursor_position);

                SetCursorPosition(prev_pos);

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
                /* Don't handle printable characters here - SDL_TEXTINPUT handles them */
                /* This prevents double input: key press + text input event */

                result = false;

            } break;
        }

    } else {
        result = false;
    }

    return result;
}

int32_t TextEdit::ProcessUTF8Input(const char* utf8_text) {
    int32_t result = false;

    if (is_being_edited && utf8_text && *utf8_text) {
        if (timer_elapsed_time(time_stamp) >= 500) {
            cursor_blink = !cursor_blink;

            DrawTillCursor();

            time_stamp = timer_get();
        }

        if (mode == TEXTEDIT_MODE_STR) {
            InsertUTF8Text(utf8_text);

            result = true;

        } else if (mode == TEXTEDIT_MODE_INT || mode == TEXTEDIT_MODE_HEX) {
            if (strlen(utf8_text) == 1) {
                char ch = utf8_text[0];

                if ((mode == TEXTEDIT_MODE_INT && isdigit(ch)) || (mode == TEXTEDIT_MODE_HEX && isxdigit(ch))) {
                    InsertCharacter(ch);

                    result = true;
                }
            }
        }
    }

    return result;
}
