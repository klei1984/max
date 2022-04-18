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

#include "game_manager.hpp"

#include <time.h>

#include "ai.hpp"
#include "cargomenu.hpp"
#include "gui.hpp"
#include "inifile.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "movie.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "saveloadmenu.hpp"
#include "sound_manager.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

#define MENU_GUI_ITEM_DEF(id, gfx, label, button, state, sfx) \
    { (id), (gfx), (label), (button), (state), (sfx) }

struct MenuLandingSequence {
    WindowInfo* panel_top;
    WindowInfo* panel_bottom;
    Image* image_1;
    Image* image_2;
    Button* button_1;
    Button* button_2;
    unsigned int time_stamp;
};

struct MenuGuiItem {
    char unknown;
    ResourceID gfx;
    const char* label;
    Button* button;
    bool disabled;
    ResourceID sfx;
};

struct PopupButtons {
    unsigned char popup_count;
    unsigned char position[10];
    unsigned char key_code[10];
    char* caption[10];
    char state[10];
    ButtonFunc* r_func[10];
    Button* button[10];
    unsigned short ulx;
    unsigned short uly;
    unsigned short width;
    unsigned short height;
};

Rect GameManager_GridPosition;
SmartPointer<UnitInfo> GameManager_SelectedUnit;
Image* GameManager_TurnTimerImage;
Point GameManager_GridCenter;
Point GameManager_SpottedEnemyPosition;
SmartList<UnitInfo> GameManager_LockedUnits;
unsigned int GameManager_TurnCounter;
unsigned int GameManager_ArrowKeyFlags;
unsigned int GameManager_TurnTimerValue;
int GameManager_GameFileNumber;
int GameManager_HumanPlayerCount;
bool GameManager_RequestMenuExit;
bool GameManager_UnknownFlag;
bool GameManager_DemoMode;
bool GameManager_IsCheater;
bool GameManager_AllVisible;
bool GameManager_RealTime;
bool GameManager_PlayScenarioIntro = true;

bool GameManager_DisableMapRendering;

bool GameManager_DisplayButtonLock;
bool GameManager_DisplayButtonScan;
bool GameManager_DisplayButtonRange;
bool GameManager_DisplayButtonGrid;
bool GameManager_DisplayButtonSurvey;
bool GameManager_DisplayButtonStatus;
bool GameManager_DisplayButtonColors;
bool GameManager_DisplayButtonHits;
bool GameManager_DisplayButtonAmmo;
bool GameManager_DisplayButtonNames;
bool GameManager_DisplayButtonMinimap2x;
bool GameManager_DisplayButtonMinimapTnt;

bool GameManager_SiteSelectState;
bool GameManager_WrapUpGame;
bool GameManager_IsTurnTimerActive;
unsigned char GameManager_RenderState;
bool GameManager_RenderEnable;
bool GameManager_PlayFlic;
bool GameManager_UnknownFlag1;
bool GameManager_MaxSurvey;
bool GameManager_UnknownFlag3;

unsigned short GameManager_ActiveTurnTeam;
unsigned char GameManager_MarkerColor = 0xFF;
unsigned char GameManager_PlayMode;
unsigned char GameManager_FastMovement;
unsigned short GameManager_MultiChatTargets[PLAYER_TEAM_MAX - 1];
unsigned int GameManager_MapBrightness;

MenuLandingSequence GameManager_LandingSequence;

