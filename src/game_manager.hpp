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
#include "unitinfo.hpp"

void GameManager_GameLoop(int game_state);
void GameManager_AddDrawBounds(Rect* bounds);
void GameManager_UpdateDrawBounds();
void GameManager_DeployUnit(unsigned short team, ResourceID unit_type, int grid_x, int grid_y);
void GameManager_DrawUnitSelector(unsigned char* buffer, int width, int offsetx, int height, int offsety, int bottom,
                                  int item_height, int top, int scaling_factor, int is_big_sprite,
                                  bool double_marker = false);
bool GameManager_RefreshOrders(unsigned short team, bool check_production);
void GameManager_HandleTurnTimer();
void GameManager_ProcessState(bool process_tick, bool clear_mouse_events = true);
bool GameManager_ProcessTick(bool render_screen);
void GameManager_GuiSwitchTeam(unsigned short team);
bool GameManager_LoadGame(int save_slot, Color* palette_buffer, bool is_text_mode);
void GameManager_NotifyEvent(UnitInfo* unit, int event);
void GameManager_SelectBuildSite(UnitInfo* unit);
void GameManager_EnableMainMenu(UnitInfo* unit);
void GameManager_DisableMainMenu();
void GameManager_MenuDeleteFlic();
void GameManager_GetScaledMessageBoxBounds(Rect* bounds);
void GameManager_MenuUnitSelect(UnitInfo* unit);
void GameManager_FillOrRestoreWindow(unsigned char id, int color, bool redraw);
void GameManager_UpdateInfoDisplay(UnitInfo* unit);
bool GameManager_IsAtGridPosition(UnitInfo* unit);
bool GameManager_OptimizeProduction(unsigned short team, Complex* complex, bool is_player_team, bool mode);
void GameManager_SelectNextUnit(int seek_direction);
bool GameManager_UpdateMapDrawBounds(int ulx, int uly);
void GameManager_UpdateMainMapView(int mode, int ulx, int uly, bool flag = true);
void GameManager_AutoSelectNext(UnitInfo* unit);
void GameManager_DrawTurnTimer(int turn_time, bool mode = false);
Color* GameManager_MenuFadeOut(int fade_steps = 50);
int GameManager_CheckLandingZones(unsigned short team1, unsigned short team2);
void GameManager_MenuInitButtons(bool mode);
void GameManager_MenuDeinitButtons();
void GameManager_InitLandingSequenceMenu(bool enable_controls);
void GameManager_DeinitPopupButtons(bool clear_mouse_events);
SmartString GameManager_GetUnitStatusMessage(UnitInfo* unit);

extern unsigned short GameManager_MultiChatTargets[PLAYER_TEAM_MAX - 1];
extern Rect GameManager_GridPosition;
extern Rect GameManager_MapWindowDrawBounds;
extern SmartPointer<UnitInfo> GameManager_SelectedUnit;
extern SmartPointer<UnitInfo> GameManager_TempTape;
extern SmartList<UnitInfo> GameManager_LockedUnits;
extern int GameManager_TurnCounter;
extern int GameManager_TurnTimerValue;
extern bool GameManager_MaxSpy;
extern int GameManager_GameFileNumber;
extern int GameManager_HumanPlayerCount;
extern bool GameManager_AllVisible;
extern bool GameManager_RealTime;
extern bool GameManager_RequestMenuExit;
extern unsigned short Gfx_MapWindowWidth;
extern bool GameManager_DisplayControlsInitialized;
extern bool GameManager_RenderMinimapDisplay;
extern bool GameManager_PlayFlic;
extern bool GameManager_MaxSurvey;
extern bool GameManager_UnknownFlag3;
extern bool GameManager_MainMenuFreezeState;
extern bool GameManager_WrapUpGame;
extern Button* Gamemanager_FlicButton;
extern unsigned char GameManager_PlayMode;
extern bool GameManager_FastMovement;
extern unsigned char GameManager_ActiveTurnTeam;
extern unsigned short GameManager_MainMapWidth;
extern unsigned short GameManager_MainMapHeight;
extern char GameManager_PlayerTeam;
extern char GameManager_GameState;

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

#endif /* GAME_MANAGER_HPP */
