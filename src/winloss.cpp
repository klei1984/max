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

#include "winloss.hpp"

#include "game_manager.hpp"
#include "inifile.hpp"
#include "units_manager.hpp"

enum {
    TRAINING_MISSION_1 = 1,
    TRAINING_MISSION_2 = 2,
    TRAINING_MISSION_3 = 3,
    TRAINING_MISSION_4 = 4,
    TRAINING_MISSION_5 = 5,
    TRAINING_MISSION_6 = 6,
    TRAINING_MISSION_7 = 7,
    TRAINING_MISSION_8 = 8,
    TRAINING_MISSION_9 = 9,
    TRAINING_MISSION_10 = 10,
    TRAINING_MISSION_11 = 11,
    TRAINING_MISSION_12 = 12,
    TRAINING_MISSION_13 = 13,
    TRAINING_MISSION_14 = 14,
    TRAINING_MISSION_15 = 15,

    SCENARIO_MISSION_9 = 9,
    SCENARIO_MISSION_18 = 18,
    SCENARIO_MISSION_20 = 20,
    SCENARIO_MISSION_24 = 24,

    CAMPAIGN_MISSION_1 = 1,
    CAMPAIGN_MISSION_2 = 2,
    CAMPAIGN_MISSION_3 = 3,
    CAMPAIGN_MISSION_4 = 4,
    CAMPAIGN_MISSION_5 = 5,
    CAMPAIGN_MISSION_6 = 6,
    CAMPAIGN_MISSION_7 = 7,
    CAMPAIGN_MISSION_8 = 8,
    CAMPAIGN_MISSION_9 = 9,
};

static bool WinLoss_HasAttackPower(unsigned short team, SmartList<UnitInfo>* units);
static int WinLoss_HasMaterials(unsigned short team, unsigned char cargo_type);
static bool WinLoss_CanRebuildComplex(unsigned short team);
static bool WinLoss_CanRebuildBuilders(unsigned short team);
static int WinLoss_GetWorthOfAssets(unsigned short team, SmartList<UnitInfo>* units);
static int WinLoss_GetTotalWorthOfAssets(unsigned short team);
static SmartList<UnitInfo>::Iterator WinLoss_GetRelevantUnitListIterator(ResourceID unit_type);
static int WinLoss_CountReadyUnits(unsigned short team, ResourceID unit_type);
static void WinLoss_CountTotalMining(unsigned short team, int* raw_mining_max, int* fuel_mining_max,
                                     int* gold_mining_max);
static bool WinLoss_HasAtLeastOneUpgradedTank(unsigned short team);
static int WinLoss_GetTotalPowerConsumption(unsigned short team, ResourceID unit_type);
static int WinLoss_GetTotalMining(unsigned short team, unsigned char cargo_type);
static int WinLoss_GetTotalConsumption(unsigned short team, unsigned char cargo_type);
static int WinLoss_GetTotalUnitsBeingConstructed(unsigned short team, ResourceID unit_type);
static bool WinLoss_HasInfiltratorExperience(unsigned short team);
static int WinLoss_CountDamangedUnits(unsigned short team, ResourceID unit_type);
static int WinLoss_CountUnitsThatUsedAmmo(unsigned short team, ResourceID unit_type);

bool WinLoss_HasAttackPower(unsigned short team, SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).ammo > 0 && (*it).unit_type != SUBMARNE && (*it).unit_type != COMMANDO) {
            return true;
        }
    }

    return false;
}

int WinLoss_HasMaterials(unsigned short team, unsigned char cargo_type) {
    int cargo_sum;

    cargo_sum = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && UnitsManager_BaseUnits[(*it).unit_type].cargo_type == cargo_type) {
            cargo_sum += (*it).storage;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && UnitsManager_BaseUnits[(*it).unit_type].cargo_type == cargo_type) {
            cargo_sum += (*it).storage;
        }
    }

    return cargo_sum;
}

