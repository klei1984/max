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

#include "gnw.h"

static uint32_t colorOpen(char* file, int32_t mode);
static uint32_t colorRead(uint32_t handle, void* buf, uint32_t size);
static uint32_t colorClose(uint32_t handle);
static void setIntensityTableColor(int32_t cc);
static void setIntensityTables(void);
static void setMixTableColor(int32_t i);
static void setMixTable(void);
static void buildBlendTable(ColorBlendTable* table, ColorIndex c);
static void rebuildColorBlendTables(void);
static int32_t redloop(void);
static int32_t greenloop(int32_t restart);
static int32_t blueloop(int32_t restart);
static void maxfill(uint32_t* buffer, int32_t side);

uint8_t systemCmap[3 * PALETTE_SIZE];
uint8_t currentGammaTable[64];
ColorBlendTable* blendTable[PALETTE_SIZE];
uint8_t mappedColor[PALETTE_SIZE];
Color colorMixAddTable[256][PALETTE_SIZE];
Color intensityColorTable[256][PALETTE_SIZE];
Color colorMixMulTable[256][PALETTE_SIZE];
ColorIndex colorTable[32 * 32 * 32];

static int32_t crinc;
static int32_t cbinc;
static int32_t cginc;
static uint32_t* cdp;
static int32_t cindex;
static uint8_t* crgbp;
static int32_t gcenter;
static int32_t x;
static int32_t bcenter;
static int32_t colormax;
static int32_t xsqr;
static uint32_t cdist;
static int32_t rcenter;
static uint32_t rdist;
static uint32_t gdist;
static int32_t gstride;
static int32_t rstride;

static ColorOpenFunc openFunc;
static ColorReadFunc readFunc;
static ColorCloseFunc closeFunc;

static char* errorStr = "color.c: No errors\n";

static int32_t colorsInited;
static double currentGamma = 1.0;

static ColorNameMangleFunc colorNameMangler;

static uint8_t cmap[768];

uint32_t colorOpen(char* file, int32_t mode) {
    uint32_t result;

    if (openFunc) {
        result = openFunc(file, mode);
    } else {
        result = -1;
    }

    return result;
}

uint32_t colorRead(uint32_t handle, void* buf, uint32_t size) {
    uint32_t result;

    if (readFunc) {
        result = readFunc(handle, buf, size);
    } else {
        result = -1;
    }

    return result;
}

uint32_t colorClose(uint32_t handle) {
    uint32_t result;

    if (closeFunc) {
        result = closeFunc(handle);
    } else {
        result = -1;
    }

    return result;
}

void colorInitIO(ColorOpenFunc o, ColorReadFunc r, ColorCloseFunc c) {
    openFunc = o;
    readFunc = r;
    closeFunc = c;
}

void colorSetNameMangler(ColorNameMangleFunc c) { colorNameMangler = c; }

Color colorMixAdd(Color a, Color b) { return colorMixAddTable[a][b]; }

Color colorMixMul(Color a, Color b) { return colorMixMulTable[a][b]; }

Color calculateColor(fixed intensity, Color color) { return intensityColorTable[color][intensity >> 9]; }

ColorIndex Color2Index(Color c) { return c; }

ColorIndex RGB2Index(ColorRGB c) { return Color2Index(RGB2Color(c)); }

Color Index2Color(ColorIndex c) { return c; }

Color RGB2Color(ColorRGB c) { return colorTable[c]; }

ColorRGB Index2RGB(ColorIndex c) { return Color2RGB(Index2Color(c)); }

ColorRGB Color2RGB(Color c) {
    ColorRGB result;

    result = (((int32_t)cmap[3 * c + 2]) >> 1);
    result |= ((int32_t)cmap[3 * c + 1] >> 1) << 5;
    result |= ((int32_t)cmap[3 * c] >> 1) << 10;

    return ((int32_t)cmap[3 * c + 2] >> 1) |
           32 * (((int32_t)cmap[3 * c + 1] >> 1) | 32 * ((int32_t)cmap[3 * c] >> 1));
}

