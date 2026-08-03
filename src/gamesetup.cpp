/* Copyright (c) 2026 M.A.X. Port Team
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

#include "gamesetup.hpp"

#include <SDL3/SDL.h>

#include <atomic>
#include <cctype>
#include <string>

#include "resource_manager.hpp"
#include "settings.hpp"

enum {
    GAMESETUP_DIALOG_PENDING,
    GAMESETUP_DIALOG_SELECTED,
    GAMESETUP_DIALOG_CANCELLED,
    GAMESETUP_DIALOG_FAILED,
};

enum {
    GAMESETUP_BUTTON_SELECT,
    GAMESETUP_BUTTON_QUIT,
};

struct GameSetup_FolderDialogContext {
    std::atomic<int32_t> State{GAMESETUP_DIALOG_PENDING};
    std::filesystem::path Folder;
};

/* The folder picker is asynchronous, and SDL may invoke the callback from a different thread. The
 * file list is only valid for the duration of the callback, so the selection is copied here, before
 * the state is published with release semantics for the polling main thread.
 */
static void SDLCALL GameSetup_FolderDialogCallback(void* userdata, const char* const* filelist, int filter) {
    auto context = static_cast<struct GameSetup_FolderDialogContext*>(userdata);

    if (!filelist) {
        SDL_Log("Folder dialog failed: %s\n", SDL_GetError());
        context->State.store(GAMESETUP_DIALOG_FAILED, std::memory_order_release);

    } else if (!filelist[0]) {
        context->State.store(GAMESETUP_DIALOG_CANCELLED, std::memory_order_release);

    } else {
        context->Folder = std::filesystem::path(filelist[0]).lexically_normal();
        context->State.store(GAMESETUP_DIALOG_SELECTED, std::memory_order_release);
    }
}

/* Opens the native folder picker and waits for the player's choice, pumping SDL events on the main
 * thread meanwhile, as several dialog backends deliver their results through the event loop.
 */
static int32_t GameSetup_SelectFolder(const std::filesystem::path& default_location,
                                      std::filesystem::path& folder) {
    struct GameSetup_FolderDialogContext context;
    std::error_code ec;
    std::string location;

    if (!default_location.empty() && std::filesystem::exists(default_location, ec) && !ec) {
        location = default_location.string();
    }

    SDL_ShowOpenFolderDialog(&GameSetup_FolderDialogCallback, &context, nullptr,
                             location.empty() ? nullptr : location.c_str(), false);

    while (context.State.load(std::memory_order_acquire) == GAMESETUP_DIALOG_PENDING) {
        SDL_PumpEvents();
        SDL_Delay(50);
    }

    folder = context.Folder;

    return context.State.load(std::memory_order_acquire);
}

/* Copies save files found next to the original game assets into the preferences folder, without
 * overwriting anything. Existing files always win, so a reinstall pointing at an old game data
 * folder cannot clobber newer saves, which mirrors the CopyFilesNoClobber behavior of the Windows
 * installer. The copy runs only when a game assets folder is newly accepted, so deliberately
 * deleted saves are not resurrected on later game starts either.
 */
static void GameSetup_MigrateSaveFiles(const std::filesystem::path& game_data_path) {
    static const char* const extensions[] = {".DTA", ".HOT", ".MLT", ".BAK"};
    std::error_code ec;

    if (std::filesystem::equivalent(game_data_path, ResourceManager_FilePathGamePref, ec) && !ec) {
        return;
    }

    ec.clear();

    for (const auto& entry : std::filesystem::directory_iterator(game_data_path, ec)) {
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }

        auto extension = entry.path().extension().string();

        for (auto& character : extension) {
            character = std::toupper(static_cast<unsigned char>(character));
        }

        for (const auto* candidate : extensions) {
            if (extension == candidate) {
                std::error_code copy_ec;

                if (std::filesystem::copy_file(entry.path(),
                                               ResourceManager_FilePathGamePref / entry.path().filename(),
                                               std::filesystem::copy_options::skip_existing, copy_ec)) {
                    SDL_Log("Migrated save file %s\n", entry.path().filename().string().c_str());
                }

                break;
            }
        }
    }
}

bool GameSetup_IsGameDataFolder(const std::filesystem::path& path) {
    std::error_code ec;

    return (std::filesystem::exists(path / "MAX.RES", ec) && !ec) ||
           (std::filesystem::exists(path / "max.res", ec) && !ec);
}

bool GameSetup_Run(std::filesystem::path& game_data_path) {
    const SDL_MessageBoxButtonData buttons[] = {
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, GAMESETUP_BUTTON_QUIT, _(b3eb)},
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, GAMESETUP_BUTTON_SELECT, _(5653)},
    };
    std::filesystem::path default_location =
        std::filesystem::path(ResourceManager_GetSettings()->GetStringValue("game_data")).lexically_normal();

    for (;;) {
        const SDL_MessageBoxData message_box{SDL_MESSAGEBOX_INFORMATION,
                                             nullptr,
                                             _(a003),
                                             _(a004),
                                             SDL_arraysize(buttons),
                                             buttons,
                                             nullptr};
        int32_t button_id{GAMESETUP_BUTTON_QUIT};

        if (!SDL_ShowMessageBox(&message_box, &button_id)) {
            SDL_Log("%s\n", SDL_GetError());

            return false;
        }

        if (button_id != GAMESETUP_BUTTON_SELECT) {
            return false;
        }

        std::filesystem::path folder;
        const int32_t state = GameSetup_SelectFolder(default_location, folder);

        if (state == GAMESETUP_DIALOG_FAILED) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _(cf05), SDL_GetError(), nullptr);

            return false;
        }

        if (state == GAMESETUP_DIALOG_CANCELLED) {
            continue;
        }

        /* The MAX subfolder of the selected folder is accepted too: original CD-ROMs and existing
         * installations keep MAX.RES in a MAX subfolder, and the Windows installer accepts both.
         */
        if (!GameSetup_IsGameDataFolder(folder)) {
            if (GameSetup_IsGameDataFolder(folder / "MAX")) {
                folder /= "MAX";

            } else {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _(a003), _(a005), nullptr);
                default_location = folder;

                continue;
            }
        }

        ResourceManager_GetSettings()->SetStringValue("game_data", folder.string());

        if (!ResourceManager_GetSettings()->Save()) {
            SDL_Log("Failed to persist the game data folder into the settings.\n");
        }

        GameSetup_MigrateSaveFiles(folder);

        game_data_path = folder;

        return true;
    }
}
