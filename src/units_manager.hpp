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

struct PopupFunctions {
    void (*init)(UnitInfo* unit, struct PopupButtons* buttons);
    bool (*test)(UnitInfo* unit);
};

extern unsigned short UnitsManager_Team;

extern SmartList<UnitInfo> UnitsManager_GroundCoverUnits;
extern SmartList<UnitInfo> UnitsManager_MobileLandSeaUnits;
extern SmartList<UnitInfo> UnitsManager_ParticleUnits;
extern SmartList<UnitInfo> UnitsManager_StationaryUnits;
extern SmartList<UnitInfo> UnitsManager_MobileAirUnits;
extern SmartList<UnitInfo> UnitsManager_UnitList6;

extern AbstractUnit UnitsManager_AbstractUnits[UNIT_END];
extern BaseUnit UnitsManager_BaseUnits[UNIT_END];

extern SmartPointer<UnitInfo> UnitsManager_Unit;

extern SmartList<UnitInfo> UnitsManager_DelayedAttackTargets[PLAYER_TEAM_MAX];

extern bool UnitsManager_OrdersPending;

extern CTInfo UnitsManager_TeamInfo[PLAYER_TEAM_MAX];

extern struct PopupFunctions UnitsManager_PopupCallbacks[];

extern TeamMissionSupplies UnitsManager_TeamMissionSupplies[PLAYER_TEAM_MAX];

void UnitsManager_PerformAction(UnitInfo* unit);
int UnitsManager_CalculateAttackDamage(UnitInfo* attacker_unit, UnitInfo* target_unit, int damage_potential);
UnitValues* UnitsManager_GetCurrentUnitValues(CTInfo* team_info, ResourceID unit_type);
void UnitsManager_AddAxisMissionLoadout(unsigned short team, SmartObjectArray<ResourceID> units);
int UnitsManager_AddDefaultMissionLoadout(unsigned short team);
bool UnitsManager_IsMasterBuilderPlaceable(UnitInfo* unit, int grid_x, int grid_y);
void UnitsManager_InitPopupMenus();
int UnitsManager_GetStealthChancePercentage(UnitInfo* unit1, UnitInfo* unit2, int order);
SmartPointer<UnitInfo> UnitsManager_SpawnUnit(ResourceID unit_type, unsigned short team, int grid_x, int grid_y,
                                              UnitInfo* parent);
void UnitsManager_ProcessRemoteOrders();
void UnitsManager_NewOrderWhileScaling(UnitInfo* unit);
void UnitsManager_CheckIfUnitDestroyed(UnitInfo* unit);
void UnitsManager_SetNewOrderInt(UnitInfo* unit, int order, int state);
void UnitsManager_UpdatePathsTimeLimit();
void UnitsManager_SetNewOrder(UnitInfo* unit, int order, int state);
void UnitsManager_MoveUnit(UnitInfo* unit, int grid_x, int grid_y);
unsigned int UnitsManager_MoveUnitAndParent(UnitInfo* unit, int grid_x, int grid_y);
void UnitsManager_SetInitialMining(UnitInfo* unit, int grid_x, int grid_y);
void UnitsManager_StartBuild(UnitInfo* unit);
bool UnitsManager_IsUnitUnderWater(UnitInfo* unit);
void UnitsManager_UpdateConnectors(UnitInfo* unit);
void UnitsManager_DestroyUnit(UnitInfo* unit);
SmartPointer<UnitInfo> UnitsManager_DeployUnit(ResourceID unit_type, unsigned short team, Complex* complex, int grid_x,
                                               int grid_y, unsigned char unit_angle, bool is_existing_unit = false,
                                               bool skip_map_status_update = false);
void UnitsManager_RemoveConnections(UnitInfo* unit);
int UnitsManager_GetTargetAngle(int distance_x, int distance_y);
void UnitsManager_AddToDelayedReactionList(UnitInfo* unit);
void UnitsManager_DrawBustedCommando(UnitInfo* unit);
void UnitsManager_TestBustedCommando(UnitInfo* unit);
void UnitsManager_ScaleUnit(UnitInfo* unit, int state);
int UnitsManager_GetAttackDamage(UnitInfo* unit1, UnitInfo* unit2, int attack_potential);

#endif /* UNITS_MANAGER_HPP */
