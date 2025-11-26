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

#ifndef TEXTEDIT_HPP
#define TEXTEDIT_HPP

#include "image.hpp"

enum {
    TEXTEDIT_MODE_STR,
    TEXTEDIT_MODE_INT,
    TEXTEDIT_MODE_HEX,
};

class TextEdit {
    WindowInfo window;
    Image* bg_image;
    char* approved_text;
    char* edited_text;
    char* text_before_cursor;
    uint16_t cursor_position;
    uint16_t buffer_size;
    uint16_t color;
    bool cursor_blink;
    bool is_selected;
    bool is_being_edited;
    uint16_t font_num;
    uint8_t mode;
    uint64_t time_stamp;

    void DrawTillCursor();
    void Clear();
    void SetCursorPosition(int32_t position);
    void Delete();
    void Backspace();
    void InsertCharacter(char character);
    void InsertUTF8Text(const char* utf8_text);

public:
    TextEdit(WindowInfo* window, char* text, int32_t buffer_size, int32_t ulx, int32_t uly, int32_t width,
             int32_t height, uint16_t color, int32_t font_num);
    ~TextEdit();

    void SetMode(int32_t mode);
    void SetEditedText(const char* text);
    void LoadBgImage();
    void EnterTextEditField();
    void LeaveTextEditField();
    void DrawFullText(int32_t refresh_screen = true);
    void UpdateWindow(WindowInfo* window);
    void AcceptEditedText();
    int32_t ProcessKeyPress(int32_t key);
    int32_t ProcessUTF8Input(const char* utf8_text);
};

#endif /* TEXTEDIT_HPP */
