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

#ifndef TIMER_H
#define TIMER_H

/* PIT clock rate is 105/88 = 1.19318 MHz
 * 1 tick = 0.838095238095238 us
 * 1 ms = 1193.18181818182 ticks = (ticks * 88)/105000
 * SDL_Delay works on milliseconds basis
 */
#define TIMER_TICKS_TO_MS(ticks) ((unsigned int)(((unsigned long long)(ticks)*88uL) / 105000uL))
#define TIMER_MS_TO_TICKS(ms) ((unsigned int)(((unsigned long long)(ms)*105000uL) / 88uL))
#define TIMER_FPS_TO_TICKS(fps) ((unsigned int)((105000000uL) / (88uL * (fps))))

int timer_init(void);
int timer_close(void);
void timer_wait(unsigned int ticks_to_wait);
unsigned int timer_elapsed_time_ms(unsigned int time_stamp);
void timer_ch2_setup(void);
void timer_set_rate(void);
unsigned int timer_get_stamp32(void);

#endif /* TIMER_H */
