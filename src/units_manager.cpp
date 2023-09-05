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

#include "units_manager.hpp"

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "allocmenu.hpp"
#include "builder.hpp"
#include "buildmenu.hpp"
#include "cursor.hpp"
#include "drawmap.hpp"
#include "hash.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "message_manager.hpp"
#include "paths_manager.hpp"
#include "production_manager.hpp"
#include "remote.hpp"
#include "repairshopmenu.hpp"
#include "researchmenu.hpp"
#include "resource_manager.hpp"
#include "smartlist.hpp"
#include "sound_manager.hpp"
#include "survey.hpp"
#include "teamunits.hpp"
#include "unitevents.hpp"
#include "unitinfo.hpp"
#include "upgrademenu.hpp"
#include "window.hpp"
#include "window_manager.hpp"

#define POPUP_MENU_TYPE_COUNT 23

static bool UnitsManager_SelfDestructActiveMenu(WindowInfo* window);
static bool UnitsManager_SelfDestructMenu();
static void UnitsManager_UpdateUnitPosition(UnitInfo* unit, int32_t grid_x, int32_t grid_y);
static void UnitsManager_UpdateMapHash(UnitInfo* unit, int32_t grid_x, int32_t grid_y);
static void UnitsManager_RemoveUnitFromUnitLists(UnitInfo* unit);
static void UnitsManager_RegisterButton(PopupButtons* buttons, bool state, const char* caption, char position,
                                        void (*event_handler)(ButtonID bid, UnitInfo* unit));

