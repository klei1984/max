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

#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "access.hpp"
#include "assertmenu.hpp"
#include "attributes.hpp"
#include "cursor.hpp"
#include "drawloadbar.hpp"
#include "enums.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "hash.hpp"
#include "help.hpp"
#include "language.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "missionmanager.hpp"
#include "randomizer.hpp"
#include "screendump.h"
#include "scripter.hpp"
#include "settings.hpp"
#include "sha2.h"
#include "sound_manager.hpp"
#include "units_manager.hpp"
#include "utf8.hpp"
#include "window_manager.hpp"

struct res_index {
    char tag[8];
    int32_t data_offset;
    int32_t data_size;
};

static_assert(sizeof(struct res_index) == 16, "The structure needs to be packed.");

struct res_header {
    char id[4];
    int32_t offset;
    int32_t size;
};

static_assert(sizeof(struct res_header) == 12, "The structure needs to be packed.");

struct GameResourceMeta {
    ResourceID res_file_item_index;
    uint8_t* resource_buffer;
    uint8_t res_file_id;
};

static constexpr int32_t ResourceManager_MinimumMemory = 6;
static constexpr int32_t ResourceManager_MinimumMemoryEnhancedGfx = 13;
static constexpr uintmax_t ResourceManager_MinimumDiskSpace = 1024 * 1024;

static const std::unordered_map<std::string, TeamClanType> ResourceManager_ClansLutStringKey = {
    {"Random", TEAM_CLAN_RANDOM},        {"Clan A", TEAM_CLAN_THE_CHOSEN}, {"Clan B", TEAM_CLAN_CRIMSON_PATH},
    {"Clan C", TEAM_CLAN_VON_GRIFFIN},   {"Clan D", TEAM_CLAN_AYERS_HAND}, {"Clan E", TEAM_CLAN_MUSASHI},
    {"Clan F", TEAM_CLAN_SACRED_EIGHTS}, {"Clan G", TEAM_CLAN_7_KNIGHTS},  {"Clan H", TEAM_CLAN_AXIS_INC}};

static const std::unordered_map<TeamClanType, std::string> ResourceManager_ClansLutEnumKey = {
    {TEAM_CLAN_RANDOM, "Random"},        {TEAM_CLAN_THE_CHOSEN, "Clan A"}, {TEAM_CLAN_CRIMSON_PATH, "Clan B"},
    {TEAM_CLAN_VON_GRIFFIN, "Clan C"},   {TEAM_CLAN_AYERS_HAND, "Clan D"}, {TEAM_CLAN_MUSASHI, "Clan E"},
    {TEAM_CLAN_SACRED_EIGHTS, "Clan F"}, {TEAM_CLAN_7_KNIGHTS, "Clan G"},  {TEAM_CLAN_AXIS_INC, "Clan H"}};

static std::unique_ptr<std::ofstream> ResourceManager_LogFile;
static std::shared_ptr<MissionManager> ResourceManager_MissionManager;
static std::shared_ptr<Attributes> ResourceManager_UnitAttributes;
static std::shared_ptr<Clans> ResourceManager_Clans;
static std::unique_ptr<Units> ResourceManager_Units;
static std::shared_ptr<Settings> ResourceManager_Settings;
static std::shared_ptr<Language> ResourceManager_LanguageManager;
static std::shared_ptr<Help> ResourceManager_HelpManager;
static std::unique_ptr<std::unordered_map<std::string, ResourceID>> ResourceManager_ResourceIDLUT;
static std::unique_ptr<std::vector<SDL_mutex*>> ResourceManager_SDLMutexes;
static std::string ResourceManager_SystemLocale{"en-US"};

FILE* res_file_handle_array[2];
struct res_index* ResourceManager_ResItemTable;
struct GameResourceMeta* ResourceManager_ResMetaTable;
uint8_t ResourceManager_ResFileCount;
int32_t ResourceManager_ResItemCount;
int32_t resource_buffer_size;
ColorIndex* color_animation_buffer;

const uint8_t ResourceManager_TeamRedColorIndices[8] = {56, 57, 58, 59, 60, 61, 62, 63};
const uint8_t ResourceManager_TeamGreenColorIndices[8] = {32, 33, 34, 35, 36, 37, 38, 39};
const uint8_t ResourceManager_TeamBlueColorIndices[8] = {48, 49, 50, 51, 52, 53, 54, 55};
const uint8_t ResourceManager_TeamGrayColorIndices[8] = {255, 161, 172, 169, 216, 213, 212, 207};
const uint8_t ResourceManager_TeamDerelictColorIndices[8] = {216, 215, 214, 213, 212, 211, 210, 209};

ColorIndex* ResourceManager_TeamRedColorIndexTable;
ColorIndex* ResourceManager_TeamGreenColorIndexTable;
ColorIndex* ResourceManager_TeamBlueColorIndexTable;
ColorIndex* ResourceManager_TeamGrayColorIndexTable;
ColorIndex* ResourceManager_TeamDerelictColorIndexTable;
ColorIndex* ResourceManager_ColorIndexTable06;
ColorIndex* ResourceManager_ColorIndexTable07;
ColorIndex* ResourceManager_ColorIndexTable08;
ColorIndex* ResourceManager_ColorIndexTable09;
ColorIndex* ResourceManager_ColorIndexTable10;
ColorIndex* ResourceManager_ColorIndexTable11;
ColorIndex* ResourceManager_ColorIndexTable12;
ColorIndex* ResourceManager_ColorIndexTable13x8;

uint8_t* ResourceManager_Minimap;
uint8_t* ResourceManager_MinimapUnits;
uint8_t* ResourceManager_MinimapFov;
uint8_t* ResourceManager_MinimapBgImage;

uint16_t* ResourceManager_MapTileIds;
uint8_t* ResourceManager_MapTileBuffer;
uint8_t* ResourceManager_MapSurfaceMap;
uint16_t* ResourceManager_CargoMap;
uint16_t ResourceManager_MapTileCount;

Point ResourceManager_MapSize;
Point ResourceManager_MinimapWindowSize;
Point ResourceManager_MinimapWindowOffset;
double ResourceManager_MinimapWindowScale;
uint8_t* ResourceManager_MainmapBgImage;
int32_t ResourceManager_MainmapZoomLimit;

const uint8_t ResourceManager_GenericTable[32] = {0x6c, 0x57, 0x36, 0xe6, 0x81, 0xe0, 0x72, 0x8a, 0xd3, 0xd9, 0xff,
                                                  0x54, 0x48, 0x3a, 0xcd, 0x75, 0xd5, 0x0d, 0xe9, 0xe7, 0x7a, 0x57,
                                                  0xae, 0xeb, 0x61, 0x16, 0xb0, 0x35, 0x55, 0x62, 0xed, 0xd1};

std::filesystem::path ResourceManager_FilePathGameData;
std::filesystem::path ResourceManager_FilePathGameBase;
std::filesystem::path ResourceManager_FilePathGamePref;

bool ResourceManager_DisableEnhancedGraphics;

TeamUnits ResourceManager_TeamUnitsRed;
TeamUnits ResourceManager_TeamUnitsGreen;
TeamUnits ResourceManager_TeamUnitsBlue;
TeamUnits ResourceManager_TeamUnitsGray;
TeamUnits ResourceManager_TeamUnitsDerelict;

static void ResourceManager_InitSDL();
static void ResourceManager_TestMemory();
static void ResourceManager_TestDiskSpace();
static void ResourceManager_InitInternals();
static void ResourceManager_InitResourceHandler();
static void ResourceManager_PreloadPatches();
static void ResourceManager_LoadMaxResources();
static int32_t ResourceManager_InitResManager();
static void ResourceManager_TestMouse();
static bool ResourceManager_GetGameDataPath(std::filesystem::path& path);
static int32_t ResourceManager_BuildResourceTable(const std::filesystem::path filepath);
static int32_t ResourceManager_BuildColorTables();
static void ResourceManager_InitMinimapResources();
static void ResourceManager_InitMainmapResources();
static bool ResourceManager_LoadMapTiles(FILE* fp, DrawLoadBar* loadbar);
static SDL_AssertState SDLCALL ResourceManager_AssertionHandler(const SDL_AssertData* data, void* userdata);
static void ResourceManager_LogOutputHandler(void* userdata, int category, SDL_LogPriority priority,
                                             const char* message);
static void ResourceManager_LogOutputFlush();
static inline void ResourceManager_FixWorldFiles(const ResourceID world);
static void Resourcemanager_InitLocale();
static void ResourceManager_InitLanguageManager();
static void ResourceManager_InitHelpManager();
static void ResourceManager_InitMissionManager();
static void ResourceManager_InitUnitAttributes();
static void ResourceManager_InitClans();
static void ResourceManager_InitSettings();
static void ResourceManager_InitUnits();
static void ResourceManager_ResetUnitsSprites();
static std::filesystem::path ResourceManager_GetFileResourcePath(const std::string& string,
                                                                 std::filesystem::path& path);
static std::filesystem::path ResourceManager_GetFileResourcePath(const std::string& string, const ResourceType type);
static bool ResourceManager_GetBasePath(std::filesystem::path& path);
static bool ResourceManager_GetPrefPath(std::filesystem::path& path);
static void ResourceManager_InitBasePath();
static void ResourceManager_InitPrefPath();
static void ResourceManager_InitGameDataPath();

static inline std::filesystem::path ResourceManager_GetFileResourcePathPrefix(ResourceType type) {
    auto path{ResourceManager_FilePathGameData};

    switch (type) {
        case ResourceType_GameBase: {
            path = ResourceManager_FilePathGameBase;
        } break;

        case ResourceType_GamePref: {
            path = ResourceManager_FilePathGamePref;
        } break;

        case ResourceType_GameData: {
            path = ResourceManager_FilePathGameData;
        } break;
    }

    return path;
}

bool ResourceManager_GetBasePath(std::filesystem::path& path) {
    bool result;
    std::filesystem::path local_path;
    std::error_code ec;

    auto platform = SDL_GetPlatform();

    if (!std::strcmp(platform, "Windows")) {
        auto base_path = SDL_GetBasePath();

        // SDL_GetBasePath may fail if SDL is not initialized yet
        if (base_path) {
            local_path = std::filesystem::path(base_path).lexically_normal();
            SDL_free(base_path);
        }

        if (!std::filesystem::exists(local_path, ec) || ec) {
            local_path = std::filesystem::current_path(ec).lexically_normal();
        }

    } else if (!std::strcmp(platform, "Linux")) {
        auto xdg_data_dirs = SDL_getenv("XDG_DATA_DIRS");

        if (xdg_data_dirs) {
            std::istringstream data(xdg_data_dirs);
            for (std::string xdg_data_dir; std::getline(data, xdg_data_dir, ':');) {
                local_path = (std::filesystem::path(xdg_data_dir) / "max-port/").lexically_normal();

                if (std::filesystem::exists(local_path, ec) && !ec) {
                    break;
                }
            }
        }

        if (!std::filesystem::exists(local_path, ec) || ec) {
            local_path = std::filesystem::path("/usr/share/max-port/").lexically_normal();

            if (!std::filesystem::exists(local_path, ec) || ec) {
                local_path = std::filesystem::path("/usr/local/share/max-port/").lexically_normal();

                if (!std::filesystem::exists(local_path, ec) || ec) {
                    auto xdg_data_home = SDL_getenv("XDG_DATA_HOME");

                    if (xdg_data_home) {
                        local_path = (std::filesystem::path(xdg_data_home) / "max-port/").lexically_normal();
                    }

#if SDL_VERSION_ATLEAST(2, 0, 1)
                    if (!std::filesystem::exists(local_path, ec) || ec) {
                        // SDL_GetPrefPath may fail if SDL is not initialized yet
                        auto pref_path = SDL_GetPrefPath("", "max-port");

                        if (pref_path) {
                            local_path = std::filesystem::path(pref_path).lexically_normal();
                            SDL_free(pref_path);
                        }
                    }
#endif /* SDL_VERSION_ATLEAST(2, 0, 1) */
                }
            }
        }

        if (!std::filesystem::exists(local_path, ec) || ec) {
            local_path = std::filesystem::current_path(ec).lexically_normal();
        }
    }

    if (!std::filesystem::exists(local_path / "PATCHES.RES", ec) || ec) {
        result = false;

    } else {
        path = local_path;

        result = true;
    }

    return result;
}

bool ResourceManager_GetPrefPath(std::filesystem::path& path) {
    bool result;
    std::filesystem::path local_path;
    std::error_code ec;

    auto platform = SDL_GetPlatform();

    if (!std::strcmp(platform, "Windows")) {
        std::filesystem::path base_path;

        if (ResourceManager_GetBasePath(base_path)) {
            if (std::filesystem::exists(base_path / ".portable", ec) && !ec) {
                local_path = base_path;
            }
        }

        if (!std::filesystem::exists(local_path, ec) || ec) {
#if SDL_VERSION_ATLEAST(2, 0, 1)
            // SDL_GetPrefPath may fail if SDL is not initialized yet
            auto pref_path = SDL_GetPrefPath("", "M.A.X. Port");

            if (pref_path) {
                local_path = std::filesystem::path(pref_path).lexically_normal();
                SDL_free(pref_path);
            } else
#endif /* SDL_VERSION_ATLEAST(2, 0, 1) */
            {
                auto app_data = SDL_getenv("APPDATA");

                if (app_data) {
                    local_path = (std::filesystem::path(app_data) / "M.A.X. Port/").lexically_normal();
                }
            }
        }

    } else if (!std::strcmp(platform, "Linux")) {
        auto xdg_data_home = SDL_getenv("XDG_DATA_HOME");

        if (xdg_data_home) {
            local_path = (std::filesystem::path(xdg_data_home) / "max-port/").lexically_normal();
        }

#if SDL_VERSION_ATLEAST(2, 0, 1)
        if (!std::filesystem::exists(local_path, ec) || ec) {
            // SDL_GetPrefPath may fail if SDL is not initialized yet
            auto pref_path = SDL_GetPrefPath("", "max-port");

            if (pref_path) {
                local_path = std::filesystem::path(pref_path).lexically_normal();
                SDL_free(pref_path);
            }
        }
#endif /* SDL_VERSION_ATLEAST(2, 0, 1) */
    }

    if (std::filesystem::exists(local_path, ec) && !ec) {
        path = local_path;

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool ResourceManager_GetGameDataPath(std::filesystem::path& path) {
    auto game_data_path = ResourceManager_GetSettings()->GetStringValue("game_data");
    auto local_path = std::filesystem::path(game_data_path).lexically_normal();
    std::error_code ec;
    bool result{false};

    if (std::filesystem::exists(local_path, ec) && !ec) {
        path = local_path;

        result = true;
    }

    return result;
}

void ResourceManager_InitBasePath() {
    if (!ResourceManager_GetBasePath(ResourceManager_FilePathGameBase)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _(cf05), _(2690), nullptr);
        exit(EXIT_FAILURE);
    }
}
void ResourceManager_InitPrefPath() {
    if (!ResourceManager_GetPrefPath(ResourceManager_FilePathGamePref)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _(cf05), _(416b), nullptr);
        exit(EXIT_FAILURE);
    }
}
void ResourceManager_InitGameDataPath() {
    if (!ResourceManager_GetGameDataPath(ResourceManager_FilePathGameData)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _(cf05), _(0f72), nullptr);
        exit(EXIT_FAILURE);
    }
}

void ResourceManager_InitResources() {
    // nothing is available
    Resourcemanager_InitLocale();
    ResourceManager_InitResourceHandler();
    ResourceManager_PreloadPatches();
    // resources from patches are available
    ResourceManager_InitLanguageManager();
    // localized strings are available
    ResourceManager_InitBasePath();
    ResourceManager_InitPrefPath();
    // resource file system base and pref paths are available
    ResourceManager_InitSettings();
    // game settings are available
    ResourceManager_InitGameDataPath();
    // resource file system game data path is available
    ResourceManager_InitSDL();
    // SDL video sub-system and logging are available
    ResourceManager_TestMemory();
    ResourceManager_TestDiskSpace();
    // tested RAM and NVM space
    ResourceManager_LoadMaxResources();
    // MAX resources are available
    ResourceManager_InitUnitAttributes();
    // MAX unit attribute definitions are available
    ResourceManager_InitClans();
    // MAX clan definitions are available
    ResourceManager_InitUnits();
    // MAX unit definitions are available
    Randomizer_Init();
    Scripter::Init();
    ResourceManager_InitInternals();
    ResourceManager_InitHelpManager();
    ResourceManager_InitMissionManager();
    ResourceManager_TestMouse();
}