bool WinLoss_CanRebuildComplex(unsigned short team) {
    SmartList<UnitInfo>::Iterator it;
    bool result;

    for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team &&
            ((*it).unit_type == SHIPYARD || (*it).unit_type == LIGHTPLT || (*it).unit_type == LANDPLT ||
             (*it).unit_type == AIRPLT || (*it).unit_type == TRAINHAL)) {
            break;
        }
    }

    if (it != nullptr) {
        int sum_cargo_fuel;
        int sum_cargo_materials;
        int power_demand;
        int sum_cargo_generated_power;

        sum_cargo_fuel = WinLoss_HasMaterials(team, CARGO_TYPE_FUEL);
        sum_cargo_materials = WinLoss_HasMaterials(team, CARGO_TYPE_RAW);
        power_demand = 1;
        sum_cargo_generated_power = 0;

        if (sum_cargo_fuel < 8 || sum_cargo_materials < 24) {
            for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == team && (*it).unit_type == MININGST) {
                    break;
                }
            }

            if (it != nullptr) {
                power_demand = 2;

            } else {
                result = false;
                return result;
            }
        }

        for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == team && Cargo_GetPowerConsumptionRate((*it).unit_type) < 0) {
                sum_cargo_generated_power -= Cargo_GetPowerConsumptionRate((*it).unit_type);
            }
        }

        result = sum_cargo_generated_power >= power_demand;

    } else {
        result = false;
    }

    return result;
}

bool WinLoss_CanRebuildBuilders(unsigned short team) {
    SmartList<UnitInfo>::Iterator it;
    bool result;

    for (it = UnitsManager_MobileLandSeaUnits.Begin(); it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && (*it).unit_type == CONSTRCT) {
            break;
        }
    }

    if (it != nullptr) {
        for (it = UnitsManager_MobileLandSeaUnits.Begin(); it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
            if ((*it).team == team && (*it).unit_type == ENGINEER) {
                break;
            }
        }

        if (it != nullptr) {
            int resources_needed;

            resources_needed = 38;

            for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == team && (*it).unit_type == MININGST) {
                    break;
                }
            }

            if (it != nullptr) {
                resources_needed = 8;
            }

            for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == team && Cargo_GetPowerConsumptionRate((*it).unit_type) < 0) {
                    break;
                }
            }

            if (it != nullptr) {
                resources_needed -= 8;
            }

            if (resources_needed == 0 || resources_needed <= WinLoss_HasMaterials(team, CARGO_TYPE_RAW)) {
                result = true;

            } else {
                result = false;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool WinLoss_CheckLossConditions(unsigned short team) {
    bool result;

    if (ini_get_setting(INI_GAME_FILE_TYPE) == GAME_TYPE_TRAINING && GameManager_GameFileNumber == 1 &&
        team == PLAYER_TEAM_RED) {
        result = true;

    } else if (WinLoss_HasAttackPower(team, &UnitsManager_MobileLandSeaUnits) ||
               WinLoss_HasAttackPower(team, &UnitsManager_MobileAirUnits) ||
               WinLoss_HasAttackPower(team, &UnitsManager_StationaryUnits) ||
               WinLoss_CanRebuildComplex(team || WinLoss_CanRebuildBuilders(team))) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

int WinLoss_GetWorthOfAssets(unsigned short team, SmartList<UnitInfo>* units) {
    int sum_build_turns_value_of_team;

    sum_build_turns_value_of_team = 0;

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team) {
            sum_build_turns_value_of_team += (*it).GetNormalRateBuildCost();
        }
    }

    return sum_build_turns_value_of_team;
}

int WinLoss_GetTotalWorthOfAssets(unsigned short team) {
    return WinLoss_GetWorthOfAssets(team, &UnitsManager_MobileLandSeaUnits) +
           WinLoss_GetWorthOfAssets(team, &UnitsManager_MobileAirUnits) +
           WinLoss_GetWorthOfAssets(team, &UnitsManager_StationaryUnits);
}

int WinLoss_DetermineWinner(unsigned short team1, unsigned short team2) {
    int result;

    if (UnitsManager_TeamInfo[team1].team_points < UnitsManager_TeamInfo[team2].team_points) {
        result = -1;
    } else if (UnitsManager_TeamInfo[team2].team_points < UnitsManager_TeamInfo[team1].team_points) {
        result = 1;
    } else {
        result = WinLoss_GetTotalWorthOfAssets(team1) - WinLoss_GetTotalWorthOfAssets(team2);
    }

    return result;
}

