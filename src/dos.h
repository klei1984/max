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
#include <sys/io.h>
#include <unistd.h>
#else
#include <io.h>
#endif

struct DOS_Registers32 {
    unsigned int eax;    /* 00 */
    unsigned int ebx;    /* 04 */
    unsigned int ecx;    /* 08 */
    unsigned int edx;    /* 0c */
    unsigned int esi;    /* 10 */
    unsigned int edi;    /* 14 */
    unsigned int eflags; /* 18 */
};

typedef struct DOS_Registers32 DOS_Registers32;

struct DOS_Registers16 {
    unsigned short ax;
    unsigned short _unused_1;
    unsigned short bx;
    unsigned short _unused_2;
    unsigned short cx;
    unsigned short _unused_3;
    unsigned short dx;
    unsigned short _unused_4;
    unsigned short si;
    unsigned short _unused_5;
    unsigned short di;
    unsigned short _unused_6;
    signed short flags;
};

typedef struct DOS_Registers16 DOS_Registers16;

struct DOS_Registers8 {
    unsigned char al;
    unsigned char ah;
    unsigned short _unused_1;
    unsigned char bl;
    unsigned char bh;
    unsigned short _unused_2;
    unsigned char cl;
    unsigned char ch;
    unsigned short _unused_3;
    unsigned char dl;
    unsigned char dh;
};

typedef struct DOS_Registers8 DOS_Registers8;

union DOS_Registers {
    DOS_Registers32 r32;
    DOS_Registers16 r16;
    DOS_Registers8 r8;
};

typedef union DOS_Registers DOS_Registers;

#ifdef __unix__
char *strupr(char *s);
char *strlwr(char *s);
char *strupr(char *s);
int stricmp(const char *s1, const char *s2);
int strnicmp(const char *s1, const char *s2, size_t len);
#endif

long int filesize(FILE *fp);

int dos_int386(int num, DOS_Registers *regs, DOS_Registers *out_regs) __attribute__((noreturn));

void dos_delay_init();

void dos_init_argv();

void dos_setenvp();

unsigned int get_dpmi_physical_memory(void);

void dos_assert(int expr, char *str_expr, char *str_file, int line);

void dos_getdrive(unsigned int *drive);

void dos_setdrive(unsigned int drive, unsigned int *total);

int dos_rand(void);

void dos_srand(unsigned int seed);

#endif /* DOS_H */
