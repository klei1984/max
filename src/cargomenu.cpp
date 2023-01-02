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

#include "cargomenu.hpp"

#include "helpmenu.hpp"
#include "inifile.hpp"
#include "menu.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

CargoMenu::CargoMenu(unsigned short team) : AbstractUpgradeMenu(team, CARGOPIC) {
    type_selector = nullptr;
    buy_upgrade_toggle_state = true;
    Rect bounds1;
    Rect bounds2;
    Rect bounds3;
    Rect bounds4;
    WindowInfo wininfo1;
    WindowInfo wininfo2;

    start_gold = ini_get_setting(INI_START_GOLD) + ini_clans.GetClanGold(UnitsManager_TeamInfo[team].team_clan);

    UnitsManager_TeamInfo[team].stats_gold_spent_on_upgrades = 0;
    interface_icon_full = I_GOLD;
    interface_icon_empty = I_GOLDE;

    unit_types2 = UnitsManager_TeamMissionSupplies[team].units;
    cargos = UnitsManager_TeamMissionSupplies[team].cargos;

    if (unit_types2->GetCount()) {
        SmartObjectArray<ResourceID> default_units;

        UnitsManager_AddAxisMissionLoadout(team, default_units);

        unit_count = default_units.GetCount() + 3;
        team_gold = UnitsManager_TeamMissionSupplies[team].team_gold;

        for (int i = 0; i < UNIT_END; ++i) {
            unitvalues_base[i] = UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(i);
        }

    } else {
        unit_count = UnitsManager_AddDefaultMissionLoadout(team);
        team_gold = start_gold;
    }

    stats_background = new (std::nothrow) Image(11, 293, 250, 174);
    cost_background = new (std::nothrow) Image(321, 293, 40, 174);
    button_background = new (std::nothrow) Image(283, 293, 38, 174);
    gold_background = new (std::nothrow) Image(361, 276, 40, 140);

    scrollbar = new (std::nothrow)
        LimitedScrollbar(this, rect_init(&bounds1, 421, 301, 441, 416), rect_init(&bounds2, 421, 276, 441, 286),
                         VERTRAW, 1104, 1105, 1106, 5, true);

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
    button_cargo_up = new (std::nothrow) Button(BLDUP__U, BLDUP__D, 413, 424);
    button_cargo_down = new (std::nothrow) Button(BLDDWN_U, BLDDWN_D, 433, 424);
    button_purchase_list_up = new (std::nothrow) Button(BLDUP__U, BLDUP__D, 327, 240);
    button_purchase_list_down = new (std::nothrow) Button(BLDDWN_U, BLDDWN_D, 348, 240);
    button_buy_upgrade_toggle = new (std::nothrow) Button(CRGTOG_U, CRGTOG_D, 542, 446);
    button_delete = new (std::nothrow) Button(CRGDEL_U, CRGDEL_D, 412, 240);
    button_buy = new (std::nothrow) Button(CRGBUY_U, CRGBUY_D, 590, 386);

    text_font(GNW_TEXT_FONT_5);

    button_delete->SetCaption("Delete");
    button_buy->SetCaption("Buy");

    button_buy_upgrade_toggle->SetCaption("Buy Units", rect_init(&bounds3, 20, 0, 80, 15), Fonts_DarkOrageColor,
                                          Fonts_GoldColor);
    button_buy_upgrade_toggle->SetCaption("Upgrades", rect_init(&bounds4, 20, 15, 80, 31));

    button_scroll_up->CopyUpDisabled(BLDUP__X);
    button_scroll_down->CopyUpDisabled(BLDDWN_X);
    button_cargo_up->CopyUpDisabled(BLDUP__X);
    button_cargo_down->CopyUpDisabled(BLDDWN_X);

    button_ground->CopyUpDisabled(CRGSELDX);
    button_ground->CopyDownDisabled(CRGSELDX);
    button_air->CopyUpDisabled(CRGSELDX);
    button_air->CopyDownDisabled(CRGSELDX);
    button_sea->CopyUpDisabled(CRGSELDX);
    button_sea->CopyDownDisabled(CRGSELDX);
    button_building->CopyUpDisabled(CRGSELDX);
    button_building->CopyDownDisabled(CRGSELDX);
    button_combat->CopyUpDisabled(CRGSELDX);
    button_combat->CopyDownDisabled(CRGSELDX);

    button_purchase_list_up->CopyUpDisabled(BLDUP__X);
    button_purchase_list_down->CopyUpDisabled(BLDDWN_X);
    button_buy->CopyUpDisabled(CRGBUY_X);
    button_delete->CopyUpDisabled(CRGDEL_X);
    button_cargo_up->SetPValue(1104);
    button_cargo_down->SetPValue(1105);
    button_buy_upgrade_toggle->SetPValue(1101);
    button_buy_upgrade_toggle->SetRValue(1100);
    button_buy_upgrade_toggle->SetFlags(1);
    button_delete->SetRValue(1103);
    button_buy->SetRValue(1102);
    button_buy_upgrade_toggle->SetSfx(KCARG0);
    button_cargo_up->RegisterButton(window1.id);
    button_cargo_down->RegisterButton(window1.id);
    button_buy_upgrade_toggle->RegisterButton(window1.id);
    button_delete->RegisterButton(window1.id);
    button_buy->RegisterButton(window1.id);

    scrollbar->Register();

    window2 = window1;

    window2.window.ulx = 11;
    window2.window.uly = 13;
    window2.window.lrx = window2.window.ulx + 280;
    window2.window.lry = window2.window.uly + 240;

    window2.buffer = &window1.buffer[window2.window.ulx + window2.window.uly * window1.width];

    {
        ResourceID id;

        id = CONSTRCT;
        unit_types1.PushBack(&id);

        id = ENGINEER;
        unit_types1.PushBack(&id);

        id = SURVEYOR;
        unit_types1.PushBack(&id);

        id = SCOUT;
        unit_types1.PushBack(&id);

        id = SCANNER;
        unit_types1.PushBack(&id);

        id = TANK;
        unit_types1.PushBack(&id);

        id = ARTILLRY;
        unit_types1.PushBack(&id);

        id = ROCKTLCH;
        unit_types1.PushBack(&id);

        id = MISSLLCH;
        unit_types1.PushBack(&id);

        id = MINELAYR;
        unit_types1.PushBack(&id);

        id = REPAIR;
        unit_types1.PushBack(&id);

        id = SPLYTRCK;
        unit_types1.PushBack(&id);

        id = FUELTRCK;
        unit_types1.PushBack(&id);

        id = SP_FLAK;
        unit_types1.PushBack(&id);

        id = GOLDTRCK;
        unit_types1.PushBack(&id);
    }

    wininfo1 = window1;

    wininfo1.window.ulx = 482;
    wininfo1.window.uly = 80;
    wininfo1.window.lrx = 605;
    wininfo1.window.lry = 370;

    wininfo1.buffer = &window1.buffer[window1.width * wininfo1.window.uly + wininfo1.window.ulx];

    Text_TextBox(window1.buffer, window1.width, "Available", 482, 57, 143, 10, 0xA2, false);

    Text_TextBox(window1.buffer, window1.width, "Cost", 625 - text_width("Cost"), 57, text_width("Cost"), 10,
                 COLOR_YELLOW, false);

    draw_line(window1.buffer, window1.width, 482, 69, 625, 69, COLOR_CHROME_YELLOW);

    wininfo2 = window1;

    wininfo2.window.ulx = 337;
    wininfo2.window.uly = 45;
    wininfo2.window.lrx = 455;
    wininfo2.window.lry = 230;

    wininfo2.buffer = &wininfo2.buffer[wininfo2.window.uly * window1.width + wininfo2.window.ulx];

    cargo_selector = new (std::nothrow) CargoSelector(this, &wininfo2, unit_types2, cargos, team, 3000,
                                                      button_purchase_list_up, button_purchase_list_down);

    text_font(GNW_TEXT_FONT_5);

    Text_TextBox(window1.buffer, window1.width, "Purchased", 337, 22, 118, 10, 0xA2, true);

    draw_line(window1.buffer, window1.width, 337, 34, 455, 34, COLOR_CHROME_YELLOW);

    text_font(GNW_TEXT_FONT_5);

    Text_TextBox(window1.buffer, window1.width, "Purchase Menu", 473, 7, 158, 18, COLOR_GREEN, true);
    Text_TextBox(&window1, "Description", 209, 264, 80, 17, true, true);
    Text_TextBox(&window1, "Cost", 320, 283, 48, 16, true, true);
    Text_TextBox(&window1, "Credit", 358, 283, 48, 16, true, true);
    Text_TextBox(&window1, "Cargo", 409, 283, 48, 16, true, true);

    Init();

    {
        SmartObjectArray<ResourceID> types;

        active_selector = new (std::nothrow)
            PurchaseTypeSelector(this, &wininfo1, types, team, 2000, button_scroll_up, button_scroll_down, 128, 15);
    }

    type_selector = active_selector;

    PopulateTeamUnitsList();

    cargo_selector->Draw();

    Select(0);
}