static void UnitsManager_Popup_OnClick_Done(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_Sentry(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_Upgrade(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_UpgradeAll(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_Enter(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_Remove(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_Stop(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_Transfer(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_Manual(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitCommons(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_OnClick_Auto(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitSurveyor(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_InitRecreationCenter(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_InitEcoSphere(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_InitPowerGenerators(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_InitFuelSuppliers(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_OnClick_Build(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_PlaceNewUnit(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_StopBuild(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_StartBuild(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitFactories(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_OnClick_TargetingMode(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitMilitary(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_OnClick_Reload(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitReloaders(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_InitMineLayers(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_InitRepair(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_OnClick_BuildStop(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitBuilders(UnitInfo* unit, struct PopupButtons* buttons);
static bool UnitsManager_Popup_IsUnitReady(UnitInfo* unit);
static void UnitsManager_Popup_OnClick_StopRemove(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitRemove(UnitInfo* unit, struct PopupButtons* buttons);
static bool UnitsManager_IsPowerGeneratorPlaceable(uint16_t team, int16_t* grid_x, int16_t* grid_y);
static void UnitsManager_Popup_OnClick_StartMasterBuilder(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitMaster(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_OnClick_ActivateNonAirUnit(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_ActivateAirUnit(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_Activate(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_Load(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitRepairShops(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_OnClick_PowerOnAllocate(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitMiningStation(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_OnClick_BuyUpgrade(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitGoldRefinery(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_OnClick_Research(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitResearch(UnitInfo* unit, struct PopupButtons* buttons);
static void UnitsManager_Popup_OnClick_PowerOn(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_PowerOff(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_Steal(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_OnClick_Disable(ButtonID bid, UnitInfo* unit);
static void UnitsManager_Popup_InitInfiltrator(UnitInfo* unit, struct PopupButtons* buttons);
static bool UnitsManager_Popup_IsNever(UnitInfo* unit);
static bool UnitsManager_Popup_IsReady(UnitInfo* unit);

static void UnitsManager_ClearPins(SmartList<UnitInfo>* units);
static void UnitsManager_ProcessOrder(UnitInfo* unit);
static void UnitsManager_ProcessOrders(SmartList<UnitInfo>* units);
static void UnitsManager_FinishUnitScaling(UnitInfo* unit);
static void UnitsManager_ActivateEngineer(UnitInfo* unit);
static void UnitsManager_DeployMasterBuilderInit(UnitInfo* unit);
static bool UnitsManager_IsFactory(ResourceID unit_type);
static void UnitsManager_ClearDelayedReaction(SmartList<UnitInfo>* units);
static void UnitsManager_ClearDelayedReactions();
static void UnitsManager_Landing(UnitInfo* unit);
static void UnitsManager_Loading(UnitInfo* unit);
static void UnitsManager_Unloading(UnitInfo* unit);
static void UnitsManager_PerformAutoSurvey(UnitInfo* unit);
static void UnitsManager_PowerUpUnit(UnitInfo* unit, int32_t factor);
static void UnitsManager_PowerDownUnit(UnitInfo* unit);
static void UnitsManager_Animate(UnitInfo* unit);
static void UnitsManager_DeployMasterBuilder(UnitInfo* unit);
static bool UnitsManager_PursueEnemy(UnitInfo* unit);
static bool UnitsManager_UpdateAttackMoves(UnitInfo* unit);
static void UnitsManager_Store(UnitInfo* unit);
static bool UnitsManager_BeginFire(UnitInfo* unit);
static void UnitsManager_BuildClearing(UnitInfo* unit, bool mode);
static void UnitsManager_BuildNext(UnitInfo* unit);
static void UnitsManager_ActivateUnit(UnitInfo* unit);
static void UnitsManager_StartExplosion(UnitInfo* unit);
static void UnitsManager_ProgressExplosion(UnitInfo* unit);
static void UnitsManager_ProgressUnloading(UnitInfo* unit);
static void UnitsManager_StartClearing(UnitInfo* unit);
static void UnitsManager_ProgressLoading(UnitInfo* unit);
static void UnitsManager_BuildingReady(UnitInfo* unit);
static void UnitsManager_Repair(UnitInfo* unit);
static void UnitsManager_Transfer(UnitInfo* unit);
static bool UnitsManager_AttemptStealthAction(UnitInfo* unit);
static void UnitsManager_CaptureUnit(UnitInfo* unit);
static void UnitsManager_DisableUnit(UnitInfo* unit);
static bool UnitsManager_AssessAttacks();
static bool UnitsManager_IsTeamReactionPending(uint16_t team, UnitInfo* unit, SmartList<UnitInfo>* units);
static bool UnitsManager_ShouldAttack(UnitInfo* unit1, UnitInfo* unit2);
static bool UnitsManager_CheckReaction(UnitInfo* unit1, UnitInfo* unit2);
static bool UnitsManager_IsReactionPending(SmartList<UnitInfo>* units, UnitInfo* unit);
static AirPath* UnitsManager_GetMissilePath(UnitInfo* unit);
static void UnitsManager_UpdateAttackPaths(UnitInfo* unit);
static bool UnitsManager_IsAttackScheduled();
static Point UnitsManager_GetAttackPosition(UnitInfo* unit1, UnitInfo* unit2);
static bool UnitsManager_CheckDelayedReactions(uint16_t team);

static void UnitsManager_ProcessOrderAwait(UnitInfo* unit);
static void UnitsManager_ProcessOrderTransform(UnitInfo* unit);
static void UnitsManager_ProcessOrderMove(UnitInfo* unit);
static void UnitsManager_ProcessOrderFire(UnitInfo* unit);
static void UnitsManager_ProcessOrderBuild(UnitInfo* unit);
static void UnitsManager_ProcessOrderActivate(UnitInfo* unit);
static void UnitsManager_ProcessOrderNewAllocate(UnitInfo* unit);
static void UnitsManager_ProcessOrderPowerOn(UnitInfo* unit);
static void UnitsManager_ProcessOrderPowerOff(UnitInfo* unit);
static void UnitsManager_ProcessOrderExplode(UnitInfo* unit);
static void UnitsManager_ProcessOrderUnload(UnitInfo* unit);
static void UnitsManager_ProcessOrderClear(UnitInfo* unit);
static void UnitsManager_ProcessOrderSentry(UnitInfo* unit);
static void UnitsManager_ProcessOrderLand(UnitInfo* unit);
static void UnitsManager_ProcessOrderTakeOff(UnitInfo* unit);
static void UnitsManager_ProcessOrderLoad(UnitInfo* unit);
static void UnitsManager_ProcessOrderIdle(UnitInfo* unit);
static void UnitsManager_ProcessOrderRepair(UnitInfo* unit);
static void UnitsManager_ProcessOrderRefuel(UnitInfo* unit);
static void UnitsManager_ProcessOrderReload(UnitInfo* unit);
static void UnitsManager_ProcessOrderTransfer(UnitInfo* unit);
static void UnitsManager_ProcessOrderHaltBuilding(UnitInfo* unit);
static void UnitsManager_ProcessOrderAwaitScaling(UnitInfo* unit);
static void UnitsManager_ProcessOrderAwaitTapePositioning(UnitInfo* unit);
static void UnitsManager_ProcessOrderAwaitDisableStealUnit(UnitInfo* unit);
static void UnitsManager_ProcessOrderDisable(UnitInfo* unit);
static void UnitsManager_ProcessOrderUpgrade(UnitInfo* unit);
static void UnitsManager_ProcessOrderLayMine(UnitInfo* unit);

uint16_t UnitsManager_Team;

uint32_t UnitsManager_UnknownCounter;

SmartList<UnitInfo> UnitsManager_GroundCoverUnits;
SmartList<UnitInfo> UnitsManager_MobileLandSeaUnits;
SmartList<UnitInfo> UnitsManager_ParticleUnits;
SmartList<UnitInfo> UnitsManager_StationaryUnits;
SmartList<UnitInfo> UnitsManager_MobileAirUnits;
SmartList<UnitInfo> UnitsManager_UnitList6;

BaseUnit UnitsManager_BaseUnits[UNIT_END];

SmartPointer<UnitInfo> UnitsManager_Unit;

SmartPointer<UnitInfo> UnitsManager_Units[PLAYER_TEAM_MAX];

SmartList<UnitInfo> UnitsManager_DelayedAttackTargets[PLAYER_TEAM_MAX];

bool UnitsManager_OrdersPending;
bool UnitsManager_byte_179448;
bool UnitsManager_byte_178170;

bool UnitsManager_TimeBenchmarkInit;
uint8_t UnitsManager_TimeBenchmarkNextIndex;
uint8_t UnitsManager_TimeBenchmarkIndices[20];
int32_t UnitsManager_TimeBenchmarkValues[20];

int8_t UnitsManager_EffectCounter;
int8_t UnitsManager_byte_17947D;

const char* const UnitsManager_Orders[] = {
    "Awaiting",   "Transforming", "Moving",    "Firing",          "Building",  "Activate Order", "New Allocate Order",
    "Power On",   "Power Off",    "Exploding", "Unloading",       "Clearing",  "Sentry",         "Landing",
    "Taking Off", "Loading",      "Idle",      "Repairing",       "Refueling", "Reloading",      "Transferring",
    "Awaiting",   "Awaiting",     "Awaiting",  "Awaiting",        "Awaiting",  "Disabled",       "Moving",
    "Repairing",  "Transferring", "Attacking", "Building Halted",
};

AbstractUnit UnitsManager_AbstractUnits[UNIT_END] = {
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ COMMTWR,
        /* shadow          */ S_COMMTW,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_COMMT,
        /* portrait        */ P_TRANSP,
        /* icon            */ I_TRANSP,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(79a5),
        /* plural name     */ _(08ad),
        /* description */
        _(9ed6)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ POWERSTN,
        /* shadow          */ S_POWERS,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_POWERS,
        /* portrait        */ P_POWSTN,
        /* icon            */ I_POWSTN,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(1c9b),
        /* plural name     */ _(1522),
        /* description */
        _(1a05)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | STANDALONE | REQUIRES_SLAB,
        /* sprite          */ POWGEN,
        /* shadow          */ S_POWGEN,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_POWGEN,
        /* portrait        */ P_POWGEN,
        /* icon            */ I_POWGEN,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(bbf4),
        /* plural name     */ _(7084),
        /* description */
        _(7ed6),
        /* tutorial description (optional) */
        _(fbcd)),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ BARRACKS,
        /* shadow          */ S_BARRAC,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_BARRAC,
        /* portrait        */ P_BARRCK,
        /* icon            */ I_BARRCK,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ 4,
        /* gender          */ 'N',
        /* singular name   */ _(c8d3),
        /* plural name     */ _(10c9),
        /* description */
        _(5a81)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | REQUIRES_SLAB,
        /* sprite          */ SHIELDGN,
        /* shadow          */ S_SHIELD,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_SHIELD,
        /* portrait        */ P_SHIELD,
        /* icon            */ I_SHIELD,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(5ea2),
        /* plural name     */ _(5a91),
        /* description */
        _(4582)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ ANIMATED | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | STANDALONE |
            REQUIRES_SLAB,
        /* sprite          */ RADAR,
        /* shadow          */ S_RADAR,
        /* data            */ D_RADAR,
        /* flics animation */ F_RADAR,
        /* portrait        */ P_RADAR,
        /* icon            */ I_RADAR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(524f),
        /* plural name     */ _(3184),
        /* description */
        _(a5d2)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | SELECTABLE | STANDALONE | REQUIRES_SLAB,
        /* sprite          */ ADUMP,
        /* shadow          */ S_ADUMP,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_ADUMP,
        /* portrait        */ P_SMSTOR,
        /* icon            */ I_SMSTOR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ _(c2d8),
        /* plural name     */ _(5053),
        /* description */
        _(53b3),
        /* tutorial description (optional) */
        _(656d)),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | SELECTABLE | STANDALONE | REQUIRES_SLAB,
        /* sprite          */ FDUMP,
        /* shadow          */ S_FDUMP,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_FDUMP,
        /* portrait        */ P_SMFUEL,
        /* icon            */ I_SMFUEL,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ FUEL,
        /* gender          */ 'N',
        /* singular name   */ _(93dc),
        /* plural name     */ _(51d9),
        /* description */
        _(e1a5),
        /* tutorial description (optional) */
        _(75d4)),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | SELECTABLE | STANDALONE | REQUIRES_SLAB,
        /* sprite          */ GOLDSM,
        /* shadow          */ S_GOLDSM,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_GOLDSM,
        /* portrait        */ P_SMVLT,
        /* icon            */ I_SMVLT,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ GOLD,
        /* gender          */ 'N',
        /* singular name   */ _(282f),
        /* plural name     */ _(4340),
        /* description */
        _(760c),
        /* tutorial description (optional) */
        _(8672)),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ DEPOT,
        /* shadow          */ S_DEPOT,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_DEPOT,
        /* portrait        */ P_DEPOT,
        /* icon            */ I_DEPOT,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ 4,
        /* gender          */ 'N',
        /* singular name   */ _(f830),
        /* plural name     */ _(e8a8),
        /* description */
        _(a135),
        /* tutorial description (optional) */
        _(731e)),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ HANGAR,
        /* shadow          */ S_HANGAR,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_HANGAR,
        /* portrait        */ P_HANGAR,
        /* icon            */ I_HANGAR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ 6,
        /* gender          */ 'N',
        /* singular name   */ _(66ad),
        /* plural name     */ _(43a4),
        /* description */
        _(42ec)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | SELECTABLE,
        /* sprite          */ DOCK,
        /* shadow          */ S_DOCK,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_DOCK,
        /* portrait        */ P_DOCK,
        /* icon            */ I_DOCK,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER,
        /* cargo type      */ 5,
        /* gender          */ 'N',
        /* singular name   */ _(49c1),
        /* plural name     */ _(5d61),
        /* description */
        _(1c3b)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ CONNECTOR_UNIT | STATIONARY | UPGRADABLE | SELECTABLE,
        /* sprite          */ CNCT_4W,
        /* shadow          */ S_CNCT4W,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_CNCT4W,
        /* portrait        */ P_CONNEC,
        /* icon            */ I_CONNEC,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ 0,
        /* gender          */ 'N',
        /* singular name   */ _(8f0d),
        /* plural name     */ _(c3d2),
        /* description */
        _(c971),
        /* tutorial description (optional) */
        _(9736)),
    AbstractUnit(
        /* flags           */ GROUND_COVER | BUILDING | STATIONARY,
        /* sprite          */ LRGRUBLE,
        /* shadow          */ S_LRGRBL,
        /* data            */ D_LRGRBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Large Rubble 1",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY,
        /* sprite          */ SMLRUBLE,
        /* shadow          */ S_SMLRBL,
        /* data            */ D_SMLRBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Small Rubble 1",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | BUILDING | STATIONARY | SELECTABLE,
        /* sprite          */ LRGTAPE,
        /* shadow          */ INVALID_ID,
        /* data            */ D_LRGRBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Large tape",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | SELECTABLE,
        /* sprite          */ SMLTAPE,
        /* shadow          */ INVALID_ID,
        /* data            */ D_LRGRBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Small tape",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | BUILDING | STATIONARY,
        /* sprite          */ LRGSLAB,
        /* shadow          */ S_LRGSLA,
        /* data            */ D_LRGSLA,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Large Slab",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY,
        /* sprite          */ SMLSLAB,
        /* shadow          */ S_SMLSLA,
        /* data            */ D_DEFALT,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Small Slab",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | BUILDING | STATIONARY,
        /* sprite          */ LRGCONES,
        /* shadow          */ S_LRGCON,
        /* data            */ D_DEFALT,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Large cones",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY,
        /* sprite          */ SMLCONES,
        /* shadow          */ S_SMLCON,
        /* data            */ D_DEFALT,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Small cones",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | SELECTABLE,
        /* sprite          */ ROAD,
        /* shadow          */ S_ROAD,
        /* data            */ D_DEFALT,
        /* flics animation */ F_ROAD,
        /* portrait        */ P_ROAD,
        /* icon            */ I_ROAD,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(a22e),
        /* plural name     */ _(02e7),
        /* description */
        _(4d4a)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | SELECTABLE,
        /* sprite          */ LANDPAD,
        /* shadow          */ S_LANDPA,
        /* data            */ D_DEFALT,
        /* flics animation */ F_LANDPA,
        /* portrait        */ P_LANDPD,
        /* icon            */ I_LANDPD,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ 6,
        /* gender          */ 'N',
        /* singular name   */ _(be71),
        /* plural name     */ _(b393),
        /* description */
        _(2627)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT | SELECTABLE,
        /* sprite          */ SHIPYARD,
        /* shadow          */ S_SHIPYA,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_SHIPYA,
        /* portrait        */ P_SHIPYD,
        /* icon            */ I_SHIPYD,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(bc7b),
        /* plural name     */ _(c323),
        /* description */
        _(29b9)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT | SELECTABLE |
            REQUIRES_SLAB,
        /* sprite          */ LIGHTPLT,
        /* shadow          */ S_LIGHTP,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_LIGHTP,
        /* portrait        */ P_LGHTPL,
        /* icon            */ I_LGHTPL,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(eebb),
        /* plural name     */ _(27ca),
        /* description */
        _(0c63),
        /* tutorial description (optional) */
        _(381d)),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT | SELECTABLE |
            REQUIRES_SLAB,
        /* sprite          */ LANDPLT,
        /* shadow          */ S_LANDPL,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_LANDPL,
        /* portrait        */ P_HVYPLT,
        /* icon            */ I_HVYPLT,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(ca88),
        /* plural name     */ _(43e4),
        /* description */
        _(e9d2),
        /* tutorial description (optional) */
        _(70ae)),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | REQUIRES_SLAB,
        /* sprite          */ SUPRTPLT,
        /* shadow          */ S_SUPRTP,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_SUPRTP,
        /* portrait        */ P_LIFESP,
        /* icon            */ I_LIFESP,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT | SELECTABLE |
            REQUIRES_SLAB,
        /* sprite          */ AIRPLT,
        /* shadow          */ S_AIRPLT,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_AIRPLT,
        /* portrait        */ P_AIRPLT,
        /* icon            */ I_AIRPLT,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(751e),
        /* plural name     */ _(331b),
        /* description */
        _(784f)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ HABITAT,
        /* shadow          */ S_HABITA,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_HABITA,
        /* portrait        */ P_HABITA,
        /* icon            */ I_HABITA,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(97e9),
        /* plural name     */ _(c2e2),
        /* description */
        _(73ae)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ RESEARCH,
        /* shadow          */ S_RESEAR,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_RESEAR,
        /* portrait        */ P_RESEAR,
        /* icon            */ I_RESEAR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(e531),
        /* plural name     */ _(e103),
        /* description */
        _(fa10)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ GREENHSE,
        /* shadow          */ S_GREENH,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_GREENH,
        /* portrait        */ P_GREENH,
        /* icon            */ I_GREENH,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(5972),
        /* plural name     */ _(4b34),
        /* description */
        _(6182)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | REQUIRES_SLAB,
        /* sprite          */ RECCENTR,
        /* shadow          */ S_RECCEN,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_RECCEN,
        /* portrait        */ P_RECCTR,
        /* icon            */ I_RECCTR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(79b5),
        /* plural name     */ _(6006),
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ TRAINHAL,
        /* shadow          */ S_TRAINH,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_TRAINH,
        /* portrait        */ P_TRNHLL,
        /* icon            */ I_TRNHLL,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(30c9),
        /* plural name     */ _(aaad),
        /* description */
        _(c11b)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | UPGRADABLE | SELECTABLE,
        /* sprite          */ WTRPLTFM,
        /* shadow          */ S_WTRPLT,
        /* data            */ D_DEFALT,
        /* flics animation */ F_WTRPLT,
        /* portrait        */ P_WATER,
        /* icon            */ I_WATER,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(9a30),
        /* plural name     */ _(cbf4),
        /* description */
        _(75fb)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE | STANDALONE |
            REQUIRES_SLAB | TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ GUNTURRT,
        /* shadow          */ S_GUNTUR,
        /* data            */ D_FIXED,
        /* flics animation */ F_GUNTUR,
        /* portrait        */ P_GUNTUR,
        /* icon            */ I_GUNTUR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(e1c0),
        /* plural name     */ _(5cd9),
        /* description */
        _(6fae),
        /* tutorial description (optional) */
        _(0fc3)),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE | STANDALONE |
            REQUIRES_SLAB | TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ ANTIAIR,
        /* shadow          */ S_ANTIAI,
        /* data            */ D_ANTIAI,
        /* flics animation */ F_ANTIAI,
        /* portrait        */ P_FXAA,
        /* icon            */ I_FXAA,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(4cb4),
        /* plural name     */ _(3764),
        /* description */
        _(5282)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE | STANDALONE |
            REQUIRES_SLAB | TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ ARTYTRRT,
        /* shadow          */ S_ARTYTR,
        /* data            */ D_FIXED,
        /* flics animation */ F_ARTYTR,
        /* portrait        */ P_ARTYTR,
        /* icon            */ I_ARTYTR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(6689),
        /* plural name     */ _(64b7),
        /* description */
        _(96b3)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | STANDALONE | REQUIRES_SLAB | TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ ANTIMSSL,
        /* shadow          */ S_ANTIMS,
        /* data            */ D_FIXED,
        /* flics animation */ F_ANTIMS,
        /* portrait        */ P_FXROCK,
        /* icon            */ I_FXROCK,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(f5c8),
        /* plural name     */ _(24cb),
        /* description */
        _(d8d2)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | SELECTABLE | STANDALONE,
        /* sprite          */ BLOCK,
        /* shadow          */ S_BLOCK,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_BLOCK,
        /* portrait        */ P_BLOCK,
        /* icon            */ I_BLOCK,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(65a8),
        /* plural name     */ _(703f),
        /* description */
        _(c75e)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | UPGRADABLE | SELECTABLE,
        /* sprite          */ BRIDGE,
        /* shadow          */ S_BRIDGE,
        /* data            */ D_BRIDGE,
        /* flics animation */ F_BRIDGE,
        /* portrait        */ P_BRIDGE,
        /* icon            */ I_BRIDGE,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(3892),
        /* plural name     */ _(d982),
        /* description */
        _(0886)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ MININGST,
        /* shadow          */ S_MINING,
        /* data            */ D_MINING,
        /* flics animation */ F_MINING,
        /* portrait        */ P_MINING,
        /* icon            */ I_MINING,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ _(72a5),
        /* plural name     */ _(b41b),
        /* description */
        _(0777),
        /* tutorial description (optional) */
        _(5a0e)),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | UPGRADABLE | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ LANDMINE,
        /* shadow          */ S_LANDMI,
        /* data            */ D_DEFALT,
        /* flics animation */ F_LANDMI,
        /* portrait        */ P_LANDMN,
        /* icon            */ I_LANDMN,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(a2c3),
        /* plural name     */ _(ee79),
        /* description */
        _(3b31)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | UPGRADABLE | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ SEAMINE,
        /* shadow          */ INVALID_ID,
        /* data            */ D_DEFALT,
        /* flics animation */ F_SEAMIN,
        /* portrait        */ P_SEAMIN,
        /* icon            */ I_SEAMIN,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(a309),
        /* plural name     */ _(e7f8),
        /* description */
        _(470d)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | STATIONARY,
        /* sprite          */ LNDEXPLD,
        /* shadow          */ INVALID_ID,
        /* data            */ D_UEXPLD,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "explosion",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | STATIONARY,
        /* sprite          */ AIREXPLD,
        /* shadow          */ INVALID_ID,
        /* data            */ D_UEXPLD,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "explosion",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | STATIONARY,
        /* sprite          */ SEAEXPLD,
        /* shadow          */ INVALID_ID,
        /* data            */ D_UEXPLD,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "explosion",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | STATIONARY,
        /* sprite          */ BLDEXPLD,
        /* shadow          */ INVALID_ID,
        /* data            */ D_BEXPLD,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "explosion",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | STATIONARY,
        /* sprite          */ HITEXPLD,
        /* shadow          */ INVALID_ID,
        /* data            */ D_SEXPLD,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "explosion",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ MASTER,
        /* shadow          */ S_MASTER,
        /* data            */ D_MOBILE,
        /* flics animation */ F_MASTER,
        /* portrait        */ P_MASTER,
        /* icon            */ I_MASTER,
        /* armory portrait */ A_MASTER,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(4ef8),
        /* plural name     */ _(603b),
        /* description */
        _(b66a)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ CONSTRCT,
        /* shadow          */ S_CONSTR,
        /* data            */ D_AMPHIB,
        /* flics animation */ F_CONSTR,
        /* portrait        */ P_CONTRC,
        /* icon            */ I_CONTRC,
        /* armory portrait */ A_CONTRC,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ _(7a39),
        /* plural name     */ _(3b10),
        /* description */
        _(d28e),
        /* tutorial description (optional) */
        _(aafe)),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ SCOUT,
        /* shadow          */ S_SCOUT,
        /* data            */ D_AMPHIB,
        /* flics animation */ F_SCOUT,
        /* portrait        */ P_SCOUT,
        /* icon            */ I_SCOUT,
        /* armory portrait */ A_SCOUT,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(8a1d),
        /* plural name     */ _(95a3),
        /* description */
        _(b2bb),
        /* tutorial description (optional) */
        _(1ccb)),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE |
            TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ TANK,
        /* shadow          */ S_TANK,
        /* data            */ D_TANK,
        /* flics animation */ F_TANK,
        /* portrait        */ P_TANK,
        /* icon            */ I_TANK,
        /* armory portrait */ A_TANK,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(c0b2),
        /* plural name     */ _(ca60),
        /* description */
        _(d58b),
        /* tutorial description (optional) */
        _(809e)),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT,
        /* sprite          */ ARTILLRY,
        /* shadow          */ S_ARTILL,
        /* data            */ D_FIRING,
        /* flics animation */ F_ARTILL,
        /* portrait        */ P_ARTY,
        /* icon            */ I_ARTY,
        /* armory portrait */ A_ARTY,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(baf1),
        /* plural name     */ _(301e),
        /* description */
        _(3a41)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ ROCKTLCH,
        /* shadow          */ S_ROCKTL,
        /* data            */ D_FIRING,
        /* flics animation */ F_ROCKTL,
        /* portrait        */ P_ROCKET,
        /* icon            */ I_ROCKET,
        /* armory portrait */ A_ROCKET,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(dcf4),
        /* plural name     */ _(a685),
        /* description */
        _(03cb)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ MISSLLCH,
        /* shadow          */ S_MISSLL,
        /* data            */ D_FIRING,
        /* flics animation */ F_MISSLL,
        /* portrait        */ P_MISSIL,
        /* icon            */ I_MISSIL,
        /* armory portrait */ A_MISSIL,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(3fa5),
        /* plural name     */ _(c263),
        /* description */
        _(0aa6)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE |
            TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ SP_FLAK,
        /* shadow          */ S_FLAK,
        /* data            */ D_SP_FLK,
        /* flics animation */ F_FLAK,
        /* portrait        */ P_AA,
        /* icon            */ I_AA,
        /* armory portrait */ A_AA,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(b6ef),
        /* plural name     */ _(a3da),
        /* description */
        _(051d)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ MINELAYR,
        /* shadow          */ S_MINELA,
        /* data            */ D_MOBILE,
        /* flics animation */ F_MINELA,
        /* portrait        */ P_MNELAY,
        /* icon            */ I_MNELAY,
        /* armory portrait */ A_MNELAY,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'M',
        /* singular name   */ _(c838),
        /* plural name     */ _(d26f),
        /* description */
        _(45c2)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT,
        /* sprite          */ SURVEYOR,
        /* shadow          */ S_SURVEY,
        /* data            */ D_AMPHIB,
        /* flics animation */ F_SURVEY,
        /* portrait        */ P_SURVEY,
        /* icon            */ I_SURVEY,
        /* armory portrait */ A_SURVEY,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(5bc4),
        /* plural name     */ _(8def),
        /* description */
        _(f3a3),
        /* tutorial description (optional) */
        _(d2c2)),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT |
            SPINNING_TURRET,
        /* sprite          */ SCANNER,
        /* shadow          */ S_SCANNE,
        /* data            */ D_SCANNR,
        /* flics animation */ F_SCANNE,
        /* portrait        */ P_SCANNR,
        /* icon            */ I_SCANNR,
        /* armory portrait */ A_SCANNR,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(71d4),
        /* plural name     */ _(e812),
        /* description */
        _(2643)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ SPLYTRCK,
        /* shadow          */ S_SPLYTR,
        /* data            */ D_MOBILE,
        /* flics animation */ F_SPLYTR,
        /* portrait        */ P_SPLYTR,
        /* icon            */ I_SPLYTR,
        /* armory portrait */ A_SPLYTR,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ _(9dff),
        /* plural name     */ _(c597),
        /* description */
        _(d932)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ GOLDTRCK,
        /* shadow          */ S_GOLDTR,
        /* data            */ D_MOBILE,
        /* flics animation */ F_GOLDTR,
        /* portrait        */ P_GOLDTR,
        /* icon            */ I_GOLDTR,
        /* armory portrait */ A_GOLDTR,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ GOLD,
        /* gender          */ 'N',
        /* singular name   */ _(e4c1),
        /* plural name     */ _(23bb),
        /* description */
        _(65cf)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ ENGINEER,
        /* shadow          */ S_ENGINE,
        /* data            */ D_AMPHIB,
        /* flics animation */ F_ENGINE,
        /* portrait        */ P_ENGINR,
        /* icon            */ I_ENGINR,
        /* armory portrait */ A_ENGINR,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ _(d933),
        /* plural name     */ _(373a),
        /* description */
        _(52b5),
        /* tutorial description (optional) */
        _(3fec)),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ BULLDOZR,
        /* shadow          */ S_BULLDO,
        /* data            */ D_MOBILE,
        /* flics animation */ F_BULLDO,
        /* portrait        */ P_BULLDZ,
        /* icon            */ I_BULLDZ,
        /* armory portrait */ A_BULLDZ,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ _(fac3),
        /* plural name     */ _(5be1),
        /* description */
        _(3876)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ REPAIR,
        /* shadow          */ S_REPAIR,
        /* data            */ D_MOBILE,
        /* flics animation */ F_REPAIR,
        /* portrait        */ P_REPAIR,
        /* icon            */ I_REPAIR,
        /* armory portrait */ A_REPAIR,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ _(f152),
        /* plural name     */ _(156b),
        /* description */
        _(8fdf)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ FUELTRCK,
        /* shadow          */ S_FUELTR,
        /* data            */ D_MOBILE,
        /* flics animation */ F_FUELTR,
        /* portrait        */ P_FUELTR,
        /* icon            */ I_FUELTR,
        /* armory portrait */ A_FUELTR,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ FUEL,
        /* gender          */ 'N',
        /* singular name   */ _(3753),
        /* plural name     */ _(7dd4),
        /* description */
        _(f5be)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT,
        /* sprite          */ CLNTRANS,
        /* shadow          */ S_CLNTRA,
        /* data            */ D_MOBILE,
        /* flics animation */ F_CLNTRA,
        /* portrait        */ P_COLNST,
        /* icon            */ I_COLNST,
        /* armory portrait */ A_COLNST,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ 4,
        /* gender          */ 'N',
        /* singular name   */ _(66e9),
        /* plural name     */ _(93e1),
        /* description */
        _(a28a)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ COMMANDO,
        /* shadow          */ S_COMMAN,
        /* data            */ D_COMMAN,
        /* flics animation */ F_COMMAN,
        /* portrait        */ P_COMMAN,
        /* icon            */ I_COMMAN,
        /* armory portrait */ A_COMMAN,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_COAST,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(ba90),
        /* plural name     */ _(6b15),
        /* description */
        _(cc81)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ INFANTRY,
        /* shadow          */ S_INFANT,
        /* data            */ D_INFANT,
        /* flics animation */ F_INFANT,
        /* portrait        */ P_INFANT,
        /* icon            */ I_INFANT,
        /* armory portrait */ A_INFANT,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_COAST,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(ad96),
        /* plural name     */ _(461c),
        /* description */
        _(9e7c)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE |
            TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ FASTBOAT,
        /* shadow          */ S_FASTBO,
        /* data            */ D_ESCORT,
        /* flics animation */ F_FASTBO,
        /* portrait        */ P_ESCORT,
        /* icon            */ I_ESCORT,
        /* armory portrait */ A_ESCORT,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(4cc2),
        /* plural name     */ _(363b),
        /* description */
        _(365d)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ CORVETTE,
        /* shadow          */ S_CORVET,
        /* data            */ D_FIRING,
        /* flics animation */ F_CORVET,
        /* portrait        */ P_CORVET,
        /* icon            */ I_CORVET,
        /* armory portrait */ A_CORVET,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(0f0c),
        /* plural name     */ _(eb92),
        /* description */
        _(916e)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE |
            TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ BATTLSHP,
        /* shadow          */ S_BATTLS,
        /* data            */ D_BATTLS,
        /* flics animation */ F_BATTLS,
        /* portrait        */ P_GUNBT,
        /* icon            */ I_GUNBT,
        /* armory portrait */ A_GUNBT,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(4ae7),
        /* plural name     */ _(9cbf),
        /* description */
        _(fe06)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | FIRES_MISSILES | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT,
        /* sprite          */ SUBMARNE,
        /* shadow          */ S_SUBMAR,
        /* data            */ D_SUB,
        /* flics animation */ F_SUBMAR,
        /* portrait        */ P_SUB,
        /* icon            */ I_SUB,
        /* armory portrait */ A_SUB,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(5662),
        /* plural name     */ _(358f),
        /* description */
        _(06b2)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ SEATRANS,
        /* shadow          */ S_SEATRA,
        /* data            */ D_MOBILE,
        /* flics animation */ F_SEATRA,
        /* portrait        */ P_SEATRN,
        /* icon            */ I_SEATRN,
        /* armory portrait */ A_SEATRN,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ 4,
        /* gender          */ 'N',
        /* singular name   */ _(2b1a),
        /* plural name     */ _(fb02),
        /* description */
        _(f368)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ MSSLBOAT,
        /* shadow          */ S_MSSLBO,
        /* data            */ D_FIRING,
        /* flics animation */ F_MSSLBO,
        /* portrait        */ P_MSLCR,
        /* icon            */ I_MSLCR,
        /* armory portrait */ A_MSLCR,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(ee2f),
        /* plural name     */ _(76ab),
        /* description */
        _(ac8d)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ SEAMNLYR,
        /* shadow          */ S_SEAMNL,
        /* data            */ D_MOBILE,
        /* flics animation */ F_SEAMNL,
        /* portrait        */ P_SEAMNL,
        /* icon            */ I_SEAMNL,
        /* armory portrait */ A_SEAMNL,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ _(efbe),
        /* plural name     */ _(f511),
        /* description */
        _(24e7)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ CARGOSHP,
        /* shadow          */ S_CARGOS,
        /* data            */ D_MOBILE,
        /* flics animation */ F_CARGOS,
        /* portrait        */ P_CARGOS,
        /* icon            */ I_CARGOS,
        /* armory portrait */ A_CARGOS,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER | SURFACE_TYPE_COAST,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ _(a04b),
        /* plural name     */ _(48bb),
        /* description */
        _(2a9f)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_AIR_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ FIGHTER,
        /* shadow          */ S_FIGHTE,
        /* data            */ D_FIRING,
        /* flics animation */ F_FIGHTE,
        /* portrait        */ P_FIGHTR,
        /* icon            */ I_FIGHTR,
        /* armory portrait */ A_FIGHTR,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST | SURFACE_TYPE_AIR,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(fe5b),
        /* plural name     */ _(c8fd),
        /* description */
        _(71d6)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_AIR_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ BOMBER,
        /* shadow          */ S_BOMBER,
        /* data            */ D_FIRING,
        /* flics animation */ F_BOMBER,
        /* portrait        */ P_BOMBER,
        /* icon            */ I_BOMBER,
        /* armory portrait */ A_BOMBER,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST | SURFACE_TYPE_AIR,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(62eb),
        /* plural name     */ _(a8d9),
        /* description */
        _(2e66)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_AIR_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ AIRTRANS,
        /* shadow          */ S_AIRTRA,
        /* data            */ D_MOBILE,
        /* flics animation */ F_AIRTRA,
        /* portrait        */ P_AIRTRN,
        /* icon            */ I_AIRTRN,
        /* armory portrait */ A_AIRTRN,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST | SURFACE_TYPE_AIR,
        /* cargo type      */ 4,
        /* gender          */ 'N',
        /* singular name   */ _(4195),
        /* plural name     */ _(883c),
        /* description */
        _(60d7)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_AIR_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT |
            SPINNING_TURRET,
        /* sprite          */ AWAC,
        /* shadow          */ S_AWAC,
        /* data            */ D_AWAC,
        /* flics animation */ F_AWAC,
        /* portrait        */ P_AWAC,
        /* icon            */ I_AWAC,
        /* armory portrait */ A_AWAC,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST | SURFACE_TYPE_AIR,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ _(d91c),
        /* plural name     */ _(4044),
        /* description */
        _(9b5d)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT | REGENERATING_UNIT,
        /* sprite          */ JUGGRNT,
        /* shadow          */ S_JUGGRN,
        /* data            */ D_FIRING,
        /* flics animation */ F_JUGGRN,
        /* portrait        */ P_JUGGER,
        /* icon            */ I_JUGGER,
        /* armory portrait */ A_JUGGER,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(1801),
        /* plural name     */ _(48c0),
        /* description */
        _(9f7c)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT | SELECTABLE |
            TURRET_SPRITE | SENTRY_UNIT | REGENERATING_UNIT,
        /* sprite          */ ALNTANK,
        /* shadow          */ S_ALNTAN,
        /* data            */ D_ALTANK,
        /* flics animation */ F_ALNTAN,
        /* portrait        */ P_ALNTAN,
        /* icon            */ I_ALNTAN,
        /* armory portrait */ A_ALNTAN,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(b952),
        /* plural name     */ _(1cff),
        /* description */
        _(91b3)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT | REGENERATING_UNIT,
        /* sprite          */ ALNASGUN,
        /* shadow          */ S_ALNASG,
        /* data            */ D_FIRING,
        /* flics animation */ F_ALNASG,
        /* portrait        */ P_ALNASG,
        /* icon            */ I_ALNASG,
        /* armory portrait */ A_ALNASG,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(0ca9),
        /* plural name     */ _(bde7),
        /* description */
        _(0a15)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_AIR_UNIT | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT | REGENERATING_UNIT,
        /* sprite          */ ALNPLANE,
        /* shadow          */ S_ALNPLA,
        /* data            */ D_FIRING,
        /* flics animation */ F_ALNPLA,
        /* portrait        */ P_ALNPLA,
        /* icon            */ I_ALNPLA,
        /* armory portrait */ A_ALNPLA,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND | SURFACE_TYPE_WATER | SURFACE_TYPE_COAST | SURFACE_TYPE_AIR,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ _(1d38),
        /* plural name     */ _(a44c),
        /* description */
        _(0f27)
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MISSILE_UNIT,
        /* sprite          */ ROCKET,
        /* shadow          */ INVALID_ID,
        /* data            */ D_TORPDO,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "missile",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | MISSILE_UNIT,
        /* sprite          */ TORPEDO,
        /* shadow          */ INVALID_ID,
        /* data            */ D_TORPDO,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "torpedo",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MISSILE_UNIT,
        /* sprite          */ ALNMISSL,
        /* shadow          */ INVALID_ID,
        /* data            */ D_ALNMSL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "alien missile",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MISSILE_UNIT,
        /* sprite          */ ALNTBALL,
        /* shadow          */ INVALID_ID,
        /* data            */ D_ALNPBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "tank plasma ball",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MISSILE_UNIT,
        /* sprite          */ ALNABALL,
        /* shadow          */ INVALID_ID,
        /* data            */ D_ALNPBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "artillery plasma ball",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | MISSILE_UNIT,
        /* sprite          */ RKTSMOKE,
        /* shadow          */ INVALID_ID,
        /* data            */ D_SMOKE,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "smoke trail",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | EXPLODING | ANIMATED | MISSILE_UNIT,
        /* sprite          */ TRPBUBLE,
        /* shadow          */ INVALID_ID,
        /* data            */ D_BUBBLE,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "bubble trail",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY,
        /* sprite          */ HARVSTER,
        /* shadow          */ S_HARVST,
        /* data            */ D_DEFALT,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "harvester",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY,
        /* sprite          */ WALDO,
        /* shadow          */ INVALID_ID,
        /* data            */ D_DEFALT,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ SURFACE_TYPE_LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "dead waldo",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        )};

CTInfo UnitsManager_TeamInfo[PLAYER_TEAM_MAX];

struct PopupFunctions UnitsManager_PopupCallbacks[POPUP_MENU_TYPE_COUNT];

TeamMissionSupplies UnitsManager_TeamMissionSupplies[PLAYER_TEAM_MAX];

static const char* const UnitsManager_BuildTimeEstimates[] = {_(360a), _(16f4), _(7101)};

static const char* const UnitsManager_ReactionsToEnemy[] = {_(1b93), _(5a01), _(eb57)};

bool UnitsManager_SelfDestructActiveMenu(WindowInfo* window) {
    Button* button_destruct;
    bool event_click_destruct;
    bool event_click_cancel;
    bool event_release;

    for (uint16_t id = SLFDOPN1; id <= SLFDOPN6; ++id) {
        uint32_t time_Stamp = timer_get();

        WindowManager_LoadSimpleImage(static_cast<ResourceID>(id), 13, 11, false, window);
        win_draw(window->id);
        GameManager_ProcessState(true);

        while (timer_get() - time_Stamp < TIMER_FPS_TO_MS(48)) {
        }
    }

    button_destruct = new (std::nothrow) Button(SLFDOK_U, SLFDOK_D, 13, 11);
    button_destruct->SetRValue(GNW_KB_KEY_RETURN);
    button_destruct->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_RETURN);
    button_destruct->SetSfx(NDONE0);
    button_destruct->RegisterButton(window->id);

    win_draw(window->id);

    event_click_destruct = false;
    event_click_cancel = false;
    event_release = false;

    while (!event_click_destruct && !event_click_cancel) {
        int32_t key = get_input();

        if (GameManager_RequestMenuExit) {
            key = GNW_KB_KEY_ESCAPE;
        }

        if (key == GNW_KB_KEY_RETURN) {
            event_click_destruct = true;
        } else if (key == GNW_KB_KEY_ESCAPE) {
            event_click_cancel = true;
        } else if (key >= GNW_INPUT_PRESS && !event_release) {
            if (key == GNW_INPUT_PRESS + GNW_KB_KEY_RETURN) {
                button_destruct->PlaySound();
            } else {
                SoundManager.PlaySfx(NCANC0);
            }

            event_release = true;
        }

        GameManager_ProcessState(true);
    }

    delete button_destruct;

    return event_click_destruct;
}

bool UnitsManager_SelfDestructMenu() {
    Window destruct_window(SELFDSTR, GameManager_GetDialogWindowCenterMode());
    WindowInfo window;
    Button* button_arm;
    Button* button_cancel;
    bool event_click_arm;
    bool event_click_cancel;
    bool event_release;

    Cursor_SetCursor(CURSOR_HAND);
    Text_SetFont(GNW_TEXT_FONT_5);
    destruct_window.SetFlags(0x10);

    destruct_window.Add();
    destruct_window.FillWindowInfo(&window);

    button_arm = new (std::nothrow) Button(SLFDAR_U, SLFDAR_D, 89, 14);
    button_arm->SetCaption(_(d5cb));
    button_arm->SetFlags(0x05);
    button_arm->SetPValue(GNW_KB_KEY_RETURN);
    button_arm->SetSfx(MBUTT0);
    button_arm->RegisterButton(window.id);

    button_cancel = new (std::nothrow) Button(SLFDCN_U, SLFDCN_D, 89, 46);
    button_cancel->SetCaption(_(ef77));
    button_cancel->SetRValue(GNW_KB_KEY_ESCAPE);
    button_cancel->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_ESCAPE);
    button_cancel->SetSfx(NCANC0);
    button_cancel->RegisterButton(window.id);

    win_draw(window.id);

    event_click_arm = false;
    event_click_cancel = false;
    event_release = false;

    while (!event_click_arm && !event_click_cancel) {
        int32_t key = get_input();

        if (GameManager_RequestMenuExit) {
            key = GNW_KB_KEY_ESCAPE;
        }

        if (key == GNW_KB_KEY_RETURN) {
            button_arm->PlaySound();
            button_arm->Disable();
            if (UnitsManager_SelfDestructActiveMenu(&window)) {
                event_click_arm = true;
            } else {
                event_click_cancel = true;
            }
        } else if (key == GNW_KB_KEY_ESCAPE) {
            event_click_cancel = true;
        } else if (key >= GNW_INPUT_PRESS && !event_release) {
            button_cancel->PlaySound();
            event_release = true;
        }

        GameManager_ProcessState(true);
    }

    delete button_arm;
    delete button_cancel;

    return event_click_arm;
}

int32_t UnitsManager_CalculateAttackDamage(UnitInfo* attacker_unit, UnitInfo* target_unit, int32_t damage_potential) {
    int32_t target_armor = target_unit->GetBaseValues()->GetAttribute(ATTRIB_ARMOR);
    int32_t attacker_damage = target_unit->GetBaseValues()->GetAttribute(ATTRIB_ATTACK);

    if (damage_potential > 1) {
        attacker_damage *= (5 - damage_potential) / 4;
    }

    if (target_unit->unit_type == SUBMARNE &&
        (attacker_unit->unit_type == BOMBER || attacker_unit->unit_type == ALNPLANE)) {
        attacker_damage /= 2;
    }

    if (target_armor < attacker_damage) {
        attacker_damage -= target_armor;
    } else {
        attacker_damage = 1;
    }

    return attacker_damage;
}

UnitValues* UnitsManager_GetCurrentUnitValues(CTInfo* team_info, ResourceID unit_type) {
    return team_info->team_units->GetCurrentUnitValues(unit_type);
}

void UnitsManager_AddAxisMissionLoadout(uint16_t team, SmartObjectArray<ResourceID> units) {
    if (ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_CLAN + team)) == TEAM_CLAN_AXIS_INC) {
        int32_t credits;
        ResourceID unit_type;
        uint16_t engineer_turns;
        uint16_t constructor_turns;

        credits = ((ini_get_setting(INI_START_GOLD) / 3) + 18) / 3;
        engineer_turns =
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], ENGINEER)->GetAttribute(ATTRIB_TURNS);
        constructor_turns =
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], CONSTRCT)->GetAttribute(ATTRIB_TURNS);

        while (credits >= engineer_turns || credits >= constructor_turns) {
            if (credits >= engineer_turns) {
                unit_type = ENGINEER;
                units.PushBack(&unit_type);
                credits -= engineer_turns;
            }

            if (credits >= constructor_turns) {
                unit_type = CONSTRCT;
                units.PushBack(&unit_type);
                credits -= constructor_turns;
            }
        }
    }
}

int32_t UnitsManager_AddDefaultMissionLoadout(uint16_t team) {
    ResourceID unit_type;
    uint16_t cargo;
    int32_t units_count;

    UnitsManager_TeamMissionSupplies[team].team_gold = 0;
    UnitsManager_TeamInfo[team].stats_gold_spent_on_upgrades = 0;

    SmartObjectArray<ResourceID> units(UnitsManager_TeamMissionSupplies[team].units);
    SmartObjectArray<uint16_t> cargos(UnitsManager_TeamMissionSupplies[team].cargos);

    units.Clear();

    unit_type = CONSTRCT;
    units.PushBack(&unit_type);

    unit_type = ENGINEER;
    units.PushBack(&unit_type);

    unit_type = SURVEYOR;
    units.PushBack(&unit_type);

    UnitsManager_AddAxisMissionLoadout(team, units);

    units_count = units.GetCount();

    cargos.Clear();

    cargo = 40;
    cargos.PushBack(&cargo);

    cargo = 20;
    cargos.PushBack(&cargo);

    for (int32_t i = 2; i < units_count; ++i) {
        cargo = 0;
        cargos.PushBack(&cargo);
    }

    return units_count;
}

void UnitsManager_Popup_OnClick_Sentry(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    GameManager_UpdateDrawBounds();

    unit->path = nullptr;
    unit->disabled_reaction_fire = false;
    unit->auto_survey = false;

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_38(unit);
    }

    if (unit->orders == ORDER_SENTRY) {
        UnitsManager_SetNewOrder(unit, ORDER_AWAIT, ORDER_STATE_1);
        GameManager_UpdateInfoDisplay(unit);

    } else {
        UnitsManager_SetNewOrder(unit, ORDER_SENTRY, ORDER_STATE_1);
        GameManager_UpdateInfoDisplay(unit);
        GameManager_AutoSelectNext(unit);
    }
}

void UnitsManager_Popup_OnClick_Upgrade(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(false);
    unit->SetParent(unit);
    UnitsManager_SetNewOrder(unit, ORDER_UPGRADE, ORDER_STATE_0);
}

void UnitsManager_Popup_OnClick_UpgradeAll(ButtonID bid, UnitInfo* unit) {
    int32_t material_cost;
    int32_t cost;
    int32_t unit_count;
    UnitInfo* upgraded_unit;
    SmartArray<Complex> complexes;
    ObjectArray<int16_t> costs;
    int32_t index;
    char mark_level[20];

    GameManager_DeinitPopupButtons(true);

    material_cost = 0;
    unit_count = 0;
    upgraded_unit = nullptr;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if (unit->team == (*it).team && unit->unit_type == (*it).unit_type && (*it).IsUpgradeAvailable() &&
            (*it).state != ORDER_STATE_UNIT_READY) {
            for (index = 0; index < complexes.GetCount(); ++index) {
                if ((*it).GetComplex() == &complexes[index]) {
                    break;
                }
            }

            if (index == complexes.GetCount()) {
                Cargo materials;
                Cargo capacity;

                (*it).GetComplex()->GetCargoInfo(materials, capacity);

                complexes.Insert((*it).GetComplex());
                costs.Append(&materials.raw);
            }

            cost = (*it).GetNormalRateBuildCost() / 4;

            if (*costs[index] >= cost) {
                (*it).SetParent(&*it);
                UnitsManager_SetNewOrder(&*it, ORDER_UPGRADE, ORDER_STATE_1);

                ++unit_count;
                material_cost += cost;

                upgraded_unit = &*it;

                *costs[index] -= cost;
            }
        }
    }

    UnitInfo::GetVersion(
        mark_level,
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[unit->team], unit->unit_type)->GetVersion());

    if (unit_count <= 0) {
        SmartString string;

        MessageManager_DrawMessage(string.Sprintf(80, _(a0ee), unit->GetNormalRateBuildCost() / 4).GetCStr(), 2, 0);

    } else if (unit_count == 1) {
        SmartString string;

        MessageManager_DrawMessage(
            string
                .Sprintf(80, _(8967), UnitsManager_BaseUnits[unit->unit_type].singular_name, mark_level, material_cost)
                .GetCStr(),
            0, upgraded_unit, Point(upgraded_unit->grid_x, upgraded_unit->grid_y));

    } else {
        SmartString string;

        MessageManager_DrawMessage(
            string
                .Sprintf(80, _(2693), unit_count, UnitsManager_BaseUnits[unit->unit_type].plural_name, mark_level,
                         material_cost)
                .GetCStr(),
            0, 0);
    }
}

void UnitsManager_Popup_OnClick_Enter(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    unit->enter_mode = !unit->enter_mode;
    unit->cursor = 0;
    unit->targeting_mode = 0;
}

void UnitsManager_Popup_OnClick_Remove(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    if (UnitsManager_SelfDestructMenu()) {
        UnitsManager_SetNewOrder(unit, ORDER_EXPLODE, ORDER_STATE_27);
    }
}

void UnitsManager_Popup_OnClick_Stop(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    UnitsManager_SetNewOrder(unit, ORDER_AWAIT, ORDER_STATE_CLEAR_PATH);
}

void UnitsManager_Popup_OnClick_Transfer(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->cursor == 3) {
        unit->cursor = 0;

    } else {
        unit->cursor = 3;
    }

    unit->targeting_mode = false;
    unit->enter_mode = false;
}

void UnitsManager_Popup_OnClick_Manual(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    unit->disabled_reaction_fire = !unit->disabled_reaction_fire;

    if (unit->orders == ORDER_SENTRY) {
        UnitsManager_SetNewOrder(unit, ORDER_AWAIT, ORDER_STATE_1);
    }

    GameManager_UpdateInfoDisplay(unit);
}

void UnitsManager_Popup_InitCommons(UnitInfo* unit, struct PopupButtons* buttons) {
    if (GameManager_PlayMode == PLAY_MODE_SIMULTANEOUS_MOVES && unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0 &&
        unit->unit_type != COMMANDO) {
        UnitsManager_RegisterButton(buttons, unit->disabled_reaction_fire, _(b0b9), '4',
                                    &UnitsManager_Popup_OnClick_Manual);
    }

    if (UnitsManager_BaseUnits[unit->unit_type].cargo_type > CARGO_TYPE_NONE &&
        UnitsManager_BaseUnits[unit->unit_type].cargo_type <= CARGO_TYPE_GOLD && unit->orders != ORDER_CLEAR &&
        unit->orders != ORDER_BUILD) {
        UnitsManager_RegisterButton(buttons, unit->cursor == 3, _(886b), '3', &UnitsManager_Popup_OnClick_Transfer);
    }

    if ((unit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) && unit->orders != ORDER_CLEAR &&
        unit->orders != ORDER_BUILD) {
        UnitsManager_RegisterButton(buttons, unit->enter_mode, _(e08d), '5', &UnitsManager_Popup_OnClick_Enter);
    }

    if (unit->path != nullptr && unit->orders != ORDER_CLEAR && unit->orders != ORDER_BUILD) {
        UnitsManager_RegisterButton(buttons, false, _(6c45), '7', &UnitsManager_Popup_OnClick_Stop);
    }

    if (unit->IsUpgradeAvailable()) {
        UnitsManager_RegisterButton(buttons, false, _(7fef), '5', &UnitsManager_Popup_OnClick_Upgrade);
        UnitsManager_RegisterButton(buttons, false, _(97a2), '6', &UnitsManager_Popup_OnClick_UpgradeAll);
    }

    if (unit->flags & SENTRY_UNIT) {
        if (unit->unit_type != COMMANDO) {
            if (unit->orders == ORDER_SENTRY) {
                UnitsManager_RegisterButton(buttons, true, _(0cf6), '8', &UnitsManager_Popup_OnClick_Sentry);

            } else if (unit->orders == ORDER_AWAIT) {
                UnitsManager_RegisterButton(buttons, false, _(524e), '8', &UnitsManager_Popup_OnClick_Sentry);
            }
        }

        UnitsManager_RegisterButton(buttons, false, _(2b7d), '9', &UnitsManager_Popup_OnClick_Done);
    }

    if (unit->flags & STATIONARY) {
        UnitsManager_RegisterButton(buttons, false, _(7e56), '0', &UnitsManager_Popup_OnClick_Remove);
    }
}

void UnitsManager_Popup_OnClick_Done(ButtonID bid, UnitInfo* unit) { UnitsManager_PerformAction(unit); }

void UnitsManager_Popup_OnClick_Auto(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    unit->auto_survey = !unit->auto_survey;

    if (unit->orders == ORDER_SENTRY) {
        UnitsManager_SetNewOrder(unit, ORDER_AWAIT, ORDER_STATE_1);
    }

    if (unit->auto_survey) {
        Ai_EnableAutoSurvey(unit);

    } else {
        unit->RemoveTasks();
    }

    GameManager_UpdateInfoDisplay(unit);
}

void UnitsManager_Popup_InitSurveyor(UnitInfo* unit, struct PopupButtons* buttons) {
    UnitsManager_RegisterButton(buttons, unit->auto_survey, _(0286), '1', &UnitsManager_Popup_OnClick_Auto);
    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_InitFuelSuppliers(UnitInfo* unit, struct PopupButtons* buttons) {
    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_OnClick_TargetingMode(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    unit->targeting_mode = !unit->targeting_mode;
    unit->enter_mode = 0;
    unit->cursor = 0;
}

void UnitsManager_Popup_InitMilitary(UnitInfo* unit, struct PopupButtons* buttons) {
    UnitsManager_RegisterButton(buttons, unit->targeting_mode, _(e183), '3', &UnitsManager_Popup_OnClick_TargetingMode);
    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_OnClick_Reload(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->cursor == 6) {
        unit->cursor = 0;

    } else {
        unit->cursor = 6;
    }
}

void UnitsManager_Popup_InitReloaders(UnitInfo* unit, struct PopupButtons* buttons) {
    UnitsManager_RegisterButton(buttons, unit->cursor == 6, _(9d39), '1', &UnitsManager_Popup_OnClick_Reload);
    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_OnClick_PlaceMine(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->GetLayingState() == 2) {
        UnitsManager_SetNewOrder(unit, ORDER_LAY_MINE, ORDER_STATE_INACTIVE);

    } else if (unit->storage > 0) {
        UnitsManager_SetNewOrder(unit, ORDER_LAY_MINE, ORDER_STATE_PLACING_MINES);

    } else {
        MessageManager_DrawMessage(_(78a0), 1, 0);
    }
}

void UnitsManager_Popup_OnClick_RemoveMine(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->GetLayingState() == 1) {
        UnitsManager_SetNewOrder(unit, ORDER_LAY_MINE, ORDER_STATE_INACTIVE);

    } else if (unit->storage != unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
        UnitsManager_SetNewOrder(unit, ORDER_LAY_MINE, ORDER_STATE_REMOVING_MINES);

    } else {
        MessageManager_DrawMessage(_(63d6), 1, 0);
    }
}

void UnitsManager_Popup_InitMineLayers(UnitInfo* unit, struct PopupButtons* buttons) {
    UnitsManager_RegisterButton(buttons, unit->GetLayingState() == 2, _(81df), '1',
                                &UnitsManager_Popup_OnClick_PlaceMine);
    UnitsManager_RegisterButton(buttons, unit->GetLayingState() == 1, _(bfe6), '0',
                                &UnitsManager_Popup_OnClick_RemoveMine);
    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_OnClick_Repair(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->cursor == 4) {
        unit->cursor = 0;

    } else {
        unit->cursor = 4;
    }
}

void UnitsManager_Popup_InitRepair(UnitInfo* unit, struct PopupButtons* buttons) {
    UnitsManager_RegisterButton(buttons, unit->cursor == 4, _(a1a1), '1', &UnitsManager_Popup_OnClick_Repair);
    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_OnClick_BuildStop(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->orders == ORDER_BUILD) {
        GameManager_SelectBuildSite(unit);
        GameManager_EnableMainMenu(unit);

    } else {
        GameManager_DisableMainMenu();
        if (BuildMenu_Menu(unit)) {
            GameManager_AutoSelectNext(unit);

        } else {
            GameManager_EnableMainMenu(unit);
        }
    }
}

void UnitsManager_Popup_InitBuilders(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->orders == ORDER_BUILD && unit->state != ORDER_STATE_13 && unit->state != ORDER_STATE_46) {
        UnitsManager_RegisterButton(buttons, false, _(bf85), '7', &UnitsManager_Popup_OnClick_BuildStop);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(b3ef), '1', &UnitsManager_Popup_OnClick_BuildStop);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

bool UnitsManager_Popup_IsNever(UnitInfo* unit) { return false; }

bool UnitsManager_Popup_IsReady(UnitInfo* unit) {
    return (unit->orders = ORDER_AWAIT && unit->state == ORDER_STATE_1) ||
           (unit->orders = ORDER_IDLE && unit->state == ORDER_STATE_BUILDING_READY);
}

void UnitsManager_RegisterButton(PopupButtons* buttons, bool state, const char* caption, char position,
                                 void (*event_handler)(ButtonID bid, UnitInfo* unit)) {
    int32_t count;
    char key;

    SDL_assert(buttons->popup_count < UNITINFO_MAX_POPUP_BUTTON_COUNT);

    key = position;

    if (position == '0') {
        key = ':';
    }

    for (count = buttons->popup_count; count > 0 && buttons->key_code[count - 1] > key; --count) {
    }

    for (int32_t i = buttons->popup_count - 1; i >= count; --i) {
        buttons->caption[i + 1] = buttons->caption[i];
        buttons->state[i + 1] = buttons->state[i];
        buttons->r_func[i + 1] = buttons->r_func[i];
        buttons->position[i + 1] = buttons->position[i];
        buttons->key_code[i + 1] = buttons->key_code[i];
    }

    buttons->caption[count] = caption;
    buttons->state[count] = state;
    buttons->r_func[count] = reinterpret_cast<ButtonFunc>(event_handler);
    buttons->position[count] = position;
    buttons->key_code[count] = key;

    ++buttons->popup_count;
}

void UnitsManager_PerformAction(UnitInfo* unit) {
    GameManager_DeinitPopupButtons(false);

    if (unit->state == ORDER_STATE_25) {
        GameManager_SelectBuildSite(unit);
    }

    if (unit->orders == ORDER_AWAIT && (unit->flags & STATIONARY)) {
        unit->state = ORDER_STATE_2;
    }

    if (unit->path != nullptr && unit->speed > 0 && unit->orders != ORDER_CLEAR && unit->orders != ORDER_BUILD &&
        !unit->GetUnitList()) {
        uint8_t order;

        order = unit->orders;

        unit->orders = unit->prior_orders;
        unit->state = unit->prior_state;

        if (order == ORDER_MOVE_TO_ATTACK) {
            UnitsManager_SetNewOrder(unit, ORDER_MOVE_TO_ATTACK, ORDER_STATE_0);

        } else {
            UnitsManager_SetNewOrder(unit, ORDER_MOVE, ORDER_STATE_0);
        }

        GameManager_UpdateDrawBounds();
    }

    if (ini_get_setting(INI_AUTO_SELECT)) {
        GameManager_SelectNextUnit(1);
    }
}

bool UnitsManager_Popup_IsUnitReady(UnitInfo* unit) {
    bool result;

    if (unit->orders == ORDER_BUILD && unit->state == ORDER_STATE_UNIT_READY) {
        result = true;

    } else {
        result = UnitsManager_Popup_IsReady(unit);
    }

    return result;
}

void UnitsManager_Popup_OnClick_Build(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    GameManager_DisableMainMenu();

    if (unit->orders == ORDER_BUILD) {
        GameManager_SelectBuildSite(unit);
        GameManager_ProcessTick(false);
    }

    if (BuildMenu_Menu(unit)) {
        GameManager_AutoSelectNext(unit);

    } else {
        GameManager_EnableMainMenu(unit);
    }
}

void UnitsManager_Popup_PlaceNewUnit(ButtonID bid, UnitInfo* unit) {
    UnitInfo* parent;
    int16_t grid_x;
    int16_t grid_y;

    GameManager_DeinitPopupButtons(true);
    GameManager_DisableMainMenu();

    parent = unit->GetParent();

    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    if (Access_FindReachableSpot(parent->unit_type, parent, &grid_x, &grid_y, 1, 1, 0)) {
        parent->FollowUnit();
        MessageManager_DrawMessage(_(87e9), 0, 0);
        GameManager_EnableMainMenu(parent);

    } else {
        MessageManager_DrawMessage(_(c710), 1, 0);
        GameManager_EnableMainMenu(parent);
    }
}

void UnitsManager_Popup_OnClick_StopBuild(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    GameManager_SelectBuildSite(unit);
    GameManager_EnableMainMenu(unit);
}

void UnitsManager_Popup_OnClick_StartBuild(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    unit->BuildOrder();
    GameManager_EnableMainMenu(unit);
}

void UnitsManager_Popup_InitFactories(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->state == ORDER_STATE_UNIT_READY) {
        UnitsManager_Popup_PlaceNewUnit(0, unit);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(7b6d), '1', &UnitsManager_Popup_OnClick_Build);

        if (unit->orders == ORDER_HALT_BUILDING || unit->orders == ORDER_HALT_BUILDING_2) {
            UnitsManager_RegisterButton(buttons, false, _(cced), '2', &UnitsManager_Popup_OnClick_StartBuild);

        } else if (unit->orders == ORDER_BUILD && unit->state != ORDER_STATE_13 && unit->state != ORDER_STATE_46) {
            UnitsManager_RegisterButton(buttons, false, _(bcf7), '7', &UnitsManager_Popup_OnClick_StopBuild);
        }

        UnitsManager_Popup_InitCommons(unit, buttons);
    }
}

void UnitsManager_Popup_OnClick_PowerOn(ButtonID bid, UnitInfo* unit) {
    char message[400];

    GameManager_DeinitPopupButtons(true);
    UnitsManager_SetNewOrder(unit, ORDER_POWER_ON, ORDER_STATE_0);

    sprintf(message, _(8576), UnitsManager_BaseUnits[unit->unit_type].singular_name, _(b8d3));

    MessageManager_DrawMessage(message, 0, 0);
}

void UnitsManager_Popup_OnClick_PowerOff(ButtonID bid, UnitInfo* unit) {
    char message[400];

    GameManager_DeinitPopupButtons(true);
    UnitsManager_SetNewOrder(unit, ORDER_POWER_OFF, ORDER_STATE_0);

    sprintf(message, _(d599), UnitsManager_BaseUnits[unit->unit_type].singular_name, _(b4dc));

    MessageManager_DrawMessage(message, 0, 0);
}

void UnitsManager_Popup_OnClick_Steal(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->cursor == 8) {
        unit->cursor = 0;

    } else {
        unit->cursor = 8;
    }
}

void UnitsManager_Popup_OnClick_Disable(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->cursor == 9) {
        unit->cursor = 0;

    } else {
        unit->cursor = 9;
    }
}

void UnitsManager_Popup_InitInfiltrator(UnitInfo* unit, struct PopupButtons* buttons) {
    UnitsManager_RegisterButton(buttons, unit->cursor == 9, _(6d6a), '1', &UnitsManager_Popup_OnClick_Disable);
    UnitsManager_RegisterButton(buttons, unit->cursor == 8, _(a0fd), '2', &UnitsManager_Popup_OnClick_Steal);
    UnitsManager_RegisterButton(buttons, unit->targeting_mode, _(5f6b), '3', &UnitsManager_Popup_OnClick_TargetingMode);
    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_OnClick_Research(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    ResearchMenu_Menu(unit);
}

void UnitsManager_Popup_InitResearch(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->orders == ORDER_POWER_ON) {
        UnitsManager_RegisterButton(buttons, false, _(2e00), '1', &UnitsManager_Popup_OnClick_Research);
        UnitsManager_RegisterButton(buttons, false, _(6889), '7', &UnitsManager_Popup_OnClick_PowerOff);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(283e), '2', &UnitsManager_Popup_OnClick_PowerOn);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}
void UnitsManager_Popup_OnClick_BuyUpgrade(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    UpgradeMenu(unit->team, unit->GetComplex()).Run();
}

void UnitsManager_Popup_InitGoldRefinery(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->orders == ORDER_POWER_ON) {
        UnitsManager_RegisterButton(buttons, false, _(3f13), '7', &UnitsManager_Popup_OnClick_PowerOff);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(53c3), '2', &UnitsManager_Popup_OnClick_PowerOn);
    }

    UnitsManager_RegisterButton(buttons, false, _(1da0), '1', &UnitsManager_Popup_OnClick_BuyUpgrade);

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_InitRecreationCenter(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->orders == ORDER_POWER_OFF) {
        UnitsManager_RegisterButton(buttons, false, _(bd97), '2', &UnitsManager_Popup_OnClick_PowerOn);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_OnClick_ActivateNonAirUnit(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    GameManager_DisableMainMenu();

    RepairShopMenu_Menu(unit);
}

void UnitsManager_Popup_OnClick_ActivateAirUnit(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    GameManager_DisableMainMenu();

    RepairShopMenu_Menu(unit);
}

void UnitsManager_Popup_OnClick_Activate(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->orders == ORDER_POWER_OFF) {
        UnitsManager_Popup_OnClick_PowerOn(0, unit);

    } else if (unit->flags & MOBILE_AIR_UNIT) {
        UnitsManager_Popup_OnClick_ActivateAirUnit(0, unit);

    } else {
        UnitsManager_Popup_OnClick_ActivateNonAirUnit(0, unit);
    }
}

void UnitsManager_Popup_OnClick_Load(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->cursor == 7) {
        unit->cursor = 0;

    } else {
        unit->cursor = 7;
    }
}

void UnitsManager_Popup_InitRepairShops(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->orders == ORDER_POWER_OFF) {
        UnitsManager_RegisterButton(buttons, false, _(cec1), '1', &UnitsManager_Popup_OnClick_Activate);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(200c), '1', &UnitsManager_Popup_OnClick_Activate);
    }

    UnitsManager_RegisterButton(buttons, unit->cursor == 7, _(8452), '2', &UnitsManager_Popup_OnClick_Load);

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_OnClick_PowerOnAllocate(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->orders == ORDER_POWER_OFF) {
        UnitsManager_Popup_OnClick_PowerOn(0, unit);

    } else {
        GameManager_DisableMainMenu();
        AllocMenu_Menu(unit);
        GameManager_EnableMainMenu(unit);
    }
}

void UnitsManager_Popup_InitMiningStation(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->orders == ORDER_POWER_OFF) {
        UnitsManager_RegisterButton(buttons, false, _(98c9), '2', &UnitsManager_Popup_OnClick_PowerOnAllocate);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(33b7), '1', &UnitsManager_Popup_OnClick_PowerOnAllocate);
        UnitsManager_RegisterButton(buttons, false, _(d9c8), '7', &UnitsManager_Popup_OnClick_PowerOff);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_InitEcoSphere(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->orders == ORDER_POWER_ON) {
        UnitsManager_RegisterButton(buttons, false, _(df26), '7', &UnitsManager_Popup_OnClick_PowerOff);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(5fe9), '2', &UnitsManager_Popup_OnClick_PowerOn);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_InitPowerGenerators(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->orders == ORDER_POWER_ON) {
        UnitsManager_RegisterButton(buttons, false, _(7a07), '7', &UnitsManager_Popup_OnClick_PowerOff);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(e7b6), '2', &UnitsManager_Popup_OnClick_PowerOn);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_OnClick_StopRemove(ButtonID bid, UnitInfo* unit) {
    SmartPointer<UnitInfo> rubble;

    GameManager_DeinitPopupButtons(true);

    if (unit->orders == ORDER_CLEAR) {
        GameManager_SelectBuildSite(unit);
        GameManager_EnableMainMenu(unit);

    } else {
        rubble = Access_GetRemovableRubble(unit->team, unit->grid_x, unit->grid_y);

        if (rubble != nullptr) {
            int32_t clearing_time;
            char message[400];

            unit->SetParent(&*rubble);

            if (rubble->flags & BUILDING) {
                clearing_time = 4;

            } else {
                clearing_time = 1;
            }

            unit->build_time = clearing_time;

            UnitsManager_SetNewOrder(unit, ORDER_CLEAR, ORDER_STATE_0);
            GameManager_UpdateInfoDisplay(unit);
            GameManager_AutoSelectNext(unit);

            sprintf(message, _(b9b7), unit->build_time);

            MessageManager_DrawMessage(message, 0, unit, Point(unit->grid_x, unit->grid_y));

        } else {
            MessageManager_DrawMessage(_(a615), 1, 0);
        }
    }
}

void UnitsManager_Popup_InitRemove(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->orders == ORDER_CLEAR) {
        UnitsManager_RegisterButton(buttons, false, _(bc20), '7', &UnitsManager_Popup_OnClick_StopRemove);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(9737), '0', &UnitsManager_Popup_OnClick_StopRemove);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

bool UnitsManager_IsPowerGeneratorPlaceable(uint16_t team, int16_t* grid_x, int16_t* grid_y) {
    *grid_x -= 1;
    *grid_y += 1;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, 2)) {
        return true;
    }

    *grid_x += 3;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, 2)) {
        return true;
    }

    *grid_x -= 3;
    *grid_y -= 1;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, 2)) {
        return true;
    }

    *grid_x += 3;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, 2)) {
        return true;
    }

    *grid_x -= 2;
    *grid_y -= 1;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, 2)) {
        return true;
    }

    *grid_x += 1;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, 2)) {
        return true;
    }

    *grid_x -= 1;
    *grid_y += 3;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, 2)) {
        return true;
    }

    *grid_x += 1;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, 2)) {
        return true;
    }

    return false;
}