static struct MenuGuiItem GameManager_MenuItems[] = {
    MENU_GUI_ITEM_DEF(4, FILES_OF, "Files", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(5, PREF_OFF, "Preferences", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(7, PLAY_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(8, PAUSE_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(14, FIND_OFF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(16, PREV_OF, "Prev", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(17, UDONE_OF, "Done", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(18, NEXT_OF, "Next", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(19, HELP_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(20, REPT_OFF, "Reports", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(21, CHAT_OFF, "Chat", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(26, SURV_OFF, "Survey", nullptr, false, ISURV0),
    MENU_GUI_ITEM_DEF(27, STAT_OFF, "Status", nullptr, false, ISTAT0),
    MENU_GUI_ITEM_DEF(28, COLOR_OF, "Colors", nullptr, false, ICOLO0),
    MENU_GUI_ITEM_DEF(29, HITS_OF, "Hits", nullptr, false, IHITS0),
    MENU_GUI_ITEM_DEF(30, AMMO_OF, "Ammo", nullptr, false, IAMMO0),
    MENU_GUI_ITEM_DEF(31, RANG_OFF, "Range", nullptr, false, IRANG0),
    MENU_GUI_ITEM_DEF(32, VISN_OFF, "Scan", nullptr, false, IVISI0),
    MENU_GUI_ITEM_DEF(33, GRID_OFF, "Grid", nullptr, false, IGRID0),
    MENU_GUI_ITEM_DEF(34, NAMES_UP, "Names", nullptr, false, INAME0),
    MENU_GUI_ITEM_DEF(15, LOCK_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(36, MIN2X_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(37, MINFL_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(3, ENDTRN_U, "End Turn", nullptr, false, MENUOP),
};

struct PopupButtons GameManager_PopupButtons;

bool GameManager_PlayerMissionSetup(unsigned short team);
void GameManager_DrawSelectSiteMessage(unsigned short team);
bool GameManager_SelectSite(unsigned short team);
void GameManager_InitMap();
void GameManager_UpdateMainMapView(int mode, int grid_x_zoom_level_max, int grid_y_zoom_level_min, bool flag = true);
static void GameManager_GameSetup(int game_state);
static void GameManager_GameLoopCleanup();
static unsigned short GameManager_EvaluateWinner();
static void GameManager_DrawTurnCounter(int turn_count);
static void GameManager_UpdateTurnTimer(bool mode, int turn_time);
static void GameManager_DrawDisplayPanel(int control_id, char* text, int color, int ulx = 0);
static void GameManager_ProgressBuildState(unsigned short team);
static void GameManager_sub_9E750(unsigned short team, int game_state, bool enable_autosave);
static bool GameManager_AreTeamsFinishedTurn();
static void GameManager_ProgressTurn();
static void GameManager_sub_953C7();
static void GameManager_AnnounceWinner(unsigned short team);
static void GameManager_UpdateScoreGraph();
static void GameManager_InitUnitsAndGameState();
static bool GameManager_InitGame();
static void GameManager_MenuDeletePanelButtons(MenuLandingSequence* control);
static Color* GameManager_MenuFadeOut(int fade_steps);
static void GameManager_MenuAnimateOpenControlPanel(MenuLandingSequence* control);
static void GameManager_UpdateHumanPlayerCount();
static void GameManager_MenuAnimateDisplayControls();
static void GameManager_MenuInitDisplayControls();
static void GameManager_DrawMouseCoordinates(int x, int y);
static bool GameManager_HandleProximityOverlaps();
static void GameManager_PopulatelMapWithResources();
static void GameManager_PopulatelMapWithAlienUnits(int alien_seperation, int alien_unit_value);
static void GameManager_ProcessTeamMissionSupplyUnits(unsigned short team);

void GameManager_GameLoop(int game_state) {
    unsigned int turn_counter;
    unsigned short team_winner;
    unsigned short team;
    bool enable_autosave;

    if (game_state == GAME_STATE_10 && ini_get_setting(INI_GAME_FILE_TYPE) == GAME_TYPE_DEMO) {
        GameManager_DemoMode = true;
    } else {
        GameManager_DemoMode = false;
    }

    GameManager_GameSetup(game_state);

    turn_counter = GameManager_TurnCounter;

    while (GUI_GameState == GAME_STATE_8_IN_GAME) {
        team_winner = GameManager_EvaluateWinner();
        GameManager_DrawTurnCounter(GameManager_TurnCounter);

        if (GameManager_PlayMode == PLAY_MODE_SIMULTANEOUS_MOVES) {
            if (Remote_IsNetworkGame) {
                if (ini_get_setting(INI_LOG_FILE_DEBUG)) {
                    MessageManager_DrawMessage("Starting announcement phase...", 0, 0);
                    Remote_AnalyzeDesync();
                }

                Remote_sub_C9753();
            }

            for (team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (game_state != GAME_STATE_10 && GUI_PlayerTeamIndex != team &&
                    (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER ||
                     UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_REMOTE)) {
                    GameManager_ActiveTurnTeam = team;
                    GameManager_ProgressBuildState(GameManager_ActiveTurnTeam);
                }
            }

            for (team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (team != GUI_PlayerTeamIndex && UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                    /// \todo Ai_sub_48A8D(team);
                }
            }

            GameManager_sub_9E750(GUI_PlayerTeamIndex, game_state, true);
            /// \todo enable_main_menu(nullptr);

            if (GUI_GameState == GAME_STATE_9 && !GameManager_AreTeamsFinishedTurn()) {
                if (Remote_IsNetworkGame) {
                    MessageManager_DrawMessage("Waiting for remote End Turn.", 0, 0);
                } else {
                    MessageManager_DrawMessage("Waiting for computer to finish turn.", 0, 0);
                }

                while (GUI_GameState == GAME_STATE_9 && GameManager_AreTeamsFinishedTurn()) {
                    GameManager_ProgressTurn();
                }
            }

            if (GUI_GameState == GAME_STATE_9) {
                GameManager_PlayMode = PLAY_MODE_UNKNOWN;
                GameManager_sub_953C7();

                /// \todo
                // while (GUI_GameState == GAME_STATE_9 && are_task_events_pending()) {
                //   GameManager_ProgressTurn();
                // }
            }

            GameManager_PlayMode = PLAY_MODE_SIMULTANEOUS_MOVES;

            GameManager_GuiSwitchTeam(GUI_PlayerTeamIndex);

            if (GUI_GameState == GAME_STATE_10) {
                game_state = GAME_STATE_10;
                GUI_GameState = GAME_STATE_8_IN_GAME;
            } else if (GUI_GameState == GAME_STATE_9) {
                for (team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                    if (UnitsManager_TeamInfo[team].field_41) {
                        /// \todo update_unit_orders_complexes_research();
                    }
                }

                game_state = GAME_STATE_8_IN_GAME;
            }

        } else {
            if (game_state == GAME_STATE_10) {
                team = GameManager_ActiveTurnTeam;
            } else {
                team = PLAYER_TEAM_RED;
            }

            enable_autosave = true;

            for (; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
                    UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED) {
                    if (game_state == GAME_STATE_10) {
                        /// \todo update_unit_orders_complexes_research(team);
                    }

                    if (GameManager_DemoMode) {
                        /// \todo update_minimap_fog_of_war(team, GameManager_AllVisible);
                        GUI_PlayerTeamIndex = team;
                    }

                    UnitsManager_TeamInfo[team].field_41 = false;
                    GameManager_sub_9E750(team, game_state, enable_autosave);

                    if (GUI_GameState == GAME_STATE_10) {
                        game_state = GAME_STATE_10;
                        GUI_GameState = GAME_STATE_8_IN_GAME;
                        break;
                    }

                    if (GUI_GameState != GAME_STATE_9) {
                        break;
                    }

                    GameManager_sub_953C7();

                    /// \todo
                    // while (are_task_events_pending()) {
                    //    GameManager_ProgressTurn();
                    // }

                    GameManager_GuiSwitchTeam(GUI_PlayerTeamIndex);

                    game_state = GAME_STATE_8_IN_GAME;
                    enable_autosave = false;
                }
            }
        }

        if (GUI_GameState == GAME_STATE_9) {
            GUI_GameState = GAME_STATE_8_IN_GAME;
            /// \todo
            // if (game_loop_check_end_game_conditions(GameManager_TurnCounter, turn_counter,
            // GameManager_DemoMode)) {
            //     break;
            // }

            for (team = 0; team < PLAYER_TEAM_MAX - 1; ++team) {
                UnitsManager_TeamInfo[team].field_41 = false;
            }

            GameManager_UpdateScoreGraph();

            ++GameManager_TurnCounter;

            GameManager_AnnounceWinner(team_winner);
        }
    }

    GameManager_GameLoopCleanup();

    if (GUI_GameState == GAME_STATE_15_FATAL_ERROR) {
        ResourceManager_ExitGame(EXIT_CODE_THANKS);
    }

    soundmgr.PlayMusic(MAIN_MSC, false);
}

void GameManager_DrawUnitSelector(unsigned char* buffer, int width, int offsetx, int height, int offsety, int bottom,
                                  int item_height, int top, int scaling_factor, int is_big_sprite, bool double_marker) {
    unsigned char color;
    int loop_count;
    int scaled_size2;
    int size2;
    int scaled_size1;
    int size1;

    if (is_big_sprite) {
        size1 = 32;
    } else {
        size1 = 16;
    }

    scaled_size1 = (size1 << 16) / scaling_factor;

    if (is_big_sprite) {
        size2 = 128;
    } else {
        size2 = 64;
    }

    scaled_size2 = (size2 << 16) / scaling_factor - 2;

    if (double_marker) {
        loop_count = 3;
    } else {
        loop_count = 1;
    }

    if (double_marker) {
        color = GameManager_MarkerColor;
    } else {
        color = 0xFF;
    }

    for (int i = 0; i < loop_count; ++i) {
        if (height >= bottom && height <= top && scaled_size1 + offsetx >= offsety && offsetx <= item_height) {
            draw_line(buffer, width, std::max(offsetx, offsety), height, std::min(scaled_size1 + offsetx, item_height),
                      height, color);
        }

        if (offsetx >= offsety && offsetx <= item_height && scaled_size1 + height >= bottom && height <= top) {
            draw_line(buffer, width, offsetx, std::max(height, bottom), offsetx, std::min(scaled_size1 + height, top),
                      color);
        }

        offsetx += scaled_size2;

        if (height >= bottom && height <= top && offsetx >= offsety && offsetx - scaled_size1 <= item_height) {
            draw_line(buffer, width, std::max(offsetx - scaled_size1, offsety), height, std::min(offsetx, item_height),
                      height, color);
        }

        if (offsetx >= offsety && offsetx <= item_height && scaled_size1 + height >= bottom && height <= top) {
            draw_line(buffer, width, offsetx, std::max(height, bottom), offsetx, std::min(scaled_size1 + height, top),
                      color);
        }

        height += scaled_size2;

        if (height >= bottom && height <= top && offsetx >= offsety && offsetx - scaled_size1 <= item_height) {
            draw_line(buffer, width, std::max(offsetx - scaled_size1, offsety), height, std::min(offsetx, item_height),
                      height, color);
        }

        if (offsetx >= offsety && offsetx <= item_height && height >= bottom && height - scaled_size1 <= top) {
            draw_line(buffer, width, offsetx, std::min(height, top), offsetx, std::max(height - scaled_size1, bottom),
                      color);
        }

        offsetx -= scaled_size2;

        if (height >= bottom && height <= top && scaled_size1 + offsetx >= offsety && offsetx <= item_height) {
            draw_line(buffer, width, std::max(offsetx, offsety), height, std::min(scaled_size1 + offsetx, item_height),
                      height, color);
        }

        if (offsetx >= offsety && offsetx <= item_height && height >= bottom && height - scaled_size1 <= top) {
            draw_line(buffer, width, offsetx, std::min(height, top), offsetx, std::max(height - scaled_size1, bottom),
                      color);
        }

        height -= scaled_size2;
        scaled_size2 -= 2;
        ++offsetx;
        ++height;
        --scaled_size1;
        color ^= 0xFF;
    }
}

bool GameManager_CargoSelection(unsigned short team) {
    CargoMenu cargo_menu(team);
    char message[200];

    if (GameManager_HumanPlayerCount) {
        sprintf(message, "%s:\nBegin cargo selection.", menu_team_names[team]);

        MessageManager_DrawMessage(message, 0, 1, true);
    }

    return cargo_menu.Run();
}

bool GameManager_PlayerMissionSetup(unsigned short team) {
    int team_gold;

    GUI_GameState = GAME_STATE_7_SITE_SELECT;

    mouse_show();

    GameManager_UnknownFlag = 1;

    GUI_PlayerTeamIndex = team;
    GameManager_ActiveTurnTeam = team;

    ini_set_setting(INI_PLAYER_CLAN, UnitsManager_TeamInfo[team].team_clan);

    if (ResourceManager_MapTileIds) {
        GameManager_UpdateMainMapView(0, 4, 0, false);
        GameManager_ProcessTick(1);
    }

    do {
        GUI_GameState = GAME_STATE_7_SITE_SELECT;

        team_gold = ini_get_setting(INI_START_GOLD) + ini_clans.GetClanGold(UnitsManager_TeamInfo[team].team_clan);

        if (team_gold) {
            if (!GameManager_CargoSelection(team)) {
                return false;
            }
        } else {
            UnitsManager_AddDefaultMissionLoadout(team);
        }

        if (!ResourceManager_MapTileIds) {
            GameManager_InitMap();
        }

        if (GameManager_HumanPlayerCount && !team_gold) {
            GameManager_DrawSelectSiteMessage(team);
        }

        UnitsManager_TeamMissionSupplies[team].proximity_alert_ack = true;

        if (GameManager_SelectSite(team)) {
            MessageManager_MessageBox_IsActive = false;

            return true;
        }

    } while (GUI_GameState != GAME_STATE_3_MAIN_MENU && GUI_GameState != GAME_STATE_15_FATAL_ERROR && team_gold);

    return false;
}

void GameManager_DrawSelectSiteMessage(unsigned short team) {
    SmartString string;

    string = menu_team_names[team];
    string += ":\n";
    string += "Select starting location.";

    MessageManager_DrawMessage(string.GetCStr(), 0, 1, true);
}

bool GameManager_SelectSite(unsigned short team) {
    /// \todo
}

void GameManager_InitMap() {
    mouse_hide();
    ResourceManager_InitInGameAssets(ini_get_setting(INI_WORLD));
    GameManager_UpdateMainMapView(0, 4, 0, false);
    GameManager_ProcessTick(1);
    mouse_show();
    GameManager_UnknownFlag = 1;
}

void GameManager_UpdateMainMapView(int mode, int grid_x_zoom_level_max, int grid_y_zoom_level_min, bool flag) {
    /// \todo
}

void GameManager_GameSetup(int game_state) {
    int zoom_level;
    int max_zoom_level;
    unsigned int timestamp;

    GameManager_InitUnitsAndGameState();

    if (Remote_IsNetworkGame) {
        if (game_state == GAME_STATE_6) {
            Remote_SendNetPacket_Signal(REMOTE_PACKET_26, GUI_PlayerTeamIndex,
                                        UnitsManager_TeamInfo[GUI_PlayerTeamIndex].team_clan);
            ResourceManager_InitClanUnitValues(GUI_PlayerTeamIndex);
        }

        dos_srand(Remote_RngSeed);
    }

    /// \todo
    // Ai_Clear();
    // PathsManager_Clear_I();

    if (game_state == GAME_STATE_6) {
        GUI_GameState = GAME_STATE_7_SITE_SELECT;

        if (!GameManager_InitGame()) {
            return;
        }

        MouseEvent::Clear();

        if (GameManager_PlayScenarioIntro && !ini_get_setting(INI_GAME_FILE_TYPE)) {
            Color* palette;

            GameManager_MenuDeletePanelButtons(&GameManager_LandingSequence);
            palette = GameManager_MenuFadeOut(150);
            Movie_Play(DEMO1FLC);
            memcpy(WindowManager_ColorPalette, palette, 3 * PALETTE_SIZE);

            delete[] palette;

            WindowManager_LoadImage(FRAMEPIC, WindowManager_GetWindow(WINDOW_MAIN_WINDOW), 640, false);
            GameManager_ProcessTick(true);
            WindowManager_FadeIn(50);

            GameManager_PlayScenarioIntro = false;
        }

        GUI_GameState = GAME_STATE_11;
        GameManager_MenuAnimateOpenControlPanel(&GameManager_LandingSequence);

        zoom_level = 4;
        max_zoom_level = 64;

        if (ResourceManager_DisableEnhancedGraphics) {
            max_zoom_level = 32;
        }

        while (zoom_level <= max_zoom_level) {
            timestamp = timer_get_stamp32();

            GameManager_UpdateMainMapView(0, zoom_level, 0, false);
            GameManager_ProcessTick(false);

            while ((timer_get_stamp32() - timestamp) < TIMER_FPS_TO_TICKS(48)) {
            }

            zoom_level *= 2;
        }

    } else {
        UnitInfo* unit;

        GUI_GameState = GAME_STATE_10;

        if (!SaveLoadMenu_Load(ini_get_setting(INI_GAME_FILE_NUMBER), ini_get_setting(INI_GAME_FILE_TYPE),
                               !Remote_IsNetworkGame)) {
            GUI_GameState = GAME_STATE_3_MAIN_MENU;
            return;
        }

        GUI_GameState = GAME_STATE_11;

        GameManager_MenuAnimateOpenControlPanel(&GameManager_LandingSequence);
        GameManager_UpdateHumanPlayerCount();
        GameManager_UpdateMainMapView(0, 4, 0, false);

        /// \todo unit = get_first_relevant_team_unit(GUI_PlayerTeamIndex);

        GameManager_UpdateMainMapView(1, unit->grid_x, unit->grid_y);
        GameManager_ProcessTick(true);
    }

    GUI_GameState = GAME_STATE_8_IN_GAME;

    /// \todo toggle_all_visible();

    soundmgr.PlayMusic(static_cast<ResourceID>(ini_get_setting(INI_WORLD) / 6 + SNOW_MSC), true);

    GameManager_MenuAnimateDisplayControls();
    GameManager_MenuInitDisplayControls();
    GameManager_DisableMainMenu();
    GameManager_UpdateTurnTimer(false, GameManager_TurnTimerValue);
    GameManager_DrawMouseCoordinates(0, 0);

    mouse_hide();
    mouse_set_position(320, 240);
    mouse_show();

    GameManager_UnknownFlag = true;
}

void GameManager_GameLoopCleanup() {
    /// \todo
}

unsigned short GameManager_EvaluateWinner() {
    unsigned short highest_team_points_team;
    int highest_team_points;
    int result;

    if (ini_setting_victory_type == VICTORY_TYPE_SCORE) {
        for (unsigned short i = 0; i < PLAYER_TEAM_MAX - 1; ++i) {
            if (UnitsManager_TeamInfo[i].team_type != TEAM_TYPE_NONE &&
                UnitsManager_TeamInfo[i].team_type != TEAM_TYPE_ELIMINATED &&
                UnitsManager_TeamInfo[i].team_points >= highest_team_points) {
                highest_team_points = UnitsManager_TeamInfo[i].team_points;
                highest_team_points_team = i;
            }
        }

        result = highest_team_points_team;
    } else {
        result = 0;
    }

    return result;
}

void GameManager_DrawTurnCounter(int turn_count) {
    char text[10];

    sprintf(text, "%i", turn_count);

    GameManager_DrawDisplayPanel(2, text, 0xA2);
}

void GameManager_UpdateTurnTimer(bool mode, int turn_time) {
    /// \todo
}

void GameManager_DrawDisplayPanel(int control_id, char* text, int color, int ulx) {
    /// \todo
}

void GameManager_ProgressBuildState(unsigned short team) {
    /// \todo
}

void GameManager_sub_9E750(unsigned short team, int game_state, bool enable_autosave) {
    /// \todo
}

bool GameManager_AreTeamsFinishedTurn() {
    /// \todo
}

void GameManager_ProgressTurn() {
    /// \todo
}

void GameManager_sub_953C7() {
    /// \todo
}

void GameManager_AnnounceWinner(unsigned short team) {
    /// \todo
}

void GameManager_UpdateScoreGraph() {
    /// \todo
}

void GameManager_InitUnitsAndGameState() {
    GameManager_IsCheater = false;

    soundmgr.SetVolume(AUDIO_TYPE_MUSIC, ini_get_setting(INI_MUSIC_LEVEL) / 3);

    ResourceManager_FreeResources();
    WindowManager_LoadPalette(FRAMEPIC);

    UnitsManager_InitPopupMenus();

    for (int i = 0; i < UNIT_END; ++i) {
        UnitsManager_BaseUnits[i].Init(&UnitsManager_AbstractUnits[i]);
        MouseEvent::ProcessInput();
    }

    for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX; ++i) {
        ResourceManager_InitClanUnitValues(i);
    }

    for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        GameManager_MultiChatTargets[i];
    }

    for (int i = 0; i < sizeof(GameManager_MenuItems) / sizeof(struct MenuGuiItem); ++i) {
        GameManager_MenuItems[i].disabled = false;
    }

    if (Remote_IsNetworkGame) {
        dos_srand(time(nullptr));
    } else {
        Remote_RngSeed = time(nullptr);
        dos_srand(Remote_RngSeed);
    }

    GameManager_MapBrightness = 0xFF;
    Remote_UpdatePauseTimer = 0;

    GameManager_DisableMapRendering = false;

    GameManager_DisplayButtonLock = false;
    GameManager_DisplayButtonScan = false;
    GameManager_DisplayButtonRange = false;
    GameManager_DisplayButtonGrid = false;
    GameManager_DisplayButtonSurvey = false;
    GameManager_DisplayButtonStatus = false;
    GameManager_DisplayButtonColors = false;
    GameManager_DisplayButtonHits = false;
    GameManager_DisplayButtonAmmo = false;
    GameManager_DisplayButtonNames = false;
    GameManager_DisplayButtonMinimap2x = false;
    GameManager_DisplayButtonMinimapTnt = false;

    GameManager_HumanPlayerCount = 0;
    GameManager_UnknownFlag = true;
    GameManager_RequestMenuExit = false;

    GameManager_SiteSelectState = false;
    GameManager_WrapUpGame = false;
    GameManager_IsTurnTimerActive = true;
    GameManager_RenderState = 0;
    GameManager_RenderEnable = false;
    MessageManager_MessageBox_IsActive = false;
    GameManager_PlayFlic = false;
    GameManager_UnknownFlag1 = false;
    GameManager_MaxSurvey = false;
    GameManager_UnknownFlag3 = false;

    GameManager_AllVisible = ini_get_setting(INI_ALL_VISIBLE);
    GameManager_RealTime = ini_get_setting(INI_REAL_TIME);
    GameManager_PlayMode = ini_get_setting(INI_PLAY_MODE);
    GameManager_FastMovement = ini_get_setting(INI_FAST_MOVEMENT);

    GameManager_ArrowKeyFlags = 0;

    GameManager_TurnCounter = 1;

    GameManager_GridCenter.x = 0;
    GameManager_GridCenter.y = 0;

    GameManager_SelectedUnit = nullptr;

    GameManager_PopupButtons.popup_count = 0;

    GameManager_TurnTimerImage = nullptr;
    GameManager_TurnTimerValue = 0;

    GameManager_SpottedEnemyPosition.x = -1;
    GameManager_SpottedEnemyPosition.y = -1;

    GameManager_LockedUnits.Clear();

    SaveLoadMenu_SaveSlot = -1;
}

bool GameManager_InitGame() {
    GameManager_GameFileNumber = 0;

    if (Remote_IsNetworkGame) {
        dos_srand(time(nullptr));
    } else {
        ResourceManager_InitTeamInfo();
    }

    for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        UnitsManager_TeamInfo[i].team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + i));
        ResourceManager_InitHeatMaps(i);
    }

    ResourceManager_InitHeatMaps(PLAYER_TEAM_ALIEN);
    GameManager_UpdateHumanPlayerCount();

    GUI_PlayerTeamIndex = PLAYER_TEAM_ALIEN;

    for (int i = PLAYER_TEAM_MAX - 1; i >= PLAYER_TEAM_RED; --i) {
        if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_PLAYER && !GameManager_PlayerMissionSetup(i)) {
            return false;
        }
    }

    if (GameManager_HumanPlayerCount && !GameManager_HandleProximityOverlaps()) {
        return false;
    }

    for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_COMPUTER) {
            if (GUI_PlayerTeamIndex == PLAYER_TEAM_ALIEN) {
                GUI_PlayerTeamIndex = i;
            }

            if (!ResourceManager_MapTileIds) {
                GameManager_InitMap();
            }

            if (Ai_SetupStrategy(i) && Remote_IsNetworkGame) {
                Remote_SendNetPacket_12(i);
            }
        }
    }

    GameManager_PopulatelMapWithResources();
    GameManager_PopulatelMapWithAlienUnits(ini_get_setting(INI_ALIEN_SEPERATION),
                                           ini_get_setting(INI_ALIEN_UNIT_VALUE));

    for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        if (UnitsManager_TeamInfo[i].team_type != TEAM_TYPE_NONE) {
            if (!ResourceManager_MapTileIds) {
                GameManager_InitMap();
            }

            GameManager_ProcessTeamMissionSupplyUnits(i);
        }
    }

    GameManager_GridCenter = UnitsManager_TeamMissionSupplies[GUI_PlayerTeamIndex].starting_position;
    UnitsManager_TeamInfo[GUI_PlayerTeamIndex].camera_position = GameManager_GridCenter;

    return true;
}

void GameManager_MenuDeletePanelButtons(MenuLandingSequence* control) {
    /// \todo
}

Color* GameManager_MenuFadeOut(int fade_steps) {
    /// \todo
}

void GameManager_MenuAnimateOpenControlPanel(MenuLandingSequence* control) {
    /// \todo
}

void GameManager_UpdateHumanPlayerCount() {
    GameManager_HumanPlayerCount = 0;

    for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_PLAYER) {
            ++GameManager_HumanPlayerCount;
        }
    }

    if (GameManager_HumanPlayerCount > 1) {
        GameManager_PlayMode = PLAY_MODE_TURN_BASED;
    } else {
        GameManager_HumanPlayerCount = 0;
    }
}

void GameManager_MenuAnimateDisplayControls() {
    /// \todo
}

void GameManager_MenuInitDisplayControls() {
    /// \todo
}

void GameManager_DrawMouseCoordinates(int x, int y) {
    /// \todo
}

bool GameManager_HandleProximityOverlaps() {
    /// \todo
}

void GameManager_PopulatelMapWithResources() {
    /// \todo
}

void GameManager_PopulatelMapWithAlienUnits(int alien_seperation, int alien_unit_value) {
    /// \todo
}

void GameManager_ProcessTeamMissionSupplyUnits(unsigned short team) {
    /// \todo
}

void GameManager_ProcessTick(bool render_screen) {
    /// \todo
}

void GameManager_ProcessState(bool process_tick, bool clear_mouse_events) {
    /// \todo
}

void GameManager_GuiSwitchTeam(unsigned short team) {
    /// \todo
}

void GameManager_EnableMainMenu(UnitInfo* unit) {
    /// \todo
}

void GameManager_DisableMainMenu() {
    /// \todo
}
