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

#ifndef TYPES_H
#define TYPES_H

void get_start_mode(void);
int init_mode_640_480(void);
int init_vesa_mode(int mode, int width, int height, int half);

struct _RMREGSX {
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int pad1;
    unsigned int ebx;
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;
};

struct _RMREGSW {
    unsigned short di;
    unsigned short pad1;
    unsigned short si;
    unsigned short pad2;
    unsigned short bp;
    unsigned short pad3;
    unsigned int pad4;
    unsigned short bx;
    unsigned short pad5;
    unsigned short dx;
    unsigned short pad6;
    unsigned short cx;
    unsigned short pad7;
    unsigned short ax;
    unsigned short pad8;
    unsigned short flags;
    unsigned short es;
    unsigned short ds;
    unsigned short fs;
    unsigned short gs;
    unsigned short ip;
    unsigned short cs;
    unsigned short sp;
    unsigned short ss;
};

struct _RMREGSB {
    unsigned int pad1;
    unsigned int pad2;
    unsigned int pad3;
    unsigned int pad4;
    unsigned char bl;
    unsigned char bh;
    unsigned char pad5;
    unsigned char pad6;
    unsigned char dl;
    unsigned char dh;
    unsigned char pad7;
    unsigned char pad8;
    unsigned char cl;
    unsigned char ch;
    unsigned char pad9;
    unsigned char pad10;
    unsigned char al;
    unsigned char ah;
    unsigned char pad11;
    unsigned char pad12;
};

typedef union RMREGS {
    struct _RMREGSX x;
    struct _RMREGSW w;
    struct _RMREGSB h;
} RMREGS;

int dpmi_rmint(int intno, RMREGS* rmregs);

struct _ModeInfo {
    unsigned short ModeAttributes;
    unsigned char WinAAttributes;
    unsigned char WinBAttributes;
    unsigned short WinGranularity;
    unsigned short WinSize;
    unsigned short WinASegment;
    unsigned short WinBSegment;
    unsigned char* WinFuncPtr;
    unsigned short BytesPerScanLine;
    unsigned short XResolution;
    unsigned short YResolution;
    unsigned char XCharSize;
    unsigned char YCharSize;
    unsigned char NumberOfPlanes;
    unsigned char BitsPerPixel;
    unsigned char NumberOfBanks;
    unsigned char MemoryModel;
    unsigned char BankSize;
    unsigned char NumberOfImagePages;
    unsigned char ReservedPageFn;
    unsigned char RedMaskSize;
    unsigned char RedFieldPosition;
    unsigned char GreenMaskSize;
    unsigned char GreenFieldPosition;
    unsigned char BlueMaskSize;
    unsigned char BlueFieldPosition;
    unsigned char RsvdMaskSize;
    unsigned char RsvdFieldPosition;
    unsigned char DirectColorModeInfo;
    unsigned char Reserved[216];
};

typedef struct _ModeInfo* ModeInfoPtr;

unsigned int vbe_GetModeInfo(int mode, ModeInfoPtr info, unsigned char** pWinA, unsigned char** pWinB);

void* dpmi_RealToLinear(void* RealPtr);

#endif /* TYPES_H */
