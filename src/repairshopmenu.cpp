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

#include "repairshopmenu.hpp"

#include "access.hpp"
#include "cargo.hpp"
#include "cursor.hpp"
#include "flicsmgr.hpp"
#include "game_manager.hpp"
#include "helpmenu.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "remote.hpp"
#include "reportstats.hpp"
#include "scrollbar.hpp"
#include "sound_manager.hpp"
#include "text.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"
#include "window.hpp"
#include "window_manager.hpp"

#define REPAIRSHOP_SLOT_COUNT 6

static void RepairShopSlot_OnClick_Activate(ButtonID bid, intptr_t value);
static void RepairShopSlot_OnClick_Reload(ButtonID bid, intptr_t value);
static void RepairShopSlot_OnClick_Repair(ButtonID bid, intptr_t value);
static void RepairShopSlot_OnClick_Upgrade(ButtonID bid, intptr_t value);

static void RepairShopMenu_OnClick_ActivateAll(ButtonID bid, intptr_t value);
static void RepairShopMenu_OnClick_ReloadAll(ButtonID bid, intptr_t value);
static void RepairShopMenu_OnClick_RepairAll(ButtonID bid, intptr_t value);
static void RepairShopMenu_OnClick_UpgradeAll(ButtonID bid, intptr_t value);
static void RepairShopMenu_OnClick_Done(ButtonID bid, intptr_t value);
static void RepairShopMenu_OnClick_UpArrow(ButtonID bid, intptr_t value);
static void RepairShopMenu_OnClick_DownArrow(ButtonID bid, intptr_t value);
static void RepairShopMenu_OnClick_Help(ButtonID bid, intptr_t value);

static void RepairShopMenu_ProcessOrders(UnitInfo *unit, uint8_t order, bool process_tick);

class RepairShopMenu;

class RepairShopSlot {
    RepairShopMenu *menu;
    SmartPointer<UnitInfo> client;

    Image *bg_image_area;
    Image *bg_stats_area;

    Button *button_activate;
    Button *button_reload;
    Button *button_repair;
    Button *button_upgrade;

    friend void RepairShopSlot_OnClick_Activate(ButtonID bid, intptr_t value);
    friend void RepairShopSlot_OnClick_Reload(ButtonID bid, intptr_t value);
    friend void RepairShopSlot_OnClick_Repair(ButtonID bid, intptr_t value);
    friend void RepairShopSlot_OnClick_Upgrade(ButtonID bid, intptr_t value);

public:
    RepairShopSlot(RepairShopMenu *repairshop, Rect *unit_bg_image_bounds, Rect *unit_stats_bounds, bool is_shop);
    ~RepairShopSlot();

    void Init();
    void Draw(UnitInfo *unit, bool draw_to_screen);
    void DrawStats(bool draw_to_screen);
    void UpdateButtons();
};

class RepairShopMenu : public Window {
    SmartList<UnitInfo> units;
    SmartPointer<UnitInfo> repairshop;

    int32_t unit_slot_index;
    int32_t unit_slots_per_screen;
    int32_t raw_material_in_complex;

    RepairShopSlot *repair_slots[REPAIRSHOP_SLOT_COUNT];

    Image *raw_material_bar;
    Image *raw_material_value_area;

    Button *button_activate_all;
    Button *button_reload_all;
    Button *button_repair_all;
    Button *button_upgrade_all;
    Button *button_done;
    Button *button_up_arrow;
    Button *button_down_arrow;
    Button *button_help;

    bool exit_loop;

