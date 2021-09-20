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

#include "assoc.h"

#include <stdlib.h>
#include <string.h>

#include "gnw.h"

static void* default_malloc(size_t t);
static void* default_realloc(void* p, size_t t);
static void default_free(void* p);

static int assoc_find(assoc_array* a, char* name, int* position);
static int assoc_read_long(FILE* fp, long* theLong);
static int assoc_read_assoc_array(FILE* fp, assoc_array* a);
static int assoc_write_long(FILE* fp, long theLong);
static int assoc_write_assoc_array(FILE* fp, assoc_array* a);

static assoc_malloc_func internal_malloc = default_malloc;
static assoc_realloc_func internal_realloc = default_realloc;
static assoc_free_func internal_free = default_free;

void* default_malloc(size_t t) { return malloc(t); }

void* default_realloc(void* p, size_t t) { return realloc(p, t); }

void default_free(void* p) { free(p); }

int assoc_find(assoc_array* a, char* name, int* position) {
    int i;
    int j;
    int min;
    int max;

    if (a->init_flag != 0xFEBAFEBA) {
        return -1;
    }

    i = 0;
    j = 0;

    if (a->size) {
        min = 0;
        max = a->size - 1;

        while (max >= min) {
            i = (min + max) >> 1;
            j = stricmp(name, a->list[i].name);

            if (!j) {
                break;
            }

            if (j >= 0) {
                min = i + 1;
            } else {
                max = i - 1;
            }
        }

        if (!j) {
            *position = i;

            return 0;
        }

        if (j >= 0) {
            *position = i + 1;
        } else {
            *position = i;
        }

        return -1;
    }

    *position = 0;
    return -1;
}

int assoc_read_long(FILE* fp, long* theLong) {
    long temp;
    int c;

    c = fgetc(fp);

    if (c != -1) {
        temp = c;
        c = fgetc(fp);

        if (c != -1) {
            temp = c | (temp << 8);
            c = fgetc(fp);

            if (c != -1) {
                temp = c | (temp << 8);
                c = fgetc(fp);

                if (c != -1) {
                    *theLong = c | (temp << 8);

                    return 0;
                }
            }
        }
    }

    return -1;
}

int assoc_read_assoc_array(FILE* fp, assoc_array* a) {
    long temp;

    if (!assoc_read_long(fp, &temp)) {
        a->size = temp;
        if (!assoc_read_long(fp, &temp)) {
            a->max = temp;
            if (!assoc_read_long(fp, &temp)) {
                a->datasize = temp;
                if (!assoc_read_long(fp, &temp)) {
                    a->list = (void*)temp;

                    return 0;
                }
            }
        }
    }

    return -1;
}

int assoc_write_long(FILE* fp, long theLong) {
    int result;

    if (fputc(theLong >> 24, fp) == -1 || fputc(theLong >> 16, fp) == -1 || fputc(theLong >> 8, fp) == -1 ||
        fputc(theLong, fp) == -1) {
        result = -1;
    } else {
        result = 0;
    }

    return result;
}

int assoc_write_assoc_array(FILE* fp, assoc_array* a) {
    int result;

    if (assoc_write_long(fp, a->size) || assoc_write_long(fp, a->max) || assoc_write_long(fp, a->datasize) ||
        assoc_write_long(fp, (long)a->list)) {
        result = -1;
    } else {
        result = 0;
    }

    return result;
}

int assoc_init(assoc_array* a, int n, size_t datasize, assoc_func_list* assoc_funcs) {
    int result;

    result = 0;
    a->size = 0;
    a->max = n;
    a->datasize = datasize;

    if (assoc_funcs) {
        a->load_save_funcs = *assoc_funcs;
    } else {
        a->load_save_funcs.loadFunc = NULL;
        a->load_save_funcs.saveFunc = NULL;
        a->load_save_funcs.loadFuncDB = NULL;
        a->load_save_funcs.saveFuncDB = NULL;
    }

    if (a->max) {
        a->list = internal_malloc(sizeof(assoc_pair) * n);

        if (!a->list) {
            result = -1;
        }

    } else {
        a->list = NULL;
    }

    if (result != -1) {
        a->init_flag = 0xFEBAFEBA;
    }

    return result;
}

int assoc_resize(assoc_array* a, int n) {
    int result;
    assoc_pair* new_list;

    if ((a->init_flag == 0xFEBAFEBA) && (n >= a->size) &&
        ((new_list = internal_realloc(a->list, sizeof(assoc_pair) * n)) != 0)) {
        a->list = new_list;
        a->max = n;

        result = 0;
    } else {
        result = -1;
    }

    return result;
}

int assoc_free(assoc_array* a) {
    int result;
    int i;

    if (a->init_flag == 0xFEBAFEBA) {
        for (i = 0; i < a->size; i++) {
            if (a->list[i].name) {
                internal_free(a->list[i].name);
            }

            if (a->list[i].data) {
                internal_free(a->list[i].data);
            }
        }

        if (a->list) {
            internal_free(a->list);
        }

        memset(a, 0, sizeof(assoc_array));

        result = 0;
    } else {
        result = -1;
    }

    return result;
}

int assoc_search(assoc_array* a, char* name) {
    int result;
    int position;

    if ((a->init_flag != 0xFEBAFEBA) || assoc_find(a, name, &position)) {
        result = -1;
    } else {
        result = position;
    }

    return result;
}

