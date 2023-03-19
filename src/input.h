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

#ifndef INPUT_H
#define INPUT_H

#include "interface.h"

typedef void (*BackgroundProcess)(void);
typedef WinID (*PauseWinFunc)(void);
typedef int (*ScreenDumpFunc)(int width, int length, unsigned char* buf, unsigned char* pal);

#define GNW_INPUT_PRESS 0x7000

int GNW_input_init(void);
void GNW_input_exit(void);
int get_input(void);
void get_input_position(int* x, int* y);
void process_bk(void);
void GNW_add_input_buffer(int input);
void flush_input_buffer(void);
void GNW_do_bk_process(void);
void add_bk_process(BackgroundProcess f);
void remove_bk_process(BackgroundProcess f);
void enable_bk(void);
void disable_bk(void);
void register_pause(int new_pause_key, PauseWinFunc new_pause_win_func);
void dump_screen(void);
void register_screendump(int new_screendump_key, ScreenDumpFunc new_screendump_func);
TOCKS get_time(void);
void pause_for_tocks(unsigned int tocks);
void block_for_tocks(unsigned int tocks);
TOCKS elapsed_time(TOCKS past_time);
TOCKS elapsed_tocks(TOCKS end_time, TOCKS start_time);
void GNW_process_message(void);

#endif /* INPUT_H */