void fadeSystemPalette(uint8_t* src, uint8_t* dest, int32_t steps) {
    uint8_t temp[3 * PALETTE_SIZE];
    int32_t i;
    int32_t j;
    int32_t d;

    for (i = 0; i < steps; i++) {
        for (j = 0; j < sizeof(temp); j++) {
            d = src[j] - dest[j];
            temp[j] = src[j] - (uint32_t)(i * d / steps);
        }

        setSystemPalette(temp);
    }

    setSystemPalette(dest);
}

void setBlackSystemPalette(void) {
    uint8_t tmp[3 * PALETTE_SIZE];

    memset(tmp, 0, sizeof(tmp));

    setSystemPalette(tmp);
}

void setSystemPalette(uint8_t* palette) {
    uint8_t npal[3 * PALETTE_SIZE];
    int32_t i;

    for (i = 0; i < sizeof(npal); i++) {
        npal[i] = currentGammaTable[palette[i]];
        systemCmap[i] = palette[i];
    }

    for (i = 0; i < PALETTE_SIZE; i++) {
        Svga_SetPaletteColor(i, npal[3 * i + 0] * 4, npal[3 * i + 1] * 4, npal[3 * i + 2] * 4);
    }
}

uint8_t* getSystemPalette(void) { return systemCmap; }

void setSystemPaletteEntries(uint8_t* pal, uint32_t start, uint32_t end) {
    uint8_t npal[3 * PALETTE_SIZE];
    uint32_t baseIndex;
    int32_t endCount;
    int32_t i;

    endCount = 3 * (end - start + 1);
    baseIndex = 3 * start;

    for (i = 0; i < endCount; i += 3) {
        npal[i] = currentGammaTable[pal[i]];
        npal[i + 1] = currentGammaTable[pal[i + 1]];
        npal[i + 2] = currentGammaTable[pal[i + 2]];

        systemCmap[baseIndex + i] = pal[i];
        systemCmap[baseIndex + i + 1] = pal[i + 1];
        systemCmap[baseIndex + i + 2] = pal[i + 2];
    }

    endCount = end - start + 1;

    for (i = start; i < endCount; i++) {
        Svga_SetPaletteColor(i, npal[3 * i + 0] * 4, npal[3 * i + 1] * 4, npal[3 * i + 2] * 4);
    }
}

void setSystemPaletteEntry(int32_t entry, uint8_t r, uint8_t g, uint8_t b) {
    SDL_assert(r < sizeof(currentGammaTable));
    SDL_assert(g < sizeof(currentGammaTable));
    SDL_assert(b < sizeof(currentGammaTable));

    systemCmap[entry * 3] = r;
    systemCmap[entry * 3 + 1] = g;
    systemCmap[entry * 3 + 2] = b;

    Svga_SetPaletteColor(entry, currentGammaTable[r] * 4, currentGammaTable[g] * 4, currentGammaTable[b] * 4);
}

void getSystemPaletteEntry(int32_t entry, uint8_t* r, uint8_t* g, uint8_t* b) {
    r[0] = systemCmap[entry * 3];
    g[0] = systemCmap[entry * 3 + 1];
    b[0] = systemCmap[entry * 3 + 2];
}

void setIntensityTableColor(int32_t cc) {
    Color color;
    ColorRGB r;
    ColorRGB g;
    ColorRGB b;
    int32_t nr;
    int32_t ng;
    int32_t nb;
    int32_t i;
    int32_t iShifted;
    ColorIndex c;

    c = cc;

    for (i = 0; i < 128; i++) {
        color = Index2Color(c);

        r = (Color2RGB(color) & 0x7C00) >> 10;
        g = (Color2RGB(color) & 0x03E0) >> 5;
        b = (Color2RGB(color) & 0x001F);

        iShifted = i << 9;

        nr = ((iShifted * r) >> 16) << 10;
        ng = ((iShifted * g) >> 16) << 5;
        nb = ((iShifted * b) >> 16);

        intensityColorTable[c][i] = Index2Color(colorTable[nr | ng | nb]);

        nr = ((iShifted * (31 - r) >> 16) + r) << 10;
        ng = ((iShifted * (31 - g) >> 16) + g) << 5;
        nb = ((iShifted * (31 - b) >> 16) + b);

        intensityColorTable[c][i + 128] = Index2Color(colorTable[nr | ng | nb]);
    }
}

