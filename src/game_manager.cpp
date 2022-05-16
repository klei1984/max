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

#include "access.hpp"
#include "ai.hpp"
#include "cargomenu.hpp"
#include "cursor.hpp"
#include "flicsmgr.hpp"
#include "gfx.hpp"
#include "gui.hpp"
#include "inifile.hpp"
#include "menu.hpp"
#include "menulandingsequence.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "movie.hpp"
#include "paths.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "saveloadmenu.hpp"
#include "sound_manager.hpp"
#include "task_manager.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

#define MENU_GUI_ITEM_DEF(id, gfx, label, button, state, sfx) \
    { (id), (gfx), (label), (button), (state), (sfx) }

#define MENU_DISPLAY_CONTROL_DEF(wid, rid, u1, button, u2, u3, image) \
    { (wid), (rid), (u1), (button), (u2), (u3), (image) }

#define MENU_GUI_ITEM_CHAT_BUTTON 10
#define MENU_GUI_ITEM_ENDTURN_BUTTON 23

#define MENU_DISPLAY_CONTROL_CORNER_FLIC 4
#define MENU_DISPLAY_CONTROL_STAT_WINDOW 5

struct MenuGuiItem {
    unsigned char wid;
    ResourceID gfx;
    const char* label;
    Button* button;
    bool disabled;
    ResourceID sfx;
};

struct MenuDisplayControl {
    unsigned char window_id;
    ResourceID resource_id;
    void* unknown_1;
    Button* button;
    char unknown_2;
    ResourceID unknown_3;
    Image* image;
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

struct ResourceRange {
    short min;
    short max;

    ResourceRange() {}
    ResourceRange(const ResourceRange& other) : min(other.min), max(other.max) {}
    ResourceRange(int min, int max) : min(min), max(max) {}

    int GetValue() const {
        return (((((max - min + 1) * dos_rand()) >> 15) + min) + ((((max - min + 1) * dos_rand()) >> 15) + min) + 1) /
               2;
    }
};

struct ResourceAllocator {
    Point m_point;
    unsigned short material_type;
    ResourceRange normal;
    ResourceRange concentrate;
    short concentrate_seperation;
    short field_16;
    short concentrate_diffusion;
    IniParameter ini_param;

    ResourceAllocator(unsigned short material_type) : material_type(material_type) {
        switch (material_type) {
            case CARGO_MATERIALS: {
                m_point.x = 0;
                m_point.y = 0;
                ini_param = INI_RAW_NORMAL_LOW;
            } break;

            case CARGO_GOLD: {
                m_point.x = 1;
                m_point.y = 0;
                ini_param = INI_GOLD_NORMAL_LOW;
            } break;

            case CARGO_FUEL: {
                m_point.x = 1;
                m_point.y = 1;
                ini_param = INI_FUEL_NORMAL_LOW;
            } break;
        }

        normal = ResourceRange(GetParameter(INI_RAW_NORMAL_LOW), GetParameter(INI_RAW_NORMAL_HIGH));
        concentrate = ResourceRange(GetParameter(INI_RAW_CONCENTRATE_LOW), GetParameter(INI_RAW_CONCENTRATE_HIGH));
        concentrate_seperation = GetParameter(INI_RAW_CONCENTRATE_SEPERATION);
        field_16 = concentrate_seperation / 5;
        concentrate_diffusion = GetParameter(INI_RAW_CONCENTRATE_DIFFUSION);
    }

    int GetParameter(IniParameter param) const {
        return ini_config.GetNumericValue(static_cast<IniParameter>(ini_param - INI_RAW_NORMAL_LOW + param));
    }

    static int GetZoneResoureLevel(int grid_x, int grid_y) {
        int result;
        Rect bounds;

        result = 0;

        rect_init(&bounds, grid_x, grid_y, grid_x + 2, grid_y + 2);

        if (bounds.ulx < 0) {
            bounds.ulx = 0;
        }

        if (bounds.uly < 0) {
            bounds.uly = 0;
        }

        if (ResourceManager_MapSize.x < bounds.lrx) {
            bounds.lrx = ResourceManager_MapSize.x;
        }

        if (ResourceManager_MapSize.y < bounds.lry) {
            bounds.lry = ResourceManager_MapSize.y;
        }

        for (int i = bounds.ulx; i < bounds.lrx; ++i) {
            for (int j = bounds.uly; j < bounds.lry; ++j) {
                result += ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i] & 0x1F;
            }
        }

        return result;
    }

    static int OptimizeResources(int grid_x, int grid_y, int level_min, int level_max) {
        int zone_resource_level;

        zone_resource_level = GetZoneResoureLevel(grid_x, grid_y);

        if ((zone_resource_level + level_min) > level_max) {
            level_min = level_max - zone_resource_level;
        }

        if (level_min < 0) {
            level_min = 0;
        }

        return level_min;
    }

    void Optimize(Point point, int resource_level, int resource_value) {
        Rect bounds;
        int map_resource_amount;
        int material_value;
        int distance_factor;
        int grid_x;
        int grid_y;

        point.x = ((((point.x + 1) - m_point.x) / 2) * 2) + m_point.x;
        point.y = ((((point.y + 1) - m_point.y) / 2) * 2) + m_point.y;

        bounds.lry = point.y - 4;

        if (bounds.lry < 0) {
            bounds.lry = 0;
        }

        bounds.lry = ((bounds.lry / 2) * 2) + m_point.y;

        bounds.lrx = point.x - 4;

        if (bounds.lrx < 0) {
            bounds.lrx = 0;
        }

        bounds.lrx = ((bounds.lrx / 2) * 2) + m_point.x;

        bounds.uly = point.y + 5;

        if (bounds.uly > ResourceManager_MapSize.y) {
            bounds.uly = ResourceManager_MapSize.y;
        }

        bounds.ulx = point.x + 5;

        if (bounds.ulx > ResourceManager_MapSize.x) {
            bounds.ulx = ResourceManager_MapSize.x;
        }

        map_resource_amount = ResourceManager_CargoMap[point.y * ResourceManager_MapSize.x + point.x] & 0x1F;

        if (point.x > 0) {
            if (point.y > 0) {
                resource_level = ResourceAllocator::OptimizeResources(point.x - 1, point.y - 1, resource_level,
                                                                      resource_value + map_resource_amount);
            }

            if (grid_y < ResourceManager_MapSize.y - 1) {
                resource_level = ResourceAllocator::OptimizeResources(point.x - 1, point.y, resource_level,
                                                                      resource_value + map_resource_amount);
            }
        }

        if (point.x < ResourceManager_MapSize.x - 1) {
            if (point.y > 0) {
                resource_level = ResourceAllocator::OptimizeResources(point.x, point.y - 1, resource_level,
                                                                      resource_value + map_resource_amount);
            }

            if (point.y < ResourceManager_MapSize.y - 1) {
                resource_level = ResourceAllocator::OptimizeResources(point.x, point.y, resource_level,
                                                                      resource_value + map_resource_amount);
            }
        }

        resource_level *= 100;

        for (grid_x = bounds.lrx; grid_x < bounds.ulx; grid_x += 2) {
            for (grid_y = bounds.lry; grid_y < bounds.uly; grid_y += 2) {
                distance_factor =
                    ((Taskmanager_sub_45F65(point.x - grid_x, point.y - grid_y) * 10) / concentrate_diffusion) + 10;
                material_value = resource_level / (distance_factor * distance_factor);

                if (material_value > (ResourceManager_CargoMap[grid_y * ResourceManager_MapSize.x + grid_x] & 0x1F)) {
                    ResourceManager_CargoMap[grid_y * ResourceManager_MapSize.x + grid_x] =
                        material_type + material_value;
                }
            }
        }
    }

