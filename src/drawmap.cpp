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

#include "drawmap.hpp"

#include "access.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "localization.hpp"
#include "researchmenu.hpp"
#include "survey.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

enum {
    OVERLAP_2IN1,
    OVERLAP_1B2_SIDE_IS_LOWER,
    OVERLAP_2B1_SIDE_IS_HIGHER,
    OVERLAP_1IN2,
};

static ObjectArray<Rect> DrawMap_DirtyRectangles;
static struct ImageSimpleHeader* DrawMap_BuildMarkImage;
static int32_t DrawMap_BuildMMarkDelayCounter = 1;
static int32_t DrawMap_BuildMarkImageIndex;
static ResourceID DrawMap_BuildMarkImages[] = {BLDMRK1, BLDMRK2, BLDMRK3, BLDMRK4, BLDMRK5};

static const char* const DrawMap_UnitCompletionLabels[] = {_(d7d0), _(2ec0), _(7c23)};

static int32_t DrawMap_GetSideOverlap(int32_t lr1, int32_t ul1, int32_t lr2, int32_t ul2);
static void Drawmap_AddDirtyZone(Rect* bounds1, Rect* bounds2, int32_t horizontal_side, int32_t vertical_side);
static void DrawMap_Callback1(int32_t ulx, int32_t uly);
static void DrawMap_RedrawUnit(UnitInfo* unit, void (*callback)(int32_t ulx, int32_t uly));
static void DrawMap_Callback2(int32_t ulx, int32_t uly);
static void DrawMap_RenderWaitBulb(UnitInfo* unit);
static void DrawMap_RenderBarDisplay(WindowInfo* window, int32_t ulx, int32_t uly, int32_t width, int32_t height,
                                     int32_t color);
static void DrawMap_RenderStatusDisplay(UnitInfo* unit, int32_t ulx, int32_t uly, int32_t width, int32_t height);
static void DrawMap_RenderColorsDisplay(int32_t ulx, int32_t uly, int32_t width, int32_t height, uint8_t* buffer,
                                        int32_t color);
static void DrawMap_RenderUnitDisplays(UnitInfoGroup* group, UnitInfo* unit);
static void DrawMap_RenderColorsDisplay(UnitInfo* unit);
static void DrawMap_RenderTextBox(UnitInfo* unit, char* text, int32_t color);
static void DrawMap_RenderNamesDisplay(UnitInfo* unit);
static void DrawMap_RenderMiniMapUnitList(SmartList<UnitInfo>* units);
static void DrawMap_RenderMiniMap();
static void DrawMap_RenderMapTile(int32_t ulx, int32_t uly, Rect bounds, uint8_t* buffer);

DrawMapBuffer::DrawMapBuffer() : buffer(nullptr), bounds({0, 0, 0, 0}) {}

void DrawMapBuffer::Deinit() {
    if (buffer) {
        int32_t width;

        width = bounds.lrx - bounds.ulx;

        for (int32_t i = 0; i < width; ++i) {
            delete[] buffer[i];
        }

        delete[] buffer;

        buffer = nullptr;
    }
}

DrawMapBuffer::~DrawMapBuffer() { Deinit(); }

void DrawMapBuffer::Init(Rect* bounds) {
    int32_t width;
    int32_t height;

    Deinit();

    this->bounds = *bounds;

    width = this->bounds.lrx - this->bounds.ulx;
    height = this->bounds.lry - this->bounds.uly;

    buffer = new (std::nothrow) uint8_t*[width];

    for (int32_t i = 0; i < width; ++i) {
        buffer[i] = new (std::nothrow) uint8_t[height];
        memset(buffer[i], 0, height);
    }
}

uint8_t** DrawMapBuffer::GetBuffer() const { return buffer; }

int32_t DrawMapBuffer::GetWidth() const { return bounds.lrx - bounds.ulx; }

int32_t DrawMapBuffer::GetHeight() const { return bounds.lry - bounds.uly; }

Rect* DrawMapBuffer::GetBounds() { return &bounds; }

int32_t DrawMap_GetSideOverlap(int32_t lr1, int32_t ul1, int32_t lr2, int32_t ul2) {
    int32_t result;

    if (lr1 >= lr2 && ul1 <= ul2) {
        result = OVERLAP_2IN1;

    } else if (lr1 <= lr2 && ul1 >= ul2) {
        result = OVERLAP_1IN2;

    } else if (lr1 >= lr2) {
        result = OVERLAP_2B1_SIDE_IS_HIGHER;

    } else {
        result = OVERLAP_1B2_SIDE_IS_LOWER;
    }

    return result;
}

