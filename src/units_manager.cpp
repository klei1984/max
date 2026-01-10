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

#include <cmath>

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "allocmenu.hpp"
#include "builder.hpp"
#include "buildmenu.hpp"
#include "cursor.hpp"
#include "drawmap.hpp"
#include "enums.hpp"
#include "game_manager.hpp"
#include "hash.hpp"
#include "message_manager.hpp"
#include "paths_manager.hpp"
#include "production_manager.hpp"
#include "quickbuild.hpp"
#include "randomizer.hpp"
#include "remote.hpp"
#include "repairshopmenu.hpp"
#include "researchmenu.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
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
static void UnitsManager_UpdateMapHash(UnitInfo* unit, int32_t grid_x, int32_t grid_y);
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
static bool UnitsManager_FindValidPowerGeneratorPosition(uint16_t team, int16_t* grid_x, int16_t* grid_y);
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
static void UnitsManager_ProcessUnitOrders(SmartList<UnitInfo>* units);
static void UnitsManager_FinishUnitScaling(UnitInfo* unit);
static void UnitsManager_ActivateEngineer(UnitInfo* unit);
static void UnitsManager_DeployMasterBuilderInit(UnitInfo* unit);
static bool UnitsManager_IsFactory(ResourceID unit_type);
static void UnitsManager_ClearDelayedReaction(SmartList<UnitInfo>* units);
static void UnitsManager_ClearDelayedReactions();
static void UnitsManager_PerformAutoSurvey(UnitInfo* unit);
static void UnitsManager_DeployMasterBuilder(UnitInfo* unit);
static bool UnitsManager_PursueEnemy(UnitInfo* unit);
static bool UnitsManager_InitUnitMove(UnitInfo* unit);
static void UnitsManager_Store(UnitInfo* unit);
static void UnitsManager_BuildClearing(UnitInfo* unit, bool mode);
static void UnitsManager_BuildNext(UnitInfo* unit);
static void UnitsManager_ActivateUnit(UnitInfo* unit);
static void UnitsManager_StartExplosion(UnitInfo* unit);
static void UnitsManager_ProgressExplosion(UnitInfo* unit);
static void UnitsManager_ProgressUnloading(UnitInfo* unit);
static void UnitsManager_StartClearing(UnitInfo* unit);
static void UnitsManager_ProgressLoading(UnitInfo* unit);
static void UnitsManager_BuildingReady(UnitInfo* unit);
static bool UnitsManager_AttemptStealthAction(UnitInfo* unit);
static void UnitsManager_CaptureUnit(UnitInfo* unit);
static void UnitsManager_DisableUnit(UnitInfo* unit);
static bool UnitsManager_AssessAttacks();
static bool UnitsManager_IsTeamReactionPending(uint16_t team, UnitInfo* unit, SmartList<UnitInfo>* units);
static bool UnitsManager_ShouldAttack(UnitInfo* unit1, UnitInfo* unit2);
static bool UnitsManager_CheckReaction(UnitInfo* unit1, UnitInfo* unit2);
static bool UnitsManager_IsReactionPending(SmartList<UnitInfo>* units, UnitInfo* unit);
static AirPath* UnitsManager_GetMissilePath(UnitInfo* unit);
static void UnitsManager_InitUnitPath(UnitInfo* unit);
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
static void UnitsManager_ProcessOrderReload(UnitInfo* unit);
static void UnitsManager_ProcessOrderTransfer(UnitInfo* unit);
static void UnitsManager_ProcessOrderHaltBuilding(UnitInfo* unit);
static void UnitsManager_ProcessOrderAwaitScaling(UnitInfo* unit);
static void UnitsManager_ProcessOrderAwaitTapePositioning(UnitInfo* unit);
static void UnitsManager_ProcessOrderAwaitDisableStealUnit(UnitInfo* unit);
static void UnitsManager_ProcessOrderDisable(UnitInfo* unit);
static void UnitsManager_ProcessOrderUpgrade(UnitInfo* unit);
static void UnitsManager_ProcessOrderLayMine(UnitInfo* unit);

SmartList<UnitInfo> UnitsManager_GroundCoverUnits;
SmartList<UnitInfo> UnitsManager_MobileLandSeaUnits;
SmartList<UnitInfo> UnitsManager_ParticleUnits;
SmartList<UnitInfo> UnitsManager_StationaryUnits;
SmartList<UnitInfo> UnitsManager_MobileAirUnits;
SmartList<UnitInfo> UnitsManager_PendingAttacks;

SmartPointer<UnitInfo> UnitsManager_PendingAirGroupLeader;

SmartPointer<UnitInfo> UnitsManager_Units[PLAYER_TEAM_MAX];

SmartList<UnitInfo> UnitsManager_DelayedAttackTargets[PLAYER_TEAM_MAX];

bool UnitsManager_DelayedReactionsPending;
uint16_t UnitsManager_DelayedReactionsTeam;
uint32_t UnitsManager_DelayedReactionsSyncCounter;

bool UnitsManager_OrdersPending;
bool UnitsManager_CombatEffectsActive;

int8_t UnitsManager_BobEffectQuota;
bool UnitsManager_ResetBobState;

const char* const UnitsManager_Orders[] = {
    "Awaiting",   "Transforming", "Moving",    "Firing",          "Building",  "Activate Order", "New Allocate Order",
    "Power On",   "Power Off",    "Exploding", "Unloading",       "Clearing",  "Sentry",         "Landing",
    "Taking Off", "Loading",      "Idle",      "Repairing",       "Refueling", "Reloading",      "Transferring",
    "Awaiting",   "Awaiting",     "Awaiting",  "Awaiting",        "Awaiting",  "Disabled",       "Moving",
    "Repairing",  "Transferring", "Attacking", "Building Halted",
};

CTInfo UnitsManager_TeamInfo[PLAYER_TEAM_MAX];

struct PopupFunctions UnitsManager_PopupCallbacks[POPUP_MENU_TYPE_COUNT];

TeamMissionSupplies UnitsManager_TeamMissionSupplies[PLAYER_TEAM_MAX];

