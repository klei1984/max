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

#include "clanselectmenu.hpp"

#include "helpmenu.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "window_manager.hpp"

struct ClanSelectMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char* label;
    int32_t event_code;
    void (ClanSelectMenu::*event_handler)();
    ResourceID sfx;
};

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    {{(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx)}

static struct MenuTitleItem clan_select_menu_screen_title = {{230, 6, 410, 26}, _(897f)};
static struct MenuTitleItem clan_select_menu_screen_text[] = {
    MENU_TITLE_ITEM_DEF(41, 350, 610, 409, ""),
    MENU_TITLE_ITEM_DEF(330, 350, 610, 409, ""),
};

static struct MenuTitleItem clan_select_menu_clan_icons[] = {
    MENU_TITLE_ITEM_DEF(32, 140, 142, 160, ""),  MENU_TITLE_ITEM_DEF(188, 140, 298, 160, ""),
    MENU_TITLE_ITEM_DEF(344, 140, 454, 160, ""), MENU_TITLE_ITEM_DEF(497, 140, 607, 160, ""),
    MENU_TITLE_ITEM_DEF(32, 290, 142, 310, ""),  MENU_TITLE_ITEM_DEF(188, 290, 298, 310, ""),
    MENU_TITLE_ITEM_DEF(344, 290, 454, 310, ""), MENU_TITLE_ITEM_DEF(497, 290, 607, 310, ""),
};

static struct ClanSelectMenuControlItem clan_select_menu_controls[] = {
    MENU_CONTROL_DEF(46, 44, 0, 0, CH_CN1_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CCHOS0),
    MENU_CONTROL_DEF(201, 44, 0, 0, CH_CN2_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CCRIM0),
    MENU_CONTROL_DEF(355, 44, 0, 0, CH_CN3_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CVONG0),
    MENU_CONTROL_DEF(510, 44, 0, 0, CH_CN4_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CAYER0),
    MENU_CONTROL_DEF(46, 194, 0, 0, CH_CN5_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CMUSA0),
    MENU_CONTROL_DEF(201, 194, 0, 0, CH_CN6_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CSACR0),
    MENU_CONTROL_DEF(355, 194, 0, 0, CH_CN7_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CKNIG0),
    MENU_CONTROL_DEF(510, 194, 0, 0, CH_CN8_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CAXIS0),
    MENU_CONTROL_DEF(243, 438, 0, 0, MNUBTN3U, _(7e91), 0, &ClanSelectMenu::EventRandom, CRAND0),
    MENU_CONTROL_DEF(354, 438, 0, 0, MNUBTN4U, _(f5ee), GNW_KB_KEY_SHIFT_ESCAPE, &ClanSelectMenu::EventCancel, CCANC0),
    MENU_CONTROL_DEF(465, 438, 0, 0, MNUBTN5U, _(eb1b), GNW_KB_KEY_SHIFT_DIVIDE, &ClanSelectMenu::EventHelp, CHELP0),
    MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, _(aff0), GNW_KB_KEY_SHIFT_RETURN, &ClanSelectMenu::EventDone, CDONE0),
};

void ClanSelectMenu::Init(int32_t team) {
    ButtonID button_list[8];

    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);

    team_clan_ini_id = static_cast<IniParameter>(INI_RED_TEAM_CLAN + team);
    team_clan = ini_get_setting(team_clan_ini_id);
    team_clan_selection = team_clan;
    image = nullptr;
    event_click_done_cancel_random = false;

    mouse_hide();
    WindowManager_LoadBigImage(CLANSEL, window, window->width, false, false, -1, -1, true);

    for (int32_t i = 0; i < CLAN_SELECT_MENU_ITEM_COUNT; ++i) {
        ButtonInit(i, i <= 7);
    }

    for (int32_t i = 0; i < 8; ++i) {
        button_list[i] = buttons[i]->GetId();
    }

    win_group_radio_buttons(8, button_list);

    image = new (std::nothrow)
        Image(WindowManager_ScaleUlx(window, clan_select_menu_screen_text[0].bounds.ulx),
              WindowManager_ScaleUly(window, clan_select_menu_screen_text[0].bounds.uly),
              clan_select_menu_screen_text[0].bounds.lrx - clan_select_menu_screen_text[0].bounds.ulx,
              clan_select_menu_screen_text[0].bounds.lry - clan_select_menu_screen_text[0].bounds.uly);
    image->Copy(window);

    Text_SetFont(GNW_TEXT_FONT_5);

    menu_draw_menu_title(window, &clan_select_menu_screen_title, COLOR_GREEN, true);
    SelectMenuItems();

    if (team_clan_selection) {
        buttons[team_clan_selection - 1]->SetRestState(true);
    }

    mouse_show();
}

void ClanSelectMenu::Deinit() {
    delete image;

    for (int32_t i = 0; i < CLAN_SELECT_MENU_ITEM_COUNT; ++i) {
        delete buttons[i];
    }
}