void ResourceManager_InitSDL() {
    SDL_LogSetOutputFunction(&ResourceManager_LogOutputHandler, nullptr);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _(cf05), _(438a), nullptr);
        SDL_Log("%s", _(438a));
        SDL_Log("%s", SDL_GetError());

        SDL_Quit();
        exit(EXIT_FAILURE);
    }
}

void ResourceManager_TestMemory() {
    const int32_t memory = SDL_GetSystemRAM();

    if (memory < ResourceManager_MinimumMemory) {
        /// \todo Convert to std::vformat()
        SmartString message;

        message.Sprintf(200, _(d0a2), ResourceManager_MinimumMemory, memory);

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _(cf05), message.GetCStr(), nullptr);
        SDL_Log("%s", message.GetCStr());

        SDL_Quit();
        exit(EXIT_FAILURE);
    }
}

void ResourceManager_TestDiskSpace() {
    std::error_code ec;
    const auto info = std::filesystem::space(ResourceManager_FilePathGamePref, ec);

    if (!ec && info.available < ResourceManager_MinimumDiskSpace) {
        /// \todo Convert to std::vformat()
        SmartString message;

        message.Sprintf(200, _(efb0), static_cast<float>(ResourceManager_MinimumDiskSpace) / (1024 * 1024));

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, _(b7f4), message.GetCStr(), nullptr);
        SDL_Log("%s", message.GetCStr());
    }
}

void ResourceManager_InitInternals() {
    auto language = ResourceManager_GetSettings()->GetStringValue("language");
    int32_t error_code;

    ResourceManager_SetSystemLocale(language);

    error_code = ResourceManager_InitResManager();

    if (error_code) {
        ResourceManager_ExitGame(error_code);
    }

    if (WindowManager_Init()) {
        ResourceManager_ExitGame(EXIT_CODE_SCREEN_INIT_FAILED);
    }

    mouse_set_wheel_sensitivity(ResourceManager_GetSettings()->GetNumericValue("mouse_wheel_sensitivity"));

    SDL_SetAssertionHandler(&ResourceManager_AssertionHandler, nullptr);

    if (SDL_GetSystemRAM() < ResourceManager_MinimumMemoryEnhancedGfx) {
        ResourceManager_GetSettings()->SetNumericValue("enhanced_graphics", false);
    }

    ResourceManager_DisableEnhancedGraphics = !ResourceManager_GetSettings()->GetNumericValue("enhanced_graphics");

    SoundManager_Init();
    register_pause(-1, nullptr);
    register_screendump(GNW_KB_KEY_LALT_C, screendump_pcx);
}

void ResourceManager_TestMouse() {
    if (!mouse_query_exist()) {
        ResourceManager_ExitGame(EXIT_CODE_NO_MOUSE);
    }
}

void ResourceManager_ExitGame(int32_t error_code) {
    const char* const ResourceManager_ErrorCodes[] = {"",      _(1347), _(c164), _(c116), _(9edb), _(6bc6),
                                                      _(5ced), _(3b69), _(c499), _(4afd), _(bc1c), _(908e),
                                                      _(b2b3), _(00a7), _(eb11), _(3004), _(07f8)};

    if ((error_code == EXIT_CODE_NO_ERROR) || (error_code == EXIT_CODE_THANKS)) {
        menu_draw_exit_logos();
    }

    SDL_Log("%s", ResourceManager_ErrorCodes[error_code]);

    ResourceManager_Exit();
}

void ResourceManager_Exit() {
    SoundManager_Deinit();
    win_exit();
    Svga_Deinit();
    ResourceManager_DestroyMutexes();
    SDL_Quit();

    exit(0);
}

void ResourceManager_InitResourceHandler() {
    ResourceManager_ResMetaTable = new (std::nothrow) GameResourceMeta[RESOURCE_E];
    ResourceManager_ResFileCount = 0;
    ResourceManager_ResItemCount = 0;

    if (ResourceManager_ResMetaTable) {
        for (int32_t i = 0; i < RESOURCE_E; ++i) {
            ResourceManager_ResMetaTable[i].resource_buffer = nullptr;
            ResourceManager_ResMetaTable[i].res_file_item_index = INVALID_ID;
            ResourceManager_ResMetaTable[i].res_file_id = 0;
        }

    } else {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "M.A.X. Error", "Insufficient memory for resource tables.",
                                 nullptr);
        exit(EXIT_FAILURE);
    }
}

void ResourceManager_PreloadPatches() {
    std::filesystem::path path{"./"};

    if (!ResourceManager_GetBasePath(path)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "M.A.X. Error", "Failed to obtain application base directory.",
                                 nullptr);
        exit(EXIT_FAILURE);
    }

    if (ResourceManager_BuildResourceTable(
            (ResourceManager_GetFileResourcePath("PATCHES.RES", path)).lexically_normal()) != EXIT_CODE_NO_ERROR) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "M.A.X. Error", "Failed to load PATCHES.RES.", nullptr);
        exit(EXIT_FAILURE);
    }
}

void ResourceManager_LoadMaxResources() {
    if (ResourceManager_BuildResourceTable(
            (ResourceManager_GetFileResourcePath("MAX.RES", ResourceManager_FilePathGameData)).lexically_normal()) !=
        EXIT_CODE_NO_ERROR) {
        /// \todo Convert to translated string
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "M.A.X. Error", "Failed to load MAX.RES.", nullptr);
        SDL_Log("%s", "Failed to load MAX.RES.");

        SDL_Quit();
        exit(EXIT_FAILURE);
    }
}

int32_t ResourceManager_InitResManager() {
    int32_t result;

    if (ResourceManager_BuildColorTables()) {
        Cursor_Init();

        result = EXIT_CODE_NO_ERROR;

    } else {
        result = EXIT_CODE_INSUFFICIENT_MEMORY;
    }

    return result;
}

uint8_t* ResourceManager_ReadResource(ResourceID id) {
    uint8_t* resource_buffer;

    if (id < MEM_END || id >= RESOURCE_E) {
        resource_buffer = nullptr;

    } else {
        if (ResourceManager_ResMetaTable == nullptr ||
            ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
            resource_buffer = nullptr;

        } else {
            FILE* fp = res_file_handle_array[ResourceManager_ResMetaTable[id].res_file_id];
            int32_t data_size =
                ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_size;
            int32_t data_offset =
                ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_offset;

            fseek(fp, data_offset, SEEK_SET);

            uint8_t* buffer = new (std::nothrow) uint8_t[data_size + sizeof('\0')];
            if (!buffer) {
                ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
            }

            if (!fread(buffer, data_size, 1, fp)) {
                ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
            }

            buffer[data_size] = '\0';
            resource_buffer = buffer;
        }
    }

    return resource_buffer;
}

uint8_t* ResourceManager_LoadResource(ResourceID id) {
    uint8_t* resource_buffer;

    if (id == INVALID_ID) {
        resource_buffer = nullptr;
    } else {
        SDL_assert(id < MEM_END);

        if (ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
            resource_buffer = nullptr;
        } else {
            if ((resource_buffer = ResourceManager_ResMetaTable[id].resource_buffer) == nullptr) {
                FILE* fp = res_file_handle_array[ResourceManager_ResMetaTable[id].res_file_id];
                int32_t data_size =
                    ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_size;
                int32_t data_offset =
                    ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_offset;

                fseek(fp, data_offset, SEEK_SET);

                resource_buffer = new (std::nothrow) uint8_t[data_size];
                if (!resource_buffer) {
                    ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
                }

                if (!fread(resource_buffer, data_size, 1, fp)) {
                    ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
                }

                ResourceManager_ResMetaTable[id].resource_buffer = resource_buffer;
                resource_buffer_size += data_size;
            }
        }
    }

    return resource_buffer;
}

uint32_t ResourceManager_GetResourceSize(ResourceID id) {
    uint32_t data_size;

    if (id == INVALID_ID || ResourceManager_ResMetaTable == nullptr ||
        ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
        data_size = 0;
    } else {
        data_size = ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_size;
    }

    return data_size;
}

int32_t ResourceManager_ReadImageHeader(ResourceID id, struct ImageBigHeader* buffer) {
    int32_t result;

    SDL_assert(ResourceManager_ResMetaTable);

    if (id == INVALID_ID || ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
        result = false;
    } else {
        FILE* fp = res_file_handle_array[ResourceManager_ResMetaTable[id].res_file_id];
        int32_t data_offset =
            ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_offset;

        fseek(fp, data_offset, SEEK_SET);

        if (!fread(buffer, sizeof(struct ImageBigHeader), 1, fp)) {
            ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
        }

        result = true;
    }

    return result;
}

int32_t ResourceManager_GetResourceFileID(ResourceID id) {
    return ResourceManager_ResMetaTable[id].res_file_item_index;
}

const char* ResourceManager_GetResourceID(ResourceID id) { return ResourceManager_ResourceIdList[id]; }

ResourceID ResourceManager_GetResourceID(std::string id) {
    if (!ResourceManager_ResourceIDLUT) {
        ResourceManager_ResourceIDLUT = std::make_unique<std::unordered_map<std::string, ResourceID>>();

        for (size_t i = 0; i < RESOURCE_E; ++i) {
            const std::string key = ResourceManager_ResourceIdList[i];
            const ResourceID value = static_cast<ResourceID>(i);

            (*ResourceManager_ResourceIDLUT)[key] = value;
        }
    }

    auto it = ResourceManager_ResourceIDLUT->find(id);

    return (it != ResourceManager_ResourceIDLUT->end()) ? it->second : INVALID_ID;
}

FILE* ResourceManager_GetFileHandle(ResourceID id) {
    FILE* fp;

    if (id == INVALID_ID || ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
        fp = nullptr;
    } else {
        fp = res_file_handle_array[ResourceManager_ResMetaTable[id].res_file_id];
        int32_t data_offset =
            ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_offset;

        fseek(fp, data_offset, SEEK_SET);
    }

    return fp;
}

std::filesystem::path ResourceManager_GetFileResourcePath(const std::string& string, std::filesystem::path& path) {
    auto filename = ResourceManager_StringToLowerCase(string);
    auto filepath = (path / filename).lexically_normal();
    std::error_code ec;

    if (!std::filesystem::exists(filepath, ec) || ec) {
        filename = ResourceManager_StringToUpperCase(string);
        filepath = (path / filename).lexically_normal();

        if (!std::filesystem::exists(filepath, ec) || ec) {
            filepath = string;
        }
    }

    return filepath;
}

std::filesystem::path ResourceManager_GetFileResourcePath(const std::string& string, const ResourceType type) {
    auto filename = ResourceManager_StringToLowerCase(string);
    auto pathprefix = ResourceManager_GetFileResourcePathPrefix(type);
    auto filepath = (pathprefix / filename).lexically_normal();
    std::error_code ec;

    if (!std::filesystem::exists(filepath, ec) || ec) {
        filename = ResourceManager_StringToUpperCase(string);
        filepath = (pathprefix / filename).lexically_normal();

        if (!std::filesystem::exists(filepath, ec) || ec) {
            filepath = string;
        }
    }

    return filepath;
}

FILE* ResourceManager_OpenFileResource(const std::string& string, const ResourceType type, const char* const mode,
                                       std::filesystem::path* path) {
    FILE* handle{nullptr};
    auto filepath = ResourceManager_GetFileResourcePath(string, type);

    handle = fopen(filepath.string().c_str(), mode);

    if (handle && path) {
        *path = filepath;
    }

    return handle;
}

FILE* ResourceManager_OpenFileResource(const ResourceID id, const ResourceType type, const char* const mode,
                                       std::filesystem::path* path) {
    char* resource{reinterpret_cast<char*>(ResourceManager_ReadResource(id))};
    FILE* handle{nullptr};

    if (resource) {
        handle = ResourceManager_OpenFileResource(resource, type, mode, path);

        delete[] resource;
    }

    return handle;
}

ResourceID ResourceManager_GetResourceID(int32_t index) {
    char buffer[9];

    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, ResourceManager_ResItemTable[index].tag, sizeof(((struct res_index){0}).tag));
    for (int32_t i = 0; i < RESOURCE_E; ++i) {
        if (!strncmp(buffer, ResourceManager_ResourceIdList[i], sizeof(buffer))) {
            return static_cast<ResourceID>(i);
        }
    }

    return INVALID_ID;
}

void ResourceManager_Realloc(ResourceID id, uint8_t* buffer, int32_t data_size) {
    if (ResourceManager_ResMetaTable[id].resource_buffer) {
        delete[] ResourceManager_ResMetaTable[id].resource_buffer;
        resource_buffer_size -=
            ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_size;
    }

    ResourceManager_ResMetaTable[id].resource_buffer = buffer;
    resource_buffer_size += data_size;
}

int32_t ResourceManager_GetFileOffset(ResourceID id) {
    int32_t data_offset;

    if (id == INVALID_ID || ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
        data_offset = 0;
    } else {
        data_offset = ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_offset;
    }

    return data_offset;
}

void ResourceManager_FreeResources() {
    mouse_hide();

    for (int16_t i = 0; i < MEM_END; ++i) {
        if (ResourceManager_ResMetaTable[i].resource_buffer) {
            delete[] ResourceManager_ResMetaTable[i].resource_buffer;
            ResourceManager_ResMetaTable[i].resource_buffer = nullptr;
        }
    }

    ResourceManager_ResetUnitsSprites();

    resource_buffer_size = 0;

    Cursor_Init();
    Cursor_SetCursor(CURSOR_HAND);
    mouse_show();
}

