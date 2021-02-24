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

#include "game.h"

#ifdef __unix__
char *strupr(char *s) { return dos_strupr(s); }
char *strlwr(char *s) { return dos_strlwr(s); }
int stricmp(const char *s1, const char *s2) { return strcasecmp(s1, s2); }
int strnicmp(const char *s1, const char *s2, size_t len) { return strncasecmp(s1, s2, len); }
#endif

static unsigned int next = 1;

long int filesize(FILE *fp) {
    long int save_pos, size_of_file;

    save_pos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    size_of_file = ftell(fp);
    fseek(fp, save_pos, SEEK_SET);

    return (size_of_file);
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

void dos_delay_init() { return; }

void dos_init_argv() { return; }

void dos_setenvp() { return; }

unsigned int get_dpmi_physical_memory(void) { return 64000000uLL; }

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

    SDL_Log("fopen failed: %s\n", filename);

    return NULL;
}

void dos_assert(int expr, char *str_expr, char *str_file, int line) {
    fprintf(stderr, "Assertion failed: %s, file %s, line %i\n", str_expr, str_file, line);
    fflush(stderr);
    abort();
}

int dos_vsprintf(char *buf, const char *format, va_list *va_arg) { return vsprintf(buf, format, *va_arg); }

void dos_getdrive(unsigned int *drive) { *drive = 4; }

void dos_setdrive(unsigned int drive, unsigned int *total) {}

static unsigned int *initrandnext() { return &next; }

int dos_rand(void) {
    unsigned int *next;
    int result;

    next = initrandnext();
    if (next) {
        *next = (*next) * 1103515245UL  + 12345UL;
        result = ((*next) >> 16) & 0x7FFF;
    } else {
        result = 0;
    }

    return result;
}

void dos_srand(unsigned int seed) {
    unsigned int *next;

    next = initrandnext();
    if (next) {
        *next = seed;
    }
}
