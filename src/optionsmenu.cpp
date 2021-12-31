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

#include "optionsmenu.hpp"

#include <algorithm>

#include "cursor.hpp"
#include "gwindow.hpp"
#include "inifile.hpp"
#include "text.hpp"

struct OptionsButton {
    char type;
    char *format;
    char ini_param_index;
    short ulx;
    short range_min;
    short range_max;
    Button *button;
    Image *image;
    char *ini_string_value;
    short rest_state;
    short last_rest_state;
};

enum OptionsType {
    OPTIONS_TYPE_SECTION,
    OPTIONS_TYPE_SLIDER,
    OPTIONS_TYPE_EDIT_INT,
    OPTIONS_TYPE_EDIT_HEX,
    OPTIONS_TYPE_EDIT_STR,
    OPTIONS_TYPE_CHECKBOX,
    OPTIONS_TYPE_LABEL,
};

#define OPTIONS_BUTTON_DEF(type, caption, ini_param_index, ulx, range_min, range_max) \
    { (type), (caption), (ini_param_index), (ulx), (range_min), (range_max), nullptr, nullptr, nullptr, 0, 0 }

static struct OptionsButton options_menu_buttons[] = {
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, ini_SETUP, 0, 0, 0),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, "Volume:", ini_music_level, 25, 0, 0),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Enhanced graphics (requires 16MB)", ini_enhanced_graphics, 210, 0, 0),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, "Music", ini_music_level, 25, 0, 100),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Disable Music", ini_disable_music, 210, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, "FX", ini_fx_sound_level, 25, 0, 100),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "FX Disabled", ini_disable_fx, 210, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, "Voice", ini_voice_level, 25, 0, 100),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Voice Disabled", ini_disable_voice, 210, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Auto-Save Enabled", ini_auto_save, 25, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_HEX, "IPX Socket:", ini_ipx_socket, 210, 0, 0x7FFF),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_STR, "Player Name:", ini_player_name, 25, 0, 0),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, ini_PREFERENCES, 0, 0, 0),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Animate Effects", ini_effects, 25, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Click to Scroll", ini_click_scroll, 210, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Double Unit Steps", ini_fast_movement, 25, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Track Selected Unit", ini_follow_unit, 210, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Halt movement when enemy is detected", ini_enemy_halt, 25, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Auto-Select Next Unit", ini_auto_select, 210, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SLIDER, "Scroll Speed", ini_quick_scroll, 25, 4, 128),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, ini_OPTIONS, 0, 0, 0),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, "Turn Time:", ini_timer, 25, 0, 32767),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, "End Turn Time:", ini_endturn, 210, 0, 32767),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, "Play Mode: %s", ini_play_mode, 25, 0, 0),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, "Computer Player(s): %s", ini_opponent, 25, 0, 0),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_LABEL, "Game ends at %i %s.", ini_victory_limit, 25, 0, 0),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_SECTION, nullptr, 0, 0, 0, 0),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Disable Fire", ini_disable_fire, 25, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_CHECKBOX, "Real Time", ini_real_time, 210, 0, 1),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, "Red Team", ini_red_team_player, 25, 0, 3),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, "Green Team", ini_green_team_player, 210, 0, 3),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, "Blue Team", ini_blue_team_player, 25, 0, 3),
    OPTIONS_BUTTON_DEF(OPTIONS_TYPE_EDIT_INT, "Gray Team", ini_gray_team_player, 210, 0, 3),
};

static const char *options_menu_play_mode_strings[] = {"Turn Based", "Simultaneous Moves"};

static const char *options_menu_opponent_strings[] = {"Clueless", "Apprentice", "Average", "Expert", "Master", "God"};

static const char *options_menu_victory_type_strings[] = {"turns", "points"};

