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

#include "reportmenu.hpp"

#include "game_manager.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "remote.hpp"
#include "reportstats.hpp"
#include "text.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"
#include "window.hpp"
#include "window_manager.hpp"

enum {
    REPORT_TYPE_UNITS,
    REPORT_TYPE_CASUALTIES,
    REPORT_TYPE_SCORE,
    REPORT_TYPE_MESSAGES,
};

static void ReportMenu_OnClick_Units(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_Casualties(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_Score(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_Messages(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_PAirUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_RAirUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_PLandUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_RLandUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_PSeaUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_RSeaUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_PStationaryUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_RStationaryUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_PProductionUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_RProductionUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_PCombatUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_RCombatUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_PDamagedUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_RDamagedUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_PStealthyUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_RStealthyUnits(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_Up(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_Down(ButtonID bid, intptr_t value);
static void ReportMenu_OnClick_Help(ButtonID bid, intptr_t value);

static bool ReportMenu_ButtonState_AirUnits = true;
static bool ReportMenu_ButtonState_LandUnits = true;
static bool ReportMenu_ButtonState_SeaUnits = true;
static bool ReportMenu_ButtonState_StationaryUnits = true;
static bool ReportMenu_ButtonState_ProductionUnits = false;
static bool ReportMenu_ButtonState_CombatUnits = false;
static bool ReportMenu_ButtonState_DamagedUnits = false;
static bool ReportMenu_ButtonState_StealthyUnits = false;

static const char *ReportMenu_TeamNames[PLAYER_TEAM_MAX] = {_(a18e), _(f3cd), _(2b0e), _(7e3b), ""};

static const ColorIndex ReportMenu_TeamColors[PLAYER_TEAM_MAX] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE, 0xA9, 0xFF};

class MessageLine : public SmartObject {
    SmartPointer<MessageLogEntry> message;
    SmartString string;

public:
    MessageLine(MessageLogEntry *message, SmartString string);
    ~MessageLine();

    const char *GetCStr() const;
    MessageLogEntry *GetMessage() const;
};

class ReportMenu : public Window {
    char radio_button_index;

    SmartArray<UnitInfo> units;
    SmartObjectArray<ResourceID> unit_types;
    SmartArray<MessageLine> message_lines;

    SmartPointer<MessageLogEntry> selected_message;

    int32_t row_counts[3];
    int32_t row_indices[3];

    bool active_units[UNIT_END];

    bool exit_loop;

    Button *button_units;
    Button *button_casulties;
    Button *button_score;
    Button *button_messages;
    Button *button_air_units;
    Button *button_land_units;
    Button *button_sea_units;
    Button *button_stationary_units;
    Button *button_production_units;
    Button *button_combat_units;
    Button *button_damaged_units;
    Button *button_stealthy_units;
    Button *button_up;
    Button *button_down;
    Button *button_done;
    Button *button_help;

    Image *report_screen_image;

    friend void ReportMenu_OnClick_Units(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_Casualties(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_Score(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_Messages(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_PAirUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_RAirUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_PLandUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_RLandUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_PSeaUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_RSeaUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_PStationaryUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_RStationaryUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_PProductionUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_RProductionUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_PCombatUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_RCombatUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_PDamagedUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_RDamagedUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_PStealthyUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_RStealthyUnits(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_Up(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_Down(ButtonID bid, intptr_t value);
    friend void ReportMenu_OnClick_Help(ButtonID bid, intptr_t value);

    void DrawUnits();
    void DrawCasualties();
    void DrawScores();
    void DrawMessages();
    void SelectUnit(int32_t index);
    void SelectUnit(Point point);

    static int32_t GetWorkingEcoSphereCount(uint16_t team);
    static void UpdateSelectedUnitStatus(UnitInfo *unit, WindowInfo *window, int32_t ulx, int32_t uly, int32_t width,
                                         int32_t height);

    void AddUnits(SmartList<UnitInfo> *unit_list);
    int32_t GetSelectedUnitIndex();
    int32_t GetMessageIndex(MessageLogEntry *message);
    int32_t GetNextMessageIndex(MessageLogEntry *message);
    void SelectMessage(MessageLogEntry *message);
    void SelectMessage(Point point);
    void MessageUp();
    void MessageDown();

public:
    ReportMenu();
    ~ReportMenu();

    void Run();
    void UpdateStatistics();
    void InitMessages();
    void Draw(bool draw_to_screen);
};

MessageLine::MessageLine(MessageLogEntry *message, SmartString string) : message(message), string(string) {}

MessageLine::~MessageLine() {}

const char *MessageLine::GetCStr() const { return string.GetCStr(); }

MessageLogEntry *MessageLine::GetMessage() const { return &*message; }

void ReportMenu_Menu() { ReportMenu().Run(); }

ReportMenu::ReportMenu() : Window(REP_FRME, GameManager_GetDialogWindowCenterMode()), units(10) {
    Rect bounds;
    WindowInfo window;
    ButtonID button_list[4];

    rect_init(&bounds, 0, 0, 90, 23);

    mouse_hide();

    Add(false);
    FillWindowInfo(&window);

    report_screen_image = new (std::nothrow) Image(20, 17, 459, 448);
    report_screen_image->Copy(&window);

    Text_SetFont(GNW_TEXT_FONT_5);

    row_counts[0] = report_screen_image->GetHeight() / 50;
    row_counts[1] = (report_screen_image->GetHeight() - 20) / 40;
    row_counts[2] = (report_screen_image->GetHeight() - 12) / Text_GetHeight();

    row_indices[0] = 0;
    row_indices[1] = 0;
    row_indices[2] = 0;

    button_units = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 509, 71);
    button_units->SetFlags(0x01);
    button_units->Copy(window.id);
    button_units->SetCaption(_(9fb6), &bounds);
    button_units->SetPFunc(&ReportMenu_OnClick_Units, reinterpret_cast<intptr_t>(this));
    button_units->SetSfx(MBUTT0);
    button_units->RegisterButton(window.id);

    button_casulties = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 509, 100);
    button_casulties->SetFlags(0x01);
    button_casulties->Copy(window.id);
    button_casulties->SetCaption(_(8787), &bounds);
    button_casulties->SetPFunc(&ReportMenu_OnClick_Casualties, reinterpret_cast<intptr_t>(this));
    button_casulties->SetSfx(MBUTT0);
    button_casulties->RegisterButton(window.id);

    button_score = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 509, 129);
    button_score->SetFlags(0x01);
    button_score->Copy(window.id);
    button_score->SetCaption(_(38e5), &bounds);
    button_score->SetPFunc(&ReportMenu_OnClick_Score, reinterpret_cast<intptr_t>(this));
    button_score->SetSfx(MBUTT0);
    button_score->RegisterButton(window.id);

    button_messages = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 509, 158);
    button_messages->SetFlags(0x01);
    button_messages->Copy(window.id);
    button_messages->SetCaption(_(04d7), &bounds);
    button_messages->SetPFunc(&ReportMenu_OnClick_Messages, reinterpret_cast<intptr_t>(this));
    button_messages->SetSfx(MBUTT0);
    button_messages->RegisterButton(window.id);

    button_list[0] = button_units->GetId();
    button_list[1] = button_casulties->GetId();
    button_list[2] = button_score->GetId();
    button_list[3] = button_messages->GetId();

    win_group_radio_buttons(4, button_list);

    Text_TextLine(&window, _(bbb4), 497, 206, 140);

    button_air_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 218);
    button_air_units->SetFlags(0x01);
    button_air_units->Copy(window.id);
    button_air_units->SetPFunc(&ReportMenu_OnClick_PAirUnits, reinterpret_cast<intptr_t>(this));
    button_air_units->SetRFunc(&ReportMenu_OnClick_RAirUnits, reinterpret_cast<intptr_t>(this));
    button_air_units->SetSfx(MBUTT0);
    button_air_units->RegisterButton(window.id);

    Text_TextLine(&window, _(2ecc), 517, 220, 120);

    button_land_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 236);
    button_land_units->SetFlags(0x01);
    button_land_units->Copy(window.id);
    button_land_units->SetPFunc(&ReportMenu_OnClick_PLandUnits, reinterpret_cast<intptr_t>(this));
    button_land_units->SetRFunc(&ReportMenu_OnClick_RLandUnits, reinterpret_cast<intptr_t>(this));
    button_land_units->SetSfx(MBUTT0);
    button_land_units->RegisterButton(window.id);

    Text_TextLine(&window, _(4a3b), 517, 238, 120);

    button_sea_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 254);
    button_sea_units->SetFlags(0x01);
    button_sea_units->Copy(window.id);
    button_sea_units->SetPFunc(&ReportMenu_OnClick_PSeaUnits, reinterpret_cast<intptr_t>(this));
    button_sea_units->SetRFunc(&ReportMenu_OnClick_RSeaUnits, reinterpret_cast<intptr_t>(this));
    button_sea_units->SetSfx(MBUTT0);
    button_sea_units->RegisterButton(window.id);

    Text_TextLine(&window, _(1c43), 517, 256, 120);

    button_stationary_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 272);
    button_stationary_units->SetFlags(0x01);
    button_stationary_units->Copy(window.id);
    button_stationary_units->SetPFunc(&ReportMenu_OnClick_PStationaryUnits, reinterpret_cast<intptr_t>(this));
    button_stationary_units->SetRFunc(&ReportMenu_OnClick_RStationaryUnits, reinterpret_cast<intptr_t>(this));
    button_stationary_units->SetSfx(MBUTT0);
    button_stationary_units->RegisterButton(window.id);

    Text_TextLine(&window, _(4393), 517, 274, 120);

    Text_TextLine(&window, _(28d5), 497, 298, 140);

    button_production_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 312);
    button_production_units->SetFlags(0x01);
    button_production_units->Copy(window.id);
    button_production_units->SetPFunc(&ReportMenu_OnClick_PProductionUnits, reinterpret_cast<intptr_t>(this));
    button_production_units->SetRFunc(&ReportMenu_OnClick_RProductionUnits, reinterpret_cast<intptr_t>(this));
    button_production_units->SetSfx(MBUTT0);
    button_production_units->RegisterButton(window.id);

    Text_TextLine(&window, _(929b), 517, 314, 120);

    button_combat_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 330);
    button_combat_units->SetFlags(0x01);
    button_combat_units->Copy(window.id);
    button_combat_units->SetPFunc(&ReportMenu_OnClick_PCombatUnits, reinterpret_cast<intptr_t>(this));
    button_combat_units->SetRFunc(&ReportMenu_OnClick_RCombatUnits, reinterpret_cast<intptr_t>(this));
    button_combat_units->SetSfx(MBUTT0);
    button_combat_units->RegisterButton(window.id);

    Text_TextLine(&window, _(1f97), 517, 332, 120);

    button_damaged_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 348);
    button_damaged_units->SetFlags(0x01);
    button_damaged_units->Copy(window.id);
    button_damaged_units->SetPFunc(&ReportMenu_OnClick_PDamagedUnits, reinterpret_cast<intptr_t>(this));
    button_damaged_units->SetRFunc(&ReportMenu_OnClick_RDamagedUnits, reinterpret_cast<intptr_t>(this));
    button_damaged_units->SetSfx(MBUTT0);
    button_damaged_units->RegisterButton(window.id);

    Text_TextLine(&window, _(045b), 517, 350, 120);

    button_stealthy_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 366);
    button_stealthy_units->SetFlags(0x01);
    button_stealthy_units->Copy(window.id);
    button_stealthy_units->SetPFunc(&ReportMenu_OnClick_PStealthyUnits, reinterpret_cast<intptr_t>(this));
    button_stealthy_units->SetRFunc(&ReportMenu_OnClick_RStealthyUnits, reinterpret_cast<intptr_t>(this));
    button_stealthy_units->SetSfx(MBUTT0);
    button_stealthy_units->RegisterButton(window.id);

    Text_TextLine(&window, _(e5ae), 517, 368, 120);

    button_up = new (std::nothrow) Button(DPTUP_UP, DPTUP_DN, 487, 426);
    button_up->CopyUpDisabled(DPTUP_X);
    button_up->Copy(window.id);
    button_up->SetRFunc(&ReportMenu_OnClick_Up, reinterpret_cast<intptr_t>(this));
    button_up->SetSfx(MBUTT0);
    button_up->RegisterButton(window.id);

    button_down = new (std::nothrow) Button(DPTDN_UP, DPTDN_DN, 516, 426);
    button_down->CopyUpDisabled(DPTDN_X);
    button_down->Copy(window.id);
    button_down->SetRFunc(&ReportMenu_OnClick_Down, reinterpret_cast<intptr_t>(this));
    button_down->SetSfx(MBUTT0);
    button_down->RegisterButton(window.id);

    button_done = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 509, 398);
    button_done->Copy(window.id);
    button_done->SetCaption(_(64cf), &bounds);
    button_done->SetRValue(GNW_KB_KEY_ESCAPE);
    button_done->SetSfx(MBUTT0);
    button_done->RegisterButton(window.id);

    button_help = new (std::nothrow) Button(DPTHP_UP, DPTHP_DN, 543, 427);
    button_help->SetRFunc(&ReportMenu_OnClick_Help, reinterpret_cast<intptr_t>(this));
    button_help->SetSfx(MBUTT0);
    button_help->RegisterButton(window.id);

    radio_button_index = REPORT_TYPE_UNITS;

    UpdateStatistics();
    InitMessages();
    Draw(true);

    button_units->SetRestState(true);
    button_air_units->SetRestState(ReportMenu_ButtonState_AirUnits);
    button_land_units->SetRestState(ReportMenu_ButtonState_LandUnits);
    button_sea_units->SetRestState(ReportMenu_ButtonState_SeaUnits);
    button_stationary_units->SetRestState(ReportMenu_ButtonState_StationaryUnits);

    button_production_units->SetRestState(ReportMenu_ButtonState_ProductionUnits);
    button_combat_units->SetRestState(ReportMenu_ButtonState_CombatUnits);
    button_damaged_units->SetRestState(ReportMenu_ButtonState_DamagedUnits);
    button_stealthy_units->SetRestState(ReportMenu_ButtonState_StealthyUnits);

    mouse_show();
}