void UnitsManager_Popup_OnClick_StartMasterBuilder(ButtonID bid, UnitInfo* unit) {
    int16_t grid_x;
    int16_t grid_y;

    GameManager_DeinitPopupButtons(true);

    grid_x = unit->target_grid_x;
    grid_y = unit->target_grid_y;

    if (UnitsManager_IsPowerGeneratorPlaceable(unit->team, &grid_x, &grid_y)) {
        UnitsManager_SetNewOrderInt(unit, ORDER_BUILD, ORDER_STATE_25);
        GameManager_TempTape =
            UnitsManager_SpawnUnit(LRGTAPE, GameManager_PlayerTeam, unit->target_grid_x, unit->target_grid_y, unit);

    } else {
        MessageManager_DrawMessage(_(5e0b), 2, 0);
    }

    MessageManager_DrawMessage(_(bb70), 0, 0);
}

bool UnitsManager_IsMasterBuilderPlaceable(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    bool result;
    int16_t raw;
    int16_t fuel;
    int16_t gold;

    result = false;

    Survey_GetTotalResourcesInArea(grid_x, grid_y, 1, &raw, &gold, &fuel, true, unit->team);

    if (raw && Cargo_GetFuelConsumptionRate(POWGEN) < fuel) {
        Hash_MapHash.Remove(unit);

        if (GameManager_TempTape != nullptr) {
            Hash_MapHash.Remove(&*GameManager_TempTape);
        }

        result = Builder_IsAccessible(unit->team, MININGST, grid_x, grid_y);

        if (GameManager_TempTape != nullptr) {
            Hash_MapHash.Add(&*GameManager_TempTape);
        }

        Hash_MapHash.Add(&*GameManager_TempTape);
    }

    return result;
}

