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

#include <ctime>

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "buildmenu.hpp"
#include "cargomenu.hpp"
#include "chatmenu.hpp"
#include "cursor.hpp"
#include "drawmap.hpp"
#include "flicsmgr.hpp"
#include "gfx.hpp"
#include "hash.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "menulandingsequence.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "movie.hpp"
#include "paths.hpp"
#include "paths_manager.hpp"
#include "production_manager.hpp"
#include "remote.hpp"
#include "reportmenu.hpp"
#include "reportstats.hpp"
#include "researchmenu.hpp"
#include "resource_manager.hpp"
#include "saveloadmenu.hpp"
#include "sound_manager.hpp"
#include "survey.hpp"
#include "task_manager.hpp"
#include "taskdebugger.hpp"
#include "text.hpp"
#include "ticktimer.hpp"
#include "transfermenu.hpp"
#include "units_manager.hpp"
#include "unitstats.hpp"
#include "window_manager.hpp"

#define MENU_QUICK_BUILD_GUI_ITEM_DEF(id1, id2, ulx, uly, width, height, r_value) \
    {(r_value), (id1), (id2), (ulx), (uly), (width), (height), (0), (0)}

#define MENU_QUICK_BUILD_UNIT_SLOTS (6)

#define MENU_GUI_ITEM_DEF(id, gfx, label, button, state, sfx) {(id), (gfx), (label), (button), (state), (sfx)}

#define MENU_DISPLAY_CONTROL_DEF(wid, rid, u1, button, u2, u3, image) \
    {(wid), (rid), (u1), (button), (u2), (u3), (image)}

#define COLOR_CYCLE_DATA(arg1, arg2, arg3, arg4, arg5) {(arg1), (arg2), (arg3), (arg4), (arg5)}

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

#define MENU_DISPLAY_CONTROL_COORDINATES 0
#define MENU_DISPLAY_CONTROL_UNIT_DESCRIPTION 1
#define MENU_DISPLAY_CONTROL_TURN_COUNTER 2
#define MENU_DISPLAY_CONTROL_TURN_TIMER 3
#define MENU_DISPLAY_CONTROL_CORNER_FLIC 4
#define MENU_DISPLAY_CONTROL_STAT_WINDOW 5

struct QuickBuildMenuGuiItem {
    int32_t r_value;
    ResourceID id1;
    ResourceID id2;
    int16_t ulx;
    int16_t uly;
    int16_t width;
    int16_t height;
    uint32_t flags;
    ButtonID bid;
};

struct MenuGuiItem {
    uint8_t wid;
    ResourceID gfx;
    const char* label;
    Button* button;
    bool disabled;
    ResourceID sfx;
};

struct MenuDisplayControl {
    uint8_t window_id;
    ResourceID resource_id;
    void* unknown_1;
    Button* button;
    char unknown_2;
    ResourceID unknown_3;
    Image* image;
};

struct ResourceRange {
    int16_t min;
    int16_t max;

    ResourceRange() : min(0), max(0) {}
    ResourceRange(const ResourceRange& other) : min(other.min), max(other.max) {}
    ResourceRange(int32_t min, int32_t max) : min(min), max(max) {}

    int32_t GetValue() const {
        return (((((max - min + 1) * dos_rand()) >> 15) + min) + ((((max - min + 1) * dos_rand()) >> 15) + min) + 1) /
               2;
    }
};

struct ResourceAllocator {
    Point m_point;
    uint16_t material_type;
    ResourceRange normal;
    ResourceRange concentrate;
    int16_t concentrate_seperation;
    int16_t field_16;
    int16_t concentrate_diffusion;
    IniParameter ini_param;

    ResourceAllocator(uint16_t material_type) : material_type(material_type) {
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

    int32_t GetParameter(IniParameter param) const {
        return ini_config.GetNumericValue(static_cast<IniParameter>(ini_param - INI_RAW_NORMAL_LOW + param));
    }

    static int32_t GetZoneResoureLevel(int32_t grid_x, int32_t grid_y) {
        int32_t result;
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

        for (int32_t i = bounds.ulx; i < bounds.lrx; ++i) {
            for (int32_t j = bounds.uly; j < bounds.lry; ++j) {
                result += ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i] & 0x1F;
            }
        }

        return result;
    }

    static int32_t OptimizeResources(int32_t grid_x, int32_t grid_y, int32_t level_min, int32_t level_max) {
        int32_t zone_resource_level;

        zone_resource_level = GetZoneResoureLevel(grid_x, grid_y);

        if ((zone_resource_level + level_min) > level_max) {
            level_min = level_max - zone_resource_level;
        }

        if (level_min < 0) {
            level_min = 0;
        }

        return level_min;
    }

    void Optimize(Point point, int32_t resource_level, int32_t resource_value) {
        Rect bounds;
        int32_t map_resource_amount;
        int32_t material_value;
        int32_t distance_factor;
        int32_t grid_x;
        int32_t grid_y;

        point.x = ((((point.x + 1) - m_point.x) / 2) * 2) + m_point.x;
        point.y = ((((point.y + 1) - m_point.y) / 2) * 2) + m_point.y;

        if (point.x >= ResourceManager_MapSize.x) {
            point.x = ResourceManager_MapSize.x - 1;
        }

        if (point.y >= ResourceManager_MapSize.y) {
            point.y = ResourceManager_MapSize.y - 1;
        }

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

            if (point.y < ResourceManager_MapSize.y - 1) {
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
                    ((TaskManager_GetDistance(point.x - grid_x, point.y - grid_y) * 10) / concentrate_diffusion) + 10;
                material_value = resource_level / (distance_factor * distance_factor);

                if (material_value > (ResourceManager_CargoMap[grid_y * ResourceManager_MapSize.x + grid_x] & 0x1F)) {
                    ResourceManager_CargoMap[grid_y * ResourceManager_MapSize.x + grid_x] =
                        material_type + material_value;
                }
            }
        }
    }

    void PopulateCargoMap() const {
        for (int32_t i = m_point.x; i < ResourceManager_MapSize.x; i += 2) {
            for (int32_t j = m_point.y; j < ResourceManager_MapSize.y; j += 2) {
                ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i] = material_type + normal.GetValue();
            }
        }
    }

    void ConcentrateResources() {
        int32_t max_resources;
        Point point1;
        Point point2;
        bool flag;

        max_resources = ini_get_setting(INI_MAX_RESOURCES);

        flag = false;

        point1.x = (((concentrate_seperation * 4) / 5 + 1) * dos_rand()) >> 15;
        point1.y = ((concentrate_seperation / 2 + 1) * dos_rand()) >> 15;

        for (int32_t i = point1.x; i < ResourceManager_MapSize.x; i += (concentrate_seperation * 4) / 5) {
            for (int32_t j = flag ? (concentrate_seperation / 2 + point1.y) : point1.y; j < ResourceManager_MapSize.y;
                 j += concentrate_seperation) {
                point2.x = (((field_16 * 2 + 1) * dos_rand()) >> 15) - field_16 + i;
                point2.y = (((field_16 * 2 + 1) * dos_rand()) >> 15) - field_16 + j;

                Optimize(point2, concentrate.GetValue(), max_resources);
            }

            flag = !flag;
        }
    }

    static void SettleMinimumResourceLevels(int32_t min_level) {
        for (int32_t i = 0; i < ResourceManager_MapSize.x; ++i) {
            for (int32_t j = 0; j < ResourceManager_MapSize.y; ++j) {
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
        int32_t max_resources;
        int32_t mixed_resource_seperation;
        int32_t mixed_resource_seperation_min;
        bool flag;

        max_resources = ini_get_setting(INI_MAX_RESOURCES);
        mixed_resource_seperation = ini_get_setting(INI_MIXED_RESOURCE_SEPERATION);
        mixed_resource_seperation_min = mixed_resource_seperation / 5;
        flag = false;

        point1.x = ((((mixed_resource_seperation * 4) / 5) + 1) * dos_rand()) >> 15;
        point1.y = ((mixed_resource_seperation / 2 + 1) * dos_rand()) >> 15;

        for (int32_t i = point1.x; i < ResourceManager_MapSize.x; i += (mixed_resource_seperation * 4) / 5) {
            for (int32_t j = flag ? (mixed_resource_seperation / 2 + point1.y) : point1.y;
                 j < ResourceManager_MapSize.y; j += mixed_resource_seperation) {
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

const char* const GameManager_OrderStatusMessages[] = {
    _(1f51), _(e96b), _(8dac), _(1d99), _(87c5), _(0010), _(2200), _(feff), _(2c8d), _(ce0f), _(dfe7),
    _(0664), _(5ece), _(68b7), _(6553), _(dc38), _(ab01), _(e2c5), _(1469), _(a4b3), _(5e16), _(6172),
    _(7bcd), _(5f83), _(8115), _(e843), _(7508), _(d80d), _(85f8), _(1925), _(bc06), _(c331)};

const char* const GameManager_EventStrings_FinishedBuilding[] = {_(1200), _(74d3), _(8c62)};

const char* const GameManager_EventStrings_FinishedConstruction[] = {_(276c), _(d82e), _(01d0)};

const char* const GameManager_EventStrings_NewSingular[] = {_(c64e), _(8d3e), _(9df3)};

const char* const GameManager_EventStrings_NewPlural[] = {_(5e9c), _(6fe5), _(2488)};

const char* const GameManager_EventStrings_EnemySpotted[] = {_(a2a6), _(7ccc), _(1f7a)};

const char* const GameManager_CheatCodes[CHEAT_CODE_COUNT] = {"[MAXSPY]", "[MAXSURVEY]", "[MAXSTORAGE]", "[MAXAMMO]",
                                                              "[MAXSUPER]"};

const ResourceID GameManager_AlienBuildings[] = {SHIELDGN, SUPRTPLT, RECCENTR};

const ResourceID GameManager_AlienUnits[] = {ALNTANK, ALNASGUN, ALNPLANE};

uint8_t GameManager_MouseButtons;
uint8_t GameManager_ActiveWindow;
int32_t GameManager_MouseX;
int32_t GameManager_MouseY;
Point GameManager_MousePosition;
Point GameManager_LastMousePosition;
Point GameManager_LastMinimapPosition;
bool GameManager_IsActiveMapPositionDisplay;
Point GameManager_MousePosition2;
Point GameManager_ScaledMousePosition;
Rect GameManager_MultiSelectBounds;
UnitInfo* GameManager_Unit;

Rect GameManager_MapView;
Rect GameManager_MapWindowDrawBounds;
SmartPointer<UnitInfo> GameManager_SelectedUnit;
SmartPointer<UnitInfo> GameManager_TempTape;
SmartPointer<UnitInfo> GameManager_QuickBuilderUnit;
SmartPointer<UnitInfo> GameManager_UnknownUnit3;
SmartPointer<UnitInfo> GameManager_UnitUnderMouseCursor;
Image* GameManager_TurnTimerImageNormal;
Image* GameManager_TurnTimerImageScaled;
Point GameManager_MapViewCenter;
Point GameManager_GridCenterOffset;
Point GameManager_SpottedEnemyPosition;
SmartList<UnitInfo> GameManager_LockedUnits;
int32_t GameManager_TurnCounter;
uint32_t GameManager_ArrowKeyFlags;
int32_t GameManager_TurnTimerValue;
uint32_t GameManager_FlicFrameTimeStamp;
bool GameManager_MaxSpy;
int32_t GameManager_GameFileNumber;
int32_t GameManager_HumanPlayerCount;
bool GameManager_RequestMenuExit;
bool GameManager_IsMapInitialized;
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
uint8_t GameManager_RenderState;
bool GameManager_RenderEnable;
bool GameManager_PlayFlic;
bool GameManager_Progress1;
bool GameManager_Progress2;
bool GameManager_MaxSurvey;
bool GameManager_QuickBuildMenuActive;
bool GameManager_IsMainMenuEnabled;
bool GameManager_IsCtrlKeyPressed;
bool GameManager_IsShiftKeyPressed;
bool GameManager_DisplayControlsInitialized;
bool GameManager_RenderFlag1;
bool GameManager_RenderMinimapDisplay;
bool GameManager_RenderFlag2;

ResourceID GameManager_QuickBuildUnitId;
ResourceID GameManager_QuickBuildMenuUnitId;

Button* Gamemanager_FlicButton;

uint8_t GameManager_CheaterTeam;
SmartString GameManager_TextInput;

uint8_t GameManager_PlayerTeam;
uint8_t GameManager_GameState;
uint8_t GameManager_ActiveTurnTeam;
uint8_t GameManager_MarkerColor = 0xFF;
uint8_t GameManager_PlayMode;
bool GameManager_FastMovement;
uint16_t GameManager_MultiChatTargets[PLAYER_TEAM_MAX - 1];

uint32_t GameManager_LastZoomLevel;

static Point GameManager_GridOffset;
static int32_t GameManager_QuickScroll;
static float GameManager_GridStepLevel;
static int32_t GameManager_GridStepOffset;

MenuLandingSequence GameManager_LandingSequence;

struct ColorCycleData {
    uint16_t start_index;
    uint16_t end_index;
    uint8_t rotate_direction;
    uint32_t time_limit;
    uint32_t time_stamp;
};

struct MenuFlic {
    WindowInfo sw;
    WindowInfo dw;
    Flic* flc;
    int32_t text_height;
};

static struct ColorCycleData GameManager_ColorCycleTable[] = {
    COLOR_CYCLE_DATA(9, 12, 0, TIMER_FPS_TO_MS(9), 0),     COLOR_CYCLE_DATA(13, 16, 1, TIMER_FPS_TO_MS(6), 0),
    COLOR_CYCLE_DATA(17, 20, 1, TIMER_FPS_TO_MS(9), 0),    COLOR_CYCLE_DATA(21, 24, 1, TIMER_FPS_TO_MS(6), 0),
    COLOR_CYCLE_DATA(25, 30, 1, TIMER_FPS_TO_MS(2), 0),    COLOR_CYCLE_DATA(31, 31, 1, TIMER_FPS_TO_MS(6), 0),
    COLOR_CYCLE_DATA(96, 102, 1, TIMER_FPS_TO_MS(8), 0),   COLOR_CYCLE_DATA(103, 109, 1, TIMER_FPS_TO_MS(8), 0),
    COLOR_CYCLE_DATA(110, 116, 1, TIMER_FPS_TO_MS(10), 0), COLOR_CYCLE_DATA(117, 122, 1, TIMER_FPS_TO_MS(6), 0),
    COLOR_CYCLE_DATA(123, 127, 1, TIMER_FPS_TO_MS(6), 0),
};

static struct QuickBuildMenuGuiItem GameManager_QuickBuildMenuItems[] = {
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 8, 7, 128, 128, 1003),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 145, 7, 128, 128, 1004),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 280, 7, 128, 128, 1005),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 8, 167, 128, 128, 1006),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 145, 167, 128, 128, 1007),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 280, 167, 128, 128, 1008),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(QWKDN_OF, QWKDN_ON, 323, 316, 24, 24, 1002),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(QWKUP_OF, QWKUP_ON, 355, 316, 24, 24, 1001),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(QWKCN_OF, QWKCN_ON, 387, 316, 24, 24, 1000),
};

static struct MenuGuiItem GameManager_MenuItems[] = {
    MENU_GUI_ITEM_DEF(WINDOW_FILES_BUTTON, FILES_OF, _(abc7), nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_PREFS_BUTTON, PREF_OFF, _(6696), nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_PLAY_BUTTON, PLAY_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_PAUSE_BUTTON, PAUSE_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_CENTER_BUTTON, FIND_OFF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_PRE_BUTTON, PREV_OF, _(7391), nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_DONE_BUTTON, UDONE_OF, _(e2e9), nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_NXT_BUTTON, NEXT_OF, _(1ff5), nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_HELP_BUTTON, HELP_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_REPORTS_BUTTON, REPT_OFF, _(dacc), nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_CHAT_BUTTON, CHAT_OFF, _(460b), nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_SURVEY_BUTTON, SURV_OFF, _(211b), nullptr, false, ISURV0),
    MENU_GUI_ITEM_DEF(WINDOW_STATUS_BUTTON, STAT_OFF, _(920e), nullptr, false, ISTAT0),
    MENU_GUI_ITEM_DEF(WINDOW_COLORS_BUTTON, COLOR_OF, _(88c4), nullptr, false, ICOLO0),
    MENU_GUI_ITEM_DEF(WINDOW_HITS_BUTTON, HITS_OF, _(5995), nullptr, false, IHITS0),
    MENU_GUI_ITEM_DEF(WINDOW_AMMO_BUTTON, AMMO_OF, _(7429), nullptr, false, IAMMO0),
    MENU_GUI_ITEM_DEF(WINDOW_RANGE_BUTTON, RANG_OFF, _(5de7), nullptr, false, IRANG0),
    MENU_GUI_ITEM_DEF(WINDOW_SCAN_BUTTON, VISN_OFF, _(ea1c), nullptr, false, IVISI0),
    MENU_GUI_ITEM_DEF(WINDOW_GRID_BUTTON, GRID_OFF, _(0d7d), nullptr, false, IGRID0),
    MENU_GUI_ITEM_DEF(WINDOW_NAME_BUTTON, NAMES_UP, _(c3b2), nullptr, false, INAME0),
    MENU_GUI_ITEM_DEF(WINDOW_LOCK_BUTTON, LOCK_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_2X_MINIMAP, MIN2X_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_TNT_MINIMAP, MINFL_OF, nullptr, nullptr, false, MENUOP),
    MENU_GUI_ITEM_DEF(WINDOW_ENDTURN_BUTTON, ENDTRN_U, _(6af6), nullptr, false, MENUOP),
};

static Point GameManager_MenuItemLabelOffsets[] = {
    {0, 2}, {0, 2}, {0, 0}, {0, 0}, {0, 0}, {-10, 2}, {0, 3}, {10, 2}, {0, 0}, {0, 3}, {0, 3}, {2, 2},
    {2, 2}, {2, 4}, {2, 2}, {2, 2}, {2, 4}, {2, 2},   {2, 2}, {2, 4},  {0, 0}, {0, 0}, {0, 0}, {0, 0},
};

static struct MenuDisplayControl GameManager_MenuDisplayControls[] = {
    MENU_DISPLAY_CONTROL_DEF(WINDOW_COORDINATES_DISPLAY, XYPOS, 0, 0, 0, INVALID_ID, nullptr),
    MENU_DISPLAY_CONTROL_DEF(WINDOW_UNIT_DESCRIPTION_DISPLAY, UNITNAME, 0, 0, 0, INVALID_ID, nullptr),
    MENU_DISPLAY_CONTROL_DEF(WINDOW_TURN_COUNTER_DISPLAY, TURNS, 0, 0, 0, INVALID_ID, nullptr),
    MENU_DISPLAY_CONTROL_DEF(WINDOW_TURN_TIMER_DISPLAY, TIMER, 0, 0, 0, INVALID_ID, nullptr),
    MENU_DISPLAY_CONTROL_DEF(WINDOW_CORNER_FLIC, INVALID_ID, 0, 0, 0, INVALID_ID, nullptr),
    MENU_DISPLAY_CONTROL_DEF(WINDOW_STAT_WINDOW, INVALID_ID, 0, 0, 0, INVALID_ID, nullptr),
};

struct PopupButtons GameManager_PopupButtons;

static uint32_t GameManager_NotifyTimeout;
static struct MenuFlic GameManager_Flic;
static TextEdit* GameManager_TextEditUnitName;
static char GameManager_UnitName[30];
static uint8_t GameManager_ColorCycleStep;
static bool GameManager_UpdateFlag;
static uint8_t GameManager_MarkerColorUpdatePeriod = 5;
static bool GameManager_IsSurveyorSelected;

static uint32_t GameManager_ScrollTimeStamp;
static float GameManager_ScrollRateLimit = 1.0f;
static float GameManager_ScrollRate;

static bool GameManager_PlayerMissionSetup(uint16_t team);
static void GameManager_DrawSelectSiteMessage(uint16_t team);
static bool GameManager_SelectSite(uint16_t team);
static void GameManager_InitMap();
static void GameManager_GameSetup(int32_t game_state);
static void GameManager_GameLoopCleanup();
static uint16_t GameManager_EvaluateWinner();
static void GameManager_AnnounceWinner(uint16_t team);
static void GameManager_DrawTurnCounter(int32_t turn_count);
static void GameManager_DrawTimer(char* text, int32_t color);
static bool GameManager_ProcessTextInput(int32_t key);
static bool GameManager_DebugDelayedEndTurn(SmartList<UnitInfo>& units);
static void GameManager_ProcessKey();
static int32_t GameManager_GetBuilderUnitCursor(UnitInfo* unit1, int32_t grid_x, int32_t grid_y, UnitInfo* unit2);
static int32_t GameManager_GetAirUnitCursor(UnitInfo* unit1, int32_t grid_x, int32_t grid_y, UnitInfo* unit2);
static bool GameManager_IsUnitNotInAir(UnitInfo* unit);
static UnitInfo* GameManager_GetUnitWithCargoType(Complex* complex, int32_t cargo_type);
static int32_t GameManager_GetUnitActionCursor(UnitInfo* unit1, int32_t grid_x, int32_t grid_y, UnitInfo* unit2);
static bool GameManager_IsValidTransferTarget(UnitInfo* unit1, UnitInfo* unit2);
static void GameManager_SetUnitOrder(const UnitOrderType, const UnitOrderStateType state, UnitInfo* unit,
                                     int32_t grid_x, int32_t grid_y);
static bool GameManager_IsValidStealTarget(UnitInfo* unit1, UnitInfo* unit2);
static bool GameManager_IsValidDisableTarget(UnitInfo* unit1, UnitInfo* unit2);
static int32_t GameManager_GetMilitaryCursor(UnitInfo* unit, int32_t grid_x, int32_t grid_y);
static void GameManager_UnitSelectOther(UnitInfo* unit1, UnitInfo* unit2, int32_t grid_x, int32_t grid_y);
static void GameManager_UnitSelect(UnitInfo* unit);
static void GameManager_ClickUnit(UnitInfo* unit1, UnitInfo* unit2, int32_t grid_x, int32_t grid_y);
static bool GameManager_UpdateSelection(UnitInfo* unit1, UnitInfo* unit2, int32_t grid_x, int32_t grid_y);
static void GameManager_SetGridOffset(int32_t grid_x_offset, int32_t grid_y_offset);
static void GameManager_ProcessInput();
static bool GameManager_CargoSelection(uint16_t team);
static void GameManager_UpdateTurnTimer(bool mode, int32_t turn_time);
static void GameManager_DrawDisplayPanel(int32_t control_id, const char* text, int32_t color, int32_t ulx = 0);
static void GameManager_ProgressBuildState(uint16_t team);
static void GameManager_UpdateGuiControl(uint16_t team);
static uint16_t GameManager_GetCrc16(uint16_t data, uint16_t crc_checksum);
static uint16_t GameManager_GetUnitListChecksum(SmartList<UnitInfo>* units, uint16_t team, uint16_t crc_checksum);
static bool GameManager_CheckDesync();
static void GameManager_UpdateGui(uint16_t team, int32_t game_state, bool enable_autosave);
static bool GameManager_AreTeamsFinishedTurn();
static void GameManager_ProgressTurn();
static void GameManager_ResetRenderState();
static bool GameManager_ProcessPopupMenuInput(int32_t key);
static void GameManager_PunishCheater();
static void GameManager_ProcessCheatCodes();
static void GameManager_AnnounceWinner(uint16_t team);
static void GameManager_UpdateScoreGraph();
static void GameManager_InitUnitsAndGameState();
static bool GameManager_InitGame();
static void GameManager_UpdateHumanPlayerCount();
static void GameManager_MenuAnimateDisplayControls();
static void GameManager_ManagePlayerAction();
static bool GameManager_InitPopupButtons(UnitInfo* unit);
static void GameManager_UpdateGridCenterOffset();
static void GameManager_UpdatePanelButtons(uint16_t team);
static bool GameManager_ScrollMainMapView(int32_t& offset_x, int32_t& offset_y);
static bool GameManager_CenterMainMapView(int32_t ulx, int32_t uly);
static void GameManager_UpdateZoomSlider();
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
static void GameManager_MenuClickFileButton(bool is_saving_allowed);
static void GameManager_MenuClickGridButton(bool rest_state);
static void GameManager_MenuClickEndTurnButton(bool state);
static void GameManager_MenuClickSurveyButton(bool rest_state);

static void GameManager_SaveLoadGame(bool save_load_mode);
static void GameManager_MenuInitDisplayControls();
static void GameManager_DrawMouseCoordinates(int32_t x, int32_t y);
static bool GameManager_HandleProximityOverlaps();
static bool GameManager_IsValidStartingPosition(int32_t grid_x, int32_t grid_y);
static uint8_t GameManager_GetWindowCursor(int32_t grid_x, int32_t grid_y);
static void GameManager_PathBuild(UnitInfo* unit);
static void GameManager_ReloadUnit(UnitInfo* unit1, UnitInfo* unit2);
static void GameManager_RepairUnit(UnitInfo* unit1, UnitInfo* unit2);
static void GameManager_TransferCargo(UnitInfo* unit1, UnitInfo* unit2);
static void GameManager_StealUnit(UnitInfo* unit1, UnitInfo* unit2);
static void GameManager_DisableUnit(UnitInfo* unit1, UnitInfo* unit2);
static void GameManager_FindValidStartingPosition(Point* position);
static void GameManager_PopulateMapWithResources();
static void GameManager_FindSpot(Point* point);
static void GameManager_SpawnAlienDerelicts(Point point, int32_t alien_unit_value);
static void GameManager_PopulateMapWithAlienUnits(int32_t alien_seperation, int32_t alien_unit_value);
static void GameManager_ProcessTeamMissionSupplyUnits(uint16_t team);
static void GameManager_FlicButtonRFunction(ButtonID bid, intptr_t value);
static void GameManager_DrawBuilderUnitStatusMessage(UnitInfo* unit);
static void GameManager_DrawDisabledUnitStatusMessage(UnitInfo* unit);
static void GameManager_PlayUnitStatusVoice(UnitInfo* unit);
static void GameManager_DrawUnitStatusMessage(UnitInfo* unit);
static void GameManager_MenuDeinitDisplayControls();
static void GameManager_SpawnNewUnits(uint16_t team, SmartList<UnitInfo>* units, uint16_t* counts);
static char* GameManager_PrependMessageChunk(const char* chunk, char* message);
static void GameManager_ReportNewUnitsMessage(uint16_t* counts);
static void GameManager_DrawProximityZones();
static int32_t GameManager_UpdateProximityState(uint16_t team);
static UnitInfo* GameManager_GetFirstRelevantUnit(uint16_t team);
static void GameManager_ColorEffect(struct ColorCycleData* color_cycle_table);
static bool GameManager_IsUnitNextToPosition(UnitInfo* unit, int32_t grid_x, int32_t grid_y);
static bool GameManager_IsInteractable(UnitInfo* unit);
static void GameManager_DrawInfoDisplayRowIcons(uint8_t* buffer, int32_t full_width, int32_t width, int32_t height,
                                                ResourceID icon, int32_t current_value, int32_t base_value);
static void GameManager_DrawInfoDisplayRow(const char* label, int32_t window_id, ResourceID icon, int32_t current_value,
                                           int32_t base_value, int32_t factor);
static void GameManager_DrawInfoDisplayType2(UnitInfo* unit);
static void GameManager_DrawInfoDisplayType1(UnitInfo* unit);
static void GameManager_DrawInfoDisplayType3(UnitInfo* unit);
static bool GameManager_SyncTurnTimer();
static void GameManager_MenuCreateFlic(ResourceID unit_type, int32_t ulx, int32_t uly);
static void GameManager_DrawFlic(Rect* bounds);
static void GameManager_AdvanceFlic();
static void GameManager_DrawCircle(UnitInfo* unit, WindowInfo* window, int32_t radius, int32_t color);
static void GameManager_RenderMap();
static void GameManager_UpdateProductions(uint16_t team, SmartList<UnitInfo>* units);
static void GameManager_ResupplyUnits(uint16_t team, SmartList<UnitInfo>* units);
static void GameManager_ManageEconomy(uint16_t team);
static Point GameManager_GetStartPositionMiningStation(uint16_t team);
static Point GameManager_GetStartingPositionPowerGenerator(Point point, uint16_t team);
static float GameManager_GetScrollRateLimit();
static float GameManager_UpdateScrollRateLimit();
static Point GameManager_GetMinimapPosition();
static void GameManager_TeamTurnTurnBased(int32_t& game_state);
static void GameManager_TeamTurnConcurrent(int32_t& game_state);
static bool GameManager_TeamTurnFinish(uint32_t turn_counter_session_start, uint16_t team_winner);
static void GameManager_RenderScanRangeIndicators();
static void GameManager_RenderSurveyIndicator(DrawMapBuffer* drawmap);
static void GameManager_RenderMultiSelectIndicator();
static void GameManager_RenderMinimap();
static void GameManager_QuickBuildMenuDrawPortraits(WindowInfo* window, ResourceID id1, ResourceID id2, int32_t width);
static void GameManager_QuickBuildMenu();

void GameManager_TeamTurnTurnBased(int32_t& game_state) {
    bool enable_autosave;

    AiLog log("Turn based team turn.");

    enable_autosave = true;

    for (uint8_t team = (game_state == GAME_STATE_10) ? GameManager_ActiveTurnTeam : PLAYER_TEAM_RED;
         team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
            UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED) {
            if (game_state != GAME_STATE_10) {
                GameManager_ManageEconomy(team);
            }

            if (GameManager_DemoMode) {
                Access_UpdateMinimapFogOfWar(team, GameManager_AllVisible);

                GameManager_PlayerTeam = team;
            }

            UnitsManager_TeamInfo[team].finished_turn = false;

            GameManager_UpdateGui(team, game_state, enable_autosave);

            if (GameManager_GameState == GAME_STATE_10) {
                game_state = GAME_STATE_10;
                GameManager_GameState = GAME_STATE_8_IN_GAME;
                break;
            }

            if (GameManager_GameState != GAME_STATE_9_END_TURN) {
                break;
            }

            GameManager_ResetRenderState();

            while (Access_AreTaskEventsPending()) {
                GameManager_ProgressTurn();
            }

            GameManager_GuiSwitchTeam(GameManager_PlayerTeam);

            game_state = GAME_STATE_8_IN_GAME;
            enable_autosave = false;
        }
    }
}

void GameManager_TeamTurnConcurrent(int32_t& game_state) {
    AiLog log("Concurrent team turn.");

    if (Remote_IsNetworkGame) {
        Remote_WaitBeginTurnAcknowledge();
    }

    for (uint8_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (game_state != GAME_STATE_10 && GameManager_PlayerTeam != team &&
            (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER ||
             UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_REMOTE)) {
            GameManager_ActiveTurnTeam = team;

            GameManager_ProgressBuildState(GameManager_ActiveTurnTeam);
        }
    }

    for (uint8_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (team != GameManager_PlayerTeam && UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
            Ai_BeginTurn(team);
        }
    }

    GameManager_UpdateGui(GameManager_PlayerTeam, game_state, true);

    log.Log("Waiting for all teams to end turn.");

    GameManager_EnableMainMenu(nullptr);

    if (GameManager_GameState == GAME_STATE_9_END_TURN && !GameManager_AreTeamsFinishedTurn()) {
        if (Remote_IsNetworkGame) {
            MessageManager_DrawMessage(_(758a), 0, 0);

        } else {
            MessageManager_DrawMessage(_(c977), 0, 0);
        }

        while (GameManager_GameState == GAME_STATE_9_END_TURN && !GameManager_AreTeamsFinishedTurn()) {
            GameManager_ProgressTurn();
        }
    }

    if (GameManager_GameState == GAME_STATE_9_END_TURN) {
        GameManager_PlayMode = PLAY_MODE_UNKNOWN;

        GameManager_ResetRenderState();

        log.Log("Waiting for units to finish moving.  Game status = %i.", GameManager_GameState);

        while (GameManager_GameState == GAME_STATE_9_END_TURN && Access_AreTaskEventsPending()) {
            GameManager_ProgressTurn();
        }

        if (Remote_IsNetworkGame) {
            if (ini_get_setting(INI_LOG_FILE_DEBUG)) {
                MessageManager_DrawMessage(_(0d6b), 0, 0);
                Remote_AnalyzeDesync();
            }
        }
    }

    log.Log("Getting ready for next turn.  Game status = %i.", GameManager_GameState);

    GameManager_PlayMode = PLAY_MODE_SIMULTANEOUS_MOVES;

    GameManager_GuiSwitchTeam(GameManager_PlayerTeam);

    if (GameManager_GameState == GAME_STATE_10) {
        game_state = GAME_STATE_10;
        GameManager_GameState = GAME_STATE_8_IN_GAME;

    } else if (GameManager_GameState == GAME_STATE_9_END_TURN) {
        log.Log("Resetting units.");

        for (uint8_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].finished_turn) {
                GameManager_ManageEconomy(team);
            }
        }

        game_state = GAME_STATE_8_IN_GAME;
    }
}

bool GameManager_TeamTurnFinish(uint32_t turn_counter_session_start, uint16_t team_winner) {
    bool result{false};

    if (GameManager_GameState == GAME_STATE_9_END_TURN) {
        AiLog log("End turn %i.", GameManager_TurnCounter);

        GameManager_GameState = GAME_STATE_8_IN_GAME;

        log.Log("Checking victory conditions.");

        if (menu_check_end_game_conditions(GameManager_TurnCounter, turn_counter_session_start, GameManager_DemoMode)) {
            result = true;

        } else {
            for (uint8_t team = 0; team < PLAYER_TEAM_MAX - 1; ++team) {
                UnitsManager_TeamInfo[team].finished_turn = false;
            }

            GameManager_UpdateScoreGraph();

            ++GameManager_TurnCounter;

            GameManager_AnnounceWinner(team_winner);
        }
    }

    return result;
}