CargoMenu::~CargoMenu() {
    delete cargo_selector;
    delete button_cargo_up;
    delete button_cargo_down;
    delete button_purchase_list_up;
    delete button_purchase_list_down;
    delete button_buy_upgrade_toggle;
    delete button_delete;
    delete button_buy;
    delete scrollbar;
}

void CargoMenu::Select(int index) {
    ResourceID unit;
    ResourceID vbar;

    if (index) {
        if (index == 1) {
            scrollbar->SetXferTakeMax(4);
        } else {
            scrollbar->SetXferTakeMax(0);
        }

    } else {
        scrollbar->SetXferTakeMax(8);
    }

    if (index >= unit_count) {
        button_delete->Enable();
    } else {
        button_delete->Disable();
    }

    unit = cargo_selector->GetLast();

    switch (unit) {
        case FUELTRCK: {
            vbar = VERTFUEL;
        } break;
        case GOLDTRCK: {
            vbar = VERTGOLD;
        } break;
        default: {
            vbar = VERTRAW;
        } break;
    }

    if (unit == GOLDTRCK) {
        scrollbar->SetFreeCapacity(0);
    } else {
        scrollbar->SetFreeCapacity(unitvalues_actual[unit]->GetAttribute(ATTRIB_STORAGE) / 5);
    }

    scrollbar->SetMaterialBar(vbar);

    scrollbar->SetValue((*cargos[index]) / 5);

    AbstractUpgradeMenu::DrawUnitInfo(unit);
    DrawUnitStats(unit);
}

