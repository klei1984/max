/* Copyright (c) 2022 M.A.X. Port Team
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

#include "upgrademenu.hpp"

#include "game_manager.hpp"
#include "helpmenu.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "remote.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

UpgradeMenu::UpgradeMenu(unsigned short team, Complex* complex) : AbstractUpgradeMenu(team, UPGRADE), complex(complex) {
    stats_background = new (std::nothrow) Image(11, 293, 250, 174);
    cost_background = new (std::nothrow) Image(321, 293, 40, 174);
    button_background = new (std::nothrow) Image(283, 293, 38, 174);
    gold_background = new (std::nothrow) Image(361, 276, 40, 140);

    button_scroll_up = new (std::nothrow) Button(BLDUP__U, BLDUP__D, 471, 387);
    button_scroll_down = new (std::nothrow) Button(BLDDWN_U, BLDDWN_D, 491, 387);
    button_description = new (std::nothrow) Button(BLDDES_U, BLDDES_D, 292, 264);
    button_done = new (std::nothrow) Button(BLDONE_U, BLDONE_D, 447, 452);
    button_help = new (std::nothrow) Button(BLDHLP_U, BLDHLP_D, 420, 452);
    button_cancel = new (std::nothrow) Button(BLDCAN_U, BLDCAN_D, 357, 452);
    button_ground = new (std::nothrow) Button(SELGRD_U, SELGRD_D, 467, 411);
    button_air = new (std::nothrow) Button(SELAIR_U, SELAIR_D, 500, 411);
    button_sea = new (std::nothrow) Button(SELSEA_U, SELSEA_D, 533, 411);
    button_building = new (std::nothrow) Button(SELBLD_U, SELBLD_D, 565, 411);
    button_combat = new (std::nothrow) Button(SELCBT_U, SELCBT_D, 598, 411);

    team_gold = start_gold = UnitsManager_TeamInfo[team].team_units->GetGold();

    window2 = window1;
    window2.window.ulx = 11;
    window2.window.uly = 13;

    window2.buffer = &window1.buffer[window1.width * window2.window.uly + window2.window.ulx];
    window2.window.lrx = window2.window.ulx + 280;
    window2.window.lry = window2.window.uly + 240;

    type_selector = nullptr;

    {
        WindowInfo wininfo;

        wininfo = window1;
        wininfo.window.ulx = 482;
        wininfo.window.uly = 80;
        wininfo.window.lrx = 625;
        wininfo.window.lry = 370;
        wininfo.buffer = &window1.buffer[window1.width * wininfo.window.uly + wininfo.window.ulx];

        Text_SetFont(GNW_TEXT_FONT_5);

        Text_TextBox(window1.buffer, window1.width, _(0b85), 327, 7, 158, 18, COLOR_GREEN, true);
        Text_TextBox(&window1, _(7a2c), 209, 264, 80, 17, true, true);
        Text_TextBox(&window1, _(f199), 320, 283, 48, 16, true, true);
        Text_TextBox(&window1, _(fb18), 358, 284, 48, 17, true, true);

        Init();

        type_selector = new (std::nothrow) UnitTypeSelector(this, &wininfo, SmartObjectArray<ResourceID>(), team, 2000,
                                                            button_scroll_up, button_scroll_down);

        PopulateTeamUnitsList();
    }
}

UpgradeMenu::~UpgradeMenu() {
    GameManager_EnableMainMenu(&*GameManager_SelectedUnit);
    GameManager_ProcessTick(true);
}

void UpgradeMenu::AbstractUpgradeMenu_vfunc7() {
    AbstractUpgradeMenu::AbstractUpgradeMenu_vfunc7();

    UnitsManager_TeamInfo[team].team_units->SetGold(team_gold);

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_09(team);
    }
}

bool UpgradeMenu::ProcessKey(int key) {
    bool result;

    if (GameManager_RequestMenuExit) {
        key = 1002;
    }

    if (key > 0 && key < GNW_INPUT_PRESS) {
        event_release = false;
    }

    switch (key) {
        case GNW_KB_KEY_LALT_P: {
            PauseMenu_Menu();
            result = true;
        } break;

        case 1001:
        case GNW_KB_KEY_SHIFT_DIVIDE: {
            HelpMenu_Menu(HELPMENU_UPGRADES_SETUP, WINDOW_MAIN_WINDOW);
            result = true;
        } break;

        default: {
            result = AbstractUpgradeMenu::ProcessKey(key);
        } break;
    }

    return result;
}