void GameManager_GameLoop(int32_t game_state) {
    uint32_t turn_counter_session_start;
    uint16_t team_winner;

    if (game_state == GAME_STATE_10 && ini_get_setting(INI_GAME_FILE_TYPE) == GAME_TYPE_DEMO) {
        GameManager_DemoMode = true;

    } else {
        GameManager_DemoMode = false;
    }

    AiLog log("Main game loop start.");

    GameManager_GameSetup(game_state);

    turn_counter_session_start = GameManager_TurnCounter;

    while (GameManager_GameState == GAME_STATE_8_IN_GAME) {
        team_winner = GameManager_EvaluateWinner();
        GameManager_DrawTurnCounter(GameManager_TurnCounter);

        if (GameManager_PlayMode == PLAY_MODE_SIMULTANEOUS_MOVES) {
            GameManager_TeamTurnConcurrent(game_state);

        } else {
            GameManager_TeamTurnTurnBased(game_state);
        }

        if (GameManager_TeamTurnFinish(turn_counter_session_start, team_winner)) {
            break;
        }
    }

    log.Log("Main game loop end.");

    GameManager_GameLoopCleanup();

    if (GameManager_GameState == GAME_STATE_15_FATAL_ERROR) {
        ResourceManager_ExitGame(EXIT_CODE_THANKS);
    }

    SoundManager_PlayMusic(MAIN_MSC, false);
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

void GameManager_DeployUnit(uint16_t team, ResourceID unit_type, int32_t grid_x, int32_t grid_y) {
    SmartPointer<UnitInfo> unit;
    uint32_t flags;
    ResourceID slab_type;

    unit = UnitsManager_DeployUnit(unit_type, team, nullptr, grid_x, grid_y, 0);

    flags = unit->flags;

    if ((flags & REQUIRES_SLAB) && Access_IsAnyLandPresent(grid_x, grid_y, flags)) {
        if (flags & BUILDING) {
            slab_type = LRGSLAB;

        } else {
            slab_type = SMLSLAB;
        }

        UnitsManager_DeployUnit(
            slab_type, team, nullptr, grid_x, grid_y,
            (dos_rand() *
             reinterpret_cast<struct BaseUnitDataFile*>(UnitsManager_BaseUnits[slab_type].data_buffer)->image_count) >>
                15);
    }

    if (unit_type == MININGST) {
        UnitsManager_SetInitialMining(&*unit, grid_x, grid_y);
    }

    if (flags & (CONNECTOR_UNIT | BUILDING | STANDALONE)) {
        UnitsManager_UpdateConnectors(&*unit);
    }
}

void GameManager_DrawUnitSelector(uint8_t* buffer, int32_t pitch, int32_t offsetx, int32_t height, int32_t offsety,
                                  int32_t bottom, int32_t item_height, int32_t top, int32_t scaling_factor,
                                  int32_t is_big_sprite, bool double_marker) {
    uint8_t color;
    int32_t loop_count;
    int32_t scaled_size2;
    int32_t size2;
    int32_t scaled_size1;
    int32_t size1;

    if (is_big_sprite) {
        size1 = 32;
    } else {
        size1 = 16;
    }

    scaled_size1 = (size1 * GFX_SCALE_DENOMINATOR) / scaling_factor;

    if (is_big_sprite) {
        size2 = 128;
    } else {
        size2 = 64;
    }

    scaled_size2 = (size2 * GFX_SCALE_DENOMINATOR) / scaling_factor - 2;

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

    for (int32_t i = 0; i < loop_count; ++i) {
        if (height >= bottom && height <= top && scaled_size1 + offsetx >= offsety && offsetx <= item_height) {
            draw_line(buffer, pitch, std::max(offsetx, offsety), height, std::min(scaled_size1 + offsetx, item_height),
                      height, color);
        }

        if (offsetx >= offsety && offsetx <= item_height && scaled_size1 + height >= bottom && height <= top) {
            draw_line(buffer, pitch, offsetx, std::max(height, bottom), offsetx, std::min(scaled_size1 + height, top),
                      color);
        }

        offsetx += scaled_size2;

        if (height >= bottom && height <= top && offsetx >= offsety && offsetx - scaled_size1 <= item_height) {
            draw_line(buffer, pitch, std::max(offsetx - scaled_size1, offsety), height, std::min(offsetx, item_height),
                      height, color);
        }

        if (offsetx >= offsety && offsetx <= item_height && scaled_size1 + height >= bottom && height <= top) {
            draw_line(buffer, pitch, offsetx, std::max(height, bottom), offsetx, std::min(scaled_size1 + height, top),
                      color);
        }

        height += scaled_size2;

        if (height >= bottom && height <= top && offsetx >= offsety && offsetx - scaled_size1 <= item_height) {
            draw_line(buffer, pitch, std::max(offsetx - scaled_size1, offsety), height, std::min(offsetx, item_height),
                      height, color);
        }

        if (offsetx >= offsety && offsetx <= item_height && height >= bottom && height - scaled_size1 <= top) {
            draw_line(buffer, pitch, offsetx, std::min(height, top), offsetx, std::max(height - scaled_size1, bottom),
                      color);
        }

        offsetx -= scaled_size2;

        if (height >= bottom && height <= top && scaled_size1 + offsetx >= offsety && offsetx <= item_height) {
            draw_line(buffer, pitch, std::max(offsetx, offsety), height, std::min(scaled_size1 + offsetx, item_height),
                      height, color);
        }

        if (offsetx >= offsety && offsetx <= item_height && height >= bottom && height - scaled_size1 <= top) {
            draw_line(buffer, pitch, offsetx, std::min(height, top), offsetx, std::max(height - scaled_size1, bottom),
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

bool GameManager_RefreshOrders(uint16_t team, bool check_production) {
    CTInfo* team_info;
    bool is_player;
    char message[200];

    team_info = &UnitsManager_TeamInfo[team];

    is_player = team_info->team_type == TEAM_TYPE_PLAYER;

    SDL_assert(team_info->team_type != TEAM_TYPE_REMOTE);

    const auto& complexes = team_info->team_units->GetComplexes();

    for (SmartList<Complex>::Iterator it = complexes.Begin(); it != complexes.End(); ++it) {
        Access_UpdateResourcesTotal(&(*it));

        if (!GameManager_OptimizeProduction(team, &(*it), is_player, true) && !check_production) {
            return false;
        }

        if (ProductionManager_UpdateIndustryOrders(team, &(*it)) && is_player) {
            sprintf(message, _(a9fd), (*it).GetId());

            MessageManager_DrawMessage(message, 1, 0, false, true);
        }
    }

    Access_RenewAttackOrders(UnitsManager_MobileLandSeaUnits, team);
    Access_RenewAttackOrders(UnitsManager_MobileAirUnits, team);

    UnitsManager_TeamInfo[team].finished_turn = true;

    GameManager_ProcessTick(false);

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_Signal(REMOTE_PACKET_06, team, 0);
    }

    GameManager_HandleTurnTimer();

    if (GameManager_PlayerTeam == team || !GameManager_PlayMode) {
        GameManager_GameState = GAME_STATE_9_END_TURN;

        GameManager_MenuClickEndTurnButton(false);
    }

    return true;
}

void GameManager_HandleTurnTimer() {
    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED) {
        int32_t timer_setting;
        bool flag;

        timer_setting = ini_get_setting(INI_ENDTURN);
        flag = false;

        if (timer_setting < 15) {
            for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER &&
                    !UnitsManager_TeamInfo[team].finished_turn) {
                    timer_setting = 15;
                    flag = true;
                }
            }
        }

        if (timer_setting > 0 && !GameManager_RequestMenuExit) {
            if (GameManager_TurnTimerValue && !flag) {
                timer_setting = std::min(timer_setting, GameManager_TurnTimerValue);
            }

            GameManager_UpdateTurnTimer(true, timer_setting);
        }
    }
}

void GameManager_GetScaledMessageBoxBounds(Rect* bounds) {
    WindowInfo* window_message_box;

    window_message_box = WindowManager_GetWindow(WINDOW_MESSAGE_BOX);

    bounds->ulx = GameManager_MapWindowDrawBounds.ulx;
    bounds->uly = ((Gfx_MapScalingFactor * 10) / GFX_SCALE_DENOMINATOR) + GameManager_MapWindowDrawBounds.uly;
    bounds->lrx = (((window_message_box->window.lrx - window_message_box->window.ulx + 1) * Gfx_MapScalingFactor) /
                   GFX_SCALE_DENOMINATOR) +
                  bounds->ulx;
    bounds->lry = (((window_message_box->window.lry - window_message_box->window.uly + 1) * Gfx_MapScalingFactor) /
                   GFX_SCALE_DENOMINATOR) +
                  bounds->uly;
}

void GameManager_RenderScanRangeIndicators() {
    WindowInfo* window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    if (GameManager_SelectedUnit != nullptr) {
        if (GameManager_SelectedUnit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER ||
            GameManager_SelectedUnit->GetOrderState() == ORDER_STATE_MOVE_GETTING_PATH ||
            GameManager_SelectedUnit->GetOrderState() == ORDER_STATE_READY_TO_EXECUTE_ORDER ||
            GameManager_SelectedUnit->GetOrder() == ORDER_BUILD) {
            if (GameManager_SelectedUnit->IsVisibleToTeam(GameManager_PlayerTeam)) {
                SmartPointer<UnitValues> unit_values(GameManager_SelectedUnit->GetBaseValues());
                UnitInfo* unit = &*GameManager_SelectedUnit;
                if (unit->team == GameManager_PlayerTeam && unit->path != nullptr) {
                    unit->path->Draw(unit, window);
                }
                if (GameManager_DisplayButtonRange) {
                    int32_t color;
                    if (Access_GetValidAttackTargetTypes(unit->GetUnitType()) & MOBILE_AIR_UNIT) {
                        color = COLOR_CHROME_YELLOW;
                    } else {
                        color = COLOR_RED;
                    }
                    GameManager_DrawCircle(unit, window, unit_values->GetAttribute(ATTRIB_RANGE), color);
                }
                if (GameManager_DisplayButtonScan) {
                    GameManager_DrawCircle(unit, window, unit_values->GetAttribute(ATTRIB_SCAN), COLOR_YELLOW);
                }
            }
        }
        if (GameManager_DisplayButtonLock && (GameManager_DisplayButtonRange || GameManager_DisplayButtonScan) &&
            GameManager_LockedUnits.GetCount()) {
            SmartPointer<UnitValues> unit_values;
            for (SmartList<UnitInfo>::Iterator it = GameManager_LockedUnits.Begin();
                 it != GameManager_LockedUnits.End(); ++it) {
                if ((*it).team != GameManager_PlayerTeam &&
                    ((*it).GetOrderState() == ORDER_STATE_EXECUTING_ORDER ||
                     (*it).GetOrderState() == ORDER_STATE_READY_TO_EXECUTE_ORDER)) {
                    if ((*it).IsVisibleToTeam(GameManager_PlayerTeam)) {
                        unit_values = (*it).GetBaseValues();
                        if (GameManager_DisplayButtonRange) {
                            int32_t color;
                            if (Access_GetValidAttackTargetTypes((*it).GetUnitType()) & MOBILE_AIR_UNIT) {
                                color = COLOR_CHROME_YELLOW;
                            } else {
                                color = COLOR_RED;
                            }
                            GameManager_DrawCircle(&*it, window, unit_values->GetAttribute(ATTRIB_RANGE), color);
                        }
                        if (GameManager_DisplayButtonScan) {
                            GameManager_DrawCircle(&*it, window, unit_values->GetAttribute(ATTRIB_SCAN), COLOR_YELLOW);
                        }
                    }
                }
            }
        }
    }
}

void GameManager_RenderSurveyIndicator(DrawMapBuffer* drawmap) {
    if (GameManager_DisplayButtonSurvey || GameManager_IsSurveyorSelected) {
        DrawMap_RenderSurveyDisplay(drawmap);
    }
}

void GameManager_RenderMultiSelectIndicator() {
    WindowInfo* window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    int32_t map_ulx{(GameManager_MultiSelectBounds.ulx * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor -
                    Gfx_MapWindowUlx};
    int32_t map_uly{(GameManager_MultiSelectBounds.uly * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor -
                    Gfx_MapWindowUly};
    int32_t map_lrx{(GameManager_MultiSelectBounds.lrx * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor -
                    Gfx_MapWindowUlx};
    int32_t map_lry{(GameManager_MultiSelectBounds.lry * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor -
                    Gfx_MapWindowUly};

    if (GameManager_MultiSelectBounds.ulx <= GameManager_MapWindowDrawBounds.lrx &&
        GameManager_MultiSelectBounds.lrx >= GameManager_MapWindowDrawBounds.ulx &&
        GameManager_MultiSelectBounds.uly <= GameManager_MapWindowDrawBounds.lry &&
        GameManager_MultiSelectBounds.lry >= GameManager_MapWindowDrawBounds.uly) {
        int32_t map_ulx2{std::max(map_ulx, 0)};
        int32_t map_uly2{std::max(map_uly, 0)};
        int32_t map_lrx2{std::min(map_lrx, WindowManager_MapWidth - 1)};
        int32_t map_lry2{std::min(map_lry, WindowManager_MapHeight - 1)};

        if (GameManager_MultiSelectBounds.uly >= GameManager_MapWindowDrawBounds.uly &&
            GameManager_MultiSelectBounds.uly <= GameManager_MapWindowDrawBounds.lry) {
            draw_line(window->buffer, window->width, map_ulx2, map_uly, map_lrx2, map_uly, COLOR_YELLOW);
        }
        if (GameManager_MultiSelectBounds.lrx >= GameManager_MapWindowDrawBounds.ulx &&
            GameManager_MultiSelectBounds.lrx <= GameManager_MapWindowDrawBounds.lrx) {
            draw_line(window->buffer, window->width, map_lrx, map_uly2, map_lrx, map_lry2, COLOR_YELLOW);
        }
        if (GameManager_MultiSelectBounds.lry >= GameManager_MapWindowDrawBounds.uly &&
            GameManager_MultiSelectBounds.lry <= GameManager_MapWindowDrawBounds.lry) {
            draw_line(window->buffer, window->width, map_ulx2, map_lry, map_lrx2, map_lry, COLOR_YELLOW);
        }
        if (GameManager_MultiSelectBounds.ulx >= GameManager_MapWindowDrawBounds.ulx &&
            GameManager_MultiSelectBounds.ulx <= GameManager_MapWindowDrawBounds.lrx) {
            draw_line(window->buffer, window->width, map_ulx, map_uly2, map_ulx, map_lry2, COLOR_YELLOW);
        }
    }
}

void GameManager_RenderMinimap() {
    if (GameManager_RenderMinimapDisplay) {
        WindowInfo* mmw = WindowManager_GetWindow(WINDOW_MINIMAP);
        const int32_t mmw_width{mmw->window.lrx - mmw->window.ulx + 1};
        const int32_t mmw_height{mmw->window.lry - mmw->window.uly + 1};
        Point map_size{ResourceManager_MapSize};
        const int32_t map_view_width{GameManager_MapView.lrx - GameManager_MapView.ulx + 1};
        const int32_t map_view_height{GameManager_MapView.lry - GameManager_MapView.uly + 1};

        buf_to_buf(ResourceManager_MinimapBgImage, mmw_width, mmw_height, mmw_width, mmw->buffer, mmw->width);

        if (map_size.x == ResourceManager_MinimapWindowSize.x && map_size.y == ResourceManager_MinimapWindowSize.y) {
            buf_to_buf(ResourceManager_MinimapUnits, map_size.x, map_size.y, map_size.x, mmw->buffer, mmw->width);

        } else {
            cscale(ResourceManager_MinimapUnits, map_size.x, map_size.y, map_size.x,
                   &mmw->buffer[ResourceManager_MinimapWindowOffset.y * mmw->width +
                                ResourceManager_MinimapWindowOffset.x],
                   mmw_width - ResourceManager_MinimapWindowOffset.x * 2,
                   mmw_height - ResourceManager_MinimapWindowOffset.y * 2, mmw->width);
        }

        draw_box(
            &mmw->buffer[ResourceManager_MinimapWindowOffset.y * mmw->width + ResourceManager_MinimapWindowOffset.x],
            mmw->width, GameManager_MapView.ulx * ResourceManager_MinimapWindowScale,
            GameManager_MapView.uly * ResourceManager_MinimapWindowScale,
            (static_cast<double>(GameManager_MapView.lrx) + 0.9) * ResourceManager_MinimapWindowScale,
            (static_cast<double>(GameManager_MapView.lry) + 0.9) * ResourceManager_MinimapWindowScale, COLOR_RED);

        if (GameManager_DisplayButtonMinimap2x) {
            uint8_t* minimap2x = new (std::nothrow) uint8_t[mmw_width * mmw_height];
            Point minimap_view_offset;

            minimap_view_offset.x = ResourceManager_MinimapWindowOffset.x / 2 +
                                    GameManager_GridCenterOffset.x * ResourceManager_MinimapWindowScale;
            minimap_view_offset.y = ResourceManager_MinimapWindowOffset.y / 2 +
                                    GameManager_GridCenterOffset.y * ResourceManager_MinimapWindowScale;

            uint8_t* address = &mmw->buffer[minimap_view_offset.y * mmw->width + minimap_view_offset.x];

            for (int32_t y = 0; y < (mmw_height / 2); ++y) {
                for (int32_t x = 0; x < (mmw_width / 2); ++x) {
                    minimap2x[(2 * y) * mmw_width + 2 * x] = address[y * mmw->width + x];
                    minimap2x[(2 * y) * mmw_width + 2 * x + 1] = address[y * mmw->width + x];
                }

                memcpy(&minimap2x[(2 * y + 1) * mmw_width], &minimap2x[(2 * y) * mmw_width], mmw_width);
            }

            buf_to_buf(minimap2x, mmw_width, mmw_height, mmw_width, mmw->buffer, mmw->width);

            delete[] minimap2x;
        }

        win_draw_rect(mmw->id, &mmw->window);
        GameManager_RenderMinimapDisplay = false;
    }
}

void GameManager_RenderMap() {
    bool is_message_box_active;

    if (ResourceManager_MapTileIds == nullptr || GameManager_DisableMapRendering) {
        return;
    }

    if (GameManager_GridOffset.x || GameManager_GridOffset.y) {
        GameManager_UpdateMainMapView(MAP_VIEW_SCROLL, GameManager_GridOffset.x, GameManager_GridOffset.y);

        GameManager_GridOffset.x = 0;
        GameManager_GridOffset.y = 0;

    } else {
        GameManager_GridStepLevel = GameManager_QuickScroll;
        GameManager_GridStepOffset = 0;
    }

    DrawMap_RenderBuildMarker();

    if (GameManager_RenderFlag1) {
        if (GameManager_RenderMinimapDisplay) {
            if (GameManager_GameState == GAME_STATE_7_SITE_SELECT || GameManager_GameState == GAME_STATE_12 ||
                GameManager_GameState == GAME_STATE_13) {
                GameManager_RenderMinimapDisplay = false;

            } else {
                memcpy(ResourceManager_MinimapUnits, ResourceManager_MinimapFov,
                       ResourceManager_MapSize.x * ResourceManager_MapSize.y);
            }
        }

        if (GameManager_RenderEnable) {
            Rect bounds = GameManager_MultiSelectBounds;
            WindowInfo* window = WindowManager_GetWindow(WINDOW_MAIN_MAP);
            int32_t width;
            int32_t height;

            width =
                (((GameManager_MousePosition2.x - window->window.ulx) * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) +
                GameManager_MapWindowDrawBounds.ulx;
            height =
                (((GameManager_MousePosition2.y - window->window.uly) * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) +
                GameManager_MapWindowDrawBounds.uly;

            GameManager_MultiSelectBounds.ulx =
                std::min(static_cast<int32_t>(GameManager_ScaledMousePosition.x), width);
            GameManager_MultiSelectBounds.uly =
                std::min(static_cast<int32_t>(GameManager_ScaledMousePosition.y), height);
            GameManager_MultiSelectBounds.lrx =
                std::max(static_cast<int32_t>(GameManager_ScaledMousePosition.x), width);
            GameManager_MultiSelectBounds.lry =
                std::max(static_cast<int32_t>(GameManager_ScaledMousePosition.y), height);

            bounds.ulx = std::min(bounds.ulx, GameManager_MultiSelectBounds.ulx);
            bounds.uly = std::min(bounds.uly, GameManager_MultiSelectBounds.uly);
            bounds.lrx = std::max(bounds.lrx, GameManager_MultiSelectBounds.lrx);
            bounds.lry = std::max(bounds.lry, GameManager_MultiSelectBounds.lry);

            GameManager_AddDrawBounds(&bounds);

            GameManager_RenderEnable = false;
        }

        is_message_box_active = MessageManager_MessageBox_IsActive;

        if (is_message_box_active) {
            Rect bounds;

            GameManager_GetScaledMessageBoxBounds(&bounds);
            if (DrawMap_IsInsideBounds(&bounds)) {
                GameManager_AddDrawBounds(&bounds);

            } else {
                is_message_box_active = false;
            }
        }

        DrawMapBuffer drawmap;

        DrawMap_RenderMapTiles(&drawmap, GameManager_DisplayButtonGrid);

        if (GameManager_HumanPlayerCount && GameManager_GameState == GAME_STATE_11 &&
            GameManager_ActiveTurnTeam != GameManager_PlayerTeam) {
            DrawMap_RedrawDirtyZones();

            GameManager_RenderMinimapDisplay = false;
            GameManager_RenderFlag1 = false;
            GameManager_RenderFlag2 = false;

        } else {
            --GameManager_MarkerColorUpdatePeriod;

            if (GameManager_MarkerColorUpdatePeriod == 0) {
                GameManager_MarkerColor ^= 0xFF;
                GameManager_MarkerColorUpdatePeriod = 5;
            }

            DrawMap_RenderUnits();

            GameManager_RenderSurveyIndicator(&drawmap);

            GameManager_RenderScanRangeIndicators();

            if (is_message_box_active) {
                MessageManager_DrawMessageBox();
            }

            if (GameManager_RenderState == 2) {
                GameManager_RenderMultiSelectIndicator();
            }

            DrawMap_RedrawDirtyZones();

            GameManager_RenderFlag1 = false;
            GameManager_RenderFlag2 = false;

            GameManager_RenderMinimap();
        }
    }
}

bool GameManager_CargoSelection(uint16_t team) {
    CargoMenu cargo_menu(team);
    char message[200];

    if (GameManager_HumanPlayerCount) {
        sprintf(message, _(df78), menu_team_names[team]);

        MessageManager_DrawMessage(message, 0, 1, true);
    }

    return cargo_menu.Run();
}

bool GameManager_PlayerMissionSetup(uint16_t team) {
    int32_t team_gold;

    GameManager_GameState = GAME_STATE_7_SITE_SELECT;

    mouse_show();

    GameManager_PlayerTeam = team;
    GameManager_ActiveTurnTeam = team;

    ini_set_setting(INI_PLAYER_CLAN, UnitsManager_TeamInfo[team].team_clan);

    if (GameManager_IsMapInitialized) {
        GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, 4, 0, false);
        GameManager_ProcessTick(1);
    }

    do {
        GameManager_GameState = GAME_STATE_7_SITE_SELECT;

        team_gold = ini_get_setting(INI_START_GOLD) + ini_clans.GetClanGold(UnitsManager_TeamInfo[team].team_clan);

        if (team_gold) {
            if (!GameManager_CargoSelection(team)) {
                return false;
            }
        } else {
            UnitsManager_AddDefaultMissionLoadout(team);
        }

        if (!GameManager_IsMapInitialized) {
            GameManager_InitMap();
        }

        if (GameManager_HumanPlayerCount && !team_gold) {
            GameManager_DrawSelectSiteMessage(team);
        }

        UnitsManager_TeamMissionSupplies[team].proximity_alert_ack = 1;

        if (GameManager_SelectSite(team)) {
            MessageManager_MessageBox_IsActive = false;

            return true;
        }

    } while (GameManager_GameState != GAME_STATE_3_MAIN_MENU && GameManager_GameState != GAME_STATE_15_FATAL_ERROR &&
             team_gold);

    return false;
}

void GameManager_DrawSelectSiteMessage(uint16_t team) {
    SmartString string;

    string = menu_team_names[team];
    string += ":\n";
    string += _(851c);

    MessageManager_DrawMessage(string.GetCStr(), 0, 1, true);
}

bool GameManager_SelectSite(uint16_t team) {
    int32_t proximity_alert_ack;
    int32_t range{0};
    int32_t scan{0};
    SmartPointer<UnitValues> unit_values;
    bool flag;
    int32_t starting_position_x;
    int32_t starting_position_y;
    int32_t grid_x_overlap{0};
    int32_t grid_y_overlap{0};
    int32_t proximity_state;

    proximity_alert_ack = UnitsManager_TeamMissionSupplies[team].proximity_alert_ack;

    if (proximity_alert_ack == 1) {
        flag = true;

        MessageManager_DrawMessage(_(dfe1), 0, 0);
        SoundManager_PlayVoice(V_M278, V_F177);

    } else {
        flag = false;
    }

    while (proximity_alert_ack != 0 && proximity_alert_ack != 6) {
        MouseEvent::Clear();
        GameManager_GameState = GAME_STATE_7_SITE_SELECT;

        if (flag) {
            while (GameManager_GameState == GAME_STATE_7_SITE_SELECT) {
                GameManager_ProgressTurn();
            }
        }

        if (GameManager_GameState == GAME_STATE_14 || GameManager_GameState == GAME_STATE_3_MAIN_MENU ||
            GameManager_GameState == GAME_STATE_15_FATAL_ERROR) {
            proximity_alert_ack = 6;
            break;
        }

        GameManager_GameState = GAME_STATE_7_SITE_SELECT;

        Cursor_SetCursor(CURSOR_HAND);

        starting_position_x = UnitsManager_TeamMissionSupplies[team].starting_position.x;
        starting_position_y = UnitsManager_TeamMissionSupplies[team].starting_position.y;

        if (GameManager_SelectedUnit == nullptr) {
            GameManager_SelectedUnit =
                UnitsManager_SpawnUnit(MASTER, team, starting_position_x, starting_position_y, nullptr);
            GameManager_SelectedUnit->SetOrder(ORDER_IDLE);

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
                    MessageManager_DrawMessage(_(5cb4), 0, 0);

                    grid_x_overlap = starting_position_x;
                    grid_y_overlap = starting_position_y;

                    proximity_alert_ack = 2;
                    flag = true;
                }

            } break;

            case 3: {
                MessageManager_DrawMessage(_(3a97), 2, 0);

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

        UnitsManager_DestroyUnit(&*GameManager_SelectedUnit);
    }

    return proximity_alert_ack == 0;
}

int32_t GameManager_UpdateProximityState(uint16_t team) {
    int32_t state;

    state = 0;

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
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

int32_t GameManager_CheckLandingZones(uint16_t team1, uint16_t team2) {
    int32_t status;
    int32_t proximity_range;
    int32_t exclude_range;
    int32_t distance_x;
    int32_t distance_y;

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

    GameManager_LastZoomLevel = 0;
    Gfx_ZoomLevel = 0;

    GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, 4, 0, false);
    GameManager_ProcessTick(1);
    mouse_show();

    GameManager_IsMapInitialized = true;
}

void GameManager_SelectNextUnit(int32_t seek_direction) {
    UnitInfo* unit;

    GameManager_ManagePlayerAction();
    GameManager_UpdateDrawBounds();

    unit = Access_SeekNextUnit(GameManager_PlayerTeam, &*GameManager_SelectedUnit, seek_direction);

    if (!unit) {
        unit = GameManager_GetFirstRelevantUnit(GameManager_PlayerTeam);
    }

    if (unit) {
        if (!GameManager_IsInsideMapView(unit)) {
            GameManager_UpdateMainMapView(MAP_VIEW_CENTER, unit->grid_x, unit->grid_y);
        }

        GameManager_MenuUnitSelect(unit);

    } else {
        GameManager_MenuUnitSelect(nullptr);
    }
}

bool GameManager_UpdateMapDrawBounds(int32_t ulx, int32_t uly) {
    int32_t width;
    int32_t height;
    int32_t width_pixels;
    int32_t height_pixels;
    bool result;

    width = GameManager_MapWindowDrawBounds.lrx - GameManager_MapWindowDrawBounds.ulx + 1;
    height = GameManager_MapWindowDrawBounds.lry - GameManager_MapWindowDrawBounds.uly + 1;

    width_pixels = (WindowManager_MapWidth * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR;
    height_pixels = (WindowManager_MapHeight * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR;

    if (ulx < 0) {
        ulx = 0;
    }

    if (uly < 0) {
        uly = 0;
    }

    while (((width_pixels * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor) < WindowManager_MapWidth) {
        ++width_pixels;
    }

    while (((height_pixels * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor) < WindowManager_MapHeight) {
        ++height_pixels;
    }

    if (ulx + width_pixels >= ResourceManager_MapSize.x * GFX_MAP_TILE_SIZE) {
        ulx = ResourceManager_MapSize.x * GFX_MAP_TILE_SIZE - width_pixels;
    }

    if (uly + height_pixels >= ResourceManager_MapSize.y * GFX_MAP_TILE_SIZE) {
        uly = ResourceManager_MapSize.y * GFX_MAP_TILE_SIZE - height_pixels;
    }

    if (ulx == GameManager_MapWindowDrawBounds.ulx && uly == GameManager_MapWindowDrawBounds.uly &&
        width_pixels == width && height_pixels == height) {
        result = false;

    } else {
        GameManager_MapWindowDrawBounds.ulx = ulx;
        GameManager_MapWindowDrawBounds.uly = uly;
        GameManager_MapWindowDrawBounds.lrx = ulx + width_pixels - 1;
        GameManager_MapWindowDrawBounds.lry = uly + height_pixels - 1;

        GameManager_MapWindowDrawBounds.ulx = std::max(0, GameManager_MapWindowDrawBounds.ulx);
        GameManager_MapWindowDrawBounds.uly = std::max(0, GameManager_MapWindowDrawBounds.uly);
        GameManager_MapWindowDrawBounds.lrx = std::max(0, GameManager_MapWindowDrawBounds.lrx);
        GameManager_MapWindowDrawBounds.lry = std::max(0, GameManager_MapWindowDrawBounds.lry);

        Gfx_MapWindowUlx = (GameManager_MapWindowDrawBounds.ulx * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor;
        Gfx_MapWindowUly = (GameManager_MapWindowDrawBounds.uly * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor;

        GameManager_MapView.ulx = GameManager_MapWindowDrawBounds.ulx / GFX_MAP_TILE_SIZE;
        GameManager_MapView.uly = GameManager_MapWindowDrawBounds.uly / GFX_MAP_TILE_SIZE;
        GameManager_MapView.lrx = GameManager_MapWindowDrawBounds.lrx / GFX_MAP_TILE_SIZE;
        GameManager_MapView.lry = GameManager_MapWindowDrawBounds.lry / GFX_MAP_TILE_SIZE;

        result = true;
    }

    return result;
}

float GameManager_GetScrollRateLimit() { return GameManager_ScrollRateLimit; }

float GameManager_UpdateScrollRateLimit() {
    const uint32_t elapsed_time = timer_elapsed_time(GameManager_ScrollTimeStamp);
    const uint32_t time_limit = TIMER_FPS_TO_MS(Svga_GetScreenRefreshRate());

    GameManager_ScrollTimeStamp = timer_get();

    if (elapsed_time > time_limit * 1.5f) {
        GameManager_ScrollRateLimit = 1.5f;

    } else {
        GameManager_ScrollRateLimit = static_cast<float>(elapsed_time) / time_limit;
    }

    return GameManager_ScrollRateLimit;
}

bool GameManager_ScrollMainMapView(int32_t& offset_x, int32_t& offset_y) {
    bool result;
    GameManager_ScrollRate += GameManager_GridStepLevel * GameManager_GetScrollRateLimit();
    const int32_t step_level = GameManager_ScrollRate;

    offset_x = offset_x * step_level + GameManager_MapWindowDrawBounds.ulx;
    offset_y = offset_y * step_level + GameManager_MapWindowDrawBounds.uly;

    const auto last_map_view_ulx = GameManager_MapView.ulx;
    const auto last_map_view_uly = GameManager_MapView.uly;

    GameManager_ScrollRate -= step_level;
    GameManager_GridStepOffset += step_level;

    if (GameManager_UpdateMapDrawBounds(offset_x, offset_y)) {
        GameManager_MapViewCenter.x += GameManager_MapView.ulx - last_map_view_ulx;
        GameManager_MapViewCenter.y += GameManager_MapView.uly - last_map_view_uly;

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool GameManager_CenterMainMapView(int32_t ulx, int32_t uly) {
    ulx = std::min(ResourceManager_MapSize.x - 4, std::max(ulx, (WindowManager_MapWidth / GFX_MAP_TILE_SIZE) / 2));
    uly = std::min(ResourceManager_MapSize.y - 4, std::max(uly, (WindowManager_MapHeight / GFX_MAP_TILE_SIZE) / 2));

    if (GameManager_MapViewCenter.x == ulx && GameManager_MapViewCenter.y == uly) {
        return false;
    }

    GameManager_MapViewCenter.x = ulx;
    GameManager_MapViewCenter.y = uly;

    ulx = (GameManager_MapViewCenter.x * GFX_MAP_TILE_SIZE) + GFX_MAP_TILE_SIZE / 2;
    uly = (GameManager_MapViewCenter.y * GFX_MAP_TILE_SIZE) + GFX_MAP_TILE_SIZE / 2;

    ulx -= (((WindowManager_MapWidth * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) / 2);
    uly -= (((WindowManager_MapHeight * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) / 2);

    return GameManager_UpdateMapDrawBounds(ulx, uly);
}

bool GameManager_ZoomMainMapView(int32_t zoom_level, bool mode) {
    zoom_level = std::min(64, std::max(ResourceManager_MainmapZoomLimit, zoom_level));

    if ((Gfx_ZoomLevel == static_cast<uint32_t>(zoom_level)) && !mode) {
        return false;
    }

    Gfx_ZoomLevel = zoom_level;
    Gfx_MapScalingFactor = GFX_SCALE_NUMERATOR / Gfx_ZoomLevel;

    GameManager_UpdateZoomSlider();

    GameManager_QuickScroll = ini_get_setting(INI_QUICK_SCROLL);

    if (GameManager_QuickScroll < 4) {
        GameManager_QuickScroll = 16;
        ini_set_setting(INI_QUICK_SCROLL, GameManager_QuickScroll);
    }

    GameManager_QuickScroll = (Gfx_MapScalingFactor * GameManager_QuickScroll) / GFX_SCALE_DENOMINATOR;

    GameManager_GridStepLevel = GameManager_QuickScroll;
    GameManager_GridStepOffset = 0;

    const Point last_center{GameManager_MapViewCenter};

    GameManager_MapViewCenter.x = 0;
    GameManager_MapViewCenter.y = 0;

    return GameManager_CenterMainMapView(last_center.x, last_center.y);
}

void GameManager_UpdateZoomSlider() {
    if (GameManager_GameState != GAME_STATE_7_SITE_SELECT) {
        WindowInfo* window{WindowManager_GetWindow(WINDOW_ZOOM_SLIDER_WINDOW)};
        const int32_t width = (25 + 3) * WindowManager_GetScale();
        const int32_t offset =
            ((window->window.lrx - window->window.ulx - width) * (64 - Gfx_ZoomLevel)) / 60 + width / 2;

        WindowManager_LoadSimpleImage(ZOOMPNL1, window->window.ulx, window->window.uly, false);
        WindowManager_LoadSimpleImage(ZOOMPTR, offset + window->window.ulx, window->window.uly + 1, true);

        win_draw_rect(window->id, &window->window);
    }
}

void GameManager_UpdateMainMapView(int32_t mode, int32_t ulx, int32_t uly, bool flag) {
    GameManager_UpdateScrollRateLimit();

    switch (mode) {
        case MAP_VIEW_ZOOM: {
            if (!GameManager_ZoomMainMapView(ulx, flag)) {
                return;
            }
        } break;

        case MAP_VIEW_CENTER: {
            if (!GameManager_CenterMainMapView(ulx, uly)) {
                return;
            }
        } break;

        case MAP_VIEW_SCROLL: {
            if (!GameManager_ScrollMainMapView(ulx, uly)) {
                return;
            }
        } break;
    }

    GameManager_UpdateGridCenterOffset();
    GameManager_DeinitPopupButtons(false);
    GameManager_UpdateDrawBounds();

    if (GameManager_RenderState == 2) {
        GameManager_RenderEnable = true;
    }

    SoundManager_UpdateSfxPosition();
}

void GameManager_AutoSelectNext(UnitInfo* unit) {
    if (!ini_get_setting(INI_AUTO_SELECT) || GameManager_IsInteractable(unit)) {
        GameManager_EnableMainMenu(unit);

    } else {
        if (unit->GetOrder() == ORDER_AWAIT) {
            unit->SetOrderState(ORDER_STATE_READY_TO_EXECUTE_ORDER);
        }

        GameManager_SelectNextUnit(1);
        GameManager_EnableMainMenu(nullptr);
        GameManager_MenuClickFindButton();
    }
}

Point GameManager_GetStartPositionMiningStation(uint16_t team) {
    TeamMissionSupplies* supplies = &UnitsManager_TeamMissionSupplies[team];
    const int32_t max_resources = ini_get_setting(INI_MAX_RESOURCES);
    const int32_t minimum_fuel = (max_resources * 8 + 10) / 20;
    const int32_t minimum_materials = (max_resources * 12 + 10) / 20;
    Point point(supplies->starting_position);
    int16_t cargo_raw{0};
    int16_t cargo_fuel{0};
    int16_t cargo_gold{0};

    Survey_GetTotalResourcesInArea(point.x, point.y, 1, &cargo_raw, &cargo_gold, &cargo_fuel, true, team);

    if (!GameManager_IsValidStartingPosition(point.x, point.y) || cargo_raw < minimum_materials ||
        cargo_fuel < minimum_fuel) {
        for (int32_t range = 1; range < 10; ++range) {
            point.x = supplies->starting_position.x - range;
            point.y = supplies->starting_position.y + range;

            for (int32_t direction = 0; direction < 8; direction += 2) {
                for (int32_t i = 0; i < 2 * range; ++i) {
                    point += Paths_8DirPointsArray[direction];

                    if (GameManager_IsValidStartingPosition(point.x, point.y)) {
                        Survey_GetTotalResourcesInArea(point.x, point.y, 1, &cargo_raw, &cargo_gold, &cargo_fuel, true,
                                                       team);
                        if (cargo_raw >= minimum_materials && cargo_fuel >= minimum_fuel) {
                            return point;
                        }
                    }
                }
            }
        }
    }

    SDL_assert(GameManager_IsValidStartingPosition(point.x, point.y));

    return point;
}

Point GameManager_GetStartingPositionPowerGenerator(Point point, uint16_t team) {
    point.x -= 1;
    point.y += 2;

    for (int32_t direction = 0; direction < 8; direction += 2) {
        for (int32_t i = 0; i < 2; ++i) {
            point += Paths_8DirPointsArray[direction];

            if (Access_IsAccessible(POWGEN, team, point.x, point.y, AccessModifier_SameClassBlocks)) {
                return point;
            }
        }
    }

    point.y -= 1;

    return point;
}

void GameManager_GameSetup(int32_t game_state) {
    WindowInfo* window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    int32_t zoom_level;
    int32_t max_zoom_level;
    uint32_t timestamp;

    GameManager_InitUnitsAndGameState();

    if (Remote_IsNetworkGame) {
        if (game_state == GAME_STATE_6) {
            Remote_SendNetPacket_Signal(REMOTE_PACKET_26, GameManager_PlayerTeam,
                                        UnitsManager_TeamInfo[GameManager_PlayerTeam].team_clan);
            ResourceManager_InitClanUnitValues(GameManager_PlayerTeam);
        }

        dos_srand(Remote_RngSeed);
    }

    Ai_Init();

    PathsManager_Clear();

    if (game_state == GAME_STATE_6) {
        GameManager_GameState = GAME_STATE_7_SITE_SELECT;

        if (!GameManager_InitGame()) {
            return;
        }

        MouseEvent::Clear();

        if (GameManager_PlayScenarioIntro && ini_get_setting(INI_GAME_FILE_TYPE) == GAME_TYPE_CUSTOM) {
            Color* palette;

            GameManager_LandingSequence.DeleteButtons();
            palette = GameManager_MenuFadeOut(250);
            Movie_Play(DEMO1FLC);
            memcpy(WindowManager_ColorPalette, palette, PALETTE_STRIDE * PALETTE_SIZE);

            delete[] palette;

            WindowManager_LoadBigImage(FRAMEPIC, window, window->width, false, true, -1, -1, false, true);
            GameManager_ProcessTick(true);
            WindowManager_FadeIn(100);

            GameManager_PlayScenarioIntro = false;
        }

        GameManager_GameState = GAME_STATE_11;
        GameManager_LandingSequence.OpenPanel();

        zoom_level = 4;
        max_zoom_level = 64;

        if (ResourceManager_DisableEnhancedGraphics) {
            max_zoom_level = 32;
        }

        while (zoom_level <= max_zoom_level) {
            timestamp = timer_get();

            GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, zoom_level, 0, false);
            GameManager_ProcessTick(false);

            while ((timer_get() - timestamp) < TIMER_FPS_TO_MS(48)) {
            }

            zoom_level *= 2;
        }

    } else {
        UnitInfo* unit;

        GameManager_GameState = GAME_STATE_10;

        if (!SaveLoadMenu_Load(ini_get_setting(INI_GAME_FILE_NUMBER), ini_get_setting(INI_GAME_FILE_TYPE),
                               !Remote_IsNetworkGame)) {
            GameManager_GameState = GAME_STATE_3_MAIN_MENU;
            return;
        }

        GameManager_GameState = GAME_STATE_11;

        GameManager_LandingSequence.OpenPanel();
        GameManager_UpdateHumanPlayerCount();
        GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, 4, 0, false);

        unit = GameManager_GetFirstRelevantUnit(GameManager_PlayerTeam);

        GameManager_UpdateMainMapView(MAP_VIEW_CENTER, unit->grid_x, unit->grid_y);
        GameManager_ProcessTick(true);
    }

    GameManager_GameState = GAME_STATE_8_IN_GAME;

    Access_UpdateVisibilityStatus(GameManager_AllVisible);

    SoundManager_PlayMusic(static_cast<ResourceID>(ini_get_setting(INI_WORLD) / 6 + SNOW_MSC), true);

    GameManager_MenuAnimateDisplayControls();
    GameManager_MenuInitDisplayControls();
    GameManager_DisableMainMenu();
    GameManager_UpdateTurnTimer(false, GameManager_TurnTimerValue);
    GameManager_DrawMouseCoordinates(0, 0);

    mouse_hide();
    mouse_set_position(WindowManager_GetWidth(window) / 2, WindowManager_GetHeight(window) / 2);
    mouse_show();
}

void GameManager_GameLoopCleanup() {
    Remote_UpdatePauseTimer = false;
    GameManager_RequestMenuExit = false;
    GameManager_TurnTimerValue = 0;

    SoundManager_FreeAllSamples();

    GameManager_DeinitPopupButtons(false);

    if (Remote_IsNetworkGame) {
        Remote_LeaveGame(GameManager_PlayerTeam, true);
    }

    GameManager_SelectedUnit = nullptr;

    GameManager_LandingSequence.ClosePanel();

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
        delete[] UnitsManager_TeamInfo[team].heat_map_complete;
        delete[] UnitsManager_TeamInfo[team].heat_map_stealth_sea;
        delete[] UnitsManager_TeamInfo[team].heat_map_stealth_land;

        UnitsManager_TeamInfo[team].heat_map_complete = nullptr;
        UnitsManager_TeamInfo[team].heat_map_stealth_sea = nullptr;
        UnitsManager_TeamInfo[team].heat_map_stealth_land = nullptr;
    }

    UnitsManager_GroundCoverUnits.Clear();
    UnitsManager_MobileLandSeaUnits.Clear();
    UnitsManager_ParticleUnits.Clear();
    UnitsManager_StationaryUnits.Clear();
    UnitsManager_MobileAirUnits.Clear();

    Hash_UnitHash.Clear();
    Hash_MapHash.Clear();

    GameManager_SelectedUnit = nullptr;

    delete[] ResourceManager_MapTileIds;
    ResourceManager_MapTileIds = nullptr;

    delete[] ResourceManager_MapTileBuffer;
    ResourceManager_MapTileBuffer = nullptr;

    delete[] ResourceManager_MapSurfaceMap;
    ResourceManager_MapSurfaceMap = nullptr;

    delete[] ResourceManager_CargoMap;
    ResourceManager_CargoMap = nullptr;

    delete GameManager_TurnTimerImageNormal;
    GameManager_TurnTimerImageNormal = nullptr;

    delete GameManager_TurnTimerImageScaled;
    GameManager_TurnTimerImageScaled = nullptr;

    GameManager_MenuDeinitDisplayControls();
    ResourceManager_FreeResources();
}

void GameManager_UpdateProductions(uint16_t team, SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team) {
            (*it).UpdateProduction();
        }
    }
}

void GameManager_ResupplyUnits(uint16_t team, SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).GetOrder() != ORDER_DISABLE) {
            (*it).Resupply();
        }
    }
}

void GameManager_ManageEconomy(uint16_t team) {
    UnitsManager_TeamInfo[team].team_units->OptimizeComplexes(team);

    GameManager_UpdateProductions(team, &UnitsManager_MobileLandSeaUnits);
    GameManager_UpdateProductions(team, &UnitsManager_StationaryUnits);
    GameManager_UpdateProductions(team, &UnitsManager_MobileAirUnits);

    GameManager_ResupplyUnits(team, &UnitsManager_MobileLandSeaUnits);
    GameManager_ResupplyUnits(team, &UnitsManager_StationaryUnits);
    GameManager_ResupplyUnits(team, &UnitsManager_MobileAirUnits);

    for (int32_t i = 0; i < RESEARCH_TOPIC_COUNT; ++i) {
        ResearchTopic* topic = &UnitsManager_TeamInfo[team].research_topics[i];

        if (topic->allocation && topic->turns_to_complete == 0) {
            ResearchMenu_UpdateResearchProgress(team, i, 0);
        }
    }
}

void GameManager_UpdateScoreGraph() {
    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        memmove(&UnitsManager_TeamInfo[team].score_graph[0], &UnitsManager_TeamInfo[team].score_graph[1],
                sizeof(UnitsManager_TeamInfo[team].score_graph) - sizeof(int16_t));

        UnitsManager_TeamInfo[team]
            .score_graph[(sizeof(UnitsManager_TeamInfo[team].score_graph) - sizeof(int16_t)) / sizeof(int16_t)] =
            UnitsManager_TeamInfo[team].team_points;
    }
}

uint16_t GameManager_EvaluateWinner() {
    int32_t result;

    if (ini_setting_victory_type == VICTORY_TYPE_SCORE) {
        uint16_t highest_team_points_team = 0;
        uint32_t highest_team_points = 0;

        for (uint16_t i = 0; i < PLAYER_TEAM_MAX - 1; ++i) {
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

void GameManager_AnnounceWinner(uint16_t team) {
    uint16_t winner;

    winner = GameManager_EvaluateWinner();

    if (winner != team) {
        ResourceID resource_id1;
        ResourceID resource_id2;
        int32_t sample_count;

        sample_count = 1;

        resource_id1 = static_cast<ResourceID>(V_M118 + team * (sample_count + 1));
        resource_id2 = static_cast<ResourceID>(resource_id1 + sample_count);

        SoundManager_PlayVoice(resource_id1, resource_id2);
    }
}

void GameManager_DrawTurnCounter(int32_t turn_count) {
    char text[10];

    sprintf(text, "%i", turn_count);

    GameManager_DrawDisplayPanel(MENU_DISPLAY_CONTROL_TURN_COUNTER, text, 0xA2);
}

void GameManager_DrawTimer(char* text, int32_t color) {
    Rect bounds;
    WindowInfo window;
    Image* turn_timer_image;

    window.id = win_get_top_win(WindowManager_WindowWidth / 2, WindowManager_WindowHeight / 2);
    window.buffer = win_get_buf(window.id);
    window.window.ulx = 0;
    window.window.uly = 0;
    window.window.lrx = win_width(window.id) - 1;
    window.window.lry = win_height(window.id) - 1;
    window.width = win_width(window.id);

    if (window.width == 640) {
        bounds.ulx = 524;
        bounds.uly = 10;
        bounds.lrx = bounds.ulx + 62;
        bounds.lry = bounds.uly + 24;

        if (!GameManager_TurnTimerImageNormal) {
            GameManager_TurnTimerImageNormal =
                new (std::nothrow) Image(bounds.ulx, bounds.uly, bounds.lrx - bounds.ulx, bounds.lry - bounds.uly);
            GameManager_TurnTimerImageNormal->Copy(&window);
        }

        turn_timer_image = GameManager_TurnTimerImageNormal;

    } else {
        bounds.ulx = WindowManager_ScaleUlx(&window, 524);
        bounds.uly = WindowManager_ScaleUly(&window, 10);
        bounds.lrx = bounds.ulx + 62;
        bounds.lry = bounds.uly + 24;

        if (!GameManager_TurnTimerImageScaled) {
            GameManager_TurnTimerImageScaled =
                new (std::nothrow) Image(bounds.ulx, bounds.uly, bounds.lrx - bounds.ulx, bounds.lry - bounds.uly);
            GameManager_TurnTimerImageScaled->Copy(&window);
        }

        turn_timer_image = GameManager_TurnTimerImageScaled;
    }

    turn_timer_image->Write(&window);

    Text_SetFont(GNW_TEXT_FONT_5);

    Text_TextBox(window.buffer, window.width, text, bounds.ulx, bounds.uly, bounds.lrx - bounds.ulx,
                 bounds.lry - bounds.uly, color, true);

    win_draw_rect(window.id, &bounds);
}

void GameManager_DrawTurnTimer(int32_t turn_time, bool mode) {
    int32_t color;
    char text[10];

    if (turn_time <= 20) {
        if (GameManager_IsTurnTimerActive && GameManager_PlayerTeam == GameManager_ActiveTurnTeam) {
            if (turn_time == 20 && ini_get_setting(INI_TIMER) > 20) {
                SoundManager_PlayVoice(V_M272, V_F273);
            } else if (!UnitsManager_TeamInfo[GameManager_PlayerTeam].finished_turn &&
                       ini_get_setting(INI_TIMER) > 20 && Remote_UpdatePauseTimer) {
                SoundManager_PlayVoice(V_M275, V_F275);
            }

            GameManager_IsTurnTimerActive = false;
        }

        color = COLOR_RED;

    } else {
        color = 0xA2;
    }

    sprintf(text, "%2.2i:%2.2i", turn_time / 60, turn_time % 60);

    if (mode) {
        GameManager_DrawTimer(text, color);

    } else {
        GameManager_DrawDisplayPanel(MENU_DISPLAY_CONTROL_TURN_TIMER, text, color);
    }
}

void GameManager_UpdateTurnTimer(bool mode, int32_t turn_time) {
    GameManager_TurnTimerValue = turn_time;
    if (GameManager_TurnTimerValue) {
        Remote_UpdatePauseTimer = mode;

        if (Remote_UpdatePauseTimer) {
            Remote_PauseTimeStamp = timer_get();
        }
    } else {
        Remote_UpdatePauseTimer = false;
    }

    GameManager_DrawTurnTimer(GameManager_TurnTimerValue);
    GameManager_RequestMenuExit = false;
}

void GameManager_DrawDisplayPanel(int32_t control_id, const char* text, int32_t color, int32_t ulx) {
    if (GameManager_DisplayControlsInitialized) {
        struct MenuDisplayControl* control = &GameManager_MenuDisplayControls[control_id];
        struct ImageSimpleHeader* image =
            reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(control->resource_id));

        buf_to_buf(&image->transparent_color, image->width, image->height, image->width, control->image->GetData(),
                   image->width);

        Text_SetFont(GNW_TEXT_FONT_5);

        Text_TextBox(control->image->GetData(), image->width, text, ulx, 0, image->width - ulx, image->height, color,
                     true);

        control->button->CopyUp(control->image->GetData());

        control->button->Disable();
        control->button->Enable();
    }
}

void GameManager_UpdateGuiControl(uint16_t team) {
    WindowInfo* window;
    char message[200];

    GameManager_UpdateTurnTimer(false, 0);
    GameManager_MenuClickSurveyButton(false);
    GameManager_MenuClickStatusButton(false);
    GameManager_MenuClickColorsButton(false);
    GameManager_MenuClickHitsButton(false);
    GameManager_MenuClickAmmoButton(false);
    GameManager_MenuClickRangeButton(false);
    GameManager_MenuClickScanButton(false);
    GameManager_MenuClickGridButton(false);
    GameManager_MenuClickMinimap2xButton(false);
    GameManager_MenuClickMinimapTntButton(false);
    GameManager_MenuDeleteFlic();
    GameManager_FillOrRestoreWindow(WINDOW_CORNER_FLIC, COLOR_BLACK, true);
    GameManager_FillOrRestoreWindow(WINDOW_STAT_WINDOW, COLOR_BLACK, true);

    if (GameManager_SelectedUnit != nullptr) {
        SoundManager_PlaySfx(&*GameManager_SelectedUnit, SFX_TYPE_INVALID);
    }

    GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, 4, 0);
    GameManager_ProcessTick(true);
    Access_UpdateMinimapFogOfWar(team, false, true);

    window = WindowManager_GetWindow(WINDOW_MINIMAP);

    const int32_t window_width{window->window.lrx - window->window.ulx + 1};
    const int32_t window_height{window->window.lry - window->window.uly + 1};

    if (ResourceManager_MapSize.x == window_width && ResourceManager_MapSize.y == window_height) {
        buf_to_buf(ResourceManager_MinimapFov, ResourceManager_MapSize.x, ResourceManager_MapSize.y,
                   ResourceManager_MapSize.x, window->buffer, window->width);

    } else {
        cscale(ResourceManager_MinimapFov, ResourceManager_MapSize.x, ResourceManager_MapSize.y,
               ResourceManager_MapSize.x, window->buffer, window_width, window_height, window->width);
    }

    win_draw_rect(window->id, &window->window);

    sprintf(message, _(c2eb), menu_team_names[team]);

    MessageManager_DrawMessage(message, 0, 1, true);

    GameManager_PlayerTeam = team;

    GameManager_UpdatePanelButtons(team);
}

uint16_t GameManager_GetCrc16(uint16_t data, uint16_t crc_checksum) {
    for (int32_t i = 0; i < 16; ++i) {
        if (crc_checksum & 0x8000) {
            crc_checksum = (crc_checksum << 1) ^ 0x1021u;
        } else {
            crc_checksum <<= 1;
        }
    }

    return crc_checksum ^ data;
}

uint16_t GameManager_GetUnitListChecksum(SmartList<UnitInfo>* units, uint16_t team, uint16_t crc_checksum) {
    if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE || team == PLAYER_TEAM_ALIEN) {
        for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
            if ((*it).GetId() != 0xFFFF && !((*it).flags & EXPLODING) && (*it).team == team) {
                crc_checksum = GameManager_GetCrc16((*it).team, crc_checksum);
                crc_checksum = GameManager_GetCrc16((*it).GetUnitType(), crc_checksum);
                crc_checksum = GameManager_GetCrc16((*it).unit_id, crc_checksum);
                crc_checksum = GameManager_GetCrc16((*it).grid_x, crc_checksum);
                crc_checksum = GameManager_GetCrc16((*it).grid_y, crc_checksum);
                crc_checksum = GameManager_GetCrc16((*it).hits, crc_checksum);

                if (!((*it).flags & STATIONARY)) {
                    crc_checksum = GameManager_GetCrc16((*it).speed, crc_checksum);
                }

                crc_checksum = GameManager_GetCrc16((*it).shots, crc_checksum);

                if (UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type) {
                    crc_checksum = GameManager_GetCrc16((*it).storage, crc_checksum);
                }

                crc_checksum = GameManager_GetCrc16((*it).ammo, crc_checksum);
            }
        }
    }

    return crc_checksum;
}

bool GameManager_CheckDesync() {
    bool result;

    if (Remote_IsNetworkGame) {
        uint16_t crc_checksum;

        crc_checksum = 0xFFFF;

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            crc_checksum = GameManager_GetUnitListChecksum(&UnitsManager_GroundCoverUnits, team, crc_checksum);
        }

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            crc_checksum = GameManager_GetUnitListChecksum(&UnitsManager_MobileLandSeaUnits, team, crc_checksum);
        }

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            crc_checksum = GameManager_GetUnitListChecksum(&UnitsManager_StationaryUnits, team, crc_checksum);
        }

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            crc_checksum = GameManager_GetUnitListChecksum(&UnitsManager_MobileAirUnits, team, crc_checksum);
        }

        if (Remote_CheckDesync(GameManager_PlayerTeam, crc_checksum)) {
            result = true;

        } else {
            if (DesyncMenu_Menu()) {
                if (!GameManager_LoadGame(10, GameManager_MenuFadeOut())) {
                    GameManager_GameState = GAME_STATE_3_MAIN_MENU;

                    MessageManager_DrawMessage(_(df36), 0, 1);
                }

            } else {
                GameManager_GameState = GAME_STATE_3_MAIN_MENU;
            }

            MouseEvent::Clear();

            result = false;
        }

    } else {
        result = true;
    }

    return result;
}

void GameManager_UpdateGui(uint16_t team, int32_t game_state, bool enable_autosave) {
    int32_t ini_timer_setting;

    GameManager_ActiveTurnTeam = team;

    GameManager_EnableMainMenu(nullptr);

    if (game_state == GAME_STATE_10) {
        ini_timer_setting = ini_get_setting(INI_TIMER);
        GameManager_GameState = SaveLoadMenu_GameState;

        GameManager_UpdatePanelButtons(GameManager_PlayerTeam);

    } else {
        ini_timer_setting = ini_get_setting(INI_TIMER);
        GameManager_GameState = GAME_STATE_11;
    }

    if (ini_timer_setting == 0 && UnitsManager_TeamInfo[GameManager_PlayerTeam].team_type == TEAM_TYPE_COMPUTER) {
        ini_timer_setting = 45;
    }

    switch (UnitsManager_TeamInfo[team].team_type) {
        case TEAM_TYPE_ELIMINATED:
        case TEAM_TYPE_NONE: {
            GameManager_GameState = GAME_STATE_9_END_TURN;
            return;
        } break;

        case TEAM_TYPE_PLAYER: {
            GameManager_MenuClickEndTurnButton(true);

            if (GameManager_PlayerTeam != team) {
                if (GameManager_HumanPlayerCount) {
                    GameManager_UpdateGuiControl(team);
                }

                GameManager_PlayerTeam = team;

                Access_UpdateMinimapFogOfWar(team, GameManager_AllVisible);
            }

            if (game_state != GAME_STATE_10) {
                GameManager_ProgressBuildState(team);

                if (GameManager_SelectedUnit != nullptr) {
                    GameManager_UpdateInfoDisplay(&*GameManager_SelectedUnit);

                } else if (GameManager_TurnCounter == 1) {
                    GameManager_SelectNextUnit(1);
                }
            }
        } break;

        case TEAM_TYPE_COMPUTER: {
            if (game_state != GAME_STATE_10) {
                GameManager_ProgressBuildState(team);

                ini_timer_setting = ini_get_setting(INI_ENDTURN);

                if (ini_timer_setting < 45) {
                    ini_timer_setting = 45;
                }
            }

            Ai_BeginTurn(team);

            GameManager_MenuClickEndTurnButton(0);
        } break;

        case TEAM_TYPE_REMOTE: {
            if (game_state != GAME_STATE_10) {
                GameManager_ProgressBuildState(team);
            }
        } break;
    }

    GameManager_IsTurnTimerActive = true;

    GameManager_UpdateTurnTimer(true, ini_timer_setting);

    if (game_state == GAME_STATE_10 && ini_get_setting(INI_GAME_FILE_TYPE) == GAME_TYPE_TRAINING &&
        GameManager_SelectedUnit != nullptr) {
        GameManager_DrawUnitStatusMessage(&*GameManager_SelectedUnit);
    }

    if (GameManager_CheckDesync()) {
        int32_t game_file_type;

        game_file_type = SaveLoadMenu_GetGameFileType();

        if (enable_autosave && game_file_type != GAME_TYPE_DEMO && ini_get_setting(INI_AUTO_SAVE)) {
            char file_name[16];
            char log_message[30];

            sprintf(file_name, "save10.%s", SaveLoadMenu_SaveFileTypes[game_file_type]);
            sprintf(log_message, _(263f), GameManager_TurnCounter);

            SaveLoadMenu_Save(file_name, log_message, false, true);
        }

        if (GameManager_ActiveTurnTeam == GameManager_PlayerTeam) {
            SoundManager_PlayVoice(V_M053, V_F053);
        }
    }

    while (GameManager_GameState == GAME_STATE_8_IN_GAME) {
        GameManager_ProgressTurn();
    }
}

bool GameManager_AreTeamsFinishedTurn() {
    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
            UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED &&
            !UnitsManager_TeamInfo[team].finished_turn) {
            return false;
        }
    }

    return true;
}