bool UnitsManager_SelfDestructActiveMenu(WindowInfo* window) {
    Button* button_destruct;
    bool event_click_destruct;
    bool event_click_cancel;
    bool event_release;

    for (uint16_t id = SLFDOPN1; id <= SLFDOPN6; ++id) {
        uint64_t time_Stamp = timer_get();

        WindowManager_LoadSimpleImage(static_cast<ResourceID>(id), 13, 11, false, window);
        win_draw(window->id);
        GameManager_ProcessState(true);

        while (timer_get() - time_Stamp < TIMER_FPS_TO_MS(48)) {
            SDL_Delay(1);
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
                ResourceManager_GetSoundManager().PlaySfx(NCANC0);
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
    destruct_window.SetFlags(WINDOW_MODAL);

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

    if (target_unit->GetUnitType() == SUBMARNE &&
        (attacker_unit->GetUnitType() == BOMBER || attacker_unit->GetUnitType() == ALNPLANE)) {
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

void UnitsManager_Popup_OnClick_Sentry(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    GameManager_UpdateDrawBounds();

    unit->path = nullptr;
    unit->disabled_reaction_fire = false;
    unit->auto_survey = false;

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_38(unit);
    }

    if (unit->GetOrder() == ORDER_SENTRY) {
        UnitsManager_SetNewOrder(unit, ORDER_AWAIT, ORDER_STATE_EXECUTING_ORDER);
        GameManager_UpdateInfoDisplay(unit);

    } else {
        UnitsManager_SetNewOrder(unit, ORDER_SENTRY, ORDER_STATE_EXECUTING_ORDER);
        GameManager_UpdateInfoDisplay(unit);
        GameManager_AutoSelectNext(unit);
    }
}

void UnitsManager_Popup_OnClick_Upgrade(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(false);
    unit->SetParent(unit);
    UnitsManager_SetNewOrder(unit, ORDER_UPGRADE, ORDER_STATE_INIT);
}

void UnitsManager_Popup_OnClick_UpgradeAll(ButtonID bid, UnitInfo* unit) {
    int32_t material_cost;
    int32_t cost;
    int32_t unit_count;
    UnitInfo* upgraded_unit;
    SmartArray<Complex> complexes;
    ObjectArray<int16_t> costs;
    int64_t index;
    char mark_level[20];

    GameManager_DeinitPopupButtons(true);

    material_cost = 0;
    unit_count = 0;
    upgraded_unit = nullptr;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if (unit->team == (*it).team && unit->GetUnitType() == (*it).GetUnitType() && (*it).IsUpgradeAvailable() &&
            (*it).GetOrderState() != ORDER_STATE_UNIT_READY) {
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
                UnitsManager_SetNewOrder(&*it, ORDER_UPGRADE, ORDER_STATE_EXECUTING_ORDER);

                ++unit_count;
                material_cost += cost;

                upgraded_unit = &*it;

                *costs[index] -= cost;
            }
        }
    }

    UnitInfo::GetVersion(
        mark_level,
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[unit->team], unit->GetUnitType())->GetVersion());

    if (unit_count <= 0) {
        SmartString string;

        MessageManager_DrawMessage(string.Sprintf(80, _(a0ee), unit->GetNormalRateBuildCost() / 4).GetCStr(), 2, 0);

    } else if (unit_count == 1) {
        SmartString string;

        MessageManager_DrawMessage(
            string
                .Sprintf(80, _(8967), ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data(), mark_level,
                         material_cost)
                .GetCStr(),
            0, upgraded_unit, Point(upgraded_unit->grid_x, upgraded_unit->grid_y));

    } else {
        SmartString string;

        MessageManager_DrawMessage(
            string
                .Sprintf(80, _(2693), unit_count, ResourceManager_GetUnit(unit->GetUnitType()).GetPluralName().data(),
                         mark_level, material_cost)
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
        UnitsManager_SetNewOrder(unit, ORDER_EXPLODE, ORDER_STATE_EXPLODE);
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

    if (unit->GetOrder() == ORDER_SENTRY) {
        UnitsManager_SetNewOrder(unit, ORDER_AWAIT, ORDER_STATE_EXECUTING_ORDER);
    }

    GameManager_UpdateInfoDisplay(unit);
}

void UnitsManager_Popup_InitCommons(UnitInfo* unit, struct PopupButtons* buttons) {
    if (GameManager_PlayMode == PLAY_MODE_SIMULTANEOUS_MOVES && unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0 &&
        unit->GetUnitType() != COMMANDO) {
        UnitsManager_RegisterButton(buttons, unit->disabled_reaction_fire, _(b0b9), '4',
                                    &UnitsManager_Popup_OnClick_Manual);
    }

    if (ResourceManager_GetUnit(unit->GetUnitType()).GetCargoType() > Unit::CargoType::CARGO_TYPE_NONE &&
        ResourceManager_GetUnit(unit->GetUnitType()).GetCargoType() <= Unit::CargoType::CARGO_TYPE_GOLD &&
        unit->GetOrder() != ORDER_CLEAR && unit->GetOrder() != ORDER_BUILD) {
        UnitsManager_RegisterButton(buttons, unit->cursor == 3, _(886b), '3', &UnitsManager_Popup_OnClick_Transfer);
    }

    if ((unit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) && unit->GetOrder() != ORDER_CLEAR &&
        unit->GetOrder() != ORDER_BUILD) {
        UnitsManager_RegisterButton(buttons, unit->enter_mode, _(e08d), '5', &UnitsManager_Popup_OnClick_Enter);
    }

    if (unit->path != nullptr && unit->GetOrder() != ORDER_CLEAR && unit->GetOrder() != ORDER_BUILD) {
        UnitsManager_RegisterButton(buttons, false, _(6c45), '7', &UnitsManager_Popup_OnClick_Stop);
    }

    if (unit->IsUpgradeAvailable()) {
        UnitsManager_RegisterButton(buttons, false, _(7fef), '5', &UnitsManager_Popup_OnClick_Upgrade);
        UnitsManager_RegisterButton(buttons, false, _(97a2), '6', &UnitsManager_Popup_OnClick_UpgradeAll);
    }

    if (unit->flags & SENTRY_UNIT) {
        if (unit->GetUnitType() != COMMANDO) {
            if (unit->GetOrder() == ORDER_SENTRY) {
                UnitsManager_RegisterButton(buttons, true, _(0cf6), '8', &UnitsManager_Popup_OnClick_Sentry);

            } else if (unit->GetOrder() == ORDER_AWAIT) {
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

    if (unit->GetOrder() == ORDER_SENTRY) {
        UnitsManager_SetNewOrder(unit, ORDER_AWAIT, ORDER_STATE_EXECUTING_ORDER);
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

    if (unit->GetOrder() == ORDER_BUILD) {
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
    if (unit->GetOrder() == ORDER_BUILD && unit->GetOrderState() != ORDER_STATE_BUILD_CANCEL &&
        unit->GetOrderState() != ORDER_STATE_BUILD_ABORT) {
        UnitsManager_RegisterButton(buttons, false, _(bf85), '7', &UnitsManager_Popup_OnClick_BuildStop);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(b3ef), '1', &UnitsManager_Popup_OnClick_BuildStop);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

bool UnitsManager_Popup_IsNever(UnitInfo* unit) { return false; }

bool UnitsManager_Popup_IsReady(UnitInfo* unit) {
    return (unit->GetOrder() == ORDER_AWAIT && unit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER) ||
           (unit->GetOrder() == ORDER_IDLE && unit->GetOrderState() == ORDER_STATE_BUILDING_READY);
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

    if (unit->GetOrderState() == ORDER_STATE_SELECT_SITE) {
        GameManager_SelectBuildSite(unit);
    }

    if (unit->GetOrder() == ORDER_AWAIT && (unit->flags & STATIONARY)) {
        unit->SetOrderState(ORDER_STATE_READY_TO_EXECUTE_ORDER);
    }

    if (unit->path != nullptr && unit->speed > 0 && unit->GetOrder() != ORDER_CLEAR &&
        unit->GetOrder() != ORDER_BUILD && !unit->GetUnitList()) {
        uint8_t order;

        order = unit->GetOrder();

        unit->SetOrder(unit->GetPriorOrder());
        unit->SetOrderState(unit->GetPriorOrderState());

        if (order == ORDER_MOVE_TO_ATTACK) {
            UnitsManager_SetNewOrder(unit, ORDER_MOVE_TO_ATTACK, ORDER_STATE_INIT);

        } else {
            UnitsManager_SetNewOrder(unit, ORDER_MOVE, ORDER_STATE_INIT);
        }

        GameManager_UpdateDrawBounds();
    }

    if (ResourceManager_GetSettings()->GetNumericValue("auto_select")) {
        GameManager_SelectNextUnit(1);
    }
}

bool UnitsManager_Popup_IsUnitReady(UnitInfo* unit) {
    bool result;

    if (unit->GetOrder() == ORDER_BUILD && unit->GetOrderState() == ORDER_STATE_UNIT_READY) {
        result = true;

    } else {
        result = UnitsManager_Popup_IsReady(unit);
    }

    return result;
}

void UnitsManager_Popup_OnClick_Build(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);
    GameManager_DisableMainMenu();

    if (unit->GetOrder() == ORDER_BUILD) {
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

    if (Access_FindReachableSpot(parent->GetUnitType(), parent, &grid_x, &grid_y, 1, 1, 0)) {
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
    if (unit->GetOrderState() == ORDER_STATE_UNIT_READY) {
        UnitsManager_Popup_PlaceNewUnit(0, unit);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(7b6d), '1', &UnitsManager_Popup_OnClick_Build);

        if (unit->GetOrder() == ORDER_HALT_BUILDING || unit->GetOrder() == ORDER_HALT_BUILDING_2) {
            UnitsManager_RegisterButton(buttons, false, _(cced), '2', &UnitsManager_Popup_OnClick_StartBuild);

        } else if (unit->GetOrder() == ORDER_BUILD && unit->GetOrderState() != ORDER_STATE_BUILD_CANCEL &&
                   unit->GetOrderState() != ORDER_STATE_BUILD_ABORT) {
            UnitsManager_RegisterButton(buttons, false, _(bcf7), '7', &UnitsManager_Popup_OnClick_StopBuild);
        }

        UnitsManager_Popup_InitCommons(unit, buttons);
    }
}

void UnitsManager_Popup_OnClick_PowerOn(ButtonID bid, UnitInfo* unit) {
    char message[400];

    GameManager_DeinitPopupButtons(true);
    UnitsManager_SetNewOrder(unit, ORDER_POWER_ON, ORDER_STATE_INIT);

    sprintf(message, _(8576), ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data(), _(b8d3));

    MessageManager_DrawMessage(message, 0, 0);
}

void UnitsManager_Popup_OnClick_PowerOff(ButtonID bid, UnitInfo* unit) {
    char message[400];

    GameManager_DeinitPopupButtons(true);
    UnitsManager_SetNewOrder(unit, ORDER_POWER_OFF, ORDER_STATE_INIT);

    sprintf(message, _(d599), ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data(), _(b4dc));

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
    if (unit->GetOrder() == ORDER_POWER_ON) {
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
    if (unit->GetOrder() == ORDER_POWER_ON) {
        UnitsManager_RegisterButton(buttons, false, _(3f13), '7', &UnitsManager_Popup_OnClick_PowerOff);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(53c3), '2', &UnitsManager_Popup_OnClick_PowerOn);
    }

    UnitsManager_RegisterButton(buttons, false, _(1da0), '1', &UnitsManager_Popup_OnClick_BuyUpgrade);

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_InitRecreationCenter(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->GetOrder() == ORDER_POWER_OFF) {
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

    if (unit->GetOrder() == ORDER_POWER_OFF) {
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
    if (unit->GetOrder() == ORDER_POWER_OFF) {
        UnitsManager_RegisterButton(buttons, false, _(cec1), '1', &UnitsManager_Popup_OnClick_Activate);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(200c), '1', &UnitsManager_Popup_OnClick_Activate);
    }

    UnitsManager_RegisterButton(buttons, unit->cursor == 7, _(8452), '2', &UnitsManager_Popup_OnClick_Load);

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_OnClick_PowerOnAllocate(ButtonID bid, UnitInfo* unit) {
    GameManager_DeinitPopupButtons(true);

    if (unit->GetOrder() == ORDER_POWER_OFF) {
        UnitsManager_Popup_OnClick_PowerOn(0, unit);

    } else {
        GameManager_DisableMainMenu();
        AllocMenu_Menu(unit);
        GameManager_EnableMainMenu(unit);
    }
}

void UnitsManager_Popup_InitMiningStation(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->GetOrder() == ORDER_POWER_OFF) {
        UnitsManager_RegisterButton(buttons, false, _(98c9), '2', &UnitsManager_Popup_OnClick_PowerOnAllocate);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(33b7), '1', &UnitsManager_Popup_OnClick_PowerOnAllocate);
        UnitsManager_RegisterButton(buttons, false, _(d9c8), '7', &UnitsManager_Popup_OnClick_PowerOff);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_InitEcoSphere(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->GetOrder() == ORDER_POWER_ON) {
        UnitsManager_RegisterButton(buttons, false, _(df26), '7', &UnitsManager_Popup_OnClick_PowerOff);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(5fe9), '2', &UnitsManager_Popup_OnClick_PowerOn);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_InitPowerGenerators(UnitInfo* unit, struct PopupButtons* buttons) {
    if (unit->GetOrder() == ORDER_POWER_ON) {
        UnitsManager_RegisterButton(buttons, false, _(7a07), '7', &UnitsManager_Popup_OnClick_PowerOff);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(e7b6), '2', &UnitsManager_Popup_OnClick_PowerOn);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_Popup_OnClick_StopRemove(ButtonID bid, UnitInfo* unit) {
    SmartPointer<UnitInfo> rubble;

    GameManager_DeinitPopupButtons(true);

    if (unit->GetOrder() == ORDER_CLEAR) {
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

            UnitsManager_SetNewOrder(unit, ORDER_CLEAR, ORDER_STATE_INIT);
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
    if (unit->GetOrder() == ORDER_CLEAR) {
        UnitsManager_RegisterButton(buttons, false, _(bc20), '7', &UnitsManager_Popup_OnClick_StopRemove);

    } else {
        UnitsManager_RegisterButton(buttons, false, _(9737), '0', &UnitsManager_Popup_OnClick_StopRemove);
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

bool UnitsManager_FindValidPowerGeneratorPosition(uint16_t team, int16_t* grid_x, int16_t* grid_y) {
    *grid_x -= 1;
    *grid_y += 1;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, AccessModifier_SameClassBlocks)) {
        return true;
    }

    *grid_x += 3;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, AccessModifier_SameClassBlocks)) {
        return true;
    }

    *grid_x -= 3;
    *grid_y -= 1;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, AccessModifier_SameClassBlocks)) {
        return true;
    }

    *grid_x += 3;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, AccessModifier_SameClassBlocks)) {
        return true;
    }

    *grid_x -= 2;
    *grid_y -= 1;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, AccessModifier_SameClassBlocks)) {
        return true;
    }

    *grid_x += 1;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, AccessModifier_SameClassBlocks)) {
        return true;
    }

    *grid_x -= 1;
    *grid_y += 3;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, AccessModifier_SameClassBlocks)) {
        return true;
    }

    *grid_x += 1;

    if (Access_IsAccessible(POWGEN, team, *grid_x, *grid_y, AccessModifier_SameClassBlocks)) {
        return true;
    }

    return false;
}

void UnitsManager_Popup_OnClick_StartMasterBuilder(ButtonID bid, UnitInfo* unit) {
    int16_t grid_x;
    int16_t grid_y;

    GameManager_DeinitPopupButtons(true);

    grid_x = unit->move_to_grid_x;
    grid_y = unit->move_to_grid_y;

    if (UnitsManager_FindValidPowerGeneratorPosition(unit->team, &grid_x, &grid_y)) {
        UnitsManager_SetNewOrderInt(unit, ORDER_BUILD, ORDER_STATE_SELECT_SITE);
        GameManager_TempTape =
            UnitsManager_SpawnUnit(LRGTAPE, GameManager_PlayerTeam, unit->move_to_grid_x, unit->move_to_grid_y, unit);

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

        result = UnitsManager_IsAccessible(unit->team, MININGST, grid_x, grid_y);

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

                unit->move_to_grid_x = grid_x;
                unit->move_to_grid_y = grid_y;
            }
        }
    }

    UnitsManager_Popup_InitCommons(unit, buttons);
}

