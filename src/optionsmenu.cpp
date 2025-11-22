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
#include <cstring>
#include <format>

#include "cursor.hpp"
#include "game_manager.hpp"
#include "helpmenu.hpp"
#include "menu.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "sound_manager.hpp"
#include "text.hpp"
#include "window_manager.hpp"

OptionsMenu::OptionsMenu(uint16_t team, ResourceID bg_image)
    : Window(bg_image, GameManager_GetDialogWindowCenterMode()),
      team(team),
      bg_image(bg_image),
      text_edit(nullptr),
      control_id(0),
      is_slider_active(false),
      button_count(sizeof(options_menu_buttons) / sizeof(struct OptionsButton)) {
    int32_t uly = bg_image == SETUPPIC ? 141 : 383;

    text_buffer[0] = '\0';
    text_buffer_key = 0;

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
        if ((strcmp(options_menu_buttons[i].setting_key.c_str(), "enhanced_graphics") != 0 || bg_image == SETUPPIC) &&
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
    }

    if (bg_image == PREFSPIC) {
        GameManager_ProcessTick(true);
    }
}

void OptionsMenu::InitSliderControl(int32_t id, int32_t ulx, int32_t uly) {
    struct ImageSimpleHeader* slider_slit_image;
    int32_t prfslit_ulx;
    int32_t prfslit_uly;
    int32_t prfslit_pos_x;
    int32_t prfslit_pos_y;
    std::string setting_key = options_menu_buttons[id].setting_key;

    Text_TextBox(&window, options_menu_buttons[id].format, ulx, uly, 185, 20, false, true);
    slider_slit_image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(PRFSLIT));
    prfslit_ulx = slider_slit_image->width;

    prfslit_pos_x = Text_GetWidth(options_menu_buttons[id].format) + ulx + 10;
    prfslit_pos_y = uly + (20 - slider_slit_image->height) / 2;

    WindowManager_LoadSimpleImage(PRFSLIT, prfslit_pos_x, prfslit_pos_y, 1, &window);

    prfslit_pos_x -= 10;
    prfslit_ulx += 20;

    options_menu_buttons[id].image = new (std::nothrow) Image(prfslit_pos_x, uly, prfslit_ulx, 20);
    options_menu_buttons[id].image->Copy(&window);

    DrawSlider(id, ResourceManager_GetSettings()->GetNumericValue(setting_key));

    options_menu_buttons[id].button = new (std::nothrow) Button(prfslit_pos_x, uly, prfslit_ulx, 20);
    options_menu_buttons[id].button->SetPValue(1002 + id);
    options_menu_buttons[id].button->SetSfx(MBUTT0);
    options_menu_buttons[id].button->RegisterButton(window.id);
}

void OptionsMenu::InitEditControl(int32_t id, int32_t ulx, int32_t uly) {
    ResourceID resource_id{INVALID_ID};
    struct ImageSimpleHeader* resource_image;
    int32_t image_ulx;
    int32_t image_uly;
    int32_t image_pos_x;
    int32_t image_pos_y;

    Text_TextBox(&window, options_menu_buttons[id].format, ulx, uly, 185, 20, false, true);

    std::string setting_key = options_menu_buttons[id].setting_key;

    switch (options_menu_buttons[id].type) {
        case OPTIONS_TYPE_EDIT_INT:
        case OPTIONS_TYPE_EDIT_HEX: {
            int32_t radix;

            if (options_menu_buttons[id].type == OPTIONS_TYPE_EDIT_HEX) {
                options_menu_buttons[id].ini_string_value = std::format("{:#x}", options_menu_buttons[id].rest_state);

            } else {
                options_menu_buttons[id].ini_string_value = std::format("{}", options_menu_buttons[id].rest_state);
            }

            resource_id = PREFEDIT;

        } break;

        case OPTIONS_TYPE_EDIT_STR: {
            if (bg_image == PREFSPIC) {
                setting_key = menu_team_name_setting[team].c_str();
            }

            options_menu_buttons[id].ini_string_value = ResourceManager_GetSettings()->GetStringValue(setting_key);

            resource_id = PREFNAME;

        } break;

        default: {
            SDL_assert(0);
        } break;
    }

    resource_image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(resource_id));

    image_ulx = resource_image->width;
    image_uly = resource_image->height;

    image_pos_x = Text_GetWidth(options_menu_buttons[id].format) + ulx + 10;
    image_pos_y = uly + (20 - image_uly) / 2;

    WindowManager_LoadSimpleImage(resource_id, image_pos_x, image_pos_y, 1, &window);

    options_menu_buttons[id].image = new (std::nothrow) Image(image_pos_x, uly, image_ulx, 20);
    options_menu_buttons[id].image->Copy(&window);

    Text_TextBox(window.buffer, window.width, options_menu_buttons[id].ini_string_value.c_str(), image_pos_x, uly,
                 image_ulx, 20, 0xA2, true, true);

    options_menu_buttons[id].button = new (std::nothrow) Button(image_pos_x, uly, image_ulx, 20);
    options_menu_buttons[id].button->SetPValue(1002 + id);
    options_menu_buttons[id].button->SetSfx(MBUTT0);
    options_menu_buttons[id].button->RegisterButton(window.id);
}

