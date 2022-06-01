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

bool Gfx_DecodeSpriteSetup(Point point, unsigned char* buffer, int divisor, Rect* bounds);
void Gfx_DecodeMapTile(Rect* bounds, unsigned int tile_size, unsigned char quotient);
void Gfx_DecodeSprite();
unsigned char* Gfx_RescaleSprite(unsigned char* buffer, unsigned int* data_size, int mode, int scaling_factor);

extern unsigned char* Gfx_ResourceBuffer;
extern unsigned int* Gfx_SpriteRowAddresses;
extern unsigned char Gfx_TeamColorIndexBase;
extern ColorIndex* Gfx_ColorIndices;
extern unsigned short Gfx_UnitBrightnessBase;
extern unsigned int Gfx_MapBrightness;
extern unsigned int Gfx_MapBigmapIileIdBufferOffset;
extern unsigned char* Gfx_MapWindowBuffer;
extern unsigned int Gfx_ZoomLevel;
extern int Gfx_MapScalingFactor;
extern int Gfx_MapWindowUlx;
extern int Gfx_MapWindowUly;
extern unsigned short Gfx_MapWindowWidth;

#endif /* GFX_HPP */
