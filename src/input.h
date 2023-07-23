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

#include <stdint.h>

#include "interface.h"

typedef void (*BackgroundProcess)(void);
typedef WinID (*PauseWinFunc)(void);
typedef int32_t (*ScreenDumpFunc)(int32_t width, int32_t length, uint8_t* buf, uint8_t* pal);

#define GNW_INPUT_PRESS 0x7000

int32_t GNW_input_init(void);
void GNW_input_exit(void);
int32_t get_input(void);
void get_input_position(int32_t* x, int32_t* y);
void process_bk(void);
void GNW_add_input_buffer(int32_t input);
void flush_input_buffer(void);
void GNW_do_bk_process(void);
void add_bk_process(BackgroundProcess f);
void remove_bk_process(BackgroundProcess f);
void enable_bk(void);
void disable_bk(void);
void register_pause(int32_t new_pause_key, PauseWinFunc new_pause_win_func);
void dump_screen(void);
void register_screendump(int32_t new_screendump_key, ScreenDumpFunc new_screendump_func);
TOCKS get_time(void);
void pause_for_tocks(uint32_t tocks);
void block_for_tocks(uint32_t tocks);
TOCKS elapsed_time(TOCKS past_time);
TOCKS elapsed_tocks(TOCKS end_time, TOCKS start_time);
void GNW_process_message(void);

#endif /* INPUT_H */
