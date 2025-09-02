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
#include "game_manager.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "sound_manager.hpp"
#include "text.hpp"
#include "window_manager.hpp"

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
    {(type), (caption), (ini_param_index), (ulx), (range_min), (range_max), nullptr, nullptr, nullptr, 0, 0}

static struct OptionsButton options_menu_buttons[] = {
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

static const char *options_menu_play_mode_strings[] = {_(d0a7), _(ee25)};

static const char *options_menu_opponent_strings[] = {_(a24d), _(37b7), _(bb85), _(b663), _(ab89), _(c7e3)};

static const char *options_menu_victory_type_strings[] = {_(b542), _(c303)};

OptionsMenu::OptionsMenu(uint16_t team, ResourceID bg_image)
    : Window(bg_image, GameManager_GetDialogWindowCenterMode()),
      team(team),
      bg_image(bg_image),
      text_edit(nullptr),
      control_id(0),
      is_slider_active(false),
      button_count(sizeof(options_menu_buttons) / sizeof(struct OptionsButton)) {
    int32_t uly = bg_image == SETUPPIC ? 141 : 383;

    Cursor_SetCursor(CURSOR_HAND);
    Text_SetFont(GNW_TEXT_FONT_5);

    Add();
    FillWindowInfo(&window);

    button_done = new (std::nothrow) Button(PRFDNE_U, PRFDNE_D, 215, uly);
    button_done->SetCaption(_(6907));
    button_done->SetRValue(1000);
    button_done->SetPValue(GNW_INPUT_PRESS + 1000);
    button_done->SetSfx(NDONE0);
    button_done->RegisterButton(window.id);

    button_cancel = new (std::nothrow) Button(PRFCAN_U, PRFCAN_D, 125, uly);
    button_cancel->SetCaption(_(3fda));
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
        Text_TextBox(window.buffer, window.width, _(f70f), 108, 12, 184, 17, COLOR_GREEN, true, true);
    }

    Init();
    win_draw_rect(window.id, &window.window);

    for (int32_t i = 0; i < button_count; ++i) {
        if ((options_menu_buttons[i].ini_param_index != INI_ENHANCED_GRAPHICS || bg_image == SETUPPIC) &&
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

    for (int32_t i = 0; i < button_count; ++i) {
        delete options_menu_buttons[i].button;
        options_menu_buttons[i].button = nullptr;

        delete options_menu_buttons[i].image;
        options_menu_buttons[i].image = nullptr;

        delete options_menu_buttons[i].ini_string_value;
        options_menu_buttons[i].ini_string_value = nullptr;
    }

    if (bg_image == PREFSPIC) {
        GameManager_ProcessTick(true);
    }
}

void OptionsMenu::InitSliderControl(int32_t id, int32_t ulx, int32_t uly) {
    IniParameter ini_param_index;
    struct ImageSimpleHeader *slider_slit_image;
    int32_t prfslit_ulx;
    int32_t prfslit_uly;
    int32_t prfslit_pos_x;
    int32_t prfslit_pos_y;

    ini_param_index = options_menu_buttons[id].ini_param_index;

    Text_TextBox(&window, options_menu_buttons[id].format, ulx, uly, 185, 20, false, true);
    slider_slit_image = reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_LoadResource(PRFSLIT));
    prfslit_ulx = slider_slit_image->width;

    prfslit_pos_x = Text_GetWidth(options_menu_buttons[id].format) + ulx + 10;
    prfslit_pos_y = uly + (20 - slider_slit_image->height) / 2;

    WindowManager_LoadSimpleImage(PRFSLIT, prfslit_pos_x, prfslit_pos_y, 1, &window);

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

void OptionsMenu::InitEditControl(int32_t id, int32_t ulx, int32_t uly) {
    IniParameter ini_param_index;
    ResourceID resource_id{INVALID_ID};
    struct ImageSimpleHeader *resource_image;
    int32_t image_ulx;
    int32_t image_uly;
    int32_t image_pos_x;
    int32_t image_pos_y;

    Text_TextBox(&window, options_menu_buttons[id].format, ulx, uly, 185, 20, false, true);
    ini_param_index = options_menu_buttons[id].ini_param_index;

    switch (options_menu_buttons[id].type) {
        case OPTIONS_TYPE_EDIT_INT:
        case OPTIONS_TYPE_EDIT_HEX: {
            int32_t radix;

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
                ini_param_index = static_cast<IniParameter>(INI_RED_TEAM_NAME + team);
            }

            ini_config.GetStringValue(ini_param_index, options_menu_buttons[id].ini_string_value, 30);
            resource_id = PREFNAME;

        } break;

        default: {
            SDL_assert(0);
        } break;
    }

    resource_image = reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_LoadResource(resource_id));

