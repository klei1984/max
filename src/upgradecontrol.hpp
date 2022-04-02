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

#ifndef UPGRADECONTROL_HPP
#define UPGRADECONTROL_HPP

#include "button.hpp"

enum {
    UPGRADE_CONTROL_0,
    UPGRADE_CONTROL_1,
    UPGRADE_CONTROL_2,
    UPGRADE_CONTROL_3,
    UPGRADE_CONTROL_4,
    UPGRADE_CONTROL_5,
    UPGRADE_CONTROL_6,
    UPGRADE_CONTROL_7,
    UPGRADE_CONTROL_8,
    UPGRADE_CONTROL_9,
    UPGRADE_CONTROL_COUNT
};

class UpgradeControl {
    unsigned short *team_gold;
    unsigned short uly;
    Button *upgrade_right;
    Button *upgrade_left;
    unsigned char id;
    unsigned short team_base_value;
    unsigned short control_base_value;
    unsigned short upgrade_amount;
    unsigned short *control_actual_value;
    unsigned short factor;
    unsigned short *upgrade_ratio;
    unsigned short research_factor;

    void Disable();
    int CalculateCost();
    int GetLevelCost(unsigned short current_value);
    int GetRatio(unsigned short value) const;
    void SetupRatio(unsigned short factor, unsigned short *upgrade_ratio);

public:
    UpgradeControl(WinID window_id, int ulx, int uly, int button_right_r_value, int button_left_r_value,
                   unsigned short *team_gold);
    ~UpgradeControl();

    void Init(int id, int base_value_team, int base_value_control, unsigned short *actual_value_control, int value);
    void Increase();
    void Decrease();
    void UpdateControlState();
    int GetCost();
    int GetId() const;
    int GetUly() const;
};

int UpgradeControl_CalculateCost(int id, unsigned short current_value, unsigned short factor,
                                 unsigned short base_value);

#endif /* UPGRADECONTROL_HPP */