OptionsMenu::OptionsMenu(unsigned short team, ResourceID bg_image)
    : Window(bg_image, bg_image == SETUPPIC ? GWINDOW_MAIN_WINDOW : GWINDOW_38),
      team(team),
      bg_image(bg_image),
      text_edit(nullptr),
      field_72(0),
      button_count(sizeof(options_menu_buttons) / sizeof(struct OptionsButton)) {
    int uly = bg_image == SETUPPIC ? 141 : 383;

    Cursor_SetCursor(CURSOR_HAND);
    text_font(5);

    Add();
    FillWindowInfo(&window);

    button_done = new (std::nothrow) Button(PRFDNE_U, PRFDNE_D, 215, uly);
    button_done->SetCaption("Done");
    button_done->SetRValue(1000);
    button_done->SetPValue(GNW_INPUT_PRESS + 1000);
    button_done->SetSfx(NDONE0);
    button_done->RegisterButton(window.id);

    button_cancel = new (std::nothrow) Button(PRFCAN_U, PRFCAN_D, 125, uly);
    button_cancel->SetCaption("Cancel");
    button_cancel->SetRValue(1001);
    button_cancel->SetPValue(GNW_INPUT_PRESS + 1001);
    button_cancel->SetSfx(NCANC0);
    button_cancel->RegisterButton(window.id);

    button_help = new (std::nothrow) Button(PRFHLP_U, PRFHLP_D, 188, uly);
    button_help->SetRValue(GNW_KB_KEY_SHIFT_DIVIDE);
    button_help->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_SHIFT_DIVIDE);
    button_help->SetSfx(MBUTT0);
    button_help->RegisterButton(window.id);

    if (bg_image == PREFSPIC) {
        Text_TextBox(window.buffer, window.width, "Preferences", 108, 12, 184, 17, 0x02, true, true);
    }

    Init();
    win_draw_rect(window.id, &window.window);

    for (int i = 0; i < button_count; ++i) {
        if ((options_menu_buttons[i].ini_param_index != ini_enhanced_graphics || bg_image == SETUPPIC) &&
            options_menu_buttons[i].type == OPTIONS_TYPE_CHECKBOX) {
            options_menu_buttons[i].button->SetRestState(options_menu_buttons[i].rest_state);
        }
    }
}

OptionsMenu::~OptionsMenu() {
    delete button_done;
    delete button_cancel;
    delete button_help;
    delete text_edit;

    for (int i = 0; i < button_count; ++i) {
        delete options_menu_buttons[i].button;
        options_menu_buttons[i].button = nullptr;

        delete options_menu_buttons[i].image;
        options_menu_buttons[i].image = nullptr;

        delete options_menu_buttons[i].ini_string_value;
        options_menu_buttons[i].ini_string_value = nullptr;
    }

    if (bg_image == PREFSPIC) {
        sub_A0EFE(1);
    }
}

void OptionsMenu::InitSliderControl(int id, int ulx, int uly) {
    enum GAME_INI_e ini_param_index;
    ImageHeader2 *slider_slit_image;
    int prfslit_ulx;
    int prfslit_uly;
    int prfslit_pos_x;
    int prfslit_pos_y;

    ini_param_index = options_menu_buttons[id].ini_param_index;

    Text_TextBox(&window, options_menu_buttons[id].format, ulx, uly, 185, 20, false, true);
    slider_slit_image = ResourceManager_LoadResource(PRFSLIT);
    prfslit_ulx = slider_slit_image->ulx;

    prfslit_pos_x = text_width(options_menu_buttons[id].format) + ulx + 10;
    prfslit_pos_y = uly + (20 - slider_slit_image->uly) / 2;

    gwin_load_image2(PRFSLIT, prfslit_pos_x, prfslit_pos_y, 1, &window);

    prfslit_pos_x -= 10;
    prfslit_ulx += 20;

    options_menu_buttons[id].image = new (std::nothrow) Image(prfslit_pos_x, uly, prfslit_ulx, 20);
    options_menu_buttons[id].image->Copy(&window);

    DrawSlider(id, ini_get_setting(ini_param_index));

    options_menu_buttons[id].button = new (std::nothrow) Button(prfslit_pos_x, uly, prfslit_ulx, 20);
    options_menu_buttons[id].button->SetPValue(1002 + id);
    options_menu_buttons[id].button->SetSfx(MBUTT0);
    options_menu_buttons[id].button->RegisterButton(window.id);
}

