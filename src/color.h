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

#include "memory.h"

#define PALETTE_SIZE 256

typedef unsigned char ColorIndex;
typedef long ColorRGB;
typedef unsigned char Color;
typedef long fixed;
typedef ColorIndex ColorBlendTable[PALETTE_SIZE][0x00000010];

typedef unsigned long (*ColorOpenFunc)(char* file, int mode);
typedef unsigned long (*ColorReadFunc)(unsigned long handle, void* buf, unsigned long size);
typedef unsigned long (*ColorCloseFunc)(unsigned long handle);
typedef char* (*ColorNameMangleFunc)(char*);

extern ColorIndex colorTable[32768];
extern Color intensityColorTable[256][256];

// void colorInitIO(ColorOpenFunc o, ColorReadFunc r, ColorCloseFunc c);
void colorSetNameMangler(ColorNameMangleFunc c);
Color colorMixAdd(Color a, Color b);
Color colorMixMul(Color a, Color b);
Color calculateColor(fixed intensity, Color color);
Color RGB2Color(ColorRGB c);
ColorRGB Index2RGB(ColorIndex c);
ColorRGB Color2RGB(Color c);
void fadeSystemPalette(unsigned char* src, unsigned char* dest, int steps);
void setBlackSystemPalette(void);
void setSystemPalette(unsigned char* cmap);
// unsigned char* getSystemPalette(void);
// void setSystemPaletteEntries(unsigned char* pal, unsigned int start, unsigned int end);
// void setSystemPaletteEntry(int entry, unsigned char r, unsigned char g, unsigned char b);
void getSystemPaletteEntry(int entry, unsigned char* r, unsigned char* g, unsigned char* b);
int loadColorTable(char* table);
char* colorError(void);
void setColorPalette(unsigned char* pal);
void setColorPaletteEntry(int entry, unsigned char r, unsigned char g, unsigned char b);
void getColorPaletteEntry(int entry, unsigned char* r, unsigned char* g, unsigned char* b);
ColorBlendTable* getColorBlendTable(ColorIndex c);
void freeColorBlendTable(ColorIndex c);
// void colorRegisterAlloc(MallocFunc m, ReallocFunc r, FreeFunc f);
void colorGamma(double gamma);
double colorGetGamma(void);
int colorMappedColor(ColorIndex i);
// int colorBuildColorTable(unsigned char* colormap, unsigned char* matchtable);
int colorSetColorTable(unsigned char* colormap, unsigned char* reserved);
int colorPushColorPalette(void);
int colorPopColorPalette(void);
int initColors(void);
// void colorsClose(void);
unsigned char* getColorPalette(void);

void update_system_palette(SDL_Palette* palette, int render);

#endif /* COLOR_H */
