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
    Image *bg_image;
    char *approved_text;
    char *edited_text;
    char *text_before_cursor;
    unsigned short cursor_position;
    unsigned short buffer_size;
    unsigned short color;
    bool cursor_blink;
    bool is_selected;
    bool is_being_edited;
    unsigned short font_num;
    unsigned char mode;
    unsigned int time_stamp;

    void DrawTillCursor();
    void Clear();
    void SetCursorPosition(int position);
    void Delete();
    void Backspace();
    void InsertCharacter(char character);

public:
    TextEdit(WindowInfo *window, char *text, int buffer_size, int ulx, int uly, int width, int height,
             unsigned short color, int font_num);
    ~TextEdit();

    void SetMode(int mode);
    void SetEditedText(const char *text);
    void LoadBgImage();
    void EnterTextEditField();
    void LeaveTextEditField();
    void DrawFullText(int refresh_screen = true);
    void UpdateWindow(WindowInfo *window);
    void AcceptEditedText();
    int ProcessKeyPress(int key);
};

#endif /* TEXTEDIT_HPP */