void UnitsManager_Popup_InitMaster(UnitInfo* unit, struct PopupButtons* buttons) {
    bool is_placeable;

    is_placeable = false;

    for (int32_t grid_y = unit->grid_y; grid_y >= std::max(unit->grid_y - 1, 0) && !is_placeable; --grid_y) {
        for (int32_t grid_x = unit->grid_x; grid_x >= std::max(unit->grid_x - 1, 0) && !is_placeable; --grid_x) {
            is_placeable = UnitsManager_IsMasterBuilderPlaceable(unit, grid_x, grid_y);

            if (is_placeable) {
                UnitsManager_RegisterButton(buttons, false, _(2c2a), '1',
                                            &UnitsManager_Popup_OnClick_StartMasterBuilder);

                unit->target_grid_x = grid_x;
                unit->target_grid_y = grid_y;
            }
        }
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_InitPopupMenus() {
    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        UnitsManager_DelayedAttackTargets[team].Clear();
    }

    UnitsManager_UnitList6.Clear();

    UnitsManager_Team = PLAYER_TEAM_RED;

    UnitsManager_UnknownCounter = 0;

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
            UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED) {
            UnitsManager_Team = team;
            break;
        }
    }

    UnitsManager_PopupCallbacks[0].init = &UnitsManager_Popup_InitCommons;
    UnitsManager_PopupCallbacks[0].test = &UnitsManager_Popup_IsNever;

    UnitsManager_PopupCallbacks[1].init = &UnitsManager_Popup_InitCommons;
    UnitsManager_PopupCallbacks[1].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[2].init = &UnitsManager_Popup_InitSurveyor;
    UnitsManager_PopupCallbacks[2].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[3].init = &UnitsManager_Popup_InitMilitary;
    UnitsManager_PopupCallbacks[3].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[4].init = &UnitsManager_Popup_InitInfiltrator;
    UnitsManager_PopupCallbacks[4].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[5].init = &UnitsManager_Popup_InitReloaders;
    UnitsManager_PopupCallbacks[5].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[6].init = &UnitsManager_Popup_InitMineLayers;
    UnitsManager_PopupCallbacks[6].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[7].init = &UnitsManager_Popup_InitFuelSuppliers;
    UnitsManager_PopupCallbacks[7].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[8].init = &UnitsManager_Popup_InitRepair;
    UnitsManager_PopupCallbacks[8].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[9].init = &UnitsManager_Popup_InitCommons;
    UnitsManager_PopupCallbacks[9].test = &UnitsManager_Popup_IsNever;

    UnitsManager_PopupCallbacks[10].init = &UnitsManager_Popup_InitFuelSuppliers;
    UnitsManager_PopupCallbacks[10].test = &UnitsManager_Popup_IsNever;

    UnitsManager_PopupCallbacks[11].init = &UnitsManager_Popup_InitRepairShops;
    UnitsManager_PopupCallbacks[11].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[12].init = &UnitsManager_Popup_InitRepairShops;
    UnitsManager_PopupCallbacks[12].test = &UnitsManager_Popup_IsNever;

    UnitsManager_PopupCallbacks[13].init = &UnitsManager_Popup_InitBuilders;
    UnitsManager_PopupCallbacks[13].test = &UnitsManager_Popup_IsUnitReady;

    UnitsManager_PopupCallbacks[14].init = &UnitsManager_Popup_InitFactories;
    UnitsManager_PopupCallbacks[14].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[15].init = &UnitsManager_Popup_InitRecreationCenter;
    UnitsManager_PopupCallbacks[15].test = &UnitsManager_Popup_IsNever;

    UnitsManager_PopupCallbacks[16].init = &UnitsManager_Popup_InitMiningStation;
    UnitsManager_PopupCallbacks[16].test = &UnitsManager_Popup_IsNever;

    UnitsManager_PopupCallbacks[17].init = &UnitsManager_Popup_InitEcoSphere;
    UnitsManager_PopupCallbacks[17].test = &UnitsManager_Popup_IsNever;

    UnitsManager_PopupCallbacks[18].init = &UnitsManager_Popup_InitResearch;
    UnitsManager_PopupCallbacks[18].test = &UnitsManager_Popup_IsNever;

    UnitsManager_PopupCallbacks[19].init = &UnitsManager_Popup_InitPowerGenerators;
    UnitsManager_PopupCallbacks[19].test = &UnitsManager_Popup_IsNever;

    UnitsManager_PopupCallbacks[20].init = &UnitsManager_Popup_InitRemove;
    UnitsManager_PopupCallbacks[20].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[21].init = &UnitsManager_Popup_InitMaster;
    UnitsManager_PopupCallbacks[21].test = &UnitsManager_Popup_IsReady;

    UnitsManager_PopupCallbacks[22].init = &UnitsManager_Popup_InitGoldRefinery;
    UnitsManager_PopupCallbacks[22].test = &UnitsManager_Popup_IsNever;
}

int32_t UnitsManager_GetStealthChancePercentage(UnitInfo* unit1, UnitInfo* unit2, int32_t order) {
    int32_t unit_turns = UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[unit2->team], unit2->unit_type)
                             ->GetAttribute(ATTRIB_TURNS);
    int32_t chance = ((unit1->GetExperience() * 12) * 8 + 640) / unit_turns;

    if (order == ORDER_AWAIT_STEAL_UNIT) {
        chance /= 4;
    }

    if (chance > 90) {
        chance = 90;
    }

    return chance;
}

SmartPointer<UnitInfo> UnitsManager_SpawnUnit(ResourceID unit_type, uint16_t team, int32_t grid_x, int32_t grid_y,
                                              UnitInfo* parent) {
    SmartPointer<UnitInfo> unit;

    unit = UnitsManager_DeployUnit(unit_type, team, nullptr, grid_x, grid_y, 0, true);
    unit->SetParent(parent);
    unit->SpotByTeam(team);

    return unit;
}

void UnitsManager_MoveUnit(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    if (unit->grid_x != grid_x || unit->grid_y != grid_y) {
        unit->RefreshScreen();
        UnitsManager_UpdateMapHash(unit, grid_x, grid_y);
        unit->RefreshScreen();
    }
}

uint32_t UnitsManager_MoveUnitAndParent(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    ResourceID unit_type;
    SmartPointer<UnitInfo> parent;
    uint32_t result;

    unit_type = unit->unit_type;
    parent = unit->GetParent();

    Hash_MapHash.Remove(unit);

    if (parent != nullptr) {
        Hash_MapHash.Remove(&*parent);

        unit_type = parent->GetConstructedUnitType();
    }

    if (UnitsManager_BaseUnits[unit->unit_type].flags & BUILDING) {
        result = Builder_IsAccessible(unit->team, unit_type, grid_x, grid_y);

    } else {
        if (Access_GetModifiedSurfaceType(grid_x, grid_y) == SURFACE_TYPE_AIR) {
            result = 0;

        } else {
            result = Access_IsAccessible(unit_type, unit->team, grid_x, grid_y, 0x2);
        }
    }

    Hash_MapHash.Add(unit);

    if (parent != nullptr) {
        Hash_MapHash.Add(&*parent);
    }

    return result;
}

void UnitsManager_SetInitialMining(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    int16_t raw;
    int16_t fuel;
    int16_t gold;
    int16_t free_capacity;

    Survey_GetTotalResourcesInArea(grid_x, grid_y, 1, &raw, &gold, &fuel, true, unit->team);

    unit->raw_mining_max = raw;
    unit->fuel_mining_max = fuel;
    unit->gold_mining_max = gold;

    unit->fuel_mining = std::min(Cargo_GetFuelConsumptionRate(POWGEN), static_cast<int32_t>(fuel));

    free_capacity = 16 - unit->fuel_mining;

    unit->raw_mining = std::min(raw, free_capacity);

    free_capacity -= unit->raw_mining;

    fuel = std::min(static_cast<int16_t>(fuel - unit->fuel_mining), free_capacity);

    unit->fuel_mining += fuel;

    free_capacity -= fuel;

    unit->gold_mining = std::min(gold, free_capacity);

    unit->total_mining = unit->raw_mining + unit->fuel_mining + unit->gold_mining;
}

void UnitsManager_StartBuild(UnitInfo* unit) {
    if (unit->unit_type == ENGINEER) {
        unit->target_grid_x = unit->grid_x;
        unit->target_grid_y = unit->grid_y;
    }

    unit->orders = unit->prior_orders;
    unit->state = unit->prior_state;

    unit->BuildOrder();

    {
        SmartString string;
        SmartObjectArray<ResourceID> build_list;
        ResourceID unit_type;
        int32_t build_speed_multiplier;
        int32_t turns_to_build_unit;

        build_list = unit->GetBuildList();
        unit_type = *build_list[0];
        build_speed_multiplier = unit->GetBuildRate();

        SDL_assert(build_list.GetCount() > 0);

        unit->GetTurnsToBuild(unit_type, build_speed_multiplier, &turns_to_build_unit);

        string.Sprintf(250, UnitsManager_BuildTimeEstimates[UnitsManager_BaseUnits[unit_type].gender],
                       UnitsManager_BaseUnits[unit_type].singular_name,
                       UnitsManager_TeamInfo[GameManager_PlayerTeam].unit_counters[unit_type], turns_to_build_unit);

        MessageManager_DrawMessage(string.GetCStr(), 0, unit, Point(unit->grid_x, unit->grid_y));
    }
}

void UnitsManager_UpdateUnitPosition(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    unit->grid_x = grid_x;
    unit->grid_y = grid_y;

    grid_x = grid_x * 64 + 32;
    grid_y = grid_y * 64 + 32;

    if (unit->flags & BUILDING) {
        grid_x += 31;
        grid_y += 31;
    }

    grid_x -= unit->x;
    grid_y -= unit->y;

    unit->OffsetDrawZones(grid_x, grid_y);
}

void UnitsManager_UpdateMapHash(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    Hash_MapHash.Remove(unit);
    UnitsManager_UpdateUnitPosition(unit, grid_x, grid_y);
    Hash_MapHash.Add(unit);
}

void UnitsManager_RemoveConnections(UnitInfo* unit) {
    uint16_t team;
    SmartPointer<UnitInfo> building;

    team = unit->team;

    unit->RefreshScreen();

    if ((unit->flags & (CONNECTOR_UNIT | BUILDING | STANDALONE)) && !(unit->flags & GROUND_COVER)) {
        int32_t unit_size;
        int32_t grid_x;
        int32_t grid_y;

        unit->DetachComplex();

        if (unit->flags & BUILDING) {
            unit_size = 2;

        } else {
            unit_size = 1;
        }

        grid_x = unit->grid_x;
        grid_y = unit->grid_y;

        if (unit->connectors & CONNECTOR_NORTH_LEFT) {
            building = Access_GetTeamBuilding(team, grid_x, grid_y - 1);

            if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                building->connectors &= ~CONNECTOR_SOUTH_LEFT;

            } else if (building == Access_GetTeamBuilding(team, grid_x + 1, grid_y - 1)) {
                building->connectors &= ~CONNECTOR_SOUTH_LEFT;

            } else {
                building->connectors &= ~CONNECTOR_SOUTH_RIGHT;
            }

            building->RefreshScreen();
        }

        if (unit->connectors & CONNECTOR_NORTH_RIGHT) {
            building = Access_GetTeamBuilding(team, grid_x + 1, grid_y - 1);

            if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                building->connectors &= ~CONNECTOR_SOUTH_LEFT;

            } else if (building == Access_GetTeamBuilding(team, grid_x, grid_y - 1)) {
                building->connectors &= ~CONNECTOR_SOUTH_RIGHT;

            } else {
                building->connectors &= ~CONNECTOR_SOUTH_LEFT;
            }

            building->RefreshScreen();
        }

        if (unit->connectors & CONNECTOR_EAST_TOP) {
            building = Access_GetTeamBuilding(team, grid_x + unit_size, grid_y);

            if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                building->connectors &= ~CONNECTOR_WEST_TOP;

            } else if (building == Access_GetTeamBuilding(team, grid_x + unit_size, grid_y + 1)) {
                building->connectors &= ~CONNECTOR_WEST_TOP;

            } else {
                building->connectors &= ~CONNECTOR_WEST_BOTTOM;
            }

            building->RefreshScreen();
        }

        if (unit->connectors & CONNECTOR_EAST_BOTTOM) {
            building = Access_GetTeamBuilding(team, grid_x + unit_size, grid_y + 1);

            if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                building->connectors &= ~CONNECTOR_WEST_TOP;

            } else if (building == Access_GetTeamBuilding(team, grid_x + unit_size, grid_y)) {
                building->connectors &= ~CONNECTOR_WEST_BOTTOM;

            } else {
                building->connectors &= ~CONNECTOR_WEST_TOP;
            }

            building->RefreshScreen();
        }

        if (unit->connectors & CONNECTOR_SOUTH_LEFT) {
            building = Access_GetTeamBuilding(team, grid_x, grid_y + unit_size);

            if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                building->connectors &= ~CONNECTOR_NORTH_LEFT;

            } else if (building == Access_GetTeamBuilding(team, grid_x + 1, grid_y + unit_size)) {
                building->connectors &= ~CONNECTOR_NORTH_LEFT;

            } else {
                building->connectors &= ~CONNECTOR_NORTH_RIGHT;
            }

            building->RefreshScreen();
        }

        if (unit->connectors & CONNECTOR_SOUTH_RIGHT) {
            building = Access_GetTeamBuilding(team, grid_x + 1, grid_y + unit_size);

            if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                building->connectors &= ~CONNECTOR_NORTH_LEFT;

            } else if (building == Access_GetTeamBuilding(team, grid_x, grid_y + unit_size)) {
                building->connectors &= ~CONNECTOR_NORTH_RIGHT;

            } else {
                building->connectors &= ~CONNECTOR_NORTH_LEFT;
            }

            building->RefreshScreen();
        }

        if (unit->connectors & CONNECTOR_WEST_TOP) {
            building = Access_GetTeamBuilding(team, grid_x - 1, grid_y);

            if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                building->connectors &= ~CONNECTOR_EAST_TOP;

            } else if (building == Access_GetTeamBuilding(team, grid_x - 1, grid_y + 1)) {
                building->connectors &= ~CONNECTOR_EAST_TOP;

            } else {
                building->connectors &= ~CONNECTOR_EAST_BOTTOM;
            }

            building->RefreshScreen();
        }

        if (unit->connectors & CONNECTOR_WEST_BOTTOM) {
            building = Access_GetTeamBuilding(team, grid_x - 1, grid_y + 1);

            if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                building->connectors &= ~CONNECTOR_EAST_TOP;

            } else if (building == Access_GetTeamBuilding(team, grid_x - 1, grid_y)) {
                building->connectors &= ~CONNECTOR_EAST_BOTTOM;

            } else {
                building->connectors &= ~CONNECTOR_EAST_TOP;
            }

            building->RefreshScreen();
        }
    }
}

int32_t UnitsManager_GetTargetAngle(int32_t distance_x, int32_t distance_y) {
    int32_t result;

    int32_t level_x = labs(distance_x);
    int32_t level_y = labs(distance_y);

    if (distance_x > 0 || distance_y > 0) {
        if (distance_x >= 0 && distance_y >= 0) {
            if (level_y / 2 > level_x || level_x == 0) {
                result = 4;

            } else if (level_x / 2 > level_y || level_y == 0) {
                result = 2;

            } else {
                result = 3;
            }

        } else if (distance_x > 0) {
            if (level_y / 2 > level_x || level_x == 0) {
                result = 0;

            } else if (level_x / 2 > level_y || level_y == 0) {
                result = 2;

            } else {
                result = 1;
            }

        } else {
            if (level_y / 2 > level_x || level_x == 0) {
                result = 4;

            } else if (level_x / 2 > level_y || level_y == 0) {
                result = 6;

            } else {
                result = 5;
            }
        }

    } else {
        if (level_y / 2 > level_x || level_x == 0) {
            result = 0;

        } else if (level_x / 2 > level_y || level_y == 0) {
            result = 6;

        } else {
            result = 7;
        }
    }

    return result;
}

int32_t UnitsManager_GetFiringAngle(int32_t distance_x, int32_t distance_y) {
    int32_t result;

    int32_t level_x = labs(distance_x);
    int32_t level_y = labs(distance_y);

    if (distance_x > 0 || distance_y > 0) {
        if (distance_x >= 0 && distance_y >= 0) {
            if (level_y / 2 > level_x || level_x == 0) {
                result = 8;

            } else if (level_x / 2 > level_y || level_y == 0) {
                result = 4;

            } else if (level_y / 2 >= level_x) {
                result = 7;

            } else if (level_x / 2 >= level_y) {
                result = 5;

            } else {
                result = 6;
            }

        } else if (distance_x > 0) {
            if (level_y / 2 > level_x || level_x == 0) {
                result = 0;

            } else if (level_x / 2 > level_y || level_y == 0) {
                result = 4;

            } else if (level_y / 2 >= level_x) {
                result = 1;

            } else if (level_x / 2 >= level_y) {
                result = 3;

            } else {
                result = 2;
            }

        } else {
            if (level_y / 2 > level_x || level_x == 0) {
                result = 8;

            } else if (level_x / 2 > level_y || level_y == 0) {
                result = 12;

            } else if (level_y / 2 >= level_x) {
                result = 9;

            } else if (level_x / 2 >= level_y) {
                result = 11;

            } else {
                result = 10;
            }
        }

    } else {
        if (level_y / 2 > level_x || level_x == 0) {
            result = 0;

        } else if (level_x / 2 > level_y || level_y == 0) {
            result = 12;

        } else if (level_y / 2 >= level_x) {
            result = 15;

        } else if (level_x / 2 >= level_y) {
            result = 13;

        } else {
            result = 14;
        }
    }

    return result;
}

void UnitsManager_DrawBustedCommando(UnitInfo* unit) { unit->DrawSpriteFrame(unit->angle + 200); }

void UnitsManager_TestBustedCommando(UnitInfo* unit) {
    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (unit->team != team) {
            if (unit->IsVisibleToTeam(team)) {
                UnitsManager_DrawBustedCommando(unit);
            }
        }
    }
}

void UnitsManager_ScaleUnit(UnitInfo* unit, int32_t state) {
    if (GameManager_PlayerTeam == unit->team) {
        SoundManager.PlaySfx(unit, state == ORDER_STATE_EXPAND ? SFX_TYPE_EXPAND : SFX_TYPE_SHRINK);
    }

    UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT_SCALING, state);
    unit->UpdateUnitDrawZones();
    unit->RefreshScreen();
}

void UnitsManager_ProcessRemoteOrders() {
    UnitsManager_EffectCounter = 5;

    Ai_ClearTasksPendingFlags();

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        UnitsManager_Units[team] = nullptr;
    }

    UnitsManager_OrdersPending = false;
    UnitsManager_byte_179448 = false;

    UnitsManager_ProcessOrders(&UnitsManager_GroundCoverUnits);
    UnitsManager_ProcessOrders(&UnitsManager_MobileLandSeaUnits);
    UnitsManager_ProcessOrders(&UnitsManager_StationaryUnits);
    UnitsManager_ProcessOrders(&UnitsManager_MobileAirUnits);
    UnitsManager_ProcessOrders(&UnitsManager_ParticleUnits);

    if (Remote_IsNetworkGame) {
        Remote_ProcessNetPackets();
    }

    if (!UnitsManager_OrdersPending) {
        if (UnitsManager_UnitList6.GetCount() > 0) {
            SmartList<UnitInfo>::Iterator unit_it(UnitsManager_UnitList6.Begin());

            if ((*unit_it).shots > 0) {
                (*unit_it).state = ORDER_STATE_41;

            } else {
                (*unit_it).UpdatePinCount((*unit_it).target_grid_x, (*unit_it).target_grid_x, -1);
                (*unit_it).RestoreOrders();

                if ((*unit_it).orders == ORDER_FIRE) {
                    (*unit_it).orders = ORDER_AWAIT;
                    (*unit_it).state = ORDER_STATE_1;
                }

                if (GameManager_SelectedUnit == *unit_it) {
                    GameManager_UpdateInfoDisplay(&*unit_it);
                }
            }

            UnitsManager_UnitList6.Remove(unit_it);

        } else {
            if (!UnitsManager_AssessAttacks()) {
                UnitsManager_ClearPins(&UnitsManager_MobileLandSeaUnits);
                UnitsManager_ClearPins(&UnitsManager_MobileAirUnits);
            }
        }
    }

    if (UnitsManager_Unit && Access_ProcessNewGroupOrder(&*UnitsManager_Unit)) {
        UnitsManager_Unit = nullptr;
    }

    for (SmartList<UnitEvent>::Iterator it = UnitEvent_UnitEvents.Begin(); it != UnitEvent_UnitEvents.End(); ++it) {
        (*it).Process();
    }

    UnitEvent_UnitEvents.Clear();

    UnitsManager_byte_17947D = UnitsManager_EffectCounter;
}

