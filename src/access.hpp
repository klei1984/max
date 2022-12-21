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

#ifndef ACCESS_HPP
#define ACCESS_HPP

#include "game_manager.hpp"

enum {
    SURFACE_TYPE_NONE = 0x0,
    SURFACE_TYPE_LAND = 0x1,
    SURFACE_TYPE_WATER = 0x2,
    SURFACE_TYPE_COAST = 0x4,
    SURFACE_TYPE_AIR = 0x8,
};

bool Access_SetUnitDestination(int grid_x, int grid_y, int target_grid_x, int target_grid_y, bool mode);
unsigned int Access_IsAccessible(ResourceID unit_type, unsigned short team, int grid_x, int grid_y, unsigned int flags);
bool Access_FindReachableSpotInt(ResourceID unit_type, UnitInfo *unit, short *grid_x, short *grid_y, int range_limit,
                                 int mode, int direction);
void Access_InitUnitStealthStatus(SmartList<UnitInfo> &units);
void Access_InitStealthMaps();
bool Access_IsSurveyorOverlayActive(UnitInfo *unit);
bool Access_IsWithinScanRange(UnitInfo *unit, int grid_x, int grid_y, int scan_range);
int Access_GetDistance(int grid_x, int grid_y);
int Access_GetDistance(Point position1, Point position2);
int Access_GetDistance(UnitInfo *unit, Point position);
int Access_GetDistance(UnitInfo *unit1, UnitInfo *unit2);
bool Access_IsWithinAttackRange(UnitInfo *unit, int grid_x, int grid_y, int attack_range);
bool Access_FindReachableSpot(ResourceID unit_type, UnitInfo *unit, short *grid_x, short *grid_y, int range,
                              int exclusion_zone, int mode);
unsigned int Access_GetAttackTargetGroup(UnitInfo *unit);
unsigned int Access_UpdateMapStatusAddUnit(UnitInfo *unit, int grid_x, int grid_y);
void Access_UpdateMapStatusRemoveUnit(UnitInfo *unit, int grid_x, int grid_y);
void Access_DrawUnit(UnitInfo *unit);
unsigned int Access_GetVelocity(UnitInfo *unit);
void Access_UpdateMapStatus(UnitInfo *unit, bool mode);
void Access_UpdateUnitVisibilityStatus(SmartList<UnitInfo> &units);
void Access_UpdateVisibilityStatus(bool all_visible);
void Access_UpdateMinimapFogOfWar(unsigned short team, bool all_visible, bool ignore_team_scan_map = false);
unsigned char Access_GetSurfaceType(int grid_x, int grid_y);
unsigned char Access_GetModifiedSurfaceType(int grid_x, int grid_y);
bool Access_IsAnyLandPresent(int grid_x, int grid_y, unsigned int flags);
bool Access_IsFullyLandCovered(int grid_x, int grid_y, unsigned int flags);
UnitInfo *Access_GetUnit8(unsigned short team, int grid_x, int grid_y);
int Access_FindUnitInUnitList(UnitInfo *unit);
bool Access_IsTeamInUnitList(unsigned short team, SmartList<UnitInfo> &units);
bool Access_IsTeamInUnitLists(unsigned short team);
UnitInfo *Access_GetSelectableUnit(UnitInfo *unit, int grid_x, int grid_y);
UnitInfo *Access_GetFirstMiningStation(unsigned short team);
UnitInfo *Access_GetFirstActiveUnit(unsigned short team, SmartList<UnitInfo> &units);
bool Access_IsWithinMovementRange(UnitInfo *unit);
UnitInfo *Access_SeekNextUnit(unsigned short team, UnitInfo *unit, bool seek_direction);
bool Access_IsChildOfUnitInList(UnitInfo *unit, SmartList<UnitInfo>::Iterator *it);
void Access_RenewAttackOrders(SmartList<UnitInfo> &units, unsigned short team);
void Access_GroupAttackOrder(UnitInfo *unit, bool mode);
int Access_GetStoredUnitCount(UnitInfo *unit);
void Access_UpdateResourcesTotal(Complex *complex);
void Access_DestroyUtilities(int grid_x, int grid_y, bool remove_slabs, bool remove_rubble, bool remove_connectors,
                             bool remove_road);
void Access_DestroyGroundCovers(int grid_x, int grid_y);
UnitInfo *Access_GetUnit5(int grid_x, int grid_y);
UnitInfo *Access_GetUnit3(int grid_x, int grid_y, unsigned int flags);
UnitInfo *Access_GetUnit1(int grid_x, int grid_y);
UnitInfo *Access_GetUnit2(int grid_x, int grid_y, unsigned short team);
UnitInfo *Access_GetUnit6(unsigned short team, int grid_x, int grid_y, unsigned int flags);
UnitInfo *Access_GetUnit7(unsigned short team, int grid_x, int grid_y);
UnitInfo *Access_GetUnit4(int grid_x, int grid_y, unsigned short team, unsigned int flags);
unsigned int Access_GetValidAttackTargetTypes(ResourceID unit_type);
bool Access_IsValidAttackTargetType(ResourceID attacker, ResourceID target);
bool Access_IsValidAttackTarget(ResourceID attacker, ResourceID target, Point point);
bool Access_IsUnitBusyAtLocation(UnitInfo *unit);
bool Access_IsValidAttackTarget(UnitInfo *attacker, UnitInfo *target, Point point);
bool Access_IsValidAttackTarget(UnitInfo *attacker, UnitInfo *target);
UnitInfo *Access_GetAttackTarget(UnitInfo *unit, int grid_x, int grid_y, bool mode = false);
UnitInfo *Access_GetEnemyMineOnSentry(unsigned short team, int grid_x, int grid_y);
UnitInfo *Access_GetAttackTarget2(UnitInfo *unit, int grid_x, int grid_y);
UnitInfo *Access_GetReceiverUnit(UnitInfo *unit, int grid_x, int grid_y);
UnitInfo *Access_GetTeamBuilding(unsigned short team, int grid_x, int grid_y);
void Access_MultiSelect(UnitInfo *unit, Rect *bounds);
bool Access_AreTaskEventsPending();
bool Access_ProcessNewGroupOrder(UnitInfo *unit);
void Access_UpdateMultiSelection(UnitInfo *unit);
bool Access_IsGroupOrderInterrupted(UnitInfo *unit);
bool Access_IsInsideBounds(Rect *bounds, Point *point);

#endif /* ACCESS_HPP */