bool GameManager_IsInsideMapView(UnitInfo* unit) {
    bool result;

    if (unit->flags & BUILDING) {
        if (unit->grid_x <= GameManager_MapView.lrx && unit->grid_y <= GameManager_MapView.lry &&
            unit->grid_x + 1 >= GameManager_MapView.ulx && unit->grid_y + 1 >= GameManager_MapView.uly) {
            result = true;

        } else {
            result = false;
        }

    } else if (unit->grid_x <= GameManager_MapView.lrx && unit->grid_y <= GameManager_MapView.lry &&
               unit->grid_x >= GameManager_MapView.ulx && unit->grid_y >= GameManager_MapView.uly) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

bool GameManager_OptimizeProduction(uint16_t team, Complex* complex, bool is_player_team, bool mode) {
    bool result;

    if (complex->material >= 0 && complex->fuel >= 0 && complex->gold >= 0 && complex->power >= 0 &&
        complex->workers >= 0) {
        result = true;

    } else {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_PLAYER) {
            is_player_team = false;
        }

        if (mode) {
            if (GameManager_SelectedUnit != nullptr && UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER &&
                GameManager_SelectedUnit->GetComplex() == complex) {
                ProductionManager_OptimizeProduction(team, complex, &*GameManager_SelectedUnit, is_player_team);

            } else {
                ProductionManager_OptimizeProduction(team, complex, nullptr, is_player_team);
            }

            result = false;

        } else {
            result = false;
        }
    }

    return result;
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

bool GameManager_ProcessPopupMenuInput(int32_t key) {
    if (key > 255 || key < 0 || GameManager_SelectedUnit == nullptr ||
        GameManager_SelectedUnit->team != GameManager_PlayerTeam ||
        UnitsManager_TeamInfo[GameManager_SelectedUnit->team].team_type != TEAM_TYPE_PLAYER ||
        (GameManager_GameState == GAME_STATE_9_END_TURN && GameManager_PlayMode != PLAY_MODE_SIMULTANEOUS_MOVES) ||
        UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type != TEAM_TYPE_PLAYER ||
        GameManager_GameState == GAME_STATE_7_SITE_SELECT || GameManager_QuickBuildMenuActive) {
        return false;
    }

    switch (GameManager_SelectedUnit->GetOrder()) {
        case ORDER_SENTRY: {
        } break;

        case ORDER_MOVE:
        case ORDER_MOVE_TO_UNIT:
        case ORDER_MOVE_TO_ATTACK: {
            if (GameManager_SelectedUnit->GetOrderState() != ORDER_STATE_EXECUTING_ORDER) {
                return false;
            }
        } break;

        case ORDER_BUILD: {
            if (GameManager_SelectedUnit->GetOrderState() != ORDER_STATE_BUILD_IN_PROGRESS) {
                return false;
            }
        } break;

        case ORDER_POWER_ON:
        case ORDER_POWER_OFF: {
            if (GameManager_SelectedUnit->GetOrderState() != ORDER_STATE_EXECUTING_ORDER) {
                return false;
            }
        } break;

        case ORDER_CLEAR: {
            if (GameManager_SelectedUnit->GetOrderState() != ORDER_STATE_IN_PROGRESS) {
                return false;
            }
        } break;
    }

    {
        bool flag;
        bool result;

        flag = false;

        if (GameManager_PopupButtons.popup_count == 0) {
            flag = true;

            GameManager_SelectedUnit->popup->init(&*GameManager_SelectedUnit, &GameManager_PopupButtons);
        }

        if (GameManager_PopupButtons.popup_count) {
            int32_t button_index;

            key = toupper(key);

            for (button_index = 0; button_index < GameManager_PopupButtons.popup_count; ++button_index) {
                if (toupper(GameManager_PopupButtons.position[button_index]) == key) {
                    break;
                }

                if (toupper(GameManager_PopupButtons.caption[button_index][0]) == key) {
                    break;
                }
            }

            result = button_index < GameManager_PopupButtons.popup_count;

            if (flag) {
                GameManager_PopupButtons.popup_count = 0;
            }

            if (result) {
                SoundManager_PlaySfx(KCARG0);
                GameManager_PopupButtons.r_func[button_index](
                    0, reinterpret_cast<intptr_t>(GameManager_SelectedUnit.Get()));
            }

        } else {
            result = false;
        }

        return result;
    }
}

void GameManager_PunishCheater() {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetOrder() == ORDER_EXPLODE) {
            return;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if ((*it).GetOrder() == ORDER_EXPLODE) {
            return;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).GetOrder() == ORDER_EXPLODE) {
            return;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == GameManager_CheaterTeam) {
            (*it).FollowUnit();
            UnitsManager_SetNewOrder(&(*it), ORDER_EXPLODE, ORDER_STATE_EXPLODE);
            return;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == GameManager_CheaterTeam) {
            (*it).FollowUnit();
            UnitsManager_SetNewOrder(&(*it), ORDER_EXPLODE, ORDER_STATE_EXPLODE);
            return;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if ((*it).team == GameManager_CheaterTeam) {
            (*it).FollowUnit();
            UnitsManager_SetNewOrder(&(*it), ORDER_EXPLODE, ORDER_STATE_EXPLODE);
            return;
        }
    }

    GameManager_IsCheater = false;
}