bool CargoMenu::EventHandler(Event* event) {
    bool result;

    if (event->GetEventId() == EVENTS_GET_EVENT_ID(ScrollbarEvent)) {
        EventScrollbarChange* scrollbar_event = dynamic_cast<EventScrollbarChange*>(event);

        team_gold += scrollbar_event->GetValue() - scrollbar_event->GetScrollbarValue();

        *cargos[cargo_selector->GetPageMaxIndex()] = scrollbar_event->GetScrollbarValue() * 5;

        DrawUnitStats(cargo_selector->GetLast());

        result = true;
    } else {
        result = AbstractUpgradeMenu::EventHandler(event);
    }

    return result;
}

void CargoMenu::DrawUnitInfo(ResourceID unit_type) {
    text_font(GNW_TEXT_FONT_5);

    if (unit_type != INVALID_ID) {
        if ((unit_types1->Find(&unit_type) != -1) &&
            (Cargo_GetRawConsumptionRate(LANDPLT, 1) * unitvalues_actual[unit_type]->GetAttribute(ATTRIB_TURNS)) <=
                team_gold) {
            button_buy->Enable();
        } else {
            button_buy->Disable();
        }
    }

    AbstractUpgradeMenu::DrawUnitInfo(unit_type);
}

void CargoMenu::AbstractUpgradeMenu_vfunc3(ResourceID unit_type) { BuyUnit(); }

bool CargoMenu::AbstractUpgradeMenu_vfunc4(UnitTypeSelector* selector, bool mode) {
    bool result;

    active_selector = selector;

    if (cargo_selector == selector) {
        if (mode && cargo_selector->GetLast() == unit_type) {
            DeleteUnit();
        } else {
            Select(selector->GetPageMaxIndex());
        }

        result = true;
    } else {
        result = AbstractUpgradeMenu::AbstractUpgradeMenu_vfunc4(selector, mode);
    }

    return result;
}

void CargoMenu::PopulateTeamUnitsList() {
    if (buy_upgrade_toggle_state) {
        type_selector->AddItems(unit_types1);

        button_ground->Disable();
        button_air->Disable();
        button_sea->Disable();
        button_building->Disable();
        button_combat->Disable();

    } else {
        button_ground->Enable();
        button_air->Enable();
        button_sea->Enable();
        button_building->Enable();
        button_combat->Enable();

        AbstractUpgradeMenu::PopulateTeamUnitsList();
    }
}

void CargoMenu::DrawUnitStats(ResourceID unit_type) {
    cargo_selector->Draw();

    scrollbar->SetXferGiveMax((*cargos[cargo_selector->GetPageMaxIndex()]) / 5 + team_gold);
    scrollbar->RefreshScreen();
    AbstractUpgradeMenu::DrawUnitStats(unit_type);
}

void CargoMenu::AbstractUpgradeMenu_vfunc7() {
    int cargo;

    AbstractUpgradeMenu::AbstractUpgradeMenu_vfunc7();

    UnitsManager_TeamMissionSupplies[team].team_gold = team_gold;

    for (int i = unit_count; i < unit_types2->GetCount(); ++i) {
        UnitsManager_TeamInfo[team].stats_gold_spent_on_upgrades -=
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], *unit_types2[i])
                ->GetAttribute(ATTRIB_TURNS) *
            Cargo_GetRawConsumptionRate(LANDPLT, 1);
    }

    for (int i = 0; i < unit_types2->GetCount(); ++i) {
        cargo = *cargos[i];

        switch (i) {
            case 0: {
                cargo -= 40;
            } break;

            case 1: {
                cargo -= 20;
            } break;
        }

        UnitsManager_TeamInfo[team].stats_gold_spent_on_upgrades -= cargo / 5;
    }
}