void Drawmap_AddDirtyZone(Rect* bounds1, Rect* bounds2, int32_t horizontal_side, int32_t vertical_side) {
    if (horizontal_side != OVERLAP_2IN1) {
        if (horizontal_side == OVERLAP_1B2_SIDE_IS_LOWER) {
            if (vertical_side != OVERLAP_2IN1) {
                Rect bounds;

                SDL_assert(vertical_side == OVERLAP_1B2_SIDE_IS_LOWER);

                rect_init(&bounds, bounds1->ulx, bounds1->uly, bounds1->lrx, bounds2->uly);

                bounds1->uly = bounds2->uly;
                bounds1->lrx = bounds2->ulx;

                Drawmap_UpdateDirtyZones(&bounds);

            } else {
                bounds1->lrx = bounds2->ulx;
            }

        } else {
            SDL_assert(horizontal_side == OVERLAP_2B1_SIDE_IS_HIGHER);

            if (vertical_side != OVERLAP_2IN1) {
                Rect bounds;

                SDL_assert(vertical_side == OVERLAP_1B2_SIDE_IS_LOWER);

                rect_init(&bounds, bounds1->ulx, bounds1->uly, bounds1->lrx, bounds2->uly);

                bounds1->uly = bounds2->uly;
                bounds1->ulx = bounds2->lrx;

                Drawmap_UpdateDirtyZones(&bounds);

            } else {
                bounds1->ulx = bounds2->lrx;
            }
        }

    } else {
        if (vertical_side == OVERLAP_2B1_SIDE_IS_HIGHER) {
            bounds1->uly = bounds2->lry;

        } else if (vertical_side == OVERLAP_1IN2) {
            Rect bounds;

            rect_init(&bounds, bounds1->ulx, bounds1->uly, bounds1->lrx, bounds2->uly);

            bounds1->uly = bounds2->lry;

            Drawmap_UpdateDirtyZones(&bounds);

        } else if (vertical_side == OVERLAP_1B2_SIDE_IS_LOWER) {
            bounds1->lry = bounds2->uly;
        }
    }
}

void Drawmap_UpdateDirtyZones(Rect* bounds) {
    Rect local;

    local = *bounds;

    if (Gfx_MapScalingFactor) {
        if (local.ulx < GameManager_MapWindowDrawBounds.ulx) {
            local.ulx = GameManager_MapWindowDrawBounds.ulx;
            GameManager_LastZoomLevel = 0;
        }

        if (local.uly < GameManager_MapWindowDrawBounds.uly) {
            local.uly = GameManager_MapWindowDrawBounds.uly;
            GameManager_LastZoomLevel = 0;
        }

        if (local.lrx > GameManager_MapWindowDrawBounds.lrx) {
            local.lrx = GameManager_MapWindowDrawBounds.lrx + 1;
            GameManager_LastZoomLevel = 0;
        }

        if (local.lry > GameManager_MapWindowDrawBounds.lry) {
            local.lry = GameManager_MapWindowDrawBounds.lry + 1;
            GameManager_LastZoomLevel = 0;
        }

        if (local.lrx > local.ulx && local.lry > local.uly) {
            for (int32_t i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
                Rect dirty;
                int32_t overlap_x;
                int32_t overlap_y;

                dirty = *DrawMap_DirtyRectangles[i];

                if (local.lrx > dirty.ulx && local.ulx < dirty.lrx && local.lry > dirty.uly && local.uly < dirty.lry) {
                    overlap_x = DrawMap_GetSideOverlap(local.ulx, local.lrx, dirty.ulx, dirty.lrx);
                    overlap_y = DrawMap_GetSideOverlap(local.uly, local.lry, dirty.uly, dirty.lry);

                    if (!overlap_x && !overlap_y) {
                        return;
                    }

                    if (overlap_x == OVERLAP_1IN2 && overlap_y == OVERLAP_1IN2) {
                        DrawMap_DirtyRectangles.Remove(i);

                    } else if (overlap_x == OVERLAP_1IN2 ||
                               (overlap_x && overlap_y && overlap_y != OVERLAP_1B2_SIDE_IS_LOWER)) {
                        Drawmap_AddDirtyZone(DrawMap_DirtyRectangles[i], &local, OVERLAP_1IN2 - overlap_x,
                                             OVERLAP_1IN2 - overlap_y);

                        dirty = *DrawMap_DirtyRectangles[i];

                        if (dirty.lrx <= dirty.ulx || dirty.lry <= dirty.uly) {
                            DrawMap_DirtyRectangles.Remove(i);
                        }

                    } else {
                        Drawmap_AddDirtyZone(&local, DrawMap_DirtyRectangles[i], overlap_x, overlap_y);

                        if (local.lrx <= local.ulx || local.lry <= local.uly) {
                            return;
                        }
                    }

                    if (DrawMap_DirtyRectangles.GetCount() < i) {
                        i = DrawMap_DirtyRectangles.GetCount();
                    }
                }
            }

            for (int32_t i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
                Rect dirty;

                dirty = *DrawMap_DirtyRectangles[i];

                if (local.ulx == dirty.ulx && local.lrx == dirty.lrx &&
                    (dirty.uly == local.lry || dirty.lry == local.uly)) {
                    DrawMap_DirtyRectangles[i]->uly = std::min(local.uly, dirty.uly);
                    DrawMap_DirtyRectangles[i]->lry = std::max(local.lry, dirty.lry);

                    return;
                }

                if (local.uly == dirty.uly && local.lry == dirty.lry &&
                    (dirty.ulx == local.lrx || dirty.lrx == local.ulx)) {
                    DrawMap_DirtyRectangles[i]->ulx = std::min(local.ulx, dirty.ulx);
                    DrawMap_DirtyRectangles[i]->lrx = std::max(local.lrx, dirty.lrx);

                    return;
                }
            }

            DrawMap_DirtyRectangles.Append(&local);
        }
    }
}