void OptionsMenu::InitCheckboxControl(int32_t id, int32_t ulx, int32_t uly) {
    Button* button;
    std::string setting_key = options_menu_buttons[id].setting_key;

    button = new (std::nothrow) Button(UNCHKED, CHECKED, ulx, uly);
    button->Copy(window.id);
    button->SetFlags(1);

    if (strcmp(setting_key.c_str(), "disable_music") == 0 || strcmp(setting_key.c_str(), "disable_fx") == 0 ||
        strcmp(setting_key.c_str(), "disable_voice") == 0 || strcmp(setting_key.c_str(), "enhanced_graphics") == 0) {
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
    char buffer[200];
    FontColor font_color = Fonts_BrightYellowColor;
    std::string setting_key = options_menu_buttons[id].setting_key;

    if (strcmp(setting_key.c_str(), "play_mode") == 0) {
        sprintf(buffer, options_menu_buttons[id].format,
                options_menu_play_mode_strings[ResourceManager_GetSettings()->GetNumericValue("play_mode")]);
    } else if (strcmp(setting_key.c_str(), "opponent") == 0) {
        sprintf(buffer, options_menu_buttons[id].format,
                options_menu_opponent_strings[ResourceManager_GetSettings()->GetNumericValue("opponent")]);
    } else if (strcmp(setting_key.c_str(), "victory_limit") == 0) {
        sprintf(buffer, options_menu_buttons[id].format, ini_setting_victory_limit,
                options_menu_victory_type_strings[ini_setting_victory_type]);
    } else if (strcmp(setting_key.c_str(), "music_level") == 0) {
        strcpy(buffer, options_menu_buttons[id].format);
        font_color = Fonts_GoldColor;
    }

    Text_TextBox(&window, buffer, ulx, uly, window.width, 20, false, true, font_color);
}

void OptionsMenu::DrawSlider(int32_t id, int32_t value) {
    OptionsButton* button;
    struct ImageSimpleHeader* slider_slit_image;
    struct ImageSimpleHeader* slider_slide_image;
    int16_t max;
    int32_t ulx;
    int32_t uly;

    button = &options_menu_buttons[id];
    button->image->Write(&window);

    max = std::max(value, static_cast<int32_t>(button->range_min));
    value = std::min(button->range_max, max);

    ulx = button->image->GetULX() + 10;
    uly = button->image->GetULY();

    slider_slit_image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(PRFSLIT));

    ulx += (value * slider_slit_image->width) / button->range_max;

    slider_slide_image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(PRFSLIDE));

    ulx -= slider_slide_image->width / 2;
    uly += (20 - slider_slide_image->height) / 2;

    WindowManager_LoadSimpleImage(PRFSLIDE, ulx, uly, 1, &window);

    button->rest_state = value;
}