void setIntensityTables(void) {
    int32_t i;

    for (i = 0; i < PALETTE_SIZE; i++) {
        if (mappedColor[i]) {
            setIntensityTableColor(i);
        } else {
            memset(intensityColorTable[i], 0, PALETTE_SIZE);
        }
    }
}

void setMixTableColor(int32_t i) {
    ColorRGB r;
    ColorRGB g;
    ColorRGB b;

    ColorRGB nr;
    ColorRGB ng;
    ColorRGB nb;

    ColorRGB max;

    for (int32_t j = 0; j < PALETTE_SIZE; j++) {
        if (mappedColor[i]) {
            if (mappedColor[j]) {
                r = ((Color2RGB(i) & 0x7C00) >> 10) + ((Color2RGB(j) & 0x7C00) >> 10);
                g = ((Color2RGB(i) & 0x03E0) >> 5) + ((Color2RGB(j) & 0x03E0) >> 5);
                b = (Color2RGB(i) & 0x001F) + (Color2RGB(j) & 0x001F);

                max = r;

                if (g > r) {
                    max = g;
                }

                if (b > max) {
                    max = b;
                }

                if (max <= 31) {
                    colorMixAddTable[i][j] = Index2Color(colorTable[(r << 10) | (g << 5) | b]);
                } else {
                    nr = r - (max - 31);
                    ng = g - (max - 31);
                    nb = b - (max - 31);

                    if (nr < 0) {
                        nr = 0;
                    }

                    if (ng < 0) {
                        ng = 0;
                    }

                    if (nb < 0) {
                        nb = 0;
                    }

                    colorMixAddTable[i][j] = calculateColor((((double)max - 31.0) * 0.0078125 + 1.0) * 65536.0,
                                                            Index2Color(colorTable[(nr << 10) | (ng << 5) | nb]));
                }

                r = (((Color2RGB(i) & 0x7C00) >> 10) * ((Color2RGB(j) & 0x7C00) >> 10)) >> 5;
                g = ((Color2RGB(i) & 0x03E0) >> 5) * ((Color2RGB(j) & 0x03E0) >> 5);
                b = (Color2RGB(i) & 0x001F) * (Color2RGB(j) & 0x001F);

                colorMixMulTable[i][j] = Index2Color(colorTable[(r << 10) | (g << 5) | b]);
            } else {
                colorMixAddTable[i][j] = i;
                colorMixMulTable[i][j] = i;
            }
        } else if (mappedColor[j]) {
            colorMixAddTable[i][j] = j;
            colorMixMulTable[i][j] = j;
        } else {
            colorMixAddTable[i][j] = i;
            colorMixMulTable[i][j] = i;
        }
    }
}

void setMixTable(void) {
    for (int32_t i = 0; i < PALETTE_SIZE; i++) {
        setMixTableColor(i);
    }
}

int32_t loadColorTable(char* table) {
    Color r;
    Color b;
    Color g;
    int32_t result;
    uint32_t in;
    uint32_t tag;
    int32_t i;

    if (colorNameMangler) {
        table = colorNameMangler(table);
    }

    if (openFunc) {
        in = openFunc(table, 0x200);
    } else {
        in = -1;
    }

    if (in == -1) {
        errorStr = "color.c: color table not found\n";
        result = 0;
    } else {
        for (i = 0; i < PALETTE_SIZE; i++) {
            if (readFunc) {
                readFunc(in, &r, sizeof(Color));
                readFunc(in, &g, sizeof(Color));
                readFunc(in, &b, sizeof(Color));
            }

            if ((r <= 0x3F) && (g <= 0x3F) && (b <= 0x3F)) {
                mappedColor[i] = 1;
            } else {
                r = 0;
                g = 0;
                b = 0;
                mappedColor[i] = 0;
            }

            setColorPaletteEntry(i, r, g, b);
        }

        if (readFunc) {
            readFunc(in, colorTable, sizeof(colorTable));
        }

        tag = 0;
        if (readFunc) {
            readFunc(in, &tag, sizeof(tag));
        }

        if (tag == 0x4E455743 /* 'NEWC' */) {
            if (readFunc) {
                readFunc(in, intensityColorTable, 256 * PALETTE_SIZE);
                readFunc(in, colorMixAddTable, 256 * PALETTE_SIZE);
                readFunc(in, colorMixMulTable, 256 * PALETTE_SIZE);
            }
        } else {
            setIntensityTables();
            setMixTable();
        }

        rebuildColorBlendTables();

        if (closeFunc) {
            closeFunc(in);
        }

        result = 1;
    }

    return result;
}