void OptionsMenu::InitEditControl(int id, int ulx, int uly) {
    enum GAME_INI_e ini_param_index;
    ResourceID resource_id;
    ImageHeader2 *resource_image;
    int image_ulx;
    int image_uly;
    int image_pos_x;
    int image_pos_y;

    Text_TextBox(&window, options_menu_buttons[id].format, ulx, uly, 185, 20, false, true);
    ini_param_index = options_menu_buttons[id].ini_param_index;

    switch (options_menu_buttons[id].type) {
        case OPTIONS_TYPE_EDIT_INT:
        case OPTIONS_TYPE_EDIT_HEX: {
            int radix;

            options_menu_buttons[id].ini_string_value = new (std::nothrow) char[30];

            if (options_menu_buttons[id].type == OPTIONS_TYPE_EDIT_HEX) {
                snprintf(options_menu_buttons[id].ini_string_value, 30, "%#x", options_menu_buttons[id].rest_state);
            } else {
                snprintf(options_menu_buttons[id].ini_string_value, 30, "%d", options_menu_buttons[id].rest_state);
            }
            resource_id = PREFEDIT;

        } break;

        case OPTIONS_TYPE_EDIT_STR: {
            options_menu_buttons[id].ini_string_value = new (std::nothrow) char[30];

            if (bg_image == PREFSPIC) {
                ini_param_index = ini_red_team_name + team;
            }

            ini_config.GetStringValue(ini_param_index, options_menu_buttons[id].ini_string_value, 30);
            resource_id = PREFNAME;

        } break;
    }

    resource_image = ResourceManager_LoadResource(resource_id);

    image_ulx = resource_image->ulx;
    image_uly = resource_image->uly;

    image_pos_x = text_width(options_menu_buttons[id].format) + ulx + 10;
    image_pos_y = uly + (20 - image_uly) / 2;

    gwin_load_image2(resource_id, image_pos_x, image_pos_y, 1, &window);

    options_menu_buttons[id].image = new (std::nothrow) Image(image_pos_x, uly, image_ulx, 20);
    options_menu_buttons[id].image->Copy(&window);

    Text_TextBox(window.buffer, window.width, options_menu_buttons[id].ini_string_value, image_pos_x, uly, image_ulx,
                 20, 0xA2, true, true);

    options_menu_buttons[id].button = new (std::nothrow) Button(image_pos_x, uly, image_ulx, 20);
    options_menu_buttons[id].button->SetPValue(1002 + id);
    options_menu_buttons[id].button->SetSfx(MBUTT0);
    options_menu_buttons[id].button->RegisterButton(window.id);
}

void OptionsMenu::InitCheckboxControl(int id, int ulx, int uly) {
    Button *button;
    enum GAME_INI_e ini_param_index;

    ini_param_index = options_menu_buttons[id].ini_param_index;

    button = options_menu_buttons[id].button;
    button = new (std::nothrow) Button(UNCHKED, CHECKED, ulx, uly);
    button->Copy(window.id);
    button->SetFlags(1);

    if (ini_param_index == ini_disable_music || ini_param_index == ini_disable_fx ||
        ini_param_index == ini_disable_voice || ini_param_index == ini_enhanced_graphics) {
        button->SetPValue(1002 + id);
        button->SetRValue(1002 + id);
    } else {
        button->SetPValue(GNW_INPUT_PRESS + 1002 + id);
        button->SetRValue(GNW_INPUT_PRESS + 1002 + id);
    }

    button->SetSfx(MBUTT0);
    button->RegisterButton(window.id);

    Text_TextBox(&window, options_menu_buttons[id].format, ulx + 25, uly - 2, 155, 24, false, true);
}