    image_ulx = resource_image->width;
    image_uly = resource_image->height;

    image_pos_x = Text_GetWidth(options_menu_buttons[id].format) + ulx + 10;
    image_pos_y = uly + (20 - image_uly) / 2;

    WindowManager_LoadSimpleImage(resource_id, image_pos_x, image_pos_y, 1, &window);

    options_menu_buttons[id].image = new (std::nothrow) Image(image_pos_x, uly, image_ulx, 20);
    options_menu_buttons[id].image->Copy(&window);

    Text_TextBox(window.buffer, window.width, options_menu_buttons[id].ini_string_value, image_pos_x, uly, image_ulx,
                 20, 0xA2, true, true);

    options_menu_buttons[id].button = new (std::nothrow) Button(image_pos_x, uly, image_ulx, 20);
    options_menu_buttons[id].button->SetPValue(1002 + id);
    options_menu_buttons[id].button->SetSfx(MBUTT0);
    options_menu_buttons[id].button->RegisterButton(window.id);
}

void OptionsMenu::InitCheckboxControl(int32_t id, int32_t ulx, int32_t uly) {
    Button *button;
    IniParameter ini_param_index;

    ini_param_index = options_menu_buttons[id].ini_param_index;

    button = new (std::nothrow) Button(UNCHKED, CHECKED, ulx, uly);
    button->Copy(window.id);
    button->SetFlags(1);

    if (ini_param_index == INI_DISABLE_MUSIC || ini_param_index == INI_DISABLE_FX ||
        ini_param_index == INI_DISABLE_VOICE || ini_param_index == INI_ENHANCED_GRAPHICS) {
        button->SetPValue(1002 + id);
        button->SetRValue(1002 + id);
    } else {
        button->SetPValue(GNW_INPUT_PRESS + 1002 + id);
        button->SetRValue(GNW_INPUT_PRESS + 1002 + id);
    }

    button->SetSfx(MBUTT0);
    button->RegisterButton(window.id);

    options_menu_buttons[id].button = button;

    Text_TextBox(&window, options_menu_buttons[id].format, ulx + 25, uly - 2, 155, 24, false, true);
}

void OptionsMenu::InitLabelControl(int32_t id, int32_t ulx, int32_t uly) {
    IniParameter ini_param_index;
    char buffer[200];
    FontColor font_color = Fonts_BrightYellowColor;

    ini_param_index = options_menu_buttons[id].ini_param_index;

    switch (ini_param_index) {
        case INI_PLAY_MODE: {
            sprintf(buffer, options_menu_buttons[id].format,
                    options_menu_play_mode_strings[ini_get_setting(ini_param_index)]);
        } break;

        case INI_OPPONENT: {
            sprintf(buffer, options_menu_buttons[id].format,
                    options_menu_opponent_strings[ini_get_setting(ini_param_index)]);
        } break;

        case INI_VICTORY_LIMIT: {
            sprintf(buffer, options_menu_buttons[id].format, ini_setting_victory_limit,
                    options_menu_victory_type_strings[ini_setting_victory_type]);
        } break;

        case INI_MUSIC_LEVEL: {
            strcpy(buffer, options_menu_buttons[id].format);
            font_color = Fonts_GoldColor;
        } break;
    }

    Text_TextBox(&window, buffer, ulx, uly, window.width, 20, false, true, font_color);
}