    void PopulateCargoMap() const {
        for (int i = m_point.x; i < ResourceManager_MapSize.x; i += 2) {
            for (int j = m_point.y; j < ResourceManager_MapSize.y; j += 2) {
                ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i] = material_type + normal.GetValue();
            }
        }
    }

    void ConcentrateResources() {
        int max_resources;
        Point point1;
        Point point2;
        bool flag;

        max_resources = ini_get_setting(INI_MAX_RESOURCES);

        point1.x = (((concentrate_seperation * 4) / 5 + 1) * dos_rand()) >> 15;
        point1.y = ((concentrate_seperation / 2 + 1) * dos_rand()) >> 15;

        for (int i = point1.x; i < ResourceManager_MapSize.x; i += (concentrate_seperation * 4) / 5) {
            for (int j = flag ? (concentrate_seperation / 2 + point1.y) : point1.y; j < ResourceManager_MapSize.y;
                 j += concentrate_seperation) {
                point2.x = (((field_16 * 2 + 1) * dos_rand()) >> 15) - field_16 + i;
                point2.y = (((field_16 * 2 + 1) * dos_rand()) >> 15) - field_16 + j;

                Optimize(point2, concentrate.GetValue(), max_resources);
            }

            flag = !flag;
        }
    }

    static void SettleMinimumResourceLevels(int min_level) {
        for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
            for (int j = 0; j < ResourceManager_MapSize.y; ++j) {
                if (GetZoneResoureLevel(i - 1, j - 1) < min_level && GetZoneResoureLevel(i - 1, j) < min_level &&
                    GetZoneResoureLevel(i, j - 1) < min_level && GetZoneResoureLevel(i, j) < min_level) {
                    ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i] = CARGO_MATERIALS;
                }
            }
        }
    }

    void SeparateResources(ResourceAllocator* allocator) {
        Point point1;
        Point point2;
        int max_resources;
        int mixed_resource_seperation;
        int mixed_resource_seperation_min;
        bool flag;

        max_resources = ini_get_setting(INI_MAX_RESOURCES);
        mixed_resource_seperation = ini_get_setting(INI_MIXED_RESOURCE_SEPERATION);
        mixed_resource_seperation_min = mixed_resource_seperation / 5;
        flag = false;

        point1.x = ((((mixed_resource_seperation * 4) / 5) + 1) * dos_rand()) >> 15;
        point1.y = ((mixed_resource_seperation / 2 + 1) * dos_rand()) >> 15;

        for (int i = point1.x; i < ResourceManager_MapSize.x; i += (mixed_resource_seperation * 4) / 5) {
            for (int j = flag ? (mixed_resource_seperation / 2 + point1.y) : point1.y; j < ResourceManager_MapSize.y;
                 j += mixed_resource_seperation) {
                point2.x =
                    (((mixed_resource_seperation_min * 2 + 1) * dos_rand()) >> 15) - mixed_resource_seperation_min + i;
                point2.y =
                    (((mixed_resource_seperation_min * 2 + 1) * dos_rand()) >> 15) - mixed_resource_seperation_min + j;

                Optimize(point2, ((5 * dos_rand()) >> 15) + 8, max_resources);
                Optimize(point2, ((5 * dos_rand()) >> 15) + 8, max_resources);
            }

            flag = !flag;
        }
    }
};

Rect GameManager_GridPosition;
Rect GameManager_MapWindowDrawBounds;
SmartPointer<UnitInfo> GameManager_SelectedUnit;
Image* GameManager_TurnTimerImage;
Point GameManager_GridCenter;
Point GameManager_SpottedEnemyPosition;
SmartList<UnitInfo> GameManager_LockedUnits;
unsigned int GameManager_TurnCounter;
unsigned int GameManager_ArrowKeyFlags;
unsigned int GameManager_TurnTimerValue;
bool GameManager_MaxSpy;
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
bool GameManager_MainMenuFreezeState;
bool GameManager_DisplayControlsInitialized;

Button* Gamemanager_FlicButton;

unsigned short GameManager_ActiveTurnTeam;
unsigned char GameManager_MarkerColor = 0xFF;
unsigned char GameManager_PlayMode;
unsigned char GameManager_FastMovement;
unsigned short GameManager_MultiChatTargets[PLAYER_TEAM_MAX - 1];

MenuLandingSequence GameManager_LandingSequence;

