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

#include "buildmenu.hpp"

#include "builder.hpp"
#include "cargo.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "missionmanager.hpp"
#include "remote.hpp"
#include "reportstats.hpp"
#include "resource_manager.hpp"
#include "sound_manager.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "unitstats.hpp"
#include "unittypeselector.hpp"
#include "window_manager.hpp"

class BuildUnitTypeSelector : public UnitTypeSelector {
    uint8_t *sprite;
    UnitInfo *unit;

public:
    BuildUnitTypeSelector(Window *window, WindowInfo *window_info, SmartObjectArray<ResourceID> unit_types,
                          UnitInfo *unit, int32_t key_code, Button *button_scroll_up, Button *button_scroll_down);
    ~BuildUnitTypeSelector();

    void Draw();
};

class AbstractBuildMenu : public Window {
protected:
    WindowInfo window;
    UnitInfo *unit;

    bool event_success;
    bool event_click_cancel;
    bool event_click_path_build;
    bool event_release;

    uint16_t build_rate;

    SmartObjectArray<ResourceID> unit_types;
    BuildUnitTypeSelector *selector;

    Button *button_up_arrow;
    Button *button_down_arrow;
    Button *button_description;
    Button *button_done;
    Button *button_help;
    Button *button_cancel;
    Button *button_path_build;

    Button *build_rate_x1;
    Button *build_rate_x2;
    Button *build_rate_x4;

    Image *stats_background;
    Image *turns_background_x1;
    Image *cost_background_x1;
    Image *turns_background_x2;
    Image *cost_background_x2;
    Image *turns_background_x4;
    Image *cost_background_x4;

    static bool button_description_rest_state;

public:
    AbstractBuildMenu(ResourceID resource_id, UnitInfo *unit);
    ~AbstractBuildMenu();

    bool EventHandler(Event *event);

    virtual bool ProcessKey(int32_t key);
    virtual void InitControls();
    virtual int32_t GetTurnsToBuild(ResourceID unit_type);
    virtual void Build() = 0;
    virtual bool Select(UnitTypeSelector *type_selector, bool mode);

    void Draw(ResourceID unit_type);
    bool Run();
};

class MobileBuildMenu : public AbstractBuildMenu {
public:
    MobileBuildMenu(UnitInfo *unit);
    ~MobileBuildMenu();

    void Build();
};

class FactoryBuildMenu : public AbstractBuildMenu {
    SmartObjectArray<ResourceID> build_queue;
    ResourceID building_id;
    UnitTypeSelector *cargo_selector;
    UnitTypeSelector *active_selector;

    Button *button_build_queue_up_arrow;
    Button *button_build_queue_down_arrow;
    Button *button_build_queue_delete;
    Button *button_build_queue_build;
    Button *button_build_queue_repeat;

    void AddSelection();
    void RemoveSelection();

public:
    FactoryBuildMenu(UnitInfo *unit);
    ~FactoryBuildMenu();

    bool ProcessKey(int32_t key);
    void InitControls();
    int32_t GetTurnsToBuild(ResourceID unit_type);
    void Build();
    bool Select(UnitTypeSelector *type_selector, bool mode);
};

const char *const BuildMenu_EventStrings_InvalidSquare[] = {_(1031), _(05fa), _(ff73)};

const char *const BuildMenu_EventStrings_Available1[] = {_(fa62), _(9eb5), _(01fa)};

const char *const BuildMenu_EventStrings_Available2[] = {_(c4e7), _(803c), _(3346)};

bool AbstractBuildMenu::button_description_rest_state = true;

BuildUnitTypeSelector::BuildUnitTypeSelector(Window *window, WindowInfo *window_info,
                                             SmartObjectArray<ResourceID> unit_types, UnitInfo *unit, int32_t key_code,
                                             Button *button_scroll_up, Button *button_scroll_down)
    : UnitTypeSelector(window, window_info, unit_types, unit->team, key_code, button_scroll_up, button_scroll_down),
      unit(unit) {
    struct ImageSimpleHeader *local_image;
    int32_t width;
    int32_t height;

    width = 20;
    height = max_item_count * 32;

    sprite = new (std::nothrow) uint8_t[4 * sizeof(uint16_t) + width * height];
    local_image = reinterpret_cast<struct ImageSimpleHeader *>(sprite);

    local_image->width = width;
    local_image->height = height;

    buf_to_buf(window_info->buffer, width, height, window_info->width, &local_image->transparent_color, width);
}

BuildUnitTypeSelector::~BuildUnitTypeSelector() { delete[] sprite; }