void GameManager_ProcessCheatCodes() {
    int32_t cheat_index;
    bool is_multiplayer_game;

    for (cheat_index = 0; cheat_index < CHEAT_CODE_COUNT; ++cheat_index) {
        if (GameManager_TextInput.IsEqual(GameManager_CheatCodes[cheat_index])) {
            break;
        }
    }

    if (cheat_index >= CHEAT_CODE_COUNT) {
        return;
    }

    is_multiplayer_game = Remote_IsNetworkGame;

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER && team != GameManager_PlayerTeam) {
            is_multiplayer_game = true;
        }
    }

    if (is_multiplayer_game) {
        if (Remote_IsNetworkGame && GameManager_PlayMode != PLAY_MODE_TURN_BASED) {
            MessageManager_DrawMessage(_(f47b), 2, 0);

        } else {
            MessageManager_DrawMessage(_(abd7), 2, 0);
            GameManager_IsCheater = true;
            GameManager_CheaterTeam = GameManager_PlayerTeam;
        }
    } else {
        switch (cheat_index) {
            case CHEAT_CODE_MAXSPY: {
                GameManager_MaxSpy = !GameManager_MaxSpy;

                GameManager_UpdateDrawBounds();
            } break;

            case CHEAT_CODE_MAXSURVEY: {
                GameManager_MaxSurvey = !GameManager_MaxSurvey;

                GameManager_MenuClickSurveyButton(true);
            } break;

            case CHEAT_CODE_MAXSTORAGE: {
                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if ((*it).team == GameManager_PlayerTeam &&
                        UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type >= CARGO_TYPE_RAW &&
                        UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type <= CARGO_TYPE_GOLD) {
                        (*it).storage = (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
                    }
                }

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                     it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                    if ((*it).team == GameManager_PlayerTeam &&
                        UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type >= CARGO_TYPE_RAW &&
                        UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type <= CARGO_TYPE_GOLD) {
                        (*it).storage = (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
                    }
                }

                GameManager_MenuUnitSelect(&*GameManager_SelectedUnit);
            } break;

            case CHEAT_CODE_MAXAMMO: {
                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if ((*it).team == GameManager_PlayerTeam) {
                        (*it).ammo = (*it).GetBaseValues()->GetAttribute(ATTRIB_AMMO);
                    }
                }

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                     it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                    if ((*it).team == GameManager_PlayerTeam) {
                        (*it).ammo = (*it).GetBaseValues()->GetAttribute(ATTRIB_AMMO);
                    }
                }

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                     it != UnitsManager_MobileAirUnits.End(); ++it) {
                    if ((*it).team == GameManager_PlayerTeam) {
                        (*it).ammo = (*it).GetBaseValues()->GetAttribute(ATTRIB_AMMO);
                    }
                }

                GameManager_MenuUnitSelect(&*GameManager_SelectedUnit);
            } break;

            case CHEAT_CODE_MAXSUPER: {
                if (GameManager_SelectedUnit != nullptr) {
                    SmartPointer<UnitValues> unit_values(GameManager_SelectedUnit->GetBaseValues());
                    SmartPointer<UnitValues> new_unit_values(new (std::nothrow) UnitValues(*unit_values));
                    UnitValues* selected_unit_values;

                    selected_unit_values =
                        UnitsManager_TeamInfo[GameManager_SelectedUnit->team].team_units->GetBaseUnitValues(
                            GameManager_SelectedUnit->GetUnitType());

                    do {
                        new_unit_values->UpdateVersion();
                        new_unit_values->SetUnitsBuilt(1);
                    } while (new_unit_values->GetVersion() < 30);

                    if (new_unit_values->GetAttribute(ATTRIB_ATTACK) <
                        2 * selected_unit_values->GetAttribute(ATTRIB_ATTACK)) {
                        new_unit_values->SetAttribute(ATTRIB_ATTACK,
                                                      2 * selected_unit_values->GetAttribute(ATTRIB_ATTACK));
                    }

                    if (new_unit_values->GetAttribute(ATTRIB_ARMOR) <
                        2 * selected_unit_values->GetAttribute(ATTRIB_ARMOR)) {
                        new_unit_values->SetAttribute(ATTRIB_ARMOR,
                                                      2 * selected_unit_values->GetAttribute(ATTRIB_ARMOR));
                    }

                    if (new_unit_values->GetAttribute(ATTRIB_HITS) <
                        2 * selected_unit_values->GetAttribute(ATTRIB_HITS)) {
                        new_unit_values->SetAttribute(
                            ATTRIB_HITS, std::min(2 * selected_unit_values->GetAttribute(ATTRIB_HITS), UINT8_MAX));
                    }

                    if (new_unit_values->GetAttribute(ATTRIB_SCAN) <
                        2 * selected_unit_values->GetAttribute(ATTRIB_SCAN)) {
                        new_unit_values->SetAttribute(ATTRIB_SCAN, 2 * selected_unit_values->GetAttribute(ATTRIB_SCAN));
                    }

                    if (new_unit_values->GetAttribute(ATTRIB_RANGE) <
                        2 * selected_unit_values->GetAttribute(ATTRIB_RANGE)) {
                        new_unit_values->SetAttribute(ATTRIB_RANGE,
                                                      2 * selected_unit_values->GetAttribute(ATTRIB_RANGE));
                    }

                    if (new_unit_values->GetAttribute(ATTRIB_SPEED) <
                        2 * selected_unit_values->GetAttribute(ATTRIB_SPEED)) {
                        new_unit_values->SetAttribute(
                            ATTRIB_SPEED, std::min(2 * selected_unit_values->GetAttribute(ATTRIB_SPEED), UINT8_MAX));
                    }

                    if (new_unit_values->GetAttribute(ATTRIB_ROUNDS) <
                        2 * selected_unit_values->GetAttribute(ATTRIB_ROUNDS)) {
                        new_unit_values->SetAttribute(ATTRIB_ROUNDS,
                                                      2 * selected_unit_values->GetAttribute(ATTRIB_ROUNDS));
                    }

                    GameManager_SelectedUnit->hits = new_unit_values->GetAttribute(ATTRIB_HITS);

                    if (unit_values->GetAttribute(ATTRIB_ATTACK) != new_unit_values->GetAttribute(ATTRIB_ATTACK) ||
                        unit_values->GetAttribute(ATTRIB_ARMOR) != new_unit_values->GetAttribute(ATTRIB_ARMOR) ||
                        unit_values->GetAttribute(ATTRIB_HITS) != new_unit_values->GetAttribute(ATTRIB_HITS) ||
                        unit_values->GetAttribute(ATTRIB_SCAN) != new_unit_values->GetAttribute(ATTRIB_SCAN) ||
                        unit_values->GetAttribute(ATTRIB_RANGE) != new_unit_values->GetAttribute(ATTRIB_RANGE) ||
                        unit_values->GetAttribute(ATTRIB_SPEED) != new_unit_values->GetAttribute(ATTRIB_SPEED) ||
                        unit_values->GetAttribute(ATTRIB_ROUNDS) != new_unit_values->GetAttribute(ATTRIB_ROUNDS)) {
                        SmartPointer<UnitInfo> new_unit = GameManager_SelectedUnit->MakeCopy();

                        GameManager_SelectedUnit->SetBaseValues(&*new_unit_values);

                        Access_UpdateMapStatus(&*GameManager_SelectedUnit, true);
                        Access_UpdateMapStatus(&*new_unit, false);

                        GameManager_RenderMinimapDisplay = true;

                        GameManager_MenuUnitSelect(&*GameManager_SelectedUnit);

                        if (unit_values->GetAttribute(ATTRIB_SCAN) != new_unit_values->GetAttribute(ATTRIB_SCAN) ||
                            unit_values->GetAttribute(ATTRIB_RANGE) != new_unit_values->GetAttribute(ATTRIB_RANGE)) {
                            int32_t radius = std::max(new_unit_values->GetAttribute(ATTRIB_SCAN),
                                                      new_unit_values->GetAttribute(ATTRIB_RANGE)) *
                                             GFX_MAP_TILE_SIZE;
                            Rect bounds;
                            int32_t unit_size;

                            if (GameManager_SelectedUnit->flags & BUILDING) {
                                unit_size = GFX_MAP_TILE_SIZE;

                            } else {
                                unit_size = GFX_MAP_TILE_SIZE / 2;
                            }

                            bounds.ulx = (GameManager_SelectedUnit->grid_x * GFX_MAP_TILE_SIZE) + unit_size;
                            bounds.uly = (GameManager_SelectedUnit->grid_y * GFX_MAP_TILE_SIZE) + unit_size;

                            bounds.lrx = bounds.ulx + radius + 1;
                            bounds.lry = bounds.uly + radius + 1;
                            bounds.ulx -= radius + 1;
                            bounds.uly -= radius + 1;

                            GameManager_AddDrawBounds(&bounds);
                        }
                    }
                }
            } break;
        }
    }
}

void GameManager_InitUnitsAndGameState() {
    GameManager_IsCheater = false;

    ResourceManager_FreeResources();
    WindowManager_ScaleResources();
    WindowManager_LoadPalette(FRAMEPIC);

    UnitsManager_InitPopupMenus();

    for (int32_t i = 0; i < UNIT_END; ++i) {
        UnitsManager_BaseUnits[i].Init(&UnitsManager_AbstractUnits[i]);
        MouseEvent::ProcessInput();
    }

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX; ++i) {
        ResourceManager_InitClanUnitValues(i);
    }

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        GameManager_MultiChatTargets[i] = 1;
    }

    for (uint32_t i = 0; i < sizeof(GameManager_MenuItems) / sizeof(struct MenuGuiItem); ++i) {
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
    GameManager_QuickBuildMenuActive = false;

    GameManager_AllVisible = ini_get_setting(INI_ALL_VISIBLE);
    GameManager_RealTime = ini_get_setting(INI_REAL_TIME);
    GameManager_PlayMode = ini_get_setting(INI_PLAY_MODE);
    GameManager_FastMovement = ini_get_setting(INI_FAST_MOVEMENT);

    GameManager_ArrowKeyFlags = 0;

    GameManager_TurnCounter = 1;

    GameManager_MapViewCenter.x = 0;
    GameManager_MapViewCenter.y = 0;

    GameManager_SelectedUnit = nullptr;

    GameManager_PopupButtons.popup_count = 0;

    GameManager_TurnTimerImageNormal = nullptr;
    GameManager_TurnTimerImageScaled = nullptr;
    GameManager_TurnTimerValue = 0;

    GameManager_SpottedEnemyPosition.x = -1;
    GameManager_SpottedEnemyPosition.y = -1;

    GameManager_LockedUnits.Clear();

    SaveLoadMenu_SaveSlot = -1;
}

bool GameManager_InitGame() {
    GameManager_GameFileNumber = 0;
    GameManager_IsMapInitialized = false;

    if (Remote_IsNetworkGame) {
        dos_srand(time(nullptr));
    } else {
        ResourceManager_InitTeamInfo();
    }

    ResourceManager_InitInGameAssets(ini_get_setting(INI_WORLD));

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        UnitsManager_TeamInfo[i].team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + i));
        ResourceManager_InitHeatMaps(i);
    }

    ResourceManager_InitHeatMaps(PLAYER_TEAM_ALIEN);
    GameManager_UpdateHumanPlayerCount();

    GameManager_PlayerTeam = PLAYER_TEAM_ALIEN;

    for (int32_t i = PLAYER_TEAM_MAX - 1; i >= PLAYER_TEAM_RED; --i) {
        if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_PLAYER && !GameManager_PlayerMissionSetup(i)) {
            return false;
        }
    }

    if (GameManager_HumanPlayerCount && !GameManager_HandleProximityOverlaps()) {
        return false;
    }

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_COMPUTER) {
            if (GameManager_PlayerTeam == PLAYER_TEAM_ALIEN) {
                GameManager_PlayerTeam = i;
            }

            if (!GameManager_IsMapInitialized) {
                GameManager_InitMap();
            }

            if (Ai_SetupStrategy(i)) {
                if (Remote_IsNetworkGame) {
                    Remote_SendNetPacket_12(i);
                }
            }
        }
    }

    GameManager_PopulateMapWithResources();
    GameManager_PopulateMapWithAlienUnits(ini_get_setting(INI_ALIEN_SEPERATION), ini_get_setting(INI_ALIEN_UNIT_VALUE));

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        if (UnitsManager_TeamInfo[i].team_type != TEAM_TYPE_NONE) {
            if (!GameManager_IsMapInitialized) {
                GameManager_InitMap();
            }

            GameManager_ProcessTeamMissionSupplyUnits(i);
        }
    }

    GameManager_MapViewCenter = UnitsManager_TeamMissionSupplies[GameManager_PlayerTeam].starting_position;
    UnitsManager_TeamInfo[GameManager_PlayerTeam].camera_position = GameManager_MapViewCenter;

    return true;
}

Color* GameManager_MenuFadeOut(int32_t time_limit) {
    Color* palette;

    palette = new (std::nothrow) Color[PALETTE_STRIDE * PALETTE_SIZE];
    memcpy(palette, WindowManager_ColorPalette, PALETTE_STRIDE * PALETTE_SIZE);
    GameManager_MenuDeinitDisplayControls();
    WindowManager_FadeOut(time_limit);
    GameManager_FillOrRestoreWindow(WINDOW_MAIN_WINDOW, COLOR_BLACK, true);

    return palette;
}

void GameManager_InitLandingSequenceMenu(bool enable_controls) { GameManager_LandingSequence.Init(enable_controls); }

UnitInfo* GameManager_GetFirstRelevantUnit(uint16_t team) {
    UnitInfo* unit;

    unit = Access_GetFirstMiningStation(team);

    if (!unit) {
        unit = Access_GetFirstActiveUnit(GameManager_PlayerTeam, UnitsManager_MobileLandSeaUnits);
    }

    if (!unit) {
        unit = Access_GetFirstActiveUnit(GameManager_PlayerTeam, UnitsManager_MobileAirUnits);
    }

    if (!unit) {
        unit = Access_GetFirstActiveUnit(GameManager_PlayerTeam, UnitsManager_StationaryUnits);
    }

    return unit;
}

void GameManager_ColorEffect(struct ColorCycleData* color_cycle_table) {
    int32_t start_index;
    int32_t end_index;
    int32_t step_count;
    Color* start_address;
    Color* end_address;
    Color rgb_color[3];

    start_index = color_cycle_table->start_index;
    end_index = color_cycle_table->end_index;

    step_count = end_index - start_index;

    start_address = &WindowManager_ColorPalette[start_index * 3];

    if (step_count) {
        if (color_cycle_table->rotate_direction) {
            end_address = &WindowManager_ColorPalette[end_index * 3];

            memcpy(rgb_color, end_address, 3);
            memmove(&start_address[3], start_address, 3 * step_count);
            memcpy(start_address, rgb_color, 3);

        } else {
            end_address = &WindowManager_ColorPalette[end_index * 3];

            memcpy(rgb_color, start_address, 3);
            memmove(start_address, &start_address[3], 3 * step_count);
            memcpy(end_address, rgb_color, 3);
        }

    } else {
        if (GameManager_ColorCycleStep <= 7) {
            GameManager_ColorCycleStep = 63;

        } else {
            GameManager_ColorCycleStep /= 2;
        }

        start_address[1] = GameManager_ColorCycleStep;
    }

    for (int32_t i = start_index; i <= end_index; ++i) {
        Color_SetSystemPaletteEntry(i, WindowManager_ColorPalette[i * 3 + 0], WindowManager_ColorPalette[i * 3 + 1],
                                    WindowManager_ColorPalette[i * 3 + 2]);
    }
}

bool GameManager_IsUnitNextToPosition(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    int32_t grid_size;
    bool result;

    if (unit->flags & BUILDING) {
        grid_size = 2;

    } else {
        grid_size = 1;
    }

    if (unit->grid_x - 1 <= grid_x && unit->grid_x + grid_size >= grid_x && unit->grid_y - 1 <= grid_y &&
        unit->grid_y + grid_size >= grid_y) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

bool GameManager_IsInteractable(UnitInfo* unit) {
    bool result;

    if (unit->GetOrder() == ORDER_SENTRY || unit->GetOrder() == ORDER_IDLE || unit->GetOrder() == ORDER_DISABLE ||
        unit->GetOrder() == ORDER_EXPLODE || unit->GetOrderState() == ORDER_STATE_DESTROY ||
        unit->GetOrderState() == ORDER_STATE_READY_TO_EXECUTE_ORDER) {
        result = false;

    } else if (unit->GetOrder() == ORDER_BUILD || unit->GetOrder() == ORDER_CLEAR) {
        result = unit->GetOrderState() == ORDER_STATE_SELECT_SITE;

    } else if (unit->speed) {
        result = true;

    } else {
        ResourceID unit_type = unit->GetUnitType();
        int16_t grid_x = unit->grid_x;
        int16_t grid_y = unit->grid_y;
        SmartPointer<UnitValues> unit_values(unit->GetBaseValues());

        if (unit->shots > 0 && Access_FindReachableSpot(unit_type, unit, &grid_x, &grid_y,
                                                        unit_values->GetAttribute(ATTRIB_RANGE), 0, 1)) {
            result = true;

        } else if ((unit_type == SPLYTRCK || unit_type == CARGOSHP || unit_type == FUELTRCK || unit_type == GOLDTRCK ||
                    unit_type == REPAIR) &&
                   unit->storage && GameManager_IsUnitNextToPosition(unit, grid_x, grid_y)) {
            result = true;

        } else {
            result = false;
        }
    }

    return result;
}

void GameManager_UpdateHumanPlayerCount() {
    GameManager_HumanPlayerCount = 0;

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
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
    uint32_t time_stamp;

    top_window = WindowManager_GetWindow(WINDOW_TOP_INSTRUMENTS_WINDOW);
    bottom_window = WindowManager_GetWindow(WINDOW_BOTTOM_INSTRUMENTS_WINDOW);

    for (int32_t i = 0; i < 5; ++i) {
        time_stamp = timer_get();
        WindowManager_LoadSimpleImage(static_cast<ResourceID>(PNLSEQ_1 + i), top_window->window.ulx,
                                      top_window->window.uly, true);
        win_draw_rect(top_window->id, &top_window->window);

        if (static_cast<ResourceID>(BPNLSQ_1 + i) <= BPNLSQ_4) {
            WindowManager_LoadSimpleImage(static_cast<ResourceID>(BPNLSQ_1 + i), bottom_window->window.ulx,
                                          bottom_window->window.uly, true);
            win_draw_rect(bottom_window->id, &bottom_window->window);
        }

        process_bk();

        while ((timer_get() - time_stamp) < TIMER_FPS_TO_MS(24)) {
        }
    }
}

bool GameManager_LoadGame(int32_t save_slot, Color* palette_buffer) {
    WindowInfo* window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    bool load_successful;
    int32_t game_file_type;

    WindowManager_FadeOut(0);
    GameManager_FillOrRestoreWindow(WINDOW_MAIN_WINDOW, COLOR_BLACK, true);
    memcpy(WindowManager_ColorPalette, palette_buffer, PALETTE_STRIDE * PALETTE_SIZE);
    delete[] palette_buffer;

    load_successful = false;

    if (save_slot) {
        if (GameManager_SelectedUnit != nullptr) {
            SoundManager_PlaySfx(&*GameManager_SelectedUnit, SFX_TYPE_INVALID);
        }

        MessageManager_ClearMessageBox();

        GameManager_PlayFlic = false;
        GameManager_GameState = GAME_STATE_10;

        game_file_type = SaveLoadMenu_GetGameFileType();

        UnitsManager_GroundCoverUnits.Clear();
        UnitsManager_MobileLandSeaUnits.Clear();
        UnitsManager_ParticleUnits.Clear();
        UnitsManager_StationaryUnits.Clear();
        UnitsManager_MobileAirUnits.Clear();

        Hash_UnitHash.Clear();
        Hash_MapHash.Clear();

        Ai_Init();

        PathsManager_Clear();

        GameManager_LockedUnits.Clear();

        load_successful = SaveLoadMenu_Load(save_slot, game_file_type, true);

        if (load_successful) {
            if (Remote_IsNetworkGame) {
                Remote_WaitBeginTurnAcknowledge();
            }

            GameManager_LandingSequence.OpenPanel();
        }
    }

    if (!load_successful) {
        WindowManager_LoadBigImage(FRAMEPIC, window, window->width, false, true, -1, -1, false, true);
        GameManager_MenuInitButtons(false);
        GameManager_MenuDeinitButtons();
        win_draw(window->id);
    }

    if (load_successful) {
        GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, 4, 0);

    } else {
        GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, Gfx_ZoomLevel, 0);
    }

    GameManager_MenuAnimateDisplayControls();

    GameManager_MenuInitDisplayControls();

    GameManager_ProcessTick(true);

    WindowManager_FadeIn(0);

    SoundManager_PlayMusic(static_cast<ResourceID>(ini_get_setting(INI_WORLD) / 6 + SNOW_MSC), true);

    if (load_successful) {
        GameManager_UpdatePanelButtons(GameManager_PlayerTeam);
    }

    if (GameManager_SelectedUnit != nullptr) {
        GameManager_EnableMainMenu(&*GameManager_SelectedUnit);

    } else {
        GameManager_EnableMainMenu(nullptr);
    }

    GameManager_DrawTurnCounter(GameManager_TurnCounter);
    GameManager_DrawTurnTimer(GameManager_TurnTimerValue);

    return load_successful;
}

void GameManager_NotifyEvent(UnitInfo* unit, int32_t event) {
    ResourceID resource_id1;
    ResourceID resource_id2;
    char text[300];

    resource_id1 = INVALID_ID;

    GameManager_UnknownUnit3 = unit;

    GameManager_SpottedEnemyPosition.x = unit->grid_x;
    GameManager_SpottedEnemyPosition.y = unit->grid_y;

    switch (event) {
        case 0: {
            sprintf(text, GameManager_EventStrings_EnemySpotted[UnitsManager_BaseUnits[unit->GetUnitType()].gender],
                    UnitsManager_BaseUnits[unit->GetUnitType()].singular_name);

            if (!GameManager_IsInsideMapView(unit) && timer_elapsed_time(GameManager_NotifyTimeout) > 5000) {
                resource_id1 = V_M070;
                resource_id2 = V_F071;
            }

            GameManager_NotifyTimeout = timer_get();

        } break;

        case 1: {
            sprintf(text, unit->hits ? _(fd20) : _(73cc), UnitsManager_BaseUnits[unit->GetUnitType()].singular_name);

            if (unit->hits && !GameManager_IsInsideMapView(unit)) {
                resource_id1 = V_M229;
                resource_id2 = V_F232;

            } else if (!GameManager_IsInsideMapView(unit)) {
                GameManager_UnknownUnit3 = nullptr;

                resource_id1 = V_M234;
                resource_id2 = V_F236;
            }
        } break;

        case 2: {
            sprintf(text, _(3206), UnitsManager_BaseUnits[unit->GetUnitType()].singular_name);

            resource_id1 = V_M243;
            resource_id2 = V_F243;
        } break;

        case 3: {
            sprintf(text, _(20c3), UnitsManager_BaseUnits[unit->GetUnitType()].singular_name);

            resource_id1 = V_M249;
            resource_id2 = V_F249;
        } break;

        case 4: {
            sprintf(text, _(f900), UnitsManager_BaseUnits[unit->GetUnitType()].singular_name);

            resource_id1 = V_M012;
            resource_id2 = V_F012;
        } break;

        case 5: {
            sprintf(text, _(eb6c), UnitsManager_BaseUnits[unit->GetUnitType()].singular_name);

            resource_id1 = V_M012;
            resource_id2 = V_F012;
        } break;
    }

    if (GameManager_UnknownUnit3 != nullptr) {
        strcat(text, _(c719));
    }

    MessageManager_DrawMessage(text, 1, unit, GameManager_SpottedEnemyPosition);

    if (resource_id1 != INVALID_ID) {
        SoundManager_PlayVoice(resource_id1, resource_id2);
    }
}

void GameManager_SelectBuildSite(UnitInfo* unit) {
    while (unit->GetOrderState() == ORDER_STATE_IN_TRANSITION) {
        GameManager_ProcessTick(false);
    }

    MessageManager_ClearMessageBox();

    if (unit->GetOrderState() == ORDER_STATE_SELECT_SITE) {
        SoundManager_PlayVoice(V_M049, V_F050, -1);

        SDL_assert(GameManager_TempTape != nullptr);

        UnitsManager_DestroyUnit(&*GameManager_TempTape);
        GameManager_TempTape = nullptr;

        unit->GetBuildList().Clear();
        unit->SetOrder(unit->GetPriorOrder());
        unit->SetOrderState(unit->GetPriorOrderState());

    } else {
        unit->target_grid_x = unit->grid_x;
        unit->target_grid_y = unit->grid_y;

        UnitsManager_SetNewOrder(unit, ORDER_BUILD, ORDER_STATE_BUILD_CANCEL);

        if (GameManager_SelectedUnit == unit) {
            SoundManager_PlaySfx(unit, SFX_TYPE_POWER_CONSUMPTION_END);
        }
    }

    GameManager_MenuUnitSelect(unit);
}

void GameManager_ManagePlayerAction() {
    if (GameManager_SelectedUnit != nullptr && GameManager_SelectedUnit->GetOrderState() == ORDER_STATE_SELECT_SITE) {
        GameManager_SelectBuildSite(&*GameManager_SelectedUnit);
    }
}

