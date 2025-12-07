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

#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include "button.hpp"
#include "flicsmgr.hpp"
#include "smartstring.hpp"
#include "unitinfo.hpp"

void GameManager_GameLoop(int32_t game_state);
void GameManager_AddDrawBounds(Rect* bounds);
void GameManager_UpdateDrawBounds();
void GameManager_DeployUnit(uint16_t team, ResourceID unit_type, int32_t grid_x, int32_t grid_y);
void GameManager_DrawUnitSelector(uint8_t* buffer, int32_t pitch, int32_t offsetx, int32_t height, int32_t offsety,
                                  int32_t bottom, int32_t item_height, int32_t top, int32_t scaling_factor,
                                  int32_t is_big_sprite, bool double_marker = false);
bool GameManager_RefreshOrders(uint16_t team, bool check_production);
void GameManager_HandleTurnTimer();
void GameManager_ProcessState(bool process_tick, bool clear_mouse_events = true);
bool GameManager_ProcessTick(bool render_screen);
void GameManager_GuiSwitchTeam(uint16_t team);
bool GameManager_LoadGame(int32_t save_slot, Color* palette_buffer);
void GameManager_NotifyEvent(UnitInfo* unit, int32_t event);
void GameManager_SelectBuildSite(UnitInfo* unit);
void GameManager_EnableMainMenu(UnitInfo* unit);
void GameManager_DisableMainMenu();
void GameManager_MenuDeleteFlic();
void GameManager_GetScaledMessageBoxBounds(Rect* bounds);
void GameManager_MenuUnitSelect(UnitInfo* unit);
void GameManager_FillOrRestoreWindow(uint8_t id, int32_t color, bool redraw);
void GameManager_UpdateInfoDisplay(UnitInfo* unit);
bool GameManager_IsInsideMapView(UnitInfo* unit);
bool GameManager_IsActiveTurn(uint16_t team);
bool GameManager_OptimizeProduction(uint16_t team, Complex* complex, bool is_player_team, bool mode);
void GameManager_SelectNextUnit(int32_t seek_direction);
bool GameManager_UpdateMapDrawBounds(int32_t ulx, int32_t uly);
void GameManager_UpdateMainMapView(int32_t mode, int32_t ulx, int32_t uly, bool flag = true);
void GameManager_AutoSelectNext(UnitInfo* unit);
void GameManager_DrawTurnTimer(int32_t turn_time, bool mode = false);
Color* GameManager_MenuFadeOut(int32_t time_limit = 100);
int32_t GameManager_CheckLandingZones(uint16_t team1, uint16_t team2);
void GameManager_MenuInitButtons(bool mode);
void GameManager_MenuDeinitButtons();
void GameManager_InitLandingSequenceMenu(bool enable_controls);
void GameManager_DeinitPopupButtons(bool clear_mouse_events);
SmartString GameManager_GetUnitStatusMessage(UnitInfo* unit);
uint8_t GameManager_GetDialogWindowCenterMode();

extern uint32_t GameManager_LastZoomLevel;
extern uint16_t GameManager_MultiChatTargets[PLAYER_TEAM_MAX - 1];
extern Rect GameManager_MapView;
extern Rect GameManager_MapWindowDrawBounds;
extern SmartPointer<UnitInfo> GameManager_SelectedUnit;
extern SmartPointer<UnitInfo> GameManager_TempTape;
extern SmartList<UnitInfo> GameManager_LockedUnits;
extern int32_t GameManager_TurnCounter;
extern int32_t GameManager_TurnTimerValue;
extern bool GameManager_MaxSpy;
extern int32_t GameManager_HumanPlayerCount;
extern bool GameManager_AllVisible;
extern bool GameManager_RealTime;
extern bool GameManager_RequestMenuExit;
extern bool GameManager_IsCheater;
extern uint8_t GameManager_CheaterTeam;
extern bool GameManager_DisplayControlsInitialized;
extern bool GameManager_RenderMinimapDisplay;
extern bool GameManager_PlayFlic;
extern bool GameManager_MaxSurvey;
extern bool GameManager_QuickBuildMenuActive;
extern bool GameManager_IsMainMenuEnabled;
extern bool GameManager_WrapUpGame;
extern Button* Gamemanager_FlicButton;
extern uint8_t GameManager_PlayMode;
extern bool GameManager_FastMovement;
extern uint8_t GameManager_ActiveTurnTeam;
extern uint8_t GameManager_PlayerTeam;
extern uint8_t GameManager_GameState;

extern bool GameManager_DisplayButtonLock;
extern bool GameManager_DisplayButtonScan;
extern bool GameManager_DisplayButtonRange;
extern bool GameManager_DisplayButtonGrid;
extern bool GameManager_DisplayButtonSurvey;
extern bool GameManager_DisplayButtonStatus;
extern bool GameManager_DisplayButtonColors;
extern bool GameManager_DisplayButtonHits;
extern bool GameManager_DisplayButtonAmmo;
extern bool GameManager_DisplayButtonNames;
extern bool GameManager_DisplayButtonMinimap2x;
extern bool GameManager_DisplayButtonMinimapTnt;

extern SmartPointer<UnitInfo> GameManager_QuickBuilderUnit;

extern int32_t ini_setting_victory_type;
extern int32_t ini_setting_victory_limit;

#endif /* GAME_MANAGER_HPP */