void BuildUnitTypeSelector::Draw() {
    struct ImageSimpleHeader *local_image;
    int32_t consumption_rate;
    Rect bounds;
    int32_t turns;
    ColorIndex color;

    local_image = reinterpret_cast<struct ImageSimpleHeader *>(sprite);

    UnitTypeSelector::Draw();

    buf_to_buf(&local_image->transparent_color, local_image->width, local_image->height, local_image->width,
               &window_info.buffer[115], window_info.width);

    Text_SetFont(GNW_TEXT_FONT_5);

    consumption_rate = Cargo_GetRawConsumptionRate(unit->GetUnitType(), 1);

    for (int32_t i = 0; i < max_item_count && i < unit_types.GetCount(); ++i) {
        turns = UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[unit->team], *unit_types[i + page_min_index])
                    ->GetAttribute(ATTRIB_TURNS);

        if ((unit->flags & STATIONARY) || unit->storage >= turns * consumption_rate) {
            color = 0xA2;

        } else {
            color = COLOR_RED;
        }

        ReportStats_DrawNumber(&window_info.buffer[(i * 32 + 16 - Text_GetHeight() / 2) * window_info.width + 135],
                               turns, 20, window_info.width, color);
    }

    bounds.ulx = window_info.window.ulx + 115;
    bounds.uly = window_info.window.uly;
    bounds.lrx = bounds.ulx + 20;
    bounds.lry = bounds.uly + local_image->height;

    win_draw_rect(window_info.id, &bounds);
}

AbstractBuildMenu::AbstractBuildMenu(ResourceID resource_id, UnitInfo *unit)
    : Window(resource_id, GameManager_GetDialogWindowCenterMode()), unit(unit) {
    event_success = false;
    event_click_cancel = false;
    event_click_path_build = false;
    event_release = false;

    button_up_arrow = nullptr;
    button_down_arrow = nullptr;
    button_description = nullptr;
    button_done = nullptr;
    button_help = nullptr;
    button_cancel = nullptr;
    button_path_build = nullptr;

    build_rate_x1 = nullptr;
    build_rate_x2 = nullptr;
    build_rate_x4 = nullptr;

    stats_background = nullptr;
    turns_background_x1 = nullptr;
    cost_background_x1 = nullptr;
    turns_background_x2 = nullptr;
    cost_background_x2 = nullptr;
    turns_background_x4 = nullptr;
    cost_background_x4 = nullptr;

    selector = nullptr;

    build_rate = unit->GetBuildRate();

    for (const auto buildable_unit : Builder_GetBuildableUnits(unit->GetUnitType())) {
        unit_types.PushBack(&buildable_unit);
    }

    Add(true);

    Cursor_SetCursor(CURSOR_HAND);

    GameManager_DisableMainMenu();

    FillWindowInfo(&window);
}

AbstractBuildMenu::~AbstractBuildMenu() {
    delete selector;
    delete button_up_arrow;
    delete button_down_arrow;
    delete button_description;
    delete button_done;
    delete button_help;
    delete button_cancel;
    delete build_rate_x1;
    delete build_rate_x2;
    delete build_rate_x4;
    delete button_path_build;

    delete stats_background;
    delete turns_background_x1;
    delete cost_background_x1;
    delete turns_background_x2;
    delete cost_background_x2;
    delete turns_background_x4;
    delete cost_background_x4;
}

bool AbstractBuildMenu::EventHandler(Event *event) {
    bool result;

    if (event->GetEventId() == EVENTS_GET_EVENT_ID(UnitSelectEvent)) {
        EventUnitSelect *select = dynamic_cast<EventUnitSelect *>(event);

        result = Select(select->GetSelector(), select->GetValue());
    } else {
        result = false;
    }

    return result;
}

bool AbstractBuildMenu::ProcessKey(int32_t key) {
    bool result;

    result = true;

    if (key > 0 && key < GNW_INPUT_PRESS) {
        event_release = false;
    }

    switch (key) {
        case GNW_KB_KEY_LALT_P: {
            PauseMenu_Menu();
        } break;

        case GNW_KB_KEY_SHIFT_DIVIDE:
        case 1100: {
            HelpMenu_Menu("CONSTRUCTOR_BUILD_SETUP", WINDOW_MAIN_WINDOW);
        } break;

        case GNW_KB_KEY_ESCAPE: {
            event_click_cancel = true;
        } break;

        case GNW_KB_KEY_KP_ENTER: {
            Build();
        } break;

        case 1101: {
            build_rate = 1;
            build_rate_x1->PlaySound();
            Draw(selector->GetLast());
        } break;

        case 1102: {
            build_rate = 2;
            build_rate_x2->PlaySound();
            Draw(selector->GetLast());
        } break;

        case 1103: {
            build_rate = 4;
            build_rate_x4->PlaySound();
            Draw(selector->GetLast());
        } break;

        case 1104: {
            button_description->PlaySound();
            button_description_rest_state = true;
            Draw(selector->GetLast());
        } break;

        case 1105: {
            button_description->PlaySound();
            button_description_rest_state = false;
            Draw(selector->GetLast());
        } break;

        case 1106: {
            event_click_path_build = true;
            Build();
        } break;

        default: {
            if (key < GNW_INPUT_PRESS) {
                result = selector->ProcessKeys(key);

            } else {
                if (event_release == false) {
                    key -= GNW_INPUT_PRESS;

                    switch (key) {
                        case GNW_KB_KEY_KP_ENTER: {
                            button_done->PlaySound();
                        } break;

                        case 1100: {
                            button_help->PlaySound();
                        } break;

                        case GNW_KB_KEY_ESCAPE: {
                            button_cancel->PlaySound();
                        } break;

                        case 1106: {
                            button_path_build->PlaySound();
                        } break;

                        default: {
                            selector->ProcessKeys(key + GNW_INPUT_PRESS);
                        } break;
                    }
                }

                event_release = true;
            }
        } break;
    }

    return result;
}

