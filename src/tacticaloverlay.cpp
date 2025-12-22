/* Copyright (c) 2025 M.A.X. Port Team
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

#include "tacticaloverlay.hpp"

#include <SDL3/SDL.h>

#include <algorithm>

#include "access.hpp"
#include "ai.hpp"
#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "hash.hpp"
#include "resource_manager.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"
#include "world.hpp"

static bool TacticalOverlay_Enabled = false;
static int32_t TacticalOverlay_Mode = 1;

void TacticalOverlay_Init() {
    TacticalOverlay_Enabled = false;
    TacticalOverlay_Mode = 1;
}

void TacticalOverlay_Toggle() { TacticalOverlay_Enabled = !TacticalOverlay_Enabled; }

void TacticalOverlay_SetMode(int32_t mode) {
    if (mode >= 1 && mode <= 7) {
        TacticalOverlay_Mode = mode;
    }
}

static void TacticalOverlay_RenderGridCellValue(WindowInfo* window, int32_t grid_x, int32_t grid_y, const char* text) {
    constexpr int32_t GRID_CELL_SIZE = 64;
    const int32_t half_cell = GRID_CELL_SIZE / 2;

    Rect text_area;
    Rect draw_area;

    // Calculate pixel position for the center of the grid cell
    text_area.ulx = (((grid_x * GRID_CELL_SIZE + half_cell) << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx;
    text_area.uly = (((grid_y * GRID_CELL_SIZE + half_cell) << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUly;

    // Set text box size around center point (adjust based on zoom)
    const int32_t box_size = (GRID_CELL_SIZE << 16) / Gfx_MapScalingFactor;

    text_area.ulx -= box_size / 2;
    text_area.uly -= box_size / 2;
    text_area.lrx = text_area.ulx + box_size;
    text_area.lry = text_area.uly + box_size;

    if (text_area.ulx >= text_area.lrx || text_area.uly >= text_area.lry) {
        return;
    }

    // Set up clipping bounds
    draw_area.ulx = 0;
    draw_area.uly = 0;
    draw_area.lrx =
        ((GameManager_MapWindowDrawBounds.lrx - GameManager_MapWindowDrawBounds.ulx) * GFX_SCALE_DENOMINATOR) /
        Gfx_MapScalingFactor;
    draw_area.lry =
        ((GameManager_MapWindowDrawBounds.lry - GameManager_MapWindowDrawBounds.uly) * GFX_SCALE_DENOMINATOR) /
        Gfx_MapScalingFactor;

    const auto font_index = Text_GetFont();
    const uint32_t zoom_level = Gfx_ZoomLevel;

    // Select font based on zoom level
    if (zoom_level >= 42) {
        Text_SetFont(GNW_TEXT_FONT_1);

    } else if (zoom_level >= 24) {
        Text_SetFont(GNW_TEXT_FONT_5);

    } else {
        Text_SetFont(GNW_TEXT_FONT_2);
    }

    // Render text with outline for visibility
    Text_AutofitTextBox(window->buffer, window->width, text, &text_area, &draw_area, GNW_TEXT_OUTLINE | COLOR_YELLOW,
                        true);

    Text_SetFont(font_index);
}

static uint32_t TacticalOverlay_GetCellValue(int32_t grid_x, int32_t grid_y, int32_t mode) {
    const uint16_t team = GameManager_PlayerTeam;
    CTInfo* team_info = &UnitsManager_TeamInfo[team];
    uint32_t result = 0;

    switch (mode) {
        case 1: {
            // Mode 1: Overall heatmap value
            if (team_info->heat_map) {
                result = team_info->heat_map->GetComplete(grid_x, grid_y);
            }
        } break;

        case 2: {
            // Mode 2: Land stealth heatmap value
            if (team_info->heat_map) {
                result = team_info->heat_map->GetStealthLand(grid_x, grid_y);
            }
        } break;

        case 3: {
            // Mode 3: Sea stealth heatmap value
            if (team_info->heat_map) {
                result = team_info->heat_map->GetStealthSea(grid_x, grid_y);
            }
        } break;

        case 4: {
            // Mode 4: Number of team units at grid cell
            const auto units = Hash_MapHash[Point(grid_x, grid_y)];

            if (units) {
                uint32_t count = 0;

                for (auto it = units->Begin(); it != units->End(); ++it) {
                    if ((*it).team == team) {
                        ++count;
                    }
                }

                result = count;
            }
        } break;

        case 5: {
            // Mode 5: Surface type from WRL file
            const World* world = ResourceManager_GetActiveWorld();

            if (world) {
                result = world->GetSurfaceType(grid_x, grid_y);
            }
        } break;

        case 6: {
            // Mode 6: Sea to land TerrainDistanceField value (water unit range field)
            if (AiPlayer_TerrainDistanceField) {
                const uint32_t value =
                    AiPlayer_TerrainDistanceField->GetMinimumRange(Point(grid_x, grid_y), SURFACE_TYPE_WATER);

                result = value;
            }
        } break;

        case 7: {
            // Mode 7: Land to sea TerrainDistanceField value (land unit range field)
            if (AiPlayer_TerrainDistanceField) {
                const uint32_t value =
                    AiPlayer_TerrainDistanceField->GetMinimumRange(Point(grid_x, grid_y), SURFACE_TYPE_LAND);

                result = value;
            }
        } break;
    }

    return result;
}

static bool TacticalOverlay_ShouldDisplayAsHex(int32_t mode) {
    // Modes that benefit from hexadecimal display
    return mode == 5;  // Surface type enum
}

void TacticalOverlay_Render() {
    if (!TacticalOverlay_Enabled) {
        return;
    }

    WindowInfo* window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    // Calculate visible grid cell range
    const int32_t grid_ulx = std::max(0, GameManager_MapView.ulx);
    const int32_t grid_uly = std::max(0, GameManager_MapView.uly);
    const int32_t grid_lrx = std::min(static_cast<int32_t>(ResourceManager_MapSize.x), GameManager_MapView.lrx + 1);
    const int32_t grid_lry = std::min(static_cast<int32_t>(ResourceManager_MapSize.y), GameManager_MapView.lry + 1);
    const bool use_hex = TacticalOverlay_ShouldDisplayAsHex(TacticalOverlay_Mode);

    // Iterate over visible grid cells
    for (int32_t grid_x = grid_ulx; grid_x < grid_lrx; ++grid_x) {
        for (int32_t grid_y = grid_uly; grid_y < grid_lry; ++grid_y) {
            const uint32_t value = TacticalOverlay_GetCellValue(grid_x, grid_y, TacticalOverlay_Mode);

            // Format value as string
            char text[32];

            if (use_hex) {
                SDL_snprintf(text, sizeof(text), "0x%X", value);

            } else {
                SDL_snprintf(text, sizeof(text), "%u", value);
            }

            TacticalOverlay_RenderGridCellValue(window, grid_x, grid_y, text);
        }
    }
}