static struct MenuGuiItem GameManager_MenuItems[] = {
    MENU_GUI_ITEM_DEF(WINDOW_FILES_BUTTON, FILES_OF, "Files", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_PREFS_BUTTON, PREF_OFF, "Preferences", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_PLAY_BUTTON, PLAY_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_PAUSE_BUTTON, PAUSE_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_CENTER_BUTTON, FIND_OFF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_PRE_BUTTON, PREV_OF, "Prev", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_DONE_BUTTON, UDONE_OF, "Done", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_NXT_BUTTON, NEXT_OF, "Next", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_HELP_BUTTON, HELP_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_REPORTS_BUTTON, REPT_OFF, "Reports", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_CHAT_BUTTON, CHAT_OFF, "Chat", nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_SURVEY_BUTTON, SURV_OFF, "Survey", nullptr, false, ISURV0),
    MENU_GUI_ITEM_DEF(WINDOW_STATUS_BUTTON, STAT_OFF, "Status", nullptr, false, ISTAT0),
    MENU_GUI_ITEM_DEF(WINDOW_COLORS_BUTTON, COLOR_OF, "Colors", nullptr, false, ICOLO0),
    MENU_GUI_ITEM_DEF(WINDOW_HITS_BUTTON, HITS_OF, "Hits", nullptr, false, IHITS0),
    MENU_GUI_ITEM_DEF(WINDOW_AMMO_BUTTON, AMMO_OF, "Ammo", nullptr, false, IAMMO0),
    MENU_GUI_ITEM_DEF(WINDOW_RANGE_BUTTON, RANG_OFF, "Range", nullptr, false, IRANG0),
    MENU_GUI_ITEM_DEF(WINDOW_SCAN_BUTTON, VISN_OFF, "Scan", nullptr, false, IVISI0),
    MENU_GUI_ITEM_DEF(WINDOW_GRID_BUTTON, GRID_OFF, "Grid", nullptr, false, IGRID0),
    MENU_GUI_ITEM_DEF(WINDOW_NAME_BUTTON, NAMES_UP, "Names", nullptr, false, INAME0),
    MENU_GUI_ITEM_DEF(WINDOW_LOCK_BUTTON, LOCK_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_2X_MINIMAP, MIN2X_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_TNT_MINIMAP, MINFL_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_ENDTURN_BUTTON, ENDTRN_U, "End Turn", nullptr, false, MENUOP),
};

