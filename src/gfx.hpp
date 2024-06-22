/* Copyright (c) 2022 M.A.X. Port Team
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

#ifndef GFX_HPP
#define GFX_HPP

#include "gnw.h"
#include "point.hpp"

#define GFX_MAP_TILE_SIZE (64)

#define GFX_MAP_SIZE (112)

#define GFX_SCALE_BASE (16)
#define GFX_SCALE_DENOMINATOR (1 << GFX_SCALE_BASE)
#define GFX_SCALE_NUMERATOR (GFX_MAP_TILE_SIZE * GFX_SCALE_DENOMINATOR)

bool Gfx_DecodeSpriteSetup(Point point, uint8_t* buffer, int32_t divisor, Rect* bounds);
void Gfx_DecodeMapTile(const Rect* const pixel_bounds, const uint32_t tile_size, const uint32_t tile_id,
                       const uint8_t quotient);
void Gfx_DecodeSprite();
void Gfx_DecodeShadow();
void Gfx_RenderCircle(uint8_t* buffer, int32_t full_width, int32_t width, int32_t height, int32_t xc, int32_t yc,
                      int32_t radius, int32_t color);
uint8_t* Gfx_RescaleSprite(uint8_t* buffer, uint32_t* data_size, int32_t mode, int32_t scaling_factor);

extern uint8_t* Gfx_ResourceBuffer;
extern uint32_t* Gfx_SpriteRowAddresses;
extern uint8_t Gfx_TeamColorIndexBase;
extern ColorIndex* Gfx_ColorIndices;
extern uint8_t Gfx_UnitBrightnessBase;
extern uint32_t Gfx_MapBrightness;
extern uint32_t Gfx_MapBigmapTileIdBufferOffset;
extern uint8_t* Gfx_MapWindowBuffer;
extern uint32_t Gfx_ZoomLevel;
extern int32_t Gfx_MapScalingFactor;
extern int32_t Gfx_MapWindowUlx;
extern int32_t Gfx_MapWindowUly;

#endif /* GFX_HPP */
