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

int timer_init(void) { return 0; }

int timer_close(void) { return 0; }

void timer_wait(unsigned int ticks_to_wait) {
    /* PIT clock rate is 105/88 = 1.19318 MHz
     * SDL_Delay works on milliseconds basis
     */
    SDL_Delay((ticks_to_wait * 88uL) / 105000uL);
}

unsigned int timer_time_remaining_ms(unsigned int time_stamp) {
    unsigned int result;
    unsigned int elapsed_ticks;

    elapsed_ticks = timer_get_stamp32();
    if (elapsed_ticks > time_stamp) {
        result = ((elapsed_ticks - time_stamp) * 88uL) / 105000uL;
    } else {
        result = 0;
    }

    return result;
}

void timer_ch2_setup(void) {}

void timer_set_rate(void) {}

unsigned int timer_get_stamp32(void) {
    /* PIT clock rate is 105/88 = 1.19318 MHz
     * SDL_GetTicks works on milliseconds basis
     */
    return (SDL_GetTicks() * 105000uL) / 88uL;
}

TOCKS get_time(void) {
    /* PIT clock rate is 105/88 = 1.19318 MHz
     * SDL_GetTicks works on milliseconds basis
     */
    return (SDL_GetTicks() * 105000uL) / 88uL;
};