static Point GameManager_MenuItemLabelOffsets[] = {
    {0, 2}, {0, 2}, {0, 0}, {0, 0}, {0, 0}, {-10, 2}, {0, 3}, {1, 2}, {0, 0}, {0, 3}, {0, 3}, {2, 2},
    {2, 2}, {2, 4}, {2, 2}, {2, 2}, {2, 4}, {2, 2},   {2, 2}, {2, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
};

static struct MenuDisplayControl GameManager_MenuDisplayControls[] = {
    MENU_DISPLAY_CONTROL_DEF(WINDOW_COORDINATES_DISPLAY, XYPOS, 0, 0, 0, INVALID_ID, 0),
    MENU_DISPLAY_CONTROL_DEF(WINDOW_UNIT_DESCRIPTION_DISPLAY, UNITNAME, 0, 0, 0, INVALID_ID, 0),
    MENU_DISPLAY_CONTROL_DEF(WINDOW_TURN_COUNTER_DISPLAY, TURNS, 0, 0, 0, INVALID_ID, 0),
    MENU_DISPLAY_CONTROL_DEF(WINDOW_TURN_TIMER_DISPLAY, TIMER, 0, 0, 0, INVALID_ID, 0),
    MENU_DISPLAY_CONTROL_DEF(WINDOW_CORNER_FLIC, INVALID_ID, 0, 0, 0, INVALID_ID, 0),
    MENU_DISPLAY_CONTROL_DEF(WINDOW_STAT_WINDOW, INVALID_ID, 0, 0, 0, INVALID_ID, 0),
};

struct PopupButtons GameManager_PopupButtons;

static struct Flic* GameManager_Flic;
static TextEdit* GameManager_TextEditUnitName;
static char GameManager_UnitName[30];

static bool GameManager_PlayerMissionSetup(unsigned short team);
static void GameManager_DrawSelectSiteMessage(unsigned short team);
static bool GameManager_SelectSite(unsigned short team);
static void GameManager_InitMap();
static void GameManager_GameSetup(int game_state);
static void GameManager_GameLoopCleanup();
static unsigned short GameManager_EvaluateWinner();
static void GameManager_DrawTurnCounter(int turn_count);
static void GameManager_DrawTimer(char* text, int color);
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
static Color* GameManager_MenuFadeOut(int fade_steps = 50);
static void GameManager_UpdateHumanPlayerCount();
static void GameManager_MenuAnimateDisplayControls();
static void GameManager_MenuInitDisplayControls();
static void GameManager_DrawMouseCoordinates(int x, int y);
static bool GameManager_HandleProximityOverlaps();
static bool GameManager_IsValidStartingPosition(int grid_x, int grid_y);
static void GameManager_IsValidStartingRange(short* grid_x, short* grid_y);
static void GameManager_PopulateMapWithResources();
static void GameManager_SpawnAlienDerelicts(Point point, int alien_unit_value);
static void GameManager_PopulateMapWithAlienUnits(int alien_seperation, int alien_unit_value);
static void GameManager_ProcessTeamMissionSupplyUnits(unsigned short team);
static void GameManager_MenuEndTurnButtonSetState(bool state);
static void GameManager_FlicButtonRFunction(ButtonID bid, int value);
static void GameManager_MenuDeinitDisplayControls();
static void GameManager_DrawProximityZones();
static int GameManager_UpdateProximityState(unsigned short team);

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
            GameManager_EnableMainMenu(nullptr);

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

void GameManager_UpdateDrawBounds() {
    /// \todo
}

void GameManager_AddDrawBounds(Rect* bounds) {
    /// \todo
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

void GameManager_GetScaledMessageBoxBounds(Rect* bounds) {
    WindowInfo* window;

    window = WindowManager_GetWindow(WINDOW_MESSAGE_BOX);

    bounds->ulx = GameManager_MapWindowDrawBounds.ulx;
    bounds->uly = ((Gfx_MapScalingFactor * 10) >> 16) + GameManager_MapWindowDrawBounds.uly;
    bounds->lrx = (((window->window.lrx - window->window.ulx + 1) * Gfx_MapScalingFactor) >> 16) + bounds->ulx;
    bounds->lry = (((window->window.lry - window->window.uly + 1) * Gfx_MapScalingFactor) >> 16) + bounds->uly;
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
    int proximity_alert_ack;
    int range;
    int scan;
    SmartPointer<UnitValues> unit_values;
    bool flag;
    int starting_position_x;
    int starting_position_y;
    int grid_x_overlap;
    int grid_y_overlap;
    int proximity_state;

    proximity_alert_ack = UnitsManager_TeamMissionSupplies[team].proximity_alert_ack;

    if (proximity_alert_ack == 1) {
        flag = true;

        MessageManager_DrawMessage("Select starting location.", 0, 0);
        soundmgr.PlayVoice(V_M278, V_F177);

    } else {
        flag = false;
    }

    while (proximity_alert_ack != 0 && proximity_alert_ack != 6) {
        MouseEvent::Clear();
        GUI_GameState = GAME_STATE_7_SITE_SELECT;

        if (flag) {
            while (GUI_GameState == GAME_STATE_7_SITE_SELECT) {
                GameManager_ProgressTurn();
            }
        }

        if (GUI_GameState == GAME_STATE_14 || GUI_GameState == GAME_STATE_3_MAIN_MENU ||
            GUI_GameState == GAME_STATE_15_FATAL_ERROR) {
            proximity_alert_ack = 6;
            break;
        }

        GUI_GameState == GAME_STATE_7_SITE_SELECT;

        Cursor_SetCursor(CURSOR_HAND);

        starting_position_x = UnitsManager_TeamMissionSupplies[team].starting_position.x;
        starting_position_y = UnitsManager_TeamMissionSupplies[team].starting_position.y;

        if (GameManager_SelectedUnit == nullptr) {
            GameManager_SelectedUnit =
                UnitsManager_SpawnUnit(MASTER, team, starting_position_x, starting_position_y, nullptr);
            GameManager_SelectedUnit->orders = ORDER_IDLE;

            unit_values = GameManager_SelectedUnit->GetBaseValues();
            range = unit_values->GetAttribute(ATTRIB_RANGE);
            scan = unit_values->GetAttribute(ATTRIB_SCAN);

            GameManager_DisplayButtonScan = true;
            GameManager_DisplayButtonRange = true;
        }

        if (flag && proximity_alert_ack == 2) {
            if (labs(starting_position_x - grid_x_overlap) <= 3 && labs(starting_position_y - grid_y_overlap) <= 3) {
                UnitsManager_TeamMissionSupplies[team].starting_position.x = grid_x_overlap;
                UnitsManager_TeamMissionSupplies[team].starting_position.y = grid_y_overlap;

                proximity_alert_ack = 4;

            } else {
                proximity_alert_ack = 1;
            }
        }

        if (proximity_alert_ack == 1 || proximity_alert_ack == 2 || proximity_alert_ack == 3) {
            UnitsManager_MoveUnit(&*GameManager_SelectedUnit, starting_position_x, starting_position_y);
            GameManager_DrawProximityZones();
        }

        UnitsManager_TeamMissionSupplies[team].proximity_alert_ack = proximity_alert_ack;

        if (Remote_IsNetworkGame) {
            Remote_SendNetPacket_12(team);
            proximity_state = Remote_SiteSelectMenu();

        } else if (flag) {
            proximity_state = GameManager_UpdateProximityState(team);
        } else {
            proximity_state = proximity_alert_ack;
        }

        switch (proximity_state) {
            case 0:
            case 5:
            case 6: {
                flag = false;
                proximity_alert_ack = proximity_state;
            } break;

            case 2: {
                if (proximity_alert_ack == 4) {
                    flag = false;

                    if (!Remote_IsNetworkGame) {
                        proximity_alert_ack = 0;
                    }

                } else {
                    MessageManager_DrawMessage(
                        "Notice: Proximity zones overlap! Select again or\nclick inside red circle to remain at "
                        "current location.",
                        0, 0);

                    grid_x_overlap = starting_position_x;
                    grid_y_overlap = starting_position_y;

                    proximity_alert_ack = 2;
                    flag = true;
                }

            } break;

            case 3: {
                MessageManager_DrawMessage("Warning: Exclusion zones overlap! Select again.", 2, 0);

                proximity_alert_ack = 1;
                flag = true;
            } break;
        }
    }

    if (GameManager_SelectedUnit != nullptr) {
        unit_values->SetAttribute(ATTRIB_RANGE, range);
        unit_values->SetAttribute(ATTRIB_SCAN, scan);

        GameManager_DisplayButtonScan = false;
        GameManager_DisplayButtonRange = false;

        /// \todo UnitsManager_sub_FF2CC(&*GameManager_SelectedUnit);
    }

    return proximity_alert_ack != 0;
}

int GameManager_UpdateProximityState(unsigned short team) {
    int state;

    state = 0;

    for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        if (i != team && UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_PLAYER) {
            switch (GameManager_CheckLandingZones(team, i)) {
                case 2: {
                    if (state == 0) {
                        state = 2;
                    }

                    if (UnitsManager_TeamMissionSupplies[i].proximity_alert_ack == 0 ||
                        UnitsManager_TeamMissionSupplies[i].proximity_alert_ack == 1) {
                        UnitsManager_TeamMissionSupplies[i].proximity_alert_ack = 2;
                    }

                } break;

                case 3: {
                    state = 3;
                    UnitsManager_TeamMissionSupplies[i].proximity_alert_ack = 3;

                } break;
            }
        }
    }

    return state;
}

int GameManager_CheckLandingZones(unsigned short team1, unsigned short team2) {
    int status;
    int proximity_range;
    int exclude_range;
    int distance_x;
    int distance_y;

    if (UnitsManager_TeamMissionSupplies[team2].units.GetCount()) {
        proximity_range = ini_get_setting(INI_PROXIMITY_RANGE);
        exclude_range = ini_get_setting(INI_EXCLUDE_RANGE);

        distance_x = labs(UnitsManager_TeamMissionSupplies[team1].starting_position.x -
                          UnitsManager_TeamMissionSupplies[team2].starting_position.x);

        distance_y = labs(UnitsManager_TeamMissionSupplies[team1].starting_position.y -
                          UnitsManager_TeamMissionSupplies[team2].starting_position.y);

        if (distance_x <= exclude_range && distance_y <= exclude_range) {
            status = 3;

        } else if (distance_x > proximity_range || distance_y > proximity_range ||
                   (UnitsManager_TeamMissionSupplies[team1].proximity_alert_ack == 4 &&
                    UnitsManager_TeamMissionSupplies[team2].proximity_alert_ack == 4)) {
            status = 0;

        } else {
            status = 2;
        }

    } else {
        status = 0;
    }

    return status;
}

void GameManager_InitMap() {
    mouse_hide();
    ResourceManager_InitInGameAssets(ini_get_setting(INI_WORLD));
    GameManager_UpdateMainMapView(0, 4, 0, false);
    GameManager_ProcessTick(1);
    mouse_show();
    GameManager_UnknownFlag = 1;
}

void GameManager_UpdateInfoDisplay(UnitInfo* unit) {
    /// \todo
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

            GameManager_LandingSequence.DeleteButtons();
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
        GameManager_LandingSequence.OpenPanel();

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

        GameManager_LandingSequence.OpenPanel();
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

void GameManager_DrawTimer(char* text, int color) {
    Rect bounds;
    WindowInfo window;

    bounds.ulx = 524;
    bounds.uly = 10;
    bounds.lrx = bounds.ulx + 62;
    bounds.lry = bounds.uly + 24;

    window.id = win_get_top_win(bounds.ulx, bounds.uly);
    window.buffer = win_get_buf(window.id);
    window.window.ulx = 0;
    window.window.uly = 0;
    window.window.lrx = 640;
    window.window.lry = 480;
    window.width = 640;

    if (!GameManager_TurnTimerImage) {
        GameManager_TurnTimerImage =
            new (std::nothrow) Image(bounds.ulx, bounds.uly, bounds.lrx - bounds.ulx, bounds.lry - bounds.uly);
        GameManager_TurnTimerImage->Copy(&window);
    }

    GameManager_TurnTimerImage->Write(&window);

    text_font(5);

    Text_TextBox(window.buffer, window.width, text, bounds.ulx, bounds.uly, bounds.lrx - bounds.ulx,
                 bounds.lry - bounds.uly, color, true);

    win_draw_rect(window.id, &bounds);
}

void GameManager_DrawTurnTimer(int turn_time, bool mode) {
    int color;
    char text[10];

    if (turn_time <= 20) {
        if (GameManager_IsTurnTimerActive && GUI_PlayerTeamIndex == GameManager_ActiveTurnTeam) {
            if (turn_time == 20 && ini_get_setting(INI_TIMER) > 20) {
                soundmgr.PlayVoice(V_M272, V_F273);
            } else if (!UnitsManager_TeamInfo[GUI_PlayerTeamIndex].field_41 && ini_get_setting(INI_TIMER) > 20 &&
                       Remote_UpdatePauseTimer) {
                soundmgr.PlayVoice(V_M275, V_F275);
            }

            GameManager_IsTurnTimerActive = false;
        }

        color = 0x1;
    }

    sprintf(text, "%2.2i:%2.2i", turn_time / 60, turn_time % 60);

    if (mode) {
        GameManager_DrawTimer(text, color);
    } else {
        GameManager_DrawDisplayPanel(3, text, color);
    }
}

void GameManager_UpdateTurnTimer(bool mode, int turn_time) {
    GameManager_TurnTimerValue = turn_time;
    if (GameManager_TurnTimerValue) {
        Remote_UpdatePauseTimer = mode;

        if (Remote_UpdatePauseTimer) {
            Remote_PauseTimeStamp = timer_get_stamp32();
        }
    } else {
        Remote_UpdatePauseTimer = false;
    }

    GameManager_DrawTurnTimer(GameManager_TurnTimerValue);
    GameManager_RequestMenuExit = false;
}

void GameManager_DrawDisplayPanel(int control_id, char* text, int color, int ulx) {
    struct ImageSimpleHeader* image;

    if (GameManager_DisplayControlsInitialized) {
        image = reinterpret_cast<struct ImageSimpleHeader*>(
            ResourceManager_LoadResource(GameManager_MenuDisplayControls[control_id].resource_id));

        buf_to_buf(image->data, image->width, image->height, image->width,
                   GameManager_MenuDisplayControls[control_id].image->GetData(), image->width);

        text_font(5);

        Text_TextBox(GameManager_MenuDisplayControls[control_id].image->GetData(), image->width, text, ulx, 0,
                     image->width - ulx, image->height, color, true);

        GameManager_MenuDisplayControls[control_id].button->CopyUp(
            GameManager_MenuDisplayControls[control_id].image->GetData());

        GameManager_MenuDisplayControls[control_id].button->Disable();
        GameManager_MenuDisplayControls[control_id].button->Enable();
    }
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

    Gfx_MapBrightness = 0xFF;
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

    GameManager_PopulateMapWithResources();
    GameManager_PopulateMapWithAlienUnits(ini_get_setting(INI_ALIEN_SEPERATION), ini_get_setting(INI_ALIEN_UNIT_VALUE));

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

Color* GameManager_MenuFadeOut(int fade_steps) {
    Color* palette;

    palette = new (std::nothrow) Color[3 * PALETTE_SIZE];
    memcpy(palette, WindowManager_ColorPalette, sizeof(WindowManager_ColorPalette));
    GameManager_MenuDeinitDisplayControls();
    WindowManager_FadeOut(fade_steps);
    GameManager_FillOrRestoreWindow(WINDOW_MAIN_WINDOW, 0x00, true);

    return palette;
}

void GameManager_InitLandingSequenceMenu(bool enable_controls) { GameManager_LandingSequence.Init(enable_controls); }

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
    WindowInfo* top_window;
    WindowInfo* bottom_window;
    unsigned int time_stamp;

    top_window = WindowManager_GetWindow(WINDOW_TOP_INSTRUMENTS_WINDOW);
    bottom_window = WindowManager_GetWindow(WINDOW_BOTTOM_INSTRUMENTS_WINDOW);

    for (int i = 0; i < 5; ++i) {
        time_stamp = timer_get_stamp32();
        WindowManager_LoadImage2(static_cast<ResourceID>(PNLSEQ_1 + i), top_window->window.ulx, top_window->window.uly,
                                 true);
        win_draw_rect(top_window->id, &top_window->window);

        if (static_cast<ResourceID>(BPNLSQ_1 + i) <= BPNLSQ_4) {
            WindowManager_LoadImage2(static_cast<ResourceID>(BPNLSQ_1 + i), bottom_window->window.ulx,
                                     bottom_window->window.uly, true);
            win_draw_rect(bottom_window->id, &bottom_window->window);
        }

        process_bk();

        while ((timer_get_stamp32() - time_stamp) < TIMER_FPS_TO_TICKS(24)) {
        }
    }
}

void GameManager_FlicButtonRFunction(ButtonID bid, int value) {
    if (!GameManager_TextEditUnitName && GameManager_SelectedUnit != nullptr &&
        GameManager_SelectedUnit->team == GUI_PlayerTeamIndex &&
        UnitsManager_TeamInfo[GUI_PlayerTeamIndex].team_type == TEAM_TYPE_PLAYER) {
        WindowInfo* window;
        Rect bounds;
        char text[40];

        window = WindowManager_GetWindow(WINDOW_CORNER_FLIC);
        GameManager_SelectedUnit->GetName(GameManager_UnitName);
        GameManager_SelectedUnit->GetDisplayName(text);

        text_font(2);

        bounds.ulx = text_width(text) - text_width(GameManager_UnitName);
        bounds.uly = 0;
        bounds.lrx = window->window.lrx - window->window.ulx;
        bounds.lry = text_height();

        GameManager_MenuDisplayControls[MENU_DISPLAY_CONTROL_CORNER_FLIC].image->Write(window, &bounds);
        GameManager_TextEditUnitName = new (std::nothrow)
            TextEdit(window, GameManager_UnitName, 30, bounds.ulx, 0, bounds.lrx - bounds.ulx, text_height(), 2, 2);

        GameManager_TextEditUnitName->LoadBgImage();

        text_to_buf(&window->buffer[bounds.ulx], GameManager_UnitName, bounds.lrx - bounds.ulx, window->width, 2);

        GameManager_TextEditUnitName->EnterTextEditField();

        bounds.ulx += window->window.ulx;
        bounds.uly += window->window.uly;
        bounds.lrx += window->window.ulx;
        bounds.lry += window->window.uly;

        win_draw_rect(window->id, &bounds);
    }
}

void GameManager_MenuInitDisplayControls() {
    WindowInfo* window;
    int width;
    int height;

    window = WindowManager_GetWindow(WINDOW_CORNER_FLIC);

    Gamemanager_FlicButton =
        new (std::nothrow) Button(window->window.ulx, window->window.uly, window->window.lrx - window->window.ulx, 14);
    Gamemanager_FlicButton->SetRFunc(GameManager_FlicButtonRFunction,
                                     reinterpret_cast<intptr_t>(Gamemanager_FlicButton));
    Gamemanager_FlicButton->RegisterButton(window->id);
    GameManager_MenuInitButtons(true);

    for (int i = 0; i < sizeof(GameManager_MenuDisplayControls) / sizeof(struct MenuDisplayControl); ++i) {
        window = WindowManager_GetWindow(GameManager_MenuDisplayControls[i].window_id);
        width = window->window.lrx - window->window.ulx + 1;
        height = window->window.lry - window->window.uly + 1;
        GameManager_MenuDisplayControls[i].image = new (std::nothrow) Image(0, 0, width, height);
        GameManager_MenuDisplayControls[i].image->Copy(window);

        if (GameManager_MenuDisplayControls[i].resource_id == INVALID_ID) {
            GameManager_MenuDisplayControls[i].button = nullptr;
        } else {
            GameManager_MenuDisplayControls[i].button =
                new (std::nothrow) Button(XYPOS, XYPOS, window->window.ulx, window->window.uly);
            GameManager_MenuDisplayControls[i].button->SetFlags(0x20);
            GameManager_MenuDisplayControls[i].button->RegisterButton(window->id);
            GameManager_MenuDisplayControls[i].button->Enable();
        }
    }

    GameManager_DisplayControlsInitialized = true;
}

void GameManager_DrawMouseCoordinates(int x, int y) {
    char text[10];

    sprintf(text, "%3.3i-%3.3i", x, y);
    GameManager_DrawDisplayPanel(0, text, 0xA2, 21);
}

bool GameManager_HandleProximityOverlaps() {
    bool flag;

    do {
        flag = false;

        for (int team = PLAYER_TEAM_MAX - 1; team >= PLAYER_TEAM_RED; --team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER &&
                (UnitsManager_TeamMissionSupplies[team].proximity_alert_ack == 2 ||
                 UnitsManager_TeamMissionSupplies[team].proximity_alert_ack == 3)) {
                flag = true;

                GUI_PlayerTeamIndex = team;
                GameManager_ActiveTurnTeam = team;

                GameManager_UpdateMainMapView(0, 4, 0, false);
                GameManager_ProcessTick(true);
                GameManager_DrawSelectSiteMessage(team);

                if (!GameManager_SelectSite(team)) {
                    return false;
                }
            }
        }
    } while (flag);

    return true;
}

bool GameManager_IsValidStartingPosition(int grid_x, int grid_y) {
    Point point;

    if (grid_x < 1 || grid_y < 1 || (grid_x + 3) > ResourceManager_MapSize.x ||
        (grid_y + 3) > ResourceManager_MapSize.y) {
        return false;
    }

    for (point.x = grid_x - 1; point.x < grid_x + 3; ++point.x) {
        for (point.y = grid_y - 1; point.y < grid_y + 3; ++point.y) {
            if (Access_GetModifiedSurfaceType(point.x, point.y) != SURFACE_TYPE_LAND) {
                return false;
            }
        }
    }

    return true;
}

void GameManager_IsValidStartingRange(short* grid_x, short* grid_y) {
    if (!GameManager_IsValidStartingPosition(*grid_x, *grid_y)) {
        for (int i = 2;; i += 2) {
            --*grid_x;
            ++*grid_y;

            for (int j = 0; j < 8; j += 2) {
                for (int k = 0; k < i; ++k) {
                    *grid_x += Paths_8DirPointsArray[j].x;
                    *grid_y += Paths_8DirPointsArray[j].y;

                    if (GameManager_IsValidStartingPosition(*grid_x, *grid_y)) {
                        return;
                    }
                }
            }
        }
    }
}

void GameManager_PopulateMapWithResources() {
    ResourceAllocator allocator_materials(CARGO_MATERIALS);
    ResourceAllocator allocator_fuel(CARGO_FUEL);
    ResourceAllocator allocator_gold(CARGO_GOLD);
    int max_resources;

    max_resources = ini_get_setting(INI_MAX_RESOURCES);

    dos_srand(Remote_RngSeed);

    allocator_materials.PopulateCargoMap();
    allocator_fuel.PopulateCargoMap();
    allocator_gold.PopulateCargoMap();

    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            Point starting_position(UnitsManager_TeamMissionSupplies[team].starting_position.x,
                                    UnitsManager_TeamMissionSupplies[team].starting_position.y);
            GameManager_IsValidStartingRange(&starting_position.x, &starting_position.y);
            allocator_materials.Optimize(starting_position, (max_resources * 12 + 10) / 20, 100);
            allocator_fuel.Optimize(starting_position, (max_resources * 8 + 10) / 20, 100);
        }
    }

    allocator_materials.SeparateResources(&allocator_fuel);

    allocator_materials.ConcentrateResources();
    allocator_fuel.ConcentrateResources();
    allocator_gold.ConcentrateResources();

    ResourceAllocator::SettleMinimumResourceLevels(ini_get_setting(INI_MIN_RESOURCES));
}