void UnitsManager_InitPopupMenus() {
    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        UnitsManager_DelayedAttackTargets[team].Clear();
    }

    UnitsManager_PendingAttacks.Clear();

    UnitsManager_DelayedReactionsTeam = PLAYER_TEAM_RED;

    UnitsManager_DelayedReactionsSyncCounter = 0;

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
            UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED) {
            UnitsManager_DelayedReactionsTeam = team;
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
    int32_t unit_turns = UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[unit2->team], unit2->GetUnitType())
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

    unit_type = unit->GetUnitType();
    parent = unit->GetParent();

    Hash_MapHash.Remove(unit);

    if (parent != nullptr) {
        Hash_MapHash.Remove(&*parent);

        unit_type = parent->GetConstructedUnitType();
    }

    if (ResourceManager_GetUnit(unit->GetUnitType()).GetFlags() & BUILDING) {
        result = UnitsManager_IsAccessible(unit->team, unit_type, grid_x, grid_y);

    } else {
        if (Access_GetModifiedSurfaceType(grid_x, grid_y) == SURFACE_TYPE_AIR) {
            result = 0;

        } else {
            result = Access_IsAccessible(unit_type, unit->team, grid_x, grid_y, AccessModifier_SameClassBlocks);
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
    if (unit->GetUnitType() == ENGINEER) {
        unit->move_to_grid_x = unit->grid_x;
        unit->move_to_grid_y = unit->grid_y;
    }

    unit->SetOrder(unit->GetPriorOrder());
    unit->SetOrderState(unit->GetPriorOrderState());

    unit->BuildOrder();

    {
        const char* const UnitsManager_BuildTimeEstimates[] = {_(360a), _(16f4), _(7101)};
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

        string.Sprintf(250, UnitsManager_BuildTimeEstimates[ResourceManager_GetUnit(unit_type).GetGender()],
                       ResourceManager_GetUnit(unit_type).GetSingularName().data(),
                       UnitsManager_TeamInfo[GameManager_PlayerTeam].unit_counters[unit_type], turns_to_build_unit);

        MessageManager_DrawMessage(string.GetCStr(), 0, unit, Point(unit->grid_x, unit->grid_y));
    }
}

void UnitsManager_UpdateMapHash(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    Hash_MapHash.Remove(unit);
    unit->UpdateGridPosition(grid_x, grid_y);
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

void UnitsManager_ScaleUnit(UnitInfo* unit, const UnitOrderStateType state) {
    if (GameManager_PlayerTeam == unit->team) {
        ResourceManager_GetSoundManager().PlaySfx(
            unit, state == ORDER_STATE_EXPAND ? Unit::SFX_TYPE_EXPAND : Unit::SFX_TYPE_SHRINK);
    }

    UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT_SCALING, state);
    unit->UpdateUnitDrawZones();
    unit->RefreshScreen();
}

void UnitsManager_ProcessOrders() {
    UnitsManager_BobEffectQuota = 5;

    Ai_ClearTasksPendingFlags();

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        UnitsManager_Units[team] = nullptr;
    }

    UnitsManager_OrdersPending = false;
    UnitsManager_CombatEffectsActive = false;

    UnitsManager_ProcessUnitOrders(&UnitsManager_GroundCoverUnits);
    UnitsManager_ProcessUnitOrders(&UnitsManager_MobileLandSeaUnits);
    UnitsManager_ProcessUnitOrders(&UnitsManager_StationaryUnits);
    UnitsManager_ProcessUnitOrders(&UnitsManager_MobileAirUnits);
    UnitsManager_ProcessUnitOrders(&UnitsManager_ParticleUnits);

    if (Remote_IsNetworkGame) {
        Remote_ProcessNetPackets();
    }

    if (!UnitsManager_OrdersPending) {
        if (UnitsManager_PendingAttacks.GetCount() > 0) {
            SmartList<UnitInfo>::Iterator unit_it(UnitsManager_PendingAttacks.Begin());

            SDL_assert((*unit_it).hits > 0);

            if ((*unit_it).shots > 0) {
                if ((*unit_it).GetOrder() == ORDER_FIRE) {
                    (*unit_it).SetOrderState(ORDER_STATE_ATTACK_BEGINNING);

                    UnitsManager_PendingAttacks.Remove(unit_it);
                }

            } else {
                AILOG(log, "{} at [{},{}] cannot fire.",
                      ResourceManager_GetUnit((*unit_it).GetUnitType()).GetSingularName().data(), (*unit_it).grid_x + 1,
                      (*unit_it).grid_y + 1);

                (*unit_it).UpdatePinCount((*unit_it).fire_on_grid_x, (*unit_it).fire_on_grid_y, -1);
                (*unit_it).RestoreOrders();

                if ((*unit_it).GetOrder() == ORDER_FIRE) {
                    AILOG_LOG(log, "Error, unit's prior orders were fire orders.");

                    (*unit_it).SetOrder(ORDER_AWAIT);
                    (*unit_it).SetOrderState(ORDER_STATE_EXECUTING_ORDER);
                }

                if (GameManager_SelectedUnit == *unit_it) {
                    GameManager_UpdateInfoDisplay(&*unit_it);
                }

                UnitsManager_PendingAttacks.Remove(unit_it);
            }

        } else {
            if (!UnitsManager_AssessAttacks()) {
                UnitsManager_ClearPins(&UnitsManager_MobileLandSeaUnits);
                UnitsManager_ClearPins(&UnitsManager_MobileAirUnits);
            }
        }
    }

    if (UnitsManager_PendingAirGroupLeader && Access_ProcessNewGroupOrder(UnitsManager_PendingAirGroupLeader.Get())) {
        UnitsManager_PendingAirGroupLeader = nullptr;
    }

    for (SmartList<UnitEvent>::Iterator it = UnitEvent_UnitEvents.Begin(); it != UnitEvent_UnitEvents.End(); ++it) {
        (*it).Process();
    }

    UnitEvent_UnitEvents.Clear();

    UnitsManager_ResetBobState = UnitsManager_BobEffectQuota;
}

void UnitsManager_SetNewOrderInt(UnitInfo* unit, const UnitOrderType order, const UnitOrderStateType state) {
    if (unit->GetOrder() != ORDER_EXPLODE && unit->GetOrderState() != ORDER_STATE_DESTROY) {
        bool delete_path = false;

        if (unit->GetOrder() == ORDER_AWAIT_SCALING) {
            AILOG(log, "New order ({}) issued for {} while scaling.", UnitsManager_Orders[order],
                  ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data());

            UnitsManager_NewOrderWhileScaling(unit);
        }

        if (unit->GetOrderState() == ORDER_STATE_NEW_ORDER) {
            AILOG(log, "New order ({}) issued for {} while waiting for path.", UnitsManager_Orders[order],
                  ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data());

            unit->SetOrder(ORDER_AWAIT);
            unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);

            delete_path = true;
        }

        if (order == ORDER_EXPLODE || unit->GetOrder() != ORDER_FIRE) {
            if (unit->GetOrder() != ORDER_NEW_ALLOCATE) {
                unit->SetPriorOrder(unit->GetOrder());
                unit->SetPriorOrderState(unit->GetOrderState());
            }

            unit->SetOrder(order);
            unit->SetOrderState(state);
        }

        if (delete_path) {
            ResourceManager_GetPathsManager().RemoveRequest(unit);
        }
    }
}

void UnitsManager_SetNewOrder(UnitInfo* unit, const UnitOrderType order, const UnitOrderStateType state) {
    UnitsManager_SetNewOrderInt(unit, order, state);

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_08(unit);
    }
}