    friend void RepairShopMenu_OnClick_ActivateAll(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_ReloadAll(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_RepairAll(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_UpgradeAll(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_Done(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_UpArrow(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_DownArrow(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_Help(ButtonID bid, intptr_t value);

public:
    RepairShopMenu(UnitInfo *unit);
    ~RepairShopMenu();

    void Run();
    bool IsReloadViable(UnitInfo *target_unit);
    bool IsRepairViable(UnitInfo *target_unit);
    bool IsUpgradeViable(UnitInfo *target_unit);
    bool IsActivateViable(UnitInfo *target_unit);

    void Activate(UnitInfo *target_unit);
    void Reload(UnitInfo *target_unit);
    void Repair(UnitInfo *target_unit);
    void Upgrade(UnitInfo *target_unit);

    void Draw(bool draw_to_screen);
    void UpdateButtons();
    void DrawCargoBar(bool draw_to_screen);
    void Deinit();
};

RepairShopSlot::RepairShopSlot(RepairShopMenu *repairshop, Rect *unit_bg_image_bounds, Rect *unit_stats_bounds,
                               bool is_shop)
    : menu(repairshop) {
    WindowInfo window;
    Rect unit_slot_button_area;

    menu->FillWindowInfo(&window);

    bg_image_area =
        new (std::nothrow) Image(unit_bg_image_bounds->ulx, unit_bg_image_bounds->uly,
                                 rect_get_width(unit_bg_image_bounds), rect_get_height(unit_bg_image_bounds));
    bg_image_area->Copy(&window);

    bg_stats_area = new (std::nothrow) Image(unit_stats_bounds->ulx, unit_stats_bounds->uly,
                                             rect_get_width(unit_stats_bounds), rect_get_height(unit_stats_bounds));
    bg_stats_area->Copy(&window);

    rect_init(&unit_slot_button_area, 0, 0, 70, 21);

    Text_SetFont(GNW_TEXT_FONT_5);

    button_activate =
        new (std::nothrow) Button(DPTBAYUP, DPTBAYDN, unit_stats_bounds->ulx - 12, unit_stats_bounds->uly + 42);
    button_activate->CopyUpDisabled(DPTBAYDN);
    button_activate->Copy(window.id);
    button_activate->SetCaption(_(2443), &unit_slot_button_area);
    button_activate->SetRFunc(&RepairShopSlot_OnClick_Activate, reinterpret_cast<intptr_t>(this));
    button_activate->SetSfx(MBUTT0);

    if (is_shop) {
        button_reload =
            new (std::nothrow) Button(DPTBAYUP, DPTBAYDN, unit_stats_bounds->ulx - 12, unit_stats_bounds->uly + 67);
        button_reload->CopyUpDisabled(DPTBAYDN);
        button_reload->Copy(window.id);
        button_reload->SetCaption(_(bdc0), &unit_slot_button_area);
        button_reload->SetRFunc(&RepairShopSlot_OnClick_Reload, reinterpret_cast<intptr_t>(this));
        button_reload->SetSfx(MBUTT0);

        button_repair =
            new (std::nothrow) Button(DPTBAYUP, DPTBAYDN, unit_stats_bounds->ulx + 63, unit_stats_bounds->uly + 42);
        button_repair->CopyUpDisabled(DPTBAYDN);
        button_repair->Copy(window.id);
        button_repair->SetCaption(_(1196), &unit_slot_button_area);
        button_repair->SetRFunc(&RepairShopSlot_OnClick_Repair, reinterpret_cast<intptr_t>(this));
        button_repair->SetSfx(MBUTT0);

        button_upgrade =
            new (std::nothrow) Button(DPTBAYUP, DPTBAYDN, unit_stats_bounds->ulx + 63, unit_stats_bounds->uly + 67);
        button_upgrade->CopyUpDisabled(DPTBAYDN);
        button_upgrade->Copy(window.id);
        button_upgrade->SetCaption(_(7ec0), &unit_slot_button_area);
        button_upgrade->SetRFunc(&RepairShopSlot_OnClick_Upgrade, reinterpret_cast<intptr_t>(this));
        button_upgrade->SetSfx(MBUTT0);

    } else {
        button_reload = nullptr;
        button_repair = nullptr;
        button_upgrade = nullptr;
    }
}

RepairShopSlot::~RepairShopSlot() {
    delete bg_image_area;
    delete bg_stats_area;
    delete button_activate;
    delete button_reload;
    delete button_repair;
    delete button_upgrade;
}

void RepairShopSlot::Init() {
    WindowInfo window;

    menu->FillWindowInfo(&window);

    button_activate->RegisterButton(window.id);

    if (button_reload) {
        button_reload->RegisterButton(window.id);
        button_repair->RegisterButton(window.id);
        button_upgrade->RegisterButton(window.id);
    }
}

void RepairShopSlot::Draw(UnitInfo *unit, bool draw_to_screen) {
    WindowInfo window;

    this->client = unit;

    menu->FillWindowInfo(&window);

    if (this->client != nullptr) {
        WindowInfo local_window;
        BaseUnit *base_unit;
        char text[200];

        local_window = window;
        base_unit = &UnitsManager_BaseUnits[unit->unit_type];

        local_window.buffer =
            &local_window.buffer[bg_image_area->GetULY() * local_window.width + bg_image_area->GetULX()];

        if (base_unit->armory_portrait != INVALID_ID) {
            WindowManager_LoadBigImage(base_unit->armory_portrait, &local_window, local_window.width, false,
                                       draw_to_screen);

        } else {
            bg_image_area->Write(&window);
            flicsmgr_construct(base_unit->flics, &window, local_window.width, bg_image_area->GetULX(),
                               bg_image_area->GetULY(), false, false);
        }

        unit->GetDisplayName(text, sizeof(text));

        Text_SetFont(GNW_TEXT_FONT_5);

        Text_TextBox(local_window.buffer, local_window.width, text, 10, 3, bg_image_area->GetWidth() - 20,
                     bg_image_area->GetHeight() - 6, GNW_TEXT_OUTLINE | 0xA2, false, false);

        DrawStats(draw_to_screen);

    } else {
        Rect bounds;

        bg_image_area->Write(&window);
        bg_stats_area->Write(&window);

        rect_init(&bounds, bg_stats_area->GetULX(), bg_stats_area->GetULY(),
                  bg_stats_area->GetULX() + bg_stats_area->GetWidth(),
                  bg_stats_area->GetULY() + bg_stats_area->GetHeight());

        if (draw_to_screen) {
            win_draw_rect(window.id, &bounds);
        }
    }

    if (draw_to_screen) {
        Rect bounds;

        rect_init(&bounds, bg_image_area->GetULX(), bg_image_area->GetULY(),
                  bg_image_area->GetULX() + bg_image_area->GetWidth(),
                  bg_image_area->GetULY() + bg_image_area->GetHeight());

        win_draw_rect(window.id, &bounds);
    }

    UpdateButtons();
}

void RepairShopSlot::DrawStats(bool draw_to_screen) {
    WindowInfo window;
    Rect bounds;

    menu->FillWindowInfo(&window);

    bg_stats_area->Write(&window);

    bounds.ulx = bg_stats_area->GetULX();
    bounds.uly = bg_stats_area->GetULY();
    bounds.lrx = bounds.ulx + bg_stats_area->GetWidth();
    bounds.lry = bounds.uly + bg_stats_area->GetHeight() / 2;

    ReportStats_DrawRow(_(f0dc), window.id, &bounds, SI_HITSB, EI_HITSB, client->hits,
                        client->GetBaseValues()->GetAttribute(ATTRIB_HITS), 4, true);

    bounds.uly = bounds.lry;
    bounds.lry = bg_stats_area->GetULY() + bg_stats_area->GetHeight();

    if (client->GetBaseValues()->GetAttribute(ATTRIB_AMMO)) {
        ReportStats_DrawRow(_(c163), window.id, &bounds, SI_AMMO, EI_AMMO, client->ammo,
                            client->GetBaseValues()->GetAttribute(ATTRIB_AMMO), 1, false);

    } else if (GameManager_PlayerTeam == client->team && client->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
        ReportStats_DrawRow(_(8688), window.id, &bounds,
                            ReportStats_CargoIcons[UnitsManager_BaseUnits[client->unit_type].cargo_type * 2],
                            ReportStats_CargoIcons[UnitsManager_BaseUnits[client->unit_type].cargo_type * 2 + 1],
                            client->storage, client->GetBaseValues()->GetAttribute(ATTRIB_STORAGE), 10, false);
    }

    if (draw_to_screen) {
        bounds.ulx = bg_stats_area->GetULX();
        bounds.uly = bg_stats_area->GetULY();
        bounds.lrx = bounds.ulx + bg_stats_area->GetWidth();
        bounds.lry = bounds.uly + bg_stats_area->GetHeight();

        win_draw_rect(window.id, &bounds);
    }
}

void RepairShopSlot::UpdateButtons() {
    if (client != nullptr && menu->IsActivateViable(&*client)) {
        button_activate->Enable();

    } else {
        button_activate->Disable();
    }

    if (button_repair) {
        if (client != nullptr && menu->IsReloadViable(&*client)) {
            button_reload->Enable();

        } else {
            button_reload->Disable();
        }

        if (client != nullptr && menu->IsRepairViable(&*client)) {
            button_repair->Enable();

        } else {
            button_repair->Disable();
        }

        if (client != nullptr && menu->IsUpgradeViable(&*client)) {
            button_upgrade->Enable();

        } else {
            button_upgrade->Disable();
        }
    }
}

RepairShopMenu::RepairShopMenu(UnitInfo *unit)
    : Window(unit->unit_type == HANGAR ? HANGRFRM : DEPOTFRM, GameManager_GetDialogWindowCenterMode()),
      repairshop(unit) {
    WindowInfo window;
    int32_t slot_window_ulx;
    int32_t slot_window_uly;

    exit_loop = false;

    Cursor_SetCursor(CURSOR_HAND);
    mouse_hide();
    Add(false);
    FillWindowInfo(&window);

    if (unit->unit_type == HANGAR) {
        Rect unit_bg_image_bounds;
        Rect unit_stats_bounds;
        int32_t index;

        unit_slots_per_screen = REPAIRSHOP_SLOT_COUNT - 2;

        for (index = 0; index < unit_slots_per_screen; ++index) {
            slot_window_ulx = (index & 1) * 226 + 18;
            slot_window_uly = (index / 2) * 235 + 9;

            repair_slots[index] = new (std::nothrow)
                RepairShopSlot(this,
                               rect_init(&unit_bg_image_bounds, slot_window_ulx, slot_window_uly, slot_window_ulx + 200,
                                         slot_window_uly + 128),
                               rect_init(&unit_stats_bounds, slot_window_ulx + 38, slot_window_uly + 140,
                                         slot_window_ulx + 160, slot_window_uly + 170),
                               true);
        }

        for (; index < REPAIRSHOP_SLOT_COUNT; ++index) {
            repair_slots[index] = nullptr;
        }

    } else {
        Rect unit_bg_image_bounds;
        Rect unit_stats_bounds;
        int32_t index;

        unit_slots_per_screen = REPAIRSHOP_SLOT_COUNT;

        for (index = 0; index < unit_slots_per_screen; ++index) {
            slot_window_ulx = (index % 3) * 155 + 17;
            slot_window_uly = (index / 3) * 235 + 9;

            if (unit->unit_type == DOCK) {
                WindowManager_LoadBigImage(E_DOCK, &window, window.width, false, false, slot_window_ulx,
                                           slot_window_uly);
            }

            repair_slots[index] = new (std::nothrow)
                RepairShopSlot(this,
                               rect_init(&unit_bg_image_bounds, slot_window_ulx, slot_window_uly, slot_window_ulx + 128,
                                         slot_window_uly + 128),
                               rect_init(&unit_stats_bounds, slot_window_ulx + 3, slot_window_uly + 140,
                                         slot_window_ulx + 125, slot_window_uly + 170),
                               unit->flags & STATIONARY);
        }

        for (; index < REPAIRSHOP_SLOT_COUNT; ++index) {
            repair_slots[index] = nullptr;
        }
    }

    {
        Rect bounds;

        rect_init(&bounds, 0, 0, 90, 23);

        unit_slot_index = 0;

        Text_SetFont(GNW_TEXT_FONT_5);

        if (unit->unit_type == AIRTRANS) {
            button_activate_all = nullptr;

        } else {
            button_activate_all = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 511, 252);
            button_activate_all->CopyUpDisabled(DPTMNUDN);
            button_activate_all->Copy(window.id);
            button_activate_all->SetCaption(_(0fbe), &bounds);
            button_activate_all->SetRFunc(&RepairShopMenu_OnClick_ActivateAll, reinterpret_cast<intptr_t>(this));
            button_activate_all->SetSfx(MBUTT0);
        }

        button_done = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 511, 372);
        button_done->Copy(window.id);
        button_done->SetCaption(_(27af), &bounds);
        button_done->SetRFunc(&RepairShopMenu_OnClick_Done, reinterpret_cast<intptr_t>(this));
        button_done->SetSfx(NDONE0);

        button_up_arrow = new (std::nothrow) Button(DPTUP_UP, DPTUP_DN, 504, 426);
        button_up_arrow->CopyUpDisabled(DPTUP_X);
        button_up_arrow->Copy(window.id);
        button_up_arrow->SetRFunc(&RepairShopMenu_OnClick_UpArrow, reinterpret_cast<intptr_t>(this));
        button_up_arrow->SetSfx(MBUTT0);

        button_down_arrow = new (std::nothrow) Button(DPTDN_UP, DPTDN_DN, 530, 426);
        button_down_arrow->CopyUpDisabled(DPTDN_X);
        button_down_arrow->Copy(window.id);
        button_down_arrow->SetRFunc(&RepairShopMenu_OnClick_DownArrow, reinterpret_cast<intptr_t>(this));
        button_down_arrow->SetSfx(MBUTT0);

        button_help = new (std::nothrow) Button(DPTHP_UP, DPTHP_DN, 584, 426);
        button_help->SetRFunc(&RepairShopMenu_OnClick_Help, reinterpret_cast<intptr_t>(this));
        button_help->SetSfx(MBUTT0);

        if (unit->unit_type == DEPOT || unit->unit_type == HANGAR || unit->unit_type == DOCK ||
            unit->unit_type == BARRACKS) {
            Cargo materials;
            Cargo capacity;

            button_reload_all = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 511, 277);
            button_reload_all->CopyUpDisabled(DPTMNUDN);
            button_reload_all->Copy(window.id);
            button_reload_all->SetCaption(_(e609), &bounds);
            button_reload_all->SetRFunc(&RepairShopMenu_OnClick_ReloadAll, reinterpret_cast<intptr_t>(this));
            button_reload_all->SetSfx(MBUTT0);

            button_repair_all = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 511, 302);
            button_repair_all->CopyUpDisabled(DPTMNUDN);
            button_repair_all->Copy(window.id);
            button_repair_all->SetCaption(_(670f), &bounds);
            button_repair_all->SetRFunc(&RepairShopMenu_OnClick_RepairAll, reinterpret_cast<intptr_t>(this));
            button_repair_all->SetSfx(MBUTT0);

            button_upgrade_all = new (std::nothrow) Button(DPTMNUUP, DPTMNUDN, 511, 327);
            button_upgrade_all->CopyUpDisabled(DPTMNUDN);
            button_upgrade_all->Copy(window.id);
            button_upgrade_all->SetCaption(_(003c), &bounds);
            button_upgrade_all->SetRFunc(&RepairShopMenu_OnClick_UpgradeAll, reinterpret_cast<intptr_t>(this));
            button_upgrade_all->SetSfx(MBUTT0);

            raw_material_bar = new (std::nothrow) Image(546, 106, 20, 115);
            raw_material_bar->Copy(&window);

            raw_material_value_area = new (std::nothrow) Image(529, 76, 54, 12);
            raw_material_value_area->Copy(&window);

            Text_SetFont(GNW_TEXT_FONT_5);

            Text_TextBox(window.buffer, window.width, _(c46e), 520, 53, 72, 20, 0xA2, true);

            unit->GetComplex()->GetCargoInfo(materials, capacity);

            raw_material_in_complex = materials.raw;

            DrawCargoBar(false);

        } else {
            button_reload_all = nullptr;
            button_repair_all = nullptr;
            button_upgrade_all = nullptr;
            raw_material_bar = nullptr;
            raw_material_value_area = nullptr;

            raw_material_in_complex = 0;
        }
    }

    if (unit->unit_type == HANGAR) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
             Access_IsChildOfUnitInList(unit, &UnitsManager_MobileAirUnits, &it); ++it) {
            units.PushBack(*it);
        }

    } else {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
             Access_IsChildOfUnitInList(unit, &UnitsManager_MobileLandSeaUnits, &it); ++it) {
            units.PushBack(*it);
        }
    }

