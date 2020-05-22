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

#include "memory.h"

#include <stdlib.h>

#include "game.h"

static void *my_malloc(size_t size);
static void *my_realloc(void *old_blk, size_t size);
static void my_free(void *ptr);
static void *mem_prep_block(void *ptr, size_t size);
static void mem_check_block(char *ptr);

static MallocFunc p_malloc = my_malloc;
static ReallocFunc p_realloc = my_realloc;
static FreeFunc p_free = my_free;

static int num_blocks;
static int max_blocks;
static size_t mem_allocated;
static size_t max_allocated;

void *mem_malloc(size_t size) { return p_malloc(size); }

void *my_malloc(size_t size) {
    void *ptr;
    size_t new_size = size + 3 * sizeof(int);
    void *result = NULL;

    if (size) {
        ptr = malloc(new_size);

        if (ptr) {
            buf_fill(ptr, new_size, 1, new_size, 0xCC);
            result = mem_prep_block(ptr, new_size);

            num_blocks++;
            if (num_blocks > max_blocks) {
                max_blocks = num_blocks;
            }

            mem_allocated += new_size;
            if (mem_allocated > max_allocated) {
                max_allocated = mem_allocated;
            }
        }
    }

    return result;
}

void *mem_realloc(void *old_blk, size_t size) { return p_realloc(old_blk, size); }

void *my_realloc(void *old_blk, size_t size) {
    int *int_ptr = NULL;
    void *new_ptr;
    void *result;

    if (old_blk) {
        int_ptr = &((int *)old_blk)[-2];
        mem_allocated -= int_ptr[0];
        mem_check_block((char *)int_ptr);
    }

    if (size) {
        size += 3 * sizeof(int);
    }

    new_ptr = (void *)realloc((char *)int_ptr, size);

    if (new_ptr) {
        if (!int_ptr) {
            num_blocks++;
            if (num_blocks > max_blocks) {
                max_blocks = num_blocks;
            }
        }

        mem_allocated += size;
        if (mem_allocated > max_allocated) {
            max_allocated = mem_allocated;
        }

        result = mem_prep_block(new_ptr, size);
    } else {
        if (size) {
            if (int_ptr) {
                mem_allocated += int_ptr[0];
                debug_printf("(%s,%u): ", __FILE__, __LINE__);
                debug_printf("Realloc failure.\n");
            }
        } else {
            --num_blocks;
        }

        result = NULL;
    }

    return result;
}

void mem_free(void *ptr) { p_free(ptr); }

void my_free(void *ptr) {
    if (ptr) {
        int *const int_ptr = &((int *)ptr)[-2];

        mem_check_block((char *)int_ptr);
        mem_allocated -= int_ptr[0];
        buf_fill((unsigned char *)int_ptr, int_ptr[0], 1, int_ptr[0], 0xCC);
        free(int_ptr);
        --num_blocks;
    }
}

void mem_check(void) {
    if (p_malloc == my_malloc) {
        debug_printf("Current memory allocated: %6d blocks, %9u bytes total\n", num_blocks, mem_allocated);
        debug_printf("Max memory allocated:     %6d blocks, %9u bytes total", max_blocks, max_allocated);
    }
}

void mem_register_func(MallocFunc *new_malloc, ReallocFunc *new_realloc, FreeFunc *new_free) {
    if (!GNW_win_init_flag) {
        p_malloc = (MallocFunc)new_malloc;
        p_realloc = (ReallocFunc)new_realloc;
        p_free = (FreeFunc)new_free;
    }
}

void *mem_prep_block(void *ptr, size_t size) {
    int *const int_ptr = (int *)ptr;
    char *const char_ptr = (char *)ptr;

    int_ptr[0] = size;
    int_ptr[1] = 0xFEEDFACE;
    ((int *)&char_ptr[size - sizeof(int)])[0] = 0xBEEFCAFE;

    return &int_ptr[2];
}

void mem_check_block(char *ptr) {
    int *const int_ptr = (int *)ptr;
    char *const char_ptr = (char *)ptr;
    size_t size = int_ptr[0];

    if (int_ptr[1] != 0xFEEDFACE) {
        debug_printf("Memory header stomped.\n");
    }
    if (((int *)&char_ptr[size - sizeof(int)])[0] != 0xBEEFCAFE) {
        debug_printf("Memory footer stomped.\n");
    }
}