void UnitsManager_SetNewOrderInt(UnitInfo* unit, int32_t order, int32_t state) {
    bool delete_path = false;

    if (unit->orders != ORDER_EXPLODE && unit->state != ORDER_STATE_14) {
        if (unit->orders == ORDER_AWAIT_SCALING) {
            AiLog log("New order (%s) issued for %s while scaling.", UnitsManager_Orders[order],
                      UnitsManager_BaseUnits[unit->unit_type].singular_name);

            UnitsManager_NewOrderWhileScaling(unit);
        }

        if (unit->state == ORDER_STATE_NEW_ORDER) {
            AiLog log("New order (%s) issued for %s while waiting for path.", UnitsManager_Orders[order],
                      UnitsManager_BaseUnits[unit->unit_type].singular_name);

            unit->orders = ORDER_AWAIT;
            unit->state = ORDER_STATE_1;

            delete_path = true;
        }

        if (order == ORDER_EXPLODE || unit->orders != ORDER_FIRE) {
            if (unit->orders != ORDER_NEW_ALLOCATE) {
                unit->prior_orders = unit->orders;
                unit->prior_state = unit->state;
            }

            unit->orders = order;
            unit->state = state;
        }

        if (delete_path) {
            PathsManager_RemoveRequest(unit);
        }
    }
}

void UnitsManager_UpdatePathsTimeLimit() {
    if (!UnitsManager_TimeBenchmarkInit) {
        for (int32_t i = 0; i < 20; ++i) {
            UnitsManager_TimeBenchmarkValues[i] = TIMER_FPS_TO_MS(30 / 1.1);
            UnitsManager_TimeBenchmarkIndices[i] = i;

            Paths_TimeBenchmarkDisable = false;
            UnitsManager_TimeBenchmarkInit = true;
        }
    }

    if (!Paths_TimeBenchmarkDisable) {
        uint8_t index = UnitsManager_TimeBenchmarkIndices[UnitsManager_TimeBenchmarkNextIndex];

        if (index < 19) {
            memmove(&UnitsManager_TimeBenchmarkIndices[index], &UnitsManager_TimeBenchmarkIndices[index + 1],
                    sizeof(UnitsManager_TimeBenchmarkIndices[0]) * (19 - index));

            uint32_t elapsed_time = timer_get() - Paths_LastTimeStamp;

            if (elapsed_time > TIMER_FPS_TO_MS(1)) {
                elapsed_time = TIMER_FPS_TO_MS(1);
            }

            UnitsManager_TimeBenchmarkValues[UnitsManager_TimeBenchmarkNextIndex] = elapsed_time;

            for (index = 0; index < 19; ++index) {
                if (UnitsManager_TimeBenchmarkValues[UnitsManager_TimeBenchmarkIndices[index]] >= elapsed_time) {
                    break;
                }
            }

            if (index < 19) {
                memmove(&UnitsManager_TimeBenchmarkIndices[index + 1], &UnitsManager_TimeBenchmarkIndices[index],
                        sizeof(UnitsManager_TimeBenchmarkIndices[0]) * (19 - index));
            }

            UnitsManager_TimeBenchmarkIndices[index] = UnitsManager_TimeBenchmarkNextIndex;

            UnitsManager_TimeBenchmarkNextIndex = (UnitsManager_TimeBenchmarkNextIndex + 1) % 20;

            uint32_t time_budget = (elapsed_time * 3) / 2;

            time_budget = std::max(time_budget, TIMER_FPS_TO_MS(50));
            time_budget = std::min(time_budget, TIMER_FPS_TO_MS(30));

            Paths_TimeLimit = time_budget + UnitsManager_TimeBenchmarkValues[UnitsManager_TimeBenchmarkIndices[10]];

            if (elapsed_time >= TIMER_FPS_TO_MS(30)) {
                elapsed_time *= 2;

            } else {
                elapsed_time += TIMER_FPS_TO_MS(30);
            }

            Paths_TimeLimit = std::min(Paths_TimeLimit, elapsed_time);
            Paths_TimeLimit = std::min(Paths_TimeLimit, TIMER_FPS_TO_MS(30 / 1.1));
        }
    }
}

void UnitsManager_SetNewOrder(UnitInfo* unit, int32_t order, int32_t state) {
    UnitsManager_SetNewOrderInt(unit, order, state);

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_08(unit);
    }
}

bool UnitsManager_IsUnitUnderWater(UnitInfo* unit) {
    bool result;

    if (unit->unit_type == SUBMARNE) {
        result = true;

    } else if (unit->unit_type == CLNTRANS) {
        if (Access_GetModifiedSurfaceType(unit->grid_x, unit->grid_y) == SURFACE_TYPE_WATER) {
            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void UnitsManager_UpdateConnectors(UnitInfo* unit) {
    unit->RefreshScreen();

    if (unit->flags & (CONNECTOR_UNIT | BUILDING | STANDALONE)) {
        if (!(unit->flags & GROUND_COVER)) {
            SmartPointer<UnitInfo> building;
            int32_t unit_size = (unit->flags & BUILDING) ? 2 : 1;
            int32_t grid_x = unit->grid_x;
            int32_t grid_y = unit->grid_y;
            CTInfo* team_info = &UnitsManager_TeamInfo[unit->team];

            if (UnitsManager_IsFactory(unit->unit_type)) {
                ++team_info->stats_factories_built;
            }

            if (unit->unit_type == MININGST) {
                ++team_info->stats_mines_built;
            }

            if (unit->flags & BUILDING) {
                ++team_info->stats_buildings_built;
            }

            building = Access_GetTeamBuilding(unit->team, grid_x, grid_y - 1);

            if (building) {
                unit->connectors |= CONNECTOR_NORTH_LEFT;

                if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                    building->connectors |= CONNECTOR_SOUTH_LEFT;

                } else if (building == Access_GetTeamBuilding(unit->team, grid_x + 1, grid_y - 1)) {
                    building->connectors |= CONNECTOR_SOUTH_LEFT;

                } else {
                    building->connectors |= CONNECTOR_SOUTH_RIGHT;
                }

                building->RefreshScreen();
            }

            building = Access_GetTeamBuilding(unit->team, grid_x, grid_y + unit_size);

            if (building) {
                unit->connectors |= CONNECTOR_SOUTH_LEFT;

                if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                    building->connectors |= CONNECTOR_NORTH_LEFT;

                } else if (building == Access_GetTeamBuilding(unit->team, grid_x + 1, grid_y + unit_size)) {
                    building->connectors |= CONNECTOR_NORTH_LEFT;

                } else {
                    building->connectors |= CONNECTOR_NORTH_RIGHT;
                }

                building->RefreshScreen();
            }

            if (unit->flags & BUILDING) {
                building = Access_GetTeamBuilding(unit->team, grid_x + 1, grid_y - 1);

                if (building) {
                    unit->connectors |= CONNECTOR_NORTH_RIGHT;

                    if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                        building->connectors |= CONNECTOR_SOUTH_LEFT;

                    } else if (building == Access_GetTeamBuilding(unit->team, grid_x, grid_y - 1)) {
                        building->connectors |= CONNECTOR_SOUTH_RIGHT;

                    } else {
                        building->connectors |= CONNECTOR_SOUTH_LEFT;
                    }

                    building->RefreshScreen();
                }

                building = Access_GetTeamBuilding(unit->team, grid_x + 1, grid_y + unit_size);

                if (building) {
                    unit->connectors |= CONNECTOR_SOUTH_RIGHT;

                    if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                        building->connectors |= CONNECTOR_NORTH_LEFT;

                    } else if (building == Access_GetTeamBuilding(unit->team, grid_x, grid_y + unit_size)) {
                        building->connectors |= CONNECTOR_NORTH_RIGHT;

                    } else {
                        building->connectors |= CONNECTOR_NORTH_LEFT;
                    }

                    building->RefreshScreen();
                }
            }

            building = Access_GetTeamBuilding(unit->team, grid_x + unit_size, grid_y);

            if (building) {
                unit->connectors |= CONNECTOR_EAST_TOP;

                if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                    building->connectors |= CONNECTOR_WEST_TOP;

                } else if (building == Access_GetTeamBuilding(unit->team, grid_x + unit_size, grid_y + 1)) {
                    building->connectors |= CONNECTOR_WEST_TOP;

                } else {
                    building->connectors |= CONNECTOR_WEST_BOTTOM;
                }

                building->RefreshScreen();
            }

            building = Access_GetTeamBuilding(unit->team, grid_x - 1, grid_y);

            if (building) {
                unit->connectors |= CONNECTOR_WEST_TOP;

                if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                    building->connectors |= CONNECTOR_EAST_TOP;

                } else if (building == Access_GetTeamBuilding(unit->team, grid_x - 1, grid_y + 1)) {
                    building->connectors |= CONNECTOR_EAST_TOP;

                } else {
                    building->connectors |= CONNECTOR_EAST_BOTTOM;
                }

                building->RefreshScreen();
            }

            if (unit->flags & BUILDING) {
                building = Access_GetTeamBuilding(unit->team, grid_x + unit_size, grid_y + 1);

                if (building) {
                    unit->connectors |= CONNECTOR_EAST_BOTTOM;

                    if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                        building->connectors |= CONNECTOR_WEST_TOP;

                    } else if (building == Access_GetTeamBuilding(unit->team, grid_x + unit_size, grid_y)) {
                        building->connectors |= CONNECTOR_WEST_BOTTOM;

                    } else {
                        building->connectors |= CONNECTOR_WEST_TOP;
                    }

                    building->RefreshScreen();
                }

                building = Access_GetTeamBuilding(unit->team, grid_x - 1, grid_y + 1);

                if (building) {
                    unit->connectors |= CONNECTOR_WEST_BOTTOM;

                    if (building->flags & (CONNECTOR_UNIT | STANDALONE)) {
                        building->connectors |= CONNECTOR_EAST_TOP;

                    } else if (building == Access_GetTeamBuilding(unit->team, grid_x - 1, grid_y)) {
                        building->connectors |= CONNECTOR_EAST_BOTTOM;

                    } else {
                        building->connectors |= CONNECTOR_EAST_TOP;
                    }

                    building->RefreshScreen();
                }
            }

            unit->AttachToPrimaryComplex();

            Access_UpdateResourcesTotal(unit->GetComplex());
        }
    }
}

void UnitsManager_DestroyUnit(UnitInfo* unit) {
    SmartPointer<UnitInfo> unit_to_destroy(unit);

    PathsManager_RemoveRequest(unit);

    if (unit_to_destroy == GameManager_SelectedUnit) {
        SoundManager.PlaySfx(unit, SFX_TYPE_INVALID);
        GameManager_SelectedUnit = nullptr;

        if (GameManager_IsMainMenuEnabled) {
            GameManager_MenuDeleteFlic();
            GameManager_FillOrRestoreWindow(WINDOW_CORNER_FLIC, COLOR_BLACK, true);
            GameManager_FillOrRestoreWindow(WINDOW_STAT_WINDOW, COLOR_BLACK, true);
            GameManager_DeinitPopupButtons(false);
            GameManager_UpdateDrawBounds();
        }
    }

    if (unit->prior_orders == ORDER_FIRE && unit->prior_state != ORDER_STATE_0) {
        unit->UpdatePinCount(unit->target_grid_x, unit->target_grid_y, -1);
    }

    if (GameManager_LockedUnits.Remove(*unit)) {
        GameManager_UpdateDrawBounds();
    }

    UnitsManager_DelayedAttackTargets[unit->team].Remove(*unit);

    unit_to_destroy->ClearUnitList();
    unit_to_destroy->SetParent(nullptr);
    unit_to_destroy->path = nullptr;

    unit->RemoveTasks();
    unit->RemoveDelayedTasks();

    Hash_MapHash.Remove(unit);

    unit->RemoveInTransitUnitFromMapHash();

    unit->RefreshScreen();

    UnitsManager_RemoveUnitFromUnitLists(unit);

    if (unit->GetId() != 0xFFFF || (unit->flags & (EXPLODING | MISSILE_UNIT))) {
        Access_UpdateMapStatus(unit, false);
    }

    Hash_UnitHash.Remove(unit);

    if (unit->GetId() != 0xFFFF && !(unit->flags & (EXPLODING | MISSILE_UNIT))) {
        Ai_RemoveUnit(unit);
    }
}

void UnitsManager_RemoveUnitFromUnitLists(UnitInfo* unit) {
    if (unit->flags & GROUND_COVER) {
        UnitsManager_GroundCoverUnits.Remove(*unit);

    } else if (unit->flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
        UnitsManager_MobileLandSeaUnits.Remove(*unit);

    } else if (unit->flags & STATIONARY) {
        UnitsManager_StationaryUnits.Remove(*unit);

    } else if (unit->flags & MOBILE_AIR_UNIT) {
        UnitsManager_MobileAirUnits.Remove(*unit);

    } else if (unit->flags & MISSILE_UNIT) {
        UnitsManager_ParticleUnits.Remove(*unit);
    }
}

SmartPointer<UnitInfo> UnitsManager_DeployUnit(ResourceID unit_type, uint16_t team, Complex* complex, int32_t grid_x,
                                               int32_t grid_y, uint8_t unit_angle, bool is_existing_unit,
                                               bool skip_map_status_update) {
    uint16_t id;
    UnitInfo* unit;
    BaseUnit* base_unit;

    id = 0xFFFF;

    if (!is_existing_unit) {
        ++UnitsManager_TeamInfo[team].number_of_objects_created;

        if (UnitsManager_TeamInfo[team].number_of_objects_created >= 0x1FFF) {
            UnitsManager_TeamInfo[team].number_of_objects_created = 1;
        }

        id = UnitsManager_TeamInfo[team].number_of_objects_created + (team << 13);
    }

    unit = new (std::nothrow) UnitInfo(unit_type, team, id, unit_angle);

    if (!is_existing_unit) {
        CTInfo* team_info;

        unit->GetBaseValues()->SetUnitsBuilt(1);

        team_info = &UnitsManager_TeamInfo[team];

        unit->unit_id = team_info->unit_counters[unit_type];

        ++team_info->unit_counters[unit_type];

        if (team_info->unit_counters[unit_type] == 0) {
            ++team_info->unit_counters[unit_type];
        }
    }

    if (GameManager_GameState != GAME_STATE_12 && !GameManager_UnknownFlag3 && !is_existing_unit) {
        unit->storage = 0;
    }

    if (unit->unit_type == COMMANDO) {
        unit->storage = unit->GetBaseValues()->GetAttribute(ATTRIB_AGENT_ADJUST);
    }

    if (unit->flags & ANIMATED) {
        unit->orders = ORDER_EXPLODE;
        unit->state = GAME_STATE_14;
    }

    if (!is_existing_unit) {
        switch (unit_type) {
            case SEATRANS:
            case AIRTRANS:
            case CLNTRANS: {
                unit->storage = 0;
            } break;

            case COMMTWR:
            case HABITAT:
            case MININGST: {
                unit->orders = ORDER_POWER_ON;
                unit->state = ORDER_STATE_0;
            } break;

            case LANDMINE:
            case SEAMINE:
            case GUNTURRT:
            case ANTIAIR:
            case ARTYTRRT:
            case ANTIMSSL:
            case RADAR: {
                unit->orders = ORDER_SENTRY;
                unit->state = ORDER_STATE_1;
            } break;

            case POWERSTN:
            case POWGEN:
            case RESEARCH: {
                unit->orders = ORDER_POWER_OFF;
                unit->state = ORDER_STATE_1;
            } break;

            case BARRACKS:
            case DEPOT:
            case HANGAR:
            case DOCK: {
                unit->storage = 0;
            } break;
        }
    }

    if (team == PLAYER_TEAM_ALIEN && (unit->flags & REGENERATING_UNIT)) {
        unit->orders = ORDER_DISABLE;
    }

    base_unit = &UnitsManager_BaseUnits[unit_type];

    if (unit->flags & (TURRET_SPRITE | SPINNING_TURRET)) {
        unit->UpdateTurretAngle(unit->angle);

        if (unit->flags & SPINNING_TURRET) {
            unit->image_index_max =
                unit->turret_image_base +
                reinterpret_cast<struct BaseUnitDataFile*>(base_unit->data_buffer)->turret_image_count - 1;
        }
    }

    if (unit_type == MININGST && !is_existing_unit) {
        UnitsManager_SetInitialMining(unit, grid_x, grid_y);
    }

    unit->SetPosition(grid_x, grid_y, skip_map_status_update);

    if (!is_existing_unit) {
        Hash_UnitHash.PushBack(unit);

        if (unit_type == DEPOT || unit_type == DOCK || unit_type == HANGAR || unit_type == BARRACKS) {
            unit->DrawSpriteFrame(unit->image_base + 1);
        }

        Ai_UpdateTerrain(unit);
    }

    return SmartPointer<UnitInfo>(unit);
}

void UnitsManager_FinishUnitScaling(UnitInfo* unit) {
    unit->RestoreOrders();

    if (unit->orders == ORDER_IDLE) {
        SmartPointer<UnitInfo> parent = unit->GetParent();

        if (parent->storage < parent->GetBaseValues()->GetAttribute(ATTRIB_STORAGE) && parent->team == unit->team) {
            unit->orders = ORDER_AWAIT;

            Access_UpdateMapStatus(unit, false);

            unit->orders = ORDER_IDLE;

            Hash_MapHash.Remove(unit);

            ++parent->storage;

            if (parent->flags & STATIONARY) {
                unit->ScheduleDelayedTasks(true);

            } else if (parent->GetTask()) {
                parent->GetTask()->EventUnitLoaded(*parent, *unit);
            }

        } else {
            unit->orders = ORDER_AWAIT;
            unit->state = ORDER_STATE_1;

            if (unit->flags & MOBILE_AIR_UNIT) {
                UnitsManager_SetNewOrderInt(unit, ORDER_TAKE_OFF, ORDER_STATE_0);
            }

            UnitsManager_ScaleUnit(unit, ORDER_STATE_EXPAND);
        }

        if (GameManager_SelectedUnit == parent) {
            GameManager_UpdateInfoDisplay(&*parent);
        }

        if (GameManager_SelectedUnit == unit) {
            SoundManager.PlaySfx(unit, SFX_TYPE_INVALID);

            GameManager_UpdateInfoDisplay(unit);
            GameManager_AutoSelectNext(unit);
        }

        unit->RefreshScreen();

        GameManager_RenderMinimapDisplay = true;
    }
}

void UnitsManager_NewOrderWhileScaling(UnitInfo* unit) {
    if (unit->orders == ORDER_AWAIT_SCALING) {
        if (unit->state == ORDER_STATE_EXPAND) {
            unit->scaler_adjust = 0;
            UnitsManager_UpdateConnectors(unit);

        } else {
            unit->scaler_adjust = 4;
            UnitsManager_FinishUnitScaling(unit);
        }
    }
}

void UnitsManager_ClearPins(SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        (*it).ClearPins();
    }
}

void UnitsManager_PerformAutoSurvey(UnitInfo* unit) {
    if (unit->auto_survey && unit->speed && GameManager_PlayMode != PLAY_MODE_UNKNOWN &&
        (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == unit->team)) {
        if (!Remote_IsNetworkGame || !UnitsManager_TeamInfo[unit->team].finished_turn) {
            if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER ||
                UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_REMOTE) {
                unit->auto_survey = false;

            } else {
                if (!unit->GetTask()) {
                    Ai_EnableAutoSurvey(unit);
                }

                unit->ScheduleDelayedTasks(false);
            }
        }
    }
}

void UnitsManager_ProcessOrderAwait(UnitInfo* unit) {
    if (unit->unit_type != ROAD && unit->unit_type != WTRPLTFM) {
        switch (unit->state) {
            case ORDER_STATE_1: {
                if (unit->unit_type == BRIDGE && unit->IsBridgeElevated()) {
                    if (!Access_GetUnit3(unit->grid_x, unit->grid_y, MOBILE_SEA_UNIT)) {
                        UnitsManager_SetNewOrderInt(unit, ORDER_MOVE, ORDER_STATE_LOWER);
                    }
                }

                UnitsManager_PerformAutoSurvey(unit);
                UnitsManager_Animate(unit);
            } break;

            case ORDER_STATE_2: {
                UnitsManager_PerformAutoSurvey(unit);
                UnitsManager_Animate(unit);
            } break;

            case ORDER_STATE_CLEAR_PATH: {
                unit->path = nullptr;
                unit->state = ORDER_STATE_1;

                if (GameManager_SelectedUnit == unit) {
                    GameManager_UpdateInfoDisplay(unit);
                    GameManager_UpdateDrawBounds();
                }

                if (unit->unit_type == BRIDGE && unit->IsBridgeElevated() &&
                    !Access_GetUnit3(unit->grid_x, unit->grid_y, MOBILE_SEA_UNIT)) {
                    UnitsManager_SetNewOrderInt(unit, ORDER_MOVE, ORDER_STATE_LOWER);
                }

                UnitsManager_PerformAutoSurvey(unit);
                UnitsManager_Animate(unit);
            } break;
        }
    }
}

void UnitsManager_ProcessOrderTransform(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Transform");
    unit->RefreshScreen();

    switch (unit->state) {
        case ORDER_STATE_0: {
            UnitsManager_DeployMasterBuilderInit(unit);
        } break;

        case ORDER_STATE_22: {
            UnitsManager_DeployMasterBuilder(unit);
        } break;

        case ORDER_STATE_23: {
            unit->orders = ORDER_POWER_ON;
            unit->state = ORDER_STATE_0;
            UnitsManager_TeamInfo[unit->team].team_type = TEAM_TYPE_PLAYER;

            GameManager_OptimizeProduction(unit->team, unit->GetComplex(), true, true);
            GameManager_AutoSelectNext(unit);
            GameManager_EnableMainMenu(nullptr);
        } break;
    }
}

void UnitsManager_ProcessOrderMove(UnitInfo* unit) {
    if (unit->state == ORDER_STATE_7) {
        unit->Redraw();
        unit->state = ORDER_STATE_5;
    }

    switch (unit->state) {
        case ORDER_STATE_1: {
            if (!UnitsManager_PursueEnemy(unit)) {
                UnitsManager_PerformAutoSurvey(unit);
                UnitsManager_Animate(unit);
            }
        } break;

        case ORDER_STATE_3: {
            Ai_SetTasksPendingFlag("Storing");
            UnitsManager_Store(unit);
        } break;

        case ORDER_STATE_5: {
            if (unit->ExpectAttack()) {
                Ai_SetTasksPendingFlag("Moving");
                unit->Move();
            }
        } break;

        case ORDER_STATE_6: {
            Ai_SetTasksPendingFlag("Moving");
            unit->Move();
        } break;

        case ORDER_STATE_12: {
            unit->SetEnemy(nullptr);
            unit->BlockedOnPathRequest(false);
        } break;

        case ORDER_STATE_0:
        case ORDER_STATE_28: {
            if (unit->IsVisibleToTeam(GameManager_PlayerTeam)) {
                if (GameManager_SelectedUnit == unit || GameManager_DisplayButtonRange ||
                    GameManager_DisplayButtonScan) {
                    GameManager_UpdateDrawBounds();
                }
            }

            if (UnitsManager_UpdateAttackMoves(unit)) {
                if (unit->ExpectAttack()) {
                    Ai_SetTasksPendingFlag("Moving");
                    unit->Move();
                }
            }
        } break;

        case ORDER_STATE_29: {
            if (!UnitsManager_PursueEnemy(unit)) {
                UnitsManager_PerformAutoSurvey(unit);
                UnitsManager_Animate(unit);
            }
        } break;

        case ORDER_STATE_ELEVATE: {
            if (unit->GetImageIndex() == unit->image_index_max) {
                UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT, ORDER_STATE_1);

            } else {
                if (!unit->IsBridgeElevated()) {
                    UnitsManager_GroundCoverUnits.Remove(*unit);
                    unit->AddToDrawList(STATIONARY | UPGRADABLE | SELECTABLE);
                }

                unit->DrawSpriteFrame(unit->GetImageIndex() + 1);
            }
        } break;

        case ORDER_STATE_LOWER: {
            if (Access_GetUnit3(unit->grid_x, unit->grid_y, MOBILE_SEA_UNIT)) {
                UnitsManager_SetNewOrderInt(unit, ORDER_MOVE, ORDER_STATE_ELEVATE);

            } else {
                if (unit->IsBridgeElevated()) {
                    unit->DrawSpriteFrame(unit->GetImageIndex() - 1);

                } else {
                    UnitsManager_StationaryUnits.Remove(*unit);
                    unit->AddToDrawList();
                    UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT, ORDER_STATE_1);
                }
            }
        } break;

        case ORDER_STATE_NEW_ORDER: {
            if (unit->flags & MOBILE_AIR_UNIT) {
                if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER && unit->GetUnitList()) {
                    UnitsManager_Unit = unit;
                }
            }

            if (!UnitsManager_PursueEnemy(unit)) {
                UnitsManager_PerformAutoSurvey(unit);
                UnitsManager_Animate(unit);
            }
        } break;
    }
}