bool GameManager_InitPopupButtons(UnitInfo* unit) {
    WindowInfo* main_map_window;
    WindowInfo* popups_window;
    bool result;

    main_map_window = WindowManager_GetWindow(WINDOW_MAIN_MAP);
    popups_window = WindowManager_GetWindow(WINDOW_POPUP_BUTTONS);

    if (unit->GetOrder() != ORDER_DISABLE && GameManager_IsInsideMapView(unit) &&
        (!(unit->flags & MOBILE_LAND_UNIT) || unit->GetOrderState() != ORDER_STATE_UNIT_READY)) {
        GameManager_PopupButtons.popup_count = 0;
        unit->popup->init(unit, &GameManager_PopupButtons);

        if (GameManager_PopupButtons.popup_count) {
            struct ImageSimpleHeader* image;
            int32_t ulx;
            int32_t uly;

            image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(UNTBTN_U));

            GameManager_PopupButtons.width = image->width + 6;
            GameManager_PopupButtons.height =
                image->height * GameManager_PopupButtons.popup_count + (GameManager_PopupButtons.popup_count + 1) * 3;

            ulx = (((unit->x + 32) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx;
            uly = ((unit->y * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor) - Gfx_MapWindowUly -
                  (GameManager_PopupButtons.height / 2);

            ulx += main_map_window->window.ulx;

            if ((GameManager_PopupButtons.width + ulx) > main_map_window->window.lrx) {
                ulx -= GameManager_PopupButtons.width + Gfx_ZoomLevel;
            }

            uly += main_map_window->window.uly;

            if (uly < main_map_window->window.uly) {
                uly = main_map_window->window.uly;

            } else if ((GameManager_PopupButtons.height + uly) > main_map_window->window.lry) {
                uly = main_map_window->window.lry - GameManager_PopupButtons.height;
            }

            popups_window->window.ulx = ulx;
            popups_window->window.uly = uly;
            popups_window->window.lrx = popups_window->window.ulx + GameManager_PopupButtons.width;
            popups_window->window.lry = popups_window->window.uly + GameManager_PopupButtons.height;

            GameManager_PopupButtons.ulx = ulx;
            GameManager_PopupButtons.uly = uly;

            ulx += 3;
            uly += 3;

            Text_SetFont(GNW_TEXT_FONT_2);

            for (int32_t i = 0; i < GameManager_PopupButtons.popup_count; ++i) {
                Button* button;
                ResourceID image_up;
                ResourceID image_down;

                if (GameManager_PopupButtons.state[i]) {
                    image_up = UNTBTN_D;
                    image_down = UNTBTN_U;
                } else {
                    image_up = UNTBTN_U;
                    image_down = UNTBTN_D;
                }

                button = new (std::nothrow) Button(image_up, image_down, ulx, uly);

                button->SetFlags(0x20);
                button->SetCaption(GameManager_PopupButtons.caption[i], -1, 2);
                button->SetRFunc(GameManager_PopupButtons.r_func[i], reinterpret_cast<intptr_t>(unit));
                button->RegisterButton(main_map_window->id);
                button->Enable();

                GameManager_PopupButtons.buttons[i] = button;
                uly += image->height + 3;
            }

            win_draw_rect(main_map_window->id, &popups_window->window);

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void GameManager_DeinitPopupButtons(bool clear_mouse_events) {
    if (GameManager_PopupButtons.popup_count) {
        WindowInfo* popups;
        WindowInfo* main_map;
        WindowInfo* main_window;
        Image* image;
        Rect bounds;

        main_window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
        image = new (std::nothrow) Image(GameManager_PopupButtons.ulx, GameManager_PopupButtons.uly,
                                         GameManager_PopupButtons.width, GameManager_PopupButtons.height);

        image->Copy(main_window);

        for (int32_t i = 0; i < GameManager_PopupButtons.popup_count; ++i) {
            delete GameManager_PopupButtons.buttons[i];
        }

        image->Write(main_window);
        delete image;

        main_map = WindowManager_GetWindow(WINDOW_MAIN_MAP);
        popups = WindowManager_GetWindow(WINDOW_POPUP_BUTTONS);

        bounds.ulx = (((popups->window.ulx - main_map->window.ulx) * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) +
                     GameManager_MapWindowDrawBounds.ulx;
        bounds.uly = (((popups->window.uly - main_map->window.uly) * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) +
                     GameManager_MapWindowDrawBounds.uly;
        bounds.lrx =
            (((popups->window.lrx - main_map->window.ulx) * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) + bounds.ulx;
        bounds.lry =
            (((popups->window.lry - main_map->window.uly) * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) + bounds.uly;

        GameManager_AddDrawBounds(&bounds);

        popups->window.ulx = -1;
        popups->window.uly = -1;
        popups->window.lrx = -1;
        popups->window.lry = -1;

        GameManager_PopupButtons.popup_count = 0;

        if (clear_mouse_events) {
            MouseEvent::Clear();
        }
    }
}

void GameManager_UpdateGridCenterOffset() {
    if (GameManager_MapViewCenter.x <= GameManager_GridCenterOffset.x) {
        GameManager_GridCenterOffset.x -= 4;

    } else if (GameManager_MapViewCenter.x >= GameManager_GridCenterOffset.x + (ResourceManager_MapSize.x / 2 - 1)) {
        GameManager_GridCenterOffset.x += 4;
    }

    if (GameManager_MapViewCenter.y <= GameManager_GridCenterOffset.y) {
        GameManager_GridCenterOffset.y -= 4;

    } else if (GameManager_MapViewCenter.y >= GameManager_GridCenterOffset.y + (ResourceManager_MapSize.y / 2 - 1)) {
        GameManager_GridCenterOffset.y += 4;
    }

    GameManager_GridCenterOffset.x =
        std::min((ResourceManager_MapSize.x / 2), std::max(0, static_cast<int32_t>(GameManager_GridCenterOffset.x)));

    GameManager_GridCenterOffset.y =
        std::min((ResourceManager_MapSize.y / 2), std::max(0, static_cast<int32_t>(GameManager_GridCenterOffset.y)));
}

void GameManager_UpdatePanelButtons(uint16_t team) {
    CTInfo* team_info;

    team_info = &UnitsManager_TeamInfo[team];

    Gfx_ZoomLevel = team_info->zoom_level;

    GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, Gfx_ZoomLevel, 0);
    GameManager_UpdateMainMapView(MAP_VIEW_CENTER, team_info->camera_position.x, team_info->camera_position.y);

    if (team_info->selected_unit != nullptr && team_info->selected_unit->hits) {
        GameManager_MenuUnitSelect(&*team_info->selected_unit);

    } else {
        GameManager_SelectNextUnit(1);
    }

    GameManager_MenuClickSurveyButton(team_info->display_button_survey);
    GameManager_MenuClickStatusButton(team_info->display_button_status);
    GameManager_MenuClickColorsButton(team_info->display_button_colors);
    GameManager_MenuClickHitsButton(team_info->display_button_hits);
    GameManager_MenuClickAmmoButton(team_info->display_button_ammo);
    GameManager_MenuClickNamesButton(team_info->display_button_names);
    GameManager_MenuClickRangeButton(team_info->display_button_range);
    GameManager_MenuClickScanButton(team_info->display_button_scan);
    GameManager_MenuClickGridButton(team_info->display_button_grid);
    GameManager_MenuClickMinimap2xButton(team_info->display_button_minimap_2x);
    GameManager_MenuClickMinimapTntButton(team_info->display_button_minimap_tnt);
    GameManager_MenuClickLockButton(GameManager_DisplayButtonLock);
}

void GameManager_MenuClickLockButton(bool rest_state) {
    GameManager_DisplayButtonLock = rest_state;

    GameManager_MenuItems[MENU_GUI_ITEM_LOCK_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_LOCK_BUTTON].button->SetRestState(rest_state);

    if (rest_state && GameManager_SelectedUnit != nullptr && GameManager_SelectedUnit->team != GameManager_PlayerTeam &&
        GameManager_LockedUnits.Find(*GameManager_SelectedUnit) == GameManager_LockedUnits.End()) {
        GameManager_LockedUnits.PushBack(*GameManager_SelectedUnit);
    }

    if (GameManager_DisplayButtonLock && !GameManager_DisplayButtonScan && !GameManager_DisplayButtonRange) {
        GameManager_MenuClickScanButton(true);
        GameManager_MenuClickRangeButton(true);
    }

    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickReportButton() {
    GameManager_MenuItems[MENU_GUI_ITEM_REPORTS_BUTTON].button->SetRestState(false);
    GameManager_DisableMainMenu();
    ReportMenu_Menu();
    Text_SetFont(GNW_TEXT_FONT_5);
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
    if (GameManager_GameState == GAME_STATE_7_SITE_SELECT) {
        HelpMenu_Menu(HELPMENU_SITE_SELECT_SETUP, WINDOW_MAIN_MAP);
    } else {
        GameManager_MenuItems[MENU_GUI_ITEM_HELP_BUTTON].button->SetRestState(false);

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
        GameManager_UpdateMainMapView(MAP_VIEW_CENTER, GameManager_SelectedUnit->grid_x,
                                      GameManager_SelectedUnit->grid_y);
    } else {
        GameManager_SelectNextUnit(1);
    }

    GameManager_UpdateGridCenterOffset();
}

void GameManager_MenuClickScanButton(bool rest_state) {
    GameManager_DisplayButtonScan = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_SCAN_BUTTON].disabled = rest_state;
    GameManager_MenuItems[MENU_GUI_ITEM_SCAN_BUTTON].button->SetRestState(rest_state);
    GameManager_UpdateDrawBounds();
}

void GameManager_MenuClickChatGoalButton() {
    GameManager_MenuItems[MENU_GUI_ITEM_CHAT_BUTTON].button->SetRestState(false);

    if (Remote_IsNetworkGame) {
        GameManager_DisableMainMenu();
        ChatMenu_Menu(GameManager_PlayerTeam);
        GameManager_EnableMainMenu(&*GameManager_SelectedUnit);

    } else if (GameManager_GameFileNumber) {
        int32_t game_file_type;
        SmartString filename;

        game_file_type = ini_get_setting(INI_GAME_FILE_TYPE);

        filename.Sprintf(20, "descr%i.%s", GameManager_GameFileNumber, SaveLoadMenu_SaveFileTypes[game_file_type]);

        auto fp{ResourceManager_OpenFileResource(filename.GetCStr(), ResourceType_Text)};

        if (fp) {
            int32_t text_size;
            const char* mission_title = "";
            int32_t mission_title_size;

            fseek(fp, 0, SEEK_END);
            text_size = ftell(fp);

            if (game_file_type == GAME_TYPE_TRAINING) {
                mission_title = SaveLoadMenu_TutorialTitles[GameManager_GameFileNumber - 1];

            } else if (game_file_type == GAME_TYPE_SCENARIO) {
                mission_title = SaveLoadMenu_ScenarioTitles[GameManager_GameFileNumber - 1];

            } else if (game_file_type == GAME_TYPE_CAMPAIGN) {
                mission_title = SaveLoadMenu_CampaignTitles[GameManager_GameFileNumber - 1];
            }

            mission_title_size = strlen(mission_title) + 2;

            auto text = std::make_unique<char[]>(text_size + mission_title_size + 5);

            memset(text.get(), '\0', text_size + mission_title_size + 5);

            strcpy(text.get(), mission_title);
            strcat(text.get(), "\n\n");

            fseek(fp, 0, SEEK_SET);
            fread(&text.get()[mission_title_size], sizeof(char), text_size, fp);
            fclose(fp);

            MessageManager_DrawMessage(text.get(), 0, 1);
        }
    }
}

void GameManager_MenuClickPreferencesButton() {
    GameManager_MenuItems[MENU_GUI_ITEM_PREFS_BUTTON].button->SetRestState(false);

    GameManager_DisableMainMenu();

    menu_preferences_window(GameManager_PlayerTeam);

    GameManager_RealTime = ini_get_setting(INI_REAL_TIME);
    GameManager_FastMovement = ini_get_setting(INI_FAST_MOVEMENT);
    GameManager_QuickScroll = (ini_get_setting(INI_QUICK_SCROLL) * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR;

    if (Remote_IsNetworkGame) {
        GameManager_PlayMode = ini_get_setting(INI_PLAY_MODE);

        Remote_SendNetPacket_17();
    }

    if (GameManager_AllVisible != ini_get_setting(INI_ALL_VISIBLE)) {
        Access_UpdateVisibilityStatus(ini_get_setting(INI_ALL_VISIBLE));
    }

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        UnitsManager_TeamInfo[team].team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team));
    }

    GameManager_EnableMainMenu(&*GameManager_SelectedUnit);
}

void GameManager_MenuClickFileButton(bool is_saving_allowed) {
    Color* palette_buffer;
    int32_t save_slot;

    if (GameManager_QuickBuildMenuActive) {
        UnitsManager_DestroyUnit(&*GameManager_QuickBuilderUnit);
        GameManager_QuickBuildMenuActive = false;
    }

    GameManager_DeinitPopupButtons(false);
    GameManager_ManagePlayerAction();

    GameManager_MenuItems[MENU_GUI_ITEM_FILES_BUTTON].button->SetRestState(false);
    palette_buffer = GameManager_MenuFadeOut();
    Cursor_SetCursor(CURSOR_HAND);
    save_slot = SaveLoadMenu_MenuLoop(is_saving_allowed);
    GameManager_LoadGame(save_slot, palette_buffer);
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

void GameManager_FlicButtonRFunction(ButtonID bid, intptr_t value) {
    if (!GameManager_TextEditUnitName && GameManager_SelectedUnit != nullptr &&
        GameManager_SelectedUnit->team == GameManager_PlayerTeam &&
        UnitsManager_TeamInfo[GameManager_PlayerTeam].team_type == TEAM_TYPE_PLAYER) {
        WindowInfo* window;
        Rect bounds;
        char text[40];

        window = WindowManager_GetWindow(WINDOW_CORNER_FLIC);
        GameManager_SelectedUnit->GetName(GameManager_UnitName, sizeof(GameManager_UnitName));
        GameManager_SelectedUnit->GetDisplayName(text, sizeof(text));

        Text_SetFont(GNW_TEXT_FONT_2);

        bounds.ulx = Text_GetWidth(text) - Text_GetWidth(GameManager_UnitName);
        bounds.uly = 0;
        bounds.lrx = window->window.lrx - window->window.ulx;
        bounds.lry = Text_GetHeight();

        GameManager_MenuDisplayControls[MENU_DISPLAY_CONTROL_CORNER_FLIC].image->Write(window, &bounds);
        GameManager_TextEditUnitName =
            new (std::nothrow) TextEdit(window, GameManager_UnitName, 30, bounds.ulx, 0, bounds.lrx - bounds.ulx,
                                        Text_GetHeight(), 2, GNW_TEXT_FONT_2);

        GameManager_TextEditUnitName->LoadBgImage();

        Text_Blit(&window->buffer[bounds.ulx], GameManager_UnitName, bounds.lrx - bounds.ulx, window->width,
                  COLOR_GREEN);

        GameManager_TextEditUnitName->EnterTextEditField();

        bounds.ulx += window->window.ulx;
        bounds.uly += window->window.uly;
        bounds.lrx += window->window.ulx;
        bounds.lry += window->window.uly;

        win_draw_rect(window->id, &bounds);
    }
}

void GameManager_SaveLoadGame(bool save_load_mode) {
    if (SaveLoadMenu_SaveSlot != -1) {
        struct SaveFormatHeader save_file_header;
        char file_name[16];
        int32_t save_type;
        SmartString string;

        save_type = SaveLoadMenu_GetGameFileType();

        sprintf(file_name, "save%i.%s", SaveLoadMenu_SaveSlot, SaveLoadMenu_SaveFileTypes[save_type]);
        ResourceManager_ToUpperCase(file_name);

        if (SaveLoad_GetSaveFileInfo(SaveLoadMenu_SaveSlot, save_type, save_file_header, false)) {
            string.Sprintf(200, save_load_mode ? _(e285) : _(470e), file_name, save_file_header.save_name);

            if (OKCancelMenu_Menu(string.GetCStr())) {
                if (save_load_mode) {
                    if (Remote_IsNetworkGame) {
                        Remote_SendNetPacket_16(file_name, save_file_header.save_name);
                    }

                    SaveLoadMenu_Save(file_name, save_file_header.save_name, true);
                    MessageManager_DrawMessage(_(f640), 1, 0);

                } else {
                    Color* palette_buffer;

                    palette_buffer = GameManager_MenuFadeOut();
                    GameManager_LoadGame(SaveLoadMenu_SaveSlot, palette_buffer);
                }
            }
        }

    } else {
        GameManager_MenuClickFileButton(save_load_mode);
    }
}

void GameManager_MenuInitDisplayControls() {
    WindowInfo* window;
    int32_t width;
    int32_t height;

    window = WindowManager_GetWindow(WINDOW_CORNER_FLIC);

    Gamemanager_FlicButton =
        new (std::nothrow) Button(window->window.ulx, window->window.uly, window->window.lrx - window->window.ulx, 14);
    Gamemanager_FlicButton->SetRFunc(GameManager_FlicButtonRFunction,
                                     reinterpret_cast<intptr_t>(Gamemanager_FlicButton));
    Gamemanager_FlicButton->RegisterButton(window->id);
    GameManager_MenuInitButtons(true);

    for (auto& control : GameManager_MenuDisplayControls) {
        window = WindowManager_GetWindow(control.window_id);
        width = window->window.lrx - window->window.ulx + 1;
        height = window->window.lry - window->window.uly + 1;
        control.image = new (std::nothrow) Image(0, 0, width, height);
        control.image->Copy(window);

        if (control.resource_id == INVALID_ID) {
            control.button = nullptr;
        } else {
            control.button = new (std::nothrow)
                Button(control.resource_id, control.resource_id, window->window.ulx, window->window.uly);
            control.button->SetFlags(0x20);
            control.button->RegisterButton(window->id);
            control.button->Enable();
        }
    }

    GameManager_DisplayControlsInitialized = true;
}

void GameManager_DrawMouseCoordinates(int32_t x, int32_t y) {
    char text[10];

    sprintf(text, "%3.3i-%3.3i", x, y);
    GameManager_DrawDisplayPanel(MENU_DISPLAY_CONTROL_COORDINATES, text, 0xA2, 21);
}

bool GameManager_HandleProximityOverlaps() {
    bool flag;

    do {
        flag = false;

        for (int32_t team = PLAYER_TEAM_MAX - 1; team >= PLAYER_TEAM_RED; --team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER &&
                (UnitsManager_TeamMissionSupplies[team].proximity_alert_ack == 2 ||
                 UnitsManager_TeamMissionSupplies[team].proximity_alert_ack == 3)) {
                flag = true;

                GameManager_PlayerTeam = team;
                GameManager_ActiveTurnTeam = team;

                GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, 4, 0, false);
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

void GameManager_SetUnitOrder(const UnitOrderType order, const UnitOrderStateType state, UnitInfo* unit, int32_t grid_x,
                              int32_t grid_y) {
    GameManager_DeinitPopupButtons(false);

    if (order == ORDER_MOVE_TO_ATTACK) {
        UnitInfo* target = Access_GetAttackTarget(unit, grid_x, grid_y);

        if (target) {
            unit->SetEnemy(target);
            unit->path = nullptr;

        } else {
            if (unit->shots > 0) {
                unit->target_grid_x = grid_x;
                unit->target_grid_y = grid_y;

                UnitsManager_SetNewOrder(unit, ORDER_FIRE, ORDER_STATE_INIT);
            }

            return;
        }
    }

    unit->target_grid_x = grid_x;
    unit->target_grid_y = grid_y;

    if (unit->GetOrder() == ORDER_MOVE || unit->GetOrder() == ORDER_MOVE_TO_UNIT ||
        unit->GetOrder() == ORDER_MOVE_TO_ATTACK) {
        unit->SetOrder(unit->GetPriorOrder());
        unit->SetOrderState(unit->GetPriorOrderState());
    }

    if (order == ORDER_MOVE_TO_UNIT) {
        unit->ClearUnitList();
    }

    unit->auto_survey = false;
    unit->RemoveTasks();
    UnitsManager_SetNewOrder(unit, order, state);

    if (GameManager_SelectedUnit == unit) {
        GameManager_UpdateInfoDisplay(&*GameManager_SelectedUnit);
    }
}

bool GameManager_IsValidStartingPosition(int32_t grid_x, int32_t grid_y) {
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

uint8_t GameManager_GetWindowCursor(int32_t grid_x, int32_t grid_y) {
    UnitInfo* team_unit = Access_GetTeamUnit(grid_x, grid_y, GameManager_PlayerTeam, SELECTABLE);
    UnitInfo* unit_under_cursor = team_unit;

    if (!unit_under_cursor) {
        unit_under_cursor = Access_GetEnemyUnit(GameManager_PlayerTeam, grid_x, grid_y, SELECTABLE);
    }

    if (GameManager_UnitUnderMouseCursor != unit_under_cursor) {
        char text[100];

        text[0] = '\0';

        if (unit_under_cursor) {
            unit_under_cursor->GetDisplayName(text, sizeof(text));
        }

        GameManager_DrawDisplayPanel(MENU_DISPLAY_CONTROL_UNIT_DESCRIPTION, text, 0xA2);

        GameManager_UnitUnderMouseCursor = unit_under_cursor;
    }

    GameManager_MousePosition.x = grid_x;
    GameManager_MousePosition.y = grid_y;

    if (GameManager_MousePosition.x >= ResourceManager_MapSize.x ||
        GameManager_MousePosition.y >= ResourceManager_MapSize.y) {
        return CURSOR_HAND;
    }

    if ((GameManager_GameState == GAME_STATE_9_END_TURN && GameManager_PlayMode != PLAY_MODE_SIMULTANEOUS_MOVES) ||
        UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type != TEAM_TYPE_PLAYER) {
        uint8_t result;

        if (team_unit) {
            result = CURSOR_FRIEND;

        } else {
            result = CURSOR_UNIT_NO_GO;
        }

        return result;
    }

    if (GameManager_QuickBuildMenuActive) {
        return CURSOR_HAND;
    }

    if (GameManager_GameState == GAME_STATE_7_SITE_SELECT) {
        uint8_t result;

        if (GameManager_IsValidStartingPosition(grid_x, grid_y)) {
            result = CURSOR_UNIT_GO;

        } else if (GameManager_MousePosition.x >= ResourceManager_MapSize.x ||
                   GameManager_MousePosition.y >= ResourceManager_MapSize.y) {
            result = CURSOR_HAND;

        } else {
            result = CURSOR_UNIT_NO_GO;
        }

        return result;
    }

    if (GameManager_RenderState != 0) {
        if (GameManager_MousePosition2.x != GameManager_MouseX || GameManager_MousePosition2.y != GameManager_MouseY) {
            if (GameManager_RenderState == 1) {
                if ((labs(GameManager_MousePosition2.x - GameManager_MouseX) > 16) ||
                    (labs(GameManager_MousePosition2.y - GameManager_MouseY) > 16)) {
                    GameManager_RenderState = 2;
                }
            }

            if (GameManager_RenderState == 2) {
                GameManager_MousePosition2.x = GameManager_MouseX;
                GameManager_MousePosition2.y = GameManager_MouseY;

                GameManager_RenderEnable = true;
                GameManager_RenderFlag1 = true;

                GameManager_ManagePlayerAction();
            }
        }

        if (GameManager_RenderState == 2) {
            return CURSOR_HAND;
        }
    }

    if (GameManager_SelectedUnit == nullptr ||
        (GameManager_SelectedUnit->GetOrder() == ORDER_MOVE &&
         GameManager_SelectedUnit->GetOrderState() != ORDER_STATE_EXECUTING_ORDER) ||
        (GameManager_SelectedUnit->GetOrder() == ORDER_MOVE_TO_UNIT &&
         GameManager_SelectedUnit->GetOrderState() != ORDER_STATE_EXECUTING_ORDER) ||
        (GameManager_SelectedUnit->GetOrder() == ORDER_MOVE_TO_ATTACK &&
         GameManager_SelectedUnit->GetOrderState() != ORDER_STATE_EXECUTING_ORDER) ||
        GameManager_SelectedUnit->GetOrder() == ORDER_FIRE || GameManager_SelectedUnit->GetOrder() == ORDER_EXPLODE ||
        GameManager_SelectedUnit->GetOrderState() == ORDER_STATE_DESTROY ||
        GameManager_SelectedUnit->GetOrder() == ORDER_DISABLE ||
        GameManager_SelectedUnit->GetOrder() == ORDER_AWAIT_DISABLE_UNIT ||
        GameManager_SelectedUnit->GetOrder() == ORDER_AWAIT_STEAL_UNIT ||
        GameManager_SelectedUnit->GetOrder() == ORDER_LOAD ||
        GameManager_SelectedUnit->GetOrder() == ORDER_AWAIT_SCALING ||
        GameManager_SelectedUnit->team != GameManager_PlayerTeam) {
        uint8_t result;

        if (GameManager_SelectedUnit != nullptr && GameManager_SelectedUnit == team_unit &&
            GameManager_SelectedUnit->GetOrder() != ORDER_DISABLE) {
            result = CURSOR_UNIT_NO_GO;

        } else if (GameManager_SelectedUnit != nullptr && team_unit && GameManager_SelectedUnit->GetUnitList() &&
                   GameManager_SelectedUnit->GetUnitList() == team_unit->GetUnitList()) {
            result = CURSOR_UNIT_NO_GO;

        } else if (team_unit) {
            result = CURSOR_FRIEND;

        } else {
            result = CURSOR_UNIT_NO_GO;
        }

        return result;
    }

    if (Access_IsGroupOrderInterrupted(&*GameManager_SelectedUnit)) {
        uint8_t result;

        if (team_unit && team_unit->GetUnitList() == GameManager_SelectedUnit->GetUnitList()) {
            result = CURSOR_UNIT_NO_GO;

        } else if (team_unit) {
            result = CURSOR_FRIEND;

        } else {
            result = CURSOR_UNIT_NO_GO;
        }

        return result;
    }

    if ((GameManager_SelectedUnit->GetOrder() == ORDER_BUILD ||
         GameManager_SelectedUnit->GetOrder() == ORDER_AWAIT_TAPE_POSITIONING) &&
        (GameManager_SelectedUnit->flags & MOBILE_LAND_UNIT)) {
        return GameManager_GetBuilderUnitCursor(&*GameManager_SelectedUnit, grid_x, grid_y, team_unit);
    }

    if (GameManager_SelectedUnit->GetOrder() == ORDER_IDLE) {
        return GameManager_GetAirUnitCursor(&*GameManager_SelectedUnit, grid_x, grid_y, team_unit);
    }

    if (GameManager_SelectedUnit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER) {
        UnitInfo* enemy;

        enemy = Access_GetEnemyUnit(GameManager_PlayerTeam, grid_x, grid_y, SELECTABLE);

        if (enemy && enemy->GetUnitType() != CNCT_4W &&
            (!(enemy->flags & GROUND_COVER) || enemy->GetUnitType() == LANDMINE || enemy->GetUnitType() == SEAMINE) &&
            Access_IsValidAttackTarget(GameManager_SelectedUnit.Get(), enemy)) {
            uint8_t result;

            result = GameManager_GetMilitaryCursor(enemy, grid_x, grid_y);

            if (result != CURSOR_UNIT_NO_GO || !team_unit) {
                return result;
            }
        }
    }

    if (GameManager_SelectedUnit->targeting_mode) {
        uint8_t result;

        if (GameManager_SelectedUnit == team_unit) {
            result = CURSOR_FRIEND;

        } else if (GameManager_SelectedUnit->shots == 0) {
            result = CURSOR_UNIT_NO_GO;

        } else if (!Access_IsWithinAttackRange(&*GameManager_SelectedUnit, grid_x, grid_y,
                                               GameManager_SelectedUnit->GetBaseValues()->GetAttribute(ATTRIB_RANGE))) {
            result = CURSOR_UNIT_NO_GO;

        } else if (team_unit &&
                   !Access_IsValidAttackTarget(&*GameManager_SelectedUnit, team_unit, Point(grid_x, grid_y))) {
            result = CURSOR_UNIT_NO_GO;

        } else {
            unit_under_cursor = team_unit;

            if (!unit_under_cursor) {
                unit_under_cursor = Access_GetEnemyUnit(GameManager_PlayerTeam, grid_x, grid_y, SELECTABLE);
            }

            Cursor_DrawAttackPowerCursor(&*GameManager_SelectedUnit, unit_under_cursor, CURSOR_ENEMY);

            result = CURSOR_ENEMY;
        }

        return result;
    }

    if (GameManager_SelectedUnit->enter_mode) {
        uint8_t result;

        if (GameManager_SelectedUnit == team_unit) {
            result = CURSOR_FRIEND;

        } else {
            if (Access_GetReceiverUnit(&*GameManager_SelectedUnit, grid_x, grid_y)) {
                result = CURSOR_UNIT_GO;

            } else {
                result = CURSOR_UNIT_NO_GO;
            }
        }

        return result;
    }

    {
        uint32_t flags;
        uint8_t result;

        flags = GameManager_SelectedUnit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | STATIONARY);

        switch (flags) {
            case MOBILE_AIR_UNIT: {
                if (GameManager_SelectedUnit->cursor == CURSOR_ARROW_SW &&
                    GameManager_SelectedUnit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER) {
                    if (GameManager_SelectedUnit->storage <
                        GameManager_SelectedUnit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
                        GameManager_Unit = Access_GetTeamUnit(grid_x, grid_y, GameManager_PlayerTeam, MOBILE_LAND_UNIT);

                        if (GameManager_Unit && GameManager_Unit->GetOrder() != ORDER_CLEAR &&
                            GameManager_Unit->GetOrder() != ORDER_BUILD &&
                            !Access_GetTeamUnit(grid_x, grid_y, GameManager_PlayerTeam, MOBILE_AIR_UNIT)) {
                            return CURSOR_LOAD;
                        }
                    }
                }

                if (team_unit && team_unit->GetUnitType() != LANDPAD && !(team_unit->flags & GROUND_COVER)) {
                    result = CURSOR_FRIEND;

                } else if (GameManager_IsShiftKeyPressed) {
                    result = CURSOR_WAY;

                } else {
                    result = CURSOR_UNIT_GO;
                }
            } break;

            case MOBILE_LAND_UNIT:
            case MOBILE_SEA_UNIT:
            case (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT): {
                result = GameManager_GetUnitActionCursor(&*GameManager_SelectedUnit, grid_x, grid_y, team_unit);
            } break;

            case STATIONARY: {
                if (GameManager_SelectedUnit->cursor == CURSOR_ARROW_SW) {
                    if (GameManager_SelectedUnit->storage <
                        GameManager_SelectedUnit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
                        switch (GameManager_SelectedUnit->GetUnitType()) {
                            case DEPOT: {
                                GameManager_Unit =
                                    Access_GetTeamUnit(grid_x, grid_y, GameManager_PlayerTeam, MOBILE_LAND_UNIT);

                                if (GameManager_Unit && Task_IsReadyToTakeOrders(GameManager_Unit) &&
                                    GameManager_Unit->GetUnitType() != COMMANDO &&
                                    GameManager_Unit->GetUnitType() != INFANTRY) {
                                    return CURSOR_LOAD;
                                }
                            } break;

                            case HANGAR: {
                                GameManager_Unit =
                                    Access_GetTeamUnit(grid_x, grid_y, GameManager_PlayerTeam, MOBILE_AIR_UNIT);

                                if (GameManager_Unit && Task_IsReadyToTakeOrders(GameManager_Unit)) {
                                    return CURSOR_LOAD;
                                }
                            } break;

                            case DOCK: {
                                GameManager_Unit =
                                    Access_GetTeamUnit(grid_x, grid_y, GameManager_PlayerTeam, MOBILE_SEA_UNIT);

                                if (GameManager_Unit && !(GameManager_Unit->flags & MOBILE_LAND_UNIT) &&
                                    Task_IsReadyToTakeOrders(GameManager_Unit)) {
                                    return CURSOR_LOAD;
                                }
                            } break;

                            case BARRACKS: {
                                GameManager_Unit =
                                    Access_GetTeamUnit(grid_x, grid_y, GameManager_PlayerTeam, MOBILE_LAND_UNIT);

                                if (GameManager_Unit && (GameManager_Unit->GetUnitType() == COMMANDO ||
                                                         GameManager_Unit->GetUnitType() == INFANTRY)) {
                                    return CURSOR_LOAD;
                                }
                            } break;
                        }
                    }
                }

                if (GameManager_SelectedUnit->cursor != CURSOR_HIDDEN) {
                    GameManager_Unit = Access_GetUnit2(grid_x, grid_y, GameManager_PlayerTeam);

                    if (GameManager_Unit &&
                        GameManager_IsValidTransferTarget(GameManager_Unit, &*GameManager_SelectedUnit)) {
                        switch (GameManager_SelectedUnit->cursor) {
                            case CURSOR_ARROW_NE: {
                                if (UnitsManager_BaseUnits[GameManager_SelectedUnit->GetUnitType()].cargo_type ==
                                        UnitsManager_BaseUnits[GameManager_Unit->GetUnitType()].cargo_type &&
                                    GameManager_Unit->GetOrder() != ORDER_CLEAR &&
                                    GameManager_Unit->GetOrder() != ORDER_BUILD) {
                                    return CURSOR_TRANSFER;
                                }
                            } break;

                            case CURSOR_ARROW_S: {
                                if (GameManager_Unit->ammo <
                                    GameManager_Unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO)) {
                                    return CURSOR_RELOAD;
                                }
                            } break;
                        }
                    }
                }

                if (team_unit) {
                    return CURSOR_FRIEND;
                } else {
                    return CURSOR_UNIT_NO_GO;
                }
            } break;

            default: {
                result = CURSOR_UNIT_NO_GO;
            } break;
        }

        return result;
    }
}

void GameManager_PathBuild(UnitInfo* unit) {
    SDL_assert(GameManager_TempTape != nullptr);

    SoundManager_PlayVoice(V_M049, V_F050, -1);

    unit->target_grid_x = GameManager_TempTape->grid_x;
    unit->target_grid_y = GameManager_TempTape->grid_y;

    UnitsManager_DestroyUnit(&*GameManager_TempTape);
    GameManager_TempTape = nullptr;

    unit->path = nullptr;

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_38(unit);
    }

    if (unit->GetUnitType() == ENGINEER &&
        (unit->grid_x != unit->target_grid_x || unit->grid_y != unit->target_grid_y)) {
        unit->path = new (std::nothrow) GroundPath(unit->target_grid_x, unit->target_grid_y);

        unit->path->Path_vfunc17(unit->target_grid_x - unit->grid_x, unit->target_grid_y - unit->grid_y);

        if (Remote_IsNetworkGame) {
            Remote_SendNetPacket_38(unit);
        }

        GameManager_UpdateDrawBounds();
    }

    UnitsManager_StartBuild(unit);
    GameManager_AutoSelectNext(unit);
}

void GameManager_ReloadUnit(UnitInfo* unit1, UnitInfo* unit2) {
    if (unit2->GetOrder() == ORDER_DISABLE) {
        SoundManager_PlaySfx(NCANC0);
        MessageManager_DrawMessage(_(d984), 1, 0);

    } else if (unit1->storage) {
        unit1->SetParent(unit2);
        MessageManager_DrawMessage(_(1fd7), 0, 0);
        UnitsManager_SetNewOrder(unit1, ORDER_RELOAD, ORDER_STATE_INIT);
        SoundManager_PlayVoice(V_M085, V_F085);

    } else {
        SoundManager_PlaySfx(NCANC0);
        MessageManager_DrawMessage(_(930a), 1, 0);
    }
}

void GameManager_RepairUnit(UnitInfo* unit1, UnitInfo* unit2) {
    if (unit2->GetOrder() == ORDER_DISABLE) {
        SoundManager_PlaySfx(NCANC0);
        MessageManager_DrawMessage(_(7c90), 1, 0);

    } else if (unit1->storage) {
        unit1->SetParent(unit2);
        MessageManager_DrawMessage(_(5ae3), 0, 0);
        UnitsManager_SetNewOrder(unit1, ORDER_REPAIR, ORDER_STATE_INIT);
        SoundManager_PlayVoice(V_M210, V_F210);

    } else {
        SoundManager_PlaySfx(NCANC0);
        MessageManager_DrawMessage(_(c65e), 1, 0);
    }
}

void GameManager_TransferCargo(UnitInfo* unit1, UnitInfo* unit2) {
    if (unit2->GetOrder() == ORDER_DISABLE) {
        SoundManager_PlaySfx(NCANC0);
        MessageManager_DrawMessage(_(42b7), 1, 0);

    } else if (UnitsManager_BaseUnits[unit1->GetUnitType()].cargo_type ==
                   UnitsManager_BaseUnits[unit2->GetUnitType()].cargo_type ||
               (unit2->GetComplex() && (unit2->flags & STATIONARY) &&
                (unit2 = GameManager_GetUnitWithCargoType(
                     unit2->GetComplex(), UnitsManager_BaseUnits[unit1->GetUnitType()].cargo_type)) != nullptr)) {
        int32_t cargo_transferred;

        unit1->SetParent(unit2);

        cargo_transferred = TransferMenu_Menu(unit1);

        if (cargo_transferred) {
            char message[200];

            unit1->target_grid_x = cargo_transferred;

            UnitsManager_SetNewOrder(unit1, ORDER_TRANSFER, ORDER_STATE_INIT);

            cargo_transferred = labs(cargo_transferred);

            switch (UnitsManager_BaseUnits[unit1->GetUnitType()].cargo_type) {
                case CARGO_TYPE_RAW: {
                    sprintf(message, _(ea89), cargo_transferred);
                } break;

                case CARGO_TYPE_FUEL: {
                    sprintf(message, _(311d), cargo_transferred);
                } break;

                case CARGO_TYPE_GOLD: {
                    sprintf(message, _(9880), cargo_transferred);
                } break;
            }

            MessageManager_DrawMessage(message, 0, 0);

            SoundManager_PlayVoice(V_M224, V_F224);
        }
    }
}

void GameManager_StealUnit(UnitInfo* unit1, UnitInfo* unit2) {
    unit2 = Access_GetAttackTarget(unit1, unit2->grid_x, unit2->grid_y);

    if (unit2) {
        if (unit1->shots > 0) {
            unit1->SetParent(unit2);
            unit1->target_grid_x = (dos_rand() * 101) >> 15;

            UnitsManager_SetNewOrder(unit1, ORDER_AWAIT_STEAL_UNIT, ORDER_STATE_INIT);

        } else {
            MessageManager_DrawMessage(_(f37a), 0, 0);
        }
    }
}

void GameManager_DisableUnit(UnitInfo* unit1, UnitInfo* unit2) {
    unit2 = Access_GetAttackTarget(unit1, unit2->grid_x, unit2->grid_y);

    if (unit2) {
        if (unit1->shots > 0) {
            unit1->SetParent(unit2);
            unit1->target_grid_x = (dos_rand() * 101) >> 15;

            UnitsManager_SetNewOrder(unit1, ORDER_AWAIT_DISABLE_UNIT, ORDER_STATE_INIT);

        } else {
            MessageManager_DrawMessage(_(3e56), 0, 0);
        }
    }
}

void GameManager_FindValidStartingPosition(Point* position) {
    if (!GameManager_IsValidStartingPosition(position->x, position->y)) {
        for (int32_t range = 2;; range += 2) {
            --position->x;
            ++position->y;

            for (int32_t direction = 0; direction < 8; direction += 2) {
                for (int32_t i = 0; i < range; ++i) {
                    *position += Paths_8DirPointsArray[direction];

                    if (GameManager_IsValidStartingPosition(position->x, position->y)) {
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

    const int32_t max_resources{ini_get_setting(INI_MAX_RESOURCES)};
    const int32_t minimum_fuel{(max_resources * 8 + 10) / 20};
    const int32_t minimum_materials{(max_resources * 12 + 10) / 20};

    dos_srand(Remote_RngSeed);

    allocator_materials.PopulateCargoMap();
    allocator_fuel.PopulateCargoMap();
    allocator_gold.PopulateCargoMap();

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            GameManager_FindValidStartingPosition(&UnitsManager_TeamMissionSupplies[team].starting_position);
            allocator_materials.Optimize(UnitsManager_TeamMissionSupplies[team].starting_position, minimum_materials,
                                         100);
            allocator_fuel.Optimize(UnitsManager_TeamMissionSupplies[team].starting_position, minimum_fuel, 100);
        }
    }

    allocator_materials.SeparateResources(&allocator_fuel);

    allocator_materials.ConcentrateResources();
    allocator_fuel.ConcentrateResources();
    allocator_gold.ConcentrateResources();

    ResourceAllocator::SettleMinimumResourceLevels(ini_get_setting(INI_MIN_RESOURCES));
}

void GameManager_FindSpot(Point* point) {
    for (int32_t range = 2;; range += 2) {
        --point->x;
        ++point->y;

        for (int32_t direction = 0; direction < 8; direction += 2) {
            for (int32_t i = 0; i < range; ++i) {
                *point += Paths_8DirPointsArray[direction];

                if (point->x >= 1 && point->y >= 1 && point->x + 3 < ResourceManager_MapSize.x &&
                    point->y + 3 < ResourceManager_MapSize.y) {
                    bool is_found;

                    is_found = true;

                    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                            if (Access_GetDistance(
                                    UnitsManager_TeamMissionSupplies[team].starting_position.x - point->x,
                                    UnitsManager_TeamMissionSupplies[team].starting_position.y - point->y) < 256) {
                                is_found = false;
                            }
                        }
                    }

                    for (int32_t grid_x = point->x; grid_x < point->x + 2 && is_found; ++grid_x) {
                        for (int32_t grid_y = point->y; grid_y < point->y + 2 && is_found; ++grid_y) {
                            if (Access_GetModifiedSurfaceType(grid_x, grid_y) == SURFACE_TYPE_AIR) {
                                is_found = false;
                            }
                        }
                    }

                    if (is_found) {
                        return;
                    }
                }
            }
        }
    }
}

void GameManager_SpawnAlienDerelicts(Point point, int32_t alien_unit_value) {
    SmartPointer<UnitInfo> unit;
    Rect bounds;
    Point position;
    int32_t surface_type;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    GameManager_FindSpot(&point);

    unit = UnitsManager_DeployUnit(LRGSLAB, PLAYER_TEAM_ALIEN, nullptr, point.x, point.y, 0, false, true);

    unit =
        UnitsManager_DeployUnit(GameManager_AlienBuildings[(dos_rand() * std::size(GameManager_AlienBuildings)) >> 15],
                                PLAYER_TEAM_ALIEN, nullptr, point.x, point.y, 0, false, true);

    position.x = point.x - 1;
    position.y = point.y + 2;

    for (int32_t direction = 0; direction < 8; direction += 2) {
        for (int32_t step_count = 0; step_count < 3; ++step_count) {
            position += Paths_8DirPointsArray[direction];

            if (Access_IsInsideBounds(&bounds, &position)) {
                surface_type = Access_GetSurfaceType(position.x, position.y);

                if (surface_type == SURFACE_TYPE_WATER || surface_type == SURFACE_TYPE_COAST) {
                    unit = UnitsManager_DeployUnit(BRIDGE, PLAYER_TEAM_ALIEN, nullptr, position.x, position.y, 0, false,
                                                   true);
                }
            }
        }
    }

    {
        ObjectArray<Point> land_tiles;
        ObjectArray<Point> water_tiles;
        int32_t cost;
        int32_t value;
        int32_t index;

        cost = UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[PLAYER_TEAM_ALIEN], ALNTANK)
                   ->GetAttribute(ATTRIB_TURNS);

        for (int32_t offset = 2; alien_unit_value >= cost && offset > 0; --offset) {
            land_tiles.Clear();
            water_tiles.Clear();

            position.x = point.x - offset;
            position.y = point.y + offset + 1;

            for (int32_t direction = 0; direction < 8; direction += 2) {
                for (int32_t step_count = 0; step_count < 2 * offset + 1; ++step_count) {
                    position += Paths_8DirPointsArray[direction];

                    if (Access_IsInsideBounds(&bounds, &position)) {
                        surface_type = Access_GetModifiedSurfaceType(position.x, position.y);

                        if (surface_type == SURFACE_TYPE_WATER) {
                            water_tiles.Append(&position);

                        } else if (surface_type == SURFACE_TYPE_LAND) {
                            land_tiles.Append(&position);
                        }
                    }
                }
            }

            if (alien_unit_value < UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[PLAYER_TEAM_ALIEN], JUGGRNT)
                                       ->GetAttribute(ATTRIB_TURNS)) {
                value = 1;

            } else {
                value = 0;
            }

            if (alien_unit_value < value) {
                water_tiles.Clear();
            }

            while (alien_unit_value >= cost && (land_tiles.GetCount() + water_tiles.GetCount()) > 0) {
                if (((dos_rand() * (land_tiles.GetCount() + water_tiles.GetCount())) >> 15) + 1 <=
                    water_tiles.GetCount()) {
                    SDL_assert(water_tiles.GetCount() > 0);

                    index = (dos_rand() * water_tiles.GetCount()) >> 15;

                    SDL_assert(index >= 0 && index < water_tiles.GetCount());

                    position = *water_tiles[index];

                    water_tiles.Remove(index);

                    unit = UnitsManager_DeployUnit(JUGGRNT, PLAYER_TEAM_ALIEN, nullptr, position.x, position.y, 0,
                                                   false, true);

                } else {
                    int32_t alien_unit_index;

                    SDL_assert(land_tiles.GetCount() > 0);

                    index = (dos_rand() * land_tiles.GetCount()) >> 15;

                    SDL_assert(index >= 0 && index < land_tiles.GetCount());

                    position = *land_tiles[index];

                    land_tiles.Remove(index);

                    do {
                        alien_unit_index = (dos_rand() * std::size(GameManager_AlienUnits)) >> 15;
                    } while (UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[PLAYER_TEAM_ALIEN],
                                                               GameManager_AlienUnits[alien_unit_index])
                                 ->GetAttribute(ATTRIB_TURNS) > alien_unit_value);

                    unit = UnitsManager_DeployUnit(GameManager_AlienUnits[alien_unit_index], PLAYER_TEAM_ALIEN, nullptr,
                                                   position.x, position.y, 0, false, true);
                }

                alien_unit_value -= unit->GetBaseValues()->GetAttribute(ATTRIB_TURNS);

                if (alien_unit_value <
                    UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[PLAYER_TEAM_ALIEN], JUGGRNT)
                        ->GetAttribute(ATTRIB_TURNS)) {
                    value = 1;

                } else {
                    value = 0;
                }

                if (alien_unit_value < value) {
                    water_tiles.Clear();
                }
            }
        }
    }
}

void GameManager_PopulateMapWithAlienUnits(int32_t alien_seperation, int32_t alien_unit_value) {
    if (alien_unit_value > 0) {
        Point point1;
        Point point2;
        bool flag = false;

        point1.x = (((alien_seperation * 4) / 5 + 1) * dos_rand()) >> 15;
        point1.y = ((alien_seperation / 2 + 1) * dos_rand()) >> 15;

        for (int32_t i = point1.x; i < ResourceManager_MapSize.x; i += (alien_seperation * 4) / 5) {
            for (int32_t j = flag ? (alien_seperation / 2 + point1.y) : point1.y; j < ResourceManager_MapSize.y;
                 j += alien_seperation) {
                point2.x = (((((alien_seperation / 5) - ((-alien_seperation) / 5)) + 1) * dos_rand()) >> 15) +
                           ((-alien_seperation) / 5) + i;
                point2.y = (((((alien_seperation / 5) - ((-alien_seperation) / 5)) + 1) * dos_rand()) >> 15) +
                           ((-alien_seperation) / 5) + j;

                GameManager_SpawnAlienDerelicts(point2, alien_unit_value);
            }

            flag = !flag;
        }
    }
}

void GameManager_ProcessTeamMissionSupplyUnits(uint16_t team) {
    TeamMissionSupplies* supplies = &UnitsManager_TeamMissionSupplies[team];
    SmartObjectArray<ResourceID> units(supplies->units);
    SmartObjectArray<uint16_t> cargos(supplies->cargos);
    SmartPointer<UnitInfo> mining_station;
    SmartPointer<UnitInfo> power_generator;
    SmartPointer<UnitInfo> team_unit;
    UnitInfo unit;
    Point mining_station_location;
    Point power_generator_location;
    ResourceID unit_type;
    int16_t grid_x;
    int16_t grid_y;

    GameManager_GameState = GAME_STATE_12;

    UnitsManager_TeamInfo[team].team_units->SetGold(supplies->team_gold);
    UnitsManager_TeamInfo[team].stats_units_built += units.GetCount();

    mining_station_location = GameManager_GetStartPositionMiningStation(team);

    mining_station =
        UnitsManager_DeployUnit(MININGST, team, nullptr, mining_station_location.x, mining_station_location.y, 0);

    mining_station->storage = 0;

    ++UnitsManager_TeamInfo[team].stats_mines_built;
    ++UnitsManager_TeamInfo[team].stats_buildings_built;

    UnitsManager_DeployUnit(
        LRGSLAB, team, nullptr, mining_station_location.x, mining_station_location.y,
        (dos_rand() *
         reinterpret_cast<struct BaseUnitDataFile*>(UnitsManager_BaseUnits[LRGSLAB].data_buffer)->image_count) >>
            15);

    UnitsManager_SetInitialMining(&*mining_station, mining_station_location.x, mining_station_location.y);

    power_generator_location = GameManager_GetStartingPositionPowerGenerator(mining_station_location, team);

    power_generator = UnitsManager_DeployUnit(POWGEN, team, mining_station->GetComplex(), power_generator_location.x,
                                              power_generator_location.y, 0);

    power_generator->SetOrder(ORDER_POWER_ON);
    power_generator->SetOrderState(ORDER_STATE_INIT);

    UnitsManager_DeployUnit(
        SMLSLAB, team, nullptr, power_generator_location.x, power_generator_location.y,
        (dos_rand() *
         reinterpret_cast<struct BaseUnitDataFile*>(UnitsManager_BaseUnits[SMLSLAB].data_buffer)->image_count) >>
            15);

    UnitsManager_UpdateConnectors(&*power_generator);

    for (int32_t i = 0; i < units->GetCount(); ++i) {
        unit_type = *units[i];

        unit.team = team;
        unit.SetUnitType(unit_type);

        grid_x = mining_station_location.x;
        grid_y = mining_station_location.y;

        Access_FindReachableSpot(unit_type, &unit, &grid_x, &grid_y, ResourceManager_MapSize.x, 0, 0);

        team_unit = UnitsManager_DeployUnit(unit_type, team, nullptr, grid_x, grid_y, 0);

        team_unit->storage = *cargos[i];
    }

    GameManager_GameState = GAME_STATE_7_SITE_SELECT;
}

bool GameManager_SyncTurnTimer() {
    bool result;

    if (Remote_UpdatePauseTimer) {
        uint32_t time_stamp;
        uint32_t seconds_elapsed;

        time_stamp = timer_get();

        seconds_elapsed = (time_stamp - Remote_PauseTimeStamp) / (TIMER_FPS_TO_MS(1) - 1);

        if (seconds_elapsed) {
            Remote_PauseTimeStamp = time_stamp;
            GameManager_TurnTimerValue -= seconds_elapsed;

            if (GameManager_TurnTimerValue <= 0) {
                GameManager_UpdateTurnTimer(false, 0);
                GameManager_RequestMenuExit = true;
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

void GameManager_ProcessState(bool process_tick, bool clear_mouse_events) {
    if (!process_tick || GameManager_GameState == GAME_STATE_3_MAIN_MENU ||
        GameManager_GameState == GAME_STATE_7_SITE_SELECT) {
        if (GameManager_SyncTurnTimer()) {
            GameManager_DrawTurnTimer(GameManager_TurnTimerValue, true);
        }

        if (Remote_IsNetworkGame) {
            if (Remote_UiProcessTick()) {
                UnitsManager_ProcessOrders();
            }

        } else {
            TickTimer_SetLastTimeStamp(timer_get());
            TickTimer_RequestTimeLimitUpdate();
        }

        TickTimer_UpdateTimeLimit();
        PathsManager_EvaluateTiles();
        Ai_CheckReactions();

    } else {
        GameManager_ProcessTick(false);
    }

    if (Remote_IsNetworkGame) {
        if (GameManager_AreTeamsFinishedTurn()) {
            GameManager_RequestMenuExit = true;
        }
    }

    if (clear_mouse_events) {
        MouseEvent::Clear();
    }
}

bool GameManager_ProcessTick(bool render_screen) {
    uint32_t time_stamp;
    bool result;

    if (render_screen) {
        GameManager_UpdateDrawBounds();
    }

    if (GameManager_SyncTurnTimer()) {
        GameManager_DrawTurnTimer(GameManager_TurnTimerValue);
    }

    GameManager_AdvanceFlic();

    time_stamp = timer_get();

    for (int32_t i = sizeof(GameManager_ColorCycleTable) / sizeof(struct ColorCycleData) - 1; i >= 0; --i) {
        ColorCycleData* data;
        data = &GameManager_ColorCycleTable[i];

        if ((time_stamp - data->time_stamp) >= data->time_limit) {
            data->time_stamp = time_stamp;

            GameManager_ColorEffect(data);
        }
    }

    time_stamp = timer_get();

    if (!TickTimer_HaveTimeToThink(TIMER_FPS_TO_MS(24))) {
        if (GameManager_IsCheater && GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
            GameManager_PunishCheater();
        }

        if (Remote_IsNetworkGame) {
            Remote_Synchronize();
        }

        TickTimer_SetLastTimeStamp(time_stamp);
        TickTimer_RequestTimeLimitUpdate();

        if (GameManager_GameState != GAME_STATE_11) {
            UnitsManager_ProcessOrders();
        }

        GameManager_RenderMap();

        if (GameManager_GameState != GAME_STATE_11 && !UnitsManager_OrdersPending) {
            TickTimer_UpdateTimeLimit();

            if (GameManager_UpdateFlag) {
                Ai_CheckReactions();
            }

            GameManager_UpdateFlag = !GameManager_UpdateFlag;

            PathsManager_EvaluateTiles();
            Ai_CheckReactions();
            Ai_CheckEndTurn();
        }

        result = true;

    } else {
        if (render_screen || GameManager_RenderEnable || GameManager_GridOffset.x || GameManager_GridOffset.y) {
            GameManager_RenderMap();
        }

        if (Remote_IsNetworkGame) {
            Remote_ProcessNetPackets();
        }

        result = false;
    }

    return result;
}

void GameManager_GuiSwitchTeam(uint16_t team) {
    CTInfo* team_info;

    team_info = &UnitsManager_TeamInfo[team];

    team_info->selected_unit = GameManager_SelectedUnit;
    team_info->zoom_level = Gfx_ZoomLevel;
    team_info->camera_position.x = GameManager_MapViewCenter.x;
    team_info->camera_position.y = GameManager_MapViewCenter.y;
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
    if (!GameManager_IsMainMenuEnabled && GameManager_DisplayControlsInitialized) {
        Gamemanager_FlicButton->Enable();

        for (uint32_t i = 0; i < sizeof(GameManager_MenuItems) / sizeof(struct MenuGuiItem); ++i) {
            GameManager_MenuItems[i].button->SetRestState(GameManager_MenuItems[i].disabled);
            GameManager_MenuItems[i].button->Enable();
        }

        if (GameManager_PlayMode) {
            GameManager_MenuClickEndTurnButton(GameManager_GameState == GAME_STATE_8_IN_GAME);
        } else {
            GameManager_MenuClickEndTurnButton(UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type ==
                                               TEAM_TYPE_PLAYER);
        }

        if (GameManager_GameState != GAME_STATE_9_END_TURN || GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
            bool flag = true;

            for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER) {
                    flag = false;
                    break;
                }
            }

            if (UnitsManager_TeamInfo[GameManager_PlayerTeam].team_type == TEAM_TYPE_PLAYER || flag) {
                if (unit) {
                    GameManager_MenuUnitSelect(unit);
                }
            }
        }

        GameManager_IsMainMenuEnabled = true;
    }
}

void GameManager_DisableMainMenu() {
    if (GameManager_IsMainMenuEnabled) {
        GameManager_MenuDeleteFlic();

        Gamemanager_FlicButton->Disable();

        for (uint32_t i = 0; i < sizeof(GameManager_MenuItems) / sizeof(struct MenuGuiItem); ++i) {
            if (GameManager_MenuItems[i].button) {
                GameManager_MenuItems[i].button->Disable();
            }
        }

        GameManager_IsMainMenuEnabled = false;
    }
}

bool GameManager_ProcessTextInput(int32_t key) {
    bool result;

    if (GameManager_TextInput.GetLength() && key >= 0 && key <= 255) {
        if (GameManager_TextInput.GetLength() < 30) {
            switch (key) {
                case GNW_KB_KEY_BACKSPACE: {
                    GameManager_TextInput = GameManager_TextInput.Substr(0, GameManager_TextInput.GetLength() - 2);
                } break;

                case GNW_KB_KEY_RETURN: {
                    MessageManager_ClearMessageBox();
                    GameManager_ProcessCheatCodes();
                    GameManager_TextInput = "";
                } break;

                default: {
                    GameManager_TextInput += toupper(key);
                } break;
            }

            MessageManager_DrawMessage(GameManager_TextInput.GetCStr(), 0, 0);

            result = true;

        } else {
            GameManager_TextInput = "";

            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool GameManager_DebugDelayedEndTurn(SmartList<UnitInfo>& units) {
    bool result{false};

    for (SmartList<UnitInfo>::Iterator it = units.Begin(), it_end = units.End(); it != it_end; ++it) {
        if ((*it).GetOrder() == ORDER_FIRE || (*it).GetOrder() == ORDER_EXPLODE ||
            ((*it).GetOrder() == ORDER_MOVE && (*it).GetOrderState() != ORDER_STATE_EXECUTING_ORDER) ||
            ((*it).GetOrder() == ORDER_MOVE_TO_UNIT && (*it).GetOrderState() != ORDER_STATE_EXECUTING_ORDER)) {
            char text[200];

            sprintf(text, "End turn delayed because %s at [%i,%i] is %s",
                    UnitsManager_BaseUnits[(*it).GetUnitType()].singular_name, (*it).grid_x + 1, (*it).grid_y + 1,
                    GameManager_OrderStatusMessages[(*it).GetOrder()]);

            MessageManager_DrawMessage(text, 0, 0);

            result = true;
            break;
        }
    }

    return result;
}

void GameManager_ProcessKey() {
    CTInfo* team_info;
    int32_t key;

    team_info = &UnitsManager_TeamInfo[GameManager_PlayerTeam];
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
        if (UnitsManager_TeamInfo[GameManager_PlayerTeam].finished_turn) {
            for (int32_t team = 0; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER &&
                    !UnitsManager_TeamInfo[team].finished_turn) {
                    GameManager_RefreshOrders(team, true);
                }
            }

        } else if (UnitsManager_TeamInfo[GameManager_PlayerTeam].team_type == TEAM_TYPE_PLAYER ||
                   UnitsManager_TeamInfo[GameManager_PlayerTeam].team_type == TEAM_TYPE_COMPUTER) {
            GameManager_DisableMainMenu();

            if (MessageManager_MessageBox_IsActive) {
                key = GNW_KB_KEY_ESCAPE;

            } else if (!GameManager_QuickBuildMenuActive && !GameManager_Progress1) {
                key = 1003;
            }
        }
    }

    if (key > 0 && key < GNW_INPUT_PRESS) {
        GameManager_SiteSelectReleaseEvent = false;
    }

    switch (key) {
        case GNW_KB_KEY_KP_ENTER: {
            if (GameManager_GameState == GAME_STATE_7_SITE_SELECT) {
                MessageManager_ClearMessageBox();

                Ai_SelectStartingPosition(GameManager_ActiveTurnTeam);

                GameManager_MapViewCenter.x =
                    UnitsManager_TeamMissionSupplies[GameManager_ActiveTurnTeam].starting_position.x;
                GameManager_MapViewCenter.y =
                    UnitsManager_TeamMissionSupplies[GameManager_ActiveTurnTeam].starting_position.y;

                UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].camera_position.x = GameManager_MapViewCenter.x;
                UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].camera_position.y = GameManager_MapViewCenter.y;

                UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].zoom_level = 64;

                GameManager_GameState = GAME_STATE_13;

            } else if (GameManager_DemoMode) {
                GameManager_GameState = GAME_STATE_3_MAIN_MENU;
                return;
            }

            if (GameManager_IsMainMenuEnabled && !UnitsManager_TeamInfo[GameManager_PlayerTeam].finished_turn) {
                SoundManager_PlaySfx(MBUTT0);
                GameManager_ResetRenderState();

                if (!GameManager_RefreshOrders(GameManager_PlayerTeam, GameManager_RequestMenuExit)) {
                    GameManager_MenuClickEndTurnButton(true);
                }
            }
        } break;

        case GNW_KB_KEY_ESCAPE: {
            if (GameManager_GameState == GAME_STATE_7_SITE_SELECT) {
                GameManager_GameState = GAME_STATE_14;

            } else if (GameManager_QuickBuildMenuActive) {
#if !defined(NDEBUG)
                UnitsManager_DestroyUnit(GameManager_QuickBuilderUnit.Get());
                GameManager_QuickBuildMenuActive = false;

                GameManager_QuickBuildMenu();
#endif /* !defined(NDEBUG) */
            } else if (GameManager_Progress1) {
                GameManager_Progress1 = false;

            } else if (GameManager_PopupButtons.popup_count) {
                GameManager_DeinitPopupButtons(false);

            } else if (MessageManager_MessageBox_IsActive) {
                MessageManager_ClearMessageBox();

            } else if (GameManager_DemoMode) {
                GameManager_GameState = GAME_STATE_3_MAIN_MENU;

            } else if (OKCancelMenu_Menu(_(d2f1))) {
                GameManager_GameState = GAME_STATE_3_MAIN_MENU;
            }
        } break;

        case GNW_KB_KEY_SPACE: {
            if (GameManager_DemoMode) {
                GameManager_GameState = GAME_STATE_3_MAIN_MENU;
            }
        } break;

        case GNW_KB_KEY_KP_PLUS:
        case GNW_KB_KEY_EQUALS: {
            if (GameManager_IsMainMenuEnabled) {
                GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, Gfx_ZoomLevel + 1, 0, false);
            }
        } break;

        case GNW_KB_KEY_KP_MINUS: {
            if (GameManager_IsMainMenuEnabled) {
                GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, Gfx_ZoomLevel - 1, 0, false);
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
            SoundManager_PlaySfx(IGRID0);
            GameManager_MenuClickGridButton(!GameManager_DisplayButtonGrid);
        } break;

        case GNW_KB_KEY_LEFTBRACKET: {
            GameManager_TextInput = "[";
        } break;

        case GNW_KB_KEY_LALT_F:
        case 1004: {
            if (GameManager_IsMainMenuEnabled) {
                GameManager_MenuClickFileButton(true);
            }
        } break;

        case GNW_KB_KEY_LALT_L: {
            if (GameManager_IsMainMenuEnabled) {
                if (Remote_IsNetworkGame) {
                    MessageManager_DrawMessage(_(5504), 2, 1, true);

                } else {
                    GameManager_SaveLoadGame(false);
                }
            }
        } break;

        case GNW_KB_KEY_LALT_P: {
            PauseMenu_Menu();
        } break;

        case GNW_KB_KEY_LALT_S: {
            if (GameManager_IsMainMenuEnabled) {
                GameManager_SaveLoadGame(true);
            }
        } break;

        case GNW_KB_KEY_LALT_Z: {
#if !defined(NDEBUG)
            if (GameManager_IsMainMenuEnabled) {
                GameManager_DeinitPopupButtons(false);

                if (GameManager_QuickBuildMenuActive) {
                    UnitsManager_DestroyUnit(GameManager_QuickBuilderUnit.Get());
                    GameManager_QuickBuildMenuActive = false;
                }

                GameManager_QuickBuildMenu();
            }
#endif /* !defined(NDEBUG) */
        } break;

        case GNW_KB_KEY_LALT_X: {
            if (OKCancelMenu_Menu(_(ecbd))) {
                GameManager_GameState = GAME_STATE_3_MAIN_MENU;
                GameManager_DeinitPopupButtons(false);
            }
        } break;

        case GNW_KB_KEY_F1: {
            if ((GameManager_IsMainMenuEnabled ||
                 UnitsManager_TeamInfo[GameManager_PlayerTeam].team_type == TEAM_TYPE_PLAYER) &&
                GameManager_SpottedEnemyPosition.x != -1) {
                GameManager_ManagePlayerAction();
                GameManager_UpdateMainMapView(MAP_VIEW_CENTER, GameManager_SpottedEnemyPosition.x,
                                              GameManager_SpottedEnemyPosition.y);

                if (GameManager_UnknownUnit3 != nullptr &&
                    GameManager_UnknownUnit3->IsVisibleToTeam(GameManager_PlayerTeam)) {
                    MessageManager_ClearMessageBox();
                    GameManager_MenuUnitSelect(&*GameManager_UnknownUnit3);
                } else {
                    MessageManager_DrawMessage(_(10eb), 0, 0);
                }
            }
        } break;

        case GNW_KB_KEY_LALT_F1: {
#if !defined(NDEBUG)
            PathsManager_SetPathDebugMode();
#endif /* !defined(NDEBUG) */
        } break;

        case GNW_KB_KEY_LALT_F2: {
            /// Debug logs
        } break;

        case GNW_KB_KEY_LALT_F3: {
#if !defined(NDEBUG)
            TaskDebugger_SetDebugMode();
#endif /* !defined(NDEBUG) */
        } break;

        case GNW_KB_KEY_LALT_F4: {
#if !defined(NDEBUG)
            {
                if (!GameManager_DebugDelayedEndTurn(UnitsManager_MobileLandSeaUnits) &&
                    !GameManager_DebugDelayedEndTurn(UnitsManager_MobileAirUnits) &&
                    !GameManager_DebugDelayedEndTurn(UnitsManager_StationaryUnits) &&
                    !GameManager_DebugDelayedEndTurn(UnitsManager_GroundCoverUnits) &&
                    !GameManager_DebugDelayedEndTurn(UnitsManager_ParticleUnits)) {
                    MessageManager_DrawMessage("Turn end is NOT delayed by units.", 0, 0);
                }
            }
#endif /* !defined(NDEBUG) */
        } break;

        case GNW_KB_KEY_F5:
        case GNW_KB_KEY_F6:
        case GNW_KB_KEY_F7:
        case GNW_KB_KEY_F8: {
            if (GameManager_IsMainMenuEnabled) {
                if (team_info->screen_locations[key - GNW_KB_KEY_F5].x != -1) {
                    GameManager_UpdateMainMapView(MAP_VIEW_CENTER, team_info->screen_locations[key - GNW_KB_KEY_F5].x,
                                                  team_info->screen_locations[key - GNW_KB_KEY_F5].y);
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
            team_info->screen_locations[key - GNW_KB_KEY_LALT_F5].x = GameManager_MapViewCenter.x;
            team_info->screen_locations[key - GNW_KB_KEY_LALT_F5].y = GameManager_MapViewCenter.y;

            MessageManager_DrawMessage(_(7615), 0, 0);
        } break;

        case 1003: {
            SoundManager_PlaySfx(MBUTT0);
            GameManager_ResetRenderState();

            if (!GameManager_RefreshOrders(GameManager_PlayerTeam, GameManager_RequestMenuExit)) {
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
            SoundManager_PlaySfx(MBUTT0);
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
                UnitsManager_TeamInfo[GameManager_PlayerTeam].team_type == TEAM_TYPE_PLAYER) {
                Access_RenewAttackOrders(UnitsManager_MobileLandSeaUnits, GameManager_PlayerTeam);
                Access_RenewAttackOrders(UnitsManager_MobileAirUnits, GameManager_PlayerTeam);

            } else if (GameManager_SelectedUnit != nullptr &&
                       GameManager_SelectedUnit->team == GameManager_PlayerTeam &&
                       UnitsManager_TeamInfo[GameManager_PlayerTeam].team_type == TEAM_TYPE_PLAYER) {
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
            SoundManager_PlaySfx(ISURV0);
            GameManager_MenuClickSurveyButton(!GameManager_DisplayButtonSurvey);
        } break;

        case 1027: {
            SoundManager_PlaySfx(ISTAT0);
            GameManager_MenuClickStatusButton(!GameManager_DisplayButtonStatus);
        } break;

        case 1028: {
            SoundManager_PlaySfx(ICOLO0);
            GameManager_MenuClickColorsButton(!GameManager_DisplayButtonColors);
        } break;

        case 1029: {
            SoundManager_PlaySfx(IHITS0);
            GameManager_MenuClickHitsButton(!GameManager_DisplayButtonHits);
        } break;

        case 1030: {
            SoundManager_PlaySfx(IAMMO0);
            GameManager_MenuClickAmmoButton(!GameManager_DisplayButtonAmmo);
        } break;

        case 1031: {
            SoundManager_PlaySfx(IRANG0);
            GameManager_MenuClickRangeButton(!GameManager_DisplayButtonRange);
        } break;

        case 1032: {
            SoundManager_PlaySfx(IVISI0);
            GameManager_MenuClickScanButton(!GameManager_DisplayButtonScan);
        } break;

        case 1034: {
            SoundManager_PlaySfx(INAME0);
            GameManager_MenuClickNamesButton(!GameManager_DisplayButtonNames);
        } break;

        case 1036: {
            SoundManager_PlaySfx(IAMMO0);
            GameManager_MenuClickMinimap2xButton(!GameManager_DisplayButtonMinimap2x);
        } break;

        case 1037: {
            SoundManager_PlaySfx(IAMMO0);
            GameManager_MenuClickMinimapTntButton(!GameManager_DisplayButtonMinimapTnt);
        } break;

        default: {
            if (!GameManager_ProcessPopupMenuInput(key) && key >= GNW_INPUT_PRESS &&
                !GameManager_SiteSelectReleaseEvent) {
                if (GameManager_GameState == GAME_STATE_7_SITE_SELECT) {
                    if (key == GNW_KB_KEY_SHIFT_DIVIDE && GameManager_LandingSequence.button_1) {
                        GameManager_LandingSequence.button_1->PlaySound();

                    } else if (GameManager_LandingSequence.button_2) {
                        GameManager_LandingSequence.button_2->PlaySound();
                    }

                } else {
                    SoundManager_PlaySfx(MBUTT0);
                }

                GameManager_SiteSelectReleaseEvent = true;
            }
        } break;
    }
}

int32_t GameManager_GetBuilderUnitCursor(UnitInfo* unit1, int32_t grid_x, int32_t grid_y, UnitInfo* unit2) {
    int32_t result;

    if (unit1->GetOrderState() == ORDER_STATE_UNIT_READY) {
        if (unit1->GetParent() && GameManager_IsUnitNextToPosition(unit1->GetParent(), grid_x, grid_y) &&
            Access_IsAccessible(unit1->GetUnitType(), GameManager_PlayerTeam, grid_x, grid_y,
                                AccessModifier_SameClassBlocks)) {
            result = CURSOR_UNIT_GO;

        } else if (unit2) {
            result = CURSOR_FRIEND;

        } else {
            result = CURSOR_UNIT_NO_GO;
        }

    } else if (unit1->GetOrderState() == ORDER_STATE_SELECT_SITE) {
        int32_t unit_grid_x;
        int32_t unit_grid_y;
        int32_t target_grid_x;
        int32_t target_grid_y;

        unit_grid_x = unit1->grid_x;
        unit_grid_y = unit1->grid_y;

        if (unit1->GetUnitType() == ENGINEER) {
            target_grid_x = grid_x;
            target_grid_y = grid_y;

        } else {
            if (unit_grid_x <= grid_x) {
                if (unit_grid_x >= grid_x) {
                    target_grid_x = GameManager_TempTape->grid_x;

                } else {
                    target_grid_x = unit_grid_x;
                }

            } else {
                target_grid_x = unit_grid_x - 1;
            }

            if (unit_grid_y <= grid_y) {
                if (unit_grid_y >= grid_y) {
                    target_grid_y = GameManager_TempTape->grid_y;

                } else {
                    target_grid_y = unit_grid_y;
                }

            } else {
                target_grid_y = unit_grid_y - 1;
            }
        }

        if (unit1->GetUnitType() == MASTER) {
            if (UnitsManager_IsMasterBuilderPlaceable(unit1, target_grid_x, target_grid_y)) {
                UnitsManager_MoveUnit(&*GameManager_TempTape, target_grid_x, target_grid_y);

                if (target_grid_x <= grid_x && target_grid_x + 1 >= grid_x && target_grid_y <= grid_y &&
                    target_grid_y + 1 >= grid_y) {
                    result = CURSOR_MAP2;

                } else {
                    result = CURSOR_UNIT_NO_GO;
                }

            } else {
                result = CURSOR_UNIT_NO_GO;
            }

        } else if (UnitsManager_MoveUnitAndParent(&*GameManager_TempTape, target_grid_x, target_grid_y)) {
            if (unit1->GetUnitType() == ENGINEER) {
                if (grid_x == unit_grid_x || grid_y == unit_grid_y) {
                    UnitsManager_MoveUnit(&*GameManager_TempTape, target_grid_x, target_grid_y);

                    result = CURSOR_MAP2;

                } else {
                    result = CURSOR_UNIT_NO_GO;
                }

            } else {
                UnitsManager_MoveUnit(&*GameManager_TempTape, target_grid_x, target_grid_y);

                if (target_grid_x <= grid_x && target_grid_x + 1 >= grid_x && target_grid_y <= grid_y &&
                    target_grid_y + 1 >= grid_y) {
                    result = CURSOR_MAP2;

                } else {
                    result = CURSOR_UNIT_NO_GO;
                }
            }

        } else {
            result = CURSOR_UNIT_NO_GO;
        }

    } else if (unit2) {
        result = CURSOR_FRIEND;

    } else {
        result = CURSOR_UNIT_NO_GO;
    }

    return result;
}

int32_t GameManager_GetAirUnitCursor(UnitInfo* unit1, int32_t grid_x, int32_t grid_y, UnitInfo* unit2) {
    UnitInfo* parent;
    int32_t result;

    parent = unit1->GetParent();

    if (parent != nullptr && GameManager_IsUnitNextToPosition(parent, grid_x, grid_y) &&
        Access_IsAccessible(unit1->GetUnitType(), GameManager_PlayerTeam, grid_x, grid_y,
                            AccessModifier_SameClassBlocks)) {
        if ((unit1->flags & MOBILE_AIR_UNIT) && parent->grid_x <= grid_x && (parent->grid_x) + 1 >= grid_x &&
            parent->grid_y <= grid_y && (parent->grid_y) + 1 >= grid_y) {
            result = CURSOR_FRIEND;

        } else {
            result = CURSOR_ACTIVATE;
        }

    } else if (unit2) {
        result = CURSOR_FRIEND;

    } else {
        result = CURSOR_UNIT_NO_GO;
    }

    return result;
}

bool GameManager_IsUnitNotInAir(UnitInfo* unit) {
    return !(unit->flags & MOBILE_AIR_UNIT) || !(unit->flags & HOVERING);
}

UnitInfo* GameManager_GetUnitWithCargoType(Complex* complex, int32_t cargo_type) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetComplex() == complex && UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type == cargo_type) {
            return &(*it);
        }
    }

    return nullptr;
}

int32_t GameManager_GetUnitActionCursor(UnitInfo* unit1, int32_t grid_x, int32_t grid_y, UnitInfo* unit2) {
    int32_t result;

    if (Access_IsAccessible(unit1->GetUnitType(), GameManager_PlayerTeam, grid_x, grid_y,
                            AccessModifier_SameClassBlocks)) {
        if (unit2 && (unit2->flags & MOBILE_AIR_UNIT)) {
            result = CURSOR_FRIEND;

        } else if (unit1->GetOrder() == ORDER_CLEAR) {
            result = CURSOR_UNIT_NO_GO;

        } else if (GameManager_IsShiftKeyPressed) {
            result = CURSOR_WAY;

        } else {
            result = CURSOR_UNIT_GO;
        }

    } else if (unit2) {
        if (unit1->cursor == CURSOR_ARROW_SW && unit1->storage < unit1->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
            GameManager_Unit = Access_GetTeamUnit(grid_x, grid_y, GameManager_PlayerTeam, MOBILE_LAND_UNIT);

            if (GameManager_Unit && Task_IsReadyToTakeOrders(GameManager_Unit)) {
                if (unit1->GetUnitType() == CLNTRANS &&
                    (GameManager_Unit->GetUnitType() == COMMANDO || GameManager_Unit->GetUnitType() == INFANTRY)) {
                    result = CURSOR_LOAD;

                } else if (unit1->GetUnitType() == SEATRANS) {
                    result = CURSOR_LOAD;

                } else if (unit2) {
                    result = CURSOR_FRIEND;

                } else {
                    result = CURSOR_UNIT_NO_GO;
                }

            } else if (unit2) {
                result = CURSOR_FRIEND;

            } else {
                result = CURSOR_UNIT_NO_GO;
            }

        } else if (unit1->cursor == CURSOR_HIDDEN) {
            result = CURSOR_FRIEND;

        } else {
            GameManager_Unit = Access_GetUnit2(grid_x, grid_y, GameManager_PlayerTeam);

            if (GameManager_Unit && GameManager_IsUnitNextToPosition(unit1, grid_x, grid_y) &&
                GameManager_Unit != unit1 && GameManager_Unit->hits > 0) {
                switch (unit1->cursor) {
                    case CURSOR_ARROW_NE: {
                        if (UnitsManager_BaseUnits[unit1->GetUnitType()].cargo_type ==
                                UnitsManager_BaseUnits[GameManager_Unit->GetUnitType()].cargo_type &&
                            GameManager_Unit->GetOrder() != ORDER_CLEAR &&
                            GameManager_Unit->GetOrder() != ORDER_BUILD) {
                            result = CURSOR_TRANSFER;

                        } else if (!(unit1->flags & STATIONARY) && (GameManager_Unit->flags & STATIONARY) &&
                                   GameManager_GetUnitWithCargoType(
                                       GameManager_Unit->GetComplex(),
                                       UnitsManager_BaseUnits[unit1->GetUnitType()].cargo_type)) {
                            result = CURSOR_TRANSFER;

                        } else {
                            result = CURSOR_FRIEND;
                        }
                    } break;

                    case CURSOR_ARROW_E: {
                        if (GameManager_Unit->hits < GameManager_Unit->GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
                            result = CURSOR_REPAIR;

                        } else {
                            result = CURSOR_FRIEND;
                        }
                    } break;

                    case CURSOR_ARROW_S: {
                        if (GameManager_Unit->ammo < GameManager_Unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO)) {
                            result = CURSOR_RELOAD;

                        } else {
                            result = CURSOR_FRIEND;
                        }
                    } break;

                    default: {
                        result = CURSOR_FRIEND;
                    } break;
                }

            } else {
                result = CURSOR_FRIEND;
            }
        }

    } else {
        result = CURSOR_UNIT_NO_GO;
    }

    return result;
}

bool GameManager_IsValidTransferTarget(UnitInfo* unit1, UnitInfo* unit2) {
    bool result;

    if (unit1 != unit2) {
        SmartPointer<Complex> complex;
        int32_t grid_x;
        int32_t grid_y;

        grid_x = unit1->grid_x;
        grid_y = unit1->grid_y;

        complex = unit2->GetComplex();

        if (complex != nullptr) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                 it != UnitsManager_StationaryUnits.End(); ++it) {
                if (complex == (*it).GetComplex() && GameManager_IsUnitNextToPosition(&(*it), grid_x, grid_y)) {
                    return true;
                }
            }

            result = false;

        } else {
            result = GameManager_IsUnitNextToPosition(unit2, grid_x, grid_y);
        }

    } else {
        result = false;
    }

    return result;
}