void OptionsMenu::DrawSlider(int32_t id, int32_t value) {
    OptionsButton *button;
    struct ImageSimpleHeader *slider_slit_image;
    struct ImageSimpleHeader *slider_slide_image;
    int16_t max;
    int32_t ulx;
    int32_t uly;

    button = &options_menu_buttons[id];
    button->image->Write(&window);

    max = std::max(value, static_cast<int32_t>(button->range_min));
    value = std::min(button->range_max, max);

    ulx = button->image->GetULX() + 10;
    uly = button->image->GetULY();

    slider_slit_image = reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_LoadResource(PRFSLIT));

    ulx += (value * slider_slit_image->width) / button->range_max;

    slider_slide_image = reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_LoadResource(PRFSLIDE));

    ulx -= slider_slide_image->width / 2;
    uly += (20 - slider_slide_image->height) / 2;

    WindowManager_LoadSimpleImage(PRFSLIDE, ulx, uly, 1, &window);

    button->rest_state = value;
}

void OptionsMenu::UpdateSlider(int32_t id) {
    OptionsButton *button;
    Rect bounds;
    int32_t mouse_x;
    int32_t mouse_y;
    int32_t value;
    int32_t audio_type;

    button = &options_menu_buttons[id];

    bounds.ulx = button->image->GetULX();
    bounds.uly = button->image->GetULY();
    bounds.lrx = bounds.ulx + button->image->GetWidth();
    bounds.lry = bounds.uly + button->image->GetHeight();

    GetCursorPosition(mouse_x, mouse_y);

    value = ((mouse_x - (bounds.ulx + 10)) * button->range_max) / (bounds.lrx - bounds.ulx - 20);

    DrawSlider(id, value);

    win_draw_rect(window.id, &bounds);

    if (button->ini_param_index != INI_QUICK_SCROLL) {
        if (button->ini_param_index == INI_MUSIC_LEVEL) {
            audio_type = AUDIO_TYPE_MUSIC;
        } else if (button->ini_param_index == INI_FX_SOUND_LEVEL) {
            audio_type = AUDIO_TYPE_SFX2;
        } else {
            audio_type = AUDIO_TYPE_VOICE;
        }

        SetVolume(id, audio_type, button->rest_state);
    }
}

void OptionsMenu::SetVolume(int32_t id, int32_t audio_type, int32_t value) {
    SoundManager_SetVolume(audio_type, static_cast<float>(value) / 100);
    ini_set_setting(options_menu_buttons[id].ini_param_index, value);
}

int32_t OptionsMenu::ProcessTextEditKeyPress(int32_t key) {
    int32_t result;

    if (text_edit) {
        OptionsButton *button;

        button = &options_menu_buttons[control_id];

        text_edit->ProcessKeyPress(key);
        text_edit->AcceptEditedText();
        text_edit->LeaveTextEditField();

        delete text_edit;
        text_edit = nullptr;

        if (button->type == OPTIONS_TYPE_EDIT_INT) {
            int32_t value;
            int32_t max;

            value = strtol(button->ini_string_value, nullptr, 10);

            max = std::max(value, static_cast<int32_t>(button->range_min));
            value = std::min(static_cast<int32_t>(button->range_max), max);

            snprintf(button->ini_string_value, 30, "%d", value);
        }

        button->image->Write(&window);

        Text_TextBox(window.buffer, window.width, button->ini_string_value, button->image->GetULX(),
                     button->image->GetULY(), button->image->GetWidth(), button->image->GetHeight(), 0xA2, true, true);

        control_id = 0;
        win_draw_rect(window.id, &window.window);

        result = true;

    } else {
        result = false;
    }

    return result;
}