    if (button_activate_all) {
        button_activate_all->RegisterButton(GetId());
    }

    if (button_reload_all) {
        button_reload_all->RegisterButton(GetId());
        button_repair_all->RegisterButton(GetId());
        button_upgrade_all->RegisterButton(GetId());
    }

    button_done->RegisterButton(GetId());
    button_up_arrow->RegisterButton(GetId());
    button_down_arrow->RegisterButton(GetId());
    button_help->RegisterButton(GetId());

    for (int32_t i = 0; i < unit_slots_per_screen; ++i) {
        repair_slots[i]->Init();
    }

    Draw(false);

    win_draw(GetId());
    mouse_show();
}

RepairShopMenu::~RepairShopMenu() { Deinit(); }

void RepairShopMenu::Run() {
    int32_t key;

    while (!exit_loop) {
        key = get_input();

        if (key == GNW_KB_KEY_LALT_P) {
            PauseMenu_Menu();

        } else if (key == GNW_KB_KEY_SHIFT_DIVIDE) {
            switch (repairshop->unit_type) {
                case HANGAR: {
                    HelpMenu_Menu(HELPMENU_HANGAR_SETUP, WINDOW_MAIN_WINDOW);
                } break;

                case DOCK: {
                    HelpMenu_Menu(HELPMENU_DOCK_SETUP, WINDOW_MAIN_WINDOW);
                } break;

                case DEPOT: {
                    HelpMenu_Menu(HELPMENU_DEPOT_SETUP, WINDOW_MAIN_WINDOW);
                } break;

                case BARRACKS: {
                    HelpMenu_Menu(HELPMENU_BARRACKS_SETUP, WINDOW_MAIN_WINDOW);
                } break;

                default: {
                    HelpMenu_Menu(HELPMENU_TRANSFER_SETUP, WINDOW_MAIN_WINDOW);
                } break;
            }
        }

        GameManager_ProcessState(false);

        if (GameManager_RequestMenuExit || key == GNW_KB_KEY_ESCAPE || key == GNW_KB_KEY_KP_ENTER) {
            RepairShopMenu_OnClick_Done(0, reinterpret_cast<intptr_t>(this));
        }
    }
}