int32_t ResourceManager_BuildColorTables() {
    int32_t result;
    ColorIndex* aligned_buffer;

    color_animation_buffer = new (std::nothrow) ColorIndex[20 * PALETTE_SIZE + PALETTE_SIZE];
    aligned_buffer =
        reinterpret_cast<ColorIndex*>(((reinterpret_cast<intptr_t>(color_animation_buffer) + PALETTE_SIZE) >> 8) << 8);

    if (color_animation_buffer) {
        ResourceManager_TeamRedColorIndexTable = &aligned_buffer[0 * PALETTE_SIZE];
        ResourceManager_TeamGreenColorIndexTable = &aligned_buffer[1 * PALETTE_SIZE];
        ResourceManager_TeamBlueColorIndexTable = &aligned_buffer[2 * PALETTE_SIZE];
        ResourceManager_TeamGrayColorIndexTable = &aligned_buffer[3 * PALETTE_SIZE];
        ResourceManager_TeamDerelictColorIndexTable = &aligned_buffer[4 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable06 = &aligned_buffer[5 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable07 = &aligned_buffer[6 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable08 = &aligned_buffer[7 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable09 = &aligned_buffer[8 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable10 = &aligned_buffer[9 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable11 = &aligned_buffer[10 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable12 = &aligned_buffer[11 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable13x8 = &aligned_buffer[12 * PALETTE_SIZE];

        {
            ColorIndex* buffer = &aligned_buffer[19 * PALETTE_SIZE];

            for (int32_t i = 0; i < PALETTE_SIZE; ++i) {
                ResourceManager_TeamDerelictColorIndexTable[i] = i;
                ResourceManager_TeamGrayColorIndexTable[i] = i;
                ResourceManager_TeamBlueColorIndexTable[i] = i;
                ResourceManager_TeamGreenColorIndexTable[i] = i;
                ResourceManager_TeamRedColorIndexTable[i] = i;
                buffer[i] = i;
            }
        }

        {
            int32_t j = 0;
            int32_t k = 32;

            while (j < 24) {
                if (j == 8) {
                    k += 8;
                }
                ResourceManager_TeamRedColorIndexTable[k] = ResourceManager_TeamRedColorIndices[j & 7];
                ResourceManager_TeamGreenColorIndexTable[k] = ResourceManager_TeamGreenColorIndices[j & 7];
                ResourceManager_TeamBlueColorIndexTable[k] = ResourceManager_TeamBlueColorIndices[j & 7];
                ResourceManager_TeamGrayColorIndexTable[k] = ResourceManager_TeamGrayColorIndices[j & 7];
                ResourceManager_TeamDerelictColorIndexTable[k] = ResourceManager_TeamDerelictColorIndices[j & 7];
                k++;
                j++;
            }
        }

        ResourceManager_TeamUnitsRed.hash_team_id = HASH_TEAM_RED;
        ResourceManager_TeamUnitsRed.color_index_table = ResourceManager_TeamRedColorIndexTable;

        ResourceManager_TeamUnitsGreen.hash_team_id = HASH_TEAM_GREEN;
        ResourceManager_TeamUnitsGreen.color_index_table = ResourceManager_TeamGreenColorIndexTable;

        ResourceManager_TeamUnitsBlue.hash_team_id = HASH_TEAM_BLUE;
        ResourceManager_TeamUnitsBlue.color_index_table = ResourceManager_TeamBlueColorIndexTable;

        ResourceManager_TeamUnitsGray.hash_team_id = HASH_TEAM_GRAY;
        ResourceManager_TeamUnitsGray.color_index_table = ResourceManager_TeamGrayColorIndexTable;

        ResourceManager_TeamUnitsDerelict.hash_team_id = HASH_TEAM_DERELICT;
        ResourceManager_TeamUnitsDerelict.color_index_table = ResourceManager_TeamDerelictColorIndexTable;

        result = 1;
    } else {
        result = 0;
    }

    return result;
}

int32_t ResourceManager_BuildResourceTable(const std::filesystem::path filepath) {
    int32_t result;
    FILE* fp;
    struct res_header header;

    fp = fopen(filepath.string().c_str(), "rb");

    res_file_handle_array[ResourceManager_ResFileCount] = fp;

    if (fp) {
        if (fread(&header, sizeof(header), 1, fp)) {
            if (!strncmp("RES0", header.id, sizeof(res_header::id))) {
                if (ResourceManager_ResItemTable) {
                    ResourceManager_ResItemTable = static_cast<struct res_index*>(
                        realloc(static_cast<void*>(ResourceManager_ResItemTable),
                                header.size + ResourceManager_ResItemCount * sizeof(struct res_index)));
                } else {
                    ResourceManager_ResItemTable = static_cast<struct res_index*>(malloc(header.size));
                }

                if (ResourceManager_ResItemTable) {
                    fseek(fp, header.offset, SEEK_SET);
                    if (fread(&ResourceManager_ResItemTable[ResourceManager_ResItemCount], header.size, 1, fp)) {
                        int16_t new_item_count = ResourceManager_ResItemCount + header.size / sizeof(struct res_index);

                        for (int32_t i = ResourceManager_ResItemCount; i < new_item_count; ++i) {
                            ResourceID id = ResourceManager_GetResourceID(i);
                            if (id != INVALID_ID &&
                                ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
                                ResourceManager_ResMetaTable[id].res_file_item_index = static_cast<ResourceID>(i);
                                ResourceManager_ResMetaTable[id].res_file_id = ResourceManager_ResFileCount;
                            }
                        }

                        ResourceManager_ResItemCount = new_item_count;
                        ++ResourceManager_ResFileCount;
                        result = EXIT_CODE_NO_ERROR;
                    } else {
                        result = EXIT_CODE_CANNOT_READ_RES_FILE;
                    }
                } else {
                    result = EXIT_CODE_INSUFFICIENT_MEMORY;
                }
            } else {
                result = EXIT_CODE_RES_FILE_INVALID_ID;
            }
        } else {
            result = EXIT_CODE_CANNOT_READ_RES_FILE;
        }
    } else {
        result = EXIT_CODE_RES_FILE_NOT_FOUND;
    }

    return result;
}

std::string ResourceManager_StringToUpperCase(const std::string& string) { return utf8_toupper_str(string); }

std::string ResourceManager_StringToLowerCase(const std::string& string) { return utf8_tolower_str(string); }

void ResourceManager_InitMinimapResources() {
    const WindowInfo* mmw = WindowManager_GetWindow(WINDOW_MINIMAP);
    const int32_t mmw_width{mmw->window.lrx - mmw->window.ulx + 1};
    const int32_t mmw_height{mmw->window.lry - mmw->window.uly + 1};

    delete[] ResourceManager_MinimapFov;
    ResourceManager_MinimapFov = nullptr;

    delete[] ResourceManager_Minimap;
    ResourceManager_Minimap = nullptr;

    delete[] ResourceManager_MinimapUnits;
    ResourceManager_MinimapUnits = nullptr;

    ResourceManager_MinimapFov = new (std::nothrow) uint8_t[ResourceManager_MapSize.x * ResourceManager_MapSize.y];
    ResourceManager_Minimap = new (std::nothrow) uint8_t[ResourceManager_MapSize.x * ResourceManager_MapSize.y];
    ResourceManager_MinimapUnits = new (std::nothrow) uint8_t[ResourceManager_MapSize.x * ResourceManager_MapSize.y];

    if (ResourceManager_MinimapFov == nullptr || ResourceManager_Minimap == nullptr ||
        ResourceManager_MinimapUnits == nullptr) {
        ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
    }

    SDL_assert(mmw_width == mmw_height);

    Point offset;
    Point map_size{ResourceManager_MapSize};
    double scale{1.0};

    if (ResourceManager_MapSize.x != mmw_width || ResourceManager_MapSize.y != mmw_height) {
        if (ResourceManager_MapSize.x > ResourceManager_MapSize.y) {
            if (ResourceManager_MapSize.x == mmw_width) {
                map_size.x = mmw_width;
                map_size.y = ResourceManager_MapSize.y;
                offset.y = (mmw_height - map_size.y) / 2;

            } else {
                scale = static_cast<double>(mmw_width) / ResourceManager_MapSize.x;

                map_size.x = mmw_width;
                map_size.y = ResourceManager_MapSize.y * scale;
                offset.y = (mmw_height - map_size.y) / 2;
            }

        } else if (ResourceManager_MapSize.x == ResourceManager_MapSize.y) {
            map_size.x = mmw_width;
            map_size.y = mmw_height;
            scale = static_cast<double>(mmw_width) / ResourceManager_MapSize.x;

        } else {
            if (ResourceManager_MapSize.y == mmw_height) {
                map_size.x = ResourceManager_MapSize.x;
                map_size.y = mmw_height;
                offset.x = (mmw_width - map_size.x) / 2;

            } else {
                scale = static_cast<double>(mmw_height) / ResourceManager_MapSize.y;

                map_size.x = ResourceManager_MapSize.x * scale;
                map_size.y = mmw_height;
                offset.x = (mmw_width - map_size.x) / 2;
            }
        }
    }

    ResourceManager_MinimapWindowSize = map_size;
    ResourceManager_MinimapWindowOffset = offset;
    ResourceManager_MinimapWindowScale = scale;
}

void ResourceManager_InitMainmapResources() {
    const WindowInfo* mmw = WindowManager_GetWindow(WINDOW_MAIN_MAP);
    const int32_t mmw_width{mmw->window.lrx - mmw->window.ulx + 1};
    const int32_t mmw_height{mmw->window.lry - mmw->window.uly + 1};

    delete[] ResourceManager_MainmapBgImage;
    ResourceManager_MainmapBgImage = new (std::nothrow) uint8_t[mmw_width * mmw_height];

    buf_to_buf(mmw->buffer, mmw_width, mmw_height, mmw->width, ResourceManager_MainmapBgImage, mmw_width);

    ResourceManager_MainmapZoomLimit = std::max(std::min(WindowManager_MapWidth / ResourceManager_MapSize.x,
                                                         WindowManager_MapHeight / ResourceManager_MapSize.y),
                                                4);
}

void ResourceManager_InitInGameAssets(int32_t world) {
    WindowInfo* window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    int32_t progress_bar_value;
    uint16_t map_layer_count = 0;
    Point map_layer_dimensions[12];
    int32_t map_minimap_count;
    int32_t file_position;
    int32_t file_offset;
    uint16_t map_tile_count = 0;

    ResourceManager_GetSettings()->SetNumericValue("world", world);

    UnitsManager_GroundCoverUnits.Clear();
    UnitsManager_MobileLandSeaUnits.Clear();
    UnitsManager_ParticleUnits.Clear();
    UnitsManager_StationaryUnits.Clear();
    UnitsManager_MobileAirUnits.Clear();

    Hash_UnitHash.Clear();
    Hash_MapHash.Clear();

    GameManager_SelectedUnit = nullptr;

    delete[] ResourceManager_MapTileIds;
    ResourceManager_MapTileIds = nullptr;

    delete[] ResourceManager_MapTileBuffer;
    ResourceManager_MapTileBuffer = nullptr;

    delete[] ResourceManager_MapSurfaceMap;
    ResourceManager_MapSurfaceMap = nullptr;

    delete[] ResourceManager_CargoMap;
    ResourceManager_CargoMap = nullptr;

    delete[] ResourceManager_MinimapBgImage;
    ResourceManager_MinimapBgImage = nullptr;

    SoundManager_FreeMusic();

    WindowManager_LoadBigImage(FRAMEPIC, window, window->width, true, false, -1, -1, false, true);

    const WindowInfo* mmw = WindowManager_GetWindow(WINDOW_MINIMAP);
    const int32_t mmw_width{mmw->window.lrx - mmw->window.ulx + 1};
    const int32_t mmw_height{mmw->window.lry - mmw->window.uly + 1};

    ResourceManager_MinimapBgImage = new (std::nothrow) uint8_t[mmw_width * mmw_height];

    buf_to_buf(mmw->buffer, mmw_width, mmw_height, mmw->width, ResourceManager_MinimapBgImage, mmw_width);

    GameManager_InitLandingSequenceMenu(GameManager_GameState == GAME_STATE_7_SITE_SELECT);

    win_draw(window->id);

    world = SNOW_1 + world;

    auto fp{ResourceManager_OpenFileResource(static_cast<ResourceID>(world), ResourceType_GameData)};

    if (!fp) {
        ResourceManager_ExitGame(EXIT_CODE_WRL_FILE_OPEN_ERROR);
    }

    progress_bar_value = 0;
    DrawLoadBar load_bar(_(df4f));

    if (fseek(fp, 3, SEEK_SET) || 1 != fread(&map_layer_count, sizeof(uint16_t), 1, fp)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    if (map_layer_count != fread(&map_layer_dimensions, sizeof(Point), map_layer_count, fp)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    map_minimap_count = 1;

    ResourceManager_MapSize = map_layer_dimensions[map_minimap_count - 1];

    ResourceManager_InitMainmapResources();
    ResourceManager_InitMinimapResources();

    const uint32_t map_cell_count{static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

    if (fseek(fp, (map_minimap_count - 1) * map_cell_count, SEEK_CUR) ||
        map_cell_count != fread(ResourceManager_MinimapFov, sizeof(uint8_t), map_cell_count, fp)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    memcpy(ResourceManager_Minimap, ResourceManager_MinimapFov, map_cell_count);

    file_position = ftell(fp);

    if (file_position == -1) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    if (fseek(fp, sizeof(uint16_t) * map_cell_count, SEEK_CUR) ||
        1 != fread(&map_tile_count, sizeof(uint16_t), 1, fp)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    {
        uint8_t* palette;

        palette = new (std::nothrow) uint8_t[PALETTE_STRIDE * PALETTE_SIZE];

        if (fseek(fp, GFX_MAP_TILE_SIZE * GFX_MAP_TILE_SIZE * map_tile_count, SEEK_CUR) ||
            PALETTE_STRIDE * PALETTE_SIZE != fread(palette, sizeof(uint8_t), PALETTE_STRIDE * PALETTE_SIZE, fp)) {
            ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
        }

        for (int32_t i = 64 * PALETTE_STRIDE; i < 160 * PALETTE_STRIDE; ++i) {
            WindowManager_ColorPalette[i] = palette[i] / 4;
        }

        Color_SetSystemPalette(WindowManager_ColorPalette);
        Color_SetColorPalette(WindowManager_ColorPalette);

        delete[] palette;
    }

    if (fseek(fp, file_position, SEEK_SET)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    file_offset = (map_layer_count - map_minimap_count) * map_cell_count;

    for (int32_t i = map_minimap_count; --i;) {
        file_offset += sizeof(uint16_t) * map_layer_dimensions[i].x * map_layer_dimensions[i].y;
    }

    if (fseek(fp, file_offset, SEEK_CUR)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    ResourceManager_MapTileIds = new (std::nothrow) uint16_t[map_cell_count];

    if (ResourceManager_MapTileIds == nullptr) {
        ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
    }

    {
        uint32_t data_chunk_size = map_cell_count / 4;

        for (uint32_t data_offset = 0; data_offset < map_cell_count;) {
            data_chunk_size = std::min(data_chunk_size, map_cell_count - data_offset);
            if (fread(&ResourceManager_MapTileIds[data_offset], sizeof(uint16_t), data_chunk_size, fp) !=
                data_chunk_size) {
                ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
            }

            data_offset += data_chunk_size;
            load_bar.SetValue(20 * data_offset / map_cell_count);
        }
    }

    for (int32_t i = map_layer_count - map_minimap_count; --i > -1;) {
        file_offset += sizeof(uint16_t) * map_layer_dimensions[map_minimap_count + i].x *
                       map_layer_dimensions[map_minimap_count + i].y;
    }

    if (fseek(fp, file_offset, SEEK_CUR)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    if (1 != fread(&ResourceManager_MapTileCount, sizeof(ResourceManager_MapTileCount), 1, fp) ||
        !ResourceManager_LoadMapTiles(fp, &load_bar)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    {
        uint8_t* palette;

        progress_bar_value = 70;

        palette = new (std::nothrow) uint8_t[PALETTE_STRIDE * PALETTE_SIZE];

        if (PALETTE_STRIDE * PALETTE_SIZE != fread(palette, sizeof(uint8_t), PALETTE_STRIDE * PALETTE_SIZE, fp)) {
            ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
        }

        for (int32_t i = 64 * PALETTE_STRIDE; i < 160 * PALETTE_STRIDE; ++i) {
            WindowManager_ColorPalette[i] = palette[i] / 4;
        }

        Color_SetSystemPalette(WindowManager_ColorPalette);
        Color_SetColorPalette(WindowManager_ColorPalette);

        progress_bar_value += 3;

        load_bar.SetValue(progress_bar_value);

        delete[] palette;
    }

    {
        const uint8_t ResourceManager_PassData[] = {SURFACE_TYPE_LAND, SURFACE_TYPE_WATER, SURFACE_TYPE_COAST,
                                                    SURFACE_TYPE_AIR};
        uint8_t* pass_table{new (std::nothrow) uint8_t[ResourceManager_MapTileCount]};

        ResourceManager_MapSurfaceMap = new (std::nothrow) uint8_t[map_cell_count];

        if (ResourceManager_MapTileCount != fread(pass_table, sizeof(uint8_t), ResourceManager_MapTileCount, fp)) {
            ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
        }

        for (int32_t i = 0; i < ResourceManager_MapTileCount; ++i) {
            pass_table[i] = ResourceManager_PassData[pass_table[i]];
        }

        for (uint32_t i = 0; i < map_cell_count; ++i) {
            ResourceManager_MapSurfaceMap[i] = pass_table[ResourceManager_MapTileIds[i]];
        }

        delete[] pass_table;
    }

    fclose(fp);

    ResourceManager_CargoMap = new (std::nothrow) uint16_t[map_cell_count];

    for (uint32_t i = 0; i < map_cell_count; ++i) {
        ResourceManager_CargoMap[i] = 0;
    }

    progress_bar_value += 3;

    load_bar.SetValue(progress_bar_value);

    if (world >= SNOW_1 && world <= SNOW_6) {
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 63, 0, 0, 63, ResourceManager_ColorIndexTable06);
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 0, 63, 0, 63, ResourceManager_ColorIndexTable07);
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 0, 0, 63, 63, ResourceManager_ColorIndexTable08);

    } else {
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 63, 0, 0, 31, ResourceManager_ColorIndexTable06);
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 0, 63, 0, 31, ResourceManager_ColorIndexTable07);
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 0, 0, 63, 31, ResourceManager_ColorIndexTable08);
    }

    if (world >= CRATER_1 && world <= CRATER_6) {
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 63, ResourceManager_ColorIndexTable10);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 0, 63, 0, ResourceManager_ColorIndexTable11);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 0, ResourceManager_ColorIndexTable09);

    } else if (world >= GREEN_1 && world <= GREEN_6) {
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 63, ResourceManager_ColorIndexTable10);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 0, 0, 31, ResourceManager_ColorIndexTable11);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 0, ResourceManager_ColorIndexTable09);

    } else if (world >= DESERT_1 && world <= DESERT_6) {
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 63, ResourceManager_ColorIndexTable10);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 0, 0, 31, ResourceManager_ColorIndexTable11);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 0, ResourceManager_ColorIndexTable09);

    } else if (world >= SNOW_1 && world <= SNOW_6) {
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 0, 0, ResourceManager_ColorIndexTable10);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 0, 0, 63, ResourceManager_ColorIndexTable11);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 0, ResourceManager_ColorIndexTable09);
    }

    for (int32_t i = 0, j = 0; i < PALETTE_STRIDE * PALETTE_SIZE; i += PALETTE_STRIDE, ++j) {
        int32_t r;
        int32_t g;
        int32_t b;

        int32_t factor;

        r = WindowManager_ColorPalette[i];
        g = WindowManager_ColorPalette[i + 1];
        b = WindowManager_ColorPalette[i + 2];

        factor = 7;

        r = (std::max(factor, r) * 31) / 63;
        g = (std::max(factor, g) * 31) / 63;
        b = (std::max(factor, b) * 31) / 63;

        ResourceManager_ColorIndexTable12[j] = Color_MapColor(WindowManager_ColorPalette, r, g, b, false);
    }

    progress_bar_value += 3;

    load_bar.SetValue(progress_bar_value);

    for (int32_t i = 0, l = 0; i < 7 * 32; i += 32, ++l) {
        for (int32_t j = 0, k = 0; j < PALETTE_STRIDE * PALETTE_SIZE; j += PALETTE_STRIDE, ++k) {
            if (j == PALETTE_STRIDE * 31) {
                ResourceManager_ColorIndexTable13x8[l * PALETTE_SIZE + k] = 31;

            } else {
                int32_t r = (WindowManager_ColorPalette[j] * i) / (7 * 32);
                int32_t g = (WindowManager_ColorPalette[j + 1] * i) / (7 * 32);
                int32_t b = (WindowManager_ColorPalette[j + 2] * i) / (7 * 32);

                ResourceManager_ColorIndexTable13x8[l * PALETTE_SIZE + k] =
                    Color_MapColor(WindowManager_ColorPalette, r, g, b, false);
            }
        }

        progress_bar_value += 3;

        load_bar.SetValue(progress_bar_value);
    }

    ResourceManager_FixWorldFiles(static_cast<ResourceID>(world));
}