bool GameManager_IsValidStealTarget(UnitInfo* unit1, UnitInfo* unit2) {
    bool result;

    if ((unit2->flags & STATIONARY) || !(unit2->flags & ELECTRONIC_UNIT) || unit2->GetOrder() == ORDER_TRANSFORM ||
        unit2->GetOrder() == ORDER_FIRE || unit2->GetOrder() == ORDER_EXPLODE ||
        unit2->GetOrderState() == ORDER_STATE_DESTROY) {
        result = false;

    } else if ((unit2->GetUnitType() == CLNTRANS || unit2->GetUnitType() == SEATRANS ||
                unit2->GetUnitType() == AIRTRANS) &&
               unit2->storage) {
        result = false;

    } else {
        Cursor_DrawStealthActionChanceCursor(
            UnitsManager_GetStealthChancePercentage(unit1, unit2, ORDER_AWAIT_STEAL_UNIT), CURSOR_STEAL);

        result = true;
    }

    return result;
}

bool GameManager_IsValidDisableTarget(UnitInfo* unit1, UnitInfo* unit2) {
    bool result;

    if (!(unit2->flags & ELECTRONIC_UNIT) || unit2->GetOrder() == ORDER_TRANSFORM || unit2->GetOrder() == ORDER_FIRE ||
        unit2->GetOrder() == ORDER_EXPLODE || unit2->GetOrder() == ORDER_TAKE_OFF ||
        unit2->GetOrder() == ORDER_DISABLE) {
        result = false;

    } else {
        Cursor_DrawStealthActionChanceCursor(
            UnitsManager_GetStealthChancePercentage(unit1, unit2, ORDER_AWAIT_DISABLE_UNIT), CURSOR_DISABLE);

        result = true;
    }

    return result;
}

