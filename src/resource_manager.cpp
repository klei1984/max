/* Copyright (c) 2021 M.A.X. Port Team
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

#include "resource_manager.hpp"

extern "C" {
extern char *read_game_resource(ResourceID id);
extern unsigned int get_resource_data_size(ResourceID id);
extern int read_game_resource_into_buffer(ResourceID id, void *buffer);
const char *res_get_resource_id_string(ResourceID id);
short res_get_resource_file_index(ResourceID id);
unsigned char init_game_resources(void);
FILE *get_file_ptr_to_resource(ResourceID id);
}

unsigned char ResourceManager_Init() { return init_game_resources(); }

char *ResourceManager_LoadResource(ResourceID id) { return read_game_resource(id); }

unsigned int ResourceManager_GetResourceSize(ResourceID id) { return get_resource_data_size(id); }

int ResourceManager_ReadImageHeader(ResourceID id, struct SpriteMeta *buffer) {
    return read_game_resource_into_buffer(id, buffer);
}

int ResourceManager_GetResourceFileID(ResourceID id) { return res_get_resource_file_index(id); }

const char *ResourceManager_GetResourceID(ResourceID id) { return res_get_resource_id_string(id); }

FILE *ResourceManager_GetFileHandle(ResourceID id) { return get_file_ptr_to_resource(id); }
