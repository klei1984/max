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

#if defined(__unix__)
#include <ctype.h>
#include <strings.h>

static char *dos_strupr(char *s);
static char *dos_strlwr(char *s);

char *strupr(char *s) { return dos_strupr(s); }
char *strlwr(char *s) { return dos_strlwr(s); }
int stricmp(const char *s1, const char *s2) { return strcasecmp(s1, s2); }
int strnicmp(const char *s1, const char *s2, size_t len) { return strncasecmp(s1, s2, len); }
#endif /* defined(__unix__) */

static uint32_t *initrandnext(void);

static uint32_t dos_rand_next = 1;

#if defined(__unix__)
char *dos_strupr(char *s) {
    char *p = s;
    while (*p) {
        *p = toupper(*p);
        ++p;
    }
    return s;
}
#endif /* defined(__unix__) */

#if defined(__unix__)
char *dos_strlwr(char *s) {
    char *p = s;
    while (*p) {
        *p = tolower(*p);
        ++p;
    }
    return s;
}
#endif /* defined(__unix__) */

long int filesize(FILE *fp) {
    long int save_pos, size_of_file;

    save_pos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    size_of_file = ftell(fp);
    fseek(fp, save_pos, SEEK_SET);

    return (size_of_file);
}

uint32_t *initrandnext(void) { return &dos_rand_next; }

int32_t dos_rand(void) {
    uint32_t *next;
    int32_t result;

    next = initrandnext();
    if (next) {
        *next = (*next) * 1103515245UL + 12345UL;
        result = ((*next) >> 16) & 0x7FFF;
    } else {
        result = 0;
    }

    return result;
}

void dos_srand(uint32_t seed) {
    uint32_t *next;

    next = initrandnext();
    if (next) {
        *next = seed;
    }
}
