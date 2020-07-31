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

#include "color.h"

#include <SDL.h>
#include <stdio.h>

#include "game.h"

static unsigned long colorOpen(char* file, int mode);
static unsigned long colorRead(unsigned long handle, void* buf, unsigned long size);
static unsigned long colorClose(unsigned long handle);
static void* defaultMalloc(size_t t);
static void* defaultRealloc(void* p, size_t t);
static void defaultFree(void* p);
static void setIntensityTableColor(int cc);
static void setIntensityTables(void);
static void setMixTableColor(int i);
static void setMixTable(void);
static void buildBlendTable(ColorBlendTable* table, ColorIndex c);
static void rebuildColorBlendTables(void);
static int redloop(void);
static int greenloop(int restart);
static int blueloop(int restart);
static void maxfill(unsigned long* buffer, int side);

static void update_system_palette(SDL_Palette* palette);

static SDL_Palette* SystemPalette;

static int colorsInited;
static double currentGamma = 1.0;

unsigned long colorOpen(char* file, int mode) {}
unsigned long colorRead(unsigned long handle, void* buf, unsigned long size) {}
unsigned long colorClose(unsigned long handle) {}
// void colorInitIO(ColorOpenFunc o, ColorReadFunc r, ColorCloseFunc c) {}
void* defaultMalloc(size_t t) {}
void* defaultRealloc(void* p, size_t t) {}
void defaultFree(void* p) {}
void colorSetNameMangler(ColorNameMangleFunc c) {}
Color colorMixAdd(Color a, Color b) {}
Color colorMixMul(Color a, Color b) {}
Color calculateColor(fixed intensity, Color color) {}
Color RGB2Color(ColorRGB c) {}
ColorRGB Index2RGB(ColorIndex c) {}
ColorRGB Color2RGB(Color c) {}
void fadeSystemPalette(unsigned char* src, unsigned char* dest, int steps) {}
void setBlackSystemPalette(void) {}

void setSystemPalette(unsigned char* cmap) {
    unsigned char npal[3 * PALETTE_SIZE];

    if (NULL == SystemPalette) {
        SystemPalette = SDL_AllocPalette(PALETTE_SIZE);
        SDL_assert(SystemPalette);
    }

    for (unsigned short index = 0; index < sizeof(npal); index++) {
        npal[index] = currentGammaTable[cmap[index]];
        systemCmap[index] = cmap[index];
    }

    for (int n = 0; n < PALETTE_SIZE; n++) {
        SystemPalette->colors[n].r = npal[3 * n + 0] * 4;
        SystemPalette->colors[n].g = npal[3 * n + 1] * 4;
        SystemPalette->colors[n].b = npal[3 * n + 2] * 4;
        SystemPalette->colors[n].a = 0;
    }

    update_system_palette(SystemPalette);
}

// unsigned char* getSystemPalette(void) {}
void setSystemPaletteEntries(unsigned char* pal, unsigned int start, unsigned int end) {}

void setSystemPaletteEntry(int entry, unsigned char r, unsigned char g, unsigned char b) {
    SDL_assert(r < sizeof(currentGammaTable));
    SDL_assert(g < sizeof(currentGammaTable));
    SDL_assert(b < sizeof(currentGammaTable));

    systemCmap[entry * 3] = r;
    systemCmap[entry * 3 + 1] = g;
    systemCmap[entry * 3 + 2] = b;

    SystemPalette->colors[entry].r = currentGammaTable[r] * 4;
    SystemPalette->colors[entry].g = currentGammaTable[g] * 4;
    SystemPalette->colors[entry].b = currentGammaTable[b] * 4;

    update_system_palette(SystemPalette);
}

void getSystemPaletteEntry(int entry, unsigned char* r, unsigned char* g, unsigned char* b) {}
void setIntensityTableColor(int cc) {}
void setIntensityTables(void) {}
void setMixTableColor(int i) {}
void setMixTable(void) {}
int loadColorTable(char* table) {}
char* colorError(void) {}
void setColorPalette(unsigned char* pal) {}
void setColorPaletteEntry(int entry, unsigned char r, unsigned char g, unsigned char b) {}
void getColorPaletteEntry(int entry, unsigned char* r, unsigned char* g, unsigned char* b) {}
void buildBlendTable(ColorBlendTable* table, ColorIndex c) {}
void rebuildColorBlendTables(void) {}
ColorBlendTable* getColorBlendTable(ColorIndex c) {}
void freeColorBlendTable(ColorIndex c) {}
// void colorRegisterAlloc(MallocFunc m, ReallocFunc r, FreeFunc f) {}

void colorGamma(double gamma) {
    double a;

    currentGamma = gamma;

    for (int i = 0; i < 64; i++) {
        a = pow(i, currentGamma);

        if (a > 63.0) {
            a = 63.0;
        }

        if (a < 0.0) {
            a = 0.0;
        }

        currentGammaTable[i] = a;
    }

    return setSystemPalette(systemCmap);
}

double colorGetGamma(void) {}
int colorMappedColor(ColorIndex i) {}
// int colorBuildColorTable(unsigned char* colormap, unsigned char* matchtable) {}
int colorSetColorTable(unsigned char* colormap, unsigned char* reserved) {}
int redloop(void) {}
int greenloop(int restart) {}
int blueloop(int restart) {}
void maxfill(unsigned long* buffer, int side) {}
int colorPushColorPalette(void) {}
int colorPopColorPalette(void) {}

int initColors(void) {
    int result;

    if (colorsInited) {
        result = 1;
    } else {
        colorsInited = 1;

        colorGamma(1.0);

        if (loadColorTable("COLOR.PAL") == 1) {
            setSystemPalette(cmap);

            result = 1;
        } else {
            result = 0;
        }
    }

    return result;
}

// void colorsClose(void) {}
unsigned char* getColorPalette(void) {}

static void update_system_palette(SDL_Palette* palette) {
    SDL_Surface* screen;

    screen = svga_get_screen();

    if (screen) {
        if (0 != SDL_SetPaletteColors(screen->format->palette, palette->colors, 0, PALETTE_SIZE)) {
            fprintf(stderr, "SDL_SetPaletteColors failed: %s\n", SDL_GetError());
        }

        svga_render();
    } else {
        fprintf(stderr, "SDL_Surface is NULL\n");
    }
}
