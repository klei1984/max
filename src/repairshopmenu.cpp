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

class RepairShopSlot {
    RepairShopMenu *repairshopmenu;
    SmartPointer<UnitInfo> unit;

    Image *bg_image_area;
    Image *bg_stats_area;

    Button *button_activate;
    Button *button_reload;
    Button *button_repair;
    Button *button_upgrade;

    friend void RepairShopMenu_OnClick_ActivateAll(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_ReloadAll(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_RepairAll(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_UpgradeAll(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_Done(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_UpArrow(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_DownArrow(ButtonID bid, intptr_t value);
    friend void RepairShopMenu_OnClick_Help(ButtonID bid, intptr_t value);

public:
    RepairShopSlot();
    ~RepairShopSlot();

    void Init();
};

RepairShopMenu::RepairShopMenu(UnitInfo *unit) : Window(unit->unit_type == HANGAR ? HANGRFRM : DEPOTFRM) {}

RepairShopMenu::~RepairShopMenu() {}

void RepairShopMenu::Run() {}

void RepairShopMenu_OnClick_ActivateAll(ButtonID bid, intptr_t value) {}

void RepairShopMenu_OnClick_ReloadAll(ButtonID bid, intptr_t value) {}

void RepairShopMenu_OnClick_RepairAll(ButtonID bid, intptr_t value) {}

void RepairShopMenu_OnClick_UpgradeAll(ButtonID bid, intptr_t value) {}

void RepairShopMenu_OnClick_Done(ButtonID bid, intptr_t value) {}

void RepairShopMenu_OnClick_UpArrow(ButtonID bid, intptr_t value) {}

void RepairShopMenu_OnClick_DownArrow(ButtonID bid, intptr_t value) {}

void RepairShopMenu_OnClick_Help(ButtonID bid, intptr_t value) {}
