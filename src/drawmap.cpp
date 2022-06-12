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
static int DrawMap_BuildMMarkDelayCounter = 1;
static int DrawMap_BuildMarkImageIndex;
static ResourceID DrawMap_BuildMarkImages[] = {BLDMRK1, BLDMRK2, BLDMRK3, BLDMRK4, BLDMRK5};

static const char* const DrawMap_UnitCompletionLabels[] = {"New %s available", "New %s available", "New %s available"};

static int DrawMap_GetSideOverlap(int lr1, int ul1, int lr2, int ul2);
static void Drawmap_AddDirtyZone(Rect* bounds1, Rect* bounds2, int horizontal_side, int vertical_side);
static void DrawMap_Callback1(int ulx, int uly);
static void DrawMap_RedrawUnit(UnitInfo* unit, void (*callback)(int ulx, int uly));
static void DrawMap_Callback2(int ulx, int uly);
static void DrawMap_RenderWaitBulb(UnitInfo* unit);
static void DrawMap_RenderBarDisplay(WindowInfo* window, int ulx, int uly, int width, int height, int color);
static void DrawMap_RenderStatusDisplay(UnitInfo* unit, int ulx, int uly, int width, int height);
static void DrawMap_RenderColorsDisplay(int ulx, int uly, int width, int height, unsigned char* buffer, int color);
static void DrawMap_RenderUnitDisplays(UnitInfoGroup* group, UnitInfo* unit);
static void DrawMap_RenderColorsDisplay(UnitInfo* unit);
static void DrawMap_RenderTextBox(UnitInfo* unit, char* text, int color);
static void DrawMap_RenderNamesDisplay(UnitInfo* unit);
static void DrawMap_RenderMiniMapUnitList(SmartList<UnitInfo>* units);
static void DrawMap_RenderMiniMap();
static void DrawMap_RenderMapTile(int ulx, int uly, Rect bounds, unsigned char* buffer);

DrawMapBuffer::DrawMapBuffer() : buffer(nullptr), bounds({0, 0, 0, 0}) {}

void DrawMapBuffer::Deinit() {
    if (buffer) {
        int width;

        width = bounds.lrx - bounds.ulx;

        for (int i = 0; i < width; ++i) {
            delete[] buffer[i];
        }

        delete[] buffer;

        buffer = nullptr;
    }
}

DrawMapBuffer::~DrawMapBuffer() { Deinit(); }

void DrawMapBuffer::Init(Rect* bounds) {
    int width;
    int height;

    Deinit();

    this->bounds = *bounds;

    width = this->bounds.lrx - this->bounds.ulx;
    height = this->bounds.lry - this->bounds.uly;

    buffer = new (std::nothrow) unsigned char*[width];

    for (int i = 0; i < width; ++i) {
        buffer[i] = new (std::nothrow) unsigned char[height];
        memset(buffer[i], 0, height);
    }
}

unsigned char** DrawMapBuffer::GetBuffer() const { return buffer; }

int DrawMapBuffer::GetWidth() const { return bounds.lrx - bounds.ulx; }

int DrawMapBuffer::GetHeight() const { return bounds.lry - bounds.uly; }

Rect* DrawMapBuffer::GetBounds() { return &bounds; }

