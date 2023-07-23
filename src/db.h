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

#ifndef DB_H
#define DB_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct db_file_s* DB_FILE;
typedef struct dir_entry_s* dir_entry;
typedef struct db_s* db_handle;

typedef void* (*db_malloc_func)(size_t);
typedef char* (*db_strdup_func)(const char*);
typedef void (*db_free_func)(void*);
typedef void (*db_read_callback)(void);

db_handle db_init(char* datafile, char* datafile_path, char* patches_path);
int32_t db_select(db_handle db);
db_handle db_current(void);
int32_t db_total(void);
int32_t db_close(db_handle db);
void db_exit(void);
int32_t db_dir_entry(char* filename, dir_entry de);
int32_t db_load(char* filename, uint32_t* comp_size, uint32_t* uncomp_size, char** data);
DB_FILE db_fopen(char* filename, char* mode);
size_t db_fread(void* ptr, size_t size, size_t nmemb, DB_FILE stream);
int32_t db_fgetc(DB_FILE stream);
int32_t db_ungetc(int32_t c, DB_FILE stream);
char* db_fgets(char* s, int32_t n, DB_FILE stream);
int32_t db_fseek(DB_FILE stream, int32_t offset, int32_t whence);
int32_t db_ftell(DB_FILE stream);
void db_rewind(DB_FILE stream);
size_t db_fwrite(void* ptr, size_t size, size_t nmemb, DB_FILE stream);
int32_t db_fputc(int32_t c, DB_FILE stream);
int32_t db_fputs(char* s, DB_FILE stream);
int32_t db_freadByte(DB_FILE stream, uint8_t* c);
int32_t db_freadShort(DB_FILE stream, uint16_t* s);
int32_t db_freadInt(DB_FILE stream, int32_t* i);
int32_t db_freadFloat(DB_FILE stream, float* q);
int32_t db_fwriteByte(DB_FILE stream, uint8_t c);
int32_t db_fwriteShort(DB_FILE stream, uint16_t s);
int32_t db_fwriteInt(DB_FILE stream, int32_t i);
int32_t db_fwriteLong(DB_FILE stream, uint32_t l);
int32_t db_fwriteFloat(DB_FILE stream, float q);
int32_t db_freadByteCount(DB_FILE stream, uint8_t* c, int32_t num);
int32_t db_freadShortCount(DB_FILE stream, uint16_t* s, int32_t num);
int32_t db_freadIntCount(DB_FILE stream, int32_t* i, int32_t num);
int32_t db_freadLongCount(DB_FILE stream, uint32_t* l, int32_t num);
int32_t db_freadFloatCount(DB_FILE stream, float* q, int32_t num);
int32_t db_fwriteByteCount(DB_FILE stream, uint8_t* c, int32_t num);
int32_t db_fwriteShortCount(DB_FILE stream, uint16_t* s, int32_t num);
int32_t db_fwriteIntCount(DB_FILE stream, int32_t* i, int32_t num);
int32_t db_fwriteLongCount(DB_FILE stream, uint32_t* l, int32_t num);
int32_t db_fwriteFloatCount(DB_FILE stream, float* q, int32_t num);
int32_t db_fprintf(DB_FILE stream, char* format);
int32_t db_feof(DB_FILE stream);
int32_t db_get_file_list(char* filespec, char*** filelist, char*** desclist, int32_t desclen);
void db_free_file_list(char*** file_list, char*** desclist);
int32_t db_assoc_load_dir_entry(FILE* fp, void*, size_t, int32_t);
int32_t db_assoc_save_dir_entry(FILE* fp, void*, size_t, int32_t);
// db_assoc_load_db_dir_entry_
// db_assoc_save_db_dir_entry_
int32_t db_get_dir_list(char* parent_dir, char*** dir_list);
void db_free_dir_list(char*** dir_list);
int32_t db_filelength(DB_FILE stream);
void db_register_mem(db_malloc_func malloc_func, db_strdup_func strdup_func, db_free_func free_func);
void db_register_callback(db_read_callback callback, uint32_t threshold);
int32_t db_fclose(DB_FILE stream);

#endif /* DB_H */