void GameManager_SpawnAlienDerelicts(Point point, int alien_unit_value) {
    /// \todo
}

void GameManager_PopulateMapWithAlienUnits(int alien_seperation, int alien_unit_value) {
    if (alien_unit_value > 0) {
        Point point1;
        Point point2;
        bool flag;

        point1.x = (((alien_seperation * 4) / 5 + 1) * dos_rand()) >> 15;
        point1.y = ((alien_seperation / 2 + 1) * dos_rand()) >> 15;

        for (int i = point1.x; i < ResourceManager_MapSize.x; i += (alien_seperation * 4) / 5) {
            for (int j = flag ? (alien_seperation / 2 + point1.y) : point1.y; j < ResourceManager_MapSize.y;
                 j += alien_seperation) {
                point2.x = (((((alien_seperation / 5) - ((-alien_seperation) / 5)) + 1) * dos_rand()) >> 15) +
                           ((-alien_seperation) / 5) + i;
                point2.y = (((((alien_seperation / 5) - ((-alien_seperation) / 5)) + 1) * dos_rand()) >> 15) +
                           ((-alien_seperation) / 5) + i;

                GameManager_SpawnAlienDerelicts(point2, alien_unit_value);
            }

            flag = !flag;
        }
    }
}

void GameManager_ProcessTeamMissionSupplyUnits(unsigned short team) {
    /// \todo
}