void ClanSelectMenu::ClanSelection() {
    team_clan_selection = key + 1;
    SelectMenuItems();
}

void ClanSelectMenu::EventRandom() {
    team_clan = TEAM_CLAN_RANDOM;
    event_click_done_cancel_random = true;
}

void ClanSelectMenu::EventCancel() { event_click_done_cancel_random = true; }

void ClanSelectMenu::EventHelp() { HelpMenu_Menu(HELPMENU_CLAN_SETUP, WINDOW_MAIN_WINDOW); }

void ClanSelectMenu::ButtonInit(int32_t index, int32_t mode) {
    struct ClanSelectMenuControlItem* control = &clan_select_menu_controls[index];

    Text_SetFont(GNW_TEXT_FONT_1);

    if (index >= 0 && index <= 7) {
        buttons[index] = new (std::nothrow) Button(control->image_id, static_cast<ResourceID>(control->image_id + 1),
                                                   WindowManager_ScaleUlx(window, control->bounds.ulx),
                                                   WindowManager_ScaleUly(window, control->bounds.uly));
        buttons[index]->Copy(static_cast<ResourceID>(CLN1LOGO + index), 41, 40);
    } else if (control->image_id == INVALID_ID) {
        buttons[index] = new (std::nothrow) Button(
            WindowManager_ScaleUlx(window, control->bounds.ulx), WindowManager_ScaleUly(window, control->bounds.uly),
            control->bounds.lrx - control->bounds.ulx, control->bounds.lry - control->bounds.uly);
    } else {
        buttons[index] = new (std::nothrow) Button(control->image_id, static_cast<ResourceID>(control->image_id + 1),
                                                   WindowManager_ScaleUlx(window, control->bounds.ulx),
                                                   WindowManager_ScaleUly(window, control->bounds.uly));

        if (control->label) {
            buttons[index]->SetCaption(control->label);
        }
    }

    if (mode) {
        buttons[index]->SetFlags(5);
        buttons[index]->SetPValue(1000 + index);
    } else {
        buttons[index]->SetFlags(0);
        buttons[index]->SetRValue(1000 + index);
        buttons[index]->SetPValue(GNW_INPUT_PRESS + index);
    }

    buttons[index]->SetSfx(control->sfx);
    buttons[index]->RegisterButton(window->id);

    menu_item[index].r_value = 1000 + index;
    menu_item[index].event_code = control->event_code;
    menu_item[index].event_handler = control->event_handler;
}

void ClanSelectMenu::SelectMenuItems() {
    char buffer[500];
    char buffer2[500];
    char* pointer;
    int32_t color;
    int32_t index;

    for (int32_t i = 0; i < 8; ++i) {
        ini_clans.GetClanName(i + 1, buffer, 100);
        clan_select_menu_clan_icons[i].title = buffer;
        color = ((i + 1) == team_clan_selection) ? COLOR_GREEN : 0xA2;

        menu_draw_menu_title(window, &clan_select_menu_clan_icons[i], color | GNW_TEXT_OUTLINE, true);
    }

    image->Write(window, WindowManager_ScaleUlx(window, clan_select_menu_screen_text[0].bounds.ulx),
                 WindowManager_ScaleUly(window, clan_select_menu_screen_text[0].bounds.uly));
    ini_clans.GetClanText(team_clan_selection, buffer, 100);

    index = 0;

    DrawClanUpgrades(buffer, index++, COLOR_GREEN);

    while (team_clan_selection) {
        ini_clans.GetStringValue(buffer2, 100);
        pointer = strstr(buffer2, "=");

        if (!pointer) {
            break;
        }

        pointer[0] = '\0';

        strcpy(buffer, buffer2);

        if (stricmp(buffer, "Name") && stricmp(buffer, "Text")) {
            strcat(buffer, ": ");
            strcat(buffer, &pointer[1]);

            DrawClanUpgrades(buffer, index++, 0xA2);
        }
    }

    win_draw(window->id);
}

void ClanSelectMenu::DrawClanUpgrades(const char* text, int32_t index, int32_t color) {
    char buffer[500];
    int32_t position;
    int32_t limit1;
    int32_t limit2;

    position = (index / 5) ? 1 : 0;
    buffer[0] = '\0';
    limit1 = index % 5;
    limit2 = 5 - limit1;

    for (int32_t i = 0; i < limit1; ++i) {
        strcat(buffer, "\n");
    }

    strcat(buffer, text);

    for (int32_t i = 0; i < limit2; ++i) {
        strcat(buffer, "\n");
    }

    clan_select_menu_screen_text[position].title = buffer;
    menu_draw_menu_title(window, &clan_select_menu_screen_text[position], color | GNW_TEXT_OUTLINE, false);
}

void ClanSelectMenu::EventDone() {
    team_clan = team_clan_selection;
    event_click_done_cancel_random = true;
}