SmartList<UnitInfo>::Iterator WinLoss_GetRelevantUnitListIterator(ResourceID unit_type) {
    unsigned int flags;

    flags = UnitsManager_BaseUnits[unit_type].flags;

    if (flags & STATIONARY) {
        if (flags & GROUND_COVER) {
            return UnitsManager_GroundCoverUnits.Begin();

        } else {
            return UnitsManager_StationaryUnits.Begin();
        }

    } else if (flags & MOBILE_AIR_UNIT) {
        return UnitsManager_MobileAirUnits.Begin();

    } else {
        return UnitsManager_MobileLandSeaUnits.Begin();
    }
}

int WinLoss_CountReadyUnits(unsigned short team, ResourceID unit_type) {
    int result;

    result = 0;

    for (SmartList<UnitInfo>::Iterator it = WinLoss_GetRelevantUnitListIterator(unit_type); it != nullptr; ++it) {
        if ((*it).team == team && (*it).unit_type == unit_type && (*it).state != ORDER_STATE_BUILDING_READY) {
            ++result;
        }
    }

    return result;
}

void WinLoss_CountTotalMining(unsigned short team, int* raw_mining_max, int* fuel_mining_max, int* gold_mining_max) {
    raw_mining_max[0] = 0;
    fuel_mining_max[0] = 0;
    gold_mining_max[0] = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).unit_type == MININGST && (*it).orders == ORDER_POWER_ON) {
            raw_mining_max[0] += (*it).raw_mining_max;
            fuel_mining_max[0] += (*it).fuel_mining_max;
            gold_mining_max[0] += (*it).gold_mining_max;
        }
    }
}

bool WinLoss_HasAtLeastOneUpgradedTank(unsigned short team) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && (*it).unit_type == TANK && (*it).GetBaseValues()->GetVersion() > 1) {
            return true;
        }
    }

    return false;
}

int WinLoss_GetTotalPowerConsumption(unsigned short team, ResourceID unit_type) {
    int result;

    result = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).unit_type == unit_type &&
            ((*it).orders == ORDER_POWER_ON || (*it).orders == ORDER_BUILD)) {
            ++result;
        }
    }

    return result;
}

int WinLoss_GetTotalMining(unsigned short team, unsigned char cargo_type) {
    int result;

    result = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).unit_type == MININGST && (*it).orders == ORDER_POWER_ON) {
            switch (cargo_type) {
                case CARGO_MATERIALS: {
                    result += (*it).raw_mining;
                } break;

                case CARGO_FUEL: {
                    result += (*it).fuel_mining;
                } break;

                case CARGO_GOLD: {
                    result += (*it).gold_mining;
                } break;
            }
        }
    }

    return result;
}

int WinLoss_GetTotalConsumption(unsigned short team, unsigned char cargo_type) {
    Cargo cargo;
    int result;

    result = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team) {
            Cargo_GetCargoDemand(&*it, &cargo);

            switch (cargo_type) {
                case CARGO_MATERIALS: {
                    if (cargo.raw < 0) {
                        result -= cargo.raw;
                    }
                } break;

                case CARGO_FUEL: {
                    if (cargo.fuel < 0) {
                        result -= cargo.fuel;
                    }
                } break;

                case CARGO_GOLD: {
                    if (cargo.gold < 0) {
                        result -= cargo.gold;
                    }
                } break;
            }
        }
    }

    return result;
}

int WinLoss_GetTotalUnitsBeingConstructed(unsigned short team, ResourceID unit_type) {
    SmartList<UnitInfo>* units;
    int result;

    result = 0;

    if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
        units = &UnitsManager_StationaryUnits;

    } else {
        units = &UnitsManager_MobileLandSeaUnits;
    }

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).orders == ORDER_BUILD && (*it).build_time != 0 &&
            (*it).GetConstructedUnitType() == unit_type) {
            ++result;
        }
    }

    return result;
}

bool WinLoss_HasInfiltratorExperience(unsigned short team) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && (*it).unit_type == COMMANDO && (*it).storage > 0) {
            return true;
        }
    }

    return false;
}

