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

#include <string>

#include "button.hpp"
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
    {(type), (caption), (ini_param_index), (ulx), (range_min), (range_max), nullptr, nullptr, "", 0, 0}

struct OptionsButton {
    char type;
    const char* format;
    std::string setting_key;
    int16_t ulx;
    int16_t range_min;
    int16_t range_max;
    Button* button;
    Image* image;
    std::string ini_string_value;
    int16_t rest_state;
    int16_t last_rest_state;
};

class OptionsMenu : public Window {
    const char* options_menu_play_mode_strings[OPTIONS_PLAY_MODE_ITEM_COUNT] = {_(d0a7), _(ee25)};
    const char* options_menu_opponent_strings[OPTIONS_OPPONENT_ITEM_COUNT] = {_(a24d), _(37b7), _(bb85),
                                                                              _(b663), _(ab89), _(c7e3)};
    const char* options_menu_victory_type_strings[OPTIONS_VICTORY_TYPE_ITEM_COUNT] = {_(b542), _(c303)};

    struct OptionsButton options_menu_buttons[OPTIONS_BUTTON_COUNT] = {
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, "setup", 0, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, _(b2c7), "music_level", 25, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(644b), "enhanced_graphics", 210, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, _(6eb0), "music_level", 25, 0, 100),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(c2e5), "disable_music", 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, _(adf0), "fx_sound_level", 25, 0, 100),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(2eeb), "disable_fx", 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, _(87bf), "voice_level", 25, 0, 100),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(e9db), "disable_voice", 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(64fb), "auto_save", 25, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_HEX, _(5940), "ipx_socket", 210, 0, 0x7FFF),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_STR, _(f11f), "player_name", 25, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, "preferences", 0, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(238c), "effects", 25, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(bf3b), "click_scroll", 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(2349), "fast_movement", 25, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(09e9), "follow_unit", 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(f99e), "enemy_halt", 25, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(c8b2), "auto_select", 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, _(24e6), "quick_scroll", 25, 4, 128),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, "options", 0, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(86df), "timer", 25, 0, 32767),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(de3f), "endturn", 210, 0, 32767),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, _(f019), "play_mode", 25, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, _(d5b2), "opponent", 25, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, _(e791), "victory_limit", 25, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, "invalid", 0, 0, 0),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(d5fa), "disable_fire", 25, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, _(6a65), "real_time", 210, 0, 1),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(3bda), "red_team_player", 25, 0, 3),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(3bb7), "green_team_player", 210, 0, 3),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(11a8), "blue_team_player", 25, 0, 3),
        OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, _(75a0), "gray_team_player", 210, 0, 3),
    };

    WindowInfo window;
    Button* button_cancel;
    Button* button_done;
    Button* button_help;
    uint16_t team;
    ResourceID bg_image;
    char text_buffer[30];
    int32_t text_buffer_key;
    TextEdit* text_edit;
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
