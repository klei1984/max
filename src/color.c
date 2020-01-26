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

#include <stdio.h>

#include "svga.h"
#include "wrappers.h"

static void update_system_palette(SDL_Palette *palette);

static SDL_Palette *SystemPalette;

#define PALETTE_SIZE 256

static void update_system_palette(SDL_Palette *palette) {
    SDL_Surface *screen;

    screen = svga_get_screen();

    if (screen) {
        if (0 != SDL_SetPaletteColors(screen->format->palette, SystemPalette->colors, 0, PALETTE_SIZE)) {
            fprintf(stderr, "SDL_SetPaletteColors failed: %s\n", SDL_GetError());
        }
    } else {
        fprintf(stderr, "SDL_Surface is NULL\n");
    }
}

void setSystemPalette(unsigned char *cmap) {
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
