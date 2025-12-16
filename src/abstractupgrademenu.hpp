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

#ifndef ABSTRACTUPGRADEMENU_H
#define ABSTRACTUPGRADEMENU_H

#include "button.hpp"
#include "unittypeselector.hpp"
#include "unitvalues.hpp"
#include "upgradecontrol.hpp"
#include "window.hpp"

class AbstractUpgradeMenu : public Window {
protected:
    WindowInfo dialog_window;
    WindowInfo unit_portrait_window;
    uint16_t team;
    uint32_t start_gold;
    uint32_t team_gold;
    uint16_t upgrade_control_count;
    uint16_t upgrade_control_next_uly;
    ResourceID interface_icon_full;
    ResourceID interface_icon_empty;
    ResourceID unit_type;
    SmartPointer<UnitValues> unitvalues_base[UNIT_END];
    SmartPointer<UnitValues> unitvalues_actual[UNIT_END];
    UnitTypeSelector* type_selector;
    Button* button_scroll_up;
    Button* button_scroll_down;
    Button* button_done;
    Button* button_help;
    Button* button_cancel;
    Button* button_ground;
    Button* button_air;
    Button* button_sea;
    Button* button_building;
    Button* button_combat;
    Button* button_description;
    UpgradeControl* upgrade_controls[UPGRADE_CONTROL_COUNT];
    uint8_t event_click_done;
    uint8_t event_click_cancel;
    uint8_t buy_upgrade_toggle_state;
    uint8_t event_release;
    Image* stats_background;
    Image* cost_background;
    Image* gold_background;
    Image* button_background;

    static bool button_ground_rest_state;
    static bool button_air_rest_state;
    static bool button_sea_rest_state;
    static bool button_building_rest_state;
    static bool button_combat_rest_state;
    static bool button_description_rest_state;

    void AddUpgrade(int32_t id, int32_t value1, int32_t value2, uint16_t* attribute, int32_t value);
    void AddUpgradeMilitary(ResourceID unit_type);
    void AdjustRowStorage(ResourceID unit_type);
    void AdjustRowConsumptions(ResourceID unit_type);
    void AddUpgradeGeneric(ResourceID unit_type);
    void AddUpgradeMobile(ResourceID unit_type);
    bool IsUnitFiltered(ResourceID unit_type);
    bool IsBetterUnit(ResourceID unit_type1, ResourceID unit_type2);

protected:
    void Init();

public:
    AbstractUpgradeMenu(uint16_t team, ResourceID bg_image);
    virtual ~AbstractUpgradeMenu();

    bool EventHandler(Event* event);
    virtual void DrawUnitInfo(ResourceID unit_type);
    virtual void OnUnitTypeConfirmed(ResourceID unit_type);
    virtual bool HandleUnitTypeSelection(UnitTypeSelector* selector, bool mode);
    virtual void PopulateTeamUnitsList();
    virtual void DrawUnitStats(ResourceID unit_type);
    virtual void CommitUpgradeChanges();
    virtual bool ProcessKey(int32_t key);

    bool Run();
};

#endif /* ABSTRACTUPGRADEMENU_H */
