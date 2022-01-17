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

#include "remote.hpp"

#include "gwindow.hpp"
#include "message_manager.hpp"
#include "networkmenu.hpp"
#include "transport.hpp"

unsigned char Remote_GameState;
bool Remote_IsHostMode;
bool Remote_IsNetworkGame;
unsigned int Remote_PauseTimeStamp;
unsigned int Remote_RngSeed;

Transport* Remote_Transport;

void Remote_Init() {}

void Remote_Deinit() {
    if (Remote_Transport) {
        Remote_Transport->Deinit();

        delete Remote_Transport;
        Remote_Transport = nullptr;
    }

    Remote_GameState = 0;
    Remote_IsNetworkGame = false;
}

int Remote_Lobby(bool is_host_mode) {
    WindowInfo* window;
    int result;

    window = gwin_get_window(GWINDOW_MAIN_WINDOW);

    Remote_IsHostMode = is_host_mode;

    CTInfo_Init();
    Remote_Init();

    SDL_assert(Remote_Transport == nullptr);
    Remote_Transport = new (std::nothrow) Transport();

    if (Remote_Transport) {
        if (Remote_Transport->Init()) {
            Remote_IsNetworkGame = NetworkMenu_MenuLoop(Remote_IsHostMode);

            result = Remote_IsNetworkGame;

        } else {
            gwin_load_image(MAINPIC, gwin_get_window(GWINDOW_MAIN_WINDOW), 640, false, true);
            MessageManager_DrawMessage(Remote_Transport->GetError(), 2, 1);

            result = false;
        }

    } else {
        SDL_Log("Unable to initialize network transport layer.\n");
        result = false;
    }

    return result;
}
