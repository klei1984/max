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

bool Access_SetUnitDestination(int32_t grid_x, int32_t grid_y, int32_t target_grid_x, int32_t target_grid_y, bool mode);
uint32_t Access_IsAccessible(ResourceID unit_type, uint16_t team, int32_t grid_x, int32_t grid_y, uint32_t flags);
bool Access_FindReachableSpotInt(ResourceID unit_type, UnitInfo *unit, int16_t *grid_x, int16_t *grid_y, int32_t range_limit,
                                 int32_t mode, int32_t direction);
void Access_InitUnitStealthStatus(SmartList<UnitInfo> &units);
void Access_InitStealthMaps();
bool Access_IsSurveyorOverlayActive(UnitInfo *unit);
bool Access_IsWithinScanRange(UnitInfo *unit, int32_t grid_x, int32_t grid_y, int32_t scan_range);
int32_t Access_GetDistance(int32_t grid_x, int32_t grid_y);
int32_t Access_GetDistance(Point position1, Point position2);
int32_t Access_GetDistance(UnitInfo *unit, Point position);
int32_t Access_GetDistance(UnitInfo *unit1, UnitInfo *unit2);
bool Access_IsWithinAttackRange(UnitInfo *unit, int32_t grid_x, int32_t grid_y, int32_t attack_range);
bool Access_FindReachableSpot(ResourceID unit_type, UnitInfo *unit, int16_t *grid_x, int16_t *grid_y, int32_t range,
                              int32_t exclusion_zone, int32_t mode);
uint32_t Access_GetAttackTargetGroup(UnitInfo *unit);
uint32_t Access_UpdateMapStatusAddUnit(UnitInfo *unit, int32_t grid_x, int32_t grid_y);
void Access_UpdateMapStatusRemoveUnit(UnitInfo *unit, int32_t grid_x, int32_t grid_y);
void Access_DrawUnit(UnitInfo *unit);
uint32_t Access_GetTargetClass(UnitInfo *unit);
void Access_UpdateMapStatus(UnitInfo *unit, bool mode);
void Access_UpdateUnitVisibilityStatus(SmartList<UnitInfo> &units);
void Access_UpdateVisibilityStatus(bool all_visible);
void Access_UpdateMinimapFogOfWar(uint16_t team, bool all_visible, bool ignore_team_scan_map = false);
uint8_t Access_GetSurfaceType(int32_t grid_x, int32_t grid_y);
uint8_t Access_GetModifiedSurfaceType(int32_t grid_x, int32_t grid_y);
bool Access_IsAnyLandPresent(int32_t grid_x, int32_t grid_y, uint32_t flags);
bool Access_IsFullyLandCovered(int32_t grid_x, int32_t grid_y, uint32_t flags);
UnitInfo *Access_GetRemovableRubble(uint16_t team, int32_t grid_x, int32_t grid_y);
int32_t Access_FindUnitInUnitList(UnitInfo *unit);
bool Access_IsTeamInUnitList(uint16_t team, SmartList<UnitInfo> &units);
bool Access_IsTeamInUnitLists(uint16_t team);
UnitInfo *Access_GetSelectableUnit(UnitInfo *unit, int32_t grid_x, int32_t grid_y);
UnitInfo *Access_GetFirstMiningStation(uint16_t team);
UnitInfo *Access_GetFirstActiveUnit(uint16_t team, SmartList<UnitInfo> &units);
bool Access_IsWithinMovementRange(UnitInfo *unit);
UnitInfo *Access_SeekNextUnit(uint16_t team, UnitInfo *unit, bool seek_direction);
bool Access_IsChildOfUnitInList(UnitInfo *unit, SmartList<UnitInfo> *list, SmartList<UnitInfo>::Iterator *it);
void Access_RenewAttackOrders(SmartList<UnitInfo> &units, uint16_t team);
void Access_GroupAttackOrder(UnitInfo *unit, bool mode);
int32_t Access_GetStoredUnitCount(UnitInfo *unit);
int32_t Access_GetRepairShopClientCount(uint16_t team, ResourceID unit_type);
void Access_UpdateResourcesTotal(Complex *complex);
void Access_DestroyUtilities(int32_t grid_x, int32_t grid_y, bool remove_slabs, bool remove_rubble, bool remove_connectors,
                             bool remove_road);
void Access_DestroyGroundCovers(int32_t grid_x, int32_t grid_y);
UnitInfo *Access_GetQuickBuilderUnit(int32_t grid_x, int32_t grid_y);
UnitInfo *Access_GetUnit3(int32_t grid_x, int32_t grid_y, uint32_t flags);
UnitInfo *Access_GetUnit1(int32_t grid_x, int32_t grid_y);
UnitInfo *Access_GetUnit2(int32_t grid_x, int32_t grid_y, uint16_t team);
UnitInfo *Access_GetEnemyUnit(uint16_t team, int32_t grid_x, int32_t grid_y, uint32_t flags);
UnitInfo *Access_GetConstructionUtility(uint16_t team, int32_t grid_x, int32_t grid_y);
UnitInfo *Access_GetTeamUnit(int32_t grid_x, int32_t grid_y, uint16_t team, uint32_t flags);
uint32_t Access_GetValidAttackTargetTypes(ResourceID unit_type);
bool Access_IsValidAttackTargetType(ResourceID attacker, ResourceID target);
bool Access_IsValidAttackTarget(ResourceID attacker, ResourceID target, Point point);
bool Access_IsUnitBusyAtLocation(UnitInfo *unit);
bool Access_IsValidAttackTarget(UnitInfo *attacker, UnitInfo *target, Point point);
bool Access_IsValidAttackTarget(UnitInfo *attacker, UnitInfo *target);
UnitInfo *Access_GetAttackTarget(UnitInfo *unit, int32_t grid_x, int32_t grid_y, bool mode = false);
UnitInfo *Access_GetEnemyMineOnSentry(uint16_t team, int32_t grid_x, int32_t grid_y);
UnitInfo *Access_GetAttackTarget2(UnitInfo *unit, int32_t grid_x, int32_t grid_y);
UnitInfo *Access_GetReceiverUnit(UnitInfo *unit, int32_t grid_x, int32_t grid_y);
UnitInfo *Access_GetTeamBuilding(uint16_t team, int32_t grid_x, int32_t grid_y);
void Access_MultiSelect(UnitInfo *unit, Rect *bounds);
bool Access_AreTaskEventsPending();
bool Access_ProcessNewGroupOrder(UnitInfo *unit);
void Access_UpdateMultiSelection(UnitInfo *unit);
bool Access_IsGroupOrderInterrupted(UnitInfo *unit);
bool Access_IsInsideBounds(Rect *bounds, Point *point);

#endif /* ACCESS_HPP */
