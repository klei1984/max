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

#include "quickbuild.hpp"

#include <vector>

#include "access.hpp"
#include "cursor.hpp"
#include "flicsmgr.hpp"
#include "game_manager.hpp"
#include "gnw.h"
#include "hash.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "sound_manager.hpp"
#include "unit.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

#define MENU_QUICK_BUILD_GUI_ITEM_DEF(id1, id2, ulx, uly, width, height, r_value) \
    {(r_value), (id1), (id2), (ulx), (uly), (width), (height), (0), (0)}

#define MENU_QUICK_BUILD_UNIT_SLOTS (6)

bool QuickBuild_MenuActive;
SmartPointer<UnitInfo> QuickBuild_PreviewUnit;

static ResourceID QuickBuild_UnitId;
static ResourceID QuickBuild_MenuUnitId;
static int32_t QuickBuild_SelectedTeam = 0;

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

static struct QuickBuildMenuGuiItem QuickBuild_MenuItems[] = {
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 8, 7, 128, 128, 1003),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 145, 7, 128, 128, 1004),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 280, 7, 128, 128, 1005),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 8, 167, 128, 128, 1006),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 145, 167, 128, 128, 1007),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(INVALID_ID, INVALID_ID, 280, 167, 128, 128, 1008),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(QWKCN_OF, QWKCN_ON, 291, 316, 24, 24, 1009),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(QWKDN_OF, QWKDN_ON, 323, 316, 24, 24, 1002),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(QWKUP_OF, QWKUP_ON, 355, 316, 24, 24, 1001),
    MENU_QUICK_BUILD_GUI_ITEM_DEF(QWKCN_OF, QWKCN_ON, 387, 316, 24, 24, 1000),
};

static void QuickBuild_MenuDrawPortraits(WindowInfo* window, const std::vector<ResourceID>& valid_units,
                                         int32_t start_index, int32_t width);
static void QuickBuild_DrawTeamIndicator(WindowInfo* window);

void QuickBuild_DrawTeamIndicator(WindowInfo* window) {
    char team_text[2];
    int32_t text_color;

    team_text[0] = '0' + QuickBuild_SelectedTeam;
    team_text[1] = '\0';

    switch (QuickBuild_SelectedTeam) {
        case 0: {
            text_color = 0x20;
        } break;

        case 1: {
            text_color = COLOR_RED;
        } break;

        case 2: {
            text_color = COLOR_GREEN;
        } break;

        case 3: {
            text_color = COLOR_BLUE;
        } break;

        case 4: {
            text_color = 0xFF;
        } break;

        case 5: {
            text_color = COLOR_YELLOW;
        } break;

        default: {
            text_color = 0x20;
        } break;
    }

    win_print(window->id, team_text, 20, 260, 330, text_color | GNW_TEXT_REFRESH_WINDOW);
}

void QuickBuild_MenuDrawPortraits(WindowInfo* window, const std::vector<ResourceID>& valid_units, int32_t start_index,
                                  int32_t width) {
    for (int32_t i = 0; i < MENU_QUICK_BUILD_UNIT_SLOTS; ++i) {
        const int32_t unit_index{start_index + i};
        Rect bounds;

        bounds.ulx = QuickBuild_MenuItems[i].ulx;
        bounds.uly = QuickBuild_MenuItems[i].uly;
        bounds.lrx = bounds.ulx + QuickBuild_MenuItems[i].width;
        bounds.lry = bounds.uly + QuickBuild_MenuItems[i].height;

        if (unit_index >= static_cast<int32_t>(valid_units.size())) {
            win_print(window->id, " ", QuickBuild_MenuItems[i].width, bounds.ulx, bounds.lry,
                      0x20 | GNW_TEXT_REFRESH_WINDOW | GNW_TEXT_ALLOW_TRUNCATED);
            win_disable_button(QuickBuild_MenuItems[i].bid);
            buf_fill(&window->buffer[bounds.uly * width + bounds.ulx], QuickBuild_MenuItems[i].width,
                     QuickBuild_MenuItems[i].height, width, COLOR_BLACK);

        } else {
            const Unit& base_unit{ResourceManager_GetUnit(valid_units[unit_index])};

            win_print(window->id, base_unit.GetSingularName().data(), QuickBuild_MenuItems[i].width, bounds.ulx,
                      bounds.lry, 0x20 | GNW_TEXT_REFRESH_WINDOW | GNW_TEXT_ALLOW_TRUNCATED);
            flicsmgr_construct(base_unit.GetFlicsAnimation(), window, width, bounds.ulx, bounds.uly, false, false);
            win_enable_button(QuickBuild_MenuItems[i].bid);
        }

        win_draw_rect(window->id, &bounds);
    }
}

