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

#include "abstractupgrademenu.hpp"

#include "cargo.hpp"
#include "cursor.hpp"
#include "researchmenu.hpp"
#include "units_manager.hpp"

AbstractUpgradeMenu::AbstractUpgradeMenu(unsigned short team, ResourceID resource_id)
    : Window(resource_id),
      team(team),
      upgrade_control_count(0),
      upgrade_control_next_uly(0),
      stats_background(nullptr),
      cost_background(nullptr),
      gold_background(nullptr),
      button_background(nullptr),
      button_scroll_up(nullptr),
      button_scroll_down(nullptr),
      button_done(nullptr),
      button_help(nullptr),
      button_cancel(nullptr),
      button_ground(nullptr),
      button_air(nullptr),
      button_sea(nullptr),
      button_building(nullptr),
      button_combat(nullptr),
      button_description(nullptr),
      type_selector(nullptr),
      start_gold(0),
      team_gold(0),
      unit_type(INVALID_ID),
      interface_icon_full(I_RAW),
      interface_icon_empty(I_RAWE),
      event_click_done(0),
      event_click_cancel(0),
      buy_upgrade_toggle_state(0),
      event_release(0) {
    for (int i = 0; i < UPGRADE_CONTROL_COUNT; ++i) {
        upgrade_controls[i] = nullptr;
    }

    for (int i = 0; i < UNIT_END; ++i) {
        unitvalues_base[i] =
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], static_cast<ResourceID>(i));
        unitvalues_actual[i] = new (std::nothrow) UnitValues(*unitvalues_base[i]);
    }

    Cursor_SetCursor(CURSOR_HAND);
    Add(true);
    FillWindowInfo(&window1);
    window1 = window2;

    /// \todo disable_main_menu();
}

bool AbstractUpgradeMenu::button_ground_rest_state = true;
bool AbstractUpgradeMenu::button_air_rest_state = true;
bool AbstractUpgradeMenu::button_sea_rest_state = true;
bool AbstractUpgradeMenu::button_building_rest_state = true;
bool AbstractUpgradeMenu::button_combat_rest_state = false;
bool AbstractUpgradeMenu::button_description_rest_state = true;

void AbstractUpgradeMenu::Init() {
    SDL_assert(stats_background);
    SDL_assert(cost_background);
    SDL_assert(gold_background);
    SDL_assert(button_background);
    SDL_assert(button_scroll_up);
    SDL_assert(button_scroll_down);
    SDL_assert(button_done);
    SDL_assert(button_help);
    SDL_assert(button_cancel);
    SDL_assert(button_ground);
    SDL_assert(button_air);
    SDL_assert(button_sea);
    SDL_assert(button_building);
    SDL_assert(button_combat);
    SDL_assert(button_description);

    button_scroll_up->CopyUpDisabled(BLDUP__X);
    button_scroll_down->CopyUpDisabled(BLDDWN_X);

    button_done->SetCaption("Done");
    button_done->SetRValue(1000);
    button_done->SetPValue(GNW_INPUT_PRESS + 1000);
    button_done->SetSfx(NDONE0);
    button_done->RegisterButton(window1.id);

    button_cancel->SetCaption("Cancel");
    button_cancel->SetRValue(1002);
    button_cancel->SetPValue(GNW_INPUT_PRESS + 1002);
    button_cancel->SetSfx(NCANC0);
    button_cancel->RegisterButton(window1.id);

    button_help->SetRValue(1001);
    button_help->SetPValue(GNW_INPUT_PRESS + 1001);
    button_help->SetSfx(NHELP0);
    button_help->RegisterButton(window1.id);

    button_ground->SetRValue(1004);
    button_ground->SetPValue(1003);
    button_ground->SetFlags(0x01);
    button_ground->SetSfx(KCARG0);
    button_ground->RegisterButton(window1.id);

    button_air->SetRValue(1006);
    button_air->SetPValue(1005);
    button_air->SetFlags(0x01);
    button_air->SetSfx(KCARG0);
    button_air->RegisterButton(window1.id);

    button_sea->SetRValue(1008);
    button_sea->SetPValue(1007);
    button_sea->SetFlags(0x01);
    button_sea->SetSfx(KCARG0);
    button_sea->RegisterButton(window1.id);

    button_building->SetRValue(1010);
    button_building->SetPValue(1009);
    button_building->SetFlags(0x01);
    button_building->SetSfx(KCARG0);
    button_building->RegisterButton(window1.id);

    button_combat->SetRValue(1012);
    button_combat->SetPValue(1011);
    button_combat->SetFlags(0x01);
    button_combat->SetSfx(KCARG0);
    button_combat->RegisterButton(window1.id);

    button_description->SetRValue(1014);
    button_description->SetPValue(1013);
    button_description->SetFlags(0x01);
    button_description->SetSfx(KCARG0);
    button_description->RegisterButton(window1.id);

    if (buy_upgrade_toggle_state) {
        button_ground->Disable(false);
        button_air->Disable(false);
        button_sea->Disable(false);
        button_building->Disable(false);
        button_combat->Disable(false);
    }

    button_ground->SetRestState(button_ground_rest_state);
    button_air->SetRestState(button_air_rest_state);
    button_sea->SetRestState(button_sea_rest_state);
    button_building->SetRestState(button_building_rest_state);
    button_combat->SetRestState(button_combat_rest_state);
    button_description->SetRestState(button_description_rest_state);

    stats_background->Copy(&window1);
    cost_background->Copy(&window1);
    button_background->Copy(&window1);
    gold_background->Copy(&window1);
}

