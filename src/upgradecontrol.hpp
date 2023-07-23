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
#include "unitvalues.hpp"

#define UPGRADECONTROL_UPGRADE_COST_LIMIT 800

enum {
    UPGRADE_CONTROL_1 = ATTRIB_ATTACK,
    UPGRADE_CONTROL_2 = ATTRIB_ROUNDS,
    UPGRADE_CONTROL_3 = ATTRIB_RANGE,
    UPGRADE_CONTROL_4 = ATTRIB_AMMO,
    UPGRADE_CONTROL_5 = ATTRIB_ARMOR,
    UPGRADE_CONTROL_6 = ATTRIB_HITS,
    UPGRADE_CONTROL_7 = ATTRIB_SCAN,
    UPGRADE_CONTROL_8 = ATTRIB_SPEED,
    UPGRADE_CONTROL_9 = ATTRIB_TURNS,
    UPGRADE_CONTROL_COUNT = 9
};

class UpgradeControl {
    uint16_t *team_gold;
    uint16_t uly;
    Button *upgrade_right;
    Button *upgrade_left;
    uint8_t id;
    uint16_t team_base_value;
    uint16_t control_base_value;
    uint16_t upgrade_amount;
    uint16_t *control_actual_value;
    uint16_t factor;
    uint16_t *upgrade_ratio;
    uint16_t research_factor;

    void Disable();
    int32_t CalculateCost();
    int32_t GetLevelCost(uint16_t current_value);
    int32_t GetRatio(uint16_t value) const;
    void SetupRatio(uint16_t factor, uint16_t *upgrade_ratio);

public:
    UpgradeControl(WinID window_id, int32_t ulx, int32_t uly, int32_t button_right_r_value, int32_t button_left_r_value,
                   uint16_t *team_gold);
    ~UpgradeControl();

    void Init(int32_t id, int32_t base_value_team, int32_t base_value_control, uint16_t *actual_value_control, int32_t value);
    void Increase();
    void Decrease();
    void UpdateControlState();
    int32_t GetCost();
    int32_t GetId() const;
    int32_t GetUly() const;
};

int32_t UpgradeControl_CalculateCost(int32_t id, uint16_t current_value, uint16_t factor,
                                 uint16_t base_value);

#endif /* UPGRADECONTROL_HPP */