ReportMenu::~ReportMenu() {
    delete report_screen_image;
    delete button_units;
    delete button_casulties;
    delete button_score;
    delete button_messages;
    delete button_air_units;
    delete button_land_units;
    delete button_sea_units;
    delete button_stationary_units;
    delete button_production_units;
    delete button_combat_units;
    delete button_damaged_units;
    delete button_stealthy_units;
    delete button_up;
    delete button_down;
    delete button_done;
    delete button_help;
}

void ReportMenu::Run() {
    SmartPointer<UnitInfo> selected_unit(GameManager_SelectedUnit);
    int32_t key;
    MouseEvent mouse_event;
    int32_t index;

    if (Remote_IsNetworkGame) {
        GameManager_ProcessState(false);

    } else {
        GameManager_DrawTurnTimer(GameManager_TurnTimerValue, true);
    }

    exit_loop = false;

    while (!exit_loop) {
        key = get_input();

        if (MouseEvent::PopFront(mouse_event) && mouse_event.buttons == MOUSE_PRESS_LEFT) {
            mouse_event.point.x -= ulx;
            mouse_event.point.y -= uly;

            if (mouse_event.point.x >= report_screen_image->GetULX() &&
                mouse_event.point.x <= report_screen_image->GetULX() + report_screen_image->GetWidth() &&
                mouse_event.point.y >= report_screen_image->GetULY() &&
                mouse_event.point.y <= report_screen_image->GetULY() + report_screen_image->GetHeight()) {
                switch (radio_button_index) {
                    case REPORT_TYPE_UNITS: {
                        SelectUnit(mouse_event.point);
                    } break;

                    case REPORT_TYPE_MESSAGES: {
                        SelectMessage(mouse_event.point);
                    } break;
                }
            }
        }

        switch (key) {
            case GNW_KB_KEY_SHIFT_DIVIDE: {
                HelpMenu_Menu(HELPMENU_REPORTS_SETUP, WINDOW_MAIN_WINDOW);
            } break;

            case GNW_KB_KEY_UP: {
                if (radio_button_index == REPORT_TYPE_UNITS) {
                    index = GetSelectedUnitIndex();

                    if (index > 0) {
                        SelectUnit(index - 1);
                    }

                } else if (radio_button_index == REPORT_TYPE_MESSAGES) {
                    MessageUp();
                }
            } break;

            case GNW_KB_KEY_DOWN: {
                if (radio_button_index == REPORT_TYPE_UNITS) {
                    index = GetSelectedUnitIndex();

                    if (index >= 0 && index < units.GetCount() - 1) {
                        SelectUnit(index + 1);
                    }

                } else if (radio_button_index == REPORT_TYPE_MESSAGES) {
                    MessageDown();
                }
            } break;

            case GNW_KB_KEY_LALT_P: {
                PauseMenu_Menu();
            } break;

            case GNW_KB_KEY_ESCAPE:
            case GNW_KB_KEY_KP_ENTER: {
                exit_loop = true;
            } break;
        }

        if (Remote_IsNetworkGame) {
            GameManager_ProcessState(false, false);
        }

        if (GameManager_RequestMenuExit) {
            exit_loop = true;
        }
    }

    if (radio_button_index == REPORT_TYPE_MESSAGES && selected_message != nullptr) {
        selected_message->Select();
    }

    if (selected_unit != GameManager_SelectedUnit) {
        GameManager_MenuUnitSelect(&*GameManager_SelectedUnit);

        if (GameManager_SelectedUnit != nullptr) {
            GameManager_UpdateMainMapView(1, GameManager_SelectedUnit->grid_x, GameManager_SelectedUnit->grid_y);
        }
    }

    if (Remote_IsNetworkGame) {
        GameManager_ProcessState(false);

    } else {
        Remote_PauseTimeStamp = timer_get();
    }
}

