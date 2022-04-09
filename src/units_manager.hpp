/* Copyright (c) 2021 M.A.X. Port Team
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

#ifndef UNITS_MANAGER_HPP
#define UNITS_MANAGER_HPP

#include "ctinfo.hpp"
#include "teammissionsupplies.hpp"
#include "teamunits.hpp"
#include "unitinfo.hpp"

extern SmartList<UnitInfo> UnitsManager_GroundCoverUnits;
extern SmartList<UnitInfo> UnitsManager_MobileLandSeaUnits;
extern SmartList<UnitInfo> UnitsManager_ParticleUnits;
extern SmartList<UnitInfo> UnitsManager_StationaryUnits;
extern SmartList<UnitInfo> UnitsManager_MobileAirUnits;
extern SmartList<UnitInfo> UnitsManager_UnitList6;

extern AbstractUnit UnitsManager_AbstractUnits[UNIT_END];
extern BaseUnit UnitsManager_BaseUnits[UNIT_END];

extern CTInfo UnitsManager_TeamInfo[5];

extern TeamMissionSupplies UnitsManager_TeamMissionSupplies[5];

int UnitsManager_CalculateAttackDamage(UnitInfo* attacker_unit, UnitInfo* target_unit, int damage_potential);
UnitValues* UnitsManager_GetCurrentUnitValues(CTInfo* team_info, ResourceID unit_type);
void UnitsManager_AddAxisMissionLoadout(unsigned short team, SmartObjectArray<ResourceID> units);
int UnitsManager_AddDefaultMissionLoadout(unsigned short team);

#endif /* UNITS_MANAGER_HPP */