bool UnitsManager_IsUnitUnderWater(UnitInfo* unit) {
    bool result;

    if (unit->GetUnitType() == SUBMARNE) {
        result = true;

    } else if (unit->GetUnitType() == CLNTRANS) {
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

            if (UnitsManager_IsFactory(unit->GetUnitType())) {
                ++team_info->stats_factories_built;
            }

            if (unit->GetUnitType() == MININGST) {
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

void UnitsManager_DestroyUnit(UnitInfo* unit, bool count_casualty) {
    SmartPointer<UnitInfo> unit_to_destroy(unit);

    AILOG(log, "{} at [{},{}] destroyed.", ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data(),
          unit->grid_x + 1, unit->grid_y + 1);

    // Count casualties for this unit and any units it's holding
    if (count_casualty && unit->GetId() != 0xFFFF && !(unit->flags & (EXPLODING | MISSILE_UNIT)) &&
        (unit->flags & SELECTABLE)) {
        const ResourceID unit_type = unit->GetUnitType();

        // Count casualties for units held by transports
        if (unit_type == HANGAR || unit_type == AIRPLT || unit_type == SEATRANS || unit_type == AIRTRANS ||
            unit_type == CLNTRANS) {
            SmartList<UnitInfo>* units;

            if (unit_type == HANGAR || unit_type == AIRPLT) {
                units = &UnitsManager_MobileAirUnits;

            } else {
                units = &UnitsManager_MobileLandSeaUnits;
            }

            for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
                if ((*it).GetOrder() == ORDER_IDLE && (*it).GetParent() == unit) {
                    const ResourceID held_unit_type = (*it).GetUnitType();

                    ++UnitsManager_TeamInfo[(*it).team].casualties[held_unit_type];

                    // Recursively count casualties for nested transports
                    if (held_unit_type == SEATRANS || held_unit_type == AIRTRANS || held_unit_type == CLNTRANS) {
                        for (SmartList<UnitInfo>::Iterator it2 = UnitsManager_MobileLandSeaUnits.Begin();
                             it2 != UnitsManager_MobileLandSeaUnits.End(); ++it2) {
                            if ((*it2).GetOrder() == ORDER_IDLE && (*it2).GetParent() == &*it) {
                                ++UnitsManager_TeamInfo[(*it2).team].casualties[(*it2).GetUnitType()];
                            }
                        }
                    }
                }
            }
        }

        // Count casualty for the unit itself
        ++UnitsManager_TeamInfo[unit->team].casualties[unit_type];
    }

    ResourceManager_GetPathsManager().RemoveRequest(unit);

    if (unit_to_destroy == GameManager_SelectedUnit) {
        ResourceManager_GetSoundManager().PlaySfx(unit, Unit::SFX_TYPE_INVALID);
        GameManager_SelectedUnit = nullptr;

        if (GameManager_IsMainMenuEnabled) {
            GameManager_MenuDeleteFlic();
            GameManager_FillOrRestoreWindow(WINDOW_CORNER_FLIC, COLOR_BLACK, true);
            GameManager_FillOrRestoreWindow(WINDOW_STAT_WINDOW, COLOR_BLACK, true);
            GameManager_DeinitPopupButtons(false);
            GameManager_UpdateDrawBounds();
        }
    }

    if (unit->GetPriorOrder() == ORDER_FIRE && unit->GetPriorOrderState() != ORDER_STATE_INIT) {
        unit->UpdatePinCount(unit->fire_on_grid_x, unit->fire_on_grid_y, -1);
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

    unit->hits = 0;
}

void UnitsManager_RemoveUnitFromUnitLists(UnitInfo* unit) {
    if (unit->flags & GROUND_COVER) {
        if (!UnitsManager_GroundCoverUnits.Remove(*unit)) {
            if (unit->GetUnitType() == BRIDGE) {
                UnitsManager_StationaryUnits.Remove(*unit);
            }
        }

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

        unit->GetBaseValues()->MarkAsInUse();

        team_info = &UnitsManager_TeamInfo[team];

        unit->unit_id = team_info->unit_counters[unit_type];

        ++team_info->unit_counters[unit_type];
    }

    if (GameManager_GameState != GAME_STATE_12_DEPLOYING_UNITS && !QuickBuild_MenuActive && !is_existing_unit) {
        unit->storage = 0;
    }

    if (unit->GetUnitType() == COMMANDO) {
        unit->experience = unit->GetBaseValues()->GetAttribute(ATTRIB_AGENT_ADJUST);
    }

    if (unit->flags & ANIMATED) {
        unit->SetOrder(ORDER_EXPLODE);
        unit->SetOrderState(ORDER_STATE_DESTROY);
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
                unit->SetOrder(ORDER_POWER_ON);
                unit->SetOrderState(ORDER_STATE_INIT);
                unit->SetPriorOrder(ORDER_POWER_ON);
                unit->SetPriorOrderState(ORDER_STATE_INIT);
            } break;

            case LANDMINE:
            case SEAMINE:
            case GUNTURRT:
            case ANTIAIR:
            case ARTYTRRT:
            case ANTIMSSL:
            case RADAR: {
                unit->SetOrder(ORDER_SENTRY);
                unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);
            } break;

            case POWERSTN:
            case POWGEN:
            case RESEARCH: {
                unit->SetOrder(ORDER_POWER_OFF);
                unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);
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
        unit->SetOrder(ORDER_DISABLE);
    }

    if (unit->flags & (TURRET_SPRITE | SPINNING_TURRET)) {
        unit->UpdateTurretAngle(unit->angle);

        if (unit->flags & SPINNING_TURRET) {
            unit->image_index_max =
                unit->turret_image_base + ResourceManager_GetUnit(unit_type).GetFrameInfo().turret_image_count - 1;
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

        Ai_UpdateTerrainDistanceField(unit);
    }

    return SmartPointer<UnitInfo>(unit);
}

void UnitsManager_FinishUnitScaling(UnitInfo* unit) {
    unit->RestoreOrders();

    if (unit->GetOrder() == ORDER_IDLE) {
        SmartPointer<UnitInfo> parent = unit->GetParent();

        if (parent->storage < parent->GetBaseValues()->GetAttribute(ATTRIB_STORAGE) && parent->team == unit->team) {
            unit->SetOrder(ORDER_AWAIT);

            Access_UpdateMapStatus(unit, false);

            unit->SetOrder(ORDER_IDLE);

            Hash_MapHash.Remove(unit);

            ++parent->storage;

            if (parent->flags & STATIONARY) {
                unit->ScheduleDelayedTasks(true);

            } else if (parent->GetTask()) {
                parent->GetTask()->EventUnitLoaded(*parent, *unit);
            }

        } else {
            unit->SetOrder(ORDER_AWAIT);
            unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);

            if (unit->flags & MOBILE_AIR_UNIT) {
                UnitsManager_SetNewOrderInt(unit, ORDER_TAKE_OFF, ORDER_STATE_INIT);
            }

            UnitsManager_ScaleUnit(unit, ORDER_STATE_EXPAND);
        }

        if (GameManager_SelectedUnit == parent) {
            GameManager_UpdateInfoDisplay(&*parent);
        }

        if (GameManager_SelectedUnit == unit) {
            ResourceManager_GetSoundManager().PlaySfx(unit, Unit::SFX_TYPE_INVALID);

            GameManager_UpdateInfoDisplay(unit);
            GameManager_AutoSelectNext(unit);
        }

        unit->RefreshScreen();

        GameManager_RenderMinimapDisplay = true;
    }
}

void UnitsManager_NewOrderWhileScaling(UnitInfo* unit) {
    if (unit->GetOrder() == ORDER_AWAIT_SCALING) {
        if (unit->GetOrderState() == ORDER_STATE_EXPAND) {
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
    if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
        if (GameManager_IsActiveTurn(unit->team)) {
            if (unit->auto_survey && unit->speed) {
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
    }
}

void UnitsManager_ProcessOrderAwait(UnitInfo* unit) {
    if (unit->GetUnitType() != ROAD && unit->GetUnitType() != WTRPLTFM) {
        switch (unit->GetOrderState()) {
            case ORDER_STATE_EXECUTING_ORDER: {
                if (unit->GetUnitType() == BRIDGE && unit->IsBridgeElevated()) {
                    if (!Access_GetActiveUnitWithFlags(unit->grid_x, unit->grid_y, MOBILE_SEA_UNIT)) {
                        UnitsManager_SetNewOrderInt(unit, ORDER_MOVE, ORDER_STATE_LOWER);
                    }
                }

                UnitsManager_PerformAutoSurvey(unit);
                unit->Animate();
            } break;

            case ORDER_STATE_READY_TO_EXECUTE_ORDER: {
                UnitsManager_PerformAutoSurvey(unit);
                unit->Animate();
            } break;

            case ORDER_STATE_CLEAR_PATH: {
                unit->path = nullptr;
                unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);

                if (GameManager_SelectedUnit == unit) {
                    GameManager_UpdateInfoDisplay(unit);
                    GameManager_UpdateDrawBounds();
                }

                if (unit->GetUnitType() == BRIDGE && unit->IsBridgeElevated() &&
                    !Access_GetActiveUnitWithFlags(unit->grid_x, unit->grid_y, MOBILE_SEA_UNIT)) {
                    UnitsManager_SetNewOrderInt(unit, ORDER_MOVE, ORDER_STATE_LOWER);
                }

                UnitsManager_PerformAutoSurvey(unit);
                unit->Animate();
            } break;
        }
    }
}

void UnitsManager_ProcessOrderTransform(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Transform");
    unit->RefreshScreen();

    switch (unit->GetOrderState()) {
        case ORDER_STATE_INIT: {
            UnitsManager_DeployMasterBuilderInit(unit);
        } break;

        case ORDER_STATE_PROGRESS_TRANSFORMING: {
            UnitsManager_DeployMasterBuilder(unit);
        } break;

        case ORDER_STATE_FINISH_TRANSFORMING: {
            unit->SetOrder(ORDER_POWER_ON);
            unit->SetOrderState(ORDER_STATE_INIT);
            UnitsManager_TeamInfo[unit->team].team_type = TEAM_TYPE_PLAYER;

            GameManager_OptimizeProduction(unit->team, unit->GetComplex(), true, true);
            GameManager_AutoSelectNext(unit);
            GameManager_EnableMainMenu(nullptr);
        } break;
    }
}

void UnitsManager_ProcessOrderMove(UnitInfo* unit) {
    if (unit->GetOrderState() == ORDER_STATE_ISSUING_PATH) {
        unit->Redraw();
        unit->SetOrderState(ORDER_STATE_IN_PROGRESS);
    }

    switch (unit->GetOrderState()) {
        case ORDER_STATE_EXECUTING_ORDER: {
            if (!UnitsManager_PursueEnemy(unit)) {
                UnitsManager_PerformAutoSurvey(unit);
                unit->Animate();
            }
        } break;

        case ORDER_STATE_STORE: {
            Ai_SetTasksPendingFlag("Storing");
            UnitsManager_Store(unit);
        } break;

        case ORDER_STATE_IN_PROGRESS: {
            if (unit->ExecuteMove()) {
                Ai_SetTasksPendingFlag("Moving");
                unit->Move();
            }
        } break;

        case ORDER_STATE_IN_TRANSITION: {
            Ai_SetTasksPendingFlag("Moving");
            unit->Move();
        } break;

        case ORDER_STATE_PATH_REQUEST_CANCEL: {
            unit->SetEnemy(nullptr);
            unit->BlockedOnPathRequest(false);
        } break;

        case ORDER_STATE_INIT:
        case ORDER_STATE_MOVE_INIT: {
            if (unit->IsVisibleToTeam(GameManager_PlayerTeam)) {
                if (GameManager_SelectedUnit == unit || GameManager_DisplayButtonRange ||
                    GameManager_DisplayButtonScan) {
                    GameManager_UpdateDrawBounds();
                }
            }

            if (UnitsManager_InitUnitMove(unit)) {
                if (unit->ExecuteMove()) {
                    Ai_SetTasksPendingFlag("Moving");
                    unit->Move();
                }
            }
        } break;

        case ORDER_STATE_MOVE_GETTING_PATH: {
            if (!UnitsManager_PursueEnemy(unit)) {
                UnitsManager_PerformAutoSurvey(unit);
                unit->Animate();
            }
        } break;

        case ORDER_STATE_ELEVATE: {
            if (unit->GetImageIndex() == unit->image_index_max) {
                UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT, ORDER_STATE_EXECUTING_ORDER);

            } else {
                if (!unit->IsBridgeElevated()) {
                    UnitsManager_GroundCoverUnits.Remove(*unit);
                    unit->AddToDrawList(STATIONARY | UPGRADABLE | SELECTABLE);
                }

                unit->DrawSpriteFrame(unit->GetImageIndex() + 1);
            }
        } break;

        case ORDER_STATE_LOWER: {
            if (Access_GetActiveUnitWithFlags(unit->grid_x, unit->grid_y, MOBILE_SEA_UNIT)) {
                UnitsManager_SetNewOrderInt(unit, ORDER_MOVE, ORDER_STATE_ELEVATE);

            } else {
                if (unit->IsBridgeElevated()) {
                    unit->DrawSpriteFrame(unit->GetImageIndex() - 1);

                } else {
                    UnitsManager_StationaryUnits.Remove(*unit);
                    unit->AddToDrawList();
                    UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT, ORDER_STATE_EXECUTING_ORDER);
                }
            }
        } break;

        case ORDER_STATE_NEW_ORDER: {
            if (unit->flags & MOBILE_AIR_UNIT) {
                if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_PLAYER && unit->GetUnitList()) {
                    UnitsManager_PendingAirGroupLeader = unit;
                }
            }

            if (!UnitsManager_PursueEnemy(unit)) {
                UnitsManager_PerformAutoSurvey(unit);
                unit->Animate();
            }
        } break;
    }
}

void UnitsManager_ProcessOrderFire(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Firing");

    UnitsManager_CombatEffectsActive = true;

    switch (unit->GetOrderState()) {
        case ORDER_STATE_INIT: {
            UnitsManager_PendingAttacks.PushBack(*unit);

            unit->UpdatePinCount(unit->fire_on_grid_x, unit->fire_on_grid_y, 1);

            unit->SetOrderState(ORDER_STATE_ATTACK_PENDING);
        } break;

        case ORDER_STATE_IN_PROGRESS: {
            UnitsManager_OrdersPending = true;

            if (unit->AimAtTarget()) {
                UnitsManager_OrdersPending = true;
                unit->PrepareFire();
            }
        } break;

        case ORDER_STATE_READY_TO_FIRE: {
            UnitsManager_OrdersPending = true;
            unit->PrepareFire();
        } break;

        case ORDER_STATE_FIRE_IN_PROGRESS: {
            UnitsManager_OrdersPending = true;
            unit->ProgressFire();
        } break;

        case ORDER_STATE_ATTACK_PENDING: {
        } break;

        case ORDER_STATE_ATTACK_BEGINNING: {
            unit->SetOrderState(ORDER_STATE_IN_PROGRESS);

            if (GameManager_SelectedUnit == unit) {
                ResourceManager_GetSoundManager().PlaySfx(unit, Unit::SFX_TYPE_TURRET);
            }

            UnitsManager_OrdersPending = true;

            if (unit->AimAtTarget()) {
                UnitsManager_OrdersPending = true;
                unit->PrepareFire();
            }
        } break;
    }
}

void UnitsManager_ProcessOrderBuild(UnitInfo* unit) {
    switch (unit->GetOrderState()) {
        case ORDER_STATE_INIT: {
            Ai_SetTasksPendingFlag("Build start");

            unit->StartBuilding();
        } break;

        case ORDER_STATE_IN_PROGRESS: {
            if (unit->ExecuteMove()) {
                Ai_SetTasksPendingFlag("Moving");

                unit->Move();
            }
        } break;

        case ORDER_STATE_IN_TRANSITION: {
            Ai_SetTasksPendingFlag("Moving");

            unit->Move();
        } break;

        case ORDER_STATE_BUILD_IN_PROGRESS: {
            if (ResourceManager_GetSettings()->GetNumericValue("effects") && unit->GetUnitType() == CONSTRCT) {
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
                ResourceManager_GetSoundManager().PlaySfx(unit, Unit::SFX_TYPE_BUILDING);
            }
        } break;

        case ORDER_STATE_BUILD_CANCEL: {
            Ai_SetTasksPendingFlag("Build cancel");

            unit->CancelBuilding();
        } break;

        case ORDER_STATE_SELECT_SITE: {
            unit->Animate();
        } break;

        case ORDER_STATE_BUILD_CLEARING: {
            Ai_SetTasksPendingFlag("Build clearing");

            UnitsManager_BuildClearing(unit, false);
        } break;

        case ORDER_STATE_UNIT_READY: {
            if (unit->path) {
                UnitsManager_BuildNext(unit);

            } else {
                unit->Animate();
            }
        } break;

        case ORDER_STATE_BUILD_ABORT: {
            Ai_SetTasksPendingFlag("Build cancel");

            unit->CancelBuilding();
        } break;
    }
}