int DrawMap_GetSideOverlap(int lr1, int ul1, int lr2, int ul2) {
    int result;

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

void Drawmap_AddDirtyZone(Rect* bounds1, Rect* bounds2, int horizontal_side, int vertical_side) {
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
        }

        if (local.uly < GameManager_MapWindowDrawBounds.uly) {
            local.uly = GameManager_MapWindowDrawBounds.uly;
        }

        if (local.lrx > GameManager_MapWindowDrawBounds.lrx) {
            local.lrx = GameManager_MapWindowDrawBounds.lrx + 1;
        }

        if (local.lry > GameManager_MapWindowDrawBounds.lry) {
            local.lry = GameManager_MapWindowDrawBounds.lry + 1;
        }

        if (local.lrx > local.ulx && local.lry > local.uly) {
            for (int i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
                Rect dirty;
                int overlap_x;
                int overlap_y;

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

            for (int i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
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

void DrawMap_Callback1(int ulx, int uly) {
    Rect bounds;

    rect_init(&bounds, ulx * 64, uly * 64, bounds.ulx + 63, bounds.uly + 63);

    GameManager_AddDrawBounds(&bounds);
}

void DrawMap_RedrawUnit(UnitInfo* unit, void (*callback)(int ulx, int uly)) {
    if (unit->team == GameManager_PlayerTeam) {
        UnitInfo* parent = unit->GetParent();

        if (parent) {
            if ((unit->orders == ORDER_IDLE ||
                 (unit->orders == ORDER_BUILDING && unit->state == ORDER_STATE_UNIT_READY &&
                  (unit->flags & STATIONARY) == 0)) &&
                parent->unit_type != AIRTRANS) {
                Point point(parent->grid_x - 1, parent->grid_y + 1);
                int limit;
                unsigned int flags;

                flags = 0x2;

                if (parent->flags & BUILDING) {
                    ++point.y;
                    limit = 3;

                } else {
                    limit = 2;
                }

                for (int i = 0; i < 8; i += 2) {
                    for (int j = 0; j < limit; ++j) {
                        point += Paths_8DirPointsArray[i];

                        if (Access_IsAccessible(unit->unit_type, GameManager_PlayerTeam, point.x, point.y, flags)) {
                            callback(point.x, point.y);
                        }
                    }
                }
            }
        }
    }
}

void DrawMap_Callback2(int ulx, int uly) {
    WindowInfo* window;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    ulx = ulx * 64 + 32;
    uly = uly * 64 + 32;

    ulx = ((ulx << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx;
    uly = ((uly << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUly;

    WindowManager_DecodeImage2(DrawMap_BuildMarkImage, ulx, uly, true, window);
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
            (unit->orders == ORDER_IDLE || (unit->orders == ORDER_BUILDING && unit->state == ORDER_STATE_UNIT_READY &&
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

            DrawMap_BuildMarkImage = WindowManager_RescaleImage(image, Gfx_MapScalingFactor);

            DrawMap_RedrawUnit(unit, &DrawMap_Callback1);
        }
    }
}

void DrawMap_RenderWaitBulb(UnitInfo* unit) {
    if (Gfx_ZoomLevel >= 8) {
        WindowInfo* window;
        struct ImageSimpleHeader* image;
        int unit_size;
        int unit_size2;
        int ulx;
        int uly;

        window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

        if (unit->flags & BUILDING) {
            unit_size = 64;

        } else {
            unit_size = 32;
        }

        unit_size = (unit_size << 16) / Gfx_MapScalingFactor;

        ulx = ((unit->x << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx + 1 - unit_size;
        uly = ((unit->y << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUly + 1 - unit_size;

        if (unit->flags & BUILDING) {
            unit_size2 = 128;

        } else {
            unit_size2 = 64;
        }

        unit_size2 = (unit_size2 << 16) / Gfx_MapScalingFactor - 2;

        image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(BULB));

        if (Gfx_ZoomLevel != 64) {
            image = WindowManager_RescaleImage(image, Gfx_MapScalingFactor);
        }

        WindowManager_DecodeImage2(image, ulx + unit_size, uly + unit_size, true, window);

        if (Gfx_ZoomLevel != 64) {
            delete[] image;
        }
    }
}

void DrawMap_RenderBarDisplay(WindowInfo* window, int ulx, int uly, int width, int height, int color) {
    ulx = std::max(0, ulx);
    uly = std::max(0, uly);
    width = std::min(width, static_cast<int>(GameManager_MainMapWidth) - 1);
    height = std::min(height, static_cast<int>(GameManager_MainMapHeight) - 1);

    width = width - ulx + 1;
    height = height - uly + 1;

    if (width > 0 && height > 0) {
        buf_fill(&window->buffer[uly * 640 + ulx], width, height, 640, color);
    }
}

void DrawMap_RenderStatusDisplay(UnitInfo* unit, int ulx, int uly, int width, int height) {
    WindowInfo* window;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    if (unit->orders == ORDER_DISABLED) {
        struct ImageSimpleHeader* image;

        image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(IL_DSBLD));

        if (image->width <= width - ulx && image->height <= height - uly) {
            WindowManager_DecodeImage2(image, (width + ulx - image->width) / 2, (height + uly - image->height) / 2,
                                       true, window);
        }

    } else if (unit->speed || unit->shots) {
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
            if (unit->shots) {
                image_shots = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(IL_SHOTS));

                if (image_shots->height > height - uly) {
                    return;
                }

                width -= image_shots->width;
            }
        }

        if (width >= 0) {
            if (unit->shots && unit->speed) {
                width /= 3;

            } else {
                width /= 2;
            }

            if (unit->speed) {
                WindowManager_DecodeImage2(image_speed, width + ulx, height - image_speed->height - 1, true, window);

                width = width * 2 + image_speed->width;
            }

            if (unit->shots) {
                WindowManager_DecodeImage2(image_shots, width + ulx, height - image_shots->height - 1, true, window);
            }
        }
    }
}

void DrawMap_RenderColorsDisplay(int ulx, int uly, int width, int height, unsigned char* buffer, int color) {
    int ulx_max;
    int uly_max;
    int width_min;
    int height_min;

    ulx_max = std::max(0, ulx);
    uly_max = std::max(0, uly);

    width_min = std::min(width, GameManager_MainMapWidth - 1);
    height_min = std::min(height, GameManager_MainMapHeight - 1);

    if (uly >= 0 && uly < GameManager_MainMapHeight) {
        draw_line(buffer, 640, ulx_max, uly, width_min, uly, color);
    }

    if (width >= 0 && width < GameManager_MainMapWidth) {
        draw_line(buffer, 640, width, uly_max, width, height_min, color);
    }

    if (height >= 0 && height < GameManager_MainMapHeight) {
        draw_line(buffer, 640, ulx_max, height, width_min, height, color);
    }

    if (ulx >= 0 && ulx < GameManager_MainMapWidth) {
        draw_line(buffer, 640, ulx, ulx_max, ulx, height_min, color);
    }
}

void DrawMap_RenderUnitDisplays(UnitInfoGroup* group, UnitInfo* unit) {
    WindowInfo* window;
    int unit_size;
    int ulx;
    int uly;
    int lrx;
    int lry;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    if (unit->flags & BUILDING) {
        unit_size = 64;

    } else {
        unit_size = 32;
    }

    ulx = unit->x - unit_size;
    ulx = ((ulx << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx + 1;

    uly = unit->y - unit_size;
    uly = ((uly << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUly + 1;

    if (unit->flags & BUILDING) {
        unit_size = 128;

    } else {
        unit_size = 64;
    }

    unit_size = (unit_size << 16) / Gfx_MapScalingFactor - 2;

    lrx = ulx + unit_size;
    lry = uly + unit_size;

    if (lrx >= 0 && lry >= 0 && ulx < GameManager_MainMapWidth && uly < GameManager_MainMapHeight) {
        int color;

        if (GameManager_DisplayButtonColors && !(unit->flags & STATIONARY)) {
            if (unit->flags & HASH_TEAM_RED) {
                color = 0x01;

            } else if (unit->flags & HASH_TEAM_GREEN) {
                color = 0x02;

            } else if (unit->flags & HASH_TEAM_BLUE) {
                color = 0x03;

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
            int display_size;
            int value_max;
            int value_actual;

            display_size = unit_size / 8;

            if (display_size < 3) {
                display_size = 3;
            }

            unit_size -= 0x100000 / Gfx_MapScalingFactor;
            ulx += 0x80000 / Gfx_MapScalingFactor;
            uly += 0x60000 / Gfx_MapScalingFactor;
            lrx = ulx + unit_size - 1;
            lry = uly + display_size - 1;

            for (int i = 0; i < 2; ++i) {
                value_max = 0;

                switch (i) {
                    case 0: {
                        if (GameManager_DisplayButtonHits) {
                            value_max = unit->GetBaseValues()->GetAttribute(ATTRIB_HITS);
                        }

                        value_actual = std::min(static_cast<int>(unit->hits), value_max);
                    } break;

                    case 1: {
                        if (GameManager_DisplayButtonAmmo && unit->team == GameManager_PlayerTeam) {
                            value_max = unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO);
                        }

                        value_actual = std::min(static_cast<int>(unit->ammo), value_max);
                    } break;
                }

                if (value_max) {
                    int width;

                    DrawMap_RenderBarDisplay(window, ulx, uly, lrx, lry, 0x00);

                    width = ((unit_size - 2) * value_actual) / value_max;

                    if (width > 0) {
                        if (value_actual > (value_max / 4)) {
                            if (value_actual > (value_max / 2)) {
                                color = 0x02;

                            } else {
                                color = 0x04;
                            }

                        } else {
                            color = 0x01;
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
    int ulx;
    int uly;
    int size;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    ulx = ((unit->x - 32) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx + 1;
    uly = ((unit->y - 32) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly + 1;

    size = Gfx_ZoomLevel - 2;

    if ((ulx + size) >= 0 && (uly + size) >= 0 && ulx < GameManager_MainMapWidth && uly < GameManager_MainMapHeight) {
        DrawMap_RenderColorsDisplay(ulx, uly, ulx + size, uly + size, window->buffer, 0x04);
    }
}

void DrawMap_RenderTextBox(UnitInfo* unit, char* text, int color) {
    WindowInfo* window;
    int unit_size;
    Rect text_area;
    Rect draw_area;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    if (unit->flags & BUILDING) {
        unit_size = 128;

    } else {
        unit_size = 64;
    }

    text_area.ulx = ((unit->x - (unit_size / 2)) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx + 4;
    text_area.uly = ((unit->y - (unit_size / 2)) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly + 4;
    text_area.lrx = text_area.ulx + (unit_size << 16) / Gfx_MapScalingFactor - 8;
    text_area.lry = text_area.uly + (unit_size << 16) / Gfx_MapScalingFactor - 8;

    if (text_area.ulx < text_area.lrx && text_area.uly < text_area.lry) {
        unsigned int zoom_level;

        zoom_level = Gfx_ZoomLevel;

        if (!(unit->flags & BUILDING)) {
            zoom_level /= 2;
        }

        draw_area.ulx = 0;
        draw_area.uly = 0;
        draw_area.lrx =
            ((GameManager_MapWindowDrawBounds.lrx - GameManager_MapWindowDrawBounds.ulx) << 16) / Gfx_MapScalingFactor;
        draw_area.lry =
            ((GameManager_MapWindowDrawBounds.lry - GameManager_MapWindowDrawBounds.uly) << 16) / Gfx_MapScalingFactor;

        if (zoom_level >= 42) {
            text_font(1);

        } else if (zoom_level >= 24) {
            text_font(5);

        } else {
            text_font(2);
        }

        Text_AutofitTextBox(window->buffer, 640, text, &text_area, &draw_area, color, true);
    }
}

void DrawMap_RenderNamesDisplay(UnitInfo* unit) {
    char text[400];

    if (unit->unit_type == RESEARCH && unit->orders == ORDER_POWER_ON &&
        UnitsManager_TeamInfo[unit->team].research_topics[unit->research_topic].turns_to_complete == 0) {
        sprintf(text, "%s research completed", ResearchMenu_TopicLabels[unit->research_topic]);
        DrawMap_RenderTextBox(unit, text, 0x1001F);

    } else if (unit->state == ORDER_STATE_UNIT_READY) {
        UnitInfo* parent;

        parent = unit->GetParent();

        sprintf(text, DrawMap_UnitCompletionLabels[UnitsManager_BaseUnits[parent->unit_type].gender],
                UnitsManager_BaseUnits[parent->unit_type].singular_name);

        if (parent->flags & STATIONARY) {
            unit = parent;
        }

        DrawMap_RenderTextBox(unit, text, 0x1001F);

    } else if (GameManager_DisplayButtonNames && (unit->flags & SELECTABLE) && unit->unit_type != LRGTAPE &&
               unit->unit_type != SMLTAPE) {
        unit->GetDisplayName(text);
        DrawMap_RenderTextBox(unit, text, 0x10004);
    }
}

void DrawMap_RenderAirShadow(UnitInfoGroup* group, UnitInfo* unit) { unit->RenderAirShadow(group->GetBounds2()); }

void DrawMap_RenderUnit(UnitInfoGroup* group, UnitInfo* unit, bool mode) {
    if (mode) {
        unit->RenderWithConnectors(group->GetBounds2());

    } else {
        unit->Render(group->GetBounds2());
    }

    if ((unit->flags & SELECTABLE) && GameManager_IsAtGridPosition(unit)) {
        if (unit->team == GameManager_PlayerTeam &&
            (unit->state == ORDER_STATE_NEW_ORDER || unit->state == ORDER_STATE_29) && unit->path != nullptr) {
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
        int unit_size;

        window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

        if (GameManager_SelectedUnit->flags & BUILDING) {
            unit_size = 64;
        } else {
            unit_size = 32;
        }

        GameManager_DrawUnitSelector(
            window->buffer, 640,
            ((GameManager_SelectedUnit->x - unit_size) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx + 1,
            ((GameManager_SelectedUnit->y - unit_size) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly + 1,
            group->GetBounds2()->ulx, group->GetBounds2()->uly, group->GetBounds2()->lrx - 1,
            group->GetBounds2()->lry - 1, Gfx_MapScalingFactor, unit->flags & BUILDING, true);
    }
}

void DrawMap_RenderMiniMapUnitList(SmartList<UnitInfo>* units) {
    WindowInfo* window;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if (((*it).IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy) &&
            (Gfx_ZoomLevel > 4 || ((*it).flags & GROUND_COVER)) && (*it).orders != ORDER_IDLE &&
            ((*it).flags & SELECTABLE) &&
            (!GameManager_DisplayButtonMinimapTnt || ((*it).flags & (HAS_FIRING_SPRITE | FIRES_MISSILES)))) {
            int color;
            int grid_x;
            int grid_y;

            if ((*it).flags & HASH_TEAM_RED) {
                color = 0x01;

            } else if ((*it).flags & HASH_TEAM_GREEN) {
                color = 0x02;

            } else if ((*it).flags & HASH_TEAM_BLUE) {
                color = 0x03;

            } else if ((*it).flags & HASH_TEAM_GRAY) {
                color = 0xFF;

            } else {
                color = 0x04;
            }

            grid_x = (*it).grid_x;
            grid_y = (*it).grid_y;

            if (GameManager_DisplayButtonMinimap2x && !((*it).flags & BUILDING)) {
                window->buffer[grid_y * 640 + grid_x] = color;

            } else {
                if (grid_x > 110) {
                    grid_x = 110;
                }

                if (grid_y > 110) {
                    grid_y = 110;
                }

                window->buffer[grid_y * 640 + grid_x] = color;
                window->buffer[grid_y * 640 + grid_x + 1] = color;
                window->buffer[(grid_y + 1) * 640 + grid_x] = color;
                window->buffer[(grid_y + 1) * 640 + grid_x + 1] = color;
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

    for (int i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
        group.ProcessDirtyZone(DrawMap_DirtyRectangles[i]);
    }

    if (GameManager_RenderMinimapDisplay) {
        DrawMap_RenderMiniMap();
    }
}

void DrawMap_RenderMapTile(int ulx, int uly, Rect bounds, unsigned char* buffer) {
    int grid_x;
    int grid_y;

    grid_x = (bounds.ulx << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
    grid_y = (bounds.uly << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

    Gfx_MapWindowBuffer = &buffer[grid_y * 640 + grid_x];
    Gfx_MapBigmapIileIdBufferOffset = ResourceManager_MapSize.x * uly + ulx;

    if (ResourceManager_DisableEnhancedGraphics) {
        Gfx_DecodeMapTile(&bounds, 32, 10);

    } else {
        Gfx_DecodeMapTile(&bounds, 64, 12);
    }
}

void DrawMap_RenderMapTiles(DrawMapBuffer* drawmap, bool display_button_grid) {
    int index;

    index = DrawMap_DirtyRectangles.GetCount();

    if (index) {
        WindowInfo* window;
        Rect dirty;
        Rect bounds;
        Rect bounds2;
        int width;
        int height;
        Point point;
        unsigned char** buffer;

        window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

        --index;
        dirty = *DrawMap_DirtyRectangles[index];

        for (--index; index >= 0; --index) {
            bounds = *DrawMap_DirtyRectangles[index];

            dirty.ulx = std::min(dirty.ulx, bounds.ulx);
            dirty.uly = std::min(dirty.uly, bounds.uly);
            dirty.lrx = std::max(dirty.lrx, bounds.lrx);
            dirty.lry = std::max(dirty.lry, bounds.lry);
        }

        dirty.ulx /= 64;
        dirty.uly /= 64;
        dirty.lrx = std::min((dirty.lrx + 63) / 64, static_cast<int>(ResourceManager_MapSize.x));
        dirty.lry = std::min((dirty.lry + 63) / 64, static_cast<int>(ResourceManager_MapSize.y));

        drawmap->Init(&dirty);

        width = dirty.lrx - dirty.ulx;
        height = dirty.lry - dirty.uly;

        buffer = drawmap->GetBuffer();

        for (int i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
            bounds = *DrawMap_DirtyRectangles[i];
            bounds2 = bounds;

            bounds.ulx /= 64;
            bounds.uly /= 64;
            bounds.lrx = std::min((bounds.lrx + 63) / 64, dirty.lrx);
            bounds.lry = std::min((bounds.lry + 63) / 64, dirty.lry);

            for (point.x = bounds.ulx; point.x < bounds.lrx; ++point.x) {
                memset(&buffer[point.x - dirty.ulx][bounds.uly - dirty.uly], 1, bounds.lry - bounds.uly);
            }

            DrawMap_RenderMapTile(bounds.ulx, bounds.uly, bounds2, window->buffer);

            if (display_button_grid) {
                bounds2.ulx = (bounds2.ulx << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
                bounds2.uly = (bounds2.uly << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly;
                bounds2.lrx = ((bounds2.lrx - 1) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
                bounds2.lry = ((bounds2.lry - 1) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

                width = bounds.lrx - bounds.ulx;
                height = bounds.lry - bounds.uly;

                bounds.ulx = ((bounds.ulx * 64) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
                bounds.uly = ((bounds.uly * 64) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

                for (int j = 0; j < width; ++j) {
                    if (bounds.ulx >= bounds2.ulx && bounds.ulx <= bounds2.lrx) {
                        draw_line(window->buffer, 640, bounds.ulx, bounds2.uly, bounds.ulx, bounds2.lry, 0x25);
                    }

                    bounds.ulx += Gfx_ZoomLevel;
                }

                for (int j = 0; j < height; ++j) {
                    if (bounds.uly >= bounds2.uly && bounds.uly <= bounds2.lry) {
                        draw_line(window->buffer, 640, bounds2.ulx, bounds.uly, bounds2.lrx, bounds.uly, 0x25);
                    }

                    bounds.uly += Gfx_ZoomLevel;
                }
            }
        }
    }
}

void DrawMap_RenderSurveyDisplay(DrawMapBuffer* drawmap) {
    Point point;
    Rect bounds;
    unsigned char** buffer;
    int width;
    int height;
    int start_x;

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
    WindowInfo window = *WindowManager_GetWindow(WINDOW_MAIN_MAP);
    Rect bounds;

    if (GameManager_SelectedUnit != nullptr) {
        DrawMap_RedrawUnit(&*GameManager_SelectedUnit, &DrawMap_Callback2);
    }

    for (int i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
        bounds = *DrawMap_DirtyRectangles[i];

        bounds.ulx = (bounds.ulx << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
        bounds.uly = (bounds.uly << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly;
        bounds.lrx = ((bounds.lrx - 1) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
        bounds.lry = ((bounds.lry - 1) << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

        bounds.ulx += window.window.ulx;
        bounds.uly += window.window.uly;
        bounds.lrx += window.window.ulx;
        bounds.lry += window.window.uly;

        SDL_assert(bounds.ulx >= 0 && bounds.uly >= 0 && bounds.lrx < 640 && bounds.lry < 480);
        SDL_assert(bounds.ulx <= bounds.lrx && bounds.uly <= bounds.lry);

        win_draw_rect(window.id, &bounds);
    }

    DrawMap_DirtyRectangles.Clear();
}

bool DrawMap_IsInsideBounds(Rect* bounds) {
    Rect dirty;

    for (int i = DrawMap_DirtyRectangles.GetCount() - 1; i >= 0; --i) {
        dirty = *DrawMap_DirtyRectangles[i];

        if (bounds->lrx > dirty.ulx && bounds->ulx < dirty.lrx && bounds->lry > dirty.uly && bounds->uly < dirty.lry) {
            return 1;
        }
    }

    return false;
}

void DrawMap_ClearDirtyZones() { DrawMap_DirtyRectangles.Clear(); }