bool RepairShopMenu::IsReloadViable(UnitInfo *target_unit) {
    bool result;

    if (target_unit->ammo < target_unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO)) {
        Cargo materials;
        Cargo capacity;

        repairshop->GetComplex()->GetCargoInfo(materials, capacity);

        result = materials.raw > 0;

    } else {
        result = false;
    }

    return result;
}

bool RepairShopMenu::IsRepairViable(UnitInfo *target_unit) {
    bool result;

    if (target_unit->hits < target_unit->GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
        Cargo materials;
        Cargo capacity;

        repairshop->GetComplex()->GetCargoInfo(materials, capacity);

        result = materials.raw > 0;

    } else {
        result = false;
    }

    return result;
}

bool RepairShopMenu::IsUpgradeViable(UnitInfo *target_unit) {
    bool result;

    if (target_unit->GetBaseValues() !=
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[target_unit->team], target_unit->unit_type) &&
        !(target_unit->flags & REGENERATING_UNIT)) {
        Cargo materials;
        Cargo capacity;

        repairshop->GetComplex()->GetCargoInfo(materials, capacity);

        result = materials.raw >= (target_unit->GetNormalRateBuildCost() / 4);

    } else {
        result = false;
    }

    return result;
}

bool RepairShopMenu::IsActivateViable(UnitInfo *target_unit) {
    return (!(target_unit->flags & MOBILE_AIR_UNIT) || target_unit->speed > 0);
}

