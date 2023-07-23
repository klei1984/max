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

#ifndef DRAWMAP_HPP
#define DRAWMAP_HPP

#include "gnw.h"
#include "unitinfogroup.hpp"

class DrawMapBuffer {
    uint8_t** buffer;
    Rect bounds;

    void Deinit();

public:
    DrawMapBuffer();
    ~DrawMapBuffer();

    void Init(Rect* bounds);

    uint8_t** GetBuffer() const;
    int32_t GetWidth() const;
    int32_t GetHeight() const;
    Rect* GetBounds();
};

void Drawmap_UpdateDirtyZones(Rect* bounds);
void DrawMap_RenderBuildMarker();
void DrawMap_RenderAirShadow(UnitInfoGroup* group, UnitInfo* unit);
void DrawMap_RenderUnit(UnitInfoGroup* group, UnitInfo* unit, bool mode = true);
void DrawMap_RenderUnits();
void DrawMap_RenderMapTiles(DrawMapBuffer* drawmap, bool display_button_grid);
void DrawMap_RenderSurveyDisplay(DrawMapBuffer* drawmap);
void DrawMap_RedrawDirtyZones();
bool DrawMap_IsInsideBounds(Rect* bounds);
void DrawMap_ClearDirtyZones();

#endif /* DRAWMAP_HPP */