void OptionsMenu::UpdateSlider(int32_t id) {
    OptionsButton* button;
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

    std::string button_key = options_menu_buttons[id].setting_key;

    if (strcmp(button_key.c_str(), "quick_scroll") != 0) {
        if (strcmp(button_key.c_str(), "music_level") == 0) {
            audio_type = AUDIO_TYPE_MUSIC;
        } else if (strcmp(button_key.c_str(), "fx_sound_level") == 0) {
            audio_type = AUDIO_TYPE_SFX2;
        } else {
            audio_type = AUDIO_TYPE_VOICE;
        }

        SetVolume(id, audio_type, button->rest_state);
    }
}

void OptionsMenu::SetVolume(int32_t id, int32_t audio_type, int32_t value) {
    SoundManager_SetVolume(audio_type, static_cast<float>(value) / 100);
    ResourceManager_GetSettings()->SetNumericValue(options_menu_buttons[id].setting_key, value);
}

int32_t OptionsMenu::ProcessTextEditKeyPress(int32_t key) {
    int32_t result;

    if (text_edit) {
        OptionsButton* button;

        button = &options_menu_buttons[control_id];

        text_edit->ProcessKeyPress(key);
        text_edit->AcceptEditedText();
        text_edit->LeaveTextEditField();

        delete text_edit;
        text_edit = nullptr;

        options_menu_buttons[text_buffer_key].ini_string_value = text_buffer;

        if (button->type == OPTIONS_TYPE_EDIT_INT) {
            int32_t value;
            int32_t max;

            value = strtol(button->ini_string_value.c_str(), nullptr, 10);

            max = std::max(value, static_cast<int32_t>(button->range_min));
            value = std::min(static_cast<int32_t>(button->range_max), max);

            button->ini_string_value = std::format("{}", value);
        }

        button->image->Write(&window);

        Text_TextBox(window.buffer, window.width, button->ini_string_value.c_str(), button->image->GetULX(),
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
    std::string setting_key;

    if (bg_image == PREFSPIC) {
        window_uly = 20;

#if !defined(NDEBUG)
        if (ResourceManager_GetSettings()->GetNumericValue("debug")) {
            window_uly = 0;
        }
#endif /* !defined(NDEBUG) */

    } else {
        window_uly = -20;
    }

    for (int32_t i = 0; i < button_count; ++i) {
        window_ulx = options_menu_buttons[i].ulx;
        setting_key = options_menu_buttons[i].setting_key;

        if (strcmp(setting_key.c_str(), "enhanced_graphics") != 0 || bg_image == SETUPPIC) {
#if !defined(NDEBUG)
            if (ResourceManager_GetSettings()->GetNumericValue("debug")) {
                if (strcmp(setting_key.c_str(), "play_mode") == 0 || strcmp(setting_key.c_str(), "opponent") == 0 ||
                    strcmp(setting_key.c_str(), "victory_limit") == 0) {
                    continue;
                }
            }
#endif /* !defined(NDEBUG) */

            if (window_ulx == 25) {
                window_uly += 20;
            }

            if (options_menu_buttons[i].type != OPTIONS_TYPE_SECTION) {
                options_menu_buttons[i].rest_state = ResourceManager_GetSettings()->GetNumericValue(setting_key);
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
                if (bg_image == SETUPPIC && strcmp(setting_key.c_str(), "setup") != 0) {
                    button_count = i;
                    return;
                }

                if (strcmp(setting_key.c_str(), "invalid") != 0
#if !defined(NDEBUG)
                    || ResourceManager_GetSettings()->GetNumericValue("debug")
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
                std::string setting_key = options_menu_buttons[i].setting_key;
                int32_t option_type = options_menu_buttons[i].type;

                switch (option_type) {
                    case OPTIONS_TYPE_SLIDER: {
                        ResourceManager_GetSettings()->SetNumericValue(setting_key, options_menu_buttons[i].rest_state);
                    } break;

                    case OPTIONS_TYPE_EDIT_INT: {
                        ResourceManager_GetSettings()->SetNumericValue(
                            setting_key, strtol(options_menu_buttons[i].ini_string_value.c_str(), nullptr, 10));
                    } break;

                    case OPTIONS_TYPE_EDIT_HEX: {
                        ResourceManager_GetSettings()->SetNumericValue(
                            setting_key, strtol(options_menu_buttons[i].ini_string_value.c_str(), nullptr, 16));
                    } break;

                    case OPTIONS_TYPE_EDIT_STR: {
                        ResourceManager_GetSettings()->SetStringValue(setting_key,
                                                                      options_menu_buttons[i].ini_string_value);

                        if (bg_image == PREFSPIC) {
                            setting_key = menu_team_name_setting[team].c_str();
                            ResourceManager_GetSettings()->SetStringValue(setting_key,
                                                                          options_menu_buttons[i].ini_string_value);

                            if (Remote_IsNetworkGame) {
                                Remote_SendNetPacket_09(team);
                            }
                        }

                    } break;

                    case OPTIONS_TYPE_CHECKBOX: {
                        if (options_menu_buttons[i].button) {
                            ResourceManager_GetSettings()->SetNumericValue(
                                setting_key, win_button_down(options_menu_buttons[i].button->GetId()));
                        }

                    } break;
                }
            }

            if (bg_image == SETUPPIC) {
                ResourceManager_DisableEnhancedGraphics =
                    !ResourceManager_GetSettings()->GetNumericValue("enhanced_graphics");
            }

            (void)ResourceManager_GetSettings()->Save();
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
                std::string setting_key = options_menu_buttons[i].setting_key;
                int32_t last_value = options_menu_buttons[i].last_rest_state;

                if (last_value != options_menu_buttons[i].rest_state) {
                    if (strcmp(setting_key.c_str(), "music_level") == 0) {
                        SetVolume(i, AUDIO_TYPE_MUSIC, last_value);
                    } else if (strcmp(setting_key.c_str(), "fx_sound_level") == 0) {
                        SetVolume(i, AUDIO_TYPE_SFX2, last_value);
                    } else if (strcmp(setting_key.c_str(), "voice_level") == 0) {
                        SetVolume(i, AUDIO_TYPE_VOICE, last_value);
                    } else if (strcmp(setting_key.c_str(), "disable_music") == 0) {
                        ResourceManager_GetSettings()->SetNumericValue("disable_music", last_value);
                        SoundManager_HaltMusicPlayback(last_value);
                    } else if (strcmp(setting_key.c_str(), "disable_fx") == 0) {
                        ResourceManager_GetSettings()->SetNumericValue("disable_fx", last_value);
                        SoundManager_HaltSfxPlayback(last_value);
                    } else if (strcmp(setting_key.c_str(), "disable_voice") == 0) {
                        ResourceManager_GetSettings()->SetNumericValue("disable_voice", last_value);
                        SoundManager_HaltVoicePlayback(last_value);
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
                            std::string setting_key;
                            int32_t value;

                            options_menu_buttons[key].button->PlaySound();
                            value = win_button_down(options_menu_buttons[key].button->GetId());
                            options_menu_buttons[key].rest_state = value;

                            setting_key = options_menu_buttons[key].setting_key;
                            ResourceManager_GetSettings()->SetNumericValue(setting_key, value);

                            if (strcmp(setting_key.c_str(), "disable_music") == 0) {
                                SoundManager_HaltMusicPlayback(value);
                            } else if (strcmp(setting_key.c_str(), "disable_fx") == 0) {
                                SoundManager_HaltSfxPlayback(value);
                            } else if (strcmp(setting_key.c_str(), "disable_voice") == 0) {
                                SoundManager_HaltVoicePlayback(value);
                            }

                        } else if (options_menu_buttons[key].type == OPTIONS_TYPE_SLIDER) {
                            control_id = key;
                            UpdateSlider(control_id);
                            is_slider_active = true;

                        } else {
                            control_id = key;
                            options_menu_buttons[key].image->Write(&window);

                            SDL_utf8strlcpy(text_buffer, options_menu_buttons[key].ini_string_value.c_str(),
                                            sizeof(text_buffer));
                            text_buffer_key = key;

                            text_edit = new (std::nothrow)
                                TextEdit(&window, text_buffer, sizeof(text_buffer),
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
                            text_edit->SetEditedText(options_menu_buttons[key].ini_string_value.c_str());
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
