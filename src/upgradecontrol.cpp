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

#include "upgradecontrol.hpp"

#include "sound_manager.hpp"

int UpgradeControl_Factors[UPGRADE_CONTROL_COUNT] = {0, 4, 4, 8, 2, 2, 2, 8, 4, 16};

UpgradeControl::UpgradeControl(WinID window_id, int ulx, int uly, int button_right_r_value, int button_left_r_value,
                               unsigned short *team_gold)
    : team_gold(team_gold), factor(1), upgrade_ratio(nullptr), uly(uly) {
    upgrade_right = new (std::nothrow) Button(UPGRGT_U, UPGRGT_D, ulx + 18, uly);
    upgrade_right->SetRValue(button_right_r_value);
    upgrade_right->CopyUpDisabled(UPGRGT_X);
    upgrade_right->Copy(window_id);
    upgrade_right->RegisterButton(window_id);

    upgrade_left = new (std::nothrow) Button(UPGLFT_U, UPGLFT_D, ulx, uly);
    upgrade_left->SetRValue(button_left_r_value);
    upgrade_left->CopyUpDisabled(UPGLFT_X);
    upgrade_left->Copy(window_id);
    upgrade_left->RegisterButton(window_id);
}

UpgradeControl::~UpgradeControl() {
    delete upgrade_right;
    delete upgrade_left;
}

void UpgradeControl::Disable() {
    upgrade_right->Disable();
    upgrade_left->Disable();
    id = 0;
}

int UpgradeControl::CalculateCost() {
    int result;

    if (this->id == UPGRADE_CONTROL_9) {
        if (*control_actual_value > upgrade_amount) {
            result = GetLevelCost(*control_actual_value - upgrade_amount) - GetLevelCost(*control_actual_value);
            if (result < 1) {
                result = 1;
            }

        } else {
            result = 0;
        }

    } else if (team_base_value > 0) {
        result = GetLevelCost(*control_actual_value + upgrade_amount) - GetLevelCost(*control_actual_value);
        if (result < 1) {
            result = 1;
        }

    } else {
        result = 0;
    }

    return result;
}

int UpgradeControl::GetLevelCost(unsigned short current_value) {
    return UpgradeControl_CalculateCost(id, current_value, research_factor, team_base_value);
}

int UpgradeControl::GetRatio(unsigned short value) const {
    int result;

    result = ((int)factor * value) / team_base_value;

    return result;
}

void UpgradeControl::SetupRatio(unsigned short factor, unsigned short *upgrade_ratio) {
    this->factor = factor;
    this->upgrade_ratio = upgrade_ratio;
}

void UpgradeControl::Init(int id, int team_base_value, int control_base_value, unsigned short *control_actual_value,
                          int research_factor) {
    this->id = id;
    this->team_base_value = team_base_value;
    this->control_base_value = control_base_value;
    this->control_actual_value = control_actual_value;
    this->research_factor = research_factor;

    if (this->id == UPGRADE_CONTROL_9) {
        upgrade_amount = 1;
    } else if (team_base_value >= 10) {
        if (team_base_value >= 25) {
            if (team_base_value >= 50) {
                upgrade_amount = 10;
            } else {
                upgrade_amount = 5;
            }
        } else {
            upgrade_amount = 2;
        }
    } else {
        upgrade_amount = 1;
    }
}

void UpgradeControl::Increase() {
    SoundManager.PlaySfx(KCARG0);

    if (id != UPGRADE_CONTROL_9) {
        *team_gold -= CalculateCost();
    }

    *control_actual_value += upgrade_amount;

    if (upgrade_ratio) {
        *upgrade_ratio = GetRatio(*control_actual_value);
    }

    if (id == UPGRADE_CONTROL_9) {
        *team_gold += CalculateCost();
    }
}

void UpgradeControl::Decrease() {
    SoundManager.PlaySfx(KCARG0);

    if (id == UPGRADE_CONTROL_9) {
        *team_gold -= CalculateCost();
    }

    *control_actual_value -= upgrade_amount;

    if (upgrade_ratio) {
        *upgrade_ratio = GetRatio(*control_actual_value);
    }

    if (id != UPGRADE_CONTROL_9) {
        *team_gold += CalculateCost();
    }
}

void UpgradeControl::UpdateControlState() {
    if (team_base_value) {
        if (id == UPGRADE_CONTROL_9) {
            if (CalculateCost() <= *team_gold && *control_actual_value > 1) {
                upgrade_left->Enable();
            } else {
                upgrade_left->Disable();
            }

            if (control_base_value <= *control_actual_value) {
                upgrade_right->Disable();
            } else {
                upgrade_right->Enable();
            }

        } else {
            int cost = CalculateCost();

            if (cost <= *team_gold && cost < UPGRADECONTROL_UPGRADE_COST_LIMIT) {
                upgrade_right->Enable();
            } else {
                upgrade_right->Disable();
            }

            if (control_base_value >= *control_actual_value) {
                upgrade_left->Disable();
            } else {
                upgrade_left->Enable();
            }
        }
    } else {
        Disable();
    }
}

int UpgradeControl::GetCost() { return CalculateCost(); }

int UpgradeControl_CalculateCost(int id, unsigned short current_value, unsigned short factor,
                                 unsigned short base_value) {
    int result;

    if (id == UPGRADE_CONTROL_9) {
        result = pow((double)base_value / (double)((int)current_value - factor), 7.5) * UpgradeControl_Factors[id];
    } else if (base_value > 0) {
        result = pow((double)((int)current_value - factor) / (double)base_value, 7.5) * UpgradeControl_Factors[id];
    } else {
        result = 0;
    }

    return result;
}

int UpgradeControl::GetId() const { return id; }

int UpgradeControl::GetUly() const { return uly; }
