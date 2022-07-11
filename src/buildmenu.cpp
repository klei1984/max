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
#include "menu.hpp"
#include "reportstats.hpp"
#include "resource_manager.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "unitstats.hpp"
#include "unittypeselector.hpp"
#include "window_manager.hpp"

class BuildUnitTypeSelector : public UnitTypeSelector {
    unsigned char *sprite;
    UnitInfo *unit;

public:
    BuildUnitTypeSelector(Window *window, WindowInfo *window_info, SmartObjectArray<ResourceID> unit_types,
                          UnitInfo *unit, int key_code, Button *button_scroll_up, Button *button_scroll_down);
    ~BuildUnitTypeSelector();

    void Draw();
};

class AbstractBuildMenu : public Window {
    WindowInfo window;
    UnitInfo *unit;

    bool event_success;
    bool event_click_cancel;
    bool event_click_path_build;
    bool event_release;

    unsigned short build_rate;

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

    virtual bool ProcessKey(int key);
    virtual void InitControls();
    virtual int GetTurnsToBuild(ResourceID unit_type);
    virtual void Build() = 0;
    virtual bool vfunc6(UnitTypeSelector *type_selector, bool mode);

    void Draw(ResourceID unit_type);
    bool Run();
};

bool AbstractBuildMenu::button_description_rest_state = true;

BuildUnitTypeSelector::BuildUnitTypeSelector(Window *window, WindowInfo *window_info,
                                             SmartObjectArray<ResourceID> unit_types, UnitInfo *unit, int key_code,
                                             Button *button_scroll_up, Button *button_scroll_down)
    : UnitTypeSelector(window, window_info, unit_types, unit->team, key_code, button_scroll_up, button_scroll_down),
      unit(unit) {
    struct ImageSimpleHeader *image;
    int width;
    int height;

    width = 20;
    height = max_item_count * 32;

    sprite = new (std::nothrow) unsigned char[4 * sizeof(unsigned short) + width * height];
    image = reinterpret_cast<struct ImageSimpleHeader *>(sprite);

    image->width = width;
    image->height = width;

    buf_to_buf(window_info->buffer, width, height, window_info->width, image->data, width);
}

BuildUnitTypeSelector::~BuildUnitTypeSelector() { delete[] sprite; }

void BuildUnitTypeSelector::Draw() {
    struct ImageSimpleHeader *image;
    int consumption_rate;
    Rect bounds;
    int turns;
    ColorIndex color;

    image = reinterpret_cast<struct ImageSimpleHeader *>(sprite);

    UnitTypeSelector::Draw();

    buf_to_buf(image->data, image->width, image->height, image->width, &window_info.buffer[115], window_info.width);

    text_font(5);

    consumption_rate = Cargo_GetRawConsumptionRate(unit->unit_type, 1);

    for (int i = 0; i < max_item_count && i < unit_types.GetCount(); ++i) {
        turns = UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[unit->team], *unit_types[i + page_min_index])
                    ->GetAttribute(ATTRIB_TURNS);

        if ((unit->flags & STATIONARY) || unit->storage >= turns * consumption_rate) {
            color = 0xA2;

        } else {
            color = 0x01;
        }

        ReportStats_DrawNumber(&window_info.buffer[(i * 32 + 16 - text_height() / 2) * window_info.width + 135], turns,
                               20, window_info.width, color);
    }

    bounds.ulx = window_info.window.ulx + 115;
    bounds.uly = window_info.window.uly;
    bounds.lrx = bounds.ulx + 20;
    bounds.lry = bounds.uly + image->height;

    win_draw_rect(window_info.id, &bounds);
}

AbstractBuildMenu::AbstractBuildMenu(ResourceID resource_id, UnitInfo *unit) : Window(resource_id), unit(unit) {
    ResourceID builder_unit;
    ResourceID buildable_unit;
    int list_size;
    int index;

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

    for (index = 0; builder_unit != unit->unit_type;) {
        builder_unit = static_cast<ResourceID>(Builder_CapabilityListNormal[index++]);
        list_size = Builder_CapabilityListNormal[index++];
    }

    for (int j = 0; j < list_size; ++j) {
        buildable_unit = static_cast<ResourceID>(Builder_CapabilityListNormal[index++]);

        if (Builder_IsBuildable(buildable_unit)) {
            unit_types.PushBack(&buildable_unit);
        }
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

        result = vfunc6(select->GetSelector(), select->GetValue());
    } else {
        result = false;
    }

    return result;
}

