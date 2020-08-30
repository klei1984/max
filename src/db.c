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

#include "db.h"

#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __unix__
#include <linux/limits.h>
#endif

#include "game.h"

#define GNW_DB_DATABASE_LIST_SIZE 10
#define GNW_DB_FILE_LIST_SIZE 10
#define GNW_DB_READ_THRESHOLD 16384

struct dir_entry_s {
    long flags;
    int offset;
    int uncomp_size;
    int comp_size;
};

static_assert(sizeof(struct dir_entry_s) == 16, "The structure needs to be packed.");

struct db_file_s {
    db_handle db;
    int flags;
    char in_use;
    char field_9[3];
    int field_12;
    int field_16;
    int field_20;
    long int file_position;
    char* buffer_position;
    char* buffer_end;
};

static_assert(sizeof(struct db_file_s) == 36, "The structure needs to be packed.");

typedef struct db_file_s db_file;

struct db_s {
    char* datafile;
    FILE* datafile_handle;
    char* datafile_path;
    char* patches_path;
    char is_patch_path_allocated;
    char field_17[3];
    assoc_array assoc_a;
    assoc_array* assoc_list;
    int db_file_count;
    db_file db_file_list[GNW_DB_FILE_LIST_SIZE];
};

static_assert(sizeof(struct db_s) == 424, "The structure needs to be packed.");

static int db_create_database(db_handle* db);
static int db_destroy_database(db_handle* db);
static int db_init_database(db_handle handle, char* datafile, char* datafile_path);
static void db_exit_database(db_handle db);
static int db_init_patches(db_handle db, char* patches_path);
static void db_exit_patches(db_handle db);
static DB_FILE db_add_fp_rec(FILE* fp, char* buffer, size_t size, int flags);
static int db_find_empty_position(int* index);
static int db_find_dir_entry(char* filename, dir_entry de);
static inline void* internal_malloc(size_t size);
static inline char* internal_strdup(const char* s);
static inline void internal_free(void* ptr);
static void* db_default_malloc(size_t size);
static char* db_default_strdup(const char* s);
static void db_default_free(void* ptr);
static void db_preload_buffer(DB_FILE stream);
static int db_fread_short(FILE* fp, unsigned short* s);
static int db_fread_long(FILE* fp, unsigned long* l);
static int db_fwrite_long(FILE* fp, unsigned long l);

static int db_used_malloc;

static char* empty_string = "\0";

static db_malloc_func malloc_function = db_default_malloc;
static db_strdup_func strdup_function = db_default_strdup;
static db_free_func free_function = db_default_free;

static db_read_callback read_callback;
static unsigned int read_threshold = GNW_DB_READ_THRESHOLD;
static unsigned int read_count;

db_handle database_list[GNW_DB_DATABASE_LIST_SIZE];
db_handle current_database;

db_handle db_init(char* datafile, char* datafile_path, char* patches_path) {
    db_handle db;

    if (db_create_database(&db)) {
        return NULL;
    }

    if (db_init_database(db, datafile, datafile_path) || db_init_patches(db, patches_path)) {
        db_close(db);

        return NULL;
    }

    if (!current_database) {
        current_database = db;
    }

    return db;
}

int db_select(db_handle db) {
    for (int i = 0; i < GNW_DB_DATABASE_LIST_SIZE; i++) {
        if (db == database_list[i]) {
            current_database = db;
            return 0;
        }
    }

    return -1;
}

db_handle db_current(void) { return current_database; }

int db_total(void) {
    int i;

    for (i = 0; (i < GNW_DB_DATABASE_LIST_SIZE) && database_list[i]; i++) {
        ;
    }

    return i;
}

int db_close(db_handle db) {
    int i;

    if (!db || db == (db_handle)-1) {
        return -1;
    }

    for (i = 0; (i < GNW_DB_DATABASE_LIST_SIZE) && (db != database_list[i]); i++) {
        ;
    }

    if (i >= GNW_DB_DATABASE_LIST_SIZE) {
        return -1;
    }

    if (db == current_database) {
        current_database = NULL;
    }

    db_exit_database(database_list[i]);
    db_exit_patches(database_list[i]);

    if (database_list[i]) {
        if (database_list[i]->datafile) {
            free_function(database_list[i]->datafile);
            database_list[i]->datafile = NULL;
        }
    }

    return 0;
}