void RepairShopMenu::Activate(UnitInfo *target_unit) {
    Deinit();

    if (repairshop->unit_type == AIRTRANS) {
        if (Access_IsAccessible(target_unit->unit_type, target_unit->team, repairshop->grid_x, repairshop->grid_y, 2)) {
            repairshop->SetParent(target_unit);
            UnitsManager_SetNewOrder(&*repairshop, ORDER_UNLOAD, ORDER_STATE_INIT);
            GameManager_EnableMainMenu(&*repairshop);

        } else {
            MessageManager_DrawMessage(_(eeb4), 1, 0);
            GameManager_EnableMainMenu(&*repairshop);
        }

    } else {
        int16_t grid_x = repairshop->grid_x;
        int16_t grid_y = repairshop->grid_y;

        if (Access_FindReachableSpot(target_unit->unit_type, target_unit, &grid_x, &grid_y, 1,
                                     repairshop->flags & BUILDING, 0)) {
            repairshop->FollowUnit();
            MessageManager_DrawMessage(_(755b), 0, 0);
            GameManager_EnableMainMenu(target_unit);

        } else {
            MessageManager_DrawMessage(_(a1f0), 1, 0);
            GameManager_EnableMainMenu(&*repairshop);
        }
    }
}

void RepairShopMenu::Reload(UnitInfo *target_unit) {
    if (IsReloadViable(target_unit)) {
        repairshop->SetParent(target_unit);
        UnitsManager_SetNewOrder(&*repairshop, ORDER_RELOAD, ORDER_STATE_INIT);

        if (Remote_IsNetworkGame) {
            RepairShopMenu_ProcessOrders(&*repairshop, ORDER_RELOAD, false);

        } else {
            UnitsManager_ProcessOrders();
        }
    }
}

