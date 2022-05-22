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

enum {
    OVERLAP_2IN1,
    OVERLAP_1B2_SIDE_IS_LOWER,
    OVERLAP_2B1_SIDE_IS_HIGHER,
    OVERLAP_1IN2,
};

ObjectArray<Rect> DrawMap_DirtyRectangles;

static int DrawMap_GetSideOverlap(int lr1, int ul1, int lr2, int ul2);
static void Drawmap_AddDirtyZone(Rect* bounds1, Rect* bounds2, int horizontal_side, int vertical_side);

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

void DrawMap_ClearDirtyZones() { DrawMap_DirtyRectangles.Clear(); }