void db_exit(void) {
    for (int i = 0; i < GNW_DB_DATABASE_LIST_SIZE; i++) {
        if (database_list[i]) {
            db_close(database_list[i]);
        }
    }
}

int db_dir_entry(char* filename, dir_entry de) {
    char path[PATH_MAX];
    FILE* fp;
    int is_normal_file;
    int result;

    if (!current_database || !filename || !de) {
        return -1;
    }

    is_normal_file = 1;
    if (filename[0] == '@') {
        strcpy(path, &filename[1]);
        is_normal_file = 0;
    }

    if (current_database->patches_path) {
        if (is_normal_file) {
            sprintf(path, "%s%s", current_database->patches_path, filename);
        }

        fp = fopen(path, "rb");
        if (fp) {
            de->flags = 4;
            de->offset = 0;
            de->uncomp_size = filesize(fp);
            de->comp_size = 0;
            fclose(fp);

            return 0;
        }
    }

    if (!current_database->datafile) {
        return -1;
    }

    if (is_normal_file) {
        sprintf(path, "%s%s", current_database->datafile_path, filename);
    }

    strupr(path);

    if (!db_find_dir_entry(path, de)) {
        if (!de->flags) {
            de->flags = sizeof(struct dir_entry_s);
        }

        result = 0;
        de->flags |= 8;
    } else {
        result = -1;
    }

    return result;
}

int db_load(char* filename, unsigned long* comp_size, unsigned long* uncomp_size, char** data) {
    char path[PATH_MAX];
    struct dir_entry_s de;
    FILE* fp;
    int file_size;
    char* buffer;
    int is_normal_file;
    int result;

    if (!current_database || !filename || !comp_size || !uncomp_size || !data) {
        return -1;
    }

    is_normal_file = 1;
    if (filename[0] == '@') {
        strcpy(path, &filename[1]);
        is_normal_file = 0;
    }

    if (current_database->patches_path) {
        if (is_normal_file) {
            sprintf(path, "%s%s", current_database->patches_path, filename);
        }

        fp = fopen(path, "rb");
        if (fp) {
            *comp_size = 0;
            file_size = filesize(fp);
            *uncomp_size = file_size;

            if (file_size) {
                buffer = internal_malloc(file_size);
                *data = buffer;

                if (buffer) {
                    int read_bytes;

                    if (file_size == fread(buffer, 1, file_size, fp)) {
                        fclose(fp);

                        return 0;
                    }

                    internal_free(buffer);
                    *data = NULL;
                }

                *uncomp_size = 0;
            }

            fclose(fp);

            return -1;
        }
    }

    if (current_database->datafile) {
        if (is_normal_file) {
            sprintf(path, "%s%s", current_database->datafile_path, filename);
        }

        strupr(path);

        if (!db_find_dir_entry(path, &de) && current_database->datafile_handle &&
            !fseek(current_database->datafile_handle, de.offset, SEEK_SET)) {
            *comp_size = de.comp_size;
            *uncomp_size = de.uncomp_size;

            buffer = internal_malloc(de.comp_size);
            *data = buffer;

            if (buffer) {
                if (de.comp_size == fread(buffer, 1, de.comp_size, current_database->datafile_handle)) {
                    return 0;
                }

                internal_free(buffer);
                *data = NULL;
            }

            *comp_size = 0;
            *uncomp_size = 0;
        }
    }

    return -1;
}