void RepairShopMenu::Repair(UnitInfo *target_unit) {
    if (IsRepairViable(target_unit)) {
        repairshop->SetParent(target_unit);
        UnitsManager_SetNewOrder(&*repairshop, ORDER_REPAIR, ORDER_STATE_INIT);

        if (Remote_IsNetworkGame) {
            RepairShopMenu_ProcessOrders(&*repairshop, ORDER_REPAIR, false);

        } else {
            UnitsManager_ProcessOrders();
        }
    }
}

void RepairShopMenu::Upgrade(UnitInfo *target_unit) {
    if (IsUpgradeViable(target_unit)) {
        repairshop->SetParent(target_unit);
        UnitsManager_SetNewOrder(&*repairshop, ORDER_UPGRADE, ORDER_STATE_INIT);

        if (Remote_IsNetworkGame) {
            RepairShopMenu_ProcessOrders(&*repairshop, ORDER_UPGRADE, false);

        } else {
            UnitsManager_ProcessOrders();
        }
    }
}

void RepairShopMenu_Menu(UnitInfo *unit) { RepairShopMenu(unit).Run(); }

void RepairShopSlot_OnClick_Activate(ButtonID bid, intptr_t value) {
    RepairShopSlot *slot;

    slot = reinterpret_cast<RepairShopSlot *>(value);

    if (slot->client != nullptr) {
        slot->menu->Activate(&*slot->client);
    }
}

void RepairShopSlot_OnClick_Reload(ButtonID bid, intptr_t value) {
    RepairShopSlot *slot;

    slot = reinterpret_cast<RepairShopSlot *>(value);

    if (slot->client != nullptr && slot->menu->IsReloadViable(&*slot->client)) {
        slot->menu->Reload(&*slot->client);
        slot->menu->DrawCargoBar(true);
        slot->menu->UpdateButtons();
        slot->DrawStats(true);
        slot->UpdateButtons();

        SoundManager_PlayVoice(V_M085, V_F085);
    }
}

void RepairShopSlot_OnClick_Repair(ButtonID bid, intptr_t value) {
    RepairShopSlot *slot;

    slot = reinterpret_cast<RepairShopSlot *>(value);

    if (slot->client != nullptr && slot->menu->IsRepairViable(&*slot->client)) {
        slot->menu->Repair(&*slot->client);
        slot->menu->DrawCargoBar(true);
        slot->menu->UpdateButtons();
        slot->DrawStats(true);
        slot->UpdateButtons();

        SoundManager_PlayVoice(V_M210, V_F210);
    }
}

void RepairShopSlot_OnClick_Upgrade(ButtonID bid, intptr_t value) {
    RepairShopSlot *slot;

    slot = reinterpret_cast<RepairShopSlot *>(value);

    if (slot->client != nullptr && slot->menu->IsUpgradeViable(&*slot->client)) {
        slot->menu->Upgrade(&*slot->client);
        slot->menu->DrawCargoBar(true);
        slot->Draw(&*slot->client, true);
        slot->menu->UpdateButtons();
    }
}

