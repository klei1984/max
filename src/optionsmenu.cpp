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

#include "cursor.hpp"
#include "gwindow.hpp"
#include "inifile.hpp"
#include "text.hpp"

struct OptionsButton {
    char type;
    const char *caption;
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

#define OPTIONS_BUTTON_DEF(type, caption, ini_param_index, ulx, range_min, range_max) \
    { (type), (caption), (ini_param_index), (ulx), (range_min), (range_max), nullptr, nullptr, nullptr, 0, 0 }

static struct OptionsButton options_menu_buttons[] = {
    OPTIONS_BUTTON_DEF(0, nullptr, ini_SETUP, 0, 0, 0),
    OPTIONS_BUTTON_DEF(6, "Volume:", ini_music_level, 25, 0, 0),
    OPTIONS_BUTTON_DEF(5, "Enhanced graphics (requires 16MB)", ini_enhanced_graphics, 210, 0, 0),
    OPTIONS_BUTTON_DEF(1, "Music", ini_music_level, 25, 0, 100),
    OPTIONS_BUTTON_DEF(5, "Disable Music", ini_disable_music, 210, 0, 1),
    OPTIONS_BUTTON_DEF(1, "FX", ini_fx_sound_level, 25, 0, 100),
    OPTIONS_BUTTON_DEF(5, "FX Disabled", ini_disable_fx, 210, 0, 1),
    OPTIONS_BUTTON_DEF(1, "Voice", ini_voice_level, 25, 0, 100),
    OPTIONS_BUTTON_DEF(5, "Voice Disabled", ini_disable_voice, 210, 0, 1),
    OPTIONS_BUTTON_DEF(5, "Auto-Save Enabled", ini_auto_save, 25, 0, 1),
    OPTIONS_BUTTON_DEF(3, "IPX Socket:", ini_ipx_socket, 210, 0, 0x7FFF),
    OPTIONS_BUTTON_DEF(4, "Player Name:", ini_player_name, 25, 0, 0),
    OPTIONS_BUTTON_DEF(0, nullptr, ini_PREFERENCES, 0, 0, 0),
    OPTIONS_BUTTON_DEF(5, "Animate Effects", ini_effects, 25, 0, 1),
    OPTIONS_BUTTON_DEF(5, "Click to Scroll", ini_click_scroll, 210, 0, 1),
    OPTIONS_BUTTON_DEF(5, "Double Unit Steps", ini_fast_movement, 25, 0, 1),
    OPTIONS_BUTTON_DEF(5, "Track Selected Unit", ini_follow_unit, 210, 0, 1),
    OPTIONS_BUTTON_DEF(5, "Halt movement when enemy is detected", ini_enemy_halt, 25, 0, 1),
    OPTIONS_BUTTON_DEF(5, "Auto-Select Next Unit", ini_auto_select, 210, 0, 1),
    OPTIONS_BUTTON_DEF(1, "Scroll Speed", ini_quick_scroll, 25, 4, 128),
    OPTIONS_BUTTON_DEF(0, nullptr, ini_OPTIONS, 0, 0, 0),
    OPTIONS_BUTTON_DEF(2, "Turn Time:", ini_timer, 25, 0, 32767),
    OPTIONS_BUTTON_DEF(2, "End Turn Time:", ini_endturn, 210, 0, 32767),
    OPTIONS_BUTTON_DEF(6, "Play Mode: %s", ini_play_mode, 25, 0, 0),
    OPTIONS_BUTTON_DEF(6, "Computer Player(s): %s", ini_opponent, 25, 0, 0),
    OPTIONS_BUTTON_DEF(6, "Game ends at %i %s.", ini_victory_limit, 25, 0, 0),
    OPTIONS_BUTTON_DEF(0, nullptr, 0, 0, 0, 0),
    OPTIONS_BUTTON_DEF(5, "Disable Fire", ini_disable_fire, 25, 0, 1),
    OPTIONS_BUTTON_DEF(5, "Real Time", ini_real_time, 210, 0, 1),
    OPTIONS_BUTTON_DEF(2, "Red Team", ini_red_team_player, 25, 0, 3),
    OPTIONS_BUTTON_DEF(2, "Green Team", ini_green_team_player, 210, 0, 3),
    OPTIONS_BUTTON_DEF(2, "Blue Team", ini_blue_team_player, 25, 0, 3),
    OPTIONS_BUTTON_DEF(2, "Gray Team", ini_gray_team_player, 210, 0, 3),
};

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
            options_menu_buttons[i].type == 5) {
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

        delete options_menu_buttons[i].button;
        options_menu_buttons[i].ini_string_value = nullptr;
    }

    if (bg_image == PREFSPIC) {
        sub_A0EFE(1);
    }
}

void OptionsMenu::Init() {}
bool OptionsMenu::Run() {}
