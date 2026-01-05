/* Copyright (c) 2022 M.A.X. Port Team
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

#include <SDL3/SDL_main.h>

#ifdef MAX_ENABLE_INSTALLER
#include "installer/installer.hpp"
#endif

#include "menu.hpp"
#include "movie.hpp"
#include "resource_manager.hpp"
#include "sound_manager.hpp"

int main(int argc, char* argv[]) {
#ifdef MAX_ENABLE_INSTALLER
    Installer::MaxInstaller installer;
    Installer::ExitCode result = installer.Run(argc, argv);

    if (result != Installer::ExitCode::Skipped) {
        return (result) == Installer::ExitCode::Success ? EXIT_SUCCESS : EXIT_FAILURE;
    }
#endif

    try {
        ResourceManager_InitResources();

        if (Movie_PlayIntro()) {
            menu_draw_logo(ILOGO, 3000);
        }

        ResourceManager_GetSoundManager().PlayMusic(MAIN_MSC, false);
        menu_draw_logo(MLOGO, 3000);

        main_menu();

    } catch (std::exception& e) {
        SDL_Log("\n%s\n", (std::string("Unhandled exception: ") + e.what()).c_str());

        ResourceManager_Exit();
    }

    return EXIT_SUCCESS;
}