bool ResourceManager_LoadMapTiles(FILE* fp, DrawLoadBar* loadbar) {
    int32_t tile_size{GFX_MAP_TILE_SIZE};
    int32_t tile_count_stride{(ResourceManager_MapTileCount + 7) / 8};
    uint8_t* tile_data_chunk{nullptr};

    if (ResourceManager_DisableEnhancedGraphics) {
        tile_size /= 2;
        tile_data_chunk = new (std::nothrow) uint8_t[tile_count_stride * GFX_MAP_TILE_SIZE * GFX_MAP_TILE_SIZE];
    }

    ResourceManager_MapTileBuffer = new (std::nothrow) uint8_t[ResourceManager_MapTileCount * tile_size * tile_size];

    for (int32_t i = 0; i < ResourceManager_MapTileCount; i += tile_count_stride) {
        loadbar->SetValue(i * 50 / ResourceManager_MapTileCount + 20);

        if (!ResourceManager_DisableEnhancedGraphics) {
            tile_data_chunk = &ResourceManager_MapTileBuffer[i * GFX_MAP_TILE_SIZE * GFX_MAP_TILE_SIZE];
        }

        const int32_t tile_count = std::min(tile_count_stride, ResourceManager_MapTileCount - i);
        const uint32_t data_size = tile_count * GFX_MAP_TILE_SIZE * GFX_MAP_TILE_SIZE;

        if (data_size != fread(tile_data_chunk, sizeof(uint8_t), data_size, fp)) {
            delete[] tile_data_chunk;
            return false;
        }

        if (ResourceManager_DisableEnhancedGraphics) {
            uint8_t* source_address{tile_data_chunk};
            uint8_t* destination_address{&ResourceManager_MapTileBuffer[tile_size * tile_size * i]};

            for (int32_t j = 0; j < tile_count; ++j) {
                for (int32_t k = 0; k < tile_size; ++k) {
                    for (int32_t l = 0; l < tile_size; ++l) {
                        *destination_address = *source_address;
                        destination_address += 1;
                        source_address += 2;
                    }

                    source_address += 64;
                }
            }
        }
    }

    loadbar->SetValue(70);

    if (ResourceManager_DisableEnhancedGraphics) {
        delete[] tile_data_chunk;
    }

    return true;
}

void ResourceManager_SetClanUpgradesFromClans(const std::string& clan_id, ResourceID unit_type,
                                              UnitValues* unit_values) {
    auto clans = ResourceManager_GetClans();
    if (clans) {
        const Clans::AttributeID attrib_map[] = {
            Clans::ATTRIB_ATTACK_RATING,     // ATTRIB_ATTACK
            Clans::ATTRIB_SHOTS_PER_TURN,    // ATTRIB_ROUNDS
            Clans::ATTRIB_ATTACK_RANGE,      // ATTRIB_RANGE
            Clans::ATTRIB_ARMOR_RATING,      // ATTRIB_ARMOR
            Clans::ATTRIB_HIT_POINTS,        // ATTRIB_HITS
            Clans::ATTRIB_MOVEMENT_POINTS,   // ATTRIB_SPEED
            Clans::ATTRIB_SCAN_RANGE,        // ATTRIB_SCAN
            Clans::ATTRIB_TURNS_TO_BUILD,    // ATTRIB_TURNS
            Clans::ATTRIB_AMMUNITION,        // ATTRIB_AMMO
            Clans::ATTRIB_MOVE_AND_FIRE,     // ATTRIB_MOVE_AND_FIRE
            Clans::ATTRIB_MOVEMENT_POINTS,   // ATTRIB_FUEL (cannot happen, must be skipped)
            Clans::ATTRIB_STORAGE_CAPACITY,  // ATTRIB_STORAGE
            Clans::ATTRIB_BLAST_RADIUS,      // ATTRIB_ATTACK_RADIUS
            Clans::ATTRIB_EXPERIENCE,        // ATTRIB_AGENT_ADJUST
        };

        for (int32_t i = ATTRIB_ATTACK; i < ATTRIB_COUNT; ++i) {
            if (i == ATTRIB_FUEL) {
                continue;
            }

            int32_t value = clans->GetUnitTradeoff(clan_id, unit_type, attrib_map[i]);

            if (value != 0) {
                unit_values->AddAttribute(i, value);
            }
        }
    }
}

void ResourceManager_InitClanUnitValues(uint16_t team) {
    SmartPointer<UnitValues> unit_values;
    TeamUnits* team_units{nullptr};
    int32_t team_clan{TEAM_CLAN_RANDOM};

    switch (team) {
        case PLAYER_TEAM_RED: {
            team_units = &ResourceManager_TeamUnitsRed;
            team_clan = ResourceManager_GetSettings()->GetNumericValue("red_team_clan");
        } break;

        case PLAYER_TEAM_GREEN: {
            team_units = &ResourceManager_TeamUnitsGreen;
            team_clan = ResourceManager_GetSettings()->GetNumericValue("green_team_clan");
        } break;

        case PLAYER_TEAM_BLUE: {
            team_units = &ResourceManager_TeamUnitsBlue;
            team_clan = ResourceManager_GetSettings()->GetNumericValue("blue_team_clan");
        } break;

        case PLAYER_TEAM_GRAY: {
            team_units = &ResourceManager_TeamUnitsGray;
            team_clan = ResourceManager_GetSettings()->GetNumericValue("gray_team_clan");
        } break;

        case PLAYER_TEAM_ALIEN: {
            team_units = &ResourceManager_TeamUnitsDerelict;
            team_clan = TEAM_CLAN_THE_CHOSEN;
        } break;

        default: {
            SDL_assert(0);
        } break;
    }

    if (team_clan == TEAM_CLAN_RANDOM) {
        team_clan = Randomizer_Generate(8) + 1;
        ResourceManager_GetSettings()->SetNumericValue(menu_team_clan_setting[team], team_clan);
    }

    UnitsManager_TeamInfo[team].team_clan = team_clan;
    team_units->ClearComplexes();
    team_units->Init();

    auto clans = ResourceManager_GetClans();
    const auto clan_id = ResourceManager_GetClanID(static_cast<TeamClanType>(team_clan));

    for (int32_t i = UNIT_START; i < UNIT_END; ++i) {
        unit_values = new (std::nothrow) UnitValues(*team_units->GetBaseUnitValues(i));

        ResourceManager_SetClanUpgradesFromClans(clan_id, static_cast<ResourceID>(i), &*unit_values);
        team_units->SetBaseUnitValues(i, *unit_values);
        team_units->SetCurrentUnitValues(i, *unit_values);
    }
}

void ResourceManager_InitHeatMaps(uint16_t team) {
    if (UnitsManager_TeamInfo[team].team_type) {
        const uint32_t map_cell_count{static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

        UnitsManager_TeamInfo[team].heat_map_complete = new (std::nothrow) int8_t[map_cell_count];
        memset(UnitsManager_TeamInfo[team].heat_map_complete, 0, map_cell_count);

        UnitsManager_TeamInfo[team].heat_map_stealth_sea = new (std::nothrow) int8_t[map_cell_count];
        memset(UnitsManager_TeamInfo[team].heat_map_stealth_sea, 0, map_cell_count);

        UnitsManager_TeamInfo[team].heat_map_stealth_land = new (std::nothrow) int8_t[map_cell_count];
        memset(UnitsManager_TeamInfo[team].heat_map_stealth_land, 0, map_cell_count);

    } else {
        UnitsManager_TeamInfo[team].heat_map_complete = nullptr;
        UnitsManager_TeamInfo[team].heat_map_stealth_sea = nullptr;
        UnitsManager_TeamInfo[team].heat_map_stealth_land = nullptr;
    }
}

void ResourceManager_InitTeamInfo() {
    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX; ++i) {
        UnitsManager_TeamMissionSupplies[i].units.Clear();
        UnitsManager_TeamInfo[i].Reset();

        if (i < PLAYER_TEAM_MAX - 1) {
            UnitsManager_TeamInfo[i].team_type =
                ResourceManager_GetSettings()->GetNumericValue(menu_team_player_setting[i]);
            UnitsManager_TeamInfo[i].team_clan =
                ResourceManager_GetSettings()->GetNumericValue(menu_team_clan_setting[i]);

        } else {
            UnitsManager_TeamInfo[i].team_type = TEAM_TYPE_NONE;
            UnitsManager_TeamInfo[i].team_clan = TEAM_CLAN_RANDOM;
        }
    }

    ResourceManager_TeamUnitsRed.ClearComplexes();
    ResourceManager_TeamUnitsGreen.ClearComplexes();
    ResourceManager_TeamUnitsBlue.ClearComplexes();
    ResourceManager_TeamUnitsGray.ClearComplexes();
    ResourceManager_TeamUnitsDerelict.ClearComplexes();

    UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_units = &ResourceManager_TeamUnitsRed;
    UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_units = &ResourceManager_TeamUnitsGreen;
    UnitsManager_TeamInfo[PLAYER_TEAM_BLUE].team_units = &ResourceManager_TeamUnitsBlue;
    UnitsManager_TeamInfo[PLAYER_TEAM_GRAY].team_units = &ResourceManager_TeamUnitsGray;
    UnitsManager_TeamInfo[PLAYER_TEAM_ALIEN].team_units = &ResourceManager_TeamUnitsDerelict;

    MessageManager_ClearMessageLogs();
}

uint8_t* ResourceManager_GetBuffer(ResourceID id) {
    return id == INVALID_ID ? nullptr : ResourceManager_ResMetaTable[id].resource_buffer;
}

SDL_AssertState SDLCALL ResourceManager_AssertionHandler(const SDL_AssertData* data, void* userdata) {
    SDL_AssertState result;

    const auto caption =
        std::format("Assertion failure at {} ({}:{}), triggered {} {}: '{}'.\n", data->function, data->filename,
                    data->linenum, data->trigger_count, (data->trigger_count == 1) ? "time" : "times", data->condition);

    result = static_cast<SDL_AssertState>(AssertMenu(caption.c_str()).Run());

    if (result == SDL_ASSERTION_BREAK || result == SDL_ASSERTION_ABORT) {
        ResourceManager_LogOutputFlush();
    }

    return result;
}

void ResourceManager_LogOutputHandler(void* userdata, int category, SDL_LogPriority priority, const char* message) {
    if (!ResourceManager_LogFile) {
        auto filepath = (ResourceManager_FilePathGamePref / "stdout.txt").lexically_normal();

        ResourceManager_LogFile = std::make_unique<std::ofstream>(filepath.string().c_str(), std::ofstream::trunc);
    }

    if (ResourceManager_LogFile && ResourceManager_LogFile->is_open()) {
        *ResourceManager_LogFile << message;
    }
}

void ResourceManager_LogOutputFlush() {
    if (ResourceManager_LogFile && ResourceManager_LogFile->is_open()) {
        *ResourceManager_LogFile << std::endl;

        ResourceManager_LogFile->flush();
    }
}

std::string ResourceManager_Sha256(const ResourceID world) {
    constexpr size_t BLOCK_SIZE{4096};

    auto fp{ResourceManager_OpenFileResource(static_cast<ResourceID>(world), ResourceType_GameData)};
    char hex_digest[SHA256_DIGEST_SIZE * 2 + sizeof('\0')];
    uint8_t data[BLOCK_SIZE];
    uint8_t digest[SHA256_DIGEST_SIZE];
    sha256_ctx ctx;
    size_t data_read;

    if (!fp) {
        ResourceManager_ExitGame(EXIT_CODE_WRL_FILE_OPEN_ERROR);
    }

    sha256_init(&ctx);

    while ((data_read = fread(data, sizeof(uint8_t), sizeof(data), fp)) != 0) {
        sha256_update(&ctx, data, data_read);
    }

    sha256_final(&ctx, digest);

    for (size_t i = 0; i < sizeof(digest); ++i) {
        sprintf(hex_digest + (i * 2), "%02x", digest[i]);
    }

    hex_digest[sizeof(hex_digest) - 1] = '\0';

    fclose(fp);

    return hex_digest;
}