void OptionsMenu::Init() {
    int32_t window_ulx;
    int32_t window_uly;
    IniParameter ini_param_index;

    if (bg_image == PREFSPIC) {
        window_uly = 20;

#if !defined(NDEBUG)
        if (ini_get_setting(INI_DEBUG)) {
            window_uly = 0;
        }
#endif /* !defined(NDEBUG) */

    } else {
        window_uly = -20;
    }

    for (int32_t i = 0; i < button_count; ++i) {
        window_ulx = options_menu_buttons[i].ulx;
        ini_param_index = options_menu_buttons[i].ini_param_index;

        if (ini_param_index != INI_ENHANCED_GRAPHICS || bg_image == SETUPPIC) {
#if !defined(NDEBUG)
            if (ini_get_setting(INI_DEBUG)) {
                if (ini_param_index == INI_PLAY_MODE || ini_param_index == INI_OPPONENT ||
                    ini_param_index == INI_VICTORY_LIMIT) {
                    continue;
                }
            }
#endif /* !defined(NDEBUG) */

            if (window_ulx == 25) {
                window_uly += 20;
            }

            if (options_menu_buttons[i].type != OPTIONS_TYPE_SECTION) {
                options_menu_buttons[i].rest_state = ini_get_setting(ini_param_index);
                options_menu_buttons[i].last_rest_state = options_menu_buttons[i].rest_state;

                switch (options_menu_buttons[i].type) {
                    case OPTIONS_TYPE_SLIDER: {
                        InitSliderControl(i, window_ulx, window_uly);
                    } break;
                    case OPTIONS_TYPE_EDIT_INT:
                    case OPTIONS_TYPE_EDIT_HEX:
                    case OPTIONS_TYPE_EDIT_STR: {
                        InitEditControl(i, window_ulx, window_uly);
                    } break;
                    case OPTIONS_TYPE_CHECKBOX: {
                        InitCheckboxControl(i, window_ulx, window_uly);
                    } break;
                    case OPTIONS_TYPE_LABEL: {
                        InitLabelControl(i, window_ulx, window_uly);
                        window_uly -= 20 - (Text_GetHeight() + 3);
                    } break;
                }

            } else {
                if (bg_image == SETUPPIC && ini_param_index != INI_SETUP) {
                    button_count = i;
                    return;
                }

                if (ini_param_index
#if !defined(NDEBUG)
                    || ini_get_setting(INI_DEBUG)
#endif /* !defined(NDEBUG) */
                ) {
                    ;

                } else {
                    button_count = i;
                    return;
                }

                window_uly += 20;
            }
        }
    }
}