void ReportMenu::UpdateStatistics() {
    uint32_t unit_flags_mask;
    uint32_t unit_flags;

    unit_flags_mask = 0;

    memset(active_units, true, sizeof(active_units));

    active_units[ROAD] = false;
    active_units[CNCT_4W] = false;
    active_units[BLOCK] = false;
    active_units[WTRPLTFM] = false;
    active_units[BRIDGE] = false;

    if (ReportMenu_ButtonState_ProductionUnits) {
        memset(active_units, false, sizeof(active_units));

        active_units[CONSTRCT] = true;
        active_units[ENGINEER] = true;
        active_units[LIGHTPLT] = true;
        active_units[LANDPLT] = true;
        active_units[AIRPLT] = true;
        active_units[SHIPYARD] = true;
        active_units[TRAINHAL] = true;
    }

    if (ReportMenu_ButtonState_CombatUnits) {
        for (int32_t unit_type = 0; unit_type < UNIT_END; ++unit_type) {
            if (!UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[GameManager_PlayerTeam],
                                                   static_cast<ResourceID>(unit_type))
                     ->GetAttribute(ATTRIB_ATTACK)) {
                active_units[unit_type] = false;
            }
        }
    }

    if (ReportMenu_ButtonState_StealthyUnits) {
        for (int32_t unit_type = 0; unit_type < UNIT_END; ++unit_type) {
            if (unit_type != SUBMARNE && unit_type != COMMANDO) {
                active_units[unit_type] = false;
            }
        }
    }

    if (ReportMenu_ButtonState_AirUnits) {
        unit_flags_mask |= MOBILE_AIR_UNIT;
    }

    if (ReportMenu_ButtonState_LandUnits) {
        unit_flags_mask |= MOBILE_LAND_UNIT;
    }

    if (ReportMenu_ButtonState_SeaUnits) {
        unit_flags_mask |= MOBILE_SEA_UNIT;
    }

    if (ReportMenu_ButtonState_StationaryUnits) {
        unit_flags_mask |= STATIONARY;
    }

    for (int32_t unit_type = 0; unit_type < UNIT_END; ++unit_type) {
        unit_flags = UnitsManager_BaseUnits[unit_type].flags;

        if (!(unit_flags & SELECTABLE) || (unit_flags & GROUND_COVER) || !(unit_flags & unit_flags_mask)) {
            active_units[unit_type] = false;
        }
    }

    units.Release();

    AddUnits(&UnitsManager_MobileLandSeaUnits);
    AddUnits(&UnitsManager_MobileAirUnits);
    AddUnits(&UnitsManager_StationaryUnits);

    row_indices[0] = GetSelectedUnitIndex();

    if (row_indices[0] > units.GetCount() - row_counts[0]) {
        row_indices[0] = units.GetCount() - row_counts[0];
    }

    if (row_indices[0] < 0) {
        row_indices[0] = 0;
    }

    unit_types.Clear();

    {
        int32_t team;
        ResourceID type;

        for (int32_t unit_type = 0; unit_type < UNIT_END; ++unit_type) {
            if (active_units[unit_type]) {
                for (team = PLAYER_TEAM_RED;
                     (team < PLAYER_TEAM_MAX - 1) && !UnitsManager_TeamInfo[team].casualties[unit_type]; ++team) {
                }

                if (team < PLAYER_TEAM_MAX - 1) {
                    type = static_cast<ResourceID>(unit_type);

                    unit_types.PushBack(&type);
                }
            }
        }
    }

    row_indices[1] = 0;
}