AbstractUpgradeMenu::~AbstractUpgradeMenu() {
    for (int i = 0; i < upgrade_control_count; ++i) {
        delete upgrade_controls[i];
    }

    delete type_selector;

    delete stats_background;
    delete cost_background;
    delete button_background;
    delete gold_background;

    delete button_scroll_up;
    delete button_scroll_down;
    delete button_done;
    delete button_help;
    delete button_cancel;
    delete button_ground;
    delete button_air;
    delete button_sea;
    delete button_building;
    delete button_combat;
    delete button_description;
}

void AbstractUpgradeMenu::AddUpgrade(int id, int value1, int value2, unsigned short *attribute, int value) {
    upgrade_controls[upgrade_control_count] =
        new (std::nothrow) UpgradeControl(window1.id, cost_background->GetULX() - 38, upgrade_control_next_uly,
                                          upgrade_control_count + 1015, upgrade_control_count + 1025, &team_gold);

    upgrade_controls[upgrade_control_count]->Init(id, value1, value2, attribute, value);
    ++upgrade_control_count;
    upgrade_control_next_uly += 19;
}

void AbstractUpgradeMenu::AddUpgradeMilitary(ResourceID unit_type) {
    SmartPointer<UnitValues> base_unitvalues(UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(unit_type));

    if (base_unitvalues->GetAttribute(ATTRIB_ATTACK)) {
        SmartPointer<UnitValues> base_attribs = unitvalues_base[unit_type];
        SmartPointer<UnitValues> actual_attribs = unitvalues_actual[unit_type];

        AddUpgrade(UPGRADE_CONTROL_1, base_unitvalues->GetAttribute(ATTRIB_ATTACK),
                   base_attribs->GetAttribute(ATTRIB_ATTACK), actual_attribs->GetAttributeAddress(ATTRIB_ATTACK),
                   ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_ATTACK, unit_type));

        if (base_unitvalues->GetAttribute(ATTRIB_ROUNDS)) {
            AddUpgrade(UPGRADE_CONTROL_2, base_unitvalues->GetAttribute(ATTRIB_ROUNDS),
                       base_attribs->GetAttribute(ATTRIB_ROUNDS), actual_attribs->GetAttributeAddress(ATTRIB_ROUNDS),
                       ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_SHOTS, unit_type));
        }

        if (base_unitvalues->GetAttribute(ATTRIB_RANGE)) {
            AddUpgrade(UPGRADE_CONTROL_3, base_unitvalues->GetAttribute(ATTRIB_RANGE),
                       base_attribs->GetAttribute(ATTRIB_RANGE), actual_attribs->GetAttributeAddress(ATTRIB_RANGE),
                       ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_RANGE, unit_type));
        }

        if (base_unitvalues->GetAttribute(ATTRIB_AMMO)) {
            AddUpgrade(UPGRADE_CONTROL_4, base_unitvalues->GetAttribute(ATTRIB_RANGE),
                       base_attribs->GetAttribute(ATTRIB_RANGE), actual_attribs->GetAttributeAddress(ATTRIB_RANGE), 0);
        }
    }
}

