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
#include "window.hpp"

#define ABSTRACT_UPGRADE_MENU_UPGRADE_CONTROL_ITEM_COUNT 10

class UpgradeControl;

class AbstractUpgradeMenu : public Window {
    WindowInfo window1;
    WindowInfo window2;
    unsigned short team;
    unsigned short field_77;
    unsigned short team_gold;
    unsigned short upgrade_control_count;
    unsigned short upgrade_control_next_uly;
    unsigned short field_85;
    unsigned short field_87;
    unsigned short field_89;
    SmartPointer<UnitValues> unitvalues_base[UNIT_END];
    SmartPointer<UnitValues> unitvalues_actual[UNIT_END];
    UnitTypeSelector *type_selector;
    Button *button_scroll_up;
    Button *button_scroll_down;
    Button *button_done;
    Button *button_help;
    Button *button_cancel;
    Button *button_ground;
    Button *button_air;
    Button *button_sea;
    Button *button_building;
    Button *button_combat;
    Button *button_description;
    UpgradeControl *upgrade_controls[ABSTRACT_UPGRADE_MENU_UPGRADE_CONTROL_ITEM_COUNT];
    unsigned char field_835;
    unsigned char field_836;
    unsigned char field_925;
    unsigned char field_926;
    Image *stats_background;
    Image *cost_background;
    Image *gold_background;
    Image *button_background;

    static bool button_ground_rest_state;
    static bool button_air_rest_state;
    static bool button_sea_rest_state;
    static bool button_building_rest_state;
    static bool button_combat_rest_state;
    static bool button_description_rest_state;

protected:
    void Init();

public:
    AbstractUpgradeMenu(unsigned short team, ResourceID bg_image);
    ~AbstractUpgradeMenu();

    bool Run();
};

#endif /* ABSTRACTUPGRADEMENU_H */