void ResourceManager_FixWorldFiles(const ResourceID world) {
    switch (world) {
        case GREEN_4: {
            /* fix tile 190 pass table info */
            const std::string reference{"dbcfc4495334640776e6a0b1776651f904583aa87f193a43436e6b1f04635241"};
            auto hash{ResourceManager_Sha256(world)};

            if (reference == hash) {
                constexpr uint16_t tile_190{190u};

                const uint32_t map_cell_count{
                    static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

                for (uint32_t i = 0; i < map_cell_count; ++i) {
                    if (ResourceManager_MapTileIds[i] == tile_190) {
                        ResourceManager_MapSurfaceMap[i] = SURFACE_TYPE_LAND;
                    }
                }
            }
        } break;

        case SNOW_3: {
            /* fix tile at grid cell position 057,057 in tile id map */
            const std::string reference{"a625d409ca1d5cd24cdeed165813e5b79c538b123491791016372492f5106801"};
            auto hash{ResourceManager_Sha256(world)};

            if (reference == hash) {
                constexpr uint16_t tile_55{55u};
                constexpr uint16_t grid_x_57{56u};
                constexpr uint16_t grid_y_57{56u};

                ResourceManager_MapTileIds[ResourceManager_MapSize.x * grid_y_57 + grid_x_57] = tile_55;
            }
        } break;

        case SNOW_5: {
            /* fix missing tile art */
            const std::string reference{"11a9255e65af53b6438803d0aef4ca96ed9d57575da3cf13779838ccda25a6e9"};
            auto hash{ResourceManager_Sha256(world)};

            if (reference == hash) {
                constexpr uint16_t tile_411{411u};
                constexpr uint16_t grid_x_55{54u};
                constexpr uint16_t grid_y_66{65u};

                const uint8_t tile_data[4096] = {
                    0x49, 0x4F, 0x59, 0x4E, 0x49, 0x52, 0x49, 0x49, 0x4E, 0x49, 0x4E, 0x40, 0x53, 0x4F, 0x49, 0x53,
                    0x53, 0x49, 0x53, 0x4F, 0x53, 0x53, 0x53, 0x49, 0x52, 0x49, 0x59, 0x42, 0x4E, 0x4F, 0x54, 0x52,
                    0x4F, 0x42, 0x54, 0x53, 0x57, 0x54, 0x53, 0x4F, 0x4F, 0x42, 0x4F, 0x53, 0x49, 0x42, 0x53, 0x4E,
                    0x42, 0x44, 0x4F, 0x4A, 0x4E, 0x4F, 0x40, 0x42, 0x53, 0x4E, 0x54, 0x4E, 0x4F, 0x53, 0x52, 0x59,
                    0x49, 0x42, 0x49, 0x49, 0x42, 0x40, 0x49, 0x53, 0x54, 0x4F, 0x49, 0x53, 0x4F, 0x59, 0x53, 0x4F,
                    0x59, 0x49, 0x4F, 0x4F, 0x4F, 0x54, 0x4F, 0x5A, 0x52, 0x4E, 0x42, 0x49, 0x4E, 0x52, 0x52, 0x42,
                    0x40, 0x53, 0x42, 0x49, 0x53, 0x53, 0x42, 0x40, 0x53, 0x42, 0x4F, 0x4E, 0x42, 0x4F, 0x53, 0x42,
                    0x49, 0x42, 0x49, 0x42, 0x42, 0x40, 0x49, 0x49, 0x4F, 0x42, 0x42, 0x52, 0x42, 0x4E, 0x53, 0x44,
                    0x5A, 0x52, 0x4E, 0x49, 0x44, 0x49, 0x40, 0x54, 0x49, 0x4F, 0x4E, 0x42, 0x4F, 0x59, 0x4F, 0x42,
                    0x53, 0x42, 0x49, 0x53, 0x4F, 0x49, 0x4E, 0x5A, 0x53, 0x42, 0x59, 0x53, 0x42, 0x59, 0x52, 0x53,
                    0x54, 0x4F, 0x53, 0x49, 0x42, 0x53, 0x4E, 0x42, 0x44, 0x4F, 0x4F, 0x42, 0x54, 0x53, 0x57, 0x54,
                    0x53, 0x4F, 0x4F, 0x42, 0x54, 0x54, 0x53, 0x4E, 0x54, 0x4F, 0x52, 0x4F, 0x42, 0x49, 0x52, 0x49,
                    0x59, 0x42, 0x53, 0x42, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x54, 0x53, 0x4E, 0x57, 0x59,
                    0x52, 0x49, 0x53, 0x40, 0x5A, 0x59, 0x53, 0x5F, 0x53, 0x59, 0x49, 0x4E, 0x42, 0x4E, 0x42, 0x42,
                    0x54, 0x4F, 0x4E, 0x42, 0x4F, 0x53, 0x42, 0x49, 0x42, 0x49, 0x40, 0x53, 0x42, 0x49, 0x53, 0x53,
                    0x42, 0x40, 0x53, 0x42, 0x59, 0x53, 0x59, 0x49, 0x42, 0x49, 0x4E, 0x52, 0x4F, 0x4E, 0x42, 0x4A,
                    0x4F, 0x53, 0x4E, 0x42, 0x42, 0x59, 0x42, 0x54, 0x49, 0x4F, 0x59, 0x4E, 0x49, 0x59, 0x49, 0x5F,
                    0x49, 0x52, 0x57, 0x52, 0x53, 0x59, 0x59, 0x57, 0x52, 0x42, 0x59, 0x54, 0x52, 0x42, 0x42, 0x53,
                    0x53, 0x49, 0x54, 0x4E, 0x53, 0x52, 0x4F, 0x54, 0x53, 0x4F, 0x54, 0x4F, 0x53, 0x49, 0x42, 0x53,
                    0x4E, 0x42, 0x44, 0x4F, 0x53, 0x57, 0x4E, 0x59, 0x4F, 0x4E, 0x49, 0x5A, 0x57, 0x59, 0x57, 0x52,
                    0x4F, 0x40, 0x42, 0x49, 0x40, 0x4E, 0x4F, 0x44, 0x49, 0x42, 0x49, 0x49, 0x42, 0x49, 0x53, 0x49,
                    0x4F, 0x4E, 0x53, 0x59, 0x4F, 0x57, 0x49, 0x49, 0x4A, 0x4F, 0x4F, 0x52, 0x42, 0x42, 0x52, 0x4E,
                    0x42, 0x54, 0x4E, 0x49, 0x53, 0x54, 0x49, 0x4F, 0x59, 0x49, 0x54, 0x4F, 0x4E, 0x42, 0x4F, 0x53,
                    0x42, 0x49, 0x42, 0x49, 0x4F, 0x4F, 0x42, 0x54, 0x4F, 0x53, 0x4F, 0x4E, 0x52, 0x57, 0x57, 0x42,
                    0x53, 0x49, 0x54, 0x4F, 0x49, 0x4F, 0x42, 0x44, 0x4F, 0x49, 0x44, 0x54, 0x44, 0x4E, 0x49, 0x49,
                    0x53, 0x52, 0x53, 0x59, 0x57, 0x49, 0x44, 0x52, 0x40, 0x52, 0x4E, 0x44, 0x40, 0x42, 0x4F, 0x52,
                    0x42, 0x54, 0x4E, 0x49, 0x53, 0x54, 0x49, 0x4F, 0x59, 0x49, 0x53, 0x49, 0x54, 0x4E, 0x53, 0x52,
                    0x4F, 0x54, 0x53, 0x4F, 0x4F, 0x4F, 0x42, 0x4F, 0x53, 0x49, 0x4E, 0x53, 0x54, 0x57, 0x4E, 0x52,
                    0x4E, 0x49, 0x53, 0x49, 0x4E, 0x4F, 0x49, 0x57, 0x53, 0x49, 0x49, 0x42, 0x4F, 0x52, 0x54, 0x42,
                    0x4E, 0x54, 0x59, 0x54, 0x42, 0x4F, 0x44, 0x4E, 0x59, 0x49, 0x49, 0x4F, 0x42, 0x4E, 0x59, 0x49,
                    0x53, 0x54, 0x44, 0x4E, 0x4E, 0x4E, 0x44, 0x4F, 0x59, 0x52, 0x42, 0x54, 0x4E, 0x49, 0x53, 0x54,
                    0x49, 0x4F, 0x59, 0x49, 0x40, 0x53, 0x42, 0x49, 0x4E, 0x4F, 0x4F, 0x4E, 0x4E, 0x40, 0x53, 0x40,
                    0x42, 0x4E, 0x52, 0x44, 0x4E, 0x53, 0x52, 0x57, 0x4E, 0x53, 0x4E, 0x49, 0x53, 0x4E, 0x52, 0x59,
                    0x54, 0x53, 0x52, 0x52, 0x40, 0x4E, 0x54, 0x4A, 0x4A, 0x44, 0x42, 0x4F, 0x42, 0x53, 0x44, 0x49,
                    0x4E, 0x52, 0x54, 0x4A, 0x40, 0x40, 0x42, 0x52, 0x52, 0x53, 0x4F, 0x52, 0x40, 0x54, 0x4F, 0x42,
                    0x54, 0x53, 0x57, 0x54, 0x53, 0x4F, 0x4F, 0x42, 0x54, 0x52, 0x40, 0x4F, 0x44, 0x4F, 0x4E, 0x4F,
                    0x40, 0x44, 0x42, 0x4E, 0x59, 0x57, 0x57, 0x4E, 0x40, 0x49, 0x40, 0x40, 0x49, 0x49, 0x54, 0x40,
                    0x4E, 0x4E, 0x59, 0x40, 0x4F, 0x42, 0x4F, 0x52, 0x4E, 0x49, 0x53, 0x4E, 0x4F, 0x40, 0x49, 0x4F,
                    0x44, 0x40, 0x4E, 0x49, 0x4A, 0x40, 0x4F, 0x44, 0x4F, 0x52, 0x53, 0x4F, 0x49, 0x54, 0x4F, 0x42,
                    0x54, 0x53, 0x57, 0x54, 0x53, 0x4F, 0x4F, 0x42, 0x4E, 0x49, 0x42, 0x40, 0x49, 0x59, 0x4F, 0x49,
                    0x40, 0x42, 0x4E, 0x49, 0x4A, 0x54, 0x59, 0x40, 0x40, 0x40, 0x42, 0x52, 0x4F, 0x4E, 0x53, 0x49,
                    0x49, 0x42, 0x49, 0x52, 0x44, 0x4A, 0x4A, 0x4F, 0x4E, 0x49, 0x54, 0x42, 0x40, 0x40, 0x40, 0x52,
                    0x49, 0x44, 0x40, 0x49, 0x52, 0x42, 0x49, 0x42, 0x40, 0x49, 0x49, 0x53, 0x49, 0x53, 0x40, 0x53,
                    0x42, 0x49, 0x53, 0x53, 0x42, 0x40, 0x53, 0x42, 0x4A, 0x42, 0x53, 0x49, 0x53, 0x4F, 0x4F, 0x4F,
                    0x49, 0x4F, 0x4A, 0x4A, 0x53, 0x49, 0x40, 0x4A, 0x49, 0x40, 0x4E, 0x40, 0x44, 0x40, 0x44, 0x4F,
                    0x4E, 0x42, 0x42, 0x54, 0x54, 0x4F, 0x59, 0x52, 0x40, 0x40, 0x49, 0x49, 0x44, 0x4F, 0x4F, 0x53,
                    0x49, 0x40, 0x44, 0x42, 0x53, 0x49, 0x49, 0x49, 0x44, 0x49, 0x49, 0x44, 0x53, 0x42, 0x54, 0x4F,
                    0x53, 0x49, 0x42, 0x53, 0x4E, 0x42, 0x44, 0x4F, 0x4E, 0x59, 0x54, 0x59, 0x52, 0x49, 0x54, 0x57,
                    0x4A, 0x54, 0x53, 0x49, 0x42, 0x40, 0x40, 0x54, 0x4A, 0x53, 0x42, 0x40, 0x4A, 0x4F, 0x42, 0x4F,
                    0x4F, 0x52, 0x52, 0x44, 0x49, 0x4A, 0x4E, 0x53, 0x40, 0x4F, 0x40, 0x4A, 0x42, 0x59, 0x4F, 0x4A,
                    0x4F, 0x53, 0x49, 0x44, 0x52, 0x49, 0x4A, 0x4F, 0x4F, 0x49, 0x4F, 0x42, 0x4F, 0x4E, 0x54, 0x4F,
                    0x4E, 0x42, 0x4F, 0x53, 0x42, 0x49, 0x42, 0x49, 0x59, 0x54, 0x4E, 0x59, 0x4F, 0x4F, 0x59, 0x53,
                    0x53, 0x4E, 0x40, 0x49, 0x4A, 0x42, 0x49, 0x4A, 0x4E, 0x53, 0x4E, 0x42, 0x42, 0x4A, 0x40, 0x40,
                    0x49, 0x40, 0x52, 0x40, 0x42, 0x4A, 0x49, 0x4F, 0x4F, 0x54, 0x42, 0x53, 0x4E, 0x44, 0x52, 0x40,
                    0x49, 0x49, 0x4E, 0x42, 0x49, 0x42, 0x49, 0x4A, 0x4E, 0x40, 0x49, 0x40, 0x40, 0x4E, 0x53, 0x49,
                    0x54, 0x4E, 0x53, 0x52, 0x4F, 0x54, 0x53, 0x4F, 0x52, 0x49, 0x52, 0x53, 0x4F, 0x4F, 0x53, 0x5A,
                    0x4A, 0x49, 0x4A, 0x42, 0x40, 0x42, 0x49, 0x4F, 0x49, 0x4F, 0x40, 0x40, 0x49, 0x54, 0x59, 0x40,
                    0x44, 0x40, 0x42, 0x4F, 0x49, 0x4F, 0x49, 0x57, 0x42, 0x44, 0x52, 0x52, 0x52, 0x49, 0x53, 0x49,
                    0x57, 0x4F, 0x52, 0x5A, 0x4F, 0x57, 0x4F, 0x4E, 0x4F, 0x49, 0x57, 0x49, 0x53, 0x52, 0x42, 0x54,
                    0x4E, 0x49, 0x53, 0x54, 0x49, 0x4F, 0x59, 0x49, 0x4A, 0x42, 0x59, 0x59, 0x59, 0x53, 0x42, 0x4E,
                    0x49, 0x42, 0x53, 0x40, 0x4A, 0x4A, 0x40, 0x4E, 0x44, 0x42, 0x40, 0x40, 0x4F, 0x59, 0x4E, 0x49,
                    0x44, 0x40, 0x40, 0x40, 0x57, 0x4F, 0x49, 0x57, 0x49, 0x57, 0x54, 0x52, 0x4E, 0x42, 0x42, 0x4F,
                    0x59, 0x49, 0x59, 0x57, 0x40, 0x4F, 0x40, 0x40, 0x49, 0x4E, 0x49, 0x4E, 0x4A, 0x49, 0x4E, 0x49,
                    0x4F, 0x53, 0x57, 0x53, 0x4E, 0x44, 0x42, 0x4E, 0x53, 0x42, 0x59, 0x4E, 0x59, 0x4A, 0x4E, 0x52,
                    0x40, 0x44, 0x54, 0x40, 0x44, 0x42, 0x57, 0x53, 0x42, 0x44, 0x4A, 0x49, 0x52, 0x57, 0x4F, 0x42,
                    0x4E, 0x53, 0x4F, 0x4F, 0x53, 0x49, 0x4E, 0x59, 0x57, 0x53, 0x4A, 0x52, 0x59, 0x52, 0x49, 0x4E,
                    0x42, 0x59, 0x5A, 0x54, 0x4F, 0x49, 0x42, 0x53, 0x49, 0x4A, 0x42, 0x49, 0x42, 0x42, 0x4F, 0x4F,
                    0x49, 0x44, 0x49, 0x4F, 0x4F, 0x54, 0x4E, 0x57, 0x49, 0x49, 0x53, 0x4A, 0x52, 0x59, 0x4F, 0x4F,
                    0x40, 0x49, 0x53, 0x49, 0x49, 0x53, 0x53, 0x49, 0x4F, 0x42, 0x4E, 0x4F, 0x40, 0x4F, 0x40, 0x49,
                    0x57, 0x52, 0x49, 0x4E, 0x53, 0x40, 0x44, 0x4E, 0x49, 0x4F, 0x44, 0x59, 0x54, 0x44, 0x44, 0x42,
                    0x4F, 0x57, 0x4F, 0x57, 0x4F, 0x49, 0x52, 0x49, 0x53, 0x49, 0x4E, 0x42, 0x52, 0x4F, 0x4F, 0x49,
                    0x42, 0x42, 0x52, 0x52, 0x59, 0x49, 0x40, 0x57, 0x4F, 0x49, 0x4F, 0x49, 0x59, 0x59, 0x52, 0x49,
                    0x4F, 0x53, 0x40, 0x52, 0x40, 0x52, 0x57, 0x52, 0x54, 0x4F, 0x49, 0x52, 0x4E, 0x49, 0x49, 0x42,
                    0x4F, 0x4F, 0x4F, 0x59, 0x54, 0x49, 0x4F, 0x42, 0x4F, 0x4E, 0x54, 0x4F, 0x59, 0x4F, 0x40, 0x42,
                    0x49, 0x4A, 0x49, 0x57, 0x52, 0x49, 0x57, 0x4F, 0x4A, 0x52, 0x4F, 0x4A, 0x54, 0x44, 0x44, 0x49,
                    0x52, 0x49, 0x52, 0x49, 0x4F, 0x44, 0x4E, 0x4E, 0x49, 0x4E, 0x53, 0x59, 0x4E, 0x4F, 0x49, 0x85,
                    0x44, 0x40, 0x40, 0x40, 0x53, 0x53, 0x4E, 0x54, 0x54, 0x49, 0x52, 0x4E, 0x4F, 0x4F, 0x40, 0x42,
                    0x44, 0x44, 0x52, 0x59, 0x59, 0x44, 0x52, 0x44, 0x49, 0x44, 0x42, 0x42, 0x53, 0x4E, 0x57, 0x59,
                    0x4A, 0x53, 0x49, 0x52, 0x53, 0x53, 0x4F, 0x44, 0x42, 0x42, 0x53, 0x52, 0x54, 0x40, 0x40, 0x44,
                    0x4E, 0x4A, 0x53, 0x59, 0x53, 0x4F, 0x59, 0x59, 0x4F, 0x4F, 0x4E, 0x52, 0x4E, 0x8C, 0x9B, 0x9B,
                    0x44, 0x57, 0x40, 0x42, 0x4E, 0x49, 0x4E, 0x4F, 0x59, 0x52, 0x53, 0x4E, 0x53, 0x52, 0x4A, 0x40,
                    0x49, 0x54, 0x49, 0x4F, 0x4E, 0x4F, 0x53, 0x4F, 0x4F, 0x49, 0x40, 0x44, 0x44, 0x52, 0x53, 0x4F,
                    0x52, 0x44, 0x49, 0x4F, 0x42, 0x4F, 0x42, 0x49, 0x4A, 0x4E, 0x57, 0x52, 0x49, 0x40, 0x42, 0x49,
                    0x4F, 0x53, 0x42, 0x4F, 0x85, 0xDB, 0xD6, 0x8E, 0x87, 0x49, 0x87, 0xDB, 0xC7, 0x9B, 0x9B, 0x9B,
                    0x49, 0x40, 0x40, 0x4A, 0x42, 0x53, 0x4E, 0x4F, 0x42, 0x57, 0x4A, 0x53, 0x42, 0x4E, 0x44, 0x4F,
                    0x44, 0x49, 0x4E, 0x54, 0x54, 0x52, 0x53, 0x49, 0x4F, 0x57, 0x4F, 0x4F, 0x4F, 0x4E, 0x53, 0x57,
                    0x57, 0x52, 0x57, 0x49, 0x57, 0x54, 0x4F, 0x49, 0x57, 0x53, 0x52, 0x40, 0x85, 0x49, 0x4F, 0x44,
                    0x82, 0x8C, 0x89, 0xC7, 0x9B, 0x9B, 0x9B, 0x9B, 0x9E, 0x9E, 0x9B, 0x9B, 0x9B, 0x9B, 0x9D, 0x9B,
                    0x40, 0x40, 0x52, 0x4F, 0x4E, 0x49, 0x59, 0x49, 0x59, 0x53, 0x54, 0x4F, 0x53, 0x4F, 0x4E, 0x53,
                    0x44, 0x42, 0x59, 0x52, 0x53, 0x40, 0x52, 0x59, 0x49, 0x40, 0x42, 0x4A, 0x49, 0x4F, 0x40, 0x53,
                    0x4E, 0x42, 0x4E, 0x44, 0x4E, 0x4F, 0x40, 0x40, 0x52, 0x53, 0xD9, 0xFB, 0x9B, 0x9B, 0x9B, 0x9D,
                    0x9D, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9D, 0x9D, 0x9D, 0x9D, 0x9B, 0x9B, 0x9B, 0x9D,
                    0x40, 0x40, 0x4F, 0x4E, 0x54, 0x4E, 0x52, 0x53, 0x4E, 0x57, 0x59, 0x49, 0x4E, 0x54, 0x44, 0x52,
                    0x4E, 0x49, 0x52, 0x4F, 0x42, 0x40, 0x40, 0x49, 0x40, 0x53, 0x57, 0x52, 0x40, 0x4F, 0x42, 0x4E,
                    0x54, 0x42, 0x53, 0x52, 0x59, 0x49, 0x42, 0x40, 0x42, 0xE4, 0xFB, 0xDC, 0x94, 0xC3, 0x94, 0x9E,
                    0xE8, 0x9B, 0x9B, 0x9B, 0xDC, 0x97, 0xC3, 0xB5, 0xB6, 0xD9, 0x9B, 0x9D, 0x9B, 0x94, 0xD9, 0x87,
                    0x40, 0x42, 0x52, 0x59, 0x4F, 0x4E, 0x42, 0x40, 0x4F, 0x4F, 0x52, 0x4E, 0x49, 0x49, 0x49, 0x42,
                    0x49, 0x4A, 0x49, 0x49, 0x49, 0x40, 0x40, 0x40, 0x42, 0x4F, 0x52, 0x42, 0x49, 0x42, 0x4F, 0x52,
                    0x53, 0x4F, 0x49, 0x49, 0x59, 0x53, 0x49, 0x42, 0xE4, 0xFB, 0x97, 0x97, 0x49, 0x42, 0x42, 0xB2,
                    0x9D, 0x9D, 0x9D, 0x9E, 0x9A, 0xFB, 0x54, 0x4F, 0x49, 0x49, 0x87, 0x87, 0x53, 0x4F, 0x4E, 0x4A,
                    0x52, 0x49, 0x54, 0x53, 0x52, 0x49, 0x53, 0x5A, 0x5A, 0x40, 0x53, 0x4E, 0x4F, 0x49, 0x4E, 0x53,
                    0x52, 0x42, 0x57, 0x59, 0x49, 0x53, 0x4E, 0x49, 0x49, 0x52, 0x49, 0x49, 0x4F, 0x42, 0x4F, 0x53,
                    0x59, 0x4F, 0x49, 0x49, 0x49, 0x40, 0x40, 0x87, 0xFB, 0x9B, 0xDC, 0x97, 0x53, 0x53, 0x4F, 0x84,
                    0x9D, 0xE8, 0x9E, 0x9E, 0x9F, 0x9E, 0x9D, 0x84, 0x4F, 0x4E, 0x54, 0x57, 0x4F, 0x4F, 0x5A, 0x52,
                    0x42, 0x40, 0x4E, 0x4F, 0x59, 0x4E, 0x49, 0x52, 0x49, 0x42, 0x4E, 0x49, 0x4A, 0x49, 0x42, 0x53,
                    0x44, 0x42, 0x52, 0x59, 0x4E, 0x59, 0x57, 0x4E, 0x4A, 0x4A, 0x42, 0x42, 0x49, 0x40, 0x4E, 0x54,
                    0x59, 0x49, 0x59, 0x4F, 0x40, 0x40, 0x4E, 0x4F, 0x95, 0x9B, 0xE7, 0x9B, 0xC1, 0x59, 0x4E, 0x4F,
                    0xC5, 0x9E, 0x9D, 0x9E, 0x9D, 0x9D, 0xDC, 0x5F, 0x4F, 0x4F, 0x52, 0x52, 0x4F, 0x52, 0x59, 0x59,
                    0x49, 0x53, 0x53, 0x59, 0x5A, 0x4F, 0x40, 0x52, 0x59, 0x57, 0x52, 0x59, 0x42, 0x49, 0x40, 0x54,
                    0x53, 0x42, 0x40, 0x52, 0x52, 0x52, 0x4F, 0x4E, 0x49, 0x49, 0x57, 0x49, 0x4E, 0x59, 0x54, 0x4E,
                    0x52, 0x4E, 0x4E, 0x42, 0x42, 0x4F, 0x49, 0x52, 0x8E, 0x9B, 0x9B, 0x97, 0x8A, 0x40, 0x49, 0x4F,
                    0x4F, 0xB3, 0xE8, 0x9D, 0x9D, 0x9B, 0x94, 0x80, 0x49, 0x52, 0x57, 0x5F, 0x57, 0x57, 0x52, 0x52,
                    0x44, 0x53, 0x4E, 0x49, 0x52, 0x59, 0x52, 0x54, 0x4E, 0x5A, 0x49, 0x53, 0x53, 0x54, 0x40, 0x42,
                    0x4A, 0x53, 0x44, 0x59, 0x57, 0x5A, 0x54, 0x4F, 0x4F, 0x4E, 0x59, 0x49, 0x53, 0x4F, 0x4E, 0x49,
                    0x49, 0x5A, 0x59, 0x5A, 0x40, 0x53, 0x44, 0x52, 0x4F, 0xC7, 0x9B, 0x9B, 0xB5, 0x42, 0x4F, 0x4F,
                    0x49, 0x59, 0x9B, 0x9D, 0x9D, 0x9D, 0x9A, 0x84, 0x49, 0x52, 0x59, 0x57, 0x54, 0x57, 0x5F, 0x57,
                    0x4A, 0x4E, 0x42, 0x42, 0x59, 0x54, 0x49, 0x49, 0x4F, 0x59, 0x4E, 0x42, 0x49, 0x52, 0x42, 0x4E,
                    0x53, 0x4E, 0x52, 0x4F, 0x42, 0x49, 0x40, 0x57, 0x59, 0x53, 0x53, 0x49, 0x42, 0x52, 0x53, 0x49,
                    0x4E, 0x4E, 0x54, 0x5F, 0x4E, 0x44, 0x4F, 0x42, 0x53, 0x87, 0x9B, 0x97, 0xB1, 0x40, 0x4A, 0x4F,
                    0x4E, 0x54, 0x9D, 0x9E, 0x9E, 0x9F, 0x9F, 0x9E, 0xD8, 0x4F, 0x4F, 0x4F, 0x4F, 0x4E, 0x52, 0x57,
                    0x4E, 0x4E, 0x4F, 0x54, 0x54, 0x59, 0x5A, 0x49, 0x44, 0x4A, 0x59, 0x4E, 0x4E, 0x53, 0x53, 0x54,
                    0x4F, 0x4E, 0x42, 0x5A, 0x54, 0x52, 0x44, 0x54, 0x53, 0x42, 0x40, 0x4F, 0x52, 0x53, 0x54, 0x5A,
                    0x4F, 0x42, 0x52, 0x52, 0x54, 0x57, 0x59, 0x54, 0x49, 0xDA, 0x9B, 0x9B, 0x90, 0x42, 0x4A, 0x4F,
                    0x4F, 0x57, 0xCA, 0xE8, 0xE8, 0x9E, 0x9D, 0xE7, 0xC7, 0xB3, 0x52, 0x59, 0x52, 0x4F, 0x4F, 0x4F,
                    0x53, 0x42, 0x4A, 0x59, 0x4E, 0x5A, 0x53, 0x40, 0x49, 0x4E, 0x59, 0x4E, 0x40, 0x4F, 0x54, 0x59,
                    0x42, 0x53, 0x42, 0x49, 0x57, 0x53, 0x54, 0x53, 0x53, 0x4A, 0x49, 0x4F, 0x54, 0x49, 0x4E, 0x57,
                    0x53, 0x4E, 0x4F, 0x4F, 0x42, 0x4E, 0x4E, 0x4E, 0x54, 0x8B, 0x9D, 0x9D, 0x9D, 0x9A, 0x5F, 0x4F,
                    0x49, 0x4F, 0x52, 0xC5, 0x9D, 0x9D, 0x9D, 0x9B, 0x90, 0x80, 0x4F, 0x57, 0x53, 0x52, 0x4F, 0x4F,
                    0x52, 0x52, 0x59, 0x4F, 0x4F, 0x49, 0x59, 0x53, 0x42, 0x4F, 0x4F, 0x42, 0x49, 0x4E, 0x49, 0x54,
                    0x4E, 0x4A, 0x42, 0x49, 0x42, 0x54, 0x54, 0x52, 0x53, 0x54, 0x4F, 0x49, 0x54, 0x42, 0x49, 0x59,
                    0x53, 0x52, 0x49, 0x4F, 0x52, 0x53, 0x59, 0x4F, 0x52, 0x4F, 0x92, 0x9E, 0x9D, 0x9D, 0xDA, 0x4F,
                    0x49, 0x49, 0x4F, 0x80, 0x9B, 0x9D, 0x9D, 0x9B, 0xC4, 0x59, 0x42, 0x4F, 0x52, 0x52, 0x52, 0x4F,
                    0x59, 0x59, 0x59, 0x52, 0x42, 0x42, 0x57, 0x42, 0x4F, 0x44, 0x49, 0x40, 0x42, 0x4F, 0x4A, 0x4E,
                    0x49, 0x4E, 0x54, 0x4E, 0x42, 0x4E, 0x4F, 0x53, 0x4F, 0x52, 0x44, 0x53, 0x57, 0x53, 0x49, 0x4A,
                    0x59, 0x57, 0x4F, 0x54, 0x54, 0x52, 0x52, 0x42, 0x4E, 0x59, 0x4A, 0x8C, 0x9B, 0x9B, 0xB7, 0x42,
                    0x4F, 0x49, 0x4F, 0x4E, 0x9A, 0x9D, 0x9E, 0x9E, 0x9A, 0xC4, 0x52, 0x57, 0x57, 0x52, 0x54, 0x52,
                    0x5A, 0x54, 0x40, 0x42, 0x49, 0x54, 0x53, 0x53, 0x59, 0x4F, 0x54, 0x40, 0x42, 0x54, 0x40, 0x49,
                    0x53, 0x4E, 0x42, 0x44, 0x4F, 0x4F, 0x4E, 0x49, 0x4A, 0x4E, 0x4F, 0x40, 0x49, 0x4F, 0x49, 0x4F,
                    0x59, 0x52, 0x52, 0x57, 0x49, 0x44, 0x42, 0x40, 0x49, 0x4E, 0x42, 0xD7, 0x9B, 0x97, 0x59, 0x40,
                    0x42, 0x49, 0x4F, 0x85, 0xCA, 0xE8, 0x9D, 0x9E, 0x9E, 0x9D, 0x9B, 0x87, 0x52, 0x52, 0x59, 0x52,
                    0x5A, 0x4A, 0x40, 0x4F, 0x57, 0x4E, 0x40, 0x4E, 0x4E, 0x4E, 0x53, 0x59, 0x54, 0x53, 0x44, 0x4E,
                    0x4F, 0x4E, 0x54, 0x40, 0x40, 0x4A, 0x53, 0x4E, 0x54, 0x42, 0x4F, 0x4F, 0x4A, 0x4F, 0x4E, 0x44,
                    0x5A, 0x4E, 0x4F, 0x57, 0x49, 0x49, 0x53, 0x54, 0x52, 0x40, 0x40, 0xD7, 0x9D, 0x9D, 0x9E, 0xD3,
                    0x8C, 0x4F, 0x4E, 0x4F, 0x87, 0x9E, 0x9E, 0x9D, 0x9E, 0x9E, 0xC7, 0x85, 0x52, 0x4F, 0x52, 0x52,
                    0x4A, 0x42, 0x44, 0x53, 0x42, 0x53, 0x54, 0x4F, 0x57, 0x57, 0x40, 0x49, 0x52, 0x49, 0x40, 0x42,
                    0x4A, 0x52, 0x4E, 0x53, 0x53, 0x49, 0x42, 0x44, 0x40, 0x40, 0x4A, 0x52, 0x53, 0x4A, 0x42, 0x49,
                    0x44, 0x40, 0x49, 0x49, 0x42, 0x49, 0x42, 0x4F, 0x49, 0x4E, 0x42, 0x42, 0xD3, 0x9F, 0x9F, 0x9E,
                    0xDA, 0x42, 0x4F, 0x4F, 0x42, 0x84, 0x9D, 0x9B, 0x9B, 0x9B, 0xC1, 0x40, 0x4A, 0x4F, 0x4F, 0x52,
                    0x42, 0x59, 0x57, 0x42, 0x59, 0x53, 0x5A, 0x44, 0x53, 0x42, 0x4E, 0x4F, 0x4E, 0x53, 0x40, 0x4F,
                    0x4E, 0x4F, 0x52, 0x4F, 0x4F, 0x53, 0x4E, 0x53, 0x4E, 0x5A, 0x52, 0x42, 0x4F, 0x5A, 0x4E, 0x53,
                    0x53, 0x44, 0x49, 0x42, 0x44, 0x53, 0x49, 0x44, 0x57, 0x49, 0x49, 0x59, 0x52, 0x96, 0x9D, 0x9B,
                    0xB5, 0x42, 0x4E, 0x4F, 0x4E, 0xC4, 0xE7, 0x9B, 0x9B, 0xC7, 0xBF, 0x42, 0x4A, 0x52, 0x4F, 0x57,
                    0x4E, 0x4A, 0x4E, 0x40, 0x4E, 0x42, 0x44, 0x4F, 0x40, 0x4A, 0x44, 0x42, 0x42, 0x40, 0x52, 0x59,
                    0x42, 0x53, 0x53, 0x49, 0x40, 0x4F, 0x42, 0x4F, 0x5A, 0x59, 0x4F, 0x4F, 0x4E, 0x4F, 0x49, 0x52,
                    0x59, 0x44, 0x4E, 0x57, 0x54, 0x59, 0x42, 0x4E, 0x44, 0x40, 0x42, 0x57, 0x4F, 0x87, 0x97, 0xFB,
                    0xB2, 0x40, 0x49, 0x59, 0xC5, 0x9B, 0x9B, 0x9B, 0xC7, 0x90, 0x81, 0x4A, 0x52, 0x52, 0x52, 0x57,
                    0x59, 0x54, 0x54, 0x53, 0x59, 0x42, 0x49, 0x59, 0x4A, 0x53, 0x4E, 0x40, 0x40, 0x52, 0x4F, 0x49,
                    0x44, 0x42, 0x40, 0x49, 0x4E, 0x49, 0x49, 0x59, 0x5A, 0x57, 0x52, 0x57, 0x54, 0x53, 0x52, 0x53,
                    0x4F, 0x49, 0x44, 0x59, 0x57, 0x4E, 0x4E, 0x42, 0x40, 0x53, 0x59, 0x52, 0x59, 0xC5, 0x9B, 0x9B,
                    0xC1, 0x44, 0x83, 0x9B, 0x9B, 0x9B, 0x9B, 0x94, 0x90, 0x81, 0x49, 0x4F, 0x4F, 0x4F, 0x52, 0x4F,
                    0x49, 0x49, 0x4E, 0x42, 0x49, 0x49, 0x40, 0x54, 0x53, 0x52, 0x59, 0x44, 0x59, 0x49, 0x49, 0x42,
                    0x40, 0x54, 0x42, 0x42, 0x42, 0x49, 0x4F, 0x4A, 0x52, 0x49, 0x42, 0x4F, 0x53, 0x59, 0x42, 0x4E,
                    0x57, 0x49, 0x53, 0x4F, 0x59, 0x59, 0x4F, 0x4E, 0x40, 0x44, 0x42, 0x49, 0x42, 0x96, 0x9E, 0x9D,
                    0x9B, 0xDC, 0x9B, 0x9B, 0x9B, 0x9B, 0xC4, 0x90, 0x4E, 0x42, 0x49, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
                    0x49, 0x40, 0x40, 0x40, 0x4E, 0x57, 0x4F, 0x44, 0x4F, 0x42, 0x42, 0x53, 0x42, 0x40, 0x52, 0x4F,
                    0x4E, 0x52, 0x4A, 0x54, 0x44, 0x52, 0x57, 0x42, 0x49, 0x52, 0x4E, 0x57, 0x57, 0x52, 0x49, 0x49,
                    0x44, 0x42, 0x4E, 0x4F, 0x4F, 0x42, 0x59, 0x52, 0x42, 0x49, 0x40, 0x57, 0x59, 0x49, 0x8E, 0x9D,
                    0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0xC7, 0x9B, 0x53, 0x42, 0x42, 0x49, 0x4F, 0x4F, 0x4E, 0x4F, 0x4F,
                    0x42, 0x40, 0x4F, 0x42, 0x42, 0x53, 0x4A, 0x59, 0x53, 0x40, 0x49, 0x52, 0x4F, 0x4E, 0x4E, 0x4E,
                    0x4F, 0x54, 0x59, 0x4E, 0x4F, 0x53, 0x4E, 0x4E, 0x42, 0x4E, 0x44, 0x4F, 0x59, 0x52, 0x4E, 0x40,
                    0x49, 0x52, 0x49, 0x49, 0x42, 0x49, 0x52, 0x4F, 0x42, 0x4F, 0x52, 0x44, 0x4E, 0x53, 0x4A, 0xD3,
                    0x9D, 0x9E, 0x9E, 0x9E, 0x9E, 0x9F, 0xD7, 0x87, 0x84, 0xB3, 0x8C, 0xD3, 0x96, 0xB4, 0x52, 0x4F,
                    0x52, 0x4A, 0x49, 0x4E, 0x44, 0x53, 0x42, 0x4E, 0x57, 0x49, 0x49, 0x40, 0x53, 0x49, 0x42, 0x4E,
                    0x59, 0x57, 0x4F, 0x57, 0x49, 0x53, 0x53, 0x4E, 0x4F, 0x44, 0x49, 0x42, 0x49, 0x4F, 0x53, 0x54,
                    0x4A, 0x49, 0x53, 0x4E, 0x52, 0x53, 0x4F, 0x53, 0x49, 0x52, 0x53, 0x4A, 0x4F, 0x54, 0x4F, 0x88,
                    0x9D, 0x9E, 0x9D, 0x9D, 0x9D, 0x9A, 0xCB, 0xCE, 0x9E, 0x9B, 0xE8, 0xCE, 0xE8, 0xE8, 0x95, 0x57,
                    0x42, 0x49, 0x52, 0x4E, 0x42, 0x4E, 0x40, 0x42, 0x52, 0x52, 0x52, 0x4E, 0x4E, 0x40, 0x42, 0x53,
                    0x42, 0x4F, 0x4E, 0x49, 0x40, 0x42, 0x52, 0x49, 0x54, 0x57, 0x53, 0x42, 0x42, 0x4E, 0x54, 0x57,
                    0x59, 0x57, 0x4A, 0x53, 0x5A, 0x4E, 0x53, 0x53, 0x4F, 0x57, 0x4A, 0x40, 0x42, 0x49, 0x57, 0x59,
                    0xC7, 0x9B, 0x9B, 0x9B, 0x9D, 0xCE, 0xCE, 0xE8, 0xE8, 0x9D, 0xCE, 0x98, 0x87, 0xD1, 0x9E, 0xCA,
                    0x54, 0x53, 0x59, 0x53, 0x49, 0x4A, 0x59, 0x59, 0x59, 0x54, 0x54, 0x53, 0x4E, 0x4F, 0x52, 0x53,
                    0x53, 0x54, 0x49, 0x49, 0x49, 0x42, 0x54, 0x4F, 0x53, 0x4F, 0x59, 0x4F, 0x53, 0x59, 0x52, 0x53,
                    0x4E, 0x4A, 0x49, 0x52, 0x57, 0x53, 0x57, 0x57, 0x4A, 0x4F, 0x40, 0x42, 0x52, 0x4F, 0x44, 0x53,
                    0x9D, 0x9D, 0x9E, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0xCE, 0xDB, 0x9B, 0xD2, 0xD3,
                    0x53, 0x49, 0x54, 0x54, 0x4F, 0x4F, 0x57, 0x59, 0x59, 0x4E, 0x49, 0x49, 0x40, 0x4E, 0x4F, 0x42,
                    0x4F, 0x53, 0x4E, 0x49, 0x53, 0x53, 0x4F, 0x4F, 0x4E, 0x4F, 0x52, 0x49, 0x53, 0x54, 0x59, 0x52,
                    0x42, 0x42, 0x49, 0x49, 0x4F, 0x4E, 0x54, 0x49, 0x4E, 0x49, 0x4F, 0x4E, 0x54, 0x44, 0x40, 0x4E,
                    0x52, 0x9F, 0x9F, 0xCF, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0xE8, 0x9E,
                    0x4F, 0x44, 0x42, 0x4F, 0x4E, 0x53, 0x4F, 0x4F, 0x4F, 0x4F, 0x52, 0x59, 0x53, 0x4F, 0x53, 0x42,
                    0x49, 0x4E, 0x49, 0x54, 0x40, 0x42, 0x4E, 0x49, 0x57, 0x5F, 0x4E, 0x57, 0x40, 0x40, 0x4E, 0x4F,
                    0x4F, 0x59, 0x49, 0x4E, 0x59, 0x53, 0x53, 0x4F, 0x4E, 0x52, 0x49, 0x4E, 0x49, 0x44, 0x42, 0x49,
                    0x54, 0x85, 0xDE, 0x9F, 0x9F, 0x9F, 0xD2, 0x96, 0xD2, 0xE8, 0xCE, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F,
                    0x4F, 0x49, 0x4E, 0x57, 0x53, 0x4A, 0x59, 0x52, 0x54, 0x4F, 0x4F, 0x40, 0x52, 0x53, 0x44, 0x42,
                    0x4A, 0x53, 0x49, 0x49, 0x49, 0x42, 0x54, 0x42, 0x4E, 0x53, 0x4F, 0x49, 0x49, 0x52, 0x54, 0x42,
                    0x42, 0x4F, 0x42, 0x4F, 0x4E, 0x42, 0x4E, 0x59, 0x52, 0x53, 0x53, 0x52, 0x42, 0x59, 0x54, 0x42,
                    0x42, 0x4F, 0x40, 0xD4, 0x9E, 0x9E, 0xD3, 0x92, 0x92, 0x96, 0x92, 0xD2, 0x9F, 0x9F, 0x9F, 0x9F,
                    0x49, 0x54, 0x52, 0x4F, 0x54, 0x5A, 0x49, 0x57, 0x54, 0x5A, 0x49, 0x40, 0x49, 0x57, 0x42, 0x4F,
                    0x53, 0x53, 0x53, 0x42, 0x53, 0x42, 0x57, 0x4F, 0x49, 0x4A, 0x4F, 0x4E, 0x49, 0x4E, 0x4F, 0x40,
                    0x54, 0x4A, 0x40, 0x4E, 0x40, 0x4E, 0x59, 0x4A, 0x54, 0x4E, 0x53, 0x53, 0x49, 0x59, 0x49, 0x49,
                    0x49, 0x4E, 0x40, 0x4E, 0x9B, 0x9D, 0xD4, 0x92, 0x92, 0x92, 0x92, 0xCA, 0x9E, 0x9F, 0x9F, 0x9F,
                    0x5A, 0x52, 0x57, 0x52, 0x49, 0x53, 0x4E, 0x53, 0x57, 0x54, 0x57, 0x49, 0x52, 0x4E, 0x49, 0x44,
                    0x4E, 0x52, 0x49, 0x59, 0x4F, 0x53, 0x4F, 0x4F, 0x54, 0x40, 0x4A, 0x40, 0x4A, 0x49, 0x4F, 0x4F,
                    0x4F, 0x4F, 0x40, 0x42, 0x40, 0x40, 0x54, 0x53, 0x4A, 0x42, 0x4E, 0x49, 0x53, 0x5A, 0x53, 0x42,
                    0x4E, 0x42, 0x40, 0x4E, 0x8E, 0x9D, 0x9D, 0xD4, 0x96, 0x92, 0x92, 0x92, 0x98, 0x9F, 0x9F, 0x9F,
                    0x40, 0x4E, 0x54, 0x54, 0x53, 0x42, 0x4F, 0x4A, 0x54, 0x40, 0x4F, 0x4E, 0x40, 0x4A, 0x4E, 0x42,
                    0x57, 0x49, 0x52, 0x5A, 0x49, 0x4F, 0x42, 0x49, 0x4E, 0x4E, 0x4E, 0x49, 0x49, 0x52, 0x49, 0x44,
                    0x42, 0x53, 0x4E, 0x49, 0x42, 0x4A, 0x53, 0x49, 0x4F, 0x4E, 0x53, 0x49, 0x49, 0x49, 0x49, 0x40,
                    0x59, 0x59, 0x52, 0x53, 0x82, 0xCA, 0x9D, 0x9D, 0xC6, 0x96, 0x92, 0x92, 0x92, 0xD3, 0xE8, 0x9E,
                    0x4F, 0x53, 0x54, 0x53, 0x49, 0x4F, 0x4E, 0x57, 0x5A, 0x53, 0x4A, 0x57, 0x42, 0x57, 0x49, 0x42,
                    0x49, 0x49, 0x4E, 0x49, 0x49, 0x4E, 0x42, 0x4F, 0x4E, 0x49, 0x40, 0x4E, 0x59, 0x53, 0x4F, 0x42,
                    0x52, 0x53, 0x40, 0x4E, 0x57, 0x54, 0x4A, 0x4F, 0x4A, 0x4A, 0x4E, 0x4F, 0x49, 0x40, 0x42, 0x4F,
                    0x54, 0x4F, 0x40, 0x4E, 0x42, 0x4E, 0x9B, 0x9B, 0xB4, 0x4F, 0x8E, 0x92, 0x92, 0xD3, 0x9E, 0x9B,
                    0x4E, 0x49, 0x4E, 0x4F, 0x4F, 0x57, 0x53, 0x52, 0x42, 0x49, 0x53, 0x54, 0x4F, 0x59, 0x54, 0x53,
                    0x54, 0x59, 0x4F, 0x42, 0x49, 0x42, 0x4E, 0x49, 0x4F, 0x53, 0x52, 0x53, 0x4E, 0x44, 0x59, 0x4F,
                    0x4E, 0x54, 0x49, 0x4E, 0x4E, 0x4F, 0x53, 0x4F, 0x4E, 0x52, 0x54, 0x59, 0x53, 0x4F, 0x59, 0x53,
                    0x49, 0x54, 0x42, 0x53, 0x49, 0x53, 0xC7, 0x9B, 0x9B, 0x84, 0x4A, 0x85, 0x92, 0xD2, 0x9E, 0x9E,
                    0x57, 0x59, 0x42, 0x49, 0x49, 0x49, 0x53, 0x5A, 0x4E, 0x59, 0x59, 0x49, 0x4E, 0x49, 0x57, 0x53,
                    0x49, 0x53, 0x4F, 0x42, 0x49, 0x4F, 0x4E, 0x44, 0x53, 0x42, 0x4F, 0x4A, 0x49, 0x49, 0x52, 0x57,
                    0x42, 0x52, 0x53, 0x53, 0x4F, 0x49, 0x4F, 0x40, 0x4E, 0x49, 0x49, 0x49, 0x53, 0x4F, 0x52, 0x49,
                    0x42, 0x4A, 0x42, 0x42, 0x44, 0x49, 0xD4, 0x9D, 0x9D, 0x9B, 0xDB, 0x4F, 0x57, 0x8F, 0xCE, 0xE8,
                    0x53, 0x4F, 0x4E, 0x4E, 0x4F, 0x49, 0x5A, 0x59, 0x59, 0x59, 0x49, 0x4E, 0x42, 0x49, 0x49, 0x44,
                    0x4F, 0x49, 0x49, 0x40, 0x53, 0x42, 0x49, 0x42, 0x54, 0x57, 0x42, 0x49, 0x44, 0x4F, 0x4F, 0x4E,
                    0x42, 0x53, 0x57, 0x42, 0x42, 0x4E, 0x49, 0x49, 0x4F, 0x49, 0x42, 0x4F, 0x57, 0x57, 0x54, 0x42,
                    0x4E, 0x40, 0x44, 0x44, 0x42, 0x44, 0x40, 0xD4, 0x9D, 0x9D, 0x9B, 0x84, 0x4A, 0x42, 0x4F, 0xD6,
                    0x54, 0x5A, 0x5A, 0x4F, 0x49, 0x5A, 0x4E, 0x52, 0x57, 0x4F, 0x4E, 0x59, 0x49, 0x49, 0x52, 0x42,
                    0x4E, 0x4E, 0x4A, 0x42, 0x49, 0x42, 0x53, 0x4E, 0x49, 0x59, 0x49, 0x4E, 0x53, 0x59, 0x4E, 0x53,
                    0x4E, 0x49, 0x4F, 0x4F, 0x49, 0x49, 0x42, 0x4F, 0x4A, 0x42, 0x40, 0x40, 0x52, 0x4E, 0x40, 0x44,
                    0x57, 0x49, 0x49, 0x42, 0x4E, 0x4E, 0x42, 0x49, 0xCA, 0x9B, 0xFB, 0x59, 0x49, 0x49, 0x42, 0x40,
                    0x49, 0x59, 0x42, 0x49, 0x53, 0x4F, 0x42, 0x49, 0x57, 0x52, 0x52, 0x4E, 0x42, 0x40, 0x44, 0x40,
                    0x52, 0x4E, 0x59, 0x57, 0x4F, 0x57, 0x49, 0x53, 0x49, 0x59, 0x49, 0x4E, 0x53, 0x59, 0x4E, 0x49,
                    0x49, 0x40, 0x5A, 0x4F, 0x42, 0x54, 0x4F, 0x57, 0x53, 0x49, 0x4E, 0x49, 0x42, 0x42, 0x40, 0x53,
                    0x4E, 0x49, 0x4E, 0x42, 0x52, 0x4E, 0x42, 0x49, 0xC5, 0x9B, 0x9B, 0x9B, 0xD7, 0x57, 0x4A, 0x40,
                    0x4F, 0x4F, 0x49, 0x4F, 0x49, 0x4A, 0x53, 0x42, 0x49, 0x4E, 0x4F, 0x42, 0x52, 0x57, 0x40, 0x4F,
                    0x42, 0x53, 0x42, 0x4F, 0x4E, 0x49, 0x40, 0x42, 0x4F, 0x49, 0x49, 0x42, 0x42, 0x49, 0x49, 0x4E,
                    0x4F, 0x53, 0x54, 0x42, 0x42, 0x49, 0x40, 0x40, 0x42, 0x42, 0x4F, 0x42, 0x40, 0x4F, 0x4A, 0x42,
                    0x49, 0x52, 0x53, 0x42, 0x4F, 0x4F, 0x4E, 0x4A, 0x9D, 0x9E, 0x9F, 0x9F, 0x9D, 0x84, 0x40, 0x40,
                    0x4E, 0x4F, 0x49, 0x49, 0x4E, 0x53, 0x53, 0x57, 0x53, 0x4E, 0x49, 0x49, 0x49, 0x53, 0x44, 0x4F,
                    0x59, 0x53, 0x53, 0x54, 0x49, 0x49, 0x49, 0x42, 0x40, 0x4E, 0x54, 0x4E, 0x53, 0x4F, 0x4F, 0x49,
                    0x4F, 0x59, 0x52, 0x4F, 0x4E, 0x4F, 0x42, 0x40, 0x4A, 0x4F, 0x57, 0x49, 0x49, 0x4E, 0x42, 0x44,
                    0x54, 0x53, 0x40, 0x59, 0x44, 0x4F, 0x49, 0x4F, 0x4E, 0x98, 0x9F, 0x9D, 0xFB, 0x59, 0x40, 0xD8,
                    0x42, 0x4E, 0x4F, 0x4E, 0x4F, 0x49, 0x59, 0x4F, 0x52, 0x49, 0x53, 0x4E, 0x42, 0x4F, 0x42, 0x53,
                    0x49, 0x42, 0x4F, 0x53, 0x4E, 0x49, 0x53, 0x53, 0x49, 0x53, 0x40, 0x49, 0x53, 0x4E, 0x49, 0x52,
                    0x53, 0x44, 0x49, 0x4F, 0x59, 0x53, 0x53, 0x59, 0x49, 0x42, 0x49, 0x49, 0x4E, 0x4F, 0x40, 0x49,
                    0x4E, 0x40, 0x42, 0x4E, 0x4F, 0x53, 0x49, 0x4E, 0x4F, 0x4F, 0x8C, 0x9B, 0x9B, 0xB1, 0x4E, 0x97,
                    0x52, 0x53, 0x52, 0x4F, 0x54, 0x54, 0x4E, 0x44, 0x53, 0x42, 0x49, 0x53, 0x4E, 0x42, 0x53, 0x42,
                    0x53, 0x53, 0x53, 0x4F, 0x49, 0x54, 0x40, 0x42, 0x4F, 0x59, 0x42, 0x42, 0x42, 0x49, 0x4A, 0x49,
                    0x59, 0x4E, 0x49, 0x44, 0x53, 0x4E, 0x49, 0x54, 0x59, 0x49, 0x4E, 0x4E, 0x4A, 0x4F, 0x52, 0x44,
                    0x40, 0x40, 0x49, 0x40, 0x49, 0x44, 0x49, 0x49, 0x5A, 0x42, 0xDB, 0x97, 0x97, 0x90, 0x59, 0x97,
                    0x59, 0x53, 0x42, 0x42, 0x49, 0x59, 0x59, 0x4F, 0x4F, 0x52, 0x49, 0x49, 0x4E, 0x42, 0x49, 0x49,
                    0x49, 0x53, 0x4A, 0x42, 0x49, 0x49, 0x49, 0x42, 0x44, 0x53, 0x42, 0x40, 0x4E, 0x4E, 0x40, 0x49,
                    0x4E, 0x53, 0x4A, 0x49, 0x4E, 0x40, 0x4F, 0x40, 0x4F, 0x54, 0x54, 0x49, 0x42, 0x4E, 0x54, 0x4F,
                    0x4F, 0x59, 0x4F, 0x4F, 0x42, 0x40, 0x4E, 0x53, 0x40, 0x4F, 0xC7, 0x9B, 0x9B, 0x97, 0xC6, 0x9B,
                    0x44, 0x4E, 0x4E, 0x4E, 0x42, 0x4E, 0x49, 0x42, 0x5A, 0x53, 0x49, 0x42, 0x40, 0x42, 0x40, 0x54,
                    0x49, 0x4E, 0x49, 0x42, 0x53, 0x42, 0x53, 0x42, 0x49, 0x42, 0x40, 0x44, 0x49, 0x4F, 0x4A, 0x4E,
                    0x40, 0x52, 0x4F, 0x42, 0x49, 0x42, 0x42, 0x40, 0x42, 0x40, 0x4A, 0x40, 0x42, 0x53, 0x53, 0x54,
                    0x59, 0x59, 0x52, 0x49, 0x42, 0x4E, 0x59, 0x59, 0x59, 0x49, 0x85, 0x9D, 0x9D, 0x9B, 0x9B, 0x9B,
                };

                /* update pass table */
                ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * grid_y_66 + grid_x_55] = SURFACE_TYPE_AIR;

                /* extend tile buffer */
                {
                    const int32_t tile_size{ResourceManager_DisableEnhancedGraphics ? (GFX_MAP_TILE_SIZE / 2)
                                                                                    : GFX_MAP_TILE_SIZE};
                    const uint32_t map_cell_count{
                        static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};
                    auto map_tile_buffer =
                        new (std::nothrow) uint8_t[(ResourceManager_MapTileCount + 1u) * tile_size * tile_size];

                    memcpy(map_tile_buffer, ResourceManager_MapTileBuffer,
                           ResourceManager_MapTileCount * tile_size * tile_size);

                    if (ResourceManager_DisableEnhancedGraphics) {
                        uint8_t* destination_address{
                            &map_tile_buffer[ResourceManager_MapTileCount * tile_size * tile_size]};

                        for (int32_t i = 0; i < tile_size; ++i) {
                            for (int32_t j = 0; j < tile_size; ++j) {
                                destination_address[i * tile_size + j] = tile_data[(i * GFX_MAP_TILE_SIZE + j) * 2];
                            }
                        }

                    } else {
                        memcpy(&map_tile_buffer[ResourceManager_MapTileCount * tile_size * tile_size], tile_data,
                               tile_size * tile_size);
                    }

                    delete[] ResourceManager_MapTileBuffer;

                    ResourceManager_MapTileBuffer = map_tile_buffer;
                }

                /* update tile count */
                ++ResourceManager_MapTileCount;

                /* update tile index vector */
                ResourceManager_MapTileIds[ResourceManager_MapSize.x * grid_y_66 + grid_x_55] = tile_411;
            }
        } break;
    }
}

