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
#include <string>

#include "enums.hpp"
#include "gnw.h"
#include "point.hpp"

#define RESOURCE_MANAGER_MAP_TILE_SIZE 64

struct __attribute__((packed)) ImageSimpleHeader {
    short width;
    short height;
    short ulx;
    short uly;
    unsigned char transparent_color;
};

static_assert(sizeof(struct ImageSimpleHeader) == 9, "The structure needs to be packed.");

struct __attribute__((packed)) ImageBigHeader {
    short ulx;
    short uly;
    short width;
    short height;
    Color palette[3 * PALETTE_SIZE];
    unsigned char transparent_color;
};

static_assert(sizeof(struct ImageBigHeader) == 777, "The structure needs to be packed.");

struct ImageMultiFrameHeader {
    short width;
    short height;
    short hotx;
    short hoty;
    unsigned int *rows;
};

struct ImageMultiHeader {
    unsigned short image_count;
    struct ImageMultiFrameHeader *frames[];
};

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
extern ColorIndex *ResourceManager_ColorIndexTable06;
extern ColorIndex *ResourceManager_ColorIndexTable07;
extern ColorIndex *ResourceManager_ColorIndexTable08;
extern ColorIndex *ResourceManager_ColorIndexTable09;
extern ColorIndex *ResourceManager_ColorIndexTable10;
extern ColorIndex *ResourceManager_ColorIndexTable11;
extern ColorIndex *ResourceManager_ColorIndexTable12;
extern ColorIndex *ResourceManager_ColorIndexTable13x8;

extern unsigned char *ResourceManager_MinimapFov;
extern unsigned char *ResourceManager_Minimap;
extern unsigned char *ResourceManager_Minimap2x;

extern unsigned short *ResourceManager_MapTileIds;
extern unsigned char *ResourceManager_MapTileBuffer;
extern unsigned char *ResourceManager_MapSurfaceMap;
extern unsigned short *ResourceManager_CargoMap;

extern Point ResourceManager_MapSize;

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
void ResourceManager_Realloc(ResourceID id, unsigned char *buffer, int data_size);
FILE *ResourceManager_GetFileHandle(ResourceID id);
void ResourceManager_InitInGameAssets(int world);
const char *ResourceManager_ToUpperCase(char *cstr);
const char *ResourceManager_ToUpperCase(std::string &string);
void ResourceManager_FreeResources();
void ResourceManager_InitClanUnitValues(unsigned short team);
void ResourceManager_InitHeatMaps(unsigned short team);
void ResourceManager_InitTeamInfo();
unsigned char *ResourceManager_GetBuffer(ResourceID id);

#endif /* RESOURCE_MANAGER_HPP */