void AbstractBuildMenu::InitControls() {
    ButtonID button_list[3];

    button_up_arrow->CopyUpDisabled(BLDUP__X);
    button_down_arrow->CopyUpDisabled(BLDDWN_X);

    button_done->SetCaption(_(11a6));
    button_cancel->SetCaption(_(1976));

    build_rate_x1->SetCaption(_(6b67));
    build_rate_x2->SetCaption(_(68dc));
    build_rate_x4->SetCaption(_(8433));

    button_description->SetPValue(1104);
    button_description->SetRValue(1105);
    button_description->SetFlags(0x01);

    button_done->SetRValue(GNW_KB_KEY_RETURN);
    button_help->SetRValue(1100);
    button_cancel->SetRValue(GNW_KB_KEY_ESCAPE);

    button_done->SetPValue(GNW_KB_KEY_RETURN + GNW_INPUT_PRESS);
    button_help->SetPValue(1100 + GNW_INPUT_PRESS);
    button_cancel->SetPValue(GNW_KB_KEY_ESCAPE + GNW_INPUT_PRESS);

    build_rate_x1->SetPValue(1101);
    build_rate_x2->SetPValue(1102);
    build_rate_x4->SetPValue(1103);

    build_rate_x1->SetFlags(0x05);
    build_rate_x2->SetFlags(0x05);
    build_rate_x4->SetFlags(0x05);

    build_rate_x1->SetSfx(MBUTT0);
    build_rate_x2->SetSfx(MBUTT0);
    build_rate_x4->SetSfx(MBUTT0);

    button_description->SetSfx(KCARG0);
    button_done->SetSfx(NDONE0);
    button_help->SetSfx(NHELP0);
    button_cancel->SetSfx(NCANC0);

    if (button_path_build) {
        button_path_build->SetPValue(1106 + GNW_INPUT_PRESS);
        button_path_build->SetRValue(1106);
        button_path_build->SetSfx(NDONE0);
    }

    button_description->RegisterButton(window.id);
    button_done->RegisterButton(window.id);
    button_help->RegisterButton(window.id);
    button_cancel->RegisterButton(window.id);
    build_rate_x1->RegisterButton(window.id);
    build_rate_x2->RegisterButton(window.id);
    build_rate_x4->RegisterButton(window.id);

    if (button_path_build) {
        button_path_build->RegisterButton(window.id);
    }

    button_list[0] = build_rate_x1->GetId();
    button_list[1] = build_rate_x2->GetId();
    button_list[2] = build_rate_x4->GetId();

    win_group_radio_buttons(3, button_list);

    switch (build_rate) {
        case 1: {
            build_rate_x1->SetRestState(true);
        } break;

        case 2: {
            build_rate_x2->SetRestState(true);
        } break;

        case 4: {
            build_rate_x4->SetRestState(true);
        } break;
    }

    button_description->SetRestState(button_description_rest_state);

    stats_background->Copy(&window);
    turns_background_x1->Copy(&window);
    cost_background_x1->Copy(&window);
    turns_background_x2->Copy(&window);
    cost_background_x2->Copy(&window);
    turns_background_x4->Copy(&window);
    cost_background_x4->Copy(&window);

    selector->Draw();

    Draw(selector->GetLast());
}

int32_t AbstractBuildMenu::GetTurnsToBuild(ResourceID unit_type) {
    return BuildMenu_GetTurnsToBuild(unit_type, unit->team);
}