void UnitsManager_ProcessOrderFire(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Firing");

    UnitsManager_byte_179448 = true;

    switch (unit->state) {
        case ORDER_STATE_0: {
            UnitsManager_UnitList6.PushBack(*unit);

            unit->UpdatePinCount(unit->target_grid_x, unit->target_grid_y, 1);

            unit->state = ORDER_STATE_40;
        } break;

        case ORDER_STATE_5: {
            UnitsManager_OrdersPending = true;

            if (UnitsManager_BeginFire(unit)) {
                UnitsManager_OrdersPending = true;
                unit->PrepareFire();
            }
        } break;

        case ORDER_STATE_8: {
            UnitsManager_OrdersPending = true;
            unit->PrepareFire();
        } break;

        case ORDER_STATE_9: {
            UnitsManager_OrdersPending = true;
            unit->ProgressFire();
        } break;

        case ORDER_STATE_40: {
        } break;

        case ORDER_STATE_41: {
            unit->state = ORDER_STATE_5;

            if (GameManager_SelectedUnit == unit) {
                SoundManager.PlaySfx(unit, SFX_TYPE_TURRET);
            }

            UnitsManager_OrdersPending = true;

            if (UnitsManager_BeginFire(unit)) {
                UnitsManager_OrdersPending = true;
                unit->PrepareFire();
            }
        } break;
    }
}

void UnitsManager_ProcessOrderBuild(UnitInfo* unit) {
    switch (unit->state) {
        case ORDER_STATE_0: {
            Ai_SetTasksPendingFlag("Build start");

            unit->StartBuilding();
        } break;

        case ORDER_STATE_5: {
            if (unit->ExpectAttack()) {
                Ai_SetTasksPendingFlag("Moving");

                unit->Move();
            }
        } break;

        case ORDER_STATE_6: {
            Ai_SetTasksPendingFlag("Moving");

            unit->Move();
        } break;

        case ORDER_STATE_11: {
            if (ini_get_setting(INI_EFFECTS) && unit->unit_type == CONSTRCT) {
                if (unit->moved & 0x01) {
                    if (unit->GetImageIndex() + 8 >= 40) {
                        unit->DrawSpriteFrame(unit->GetImageIndex() - 16);

                    } else {
                        unit->DrawSpriteFrame(unit->GetImageIndex() + 8);
                    }
                }

                ++unit->moved;
            }

            if (GameManager_SelectedUnit == unit) {
                SoundManager.PlaySfx(unit, SFX_TYPE_BUILDING);
            }
        } break;

        case ORDER_STATE_13: {
            Ai_SetTasksPendingFlag("Build cancel");

            unit->CancelBuilding();
        } break;

        case ORDER_STATE_25: {
            UnitsManager_Animate(unit);
        } break;

        case ORDER_STATE_26: {
            Ai_SetTasksPendingFlag("Build clearing");

            UnitsManager_BuildClearing(unit, false);
        } break;

        case ORDER_STATE_UNIT_READY: {
            if (unit->path) {
                UnitsManager_BuildNext(unit);

            } else {
                UnitsManager_Animate(unit);
            }
        } break;

        case ORDER_STATE_46: {
            Ai_SetTasksPendingFlag("Build cancel");

            unit->CancelBuilding();
        } break;
    }
}

void UnitsManager_ProcessOrderActivate(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Activation");

    switch (unit->state) {
        case ORDER_STATE_1: {
            UnitsManager_ActivateUnit(unit);
        } break;

        case ORDER_STATE_6: {
            unit->GetParent()->RefreshScreen();

            if (unit->unit_type == CONSTRCT) {
                unit->state = ORDER_STATE_11;

                UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_36);

            } else {
                UnitsManager_ActivateEngineer(unit);
            }
        } break;

        case ORDER_STATE_11: {
            UnitsManager_ActivateEngineer(unit);
        } break;
    }
}

void UnitsManager_ProcessOrderNewAllocate(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("New allocation");

    unit->RestoreOrders();

    if (GameManager_SelectedUnit == unit) {
        GameManager_MenuUnitSelect(unit);
    }
}

void UnitsManager_ProcessOrderPowerOn(UnitInfo* unit) {
    if (unit->state == ORDER_STATE_0) {
        Ai_SetTasksPendingFlag("Power up");

        if (GameManager_SelectedUnit == unit) {
            SoundManager.PlaySfx(unit, SFX_TYPE_POWER_CONSUMPTION_START);
        }

        UnitsManager_PowerUpUnit(unit, -1);
        unit->DrawSpriteFrame(unit->image_base + 1);

        if (unit->unit_type == RESEARCH) {
            ResearchMenu_UpdateResearchProgress(unit->team, unit->research_topic, 1);
        }
    }

    UnitsManager_Animate(unit);
}

void UnitsManager_ProcessOrderPowerOff(UnitInfo* unit) {
    if (unit->state == ORDER_STATE_0) {
        UnitsManager_PowerDownUnit(unit);
    }

    UnitsManager_Animate(unit);
}

void UnitsManager_ProcessOrderExplode(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Exploding");

    switch (unit->state) {
        case ORDER_STATE_0: {
            UnitsManager_StartExplosion(unit);
        } break;

        case ORDER_STATE_14: {
            UnitsManager_ProgressExplosion(unit);
        } break;

        case ORDER_STATE_27: {
            unit->hits = 0;
            ++UnitsManager_TeamInfo[unit->team].casualties[unit->unit_type];

            UnitsManager_CheckIfUnitDestroyed(unit);

            UnitsManager_StartExplosion(unit);
        } break;
    }
}

void UnitsManager_ProcessOrderUnload(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Unloading");

    switch (unit->state) {
        case ORDER_STATE_0: {
            unit->moved = 0;
            unit->state = ORDER_STATE_17;

            if (Paths_IsOccupied(unit->grid_x, unit->grid_y, 0, unit->team)) {
                unit->orders = ORDER_AWAIT;
                unit->state = ORDER_STATE_1;

            } else {
                UnitsManager_ProgressUnloading(unit);
            }
        } break;

        case ORDER_STATE_17: {
            UnitsManager_ProgressUnloading(unit);
        } break;

        case ORDER_STATE_18: {
            UnitsManager_Unloading(unit);
        } break;
    }
}

void UnitsManager_ProcessOrderClear(UnitInfo* unit) {
    if (unit->state == ORDER_STATE_0) {
        Ai_SetTasksPendingFlag("Clear start");

        UnitsManager_StartClearing(unit);
    }
}

void UnitsManager_ProcessOrderSentry(UnitInfo* unit) {
    if ((unit->flags & ANIMATED) && ini_get_setting(INI_EFFECTS)) {
        if (unit->GetImageIndex() + 1 <= unit->image_index_max) {
            unit->DrawSpriteFrame(unit->GetImageIndex() + 1);

        } else {
            unit->DrawSpriteFrame(unit->image_base);
        }

    } else {
        UnitsManager_Animate(unit);
    }
}

void UnitsManager_ProcessOrderLand(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Landing");

    switch (unit->state) {
        case ORDER_STATE_0: {
            unit->moved = 0;
            unit->state = ORDER_STATE_19;

            UnitsManager_Landing(unit);
        } break;

        case ORDER_STATE_19: {
            UnitsManager_Landing(unit);
        } break;
    }
}

void UnitsManager_ProcessOrderTakeOff(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Taking off");

    switch (unit->state) {
        case ORDER_STATE_0: {
            unit->moved = 0;
            unit->state = ORDER_STATE_20;

            if (unit->Take()) {
                UnitsManager_RemoveUnitFromUnitLists(unit);

                unit->AddToDrawList();
                unit->RestoreOrders();
            }
        } break;

        case ORDER_STATE_20: {
            if (unit->Take()) {
                UnitsManager_RemoveUnitFromUnitLists(unit);

                unit->AddToDrawList();
                unit->RestoreOrders();
            }
        } break;
    }
}

void UnitsManager_ProcessOrderLoad(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Loading");

    switch (unit->state) {
        case ORDER_STATE_0: {
            unit->moved = 0;
            unit->state = ORDER_STATE_15;

            UnitsManager_ScaleUnit(unit->GetParent(), ORDER_STATE_SHRINK);

            UnitsManager_ProgressLoading(unit);
        } break;

        case ORDER_STATE_15: {
            UnitsManager_ProgressLoading(unit);
        } break;

        case ORDER_STATE_16: {
            UnitsManager_Loading(unit);
        } break;
    }
}

void UnitsManager_ProcessOrderIdle(UnitInfo* unit) {
    if (unit->state == ORDER_STATE_BUILDING_READY) {
        UnitsManager_BuildingReady(unit);
    }
}

void UnitsManager_ProcessOrderRepair(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Repairing");

    UnitsManager_Repair(unit);
}

void UnitsManager_ProcessOrderRefuel(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Refuelling");

    unit->Refuel(unit->GetParent());
}

void UnitsManager_ProcessOrderReload(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Reloading");

    unit->Reload(unit->GetParent());
}

void UnitsManager_ProcessOrderTransfer(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Tranferring");

    UnitsManager_Transfer(unit);
}

void UnitsManager_ProcessOrderHaltBuilding(UnitInfo* unit) {
    if (unit->state == ORDER_STATE_13) {
        Ai_SetTasksPendingFlag("Build halting");

        unit->CancelBuilding();
    }
}

void UnitsManager_ProcessOrderAwaitScaling(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Scaling");

    unit->RefreshScreen();

    if (unit->prior_orders == ORDER_AWAIT_DISABLE_UNIT || unit->prior_orders == ORDER_AWAIT_STEAL_UNIT) {
        UnitsManager_byte_179448 = true;
    }

    switch (unit->state) {
        case ORDER_STATE_EXPAND: {
            if (unit->scaler_adjust) {
                --unit->scaler_adjust;

            } else {
                unit->RestoreOrders();
                UnitsManager_UpdateConnectors(unit);

                if (unit->GetTask()) {
                    unit->GetTask()->AddUnit(*unit);
                }

                unit->ScheduleDelayedTasks(true);
            }
        } break;

        case ORDER_STATE_SHRINK: {
            if (unit->scaler_adjust == 4) {
                UnitsManager_FinishUnitScaling(unit);

            } else {
                ++unit->scaler_adjust;
            }
        } break;
    }
}

void UnitsManager_ProcessOrderAwaitTapePositioning(UnitInfo* unit) { unit->PositionInTape(); }

void UnitsManager_ProcessOrderAwaitDisableStealUnit(UnitInfo* unit) {
    UnitsManager_byte_179448 = true;

    Ai_SetTasksPendingFlag("Disable/Steal");

    switch (unit->state) {
        case ORDER_STATE_0: {
            unit->state = ORDER_STATE_5;

            UnitsManager_ScaleUnit(unit, ORDER_STATE_SHRINK);
        } break;

        case ORDER_STATE_1: {
            if (UnitsManager_AttemptStealthAction(unit)) {
                if (unit->orders == ORDER_AWAIT_STEAL_UNIT) {
                    UnitsManager_CaptureUnit(unit);

                } else {
                    UnitsManager_DisableUnit(unit);
                }
            }

            unit->orders = ORDER_AWAIT;
        } break;

        case ORDER_STATE_5: {
            unit->GetParent()->ShakeSabotage();

            if (unit->GetParent()->shake_effect_state == 0) {
                unit->state = ORDER_STATE_1;

                UnitsManager_ScaleUnit(unit, ORDER_STATE_EXPAND);
            }
        } break;
    }
}

void UnitsManager_ProcessOrderDisable(UnitInfo* unit) { UnitsManager_Animate(unit); }

void UnitsManager_ProcessOrderUpgrade(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Upgrading");

    unit->Upgrade(unit->GetParent());
}

void UnitsManager_ProcessOrderLayMine(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("lay mines");

    switch (unit->state) {
        case ORDER_STATE_INACTIVE: {
            unit->SetLayingState(0);
        } break;

        case ORDER_STATE_PLACING_MINES: {
            unit->SetLayingState(2);
            unit->PlaceMine();
        } break;

        case ORDER_STATE_REMOVING_MINES: {
            unit->SetLayingState(1);
            unit->PickUpMine();
        } break;
    }

    unit->RestoreOrders();
    unit->ScheduleDelayedTasks(true);
}

void UnitsManager_ProcessOrder(UnitInfo* unit) {
    if ((unit->flags & SPINNING_TURRET) && unit->orders != ORDER_DISABLE && ini_get_setting(INI_EFFECTS)) {
        unit->SpinningTurretAdvanceAnimation();
    }

    if ((unit->flags & MISSILE_UNIT) && unit->state == ORDER_STATE_14) {
        UnitsManager_OrdersPending = true;
        UnitsManager_byte_179448 = true;
    }

    switch (unit->orders) {
        case ORDER_AWAIT: {
            UnitsManager_ProcessOrderAwait(unit);

        } break;

        case ORDER_TRANSFORM: {
            UnitsManager_ProcessOrderTransform(unit);
        } break;

        case ORDER_MOVE: {
            UnitsManager_ProcessOrderMove(unit);
        } break;

        case ORDER_FIRE: {
            UnitsManager_ProcessOrderFire(unit);
        } break;

        case ORDER_BUILD: {
            UnitsManager_ProcessOrderBuild(unit);
        } break;

        case ORDER_ACTIVATE: {
            UnitsManager_ProcessOrderActivate(unit);
        } break;

        case ORDER_NEW_ALLOCATE: {
            UnitsManager_ProcessOrderNewAllocate(unit);
        } break;

        case ORDER_POWER_ON: {
            UnitsManager_ProcessOrderPowerOn(unit);
        } break;

        case ORDER_POWER_OFF: {
            UnitsManager_ProcessOrderPowerOff(unit);
        } break;

        case ORDER_EXPLODE: {
            UnitsManager_ProcessOrderExplode(unit);
        } break;

        case ORDER_UNLOAD: {
            UnitsManager_ProcessOrderUnload(unit);
        } break;

        case ORDER_CLEAR: {
            UnitsManager_ProcessOrderClear(unit);
        } break;

        case ORDER_SENTRY: {
            UnitsManager_ProcessOrderSentry(unit);
        } break;

        case ORDER_LAND: {
            UnitsManager_ProcessOrderLand(unit);
        } break;

        case ORDER_TAKE_OFF: {
            UnitsManager_ProcessOrderTakeOff(unit);
        } break;

        case ORDER_LOAD: {
            UnitsManager_ProcessOrderLoad(unit);
        } break;

        case ORDER_IDLE: {
            UnitsManager_ProcessOrderIdle(unit);
        } break;

        case ORDER_REPAIR: {
            UnitsManager_ProcessOrderRepair(unit);
        } break;

        case ORDER_REFUEL: {
            UnitsManager_ProcessOrderRefuel(unit);
        } break;

        case ORDER_RELOAD: {
            UnitsManager_ProcessOrderReload(unit);
        } break;

        case ORDER_TRANSFER: {
            UnitsManager_ProcessOrderTransfer(unit);
        } break;

        case ORDER_HALT_BUILDING: {
            UnitsManager_ProcessOrderHaltBuilding(unit);
        } break;

        case ORDER_AWAIT_SCALING: {
            UnitsManager_ProcessOrderAwaitScaling(unit);
        } break;

        case ORDER_AWAIT_TAPE_POSITIONING: {
            UnitsManager_ProcessOrderAwaitTapePositioning(unit);
        } break;

        case ORDER_AWAIT_STEAL_UNIT: {
            UnitsManager_ProcessOrderAwaitDisableStealUnit(unit);
        } break;

        case ORDER_AWAIT_DISABLE_UNIT: {
            UnitsManager_ProcessOrderAwaitDisableStealUnit(unit);
        } break;

        case ORDER_DISABLE: {
            UnitsManager_ProcessOrderDisable(unit);
        } break;

        case ORDER_MOVE_TO_UNIT: {
            UnitsManager_ProcessOrderMove(unit);
        } break;

        case ORDER_UPGRADE: {
            UnitsManager_ProcessOrderUpgrade(unit);
        } break;

        case ORDER_LAY_MINE: {
            UnitsManager_ProcessOrderLayMine(unit);
        } break;

        case ORDER_MOVE_TO_ATTACK: {
            UnitsManager_ProcessOrderMove(unit);
        } break;

        case ORDER_HALT_BUILDING_2: {
            UnitsManager_ProcessOrderHaltBuilding(unit);
        } break;
    }
}

void UnitsManager_ProcessOrders(SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        UnitsManager_ProcessOrder(&*it);
    }
}

void UnitsManager_CheckIfUnitDestroyed(UnitInfo* unit) {
    if (unit->hits == 0) {
        unit->engine = 0;
        unit->weapon = 0;
        unit->comm = 0;
    }
}

void UnitsManager_ActivateEngineer(UnitInfo* unit) {
    Access_DestroyUtilities(unit->grid_x, unit->grid_y, false, false, false, false);
    unit->prior_orders = ORDER_AWAIT;
    unit->prior_state = ORDER_STATE_1;
    unit->orders = ORDER_MOVE;
    unit->state = ORDER_STATE_0;
    unit->GetParent()->SetParent(unit);
    unit->SetParent(nullptr);
}

void UnitsManager_DeployMasterBuilderInit(UnitInfo* unit) {
    if (GameManager_SelectedUnit == unit) {
        GameManager_DisableMainMenu();
    }

    unit->DeployConstructionSiteMarkers(MININGST);
    unit->state = ORDER_STATE_22;

    UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_34);
}

bool UnitsManager_IsFactory(ResourceID unit_type) {
    return unit_type == SHIPYARD || unit_type == LIGHTPLT || unit_type == LANDPLT || unit_type == AIRPLT;
}

void UnitsManager_AddToDelayedReactionList(UnitInfo* unit) {
    if (!ini_get_setting(INI_DISABLE_FIRE)) {
        UnitsManager_DelayedAttackTargets[unit->team].PushBack(*unit);
        UnitsManager_byte_178170 = true;
    }
}

void UnitsManager_ClearDelayedReaction(SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).delayed_reaction && (*it).hits > 0) {
            (*it).delayed_reaction = 0;
        }
    }
}

void UnitsManager_ClearDelayedReactions() {
    UnitsManager_byte_178170 = false;

    UnitsManager_ClearDelayedReaction(&UnitsManager_MobileLandSeaUnits);
    UnitsManager_ClearDelayedReaction(&UnitsManager_MobileAirUnits);

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        UnitsManager_DelayedAttackTargets[team].Clear();
    }
}

void UnitsManager_Landing(UnitInfo* unit) {
    if (unit->Land()) {
        if (unit->GetParent()) {
            unit->orders = ORDER_IDLE;
            unit->state = ORDER_STATE_3;

            UnitsManager_ScaleUnit(unit, ORDER_STATE_SHRINK);

        } else {
            UnitsManager_RemoveUnitFromUnitLists(unit);
            unit->AddToDrawList();

            unit->orders = ORDER_AWAIT;
            unit->state = ORDER_STATE_1;
        }

        if (unit->IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy) {
            unit->RefreshScreen();
        }
    }
}

void UnitsManager_Loading(UnitInfo* unit) {
    if (unit->Take()) {
        SmartPointer<UnitInfo> parent = unit->GetParent();
        BaseUnit* base_unit = &UnitsManager_BaseUnits[parent->unit_type];

        unit->orders = ORDER_AWAIT;
        unit->state = ORDER_STATE_1;
        unit->SetParent(nullptr);

        if (unit->GetTask()) {
            unit->GetTask()->EventUnitLoaded(*unit, *parent);
        }

        if (GameManager_SelectedUnit == unit) {
            char message[400];

            sprintf(message, _(c15c), base_unit->singular_name, parent->unit_id);

            MessageManager_DrawMessage(message, 0, 0);
        }
    }
}

void UnitsManager_Unloading(UnitInfo* unit) {
    if (unit->Take()) {
        SmartPointer<UnitInfo> parent = unit->GetParent();
        BaseUnit* base_unit = &UnitsManager_BaseUnits[parent->unit_type];

        unit->orders = ORDER_AWAIT;
        unit->state = ORDER_STATE_1;
        unit->SetParent(nullptr);

        if (unit->GetTask()) {
            unit->GetTask()->EventUnitUnloaded(*unit, *parent);
        }

        if (GameManager_SelectedUnit == unit) {
            char message[400];

            sprintf(message, _(60f3), base_unit->singular_name, parent->unit_id);

            MessageManager_DrawMessage(message, 0, 0);
        }
    }
}

void UnitsManager_PowerUpUnit(UnitInfo* unit, int32_t factor) {
    Cargo_UpdateResourceLevels(unit, factor);

    unit->state = ORDER_STATE_1;

    if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) {
        GameManager_OptimizeProduction(unit->team, unit->GetComplex(), false, true);

        if (unit->IsVisibleToTeam(GameManager_PlayerTeam)) {
            unit->RefreshScreen();
        }

        if (GameManager_SelectedUnit == unit) {
            GameManager_UpdateInfoDisplay(unit);
        }

    } else {
        if (GameManager_SelectedUnit == unit) {
            GameManager_UpdateInfoDisplay(unit);

            if (GameManager_PlayerTeam == unit->team) {
                GameManager_OptimizeProduction(unit->team, unit->GetComplex(), true, true);
            }

            GameManager_AutoSelectNext(unit);
        }
    }
}

void UnitsManager_PowerDownUnit(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Power Down");

    if (GameManager_SelectedUnit == unit) {
        SoundManager.PlaySfx(unit, SFX_TYPE_POWER_CONSUMPTION_END);
    }

    UnitsManager_PowerUpUnit(unit, 1);

    unit->DrawSpriteFrame(unit->image_base);

    if (unit->unit_type == RESEARCH) {
        ResearchMenu_UpdateResearchProgress(unit->team, unit->research_topic, -1);
    }
}

void UnitsManager_Animate(UnitInfo* unit) {
    if (ini_get_setting(INI_EFFECTS)) {
        bool is_unit_moved = false;

        if (unit->flags & HOVERING) {
            is_unit_moved = unit->ShakeAir();
        }

        if (((unit->flags & MOBILE_SEA_UNIT) || unit->unit_type == SEAMINE) &&
            Access_GetModifiedSurfaceType(unit->grid_x, unit->grid_y) == SURFACE_TYPE_WATER) {
            is_unit_moved = unit->ShakeWater();
        }

        if (GameManager_SelectedUnit == unit || is_unit_moved) {
            int32_t unit_size = (unit->flags & BUILDING) ? 63 : 31;
            Rect bounds;

            bounds.ulx = unit->x - unit_size;
            bounds.uly = unit->y - unit_size;
            bounds.lrx = bounds.ulx + 1 + unit_size * 2;
            bounds.lry = bounds.uly + 1 + unit_size * 2;

            if (is_unit_moved) {
                bounds.ulx -= 3;
                bounds.uly -= 3;
                bounds.lrx += 3;
                bounds.lry += 3;

                if (GameManager_IsAtGridPosition(unit)) {
                    --UnitsManager_EffectCounter;
                }
            }

            GameManager_AddDrawBounds(&bounds);
        }
    }
}