bool AbstractBuildMenu::ProcessKey(int key) {
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
            HelpMenu_Menu(HELPMENU_CONSTRUCTOR_BUILD_SETUP, WINDOW_MAIN_WINDOW);
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

    button_done->SetCaption("Done");
    button_cancel->SetCaption("Cancel");

    build_rate_x1->SetCaption("Build X1");
    build_rate_x2->SetCaption("Build X2");
    build_rate_x4->SetCaption("Build X4");

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

    Draw(selector->GetLast());
}

int AbstractBuildMenu::GetTurnsToBuild(ResourceID unit_type) {
    return UnitsManager_GetTurnsToBuild(unit_type, unit->team);
}

bool AbstractBuildMenu::vfunc6(UnitTypeSelector *type_selector, bool mode) {
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
    int turns;
    int max_build_rate;
    int consumption_rate;
    ColorIndex color;

    if (unit_type == INVALID_ID) {
        return;
    }

    base_unit = &UnitsManager_BaseUnits[unit_type];

    local_window = window;

    local_window.buffer = &window.buffer[11 + window.width * 13];

    if (!WindowManager_LoadImage(base_unit->portrait, &local_window, window.width, false)) {
        buf_fill(local_window.buffer, 300, 240, window.width, 0x00);
        flicsmgr_construct(base_unit->flics, &local_window, window.width, 16, 17, false, false);
    }

    stats_background->Write(&local_window);

    UnitStats_DrawStats(&window.buffer[stats_background->GetULX() + window.width * stats_background->GetULY()],
                        window.width, unit_type, unit->team,
                        *UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[unit->team], unit_type),
                        stats_background->GetWidth(), I_RAW, I_RAWE);

    if (button_description_rest_state) {
        text_font(5);

        Text_TextBox(window.buffer, window.width, base_unit->description, 16, 17, 290, 230, 0x100A2, false, false);
    }

    turns = GetTurnsToBuild(unit_type);

    turns_background_x1->Write(&window);
    cost_background_x1->Write(&window);
    turns_background_x2->Write(&window);
    cost_background_x2->Write(&window);
    turns_background_x4->Write(&window);
    cost_background_x4->Write(&window);

    max_build_rate = BuildMenu_GetMaxPossibleBuildRate(unit->unit_type, turns, unit->storage);

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
        color = 0x2;

    } else {
        color = 0xA2;
    }

    ReportStats_DrawNumber(&window.buffer[turns_background_x1->GetULX() + turns_background_x1->GetWidth() +
                                          window.width * turns_background_x1->GetULY() - 1],
                           turns, turns_background_x1->GetWidth(), window.width, color);

    if (build_rate == 1) {
        color = 0x2;

    } else {
        color = 0xA2;
    }

    ReportStats_DrawNumber(&window.buffer[cost_background_x1->GetULX() + cost_background_x1->GetWidth() +
                                          window.width * cost_background_x1->GetULY() - 1],
                           turns * Cargo_GetRawConsumptionRate(unit->unit_type, 1), cost_background_x1->GetWidth(),
                           window.width, color);

    if (unit->unit_type == TRAINHAL) {
        build_rate_x2->Enable(false);
        build_rate_x4->Enable(false);

    } else {
        if (max_build_rate >= 2) {
            build_rate_x2->Enable(true);
            consumption_rate = unit->GetTurnsToBuild(unit_type, 2, &turns);

            if (build_rate == 2) {
                color = 0x2;

            } else {
                color = 0xA2;
            }

            ReportStats_DrawNumber(&window.buffer[turns_background_x2->GetULX() + turns_background_x2->GetWidth() +
                                                  window.width * turns_background_x2->GetULY() - 1],
                                   turns, turns_background_x1->GetWidth(), window.width, color);

            if (build_rate == 2) {
                color = 0x2;

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
                color = 0x2;

            } else {
                color = 0xA2;
            }

            ReportStats_DrawNumber(&window.buffer[turns_background_x4->GetULX() + turns_background_x4->GetWidth() +
                                                  window.width * turns_background_x4->GetULY() - 1],
                                   turns, turns_background_x1->GetWidth(), window.width, color);

            if (build_rate == 4) {
                color = 0x2;

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
    /// \todo
}

int BuildMenu_GetMaxPossibleBuildRate(ResourceID unit_type, int build_time, int storage) {
    /// \todo
}

bool AbstractBuildMenu::Run() {
    event_click_cancel = false;
    event_success = false;
    event_click_path_build = false;

    while (!event_click_cancel) {
        int key = get_input();

        if (GameManager_RequestMenuExit || unit->orders == ORDER_DISABLED || unit->team != GameManager_PlayerTeam) {
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
