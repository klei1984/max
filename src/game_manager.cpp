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
#include "drawmap.hpp"
#include "flicsmgr.hpp"
#include "gfx.hpp"
#include "gui.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "menu.hpp"
#include "menulandingsequence.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "movie.hpp"
#include "paths.hpp"
#include "remote.hpp"
#include "reportmenu.hpp"
#include "resource_manager.hpp"
#include "saveloadmenu.hpp"
#include "sound_manager.hpp"
#include "task_manager.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "unitstats.hpp"
#include "window_manager.hpp"

#define MENU_GUI_ITEM_DEF(id, gfx, label, button, state, sfx) \
    { (id), (gfx), (label), (button), (state), (sfx) }

#define MENU_DISPLAY_CONTROL_DEF(wid, rid, u1, button, u2, u3, image) \
    { (wid), (rid), (u1), (button), (u2), (u3), (image) }

#define MENU_GUI_ITEM_FILES_BUTTON 0
#define MENU_GUI_ITEM_PREFS_BUTTON 1
#define MENU_GUI_ITEM_PLAY_BUTTON 2
#define MENU_GUI_ITEM_PAUSE_BUTTON 3
#define MENU_GUI_ITEM_CENTER_BUTTON 4
#define MENU_GUI_ITEM_PREV_BUTTON 5
#define MENU_GUI_ITEM_DONE_BUTTON 6
#define MENU_GUI_ITEM_NEXT_BUTTON 7
#define MENU_GUI_ITEM_HELP_BUTTON 8
#define MENU_GUI_ITEM_REPORTS_BUTTON 9
#define MENU_GUI_ITEM_CHAT_BUTTON 10
#define MENU_GUI_ITEM_SURVEY_BUTTON 11
#define MENU_GUI_ITEM_STATUS_BUTTON 12
#define MENU_GUI_ITEM_COLORS_BUTTON 13
#define MENU_GUI_ITEM_HITS_BUTTON 14
#define MENU_GUI_ITEM_AMMO_BUTTON 15
#define MENU_GUI_ITEM_RANGE_BUTTON 16
#define MENU_GUI_ITEM_SCAN_BUTTON 17
#define MENU_GUI_ITEM_GRID_BUTTON 18
#define MENU_GUI_ITEM_NAMES_BUTTON 19
#define MENU_GUI_ITEM_LOCK_BUTTON 20
#define MENU_GUI_ITEM_2X_MINIMAP_BUTTON 21
#define MENU_GUI_ITEM_TNT_MINIMAP_BUTTON 22
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
    Button* buttons[10];
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

unsigned char GameManager_MouseButtons;
unsigned char GameManager_ActiveWindow;
int GameManager_MouseX;
int GameManager_MouseY;
Point GameManager_MousePosition;
Point GameManager_LastMousePosition;
Point GameManager_MousePosition2;
Point GameManager_ScaledMousePosition;
Rect GameManager_RenderArea;
UnitInfo* GameManager_Unit;

Rect GameManager_GridPosition;
Rect GameManager_MapWindowDrawBounds;
SmartPointer<UnitInfo> GameManager_SelectedUnit;
SmartPointer<UnitInfo> GameManager_TempTape;
SmartPointer<UnitInfo> GameManager_UnknownUnit2;
SmartPointer<UnitInfo> GameManager_UnknownUnit3;
SmartPointer<UnitInfo> GameManager_UnknownUnit4;
Image* GameManager_TurnTimerImage;
Point GameManager_GridCenter;
Point GameManager_GridCenterOffset;
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

bool GameManager_SiteSelectReleaseEvent;
bool GameManager_WrapUpGame;
bool GameManager_IsTurnTimerActive;
unsigned char GameManager_RenderState;
bool GameManager_RenderEnable;
bool GameManager_PlayFlic;
bool GameManager_Progress1;
bool GameManager_Progress2;
bool GameManager_MaxSurvey;
bool GameManager_UnknownFlag3;
bool GameManager_MainMenuFreezeState;
bool GameManager_IsCtrlKeyPressed;
bool GameManager_IsShiftKeyPressed;
bool GameManager_DisplayControlsInitialized;
bool GameManager_RenderFlag1;
bool GameManager_RenderMinimapDisplay;
bool GameManager_RenderFlag2;

ResourceID GameManager_UnitType;

Button* Gamemanager_FlicButton;

unsigned char GameManager_CheaterTeam;
SmartString GameManager_TextInput;

unsigned char GameManager_ActiveTurnTeam;
unsigned char GameManager_MarkerColor = 0xFF;
unsigned short GameManager_MainMapWidth;
unsigned short GameManager_MainMapHeight;
unsigned char GameManager_PlayMode;
unsigned char GameManager_FastMovement;
unsigned short GameManager_MultiChatTargets[PLAYER_TEAM_MAX - 1];

Point GameManager_GridOffset;
int GameManager_QuickScroll;
int GameManager_GridStepLevel;
int GameManager_GridStepOffset;

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
static bool GameManager_ProcessTextInput(int key);
static void GameManager_ProcessKey();
static void GameManager_UnitSelectOther(UnitInfo* unit1, UnitInfo* unit2, int grid_x, int grid_y);
static void GameManager_UnitSelect(UnitInfo* unit);
static void GameManager_ClickUnit(UnitInfo* unit1, UnitInfo* unit2, int grid_x, int grid_y);
static bool GameManager_UpdateSelection(UnitInfo* unit1, UnitInfo* unit2, int grid_x, int grid_y);
static void GameManager_SetGridOffset(int grid_x_offset, int grid_y_offset);
static void GameManager_ProcessInput();
static bool GameManager_CargoSelection(unsigned short team);
static void GameManager_UpdateTurnTimer(bool mode, int turn_time);
static void GameManager_DrawDisplayPanel(int control_id, char* text, int color, int ulx = 0);
static void GameManager_ProgressBuildState(unsigned short team);
static void GameManager_sub_9E750(unsigned short team, int game_state, bool enable_autosave);
static bool GameManager_AreTeamsFinishedTurn();
static void GameManager_ProgressTurn();
static void GameManager_ResetRenderState();
static bool GameManager_ProcessPopupMenuInput(int key);
static void GameManager_AnnounceWinner(unsigned short team);
static void GameManager_UpdateScoreGraph();
static void GameManager_InitUnitsAndGameState();
static bool GameManager_InitGame();
static Color* GameManager_MenuFadeOut(int fade_steps = 50);
static void GameManager_UpdateHumanPlayerCount();
static void GameManager_MenuAnimateDisplayControls();
static void GameManager_ManagePlayerAction();
static void GameManager_DeinitPopupButtons(bool clear_mouse_events);
static void GameManager_GetGridCenterOffset(bool minimap_zoom_state);

static void GameManager_MenuClickLockButton(bool rest_state);
static void GameManager_MenuClickReportButton();
static void GameManager_MenuClickStatusButton(bool rest_state);
static void GameManager_MenuClickRangeButton(bool rest_state);
static void GameManager_MenuClickColorsButton(bool rest_state);
static void GameManager_MenuClickHitsButton(bool rest_state);
static void GameManager_MenuClickNamesButton(bool rest_state);
static void GameManager_MenuClickAmmoButton(bool rest_state);
static void GameManager_MenuClickMinimap2xButton(bool rest_state);
static void GameManager_MenuClickMinimapTntButton(bool rest_state);
static void GameManager_MenuClickHelpButton();
static void GameManager_MenuClickPlayFlicButton();
static void GameManager_MenuClickPauseFlicButton();
static void GameManager_MenuClickFindButton();
static void GameManager_MenuClickScanButton(bool rest_state);
static void GameManager_MenuClickChatGoalButton();
static void GameManager_MenuClickPreferencesButton();
static void GameManager_MenuClickFileButton(bool is_saving_allowed, bool is_text_mode);
static void GameManager_MenuClickGridButton(bool rest_state);
static void GameManager_MenuClickEndTurnButton(bool state);
static void GameManager_MenuClickSurveyButton(bool rest_state);