bool GameManager_ProcessTick(bool render_screen) {
    /// \todo
}

void GameManager_ProcessState(bool process_tick, bool clear_mouse_events) {
    /// \todo
}

void GameManager_GuiSwitchTeam(unsigned short team) {
    /// \todo
}

void GameManager_EnableMainMenu(UnitInfo* unit) {
    if (!GameManager_MainMenuFreezeState && GameManager_DisplayControlsInitialized) {
        Gamemanager_FlicButton->Enable();

        for (int i = 0; i < sizeof(GameManager_MenuItems) / sizeof(struct MenuGuiItem); ++i) {
            GameManager_MenuItems[i].button->SetRestState(GameManager_MenuItems[i].disabled);
            GameManager_MenuItems[i].button->Enable();
        }

        if (GameManager_PlayMode) {
            GameManager_MenuEndTurnButtonSetState(GUI_GameState == GAME_STATE_8_IN_GAME);
        } else {
            GameManager_MenuEndTurnButtonSetState(UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type ==
                                                  TEAM_TYPE_PLAYER);
        }

        if (GUI_GameState != GAME_STATE_9 || GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
            bool flag = true;

            for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER) {
                    flag = false;
                    break;
                }
            }

            if (UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type == TEAM_TYPE_PLAYER || flag) {
                if (unit) {
                    GameManager_MenuUnitSelect(unit);
                }

                GameManager_MainMenuFreezeState = true;
            }
        }
    }
}