DB_FILE db_fopen(char* filename, char* mode) {
    char path[PATH_MAX];
    int is_normal_file;
    FILE* fp = NULL;
    struct dir_entry_s de;
    char* buffer;
    int op_mode;

    if (!current_database || !filename || !mode || current_database->db_file_count >= GNW_DB_FILE_LIST_SIZE) {
        return NULL;
    }

    op_mode = -1;

    for (int i = 0; mode[i]; i++) {
        switch (mode[i]) {
            case '+':
                op_mode = 0;
                break;
            case 'a':
                op_mode = 0;
                break;
            case 'b':
                break;
            case 'r':
                op_mode = 1;
                break;
            case 't':
                break;
            case 'w':
                op_mode = 0;
                break;
            default:
                break;
        }
    }

    if (op_mode == -1) {
        return NULL;
    }

    is_normal_file = 1;

    if (*filename == '@') {
        strcpy(path, &filename[1]);
        is_normal_file = 0;
    }

    if (current_database->patches_path) {
        if (is_normal_file) {
            sprintf(path, "%s%s", current_database->patches_path, filename);
        }

        fp = fopen(path, mode);
        if (fp) {
            return db_add_fp_rec(fp, NULL, 0, 0x04);
        }
    }

    if (op_mode && current_database->datafile) {
        if (is_normal_file) {
            sprintf(path, "%s%s", current_database->datafile_path, filename);
        }

        strupr(path);

        if (db_find_dir_entry(path, &de) != -1 && current_database->datafile_handle &&
            !fseek(current_database->datafile_handle, de.offset, SEEK_SET)) {
            if (!de.flags) {
                de.flags = 0x10;
            }

            if ((de.flags & 0xF0) < 0x20) {
                if ((de.flags & 0xF0) == 0x10) {
                    buffer = internal_malloc(de.uncomp_size);
                    if (buffer) {
                        lzss_decode_to_buf(current_database->datafile_handle, buffer, de.comp_size);

                        return db_add_fp_rec(NULL, buffer, de.uncomp_size, 0x00);
                    }
                }
            } else {
                if ((de.flags & 0xF0) <= 0x20) {
                    return db_add_fp_rec(current_database->datafile_handle, NULL, de.uncomp_size, 0x28);
                }

                if ((de.flags & 0xF0) == 0x40) {
                    buffer = internal_malloc(GNW_DB_READ_THRESHOLD);

                    if (buffer) {
                        return db_add_fp_rec(current_database->datafile_handle, buffer, de.uncomp_size, 0x48);
                    }
                }
            }
        }
    }

    return NULL;
}

size_t db_fread(void* ptr, size_t size, size_t nmemb, DB_FILE stream) {
    if (!stream) {
        return 0;
    }

    if (stream->flags & 4) {
        if (read_callback) {
            SDL_assert(0 /** \todo implement rest of unused code */);
        }

        return fread(ptr, size, nmemb, (FILE*)stream->field_12);
    } else {
        SDL_assert(0 /** \todo implement rest of unused code */);
    }

    return 0;
}

int db_fgetc(DB_FILE stream) {
    int result;

    result = -1;

    if (!stream) {
        if (read_callback) {
            read_count++;
            if (read_count >= read_threshold) {
                read_count = 0;
                read_callback();
            }
        }

        return result;
    }

    if (stream->flags & 4) {
        result = fgetc((FILE*)stream->field_12);
        if (read_callback) {
            read_count++;
            if (read_count >= read_threshold) {
                read_count = 0;
                read_callback();
            }
        }

        return result;
    }

    SDL_assert(0 /** \todo implement rest of unused code */);

    return result;
}

int db_ungetc(int c, DB_FILE stream) {
    int result;

    result = c;

    if (!stream) {
        return result;
    }

    if (stream->flags & 4) {
        result = ungetc(c, (FILE*)stream->field_12);
    } else {
        if ((stream->flags & 0xF0) < 0x20) {
            if ((stream->flags & 0xF0) != 0x10) {
                return result;
            }

        } else {
            if ((stream->flags & 0xF0) <= 0x20) {
                if (stream->file_position != stream->field_20 &&
                    !fseek(stream->db->datafile_handle, stream->file_position, SEEK_SET) &&
                    !fseek(stream->db->datafile_handle, -1, SEEK_CUR)) {
                    stream->file_position = ftell(stream->db->datafile_handle);
                    stream->field_16++;
                }

                return result;
            }

            if ((stream->flags & 0xF0) != 0x40) {
                return result;
            }
        }

        if (stream->buffer_end != stream->buffer_position) {
            stream->buffer_end--;
            stream->field_16++;

            return result;
        }
    }

    return result;
}