int32_t OptionsMenu::ProcessKeyPress(int32_t key) {
    if (is_slider_active) {
        int32_t mouse_buttons;

        mouse_buttons = mouse_get_buttons();

        if ((mouse_buttons & MOUSE_RELEASE_LEFT) == 0 && mouse_buttons) {
            UpdateSlider(control_id);
        } else {
            control_id = 0;
            is_slider_active = false;
        }
    }

    switch (key) {
        case GNW_KB_KEY_SHIFT_DIVIDE: {
            if (text_edit) {
                text_edit->ProcessKeyPress(key);
            } else if (bg_image == SETUPPIC) {
                HelpMenu_Menu("SETUP_MENU_SETUP", WINDOW_MAIN_WINDOW, false);
            } else {
                HelpMenu_Menu("PREFS_MENU_SETUP", WINDOW_MAIN_WINDOW, Remote_IsNetworkGame == false);
            }

        } break;

        case GNW_KB_KEY_RETURN:
        case 1000: {
            if (key == GNW_KB_KEY_RETURN) {
                if (ProcessTextEditKeyPress(key)) {
                    break;
                }
            }

            ProcessTextEditKeyPress(GNW_KB_KEY_RETURN);

            for (int32_t i = 0; i < button_count; ++i) {
                IniParameter ini_param_index;
                int32_t option_type;

                ini_param_index = options_menu_buttons[i].ini_param_index;
                option_type = options_menu_buttons[i].type;

                switch (option_type) {
                    case OPTIONS_TYPE_SLIDER: {
                        ini_set_setting(ini_param_index, options_menu_buttons[i].rest_state);
                    } break;

                    case OPTIONS_TYPE_EDIT_INT: {
                        ini_set_setting(ini_param_index, strtol(options_menu_buttons[i].ini_string_value, nullptr, 10));
                    } break;

                    case OPTIONS_TYPE_EDIT_HEX: {
                        char buffer[20];

                        snprintf(buffer, 20, "0x%s", options_menu_buttons[i].ini_string_value);

                        ini_config.SetStringValue(ini_param_index, buffer);
                        ini_set_setting(ini_param_index, strtol(options_menu_buttons[i].ini_string_value, nullptr, 16));
                    } break;

                    case OPTIONS_TYPE_EDIT_STR: {
                        ini_config.SetStringValue(ini_param_index, options_menu_buttons[i].ini_string_value);

                        if (bg_image == PREFSPIC) {
                            ini_param_index = static_cast<IniParameter>(INI_RED_TEAM_NAME + team);
                            ini_config.SetStringValue(ini_param_index, options_menu_buttons[i].ini_string_value);

                            if (Remote_IsNetworkGame) {
                                Remote_SendNetPacket_09(team);
                            }
                        }

                    } break;

                    case OPTIONS_TYPE_CHECKBOX: {
                        if (options_menu_buttons[i].button) {
                            ini_set_setting(ini_param_index, win_button_down(options_menu_buttons[i].button->GetId()));
                        }

                    } break;
                }
            }

            if (bg_image == SETUPPIC) {
                ResourceManager_DisableEnhancedGraphics = !ini_get_setting(INI_ENHANCED_GRAPHICS);
            }

            ini_config.Save();
            exit_menu = true;

        } break;

        case GNW_KB_KEY_LALT_P: {
            PauseMenu_Menu();

        } break;

        case GNW_KB_KEY_ESCAPE:
        case 1001: {
            if (key == GNW_KB_KEY_ESCAPE) {
                if (ProcessTextEditKeyPress(key)) {
                    break;
                }
            }

            ProcessTextEditKeyPress(GNW_KB_KEY_ESCAPE);

            for (int32_t i = 0; i < button_count; ++i) {
                int32_t ini_param_index;
                int32_t last_value;

                ini_param_index = options_menu_buttons[i].ini_param_index;
                last_value = options_menu_buttons[i].last_rest_state;

                if (last_value != options_menu_buttons[i].rest_state) {
                    switch (ini_param_index) {
                        case INI_MUSIC_LEVEL: {
                            SetVolume(i, AUDIO_TYPE_MUSIC, last_value);
                        } break;

                        case INI_FX_SOUND_LEVEL: {
                            SetVolume(i, AUDIO_TYPE_SFX2, last_value);
                        } break;

                        case INI_VOICE_LEVEL: {
                            SetVolume(i, AUDIO_TYPE_VOICE, last_value);

                        } break;
                        case INI_DISABLE_MUSIC: {
                            ini_set_setting(INI_DISABLE_MUSIC, last_value);
                            SoundManager_HaltMusicPlayback(last_value);

                        } break;

                        case INI_DISABLE_FX: {
                            ini_set_setting(INI_DISABLE_FX, last_value);
                            SoundManager_HaltSfxPlayback(last_value);

                        } break;

                        case INI_DISABLE_VOICE: {
                            ini_set_setting(INI_DISABLE_VOICE, last_value);
                            SoundManager_HaltVoicePlayback(last_value);

                        } break;
                    }
                }
            }

            exit_menu = true;

        } break;

        default: {
            if (key < GNW_INPUT_PRESS) {
                if (key < 1002) {
                    if (text_edit) {
                        text_edit->ProcessKeyPress(key);
                    }

                } else {
                    key -= 1002;

                    if (key != control_id) {
                        ProcessTextEditKeyPress(GNW_KB_KEY_RETURN);

                        if (options_menu_buttons[key].type == OPTIONS_TYPE_CHECKBOX) {
                            IniParameter ini_param_index;
                            int32_t value;

                            options_menu_buttons[key].button->PlaySound();
                            value = win_button_down(options_menu_buttons[key].button->GetId());
                            options_menu_buttons[key].rest_state = value;

                            ini_param_index = options_menu_buttons[key].ini_param_index;
                            ini_set_setting(ini_param_index, value);

                            switch (ini_param_index) {
                                case INI_DISABLE_MUSIC: {
                                    SoundManager_HaltMusicPlayback(value);
                                } break;

                                case INI_DISABLE_FX: {
                                    SoundManager_HaltSfxPlayback(value);
                                } break;

                                case INI_DISABLE_VOICE: {
                                    SoundManager_HaltVoicePlayback(value);
                                } break;
                            }

                        } else if (options_menu_buttons[key].type == OPTIONS_TYPE_SLIDER) {
                            control_id = key;
                            UpdateSlider(control_id);
                            is_slider_active = true;

                        } else {
                            control_id = key;
                            options_menu_buttons[key].image->Write(&window);
                            text_edit = new (std::nothrow)
                                TextEdit(&window, options_menu_buttons[key].ini_string_value, 30,
                                         options_menu_buttons[key].image->GetULX() + 5,
                                         options_menu_buttons[key].image->GetULY(),
                                         options_menu_buttons[key].image->GetWidth() - 5,
                                         options_menu_buttons[key].image->GetHeight() + 1, 0xA2, GNW_TEXT_FONT_5);

                            switch (options_menu_buttons[key].type) {
                                case OPTIONS_TYPE_EDIT_INT: {
                                    text_edit->SetMode(TEXTEDIT_MODE_INT);
                                } break;

                                case OPTIONS_TYPE_EDIT_HEX: {
                                    text_edit->SetMode(TEXTEDIT_MODE_HEX);
                                } break;

                                case OPTIONS_TYPE_EDIT_STR: {
                                    text_edit->SetMode(TEXTEDIT_MODE_STR);
                                } break;
                            }

                            text_edit->LoadBgImage();
                            text_edit->SetEditedText(options_menu_buttons[key].ini_string_value);
                            text_edit->DrawFullText();
                            text_edit->EnterTextEditField();
                        }
                    }
                }

            } else if (!event_release) {
                event_release = true;

                key -= GNW_INPUT_PRESS;

                switch (key) {
                    case 1000: {
                        button_done->PlaySound();
                    } break;

                    case 1001: {
                        button_help->PlaySound();
                    } break;

                    case GNW_KB_KEY_SHIFT_DIVIDE: {
                        button_cancel->PlaySound();
                    } break;

                    default: {
                        SDL_assert(key - 1002 >= 0 && static_cast<size_t>(key - 1002) <
                                                          sizeof(options_menu_buttons) / sizeof(struct OptionsButton));

                        options_menu_buttons[key - 1002].button->PlaySound();
                        event_release = false;

                    } break;
                }
            }

        } break;
    }

    return true;
}

void OptionsMenu::Run() {
    exit_menu = false;
    event_release = false;

    while (!exit_menu) {
        int32_t key = get_input();

        if (key > 0 && key < GNW_INPUT_PRESS) {
            event_release = false;
        }

        if (bg_image == PREFSPIC && Remote_IsNetworkGame) {
            if (GameManager_RequestMenuExit) {
                key = 1001;
            }

            GameManager_ProcessState(true);
        }

        ProcessKeyPress(key);
    }

    if (!Remote_IsNetworkGame) {
        Remote_PauseTimeStamp = timer_get();
    }
}
