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

#include "dos.h"

#include <SDL.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "svga.h"
#include "wrappers.h"

#ifdef __unix__
#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif

static int open_flag_mapping[][2] = {{DOS_O_RDONLY, O_RDONLY},
                                     {DOS_O_WRONLY, O_WRONLY},
                                     {DOS_O_RDWR, O_RDWR},
                                     {DOS_O_APPEND, O_APPEND},
                                     {DOS_O_CREAT, O_CREAT},
                                     {DOS_O_TRUNC, O_TRUNC},
                                     {DOS_O_NOINHERIT, 0},
                                     {DOS_O_TEXT, 0},
                                     {DOS_O_BINARY, O_BINARY},
                                     {DOS_O_EXCL, O_EXCL},
                                     {-1, -1}};

#ifdef __unix__
static inline char *strupr(char *s) { return dos_strupr(s); }
int stricmp(const char *s1, const char *s2) { return strcasecmp(s1, s2); }
int strnicmp(const char *s1, const char *s2, size_t len) { return strncasecmp(s1, s2, len); }
#endif

int dos_open_flags_to_native(int flags) {
    int rflags = 0;
    int n;

    for (n = 0; open_flag_mapping[n][0] != -1; n++) {
        if ((open_flag_mapping[n][0] & flags) == open_flag_mapping[n][0]) rflags |= open_flag_mapping[n][1];
    }

    return rflags;
}

static void __attribute__((noreturn)) print_interrupt_info_and_abort(int num, DOS_Registers *regs, void *caller) {
    fprintf(stderr,
            "DOS Interrupt via int386/int386x ()\n"
            "  Called from: %p\n"
            "  int %02x\n"
            "  eax: %08x  ebx: %08x  ecx: %08x  edx: %08x\n"
            "  esi: %08x  edi: %08x  eflags: %08x\n",
            caller, num, regs->r32.eax, regs->r32.ebx, regs->r32.ecx, regs->r32.edx, regs->r32.esi, regs->r32.edi,
            regs->r32.eflags);
    fflush(stderr);
    abort();
}

int dos_int386(int num, DOS_Registers *regs, DOS_Registers *out_regs) {
    void *eip_caller = *(&eip_caller + 8);
    print_interrupt_info_and_abort(num, regs, eip_caller);
}

int dos_int386x(int num, DOS_Registers *regs, DOS_Registers *out_regs, DOS_SegmentRegisters *sregs) {
    void *eip_caller = *(&eip_caller + 9);
    print_interrupt_info_and_abort(num, regs, eip_caller);
}

static void __attribute__((noreturn)) print_caller_info_and_abort(const char *function, void *caller) {
    fprintf(stderr,
            "DOS-specific function %s ()\n"
            "  Called from: %p\n",
            function, caller);
    fflush(stderr);
    abort();
}

void *__attribute__((noreturn)) dos_getvect(int num) {
    void *eip_caller = *(&eip_caller + 6);
    print_caller_info_and_abort("dos_getvect", eip_caller);
}

void __attribute__((noreturn)) dos_setvect(int num, void *function) {
    void *eip_caller = *(&eip_caller + 7);
    print_caller_info_and_abort("dos_setvect", eip_caller);
}

void dos_delay_init() { return; }

void dos_init_387_emulator(int control_word) { return; }

void dos_fini_387_emulator(void) { return; }

void dos_init_argv() { return; }

void dos_setenvp() { return; }

unsigned int get_dpmi_physical_memory(void) { return 64000000uLL; }

static int open_helper(const char *path, unsigned int flags) {
    int handle;
    int length;
    char *path_ptr;

    if ((length = strlen(path)) != 0) {
        path_ptr = malloc(length + 1);
        if (path_ptr) {
            int handle;

            path_ptr = strncpy(path_ptr, path, length + 1);
            path_ptr = strupr(path_ptr);

            handle = open(path_ptr, flags, 0666);

            free(path_ptr);

            return handle;
        }
    }

    return -1;
}

int posix_open(const char *path, int open_flags, ...) {
    return open_helper(path, dos_open_flags_to_native(open_flags));
}

unsigned int _dos_open(const char *path, unsigned int mode, int *handle) {
    int file_handle;

    if (handle) {
        file_handle = open_helper(path, dos_open_flags_to_native(mode));
        if (file_handle != -1) {
            *handle = file_handle;
            return 0;
        }
    }

    return -1;
}

FILE *dos_fopen(const char *filename, const char *mode) {
    int length;
    char *path_ptr;
    FILE *handle;

    if ((length = strlen(filename)) != 0) {
        path_ptr = malloc(length + 1);
        if (path_ptr) {
            path_ptr = strncpy(path_ptr, filename, length + 1);
            path_ptr = strupr(path_ptr);

            handle = fopen(path_ptr, mode);

            free(path_ptr);

            return handle;
        }
    }

    return NULL;
}

void dos_assert(int expr, char *str_expr, char *str_file, int line) {
    fprintf(stderr, "Assertion failed: %s, file %s, line %i\n", str_expr, str_file, line);
    fflush(stderr);
    abort();
}