void ReportMenu::InitMessages() {
    int32_t row_limit;
    int32_t row_count;
    SmartString *rows;
    bool skip_new_paragraph;

    skip_new_paragraph = true;
    row_limit = (Text_GetHeight() + 31) / Text_GetHeight();

    message_lines.Release();

    Text_SetFont(GNW_TEXT_FONT_5);

    for (SmartList<MessageLogEntry>::Iterator it = MessageManager_TeamMessageLog[GameManager_PlayerTeam].Begin();
         it != MessageManager_TeamMessageLog[GameManager_PlayerTeam].End(); ++it) {
        rows = Text_SplitText((*it).GetCStr(), 100, report_screen_image->GetWidth() - 95, &row_count);

        if (rows) {
            if (!skip_new_paragraph) {
                message_lines.Insert(new (std::nothrow) MessageLine(nullptr, SmartString()));
            }

            skip_new_paragraph = false;

            for (int32_t i = 0; i < (row_limit - row_count) / 2; ++i) {
                message_lines.Insert(new (std::nothrow) MessageLine(&*it, SmartString()));
            }

            for (int32_t i = 0; i < row_count; ++i) {
                message_lines.Insert(new (std::nothrow) MessageLine(&*it, rows[i]));
            }

            for (int32_t i = ((row_limit - row_count) / 2) + row_count; i < row_limit; ++i) {
                message_lines.Insert(new (std::nothrow) MessageLine(&*it, SmartString()));
            }

            delete[] rows;
        }
    }

    row_indices[2] = message_lines.GetCount() - row_counts[2];

    if (row_indices[2] < 0) {
        row_indices[2] = 0;
    }

    selected_message = nullptr;
}

void ReportMenu::DrawUnits() {
    int32_t row_limit;
    int32_t window_ulx;
    int32_t window_uly;
    int32_t item_max;
    WindowInfo window;
    Rect bounds;
    char text[50];

    row_limit = report_screen_image->GetHeight() / row_counts[0];

    window_ulx = report_screen_image->GetULX();

    item_max = std::min(static_cast<int32_t>(units.GetCount()), row_counts[0] + row_indices[0]);

    FillWindowInfo(&window);

    window_uly = report_screen_image->GetULY() + (row_limit - 50) / 2;

    for (int32_t i = row_indices[0]; i < item_max; ++i) {
        ReportStats_DrawListItemIcon(window.buffer, window.width, units[i].GetUnitType(), GameManager_PlayerTeam,
                                     window_ulx + 16, window_uly + 25);

        units[i].GetDisplayName(text, sizeof(text));

        Text_SetFont(GNW_TEXT_FONT_5);

        Text_TextBox(window.buffer, window.width, text, window_ulx + 35, window_uly, 80, 50, 0xA2, false);

        bounds.ulx = window_ulx + 117;
        bounds.uly = window_uly;
        bounds.lrx = window_ulx + 260;
        bounds.lry = window_uly + 50;

        ReportStats_Draw(&units[i], window.id, &bounds);

        Text_SetFont(GNW_TEXT_FONT_5);

        Text_TextBox(window.buffer, window.width,
                     SmartString().Sprintf(10, "%i,%i", units[i].grid_x + 1, units[i].grid_y + 1).GetCStr(),
                     window_ulx + 265, window_uly, 40, 50, 0xA2, true);

        UpdateSelectedUnitStatus(&units[i], &window, window_ulx + 310, window_uly,
                                 report_screen_image->GetWidth() - 310, 50);

        if (GameManager_SelectedUnit == units[i]) {
            GameManager_DrawUnitSelector(window.buffer, window.width, window_ulx, window_uly + 9, window_ulx,
                                         window_uly + 9, window_ulx + 32, window_uly + 41,
                                         (units[i].flags & BUILDING) ? 0x40000 : 0x20000, units[i].flags & BUILDING);
        }

        window_uly += row_limit;
    }

    button_up->Enable(row_indices[0] > 0);
    button_down->Enable(row_indices[0] + row_counts[0] < units.GetCount());
}