char* colorError(void) { return errorStr; }

void setColorPalette(uint8_t* pal) {
    memcpy(cmap, pal, sizeof(cmap));
    memset(mappedColor, 1, sizeof(mappedColor));
}

void setColorPaletteEntry(int32_t entry, uint8_t r, uint8_t g, uint8_t b) {
    int32_t baseIndex;

    baseIndex = 3 * entry;

    cmap[baseIndex] = r;
    cmap[baseIndex + 1] = g;
    cmap[baseIndex + 2] = b;
}

void getColorPaletteEntry(int32_t entry, uint8_t* r, uint8_t* g, uint8_t* b) {
    int32_t baseIndex;

    baseIndex = 3 * entry;

    *r = cmap[baseIndex];
    *g = cmap[baseIndex + 1];
    *b = cmap[baseIndex + 2];
}

void buildBlendTable(ColorBlendTable* table, ColorIndex c) {
    Color mixcolor;
    Color backcolor;

    ColorRGB r;
    ColorRGB g;
    ColorRGB b;

    ColorRGB nr;
    ColorRGB ng;
    ColorRGB nb;

    ColorRGB br;
    ColorRGB bg;
    ColorRGB bb;

    int32_t k;
    int32_t j;

    mixcolor = Index2Color(c);

    br = (Color2RGB(mixcolor) & 0x7C00) >> 10;
    bg = (Color2RGB(mixcolor) & 0x3E0) >> 5;
    bb = Color2RGB(mixcolor) & 0x1F;

    for (k = 0; k < 256; ++k) {
        (*table)[0][k] = k;
    }

    for (j = 1; j < 8; ++j) {
        for (k = 0; k < 256; ++k) {
            backcolor = Index2Color(k);

            r = (Color2RGB(backcolor) & 0x7C00) >> 10;
            g = (Color2RGB(backcolor) & 0x3E0) >> 5;
            b = Color2RGB(backcolor) & 0x1F;

            nr = ((r * (7 - j) + br * j) / 7) << 10;
            ng = ((g * (7 - j) + bg * j) / 7) << 5;
            nb = (b * (7 - j) + bb * j) / 7;

            (*table)[j][k] = Color2Index(Index2Color(colorTable[nr | ng | nb]));
        }
    }

    for (j = 8; j < 16; ++j) {
        for (k = 0; k < 256; ++k) {
            (*table)[j][k] = Color2Index(calculateColor(((j - 8) << 16) / 7 + 0xFFFF, mixcolor));
        }
    }
}

void rebuildColorBlendTables(void) {
    for (int32_t i = 0; i < 256; i++) {
        if (blendTable[i]) {
            buildBlendTable(blendTable[i], i);
        }
    }
}

ColorBlendTable* getColorBlendTable(ColorIndex c) {
    if (!blendTable[c]) {
        blendTable[c] = (ColorBlendTable*)((char*)malloc(sizeof(ColorBlendTable) + sizeof(int32_t)) + sizeof(int32_t));

        SDL_assert(blendTable[c]);

        ((int32_t*)blendTable[c])[-1] = 1;

        buildBlendTable(blendTable[c], c);
    }

    ((int32_t*)blendTable[c])[-1]++;

    return blendTable[c];
}

void freeColorBlendTable(ColorIndex c) {
    if (blendTable[c]) {
        ((int32_t*)blendTable[c])[-1]--;

        if (!((int32_t*)blendTable[c])[-1]) {
            free(&((int32_t*)blendTable[c])[-1]);
            blendTable[c] = NULL;
        }
    }
}

