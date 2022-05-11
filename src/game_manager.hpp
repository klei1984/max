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

#include "UnitInfo.hpp"
#include "button.hpp"
#include "flicsmgr.hpp"

void GameManager_GameLoop(int game_state);
void GameManager_AddDrawBounds(Rect* bounds);
void GameManager_UpdateDrawBounds();
void GameManager_DrawUnitSelector(unsigned char* buffer, int width, int offsetx, int height, int offsety, int bottom,
                                  int item_height, int top, int scaling_factor, int is_big_sprite,
                                  bool double_marker = false);
bool GameManager_ProcessTick(bool render_screen);
void GameManager_ProcessState(bool process_tick, bool clear_mouse_events = true);
void GameManager_GuiSwitchTeam(unsigned short team);
void GameManager_EnableMainMenu(UnitInfo* unit);
void GameManager_DisableMainMenu();
void GameManager_MenuDeleteFlic();
void GameManager_GetScaledMessageBoxBounds(Rect* bounds);
void GameManager_MenuUnitSelect(UnitInfo* unit);
void GameManager_FillOrRestoreWindow(unsigned char id, int color, bool redraw);
void GameManager_UpdateMainMapView(int mode, int grid_x_zoom_level_max, int grid_y_zoom_level_min, bool flag = true);
void GameManager_DrawTurnTimer(int turn_time, bool mode = false);
int GameManager_CheckLandingZones(unsigned short team1, unsigned short team2);
void GameManager_MenuInitButtons(bool mode);
void GameManager_MenuDeinitButtons();
void GameManager_InitLandingSequenceMenu(bool enable_controls);

extern Rect GameManager_GridPosition;
extern Rect GameManager_MapWindowDrawBounds;
extern SmartPointer<UnitInfo> GameManager_SelectedUnit;
extern SmartList<UnitInfo> GameManager_LockedUnits;
extern unsigned int GameManager_TurnCounter;
extern int GameManager_GameFileNumber;
extern int GameManager_HumanPlayerCount;
extern bool GameManager_AllVisible;
extern bool GameManager_RequestMenuExit;
extern unsigned short Gfx_MapWindowWidth;
extern bool GameManager_DisplayControlsInitialized;
extern bool GameManager_PlayFlic;
extern bool GameManager_MaxSurvey;
extern Button* Gamemanager_FlicButton;

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
