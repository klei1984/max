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

#define DOS_O_RDONLY 0x0000
#define DOS_O_WRONLY 0x0001
#define DOS_O_RDWR 0x0002
#define DOS_O_APPEND 0x0010
#define DOS_O_CREAT 0x0020
#define DOS_O_TRUNC 0x0040
#define DOS_O_NOINHERIT 0x0080
#define DOS_O_TEXT 0x0100
#define DOS_O_BINARY 0x0200
#define DOS_O_EXCL 0x0400

#define DOS_CLOCKS_PER_SEC 100

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

struct DOS_SegmentRegisters {
    unsigned short es;
    unsigned short cs;
    unsigned short ss;
    unsigned short ds;
    unsigned short fs;
    unsigned short gs;
};

typedef struct DOS_SegmentRegisters DOS_SegmentRegisters;

struct dostime_t {
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
    unsigned char hsecond; /* .01 of a sec */
};

struct dosdate_t {
    unsigned char day;
    unsigned char month;
    unsigned short year;
    unsigned char dayofweek;
};

// int dos_open_flags_to_native (int flags);

// bool dos_path_to_native (const char *path, char *buffer, size_t len);

int dos_sopen(const char *path, int open_flags, int share_flags, ...);

int dos_open(const char *path, int open_flags, ...);

void dos_gettime(struct dostime_t *t);

void dos_getdate(struct dosdate_t *d);

int dos_int386(int num, DOS_Registers *regs, DOS_Registers *out_regs) __attribute__((noreturn));

int dos_int386x(int num, DOS_Registers *regs, DOS_Registers *out_regs, DOS_SegmentRegisters *sregs)
    __attribute__((noreturn));

void *dos_getvect(int num) __attribute__((noreturn));

void dos_setvect(int num, void *function) __attribute__((noreturn));

void dos_delay_init();

void dos_init_387_emulator(int);

void dos_fini_387_emulator(void);

void dos_init_argv();

void dos_setenvp();

unsigned int get_dpmi_physical_memory(void);

unsigned int _dos_open(const char *path, unsigned int mode, int *handle);

void dos_assert(int expr, char *str_expr, char *str_file, int line);

void dos_getdrive(unsigned int *drive);

void dos_setdrive(unsigned int drive, unsigned int *total);

#endif /* DOS_H */
