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
#include <filesystem>
#include <string>
#include <vector>

#include "attributes.hpp"
#include "clans.hpp"
#include "enums.hpp"
#include "gnw.h"
#include "point.hpp"
#include "resourcetable.hpp"
#include "units.hpp"

class MissionManager;
class Settings;

struct __attribute__((packed)) ImageSimpleHeader {
    int16_t width;
    int16_t height;
    int16_t ulx;
    int16_t uly;
    uint8_t transparent_color;
};

static_assert(sizeof(struct ImageSimpleHeader) == 9, "The structure needs to be packed.");

struct __attribute__((packed)) ImageBigHeader {
    int16_t ulx;
    int16_t uly;
    int16_t width;
    int16_t height;
    Color palette[3 * PALETTE_SIZE];
    uint8_t transparent_color;
};

static_assert(sizeof(struct ImageBigHeader) == 777, "The structure needs to be packed.");

struct ImageMultiFrameHeader {
    int16_t width;
    int16_t height;
    int16_t hotx;
    int16_t hoty;
    uint32_t* rows;
};

struct ImageMultiHeader {
    uint16_t image_count;
    struct ImageMultiFrameHeader* frames[];
};

extern const uint8_t ResourceManager_GenericTable[32];

extern std::filesystem::path ResourceManager_FilePathGameData;
extern std::filesystem::path ResourceManager_FilePathGameBase;
extern std::filesystem::path ResourceManager_FilePathGamePref;

extern bool ResourceManager_DisableEnhancedGraphics;

extern ColorIndex* ResourceManager_TeamRedColorIndexTable;
extern ColorIndex* ResourceManager_TeamGreenColorIndexTable;
extern ColorIndex* ResourceManager_TeamBlueColorIndexTable;
extern ColorIndex* ResourceManager_TeamGrayColorIndexTable;
extern ColorIndex* ResourceManager_TeamDerelictColorIndexTable;
extern ColorIndex* ResourceManager_ColorIndexTable06;
extern ColorIndex* ResourceManager_ColorIndexTable07;
extern ColorIndex* ResourceManager_ColorIndexTable08;
extern ColorIndex* ResourceManager_ColorIndexTable09;
extern ColorIndex* ResourceManager_ColorIndexTable10;
extern ColorIndex* ResourceManager_ColorIndexTable11;
extern ColorIndex* ResourceManager_ColorIndexTable12;
extern ColorIndex* ResourceManager_ColorIndexTable13x8;

extern uint8_t* ResourceManager_Minimap;
extern uint8_t* ResourceManager_MinimapUnits;
extern uint8_t* ResourceManager_MinimapFov;
extern uint8_t* ResourceManager_MinimapBgImage;

extern uint16_t* ResourceManager_MapTileIds;
extern uint8_t* ResourceManager_MapTileBuffer;
extern uint8_t* ResourceManager_MapSurfaceMap;
extern uint16_t* ResourceManager_CargoMap;

extern Point ResourceManager_MapSize;
extern Point ResourceManager_MinimapWindowSize;
extern Point ResourceManager_MinimapWindowOffset;
extern double ResourceManager_MinimapWindowScale;
extern uint8_t* ResourceManager_MainmapBgImage;
extern int32_t ResourceManager_MainmapZoomLimit;

void ResourceManager_InitResources();
void ResourceManager_ExitGame(int32_t error_code);
void ResourceManager_Exit();
uint8_t* ResourceManager_ReadResource(ResourceID id);
uint8_t* ResourceManager_LoadResource(ResourceID id);
uint32_t ResourceManager_GetResourceSize(ResourceID id);
int32_t ResourceManager_ReadImageHeader(ResourceID id, struct ImageBigHeader* buffer);
int32_t ResourceManager_GetResourceFileID(ResourceID id);
const char* ResourceManager_GetResourceID(ResourceID id);
ResourceID ResourceManager_GetResourceID(std::string id);
void ResourceManager_Realloc(ResourceID id, uint8_t* buffer, int32_t data_size);
FILE* ResourceManager_GetFileHandle(ResourceID id);
FILE* ResourceManager_OpenFileResource(const std::string& string, const ResourceType type,
                                       const char* const mode = "rb", std::filesystem::path* path = nullptr);
FILE* ResourceManager_OpenFileResource(const ResourceID id, const ResourceType type, const char* const mode = "rb",
                                       std::filesystem::path* path = nullptr);
void ResourceManager_InitInGameAssets(int32_t world);
std::string ResourceManager_StringToUpperCase(const std::string& string);
std::string ResourceManager_StringToLowerCase(const std::string& string);
void ResourceManager_FreeResources();
void ResourceManager_InitClanUnitValues(uint16_t team);
void ResourceManager_InitHeatMaps(uint16_t team);
void ResourceManager_InitTeamInfo();
uint8_t* ResourceManager_GetBuffer(ResourceID id);
std::string ResourceManager_Sha256(const ResourceID world);
const std::string& ResourceManager_GetLanguageEntry(const uint32_t key);
std::string& ResourceManager_GetSystemLocale();
void ResourceManager_SetSystemLocale(const std::string locale);
std::string ResourceManager_GetHelpEntry(const std::string& section, const int32_t position_x,
                                         const int32_t position_y);
[[nodiscard]] bool ResourceManager_GetUnitAttributes(const uint32_t index, UnitAttributes* const attributes);
std::shared_ptr<MissionManager> ResourceManager_GetMissionManager();
std::shared_ptr<Clans> ResourceManager_GetClans();
std::shared_ptr<Settings> ResourceManager_GetSettings();
Unit& ResourceManager_GetUnit(const ResourceID unit_type);
Units& ResourceManager_GetUnits();
TeamClanType ResourceManager_GetClanID(const std::string clan_id);
std::string ResourceManager_GetClanID(const TeamClanType clan_id);

class ResourceManager_MutexLock {
private:
    SDL_Mutex* m_mutex;

public:
    explicit ResourceManager_MutexLock(SDL_Mutex* mutex) : m_mutex(mutex) { SDL_LockMutex(m_mutex); }
    virtual ~ResourceManager_MutexLock() { SDL_UnlockMutex(m_mutex); }
};

[[nodiscard]] SDL_Mutex* ResourceManager_CreateMutex();
void ResourceManager_DestroyMutexes();
std::vector<std::filesystem::path> ResourceManager_GetFileList(const std::filesystem::path& folder,
                                                               const std::string& extension);

#define _(key) ResourceManager_GetLanguageEntry(0x##key).c_str()

#endif /* RESOURCE_MANAGER_HPP */