void colorGamma(double gamma) {
    double a;

    currentGamma = gamma;

    for (int32_t i = 0; i < 64; i++) {
        a = pow(i, currentGamma);

        if (a > 63.0) {
            a = 63.0;
        }

        if (a < 0.0) {
            a = 0.0;
        }

        currentGammaTable[i] = a;
    }

    setSystemPalette(systemCmap);
}

double colorGetGamma(void) { return currentGamma; }

int32_t colorMappedColor(ColorIndex i) { return mappedColor[i]; }

int32_t colorBuildColorTable(uint8_t* colormap, uint8_t* matchtable) {
    Color r;
    Color g;
    Color b;

    int32_t c3Index;
    int32_t result;
    uint32_t* dist_buf;

    colormax = 1 << 5;
    x = 8;
    xsqr = 8 * 8;
    gstride = 1 << 5;
    rstride = 1 << 10;

    dist_buf = (uint32_t*)malloc(sizeof(uint32_t) * colormax * colormax * colormax);

    if (dist_buf) {
        maxfill(dist_buf, colormax);
        memset(colorTable, 0, sizeof(colorTable));

        for (cindex = 0; cindex < PALETTE_SIZE; cindex++) {
            if (matchtable && matchtable[cindex]) {
                mappedColor[cindex] = 0;
            } else {
                c3Index = 3 * cindex;

                r = colormap[c3Index];
                g = colormap[c3Index + 1];
                b = colormap[c3Index + 2];

                rcenter = (int32_t)r >> 3;
                gcenter = (int32_t)g >> 3;
                bcenter = (int32_t)b >> 3;

                rdist = r - (x * ((int32_t)r >> 3) + x / 2);
                gdist = g - (x * ((int32_t)g >> 3) + x / 2);
                cdist = b - (x * ((int32_t)b >> 3) + x / 2);

                cdist = cdist * cdist + gdist * gdist / 36 + rdist * rdist / 9;

                crinc = 2 * ((rcenter + 1) * xsqr - x * r);
                cginc = 2 * ((gcenter + 1) * xsqr - x * g);
                cbinc = 2 * ((bcenter + 1) * xsqr - x * b);

                cdp = &dist_buf[rstride * rcenter + gstride * gcenter + bcenter];

                crgbp = &colorTable[rstride * rcenter + gstride * gcenter + bcenter];

                redloop();

                cmap[c3Index] = r >> 2;
                cmap[c3Index + 1] = g >> 2;
                cmap[c3Index + 2] = b >> 2;

                mappedColor[cindex] = 1;
            }
        }

        free(dist_buf);

        setIntensityTables();

        setMixTable();

        rebuildColorBlendTables();

        result = 0;
    } else {
        result = 1;
    }

    return result;
}

int32_t colorSetColorTable(uint8_t* colormap, uint8_t* matchtable) {
    int32_t result;

    if (colorBuildColorTable(colormap, matchtable)) {
        result = 1;
    } else {
        setSystemPalette(cmap);

        result = 0;
    }

    return result;
}

int32_t redloop(void) {
    SDL_assert(0 /** \todo implement unused code */);

    return 0;
}

int32_t greenloop(int32_t restart) {
    SDL_assert(0 /** \todo implement unused code */);

    return 0;
}

int32_t blueloop(int32_t restart) {
    SDL_assert(0 /** \todo implement unused code */);

    return 0;
}

void maxfill(uint32_t* buffer, int32_t side) {
    int32_t i;
    int32_t count;

    count = side * side * side;

    for (i = 0; i < count; i++) {
        buffer[i] = -1;
    }
}

int32_t colorPushColorPalette(void) {
    SDL_assert(0 /** \todo implement unused code */);

    return 0;
}
int32_t colorPopColorPalette(void) {
    SDL_assert(0 /** \todo implement unused code */);

    return 0;
}

int32_t initColors(void) {
    int32_t result;

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

void colorsClose(void) {
    int32_t i;

    for (i = 0; i < 256; i++) {
        freeColorBlendTable(i);
    }

    /** \todo implement color palette stack clean up code */
}

uint8_t* getColorPalette(void) { return cmap; }
