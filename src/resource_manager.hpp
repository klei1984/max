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

#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include "enums.hpp"

struct SpriteHeader {
    short ulx;
    short uly;
    short width;
    short height;
    char data[];
};

struct SpriteMeta {
    short ulx;
    short uly;
    short width;
    short height;
    unsigned char palette[768];
    char data[];
};

unsigned char ResourceManager_Init();
char *ResourceManager_LoadResource(ResourceID id);
unsigned int ResourceManager_GetResourceSize(ResourceID id);
int ResourceManager_ReadImageHeader(ResourceID id, struct SpriteMeta *buffer);

int ResourceManager_GetResourceFileID(ResourceID id);
const char *ResourceManager_GetResourceID(ResourceID id);

#endif /* RESOURCE_MANAGER_HPP */
