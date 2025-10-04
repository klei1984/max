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

#ifndef SVGA_H
#define SVGA_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "rect.h"

typedef void (*ScreenBlitFunc)(uint8_t* srcBuf, uint32_t srcW, uint32_t srcH, uint32_t subX, uint32_t subY,
                               uint32_t subW, uint32_t subH, uint32_t dstX, uint32_t dstY);

extern Rect scr_size;
extern ScreenBlitFunc scr_blit;

int32_t Svga_Init(void);
void Svga_Deinit(void);
void Svga_Blit(uint8_t* srcBuf, uint32_t srcW, uint32_t srcH, uint32_t subX, uint32_t subY, uint32_t subW,
               uint32_t subH, uint32_t dstX, uint32_t dstY);
int32_t Svga_WarpMouse(int32_t window_x, int32_t window_y);
void Svga_SetPaletteColor(int32_t index, SDL_Color* color);
void Svga_SetPalette(SDL_Palette* palette);
SDL_Palette* Svga_GetPalette(void);
int32_t Svga_GetScreenWidth(void);
int32_t Svga_GetScreenHeight(void);
int32_t Svga_GetScreenRefreshRate(void);
bool Svga_IsFullscreen(void);
bool Svga_GetWindowFlags(uint32_t* flags);

#ifdef __cplusplus
}
#endif

#endif /* SVGA_H */