bool CargoMenu::ProcessKey(int key) {
    bool result;

    if (key > 0 && key < GNW_INPUT_PRESS) {
        event_release = false;
    }

    if (key >= 1015 && key < 1025) {
        if (upgrade_controls[key - 1015]->GetId() == 9) {
            UpdateTeamGold(1);
        }

    } else if (key >= 1025 && key < 1035) {
        if (upgrade_controls[key - 1025]->GetId() == 9) {
            UpdateTeamGold(-1);
        }
    }

    switch (key) {
        case 1100: {
            button_buy_upgrade_toggle->PlaySound();
            buy_upgrade_toggle_state = true;

            PopulateTeamUnitsList();
            result = true;

        } break;

        case 1101: {
            button_buy_upgrade_toggle->PlaySound();
            buy_upgrade_toggle_state = false;

            PopulateTeamUnitsList();
            result = true;

        } break;

        case 1102: {
            BuyUnit();
            result = true;

        } break;

        case 1103: {
            DeleteUnit();
            result = true;

        } break;

        case GNW_KB_KEY_LALT_P: {
            PauseMenu_Menu();
            result = true;

        } break;

        case 1001:
        case GNW_KB_KEY_SHIFT_DIVIDE: {
            HelpMenu_Menu(HELPMENU_CARGO_SETUP, WINDOW_MAIN_WINDOW);
            result = true;

        } break;

        default: {
            if (active_selector->ProcessKeys(key) || cargo_selector->ProcessKeys(key) || scrollbar->ProcessKey(key)) {
                result = true;

            } else {
                if (AbstractUpgradeMenu::ProcessKey(key)) {
                    if (key >= 1025 && key < 1035) {
                        UpdateScrollbar();
                    }

                    result = true;

                } else {
                    result = false;
                }
            }

        } break;
    }

    return result;
}

void CargoMenu::BuyUnit() {
    ResourceID last;
    int cost;

    last = type_selector->GetLast();

    if (unit_types1->Find(&last) != -1) {
        cost = Cargo_GetRawConsumptionRate(LANDPLT, 1) * unitvalues_actual[last]->GetAttribute(ATTRIB_TURNS);

        if (team_gold >= cost) {
            cargo_selector->PushBack(last);
            team_gold -= cost;

            if (team_gold < cost) {
                button_buy->Disable();
            }

            DrawUnitStats(last);
        }
    }
}

void CargoMenu::DeleteUnit() {
    if (cargo_selector->GetPageMaxIndex() >= unit_count) {
        ResourceID last;

        last = cargo_selector->GetLast();

        team_gold += Cargo_GetRawConsumptionRate(LANDPLT, 1) * unitvalues_actual[last]->GetAttribute(ATTRIB_TURNS);
        team_gold += (*cargos[cargo_selector->GetPageMaxIndex()]) / 5;

        cargo_selector->RemoveLast();

        last = type_selector->GetLast();

        if ((unit_types1->Find(&last) != -1) &&
            (Cargo_GetRawConsumptionRate(LANDPLT, 1) * unitvalues_actual[last]->GetAttribute(ATTRIB_TURNS) <=
             team_gold)) {
            button_buy->Enable();
        }

        DrawUnitStats(cargo_selector->GetLast());
    }
}

void CargoMenu::UpdateScrollbar() {
    int storage_max;
    bool flag;

    flag = false;

    for (int i = 0; i < unit_types2->GetCount(); ++i) {
        storage_max = unitvalues_actual[*unit_types2[i]]->GetAttribute(ATTRIB_STORAGE);

        if (*cargos[i] > storage_max) {
            flag = true;

            team_gold += ((*cargos[i]) - storage_max) / 5;
            *cargos[i] = storage_max;

            if (cargo_selector->GetPageMaxIndex() == i) {
                scrollbar->SetFreeCapacity(storage_max / 5);
                scrollbar->SetValue(*cargos[i] / 5);
            }
        }
    }

    if (flag) {
        DrawUnitStats(cargo_selector->GetLast());
    }
}

void CargoMenu::UpdateTeamGold(int factor) {
    ResourceID last;

    last = type_selector->GetLast();

    for (int i = 2; i < unit_types2->GetCount(); ++i) {
        if (last == *unit_types2[i]) {
            team_gold -= Cargo_GetRawConsumptionRate(LANDPLT, 1) * factor;
        }
    }
}

UnitValues* CargoMenu::GetCurrentUnitValues(ResourceID unit_type) { return &*unitvalues_actual[unit_type]; }