int32_t GameManager_GetMilitaryCursor(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    if (GameManager_SelectedUnit->delayed_reaction) {
        return CURSOR_UNIT_NO_GO;
    }

    if (GameManager_SelectedUnit->GetUnitType() != COMMANDO) {
        if (GameManager_SelectedUnit->weapon != 2) {
            return CURSOR_UNIT_NO_GO;
        }

        if (GameManager_SelectedUnit->ammo == 0) {
            if (GameManager_SelectedUnit->GetUnitList() == nullptr) {
                return CURSOR_UNIT_NO_GO;
            }
        }
    }

    {
        UnitInfo* target = Access_GetAttackTarget(GameManager_SelectedUnit.Get(), grid_x, grid_y);

        if (target) {
            if (GameManager_SelectedUnit->GetUnitType() == COMMANDO && GameManager_SelectedUnit->targeting_mode == 0) {
                if (GameManager_IsUnitNextToPosition(GameManager_SelectedUnit.Get(), grid_x, grid_y) &&
                    GameManager_IsUnitNotInAir(target)) {
                    if (GameManager_SelectedUnit->cursor == CURSOR_ARROW_NW ||
                        GameManager_SelectedUnit->cursor == CURSOR_HIDDEN) {
                        if (GameManager_IsValidDisableTarget(GameManager_SelectedUnit.Get(), target)) {
                            return CURSOR_DISABLE;
                        }
                    }

                    if (GameManager_SelectedUnit->cursor == CURSOR_ARROW_W ||
                        GameManager_SelectedUnit->cursor == CURSOR_HIDDEN) {
                        if (GameManager_IsValidStealTarget(GameManager_SelectedUnit.Get(), target)) {
                            return CURSOR_STEAL;
                        }
                    }
                }
            }

            if (GameManager_SelectedUnit->weapon != 2) {
                return CURSOR_UNIT_NO_GO;
            }

            if (GameManager_SelectedUnit->ammo == 0 && GameManager_SelectedUnit->GetUnitList() == nullptr) {
                return CURSOR_UNIT_NO_GO;
            }

            {
                int32_t result;

                if (Access_IsWithinAttackRange(GameManager_SelectedUnit.Get(), grid_x, grid_y,
                                               GameManager_SelectedUnit->GetBaseValues()->GetAttribute(ATTRIB_RANGE))) {
                    if (GameManager_SelectedUnit->ammo > 0) {
                        result = CURSOR_ENEMY;

                    } else {
                        result = CURSOR_UNIT_NO_GO;
                    }

                } else if (GameManager_SelectedUnit->flags & STATIONARY) {
                    result = CURSOR_UNIT_NO_GO;

                } else {
                    result = CURSOR_FAR_TARGET;
                }

                if (result != CURSOR_UNIT_NO_GO) {
                    Cursor_DrawAttackPowerCursor(GameManager_SelectedUnit.Get(), target, result);
                }

                return result;
            }

        } else {
            return CURSOR_UNIT_NO_GO;
        }
    }
}

void GameManager_UnitSelectOther(UnitInfo* unit1, UnitInfo* unit2, int32_t grid_x, int32_t grid_y) {
    if (unit1 != unit2) {
        GameManager_DeinitPopupButtons(false);
        GameManager_UpdateDrawBounds();
        GameManager_MenuUnitSelect(unit2);

    } else {
        unit2 = Access_GetSelectableUnit(unit1, grid_x, grid_y);

        if (unit2 && unit2->IsVisibleToTeam(GameManager_PlayerTeam)) {
            GameManager_DeinitPopupButtons(false);
            GameManager_UpdateDrawBounds();
            GameManager_MenuUnitSelect(unit2);
        }
    }
}

void GameManager_UnitSelect(UnitInfo* unit) {
    GameManager_DeinitPopupButtons(false);
    GameManager_UpdateDrawBounds();
    GameManager_MenuUnitSelect(unit);

    SDL_assert(unit != nullptr);

    if (unit->team == GameManager_ActiveTurnTeam &&
        UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type == TEAM_TYPE_PLAYER) {
        if ((unit->flags & STATIONARY) &&
            (GameManager_GameState != GAME_STATE_9_END_TURN || GameManager_PlayMode == PLAY_MODE_SIMULTANEOUS_MOVES)) {
            GameManager_InitPopupButtons(unit);
        }

        GameManager_DrawUnitStatusMessage(unit);
        GameManager_PlayUnitStatusVoice(unit);
    }
}

void GameManager_ClickUnit(UnitInfo* unit1, UnitInfo* unit2, int32_t grid_x, int32_t grid_y) {
    if (unit1 != unit2) {
        unit2->ClearUnitList();
        GameManager_UnitSelect(unit2);

    } else {
        unit1->ClearUnitList();
        GameManager_DrawUnitStatusMessage(unit1);

        if (GameManager_PopupButtons.popup_count) {
            GameManager_DeinitPopupButtons(false);

        } else if ((GameManager_GameState != GAME_STATE_9_END_TURN ||
                    GameManager_PlayMode == PLAY_MODE_SIMULTANEOUS_MOVES) &&
                   GameManager_InitPopupButtons(unit1)) {
            return;
        }

        unit2 = Access_GetSelectableUnit(unit1, grid_x, grid_y);

        if (unit1 != unit2 && unit2 && unit2->IsVisibleToTeam(unit1->team)) {
            unit2->ClearUnitList();
            GameManager_UnitSelect(unit2);
        }
    }
}

bool GameManager_UpdateSelection(UnitInfo* unit1, UnitInfo* unit2, int32_t grid_x, int32_t grid_y) {
    bool result;

    if (unit2 && unit2->team != GameManager_PlayerTeam) {
        GameManager_UnitSelectOther(unit1, unit2, grid_x, grid_y);

        result = false;

    } else if ((GameManager_GameState != GAME_STATE_9_END_TURN ||
                GameManager_PlayMode == PLAY_MODE_SIMULTANEOUS_MOVES) &&
               UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type == TEAM_TYPE_PLAYER) {
        if (GameManager_IsShiftKeyPressed && unit1 && unit2) {
            if (unit2->team == GameManager_PlayerTeam &&
                !(unit2->GetOrder() == ORDER_DISABLE ||
                  (unit2->GetOrder() == ORDER_IDLE && unit2->GetPriorOrder() == ORDER_DISABLE)) &&
                unit1->team == GameManager_PlayerTeam &&
                !(unit1->GetOrder() == ORDER_DISABLE ||
                  (unit1->GetOrder() == ORDER_IDLE && unit1->GetPriorOrder() == ORDER_DISABLE))) {
                if (unit2->GetFirstFromUnitList() == unit1) {
                    unit2->ClearUnitList();

                } else if (unit1 != unit2 && (unit1->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) &&
                           (unit2->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) &&
                           unit1->GetOrderState() == ORDER_STATE_EXECUTING_ORDER &&
                           unit2->GetOrderState() == ORDER_STATE_EXECUTING_ORDER) {
                    SmartList<UnitInfo>* units = unit1->GetUnitList();

                    if (!units || units->GetCount() < 10) {
                        unit2->ClearUnitList();
                        unit2->AssignUnitList(unit1);

                        Access_UpdateMultiSelection(unit1);
                    }
                }

                GameManager_UpdateDrawBounds();

                result = false;

            } else {
                result = false;
            }

        } else {
            result = true;
        }

    } else if (unit1 != unit2) {
        GameManager_UnitSelect(unit2);

        result = false;

    } else {
        result = false;
    };

    return result;
}

void GameManager_SetGridOffset(int32_t grid_x_offset, int32_t grid_y_offset) {
    if (!ini_get_setting(INI_CLICK_SCROLL) || (GameManager_MouseButtons & MOUSE_PRESS_LEFT) ||
        GameManager_RenderState == 2) {
        GameManager_GridOffset.x = grid_x_offset;
        GameManager_GridOffset.y = grid_y_offset;
    }
}

void GameManager_ProcessInput() {
    WindowInfo* window;
    uint8_t window_index;
    uint8_t window_cursor;

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

    for (window_index = WINDOW_POPUP_BUTTONS; window_index < WINDOW_COUNT; ++window_index) {
        window = WindowManager_GetWindow(window_index);

        if (GameManager_MouseX >= window->window.ulx && GameManager_MouseY >= window->window.uly &&
            GameManager_MouseX <= window->window.lrx && GameManager_MouseY <= window->window.lry) {
            break;
        }
    }

    if (window_index == WINDOW_MESSAGE_BOX) {
        window_index = WINDOW_MAIN_MAP;
    }

    if (GameManager_GameState == GAME_STATE_7_SITE_SELECT && window_index != WINDOW_MAIN_MAP) {
        window_index = WINDOW_MAIN_WINDOW;
    }

    window_cursor = Cursor_GetDefaultWindowCursor(window_index);

    if (window_index == WINDOW_MAIN_MAP) {
        int32_t grid_x;
        int32_t grid_y;

        window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

        grid_x = ((((GameManager_MouseX - window->window.ulx) * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) +
                  GameManager_MapWindowDrawBounds.ulx) /
                 GFX_MAP_TILE_SIZE;

        grid_y = ((((GameManager_MouseY - window->window.uly) * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) +
                  GameManager_MapWindowDrawBounds.uly) /
                 GFX_MAP_TILE_SIZE;

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
                (((GameManager_MouseX - window->window.ulx) * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) +
                GameManager_MapWindowDrawBounds.ulx;

            GameManager_ScaledMousePosition.y =
                (((GameManager_MouseY - window->window.uly) * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR) +
                GameManager_MapWindowDrawBounds.uly;

            GameManager_MultiSelectBounds.ulx = GameManager_ScaledMousePosition.x;
            GameManager_MultiSelectBounds.lrx = GameManager_ScaledMousePosition.x;
            GameManager_MultiSelectBounds.uly = GameManager_ScaledMousePosition.y;
            GameManager_MultiSelectBounds.lry = GameManager_ScaledMousePosition.y;

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
                Access_MultiSelect(&*GameManager_SelectedUnit, &GameManager_MultiSelectBounds);
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
            if (GameManager_MousePosition.x < ResourceManager_MapSize.x &&
                GameManager_MousePosition.y < ResourceManager_MapSize.y) {
                GameManager_DrawMouseCoordinates(GameManager_MousePosition.x + 1, GameManager_MousePosition.y + 1);
                GameManager_IsActiveMapPositionDisplay = true;

            } else {
                GameManager_DrawDisplayPanel(MENU_DISPLAY_CONTROL_COORDINATES, " ", 0xA2, 21);
                GameManager_IsActiveMapPositionDisplay = false;
            }

            if (GameManager_QuickBuildMenuActive) {
                UnitsManager_MoveUnit(&*GameManager_QuickBuilderUnit, GameManager_MousePosition.x,
                                      GameManager_MousePosition.y);
            }

            GameManager_LastMousePosition.x = GameManager_MousePosition.x;
            GameManager_LastMousePosition.y = GameManager_MousePosition.y;
        }

    } else if (window_index == WINDOW_MINIMAP) {
        Point minimap_position = GameManager_GetMinimapPosition();

        if (minimap_position.x != -1 && minimap_position.y != -1) {
            if (minimap_position.x != GameManager_LastMinimapPosition.x ||
                minimap_position.y != GameManager_LastMinimapPosition.y) {
                if (minimap_position.x < ResourceManager_MapSize.x && minimap_position.y < ResourceManager_MapSize.y) {
                    GameManager_DrawMouseCoordinates(minimap_position.x + 1, minimap_position.y + 1);
                    GameManager_IsActiveMapPositionDisplay = true;

                } else {
                    GameManager_DrawDisplayPanel(MENU_DISPLAY_CONTROL_COORDINATES, " ", 0xA2, 21);
                    GameManager_IsActiveMapPositionDisplay = false;
                }

                GameManager_LastMinimapPosition.x = minimap_position.x;
                GameManager_LastMinimapPosition.y = minimap_position.y;
            }

        } else {
            if (GameManager_IsActiveMapPositionDisplay) {
                GameManager_DrawDisplayPanel(MENU_DISPLAY_CONTROL_COORDINATES, " ", 0xA2, 21);
                GameManager_IsActiveMapPositionDisplay = false;
            }
        }

    } else {
        if (GameManager_IsActiveMapPositionDisplay) {
            GameManager_DrawDisplayPanel(MENU_DISPLAY_CONTROL_COORDINATES, " ", 0xA2, 21);
            GameManager_IsActiveMapPositionDisplay = false;

            GameManager_LastMousePosition.x = -1;
            GameManager_LastMousePosition.y = -1;
        }
    }

    if (GameManager_MouseButtons) {
        window = WindowManager_GetWindow(window_index);

        switch (window_index) {
            case WINDOW_POPUP_BUTTONS: {
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
                    GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, Gfx_ZoomLevel + 10, 0, false);
                }
            } break;

            case WINDOW_ZOOM_SLIDER_BUTTON: {
                if (GameManager_MouseButtons & MOUSE_PRESS_LEFT) {
                    int32_t slider_offset;

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

                    GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, slider_offset, 0, 0);
                }
            } break;

            case WINDOW_ZOOM_MINUS_BUTTON: {
                if (GameManager_MouseButtons & MOUSE_RELEASE_LEFT) {
                    GameManager_UpdateMainMapView(MAP_VIEW_ZOOM, Gfx_ZoomLevel - 10, 0, false);
                }
            } break;

            case WINDOW_MINIMAP: {
                Point minimap_position = GameManager_GetMinimapPosition();

                if (minimap_position.x != -1 && minimap_position.y != -1) {
                    if (GameManager_MouseButtons & (MOUSE_PRESS_LEFT | MOUSE_RELEASE_LEFT)) {
                        GameManager_UpdateMainMapView(MAP_VIEW_CENTER, minimap_position.x, minimap_position.y);

                    } else if (GameManager_MouseButtons & MOUSE_RELEASE_RIGHT) {
                        uint8_t cursor;

                        cursor = GameManager_GetWindowCursor(minimap_position.x, minimap_position.y);

                        if (cursor == CURSOR_UNIT_GO || cursor == CURSOR_WAY) {
                            if (GameManager_SelectedUnit->GetOrderState() != ORDER_STATE_UNIT_READY) {
                                GameManager_SetUnitOrder(
                                    ORDER_MOVE, cursor == CURSOR_UNIT_GO ? ORDER_STATE_INIT : ORDER_STATE_MOVE_INIT,
                                    &*GameManager_SelectedUnit, minimap_position.x, minimap_position.y);
                            }
                        }
                    }
                }

            } break;

            case WINDOW_MAIN_MAP: {
                if (!(GameManager_MouseButtons & (MOUSE_PRESS_LEFT | MOUSE_PRESS_RIGHT))) {
                    if (GameManager_MousePosition.x < ResourceManager_MapSize.x &&
                        GameManager_MousePosition.y < ResourceManager_MapSize.y) {
                        SoundManager_PlaySfx(KCARG0);

                        if (MessageManager_MessageBox_IsActive) {
                            MessageManager_ClearMessageBox();
                        }

                        if (GameManager_GameState == GAME_STATE_7_SITE_SELECT) {
                            UnitsManager_TeamMissionSupplies[GameManager_ActiveTurnTeam].starting_position =
                                GameManager_MousePosition;
                            UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].camera_position =
                                GameManager_MousePosition;
                            UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].zoom_level = 64;

                            if (Cursor_GetCursor() == CURSOR_UNIT_GO) {
                                GameManager_GameState = GAME_STATE_13;
                                SoundManager_PlaySfx(NDONE0);

                            } else {
                                SoundManager_PlaySfx(NCANC0);
                            }

                        } else {
                            UnitInfo* unit =
                                Access_GetTeamUnit(GameManager_MousePosition.x, GameManager_MousePosition.y,
                                                   GameManager_PlayerTeam, SELECTABLE);

                            if (GameManager_QuickBuildMenuActive) {
                                if (GameManager_MouseButtons & MOUSE_RELEASE_RIGHT) {
                                    if (!unit) {
                                        unit = Access_GetQuickBuilderUnit(GameManager_MousePosition.x,
                                                                          GameManager_MousePosition.y);
                                    }

                                    if (unit) {
                                        UnitsManager_SetNewOrder(unit, ORDER_BUILD, ORDER_STATE_BUILD_CLEARING);
                                    }

                                } else {
                                    if (UnitsManager_MoveUnitAndParent(&*GameManager_QuickBuilderUnit,
                                                                       GameManager_MousePosition.x,
                                                                       GameManager_MousePosition.y)) {
                                        if (Remote_IsNetworkGame) {
                                            Remote_SendNetPacket_14(
                                                GameManager_PlayerTeam, GameManager_QuickBuildUnitId,
                                                GameManager_MousePosition.x, GameManager_MousePosition.y);
                                        }

                                        GameManager_DeployUnit(GameManager_PlayerTeam, GameManager_QuickBuildUnitId,
                                                               GameManager_MousePosition.x,
                                                               GameManager_MousePosition.y);
                                    }
                                }

                            } else {
                                UnitInfo* unit2 =
                                    Access_GetEnemyUnit(GameManager_PlayerTeam, GameManager_MousePosition.x,
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
                                            if (GameManager_UpdateSelection(
                                                    &*GameManager_SelectedUnit, GameManager_Unit,
                                                    GameManager_MousePosition.x, GameManager_MousePosition.y)) {
                                                GameManager_RepairUnit(&*GameManager_SelectedUnit, GameManager_Unit);
                                            }
                                        } break;

                                        case CURSOR_TRANSFER: {
                                            if (GameManager_UpdateSelection(
                                                    &*GameManager_SelectedUnit, GameManager_Unit,
                                                    GameManager_MousePosition.x, GameManager_MousePosition.y)) {
                                                GameManager_TransferCargo(&*GameManager_SelectedUnit, GameManager_Unit);
                                            }
                                        } break;

                                        case CURSOR_FUEL: {
                                        } break;

                                        case CURSOR_RELOAD: {
                                            if (GameManager_UpdateSelection(
                                                    &*GameManager_SelectedUnit, GameManager_Unit,
                                                    GameManager_MousePosition.x, GameManager_MousePosition.y)) {
                                                GameManager_ReloadUnit(&*GameManager_SelectedUnit, GameManager_Unit);
                                            }
                                        } break;

                                        case CURSOR_LOAD: {
                                            if (GameManager_UpdateSelection(
                                                    &*GameManager_SelectedUnit, GameManager_Unit,
                                                    GameManager_MousePosition.x, GameManager_MousePosition.y)) {
                                                if (GameManager_SelectedUnit->storage <
                                                    GameManager_SelectedUnit->GetBaseValues()->GetAttribute(
                                                        ATTRIB_STORAGE)) {
                                                    if (GameManager_SelectedUnit->GetUnitType() == AIRTRANS) {
                                                        GameManager_SelectedUnit->SetParent(GameManager_Unit);
                                                        GameManager_SetUnitOrder(
                                                            ORDER_MOVE, ORDER_STATE_INIT, &*GameManager_SelectedUnit,
                                                            GameManager_MousePosition.x, GameManager_MousePosition.y);

                                                    } else if (GameManager_Unit->GetOrder() == ORDER_DISABLE) {
                                                        MessageManager_DrawMessage(_(9897), 1, 0);

                                                    } else if (GameManager_SelectedUnit->grid_x ==
                                                                   GameManager_Unit->grid_x &&
                                                               GameManager_SelectedUnit->grid_y ==
                                                                   GameManager_Unit->grid_y &&
                                                               GameManager_Unit->GetOrderState() ==
                                                                   ORDER_STATE_EXECUTING_ORDER) {
                                                        GameManager_Unit->SetParent(&*GameManager_SelectedUnit);
                                                        UnitsManager_SetNewOrder(GameManager_Unit, ORDER_LAND,
                                                                                 ORDER_STATE_INIT);

                                                    } else {
                                                        GameManager_SetUnitOrder(ORDER_MOVE_TO_UNIT, ORDER_STATE_INIT,
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

                                            GameManager_SetUnitOrder(ORDER_MOVE_TO_ATTACK, ORDER_STATE_EXECUTING_ORDER,
                                                                     &*GameManager_SelectedUnit,
                                                                     GameManager_MousePosition.x,
                                                                     GameManager_MousePosition.y);
                                        } break;

                                        case CURSOR_FAR_TARGET: {
                                            GameManager_SetUnitOrder(
                                                ORDER_MOVE_TO_ATTACK, ORDER_STATE_INIT, &*GameManager_SelectedUnit,
                                                GameManager_MousePosition.x, GameManager_MousePosition.y);
                                        } break;

                                        case CURSOR_UNIT_GO:
                                        case CURSOR_WAY: {
                                            if (GameManager_SelectedUnit != nullptr) {
                                                if (GameManager_SelectedUnit->enter_mode) {
                                                    GameManager_SetUnitOrder(ORDER_MOVE_TO_UNIT, ORDER_STATE_INIT,
                                                                             &*GameManager_SelectedUnit, unit->grid_x,
                                                                             unit->grid_y);
                                                    GameManager_SelectedUnit->enter_mode = false;

                                                } else if (GameManager_SelectedUnit->GetOrderState() ==
                                                           ORDER_STATE_UNIT_READY) {
                                                    GameManager_DeinitPopupButtons(false);
                                                    GameManager_UpdateDrawBounds();

                                                    GameManager_SelectedUnit->target_grid_x =
                                                        GameManager_MousePosition.x;
                                                    GameManager_SelectedUnit->target_grid_y =
                                                        GameManager_MousePosition.y;

                                                    UnitsManager_SetNewOrder(&*GameManager_SelectedUnit, ORDER_ACTIVATE,
                                                                             ORDER_STATE_IN_TRANSITION);

                                                } else {
                                                    GameManager_SetUnitOrder(
                                                        ORDER_MOVE,
                                                        Cursor_GetCursor() == CURSOR_UNIT_GO ? ORDER_STATE_INIT
                                                                                             : ORDER_STATE_MOVE_INIT,
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

                                            UnitsManager_SetNewOrder(parent, ORDER_ACTIVATE,
                                                                     ORDER_STATE_EXECUTING_ORDER);
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
                }
            } break;

            default: {
                GameManager_DeinitPopupButtons(false);
            } break;
        }
    }
}

void GameManager_MenuDeleteFlic() {
    if (GameManager_Flic.flc) {
        flicsmgr_delete(GameManager_Flic.flc);
        free(GameManager_Flic.flc);
        GameManager_Flic.flc = nullptr;
    }

    delete[] GameManager_Flic.sw.buffer;

    GameManager_Flic.sw.id = -1;
    GameManager_Flic.sw.buffer = nullptr;

    GameManager_Flic.dw.id = -1;
    GameManager_Flic.dw.buffer = nullptr;
}

void GameManager_MenuCreateFlic(ResourceID unit_type, int32_t ulx, int32_t uly) {
    WindowInfo* main_window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    WindowInfo* flic_window = WindowManager_GetWindow(WINDOW_CORNER_FLIC);
    BaseUnit* base_unit = &UnitsManager_BaseUnits[unit_type];

    GameManager_MenuDeleteFlic();

    GameManager_Flic.sw.id = -1;
    GameManager_Flic.sw.width = FLICSMGR_FLIC_SIZE;
    GameManager_Flic.sw.buffer = new (std::nothrow) uint8_t[FLICSMGR_FLIC_SIZE * FLICSMGR_FLIC_SIZE];
    GameManager_Flic.sw.window.ulx = 0;
    GameManager_Flic.sw.window.uly = 0;
    GameManager_Flic.sw.window.lrx = FLICSMGR_FLIC_SIZE - 1;
    GameManager_Flic.sw.window.lry = FLICSMGR_FLIC_SIZE - 1;

    GameManager_Flic.dw.id = main_window->id;
    GameManager_Flic.dw.width = main_window->width;
    GameManager_Flic.dw.buffer = main_window->buffer;
    GameManager_Flic.dw.window = flic_window->window;

    GameManager_Flic.flc =
        flicsmgr_construct(base_unit->flics, &GameManager_Flic.sw, GameManager_Flic.sw.width, 0, 0, 2, 0);

    const int32_t font_id = Text_GetFont();
    Text_SetFont(GNW_TEXT_FONT_2);

    GameManager_Flic.text_height = Text_GetHeight() * (unit_type == COMMANDO ? 3 : 2) + 1;

    Text_SetFont(font_id);

    GameManager_DrawFlic(&GameManager_Flic.dw.window);
}

void GameManager_DrawFlic(Rect* bounds) {
    const int32_t local_width = GameManager_Flic.dw.window.lrx - GameManager_Flic.dw.window.ulx + 1;
    const int32_t local_height = GameManager_Flic.dw.window.lry - GameManager_Flic.dw.window.uly + 1;
    const int32_t local_offset_y = bounds->uly - GameManager_Flic.dw.window.uly;
    uint8_t* const local_buffer = new (std::nothrow) uint8_t[local_width * local_height];

    cscale(GameManager_Flic.sw.buffer, FLICSMGR_FLIC_SIZE, FLICSMGR_FLIC_SIZE, FLICSMGR_FLIC_SIZE, local_buffer,
           local_width, local_height, local_width);

    buf_to_buf(&local_buffer[local_width * local_offset_y], local_width, local_height - local_offset_y, local_width,
               &GameManager_Flic.dw.buffer[GameManager_Flic.dw.width * bounds->uly + bounds->ulx],
               GameManager_Flic.dw.width);

    delete[] local_buffer;

    win_draw_rect(GameManager_Flic.dw.id, bounds);
}

void GameManager_AdvanceFlic() {
    if (GameManager_Flic.flc && GameManager_PlayFlic && ini_get_setting(INI_EFFECTS)) {
        uint32_t time_stamp = timer_get();

        if ((time_stamp - GameManager_FlicFrameTimeStamp) >= TIMER_FPS_TO_MS(15)) {
            GameManager_FlicFrameTimeStamp = time_stamp;

            // the original design assumes that animated FLICs do not redraw the top text rows
            GameManager_PlayFlic = flicsmgr_advance_animation(GameManager_Flic.flc);

            Rect bounds = GameManager_Flic.dw.window;
            bounds.uly += GameManager_Flic.text_height;

            GameManager_DrawFlic(&bounds);
        }
    }
}

void GameManager_DrawCircle(UnitInfo* unit, WindowInfo* window, int32_t radius, int32_t color) {
    if (radius > 0 && unit->GetOrder() != ORDER_DISABLE) {
        Rect bounds;
        int32_t scaled_radius;
        int32_t unit_size;

        radius *= GFX_MAP_TILE_SIZE;
        scaled_radius = (radius * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor;

        if (color == COLOR_YELLOW) {
            ++scaled_radius;
        }

        if (unit->flags & BUILDING) {
            unit_size = GFX_MAP_TILE_SIZE;

        } else {
            unit_size = GFX_MAP_TILE_SIZE / 2;
        }

        bounds.ulx = (unit->grid_x * GFX_MAP_TILE_SIZE) + unit_size;
        bounds.uly = (unit->grid_y * GFX_MAP_TILE_SIZE) + unit_size;

        Gfx_RenderCircle(window->buffer, WindowManager_WindowWidth, WindowManager_MapWidth, WindowManager_MapHeight,
                         (bounds.ulx * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUlx,
                         (bounds.uly * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUly, scaled_radius,
                         color);

        radius = (scaled_radius * Gfx_MapScalingFactor) / GFX_SCALE_DENOMINATOR;

        bounds.lrx = bounds.ulx + radius + 1;
        bounds.lry = bounds.uly + radius + 1;
        bounds.ulx -= radius + 1;
        bounds.uly -= radius + 1;

        GameManager_AddDrawBounds(&bounds);
    }
}

void GameManager_MenuUnitSelect(UnitInfo* unit) {
    if (GameManager_DisplayControlsInitialized) {
        if (GameManager_SelectedUnit != nullptr && GameManager_SelectedUnit != unit) {
            GameManager_SelectedUnit->RefreshScreen();
            SoundManager_PlaySfx(&*GameManager_SelectedUnit, SFX_TYPE_INVALID);
        }

        Gamemanager_FlicButton->Enable(unit && unit->team == GameManager_PlayerTeam);

        if (GameManager_TextEditUnitName) {
            delete GameManager_TextEditUnitName;
            GameManager_TextEditUnitName = nullptr;
        }

        if (unit) {
            WindowInfo* window;
            int32_t sound;
            char text[100];

            unit->targeting_mode = 0;
            unit->enter_mode = 0;
            unit->cursor = 0;

            GameManager_IsSurveyorSelected = Access_IsSurveyorOverlayActive(unit);

            if (GameManager_PlayMode) {
                GameManager_MenuClickEndTurnButton(GameManager_GameState == GAME_STATE_8_IN_GAME);

            } else {
                GameManager_MenuClickEndTurnButton(UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type ==
                                                   TEAM_TYPE_PLAYER);
            }

            window = WindowManager_GetWindow(WINDOW_CORNER_FLIC);

            GameManager_MenuCreateFlic(unit->GetUnitType(), window->window.ulx, window->window.uly);

            unit->GetDisplayName(text, sizeof(text));

            Text_SetFont(GNW_TEXT_FONT_2);

            win_print(window->id, text, FLICSMGR_FLIC_SIZE, window->window.ulx, window->window.uly,
                      GNW_TEXT_REFRESH_WINDOW | GNW_TEXT_ALLOW_TRUNCATED | COLOR_GREEN);

            if (unit->GetUnitType() == COMMANDO) {
                int32_t experience;
                char exp_text[10];

                experience = unit->GetExperience();

                if (experience > 0) {
                    if (experience > 2) {
                        if (experience > 5) {
                            if (experience > 10) {
                                strcpy(text, _(1d69));

                            } else {
                                strcpy(text, _(13d9));
                            }

                        } else {
                            strcpy(text, _(abcf));
                        }

                    } else {
                        strcpy(text, _(b38a));
                    }

                } else {
                    strcpy(text, _(ea0b));
                }

                if (experience > 0) {
                    sprintf(exp_text, " (+%i)", experience);
                    strcat(text, exp_text);
                }

                win_print(window->id, text, 128, window->window.ulx, window->window.uly + Text_GetHeight() * 2,
                          GNW_TEXT_REFRESH_WINDOW | GNW_TEXT_ALLOW_TRUNCATED | COLOR_GREEN);
            }

            GameManager_UpdateInfoDisplay(unit);
            unit->RefreshScreen();

            sound = SFX_TYPE_IDLE;

            if (unit->GetOrder() == ORDER_BUILD && (unit->GetOrderState() == ORDER_STATE_BUILD_IN_PROGRESS ||
                                                    unit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER)) {
                sound = SFX_TYPE_BUILDING;

            } else if (unit->GetOrder() == ORDER_CLEAR && unit->GetOrderState() == ORDER_STATE_IN_PROGRESS) {
                sound = SFX_TYPE_BUILDING;

            } else if (unit->GetOrder() == ORDER_POWER_ON && unit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER) {
                sound = SFX_TYPE_BUILDING;
            }

            SoundManager_PlaySfx(unit, sound);

            if (unit->GetUnitList()) {
                unit->MoveToFrontInUnitList();
            }

            GameManager_SelectedUnit = unit;

        } else {
            GameManager_MenuDeleteFlic();
            GameManager_FillOrRestoreWindow(WINDOW_CORNER_FLIC, COLOR_BLACK, true);
            GameManager_FillOrRestoreWindow(WINDOW_STAT_WINDOW, COLOR_BLACK, true);
            GameManager_DeinitPopupButtons(false);
            GameManager_SelectedUnit = nullptr;
        }
    }
}

void GameManager_FillOrRestoreWindow(uint8_t id, int32_t color, bool redraw) {
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

void GameManager_DrawInfoDisplayRowIcons(uint8_t* buffer, int32_t full_width, int32_t width, int32_t height,
                                         ResourceID icon, int32_t current_value, int32_t base_value) {
    struct ImageSimpleHeader* image;
    int32_t width_offset;
    int32_t height_offset;
    int32_t buffer_offset;
    int32_t base_value_scaled;
    int32_t base_value_diff;
    int32_t factor;
    int32_t offset;
    int32_t ulx;
    int32_t uly;
    int32_t icon_index;

    if (current_value) {
        image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(icon));

    } else {
        image = reinterpret_cast<struct ImageSimpleHeader*>(
            ResourceManager_LoadResource(static_cast<ResourceID>(icon + 1)));
    }

    if (image) {
        if (base_value > 5) {
            height_offset = ((height - 1) - image->height) / 4;

        } else {
            height_offset = 0;
        }

        buffer_offset = ((((height - 1) - (height_offset * 4)) - image->height) / 2) * full_width;

        base_value_scaled = (base_value + 4) / 5;

        base_value_diff = base_value - ((base_value_scaled - 1) * 5);

        width -= image->width * base_value_scaled;

        width_offset = (base_value_scaled * 4) - (5 - base_value_diff);

        if (width_offset) {
            if (width >= width_offset) {
                factor = width / width_offset;

                if (factor > image->width) {
                    factor = image->width;
                }

                offset = 0;

            } else {
                factor = 1;
                offset = (base_value_scaled + (width_offset - width) - 2) / (base_value_scaled - 1);
            }

        } else {
            factor = image->width;
            offset = 0;
        }

        ulx = (image->width + factor * 4) - offset;

        for (int32_t i = 0; i < base_value; ++i) {
            int32_t buffer_position;

            icon_index = i % 5;
            buffer_position = ulx * (i / 5) + (factor * icon_index);
            uly = icon_index * height_offset;

            UnitStats_DrawImage(&buffer[buffer_offset + buffer_position + uly * full_width], full_width, image);
            --current_value;

            if (current_value == 0) {
                image = reinterpret_cast<struct ImageSimpleHeader*>(
                    ResourceManager_LoadResource(static_cast<ResourceID>(icon + 1)));
            }
        }
    }
}

void GameManager_DrawInfoDisplayRow(const char* label, int32_t window_id, ResourceID icon, int32_t current_value,
                                    int32_t base_value, int32_t factor) {
    WindowInfo* window;
    int32_t width;
    int32_t height;
    int32_t color;
    int32_t size;
    char text[50];

    window = WindowManager_GetWindow(window_id);

    width = window->window.lrx - window->window.ulx;
    height = window->window.lry - window->window.uly;

    if (window_id != WINDOW_STAT_ROW_4) {
        draw_line(window->buffer, window->width, 0, height, width, height, 0x3C);
    }

    if (base_value && factor) {
        current_value = std::min(current_value, base_value);

        if (current_value <= base_value / 4) {
            color = COLOR_RED;

            if (icon == SI_HITSB) {
                icon = SI_HITSR;
            }

        } else if (current_value <= base_value / 2) {
            color = COLOR_YELLOW;

            if (icon == SI_HITSB) {
                icon = SI_HITSY;
            }

        } else {
            color = COLOR_GREEN;
        }

        Text_SetFont(GNW_TEXT_FONT_2);

        if (icon == SI_FUEL || icon == SI_GOLD) {
            size = 20;
        } else {
            size = 25;
        }

        if (((factor + base_value - 1) / factor) > size) {
            factor = (size + base_value - 1) / size;
        }

        sprintf(text, "%i/%i", current_value, base_value);

        Text_TextBox(window->buffer, window->width, text, 0, 0, 45, height, color, true, true);

        current_value = (factor + current_value - 1) / factor;
        base_value = (factor + base_value - 1) / factor;

        Text_TextBox(window->buffer, window->width, label, 45, 0, 30, height, 0xA2, false, true);

        width -= 75;

        GameManager_DrawInfoDisplayRowIcons(&window->buffer[75], window->width, width, height, icon, current_value,
                                            base_value);
    }
}

void GameManager_DrawInfoDisplayType2(UnitInfo* unit) {
    int32_t power_need;

    power_need = Cargo_GetPowerConsumptionRate(unit->GetUnitType());

    if (power_need && GameManager_PlayerTeam == unit->team && !Cargo_GetLifeConsumptionRate(unit->GetUnitType())) {
        int32_t current_power_need;
        int32_t current_value;
        int32_t base_value;
        Cargo cargo;

        current_value = 0;
        base_value = 0;

        cargo = Cargo_GetNetProduction(unit);

        current_power_need = cargo.power;

        if (power_need >= 0) {
            GameManager_DrawInfoDisplayRow(_(8d59), WINDOW_STAT_ROW_2, SI_POWER, -current_power_need, power_need, 1);

        } else {
            GameManager_DrawInfoDisplayRow(_(848e), WINDOW_STAT_ROW_2, SI_POWER, current_power_need, -power_need, 1);
        }

        power_need = 0;
        current_power_need = 0;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).GetComplex() == unit->GetComplex()) {
                cargo.power = Cargo_GetPowerConsumptionRate((*it).GetUnitType());

                if (cargo.power >= 0) {
                    base_value += cargo.power;

                } else {
                    power_need -= cargo.power;
                }

                const bool upgrade_all =
                    (*it).GetOrder() == ORDER_UPGRADE && (*it).GetOrderState() == ORDER_STATE_EXECUTING_ORDER;

                if (upgrade_all) {
                    (*it).SetPriorOrder((*it).SetOrder((*it).GetPriorOrder()));
                }

                cargo = Cargo_GetNetProduction(it->Get());

                if (upgrade_all) {
                    (*it).SetPriorOrder((*it).SetOrder((*it).GetPriorOrder()));
                }

                if (cargo.power >= 0) {
                    current_power_need += cargo.power;

                } else {
                    current_value -= cargo.power;
                }
            }
        }

        if (Cargo_GetPowerConsumptionRate(unit->GetUnitType()) > 0) {
            GameManager_DrawInfoDisplayRow(_(c9b3), WINDOW_STAT_ROW_3, SI_POWER, current_value, base_value, 1);

        } else {
            GameManager_DrawInfoDisplayRow(_(d4ca), WINDOW_STAT_ROW_3, SI_POWER, current_power_need, power_need, 1);
            GameManager_DrawInfoDisplayRow(_(81c5), WINDOW_STAT_ROW_4, SI_POWER, current_value, base_value, 1);
        }
    }
}

void GameManager_DrawInfoDisplayType1(UnitInfo* unit) {
    int32_t life_need;

    life_need = Cargo_GetLifeConsumptionRate(unit->GetUnitType());

    if (life_need && GameManager_PlayerTeam == unit->team) {
        int32_t current_life_need;
        int32_t current_value;
        int32_t base_value;
        Cargo cargo;

        current_value = 0;
        base_value = 0;

        cargo = Cargo_GetNetProduction(unit);

        current_life_need = cargo.life;

        if (life_need >= 0) {
            GameManager_DrawInfoDisplayRow(_(3aea), WINDOW_STAT_ROW_2, SI_WORK, -current_life_need, life_need, 1);

        } else {
            GameManager_DrawInfoDisplayRow(_(1e1a), WINDOW_STAT_ROW_2, SI_WORK, current_life_need, -life_need, 1);
        }

        life_need = 0;
        current_life_need = 0;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).GetComplex() == unit->GetComplex()) {
                cargo.life = Cargo_GetLifeConsumptionRate((*it).GetUnitType());

                if (cargo.life >= 0) {
                    base_value += cargo.life;

                } else {
                    life_need -= cargo.life;
                }

                const bool upgrade_all =
                    (*it).GetOrder() == ORDER_UPGRADE && (*it).GetOrderState() == ORDER_STATE_EXECUTING_ORDER;

                if (upgrade_all) {
                    (*it).SetPriorOrder((*it).SetOrder((*it).GetPriorOrder()));
                }

                cargo = Cargo_GetNetProduction(it->Get());

                if (upgrade_all) {
                    (*it).SetPriorOrder((*it).SetOrder((*it).GetPriorOrder()));
                }

                if (cargo.life >= 0) {
                    current_life_need += cargo.life;

                } else {
                    current_value -= cargo.life;
                }
            }
        }

        if (Cargo_GetLifeConsumptionRate(unit->GetUnitType()) > 0) {
            GameManager_DrawInfoDisplayRow(_(c6fe), WINDOW_STAT_ROW_3, SI_WORK, current_value, base_value, 1);

        } else {
            GameManager_DrawInfoDisplayRow(_(7be8), WINDOW_STAT_ROW_3, SI_WORK, current_life_need, life_need, 1);
            GameManager_DrawInfoDisplayRow(_(160c), WINDOW_STAT_ROW_4, SI_WORK, current_value, base_value, 1);
        }
    }
}

void GameManager_DrawInfoDisplayType3(UnitInfo* unit) {
    uint32_t current_value;
    uint32_t value_limit;

    if (ini_setting_victory_type == VICTORY_TYPE_SCORE) {
        current_value = ini_setting_victory_limit;

    } else {
        current_value = 0;

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
                UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED) {
                current_value = std::max(UnitsManager_TeamInfo[unit->team].team_points, current_value);
            }
        }
    }

    if (current_value < 1) {
        current_value = 10;
    }

    value_limit = (current_value + 14) / 15;

    GameManager_DrawInfoDisplayRow(_(c54a), WINDOW_STAT_ROW_2, SI_WORK, unit->storage, unit->storage, value_limit);
    GameManager_DrawInfoDisplayRow(_(5f25), WINDOW_STAT_ROW_3, SI_WORK, UnitsManager_TeamInfo[unit->team].team_points,
                                   current_value, value_limit);
}

SmartString GameManager_GetUnitStatusMessage(UnitInfo* unit) {
    SmartString message;

    if (unit->GetOrder() == ORDER_DISABLE && unit->team != PLAYER_TEAM_ALIEN) {
        if (unit->recoil_delay == 1) {
            message = _(54ba);

        } else {
            message.Sprintf(25, _(45e4), unit->recoil_delay);
        }

    } else {
        message = GameManager_OrderStatusMessages[unit->GetOrder()];

        if ((unit->GetOrder() == ORDER_AWAIT || unit->GetOrder() == ORDER_MOVE) &&
            unit->team == GameManager_PlayerTeam) {
            if (unit->auto_survey) {
                message = _(2a38);
            }

            if (unit->disabled_reaction_fire) {
                message = _(c35e);
            }

            if (unit->GetLayingState() == 2) {
                message = _(bd3a);
            }

            if (unit->GetLayingState() == 1) {
                message = _(361e);
            }
        }
    }

    return message;
}

int32_t GameManager_GetDialogWindowCenterMode() {
    int32_t result;

    if (GameManager_GameState <= GAME_STATE_3_MAIN_MENU) {
        result = WINDOW_MAIN_WINDOW;

    } else if (ini_get_setting(INI_DIALOG_CENTER_MODE)) {
        result = WINDOW_MAIN_WINDOW;

    } else {
        result = WINDOW_MAIN_MAP;
    }

    return result;
}

void GameManager_UpdateInfoDisplay(UnitInfo* unit) {
    if (unit->IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy) {
        SmartPointer<UnitValues> unit_values(unit->GetBaseValues());
        WindowInfo* window;
        uint8_t scaling_factor{0};

        window = WindowManager_GetWindow(WINDOW_CORNER_FLIC);

        Text_SetFont(GNW_TEXT_FONT_2);

        win_print(window->id, GameManager_GetUnitStatusMessage(unit).GetCStr(), 128, window->window.ulx,
                  window->window.uly + Text_GetHeight(), GNW_TEXT_REFRESH_WINDOW | GNW_TEXT_ALLOW_TRUNCATED | 0xA2);

        GameManager_FillOrRestoreWindow(WINDOW_STAT_WINDOW, COLOR_BLACK, false);

        GameManager_DrawInfoDisplayRow(_(c02c), WINDOW_STAT_ROW_1, SI_HITSB, unit->hits,
                                       unit_values->GetAttribute(ATTRIB_HITS), 4);

        if (unit_values->GetAttribute(ATTRIB_ATTACK)) {
            int32_t ammo;

            if (unit->team == GameManager_PlayerTeam) {
                ammo = unit_values->GetAttribute(ATTRIB_AMMO);

            } else {
                ammo = 0;
            }

            GameManager_DrawInfoDisplayRow(_(5027), WINDOW_STAT_ROW_2, SI_AMMO, unit->ammo, ammo, 1);

        } else {
            uint8_t cargo_type;
            int32_t storage_value;

            cargo_type = UnitsManager_BaseUnits[unit->GetUnitType()].cargo_type;
            storage_value = unit_values->GetAttribute(ATTRIB_STORAGE);

            if (storage_value <= 4) {
                scaling_factor = 1;

            } else if (storage_value <= 8) {
                scaling_factor = 2;

            } else {
                scaling_factor = 3;
            }

            if (cargo_type < CARGO_TYPE_LAND) {
                scaling_factor = 10;
            }

            if (unit->team != GameManager_PlayerTeam && !GameManager_MaxSpy) {
                scaling_factor = 0;
            }

            GameManager_DrawInfoDisplayRow(_(1274), WINDOW_STAT_ROW_2, ReportStats_CargoIcons[cargo_type * 2],
                                           unit->storage, storage_value, scaling_factor);
        }

        GameManager_DrawInfoDisplayRow(_(c372), WINDOW_STAT_ROW_3, SI_SPEED, unit->speed,
                                       unit_values->GetAttribute(ATTRIB_SPEED), 1);

        GameManager_DrawInfoDisplayRow(
            _(2bce), WINDOW_STAT_ROW_4, SI_SHOTS, unit->shots,
            std::min(unit_values->GetAttribute(ATTRIB_ROUNDS), unit_values->GetAttribute(ATTRIB_AMMO)), 1);

        if (unit_values->GetAttribute(ATTRIB_STORAGE) == 0) {
            GameManager_DrawInfoDisplayType2(unit);
        }

        if (unit->GetUnitType() == GREENHSE && unit->team == GameManager_PlayerTeam) {
            GameManager_DrawInfoDisplayType3(unit);

        } else {
            GameManager_DrawInfoDisplayType1(unit);
        }

        if ((unit->team == GameManager_PlayerTeam || GameManager_MaxSpy) &&
            unit_values->GetAttribute(ATTRIB_STORAGE) > 0 && (unit->flags & STATIONARY) &&
            UnitsManager_BaseUnits[unit->GetUnitType()].cargo_type >= CARGO_TYPE_RAW &&
            UnitsManager_BaseUnits[unit->GetUnitType()].cargo_type <= CARGO_TYPE_GOLD) {
            Cargo materials;
            Cargo capacity;
            int32_t value{0};
            int32_t value_limit{0};

            unit->GetComplex()->GetCargoInfo(materials, capacity);

            switch (UnitsManager_BaseUnits[unit->GetUnitType()].cargo_type) {
                case CARGO_TYPE_RAW: {
                    value = materials.raw;
                    value_limit = capacity.raw;
                } break;

                case CARGO_TYPE_FUEL: {
                    value = materials.fuel;
                    value_limit = capacity.fuel;
                } break;

                case CARGO_TYPE_GOLD: {
                    value = materials.gold;
                    value_limit = capacity.gold;
                } break;
            }

            GameManager_DrawInfoDisplayRow(
                _(d28b), WINDOW_STAT_ROW_3,
                ReportStats_CargoIcons[UnitsManager_BaseUnits[unit->GetUnitType()].cargo_type * 2], value, value_limit,
                scaling_factor);
        }

        window = WindowManager_GetWindow(WINDOW_STAT_WINDOW);

        win_draw_rect(window->id, &window->window);
    }
}

void GameManager_MenuInitButtons(bool mode) {
    WindowInfo* window;
    int32_t r_value;
    int32_t p_value;
    uint32_t flags;

    window = WindowManager_GetWindow(WINDOW_CORNER_FLIC);

    if (GameManager_GameFileNumber && !Remote_IsNetworkGame) {
        GameManager_MenuItems[MENU_GUI_ITEM_CHAT_BUTTON].gfx = GOAL_OFF;
        GameManager_MenuItems[MENU_GUI_ITEM_CHAT_BUTTON].label = _(cdbf);
    } else {
        GameManager_MenuItems[MENU_GUI_ITEM_CHAT_BUTTON].gfx = CHAT_OFF;
        GameManager_MenuItems[MENU_GUI_ITEM_CHAT_BUTTON].label = _(ea1a);
    }

    for (uint32_t i = 0; i < sizeof(GameManager_MenuItems) / sizeof(struct MenuGuiItem); ++i) {
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
            Text_SetFont(GNW_TEXT_FONT_2);

            if (i == MENU_GUI_ITEM_ENDTURN_BUTTON) {
                flags |= 0x4;
                Text_SetFont(GNW_TEXT_FONT_5);
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
                        GameManager_MenuItemLabelOffsets[i].y, FontColor(0xA2, 0xA7, 0xE6),
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

    Text_SetFont(GNW_TEXT_FONT_5);

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        struct ImageSimpleHeader* image;
        WindowInfo wininfo;
        int32_t color;

        image = reinterpret_cast<struct ImageSimpleHeader*>(
            ResourceManager_LoadResource(static_cast<ResourceID>(R_ENDT_D + i)));

        wininfo.id = window->id;
        wininfo.buffer = &image->transparent_color;
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

        Text_TextBox(&wininfo, _(aeb0), 0, 0, image->width, image->height, true, true, FontColor(color, color, 0x3F));
    }
}

void GameManager_MenuDeinitButtons() {
    for (uint32_t i = 0; i < sizeof(GameManager_MenuItems) / sizeof(struct MenuGuiItem); ++i) {
        delete GameManager_MenuItems[i].button;
        GameManager_MenuItems[i].button = nullptr;
    }
}

void GameManager_DrawBuilderUnitStatusMessage(UnitInfo* unit) {
    SmartString string;

    if (unit->GetUnitType() == BULLDOZR) {
        string.Sprintf(80, _(af4a), unit->build_time);

    } else if (unit->GetOrderState() == ORDER_STATE_UNIT_READY) {
        BaseUnit* base_unit1 = &UnitsManager_BaseUnits[unit->GetUnitType()];
        BaseUnit* base_unit2 = &UnitsManager_BaseUnits[unit->GetParent()->GetUnitType()];

        if (unit->flags & STATIONARY) {
            string.Sprintf(200, GameManager_EventStrings_FinishedBuilding[base_unit2->gender],
                           base_unit2->singular_name, base_unit2->singular_name);
        } else {
            string.Sprintf(200, GameManager_EventStrings_FinishedConstruction[base_unit1->gender],
                           base_unit2->singular_name, base_unit1->singular_name);
        }

    } else {
        SmartObjectArray<ResourceID> build_list = unit->GetBuildList();

        SDL_assert(build_list.GetCount() > 0);

        if (unit->GetOrder() == ORDER_HALT_BUILDING || unit->GetOrder() == ORDER_HALT_BUILDING_2) {
            string.Sprintf(200, _(cdf8), UnitsManager_BaseUnits[*build_list[0]].singular_name, unit->build_time);

        } else {
            int32_t turns_to_build;

            unit->GetTurnsToBuild(*build_list[0], unit->GetBuildRate(), &turns_to_build);

            string.Sprintf(200, _(ee24), UnitsManager_BaseUnits[*build_list[0]].singular_name, turns_to_build);
        }
    }

    MessageManager_DrawMessage(string.GetCStr(), 0, 0);
}

void GameManager_DrawDisabledUnitStatusMessage(UnitInfo* unit) {
    SmartString string;

    string.Sprintf(200, _(bda4), unit->recoil_delay);

    MessageManager_DrawMessage(string.GetCStr(), 0, 0);
}

void GameManager_PlayUnitStatusVoice(UnitInfo* unit) {
    bool is_ammo_low;
    bool is_ammo_depleted;
    bool is_movement_exhausted;
    bool is_damaged;
    bool is_critical;

    int32_t unit_ammo;
    int32_t unit_hits;

    is_ammo_low = false;
    is_ammo_depleted = false;
    is_movement_exhausted = false;
    is_damaged = false;
    is_critical = false;

    SoundManager_PlayVoice(V_START, V_START2, -1);

    unit_ammo = unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO);

    if (unit_ammo) {
        is_ammo_depleted = unit->ammo == 0;
        is_ammo_low = unit->ammo <= (unit_ammo / 4);
    }

    if (unit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
        is_movement_exhausted = unit->speed == 0;
    }

    unit_hits = unit->GetBaseValues()->GetAttribute(ATTRIB_HITS);

    if (unit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | STATIONARY)) {
        is_damaged = unit->hits <= (unit_hits / 2);
        is_critical = unit->hits <= (unit_hits / 4);
    }

    if (is_ammo_depleted) {
        SoundManager_PlayVoice(V_M142, V_F142);

    } else if (is_ammo_low) {
        SoundManager_PlayVoice(V_M138, V_F138);
    }

    if (is_movement_exhausted) {
        SoundManager_PlayVoice(V_M145, V_F145);
    }

    if (is_critical) {
        SoundManager_PlayVoice(V_M154, V_F155);

    } else if (is_damaged) {
        SoundManager_PlayVoice(V_M150, V_F151);
    }

    if (!is_ammo_depleted && !is_ammo_low && !is_movement_exhausted && !is_damaged) {
        if (unit->GetUnitType() == SURVEYOR) {
            SoundManager_PlayVoice(V_M191, V_F192);

        } else if (unit->GetOrder() == ORDER_MOVE_TO_ATTACK) {
            SoundManager_PlayVoice(V_M196, V_F198);

        } else if ((unit->GetOrder() == ORDER_AWAIT || unit->GetOrder() == ORDER_MOVE) && unit->GetLayingState() == 2) {
            SoundManager_PlayVoice(V_M181, V_F182);

        } else if ((unit->GetOrder() == ORDER_AWAIT || unit->GetOrder() == ORDER_MOVE) && unit->GetLayingState() == 1) {
            SoundManager_PlayVoice(V_M186, V_F187);

        } else if (unit->GetOrder() == ORDER_CLEAR) {
            SoundManager_PlayVoice(V_M171, V_F171);

        } else if (unit->GetOrder() == ORDER_SENTRY &&
                   (unit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT))) {
            SoundManager_PlayVoice(V_M158, V_F158);

        } else if (unit->GetOrder() == ORDER_BUILD) {
            if (unit->GetOrderState() == ORDER_STATE_BUILDING_READY) {
                SoundManager_PlayVoice(V_M162, V_F165);

            } else if (unit->GetOrderState() == ORDER_STATE_UNIT_READY) {
                SoundManager_PlayVoice(V_M166, V_F169);

            } else {
                SoundManager_PlayVoice(V_M284, V_F284);
            }

        } else if (unit->GetUnitType() == RESEARCH && unit->GetOrder() == ORDER_POWER_ON &&
                   UnitsManager_TeamInfo[unit->team].research_topics[unit->research_topic].turns_to_complete == 0) {
            SoundManager_PlayVoice(V_M093, V_F093);

        } else if (unit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
            SoundManager_PlayVoice(V_M001, V_F004);
        }
    }
}

void GameManager_DrawUnitStatusMessage(UnitInfo* unit) {
    if (unit->GetOrder() == ORDER_DISABLE && unit->team != PLAYER_TEAM_ALIEN) {
        GameManager_DrawDisabledUnitStatusMessage(unit);
    }

    if (ini_get_setting(INI_GAME_FILE_TYPE) == GAME_TYPE_TRAINING) {
        MessageManager_DrawMessage(UnitsManager_BaseUnits[unit->GetUnitType()].tutorial, 0, 0);
    }

    if (unit->GetOrder() == ORDER_BUILD || unit->GetOrder() == ORDER_CLEAR || unit->GetOrder() == ORDER_HALT_BUILDING ||
        unit->GetOrder() == ORDER_HALT_BUILDING_2) {
        GameManager_DrawBuilderUnitStatusMessage(unit);
    }
}

void GameManager_MenuDeinitDisplayControls() {
    if (GameManager_DisplayControlsInitialized) {
        delete Gamemanager_FlicButton;
        Gamemanager_FlicButton = nullptr;

        GameManager_MenuDeinitButtons();

        for (uint32_t i = 0; i < sizeof(GameManager_MenuDisplayControls) / sizeof(struct MenuDisplayControl); ++i) {
            delete GameManager_MenuDisplayControls[i].image;
            GameManager_MenuDisplayControls[i].image = nullptr;

            delete GameManager_MenuDisplayControls[i].button;
            GameManager_MenuDisplayControls[i].button = nullptr;
        }

        GameManager_DisplayControlsInitialized = false;
        GameManager_IsMainMenuEnabled = false;
    }
}

void GameManager_SpawnNewUnits(uint16_t team, SmartList<UnitInfo>* units, uint16_t* counts) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).build_time && (*it).GetOrder() != ORDER_HALT_BUILDING &&
            (*it).GetOrder() != ORDER_HALT_BUILDING_2) {
            if ((*it).build_time <= (*it).GetBuildRate()) {
                (*it).build_time = 0;

                if ((*it).GetUnitType() != BULLDOZR) {
                    ++counts[(*it).GetConstructedUnitType()];
                }

                (*it).SpawnNewUnit();

            } else {
                (*it).build_time -= (*it).GetBuildRate();
            }
        }
    }
}