void UnitsManager_DeployMasterBuilder(UnitInfo* unit) {
    uint16_t unit_team = unit->team;
    SmartPointer<Task> unit_task(unit->GetTask());
    SmartPointer<UnitInfo> mining_station;
    SmartPointer<UnitInfo> power_generator;
    SmartPointer<UnitInfo> small_slab;

    int32_t mining_station_grid_x = unit->target_grid_x;
    int32_t mining_station_grid_y = unit->target_grid_y;

    UnitsManager_DestroyUnit(unit);

    Access_DestroyUtilities(mining_station_grid_x, mining_station_grid_y, false, false, false, true);

    mining_station =
        UnitsManager_DeployUnit(MININGST, unit_team, nullptr, mining_station_grid_x, mining_station_grid_y, 0);

    int16_t power_generator_grid_x = mining_station_grid_x;
    int16_t power_generator_grid_y = mining_station_grid_y;

    UnitsManager_IsPowerGeneratorPlaceable(unit_team, &power_generator_grid_x, &power_generator_grid_y);

    power_generator = UnitsManager_DeployUnit(POWGEN, unit_team, mining_station->GetComplex(), mining_station_grid_x,
                                              mining_station_grid_y, 0);

    small_slab = UnitsManager_DeployUnit(
        SMLSLAB, unit_team, nullptr, power_generator_grid_x, power_generator_grid_y,
        (dos_rand() *
         reinterpret_cast<struct BaseUnitDataFile*>(UnitsManager_BaseUnits[SMLSLAB].data_buffer)->image_count) >>
            15);

    UnitsManager_SetInitialMining(&*mining_station, mining_station_grid_x, mining_station_grid_y);

    mining_station->scaler_adjust = 4;
    small_slab->scaler_adjust = 4;
    power_generator->scaler_adjust = 4;

    mining_station->orders = ORDER_TRANSFORM;
    mining_station->state = ORDER_STATE_23;

    UnitsManager_ScaleUnit(&*mining_station, ORDER_STATE_EXPAND);
    UnitsManager_ScaleUnit(&*small_slab, ORDER_STATE_EXPAND);
    UnitsManager_ScaleUnit(&*power_generator, ORDER_STATE_EXPAND);

    if (unit_task) {
        mining_station->AddTask(&*unit_task);
        unit_task->AddUnit(*power_generator);
    }
}

bool UnitsManager_PursueEnemy(UnitInfo* unit) {
    bool result;

    if (unit->orders == ORDER_MOVE_TO_ATTACK && unit->state == ORDER_STATE_1 && !unit->delayed_reaction &&
        !UnitsManager_Units[unit->team]) {
        UnitInfo* enemy_unit = unit->GetEnemy();
        int32_t unit_range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
        Point position;

        if (enemy_unit) {
            position = UnitsManager_GetAttackPosition(unit, enemy_unit);

            if (enemy_unit->hits > 0 && unit->ammo > 0 && unit->team != enemy_unit->team &&
                enemy_unit->orders != ORDER_IDLE &&
                (enemy_unit->IsVisibleToTeam(unit->team) || !enemy_unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED))) {
                if ((UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER ||
                     !enemy_unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED)) &&
                    Access_GetDistance(unit, position) > unit_range * unit_range) {
                    enemy_unit = nullptr;

                } else if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_ELIMINATED) {
                    enemy_unit = nullptr;
                }

            } else {
                unit->ScheduleDelayedTasks(true);
                enemy_unit = nullptr;
            }
        }

        if (enemy_unit) {
            if (Access_GetDistance(unit, position) > unit_range * unit_range &&
                (!unit->path || unit->target_grid_x != enemy_unit->grid_x ||
                 unit->target_grid_y != enemy_unit->grid_y) &&
                ((enemy_unit->orders != ORDER_MOVE && enemy_unit->orders != ORDER_MOVE_TO_UNIT &&
                  enemy_unit->orders != ORDER_MOVE_TO_ATTACK) ||
                 enemy_unit->state == ORDER_STATE_1)) {
                if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_REMOTE) {
                    result = false;

                } else {
                    unit->orders = ORDER_AWAIT;

                    UnitsManager_SetNewOrder(unit, ORDER_MOVE_TO_ATTACK, ORDER_STATE_28);

                    result = true;
                }

            } else {
                if (unit->shots > 0 && Access_IsWithinAttackRange(unit, position.x, position.y, unit_range)) {
                    if (UnitsManager_ParticleUnits.GetCount() > 0) {
                        result = false;

                    } else {
                        UnitsManager_Units[unit->team] = unit;

                        UnitsManager_byte_178170 = true;

                        result = true;
                    }

                } else {
                    result = false;
                }
            }

        } else {
            if (UnitsManager_TeamInfo[unit->team].team_type != TEAM_TYPE_REMOTE) {
                unit->SetEnemy(nullptr);

                UnitsManager_SetNewOrder(unit, ORDER_AWAIT, ORDER_STATE_CLEAR_PATH);

                unit->ScheduleDelayedTasks(false);

                if (GameManager_SelectedUnit == unit) {
                    GameManager_UpdateInfoDisplay(unit);
                }
            }

            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool UnitsManager_UpdateAttackMoves(UnitInfo* unit) {
    unit->RefreshScreen();
    UnitsManager_UpdateAttackPaths(unit);

    if (unit->state == ORDER_STATE_0) {
        unit->FollowUnit();
        unit->Redraw();

        unit->state = ORDER_STATE_5;

        if (Remote_IsNetworkGame && (unit->flags & MOBILE_AIR_UNIT)) {
            Remote_SendNetPacket_38(unit);

            return false;
        }
    }

    return (unit->state == ORDER_STATE_5);
}

void UnitsManager_Store(UnitInfo* unit) {
    SmartPointer<UnitInfo> parent(unit->GetParent());

    SDL_assert(parent->storage < parent->GetBaseValues()->GetAttribute(ATTRIB_STORAGE));

    ++parent->storage;

    if (GameManager_SelectedUnit == parent) {
        GameManager_UpdateInfoDisplay(&*parent);

        if (GameManager_PlayerTeam == parent->team) {
            GameManager_OptimizeProduction(parent->team, parent->GetComplex(), true, true);
        }
    }

    unit->orders = ORDER_IDLE;
    unit->state = ORDER_STATE_3;

    unit->ClearUnitList();
}

bool UnitsManager_BeginFire(UnitInfo* unit) {
    bool result;
    int32_t unit_angle;

    if (unit->flags & TURRET_SPRITE) {
        unit_angle = unit->turret_angle;

    } else {
        unit_angle = unit->angle;
    }

    int32_t distance_x = unit->target_grid_x - unit->grid_x;
    int32_t distance_y = unit->target_grid_y - unit->grid_y;
    int32_t target_angle = UnitsManager_GetTargetAngle(distance_x, distance_y);

    if (unit_angle == target_angle) {
        unit->state = ORDER_STATE_8;

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (unit->team != team && UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
                UnitsManager_TeamInfo[team]
                    .heat_map_complete[unit->grid_y * ResourceManager_MapSize.x + unit->grid_x]) {
                unit->SpotByTeam(team);
            }
        }

        result = true;

    } else {
        if (((target_angle - unit_angle + 8) & 0x07) > 4) {
            target_angle = -1;

        } else {
            target_angle = 1;
        }

        unit_angle = ((unit_angle + target_angle) & 0x07);

        if (unit->flags & TURRET_SPRITE) {
            unit->UpdateTurretAngle(unit_angle, true);

        } else {
            unit->UpdateAngle(unit_angle);
        }

        result = false;
    }

    return result;
}

void UnitsManager_BuildClearing(UnitInfo* unit, bool mode) {
    ResourceID unit_type = unit->unit_type;
    uint16_t unit_team = unit->team;
    uint32_t unit_flags = unit->flags;
    int32_t unit_grid_x = unit->grid_x;
    int32_t unit_grid_y = unit->grid_y;
    ResourceID rubble_type = INVALID_ID;
    int32_t cargo_amount = 0;
    int32_t unit_orders = unit->orders;

    unit->RefreshScreen();

    if (unit_flags & STATIONARY) {
        UnitsManager_RemoveConnections(unit);

        if (unit->unit_type == RESEARCH) {
            ResearchMenu_UpdateResearchProgress(unit->team, unit->research_topic, -1);
        }

        if (unit->unit_type == GREENHSE) {
            UnitsManager_TeamInfo[unit->team].team_points -= unit->storage;
        }
    }

    if (mode && unit->unit_type != COMMANDO && unit->unit_type != INFANTRY &&
        !(unit_flags & (CONNECTOR_UNIT | HOVERING)) &&
        Access_IsFullyLandCovered(unit_grid_x, unit_grid_y, unit_flags)) {
        if (unit_flags & BUILDING) {
            rubble_type = LRGRUBLE;

        } else {
            rubble_type = SMLRUBLE;
        }

        cargo_amount = unit->GetNormalRateBuildCost() / 2;

        if (UnitsManager_BaseUnits[unit->unit_type].cargo_type == CARGO_TYPE_RAW) {
            cargo_amount += unit->storage;
        }
    }

    if ((unit_flags & (MOBILE_AIR_UNIT | MOBILE_LAND_UNIT | STATIONARY)) && !(unit->flags & HOVERING)) {
        Access_DestroyGroundCovers(unit_grid_x, unit_grid_y);

        if (unit_flags & BUILDING) {
            Access_DestroyGroundCovers(unit_grid_x + 1, unit_grid_y);
            Access_DestroyGroundCovers(unit_grid_x, unit_grid_y + 1);
            Access_DestroyGroundCovers(unit_grid_x + 1, unit_grid_y + 1);
        }
    }

    if (unit_type == LANDMINE || unit_type == SEAMINE) {
        SmartPointer<UnitInfo> target_unit(
            Access_GetUnit6(unit_team, unit_grid_x, unit_grid_y, (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)));

        if (!target_unit) {
            target_unit = Access_GetAttackTarget2(unit, unit_grid_x, unit_grid_y);
        }

        if (target_unit) {
            target_unit->SpotByTeam(unit_team);
            target_unit->AttackUnit(unit, 0, (target_unit->angle + 4) % 7);
        }

        rubble_type = INVALID_ID;
    }

    SmartList<UnitInfo>* units;

    if (unit_type == HANGAR || unit_type == AIRPLT) {
        units = &UnitsManager_MobileAirUnits;

    } else {
        units = &UnitsManager_MobileLandSeaUnits;
    }

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); Access_IsChildOfUnitInList(unit, units, &it);
         UnitsManager_DestroyUnit(&*it)) {
        if ((*it).unit_type == SEATRANS || (*it).unit_type == AIRTRANS || (*it).unit_type == CLNTRANS) {
            for (SmartList<UnitInfo>::Iterator it2 = UnitsManager_MobileLandSeaUnits.Begin();
                 Access_IsChildOfUnitInList(&*it, units, &it2); UnitsManager_DestroyUnit(&*it2)) {
                ++UnitsManager_TeamInfo[(*it2).team].casualties[(*it2).unit_type];
            }
        }

        ++UnitsManager_TeamInfo[(*it).team].casualties[(*it).unit_type];
    }

    UnitsManager_DestroyUnit(unit);

    if ((unit_flags & STATIONARY) && UnitsManager_TeamInfo[unit_team].team_type != TEAM_TYPE_REMOTE) {
        UnitsManager_TeamInfo[unit_team].team_units->OptimizeComplexes(unit_team);
    }

    if ((unit_flags & (BUILDING | STANDALONE)) || unit_orders == ORDER_BUILD || unit_orders == ORDER_CLEAR) {
        if (unit_type == CONSTRCT && unit_orders == ORDER_BUILD) {
            SmartPointer<UnitInfo> utility_unit(Access_GetUnit7(unit_team, unit_grid_x, unit_grid_y));

            unit_grid_x = utility_unit->grid_x;
            unit_grid_y = utility_unit->grid_y;

            if (Access_IsFullyLandCovered(unit_grid_x, unit_grid_y, utility_unit->flags)) {
                rubble_type = LRGRUBLE;

            } else {
                rubble_type = INVALID_ID;
            }
        }

        Access_DestroyUtilities(unit_grid_x, unit_grid_y, true, true, false, false);
    }

    if (rubble_type != INVALID_ID) {
        int32_t image_index =
            reinterpret_cast<struct BaseUnitDataFile*>(UnitsManager_BaseUnits[rubble_type].data_buffer)->image_count -
            1;

        image_index = ((dos_rand() * (image_index + 1)) >> 15);

        UnitInfo* rubble_unit =
            &*UnitsManager_DeployUnit(rubble_type, unit_team, nullptr, unit_grid_x, unit_grid_y, image_index);

        rubble_unit->storage = cargo_amount;
    }

    GameManager_RenderMinimapDisplay = true;
}

void UnitsManager_BuildNext(UnitInfo* unit) {
    ResourceID unit_type = *unit->GetBuildList()[0];
    int32_t turns_to_build = BuildMenu_GetTurnsToBuild(unit_type, unit->team);

    if (unit->storage >= turns_to_build * Cargo_GetRawConsumptionRate(unit->unit_type, 1) &&
        (unit->path->GetEndX() != unit->grid_x || unit->path->GetEndY() != unit->grid_y)) {
        bool remove_road = unit_type != CNCT_4W && unit_type != ROAD && unit_type != BRIDGE && unit_type != WTRPLTFM;
        bool remove_connectors =
            unit_type != CNCT_4W && unit_type != ROAD && unit_type != BRIDGE && unit_type != WTRPLTFM;

        Access_DestroyUtilities(unit->grid_x, unit->grid_y, false, false, remove_connectors, remove_road);

        unit->state = ORDER_STATE_5;

    } else {
        unit->ClearBuildListAndPath();
    }
}

void UnitsManager_ActivateUnit(UnitInfo* unit) {
    SmartPointer<UnitInfo> parent(unit->GetParent());

    SDL_assert(&*parent);

    if (((parent->flags & MOBILE_AIR_UNIT) && parent->speed > 0) ||
        (!Paths_IsOccupied(unit->target_grid_x, unit->target_grid_y, 0, unit->team) &&
         !(parent->flags & MOBILE_AIR_UNIT))) {
        SDL_assert(unit->storage > 0);

        --unit->storage;

        unit->SetParent(nullptr);

        if (unit->GetBuildList().GetCount() > 0) {
            unit->orders = ORDER_BUILD;
            unit->state = ORDER_STATE_1;
            unit->build_time = BuildMenu_GetTurnsToBuild(unit->GetConstructedUnitType(), unit->team);

            unit->StartBuilding();

        } else {
            unit->orders = ORDER_AWAIT;
            unit->state = ORDER_STATE_1;
        }

        if (GameManager_SelectedUnit == parent) {
            DrawMap_RenderBuildMarker();
        }

        if (parent->flags & MOBILE_AIR_UNIT) {
            parent->SetParent(nullptr);
            parent->target_grid_x = unit->target_grid_x;
            parent->target_grid_y = unit->target_grid_y;
            parent->orders = ORDER_MOVE;
            parent->state = ORDER_STATE_0;

            Access_UpdateMapStatus(&*parent, true);
            UnitsManager_ScaleUnit(&*parent, ORDER_STATE_EXPAND);

        } else {
            UnitsManager_UpdateMapHash(&*parent, unit->target_grid_x, unit->target_grid_y);

            parent->orders = ORDER_AWAIT;
            parent->state = ORDER_STATE_1;

            if (parent->unit_type == COMMANDO || parent->unit_type == INFANTRY || parent->unit_type == CLNTRANS) {
                if (parent->unit_type == COMMANDO) {
                    parent->image_base = 0;

                } else if (parent->unit_type == INFANTRY) {
                    parent->image_base = 0;

                } else {
                    parent->image_base = 8;
                }

                parent->DrawSpriteFrame(parent->image_base + parent->angle);
            }

            if ((parent->flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) == MOBILE_SEA_UNIT) {
                UnitInfo* bridge_unit = Access_GetUnit1(unit->target_grid_x, unit->target_grid_y);

                if (bridge_unit) {
                    UnitsManager_SetNewOrderInt(bridge_unit, ORDER_MOVE, ORDER_STATE_ELEVATE);
                }
            }

            parent->InitStealthStatus();

            Access_UpdateMapStatus(&*parent, true);

            UnitsManager_ScaleUnit(&*parent, ORDER_STATE_EXPAND);
        }

        if (GameManager_SelectedUnit == parent) {
            GameManager_MenuUnitSelect(&*parent);

            if (GameManager_DisplayButtonRange || GameManager_DisplayButtonScan) {
                GameManager_UpdateDrawBounds();
            }
        }

        if (unit->GetTask() &&
            (unit->unit_type == AIRTRANS || unit->unit_type == SEATRANS || unit->unit_type == CLNTRANS)) {
            unit->GetTask()->EventUnitUnloaded(*unit, *parent);
        }

        unit->RefreshScreen();

    } else {
        unit->RestoreOrders();
    }
}

void UnitsManager_StartExplosion(UnitInfo* unit) {
    SmartPointer<UnitInfo> explosion;
    uint32_t unit_flags = unit->flags;

    unit->RefreshScreen();

    if (unit->hits > 0) {
        SoundManager.PlaySfx(unit, SFX_TYPE_HIT);

    } else {
        if (GameManager_SelectedUnit == unit) {
            SoundManager.PlaySfx(unit, SFX_TYPE_INVALID);
        }

        SoundManager.PlaySfx(unit, SFX_TYPE_EXPLOAD);
    }

    if ((unit->unit_type == COMMANDO || unit->unit_type == INFANTRY) && unit->hits == 0) {
        if (unit->unit_type == COMMANDO) {
            unit->image_base = 168;

        } else {
            unit->image_base = 168;
        }

        unit->image_base += (unit->angle / 2) * 8;
        unit->image_index_max = unit->image_base + 7;
        unit->DrawSpriteFrame(unit->image_base);

        unit->state = ORDER_STATE_14;
        unit->moved = 0;
        unit->SetParent(nullptr);

    } else {
        ResourceID unit_type;

        if (unit->hits > 0) {
            unit_type = HITEXPLD;

        } else if (unit_flags & BUILDING) {
            unit_type = BLDEXPLD;

        } else if (unit_flags & MOBILE_LAND_UNIT) {
            unit_type = LNDEXPLD;

        } else if (unit_flags & MOBILE_AIR_UNIT) {
            unit_type = AIREXPLD;

        } else if (unit_flags & MOBILE_SEA_UNIT) {
            unit_type = SEAEXPLD;

        } else {
            unit_type = LNDEXPLD;
        }

        BaseUnit* base_unit = &UnitsManager_BaseUnits[unit_type];

        base_unit->flags &= ~(MISSILE_UNIT | MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | STATIONARY);

        if (unit_flags & GROUND_COVER) {
            base_unit->flags |= MOBILE_LAND_UNIT;

        } else if (unit_flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
            base_unit->flags |= STATIONARY;

        } else if (unit_flags & STATIONARY) {
            base_unit->flags |= MOBILE_AIR_UNIT;

        } else {
            base_unit->flags |= MISSILE_UNIT;
        }

        explosion = UnitsManager_DeployUnit(unit_type, unit->team, nullptr, unit->grid_x, unit->grid_y, 0, true);

        if (unit_type != BLDEXPLD) {
            explosion->OffsetDrawZones(unit->x - explosion->x, unit->y - explosion->y);
        }

        unit->RestoreOrders();

        if (unit->hits == 0) {
            explosion->SetParent(unit);
            unit->state = ORDER_STATE_14;

            if (GameManager_SelectedUnit == unit && ini_get_setting(INI_AUTO_SELECT)) {
                GameManager_AutoSelectNext(unit);
            }
        }
    }
}

void UnitsManager_ProgressExplosion(UnitInfo* unit) {
    if (unit->GetImageIndex() == unit->image_index_max) {
        UnitsManager_DestroyUnit(unit);

    } else {
        if (unit->GetParent() && unit->GetImageIndex() == unit->image_index_max / 2) {
            SmartPointer<UnitInfo> parent(unit->GetParent());

            unit->SetParent(nullptr);

            if (parent && parent->hits == 0) {
                UnitsManager_BuildClearing(&*parent, true);
            }
        }

        if (unit->moved & 0x01) {
            unit->DrawSpriteFrame(unit->GetImageIndex() + 1);
        }

        ++unit->moved;

        unit->RefreshScreen();
    }
}

void UnitsManager_ProgressUnloading(UnitInfo* unit) {
    if (unit->Land()) {
        if (GameManager_SelectedUnit == unit) {
            SoundManager.PlaySfx(unit, SFX_TYPE_POWER_CONSUMPTION_END, true);
        }

        SmartPointer<UnitInfo> parent(unit->GetParent());

        UnitsManager_UpdateMapHash(&*parent, unit->grid_x, unit->grid_y);

        parent->RestoreOrders();

        Access_UpdateMapStatus(&*parent, true);

        UnitsManager_ScaleUnit(&*parent, ORDER_STATE_EXPAND);

        if (UnitsManager_TeamInfo[parent->team].team_type == TEAM_TYPE_PLAYER) {
            GameManager_RenderMinimapDisplay = true;
        }

        SDL_assert(unit->storage > 0);

        --unit->storage;

        unit->moved = 0;

        unit->state = ORDER_STATE_18;

        if (GameManager_SelectedUnit == unit) {
            GameManager_UpdateInfoDisplay(unit);
        }
    }
}

void UnitsManager_StartClearing(UnitInfo* unit) {
    uint16_t unit_team = unit->team;
    SmartPointer<UnitInfo> parent(unit->GetParent());
    int32_t grid_x = parent->grid_x;
    int32_t grid_y = parent->grid_y;
    ResourceID cone_type;
    ResourceID tape_type;

    if (parent->flags & BUILDING) {
        cone_type = LRGCONES;
        tape_type = LRGTAPE;

    } else {
        cone_type = SMLCONES;
        tape_type = SMLTAPE;
    }

    UnitsManager_DeployUnit(cone_type, unit_team, nullptr, grid_x, grid_y, 0);
    SmartPointer<UnitInfo> tape_unit = UnitsManager_DeployUnit(tape_type, unit_team, nullptr, grid_x, grid_y, 0);

    tape_unit->SetParent(unit);

    unit->ClearUnitList();

    unit->state = ORDER_STATE_5;

    if (tape_type == LRGTAPE) {
        UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_34);
    }

    unit->DrawSpriteFrame(unit->GetImageIndex() + 8);

    if (GameManager_SelectedUnit == unit) {
        SoundManager.PlaySfx(unit, SFX_TYPE_BUILDING);
    }
}

void UnitsManager_ProgressLoading(UnitInfo* unit) {
    if (unit->Land()) {
        UnitValues* base_values = unit->GetBaseValues();
        SmartPointer<UnitInfo> parent(unit->GetParent());

        if (parent->hits > 0 && parent->orders != ORDER_FIRE && parent->orders != ORDER_EXPLODE &&
            parent->state != ORDER_STATE_14) {
            if (GameManager_SelectedUnit == unit) {
                SoundManager.PlaySfx(unit, SFX_TYPE_POWER_CONSUMPTION_START, true);
            }

            parent->path = nullptr;
            parent->SetParent(unit);

            Hash_MapHash.Remove(&*parent);

            if (parent->orders != ORDER_DISABLE) {
                parent->orders = ORDER_AWAIT;
                parent->state = ORDER_STATE_1;
            }

            Access_UpdateMapStatus(&*parent, false);

            UnitsManager_SetNewOrderInt(&*parent, ORDER_IDLE, ORDER_STATE_4);

            if (UnitsManager_TeamInfo[parent->team].team_type == TEAM_TYPE_PLAYER) {
                GameManager_RenderMinimapDisplay = true;
            }

            SDL_assert(unit->storage <= base_values->GetAttribute(ATTRIB_STORAGE));

            ++unit->storage;

            if (unit->storage == base_values->GetAttribute(ATTRIB_STORAGE)) {
                unit->cursor = CURSOR_HIDDEN;
            }
        }

        unit->moved = 0;
        unit->state = ORDER_STATE_16;

        if (GameManager_SelectedUnit == unit) {
            GameManager_UpdateInfoDisplay(unit);
        }
    }
}

