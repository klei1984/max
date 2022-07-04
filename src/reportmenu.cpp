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

#include "helpmenu.hpp"
#include "message_manager.hpp"
#include "text.hpp"
#include "unitinfo.hpp"
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

class MessageLine : public SmartObject {
    SmartPointer<MessageLogEntry> message;
    SmartString string;

public:
    MessageLine(MessageLogEntry *entry, SmartString msg);
    ~MessageLine();

    const char *GetCStr() const;
};

class ReportMenu : public Window {
    char radio_button_index;

    SmartArray<UnitInfo> units;
    SmartObjectArray<ResourceID> unit_types;
    SmartArray<MessageLine> message_lines;

    SmartPointer<MessageLogEntry> messages;

    int row_counts[3];
    int row_indices[3];

    char field_64[94];

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

public:
    ReportMenu();
    ~ReportMenu();

    void Run();
    void UpdateStatistics();
    void InitMessages();
    void Draw(bool draw_to_screen);
};

MessageLine::MessageLine(MessageLogEntry *entry, SmartString msg) {}

MessageLine::~MessageLine() {}

const char *MessageLine::GetCStr() const { return string.GetCStr(); }

void ReportMenu_Menu() { ReportMenu().Run(); }

ReportMenu::ReportMenu() : Window(REP_FRME), units(10) {
    Rect bounds;
    WindowInfo window;
    ButtonID button_list[4];

    rect_init(&bounds, 0, 0, 90, 23);

    mouse_hide();

    Add(false);
    FillWindowInfo(&window);

    report_screen_image = new (std::nothrow) Image(20, 17, 459, 448);
    report_screen_image->Copy(&window);

    text_font(5);

    row_counts[0] = report_screen_image->GetHeight() / 50;
    row_counts[1] = (report_screen_image->GetHeight() - 20) / 40;
    row_counts[2] = (report_screen_image->GetHeight() - 12) / text_height();

    row_indices[0] = 0;
    row_indices[1] = 0;
    row_indices[2] = 0;

    button_units = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 509, 71);
    button_units->SetFlags(0x01);
    button_units->Copy(window.id);
    button_units->SetCaption("Units", &bounds);
    button_units->SetPFunc(&ReportMenu_OnClick_Units, reinterpret_cast<intptr_t>(this));
    button_units->SetSfx(MBUTT0);
    button_units->RegisterButton(window.id);

    button_casulties = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 509, 100);
    button_casulties->SetFlags(0x01);
    button_casulties->Copy(window.id);
    button_casulties->SetCaption("Casualties", &bounds);
    button_casulties->SetPFunc(&ReportMenu_OnClick_Casualties, reinterpret_cast<intptr_t>(this));
    button_casulties->SetSfx(MBUTT0);
    button_casulties->RegisterButton(window.id);

    button_score = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 509, 129);
    button_score->SetFlags(0x01);
    button_score->Copy(window.id);
    button_score->SetCaption("Score", &bounds);
    button_score->SetPFunc(&ReportMenu_OnClick_Score, reinterpret_cast<intptr_t>(this));
    button_score->SetSfx(MBUTT0);
    button_score->RegisterButton(window.id);

    button_messages = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 509, 158);
    button_messages->SetFlags(0x01);
    button_messages->Copy(window.id);
    button_messages->SetCaption("Messages", &bounds);
    button_messages->SetPFunc(&ReportMenu_OnClick_Messages, reinterpret_cast<intptr_t>(this));
    button_messages->SetSfx(MBUTT0);
    button_messages->RegisterButton(window.id);

    button_list[0] = button_units->GetId();
    button_list[1] = button_casulties->GetId();
    button_list[2] = button_score->GetId();
    button_list[3] = button_messages->GetId();

    win_group_radio_buttons(4, button_list);

    Text_TextLine(&window, "Include:", 497, 206, 140);

    button_air_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 218);
    button_air_units->SetFlags(0x01);
    button_air_units->Copy(window.id);
    button_air_units->SetPFunc(&ReportMenu_OnClick_PAirUnits, reinterpret_cast<intptr_t>(this));
    button_air_units->SetRFunc(&ReportMenu_OnClick_RAirUnits, reinterpret_cast<intptr_t>(this));
    button_air_units->SetSfx(MBUTT0);
    button_air_units->RegisterButton(window.id);

    Text_TextLine(&window, "Air Units", 517, 220, 120);

    button_land_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 236);
    button_land_units->SetFlags(0x01);
    button_land_units->Copy(window.id);
    button_land_units->SetPFunc(&ReportMenu_OnClick_PLandUnits, reinterpret_cast<intptr_t>(this));
    button_land_units->SetRFunc(&ReportMenu_OnClick_RLandUnits, reinterpret_cast<intptr_t>(this));
    button_land_units->SetSfx(MBUTT0);
    button_land_units->RegisterButton(window.id);

    Text_TextLine(&window, "Land Units", 517, 238, 120);

    button_sea_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 254);
    button_sea_units->SetFlags(0x01);
    button_sea_units->Copy(window.id);
    button_sea_units->SetPFunc(&ReportMenu_OnClick_PSeaUnits, reinterpret_cast<intptr_t>(this));
    button_sea_units->SetRFunc(&ReportMenu_OnClick_RSeaUnits, reinterpret_cast<intptr_t>(this));
    button_sea_units->SetSfx(MBUTT0);
    button_sea_units->RegisterButton(window.id);

    Text_TextLine(&window, "Sea Units", 517, 256, 120);

    button_stationary_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 272);
    button_stationary_units->SetFlags(0x01);
    button_stationary_units->Copy(window.id);
    button_stationary_units->SetPFunc(&ReportMenu_OnClick_PStationaryUnits, reinterpret_cast<intptr_t>(this));
    button_stationary_units->SetRFunc(&ReportMenu_OnClick_RStationaryUnits, reinterpret_cast<intptr_t>(this));
    button_stationary_units->SetSfx(MBUTT0);
    button_stationary_units->RegisterButton(window.id);

    Text_TextLine(&window, "Stationary Units", 517, 274, 120);

    Text_TextLine(&window, "Limit To:", 497, 298, 140);

    button_production_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 312);
    button_production_units->SetFlags(0x01);
    button_production_units->Copy(window.id);
    button_production_units->SetPFunc(&ReportMenu_OnClick_PProductionUnits, reinterpret_cast<intptr_t>(this));
    button_production_units->SetRFunc(&ReportMenu_OnClick_RProductionUnits, reinterpret_cast<intptr_t>(this));
    button_production_units->SetSfx(MBUTT0);
    button_production_units->RegisterButton(window.id);

    Text_TextLine(&window, "Production Units", 517, 314, 120);

    button_combat_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 330);
    button_combat_units->SetFlags(0x01);
    button_combat_units->Copy(window.id);
    button_combat_units->SetPFunc(&ReportMenu_OnClick_PCombatUnits, reinterpret_cast<intptr_t>(this));
    button_combat_units->SetRFunc(&ReportMenu_OnClick_RCombatUnits, reinterpret_cast<intptr_t>(this));
    button_combat_units->SetSfx(MBUTT0);
    button_combat_units->RegisterButton(window.id);

    Text_TextLine(&window, "Combat Units", 517, 332, 120);

    button_damaged_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 348);
    button_damaged_units->SetFlags(0x01);
    button_damaged_units->Copy(window.id);
    button_damaged_units->SetPFunc(&ReportMenu_OnClick_PDamagedUnits, reinterpret_cast<intptr_t>(this));
    button_damaged_units->SetRFunc(&ReportMenu_OnClick_RDamagedUnits, reinterpret_cast<intptr_t>(this));
    button_damaged_units->SetSfx(MBUTT0);
    button_damaged_units->RegisterButton(window.id);

    Text_TextLine(&window, "Damaged Units", 517, 350, 120);

    button_damaged_units = new (std::nothrow) Button(UNCHKED, CHECKED, 496, 366);
    button_damaged_units->SetFlags(0x01);
    button_damaged_units->Copy(window.id);
    button_damaged_units->SetPFunc(&ReportMenu_OnClick_PStealthyUnits, reinterpret_cast<intptr_t>(this));
    button_damaged_units->SetRFunc(&ReportMenu_OnClick_RStealthyUnits, reinterpret_cast<intptr_t>(this));
    button_damaged_units->SetSfx(MBUTT0);
    button_damaged_units->RegisterButton(window.id);

    Text_TextLine(&window, "Stealthy Units", 517, 368, 120);

    button_up = new (std::nothrow) Button(DPTUP_UP, DPTUP_DN, 487, 426);
    button_up->CopyDownDisabled(DPTUP_X);
    button_up->Copy(window.id);
    button_up->SetRFunc(&ReportMenu_OnClick_Up, reinterpret_cast<intptr_t>(this));
    button_up->SetSfx(MBUTT0);
    button_up->RegisterButton(window.id);

    button_down = new (std::nothrow) Button(DPTDN_UP, DPTDN_DN, 516, 426);
    button_down->CopyDownDisabled(DPTDN_X);
    button_down->Copy(window.id);
    button_down->SetRFunc(&ReportMenu_OnClick_Down, reinterpret_cast<intptr_t>(this));
    button_down->SetSfx(MBUTT0);
    button_down->RegisterButton(window.id);

    button_done = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 509, 398);
    button_done->Copy(window.id);
    button_done->SetCaption("Done", &bounds);
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

    /// \todo Is this a defect? We just rendered the entire screen above
    win_draw(window.id);

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

void ReportMenu::Run() {}

void ReportMenu::UpdateStatistics() {}

void ReportMenu::InitMessages() {}

void ReportMenu::Draw(bool draw_to_screen) {}

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

    if (menu->radio_button_index == REPORT_TYPE_UNITS) {
        menu->Draw(true);
    }
}

void ReportMenu_OnClick_RDamagedUnits(ButtonID bid, intptr_t value) {
    ReportMenu *menu;

    menu = reinterpret_cast<ReportMenu *>(value);

    ReportMenu_ButtonState_DamagedUnits = false;

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

void ReportMenu_OnClick_Down(ButtonID bid, intptr_t value) {}

void ReportMenu_OnClick_Help(ButtonID bid, intptr_t value) { HelpMenu_Menu(HELPMENU_REPORTS_SETUP, WINDOW_MAIN_MAP); }