void QuickBuild_Menu() {
    auto main_map_window{WindowManager_GetWindow(WINDOW_MAIN_MAP)};
    auto window{WindowManager_GetWindow(WINDOW_POPUP_BUTTONS)};
    constexpr uint16_t window_width{416};
    constexpr uint16_t window_height{345};

    // Save and enable AllVisible mode to make all units visible during quick build
    const bool saved_all_visible = GameManager_AllVisible;
    GameManager_AllVisible = true;

    // Build filtered list of valid unit types
    std::vector<ResourceID> valid_units;
    valid_units.reserve(UNIT_END - UNIT_START);

    for (int32_t i = UNIT_START; i < UNIT_END; ++i) {
        if (QuickBuild_IsUnitValid(static_cast<ResourceID>(i))) {
            valid_units.push_back(static_cast<ResourceID>(i));
        }
    }

    if (valid_units.empty()) {
        return;
    }

    window->id = win_add(main_map_window->window.ulx + 20, main_map_window->window.uly + 20, window_width,
                         window_height, COLOR_BLACK, WINDOW_NO_FLAGS);
    window->buffer = win_get_buf(window->id);

    if (WindowManager_LoadBigImage(QWKBUILD, window, window_width, false)) {
        GameManager_FillOrRestoreWindow(WINDOW_CORNER_FLIC, COLOR_BLACK, true);
        GameManager_FillOrRestoreWindow(WINDOW_STAT_WINDOW, COLOR_BLACK, true);

        Cursor_SetCursor(CURSOR_HAND);

        for (auto& item : QuickBuild_MenuItems) {
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

            // Draw initial button image to window buffer and refresh to screen
            if (image_off != nullptr) {
                Rect button_rect;

                buf_to_buf(&image_off->transparent_color, image_off->width, image_off->height, image_off->width,
                           &window->buffer[item.uly * window_width + item.ulx], window_width);

                button_rect.ulx = item.ulx;
                button_rect.uly = item.uly;
                button_rect.lrx = item.ulx + item.width;
                button_rect.lry = item.uly + item.height;

                win_draw_rect(window->id, &button_rect);
            }
        }

        // Find current page index based on QuickBuild_MenuUnitId
        int32_t current_page_index{0};

        for (size_t i = 0; i < valid_units.size(); ++i) {
            if (valid_units[i] >= QuickBuild_MenuUnitId) {
                current_page_index =
                    (static_cast<int32_t>(i) / MENU_QUICK_BUILD_UNIT_SLOTS) * MENU_QUICK_BUILD_UNIT_SLOTS;
                break;
            }
        }

        QuickBuild_MenuDrawPortraits(window, valid_units, current_page_index, window_width);
        QuickBuild_DrawTeamIndicator(window);

        int32_t key;

        do {
            key = get_input();

            if (GameManager_RequestMenuExit) {
                key = 1000;
            }

            switch (key) {
                case GNW_KB_KEY_ESCAPE: {
                    key = 1000;

                    QuickBuild_MenuActive = false;

                    GameManager_MenuUnitSelect(nullptr);
                } break;

                case 1000: {
                    QuickBuild_MenuActive = false;

                    GameManager_MenuUnitSelect(nullptr);
                } break;

                case 1001: {
                    // Page up
                    if (current_page_index - MENU_QUICK_BUILD_UNIT_SLOTS >= 0) {
                        current_page_index -= MENU_QUICK_BUILD_UNIT_SLOTS;

                        QuickBuild_MenuDrawPortraits(window, valid_units, current_page_index, window_width);
                    }
                } break;

                case 1002: {
                    // Page down
                    if (current_page_index + MENU_QUICK_BUILD_UNIT_SLOTS < static_cast<int32_t>(valid_units.size())) {
                        current_page_index += MENU_QUICK_BUILD_UNIT_SLOTS;

                        QuickBuild_MenuDrawPortraits(window, valid_units, current_page_index, window_width);
                    }
                } break;

                case 1003:
                case 1004:
                case 1005:
                case 1006:
                case 1007:
                case 1008: {
                    const int32_t selected_index{current_page_index + key - 1003};

                    if (selected_index < static_cast<int32_t>(valid_units.size())) {
                        QuickBuild_UnitId = valid_units[selected_index];
                        QuickBuild_MenuUnitId = QuickBuild_UnitId;
                        QuickBuild_MenuActive = true;

                        const Unit& base_unit{ResourceManager_GetUnit(QuickBuild_UnitId)};
                        auto portrait_window{WindowManager_GetWindow(WINDOW_CORNER_FLIC)};
                        auto main_window{WindowManager_GetWindow(WINDOW_MAIN_WINDOW)};

                        flicsmgr_construct(base_unit.GetFlicsAnimation(), main_window, main_window->width,
                                           portrait_window->window.ulx, portrait_window->window.uly, false, false);

                        const uint16_t spawn_team = (QuickBuild_SelectedTeam == 0)
                                                        ? GameManager_PlayerTeam
                                                        : static_cast<uint16_t>(QuickBuild_SelectedTeam - 1);

                        QuickBuild_PreviewUnit = UnitsManager_SpawnUnit(QuickBuild_UnitId, spawn_team, 0, 0, nullptr);

                        // Set ORDER_IDLE so AI systems ignore this preview unit (it should not be attacked, cause enemy
                        // reactions, or fire on enemies until actually deployed). The renderer has special handling to
                        // allow QuickBuild preview units (QuickBuild_PreviewUnit) despite ORDER_IDLE.
                        QuickBuild_PreviewUnit->SetOrder(ORDER_IDLE);

                        QuickBuild_UpdateAmphibiousUnitImageBase(&*QuickBuild_PreviewUnit, GameManager_MousePosition);

                        key = 1000;
                    }
                } break;

                case 1009: {
                    // Team selection - cycle through 0-5
                    QuickBuild_SelectedTeam = (QuickBuild_SelectedTeam + 1) % 6;
                    QuickBuild_DrawTeamIndicator(window);
                } break;
            }

            GameManager_ProcessState(true);

        } while (key != 1000);

        for (auto& item : QuickBuild_MenuItems) {
            win_delete_button(item.bid);
        }
    }

    win_delete(window->id);

    // Restore AllVisible mode
    GameManager_AllVisible = saved_all_visible;
}