void UnitsManager_BuildingReady(UnitInfo* unit) {
    SmartPointer<UnitInfo> parent(unit->GetParent());
    bool is_found = false;

    if (parent) {
        if (parent->hits > 0) {
            if (parent->orders == ORDER_MOVE && parent->state != ORDER_STATE_1) {
                is_found = false;

            } else {
                Rect bounds;
                Point position(parent->grid_x, parent->grid_y);

                unit->GetBounds(&bounds);

                is_found = !Access_IsInsideBounds(&bounds, &position);
            }

        } else {
            Rect bounds;
            Point position(parent->grid_x, parent->grid_y);

            unit->GetBounds(&bounds);

            is_found = !Access_IsInsideBounds(&bounds, &position);

            if (!is_found) {
                UnitsManager_DestroyUnit(unit);
            }
        }

    } else {
        is_found = true;
    }

    if (is_found) {
        if (unit->unit_type != CNCT_4W) {
            bool remove_road = unit->unit_type != ROAD;

            Access_DestroyUtilities(unit->grid_x, unit->grid_y, false, false, true, remove_road);

            if (unit->flags & BUILDING) {
                Access_DestroyUtilities(unit->grid_x + 1, unit->grid_y, false, false, true, true);
                Access_DestroyUtilities(unit->grid_x, unit->grid_y + 1, false, false, true, true);
                Access_DestroyUtilities(unit->grid_x + 1, unit->grid_y + 1, false, false, true, true);
            }
        }

        unit->RestoreOrders();
        unit->scaler_adjust = 4;
        unit->SetParent(nullptr);

        UnitsManager_ScaleUnit(unit, ORDER_STATE_EXPAND);

        Access_UpdateMapStatus(unit, true);

        if (parent && UnitsManager_TeamInfo[parent->team].team_type == TEAM_TYPE_PLAYER) {
            GameManager_RenderMinimapDisplay = true;
        }
    }
}

void UnitsManager_Repair(UnitInfo* unit) {
    SmartPointer<UnitInfo> parent(unit->GetParent());

    if (GameManager_SelectedUnit == unit) {
        SoundManager.PlaySfx(unit, SFX_TYPE_POWER_CONSUMPTION_START, true);
    }

    if (unit->GetComplex()) {
        Cargo materials;
        Cargo capacity;

        unit->GetComplex()->GetCargoInfo(materials, capacity);

        int32_t repair_cost = parent->Repair(materials.raw);

        unit->GetComplex()->Transfer(-repair_cost, 0, 0);

    } else {
        int32_t repair_cost = parent->Repair(unit->storage);

        unit->storage -= repair_cost;
    }

    if (parent->IsVisibleToTeam(GameManager_PlayerTeam)) {
        parent->RefreshScreen();
    }

    unit->RestoreOrders();

    if (GameManager_SelectedUnit == unit) {
        GameManager_UpdateInfoDisplay(unit);
    }

    parent->ScheduleDelayedTasks(true);
}

void UnitsManager_Transfer(UnitInfo* unit) {
    SmartPointer<UnitInfo> source(unit);
    SmartPointer<UnitInfo> target(unit->GetParent());
    int32_t cargo_type = UnitsManager_BaseUnits[unit->unit_type].cargo_type;
    int32_t transfer_amount = unit->target_grid_x;

    if (transfer_amount < 0) {
        source = target;
        target = unit;

        transfer_amount = labs(transfer_amount);
    }

    if (cargo_type == CARGO_TYPE_RAW) {
        target->TransferRaw(transfer_amount);

        if (source->GetComplex()) {
            source->GetComplex()->material -= transfer_amount;
        }

        if (source->storage >= transfer_amount) {
            source->storage -= transfer_amount;

        } else {
            transfer_amount -= source->storage;
            source->storage = 0;

            source->GetComplex()->Transfer(-transfer_amount, 0, 0);
            source->GetComplex()->material += transfer_amount;
        }

    } else if (cargo_type == CARGO_TYPE_FUEL) {
        target->TransferFuel(transfer_amount);

        if (source->GetComplex()) {
            source->GetComplex()->fuel -= transfer_amount;
        }

        if (source->storage >= transfer_amount) {
            source->storage -= transfer_amount;

        } else {
            transfer_amount -= source->storage;
            source->storage = 0;

            source->GetComplex()->Transfer(0, -transfer_amount, 0);
            source->GetComplex()->fuel += transfer_amount;
        }

    } else {
        SDL_assert(cargo_type == CARGO_TYPE_GOLD);

        target->TransferGold(transfer_amount);

        if (source->GetComplex()) {
            source->GetComplex()->gold -= transfer_amount;
        }

        if (source->storage >= transfer_amount) {
            source->storage -= transfer_amount;

        } else {
            transfer_amount -= source->storage;
            source->storage = 0;

            source->GetComplex()->Transfer(0, 0, -transfer_amount);
            source->GetComplex()->gold += transfer_amount;
        }
    }

    unit->RestoreOrders();

    if (GameManager_SelectedUnit == source) {
        GameManager_UpdateInfoDisplay(&*source);
    }

    if (GameManager_SelectedUnit == target) {
        GameManager_UpdateInfoDisplay(&*target);
    }

    target = unit->GetParent();

    if (target->GetTask()) {
        target->GetTask()->EventCargoTransfer(*target);
    }
}

bool UnitsManager_AttemptStealthAction(UnitInfo* unit) {
    SmartPointer<UnitInfo> parent(unit->GetParent());
    int32_t stealth_chance = UnitsManager_GetStealthChancePercentage(unit, &*parent, unit->orders);
    bool result;

    if (unit->team == parent->team) {
        result = false;

    } else {
        if (!GameManager_RealTime) {
            --unit->shots;

            if (unit->shots == 0) {
                unit->cursor = CURSOR_HIDDEN;
            }

            if (GameManager_SelectedUnit == unit) {
                GameManager_MenuUnitSelect(unit);
            }
        }

        if (unit->target_grid_x <= stealth_chance) {
            if (parent->orders != ORDER_DISABLE) {
                ++unit->storage;
            }

            if (GameManager_SelectedUnit == unit) {
                GameManager_MenuUnitSelect(unit);
            }

            result = true;

        } else {
            if (parent->orders == ORDER_DISABLE) {
                if (GameManager_PlayerTeam == unit->team) {
                    if (unit->orders == ORDER_AWAIT_STEAL_UNIT) {
                        MessageManager_DrawMessage(_(1141), 1, unit, Point(unit->grid_x, unit->grid_y));

                    } else {
                        MessageManager_DrawMessage(_(1c5c), 1, unit, Point(unit->grid_x, unit->grid_y));
                    }
                }

                result = false;

            } else {
                unit->SpotByTeam(parent->team);

                Access_DrawUnit(unit);

                unit->RefreshScreen();

                if (GameManager_PlayerTeam == parent->team) {
                    if (unit->orders == ORDER_AWAIT_STEAL_UNIT) {
                        GameManager_NotifyEvent(&*parent, 4);

                    } else {
                        GameManager_NotifyEvent(&*parent, 5);
                    }

                } else if (GameManager_PlayerTeam == unit->team) {
                    if (unit->orders == ORDER_AWAIT_STEAL_UNIT) {
                        MessageManager_DrawMessage(_(f876), 1, unit, Point(unit->grid_x, unit->grid_y));

                    } else {
                        MessageManager_DrawMessage(_(a939), 1, unit, Point(unit->grid_x, unit->grid_y));
                    }

                    SoundManager.PlayVoice(V_M007, V_F012);
                }

                UnitsManager_AddToDelayedReactionList(unit);

                result = false;
            }
        }
    }

    return result;
}

void UnitsManager_CaptureUnit(UnitInfo* unit) {
    SmartPointer<UnitInfo> parent(unit->GetParent());
    uint16_t old_team = unit->team;
    uint16_t new_team = parent->team;

    if (GameManager_PlayerTeam == new_team) {
        GameManager_NotifyEvent(&*parent, 2);

    } else if (GameManager_PlayerTeam == old_team) {
        MessageManager_DrawMessage(_(8c00), 0, &*parent, Point(parent->grid_x, parent->grid_y));
        SoundManager.PlayVoice(V_M239, V_F242);
    }

    if (parent->orders == ORDER_BUILD || parent->orders == ORDER_CLEAR) {
        if (parent->state == ORDER_STATE_25) {
            GameManager_SelectBuildSite(&*parent);

        } else {
            parent->CancelBuilding();
        }
    }

    parent->ChangeTeam(old_team);

    Ai_UnitSpotted(unit, new_team);
}

void UnitsManager_DisableUnit(UnitInfo* unit) {
    SmartPointer<UnitInfo> parent(unit->GetParent());
    uint16_t unit_team = unit->team;
    bool is_found = false;
    int32_t turns_disabled;

    if (parent->GetBaseValues()->GetAttribute(ATTRIB_SPEED)) {
        turns_disabled =
            ((parent->GetBaseValues()->GetAttribute(ATTRIB_SPEED) * (unit->GetExperience() + 8)) -
             (parent->GetBaseValues()->GetAttribute(ATTRIB_TURNS) * parent->speed)) /
            (parent->GetBaseValues()->GetAttribute(ATTRIB_TURNS) * parent->GetBaseValues()->GetAttribute(ATTRIB_SPEED));

    } else {
        turns_disabled = (unit->GetExperience() + 8) / parent->GetBaseValues()->GetAttribute(ATTRIB_TURNS);
    }

    if (turns_disabled < 1) {
        turns_disabled = 1;
    }

    if (GameManager_PlayerTeam == parent->team) {
        GameManager_NotifyEvent(&*parent, 3);

    } else if (GameManager_PlayerTeam == unit_team) {
        Point position(parent->grid_x, parent->grid_y);

        if (turns_disabled == 1) {
            MessageManager_DrawMessage(_(ea13), 0, &*parent, position);

        } else {
            SmartString message;

            message.Sprintf(100, _(836d), turns_disabled);

            MessageManager_DrawMessage(message.GetCStr(), 0, &*parent, position);
        }

        SoundManager.PlayVoice(V_M244, V_F244);
    }

    switch (parent->orders) {
        case ORDER_MOVE: {
            parent->MoveFinished(false);
            parent->state = ORDER_STATE_1;
        } break;

        case ORDER_BUILD:
        case ORDER_CLEAR: {
            if (parent->state == ORDER_STATE_25) {
                GameManager_SelectBuildSite(&*parent);

            } else {
                do {
                    parent->CancelBuilding();
                } while (parent->orders == ORDER_HALT_BUILDING);

                if (!(parent->flags & STATIONARY)) {
                    unit->orders = ORDER_AWAIT;
                    parent->BusyWaitOrder();
                }
            }
        } break;

        case ORDER_POWER_ON: {
            UnitsManager_SetNewOrderInt(&*parent, ORDER_POWER_OFF, ORDER_STATE_1);
            UnitsManager_PowerDownUnit(&*parent);

            is_found = true;
        } break;
    }

    SmartPointer<UnitInfo> unit_copy(parent->MakeCopy());

    parent->path = nullptr;

    UnitsManager_SetNewOrderInt(&*parent, ORDER_DISABLE, ORDER_STATE_1);

    parent->RemoveTasks();
    parent->recoil_delay = turns_disabled;

    Access_UpdateMapStatus(&*parent, true);
    Access_UpdateMapStatus(&*unit_copy, false);

    Ai_UnitSpotted(unit, parent->team);

    if (UnitsManager_TeamInfo[parent->team].team_type == TEAM_TYPE_PLAYER) {
        GameManager_RenderMinimapDisplay = true;
    }

    if (is_found) {
        ProductionManager_ManageMining(parent->team, parent->GetComplex(), &*parent,
                                       GameManager_PlayerTeam == parent->team);
    }
}

bool UnitsManager_AssessAttacks() {
    bool result;

    if (ini_get_setting(INI_DISABLE_FIRE)) {
        result = false;

    } else {
        bool are_attacks_delayed = false;

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            if (UnitsManager_DelayedAttackTargets[team].GetCount() > 0 || UnitsManager_Units[team]) {
                are_attacks_delayed = true;
            }
        }

        if (are_attacks_delayed) {
            if (UnitsManager_TeamInfo[UnitsManager_Team].team_type == TEAM_TYPE_REMOTE) {
                result = true;

            } else {
                are_attacks_delayed = false;

                int32_t team = UnitsManager_Team;

                do {
                    if (UnitsManager_CheckDelayedReactions(UnitsManager_Team)) {
                        are_attacks_delayed = true;
                    }

                    UnitsManager_Team = (UnitsManager_Team + 1) % 4;

                    if (UnitsManager_TeamInfo[UnitsManager_Team].team_type == TEAM_TYPE_REMOTE) {
                        if (are_attacks_delayed) {
                            UnitsManager_byte_178170 = true;

                        } else if (UnitsManager_byte_178170) {
                            UnitsManager_byte_178170 = false;

                        } else {
                            UnitsManager_ClearDelayedReactions();
                        }

                        ++UnitsManager_UnknownCounter;

                        Remote_SendNetPacket_46(UnitsManager_Team, are_attacks_delayed, UnitsManager_UnknownCounter);

                        return true;
                    }

                    if (are_attacks_delayed) {
                        return true;
                    }

                } while (team != UnitsManager_Team);

                UnitsManager_ClearDelayedReactions();

                result = UnitsManager_IsAttackScheduled();
            }

        } else {
            UnitsManager_byte_178170 = 0;

            result = false;
        }
    }

    return result;
}

bool UnitsManager_IsTeamReactionPending(uint16_t team, UnitInfo* unit, SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && UnitsManager_CheckReaction(&*it, unit)) {
            return true;
        }
    }

    return false;
}

bool UnitsManager_ShouldAttack(UnitInfo* unit1, UnitInfo* unit2) {
    bool result;

    if ((unit1->unit_type == SP_FLAK || unit1->unit_type == ANTIAIR || unit1->unit_type == FASTBOAT) &&
        !(unit2->flags & MOBILE_AIR_UNIT)) {
        result = false;

    } else if (unit2->hits > 0) {
        if (unit1->orders == ORDER_IDLE) {
            result = false;

        } else if ((unit1->orders == ORDER_MOVE || unit1->orders == ORDER_MOVE_TO_UNIT ||
                    unit1->orders == ORDER_MOVE_TO_ATTACK) &&
                   unit1->state != ORDER_STATE_1) {
            result = false;

        } else if (unit1->orders == ORDER_DISABLE) {
            result = false;

        } else if (unit1->state == ORDER_STATE_27) {
            result = false;

        } else if (unit2->IsVisibleToTeam(unit1->team)) {
            if ((unit1->unit_type == COMMANDO || unit1->unit_type == SUBMARNE) &&
                !unit1->IsVisibleToTeam(unit2->team)) {
                result = false;

            } else if (UnitsManager_TeamInfo[unit1->team].team_type == TEAM_TYPE_COMPUTER) {
                result = Ai_IsTargetTeam(unit1, unit2);

            } else {
                if (GameManager_PlayMode == PLAY_MODE_TURN_BASED || unit1->orders == ORDER_SENTRY) {
                    result = true;

                } else if (UnitsManager_TeamInfo[unit1->team].finished_turn &&
                           GameManager_PlayMode == PLAY_MODE_UNKNOWN) {
                    result = true;

                } else if (unit2->shots > 0 && !unit1->disabled_reaction_fire) {
                    uint16_t unit1_team = unit1->team;
                    int32_t unit2_distance = unit2->GetBaseValues()->GetAttribute(ATTRIB_RANGE);

                    unit2_distance = unit2_distance * unit2_distance;

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                        if ((*it).team == unit1_team && Access_GetDistance(unit2, &*it) <= unit2_distance &&
                            Access_IsValidAttackTarget(unit2, &*it)) {
                            return true;
                        }
                    }

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                         it != UnitsManager_MobileAirUnits.End(); ++it) {
                        if ((*it).team == unit1_team && Access_GetDistance(unit2, &*it) <= unit2_distance &&
                            Access_IsValidAttackTarget(unit2, &*it)) {
                            return true;
                        }
                    }

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                         it != UnitsManager_StationaryUnits.End(); ++it) {
                        if ((*it).team == unit1_team && Access_GetDistance(unit2, &*it) <= unit2_distance &&
                            Access_IsValidAttackTarget(unit2, &*it)) {
                            return true;
                        }
                    }

                    result = false;

                } else {
                    result = false;
                }
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool UnitsManager_CheckReaction(UnitInfo* unit1, UnitInfo* unit2) {
    bool result;

    if (unit1->shots > 0 && !unit1->delayed_reaction &&
        Access_IsWithinAttackRange(unit1, unit2->grid_x, unit2->grid_y,
                                   unit1->GetBaseValues()->GetAttribute(ATTRIB_RANGE)) &&
        Access_IsValidAttackTarget(unit1, unit2) && UnitsManager_ShouldAttack(unit1, unit2)) {
        unit1->target_grid_x = unit2->grid_x;
        unit1->target_grid_y = unit2->grid_y;

        if (GameManager_PlayerTeam == unit1->team) {
            BaseUnit* base_unit = &UnitsManager_BaseUnits[unit2->unit_type];
            Point position(unit1->grid_x, unit1->grid_y);
            SmartString message;

            message.Sprintf(150, UnitsManager_ReactionsToEnemy[base_unit->gender],
                            UnitsManager_BaseUnits[unit1->unit_type].singular_name, unit1->grid_x + 1,
                            unit1->grid_y + 1, base_unit->singular_name, unit2->grid_x + 1, unit2->grid_y + 1);

            MessageManager_DrawMessage(message.GetCStr(), 0, unit1, position);
        }

        UnitsManager_SetNewOrder(unit1, ORDER_FIRE, ORDER_STATE_0);

        if (GameManager_PlayerTeam == unit1->team && GameManager_SelectedUnit == unit1 &&
            !GameManager_IsAtGridPosition(unit1)) {
            SoundManager.PlayVoice(V_M250, V_F251);
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool UnitsManager_IsReactionPending(SmartList<UnitInfo>* units, UnitInfo* unit) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if (UnitsManager_CheckReaction(&*it, unit)) {
            return true;
        }
    }

    return false;
}

AirPath* UnitsManager_GetMissilePath(UnitInfo* unit) {
    int32_t distance_x = unit->target_grid_x - unit->grid_x;
    int32_t distance_y = unit->target_grid_y - unit->grid_y;
    int32_t distance = sqrt(distance_x * distance_x + distance_y * distance_y) * 4.0 + 0.5;
    AirPath* result;

    if (distance > 0) {
        result = new (std::nothrow)
            AirPath(unit, distance_x, distance_y, distance / 4, unit->target_grid_x, unit->target_grid_y);

    } else {
        result = nullptr;
    }

    return result;
}

void UnitsManager_UpdateAttackPaths(UnitInfo* unit) {
    unit->path = nullptr;

    if (unit->flags & MISSILE_UNIT) {
        unit->path = UnitsManager_GetMissilePath(unit);

    } else {
        if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_REMOTE) {
            unit->state = ORDER_STATE_NEW_ORDER;

        } else {
            if (unit->state == ORDER_STATE_28) {
                if ((unit->flags & MOBILE_AIR_UNIT) && unit->orders != ORDER_MOVE_TO_ATTACK) {
                    unit->path = Paths_GetAirPath(unit);

                    unit->RestoreOrders();

                    if (Remote_IsNetworkGame) {
                        Remote_SendNetPacket_38(unit);
                    }

                } else {
                    if (Paths_RequestPath(unit, 0x02)) {
                        unit->state = ORDER_STATE_29;
                    }
                }

            } else {
                if (unit->GetUnitList()) {
                    Access_GroupAttackOrder(unit, true);

                } else {
                    if ((unit->flags & MOBILE_AIR_UNIT) && unit->orders != ORDER_MOVE_TO_ATTACK) {
                        unit->path = Paths_GetAirPath(unit);

                    } else {
                        if (Paths_RequestPath(unit, 0x02)) {
                            unit->state = ORDER_STATE_NEW_ORDER;
                        }
                    }
                }
            }
        }
    }
}

bool UnitsManager_IsAttackScheduled() {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if ((*it).orders == ORDER_FIRE) {
            return true;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).orders == ORDER_FIRE) {
            return true;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).orders == ORDER_FIRE) {
            return true;
        }
    }

    return false;
}

Point UnitsManager_GetAttackPosition(UnitInfo* unit1, UnitInfo* unit2) {
    if (unit2->unit_type == CONSTRCT && unit2->orders == ORDER_BUILD) {
        UnitInfo* unit = Access_GetUnit7(unit2->team, unit2->grid_x, unit2->grid_y);

        if (unit) {
            unit2 = unit;
        }
    }

    Point position(unit2->grid_x, unit2->grid_y);

    if (unit2->flags & BUILDING) {
        if (unit1->grid_x > position.x) {
            ++position.x;
        }

        if (unit1->grid_y > position.y) {
            ++position.y;
        }

        if (unit1->unit_type == SUBMARNE || unit1->unit_type == CORVETTE) {
            int32_t direction = 4;

            while (direction >= 0) {
                int32_t surface_type = Access_GetSurfaceType(position.x, position.y);

                if (surface_type == SURFACE_TYPE_WATER || surface_type == SURFACE_TYPE_COAST) {
                    break;
                }

                position.x = unit2->grid_x + (direction & 0x01);
                position.y = unit2->grid_y + (direction & 0x02);

                --direction;
            }
        }
    }

    return position;
}

bool UnitsManager_CheckDelayedReactions(uint16_t team) {
    bool result;

    if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
        UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED) {
        if (UnitsManager_Units[team]) {
            UnitInfo* unit1 = &*UnitsManager_Units[team];
            UnitInfo* unit2 = unit1->GetEnemy();
            Point position = UnitsManager_GetAttackPosition(unit1, unit2);

            unit1->target_grid_x = position.x;
            unit1->target_grid_y = position.y;

            if (UnitsManager_TeamInfo[unit1->team].team_type == TEAM_TYPE_COMPUTER &&
                unit1->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE) && unit1->shots == 1) {
                unit1->orders = ORDER_AWAIT;

                unit1->SetEnemy(nullptr);
            }

            UnitsManager_SetNewOrder(unit1, ORDER_FIRE, ORDER_STATE_0);

            if (GameManager_PlayerTeam == unit1->team && GameManager_SelectedUnit == unit1 &&
                !GameManager_IsAtGridPosition(unit1)) {
                SoundManager.PlayVoice(V_M250, V_F251);
            }

            if (GameManager_SelectedUnit == unit1) {
                GameManager_UpdateInfoDisplay(unit1);
            }

            result = true;

        } else {
            for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
                if (team != i) {
                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_DelayedAttackTargets[i].Begin();
                         it != UnitsManager_DelayedAttackTargets[i].End(); ++it) {
                        if ((*it).IsVisibleToTeam(team) &&
                            UnitsManager_IsReactionPending(&UnitsManager_DelayedAttackTargets[team], &*it)) {
                            return true;
                        }
                    }

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_DelayedAttackTargets[i].Begin();
                         it != UnitsManager_DelayedAttackTargets[i].End(); ++it) {
                        if ((*it).IsVisibleToTeam(team) &&
                            (UnitsManager_IsTeamReactionPending(team, &*it, &UnitsManager_MobileLandSeaUnits) ||
                             UnitsManager_IsTeamReactionPending(team, &*it, &UnitsManager_MobileAirUnits) ||
                             UnitsManager_IsTeamReactionPending(team, &*it, &UnitsManager_StationaryUnits))) {
                            return true;
                        }
                    }
                }
            }

            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

int32_t UnitsManager_GetAttackDamage(UnitInfo* attacker, UnitInfo* target, int32_t attack_potential) {
    int32_t target_armor = target->GetBaseValues()->GetAttribute(ATTRIB_ARMOR);
    int32_t attacker_attack = attacker->GetBaseValues()->GetAttribute(ATTRIB_ATTACK);
    int32_t result;

    if (attack_potential > 1) {
        attacker_attack = ((5 - attack_potential) * attacker_attack) / 4;
    }

    if (target->unit_type == SUBMARNE && (attacker->unit_type == BOMBER || attacker->unit_type == ALNPLANE)) {
        attacker_attack /= 2;
    }

    if (target_armor >= attacker_attack) {
        result = 1;

    } else {
        result = attacker_attack - target_armor;
    }

    return result;
}
