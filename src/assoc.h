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

#ifndef ASSOC_H
#define ASSOC_H

#include <stddef.h>
#include <stdio.h>

typedef struct db_file_s* DB_FILE;

typedef void* (*assoc_malloc_func)(size_t);
typedef void* (*assoc_realloc_func)(void*, size_t);
typedef void (*assoc_free_func)(void*);

typedef int (*assoc_load_func_db)(DB_FILE stream, void*, size_t, int);
typedef int (*assoc_save_func_db)(DB_FILE stream, void*, size_t, int);
typedef int (*assoc_load_func)(FILE* fp, void*, size_t, int);
typedef int (*assoc_save_func)(FILE* fp, void*, size_t, int);

struct assoc_func_list_s {
    assoc_load_func loadFunc;
    assoc_save_func saveFunc;
    assoc_load_func_db loadFuncDB;
    assoc_save_func_db saveFuncDB;
};

typedef struct assoc_func_list_s assoc_func_list;

struct assoc_pair_s {
    char* name;
    void* data;
};

typedef struct assoc_pair_s assoc_pair;

struct assoc_array_s {
    int init_flag;
    int size;
    int max;
    size_t datasize;
    assoc_func_list load_save_funcs;
    assoc_pair* list;
};

typedef struct assoc_array_s assoc_array;

int assoc_init(assoc_array* a, int n, size_t datasize, assoc_func_list* assoc_funcs);
int assoc_resize(assoc_array* a, int n);
int assoc_free(assoc_array* a);
int assoc_search(assoc_array* a, char* name);
int assoc_insert(assoc_array* a, char* name, void* data);
int assoc_delete(assoc_array* a, char* name);
int assoc_copy(assoc_array* dst, assoc_array* src);
int assoc_find_first(assoc_array* a, char* name, int find_num);
int assoc_load(FILE* fp, assoc_array* a, int flags);
int assoc_save(FILE* fp, assoc_array* a, int flags);
void assoc_register_mem(assoc_malloc_func malloc_func, assoc_realloc_func realloc_func, assoc_free_func free_func);

#endif /* ASSOC_H */
