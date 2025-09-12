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
#include "inifile.hpp"
#include "smartstring.hpp"
#include "textedit.hpp"
#include "window.hpp"

enum OptionsType {
    OPTIONS_TYPE_SECTION,
    OPTIONS_TYPE_SLIDER,
    OPTIONS_TYPE_EDIT_INT,
    OPTIONS_TYPE_EDIT_HEX,
    OPTIONS_TYPE_EDIT_STR,
    OPTIONS_TYPE_CHECKBOX,
    OPTIONS_TYPE_LABEL,
};

#define OPTIONS_BUTTON_COUNT 33
#define OPTIONS_PLAY_MODE_ITEM_COUNT 2
#define OPTIONS_OPPONENT_ITEM_COUNT 6
#define OPTIONS_VICTORY_TYPE_ITEM_COUNT 2

#define OPTIONS_BUTTON_DEF(type, caption, ini_param_index, ulx, range_min, range_max) \
    {(type), (caption), (ini_param_index), (ulx), (range_min), (range_max), nullptr, nullptr, nullptr, 0, 0}

struct OptionsButton {
    char type;
    const char *format;
    IniParameter ini_param_index;
    int16_t ulx;
    int16_t range_min;
    int16_t range_max;
    Button *button;
    Image *image;
    char *ini_string_value;
    int16_t rest_state;
    int16_t last_rest_state;
};

class OptionsMenu : public Window {
    const char *options_menu_play_mode_strings[OPTIONS_PLAY_MODE_ITEM_COUNT] = {_(d0a7), _(ee25)};
    const char *options_menu_opponent_strings[OPTIONS_OPPONENT_ITEM_COUNT] = {_(a24d), _(37b7), _(bb85),
                                                                              _(b663), _(ab89), _(c7e3)};
    const char *options_menu_victory_type_strings[OPTIONS_VICTORY_TYPE_ITEM_COUNT] = {_(b542), _(c303)};

    struct OptionsButton options_menu_buttons[OPTIONS_BUTTON_COUNT] = {
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, INI_SETUP, 0, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, _(b2c7), INI_MUSIC_LEVEL, 25, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(644b), INI_ENHANCED_GRAPHICS, 210, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, _(6eb0), INI_MUSIC_LEVEL, 25, 0, 100),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(c2e5), INI_DISABLE_MUSIC, 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, _(adf0), INI_FX_SOUND_LEVEL, 25, 0, 100),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(2eeb), INI_DISABLE_FX, 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, _(87bf), INI_VOICE_LEVEL, 25, 0, 100),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(e9db), INI_DISABLE_VOICE, 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(64fb), INI_AUTO_SAVE, 25, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_HEX, _(5940), INI_IPX_SOCKET, 210, 0, 0x7FFF),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_STR, _(f11f), INI_PLAYER_NAME, 25, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, INI_PREFERENCES, 0, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(238c), INI_EFFECTS, 25, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(bf3b), INI_CLICK_SCROLL, 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(2349), INI_FAST_MOVEMENT, 25, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(09e9), INI_FOLLOW_UNIT, 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(f99e), INI_ENEMY_HALT, 25, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(c8b2), INI_AUTO_SELECT, 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, _(24e6), INI_QUICK_SCROLL, 25, 4, 128),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, INI_OPTIONS, 0, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(86df), INI_TIMER, 25, 0, 32767),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(de3f), INI_ENDTURN, 210, 0, 32767),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, _(f019), INI_PLAY_MODE, 25, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, _(d5b2), INI_OPPONENT, 25, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, _(e791), INI_VICTORY_LIMIT, 25, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, INI_INVALID_ID, 0, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(d5fa), INI_DISABLE_FIRE, 25, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(6a65), INI_REAL_TIME, 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(3bda), INI_RED_TEAM_PLAYER, 25, 0, 3),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(3bb7), INI_GREEN_TEAM_PLAYER, 210, 0, 3),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(11a8), INI_BLUE_TEAM_PLAYER, 25, 0, 3),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(75a0), INI_GRAY_TEAM_PLAYER, 210, 0, 3),
    };

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
