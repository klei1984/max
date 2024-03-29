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

#ifndef OPTIONSMENU_HPP
#define OPTIONSMENU_HPP

#include "button.hpp"
#include "smartstring.hpp"
#include "textedit.hpp"
#include "window.hpp"

class OptionsMenu : public Window {
    WindowInfo window;
    Button *button_cancel;
    Button *button_done;
    Button *button_help;
    uint16_t team;
    ResourceID bg_image;
    TextEdit *text_edit;
    uint16_t control_id;
    uint16_t button_count;
    bool exit_menu;
    bool is_slider_active;
    bool event_release;

    void Init();
    void InitSliderControl(int32_t id, int32_t ulx, int32_t uly);
    void InitEditControl(int32_t id, int32_t ulx, int32_t uly);
    void InitCheckboxControl(int32_t id, int32_t ulx, int32_t uly);
    void InitLabelControl(int32_t id, int32_t ulx, int32_t uly);
    void DrawSlider(int32_t id, int32_t value);
    void UpdateSlider(int32_t id);
    void SetVolume(int32_t id, int32_t audio_type, int32_t value);
    int32_t ProcessTextEditKeyPress(int32_t key);
    int32_t ProcessKeyPress(int32_t key);

public:
    OptionsMenu(uint16_t team, ResourceID bg_image);
    ~OptionsMenu();

    void Run();
};

#endif /* OPTIONSMENU_HPP */