bool QuickBuild_IsUnitValid(ResourceID unit_type) {
    if (unit_type >= UNIT_END) {
        return false;
    }

    if ((ResourceManager_GetUnit(unit_type).GetFlags() & (MISSILE_UNIT | EXPLODING)) || unit_type == LRGSLAB ||
        unit_type == SMLSLAB || unit_type == LRGCONES || unit_type == SMLCONES || unit_type == LRGTAPE ||
        unit_type == SMLTAPE) {
        return false;
    }

    return true;
}

bool QuickBuild_IsPlacementValid(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    ResourceID unit_type{unit->GetUnitType()};
    SmartPointer<UnitInfo> parent{unit->GetParent()};
    const uint32_t unit_flags{ResourceManager_GetUnit(unit->GetUnitType()).GetFlags()};
    uint32_t result;

    Hash_MapHash.Remove(unit);

    if (parent != nullptr) {
        Hash_MapHash.Remove(parent.Get());
        unit_type = parent->GetConstructedUnitType();
    }

    if (unit_flags & BUILDING) {
        if (grid_x >= 0 && grid_y >= 0 && grid_x <= ResourceManager_MapSize.x - 2 &&
            grid_y <= ResourceManager_MapSize.y - 2) {
            result = Access_IsAccessible(unit_type, unit->team, grid_x, grid_y,
                                         AccessModifier_IgnoreVisibility | AccessModifier_SameClassBlocks) &&
                     Access_IsAccessible(unit_type, unit->team, grid_x + 1, grid_y,
                                         AccessModifier_IgnoreVisibility | AccessModifier_SameClassBlocks) &&
                     Access_IsAccessible(unit_type, unit->team, grid_x, grid_y + 1,
                                         AccessModifier_IgnoreVisibility | AccessModifier_SameClassBlocks) &&
                     Access_IsAccessible(unit_type, unit->team, grid_x + 1, grid_y + 1,
                                         AccessModifier_IgnoreVisibility | AccessModifier_SameClassBlocks);
        } else {
            result = 0;
        }

    } else {
        if (Access_GetModifiedSurfaceType(grid_x, grid_y) == SURFACE_TYPE_AIR) {
            result = 0;

        } else {
            if (unit_type == LANDMINE || unit_type == SEAMINE) {
                bool has_valid_ground_cover = false;
                bool has_blocking_units = false;
                const auto units = Hash_MapHash[Point(grid_x, grid_y)];

                if (units) {
                    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                        const ResourceID other_type = (*it).GetUnitType();

                        if (unit_type == LANDMINE) {
                            if (other_type == BRIDGE || other_type == ROAD || other_type == LRGRUBLE ||
                                other_type == SMLRUBLE) {
                                has_valid_ground_cover = true;
                            }

                        } else if (unit_type == SEAMINE) {
                            if (other_type == BRIDGE) {
                                has_valid_ground_cover = true;
                            }
                        }

                        if (other_type == LANDMINE || other_type == SEAMINE) {
                            has_blocking_units = true;
                            break;

                        } else if (((*it).GetOrder() != ORDER_IDLE || ((*it).flags & STATIONARY)) &&
                                   other_type != BRIDGE && other_type != ROAD && other_type != LRGRUBLE &&
                                   other_type != SMLRUBLE && other_type != WTRPLTFM && other_type != CNCT_4W) {
                            has_blocking_units = true;
                            break;
                        }
                    }
                }

                if (has_valid_ground_cover && !has_blocking_units) {
                    result = 1;

                } else {
                    result = Access_IsAccessible(unit_type, unit->team, grid_x, grid_y,
                                                 AccessModifier_IgnoreVisibility | AccessModifier_SameClassBlocks);
                }

            } else {
                result = Access_IsAccessible(unit_type, unit->team, grid_x, grid_y,
                                             AccessModifier_IgnoreVisibility | AccessModifier_SameClassBlocks);
            }

            // Do not allow placement of landed aircraft on any unit except landing pads
            if (result && (unit_flags & MOBILE_AIR_UNIT) && !(unit->flags & HOVERING)) {
                const auto units = Hash_MapHash[Point(grid_x, grid_y)];

                if (units) {
                    bool landing_pad_present = false;

                    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                        if ((*it).GetOrder() != ORDER_IDLE || ((*it).flags & STATIONARY)) {
                            if ((*it).GetUnitType() == LANDPAD) {
                                landing_pad_present = true;
                                break;
                            }
                        }
                    }

                    if (!landing_pad_present) {
                        result = 0;
                    }
                }
            }
        }
    }

    // Do not allow placement on cells with enemy mines
    if (result) {
        if (unit_flags & BUILDING) {
            if (Access_GetEnemyMineOnSentry(unit->team, grid_x, grid_y) ||
                Access_GetEnemyMineOnSentry(unit->team, grid_x + 1, grid_y) ||
                Access_GetEnemyMineOnSentry(unit->team, grid_x, grid_y + 1) ||
                Access_GetEnemyMineOnSentry(unit->team, grid_x + 1, grid_y + 1)) {
                result = 0;
            }

        } else {
            if (Access_GetEnemyMineOnSentry(unit->team, grid_x, grid_y)) {
                result = 0;
            }
        }
    }

    Hash_MapHash.Add(unit);

    if (parent != nullptr) {
        Hash_MapHash.Add(parent.Get());
    }

    return result;
}

