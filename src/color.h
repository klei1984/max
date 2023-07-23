/* Copyright (c) 2020 M.A.X. Port Team
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

#ifndef COLOR_H
#define COLOR_H

#include <SDL.h>
#include <stddef.h>
#include <stdint.h>

#define PALETTE_SIZE 256

#define COLOR_BLACK 0x00
#define COLOR_RED 0x01
#define COLOR_GREEN 0x02
#define COLOR_BLUE 0x03
#define COLOR_YELLOW 0x04
#define COLOR_CHROME_YELLOW 0x05
#define COLOR_ROMAN_SILVER 0x06
#define COLOR_RED_ORANGE 0x07
#define COLOR_PASTEL_YELLOW 0x08

typedef uint8_t ColorIndex;
typedef int32_t ColorRGB;
typedef uint8_t Color;
typedef int32_t fixed;
typedef ColorIndex ColorBlendTable[16][PALETTE_SIZE];

extern ColorIndex colorTable[32 * 32 * 32];
extern Color intensityColorTable[256][PALETTE_SIZE];

Color colorMixAdd(Color a, Color b);
Color colorMixMul(Color a, Color b);
Color calculateColor(fixed intensity, Color color);
ColorIndex Color2Index(Color c);
ColorIndex RGB2Index(ColorRGB c);
Color Index2Color(ColorIndex c);
Color RGB2Color(ColorRGB c);
ColorRGB Index2RGB(ColorIndex c);
ColorRGB Color2RGB(Color c);
void fadeSystemPalette(uint8_t* src, uint8_t* dest, int32_t steps);
void setBlackSystemPalette(void);
void setSystemPalette(uint8_t* palette);
uint8_t* getSystemPalette(void);
void setSystemPaletteEntries(uint8_t* pal, uint32_t start, uint32_t end);
void setSystemPaletteEntry(int32_t entry, uint8_t r, uint8_t g, uint8_t b);
void getSystemPaletteEntry(int32_t entry, uint8_t* r, uint8_t* g, uint8_t* b);
int32_t loadColorTable(const char* table);
void setColorPalette(uint8_t* pal);
void setColorPaletteEntry(int32_t entry, uint8_t r, uint8_t g, uint8_t b);
void getColorPaletteEntry(int32_t entry, uint8_t* r, uint8_t* g, uint8_t* b);
ColorBlendTable* getColorBlendTable(ColorIndex c);
void freeColorBlendTable(ColorIndex c);
void colorGamma(double gamma);
double colorGetGamma(void);
int32_t colorMappedColor(ColorIndex i);
int32_t colorBuildColorTable(uint8_t* colormap, uint8_t* matchtable);
int32_t colorSetColorTable(uint8_t* colormap, uint8_t* matchtable);
int32_t colorPushColorPalette(void);
int32_t colorPopColorPalette(void);
int32_t initColors(void);
void colorsClose(void);
uint8_t* getColorPalette(void);

#endif /* COLOR_H */