char* db_fgets(char* s, int n, DB_FILE stream) {
    char* result;

    result = NULL;

    if (stream && n) {
        if (stream->flags & 4) {
            result = fgets(s, n, (FILE*)stream->field_12);
        } else if (s) {
            int i;
            int c;

            for (i = 0; i + 1 < n; i++) {
                c = db_fgetc(stream);
                if (c == -1) {
                    break;
                }

                s[i] = c;

                if (c == '\n') {
                    i++;
                    break;
                }
            }

            s[i] = '\0';
            if (i) {
                result = s;
            }
        }
    }

    return result;
}

long db_ftell(DB_FILE stream) {
    long result;

    result = -1;

    if (!stream) {
        return result;
    }

    if (stream->flags & 4) {
        return ftell((FILE*)stream->field_12);
    }

    if ((stream->flags & 0xF0) >= 0x20) {
        if ((stream->flags & 0xF0) > 0x20 && (stream->flags & 0xF0) != 0x40) {
            return -1;
        }

        result = stream->field_12 - stream->field_16;

        return result;
    }

    if ((stream->flags & 0xF0) == 0x10) {
        result = stream->field_12 - stream->field_16;
    }

    return result;
}

size_t db_fwrite(void* ptr, size_t size, size_t nmemb, DB_FILE stream) {
    size_t result;
    if (stream && (stream->flags & 4)) {
        result = fwrite(ptr, size, nmemb, (FILE*)stream->field_12);
    } else {
        result = nmemb - 1;
    }

    return result;
}

int db_fputc(int c, DB_FILE stream) {
    int result;

    if (stream && (stream->flags & 4)) {
        result = fputc(c, (FILE*)stream->field_12);
    } else {
        result = -1;
    }

    return result;
}

int db_fputs(char* s, DB_FILE stream) {
    int result;

    if (stream && (stream->flags & 4)) {
        result = fputs(s, (FILE*)stream->field_12);
    } else {
        result = -1;
    }

    return result;
}

int db_freadByte(DB_FILE stream, unsigned char* c) {
    int result;

    result = db_fgetc(stream);

    if (result != -1) {
        *c = result;
        result = 0;
    }

    return result;
}

int db_freadShort(DB_FILE stream, unsigned short* s) {
    int result;
    unsigned char c;

    if (db_freadByte((DB_FILE)stream, &c) == -1) {
        result = -1;
    } else {
        *s = c;
        if (db_freadByte(stream, &c) == -1) {
            result = -1;
        } else {
            *s <<= 8;
            *s |= c;
            result = 0;
        }
    }

    return result;
}

int db_freadInt(DB_FILE stream, int* i) {
    int result;
    unsigned short s;

    if (db_freadShort((DB_FILE)stream, &s) == -1) {
        result = -1;
    } else {
        *i = s;
        if (db_freadShort(stream, &s) == -1) {
            result = -1;
        } else {
            *i <<= 16;
            *i |= s;
            result = 0;
        }
    }

    return result;
}

int db_freadFloat(DB_FILE stream, float* q) {
    int result;
    int i;

    result = db_freadInt(stream, &i);
    if (result != -1) {
        result = 0;
        *q = (long double)(unsigned int)i;
    }

    return result;
}