void AbstractUpgradeMenu::AdjustRowStorage(ResourceID unit_type) {
    if (unitvalues_actual[unit_type]->GetAttribute(ATTRIB_STORAGE)) {
        switch (unit_type) {
            case ADUMP:
            case FDUMP:
            case GOLDSM:
            case ENGINEER:
            case CONSTRCT:
            case CARGOSHP:
            case SPLYTRCK:
            case FUELTRCK:
            case GOLDTRCK: {
                upgrade_control_next_uly += 19;
            } break;
        }
    }
}

void AbstractUpgradeMenu::AdjustRowConsumptions(ResourceID unit_type) {
    int raw_consumption;
    int fuel_consumption;
    int power_consumption;
    int life_consumption;

    raw_consumption = Cargo_GetRawConsumptionRate(unit_type, 1);
    fuel_consumption = Cargo_GetFuelConsumptionRate(unit_type);
    power_consumption = Cargo_GetPowerConsumptionRate(unit_type);
    life_consumption = Cargo_GetLifeConsumptionRate(unit_type);

    if (power_consumption < 0) {
        upgrade_control_next_uly += 19;
    }

    if (life_consumption < 0) {
        upgrade_control_next_uly += 19;
    }

    if (raw_consumption > 0 || fuel_consumption > 0 || power_consumption > 0 || life_consumption > 0) {
        upgrade_control_next_uly += 19;
    }
}

void AbstractUpgradeMenu::AddUpgradeGeneric(ResourceID unit_type) {
    SmartPointer<UnitValues> base_unitvalues(UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(unit_type));
    SmartPointer<UnitValues> base_attribs = unitvalues_base[unit_type];
    SmartPointer<UnitValues> actual_attribs = unitvalues_actual[unit_type];

    if (base_unitvalues->GetAttribute(ATTRIB_ARMOR)) {
        AddUpgrade(UPGRADE_CONTROL_5, base_unitvalues->GetAttribute(ATTRIB_ARMOR),
                   base_attribs->GetAttribute(ATTRIB_ARMOR), actual_attribs->GetAttributeAddress(ATTRIB_ARMOR),
                   ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_ARMOR, unit_type));
    }

    if (base_unitvalues->GetAttribute(ATTRIB_HITS)) {
        AddUpgrade(UPGRADE_CONTROL_6, base_unitvalues->GetAttribute(ATTRIB_HITS),
                   base_attribs->GetAttribute(ATTRIB_HITS), actual_attribs->GetAttributeAddress(ATTRIB_HITS),
                   ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_HITS, unit_type));
    }

    if (base_unitvalues->GetAttribute(ATTRIB_SCAN)) {
        AddUpgrade(UPGRADE_CONTROL_7, base_unitvalues->GetAttribute(ATTRIB_SCAN),
                   base_attribs->GetAttribute(ATTRIB_SCAN), actual_attribs->GetAttributeAddress(ATTRIB_SCAN),
                   ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_SCAN, unit_type));
    }
}

void AbstractUpgradeMenu::AddUpgradeMobile(ResourceID unit_type) {
    SmartPointer<UnitValues> base_unitvalues(UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(unit_type));

    if (base_unitvalues->GetAttribute(ATTRIB_SPEED)) {
        SmartPointer<UnitValues> base_attribs = unitvalues_base[unit_type];
        SmartPointer<UnitValues> actual_attribs = unitvalues_actual[unit_type];

        AddUpgrade(UPGRADE_CONTROL_8, base_unitvalues->GetAttribute(ATTRIB_SPEED),
                   base_attribs->GetAttribute(ATTRIB_SPEED), actual_attribs->GetAttributeAddress(ATTRIB_SPEED),
                   ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_SPEED, unit_type));
    }
}

bool AbstractUpgradeMenu::SelectUnit() {}

void AbstractUpgradeMenu::DrawUnitInfo(ResourceID unit_type) {}

void AbstractUpgradeMenu::AbstractUpgradeMenu_vfunc3(ResourceID unit_type) {}

bool AbstractUpgradeMenu::AbstractUpgradeMenu_vfunc4(UnitTypeSelector *type_selector, bool mode) {
    bool result;

    if (this->type_selector == type_selector) {
        ResourceID unit_type = type_selector->GetLast();

        if (mode && this->unit_type == unit_type) {
            AbstractUpgradeMenu_vfunc3(unit_type);
        } else {
            DrawUnitInfo(unit_type);
        }

        result = true;
    } else {
        result = false;
    }

    return result;
}