void ReportMenu::DrawCasualties() {
    int32_t row_limit;
    int32_t window_ulx;
    int32_t window_uly;
    int32_t item_max;
    WindowInfo window;
    int32_t width;

    row_limit = (report_screen_image->GetHeight() - 20) / row_counts[1];

    window_ulx = report_screen_image->GetULX();

    item_max = std::min(static_cast<int32_t>(unit_types.GetCount()), row_counts[1] + row_indices[1]);

    FillWindowInfo(&window);

    Text_SetFont(GNW_TEXT_FONT_5);

    window_ulx += 145;

    width = (report_screen_image->GetWidth() - 145) / 4;

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            Text_TextBox(window.buffer, window.width, ReportMenu_TeamNames[team], window_ulx,
                         report_screen_image->GetULY(), width, Text_GetHeight() + 1, ReportMenu_TeamColors[team], true);
            window_ulx += width;
        }
    }

    window_uly = ((row_limit - 40) / 2) + report_screen_image->GetULY() + 20;

    for (int32_t i = row_indices[1]; i < item_max; ++i) {
        window_ulx = report_screen_image->GetULX();

        ReportStats_DrawListItem(window.buffer, window.width, *unit_types[i], window_ulx, window_uly, 140, 0xA2);

        window_ulx += 145;

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                ReportStats_DrawNumber(&window.buffer[window_ulx + (width / 2) + 15 + (window_uly + 11) * window.width],
                                       UnitsManager_TeamInfo[team].casualties[*unit_types[i]], (width / 2) + 15,
                                       window.width, ReportMenu_TeamColors[team]);
                window_ulx += width;
            }
        }

        window_uly += row_limit;
    }

    button_up->Enable(row_indices[1] > 0);
    button_down->Enable(row_indices[1] + row_counts[1] < unit_types.GetCount());
}

void ReportMenu::DrawScores() {
    SmartString string;
    int32_t window_ulx;
    int32_t window_uly;
    int32_t item_max;
    WindowInfo window;
    int32_t x1;
    int32_t x2;
    int32_t y1;
    int32_t y2;
    int32_t ecosphere_counts[PLAYER_TEAM_MAX - 1];
    int32_t team_score;
    int32_t max_score;
    const char *unit_name;
    int32_t width;
    int32_t height;
    int32_t y_min;
    int32_t y_max;
    int32_t ulx;
    int32_t team_points;

    max_score = 0;

    x1 = report_screen_image->GetULX() + 38;
    y1 = report_screen_image->GetULY() + 60;
    x2 = report_screen_image->GetULX() + report_screen_image->GetWidth();
    y2 = report_screen_image->GetULY() + report_screen_image->GetHeight() - 22;

    Text_SetFont(GNW_TEXT_FONT_5);

    window_ulx = report_screen_image->GetULX();
    window_uly = report_screen_image->GetULY();

    FillWindowInfo(&window);

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            ecosphere_counts[team] = GetWorkingEcoSphereCount(team);

            team_score = ecosphere_counts[team] * 10 + UnitsManager_TeamInfo[team].team_points;

            if (team_score > max_score) {
                max_score = team_score;
            }

            for (int32_t i = 0; i < 50; ++i) {
                if (UnitsManager_TeamInfo[team].score_graph[i] > max_score) {
                    max_score = UnitsManager_TeamInfo[team].score_graph[i];
                }
            }

            if (ecosphere_counts[team] == 1) {
                unit_name = _(0334);

            } else {
                unit_name = _(3baf);
            }

            string.Sprintf(70, _(13b0), ReportMenu_TeamNames[team], UnitsManager_TeamInfo[team].team_points,
                           ecosphere_counts[team], unit_name);

            Text_Blit(&window.buffer[window_ulx + window.width * window_uly], string.GetCStr(),
                      report_screen_image->GetWidth(), window.width, ReportMenu_TeamColors[team]);

            window_uly += 12;
        }
    }

    if (max_score < 20) {
        max_score = 20;
    }

    max_score = ((max_score + 9) / 10) * 10;

    draw_line(window.buffer, window.width, x1 - 2, y1, x1 - 2, y2 + 2, COLOR_YELLOW);
    draw_line(window.buffer, window.width, x1 - 2, y2 + 2, x2 - 1, y2 + 2, COLOR_YELLOW);

    width = x2 - x1;
    height = y2 - y1;

    for (int32_t i = 0; i <= 10; ++i) {
        ReportStats_DrawNumber(
            &window.buffer[x1 - 4 + (y2 - ((((max_score * i) / 10) * height) / max_score) - 10) * window.width],
            (max_score * i) / 10, 34, window.width, 0x04);
    }

    y_min = GameManager_TurnCounter - 49;
    y_max = GameManager_TurnCounter + 10;

    if (y_min < 1) {
        y_min = 1;
    }

    {
        int32_t index;

        index = (y_min - GameManager_TurnCounter) / 10;
        index = index * 10 + GameManager_TurnCounter;

        window_uly = y2 + 4;

        while (index <= y_max) {
            ReportStats_DrawNumber(&window.buffer[((index - y_min) * width) / 60 + x1 + window_uly * window.width],
                                   index, 34, window.width, 0x04);
            index += 10;
        }
    }

    if (y_min != GameManager_TurnCounter) {
        window_ulx = (((GameManager_TurnCounter - y_min) * width) / 60) + x1;

        draw_line(window.buffer, window.width, window_ulx, y1, window_ulx, y2, 0xD7);
    }

    if (ini_setting_victory_type == VICTORY_TYPE_SCORE) {
        window_uly = ini_setting_victory_limit;

    } else {
        window_uly = 10;

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                window_uly = std::max(static_cast<int32_t>(ecosphere_counts[team] *
                                                               (ini_setting_victory_limit - GameManager_TurnCounter) +
                                                           UnitsManager_TeamInfo[team].team_points),
                                      window_uly);
            }
        }
    }

    if (window_uly <= max_score) {
        window_uly = y2 - (window_uly * height) / max_score;

        draw_line(window.buffer, window.width, x1, window_uly, x2 - 1, window_uly, 0xD7);
    }

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            ulx = x1;

            if (y_min == GameManager_TurnCounter) {
                team_points = UnitsManager_TeamInfo[team].team_points;

            } else {
                team_points = UnitsManager_TeamInfo[team].score_graph[y_min + 49 - GameManager_TurnCounter];
            }

            team_points = y2 - (height * team_points) / max_score;

            for (int32_t i = y_min + 1; i < GameManager_TurnCounter; ++i) {
                window_ulx = ulx;
                window_uly = team_points;

                ulx = ((i - y_min) * width) / 60 + x1;

                team_points = UnitsManager_TeamInfo[team].score_graph[(i + 49) - GameManager_TurnCounter];

                team_points = y2 - (height * team_points) / max_score;

                draw_line(window.buffer, window.width, window_ulx + team, window_uly + team, ulx + team,
                          team_points + team, ReportMenu_TeamColors[team]);
            }

            if (y_min < GameManager_TurnCounter) {
                window_ulx = ulx;
                window_uly = team_points;

                ulx = ((GameManager_TurnCounter - y_min) * width) / 60 + x1;

                team_points = UnitsManager_TeamInfo[team].team_points;

                team_points = y2 - (height * team_points) / max_score;

                draw_line(window.buffer, window.width, window_ulx + team, window_uly + team, ulx + team,
                          team_points + team, ReportMenu_TeamColors[team]);
            }

            window_ulx = ulx;
            window_uly = team_points;

            ulx = ((y_max - y_min) * width) / 60 + x1;

            team_points = ecosphere_counts[team] * 10 + UnitsManager_TeamInfo[team].team_points;

            team_points = y2 - (height * team_points) / max_score;

            draw_line(window.buffer, window.width, window_ulx + team, window_uly + team, ulx + team, team_points + team,
                      ReportMenu_TeamColors[team]);
        }
    }

    button_up->Enable(false);
    button_down->Enable(false);
}