void Resourcemanager_InitLocale() {
    auto locales = SDL_GetPreferredLocales();

    if (locales) {
        for (int i = 0; locales[i].language; i++) {
            if (locales[i].language && locales[i].country) {
                ResourceManager_SystemLocale = std::format("{}-{}", locales[i].language, locales[i].country);

                break;
            }
        }

        SDL_free(locales);
    }
}

void ResourceManager_InitLanguageManager() {
    std::filesystem::path path{"./"};

    if (!ResourceManager_GetBasePath(path)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "M.A.X. Error", "Failed to obtain application base directory.",
                                 nullptr);
        exit(EXIT_FAILURE);
    }

    ResourceManager_LanguageManager = std::make_shared<Language>();
    if (ResourceManager_LanguageManager &&
        ResourceManager_LanguageManager->LoadFile((path / "language.json").lexically_normal().string())) {
        ResourceManager_LanguageManager->SetLanguage(ResourceManager_GetSystemLocale());

    } else {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "M.A.X. Error", "Failed to load `language.json` asset.",
                                 nullptr);
        exit(EXIT_FAILURE);
    }
}

std::string& ResourceManager_GetSystemLocale() { return ResourceManager_SystemLocale; }

void ResourceManager_SetSystemLocale(const std::string locale) { ResourceManager_SystemLocale = locale; }