void OptionsMenu::InitLabelControl(int id, int ulx, int uly) {
    enum GAME_INI_e ini_param_index;
    char buffer[200];
    FontColor font_color = Fonts_BrightYellowColor;

    ini_param_index = options_menu_buttons[id].ini_param_index;

    switch (ini_param_index) {
        case ini_play_mode: {
            sprintf(buffer, options_menu_buttons[id].format,
                    options_menu_play_mode_strings[ini_get_setting(ini_param_index)]);
        } break;

        case ini_opponent: {
            sprintf(buffer, options_menu_buttons[id].format,
                    options_menu_opponent_strings[ini_get_setting(ini_param_index)]);
        } break;

        case ini_victory_limit: {
            sprintf(buffer, options_menu_buttons[id].format, ini_setting_victory_limit,
                    options_menu_victory_type_strings[ini_setting_victory_type]);
        } break;

        case ini_music_level: {
            strcpy(buffer, options_menu_buttons[id].format);
            font_color = Fonts_GoldColor;
        } break;
    }

    Text_TextBox(&window, buffer, ulx, uly, window.width, 20, false, true, font_color);
}

void OptionsMenu::DrawSlider(int id, int value) {
    OptionsButton *button;
    ImageHeader2 *slider_slit_image;
    ImageHeader2 *slider_slide_image;
    short max;
    int ulx;
    int uly;
    int width;
    int height;

    button = options_menu_buttons[id].button;
    button->image->Write(&window);

    max = std::max(value, static_cast<int>(button->range_min));
    value = std::min(button->range_max, max);

    ulx = button->image->GetULX() + 10;
    uly = button->image->GetULY();
    width = button->image->GetWidth() - 20;
    height = button->image->GetHeight();

    slider_slit_image = ResourceManager_LoadResource(PRFSLIT);

    ulx += (value * slider_slit_image->ulx) / button->range_max;

    slider_slide_image = ResourceManager_LoadResource(PRFSLIDE);

    ulx -= slider_slide_image->ulx / 2;
    uly += (20 - slider_slide_image->uly) / 2;

    gwin_load_image2(PRFSLIDE, ulx, uly, 1, &window);

    button->rest_state = value;
}

void OptionsMenu::Init() {
    int ulx;
    int uly;
    enum GAME_INI_e ini_param_index;

    if (bg_image == PREFSPIC) {
        uly = 20;
    } else {
        uly = -20;
    }

    for (int i = 0; i < button_count; ++i) {
        ulx = options_menu_buttons[i].ulx;
        ini_param_index = options_menu_buttons[i].ini_param_index;

        if (ini_param_index != ini_enhanced_graphics || bg_image == SETUPPIC) {
            if (ulx == 25) {
                uly += 20;
            }

            if (options_menu_buttons[i].type != OPTIONS_TYPE_SECTION) {
                options_menu_buttons[i].rest_state = ini_get_setting(ini_param_index);
                options_menu_buttons[i].last_rest_state = options_menu_buttons[i].rest_state;

                switch (options_menu_buttons[i].type) {
                    case OPTIONS_TYPE_SLIDER: {
                        InitSliderControl(i, ulx, uly);
                    } break;
                    case OPTIONS_TYPE_EDIT_INT:
                    case OPTIONS_TYPE_EDIT_HEX:
                    case OPTIONS_TYPE_EDIT_STR: {
                        InitEditControl(i, ulx, uly);
                    } break;
                    case OPTIONS_TYPE_CHECKBOX: {
                        InitCheckboxControl(i, ulx, uly);
                    } break;
                    case OPTIONS_TYPE_LABEL: {
                        InitLabelControl(i, ulx, uly);
                        uly -= 20 - text_height() + 3;
                    } break;
                }

            } else {
                if (bg_image == SETUPPIC && ini_param_index != ini_SETUP) {
                    button_count = i;
                    return;
                }

                if (!ini_param_index) {
                    button_count = i;
                    return;
                }

                uly += 20;
            }
        }
    }
}

bool OptionsMenu::Run() {}