void QuickBuild_UpdateAmphibiousUnitImageBase(UnitInfo* unit, Point position) {
    if ((unit->flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) == (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
        int32_t surface_type = Access_GetModifiedSurfaceType(position.x, position.y);
        int32_t image_base;

        if (unit->GetUnitType() == CLNTRANS) {
            image_base = (surface_type == SURFACE_TYPE_WATER) ? 0 : 8;

        } else {
            image_base = (surface_type == SURFACE_TYPE_WATER) ? 8 : 0;
        }

        if (image_base != unit->image_base) {
            if (unit->GetUnitType() == CLNTRANS) {
                unit->firing_image_base = image_base;

            } else {
                unit->firing_image_base = image_base + 16;
            }

            unit->UpdateSpriteFrame(image_base, unit->image_index_max);
        }
    }
}

ResourceID QuickBuild_GetUnitId() { return QuickBuild_UnitId; }

void QuickBuild_SetMenuUnitId(ResourceID unit_id) { QuickBuild_MenuUnitId = unit_id; }

ResourceID QuickBuild_GetMenuUnitId() { return QuickBuild_MenuUnitId; }

UnitInfo* QuickBuild_GetTargetUnit(int32_t grid_x, int32_t grid_y) {
    UnitInfo* unit{nullptr};

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // Determine which team to search for based on QuickBuild_SelectedTeam
            // 0 = active team, 1-5 = specific team (RED, GREEN, BLUE, GRAY, ALIEN)
            const uint16_t target_team = (QuickBuild_SelectedTeam == 0)
                                             ? GameManager_PlayerTeam
                                             : static_cast<uint16_t>(QuickBuild_SelectedTeam - 1);

            // First pass: Check for units of the target team
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                // Skip the quick builder preview unit that follows the mouse cursor
                if (it->Get() == QuickBuild_PreviewUnit.Get()) {
                    continue;
                }

                if ((*it).team == target_team) {
                    const auto unit_type = (*it).GetUnitType();

                    // For alien team, check for alien-specific units
                    if (target_team == PLAYER_TEAM_ALIEN) {
                        if (unit_type == SHIELDGN || unit_type == SUPRTPLT || unit_type == RECCENTR ||
                            unit_type == ALNTANK || unit_type == ALNASGUN || unit_type == ALNPLANE) {
                            unit = it->Get();
                            break;
                        }
                    }

                    // Check for ground cover units
                    if (((*it).flags & GROUND_COVER) || unit_type == HARVSTER || unit_type == WALDO) {
                        unit = it->Get();
                        break;
                    }

                    // For non-ground-cover units, only select if they're stationary or have valid orders
                    if ((*it).GetOrder() != ORDER_IDLE || ((*it).flags & STATIONARY)) {
                        unit = it->Get();
                        break;
                    }
                }
            }
        }
    }

    return unit;
}

