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

#include <format>

#include "helpmenu.hpp"
#include "inifile.hpp"
#include "menu.hpp"
#include "resource_manager.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

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

void ClanSelectMenu::EventHelp() { HelpMenu_Menu("CLAN_SETUP", WINDOW_MAIN_WINDOW); }

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
    auto clans = ResourceManager_GetClans();

    if (clans) {
        for (int32_t i = 0; i < 8; ++i) {
            const auto clan_id = ResourceManager_GetClanID(static_cast<TeamClanType>(i + 1));
            const int32_t color = ((i + 1) == team_clan_selection) ? COLOR_GREEN : 0xA2;

            clan_select_menu_clan_icons[i].title = clans->GetName(clan_id);

            menu_draw_menu_title(window, &clan_select_menu_clan_icons[i], color | GNW_TEXT_OUTLINE, true);
        }
    }

    image->Write(window, WindowManager_ScaleUlx(window, clan_select_menu_screen_text[0].bounds.ulx),
                 WindowManager_ScaleUly(window, clan_select_menu_screen_text[0].bounds.uly));

    if (clans && team_clan_selection) {
        DrawClanUpgrades();
    }

    win_draw(window->id);
}

void ClanSelectMenu::DrawClanUpgrades() {
    const char* attribute_names[Clans::ATTRIB_COUNT] = {
        _(6976), _(62f5), _(d81e), _(fca3), _(4027), _(bbcc), _(2269),
        _(206c), _(59ad), _(49a2), _(24d8), _(4a91), _(e9d8),
    };

    auto clans = ResourceManager_GetClans();
    const auto clan_id = ResourceManager_GetClanID(static_cast<TeamClanType>(team_clan_selection));
    const auto description = clans->GetDescription(clan_id);
    std::string unit_bonuses_text;

    for (ResourceID unit_id = UNIT_START; unit_id < UNIT_END; unit_id = static_cast<ResourceID>(unit_id + 1)) {
        std::string unit_bonuses;

        for (int32_t attr = Clans::ATTRIB_TURNS_TO_BUILD; attr < Clans::ATTRIB_COUNT; ++attr) {
            int32_t value = clans->GetUnitTradeoff(clan_id, unit_id, static_cast<Clans::AttributeID>(attr));

            if (value != 0) {
                if (!unit_bonuses.empty()) {
                    unit_bonuses += ", ";
                }

                // use non breaking space
                unit_bonuses += std::format("{:+}\xC2\xA0{}", value, attribute_names[attr]);
            }
        }

        if (!unit_bonuses.empty()) {
            const char* unit_name = UnitsManager_BaseUnits[unit_id].GetSingularName();

            unit_bonuses_text += std::format(" {}: {}.", unit_name, unit_bonuses);
        }
    }

    const int32_t description_width_pixels = Text_GetWidth(description.c_str());
    const int32_t nbsp_width_pixels = Text_GetGlyphWidth(U'\u00A0');

    SDL_assert(nbsp_width_pixels != 0);

    const int32_t space_count = description_width_pixels / nbsp_width_pixels + 1;

    std::string spacing;

    // spacing must use non breaking space as normal spaces are merged down to one by the text engine
    for (int32_t i = 0; i < space_count; ++i) {
        spacing += "\xC2\xA0";
    }

    unit_bonuses_text = spacing + unit_bonuses_text;

    clan_select_menu_screen_text[0].title = description;

    menu_draw_menu_title(window, &clan_select_menu_screen_text[0], COLOR_GREEN | GNW_TEXT_OUTLINE, false, false);

    clan_select_menu_screen_text[0].title = unit_bonuses_text;
    menu_draw_menu_title(window, &clan_select_menu_screen_text[0], 0xA2 | GNW_TEXT_OUTLINE, false, false);
}

void ClanSelectMenu::EventDone() {
    team_clan = team_clan_selection;
    event_click_done_cancel_random = true;
}
