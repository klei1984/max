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

#ifndef CARGOMENU_HPP
#define CARGOMENU_HPP

#include "abstractupgrademenu.hpp"
#include "cargoselector.hpp"
#include "purchasetypeselector.hpp"
#include "scrollbar.hpp"

class CargoMenu : public AbstractUpgradeMenu {
    SmartObjectArray<ResourceID> unit_types1;
    SmartObjectArray<ResourceID> unit_types2;
    SmartObjectArray<uint16_t> cargos;
    CargoSelector *cargo_selector;
    UnitTypeSelector *active_selector;
    Button *button_purchase_list_up;
    Button *button_purchase_list_down;
    Button *button_cargo_up;
    Button *button_cargo_down;
    Button *button_buy_upgrade_toggle;
    Button *button_delete;
    Button *button_buy;
    LimitedScrollbar *scrollbar;
    uint16_t unit_count;

    void Select(int32_t index);
    void BuyUnit();
    void DeleteUnit();
    void UpdateScrollbar();
    void UpdateTeamGold(int32_t factor);

public:
    CargoMenu(uint16_t team);
    ~CargoMenu();

    bool EventHandler(Event *event);
    void DrawUnitInfo(ResourceID unit_type);
    void AbstractUpgradeMenu_vfunc3(ResourceID unit_type);
    bool AbstractUpgradeMenu_vfunc4(UnitTypeSelector *selector, bool mode);
    void PopulateTeamUnitsList();
    void DrawUnitStats(ResourceID unit_type);
    void AbstractUpgradeMenu_vfunc7();
    bool ProcessKey(int32_t key);

    UnitValues *GetCurrentUnitValues(ResourceID unit_type);
};

#endif /* CARGOMENU_HPP */
