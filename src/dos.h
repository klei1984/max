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

#ifndef DOS_H
#define DOS_H

#include <stdio.h>

#ifdef __unix__
#include <linux/limits.h>
#include <sys/io.h>
#include <unistd.h>

char *strupr(char *s);
char *strlwr(char *s);
int stricmp(const char *s1, const char *s2);
int strnicmp(const char *s1, const char *s2, size_t len);
#else
#include <io.h>
#endif

long int filesize(FILE *fp);

void dos_getdrive(unsigned int *drive);

void dos_setdrive(unsigned int drive, unsigned int *total);

int dos_rand(void);

void dos_srand(unsigned int seed);

#endif /* DOS_H */