int assoc_insert(assoc_array* a, char* name, void* data) {
    void* tdata;
    int i;
    int position;
    char* tname;

    if ((a->init_flag != 0xFEBAFEBA) || !assoc_find(a, name, &position)) {
        return -1;
    }

    if ((a->size == a->max) && assoc_resize(a, 2 * (a->max + 1)) == -1) {
        return -1;
    }

    tname = internal_malloc(strlen(name) + 1);

    if (!tname) {
        return -1;
    }

    strcpy(tname, name);

    if (data && a->datasize) {
        tdata = internal_malloc(a->datasize);

        if (!tdata) {
            internal_free(tname);

            return -1;
        }
    } else {
        tdata = NULL;
    }

    if (tdata && a->datasize) {
        memcpy(tdata, data, a->datasize);
    }

    for (i = a->size; i > position; i--) {
        memcpy(&a->list[i], &a->list[i - 1], sizeof(assoc_array));
    }

    a->list[position].name = tname;
    a->list[position].data = tdata;

    a->size++;

    return 0;
}

int assoc_delete(assoc_array* a, char* name) {
    int result;
    int position;
    int i;

    if (a->init_flag != 0xFEBAFEBA || assoc_find(a, name, &position) == -1) {
        result = -1;
    } else {
        internal_free(a->list[position].name);

        if (a->list[position].data) {
            internal_free(a->list[position].data);
        }

        a->size--;

        for (i = position; i < a->size; i++) {
            memcpy(&a->list[i], &a->list[i + 1], sizeof(assoc_array));
        }

        result = 0;
    }

    return result;
}

int assoc_copy(assoc_array* dst, assoc_array* src) {
    int result;
    int i;

    if (src->init_flag == 0xFEBAFEBA) {
        if (!assoc_init(dst, src->max, src->datasize, &src->load_save_funcs)) {
            for (i = 0; i < src->size; i++) {
                if (assoc_insert(dst, src->list[i].name, src->list[i].data) == -1) {
                    return -1;
                }
            }
        }

        result = 0;
    } else {
        result = -1;
    }

    return result;
}

int assoc_find_first(assoc_array* a, char* name, int find_num) {
    int result;
    size_t find_len;
    char* str;

    if (a->init_flag == 0xFEBAFEBA) {
        if (find_num != -1) {
            find_num++;
        } else {
            assoc_find(a, name, &find_num);
        }

        if (find_num < a->size) {
            find_len = strlen(name);
            str = a->list[find_num].name;
            if (!strnicmp(a->list[find_num].name, name, find_len)) {
                if ((!str[find_len] || (str[find_len] == ' ' && str[find_len + 1] == '(') ||
                     (str[find_len] == ' ' && str[find_len + 1] == '[') ||
                     (str[find_len] == '/' && str[find_len + 1] == 'T' && str[find_len + 2] == 'L'))) {
                    result = find_num;
                } else {
                    result = -1;
                }
            } else {
                result = -1;
            }
        } else {
            result = -1;
        }

    } else {
        result = -1;
    }

    return result;
}

int assoc_load(FILE* fp, assoc_array* a, int flags) {
    int len;
    int i;

    if (a->init_flag != 0xFEBAFEBA) {
        return -1;
    }

    for (i = 0; i < a->size; i++) {
        if (a->list[i].name) {
            internal_free(a->list[i].name);
        }

        if (a->list[i].data) {
            internal_free(a->list[i].data);
        }
    }

    if (a->list) {
        internal_free(a->list);
    }

    if (assoc_read_assoc_array(fp, a)) {
        return -1;
    }

    a->list = NULL;

    if (a->max <= 0) {
        return 0;
    }

    a->list = internal_malloc(sizeof(assoc_array) * a->max);

    if (a->list) {
        for (i = 0; i < a->size; i++) {
            a->list[i].name = NULL;
            a->list[i].data = NULL;
        }

        for (i = 0; i < a->size; i++) {
            len = fgetc(fp);

            if (len == -1) {
                return -1;
            }

            len++;

            a->list[i].name = internal_malloc(len);

            if (!a->list[i].name) {
                return -1;
            }

            if (!fgets(a->list[i].name, len, fp)) {
                return -1;
            }

            if (a->datasize) {
                a->list[i].data = internal_malloc(a->datasize);
                if (!a->list[i].data) {
                    return -1;
                }

                if (a->load_save_funcs.loadFunc) {
                    if (a->load_save_funcs.loadFunc(fp, a->list[i].data, a->datasize, flags)) {
                        return -1;
                    }

                } else if (fread(a->list[i].data, a->datasize, 1, fp) != 1) {
                    return -1;
                }
            }
        }

        return 0;
    }

    return -1;
}

int assoc_save(FILE* fp, assoc_array* a, int flags) {
    int result;
    size_t len;
    int i;

    if (a->init_flag != 0xFEBAFEBA || assoc_write_assoc_array(fp, a)) {
        result = -1;
    } else {
        for (i = 0; i < a->size; i++) {
            len = strlen(a->list[i].name);

            if (fputc(len, fp) == -1) {
                return -1;
            }

            if (fputs(a->list[i].name, fp) == -1) {
                return -1;
            }

            if (a->load_save_funcs.saveFunc) {
                if (a->datasize && (a->load_save_funcs.saveFunc)(fp, a->list[i].data, a->datasize, flags)) {
                    return -1;
                }
            } else if (a->datasize && fwrite(a->list[i].data, a->datasize, 1, fp) != 1) {
                return -1;
            }
        }

        result = 0;
    }

    return result;
}

void assoc_register_mem(assoc_malloc_func malloc_func, assoc_realloc_func realloc_func, assoc_free_func free_func) {
    if (malloc_func && realloc_func && free_func) {
        internal_malloc = malloc_func;
        internal_realloc = realloc_func;
        internal_free = free_func;
    } else {
        internal_malloc = default_malloc;
        internal_realloc = default_realloc;
        internal_free = default_free;
    }
}
