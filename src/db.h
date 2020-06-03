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

typedef void* DB_FILE;

typedef struct dir_entry_s {
    long length;
} dir_entry;

typedef void* (*db_malloc_func)(size_t);
typedef char* (*db_strdup_func)(char*);
typedef void (*db_free_func)(void*);
typedef void (*db_read_callback)(void);

int db_init(char* datafile, char* datafile_path, char* patches_path, int show_cursor);
int db_select(int db_handle);
int db_current(void);
int db_total(void);
int db_close(int db_handle);
void db_exit(void);
int db_dir_entry(char* filename, dir_entry* de);
int db_load(char* filename, unsigned long* comp_size, unsigned long* uncomp_size, char** data);
int db_read_to_buf(char* filename, unsigned char* buf);
// int db_fclose(DB_FILE* stream);
// DB_FILE* db_fopen(char* filename, char* mode);
int db_fprintf(DB_FILE* stream, char* format);
int db_fscanf(DB_FILE* stream, char* format);
int db_vfprintf(DB_FILE* stream, char* format, char** arg);
int db_fgetc(DB_FILE* stream);
char* db_fgets(char* s, int n, DB_FILE* stream);
int db_fputc(int c, DB_FILE* stream);
int db_fputs(char* s, DB_FILE* stream);
int db_ungetc(int c, DB_FILE* stream);
// size_t db_fread(void* ptr, size_t size, size_t nmemb, DB_FILE* stream);
size_t db_fwrite(void* ptr, size_t size, size_t nmemb, DB_FILE* stream);
int db_fseek(DB_FILE* stream, long offset, int whence);
long db_ftell(DB_FILE* stream);
void db_rewind(DB_FILE* stream);
int db_feof(DB_FILE* stream);
int db_freadByte(DB_FILE* stream, unsigned char* c);
// int db_freadShort(DB_FILE* stream, unsigned short* s);
// int db_freadInt(DB_FILE* stream, int* i);
int db_freadLong(DB_FILE* stream, unsigned long* l);
int db_freadFloat(DB_FILE* stream, float* q);
int db_fwriteByte(DB_FILE* stream, unsigned char c);
// int db_fwriteShort(DB_FILE* stream, unsigned short s);
int db_fwriteInt(DB_FILE* stream, int i);
// int db_fwriteLong(DB_FILE* stream, unsigned long l);
int db_fwriteFloat(DB_FILE* stream, float q);
int db_freadByteCount(DB_FILE* stream, unsigned char* c, int num);
int db_freadShortCount(DB_FILE* stream, unsigned short* s, int num);
int db_freadIntCount(DB_FILE* stream, int* i, int num);
int db_freadLongCount(DB_FILE* stream, unsigned long* l, int num);
int db_freadFloatCount(DB_FILE* stream, float* q, int num);
int db_fwriteByteCount(DB_FILE* stream, unsigned char* c, int num);
int db_fwriteShortCount(DB_FILE* stream, unsigned short* s, int num);
int db_fwriteIntCount(DB_FILE* stream, int* i, int num);
int db_fwriteLongCount(DB_FILE* stream, unsigned long* l, int num);
int db_fwriteFloatCount(DB_FILE* stream, float* q, int num);
int db_get_file_list(char* filespec, char*** filelist, char*** desclist, int desclen);
void db_free_file_list(char*** file_list, char*** desclist);
int db_get_dir_list(char* parent_dir, char*** dir_list);
void db_free_dir_list(char*** dir_list);
long db_filelength(DB_FILE* stream);
void db_register_mem(db_malloc_func* malloc_func, db_strdup_func* strdup_func, db_free_func* free_func);
void db_register_callback(db_read_callback* callback, unsigned int threshold);
void db_enable_hash_table(void);
int db_reset_hash_tables(void);
int db_add_hash_entry(char* filename, char path_marker);

#endif /* DB_H */
