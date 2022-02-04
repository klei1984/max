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

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*mve_cb_Alloc)(size_t size);
typedef void (*mve_cb_Free)(void *ptr);
typedef int (*mve_cb_Read)(FILE *handle, void *buf, size_t count);
typedef void (*mve_cb_ShowFrame)(unsigned char *buffer, int bufw, int bufh, int sx, int sy, int w, int h, int dstx,
                                 int dsty);
typedef void (*mve_cb_SetPalette)(unsigned char *p, int start, int count);
typedef int (*mve_cb_Ctl)(void);

void MVE_memCallbacks(mve_cb_Alloc alloc, mve_cb_Free free);
void MVE_logDumpStats(void);
void MVE_ioCallbacks(mve_cb_Read read);
void MVE_sndVolume(unsigned int volume);
int MVE_sndConfigure();
void MVE_sndPause(void);
void MVE_sndResume(void);
void MVE_sfSVGA(int width, int height, int bytes_per_scan_line, int write_window, void *write_win_ptr, int window_size,
                int window_granuality, void *window_function, int hicolor);
void MVE_sfCallbacks(mve_cb_ShowFrame showframe);
void MVE_palCallbacks(mve_cb_SetPalette setpalette);
void MVE_rmCallbacks(mve_cb_Ctl ctl);
void MVE_rmFastMode(int fastmode);
void MVE_rmHScale(int hscale_step);
void MVE_rmFrameCounts(int *frame_count, int *drop_count);
int MVE_rmUnprotect(void);
int MVE_rmPrepMovie(FILE *handle, int dx, int dy, int track);
int MVE_rmHoldMovie(void);
int MVE_rmStepMovie(void);
void MVE_rmEndMovie(void);
int MVE_RunMovie(FILE *handle, int dx, int dy, int track);
void MVE_ReleaseMem(void);
const char *MVE_strerror(int error_code);

/// \todo Reimplement missing stuff
extern int MVE_RunMovie(FILE* fp, int dx, int dy, int track)	__asm("_MVE_RunMovie_");
extern int MVE_gfxMode(int mode)	__asm("_MVE_gfxMode_");
extern void MVE_sfSVGA(int width, int height, int bytes_per_scan_line, int write_window, void *write_win_ptr, int window_size, int window_granuality, void *window_function, int hicolor)	__asm("_MVE_sfSVGA_");
extern int MVE_rmPrepMovie(FILE* fp, int dx, int dy, int track)	__asm("_MVE_rmPrepMovie_");
extern int MVE_rmStepMovie(void)	__asm("_MVE_rmStepMovie_");
extern void MVE_rmEndMovie(void)	__asm("_MVE_rmEndMovie_");
extern void MVE_ReleaseMem(void)	__asm("_MVE_ReleaseMem_");

#ifdef __cplusplus
}
#endif

#endif /* MVELIB32_H */
