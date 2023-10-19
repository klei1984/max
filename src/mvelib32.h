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

#ifndef MVELIB32_H
#define MVELIB32_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*mve_cb_Alloc)(size_t size);
typedef void (*mve_cb_Free)(void *ptr);
typedef int32_t (*mve_cb_Read)(FILE *handle, void *buf, size_t count);
typedef void (*mve_cb_ShowFrame)(uint8_t *buffer, int32_t bufw, int32_t bufh, int32_t sx, int32_t sy, int32_t w,
                                 int32_t h, int32_t dstx, int32_t dsty);
typedef void (*mve_cb_SetPalette)(uint8_t *p, int32_t start, int32_t count);
typedef int32_t (*mve_cb_Ctl)(void);

void MVE_sndInit(void *handle);
void MVE_memCallbacks(mve_cb_Alloc alloc, mve_cb_Free free);
void MVE_logDumpStats(void);
void MVE_ioCallbacks(mve_cb_Read read);
void MVE_sndVolume(uint32_t volume);
void MVE_sfSVGA(int32_t width, int32_t height, int32_t bytes_per_scan_line, int32_t write_window,
                uint8_t *write_win_ptr, int32_t window_size, int32_t window_granuality, void *window_function,
                int32_t hicolor);
void MVE_sfCallbacks(mve_cb_ShowFrame showframe);
void MVE_palCallbacks(mve_cb_SetPalette setpalette);
void MVE_rmCallbacks(mve_cb_Ctl ctl);
void MVE_rmFastMode(int32_t fastmode);
void MVE_rmHScale(int32_t hscale_step);
void MVE_rmFrameCounts(int32_t *frame_count, int32_t *drop_count);
int32_t MVE_rmUnprotect(void);
int32_t MVE_rmPrepMovie(FILE *handle, int32_t dx, int32_t dy, int32_t track);
int32_t MVE_rmHoldMovie(void);
int32_t MVE_rmStepMovie(void);
void MVE_rmEndMovie(void);
int32_t MVE_RunMovie(FILE *handle, int32_t dx, int32_t dy, int32_t track);
int32_t MVE_RunMovieContinue(FILE *handle, int32_t dx, int32_t dy, int32_t track);
void MVE_ReleaseMem(void);
const char *MVE_strerror(int32_t error_code);

#ifdef __cplusplus
}
#endif

#endif /* MVELIB32_H */