void ReportMenu::DrawMessages() {
    int32_t window_ulx;
    int32_t window_uly;
    int32_t item_max;
    WindowInfo window;
    MessageLogEntry *message1;
    MessageLogEntry *message2;
    ColorIndex color;

    Text_SetFont(GNW_TEXT_FONT_5);

    window_ulx = report_screen_image->GetULX() + 5;
    window_uly = report_screen_image->GetULY() + 6;

    item_max = std::min<int32_t>(message_lines.GetCount(), row_counts[2] + row_indices[2]);

    FillWindowInfo(&window);

    if (row_indices[2] > 0) {
        message1 = message_lines[row_indices[2] - 1].GetMessage();

    } else {
        message1 = nullptr;
    }

    for (int32_t i = row_indices[2]; i < item_max; ++i) {
        message2 = message_lines[i].GetMessage();

        if (selected_message == message2) {
            color = COLOR_GREEN;

        } else {
            color = 0xA2;
        }

        Text_Blit(&window.buffer[window_ulx + window_uly * window.width + 85], message_lines[i].GetCStr(),
                  report_screen_image->GetWidth() - 95, window.width, color);

        if (message2 && message2 != message1 &&
            (report_screen_image->GetULY() + report_screen_image->GetHeight() - window_uly) >= 38) {
            if (message2->GetUnit()) {
                Point position = message2->GetPosition();
                UnitInfo *unit = message2->GetUnit();
                SmartString string;

                string.Sprintf(10, "%i,%i", position.x + 1, position.y + 1);

                Text_TextBox(window.buffer, window.width, string.GetCStr(), window_ulx + 40, window_uly, 40, 32,
                             ReportMenu_TeamColors[unit->team], true);

                ReportStats_DrawListItemIcon(window.buffer, window.width, unit->GetUnitType(), unit->team,
                                             window_ulx + 16, window_uly + 16);

            } else if (message2->GetIcon() != INVALID_ID) {
                struct ImageSimpleHeader *sprite =
                    reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_LoadResource(message2->GetIcon()));

                WindowManager_DecodeSimpleImage(sprite, window_ulx, window_uly + 16 - (sprite->height / 2), true,
                                                &window);
            }
        }

        message1 = message2;

        window_uly += Text_GetHeight();
    }

    if (selected_message != nullptr) {
        int32_t start_index;
        int32_t end_index;
        int32_t image_uly;
        int32_t image_lry;
        int32_t x1;
        int32_t x2;
        int32_t y1;
        int32_t y2;

        start_index = GetMessageIndex(&*selected_message) - row_indices[2];
        end_index = GetNextMessageIndex(&*selected_message) - row_indices[2];

        image_uly = report_screen_image->GetULY();
        image_lry = report_screen_image->GetULY() + report_screen_image->GetHeight();

        x1 = report_screen_image->GetULX();
        y1 = Text_GetHeight() * start_index + image_uly + 3;
        x2 = report_screen_image->GetWidth() + x1 - 1;
        y2 = Text_GetHeight() * end_index + image_uly + 8;

        if (image_uly <= y1 && image_lry > y1) {
            draw_line(window.buffer, window.width, x1, y1, x2, y1, 0xA2);
        }

        if (image_uly <= y2 && image_lry > y1) {
            draw_line(window.buffer, window.width, x1, y2, x2, y2, 0xA2);

            draw_line(window.buffer, window.width, x1, std::max(y1, image_uly), x1, std::min(y2, image_lry), 0xA2);
            draw_line(window.buffer, window.width, x2, std::max(y1, image_uly), x2, std::min(y2, image_lry), 0xA2);
        }
    }

    button_up->Enable(row_indices[2] > 0);
    button_down->Enable(row_indices[2] + row_counts[2] < message_lines.GetCount());
}