static void GameManager_SaveLoadGame(bool save_load_mode);
static void GameManager_MenuInitDisplayControls();
static void GameManager_DrawMouseCoordinates(int x, int y);
static bool GameManager_HandleProximityOverlaps();
static void GameManager_SetUnitOrder(int order, int state, UnitInfo* unit, int offset_x, int offset_y);
static bool GameManager_IsValidStartingPosition(int grid_x, int grid_y);
static unsigned char GameManager_GetWindowCursor(int grid_x, int grid_y);
static void GameManager_PathBuild(UnitInfo* unit);
static void GameManager_ReloadUnit(UnitInfo* unit1, UnitInfo* unit2);
static void GameManager_RepairUnit(UnitInfo* unit1, UnitInfo* unit2);
static void GameManager_TransferCargo(UnitInfo* unit1, UnitInfo* unit2);
static void GameManager_StealUnit(UnitInfo* unit1, UnitInfo* unit2);
static void GameManager_DisableUnit(UnitInfo* unit1, UnitInfo* unit2);
static void GameManager_IsValidStartingRange(short* grid_x, short* grid_y);
static void GameManager_PopulateMapWithResources();
static void GameManager_SpawnAlienDerelicts(Point point, int alien_unit_value);
static void GameManager_PopulateMapWithAlienUnits(int alien_seperation, int alien_unit_value);
static void GameManager_ProcessTeamMissionSupplyUnits(unsigned short team);
static void GameManager_FlicButtonRFunction(ButtonID bid, int value);
static void GameManager_MenuDeinitDisplayControls();
static void GameManager_DrawProximityZones();
static int GameManager_UpdateProximityState(unsigned short team);
static UnitInfo* GameManager_GetFirstRelevantUnit(unsigned short team);

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
                GameManager_ResetRenderState();

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

                    GameManager_ResetRenderState();

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
    Rect bounds;

    DrawMap_ClearDirtyZones();

    rect_init(&bounds, GameManager_MapWindowDrawBounds.ulx, GameManager_MapWindowDrawBounds.uly,
              GameManager_MapWindowDrawBounds.lrx + 1, GameManager_MapWindowDrawBounds.lry + 1);

    GameManager_RenderFlag2 = true;
    GameManager_RenderMinimapDisplay = true;
    GameManager_RenderFlag1 = true;

    Drawmap_UpdateDirtyZones(&bounds);
}

void GameManager_AddDrawBounds(Rect* bounds) {
    Rect new_bounds;

    rect_init(&new_bounds, bounds->ulx, bounds->uly, bounds->lrx + 1, bounds->lry + 1);

    Drawmap_UpdateDirtyZones(&new_bounds);

    GameManager_RenderFlag1 = true;
}

