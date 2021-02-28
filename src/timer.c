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

#include "timer.h"

#include <SDL.h>

int timer_init(void) { return 0; }

int timer_close(void) { return 0; }

/* PIT clock rate is 105/88 = 1.19318 MHz
 * 1 tick = 0.838095238095238 us
 * 1 ms = 1193.18181818182 ticks = (ticks * 88)/105000
 * SDL_Delay works on milliseconds basis
 */
#define TIMER_CONVERT_TICKS_TO_MS(ticks) ((unsigned int)(((unsigned long long)(ticks)*88uL) / 105000uL))
#define TIMER_CONVERT_MS_TO_TICKS(ms) ((unsigned int)(((unsigned long long)(ms)*105000uL) / 88uL))

void timer_wait(unsigned int ticks_to_wait) { SDL_Delay(TIMER_CONVERT_TICKS_TO_MS(ticks_to_wait)); }

unsigned int timer_elapsed_time_ms(unsigned int time_stamp) {
    unsigned int result;
    unsigned int time_stamp_ms;
    unsigned int time_ms;

    time_stamp_ms = TIMER_CONVERT_TICKS_TO_MS(time_stamp);
    time_ms = SDL_GetTicks();

    if (time_ms > time_stamp_ms) {
        result = time_ms - time_stamp_ms;
    } else {
        result = 0uL;
    }

    return result;
}

void timer_ch2_setup(void) {}

void timer_set_rate(void) {}

unsigned int timer_get_stamp32(void) { return TIMER_CONVERT_MS_TO_TICKS(SDL_GetTicks()); }