void UnitsManager_ProcessOrderActivate(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Activation");

    switch (unit->GetOrderState()) {
        case ORDER_STATE_EXECUTING_ORDER: {
            UnitsManager_ActivateUnit(unit);
        } break;

        case ORDER_STATE_IN_TRANSITION: {
            unit->GetParent()->RefreshScreen();

            if (unit->GetUnitType() == CONSTRCT) {
                unit->SetOrderState(ORDER_STATE_BUILD_IN_PROGRESS);

                UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_TAPE_POSITIONING_DEINIT);

            } else {
                UnitsManager_ActivateEngineer(unit);
            }
        } break;

        case ORDER_STATE_BUILD_IN_PROGRESS: {
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
    if (unit->GetOrderState() == ORDER_STATE_INIT) {
        Ai_SetTasksPendingFlag("Power up");

        if (GameManager_SelectedUnit == unit) {
            ResourceManager_GetSoundManager().PlaySfx(unit, Unit::SFX_TYPE_POWER_CONSUMPTION_START);
        }

        unit->PowerUp(-1);
        unit->DrawSpriteFrame(unit->image_base + 1);

        if (unit->GetUnitType() == RESEARCH) {
            ResearchMenu_UpdateResearchProgress(unit->team, unit->research_topic, 1);
        }
    }

    unit->Animate();
}

void UnitsManager_ProcessOrderPowerOff(UnitInfo* unit) {
    if (unit->GetOrderState() == ORDER_STATE_INIT) {
        unit->PowerDown();
    }

    unit->Animate();
}

void UnitsManager_ProcessOrderExplode(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Exploding");

    switch (unit->GetOrderState()) {
        case ORDER_STATE_INIT: {
            UnitsManager_StartExplosion(unit);
        } break;

        case ORDER_STATE_DESTROY: {
            UnitsManager_ProgressExplosion(unit);
        } break;

        case ORDER_STATE_EXPLODE: {
            unit->hits = 0;

            unit->CheckIfDestroyed();

            UnitsManager_StartExplosion(unit);
        } break;
    }
}

void UnitsManager_ProcessOrderUnload(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Unloading");

    switch (unit->GetOrderState()) {
        case ORDER_STATE_INIT: {
            unit->moved = 0;
            unit->SetOrderState(ORDER_STATE_UNLOADING_IN_PROGRESS);

            const Point site{unit->grid_x, unit->grid_y};

            if (Paths_IsSiteReserved(site) || Paths_IsOccupied(unit->grid_x, unit->grid_y, 0, unit->team)) {
                unit->SetOrder(ORDER_AWAIT);
                unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);

            } else {
                Paths_ReserveSite(site);
                UnitsManager_ProgressUnloading(unit);
            }
        } break;

        case ORDER_STATE_UNLOADING_IN_PROGRESS: {
            UnitsManager_ProgressUnloading(unit);
        } break;

        case ORDER_STATE_FINISH_UNLOADING: {
            unit->ProcessUnloading();
        } break;
    }
}

void UnitsManager_ProcessOrderClear(UnitInfo* unit) {
    if (unit->GetOrderState() == ORDER_STATE_INIT) {
        Ai_SetTasksPendingFlag("Clear start");

        UnitsManager_StartClearing(unit);
    }
}

void UnitsManager_ProcessOrderSentry(UnitInfo* unit) {
    if ((unit->flags & ANIMATED) && ResourceManager_GetSettings()->GetNumericValue("effects")) {
        if (unit->GetImageIndex() + 1 <= unit->image_index_max) {
            unit->DrawSpriteFrame(unit->GetImageIndex() + 1);

        } else {
            unit->DrawSpriteFrame(unit->image_base);
        }

    } else {
        unit->Animate();
    }
}

void UnitsManager_ProcessOrderLand(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Landing");

    switch (unit->GetOrderState()) {
        case ORDER_STATE_INIT: {
            unit->moved = 0;
            unit->SetOrderState(ORDER_STATE_FINISH_LANDING);

            unit->ProcessLanding();
        } break;

        case ORDER_STATE_FINISH_LANDING: {
            unit->ProcessLanding();
        } break;
    }
}