void GameManager_DeployUnit(unsigned short team, ResourceID unit_type, int grid_x, int grid_y) {
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

bool GameManager_RefreshOrders(unsigned short team, bool check_production) {
    /// \todo
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

void GameManager_SelectNextUnit(int seek_direction) {
    /// \todo
}

bool GameManager_UpdateMapDrawBounds(int ulx, int uly) {
    int width;
    int height;
    int width_pixels;
    int height_pixels;
    bool result;

    width = GameManager_MapWindowDrawBounds.lrx - GameManager_MapWindowDrawBounds.ulx + 1;
    height = GameManager_MapWindowDrawBounds.lry - GameManager_MapWindowDrawBounds.uly + 1;

    width_pixels = (GameManager_MainMapWidth * Gfx_MapScalingFactor) >> 16;
    height_pixels = (GameManager_MainMapHeight * Gfx_MapScalingFactor) >> 16;

    if (ulx < 0) {
        ulx = 0;
    }

    if (uly < 0) {
        uly = 0;
    }

    while (((width_pixels << 16) / Gfx_MapScalingFactor) < GameManager_MainMapWidth) {
        ++width_pixels;
    }

    while (((height_pixels << 16) / Gfx_MapScalingFactor) < GameManager_MainMapHeight) {
        ++height_pixels;
    }

    if (ulx + width_pixels >= 112 * 64) {
        ulx = 112 * 64 - width_pixels;
    }

    if (uly + height_pixels >= 112 * 64) {
        uly = 112 * 64 - height_pixels;
    }

    if (ulx == GameManager_MapWindowDrawBounds.ulx && uly == GameManager_MapWindowDrawBounds.uly &&
        width_pixels == width && height_pixels == height) {
        result = false;
    } else {
        GameManager_MapWindowDrawBounds.ulx = ulx;
        GameManager_MapWindowDrawBounds.uly = uly;
        GameManager_MapWindowDrawBounds.lrx = ulx + width_pixels - 1;
        GameManager_MapWindowDrawBounds.lry = uly + height_pixels - 1;

        Gfx_MapWindowUlx = (GameManager_MapWindowDrawBounds.ulx << 16) / Gfx_MapScalingFactor;
        Gfx_MapWindowUly = (GameManager_MapWindowDrawBounds.uly << 16) / Gfx_MapScalingFactor;

        GameManager_GridPosition.ulx = GameManager_MapWindowDrawBounds.ulx / 64;
        GameManager_GridPosition.uly = GameManager_MapWindowDrawBounds.uly / 64;
        GameManager_GridPosition.lrx = GameManager_MapWindowDrawBounds.lrx / 64;
        GameManager_GridPosition.lry = GameManager_MapWindowDrawBounds.lry / 64;

        result = true;
    }

    return result;
}

void GameManager_UpdateMainMapView(int mode, int ulx, int uly, bool flag) {
    int grid_ulx;
    int grid_uly;
    unsigned int new_zoom_level;

    if (mode == 2) {
        grid_ulx = GameManager_GridPosition.ulx;
        grid_uly = GameManager_GridPosition.uly;

        ulx = ulx * GameManager_GridStepLevel + GameManager_MapWindowDrawBounds.ulx;

        uly = uly * GameManager_GridStepLevel + GameManager_MapWindowDrawBounds.uly;

        GameManager_GridStepOffset += GameManager_GridStepLevel;

        if (GameManager_UpdateMapDrawBounds(ulx, uly)) {
            GameManager_GridCenter.x += GameManager_GridPosition.ulx - grid_uly;
            GameManager_GridCenter.y += GameManager_GridPosition.uly - grid_ulx;

        } else {
            return;
        }

    } else if (mode == 0 || mode == 1) {
        if (mode == 0) {
            new_zoom_level = std::min(64, std::max(4, ulx));

            if (Gfx_ZoomLevel == new_zoom_level && !flag) {
                return;
            }

            Gfx_ZoomLevel = new_zoom_level;

            if (GUI_GameState != GAME_STATE_7_SITE_SELECT) {
                WindowInfo* window;

                window = WindowManager_GetWindow(WINDOW_ZOOM_SLIDER_WINDOW);
                WindowManager_LoadImage2(ZOOMPNL1, window->window.ulx, window->window.uly, false);

                new_zoom_level = ((window->window.lrx - window->window.ulx - 28) * (64 - Gfx_ZoomLevel)) / 60 + 14;

                WindowManager_LoadImage2(ZOOMPTR, new_zoom_level + window->window.ulx, window->window.uly + 1, true);
            }

            Gfx_MapScalingFactor = 0x400000 / Gfx_ZoomLevel;

            GameManager_QuickScroll = ini_get_setting(INI_QUICK_SCROLL);

            if (GameManager_QuickScroll < 4) {
                GameManager_QuickScroll = 16;
                ini_set_setting(INI_QUICK_SCROLL, GameManager_QuickScroll);
            }

            GameManager_QuickScroll = (Gfx_MapScalingFactor * GameManager_QuickScroll) >> 16;

            GameManager_GridStepLevel = GameManager_QuickScroll;
            GameManager_GridStepOffset = 0;

            ulx = GameManager_GridCenter.x;
            uly = GameManager_GridCenter.y;

            GameManager_GridCenter.x = 0;
            GameManager_GridCenter.y = 0;
        }

        ulx = std::min(ResourceManager_MapSize.x - 4, std::max(ulx, 3));
        uly = std::min(ResourceManager_MapSize.y - 4, std::max(uly, 3));

        if (GameManager_GridCenter.x == ulx && GameManager_GridCenter.y == uly) {
            return;
        }

        GameManager_GridCenter.x = ulx;
        GameManager_GridCenter.y = uly;

        ulx = (GameManager_GridCenter.x << 6) + 32;
        uly = (GameManager_GridCenter.y << 6) + 32;

        ulx -= (((GameManager_MainMapWidth * Gfx_MapScalingFactor) >> 16) / 2);
        uly -= (((GameManager_MainMapHeight * Gfx_MapScalingFactor) >> 16) / 2);

        if (!GameManager_UpdateMapDrawBounds(ulx, uly)) {
            return;
        }
    }

    GameManager_GetGridCenterOffset(GameManager_DisplayButtonMinimap2x);
    GameManager_DeinitPopupButtons(false);
    GameManager_UpdateDrawBounds();

    if (GameManager_RenderState == 2) {
        GameManager_RenderEnable = true;
    }

    soundmgr.UpdateSfxPosition();
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

        unit = GameManager_GetFirstRelevantUnit(GUI_PlayerTeamIndex);

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
    do {
        GameManager_Progress2 = true;
        GameManager_ProcessInput();
    } while (GameManager_Progress1 && GameManager_Progress2);

    GameManager_ProcessTick(false);
}

void GameManager_ResetRenderState() {
    if (GameManager_RenderState) {
        GameManager_UpdateDrawBounds();
    }

    GameManager_RenderState = 0;
    GameManager_RenderEnable = false;
    GameManager_DeinitPopupButtons(false);
    GameManager_ManagePlayerAction();
    MessageManager_ClearMessageBox();
}

bool GameManager_ProcessPopupMenuInput(int key) {
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

    GameManager_SiteSelectReleaseEvent = false;
    GameManager_WrapUpGame = false;
    GameManager_IsTurnTimerActive = true;
    GameManager_RenderState = 0;
    GameManager_RenderEnable = false;
    MessageManager_MessageBox_IsActive = false;
    GameManager_PlayFlic = false;
    GameManager_Progress1 = false;
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

UnitInfo* GameManager_GetFirstRelevantUnit(unsigned short team) {
    UnitInfo* unit;

    unit = Access_GetFirstMiningStation(team);

    if (!unit) {
        unit = Access_GetFirstActiveUnit(GUI_PlayerTeamIndex, UnitsManager_MobileLandSeaUnits);
    }

    if (!unit) {
        unit = Access_GetFirstActiveUnit(GUI_PlayerTeamIndex, UnitsManager_MobileAirUnits);
    }

    if (!unit) {
        unit = Access_GetFirstActiveUnit(GUI_PlayerTeamIndex, UnitsManager_StationaryUnits);
    }

    return unit;
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

void GameManager_SelectBuildSite(UnitInfo* unit) {
    /// \todo
}

void GameManager_ManagePlayerAction() {
    if (GameManager_SelectedUnit != nullptr && GameManager_SelectedUnit->state == ORDER_STATE_25) {
        GameManager_SelectBuildSite(&*GameManager_SelectedUnit);
    }
}

void GameManager_DeinitPopupButtons(bool clear_mouse_events) {
    if (GameManager_PopupButtons.popup_count) {
        WindowInfo* window1;
        WindowInfo* window2;
        Image* image;
        Rect bounds;

        window1 = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
        image = new (std::nothrow) Image(GameManager_PopupButtons.ulx, GameManager_PopupButtons.uly,
                                         GameManager_PopupButtons.width, GameManager_PopupButtons.height);

        image->Copy(window1);

        for (int i = 0; i < GameManager_PopupButtons.popup_count; ++i) {
            delete GameManager_PopupButtons.buttons[i];
        }

        image->Write(window1);
        delete image;

        window2 = WindowManager_GetWindow(WINDOW_MAIN_MAP);
        window1 = WindowManager_GetWindow(WINDOW_UNKNOWN_01);

        bounds.ulx = (((window1->window.ulx - window2->window.ulx) * Gfx_MapScalingFactor) >> 16) +
                     GameManager_MapWindowDrawBounds.ulx;
        bounds.uly = (((window1->window.uly - window2->window.uly) * Gfx_MapScalingFactor) >> 16) +
                     GameManager_MapWindowDrawBounds.uly;
        bounds.lrx = (((window1->window.lrx - window2->window.ulx) * Gfx_MapScalingFactor) >> 16) + bounds.ulx;
        bounds.lry = (((window1->window.lry - window2->window.uly) * Gfx_MapScalingFactor) >> 16) + bounds.uly;

        GameManager_AddDrawBounds(&bounds);

        window1->window.ulx = -1;
        window1->window.uly = -1;
        window1->window.lrx = -1;
        window1->window.lry = -1;

        GameManager_PopupButtons.popup_count = 0;

        if (clear_mouse_events) {
            MouseEvent::Clear();
        }
    }
}

void GameManager_GetGridCenterOffset(bool minimap_zoom_state) {
    if (minimap_zoom_state) {
        if (GameManager_GridCenter.x <= GameManager_GridCenterOffset.x) {
            GameManager_GridCenterOffset.x -= 4;

        } else if (GameManager_GridCenter.x >= GameManager_GridCenterOffset.x + 55) {
            GameManager_GridCenterOffset.x += 4;
        }

        if (GameManager_GridCenter.y <= GameManager_GridCenterOffset.y) {
            GameManager_GridCenterOffset.y -= 4;

        } else if (GameManager_GridCenter.y >= GameManager_GridCenterOffset.y + 55) {
            GameManager_GridCenterOffset.y += 4;
        }

    } else {
        GameManager_GridCenterOffset.x = GameManager_GridCenter.x - 28;
        GameManager_GridCenterOffset.y = GameManager_GridCenter.y - 28;
    }

    GameManager_GridCenterOffset.x = std::min(56, std::max(0, static_cast<int>(GameManager_GridCenterOffset.x)));
    GameManager_GridCenterOffset.y = std::min(56, std::max(0, static_cast<int>(GameManager_GridCenterOffset.y)));
}

void GameManager_MenuClickLockButton(bool rest_state) {
    /// \todo
}

void GameManager_MenuClickReportButton() {
    GameManager_MenuItems[MENU_GUI_ITEM_REPORTS_BUTTON].button->SetRestState(false);
    GameManager_DisableMainMenu();
    ReportMenu_Menu();
    text_font(5);
    GameManager_EnableMainMenu(&*GameManager_SelectedUnit);
}

void GameManager_MenuClickStatusButton(bool rest_state) {
    GameManager_DisplayButtonStatus = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_STATUS_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_STATUS_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickRangeButton(bool rest_state) {
    GameManager_DisplayButtonRange = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_RANGE_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_RANGE_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickColorsButton(bool rest_state) {
    GameManager_DisplayButtonColors = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_COLORS_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_COLORS_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickHitsButton(bool rest_state) {
    GameManager_DisplayButtonHits = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_HITS_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_HITS_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickNamesButton(bool rest_state) {
    GameManager_DisplayButtonNames = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_NAMES_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_NAMES_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickAmmoButton(bool rest_state) {
    GameManager_DisplayButtonAmmo = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_AMMO_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_AMMO_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickMinimap2xButton(bool rest_state) {
    GameManager_DisplayButtonMinimap2x = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_2X_MINIMAP_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_2X_MINIMAP_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickMinimapTntButton(bool rest_state) {
    GameManager_DisplayButtonMinimapTnt = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_TNT_MINIMAP_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_TNT_MINIMAP_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickHelpButton() {
    GameManager_MenuItems[MENU_GUI_ITEM_HELP_BUTTON].button->SetRestState(false);

    if (GUI_GameState == GAME_STATE_7_SITE_SELECT) {
        HelpMenu_Menu(HELPMENU_SITE_SELECT_SETUP, WINDOW_MAIN_MAP);
    } else {
        HelpMenu_Menu(HELPMENU_GAME_SCREEN_SETUP, WINDOW_MAIN_MAP);
    }
}

void GameManager_MenuClickPlayFlicButton() {
    GameManager_PlayFlic = true;
    GameManager_MenuItems[MENU_GUI_ITEM_PLAY_BUTTON].button->SetRestState(false);
}

void GameManager_MenuClickPauseFlicButton() {
    GameManager_PlayFlic = false;
    GameManager_MenuItems[MENU_GUI_ITEM_PAUSE_BUTTON].button->SetRestState(false);
}

void GameManager_MenuClickFindButton() {
    GameManager_MenuItems[MENU_GUI_ITEM_CENTER_BUTTON].button->SetRestState(false);

    if (GameManager_SelectedUnit != nullptr) {
        GameManager_UpdateMainMapView(1, GameManager_SelectedUnit->grid_x, GameManager_SelectedUnit->grid_y);
    } else {
        GameManager_SelectNextUnit(1);
    }

    if (GameManager_DisplayButtonMinimap2x) {
        GameManager_GetGridCenterOffset(false);
    }
}

void GameManager_MenuClickScanButton(bool rest_state) {
    GameManager_DisplayButtonScan = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_SCAN_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_SCAN_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickChatGoalButton() {
    /// \todo
}

void GameManager_MenuClickPreferencesButton() {
    /// \todo
}

void GameManager_MenuClickFileButton(bool is_saving_allowed, bool is_text_mode) {
    /// \todo
}

void GameManager_MenuClickGridButton(bool rest_state) {
    GameManager_DisplayButtonGrid = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_GRID_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_GRID_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickEndTurnButton(bool rest_state) {
    GameManager_MenuItems[MENU_GUI_ITEM_ENDTURN_BUTTON].button->CopyDown(
        static_cast<ResourceID>(GameManager_ActiveTurnTeam + R_ENDT_D));
    GameManager_MenuItems[MENU_GUI_ITEM_ENDTURN_BUTTON].button->Disable();
    GameManager_MenuItems[MENU_GUI_ITEM_ENDTURN_BUTTON].button->Enable();
    GameManager_MenuItems[MENU_GUI_ITEM_ENDTURN_BUTTON].button->SetRestState(!rest_state);
}

void GameManager_MenuClickSurveyButton(bool rest_state) {
    GameManager_DisplayButtonSurvey = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_SURVEY_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_SURVEY_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
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

void GameManager_SaveLoadGame(bool save_load_mode) {
    /// \todo
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

void GameManager_SetUnitOrder(int order, int state, UnitInfo* unit, int offset_x, int offset_y) {
    /// \todo
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

unsigned char GameManager_GetWindowCursor(int grid_x, int grid_y) {
    /// \todo
}

void GameManager_PathBuild(UnitInfo* unit) {
    /// \todo
}

void GameManager_ReloadUnit(UnitInfo* unit1, UnitInfo* unit2) {
    /// \todo
}

void GameManager_RepairUnit(UnitInfo* unit1, UnitInfo* unit2) {
    /// \todo
}

void GameManager_TransferCargo(UnitInfo* unit1, UnitInfo* unit2) {
    /// \todo
}

void GameManager_StealUnit(UnitInfo* unit1, UnitInfo* unit2) {
    /// \todo
}

void GameManager_DisableUnit(UnitInfo* unit1, UnitInfo* unit2) {
    /// \todo
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
    CTInfo* team_info;

    team_info = &UnitsManager_TeamInfo[team];

    team_info->selected_unit = GameManager_SelectedUnit;
    team_info->zoom_level = Gfx_ZoomLevel;
    team_info->camera_position.x = GameManager_GridCenter.x;
    team_info->camera_position.y = GameManager_GridCenter.y;
    team_info->display_button_range = GameManager_DisplayButtonRange;
    team_info->display_button_scan = GameManager_DisplayButtonScan;
    team_info->display_button_status = GameManager_DisplayButtonStatus;
    team_info->display_button_colors = GameManager_DisplayButtonColors;
    team_info->display_button_hits = GameManager_DisplayButtonHits;
    team_info->display_button_names = GameManager_DisplayButtonNames;
    team_info->display_button_ammo = GameManager_DisplayButtonAmmo;
    team_info->display_button_minimap_2x = GameManager_DisplayButtonMinimap2x;
    team_info->display_button_minimap_tnt = GameManager_DisplayButtonMinimapTnt;
    team_info->display_button_grid = GameManager_DisplayButtonGrid;
    team_info->display_button_survey = GameManager_DisplayButtonSurvey;
}

void GameManager_EnableMainMenu(UnitInfo* unit) {
    if (!GameManager_MainMenuFreezeState && GameManager_DisplayControlsInitialized) {
        Gamemanager_FlicButton->Enable();

        for (int i = 0; i < sizeof(GameManager_MenuItems) / sizeof(struct MenuGuiItem); ++i) {
            GameManager_MenuItems[i].button->SetRestState(GameManager_MenuItems[i].disabled);
            GameManager_MenuItems[i].button->Enable();
        }

        if (GameManager_PlayMode) {
            GameManager_MenuClickEndTurnButton(GUI_GameState == GAME_STATE_8_IN_GAME);
        } else {
            GameManager_MenuClickEndTurnButton(UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type ==
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

bool GameManager_ProcessTextInput(int key) {
    /// \todo
}

void GameManager_ProcessKey() {
    CTInfo* team_info;
    int key;

    team_info = &UnitsManager_TeamInfo[GUI_PlayerTeamIndex];
    key = get_input();

    if (GameManager_TextEditUnitName && GameManager_TextEditUnitName->ProcessKeyPress(key)) {
        if (key == GNW_KB_KEY_RETURN) {
            GameManager_SelectedUnit->SetName(GameManager_UnitName);

            delete GameManager_TextEditUnitName;
            GameManager_TextEditUnitName = nullptr;

            if (Remote_IsNetworkGame) {
                Remote_SendNetPacket_43(&*GameManager_SelectedUnit, GameManager_UnitName);
            }
        }

        key = -1;
    }

    if (GameManager_ProcessTextInput(key)) {
        key = -1;
    }

    GameManager_IsShiftKeyPressed = keys[GNW_KB_SCAN_RSHIFT] | keys[GNW_KB_SCAN_LSHIFT];
    GameManager_IsCtrlKeyPressed = keys[GNW_KB_SCAN_LCTRL] | keys[GNW_KB_SCAN_RCTRL];

    if (GameManager_RequestMenuExit) {
        if (UnitsManager_TeamInfo[GUI_PlayerTeamIndex].field_41) {
            for (int team = 0; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER &&
                    !UnitsManager_TeamInfo[team].field_41) {
                    GameManager_RefreshOrders(team, true);
                }
            }

        } else if (UnitsManager_TeamInfo[GUI_PlayerTeamIndex].team_type == TEAM_TYPE_PLAYER ||
                   UnitsManager_TeamInfo[GUI_PlayerTeamIndex].team_type == TEAM_TYPE_COMPUTER) {
            GameManager_DisableMainMenu();

            if (MessageManager_MessageBox_IsActive) {
                key = GNW_KB_KEY_ESCAPE;

            } else if (!GameManager_UnknownFlag3 && !GameManager_Progress1) {
                key = 1003;
            }
        }
    }

    if (key > 0 && key < GNW_INPUT_PRESS) {
        GameManager_SiteSelectReleaseEvent = false;
    }

    switch (key) {
        case GNW_KB_KEY_KP_ENTER: {
            if (GUI_GameState == GAME_STATE_7_SITE_SELECT) {
                MessageManager_ClearMessageBox();

                Ai_SelectStartingPosition(GameManager_ActiveTurnTeam);

                GameManager_GridCenter.x =
                    UnitsManager_TeamMissionSupplies[GameManager_ActiveTurnTeam].starting_position.x;
                GameManager_GridCenter.y =
                    UnitsManager_TeamMissionSupplies[GameManager_ActiveTurnTeam].starting_position.y;

                UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].camera_position.x = GameManager_GridCenter.x;
                UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].camera_position.y = GameManager_GridCenter.y;

                UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].zoom_level = 64;

                GUI_GameState = ORDER_STATE_13;

            } else if (GameManager_DemoMode) {
                GUI_GameState = GAME_STATE_3_MAIN_MENU;
                return;
            }

            if (GameManager_MainMenuFreezeState && !UnitsManager_TeamInfo[GUI_PlayerTeamIndex].field_41) {
                soundmgr.PlaySfx(MBUTT0);
                GameManager_ResetRenderState();

                if (!GameManager_RefreshOrders(GUI_PlayerTeamIndex, GameManager_RequestMenuExit)) {
                    GameManager_MenuClickEndTurnButton(true);
                }
            }
        } break;

        case GNW_KB_KEY_ESCAPE: {
            if (GUI_GameState == GAME_STATE_7_SITE_SELECT) {
                GUI_GameState = GAME_STATE_14;

            } else if (GameManager_Progress1) {
                GameManager_Progress1 = false;

            } else if (GameManager_PopupButtons.popup_count) {
                GameManager_DeinitPopupButtons(false);

            } else if (MessageManager_MessageBox_IsActive) {
                MessageManager_ClearMessageBox();

            } else if (GameManager_DemoMode) {
                GUI_GameState = GAME_STATE_3_MAIN_MENU;

            } else if (OKCancelMenu_menu_loop("OK to exit game?", false)) {
                GUI_GameState = ORDER_STATE_3;
            }

        } break;

        case GNW_KB_KEY_SPACE: {
            if (GameManager_DemoMode) {
                GUI_GameState = GAME_STATE_3_MAIN_MENU;
            }
        } break;

        case GNW_KB_KEY_KP_PLUS:
        case GNW_KB_KEY_EQUALS: {
            if (GameManager_MainMenuFreezeState) {
                GameManager_UpdateMainMapView(0, Gfx_ZoomLevel + 1, 0, false);
            }
        } break;

        case GNW_KB_KEY_KP_MINUS: {
            if (GameManager_MainMenuFreezeState) {
                GameManager_UpdateMainMapView(0, Gfx_ZoomLevel - 1, 0, false);
            }
        } break;

        case GNW_KB_KEY_SHIFT_DIVIDE:
        case 1019: {
            GameManager_MenuClickHelpButton();
        } break;

        case GNW_KB_KEY_F:
        case GNW_KB_KEY_SHIFT_F:
        case 1014: {
            GameManager_MenuClickFindButton();
        } break;

        case GNW_KB_KEY_G:
        case GNW_KB_KEY_SHIFT_G:
        case 1033: {
            soundmgr.PlaySfx(IGRID0);
            GameManager_MenuClickGridButton(!GameManager_DisplayButtonGrid);
        } break;

        case GNW_KB_KEY_LEFTBRACKET: {
            GameManager_TextInput = "[";
        } break;

        case GNW_KB_KEY_LALT_F:
        case 1004: {
            if (GameManager_MainMenuFreezeState) {
                GameManager_MenuClickFileButton(true, false);
            }
        } break;

        case GNW_KB_KEY_LALT_L: {
            if (GameManager_MainMenuFreezeState) {
                if (Remote_IsNetworkGame) {
                    MessageManager_DrawMessage("Unable to load a saved game while remote play in progress.", 2, 1,
                                               true);

                } else {
                    GameManager_SaveLoadGame(false);
                }
            }
        } break;

        case GNW_KB_KEY_LALT_P: {
            PauseMenu_Menu();
        } break;

        case GNW_KB_KEY_LALT_S: {
            if (GameManager_MainMenuFreezeState) {
                GameManager_SaveLoadGame(true);
            }
        } break;

        case GNW_KB_KEY_LALT_X: {
            if (OKCancelMenu_menu_loop("OK to exit game?", false)) {
                GUI_GameState = ORDER_STATE_3;
                GameManager_DeinitPopupButtons(false);
            }
        } break;

        case GNW_KB_KEY_F1: {
            if ((GameManager_MainMenuFreezeState ||
                 UnitsManager_TeamInfo[GUI_PlayerTeamIndex].team_type == TEAM_TYPE_PLAYER) &&
                GameManager_SpottedEnemyPosition.x != -1) {
                GameManager_ManagePlayerAction();
                GameManager_UpdateMainMapView(1, GameManager_SpottedEnemyPosition.x,
                                              GameManager_SpottedEnemyPosition.y);

                if (GameManager_UnknownUnit3 != nullptr &&
                    GameManager_UnknownUnit3->IsVisibleToTeam(GUI_PlayerTeamIndex)) {
                    MessageManager_ClearMessageBox();
                    GameManager_MenuUnitSelect(&*GameManager_UnknownUnit3);
                } else {
                    MessageManager_DrawMessage("Tagged unit out of range.", 0, 0);
                }
            }
        } break;

        case GNW_KB_KEY_F5:
        case GNW_KB_KEY_F6:
        case GNW_KB_KEY_F7:
        case GNW_KB_KEY_F8: {
            if (GameManager_MainMenuFreezeState) {
                if (team_info->screen_location[key - GNW_KB_KEY_F5].x != -1) {
                    GameManager_UpdateMainMapView(1, team_info->screen_location[key - GNW_KB_KEY_F5].x,
                                                  team_info->screen_location[key - GNW_KB_KEY_F5].y);
                }
            }
        } break;

        case GNW_KB_KEY_UP: {
            GameManager_ArrowKeyFlags |= 1;
        } break;

        case GNW_KB_KEY_LEFT: {
            GameManager_ArrowKeyFlags |= 4;
        } break;

        case GNW_KB_KEY_RIGHT: {
            GameManager_ArrowKeyFlags |= 8;
        } break;

        case GNW_KB_KEY_DOWN: {
            GameManager_ArrowKeyFlags |= 2;
        } break;

        case GNW_KB_KEY_LALT_F5:
        case GNW_KB_KEY_LALT_F6:
        case GNW_KB_KEY_LALT_F7:
        case GNW_KB_KEY_LALT_F8: {
            team_info->screen_location[key - GNW_KB_KEY_LALT_F5].x = GameManager_GridCenter.x;
            team_info->screen_location[key - GNW_KB_KEY_LALT_F5].y = GameManager_GridCenter.y;

            MessageManager_DrawMessage("Window position has been saved.", 0, 0);
        } break;

        case 1003: {
            soundmgr.PlaySfx(MBUTT0);
            GameManager_ResetRenderState();

            if (!GameManager_RefreshOrders(GUI_PlayerTeamIndex, GameManager_RequestMenuExit)) {
                GameManager_MenuClickEndTurnButton(true);
            }
        } break;

        case 1005: {
            GameManager_MenuClickPreferencesButton();
        } break;

        case 1007: {
            GameManager_MenuClickPlayFlicButton();
        } break;

        case 1008: {
            GameManager_MenuClickPauseFlicButton();
        } break;

        case 1015: {
            soundmgr.PlaySfx(MBUTT0);
            GameManager_MenuClickLockButton(!GameManager_DisplayButtonLock);
        } break;

        case 1016: {
            GameManager_MenuItems[MENU_GUI_ITEM_PREV_BUTTON].button->SetRestState(false);
            GameManager_SelectNextUnit(0);
            GameManager_MenuClickFindButton();
        } break;

        case 1017: {
            GameManager_MenuItems[MENU_GUI_ITEM_DONE_BUTTON].button->SetRestState(false);

            if (GameManager_IsShiftKeyPressed &&
                UnitsManager_TeamInfo[GUI_PlayerTeamIndex].team_type == TEAM_TYPE_PLAYER) {
                Access_RenewAttackOrders(UnitsManager_MobileLandSeaUnits, GUI_PlayerTeamIndex);
                Access_RenewAttackOrders(UnitsManager_MobileAirUnits, GUI_PlayerTeamIndex);

            } else if (GameManager_SelectedUnit != nullptr && GameManager_SelectedUnit->team == GUI_PlayerTeamIndex &&
                       UnitsManager_TeamInfo[GUI_PlayerTeamIndex].team_type == TEAM_TYPE_PLAYER) {
                UnitsManager_PerformAction(&*GameManager_SelectedUnit);

            } else {
                GameManager_SelectNextUnit(1);
                GameManager_MenuClickFindButton();
            }
        } break;

        case 1018: {
            GameManager_MenuItems[MENU_GUI_ITEM_NEXT_BUTTON].button->SetRestState(false);
            GameManager_SelectNextUnit(1);
            GameManager_MenuClickFindButton();
        } break;

        case 1020: {
            GameManager_ManagePlayerAction();
            GameManager_MenuClickReportButton();
        } break;

        case 1021: {
            GameManager_ManagePlayerAction();
            GameManager_MenuClickChatGoalButton();
        } break;

        case 1026: {
            soundmgr.PlaySfx(ISURV0);
            GameManager_MenuClickSurveyButton(!GameManager_DisplayButtonSurvey);
        } break;

        case 1027: {
            soundmgr.PlaySfx(ISTAT0);
            GameManager_MenuClickStatusButton(!GameManager_DisplayButtonStatus);
        } break;

        case 1028: {
            soundmgr.PlaySfx(ICOLO0);
            GameManager_MenuClickColorsButton(!GameManager_DisplayButtonColors);
        } break;

        case 1029: {
            soundmgr.PlaySfx(IHITS0);
            GameManager_MenuClickHitsButton(!GameManager_DisplayButtonHits);
        } break;

        case 1030: {
            soundmgr.PlaySfx(IAMMO0);
            GameManager_MenuClickAmmoButton(!GameManager_DisplayButtonAmmo);
        } break;

        case 1031: {
            soundmgr.PlaySfx(IRANG0);
            GameManager_MenuClickRangeButton(!GameManager_DisplayButtonRange);
        } break;

        case 1032: {
            soundmgr.PlaySfx(IVISI0);
            GameManager_MenuClickScanButton(!GameManager_DisplayButtonScan);
        } break;

        case 1034: {
            soundmgr.PlaySfx(INAME0);
            GameManager_MenuClickNamesButton(!GameManager_DisplayButtonNames);
        } break;

        case 1036: {
            soundmgr.PlaySfx(IAMMO0);
            GameManager_MenuClickMinimap2xButton(!GameManager_DisplayButtonMinimap2x);
        } break;

        case 1037: {
            soundmgr.PlaySfx(IAMMO0);
            GameManager_MenuClickMinimapTntButton(!GameManager_DisplayButtonMinimapTnt);
        } break;

        default: {
            if (!GameManager_ProcessPopupMenuInput(key) && key >= GNW_INPUT_PRESS &&
                !GameManager_SiteSelectReleaseEvent) {
                if (GUI_GameState == GAME_STATE_7_SITE_SELECT) {
                    if (key == GNW_KB_KEY_SHIFT_DIVIDE && GameManager_LandingSequence.button_1) {
                        GameManager_LandingSequence.button_1->PlaySound();

                    } else if (GameManager_LandingSequence.button_2) {
                        GameManager_LandingSequence.button_2->PlaySound();
                    }

                } else {
                    soundmgr.PlaySfx(MBUTT0);
                }

                GameManager_SiteSelectReleaseEvent = true;
            }
        } break;
    }
}

void GameManager_UnitSelectOther(UnitInfo* unit1, UnitInfo* unit2, int grid_x, int grid_y) {
    /// \todo
}

void GameManager_UnitSelect(UnitInfo* unit) {
    /// \todo
}

void GameManager_ClickUnit(UnitInfo* unit1, UnitInfo* unit2, int grid_x, int grid_y) {
    /// \todo
}

bool GameManager_UpdateSelection(UnitInfo* unit1, UnitInfo* unit2, int grid_x, int grid_y) {
    /// \todo
}

void GameManager_SetGridOffset(int grid_x_offset, int grid_y_offset) {
    if (!ini_get_setting(INI_CLICK_SCROLL) || (GameManager_MouseButtons & MOUSE_PRESS_LEFT) ||
        GameManager_RenderState == 2) {
        GameManager_GridOffset.x = grid_x_offset;
        GameManager_GridOffset.y = grid_y_offset;
    }
}

void GameManager_ProcessInput() {
    CTInfo* team_info;
    WindowInfo* window;
    unsigned char window_index;
    unsigned char window_cursor;

    team_info = &UnitsManager_TeamInfo[GUI_PlayerTeamIndex];

    GameManager_ProcessKey();

    if (GameManager_ArrowKeyFlags) {
        Point position_change(0, 0);

        if (GameManager_ArrowKeyFlags & 1) {
            if (keys[GNW_KB_SCAN_UP_REL] || keys[GNW_KB_SCAN_UP]) {
                position_change.y = -1;
            } else {
                GameManager_ArrowKeyFlags ^= 1;
            }
        }

        if (GameManager_ArrowKeyFlags & 2) {
            if (keys[GNW_KB_SCAN_DOWN_REL] || keys[GNW_KB_SCAN_DOWN]) {
                position_change.y = 1;
            } else {
                GameManager_ArrowKeyFlags ^= 2;
            }
        }

        if (GameManager_ArrowKeyFlags & 4) {
            if (keys[GNW_KB_SCAN_LEFT_REL] || keys[GNW_KB_SCAN_LEFT]) {
                position_change.x = -1;
            } else {
                GameManager_ArrowKeyFlags ^= 4;
            }
        }

        if (GameManager_ArrowKeyFlags & 8) {
            if (keys[GNW_KB_SCAN_RIGHT_REL] || keys[GNW_KB_SCAN_RIGHT]) {
                position_change.x = 1;
            } else {
                GameManager_ArrowKeyFlags ^= 8;
            }
        }

        GameManager_GridOffset.x = position_change.x;
        GameManager_GridOffset.y = position_change.y;
    }

    {
        MouseEvent mouse_event;

        if (MouseEvent::PopFront(mouse_event)) {
            GameManager_MouseButtons = mouse_event.buttons;
            GameManager_MouseX = mouse_event.point.x;
            GameManager_MouseY = mouse_event.point.y;

        } else {
            GameManager_MouseButtons = 0;
            mouse_get_position(&GameManager_MouseX, &GameManager_MouseY);
        }
    }

    for (window_index = WINDOW_UNKNOWN_01; window_index < WINDOW_COUNT; ++window_index) {
        window = WindowManager_GetWindow(window_index);

        if (GameManager_MouseX >= window->window.ulx && GameManager_MouseY >= window->window.uly &&
            GameManager_MouseX <= window->window.lrx && GameManager_MouseY <= window->window.lry) {
            break;
        }
    }

    if (window_index == WINDOW_MESSAGE_BOX) {
        window_index == WINDOW_MAIN_MAP;
    }

    if (GUI_GameState == GAME_STATE_7_SITE_SELECT && window_index != WINDOW_MAIN_MAP) {
        window_index = WINDOW_MAIN_WINDOW;
    }

    window_cursor = Cursor_GetDefaultWindowCursor(window_index);

    if (window_index == WINDOW_MAIN_MAP) {
        int grid_x;
        int grid_y;

        window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

        grid_x = ((((GameManager_MouseX - window->window.ulx) * Gfx_MapScalingFactor) >> 16) +
                  GameManager_MapWindowDrawBounds.ulx) /
                 64;

        grid_y = ((((GameManager_MouseY - window->window.uly) * Gfx_MapScalingFactor) >> 16) +
                  GameManager_MapWindowDrawBounds.uly) /
                 64;

        window_cursor = GameManager_GetWindowCursor(grid_x, grid_y);
    }

    Cursor_SetCursor(window_cursor);

    if (GameManager_MouseButtons & (MOUSE_PRESS_LEFT | MOUSE_PRESS_RIGHT)) {
        GameManager_ActiveWindow = window_index;

        if (window_index == WINDOW_MAIN_MAP) {
            window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

            GameManager_MousePosition2.x = GameManager_MouseX;
            GameManager_MousePosition2.y = GameManager_MouseY;

            GameManager_ScaledMousePosition.x =
                (((GameManager_MouseX - window->window.ulx) * Gfx_MapScalingFactor) >> 16) +
                GameManager_MapWindowDrawBounds.ulx;

            GameManager_ScaledMousePosition.y =
                (((GameManager_MouseY - window->window.uly) * Gfx_MapScalingFactor) >> 16) +
                GameManager_MapWindowDrawBounds.uly;

            GameManager_RenderArea.ulx = GameManager_ScaledMousePosition.x;
            GameManager_RenderArea.lrx = GameManager_ScaledMousePosition.x;
            GameManager_RenderArea.uly = GameManager_ScaledMousePosition.y;
            GameManager_RenderArea.lry = GameManager_ScaledMousePosition.y;

            GameManager_RenderState = 1;
        }
    }

    if (!GameManager_MouseButtons && (mouse_get_buttons() & MOUSE_LONG_PRESS_LEFT)) {
        GameManager_MouseButtons |= MOUSE_PRESS_LEFT;
    }

    if (GameManager_MouseButtons && GameManager_ActiveWindow != window_index) {
        if (GameManager_ActiveWindow == WINDOW_MINIMAP) {
            window_index = WINDOW_MINIMAP;

        } else if ((GameManager_ActiveWindow < WINDOW_SCROLL_UP_WINDOW ||
                    GameManager_ActiveWindow > WINDOW_SCROLL_UP_LEFT_WINDOW || window_index < WINDOW_SCROLL_UP_WINDOW ||
                    window_index > WINDOW_SCROLL_UP_LEFT_WINDOW) &&
                   (GameManager_RenderState == 0 ||
                    (GameManager_MouseButtons & (MOUSE_PRESS_LEFT | MOUSE_PRESS_RIGHT)))) {
            GameManager_MouseButtons = 0;
        }
    }

    if (GameManager_MouseButtons & (MOUSE_RELEASE_LEFT | MOUSE_RELEASE_RIGHT)) {
        GameManager_ActiveWindow = WINDOW_MAIN_WINDOW;

        if (GameManager_RenderState) {
            if (GameManager_RenderState == 2) {
                Access_MultiSelect(&*GameManager_SelectedUnit, &GameManager_RenderArea);
                GameManager_UpdateDrawBounds();
            }

            GameManager_RenderState = 0;
            GameManager_RenderEnable = false;
        }
    }

    switch (window_index) {
        case WINDOW_SCROLL_UP_WINDOW: {
            GameManager_SetGridOffset(0, -1);
        } break;

        case WINDOW_SCROLL_UP_RIGHT_WINDOW:
        case WINDOW_SCROLL_RIGHT_UP_WINDOW: {
            GameManager_SetGridOffset(1, -1);
        } break;

        case WINDOW_SCROLL_RIGHT_WINDOW: {
            GameManager_SetGridOffset(1, 0);
        } break;

        case WINDOW_SCROLL_RIGHT_DOWN_WINDOW:
        case WINDOW_SCROLL_DOWN_RIGHT_WINDOW: {
            GameManager_SetGridOffset(1, 1);
        } break;

        case WINDOW_SCROLL_DOWN_WINDOW: {
            GameManager_SetGridOffset(0, 1);
        } break;

        case WINDOW_SCROLL_DOWN_LEFT_WINDOW:
        case WINDOW_SCROLL_LEFT_DOWN_WINDOW: {
            GameManager_SetGridOffset(-1, 1);
        } break;

        case WINDOW_SCROLL_LEFT_WINDOW: {
            GameManager_SetGridOffset(-1, 0);
        } break;

        case WINDOW_SCROLL_LEFT_UP_WINDOW:
        case WINDOW_SCROLL_UP_LEFT_WINDOW: {
            GameManager_SetGridOffset(-1, -1);
        } break;
    }

    if (window_index == WINDOW_MAIN_MAP) {
        if (GameManager_MousePosition.x != GameManager_LastMousePosition.x ||
            GameManager_MousePosition.y != GameManager_LastMousePosition.y) {
            GameManager_DrawMouseCoordinates(GameManager_MousePosition.x + 1, GameManager_MousePosition.y + 1);

            if (GameManager_UnknownFlag3) {
                UnitsManager_MoveUnit(&*GameManager_UnknownUnit2, GameManager_MousePosition.x,
                                      GameManager_MousePosition.y);
            }

            GameManager_LastMousePosition.x = GameManager_MousePosition.x;
            GameManager_LastMousePosition.y = GameManager_MousePosition.y;
        }
    }

    if (GameManager_MouseButtons) {
        window = WindowManager_GetWindow(window_index);

        switch (window_index) {
            case WINDOW_UNKNOWN_01: {
            } break;

            case WINDOW_MESSAGE_BOX: {
                if (GameManager_MouseButtons & MOUSE_RELEASE_LEFT) {
                    MessageManager_ClearMessageBox();
                }
            } break;

            case WINDOW_CORNER_FLIC: {
                if (GameManager_MouseButtons & MOUSE_RELEASE_LEFT) {
                    if (window->window.uly + 14 <= GameManager_MouseY) {
                        GameManager_PlayFlic = !GameManager_PlayFlic;
                    }
                }
            } break;

            case WINDOW_ZOOM_PLUS_BUTTON: {
                if (GameManager_MouseButtons & MOUSE_RELEASE_LEFT) {
                    GameManager_UpdateMainMapView(0, Gfx_ZoomLevel + 10, 0, false);
                }
            } break;

            case WINDOW_ZOOM_SLIDER_BUTTON: {
                if (GameManager_MouseButtons & MOUSE_PRESS_LEFT) {
                    int slider_offset;

                    window = WindowManager_GetWindow(WINDOW_ZOOM_SLIDER_BUTTON);

                    GameManager_MouseX -= window->window.ulx + 14;

                    slider_offset = window->window.lrx - window->window.ulx - 30;

                    if (GameManager_MouseX < 0) {
                        GameManager_MouseX = 0;
                    }

                    if (slider_offset < GameManager_MouseX) {
                        GameManager_MouseX = slider_offset;
                    }

                    slider_offset = 64 - ((GameManager_MouseX * 60) / slider_offset);

                    GameManager_UpdateMainMapView(0, slider_offset, 0, 0);
                }
            } break;

            case WINDOW_ZOOM_MINUS_BUTTON: {
                if (GameManager_MouseButtons & MOUSE_RELEASE_LEFT) {
                    GameManager_UpdateMainMapView(0, Gfx_ZoomLevel - 10, 0, false);
                }
            } break;

            case WINDOW_MINIMAP: {
                int offset_x;
                int offset_y;

                offset_x = GameManager_MouseX - window->window.ulx;
                offset_y = GameManager_MouseY - window->window.uly;

                offset_x = std::min(ResourceManager_MapSize.x - 1, std::max(0, offset_x));
                offset_y = std::min(ResourceManager_MapSize.y - 1, std::max(0, offset_y));

                if (GameManager_DisplayButtonMinimap2x) {
                    offset_x = (offset_x / 2) + GameManager_GridCenterOffset.x;
                    offset_y = (offset_y / 2) + GameManager_GridCenterOffset.y;
                }

                if (GameManager_MouseButtons & (MOUSE_PRESS_LEFT | MOUSE_RELEASE_LEFT)) {
                    GameManager_UpdateMainMapView(1, offset_x, offset_y);

                } else if (GameManager_MouseButtons & MOUSE_RELEASE_RIGHT) {
                    unsigned char cursor;

                    cursor = GameManager_GetWindowCursor(offset_x, offset_y);

                    if (cursor == CURSOR_UNIT_GO || cursor == CURSOR_WAY) {
                        if (GameManager_SelectedUnit->state != ORDER_STATE_UNIT_READY) {
                            GameManager_SetUnitOrder(ORDER_MOVING,
                                                     cursor == CURSOR_UNIT_GO ? ORDER_STATE_0 : ORDER_STATE_28,
                                                     &*GameManager_SelectedUnit, offset_x, offset_y);
                        }
                    }
                }

            } break;

            case WINDOW_MAIN_MAP: {
                if (!(GameManager_MouseButtons & (MOUSE_PRESS_LEFT | MOUSE_PRESS_RIGHT))) {
                    soundmgr.PlaySfx(KCARG0);

                    if (MessageManager_MessageBox_IsActive) {
                        MessageManager_ClearMessageBox();
                    }

                    if (GUI_GameState == ORDER_STATE_7) {
                        UnitsManager_TeamMissionSupplies[GameManager_ActiveTurnTeam].starting_position =
                            GameManager_MousePosition;
                        UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].camera_position = GameManager_MousePosition;
                        UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].zoom_level = 64;

                        if (Cursor_GetCursor() == CURSOR_UNIT_GO) {
                            GUI_GameState = GAME_STATE_13;
                            soundmgr.PlaySfx(NDONE0);

                        } else {
                            soundmgr.PlaySfx(NCANC0);
                        }

                    } else {
                        UnitInfo* unit = Access_GetUnit(GameManager_MousePosition.x, GameManager_MousePosition.y,
                                                        GUI_PlayerTeamIndex, SELECTABLE);

                        if (GameManager_UnknownFlag3) {
                            if (GameManager_MouseButtons & MOUSE_RELEASE_RIGHT) {
                                if (!unit) {
                                    unit = Access_GetUnit(GameManager_MousePosition.x, GameManager_MousePosition.y);
                                }

                                if (unit) {
                                    UnitsManager_SetNewOrder(unit, ORDER_BUILDING, ORDER_STATE_26);
                                }

                            } else {
                                if (UnitsManager_MoveUnitAndParent(&*GameManager_UnknownUnit2,
                                                                   GameManager_MousePosition.x,
                                                                   GameManager_MousePosition.y)) {
                                    if (Remote_IsNetworkGame) {
                                        Remote_SendNetPacket_14(GUI_PlayerTeamIndex, GameManager_UnitType,
                                                                GameManager_MousePosition.x,
                                                                GameManager_MousePosition.y);
                                    }

                                    GameManager_DeployUnit(GUI_PlayerTeamIndex, GameManager_UnitType,
                                                           GameManager_MousePosition.x, GameManager_MousePosition.y);
                                }
                            }

                        } else {
                            UnitInfo* unit2 = Access_GetUnit6(GUI_PlayerTeamIndex, GameManager_MousePosition.x,
                                                              GameManager_MousePosition.y, SELECTABLE);

                            if (GameManager_MouseButtons & MOUSE_RELEASE_RIGHT) {
                                GameManager_ManagePlayerAction();

                                if (GameManager_SelectedUnit != nullptr &&
                                    (GameManager_SelectedUnit == unit || GameManager_SelectedUnit == unit2)) {
                                    UnitStats_Menu(&*GameManager_SelectedUnit);

                                } else if (unit) {
                                    GameManager_UnitSelect(unit);

                                } else if (unit2) {
                                    if (GameManager_DisplayButtonLock) {
                                        if (GameManager_LockedUnits.Remove(*unit2)) {
                                            GameManager_UpdateDrawBounds();
                                        }

                                        GameManager_LockedUnits.PushBack(*unit2);
                                    }

                                    GameManager_UnitSelectOther(&*GameManager_SelectedUnit, unit2,
                                                                GameManager_MousePosition.x,
                                                                GameManager_MousePosition.y);

                                } else if (GameManager_SelectedUnit != nullptr) {
                                    GameManager_SelectedUnit->targeting_mode = 0;
                                    GameManager_SelectedUnit->enter_mode = 0;
                                    GameManager_SelectedUnit->cursor = CURSOR_HIDDEN;

                                    GameManager_UpdateInfoDisplay(&*GameManager_SelectedUnit);
                                }

                            } else {
                                switch (Cursor_GetCursor()) {
                                    case CURSOR_REPAIR: {
                                        if (GameManager_UpdateSelection(&*GameManager_SelectedUnit, GameManager_Unit,
                                                                        GameManager_MousePosition.x,
                                                                        GameManager_MousePosition.y)) {
                                            GameManager_RepairUnit(&*GameManager_SelectedUnit, GameManager_Unit);
                                        }
                                    } break;

                                    case CURSOR_TRANSFER: {
                                        if (GameManager_UpdateSelection(&*GameManager_SelectedUnit, GameManager_Unit,
                                                                        GameManager_MousePosition.x,
                                                                        GameManager_MousePosition.y)) {
                                            GameManager_TransferCargo(&*GameManager_SelectedUnit, GameManager_Unit);
                                        }
                                    } break;

                                    case CURSOR_FUEL: {
                                    } break;

                                    case CURSOR_RELOAD: {
                                        if (GameManager_UpdateSelection(&*GameManager_SelectedUnit, GameManager_Unit,
                                                                        GameManager_MousePosition.x,
                                                                        GameManager_MousePosition.y)) {
                                            GameManager_ReloadUnit(&*GameManager_SelectedUnit, GameManager_Unit);
                                        }
                                    } break;

                                    case CURSOR_LOAD: {
                                        if (GameManager_UpdateSelection(&*GameManager_SelectedUnit, GameManager_Unit,
                                                                        GameManager_MousePosition.x,
                                                                        GameManager_MousePosition.y)) {
                                            if (GameManager_SelectedUnit->storage <
                                                GameManager_SelectedUnit->GetBaseValues()->GetAttribute(
                                                    ATTRIB_STORAGE)) {
                                                if (GameManager_SelectedUnit->unit_type == AIRTRANS) {
                                                    GameManager_SelectedUnit->SetParent(GameManager_Unit);
                                                    GameManager_SetUnitOrder(
                                                        ORDER_MOVING, ORDER_STATE_0, &*GameManager_SelectedUnit,
                                                        GameManager_MousePosition.x, GameManager_MousePosition.y);

                                                } else if (GameManager_Unit->orders == ORDER_DISABLED) {
                                                    MessageManager_DrawMessage("Unable to load disabled units.", 1, 0);

                                                } else if (GameManager_SelectedUnit->grid_x ==
                                                               GameManager_Unit->grid_x &&
                                                           GameManager_SelectedUnit->grid_y ==
                                                               GameManager_Unit->grid_y &&
                                                           GameManager_Unit->state == ORDER_STATE_1) {
                                                    GameManager_Unit->SetParent(&*GameManager_SelectedUnit);
                                                    UnitsManager_SetNewOrder(GameManager_Unit, ORDER_LANDING,
                                                                             ORDER_STATE_0);

                                                } else {
                                                    GameManager_SetUnitOrder(ORDER_MOVING_27, ORDER_STATE_0,
                                                                             GameManager_Unit,
                                                                             GameManager_SelectedUnit->grid_x,
                                                                             GameManager_SelectedUnit->grid_y);
                                                }
                                            }
                                        }
                                    } break;

                                    case CURSOR_FRIEND: {
                                        if (GameManager_UpdateSelection(&*GameManager_SelectedUnit, unit,
                                                                        GameManager_MousePosition.x,
                                                                        GameManager_MousePosition.y)) {
                                            GameManager_ClickUnit(&*GameManager_SelectedUnit, unit,
                                                                  GameManager_MousePosition.x,
                                                                  GameManager_MousePosition.y);
                                        }
                                    } break;

                                    case CURSOR_ENEMY: {
                                        GameManager_SelectedUnit->target_grid_x = GameManager_MousePosition.x;
                                        GameManager_SelectedUnit->target_grid_y = GameManager_MousePosition.y;

                                        GameManager_SetUnitOrder(
                                            ORDER_ATTACKING, ORDER_STATE_1, &*GameManager_SelectedUnit,
                                            GameManager_MousePosition.x, GameManager_MousePosition.y);
                                    } break;

                                    case CURSOR_FAR_TARGET: {
                                        GameManager_SetUnitOrder(
                                            ORDER_ATTACKING, ORDER_STATE_0, &*GameManager_SelectedUnit,
                                            GameManager_MousePosition.x, GameManager_MousePosition.y);
                                    } break;

                                    case CURSOR_UNIT_GO:
                                    case CURSOR_WAY: {
                                        if (GameManager_SelectedUnit != nullptr) {
                                            if (GameManager_SelectedUnit->enter_mode) {
                                                GameManager_SetUnitOrder(ORDER_MOVING_27, ORDER_STATE_0,
                                                                         &*GameManager_SelectedUnit, unit->grid_x,
                                                                         unit->grid_y);
                                                GameManager_SelectedUnit->enter_mode = false;

                                            } else if (GameManager_SelectedUnit->state == ORDER_STATE_UNIT_READY) {
                                                GameManager_DeinitPopupButtons(false);
                                                GameManager_UpdateDrawBounds();

                                                GameManager_SelectedUnit->target_grid_x = GameManager_MousePosition.x;
                                                GameManager_SelectedUnit->target_grid_y = GameManager_MousePosition.y;

                                                UnitsManager_SetNewOrder(&*GameManager_SelectedUnit,
                                                                         ORDER_ACTIVATE_ORDER, ORDER_STATE_6);

                                            } else {
                                                GameManager_SetUnitOrder(
                                                    ORDER_MOVING,
                                                    Cursor_GetCursor() == CURSOR_UNIT_GO ? ORDER_STATE_0
                                                                                         : ORDER_STATE_28,
                                                    &*GameManager_SelectedUnit, GameManager_MousePosition.x,
                                                    GameManager_MousePosition.y);
                                            }
                                        }
                                    } break;

                                    case CURSOR_UNIT_NO_GO: {
                                        GameManager_ManagePlayerAction();

                                        if (GameManager_SelectedUnit != nullptr) {
                                            GameManager_SelectedUnit->enter_mode = false;
                                            GameManager_SelectedUnit->targeting_mode = false;
                                        }

                                        if (unit) {
                                            GameManager_UnitSelect(unit);

                                        } else if ((GameManager_SelectedUnit != nullptr &&
                                                    GameManager_SelectedUnit == unit2) ||
                                                   unit2) {
                                            if (GameManager_DisplayButtonLock) {
                                                if (GameManager_LockedUnits.Remove(*unit2)) {
                                                    GameManager_UpdateDrawBounds();

                                                } else {
                                                    GameManager_LockedUnits.PushBack(*unit2);
                                                }

                                            } else if (GameManager_SelectedUnit != unit2) {
                                                GameManager_UnitSelectOther(&*GameManager_SelectedUnit, unit2,
                                                                            GameManager_MousePosition.x,
                                                                            GameManager_MousePosition.y);
                                            }
                                        }
                                    } break;

                                    case CURSOR_GROUP: {
                                    } break;

                                    case CURSOR_ACTIVATE: {
                                        UnitInfo* parent = GameManager_SelectedUnit->GetParent();

                                        parent->target_grid_x = GameManager_MousePosition.x;
                                        parent->target_grid_y = GameManager_MousePosition.y;

                                        parent->SetParent(&*GameManager_SelectedUnit);

                                        UnitsManager_SetNewOrder(parent, ORDER_ACTIVATE_ORDER, ORDER_STATE_1);
                                    } break;

                                    case CURSOR_MAP2: {
                                        GameManager_PathBuild(&*GameManager_SelectedUnit);
                                    } break;

                                    case CURSOR_STEAL: {
                                        GameManager_StealUnit(&*GameManager_SelectedUnit, unit2);
                                    } break;

                                    case CURSOR_DISABLE: {
                                        GameManager_DisableUnit(&*GameManager_SelectedUnit, unit2);
                                    } break;
                                }
                            }
                        }
                    }
                }
            } break;

            default: {
                GameManager_DeinitPopupButtons(false);
            } break;
        }
    }
}

void GameManager_MenuDeleteFlic() {
    if (GameManager_Flic) {
        flicsmgr_delete(GameManager_Flic);
        free(GameManager_Flic);
        GameManager_Flic = nullptr;
    }
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
