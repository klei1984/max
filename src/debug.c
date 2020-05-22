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

#include "debug.h"

#include "game.h"

static int debug_log(char *str);
static int debug_screen(char *str);
static void debug_exit(void);

static text_size_func debug_func;
static FILE *fd;

void GNW_debug_init(void) { atexit(debug_exit); }

void debug_register_log(char *fname, char *mode) {
    if ((mode[0] == 'w' || mode[0] == 'a') && mode[1] == 't') {
        if (fd) {
            int result = fclose(fd);
            SDL_assert(result == 0);
        }

        fd = fopen(fname, mode);
        debug_func = debug_log;
    }
}

void debug_register_screen(void) {
    if (debug_func != debug_screen) {
        if (fd) {
            fclose(fd);
            fd = NULL;
        }
        debug_func = debug_screen;
    }
}

void debug_register_env(void) {
    char *ptr;

    ptr = getenv("DEBUGACTIVE");
    if (ptr != NULL) {
        if (!strcmp(ptr, "log")) {
            debug_register_log("debug.log", "wt");
        } else if (!strcmp(ptr, "screen")) {
            debug_register_screen();
        } else if (!strcmp(ptr, "gnw")) {
            debug_register_func(win_debug);
        }
    }
}

void debug_register_func(text_size_func func) {
    if (debug_func != func) {
        if (fd) {
            int result = fclose(fd);
            SDL_assert(result == 0);
            fd = NULL;
        }
        debug_func = func;
    }
}

int debug_printf(char *format, ...) {
    int result;

    if (debug_func) {
        char buffer[256];
        va_list args;

        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);

        result = debug_func(buffer);
        va_end(args);
    } else {
        result = -1;
    }

    return result;
}

int debug_puts(char *str) {
    int result;

    if (debug_func) {
        result = debug_func(str);
    } else {
        result = -1;
    }

    return result;
}

void debug_clear(void) {
    if (debug_func == debug_screen) {
        /* clear screen in a portable way */;
    }
}

int debug_log(char *str) {
    int result;

    if ((debug_func != debug_log) || (fd && (fprintf(fd, str) >= 0) && (fflush(fd) != -1))) {
        result = 0;
    } else {
        result = -1;
    }

    return result;
}

int debug_screen(char *str) {
    if (debug_func == debug_screen) {
        printf(str);
        fflush(stdout);
    }

    return 0;
}

void debug_exit(void) {
    if (fd) {
        int result = fclose(fd);
        SDL_assert(result == 0);
        fd = NULL;
    }
}