void QuickBuild_ProcessClick(UnitInfo* unit, Point mouse_position, uint32_t mouse_buttons, uint16_t player_team) {
    if (mouse_buttons & MOUSE_RELEASE_RIGHT) {
        if (!unit) {
            unit = QuickBuild_GetTargetUnit(mouse_position.x, mouse_position.y);
        }

        if (unit) {
            UnitsManager_SetNewOrder(unit, ORDER_BUILD, ORDER_STATE_BUILD_CLEARING);

            if (QuickBuild_PreviewUnit) {
                QuickBuild_PreviewUnit->RefreshScreen();
            }
        }

    } else {
        if (QuickBuild_IsPlacementValid(&*QuickBuild_PreviewUnit, mouse_position.x, mouse_position.y)) {
            ResourceManager_GetSoundManager().PlaySfx(KCARG0);

            const uint16_t deploy_team =
                (QuickBuild_SelectedTeam == 0) ? player_team : static_cast<uint16_t>(QuickBuild_SelectedTeam - 1);

            if (Remote_IsNetworkGame) {
                Remote_SendNetPacket_14(deploy_team, QuickBuild_GetUnitId(), mouse_position.x, mouse_position.y);
            }

            GameManager_DeployUnit(deploy_team, QuickBuild_GetUnitId(), mouse_position.x, mouse_position.y);
        } else {
            ResourceManager_GetSoundManager().PlaySfx(NCANC0);
        }
    }
}