void GameManager_DisableMainMenu() {
    if (GameManager_MainMenuFreezeState) {
        GameManager_MenuDeleteFlic();

        Gamemanager_FlicButton->Disable();

        for (int i = 0; i < sizeof(GameManager_MenuItems) / sizeof(struct MenuGuiItem); ++i) {
            if (GameManager_MenuItems[i].button) {
                GameManager_MenuItems[i].button->Disable();
            }
        }

        GameManager_MainMenuFreezeState = false;
    }
}

void GameManager_MenuDeleteFlic() {
    if (GameManager_Flic) {
        flicsmgr_delete(GameManager_Flic);
        free(GameManager_Flic);
        GameManager_Flic = nullptr;
    }
}

void GameManager_MenuEndTurnButtonSetState(bool state) {
    GameManager_MenuItems[MENU_GUI_ITEM_ENDTURN_BUTTON].button->CopyDown(
        static_cast<ResourceID>(GameManager_ActiveTurnTeam + R_ENDT_D));
    GameManager_MenuItems[MENU_GUI_ITEM_ENDTURN_BUTTON].button->Disable();
    GameManager_MenuItems[MENU_GUI_ITEM_ENDTURN_BUTTON].button->Enable();
    GameManager_MenuItems[MENU_GUI_ITEM_ENDTURN_BUTTON].button->SetRestState(!state);
}

void GameManager_MenuUnitSelect(UnitInfo* unit) {
    /// \todo
}

void GameManager_FillOrRestoreWindow(unsigned char id, int color, bool redraw) {
    WindowInfo* window;

    window = WindowManager_GetWindow(id);

    switch (id) {
        case WINDOW_CORNER_FLIC: {
            GameManager_MenuDisplayControls[MENU_DISPLAY_CONTROL_CORNER_FLIC].image->Write(window);
        } break;

        case WINDOW_STAT_WINDOW: {
            GameManager_MenuDisplayControls[MENU_DISPLAY_CONTROL_STAT_WINDOW].image->Write(window);
        } break;

        default: {
            buf_fill(window->buffer, window->window.lrx - window->window.ulx, window->window.lry - window->window.uly,
                     window->width, color);
        } break;
    }

    if (redraw) {
        win_draw_rect(window->id, &window->window);
    }
}

