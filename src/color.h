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

typedef unsigned char ColorIndex;
typedef long ColorRGB;
typedef unsigned char Color;
typedef long fixed;
typedef ColorIndex ColorBlendTable[16][PALETTE_SIZE];

typedef unsigned long (*ColorOpenFunc)(char* file, int mode);
typedef unsigned long (*ColorReadFunc)(unsigned long handle, void* buf, unsigned long size);
typedef unsigned long (*ColorCloseFunc)(unsigned long handle);
typedef char* (*ColorNameMangleFunc)(char* table);

extern ColorIndex colorTable[32 * 32 * 32];
extern Color intensityColorTable[256][PALETTE_SIZE];

void colorInitIO(ColorOpenFunc o, ColorReadFunc r, ColorCloseFunc c);
void colorSetNameMangler(ColorNameMangleFunc c);
Color colorMixAdd(Color a, Color b);
Color colorMixMul(Color a, Color b);
Color calculateColor(fixed intensity, Color color);
ColorIndex Color2Index(Color c);
ColorIndex RGB2Index(ColorRGB c);
Color Index2Color(ColorIndex c);
Color RGB2Color(ColorRGB c);
ColorRGB Index2RGB(ColorIndex c);
ColorRGB Color2RGB(Color c);
void fadeSystemPalette(unsigned char* src, unsigned char* dest, int steps);
void setBlackSystemPalette(void);
void setSystemPalette(unsigned char* palette);
unsigned char* getSystemPalette(void);
void setSystemPaletteEntries(unsigned char* pal, unsigned int start, unsigned int end);
void setSystemPaletteEntry(int entry, unsigned char r, unsigned char g, unsigned char b);
void getSystemPaletteEntry(int entry, unsigned char* r, unsigned char* g, unsigned char* b);
int loadColorTable(char* table);
char* colorError(void);
void setColorPalette(unsigned char* pal);
void setColorPaletteEntry(int entry, unsigned char r, unsigned char g, unsigned char b);
void getColorPaletteEntry(int entry, unsigned char* r, unsigned char* g, unsigned char* b);
ColorBlendTable* getColorBlendTable(ColorIndex c);
void freeColorBlendTable(ColorIndex c);
void colorGamma(double gamma);
double colorGetGamma(void);
int colorMappedColor(ColorIndex i);
int colorBuildColorTable(unsigned char* colormap, unsigned char* matchtable);
int colorSetColorTable(unsigned char* colormap, unsigned char* matchtable);
int colorPushColorPalette(void);
int colorPopColorPalette(void);
int initColors(void);
void colorsClose(void);
unsigned char* getColorPalette(void);

#endif /* COLOR_H */