int32_t ReportMenu::GetWorkingEcoSphereCount(uint16_t team) {
    int32_t result;

    result = 0;

    if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == team && (*it).GetUnitType() == GREENHSE && (*it).GetOrder() == ORDER_POWER_ON) {
                ++result;
            }
        }
    }

    return result;
}

void ReportMenu::UpdateSelectedUnitStatus(UnitInfo *unit, WindowInfo *window, int32_t ulx, int32_t uly, int32_t width,
                                          int32_t height) {
    SmartString string;

    Text_SetFont(GNW_TEXT_FONT_5);

    width -= 40;

    if (unit->GetOrder() == ORDER_BUILD || unit->GetOrder() == ORDER_CLEAR || unit->GetOrder() == ORDER_HALT_BUILDING ||
        unit->GetOrder() == ORDER_HALT_BUILDING_2) {
        if (unit->GetUnitType() == BULLDOZR) {
            string.Sprintf(80, _(138d), unit->build_time);

        } else if (unit->GetOrderState() == ORDER_STATE_UNIT_READY) {
            string = GameManager_GetUnitStatusMessage(unit);

        } else {
            SmartObjectArray<ResourceID> build_list = unit->GetBuildList();

            SDL_assert(build_list.GetCount() > 0);

            if (unit->GetOrder() == ORDER_HALT_BUILDING || unit->GetOrder() == ORDER_HALT_BUILDING_2) {
                string.Sprintf(200, _(abea), UnitsManager_BaseUnits[*build_list[0]].singular_name, unit->build_time);

            } else {
                int32_t turns_to_build;

                unit->GetTurnsToBuild(*build_list[0], unit->GetBuildRate(), &turns_to_build);

                string.Sprintf(200, _(4262), UnitsManager_BaseUnits[*build_list[0]].singular_name, turns_to_build);

                ReportStats_DrawListItemIcon(window->buffer, window->width, *build_list[0], GameManager_PlayerTeam,
                                             ulx + width + 20, (height / 2) + uly);
            }
        }

    } else {
        string = GameManager_GetUnitStatusMessage(unit);
    }

    Text_TextBox(window->buffer, window->width, string.GetCStr(), ulx, uly, width, height, 0xA2, false);
}

void ReportMenu::SelectUnit(int32_t index) {
    if (index >= 0 && index < units.GetCount()) {
        if (GameManager_SelectedUnit == units[index]) {
            exit_loop = true;

        } else {
            GameManager_SelectedUnit = units[index];
        }

        if (index < row_indices[0]) {
            row_indices[0] = index;
        }

        if (index >= row_counts[0] + row_indices[0]) {
            row_indices[0] = index - row_counts[0] + 1;
        }

        Draw(true);
    }
}

void ReportMenu::SelectUnit(Point point) {
    int32_t index;
    int32_t image_uly;
    int32_t offset;

    offset = report_screen_image->GetHeight() / row_counts[0];

    image_uly = ((offset - 50) / 2) + report_screen_image->GetULY();

    if (point.y >= image_uly) {
        index = (point.y - image_uly) / offset;

        if (index < row_counts[0]) {
            index += row_indices[0];

            if (units.GetCount() > index) {
                SelectUnit(index);
            }
        }
    }
}

void ReportMenu::AddUnits(SmartList<UnitInfo> *unit_list) {
    int32_t index;

    for (SmartList<UnitInfo>::Iterator it = unit_list->Begin(); it != unit_list->End(); ++it) {
        if ((*it).team == GameManager_PlayerTeam && active_units[(*it).GetUnitType()] &&
            ((*it).GetOrder() != ORDER_IDLE || (*it).GetOrderState() != ORDER_STATE_BUILDING_READY) &&
            (!ReportMenu_ButtonState_DamagedUnits || (*it).hits < (*it).GetBaseValues()->GetAttribute(ATTRIB_HITS))) {
            for (index = 0; index < units.GetCount(); ++index) {
                if (units[index].GetUnitType() > (*it).GetUnitType()) {
                    break;
                }

                if (units[index].GetUnitType() == (*it).GetUnitType() && units[index].unit_id > (*it).unit_id) {
                    break;
                }
            }

            units.Insert(&*it, index);
        }
    }
}

int32_t ReportMenu::GetSelectedUnitIndex() {
    for (int32_t i = units.GetCount() - 1; i >= 0; --i) {
        if (GameManager_SelectedUnit == units[i]) {
            return i;
        }
    }

    return -1;
}

int32_t ReportMenu::GetMessageIndex(MessageLogEntry *message) {
    int32_t index;

    for (index = 0; index < message_lines.GetCount(); ++index) {
        if (message_lines[index].GetMessage() == message) {
            break;
        }
    }

    return index;
}

int32_t ReportMenu::GetNextMessageIndex(MessageLogEntry *message) {
    int32_t index;

    for (index = GetMessageIndex(message); index < message_lines.GetCount(); ++index) {
        if (message_lines[index].GetMessage() != message) {
            break;
        }
    }

    return index;
}

void ReportMenu::SelectMessage(MessageLogEntry *message) {
    int32_t message_index;
    int32_t index;

    if (selected_message == message) {
        exit_loop = true;
    }

    selected_message = message;

    message_index = GetMessageIndex(message);

    for (index = message_index; index < message_lines.GetCount(); ++index) {
        if (message_lines[index].GetMessage() != message) {
            break;
        }
    }

    if (index >= row_counts[2] + row_indices[2]) {
        row_indices[2] = index - row_counts[2];
    }

    if (message_index < row_indices[2]) {
        row_indices[2] = message_index;
    }

    if (row_counts[2] + row_indices[2] > message_lines.GetCount()) {
        row_indices[2] = message_lines.GetCount() - row_counts[2];
    }

    if (row_indices[2] < 0) {
        row_indices[2] = 0;
    }

    Draw(true);
}

void ReportMenu::SelectMessage(Point point) {
    Text_SetFont(GNW_TEXT_FONT_5);

    int32_t index = ((point.y - report_screen_image->GetULY() - 6) / Text_GetHeight()) + row_indices[2];

    if (message_lines.GetCount() > index) {
        if (message_lines[index].GetMessage()) {
            SelectMessage(message_lines[index].GetMessage());
        }
    }
}