int db_fwriteByte(DB_FILE stream, unsigned char c) {
    int result;

    if (stream && (stream->flags & 4)) {
        if (fputc(c, (FILE*)stream->field_12) != -1) {
            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int db_fwriteShort(DB_FILE stream, unsigned short s) {
    int result;

    if (db_fwriteByte(stream, s >> 8) != -1) {
        if (db_fwriteByte(stream, s) != -1) {
            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int db_fwriteInt(DB_FILE stream, int i) {
    int result;

    if (db_fwriteShort(stream, i >> 16) != -1) {
        if (db_fwriteShort(stream, i) != -1) {
            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int db_fwriteLong(DB_FILE stream, unsigned long l) {
    int result;

    if (db_fwriteShort(stream, l >> 16) != -1) {
        if (db_fwriteShort(stream, l) != -1) {
            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int db_fwriteFloat(DB_FILE stream, float q) { return db_fwriteInt(stream, (signed long long)q); }

int db_assoc_load_dir_entry(FILE* fp, void* dir_entry, size_t size, int unknown) {
    SDL_assert(0 /** \todo implement unused code */);
}

int db_assoc_save_dir_entry(FILE* fp, void* dir_entry, size_t size, int unknown) {
    SDL_assert(0 /** \todo implement unused code */);
}

long db_filelength(DB_FILE stream) {
    int result;

    result = -1;

    if (stream) {
        if (stream->flags & 4) {
            result = filesize((FILE*)stream->field_12);
        } else {
            result = stream->field_12;
        }
    }

    return result;
}

void db_register_mem(db_malloc_func malloc_func, db_strdup_func strdup_func, db_free_func free_func) {
    if (!db_used_malloc) {
        if (malloc_func && strdup_func && free_func) {
            malloc_function = malloc_func;
            strdup_function = strdup_func;
            free_function = free_func;
        } else {
            malloc_function = db_default_malloc;
            strdup_function = db_default_strdup;
            free_function = db_default_free;
        }
    }
}

void db_register_callback(db_read_callback callback, unsigned int threshold) {
    read_callback = callback;
    read_threshold = threshold;
    read_count = 0;
}

int db_create_database(db_handle* db) {
    int result;
    int i;

    for (i = 0; (i < GNW_DB_DATABASE_LIST_SIZE) && database_list[i]; i++) {
        ;
    }

    if (i < GNW_DB_DATABASE_LIST_SIZE) {
        database_list[i] = internal_malloc(sizeof(struct db_s));
        if (database_list[i]) {
            memset(database_list[i], 0, sizeof(struct db_s));
            *db = database_list[i];
            result = 0;
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }

    return result;
}

int db_destroy_database(db_handle* db) {
    int result;

    if (db && *db) {
        free_function(*db);
        *db = NULL;

        result = 0;
    } else {
        result = -1;
    }

    return result;
}

int db_init_database(db_handle db, char* datafile, char* datafile_path) {
    assoc_func_list assoc_funcs;
    int i;
    int j;
    char* path;
    int size;

    if (!db) {
        return -1;
    }

    if (!datafile) {
        return 0;
    }

    db->datafile = internal_strdup(datafile);

    if (!db->datafile) {
        return -1;
    }

    db->datafile_handle = fopen(db->datafile, "rb");

    if (!db->datafile_handle) {
        free_function(db->datafile);
        db->datafile = NULL;

        return -1;
    }

    if (assoc_init(&db->assoc_a, 0, sizeof(assoc_array), NULL) || assoc_load(db->datafile_handle, &db->assoc_a, 0)) {
        fclose(db->datafile_handle);
        free_function(db->datafile);
        db->datafile = NULL;

        return -1;
    }

    db->assoc_list = internal_malloc(sizeof(assoc_array) * db->assoc_a.size);

    if (!db->assoc_list) {
        assoc_free(&db->assoc_a);
        fclose(db->datafile_handle);
        free_function(db->datafile);
        db->datafile = NULL;

        return -1;
    }

    assoc_funcs.loadFunc = db_assoc_load_dir_entry;
    assoc_funcs.saveFunc = db_assoc_save_dir_entry;
    assoc_funcs.loadFuncDB = NULL;
    assoc_funcs.saveFuncDB = NULL;

    for (i = 0; i < db->assoc_a.size; i++) {
        if (assoc_init(&db->assoc_list[i], 0, sizeof(dir_entry), &assoc_funcs) ||
            assoc_load(db->datafile_handle, &db->assoc_list[i], 0)) {
            for (j = i; j >= 0; j--) {
                assoc_free(&db->assoc_list[j]);
            }

            break;
        }
    }

    if (i != db->assoc_a.size) {
        free_function(db->assoc_list);
        assoc_free(&db->assoc_a);
        fclose(db->datafile_handle);
        free_function(db->datafile);
        db->datafile = NULL;

        return -1;
    }

    if (datafile_path && strlen(datafile_path)) {
        if (datafile_path[0] == '\\') {
            path = &datafile_path[1];
        } else {
            path = ".\\";
        }
    }

    size = strlen(path);

    db->datafile_path = internal_malloc(size + 2);

    if (!db->datafile_path) {
        free_function(db->assoc_list);
        assoc_free(&db->assoc_a);
        fclose(db->datafile_handle);
        free_function(db->datafile);
        db->datafile = NULL;

        return -1;
    }

    strcpy(db->datafile_path, path);

    if (db->datafile_path[size - 2] != '\\') {
        db->datafile_path[size - 1] = '\\';
        db->datafile_path[size] = '\0';
    }

    return 0;
}

void db_exit_database(db_handle db) {}

int db_init_patches(db_handle db, char* patches_path) {
    int size;

    if (db) {
        if (!patches_path) {
            db->patches_path = NULL;

            return 0;
        }

        size = strlen(patches_path) + 1;

        if (size == 1) {
            db->patches_path = empty_string;

            return 0;
        }

        db->patches_path = internal_malloc(size + 1);

        if (db->patches_path) {
            strcpy(db->patches_path, patches_path);
            db->is_patch_path_allocated = 1;

            if (db->patches_path[size - 2] != '\\') {
                db->patches_path[size - 1] = '\\';
                db->patches_path[size] = '\0';
            }

            return 0;
        }
    }

    return -1;
}

void db_exit_patches(db_handle db) {
    if (db) {
        if (db->patches_path) {
            if (db->is_patch_path_allocated == 1) {
                free_function(db->patches_path);
            }
        }

        db->patches_path = empty_string;
        db->is_patch_path_allocated = 0;
    }
}

DB_FILE db_add_fp_rec(FILE* fp, char* buffer, size_t size, int flags) {
    DB_FILE result;
    int fp_rec_index;

    result = NULL;

    if (current_database->db_file_count < 10 && !db_find_empty_position(&fp_rec_index)) {
        memset(&current_database->db_file_list[fp_rec_index], 0, sizeof(db_file));

        current_database->db_file_list[fp_rec_index].db = current_database;

        if (flags & 4) {
            current_database->db_file_list[fp_rec_index].field_12 = (int)fp;

            result = (DB_FILE)&current_database->db_file_list[fp_rec_index].db;
        } else {
            current_database->db_file_list[fp_rec_index].field_12 = size;
            current_database->db_file_list[fp_rec_index].field_16 = size;

            if ((flags & 0xF0) >= 0x20) {
                if ((flags & 0xF0) <= 0x20) {
                    current_database->db_file_list[fp_rec_index].field_20 = ftell(fp);
                    current_database->db_file_list[fp_rec_index].file_position = ftell(fp);

                    result = (DB_FILE)&current_database->db_file_list[fp_rec_index].db;
                } else if ((flags & 0xF0) == 0x40) {
                    current_database->db_file_list[fp_rec_index].field_20 = ftell(fp);
                    current_database->db_file_list[fp_rec_index].file_position = ftell(fp);

                    current_database->db_file_list[fp_rec_index].buffer_position = buffer;
                    current_database->db_file_list[fp_rec_index].buffer_end = buffer + GNW_DB_READ_THRESHOLD;

                    result = (DB_FILE)&current_database->db_file_list[fp_rec_index].db;
                }

            } else if ((flags & 0xF0) == 0x10) {
                current_database->db_file_list[fp_rec_index].buffer_position = buffer;
                current_database->db_file_list[fp_rec_index].buffer_end = buffer;

                result = (DB_FILE)&current_database->db_file_list[fp_rec_index].db;
            }
        }
    }

    if (result) {
        current_database->db_file_list[fp_rec_index].flags = flags;
        current_database->db_file_list[fp_rec_index].in_use = 1;
        current_database->db_file_count++;
    }

    return result;
}

int db_fclose(DB_FILE stream) {
    if (stream) {
        if (stream->flags & 4) {
            fclose((FILE*)stream->field_12);
            stream->db->db_file_count--;
            memset(stream, 0, sizeof(struct db_file_s));

            return 0;
        }

        if ((stream->flags & 0xF0) < 0x20) {
            if ((stream->flags & 0xF0) != 0x10) {
                stream->db->db_file_count--;
                memset(stream, 0, sizeof(struct db_file_s));

                return 0;
            }
        } else if ((stream->flags & 0xF0) <= 0x20 || (stream->flags & 0xF0) != 0x40) {
            stream->db->db_file_count--;
            memset(stream, 0, sizeof(struct db_file_s));

            return 0;
        }

        if (stream->buffer_position) {
            free_function(stream->buffer_position);
        }

        stream->db->db_file_count--;
        memset(stream, 0, sizeof(struct db_file_s));

        return 0;
    }

    return -1;
}

int db_find_empty_position(int* index) {
    if (index && (current_database->db_file_count < 10)) {
        int i;

        for (i = 0; i < 10 && current_database->db_file_list[i].in_use; i++) {
            ;
        }

        if (i < 10) {
            *index = i;
            return 0;
        }
    }

    return -1;
}

int db_find_dir_entry(char* filename, dir_entry de) {
    int size;
    int i;
    int j;

    if (!current_database->datafile || !filename || !de) {
        return -1;
    }

    if (filename[0] == '.') {
        filename = &filename[1];

        if (filename[0] == '\\') {
            filename = &filename[1];
        }
    }

    for (size = strlen(filename) - 1; (size >= 0) && (filename[size] != '\\'); size--) {
        ;
    }

    if (size < 0) {
        i = 0;
    } else {
        filename[size] = '\0';
        i = assoc_search(&current_database->assoc_a, filename);
    }

    if (i == -1 || (j = assoc_search(&current_database->assoc_list[i], &filename[size + 1]), j == -1)) {
        if (size >= 0) {
            filename[size] = '\\';
        }

        return -1;
    }

    if (size >= 0) {
        filename[size] = '\\';
    }

    *de = *(dir_entry)current_database->assoc_list[i].list[j].data;

    return 0;
}

// db_findfirst_
// db_findnext_
// db_findclose_

void* internal_malloc(size_t size) {
    db_used_malloc = 1;

    return malloc_function(size);
}

char* internal_strdup(const char* s) {
    db_used_malloc = 1;

    return strdup_function(s);
}

void internal_free(void* ptr) { free_function(ptr); }

void* db_default_malloc(size_t size) { return malloc(size); }

char* db_default_strdup(const char* s) {
    db_used_malloc = 1;

    return strdup(s);
}

void db_default_free(void* ptr) { free(ptr); }

void db_preload_buffer(DB_FILE stream) {
    unsigned short size;

    if ((stream->flags & 0x08) && ((stream->flags & 0xF0) == 0x40) && stream->field_16 &&
        (stream->buffer_position + GNW_DB_READ_THRESHOLD) <= stream->buffer_end &&
        !fseek(stream->db->datafile_handle, stream->file_position, SEEK_SET) &&
        !db_fread_short(stream->db->datafile_handle, &size)) {
        if (size & 0x8000) {
            size_t bytes_read;

            size &= 0x7FFF;

            bytes_read = fread((void*)stream->buffer_position, 1, size, stream->db->datafile_handle);
            SDL_assert(bytes_read == size);
        } else {
            lzss_decode_to_buf(stream->db->datafile_handle, stream->buffer_position, size);
        }

        stream->buffer_end = stream->buffer_position;
        stream->file_position = ftell(stream->db->datafile_handle);
    }
}

int db_fread_short(FILE* fp, unsigned short* s) {
    int c1;
    int c2;
    int result;

    c1 = fgetc(fp);
    c2 = fgetc(fp);

    if (c1 == -1 || c2 == -1) {
        result = -1;
    } else {
        *s = c2 | (c1 << 8);
        result = 0;
    }

    return result;
}
