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

#include <cstdio>

#include "enums.hpp"
#include "gnw.h"

extern "C" {

extern char ResourceManager_FilePathCd[PATH_MAX];
extern char ResourceManager_FilePathGameInstall[PATH_MAX];
extern char ResourceManager_FilePathGameResFile[PATH_MAX];
extern char ResourceManager_FilePathMovie[PATH_MAX];
extern char ResourceManager_FilePathText[PATH_MAX];
extern char ResourceManager_FilePathFlc[PATH_MAX];
extern char ResourceManager_FilePathVoiceSpw[PATH_MAX];
extern char ResourceManager_FilePathSfxSpw[PATH_MAX];
extern char ResourceManager_FilePathMsc[PATH_MAX];

extern bool ResourceManager_IsMaxDdInUse;
extern bool ResourceManager_DisableEnhancedGraphics;

extern ColorIndex *ResourceManager_TeamRedColorIndexTable;
extern ColorIndex *ResourceManager_TeamGreenColorIndexTable;
extern ColorIndex *ResourceManager_TeamBlueColorIndexTable;
extern ColorIndex *ResourceManager_TeamGrayColorIndexTable;
extern ColorIndex *ResourceManager_TeamDerelictColorIndexTable;
extern ColorIndex *dword_1770B4;
extern ColorIndex *dword_1770B8;
extern ColorIndex *dword_1770BC;
extern ColorIndex *dword_1770C4;
extern ColorIndex *dword_1770C0;
extern ColorIndex *dword_1770C8;
extern ColorIndex *dword_1770CC;
extern ColorIndex *dword_17945C;

extern char *ResourceManager_MinimapFov;
extern char *ResourceManager_Minimap;
extern char *ResourceManager_Minimap2x;

void ResourceManager_InitPaths(int argc, char *argv[]);
void ResourceManager_InitResources();
void ResourceManager_ExitGame(int error_code);
ColorIndex ResourceManager_FindClosestPaletteColor(Color r, Color g, Color b, bool full_scan);
unsigned char *ResourceManager_ReadResource(ResourceID id);
unsigned char *ResourceManager_LoadResource(ResourceID id);
unsigned int ResourceManager_GetResourceSize(ResourceID id);
int ResourceManager_ReadImageHeader(ResourceID id, struct ImageBigHeader *buffer);
int ResourceManager_GetResourceFileID(ResourceID id);
const char *ResourceManager_GetResourceID(ResourceID id);
FILE *ResourceManager_GetFileHandle(ResourceID id);

char *ResourceManager_ToUpperCase(char *cstr);
}

#endif /* RESOURCE_MANAGER_HPP */
