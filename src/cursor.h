/* Copyright (c) 2025 M.A.X. Port Team
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

#ifndef CURSOR_H
#define CURSOR_H

/**
 * \file cursor.h
 * \brief C-compatible interface for hardware cursor functions.
 *
 * This header provides extern "C" linkage for cursor visibility functions that need to be called from C code (e.g.,
 * mouse.c).
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Show the hardware cursor.
 *
 * Makes the cursor visible on screen. If the cursor cache is dirty, it will be regenerated before showing.
 */
void Cursor_Show(void);

/**
 * \brief Hide the hardware cursor.
 *
 * Makes the cursor invisible on screen.
 */
void Cursor_Hide(void);

#ifdef __cplusplus
}
#endif

#endif /* CURSOR_H */