const std::string& ResourceManager_GetLanguageEntry(const uint32_t key) {
    SDL_assert(ResourceManager_LanguageManager);

    return ResourceManager_LanguageManager->GetEntry(key);
}

void ResourceManager_InitHelpManager() {
    const auto path = ResourceManager_FilePathGameBase / "help.json";

    ResourceManager_HelpManager = std::make_shared<Help>();
    (void)ResourceManager_HelpManager->LoadFile(path.lexically_normal().string());
    ResourceManager_HelpManager->SetLanguage(ResourceManager_GetSystemLocale());
}

std::string ResourceManager_GetHelpEntry(const std::string& section, const int32_t position_x,
                                         const int32_t position_y) {
    std::string result;

    if (ResourceManager_HelpManager) {
        (void)ResourceManager_HelpManager->GetEntry(section, position_x, position_y, result);
    }

    return result;
}

void ResourceManager_InitMissionManager() {
    ResourceManager_MissionManager = std::make_shared<MissionManager>();

    ResourceManager_MissionManager->SetLanguage(ResourceManager_SystemLocale);
}

void ResourceManager_InitUnitAttributes() {
    ResourceManager_UnitAttributes = std::make_shared<Attributes>();

    if (ResourceManager_UnitAttributes && ResourceManager_UnitAttributes->LoadResource()) {
    } else {
        SDL_Log("Warning: Failed to load unit attributes definitions.\n");
    }
}

