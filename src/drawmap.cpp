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

#include "game_manager.hpp"
#include "gfx.hpp"
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

static int DrawMap_GetSideOverlap(int lr1, int ul1, int lr2, int ul2);
static void Drawmap_AddDirtyZone(Rect* bounds1, Rect* bounds2, int horizontal_side, int vertical_side);
static void DrawMap_Callback1(int ulx, int uly);
static void DrawMap_RedrawUnit(UnitInfo* unit, void (*callback)(int ulx, int uly));
static void DrawMap_Callback2(int ulx, int uly);
static void DrawMap_RenderWaitBulb(UnitInfo* unit);
static void DrawMap_RenderBarDisplay(int ulx, int uly, int width, int height, int color);
static void DrawMap_RenderStatusDisplay(UnitInfo* unit, int ulx, int uly, int width, int height);
static void DrawMap_RenderColorsDisplay(int ulx, int uly, int width, int height, unsigned char buffer, int color);
static void DrawMap_RenderUnitDisplays(UnitInfoGroup* group, UnitInfo* unit);
static void DrawMap_RenderColorsDisplay(UnitInfo* unit);
static void DrawMap_RenderTextBox(UnitInfo* unit, char* text, int color);
static void DrawMap_RenderNamesDisplay(UnitInfo* unit);
static void DrawMap_RenderMiniMapUnitList(SmartList<UnitInfo>* units);
static void DrawMap_RenderMiniMap();
static void DrawMap_RenderMapTile(int ulx, int uly);

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

                DrawMap_DirtyRectangles.Append(&local);
            }
        }
    }
}

void DrawMap_Callback1(int ulx, int uly) {
    Rect bounds;

    rect_init(&bounds, ulx * 64, uly * 64, bounds.ulx + 63, bounds.uly + 63);

    GameManager_AddDrawBounds(&bounds);
}

void DrawMap_RedrawUnit(UnitInfo* unit, void (*callback)(int ulx, int uly)) {
    /// \todo
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
    /// \todo
}

void DrawMap_RenderWaitBulb(UnitInfo* unit) {
    /// \todo
}

void DrawMap_RenderBarDisplay(int ulx, int uly, int width, int height, int color) {
    /// \todo
}

void DrawMap_RenderStatusDisplay(UnitInfo* unit, int ulx, int uly, int width, int height) {
    /// \todo
}

void DrawMap_RenderColorsDisplay(int ulx, int uly, int width, int height, unsigned char buffer, int color) {
    /// \todo
}

void DrawMap_RenderUnitDisplays(UnitInfoGroup* group, UnitInfo* unit) {
    /// \todo
}

void DrawMap_RenderColorsDisplay(UnitInfo* unit) {
    /// \todo
}

void DrawMap_RenderTextBox(UnitInfo* unit, char* text, int color) {
    /// \todo
}

void DrawMap_RenderNamesDisplay(UnitInfo* unit) {
    /// \todo
}

void DrawMap_RenderAirShadow(UnitInfoGroup* group, UnitInfo* unit) { unit->RenderAirShadow(group->GetBounds2()); }

void DrawMap_RenderUnit(UnitInfoGroup* group, UnitInfo* unit, bool mode) {
    /// \todo
}

void DrawMap_RenderMiniMapUnitList(SmartList<UnitInfo>* units) {
    /// \todo
}

void DrawMap_RenderMiniMap() {
    DrawMap_RenderMiniMapUnitList(&UnitsManager_StationaryUnits);
    DrawMap_RenderMiniMapUnitList(&UnitsManager_MobileLandSeaUnits);
    DrawMap_RenderMiniMapUnitList(&UnitsManager_MobileAirUnits);
}

void DrawMap_RenderUnits() {
    /// \todo
}

void DrawMap_RenderMapTile(int ulx, int uly) {
    /// \todo
}

void DrawMap_RenderMapTiles(DrawMapBuffer* drawmap, bool display_button_grid) {
    /// \todo
}

void DrawMap_RenderSurveyDisplay(DrawMapBuffer* drawmap) {
    /// \todo
}

void DrawMap_RedrawDirtyZones() {
    /// \todo
}

void DrawMap_ClearDirtyZones() { DrawMap_DirtyRectangles.Clear(); }