void DrawMap_Callback1(int32_t ulx, int32_t uly) {
    Rect bounds;

    bounds.ulx = ulx * 64;
    bounds.uly = uly * 64;
    bounds.lrx = bounds.ulx + 63;
    bounds.lry = bounds.uly + 63;

    GameManager_AddDrawBounds(&bounds);
}

void DrawMap_RedrawUnit(UnitInfo* unit, void (*callback)(int32_t ulx, int32_t uly)) {
    if (unit->team == GameManager_PlayerTeam) {
        UnitInfo* parent = unit->GetParent();

        if (parent) {
            if ((unit->GetOrder() == ORDER_IDLE ||
                 (unit->GetOrder() == ORDER_BUILD && unit->GetOrderState() == ORDER_STATE_UNIT_READY &&
                  (unit->flags & STATIONARY) == 0)) &&
                parent->GetUnitType() != AIRTRANS) {
                Point point(parent->grid_x - 1, parent->grid_y + 1);
                int32_t limit;

                if (parent->flags & BUILDING) {
                    ++point.y;
                    limit = 3;

                } else {
                    limit = 2;
                }

                for (int32_t i = 0; i < 8; i += 2) {
                    for (int32_t j = 0; j < limit; ++j) {
                        point += Paths_8DirPointsArray[i];

                        if (Access_IsAccessible(unit->GetUnitType(), GameManager_PlayerTeam, point.x, point.y,
                                                AccessModifier_SameClassBlocks)) {
                            callback(point.x, point.y);
                        }
                    }
                }
            }
        }
    }
}