char* GameManager_PrependMessageChunk(const char* chunk, char* message) {
    memmove(&message[strlen(chunk)], message, strlen(message) + 1);
    memcpy(message, chunk, strlen(chunk));

    return message;
}

void GameManager_ReportNewUnitsMessage(uint16_t* counts) {
    int32_t unit_count;
    int32_t different_type_count;
    bool flag;
    char message[3000];
    char chunk[400];

    unit_count = 0;
    different_type_count = 0;
    message[0] = '\0';
    flag = false;

    for (int32_t unit_type = UNIT_END - 1; unit_type >= 0; --unit_type) {
        if (counts[unit_type]) {
            if (BuildMenu_GetTurnsToBuild(static_cast<ResourceID>(unit_type), GameManager_PlayerTeam) > 1) {
                flag = true;
            }

            unit_count += counts[unit_type];
            ++different_type_count;

            if (counts[unit_type] > 1) {
                sprintf(chunk, GameManager_EventStrings_NewPlural[UnitsManager_BaseUnits[unit_type].gender],
                        counts[unit_type], UnitsManager_BaseUnits[unit_type].plural_name);

            } else {
                sprintf(chunk, GameManager_EventStrings_NewSingular[UnitsManager_BaseUnits[unit_type].gender],
                        UnitsManager_BaseUnits[unit_type].singular_name);
            }

            if (different_type_count == 1) {
                strcat(message, chunk);

            } else {
                if (different_type_count == 2) {
                    strcat(chunk, _(e3ee));

                } else {
                    strcat(chunk, _(0a07));
                }

                SDL_assert(strlen(chunk) + strlen(message) < sizeof(message) + 1);

                GameManager_PrependMessageChunk(chunk, message);
            }
        }
    }

    if (unit_count > 0) {
        message[0] = toupper(message[0]);

        if (unit_count == 1) {
            strcat(message, _(a8bd));
            strcat(message, _(e3cd));

            if (flag) {
                SoundManager_PlayVoice(V_M215, V_F217);
            }

        } else {
            strcat(message, _(2cd0));
            strcat(message, _(9753));

            if (flag) {
                SoundManager_PlayVoice(V_M206, V_F207);
            }
        }
    }

    sprintf(chunk, _(9733), GameManager_TurnCounter);

    GameManager_PrependMessageChunk(chunk, message);

    MessageManager_DrawMessage(message, 0, 0, false, true);
}

void GameManager_ProgressBuildState(uint16_t team) {
    uint16_t unit_counters[UNIT_END];

    ResearchMenu_NewTurn(team);

    memset(unit_counters, 0, sizeof(unit_counters));

    GameManager_SpawnNewUnits(team, &UnitsManager_GroundCoverUnits, unit_counters);
    GameManager_SpawnNewUnits(team, &UnitsManager_MobileLandSeaUnits, unit_counters);
    GameManager_SpawnNewUnits(team, &UnitsManager_StationaryUnits, unit_counters);
    GameManager_SpawnNewUnits(team, &UnitsManager_MobileAirUnits, unit_counters);

    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER || GameManager_PlayerTeam == team) {
        GameManager_ReportNewUnitsMessage(unit_counters);
    }

    const auto& complexes = UnitsManager_TeamInfo[team].team_units->GetComplexes();

    for (SmartList<Complex>::Iterator it = complexes.Begin(); it != complexes.End(); ++it) {
        Access_UpdateResourcesTotal(&(*it));
        GameManager_OptimizeProduction(team, &(*it), false, true);
    }

    GameManager_GameState = GAME_STATE_8_IN_GAME;
}

void GameManager_DrawProximityZones() {
    int32_t ini_proximity_range;
    int32_t ini_exclude_range;
    int32_t range;
    Rect bounds;

    SmartPointer<UnitValues> base_values = GameManager_SelectedUnit->GetBaseValues();

    GameManager_UpdateDrawBounds();

    range = 0;
    base_values->SetAttribute(ATTRIB_RANGE, range);

    ini_proximity_range = ini_get_setting(INI_PROXIMITY_RANGE);
    ini_exclude_range = ini_get_setting(INI_EXCLUDE_RANGE);

    for (int32_t proximity_range = 1; proximity_range <= ini_proximity_range; ++proximity_range) {
        base_values->SetAttribute(ATTRIB_SCAN, proximity_range);

        if (range < ini_exclude_range) {
            ++range;
            base_values->SetAttribute(ATTRIB_RANGE, range);
        }

        bounds.ulx = GameManager_SelectedUnit->x - (proximity_range << 6);
        bounds.lrx = GameManager_SelectedUnit->x + (proximity_range << 6);
        bounds.uly = GameManager_SelectedUnit->y - (proximity_range << 6);
        bounds.lry = GameManager_SelectedUnit->y + (proximity_range << 6);

        GameManager_AddDrawBounds(&bounds);

        while (!GameManager_ProcessTick(false)) {
        }
    }
}

Point GameManager_GetMinimapPosition() {
    const double scale = WindowManager_GetScale();
    double map_scale;
    WindowInfo* minimap = WindowManager_GetWindow(WINDOW_MINIMAP);
    Point minimap_position;
    constexpr int32_t minimap_slot_size{112};
    Rect minimap_bounds;
    Point mouse_position{GameManager_MouseX, GameManager_MouseY};
    Point window_offset{ResourceManager_MinimapWindowOffset};

    if (GameManager_DisplayButtonMinimap2x) {
        window_offset.x /= 2;
        window_offset.y /= 2;
    }

    minimap_bounds.ulx = minimap->window.ulx + ResourceManager_MinimapWindowOffset.x;
    minimap_bounds.uly = minimap->window.uly + ResourceManager_MinimapWindowOffset.y;
    minimap_bounds.lrx = minimap_bounds.ulx + ResourceManager_MinimapWindowSize.x + 1;
    minimap_bounds.lry = minimap_bounds.uly + ResourceManager_MinimapWindowSize.y + 1;

    if (Access_IsInsideBounds(&minimap_bounds, &mouse_position)) {
        minimap_position.x = (GameManager_MouseX - minimap->window.ulx - ResourceManager_MinimapWindowOffset.x) / scale;
        minimap_position.y = (GameManager_MouseY - minimap->window.uly - ResourceManager_MinimapWindowOffset.y) / scale;

        if (ResourceManager_MapSize.x >= ResourceManager_MapSize.y) {
            map_scale = ResourceManager_MapSize.x / static_cast<double>(minimap_slot_size);

        } else {
            map_scale = ResourceManager_MapSize.y / static_cast<double>(minimap_slot_size);
        }

        minimap_position.x *= map_scale;
        minimap_position.y *= map_scale;

        minimap_position.x = std::min(ResourceManager_MapSize.x - 1, std::max<int32_t>(0, minimap_position.x));
        minimap_position.y = std::min(ResourceManager_MapSize.y - 1, std::max<int32_t>(0, minimap_position.y));

        if (GameManager_DisplayButtonMinimap2x) {
            minimap_position.x = (minimap_position.x / 2) + GameManager_GridCenterOffset.x;
            minimap_position.y = (minimap_position.y / 2) + GameManager_GridCenterOffset.y;
        }

    } else {
        minimap_position.x = -1;
        minimap_position.y = -1;
    }

    return minimap_position;
}

void GameManager_QuickBuildMenuDrawPortraits(WindowInfo* window, ResourceID id1, ResourceID id2, int32_t width) {
    uint16_t unit_id{id1};

    for (int32_t i = 0; i < MENU_QUICK_BUILD_UNIT_SLOTS; ++i) {
        auto base_unit{&UnitsManager_BaseUnits[unit_id]};
        Rect bounds;

        bounds.ulx = GameManager_QuickBuildMenuItems[i].ulx;
        bounds.uly = GameManager_QuickBuildMenuItems[i].uly;
        bounds.lrx = bounds.ulx + GameManager_QuickBuildMenuItems[i].width;
        bounds.lry = bounds.uly + GameManager_QuickBuildMenuItems[i].height;

        ++unit_id;

        if (unit_id > id2) {
            win_print(window->id, " ", GameManager_QuickBuildMenuItems[i].width, bounds.ulx, bounds.lry,
                      0x20 | GNW_TEXT_REFRESH_WINDOW | GNW_TEXT_ALLOW_TRUNCATED);
            win_disable_button(GameManager_QuickBuildMenuItems[i].bid);
            buf_fill(&window->buffer[bounds.uly * width + bounds.ulx], GameManager_QuickBuildMenuItems[i].width,
                     GameManager_QuickBuildMenuItems[i].height, width, COLOR_BLACK);

        } else {
            win_print(window->id, base_unit->singular_name, GameManager_QuickBuildMenuItems[i].width, bounds.ulx,
                      bounds.lry, 0x20 | GNW_TEXT_REFRESH_WINDOW | GNW_TEXT_ALLOW_TRUNCATED);
            flicsmgr_construct(base_unit->flics, window, width, bounds.ulx, bounds.uly, false, false);
            win_enable_button(GameManager_QuickBuildMenuItems[i].bid);
        }

        win_draw_rect(window->id, &bounds);
    }
}

void GameManager_QuickBuildMenu() {
    auto main_map_window{WindowManager_GetWindow(WINDOW_MAIN_MAP)};
    auto window{WindowManager_GetWindow(WINDOW_POPUP_BUTTONS)};
    constexpr uint16_t window_width{416};
    constexpr uint16_t window_height{345};

    window->id = win_add(main_map_window->window.ulx + 20, main_map_window->window.uly + 20, window_width,
                         window_height, COLOR_BLACK, WINDOW_NO_FLAGS);
    window->buffer = win_get_buf(window->id);

    if (WindowManager_LoadBigImage(QWKBUILD, window, window_width, false)) {
        GameManager_FillOrRestoreWindow(WINDOW_CORNER_FLIC, COLOR_BLACK, true);
        GameManager_FillOrRestoreWindow(WINDOW_STAT_WINDOW, COLOR_BLACK, true);

        Cursor_SetCursor(CURSOR_HAND);

        for (auto& item : GameManager_QuickBuildMenuItems) {
            auto image_off = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(item.id1));
            auto image_on = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(item.id2));
            uint8_t* up;
            uint8_t* down;

            if (image_off != nullptr) {
                up = &image_off->transparent_color;

            } else {
                up = nullptr;
            }

            if (image_on != nullptr) {
                down = &image_on->transparent_color;

            } else {
                down = nullptr;
            }

            item.bid = win_register_button(window->id, item.ulx, item.uly, item.width, item.height, -1, -1, -1,
                                           item.r_value, up, down, nullptr, item.flags);
        }

        ResourceID unit_id_limit{UNIT_END};
        uint16_t page_min_id{0};
        uint16_t page_max_id{static_cast<uint16_t>(
            (((unit_id_limit - 1) / MENU_QUICK_BUILD_UNIT_SLOTS) + 1) * MENU_QUICK_BUILD_UNIT_SLOTS + page_min_id)};

        unit_id_limit = static_cast<ResourceID>(static_cast<uint16_t>(unit_id_limit) + page_min_id);

        GameManager_QuickBuildMenuDrawPortraits(window, GameManager_QuickBuildMenuUnitId, unit_id_limit, window_width);

        int32_t key;

        do {
            key = get_input();

            if (GameManager_RequestMenuExit) {
                key = 1000;
            }

            switch (key) {
                case GNW_KB_KEY_ESCAPE: {
                    key = 1000;

                    GameManager_QuickBuildMenuActive = false;

                    GameManager_SelectNextUnit(1);
                } break;

                case 1000: {
                    GameManager_QuickBuildMenuActive = false;

                    GameManager_SelectNextUnit(1);
                } break;

                case 1001: {
                    if (static_cast<uint16_t>(GameManager_QuickBuildMenuUnitId) - MENU_QUICK_BUILD_UNIT_SLOTS >=
                        page_min_id) {
                        GameManager_QuickBuildMenuUnitId = static_cast<ResourceID>(
                            static_cast<uint16_t>(GameManager_QuickBuildMenuUnitId) - MENU_QUICK_BUILD_UNIT_SLOTS);

                        GameManager_QuickBuildMenuDrawPortraits(window, GameManager_QuickBuildMenuUnitId, unit_id_limit,
                                                                window_width);
                    }
                } break;

                case 1002: {
                    if (static_cast<uint16_t>(GameManager_QuickBuildMenuUnitId) + MENU_QUICK_BUILD_UNIT_SLOTS <
                        page_max_id) {
                        GameManager_QuickBuildMenuUnitId = static_cast<ResourceID>(
                            static_cast<uint16_t>(GameManager_QuickBuildMenuUnitId) + MENU_QUICK_BUILD_UNIT_SLOTS);

                        GameManager_QuickBuildMenuDrawPortraits(window, GameManager_QuickBuildMenuUnitId, unit_id_limit,
                                                                window_width);
                    }
                } break;

                case 1003:
                case 1004:
                case 1005:
                case 1006:
                case 1007:
                case 1008: {
                    GameManager_QuickBuildUnitId =
                        static_cast<ResourceID>(GameManager_QuickBuildMenuUnitId + key - 1003);
                    GameManager_QuickBuildMenuActive = true;

                    auto base_unit{&UnitsManager_BaseUnits[GameManager_QuickBuildUnitId]};
                    auto portrait_window{WindowManager_GetWindow(WINDOW_CORNER_FLIC)};
                    auto main_window{WindowManager_GetWindow(WINDOW_MAIN_WINDOW)};

                    flicsmgr_construct(base_unit->flics, main_window, main_window->width, portrait_window->window.ulx,
                                       portrait_window->window.uly, false, false);

                    GameManager_QuickBuilderUnit =
                        UnitsManager_SpawnUnit(GameManager_QuickBuildUnitId, GameManager_PlayerTeam, 0, 0, nullptr);

                    key = 1000;
                } break;
            }

            GameManager_ProcessState(true);

        } while (key != 1000);

        for (auto& item : GameManager_QuickBuildMenuItems) {
            win_delete_button(item.bid);
        }
    }

    win_delete(window->id);
}

bool GameManager_IsActiveTurn(uint16_t team) {
    return (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team);
}