void RepairShopMenu_OnClick_ActivateAll(ButtonID bid, intptr_t value) {
    RepairShopMenu *shop = reinterpret_cast<RepairShopMenu *>(value);
    SmartPointer<UnitInfo> unit(shop->repairshop);
    SmartList<UnitInfo> units;
    Point point(unit->grid_x - 1, unit->grid_y + 1);
    Rect bounds;
    int32_t limit;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    if (unit->flags & BUILDING) {
        ++point.y;
        limit = 3;

    } else {
        limit = 2;
    }

    for (SmartList<UnitInfo>::Iterator it = shop->units.Begin(); it != shop->units.End(); ++it) {
        units.PushBack(*it);
    }

    shop->Deinit();

    for (int32_t i = 0; i < 8 && units.GetCount() > 0; i += 2) {
        for (int32_t j = 0; j < limit && units.GetCount() > 0; ++j) {
            point += Paths_8DirPointsArray[i];

            if (Access_IsInsideBounds(&bounds, &point)) {
                SmartList<UnitInfo>::Iterator it = units.Begin();

                for (; it != units.End() && !Access_IsAccessible((*it).unit_type, (*it).team, point.x, point.y, 2);
                     ++it) {
                }

                if (it != units.End()) {
                    unit->SetParent(&*it);
                    unit->target_grid_x = point.x;
                    unit->target_grid_y = point.y;

                    UnitsManager_SetNewOrder(&*unit, ORDER_ACTIVATE, ORDER_STATE_1);

                    if (Remote_IsNetworkGame) {
                        RepairShopMenu_ProcessOrders(&*unit, ORDER_ACTIVATE, true);

                    } else {
                        while (unit->orders == ORDER_ACTIVATE) {
                            UnitsManager_ProcessOrders();
                            GameManager_ProcessState(true);
                        }
                    }

                    units.Remove(it);
                }
            }
        }
    }

    GameManager_EnableMainMenu(&*GameManager_SelectedUnit);
}

void RepairShopMenu_OnClick_ReloadAll(ButtonID bid, intptr_t value) {
    RepairShopMenu *shop;
    bool success;

    shop = reinterpret_cast<RepairShopMenu *>(value);
    success = false;

    for (SmartList<UnitInfo>::Iterator it = shop->units.Begin(); it != shop->units.End(); ++it) {
        if (shop->IsReloadViable(&*it)) {
            shop->Reload(&*it);
            success = true;
        }
    }

    shop->Draw(true);

    if (success) {
        SoundManager_PlayVoice(V_M089, V_F089);
    }
}

void RepairShopMenu_OnClick_RepairAll(ButtonID bid, intptr_t value) {
    RepairShopMenu *shop;
    bool success;

    shop = reinterpret_cast<RepairShopMenu *>(value);
    success = false;

    for (SmartList<UnitInfo>::Iterator it = shop->units.Begin(); it != shop->units.End(); ++it) {
        if (shop->IsRepairViable(&*it)) {
            shop->Repair(&*it);
            success = true;
        }
    }

    shop->Draw(true);

    if (success) {
        SoundManager_PlayVoice(V_M211, V_F211);
    }
}

void RepairShopMenu_OnClick_UpgradeAll(ButtonID bid, intptr_t value) {
    RepairShopMenu *shop;

    shop = reinterpret_cast<RepairShopMenu *>(value);

    for (SmartList<UnitInfo>::Iterator it = shop->units.Begin(); it != shop->units.End(); ++it) {
        shop->Upgrade(&*it);
    }

    shop->Draw(true);
}

void RepairShopMenu_OnClick_Done(ButtonID bid, intptr_t value) {
    RepairShopMenu *shop;

    shop = reinterpret_cast<RepairShopMenu *>(value);

    shop->Deinit();

    GameManager_EnableMainMenu(&*GameManager_SelectedUnit);
}

void RepairShopMenu_OnClick_UpArrow(ButtonID bid, intptr_t value) {
    RepairShopMenu *shop;

    shop = reinterpret_cast<RepairShopMenu *>(value);

    shop->unit_slot_index -= shop->unit_slots_per_screen;

    if (shop->unit_slot_index < 0) {
        shop->unit_slot_index = 0;
    }

    shop->Draw(true);
}

void RepairShopMenu_OnClick_DownArrow(ButtonID bid, intptr_t value) {
    RepairShopMenu *shop;

    shop = reinterpret_cast<RepairShopMenu *>(value);

    if (shop->unit_slot_index + shop->unit_slots_per_screen < shop->units.GetCount()) {
        shop->unit_slot_index += shop->unit_slots_per_screen;

        shop->Draw(true);
    }
}

void RepairShopMenu_OnClick_Help(ButtonID bid, intptr_t value) {
    RepairShopMenu *shop;

    shop = reinterpret_cast<RepairShopMenu *>(value);

    switch (shop->repairshop->unit_type) {
        case HANGAR: {
            HelpMenu_Menu(HELPMENU_HANGAR_SETUP, WINDOW_MAIN_WINDOW);
        } break;

        case DOCK: {
            HelpMenu_Menu(HELPMENU_DOCK_SETUP, WINDOW_MAIN_WINDOW);
        } break;

        case DEPOT: {
            HelpMenu_Menu(HELPMENU_DEPOT_SETUP, WINDOW_MAIN_WINDOW);
        } break;

        case BARRACKS: {
            HelpMenu_Menu(HELPMENU_BARRACKS_SETUP, WINDOW_MAIN_WINDOW);
        } break;

        default: {
            HelpMenu_Menu(HELPMENU_TRANSFER_SETUP, WINDOW_MAIN_WINDOW);
        } break;
    }
}