void DrawMap_Callback2(int32_t ulx, int32_t uly) {
    WindowInfo* window;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    ulx = ulx * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2;
    uly = uly * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2;

    ulx = ((ulx * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx;
    uly = ((uly * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor) - Gfx_MapWindowUly;

    WindowManager_DecodeSimpleImage(DrawMap_BuildMarkImage, ulx, uly, true, window);
}

void DrawMap_RenderBuildMarker() {
    if (DrawMap_BuildMarkImage) {
        delete[] DrawMap_BuildMarkImage;
        DrawMap_BuildMarkImage = nullptr;
    }

    if (GameManager_SelectedUnit != nullptr) {
        UnitInfo* unit;

        unit = &*GameManager_SelectedUnit;

        if (unit->team == GameManager_PlayerTeam &&
            (unit->GetOrder() == ORDER_IDLE ||
             (unit->GetOrder() == ORDER_BUILD && unit->GetOrderState() == ORDER_STATE_UNIT_READY &&
              !(unit->flags & STATIONARY)))) {
            struct ImageSimpleHeader* image;

            --DrawMap_BuildMMarkDelayCounter;

            if (DrawMap_BuildMMarkDelayCounter == 0) {
                ++DrawMap_BuildMarkImageIndex;

                if (DrawMap_BuildMarkImageIndex == sizeof(DrawMap_BuildMarkImages) / sizeof(ResourceID)) {
                    DrawMap_BuildMarkImageIndex = 0;
                }

                DrawMap_BuildMMarkDelayCounter = 2;
            }

            image = reinterpret_cast<struct ImageSimpleHeader*>(
                ResourceManager_LoadResource(DrawMap_BuildMarkImages[DrawMap_BuildMarkImageIndex]));

            DrawMap_BuildMarkImage = WindowManager_RescaleSimpleImage(image, Gfx_MapScalingFactor);

            DrawMap_RedrawUnit(unit, &DrawMap_Callback1);
        }
    }
}

void DrawMap_RenderWaitBulb(UnitInfo* unit) {
    if (Gfx_ZoomLevel >= 8) {
        WindowInfo* window;
        struct ImageSimpleHeader* image;
        int32_t unit_size;
        int32_t unit_size2;
        int32_t ulx;
        int32_t uly;

        window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

        if (unit->flags & BUILDING) {
            unit_size = 64;

        } else {
            unit_size = 32;
        }

        unit_size = (unit_size * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor;

        ulx = ((unit->x * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx + 1 - unit_size;
        uly = ((unit->y * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor) - Gfx_MapWindowUly + 1 - unit_size;

        image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(BULB));

        if (Gfx_ZoomLevel != 64) {
            image = WindowManager_RescaleSimpleImage(image, Gfx_MapScalingFactor);
        }

        WindowManager_DecodeSimpleImage(image, ulx + unit_size, uly + unit_size, true, window);

        if (Gfx_ZoomLevel != 64) {
            delete[] image;
        }
    }
}

void DrawMap_RenderBarDisplay(WindowInfo* window, int32_t ulx, int32_t uly, int32_t width, int32_t height,
                              int32_t color) {
    ulx = std::max(0, ulx);
    uly = std::max(0, uly);
    width = std::min(width, static_cast<int32_t>(WindowManager_MapWidth) - 1);
    height = std::min(height, static_cast<int32_t>(WindowManager_MapHeight) - 1);

    width = width - ulx + 1;
    height = height - uly + 1;

    if (width > 0 && height > 0) {
        buf_fill(&window->buffer[uly * window->width + ulx], width, height, window->width, color);
    }
}

void DrawMap_RenderStatusDisplay(UnitInfo* unit, int32_t ulx, int32_t uly, int32_t width, int32_t height) {
    WindowInfo* window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    if (unit->GetOrder() == ORDER_DISABLE) {
        struct ImageSimpleHeader* image;

        image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(IL_DSBLD));

        if (image->width <= width - ulx && image->height <= height - uly) {
            WindowManager_DecodeSimpleImage(image, (width + ulx - image->width) / 2, (height + uly - image->height) / 2,
                                            true, window);
        }

    } else if (unit->speed || unit->shots > 0) {
        struct ImageSimpleHeader* image_speed;
        struct ImageSimpleHeader* image_shots;

        image_speed = nullptr;
        image_shots = nullptr;

        width -= ulx;

        if (unit->speed) {
            image_speed = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(IL_SPEED));

            if (image_speed->height > height - uly) {
                return;
            }

            width -= image_speed->width;
        }

        if (width >= 0) {
            if (unit->shots > 0) {
                image_shots = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(IL_SHOTS));

                if (image_shots->height > height - uly) {
                    return;
                }

                width -= image_shots->width;
            }
        }

        if (width >= 0) {
            if (unit->shots > 0 && unit->speed) {
                width /= 3;

            } else {
                width /= 2;
            }

            if (unit->speed) {
                WindowManager_DecodeSimpleImage(image_speed, width + ulx, height - image_speed->height - 1, true,
                                                window);

                width = width * 2 + image_speed->width;
            }

            if (unit->shots > 0) {
                WindowManager_DecodeSimpleImage(image_shots, width + ulx, height - image_shots->height - 1, true,
                                                window);
            }
        }
    }
}

void DrawMap_RenderColorsDisplay(int32_t ulx, int32_t uly, int32_t width, int32_t height, uint8_t* buffer,
                                 int32_t color) {
    WindowInfo* window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    int32_t ulx_max;
    int32_t uly_max;
    int32_t width_min;
    int32_t height_min;

    ulx_max = std::max(0, ulx);
    uly_max = std::max(0, uly);

    width_min = std::min(width, WindowManager_MapWidth - 1);
    height_min = std::min(height, WindowManager_MapHeight - 1);

    if (uly >= 0 && uly < WindowManager_MapHeight) {
        draw_line(buffer, window->width, ulx_max, uly, width_min, uly, color);
    }

    if (width >= 0 && width < WindowManager_MapWidth) {
        draw_line(buffer, window->width, width, uly_max, width, height_min, color);
    }

    if (height >= 0 && height < WindowManager_MapHeight) {
        draw_line(buffer, window->width, ulx_max, height, width_min, height, color);
    }

    if (ulx >= 0 && ulx < WindowManager_MapWidth) {
        draw_line(buffer, window->width, ulx, uly_max, ulx, height_min, color);
    }
}

void DrawMap_RenderUnitDisplays(UnitInfoGroup* group, UnitInfo* unit) {
    WindowInfo* window;
    int32_t unit_size;
    int32_t ulx;
    int32_t uly;
    int32_t lrx;
    int32_t lry;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    if (unit->flags & BUILDING) {
        unit_size = 64;

    } else {
        unit_size = 32;
    }

    ulx = unit->x - unit_size;
    ulx = ((ulx * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx + 1;

    uly = unit->y - unit_size;
    uly = ((uly * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor) - Gfx_MapWindowUly + 1;

    if (unit->flags & BUILDING) {
        unit_size = 128;

    } else {
        unit_size = 64;
    }

    unit_size = (unit_size * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - 2;

    lrx = ulx + unit_size;
    lry = uly + unit_size;

    if (lrx >= 0 && lry >= 0 && ulx < WindowManager_MapWidth && uly < WindowManager_MapHeight) {
        int32_t color;

        if (GameManager_DisplayButtonColors && !(unit->flags & STATIONARY)) {
            if (unit->flags & HASH_TEAM_RED) {
                color = COLOR_RED;

            } else if (unit->flags & HASH_TEAM_GREEN) {
                color = COLOR_GREEN;

            } else if (unit->flags & HASH_TEAM_BLUE) {
                color = COLOR_BLUE;

            } else if (unit->flags & HASH_TEAM_GRAY) {
                color = 0xFF;

            } else {
                color = 0xA9;
            }

            DrawMap_RenderColorsDisplay(ulx, uly, lrx, lry, window->buffer, color);
        }

        if (GameManager_DisplayButtonStatus) {
            DrawMap_RenderStatusDisplay(unit, ulx - 1, uly - 1, lrx + 2, lry + 2);
        }

        if (Gfx_ZoomLevel > 8 && (GameManager_DisplayButtonHits || GameManager_DisplayButtonAmmo)) {
            int32_t display_size;
            int32_t value_max;
            int32_t value_actual;

            display_size = unit_size / 8;

            if (display_size < 3) {
                display_size = 3;
            }

            unit_size -= 0x100000 / Gfx_MapScalingFactor;
            ulx += 0x80000 / Gfx_MapScalingFactor;
            uly += 0x60000 / Gfx_MapScalingFactor;
            lrx = ulx + unit_size - 1;
            lry = uly + display_size - 1;

            for (int32_t i = 0; i < 2; ++i) {
                value_max = 0;

                switch (i) {
                    case 0: {
                        if (GameManager_DisplayButtonHits) {
                            value_max = unit->GetBaseValues()->GetAttribute(ATTRIB_HITS);
                        }

                        value_actual = std::min(static_cast<int32_t>(unit->hits), value_max);
                    } break;

                    case 1: {
                        if (GameManager_DisplayButtonAmmo && unit->team == GameManager_PlayerTeam) {
                            value_max = unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO);
                        }

                        value_actual = std::min(static_cast<int32_t>(unit->ammo), value_max);
                    } break;
                }

                if (value_max) {
                    int32_t width;

                    DrawMap_RenderBarDisplay(window, ulx, uly, lrx, lry, COLOR_BLACK);

                    width = ((unit_size - 2) * value_actual) / value_max;

                    if (width > 0) {
                        if (value_actual > (value_max / 4)) {
                            if (value_actual > (value_max / 2)) {
                                color = COLOR_GREEN;

                            } else {
                                color = COLOR_YELLOW;
                            }

                        } else {
                            color = COLOR_RED;
                        }

                        DrawMap_RenderBarDisplay(window, ulx + 1, uly + 1, ulx + width, lry - 1, color);
                    }
                }

                uly += display_size - 1;
                lry = uly + display_size - 1;
            }
        }
    }
}

void DrawMap_RenderColorsDisplay(UnitInfo* unit) {
    WindowInfo* window;
    int32_t ulx;
    int32_t uly;
    int32_t size;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    ulx = ((unit->x - 32) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUlx + 1;
    uly = ((unit->y - 32) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUly + 1;

    size = Gfx_ZoomLevel - 2;

    if ((ulx + size) >= 0 && (uly + size) >= 0 && ulx < WindowManager_MapWidth && uly < WindowManager_MapHeight) {
        DrawMap_RenderColorsDisplay(ulx, uly, ulx + size, uly + size, window->buffer, COLOR_YELLOW);
    }
}

void DrawMap_RenderTextBox(UnitInfo* unit, char* text, int32_t color) {
    WindowInfo* window;
    int32_t unit_size;
    Rect text_area;
    Rect draw_area;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    if (unit->flags & BUILDING) {
        unit_size = 128;

    } else {
        unit_size = 64;
    }

    text_area.ulx = ((unit->x - (unit_size / 2)) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUlx + 4;
    text_area.uly = ((unit->y - (unit_size / 2)) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUly + 4;
    text_area.lrx = text_area.ulx + (unit_size * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - 8;
    text_area.lry = text_area.uly + (unit_size * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - 8;

    if (text_area.ulx < text_area.lrx && text_area.uly < text_area.lry) {
        const auto font_index = Text_GetFont();
        uint32_t zoom_level;

        zoom_level = Gfx_ZoomLevel;

        if (!(unit->flags & BUILDING)) {
            zoom_level /= 2;
        }

        draw_area.ulx = 0;
        draw_area.uly = 0;
        draw_area.lrx =
            ((GameManager_MapWindowDrawBounds.lrx - GameManager_MapWindowDrawBounds.ulx) * GFX_SCALE_DENOMINATOR) /
            Gfx_MapScalingFactor;
        draw_area.lry =
            ((GameManager_MapWindowDrawBounds.lry - GameManager_MapWindowDrawBounds.uly) * GFX_SCALE_DENOMINATOR) /
            Gfx_MapScalingFactor;

        if (zoom_level >= 42) {
            Text_SetFont(GNW_TEXT_FONT_1);

        } else if (zoom_level >= 24) {
            Text_SetFont(GNW_TEXT_FONT_5);

        } else {
            Text_SetFont(GNW_TEXT_FONT_2);
        }

        Text_AutofitTextBox(window->buffer, window->width, text, &text_area, &draw_area, color, true);

        Text_SetFont(font_index);
    }
}

void DrawMap_RenderNamesDisplay(UnitInfo* unit) {
    char text[400];

    if (unit->GetUnitType() == RESEARCH && unit->GetOrder() == ORDER_POWER_ON &&
        UnitsManager_TeamInfo[unit->team].research_topics[unit->research_topic].turns_to_complete == 0) {
        sprintf(text, _(e3a4), ResearchMenu_TopicLabels[unit->research_topic]);
        DrawMap_RenderTextBox(unit, text, GNW_TEXT_OUTLINE | 0x1F);

    } else if (unit->GetOrderState() == ORDER_STATE_UNIT_READY) {
        UnitInfo* parent = unit->GetParent();

        if (parent) {
            sprintf(text, DrawMap_UnitCompletionLabels[UnitsManager_BaseUnits[parent->GetUnitType()].gender],
                    UnitsManager_BaseUnits[parent->GetUnitType()].singular_name);

            if (parent->flags & STATIONARY) {
                unit = parent;
            }

            DrawMap_RenderTextBox(unit, text, GNW_TEXT_OUTLINE | 0x1F);
        }

    } else if (GameManager_DisplayButtonNames && (unit->flags & SELECTABLE) && unit->GetUnitType() != LRGTAPE &&
               unit->GetUnitType() != SMLTAPE) {
        unit->GetDisplayName(text, sizeof(text));
        DrawMap_RenderTextBox(unit, text, GNW_TEXT_OUTLINE | COLOR_YELLOW);
    }
}

void DrawMap_RenderAirShadow(UnitInfoGroup* group, UnitInfo* unit) { unit->RenderAirShadow(group->GetBounds2()); }

void DrawMap_RenderUnit(UnitInfoGroup* group, UnitInfo* unit, bool mode) {
    if (mode) {
        unit->RenderWithConnectors(group->GetBounds2());

    } else {
        unit->Render(group->GetBounds2());
    }

    if ((unit->flags & SELECTABLE) && GameManager_IsInsideMapView(unit)) {
        if (unit->team == GameManager_PlayerTeam &&
            (unit->GetOrderState() == ORDER_STATE_NEW_ORDER ||
             unit->GetOrderState() == ORDER_STATE_MOVE_GETTING_PATH) &&
            unit->path == nullptr) {
            DrawMap_RenderWaitBulb(unit);
        }

        if (GameManager_DisplayButtonColors || GameManager_DisplayButtonStatus || GameManager_DisplayButtonHits ||
            GameManager_DisplayButtonAmmo) {
            DrawMap_RenderUnitDisplays(group, unit);
        }

        if (GameManager_SelectedUnit != nullptr && GameManager_SelectedUnit->GetUnitList()) {
            if (unit->GetUnitList() == GameManager_SelectedUnit->GetUnitList()) {
                DrawMap_RenderColorsDisplay(unit);
            }
        }
    }

    if (unit->team == GameManager_PlayerTeam) {
        DrawMap_RenderNamesDisplay(unit);
    }

    if (GameManager_SelectedUnit == unit) {
        WindowInfo* window;
        int32_t unit_size;

        window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

        if (GameManager_SelectedUnit->flags & BUILDING) {
            unit_size = 64;
        } else {
            unit_size = 32;
        }

        GameManager_DrawUnitSelector(
            window->buffer, window->width,
            ((GameManager_SelectedUnit->x - unit_size) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor -
                Gfx_MapWindowUlx + 1,
            ((GameManager_SelectedUnit->y - unit_size) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor -
                Gfx_MapWindowUly + 1,
            group->GetBounds2()->ulx, group->GetBounds2()->uly, group->GetBounds2()->lrx - 1,
            group->GetBounds2()->lry - 1, Gfx_MapScalingFactor, unit->flags & BUILDING, true);
    }
}

void DrawMap_RenderMiniMapUnitList(SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if (((*it).IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy) &&
            (Gfx_ZoomLevel > 4 || !((*it).flags & GROUND_COVER)) && (*it).GetOrder() != ORDER_IDLE &&
            ((*it).flags & SELECTABLE) &&
            (!GameManager_DisplayButtonMinimapTnt || ((*it).flags & (HAS_FIRING_SPRITE | FIRES_MISSILES)))) {
            int32_t color;

            if ((*it).flags & HASH_TEAM_RED) {
                color = COLOR_RED;

            } else if ((*it).flags & HASH_TEAM_GREEN) {
                color = COLOR_GREEN;

            } else if ((*it).flags & HASH_TEAM_BLUE) {
                color = COLOR_BLUE;

            } else if ((*it).flags & HASH_TEAM_GRAY) {
                color = 0xFF;

            } else {
                color = COLOR_YELLOW;
            }

            const int32_t grid_x{(*it).grid_x};
            const int32_t grid_y{(*it).grid_y};

            if ((*it).flags & BUILDING) {
                SDL_assert(grid_x >= 0 && grid_x + 1 < ResourceManager_MapSize.x);
                SDL_assert(grid_y >= 0 && grid_y + 1 < ResourceManager_MapSize.y);

                ResourceManager_MinimapUnits[grid_y * ResourceManager_MapSize.x + grid_x] = color;
                ResourceManager_MinimapUnits[grid_y * ResourceManager_MapSize.x + grid_x + 1] = color;
                ResourceManager_MinimapUnits[(grid_y + 1) * ResourceManager_MapSize.x + grid_x] = color;
                ResourceManager_MinimapUnits[(grid_y + 1) * ResourceManager_MapSize.x + grid_x + 1] = color;

            } else {
                SDL_assert(grid_x >= 0 && grid_x < ResourceManager_MapSize.x);
                SDL_assert(grid_y >= 0 && grid_y < ResourceManager_MapSize.y);

                ResourceManager_MinimapUnits[grid_y * ResourceManager_MapSize.x + grid_x] = color;
            }
        }
    }
}

void DrawMap_RenderMiniMap() {
    DrawMap_RenderMiniMapUnitList(&UnitsManager_StationaryUnits);
    DrawMap_RenderMiniMapUnitList(&UnitsManager_MobileLandSeaUnits);
    DrawMap_RenderMiniMapUnitList(&UnitsManager_MobileAirUnits);
}

void DrawMap_RenderUnits() {
    UnitInfoGroup group;
    WindowInfo* window;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    Gfx_MapWindowBuffer = window->buffer;

    for (int32_t i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
        group.ProcessDirtyZone(DrawMap_DirtyRectangles[i]);
    }

    if (GameManager_RenderMinimapDisplay) {
        DrawMap_RenderMiniMap();
    }
}

void DrawMap_RenderMapTile(int32_t ulx, int32_t uly, Rect bounds, uint8_t* buffer) {
    const int32_t position_x = (bounds.ulx * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
    const int32_t position_y = (bounds.uly * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

    Gfx_MapWindowBuffer =
        &buffer[position_y * WindowManager_GetWidth(WindowManager_GetWindow(WINDOW_MAIN_WINDOW)) + position_x];

    if (ResourceManager_DisableEnhancedGraphics) {
        Gfx_DecodeMapTile(&bounds, GFX_MAP_TILE_SIZE / 2, ResourceManager_MapSize.x * uly + ulx, 10);

    } else {
        Gfx_DecodeMapTile(&bounds, GFX_MAP_TILE_SIZE, ResourceManager_MapSize.x * uly + ulx, 12);
    }
}

void DrawMap_RenderMapTiles(DrawMapBuffer* drawmap, bool display_button_grid) {
    int32_t index;

    index = DrawMap_DirtyRectangles.GetCount();

    if (index > 0) {
        WindowInfo* window{WindowManager_GetWindow(WINDOW_MAIN_MAP)};
        Rect dirty;
        Point point;
        uint8_t** buffer;

        --index;
        dirty = *DrawMap_DirtyRectangles[index];

        for (--index; index >= 0; --index) {
            Rect pixel_bounds = *DrawMap_DirtyRectangles[index];

            dirty.ulx = std::min(dirty.ulx, pixel_bounds.ulx);
            dirty.uly = std::min(dirty.uly, pixel_bounds.uly);
            dirty.lrx = std::max(dirty.lrx, pixel_bounds.lrx);
            dirty.lry = std::max(dirty.lry, pixel_bounds.lry);
        }

        dirty.ulx /= GFX_MAP_TILE_SIZE;
        dirty.uly /= GFX_MAP_TILE_SIZE;
        dirty.lrx =
            std::min<int32_t>((dirty.lrx + GFX_MAP_TILE_SIZE - 1) / GFX_MAP_TILE_SIZE, ResourceManager_MapSize.x);
        dirty.lry =
            std::min<int32_t>((dirty.lry + GFX_MAP_TILE_SIZE - 1) / GFX_MAP_TILE_SIZE, ResourceManager_MapSize.y);

        drawmap->Init(&dirty);

        buffer = drawmap->GetBuffer();

        for (int32_t i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
            Rect pixel_bounds = *DrawMap_DirtyRectangles[i];
            Rect grid_bounds;

            grid_bounds.ulx = pixel_bounds.ulx / GFX_MAP_TILE_SIZE;
            grid_bounds.uly = pixel_bounds.uly / GFX_MAP_TILE_SIZE;
            grid_bounds.lrx = (pixel_bounds.lrx + GFX_MAP_TILE_SIZE - 1) / GFX_MAP_TILE_SIZE;
            grid_bounds.lry = (pixel_bounds.lry + GFX_MAP_TILE_SIZE - 1) / GFX_MAP_TILE_SIZE;

            grid_bounds.lrx = std::min(grid_bounds.lrx, dirty.lrx);
            grid_bounds.lry = std::min(grid_bounds.lry, dirty.lry);

            for (point.x = grid_bounds.ulx; point.x < grid_bounds.lrx; ++point.x) {
                memset(&buffer[point.x - dirty.ulx][grid_bounds.uly - dirty.uly], 1, grid_bounds.lry - grid_bounds.uly);
            }

            DrawMap_RenderMapTile(grid_bounds.ulx, grid_bounds.uly, pixel_bounds, window->buffer);

            if (display_button_grid) {
                pixel_bounds.ulx = (pixel_bounds.ulx * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
                pixel_bounds.uly = (pixel_bounds.uly * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUly;
                pixel_bounds.lrx =
                    ((pixel_bounds.lrx - 1) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
                pixel_bounds.lry =
                    ((pixel_bounds.lry - 1) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

                int32_t width = grid_bounds.lrx - grid_bounds.ulx;
                int32_t height = grid_bounds.lry - grid_bounds.uly;

                int32_t position_x =
                    ((grid_bounds.ulx * GFX_MAP_TILE_SIZE) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor -
                    Gfx_MapWindowUlx;
                int32_t position_y =
                    ((grid_bounds.uly * GFX_MAP_TILE_SIZE) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor -
                    Gfx_MapWindowUly;

                for (int32_t j = 0; j < width; ++j) {
                    if (position_x >= pixel_bounds.ulx && position_x <= pixel_bounds.lrx) {
                        draw_line(window->buffer, window->width, position_x, pixel_bounds.uly, position_x,
                                  pixel_bounds.lry, 0x25);
                    }

                    position_x += Gfx_ZoomLevel;
                }

                for (int32_t j = 0; j < height; ++j) {
                    if (position_y >= pixel_bounds.uly && position_y <= pixel_bounds.lry) {
                        draw_line(window->buffer, window->width, pixel_bounds.ulx, position_y, pixel_bounds.lrx,
                                  position_y, 0x25);
                    }

                    position_y += Gfx_ZoomLevel;
                }
            }
        }
    }
}

void DrawMap_RenderSurveyDisplay(DrawMapBuffer* drawmap) {
    Point point;
    Rect bounds;
    uint8_t** buffer;
    int32_t width;
    int32_t height;
    int32_t start_x;

    bounds = *drawmap->GetBounds();
    buffer = drawmap->GetBuffer();
    width = drawmap->GetWidth();
    height = drawmap->GetHeight();

    for (point.y = 0; point.y < height; ++point.y) {
        for (point.x = 0; point.x < width; ++point.x) {
            while (point.x < width && buffer[point.x][point.y] == 0) {
                ++point.x;
            }

            if (point.x < width) {
                start_x = point.x;

                while (point.x < width && buffer[point.x][point.y] == 1) {
                    ++point.x;
                }

                Survey_RenderMarkers(GameManager_PlayerTeam, bounds.ulx + start_x, bounds.uly + point.y,
                                     bounds.ulx + point.x);
            }
        }
    }
}

void DrawMap_RedrawDirtyZones() {
    WindowInfo* window = WindowManager_GetWindow(WINDOW_MAIN_MAP);
    Rect bounds;

    if (GameManager_SelectedUnit != nullptr) {
        DrawMap_RedrawUnit(&*GameManager_SelectedUnit, &DrawMap_Callback2);
    }

    for (int32_t i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
        bounds = *DrawMap_DirtyRectangles[i];

        bounds.ulx = (bounds.ulx * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
        bounds.uly = (bounds.uly * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUly;
        bounds.lrx = ((bounds.lrx - 1) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
        bounds.lry = ((bounds.lry - 1) * GFX_SCALE_DENOMINATOR) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

        bounds.ulx += window->window.ulx;
        bounds.uly += window->window.uly;
        bounds.lrx += window->window.ulx;
        bounds.lry += window->window.uly;

        SDL_assert(bounds.ulx >= 0 && bounds.uly >= 0 &&
                   bounds.lrx < WindowManager_GetWidth(WindowManager_GetWindow(WINDOW_MAIN_WINDOW)) &&
                   bounds.lry < WindowManager_GetHeight(WindowManager_GetWindow(WINDOW_MAIN_WINDOW)));
        SDL_assert(bounds.ulx <= bounds.lrx && bounds.uly <= bounds.lry);

        win_draw_rect(window->id, &bounds);
    }

    DrawMap_DirtyRectangles.Clear();

    const int32_t map_width = ResourceManager_MapSize.x * Gfx_ZoomLevel;
    const int32_t map_height = ResourceManager_MapSize.y * Gfx_ZoomLevel;

    if (GameManager_LastZoomLevel != Gfx_ZoomLevel) {
        if (WindowManager_MapWidth > map_width) {
            bounds.ulx = map_width;
            bounds.uly = 0;
            bounds.lrx = WindowManager_MapWidth - 1;
            bounds.lry = WindowManager_MapHeight - 1;

            buf_to_buf(&ResourceManager_MainmapBgImage[map_width], WindowManager_MapWidth - map_width,
                       WindowManager_MapHeight, WindowManager_MapWidth, &window->buffer[bounds.ulx], window->width);

            bounds.ulx += window->window.ulx;
            bounds.uly += window->window.uly;
            bounds.lrx += window->window.ulx;
            bounds.lry += window->window.uly;

            win_draw_rect(window->id, &bounds);
        }

        if (WindowManager_MapHeight > map_height) {
            bounds.ulx = 0;
            bounds.uly = map_height;
            bounds.lrx = WindowManager_MapWidth - 1;
            bounds.lry = WindowManager_MapHeight - 1;

            buf_to_buf(&ResourceManager_MainmapBgImage[map_height * WindowManager_MapWidth], WindowManager_MapWidth,
                       WindowManager_MapHeight - map_height, WindowManager_MapWidth,
                       &window->buffer[bounds.uly * window->width], window->width);

            bounds.ulx += window->window.ulx;
            bounds.uly += window->window.uly;
            bounds.lrx += window->window.ulx;
            bounds.lry += window->window.uly;

            win_draw_rect(window->id, &bounds);
        }

        GameManager_LastZoomLevel = Gfx_ZoomLevel;
    }
}

bool DrawMap_IsInsideBounds(Rect* bounds) {
    Rect dirty;

    for (int32_t i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
        dirty = *DrawMap_DirtyRectangles[i];

        if (bounds->lrx > dirty.ulx && bounds->ulx < dirty.lrx && bounds->lry > dirty.uly && bounds->uly < dirty.lry) {
            return 1;
        }
    }

    return false;
}

void DrawMap_ClearDirtyZones() { DrawMap_DirtyRectangles.Clear(); }