void ResourceManager_InitClans() {
    ResourceManager_Clans = std::make_shared<Clans>();

    if (ResourceManager_Clans && ResourceManager_Clans->LoadResource()) {
    } else {
        SDL_Log("Warning: Failed to load clans definitions.\n");
    }
}

void ResourceManager_InitSettings() {
    ResourceManager_Settings = std::make_shared<Settings>();

    if (ResourceManager_Settings && ResourceManager_Settings->Load()) {
        // If settings.json didn't exist, save defaults to create it with all settings (including debug)
        std::filesystem::path settings_path = ResourceManager_FilePathGamePref / "settings.json";
        std::error_code ec;
        if (!std::filesystem::exists(settings_path, ec) || ec) {
            if (!ResourceManager_Settings->Save()) {
                SDL_Log("Warning: Failed to create default settings file.\n");
            }
        }
    } else {
        SDL_Log("Warning: Failed to load settings definitions.\n");
    }
}

void ResourceManager_InitUnits() {
    ResourceManager_Units = std::make_unique<Units>();

    if (ResourceManager_Units && ResourceManager_Units->LoadResource()) {
    } else {
        SDL_Log("Warning: Failed to load unit definitions.\n");
    }
}

std::shared_ptr<Clans> ResourceManager_GetClans() { return ResourceManager_Clans; }

std::shared_ptr<Settings> ResourceManager_GetSettings() { return ResourceManager_Settings; }

Unit& ResourceManager_GetUnit(const ResourceID unit_type) {
    SDL_assert(ResourceManager_Units);
    const char* unit_id = ResourceManager_GetResourceID(unit_type);
    Unit* unit = ResourceManager_Units->GetUnit(unit_id);
    SDL_assert(unit);
    return *unit;
}

Units& ResourceManager_GetUnits() { return *ResourceManager_Units; }

void ResourceManager_ResetUnitsSprites() {
    for (auto [unit_id, unit] : ResourceManager_GetUnits()) {
        unit.SetSpriteData(nullptr);
        unit.SetShadowData(nullptr);
    }
}

TeamClanType ResourceManager_GetClanID(const std::string clan_id) {
    return ResourceManager_ClansLutStringKey.at(clan_id);
}

std::string ResourceManager_GetClanID(const TeamClanType clan_id) {
    return ResourceManager_ClansLutEnumKey.at(clan_id);
}

[[nodiscard]] bool ResourceManager_GetUnitAttributes(const uint32_t index, UnitAttributes* const attributes) {
    SDL_assert(ResourceManager_UnitAttributes);
    return ResourceManager_UnitAttributes->GetUnitAttributes(
        ResourceManager_GetResourceID(static_cast<ResourceID>(index)), attributes);
}

std::shared_ptr<MissionManager> ResourceManager_GetMissionManager() { return ResourceManager_MissionManager; }

[[nodiscard]] SDL_mutex* ResourceManager_CreateMutex() {
    if (!ResourceManager_SDLMutexes) {
        ResourceManager_SDLMutexes = std::make_unique<std::vector<SDL_mutex*>>();

        if (!ResourceManager_SDLMutexes) {
            ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
        }
    }

    auto mutex = SDL_CreateMutex();

    if (mutex) {
        ResourceManager_SDLMutexes->push_back(mutex);
    }

    return mutex;
}

void ResourceManager_DestroyMutexes() {
    for (SDL_mutex*& mutex : *ResourceManager_SDLMutexes) {
        SDL_DestroyMutex(mutex);

        mutex = nullptr;
    }

    ResourceManager_SDLMutexes->clear();
}

std::vector<std::filesystem::path> ResourceManager_GetFileList(const std::filesystem::path& folder,
                                                               const std::string& extension) {
    std::vector<std::filesystem::path> file_list;

    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_regular_file()) {
            auto file_extension = entry.path().extension().string();

            file_extension = utf8_tolower_str(file_extension);

            if (file_extension == extension) {
                file_list.push_back(std::filesystem::path(entry.path()).lexically_normal());
            }
        }
    }

    return file_list;
}