void ReportMenu::MessageUp() {
    if (message_lines.GetCount()) {
        int32_t index;

        if (selected_message != nullptr) {
            index = GetMessageIndex(&*selected_message);

        } else {
            index = message_lines.GetCount() + 1;
        }

        if (index >= 2) {
            SelectMessage(message_lines[index - 2].GetMessage());
        }
    }
}

void ReportMenu::MessageDown() {
    if (message_lines.GetCount()) {
        int32_t index;

        if (selected_message != nullptr) {
            index = GetNextMessageIndex(&*selected_message);

        } else {
            index = message_lines.GetCount() - 2;
        }

        if (message_lines.GetCount() > index) {
            SelectMessage(message_lines[index + 1].GetMessage());
        }
    }
}

void ReportMenu::Draw(bool draw_to_screen) {
    WindowInfo window;

    FillWindowInfo(&window);

    report_screen_image->Write(&window);

    switch (radio_button_index) {
        case REPORT_TYPE_UNITS: {
            DrawUnits();
        } break;

        case REPORT_TYPE_CASUALTIES: {
            DrawCasualties();
        } break;

        case REPORT_TYPE_SCORE: {
            DrawScores();
        } break;

        case REPORT_TYPE_MESSAGES: {
            DrawMessages();
        } break;
    }

    if (draw_to_screen) {
        win_draw(window.id);
    }
}

void ReportMenu_OnClick_Units(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    if (menu->radio_button_index != REPORT_TYPE_UNITS) {
        menu->radio_button_index = REPORT_TYPE_UNITS;

        menu->Draw(true);
    }
}

void ReportMenu_OnClick_Casualties(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    if (menu->radio_button_index != REPORT_TYPE_CASUALTIES) {
        menu->radio_button_index = REPORT_TYPE_CASUALTIES;

        menu->Draw(true);
    }
}

void ReportMenu_OnClick_Score(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    if (menu->radio_button_index != REPORT_TYPE_SCORE) {
        menu->radio_button_index = REPORT_TYPE_SCORE;

        menu->Draw(true);
    }
}

void ReportMenu_OnClick_Messages(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    if (menu->radio_button_index != REPORT_TYPE_MESSAGES) {
        menu->radio_button_index = REPORT_TYPE_MESSAGES;

        menu->Draw(true);
    }
}

void ReportMenu_OnClick_PAirUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_AirUnits = true;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_RAirUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_AirUnits = false;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_PLandUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_LandUnits = true;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_RLandUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_LandUnits = false;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_PSeaUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_SeaUnits = true;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_RSeaUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_SeaUnits = false;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_PStationaryUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_StationaryUnits = true;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_RStationaryUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_StationaryUnits = false;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_PProductionUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_ProductionUnits = true;

    menu->button_combat_units->SetRestState(false);
    ReportMenu_ButtonState_CombatUnits = false;

    menu->button_stealthy_units->SetRestState(false);
    ReportMenu_ButtonState_StealthyUnits = false;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_RProductionUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_ProductionUnits = false;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_PCombatUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_CombatUnits = true;

    menu->button_production_units->SetRestState(false);
    ReportMenu_ButtonState_ProductionUnits = false;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_RCombatUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_CombatUnits = false;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_PDamagedUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_DamagedUnits = true;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_RDamagedUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_DamagedUnits = false;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_PStealthyUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_StealthyUnits = true;

    menu->button_production_units->SetRestState(false);
    ReportMenu_ButtonState_ProductionUnits = false;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_RStealthyUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_StealthyUnits = false;

    menu->UpdateStatistics();

    if (menu->radio_button_index == REPORT_TYPE_UNITS || menu->radio_button_index == REPORT_TYPE_CASUALTIES) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_Up(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    switch (menu->radio_button_index) {
        case REPORT_TYPE_UNITS: {
            menu->row_indices[0] -= menu->row_counts[0];

            if (menu->row_indices[0] < 0) {
                menu->row_indices[0] = 0;
            }

            menu->Draw(true);
        } break;

        case REPORT_TYPE_CASUALTIES: {
            menu->row_indices[1] -= menu->row_counts[1];

            if (menu->row_indices[1] < 0) {
                menu->row_indices[1] = 0;
            }

            menu->Draw(true);
        } break;

        case REPORT_TYPE_SCORE: {
        } break;

        case REPORT_TYPE_MESSAGES: {
            menu->row_indices[2] -= menu->row_counts[2];

            if (menu->row_indices[2] < 0) {
                menu->row_indices[2] = 0;
            }

            menu->Draw(true);
        } break;
    }
}

void ReportMenu_OnClick_Down(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    switch (menu->radio_button_index) {
        case REPORT_TYPE_UNITS: {
            menu->row_indices[0] += menu->row_counts[0];

            if (menu->row_indices[0] > menu->units.GetCount() - menu->row_counts[0]) {
                menu->row_indices[0] = menu->units.GetCount() - menu->row_counts[0];
            }

            if (menu->row_indices[0] < 0) {
                menu->row_indices[0] = 0;
            }

            menu->Draw(true);
        } break;

        case REPORT_TYPE_CASUALTIES: {
            menu->row_indices[1] += menu->row_counts[1];

            if (menu->row_indices[1] > menu->unit_types.GetCount() - menu->row_counts[1]) {
                menu->row_indices[1] = menu->unit_types.GetCount() - menu->row_counts[1];
            }

            if (menu->row_indices[1] < 0) {
                menu->row_indices[1] = 0;
            }

            menu->Draw(true);
        } break;

        case REPORT_TYPE_SCORE: {
        } break;

        case REPORT_TYPE_MESSAGES: {
            menu->row_indices[2] += menu->row_counts[2];

            if (menu->row_indices[2] > menu->message_lines.GetCount() - menu->row_counts[2]) {
                menu->row_indices[2] = menu->message_lines.GetCount() - menu->row_counts[2];
            }

            if (menu->row_indices[2] < 0) {
                menu->row_indices[2] = 0;
            }

            menu->Draw(true);
        } break;
    }
}

void ReportMenu_OnClick_Help(ButtonID bid, intptr_t value) { HelpMenu_Menu(HELPMENU_REPORTS_SETUP, WINDOW_MAIN_MAP); }