void RepairShopMenu_ProcessOrders(UnitInfo *unit, uint8_t order, bool process_tick) {
    while (unit->orders == order) {
        GameManager_ProcessState(process_tick);
    }

    while (!Remote_UiProcessTick()) {
        ;
    }

    UnitsManager_ProcessOrders();
}

void RepairShopMenu::Draw(bool draw_to_screen) {
    SmartList<UnitInfo>::Iterator it = units.Begin();

    UpdateButtons();

    for (int32_t i = 0; i < unit_slot_index; ++i) {
        ++it;
    }

    for (int32_t i = 0; i < unit_slots_per_screen; ++i) {
        if (it != units.End()) {
            repair_slots[i]->Draw(&*it, draw_to_screen);
            ++it;

        } else {
            repair_slots[i]->Draw(nullptr, draw_to_screen);
        }
    }

    if (raw_material_in_complex > 0) {
        DrawCargoBar(draw_to_screen);
    }
}

void RepairShopMenu::UpdateButtons() {
    bool is_reload_viable;
    bool is_repair_viable;
    bool is_upgrade_viable;

    if (unit_slot_index) {
        button_up_arrow->Enable();

    } else {
        button_up_arrow->Disable();
    }

    if (unit_slot_index + unit_slots_per_screen < units.GetCount()) {
        button_down_arrow->Enable();

    } else {
        button_down_arrow->Disable();
    }

    button_help->Enable();

    is_reload_viable = false;
    is_repair_viable = false;
    is_upgrade_viable = false;

    if (units.GetCount()) {
        if (button_activate_all) {
            button_activate_all->Enable();
        }

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            if (IsReloadViable(&*it)) {
                is_reload_viable = true;
            }

            if (IsRepairViable(&*it)) {
                is_repair_viable = true;
            }

            if (IsUpgradeViable(&*it)) {
                is_upgrade_viable = true;
            }
        }

    } else {
        if (button_activate_all) {
            button_activate_all->Disable();
        }
    }

    if (button_reload_all) {
        button_reload_all->Enable(is_reload_viable);
        button_repair_all->Enable(is_repair_viable);
        button_upgrade_all->Enable(is_upgrade_viable);
    }
}

void RepairShopMenu::DrawCargoBar(bool draw_to_screen) {
    Cargo materials;
    Cargo capacity;
    WindowInfo window;
    int32_t bar_height;
    uint8_t *buffer;
    char text[50];

    FillWindowInfo(&window);
    repairshop->GetComplex()->GetCargoInfo(materials, capacity);

    raw_material_bar->Write(&window);
    raw_material_value_area->Write(&window);

    if (raw_material_in_complex) {
        bar_height = (materials.raw * raw_material_bar->GetHeight()) / raw_material_in_complex;

    } else {
        bar_height = 0;
    }

    buffer = &window.buffer[raw_material_bar->GetULY() * window.width + raw_material_bar->GetULX()];
    buffer = &buffer[(raw_material_bar->GetHeight() - bar_height) * window.width];

    LoadVerticalBar(buffer, window.width, bar_height, raw_material_bar->GetWidth(), VERTRAW);

    Text_SetFont(GNW_TEXT_FONT_5);

    sprintf(text, "%i", materials.raw);

    Text_TextBox(window.buffer, window.width, text, raw_material_value_area->GetULX(),
                 raw_material_value_area->GetULY(), raw_material_value_area->GetWidth(),
                 raw_material_value_area->GetHeight(), 0xA2, true);

    if (draw_to_screen) {
        raw_material_bar->Draw(window.id);
        raw_material_value_area->Draw(window.id);
    }
}

void RepairShopMenu::Deinit() {
    exit_loop = true;

    if (unit_slots_per_screen > 0) {
        for (int32_t i = 0; i < unit_slots_per_screen; ++i) {
            delete repair_slots[i];
            repair_slots[i] = nullptr;
        }

        unit_slots_per_screen = 0;

        delete raw_material_bar;
        delete raw_material_value_area;
        delete button_activate_all;
        delete button_reload_all;
        delete button_repair_all;
        delete button_upgrade_all;
        delete button_done;
        delete button_up_arrow;
        delete button_down_arrow;
        delete button_help;

        raw_material_bar = nullptr;
        raw_material_value_area = nullptr;
        button_activate_all = nullptr;
        button_reload_all = nullptr;
        button_repair_all = nullptr;
        button_upgrade_all = nullptr;
        button_done = nullptr;
        button_up_arrow = nullptr;
        button_down_arrow = nullptr;
        button_help = nullptr;

        win_delete(GetId());
        ResetId();
    }
}