bool AbstractBuildMenu::Select(UnitTypeSelector *type_selector, bool mode) {
    bool result;

    if (selector == type_selector) {
        if (mode) {
            Build();

        } else {
            Draw(selector->GetLast());
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

void AbstractBuildMenu::Draw(ResourceID unit_type) {
    BaseUnit *base_unit;
    WindowInfo local_window;
    int32_t turns;
    int32_t max_build_rate;
    int32_t consumption_rate;
    ColorIndex color;

    if (unit_type == INVALID_ID) {
        return;
    }

    base_unit = &UnitsManager_BaseUnits[unit_type];

    local_window = window;

    local_window.buffer = &window.buffer[11 + window.width * 13];

    if (!WindowManager_LoadBigImage(base_unit->portrait, &local_window, window.width, false, true, -1, -1, false)) {
        buf_fill(local_window.buffer, 300, 240, window.width, COLOR_BLACK);
        flicsmgr_construct(base_unit->flics, &local_window, window.width, 16, 17, false, false);
    }

    stats_background->Write(&window);

    UnitStats_DrawStats(&window.buffer[stats_background->GetULX() + window.width * stats_background->GetULY()],
                        window.width, unit_type, unit->team,
                        *UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[unit->team], unit_type),
                        stats_background->GetWidth(), I_RAW, I_RAWE);

    if (button_description_rest_state) {
        Text_SetFont(GNW_TEXT_FONT_5);

        Text_TextBox(window.buffer, window.width, base_unit->description, 16, 17, 290, 230, GNW_TEXT_OUTLINE | 0xA2,
                     false, false);
    }

    turns = GetTurnsToBuild(unit_type);

    turns_background_x1->Write(&window);
    cost_background_x1->Write(&window);
    turns_background_x2->Write(&window);
    cost_background_x2->Write(&window);
    turns_background_x4->Write(&window);
    cost_background_x4->Write(&window);

    max_build_rate = BuildMenu_GetMaxPossibleBuildRate(unit->GetUnitType(), turns, unit->storage);

    if (build_rate > max_build_rate) {
        build_rate_x2->SetRestState(false);
        build_rate_x4->SetRestState(false);

        build_rate = max_build_rate;

        switch (build_rate) {
            case 1: {
                build_rate_x1->SetRestState(true);
            } break;

            case 2: {
                build_rate_x2->SetRestState(true);
            } break;

            case 4: {
                build_rate_x4->SetRestState(true);
            } break;
        }
    }

    if (build_rate == 1) {
        color = COLOR_GREEN;

    } else {
        color = 0xA2;
    }

    ReportStats_DrawNumber(&window.buffer[turns_background_x1->GetULX() + turns_background_x1->GetWidth() +
                                          window.width * turns_background_x1->GetULY() - 1],
                           turns, turns_background_x1->GetWidth(), window.width, color);

    if (build_rate == 1) {
        color = COLOR_GREEN;

    } else {
        color = 0xA2;
    }

    ReportStats_DrawNumber(&window.buffer[cost_background_x1->GetULX() + cost_background_x1->GetWidth() +
                                          window.width * cost_background_x1->GetULY() - 1],
                           turns * Cargo_GetRawConsumptionRate(unit->GetUnitType(), 1), cost_background_x1->GetWidth(),
                           window.width, color);

    if (unit->GetUnitType() == TRAINHAL) {
        build_rate_x2->Enable(false);
        build_rate_x4->Enable(false);

    } else {
        if (max_build_rate >= 2) {
            build_rate_x2->Enable(true);
            consumption_rate = unit->GetTurnsToBuild(unit_type, 2, &turns);

            if (build_rate == 2) {
                color = COLOR_GREEN;

            } else {
                color = 0xA2;
            }

            ReportStats_DrawNumber(&window.buffer[turns_background_x2->GetULX() + turns_background_x2->GetWidth() +
                                                  window.width * turns_background_x2->GetULY() - 1],
                                   turns, turns_background_x1->GetWidth(), window.width, color);

            if (build_rate == 2) {
                color = COLOR_GREEN;

            } else {
                color = 0xA2;
            }

            ReportStats_DrawNumber(&window.buffer[cost_background_x2->GetULX() + cost_background_x2->GetWidth() +
                                                  window.width * cost_background_x2->GetULY() - 1],
                                   consumption_rate, cost_background_x2->GetWidth(), window.width, color);

        } else {
            build_rate_x2->Enable(false);
        }

        if (max_build_rate >= 4) {
            build_rate_x4->Enable(true);
            consumption_rate = unit->GetTurnsToBuild(unit_type, 4, &turns);

            if (build_rate == 4) {
                color = COLOR_GREEN;

            } else {
                color = 0xA2;
            }

            ReportStats_DrawNumber(&window.buffer[turns_background_x4->GetULX() + turns_background_x4->GetWidth() +
                                                  window.width * turns_background_x4->GetULY() - 1],
                                   turns, turns_background_x1->GetWidth(), window.width, color);

            if (build_rate == 4) {
                color = COLOR_GREEN;

            } else {
                color = 0xA2;
            }

            ReportStats_DrawNumber(&window.buffer[cost_background_x4->GetULX() + cost_background_x4->GetWidth() +
                                                  window.width * cost_background_x4->GetULY() - 1],
                                   consumption_rate, cost_background_x4->GetWidth(), window.width, color);

        } else {
            build_rate_x4->Enable(false);
        }
    }

    win_draw(window.id);
}

bool BuildMenu_Menu(UnitInfo *unit) {
    bool result;

    if (unit->flags & STATIONARY) {
        FactoryBuildMenu build_menu(unit);

        build_menu.InitControls();
        result = build_menu.Run();

    } else {
        MobileBuildMenu build_menu(unit);

        build_menu.InitControls();
        result = build_menu.Run();
    }

    return result;
}

int32_t BuildMenu_GetTurnsToBuild(ResourceID unit_type, uint16_t team) {
    int32_t result;

    if (ini_get_setting(INI_QUICK_BUILD)) {
        result = 1;

    } else {
        CTInfo *team_info;
        int32_t turns_to_build;

        team_info = &UnitsManager_TeamInfo[team];

        turns_to_build = UnitsManager_GetCurrentUnitValues(team_info, unit_type)->GetAttribute(ATTRIB_TURNS);

        if (team_info->team_type == TEAM_TYPE_COMPUTER) {
            switch (ini_get_setting(INI_OPPONENT)) {
                case OPPONENT_TYPE_MASTER: {
                    result = (turns_to_build * 4 + 2) / 5;
                } break;

                case OPPONENT_TYPE_GOD: {
                    result = (turns_to_build * 2 + 1) / 3;
                } break;

                case OPPONENT_TYPE_CLUELESS: {
                    result = (turns_to_build * 5) / 4;
                } break;

                default: {
                    result = turns_to_build;
                } break;
            }

        } else {
            result = turns_to_build;
        }
    }

    return result;
}

int32_t BuildMenu_GetMaxPossibleBuildRate(ResourceID unit_type, int32_t build_time, int32_t storage) {
    constexpr int32_t rate_1x = 1;
    constexpr int32_t rate_2x = 2;
    constexpr int32_t rate_4x = 4;

    int32_t result;

    if (build_time > rate_1x) {
        int32_t build_speed_multiplier;

        if (build_time > rate_2x) {
            build_speed_multiplier = rate_4x;

        } else {
            build_speed_multiplier = rate_2x;
        }

        if (UnitsManager_BaseUnits[unit_type].flags & MOBILE_LAND_UNIT) {
            const int32_t cost_1x = Cargo_GetRawConsumptionRate(unit_type, 1);
            const int32_t cost_2x = Cargo_GetRawConsumptionRate(unit_type, 2);
            const int32_t cost_4x = Cargo_GetRawConsumptionRate(unit_type, 4);
            int32_t turns_1x{build_time / rate_1x};
            int32_t turns_2x{0};
            int32_t turns_4x{0};

            const int32_t const_change_2x = cost_2x - cost_1x * (rate_2x / rate_1x);

            for (int32_t cost{turns_1x * cost_1x + const_change_2x}; cost <= storage;) {
                if (turns_1x >= rate_2x / rate_1x) {
                    ++turns_2x;
                    turns_1x -= rate_2x / rate_1x;
                    cost += const_change_2x;

                } else {
                    break;
                }
            }

            const int32_t const_change_4x = cost_4x - cost_2x * (rate_4x / rate_2x);

            for (int32_t cost{turns_1x * cost_1x + turns_2x * cost_2x + const_change_4x}; cost <= storage;) {
                if (turns_2x >= rate_4x / rate_2x) {
                    ++turns_4x;
                    turns_2x -= rate_4x / rate_2x;
                    cost += const_change_4x;

                } else {
                    if (turns_2x > 0 and turns_1x == 1 and cost - rate_2x - rate_1x + rate_4x <= storage) {
                        ++turns_4x;
                    }

                    break;
                }
            }

            build_speed_multiplier = turns_4x ? rate_4x : turns_2x ? rate_2x : rate_1x;
        }

        result = build_speed_multiplier;

    } else {
        result = rate_1x;
    }

    return result;
}

bool AbstractBuildMenu::Run() {
    event_click_cancel = false;
    event_success = false;
    event_click_path_build = false;

    while (!event_click_cancel) {
        int32_t key = get_input();

        if (GameManager_RequestMenuExit || unit->GetOrder() == ORDER_DISABLE || unit->team != GameManager_PlayerTeam) {
            key = GNW_KB_KEY_ESCAPE;
        }

        if (key > 0) {
            ProcessKey(key);
        }

        GameManager_ProcessState(false);
    }

    GameManager_EnableMainMenu(unit);
    GameManager_ProcessTick(true);

    return event_success;
}

MobileBuildMenu::MobileBuildMenu(UnitInfo *unit) : AbstractBuildMenu(CONBUILD, unit) {
    WindowInfo window_info;

    Text_SetFont(GNW_TEXT_FONT_5);

    button_up_arrow = new (std::nothrow) Button(BLDUP__U, BLDUP__D, 471, 441);
    button_down_arrow = new (std::nothrow) Button(BLDDWN_U, BLDDWN_D, 491, 441);
    button_description = new (std::nothrow) Button(BLDDES_U, BLDDES_D, 292, 264);
    button_done = new (std::nothrow) Button(BLDONE_U, BLDONE_D, 397, 452);
    button_help = new (std::nothrow) Button(BLDHLP_U, BLDHLP_D, 370, 452);
    button_cancel = new (std::nothrow) Button(BLDCAN_U, BLDCAN_D, 307, 452);
    build_rate_x1 = new (std::nothrow) Button(BLDBLD_U, BLDBLD_D, 292, 345);
    build_rate_x2 = new (std::nothrow) Button(BLD2X_U, BLD2X_D, 292, 369);
    build_rate_x4 = new (std::nothrow) Button(BLD4X_U, BLD4X_D, 292, 394);

    if (unit->GetUnitType() == ENGINEER) {
        button_path_build = new (std::nothrow) Button(BLDPTH_U, BLDPTH_D, 347, 427);

        button_path_build->SetCaption(_(62fa));
    }

    window_info = window;

    window_info.window.ulx = 483;
    window_info.window.uly = 60;
    window_info.window.lrx = 598;
    window_info.window.lry = 429;
    window_info.buffer = &window.buffer[window_info.window.ulx + window_info.window.uly * window.width];

    selector = new (std::nothrow)
        BuildUnitTypeSelector(this, &window_info, unit_types, this->unit, 2000, button_up_arrow, button_down_arrow);

    stats_background = new (std::nothrow) Image(11, 293, 250, 174);
    turns_background_x1 = new (std::nothrow) Image(378, 349, 20, 11);
    cost_background_x1 = new (std::nothrow) Image(414, 349, 30, 11);
    turns_background_x2 = new (std::nothrow) Image(378, 374, 20, 11);
    cost_background_x2 = new (std::nothrow) Image(414, 374, 30, 11);
    turns_background_x4 = new (std::nothrow) Image(378, 399, 20, 11);
    cost_background_x4 = new (std::nothrow) Image(414, 399, 30, 11);

    Text_TextBox(window.buffer, window.width, _(3a76), 327, 7, 158, 18, COLOR_GREEN, true);

    Text_SetFont(GNW_TEXT_FONT_5);

    Text_TextBox(&window, _(526e), 209, 264, 80, 17, true, true);
    Text_TextBox(&window, _(53cf), 368, 330, 41, 13, true, true);
    Text_TextBox(&window, _(204a), 409, 330, 41, 13, true, true);
}

MobileBuildMenu::~MobileBuildMenu() {}

void MobileBuildMenu::Build() {
    ResourceID unit_type;
    const auto mission_category = ResourceManager_GetMissionManager()->GetMission()->GetCategory();

    unit_type = selector->GetLast();

    if (Builder_IsBuildable(unit_type)) {
        int16_t grid_x;
        int16_t grid_y;

        event_click_cancel = true;

        grid_x = unit->grid_x;
        grid_y = unit->grid_y;

        if (UnitsManager_IssueBuildOrder(unit, &grid_x, &grid_y, unit_type)) {
            BaseUnit *base_unit;
            int32_t turns_to_build;
            int32_t max_build_rate;

            base_unit = &UnitsManager_BaseUnits[unit_type];

            turns_to_build = GetTurnsToBuild(unit_type);

            max_build_rate = BuildMenu_GetMaxPossibleBuildRate(unit->GetUnitType(), turns_to_build, unit->storage);

            if (build_rate > max_build_rate) {
                build_rate = max_build_rate;
            }

            if (build_rate == 1 &&
                unit->storage < Cargo_GetRawConsumptionRate(unit->GetUnitType(), 1) * turns_to_build) {
                MessageManager_DrawMessage(_(c529), 2, 0);

            } else {
                unit->GetBuildList().PushBack(&unit_type);

                unit->SetBuildRate(build_rate);

                event_success = true;

                unit->path = nullptr;

                if (Remote_IsNetworkGame) {
                    Remote_SendNetPacket_38(unit);
                }

                if ((base_unit->flags & BUILDING) || event_click_path_build) {
                    UnitsManager_SetNewOrderInt(unit, ORDER_BUILD, ORDER_STATE_SELECT_SITE);

                    GameManager_TempTape = UnitsManager_SpawnUnit((base_unit->flags & BUILDING) ? LRGTAPE : SMLTAPE,
                                                                  GameManager_PlayerTeam, grid_x, grid_y, unit);

                    MessageManager_DrawMessage(_(06cb), 0, 0);

                    if (mission_category == MISSION_CATEGORY_TRAINING) {
                        SoundManager_PlayVoice(V_M049, V_F050);
                    }

                } else {
                    SmartString string;
                    Point point;

                    unit->target_grid_x = grid_x;
                    unit->target_grid_y = grid_y;

                    unit->BuildOrder();

                    unit->GetTurnsToBuild(unit_type, build_rate, &turns_to_build);

                    string.Sprintf(250, BuildMenu_EventStrings_Available1[base_unit->gender], base_unit->singular_name,
                                   UnitsManager_TeamInfo[unit->team].unit_counters[unit_type], turns_to_build);

                    if (ResourceManager_GetMissionManager()->GetMission()->IdentifyMission(
                            "24c19a1201ad24cfdc0c4cfc158596a505284992e57b34d65f2ddc2b243ce549")) {
                        string += _(6682);
                    }

                    point.x = unit->grid_x;
                    point.y = unit->grid_y;

                    MessageManager_DrawMessage(string.GetCStr(), 0, unit, point);
                }
            }

        } else {
            SmartString string;

            string.Sprintf(150,
                           BuildMenu_EventStrings_InvalidSquare[UnitsManager_BaseUnits[unit->GetUnitType()].gender],
                           UnitsManager_BaseUnits[unit->GetUnitType()].singular_name);

            MessageManager_DrawMessage(string.GetCStr(), 1, 0);
        }

    } else {
        unit->GetBuildList().Clear();
    }
}

FactoryBuildMenu::FactoryBuildMenu(UnitInfo *unit)
    : AbstractBuildMenu(FACBUILD, unit), build_queue(unit->GetBuildList(), true) {
    WindowInfo window_info;

    if (unit->GetBuildList()->GetCount()) {
        building_id = unit->GetConstructedUnitType();

    } else {
        building_id = INVALID_ID;
    }

    button_up_arrow = new (std::nothrow) Button(BLDUP__U, BLDUP__D, 471, 441);
    button_down_arrow = new (std::nothrow) Button(BLDDWN_U, BLDDWN_D, 491, 441);
    button_description = new (std::nothrow) Button(BLDDES_U, BLDDES_D, 292, 264);
    button_done = new (std::nothrow) Button(BLDONE_U, BLDONE_D, 397, 452);
    button_help = new (std::nothrow) Button(BLDHLP_U, BLDHLP_D, 370, 452);
    button_cancel = new (std::nothrow) Button(BLDCAN_U, BLDCAN_D, 307, 452);
    button_build_queue_up_arrow = new (std::nothrow) Button(BLDUP__U, BLDUP__D, 327, 293);
    button_build_queue_down_arrow = new (std::nothrow) Button(BLDDWN_U, BLDDWN_D, 347, 293);
    build_rate_x1 = new (std::nothrow) Button(BLDBLD_U, BLDBLD_D, 292, 371);
    build_rate_x2 = new (std::nothrow) Button(BLD2X_U, BLD2X_D, 292, 396);
    build_rate_x4 = new (std::nothrow) Button(BLD4X_U, BLD4X_D, 292, 421);
    button_build_queue_delete = new (std::nothrow) Button(BLDDEL_U, BLDDEL_D, 412, 293);
    button_build_queue_repeat = new (std::nothrow) Button(BLDREP_U, BLDREP_D, 447, 322);
    button_build_queue_build = new (std::nothrow) Button(BLDBLD_U, BLDBLD_D, 548, 441);

    Text_SetFont(GNW_TEXT_FONT_5);

    button_build_queue_delete->SetCaption(_(e2d0));
    button_build_queue_build->SetCaption(_(bdd2));

    window_info = window;

    window_info.window.ulx = 483;
    window_info.window.uly = 60;
    window_info.window.lrx = 598;
    window_info.window.lry = 429;

    window_info.buffer = &window.buffer[window_info.window.ulx + window.width * window_info.window.uly];

    selector = new (std::nothrow)
        BuildUnitTypeSelector(this, &window_info, unit_types, unit, 2000, button_up_arrow, button_down_arrow);

    active_selector = selector;

    window_info.window.ulx = 337;
    window_info.window.uly = 60;
    window_info.window.lrx = 453;
    window_info.window.lry = 280;

    window_info.buffer = &window.buffer[window_info.window.ulx + window.width * window_info.window.uly];

    cargo_selector = new (std::nothrow) UnitTypeSelector(this, &window_info, build_queue, unit->team, 3000,
                                                         button_build_queue_up_arrow, button_build_queue_down_arrow);

    stats_background = new (std::nothrow) Image(11, 293, 250, 174);
    turns_background_x1 = new (std::nothrow) Image(378, 376, 20, 11);
    cost_background_x1 = new (std::nothrow) Image(414, 376, 30, 11);
    turns_background_x2 = new (std::nothrow) Image(378, 401, 20, 11);
    cost_background_x2 = new (std::nothrow) Image(414, 401, 30, 11);
    turns_background_x4 = new (std::nothrow) Image(378, 426, 20, 11);
    cost_background_x4 = new (std::nothrow) Image(414, 426, 30, 11);

    Text_SetFont(GNW_TEXT_FONT_5);

    Text_TextBox(window.buffer, window.width, _(faaf), 327, 7, 158, 18, COLOR_GREEN, true);
    Text_TextBox(&window, _(207f), 209, 264, 80, 17, true, true);
    Text_TextBox(&window, _(2180), 368, 357, 41, 13, true, true);
    Text_TextBox(&window, _(064b), 409, 357, 41, 13, true, true);
    Text_TextBox(&window, _(b6b7), 395, 322, 50, 17, true, true);
}

FactoryBuildMenu::~FactoryBuildMenu() {
    delete cargo_selector;
    delete button_build_queue_delete;
    delete button_build_queue_build;
    delete button_build_queue_repeat;
    delete button_build_queue_up_arrow;
    delete button_build_queue_down_arrow;
}

bool FactoryBuildMenu::ProcessKey(int32_t key) {
    bool result;

    result = true;

    if (key > 0 && key < GNW_INPUT_PRESS) {
        event_release = false;
    }

    switch (key) {
        case GNW_KB_KEY_LALT_P: {
            PauseMenu_Menu();
        } break;

        case 1000: {
            AddSelection();
        } break;

        case 1001: {
            RemoveSelection();
        } break;

        case 1002: {
            button_build_queue_repeat->PlaySound();
            unit->SetRepeatBuildState(true);
        } break;

        case 1003: {
            button_build_queue_repeat->PlaySound();
            unit->SetRepeatBuildState(false);
        } break;

        case GNW_KB_KEY_SHIFT_DIVIDE:
        case 1100: {
            HelpMenu_Menu("FACTORY_BUILD_SETUP", WINDOW_MAIN_WINDOW);
        } break;

        default: {
            if (key < GNW_INPUT_PRESS) {
                if (active_selector->ProcessKeys(key) || cargo_selector->ProcessKeys(key)) {
                    result = true;

                } else {
                    result = AbstractBuildMenu::ProcessKey(key);
                }

            } else {
                if (event_release == false) {
                    key -= GNW_INPUT_PRESS;

                    switch (key) {
                        case 1000: {
                            button_build_queue_build->PlaySound();
                        } break;

                        case 1001: {
                            button_build_queue_delete->PlaySound();
                        } break;

                        default: {
                            AbstractBuildMenu::ProcessKey(key + GNW_INPUT_PRESS);
                        } break;
                    }
                }

                event_release = true;
            }
        } break;
    }

    return result;
}

void FactoryBuildMenu::InitControls() {
    button_build_queue_build->SetRValue(1000);
    button_build_queue_build->SetPValue(1000 + GNW_INPUT_PRESS);
    button_build_queue_delete->SetRValue(1001);
    button_build_queue_delete->SetPValue(1001 + GNW_INPUT_PRESS);
    button_build_queue_repeat->SetPValue(1002);
    button_build_queue_repeat->SetRValue(1003);
    button_build_queue_repeat->SetFlags(0x01);
    button_build_queue_build->SetSfx(MBUTT0);
    button_build_queue_delete->SetSfx(MBUTT0);
    button_build_queue_repeat->SetSfx(KCARG0);
    button_build_queue_up_arrow->CopyUpDisabled(BLDUP__X);
    button_build_queue_down_arrow->CopyUpDisabled(BLDDWN_X);
    button_build_queue_up_arrow->SetSfx(KCARG0);
    button_build_queue_down_arrow->SetSfx(KCARG0);
    button_build_queue_delete->RegisterButton(window.id);
    button_build_queue_build->RegisterButton(window.id);
    button_build_queue_repeat->RegisterButton(window.id);
    button_build_queue_repeat->SetRestState(unit->GetRepeatBuildState());

    cargo_selector->Draw();

    AbstractBuildMenu::InitControls();

    if (build_queue->GetCount() > 0) {
        Draw(cargo_selector->GetLast());
    }
}

int32_t FactoryBuildMenu::GetTurnsToBuild(ResourceID unit_type) {
    int32_t result;

    if (unit_type == building_id) {
        result = unit->build_time;

    } else {
        result = BuildMenu_GetTurnsToBuild(unit_type, unit->team);
    }

    return result;
}

void FactoryBuildMenu::Build() {
    SmartObjectArray<ResourceID> build_list = unit->GetBuildList();

    build_list->Clear();

    for (int32_t i = 0; i < build_queue->GetCount(); ++i) {
        build_list.PushBack(build_queue[i]);
    }

    if (build_list->GetCount()) {
        BaseUnit *base_unit;
        ResourceID unit_type;
        SmartString string;

        unit_type = *build_list[0];

        base_unit = &UnitsManager_BaseUnits[unit_type];

        event_click_cancel = true;
        event_success = true;

        unit->build_time = GetTurnsToBuild(unit_type);

        unit->SetBuildRate(build_rate);

        string.Sprintf(250, BuildMenu_EventStrings_Available2[base_unit->gender], base_unit->singular_name,
                       UnitsManager_TeamInfo[unit->team].unit_counters[unit_type],
                       (unit->build_time + build_rate - 1) / build_rate);

        MessageManager_DrawMessage(string.GetCStr(), 1, unit, Point(unit->grid_x, unit->grid_y));

        unit->BuildOrder();

    } else {
        UnitsManager_SetNewOrder(unit, ORDER_HALT_BUILDING, ORDER_STATE_BUILD_CANCEL);

        event_click_cancel = true;
    }
}

bool FactoryBuildMenu::Select(UnitTypeSelector *type_selector, bool mode) {
    bool result;

    active_selector = type_selector;

    if (type_selector == selector) {
        if (mode) {
            AddSelection();

        } else {
            Draw(type_selector->GetLast());
        }

        result = true;

    } else if (type_selector == cargo_selector) {
        if (mode) {
            RemoveSelection();

        } else {
            Draw(type_selector->GetLast());
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

void FactoryBuildMenu::AddSelection() {
    ResourceID unit_type;

    unit_type = selector->GetLast();

    if (Builder_IsBuildable(unit_type)) {
        cargo_selector->Add(unit_type, cargo_selector->GetPageMaxIndex() + 1);
    }
}

void FactoryBuildMenu::RemoveSelection() {
    cargo_selector->RemoveLast();

    if (!build_queue->GetCount()) {
        active_selector = selector;
    }
}