int WinLoss_CountDamangedUnits(unsigned short team, ResourceID unit_type) {
    int result;

    result = 0;

    for (SmartList<UnitInfo>::Iterator it = WinLoss_GetRelevantUnitListIterator(unit_type); it != nullptr; ++it) {
        if ((*it).team == team && (*it).unit_type == unit_type &&
            (*it).hits != (*it).GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
            ++result;
        }
    }

    return result;
}

int WinLoss_CountUnitsThatUsedAmmo(unsigned short team, ResourceID unit_type) {
    int result;

    result = 0;

    for (SmartList<UnitInfo>::Iterator it = WinLoss_GetRelevantUnitListIterator(unit_type); it != nullptr; ++it) {
        if ((*it).team == team && (*it).unit_type == unit_type &&
            (*it).ammo != (*it).GetBaseValues()->GetAttribute(ATTRIB_AMMO)) {
            ++result;
        }
    }

    return result;
}

int WinLoss_CheckWinConditions(unsigned short team, int turn_counter) {
    int result;

    if (team != PLAYER_TEAM_RED) {
        result = VICTORY_STATE_GENERIC;

    } else {
        switch (ini_get_setting(INI_GAME_FILE_TYPE)) {
            case GAME_TYPE_CAMPAIGN: {
                switch (GameManager_GameFileNumber) {
                    case CAMPAIGN_MISSION_1: {
                        if (WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, RESEARCH) == 0) {
                            result = VICTORY_STATE_LOST;

                        } else if (WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, GREENHSE)) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;

                    case CAMPAIGN_MISSION_2: {
                        if (UnitsManager_TeamInfo[team].casualties[RESEARCH] > 0) {
                            result = VICTORY_STATE_LOST;

                        } else if (turn_counter < ini_setting_victory_limit) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;

                    case CAMPAIGN_MISSION_3: {
                        if (WinLoss_CountReadyUnits(team, SPLYTRCK) +
                                WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, SPLYTRCK) <
                            2) {
                            result = VICTORY_STATE_LOST;

                        } else if (WinLoss_CountReadyUnits(team, SPLYTRCK) > 2) {
                            result = VICTORY_STATE_WON;

                        } else if (WinLoss_CountReadyUnits(team, COMMANDO)) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_LOST;
                        }
                    } break;

                    case CAMPAIGN_MISSION_4:
                    case CAMPAIGN_MISSION_5:
                    case CAMPAIGN_MISSION_9: {
                        if (WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, GREENHSE)) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;

                    case CAMPAIGN_MISSION_8: {
                        if (UnitsManager_TeamInfo[team].casualties[RESEARCH] > 0) {
                            result = VICTORY_STATE_LOST;

                        } else {
                            int alien_mobile_units_count;

                            alien_mobile_units_count = WinLoss_CountReadyUnits(team, ALNPLANE);
                            alien_mobile_units_count += WinLoss_CountReadyUnits(team, ALNTANK);
                            alien_mobile_units_count += WinLoss_CountReadyUnits(team, ALNASGUN);
                            alien_mobile_units_count += WinLoss_CountReadyUnits(team, JUGGRNT);

                            alien_mobile_units_count += WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, ALNPLANE);
                            alien_mobile_units_count += WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, ALNTANK);
                            alien_mobile_units_count += WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, ALNASGUN);
                            alien_mobile_units_count += WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, JUGGRNT);

                            if (alien_mobile_units_count < 4) {
                                result = VICTORY_STATE_LOST;

                            } else if (turn_counter < ini_setting_victory_limit) {
                                result = VICTORY_STATE_PENDING;

                            } else {
                                alien_mobile_units_count = WinLoss_CountReadyUnits(team, ALNPLANE);
                                alien_mobile_units_count += WinLoss_CountReadyUnits(team, ALNTANK);
                                alien_mobile_units_count += WinLoss_CountReadyUnits(team, ALNASGUN);
                                alien_mobile_units_count += WinLoss_CountReadyUnits(team, JUGGRNT);

                                if (alien_mobile_units_count >= 4) {
                                    result = VICTORY_STATE_WON;

                                } else {
                                    result = VICTORY_STATE_LOST;
                                }
                            }
                        }
                    } break;

                    default: {
                        if (ini_setting_victory_type == VICTORY_TYPE_SCORE) {
                            if (UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_points >= ini_setting_victory_limit) {
                                result = VICTORY_STATE_WON;

                            } else if (UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_points >=
                                       ini_setting_victory_limit) {
                                result = VICTORY_STATE_LOST;

                            } else {
                                result = VICTORY_STATE_PENDING;
                            }

                        } else if (turn_counter < ini_setting_victory_limit) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;
                }
            } break;

            case GAME_TYPE_TRAINING: {
                switch (GameManager_GameFileNumber) {
                    case TRAINING_MISSION_1: {
                        if (WinLoss_HasMaterials(team, CARGO_TYPE_RAW) < 1 ||
                            WinLoss_HasMaterials(team, CARGO_TYPE_FUEL) < 1 ||
                            WinLoss_CountReadyUnits(team, ADUMP) <= 0) {
                            if (WinLoss_CountReadyUnits(team, MININGST) && WinLoss_CountReadyUnits(team, ENGINEER)) {
                                result = VICTORY_STATE_PENDING;

                            } else {
                                result = VICTORY_STATE_LOST;
                            }

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;

                    case TRAINING_MISSION_2: {
                        if (WinLoss_CountReadyUnits(team, LIGHTPLT) > 0 &&
                            WinLoss_CountReadyUnits(team, GUNTURRT) > 0) {
                            result = VICTORY_STATE_WON;

                        } else if ((WinLoss_CountReadyUnits(team, ENGINEER) ||
                                    WinLoss_CountReadyUnits(team, GUNTURRT)) &&
                                   (WinLoss_CountReadyUnits(team, CONSTRCT) ||
                                    WinLoss_CountReadyUnits(team, LIGHTPLT))) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_LOST;
                        }
                    } break;

                    case TRAINING_MISSION_3: {
                        if (WinLoss_CountReadyUnits(team, LANDPLT) > 0 && WinLoss_CountReadyUnits(team, SCOUT) > 2) {
                            result = VICTORY_STATE_WON;

                        } else if ((WinLoss_CountReadyUnits(team, LIGHTPLT) > 0 ||
                                    WinLoss_CountReadyUnits(team, SCOUT) > 2) &&
                                   WinLoss_CountReadyUnits(team, MININGST) &&
                                   (WinLoss_CountReadyUnits(team, CONSTRCT) ||
                                    WinLoss_CountReadyUnits(team, LANDPLT))) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_LOST;
                        }
                    } break;

                    case TRAINING_MISSION_4: {
                        int raw_mining_max;
                        int fuel_mining_max;
                        int gold_mining_max;

                        WinLoss_CountTotalMining(team, &raw_mining_max, &fuel_mining_max, &gold_mining_max);

                        if (raw_mining_max < 22) {
                            if (WinLoss_CountReadyUnits(team, CONSTRCT) && WinLoss_CountReadyUnits(team, MININGST)) {
                                result = VICTORY_STATE_PENDING;

                            } else {
                                result = VICTORY_STATE_LOST;
                            }

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;

                    case TRAINING_MISSION_5: {
                        if (WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, MININGST)) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;

                    case TRAINING_MISSION_6: {
                        if (WinLoss_GetTotalUnitsBeingConstructed(team, GUNTURRT) > 0) {
                            result = VICTORY_STATE_WON;

                        } else {
                            result = VICTORY_STATE_PENDING;
                        }
                    } break;

                    case TRAINING_MISSION_7: {
                        if (WinLoss_GetTotalPowerConsumption(team, LIGHTPLT) +
                                    WinLoss_GetTotalPowerConsumption(team, LANDPLT) >
                                1 &&
                            (WinLoss_GetTotalMining(team, CARGO_FUEL) - WinLoss_GetTotalConsumption(team, CARGO_FUEL) >
                             0)) {
                            result = VICTORY_STATE_WON;

                        } else if (WinLoss_CountReadyUnits(team, LIGHTPLT) + WinLoss_CountReadyUnits(team, LANDPLT) >
                                       1 &&
                                   WinLoss_CountReadyUnits(team, MININGST)) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_LOST;
                        }
                    } break;

                    case TRAINING_MISSION_8: {
                        if (WinLoss_HasMaterials(team, CARGO_TYPE_GOLD) > 0) {
                            result = VICTORY_STATE_WON;

                        } else {
                            result = VICTORY_STATE_PENDING;
                        }
                    } break;

                    case TRAINING_MISSION_9: {
                        if (WinLoss_HasAtLeastOneUpgradedTank(team)) {
                            result = VICTORY_STATE_WON;

                        } else {
                            result = VICTORY_STATE_PENDING;
                        }
                    } break;

                    case TRAINING_MISSION_10: {
                        if (UnitsManager_TeamInfo[team].team_points > 0) {
                            result = VICTORY_STATE_WON;

                        } else {
                            result = VICTORY_STATE_PENDING;
                        }
                    } break;

                    case TRAINING_MISSION_11: {
                        if (WinLoss_HasInfiltratorExperience(team)) {
                            result = VICTORY_STATE_WON;

                        } else if (WinLoss_CountReadyUnits(team, TRAINHAL) && WinLoss_CountReadyUnits(team, HABITAT) &&
                                   WinLoss_CountReadyUnits(team, MININGST)) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_LOST;
                        }
                    } break;

                    case TRAINING_MISSION_12: {
                        if (WinLoss_CountReadyUnits(team, LANDMINE) > 0 &&
                            UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].casualties[LANDMINE] > 0) {
                            result = VICTORY_STATE_WON;

                        } else {
                            result = VICTORY_STATE_PENDING;
                        }
                    } break;

                    case TRAINING_MISSION_13: {
                        if (WinLoss_CountReadyUnits(team, TANK) < 3 || WinLoss_CountDamangedUnits(team, TANK)) {
                            if (UnitsManager_TeamInfo[team].casualties[TANK] <= 0) {
                                result = VICTORY_STATE_PENDING;

                            } else {
                                result = VICTORY_STATE_LOST;
                            }

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;

                    case TRAINING_MISSION_14: {
                        if (WinLoss_CountReadyUnits(team, TANK) < 3 || WinLoss_CountUnitsThatUsedAmmo(team, TANK)) {
                            if (UnitsManager_TeamInfo[team].casualties[TANK] <= 0) {
                                result = VICTORY_STATE_PENDING;

                            } else {
                                result = VICTORY_STATE_LOST;
                            }

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;

                    case TRAINING_MISSION_15: {
                        if (UnitsManager_TeamInfo[team].research_topics[RESEARCH_TOPIC_ARMOR].research_level > 0) {
                            result = VICTORY_STATE_WON;

                        } else if (WinLoss_CountReadyUnits(team, RESEARCH)) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_LOST;
                        }
                    } break;

                    default: {
                        result = VICTORY_STATE_GENERIC;
                    } break;
                }
            } break;

            case GAME_TYPE_SCENARIO: {
                switch (GameManager_GameFileNumber) {
                    case SCENARIO_MISSION_9: {
                        if (UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_points <=
                            UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_points) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;

                    case SCENARIO_MISSION_18: {
                        if (WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, RESEARCH)) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;

                    case SCENARIO_MISSION_20: {
                        int red_max_team_points_forecast;
                        int green_max_team_points_forecast;

                        red_max_team_points_forecast =
                            ((ini_setting_victory_limit - turn_counter) * WinLoss_CountReadyUnits(team, GREENHSE)) +
                            UnitsManager_TeamInfo[team].team_points;

                        green_max_team_points_forecast = ((ini_setting_victory_limit - turn_counter) *
                                                          WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, GREENHSE)) +
                                                         UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_points;

                        if (red_max_team_points_forecast >= green_max_team_points_forecast) {
                            result = VICTORY_STATE_GENERIC;

                        } else {
                            result = VICTORY_STATE_LOST;
                        }
                    } break;

                    case SCENARIO_MISSION_24: {
                        if (WinLoss_CountReadyUnits(PLAYER_TEAM_GREEN, GREENHSE)) {
                            result = VICTORY_STATE_PENDING;

                        } else {
                            result = VICTORY_STATE_WON;
                        }
                    } break;

                    default: {
                        result = VICTORY_STATE_GENERIC;
                    } break;
                }
            } break;

            default: {
                result = VICTORY_STATE_GENERIC;
            } break;
        }
    }

    return result;
}