void GameManager_MenuInitButtons(bool mode) {
    WindowInfo* window;
    int r_value;
    int p_value;
    unsigned int flags;

    window = WindowManager_GetWindow(WINDOW_CORNER_FLIC);

    if (GameManager_GameFileNumber && !Remote_IsNetworkGame) {
        GameManager_MenuItems[MENU_GUI_ITEM_CHAT_BUTTON].gfx = GOAL_OFF;
        GameManager_MenuItems[MENU_GUI_ITEM_CHAT_BUTTON].label = "Goal";
    } else {
        GameManager_MenuItems[MENU_GUI_ITEM_CHAT_BUTTON].gfx = CHAT_OFF;
        GameManager_MenuItems[MENU_GUI_ITEM_CHAT_BUTTON].label = "Chat";
    }

    for (int i = 0; i < sizeof(GameManager_MenuItems) / sizeof(struct MenuGuiItem); ++i) {
        delete GameManager_MenuItems[i].button;
        GameManager_MenuItems[i].button = nullptr;

        if (i != MENU_GUI_ITEM_ENDTURN_BUTTON || mode) {
            window = WindowManager_GetWindow(GameManager_MenuItems[i].wid);

            GameManager_MenuItems[i].button = new (std::nothrow)
                Button(GameManager_MenuItems[i].gfx, static_cast<ResourceID>(GameManager_MenuItems[i].gfx + 1),
                       window->window.ulx, window->window.uly);
            r_value = 1000 + GameManager_MenuItems[i].wid;
            p_value = r_value;

            flags = 0x20;
            text_font(2);

            if (i == MENU_GUI_ITEM_ENDTURN_BUTTON) {
                flags |= 0x4;
                text_font(5);
            }

            if (i <= MENU_GUI_ITEM_CHAT_BUTTON) {
                p_value = GNW_INPUT_PRESS + GameManager_MenuItems[i].wid;
            } else {
                flags |= 0x1;
            }

            GameManager_MenuItems[i].button->SetFlags(flags);
            GameManager_MenuItems[i].button->SetRValue(r_value);
            GameManager_MenuItems[i].button->SetPValue(p_value);

            if (GameManager_MenuItems[i].label) {
                if (i == MENU_GUI_ITEM_ENDTURN_BUTTON) {
                    GameManager_MenuItems[i].button->SetCaption(
                        GameManager_MenuItems[i].label, GameManager_MenuItemLabelOffsets[i].x,
                        GameManager_MenuItemLabelOffsets[i].y, FontColor(0xA0, 0xA2, 0xE6),
                        FontColor(0xA0, 0xA2, 0xE6));
                } else {
                    GameManager_MenuItems[i].button->SetCaption(
                        GameManager_MenuItems[i].label, GameManager_MenuItemLabelOffsets[i].x,
                        GameManager_MenuItemLabelOffsets[i].y, FontColor(0xA0, 0xA7, 0xE6),
                        FontColor(0xA2, 0xA3, 0x24));
                }
            }

            GameManager_MenuItems[i].button->RegisterButton(window->id);

            if (i == MENU_GUI_ITEM_ENDTURN_BUTTON) {
                GameManager_MenuItems[i].button->Disable();
            }

            GameManager_MenuItems[i].button->Enable();
        }
    }

    text_font(5);

    for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        struct ImageSimpleHeader* image;
        WindowInfo wininfo;
        int color;

        image = reinterpret_cast<struct ImageSimpleHeader*>(
            ResourceManager_LoadResource(static_cast<ResourceID>(R_ENDT_D + i)));

        wininfo.id = window->id;
        wininfo.buffer = image->data;
        wininfo.window.ulx = 0;
        wininfo.window.uly = 0;
        wininfo.window.lrx = image->width;
        wininfo.window.lry = image->height;
        wininfo.width = image->width;

        if (i == PLAYER_TEAM_GRAY) {
            color = 0xFF;
        } else {
            color = i + 1;
        }

        Text_TextBox(&wininfo, "End Turn", 0, 0, image->width, image->height, true, true,
                     FontColor(color, color, 0x3F));
    }
}

void GameManager_MenuDeinitButtons() {
    for (int i = 0; i < sizeof(GameManager_MenuItems) / sizeof(struct MenuGuiItem); ++i) {
        delete GameManager_MenuItems[i].button;
        GameManager_MenuItems[i].button = nullptr;
    }
}

void GameManager_MenuDeinitDisplayControls() {
    if (GameManager_DisplayControlsInitialized) {
        delete Gamemanager_FlicButton;
        Gamemanager_FlicButton = nullptr;

        GameManager_MenuDeinitButtons();

        for (int i = 0; i < sizeof(GameManager_MenuDisplayControls) / sizeof(struct MenuDisplayControl); ++i) {
            delete GameManager_MenuDisplayControls[i].image;
            GameManager_MenuDisplayControls[i].image = nullptr;

            delete GameManager_MenuDisplayControls[i].button;
            GameManager_MenuDisplayControls[i].button = nullptr;
        }

        GameManager_DisplayControlsInitialized = false;
        GameManager_MainMenuFreezeState = false;
    }
}

void GameManager_DrawProximityZones() {
    WindowInfo* window;
    int ini_proximity_range;
    int ini_exclude_range;
    int range;
    Rect bounds;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    SmartPointer<UnitValues> base_values = GameManager_SelectedUnit->GetBaseValues();

    GameManager_UpdateDrawBounds();

    range = 0;
    base_values->SetAttribute(ATTRIB_RANGE, range);

    ini_proximity_range = ini_get_setting(INI_PROXIMITY_RANGE);
    ini_exclude_range = ini_get_setting(INI_EXCLUDE_RANGE);

    for (int proximity_range = 1; proximity_range <= ini_proximity_range; ++proximity_range) {
        base_values->SetAttribute(ATTRIB_SCAN, proximity_range);

        if (range < ini_exclude_range) {
            ++range;
            base_values->SetAttribute(ATTRIB_RANGE, range);
        }

        bounds.ulx = GameManager_SelectedUnit->x - proximity_range << 6;
        bounds.lrx = GameManager_SelectedUnit->x + proximity_range << 6;
        bounds.uly = GameManager_SelectedUnit->y - proximity_range << 6;
        bounds.lry = GameManager_SelectedUnit->y + proximity_range << 6;

        GameManager_AddDrawBounds(&bounds);

        while (!GameManager_ProcessTick(false)) {
        }
    }
}
