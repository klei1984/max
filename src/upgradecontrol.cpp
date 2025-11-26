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

#include <cmath>

#include "sound_manager.hpp"

int32_t UpgradeControl_Factors[UPGRADE_CONTROL_COUNT] = {4, 4, 8, 2, 2, 2, 8, 4, 16};

UpgradeControl::UpgradeControl(WinID window_id, int32_t ulx, int32_t uly, int32_t button_right_r_value,
                               int32_t button_left_r_value, uint32_t* team_gold)
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

int32_t UpgradeControl::CalculateCost() {
    int32_t result;

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

int32_t UpgradeControl::GetLevelCost(uint16_t current_value) {
    return UpgradeControl_CalculateCost(id, current_value, research_factor, team_base_value);
}

int32_t UpgradeControl::GetRatio(uint16_t value) const {
    int32_t result;

    result = ((int32_t)factor * value) / team_base_value;

    return result;
}

void UpgradeControl::SetupRatio(uint16_t factor, int32_t* upgrade_ratio) {
    this->factor = factor;
    this->upgrade_ratio = upgrade_ratio;
}

void UpgradeControl::Init(int32_t id, int32_t team_base_value, int32_t control_base_value,
                          uint16_t* control_actual_value, int32_t research_factor) {
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

    if (id == UPGRADE_CONTROL_6 || id == UPGRADE_CONTROL_8) {
        if (*control_actual_value + upgrade_amount > UINT16_MAX) {
            upgrade_amount = UINT16_MAX - *control_actual_value;
        }
    }
}

void UpgradeControl::Increase() {
    SoundManager_PlaySfx(KCARG0);

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
    SoundManager_PlaySfx(KCARG0);

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
            if (static_cast<uint32_t>(CalculateCost()) <= *team_gold && *control_actual_value > 1) {
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
            int32_t cost = CalculateCost();

            if (cost >= 0 && static_cast<uint32_t>(cost) <= *team_gold && cost < UPGRADECONTROL_UPGRADE_COST_LIMIT) {
                upgrade_right->Enable();

                if (id == UPGRADE_CONTROL_6 || id == UPGRADE_CONTROL_8) {
                    if (*control_actual_value >= UINT16_MAX) {
                        upgrade_right->Disable();
                    }
                }

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

int32_t UpgradeControl::GetCost() { return CalculateCost(); }

int32_t UpgradeControl_CalculateCost(int32_t id, uint16_t current_value, uint16_t factor, uint16_t base_value) {
    int32_t result;

    if (id == UPGRADE_CONTROL_9) {
        result =
            std::pow((double)base_value / (double)((int32_t)current_value - factor), 7.5) * UpgradeControl_Factors[id];
    } else if (base_value > 0) {
        result =
            std::pow((double)((int32_t)current_value - factor) / (double)base_value, 7.5) * UpgradeControl_Factors[id];
    } else {
        result = 0;
    }

    return result;
}

int32_t UpgradeControl::GetId() const { return id; }

int32_t UpgradeControl::GetUly() const { return uly; }