void UnitsManager_ProcessOrderTakeOff(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Taking off");

    switch (unit->GetOrderState()) {
        case ORDER_STATE_INIT: {
            unit->moved = 0;
            unit->SetOrderState(ORDER_STATE_FINISH_TAKE_OFF);

            if (unit->Take()) {
                UnitsManager_RemoveUnitFromUnitLists(unit);

                unit->AddToDrawList();
                unit->RestoreOrders();
            }
        } break;

        case ORDER_STATE_FINISH_TAKE_OFF: {
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

    switch (unit->GetOrderState()) {
        case ORDER_STATE_INIT: {
            unit->moved = 0;
            unit->SetOrderState(ORDER_STATE_LOADING_IN_PROGRESS);

            UnitsManager_ScaleUnit(unit->GetParent(), ORDER_STATE_SHRINK);

            UnitsManager_ProgressLoading(unit);
        } break;

        case ORDER_STATE_LOADING_IN_PROGRESS: {
            UnitsManager_ProgressLoading(unit);
        } break;

        case ORDER_STATE_FINISH_LOADING: {
            unit->ProcessLoading();
        } break;
    }
}

void UnitsManager_ProcessOrderIdle(UnitInfo* unit) {
    if (unit->GetOrderState() == ORDER_STATE_BUILDING_READY) {
        UnitsManager_BuildingReady(unit);
    }
}

void UnitsManager_ProcessOrderRepair(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Repairing");

    unit->ProcessRepair();
}

void UnitsManager_ProcessOrderReload(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Reloading");

    unit->Reload(unit->GetParent());
}

void UnitsManager_ProcessOrderTransfer(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Tranferring");

    unit->ProcessTransfer();
}

void UnitsManager_ProcessOrderHaltBuilding(UnitInfo* unit) {
    if (unit->GetOrderState() == ORDER_STATE_BUILD_CANCEL) {
        Ai_SetTasksPendingFlag("Build halting");

        unit->CancelBuilding();
    }
}

void UnitsManager_ProcessOrderAwaitScaling(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Scaling");

    unit->RefreshScreen();

    if (unit->GetPriorOrder() == ORDER_AWAIT_DISABLE_UNIT || unit->GetPriorOrder() == ORDER_AWAIT_STEAL_UNIT) {
        UnitsManager_CombatEffectsActive = true;
    }

    switch (unit->GetOrderState()) {
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
    UnitsManager_CombatEffectsActive = true;

    Ai_SetTasksPendingFlag("Disable/Steal");

    switch (unit->GetOrderState()) {
        case ORDER_STATE_INIT: {
            unit->SetOrderState(ORDER_STATE_IN_PROGRESS);

            UnitsManager_ScaleUnit(unit, ORDER_STATE_SHRINK);
        } break;

        case ORDER_STATE_EXECUTING_ORDER: {
            if (UnitsManager_AttemptStealthAction(unit)) {
                if (unit->GetOrder() == ORDER_AWAIT_STEAL_UNIT) {
                    UnitsManager_CaptureUnit(unit);

                } else {
                    UnitsManager_DisableUnit(unit);
                }
            }

            unit->SetOrder(ORDER_AWAIT);
        } break;

        case ORDER_STATE_IN_PROGRESS: {
            unit->GetParent()->ShakeSabotage();

            if (unit->GetParent()->shake_effect_state == 0) {
                unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);

                UnitsManager_ScaleUnit(unit, ORDER_STATE_EXPAND);
            }
        } break;
    }
}

void UnitsManager_ProcessOrderDisable(UnitInfo* unit) { unit->Animate(); }

void UnitsManager_ProcessOrderUpgrade(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("Upgrading");

    unit->Upgrade(unit->GetParent());
}

void UnitsManager_ProcessOrderLayMine(UnitInfo* unit) {
    Ai_SetTasksPendingFlag("lay mines");

    switch (unit->GetOrderState()) {
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
    if ((unit->flags & SPINNING_TURRET) && unit->GetOrder() != ORDER_DISABLE &&
        ResourceManager_GetSettings()->GetNumericValue("effects")) {
        unit->SpinningTurretAdvanceAnimation();
    }

    if ((unit->flags & MISSILE_UNIT) || unit->GetOrderState() == ORDER_STATE_DESTROY) {
        UnitsManager_OrdersPending = true;
        UnitsManager_CombatEffectsActive = true;
    }

    switch (unit->GetOrder()) {
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

void UnitsManager_ProcessUnitOrders(SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        UnitsManager_ProcessOrder(&*it);
    }
}

void UnitsManager_ActivateEngineer(UnitInfo* unit) {
    Access_DestroyUtilities(unit->grid_x, unit->grid_y, false, false, false, false);

    unit->SetPriorOrder(ORDER_AWAIT);
    unit->SetPriorOrderState(ORDER_STATE_EXECUTING_ORDER);
    unit->SetOrder(ORDER_MOVE);
    unit->SetOrderState(ORDER_STATE_INIT);
    unit->GetParent()->SetParent(unit);
    unit->SetParent(nullptr);

    unit->SetSpriteFrameForTerrain(unit->grid_x, unit->grid_y);
}

void UnitsManager_DeployMasterBuilderInit(UnitInfo* unit) {
    if (GameManager_SelectedUnit == unit) {
        GameManager_DisableMainMenu();
    }

    unit->DeployConstructionSiteUtilities(MININGST);
    unit->SetOrderState(ORDER_STATE_PROGRESS_TRANSFORMING);

    UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_TAPE_POSITIONING_INIT);
}

bool UnitsManager_IsFactory(ResourceID unit_type) {
    return unit_type == SHIPYARD || unit_type == LIGHTPLT || unit_type == LANDPLT || unit_type == AIRPLT;
}

void UnitsManager_AddToDelayedReactionList(UnitInfo* unit) {
    if (!ResourceManager_GetSettings()->GetNumericValue("disable_fire")) {
        UnitsManager_DelayedAttackTargets[unit->team].PushBack(*unit);
        UnitsManager_DelayedReactionsPending = true;
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
    UnitsManager_DelayedReactionsPending = false;

    UnitsManager_ClearDelayedReaction(&UnitsManager_MobileLandSeaUnits);
    UnitsManager_ClearDelayedReaction(&UnitsManager_MobileAirUnits);
    UnitsManager_ClearDelayedReaction(&UnitsManager_StationaryUnits);

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        UnitsManager_DelayedAttackTargets[team].Clear();
    }
}

void UnitsManager_DeployMasterBuilder(UnitInfo* unit) {
    uint16_t unit_team = unit->team;
    SmartPointer<Task> unit_task(unit->GetTask());
    SmartPointer<UnitInfo> mining_station;
    SmartPointer<UnitInfo> power_generator;
    SmartPointer<UnitInfo> small_slab;

    int32_t mining_station_grid_x = unit->move_to_grid_x;
    int32_t mining_station_grid_y = unit->move_to_grid_y;

    UnitsManager_DestroyUnit(unit);

    Access_DestroyUtilities(mining_station_grid_x, mining_station_grid_y, false, false, false, true);

    mining_station =
        UnitsManager_DeployUnit(MININGST, unit_team, nullptr, mining_station_grid_x, mining_station_grid_y, 0);

    int16_t power_generator_grid_x = mining_station_grid_x;
    int16_t power_generator_grid_y = mining_station_grid_y;

    UnitsManager_FindValidPowerGeneratorPosition(unit_team, &power_generator_grid_x, &power_generator_grid_y);

    power_generator = UnitsManager_DeployUnit(POWGEN, unit_team, mining_station->GetComplex(), mining_station_grid_x,
                                              mining_station_grid_y, 0);

    small_slab =
        UnitsManager_DeployUnit(SMLSLAB, unit_team, nullptr, power_generator_grid_x, power_generator_grid_y,
                                Randomizer_Generate(ResourceManager_GetUnit(SMLSLAB).GetFrameInfo().image_count));

    UnitsManager_SetInitialMining(&*mining_station, mining_station_grid_x, mining_station_grid_y);

    mining_station->scaler_adjust = 4;
    small_slab->scaler_adjust = 4;
    power_generator->scaler_adjust = 4;

    mining_station->SetOrder(ORDER_TRANSFORM);
    mining_station->SetOrderState(ORDER_STATE_FINISH_TRANSFORMING);

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

    if (unit->GetOrder() == ORDER_MOVE_TO_ATTACK && unit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER &&
        !unit->delayed_reaction && !UnitsManager_Units[unit->team]) {
        UnitInfo* enemy_unit = unit->GetEnemy();
        int32_t unit_range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
        Point position;

        if (enemy_unit) {
            position = UnitsManager_GetAttackPosition(unit, enemy_unit);

            if (enemy_unit->hits > 0 && unit->ammo > 0 && unit->team != enemy_unit->team &&
                enemy_unit->GetOrder() != ORDER_IDLE &&
                (enemy_unit->IsVisibleToTeam(unit->team) || !enemy_unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED))) {
                if ((UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER ||
                     !unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED)) &&
                    Access_GetSquaredDistance(unit, position) > unit_range * unit_range) {
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
            if (Access_GetSquaredDistance(unit, position) > unit_range * unit_range &&
                (!unit->path || unit->move_to_grid_x != enemy_unit->grid_x ||
                 unit->move_to_grid_y != enemy_unit->grid_y) &&
                ((enemy_unit->GetOrder() != ORDER_MOVE && enemy_unit->GetOrder() != ORDER_MOVE_TO_UNIT &&
                  enemy_unit->GetOrder() != ORDER_MOVE_TO_ATTACK) ||
                 enemy_unit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER)) {
                if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_REMOTE) {
                    result = false;

                } else {
                    unit->SetOrder(ORDER_AWAIT);

                    UnitsManager_SetNewOrder(unit, ORDER_MOVE_TO_ATTACK, ORDER_STATE_MOVE_INIT);

                    result = true;
                }

            } else {
                if (unit->shots > 0 && Access_IsWithinAttackRange(unit, position.x, position.y, unit_range)) {
                    if (UnitsManager_ParticleUnits.GetCount() > 0) {
                        result = false;

                    } else {
                        UnitsManager_Units[unit->team] = unit;

                        UnitsManager_DelayedReactionsPending = true;

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

bool UnitsManager_InitUnitMove(UnitInfo* unit) {
    unit->RefreshScreen();
    UnitsManager_InitUnitPath(unit);

    if (unit->GetOrderState() == ORDER_STATE_INIT) {
        unit->FollowUnit();
        unit->Redraw();

        unit->SetOrderState(ORDER_STATE_IN_PROGRESS);

        if (Remote_IsNetworkGame && (unit->flags & MOBILE_AIR_UNIT)) {
            Remote_SendNetPacket_38(unit);

            return false;
        }
    }

    return (unit->GetOrderState() == ORDER_STATE_IN_PROGRESS);
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

    unit->SetOrder(ORDER_IDLE);
    unit->SetOrderState(ORDER_STATE_STORE);

    unit->ClearUnitList();
}

void UnitsManager_BuildClearing(UnitInfo* unit, bool mode) {
    ResourceID unit_type = unit->GetUnitType();
    uint16_t unit_team = unit->team;
    uint32_t unit_flags = unit->flags;
    int32_t unit_grid_x = unit->grid_x;
    int32_t unit_grid_y = unit->grid_y;
    ResourceID rubble_type = INVALID_ID;
    int32_t cargo_amount = 0;
    int32_t unit_orders = unit->GetOrder();

    unit->RefreshScreen();

    if (unit_flags & STATIONARY) {
        UnitsManager_RemoveConnections(unit);

        if (unit->GetUnitType() == RESEARCH) {
            if (unit->GetOrder() == ORDER_POWER_ON && unit->GetOrderState() != ORDER_STATE_INIT) {
                ResearchMenu_UpdateResearchProgress(unit->team, unit->research_topic, -1);
            }
        }

        if (unit->GetUnitType() == GREENHSE) {
            UnitsManager_TeamInfo[unit->team].team_points -= unit->storage;
        }
    }

    if (mode && unit->GetUnitType() != COMMANDO && unit->GetUnitType() != INFANTRY &&
        !(unit_flags & (CONNECTOR_UNIT | HOVERING)) &&
        Access_IsFullyLandCovered(unit_grid_x, unit_grid_y, unit_flags)) {
        if (unit_flags & BUILDING) {
            rubble_type = LRGRUBLE;

        } else {
            rubble_type = SMLRUBLE;
        }

        cargo_amount = unit->GetNormalRateBuildCost() / 2;

        if (ResourceManager_GetUnit(unit->GetUnitType()).GetCargoType() == Unit::CargoType::CARGO_TYPE_RAW) {
            cargo_amount += unit->storage;
        }
    }

    if ((unit_flags & (MOBILE_AIR_UNIT | MOBILE_LAND_UNIT | STATIONARY)) && !(unit->flags & HOVERING)) {
        Access_DestroyGroundCovers(unit);

        if (unit->flags & BUILDING) {
            Access_ValidateUnitsOnTerrain(unit_grid_x, unit_grid_y, unit);
            Access_ValidateUnitsOnTerrain(unit_grid_x + 1, unit_grid_y, unit);
            Access_ValidateUnitsOnTerrain(unit_grid_x, unit_grid_y + 1, unit);
            Access_ValidateUnitsOnTerrain(unit_grid_x + 1, unit_grid_y + 1, unit);

        } else {
            Access_ValidateUnitsOnTerrain(unit_grid_x, unit_grid_y, unit);
        }
    }

    if (unit_type == LANDMINE || unit_type == SEAMINE) {
        SmartPointer<UnitInfo> target_unit(
            Access_GetEnemyUnit(unit_team, unit_grid_x, unit_grid_y, (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)));

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

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); Access_IsHeldByUnit(unit, units, &it);
         UnitsManager_DestroyUnit(it->Get())) {
        if ((*it).GetUnitType() == SEATRANS || (*it).GetUnitType() == AIRTRANS || (*it).GetUnitType() == CLNTRANS) {
            for (SmartList<UnitInfo>::Iterator it2 = UnitsManager_MobileLandSeaUnits.Begin();
                 Access_IsHeldByUnit(it->Get(), &UnitsManager_MobileLandSeaUnits, &it2);
                 UnitsManager_DestroyUnit(it2->Get())) {
            }
        }
    }

    bool skip_rubble_removal = false;

    if (unit_type == BULLDOZR && unit_orders == ORDER_CLEAR) {
        skip_rubble_removal = true;
    }

    if (unit_type == ENGINEER && unit_orders == ORDER_BUILD && unit->build_list.GetCount() > 0 &&
        *unit->build_list[0] == CNCT_4W) {
        skip_rubble_removal = true;
    }

    UnitsManager_DestroyUnit(unit);

    if ((unit_flags & STATIONARY) && UnitsManager_TeamInfo[unit_team].team_type != TEAM_TYPE_REMOTE) {
        UnitsManager_TeamInfo[unit_team].team_units->OptimizeComplexes(unit_team);
    }

    if ((unit_flags & (BUILDING | STANDALONE)) || unit_orders == ORDER_BUILD || unit_orders == ORDER_CLEAR) {
        if (unit_type == CONSTRCT && unit_orders == ORDER_BUILD && rubble_type == SMLRUBLE) {
            SmartPointer<UnitInfo> utility_unit(Access_GetConstructionUtility(unit_team, unit_grid_x, unit_grid_y));

            unit_grid_x = utility_unit->grid_x;
            unit_grid_y = utility_unit->grid_y;

            if (Access_IsFullyLandCovered(unit_grid_x, unit_grid_y, utility_unit->flags)) {
                rubble_type = LRGRUBLE;

            } else {
                rubble_type = INVALID_ID;
            }
        }

        Access_DestroyUtilities(unit_grid_x, unit_grid_y, true, !skip_rubble_removal, false, false);
    }

    if (rubble_type != INVALID_ID) {
        int32_t image_index = ResourceManager_GetUnit(rubble_type).GetFrameInfo().image_count - 1;

        image_index = Randomizer_Generate(image_index + 1);

        UnitInfo* rubble_unit =
            &*UnitsManager_DeployUnit(rubble_type, unit_team, nullptr, unit_grid_x, unit_grid_y, image_index);

        rubble_unit->storage = cargo_amount;
    }

    GameManager_RenderMinimapDisplay = true;
}

void UnitsManager_BuildNext(UnitInfo* unit) {
    ResourceID unit_type = *unit->GetBuildList()[0];
    int32_t turns_to_build = BuildMenu_GetTurnsToBuild(unit_type, unit->team);

    if (unit->storage >= turns_to_build * Cargo_GetRawConsumptionRate(unit->GetUnitType(), 1) &&
        (unit->path->GetEndX() != unit->grid_x || unit->path->GetEndY() != unit->grid_y)) {
        bool remove_road = unit_type != CNCT_4W && unit_type != ROAD && unit_type != BRIDGE && unit_type != WTRPLTFM;
        bool remove_connectors =
            unit_type != CNCT_4W && unit_type != ROAD && unit_type != BRIDGE && unit_type != WTRPLTFM;

        Access_DestroyUtilities(unit->grid_x, unit->grid_y, false, false, remove_connectors, remove_road);

        unit->SetOrderState(ORDER_STATE_IN_PROGRESS);

    } else {
        unit->ClearBuildListAndPath();
    }
}

void UnitsManager_ActivateUnit(UnitInfo* unit) {
    SmartPointer<UnitInfo> client(unit->GetParent());

    SDL_assert(client.Get());

    if (((client->flags & MOBILE_AIR_UNIT) && client->speed > 0) ||
        (!Paths_IsOccupied(unit->move_to_grid_x, unit->move_to_grid_y, 0, unit->team) &&
         !(client->flags & MOBILE_AIR_UNIT))) {
        SDL_assert(unit->storage > 0);

        --unit->storage;

        unit->SetParent(nullptr);

        if (unit->GetBuildList().GetCount() > 0) {
            unit->SetOrder(ORDER_BUILD);
            unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);
            unit->build_time = BuildMenu_GetTurnsToBuild(unit->GetConstructedUnitType(), unit->team);

            unit->StartBuilding();

        } else {
            unit->SetOrder(ORDER_AWAIT);
            unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);
        }

        if (GameManager_SelectedUnit == client) {
            DrawMap_RenderBuildMarker();
        }

        if (client->flags & MOBILE_AIR_UNIT) {
            client->SetParent(nullptr);
            client->move_to_grid_x = unit->move_to_grid_x;
            client->move_to_grid_y = unit->move_to_grid_y;
            client->SetOrder(ORDER_MOVE);
            client->SetOrderState(ORDER_STATE_INIT);

            Access_UpdateMapStatus(client.Get(), true);
            UnitsManager_ScaleUnit(client.Get(), ORDER_STATE_EXPAND);

        } else {
            // Check for enemy mines at the activation location BEFORE moving unit to that cell
            Access_TriggerEnemyMine(client->team, unit->move_to_grid_x, unit->move_to_grid_y);

            UnitsManager_UpdateMapHash(client.Get(), unit->move_to_grid_x, unit->move_to_grid_y);

            client->SetOrder(ORDER_AWAIT);
            client->SetOrderState(ORDER_STATE_EXECUTING_ORDER);

            client->SetSpriteFrameForTerrain(unit->move_to_grid_x, unit->move_to_grid_y);

            if ((client->flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) == MOBILE_SEA_UNIT) {
                UnitInfo* bridge_unit = Access_GetBridge(unit->move_to_grid_x, unit->move_to_grid_y);

                if (bridge_unit) {
                    UnitsManager_SetNewOrderInt(bridge_unit, ORDER_MOVE, ORDER_STATE_ELEVATE);
                }
            }

            client->InitStealthStatus();

            Access_UpdateMapStatus(client.Get(), true);

            UnitsManager_ScaleUnit(client.Get(), ORDER_STATE_EXPAND);
        }

        if (GameManager_SelectedUnit == client) {
            GameManager_MenuUnitSelect(client.Get());

            if (GameManager_DisplayButtonRange || GameManager_DisplayButtonScan) {
                GameManager_UpdateDrawBounds();
            }
        }

        if (unit->GetTask() &&
            (unit->GetUnitType() == AIRTRANS || unit->GetUnitType() == SEATRANS || unit->GetUnitType() == CLNTRANS)) {
            unit->GetTask()->EventUnitUnloaded(*unit, *client);
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
        ResourceManager_GetSoundManager().PlaySfx(unit, Unit::SFX_TYPE_HIT);

    } else {
        if (GameManager_SelectedUnit == unit) {
            ResourceManager_GetSoundManager().PlaySfx(unit, Unit::SFX_TYPE_INVALID);
        }

        ResourceManager_GetSoundManager().PlaySfx(unit, Unit::SFX_TYPE_EXPLOAD);
    }

    if ((unit->GetUnitType() == COMMANDO || unit->GetUnitType() == INFANTRY) && unit->hits == 0) {
        if (unit->GetUnitType() == COMMANDO) {
            unit->image_base = 168;

        } else {
            unit->image_base = 168;
        }

        unit->image_base += (unit->angle / 2) * 8;
        unit->image_index_max = unit->image_base + 7;
        unit->DrawSpriteFrame(unit->image_base);

        unit->SetOrderState(ORDER_STATE_DESTROY);
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

        {
            auto& base_unit = ResourceManager_GetUnit(unit_type);

            if (unit_flags & GROUND_COVER) {
                base_unit.SetFlags(MOBILE_LAND_UNIT);

            } else if (unit_flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
                base_unit.SetFlags(STATIONARY);

            } else if (unit_flags & STATIONARY) {
                base_unit.SetFlags(MOBILE_AIR_UNIT);

            } else {
                base_unit.SetFlags(MISSILE_UNIT);
            }
        }

        explosion = UnitsManager_DeployUnit(unit_type, unit->team, nullptr, unit->grid_x, unit->grid_y, 0, true);

        if (unit_type != BLDEXPLD) {
            explosion->OffsetDrawZones(unit->x - explosion->x, unit->y - explosion->y);
        }

        unit->RestoreOrders();

        if (unit->hits == 0) {
            explosion->SetParent(unit);
            unit->SetOrderState(ORDER_STATE_DESTROY);

            if (GameManager_SelectedUnit == unit && ResourceManager_GetSettings()->GetNumericValue("auto_select")) {
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
            ResourceManager_GetSoundManager().PlaySfx(unit, Unit::SFX_TYPE_POWER_CONSUMPTION_END, true);
        }

        SmartPointer<UnitInfo> client(unit->GetParent());

        // Check for enemy mines at the unload location BEFORE placing the unit
        Access_TriggerEnemyMine(client->team, unit->grid_x, unit->grid_y);

        UnitsManager_UpdateMapHash(client.Get(), unit->grid_x, unit->grid_y);

        client->RestoreOrders();

        client->SetSpriteFrameForTerrain(unit->grid_x, unit->grid_y);

        Access_UpdateMapStatus(client.Get(), true);

        UnitsManager_ScaleUnit(client.Get(), ORDER_STATE_EXPAND);

        if (UnitsManager_TeamInfo[client->team].team_type == TEAM_TYPE_PLAYER) {
            GameManager_RenderMinimapDisplay = true;
        }

        SDL_assert(unit->storage > 0);

        --unit->storage;

        unit->moved = 0;

        unit->SetOrderState(ORDER_STATE_FINISH_UNLOADING);

        const Point site{client->grid_x, client->grid_y};

        Paths_RemoveSiteReservation(site);

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

    unit->SetOrderState(ORDER_STATE_IN_PROGRESS);

    if (tape_type == LRGTAPE) {
        UnitsManager_SetNewOrderInt(unit, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_TAPE_POSITIONING_INIT);
    }

    unit->DrawSpriteFrame(unit->GetImageIndex() + 8);

    if (GameManager_SelectedUnit == unit) {
        ResourceManager_GetSoundManager().PlaySfx(unit, Unit::SFX_TYPE_BUILDING);
    }
}

void UnitsManager_ProgressLoading(UnitInfo* unit) {
    if (unit->Land()) {
        UnitValues* base_values = unit->GetBaseValues();
        SmartPointer<UnitInfo> parent(unit->GetParent());

        if (parent->hits > 0 && parent->GetOrder() != ORDER_FIRE && parent->GetOrder() != ORDER_EXPLODE &&
            parent->GetOrderState() != ORDER_STATE_DESTROY) {
            if (GameManager_SelectedUnit == unit) {
                ResourceManager_GetSoundManager().PlaySfx(unit, Unit::SFX_TYPE_POWER_CONSUMPTION_START, true);
            }

            parent->path = nullptr;
            parent->SetParent(unit);

            Hash_MapHash.Remove(&*parent);

            if (parent->GetOrder() != ORDER_DISABLE) {
                parent->SetOrder(ORDER_AWAIT);
                parent->SetOrderState(ORDER_STATE_EXECUTING_ORDER);
            }

            Access_UpdateMapStatus(&*parent, false);

            UnitsManager_SetNewOrderInt(&*parent, ORDER_IDLE, ORDER_STATE_PREPARE_STORE);

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
        unit->SetOrderState(ORDER_STATE_FINISH_LOADING);

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
            if (parent->GetOrder() == ORDER_MOVE && parent->GetOrderState() != ORDER_STATE_EXECUTING_ORDER) {
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
        if (unit->GetUnitType() != CNCT_4W) {
            bool remove_road = unit->GetUnitType() != ROAD;

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

bool UnitsManager_AttemptStealthAction(UnitInfo* unit) {
    SmartPointer<UnitInfo> parent(unit->GetParent());
    int32_t stealth_chance = UnitsManager_GetStealthChancePercentage(unit, &*parent, unit->GetOrder());
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

        if (unit->stealth_dice_roll <= stealth_chance) {
            if (parent->GetOrder() != ORDER_DISABLE) {
                ++unit->experience;
            }

            if (GameManager_SelectedUnit == unit) {
                GameManager_MenuUnitSelect(unit);
            }

            result = true;

        } else {
            if (parent->GetOrder() == ORDER_DISABLE) {
                if (GameManager_PlayerTeam == unit->team) {
                    if (unit->GetOrder() == ORDER_AWAIT_STEAL_UNIT) {
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
                    if (unit->GetOrder() == ORDER_AWAIT_STEAL_UNIT) {
                        GameManager_NotifyEvent(&*parent, 4);

                    } else {
                        GameManager_NotifyEvent(&*parent, 5);
                    }

                } else if (GameManager_PlayerTeam == unit->team) {
                    if (unit->GetOrder() == ORDER_AWAIT_STEAL_UNIT) {
                        MessageManager_DrawMessage(_(f876), 1, unit, Point(unit->grid_x, unit->grid_y));

                    } else {
                        MessageManager_DrawMessage(_(a939), 1, unit, Point(unit->grid_x, unit->grid_y));
                    }

                    ResourceManager_GetSoundManager().PlayVoice(V_M007, V_F012);
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
        ResourceManager_GetSoundManager().PlayVoice(V_M239, V_F242);
    }

    if (parent->GetOrder() == ORDER_BUILD || parent->GetOrder() == ORDER_CLEAR) {
        if (parent->GetOrderState() == ORDER_STATE_SELECT_SITE) {
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

        ResourceManager_GetSoundManager().PlayVoice(V_M244, V_F244);
    }

    switch (parent->GetOrder()) {
        case ORDER_MOVE: {
            parent->MoveFinished(false);
            parent->SetOrderState(ORDER_STATE_EXECUTING_ORDER);
        } break;

        case ORDER_BUILD:
        case ORDER_CLEAR: {
            if (parent->GetOrderState() == ORDER_STATE_SELECT_SITE) {
                GameManager_SelectBuildSite(&*parent);

            } else {
                do {
                    parent->CancelBuilding();
                } while (parent->GetOrder() == ORDER_HALT_BUILDING);

                if (!(parent->flags & STATIONARY)) {
                    unit->SetOrder(ORDER_AWAIT);
                    parent->BusyWaitOrder();
                }
            }
        } break;

        case ORDER_POWER_ON: {
            UnitsManager_SetNewOrderInt(&*parent, ORDER_POWER_OFF, ORDER_STATE_EXECUTING_ORDER);
            parent->PowerDown();

            is_found = true;
        } break;
    }

    SmartPointer<UnitInfo> unit_copy(parent->MakeCopy());

    parent->path = nullptr;

    UnitsManager_SetNewOrderInt(&*parent, ORDER_DISABLE, ORDER_STATE_EXECUTING_ORDER);

    parent->RemoveTasks();
    parent->disabled_turns_remaining = turns_disabled;

    Access_UpdateMapStatus(&*parent, true);
    Access_UpdateMapStatus(&*unit_copy, false);

    Ai_UnitSpotted(unit, parent->team);

    if (UnitsManager_TeamInfo[parent->team].team_type == TEAM_TYPE_PLAYER) {
        GameManager_RenderMinimapDisplay = true;
    }

    if (is_found) {
        ProductionManager_OptimizeProduction(parent->team, parent->GetComplex(), &*parent,
                                             GameManager_PlayerTeam == parent->team);
    }
}

bool UnitsManager_AssessAttacks() {
    bool result;

    if (ResourceManager_GetSettings()->GetNumericValue("disable_fire")) {
        result = false;

    } else {
        AILOG(log, "Assess pending attacks.");

        bool are_attacks_delayed = false;

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            if (UnitsManager_DelayedAttackTargets[team].GetCount() > 0 || UnitsManager_Units[team]) {
                are_attacks_delayed = true;
            }
        }

        if (are_attacks_delayed) {
            if (UnitsManager_TeamInfo[UnitsManager_DelayedReactionsTeam].team_type == TEAM_TYPE_REMOTE) {
                result = true;

            } else {
                are_attacks_delayed = false;

                uint16_t team = UnitsManager_DelayedReactionsTeam;

                do {
                    if (UnitsManager_CheckDelayedReactions(UnitsManager_DelayedReactionsTeam)) {
                        are_attacks_delayed = true;
                    }

                    UnitsManager_DelayedReactionsTeam =
                        (UnitsManager_DelayedReactionsTeam + 1) % TRANSPORT_MAX_TEAM_COUNT;

                    if (UnitsManager_TeamInfo[UnitsManager_DelayedReactionsTeam].team_type == TEAM_TYPE_REMOTE) {
                        if (are_attacks_delayed) {
                            UnitsManager_DelayedReactionsPending = true;

                        } else if (UnitsManager_DelayedReactionsPending) {
                            UnitsManager_DelayedReactionsPending = false;

                        } else {
                            UnitsManager_ClearDelayedReactions();
                        }

                        ++UnitsManager_DelayedReactionsSyncCounter;

                        Remote_SendNetPacket_46(UnitsManager_DelayedReactionsTeam, are_attacks_delayed,
                                                UnitsManager_DelayedReactionsSyncCounter);

                        return true;
                    }

                    if (are_attacks_delayed) {
                        return true;
                    }

                } while (team != UnitsManager_DelayedReactionsTeam);

                UnitsManager_ClearDelayedReactions();

                result = UnitsManager_IsAttackScheduled();
            }

        } else {
            UnitsManager_DelayedReactionsPending = false;

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

    if ((unit1->GetUnitType() == SP_FLAK || unit1->GetUnitType() == ANTIAIR || unit1->GetUnitType() == FASTBOAT) &&
        !(unit2->flags & MOBILE_AIR_UNIT)) {
        result = false;

    } else if (unit2->hits > 0) {
        if (unit1->GetOrder() == ORDER_IDLE) {
            result = false;

        } else if ((unit1->GetOrder() == ORDER_MOVE || unit1->GetOrder() == ORDER_MOVE_TO_UNIT ||
                    unit1->GetOrder() == ORDER_MOVE_TO_ATTACK) &&
                   unit1->GetOrderState() != ORDER_STATE_EXECUTING_ORDER) {
            result = false;

        } else if (unit1->GetOrder() == ORDER_DISABLE) {
            result = false;

        } else if (unit1->GetOrderState() == ORDER_STATE_EXPLODE) {
            result = false;

        } else if (unit2->IsVisibleToTeam(unit1->team)) {
            if ((unit1->GetUnitType() == COMMANDO || unit1->GetUnitType() == SUBMARNE) &&
                !unit1->IsVisibleToTeam(unit2->team)) {
                result = false;

            } else if (UnitsManager_TeamInfo[unit1->team].team_type == TEAM_TYPE_COMPUTER) {
                result = Ai_IsTargetTeam(unit1, unit2);

            } else {
                if (GameManager_PlayMode == PLAY_MODE_TURN_BASED || unit1->GetOrder() == ORDER_SENTRY) {
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
                        if ((*it).team == unit1_team && Access_GetSquaredDistance(unit2, &*it) <= unit2_distance &&
                            Access_IsValidAttackTarget(unit2, &*it)) {
                            return true;
                        }
                    }

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                         it != UnitsManager_MobileAirUnits.End(); ++it) {
                        if ((*it).team == unit1_team && Access_GetSquaredDistance(unit2, &*it) <= unit2_distance &&
                            Access_IsValidAttackTarget(unit2, &*it)) {
                            return true;
                        }
                    }

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                         it != UnitsManager_StationaryUnits.End(); ++it) {
                        if ((*it).team == unit1_team && Access_GetSquaredDistance(unit2, &*it) <= unit2_distance &&
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
        unit1->fire_on_grid_x = unit2->grid_x;
        unit1->fire_on_grid_y = unit2->grid_y;

        if (GameManager_PlayerTeam == unit1->team) {
            const char* const UnitsManager_ReactionsToEnemy[] = {_(1b93), _(5a01), _(eb57)};
            const Unit& base_unit = ResourceManager_GetUnit(unit2->GetUnitType());
            Point position(unit1->grid_x, unit1->grid_y);
            SmartString message;

            message.Sprintf(150, UnitsManager_ReactionsToEnemy[base_unit.GetGender()],
                            ResourceManager_GetUnit(unit1->GetUnitType()).GetSingularName().data(), unit1->grid_x + 1,
                            unit1->grid_y + 1, base_unit.GetSingularName().data(), unit2->grid_x + 1,
                            unit2->grid_y + 1);

            MessageManager_DrawMessage(message.GetCStr(), 0, unit1, position);
        }

        UnitsManager_SetNewOrder(unit1, ORDER_FIRE, ORDER_STATE_INIT);

        if (GameManager_PlayerTeam == unit1->team && GameManager_SelectedUnit == unit1 &&
            !GameManager_IsInsideMapView(unit1)) {
            ResourceManager_GetSoundManager().PlayVoice(V_M250, V_F251);
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
    int32_t distance_x = unit->fire_on_grid_x - unit->grid_x;
    int32_t distance_y = unit->fire_on_grid_y - unit->grid_y;
    int32_t distance = std::sqrt(distance_x * distance_x + distance_y * distance_y) * 4.0 + 0.5;
    AirPath* result;

    if (distance > 0) {
        result = new (std::nothrow)
            AirPath(unit, distance_x, distance_y, distance / 4, unit->fire_on_grid_x, unit->fire_on_grid_y);

    } else {
        result = nullptr;
    }

    return result;
}

void UnitsManager_InitUnitPath(UnitInfo* unit) {
    unit->path = nullptr;

    if (unit->flags & MISSILE_UNIT) {
        unit->path = UnitsManager_GetMissilePath(unit);

    } else {
        if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_REMOTE) {
            unit->SetOrderState(ORDER_STATE_NEW_ORDER);

        } else {
            if (unit->GetOrderState() == ORDER_STATE_MOVE_INIT) {
                if ((unit->flags & MOBILE_AIR_UNIT) && unit->GetOrder() != ORDER_MOVE_TO_ATTACK) {
                    unit->path = Paths_GetAirPath(unit);

                    unit->RestoreOrders();

                    if (Remote_IsNetworkGame) {
                        Remote_SendNetPacket_38(unit);
                    }

                } else {
                    if (Paths_RequestPath(unit, AccessModifier_SameClassBlocks)) {
                        unit->SetOrderState(ORDER_STATE_MOVE_GETTING_PATH);
                    }
                }

            } else {
                if (unit->GetUnitList()) {
                    Access_GroupAttackOrder(unit, true);

                } else {
                    if ((unit->flags & MOBILE_AIR_UNIT) && unit->GetOrder() != ORDER_MOVE_TO_ATTACK) {
                        unit->path = Paths_GetAirPath(unit);

                    } else {
                        if (Paths_RequestPath(unit, AccessModifier_SameClassBlocks)) {
                            unit->SetOrderState(ORDER_STATE_NEW_ORDER);
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
        if ((*it).GetOrder() == ORDER_FIRE) {
            return true;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).GetOrder() == ORDER_FIRE) {
            return true;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetOrder() == ORDER_FIRE) {
            return true;
        }
    }

    return false;
}

Point UnitsManager_GetAttackPosition(UnitInfo* unit1, UnitInfo* unit2) {
    if (unit2->GetUnitType() == CONSTRCT && unit2->GetOrder() == ORDER_BUILD) {
        auto utility_unit = Access_GetConstructionUtility(unit2->team, unit2->grid_x, unit2->grid_y);

        if (utility_unit) {
            unit2 = utility_unit;
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

        if (unit1->GetUnitType() == SUBMARNE || unit1->GetUnitType() == CORVETTE) {
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
        AILOG(log, "Check pending reactions.");

        if (UnitsManager_Units[team]) {
            UnitInfo* unit1 = UnitsManager_Units[team].Get();
            UnitInfo* unit2 = unit1->GetEnemy();
            Point position = UnitsManager_GetAttackPosition(unit1, unit2);

            unit1->fire_on_grid_x = position.x;
            unit1->fire_on_grid_y = position.y;

            if (UnitsManager_TeamInfo[unit1->team].team_type == TEAM_TYPE_COMPUTER &&
                unit1->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE) && unit1->shots == 1) {
                unit1->SetOrder(ORDER_AWAIT);

                unit1->SetEnemy(nullptr);
            }

            UnitsManager_SetNewOrder(unit1, ORDER_FIRE, ORDER_STATE_INIT);

            if (GameManager_PlayerTeam == unit1->team && GameManager_SelectedUnit == unit1 &&
                !GameManager_IsInsideMapView(unit1)) {
                ResourceManager_GetSoundManager().PlayVoice(V_M250, V_F251);
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

    if (target->GetUnitType() == SUBMARNE &&
        (attacker->GetUnitType() == BOMBER || attacker->GetUnitType() == ALNPLANE)) {
        attacker_attack /= 2;
    }

    if (target_armor >= attacker_attack) {
        result = 1;

    } else {
        result = attacker_attack - target_armor;
    }

    return result;
}

bool UnitsManager_IsAccessible(uint16_t team, ResourceID unit_type, int32_t grid_x, int32_t grid_y) {
    bool result;

    if (grid_x >= 0 && grid_y >= 0 && grid_x <= ResourceManager_MapSize.x - 2 &&
        grid_y <= ResourceManager_MapSize.x - 2) {
        result = Access_IsAccessible(unit_type, team, grid_x, grid_y, AccessModifier_SameClassBlocks) &&
                 Access_IsAccessible(unit_type, team, grid_x + 1, grid_y, AccessModifier_SameClassBlocks) &&
                 Access_IsAccessible(unit_type, team, grid_x, grid_y + 1, AccessModifier_SameClassBlocks) &&
                 Access_IsAccessible(unit_type, team, grid_x + 1, grid_y + 1, AccessModifier_SameClassBlocks);
    } else {
        result = 0;
    }

    return result;
}

bool UnitsManager_IssueBuildOrder(UnitInfo* unit, int16_t* grid_x, int16_t* grid_y, ResourceID unit_type) {
    const Unit& base_unit = ResourceManager_GetUnit(unit_type);
    bool result;
    uint16_t team;

    team = unit->team;

    if (unit->flags & STATIONARY) {
        result = true;

    } else {
        Hash_MapHash.Remove(unit);

        if (base_unit.GetFlags() & BUILDING) {
            result = UnitsManager_IsAccessible(team, unit_type, *grid_x, *grid_y) ||
                     UnitsManager_IsAccessible(team, unit_type, *grid_x, --*grid_y) ||
                     UnitsManager_IsAccessible(team, unit_type, --*grid_x, *grid_y) ||
                     UnitsManager_IsAccessible(team, unit_type, *grid_x, ++*grid_y);

        } else {
            result = Access_IsAccessible(unit_type, team, *grid_x, *grid_y, AccessModifier_SameClassBlocks);
        }

        Hash_MapHash.Add(unit);
    }

    return result;
}

bool UnitsManager_LoadUnit(UnitInfo* unit) {
    UnitInfo* shop;
    bool result;

    shop = Access_GetReceiverUnit(unit, unit->grid_x, unit->grid_y);

    if (shop) {
        if (shop->GetUnitType() == LANDPAD && !Access_GetUnit2(unit->grid_x, unit->grid_y, unit->team)) {
            unit->SetParent(nullptr);
            unit->SetOrder(ORDER_LAND);
            unit->SetOrderState(ORDER_STATE_INIT);

            result = true;

        } else if (shop->GetUnitType() == HANGAR && unit->GetOrder() == ORDER_MOVE_TO_UNIT) {
            const int32_t stored_units = Access_GetStoredUnitCount(shop);
            const int32_t storable_units = shop->GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            SDL_assert(stored_units <= storable_units && shop->storage == stored_units);

            if (stored_units == storable_units) {
                unit->SetOrder(ORDER_AWAIT);
                unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);

                if (GameManager_SelectedUnit == unit) {
                    MessageManager_DrawMessage(_(7c8d), 1, unit, Point(unit->grid_x, unit->grid_y));
                }

            } else {
                unit->SetParent(shop);
                unit->SetOrder(ORDER_LAND);
                unit->SetOrderState(ORDER_STATE_INIT);

                if (GameManager_SelectedUnit == unit) {
                    GameManager_MenuUnitSelect(shop);
                }
            }

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}