void AbstractUpgradeMenu::AbstractUpgradeMenu_vfunc5() {}

void AbstractUpgradeMenu::DrawUnitStats(ResourceID unit_type) {}

void AbstractUpgradeMenu::AbstractUpgradeMenu_vfunc7() {}

bool AbstractUpgradeMenu::ProcessKey(int key) {
    bool result;

    if (key > 0 && key < GNW_INPUT_PRESS) {
        event_release = false;
    }

    if (key >= 1015 && key < 1025) {
        upgrade_controls[key - 1015]->Increase();
        DrawUnitStats(unit_type);
    } else if (key >= 1025 && key < 1035) {
        upgrade_controls[key - 1025]->Decrease();
        DrawUnitStats(unit_type);
    } else {
        switch (key) {
            case 1002:
            case GNW_KB_KEY_ESCAPE: {
                event_click_cancel = true;
                event_click_done = true;
                result = true;
            } break;

            case 1000:
            case GNW_KB_KEY_RETURN: {
                AbstractUpgradeMenu_vfunc7();
                event_click_done = true;
                result = true;
            } break;

            case 1003: {
                button_ground->PlaySound();
                button_ground_rest_state = true;
                AbstractUpgradeMenu_vfunc5();
                result = true;
            } break;

            case 1004: {
                button_ground->PlaySound();
                button_ground_rest_state = false;
                AbstractUpgradeMenu_vfunc5();
                result = true;
            } break;

            case 1005: {
                button_air->PlaySound();
                button_air_rest_state = true;
                AbstractUpgradeMenu_vfunc5();
                result = true;
            } break;

            case 1006: {
                button_air->PlaySound();
                button_air_rest_state = false;
                AbstractUpgradeMenu_vfunc5();
                result = true;
            } break;

            case 1007: {
                button_sea->PlaySound();
                button_sea_rest_state = true;
                AbstractUpgradeMenu_vfunc5();
                result = true;
            } break;

            case 1008: {
                button_sea->PlaySound();
                button_sea_rest_state = false;
                AbstractUpgradeMenu_vfunc5();
                result = true;
            } break;

            case 1009: {
                button_building->PlaySound();
                button_building_rest_state = true;
                AbstractUpgradeMenu_vfunc5();
                result = true;
            } break;

            case 1010: {
                button_building->PlaySound();
                button_building_rest_state = false;
                AbstractUpgradeMenu_vfunc5();
                result = true;
            } break;

            case 1011: {
                button_combat->PlaySound();
                button_combat_rest_state = true;
                AbstractUpgradeMenu_vfunc5();
                result = true;
            } break;

            case 1012: {
                button_combat->PlaySound();
                button_combat_rest_state = false;
                AbstractUpgradeMenu_vfunc5();
                result = true;
            } break;

            case 1013: {
                button_description->PlaySound();
                button_description_rest_state = true;
                DrawUnitInfo(unit_type);
                result = true;
            } break;

            case 1014: {
                button_description->PlaySound();
                button_description_rest_state = false;
                DrawUnitInfo(unit_type);
                result = true;
            } break;

            default: {
                if (key < GNW_INPUT_PRESS) {
                    result = type_selector->ProcessKeys(key);
                } else {
                    if (!event_release) {
                        key -= GNW_INPUT_PRESS;
                        switch (key) {
                            case 1000: {
                                button_done->PlaySound();
                            } break;

                            case 1001: {
                                button_help->PlaySound();
                            } break;

                            case 1002: {
                                button_cancel->PlaySound();
                            } break;

                            default: {
                                type_selector->ProcessKeys(key + GNW_INPUT_PRESS);
                            } break;
                        }
                    }

                    event_release = true;
                    result = true;
                }
            } break;
        }
    }

    return result;
}

bool AbstractUpgradeMenu::Run() {
    type_selector->Draw();
    DrawUnitInfo(type_selector->GetLast());

    while (!event_click_done) {
        int key;

        key = get_input();

        if (key > 0) {
            ProcessKey(key);
        }

        // GameManager_sub_A0E32(0);
    }

    return event_click_cancel == false;
}
