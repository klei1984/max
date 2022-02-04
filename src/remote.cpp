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

#include "message_manager.hpp"
#include "networkmenu.hpp"
#include "transport.hpp"
#include "window_manager.hpp"

unsigned char Remote_GameState;
bool Remote_IsHostMode;
bool Remote_IsNetworkGame;
bool Remote_UnpauseGameEvent;
unsigned int Remote_PauseTimeStamp;
unsigned int Remote_TimeoutTimeStamp;
unsigned int Remote_RngSeed;

Transport* Remote_Transport;

NetworkMenu* Remote_NetworkMenu;

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

    window = WindowManager_GetWindow(GWINDOW_MAIN_WINDOW);

    Remote_IsHostMode = is_host_mode;

    /// \todo Implement missing stuff
    // CTInfo_Init();
    Remote_Init();

    SDL_assert(Remote_Transport == nullptr);
    Remote_Transport = new (std::nothrow) Transport();

    if (Remote_Transport) {
        if (Remote_Transport->Init()) {
            Remote_IsNetworkGame = NetworkMenu_MenuLoop(Remote_IsHostMode);

            result = Remote_IsNetworkGame;

        } else {
            WindowManager_LoadImage(MAINPIC, WindowManager_GetWindow(GWINDOW_MAIN_WINDOW), 640, false, true);
            MessageManager_DrawMessage(Remote_Transport->GetError(), 2, 1);

            result = false;
        }

    } else {
        SDL_Log("Unable to initialize network transport layer.\n");
        result = false;
    }

    return result;
}

void Remote_SetupConnection() {}

bool Remote_sub_CAC94() { return false; }

bool Remote_sub_C8835(bool mode) { return false; }

bool Remote_CheckRestartAfterDesyncEvent() { return false; }

void Remote_RegisterMenu(NetworkMenu* menu) {}

void Remote_ProcessNetPackets() {}

void Remote_sub_C9753() {}

int Remote_CheckUnpauseEvent() {
    Remote_UnpauseGameEvent = false;

    Remote_ProcessNetPackets();
    Remote_TimeoutTimeStamp = timer_get_stamp32();

    return Remote_UnpauseGameEvent;
}

void Remote_SendNetPacket_signal(int packet_type, int team, int parameter) {}

void Remote_SendNetPacket_08(UnitInfo* unit) {}

void Remote_SendNetPacket_09(int team) {}

void Remote_SendNetPacket_10(int team, ResourceID unit_type) {}

void Remote_SendNetPacket_11(int team, Complex* complex) {}

void Remote_SendNetPacket_12(int team) {}

void Remote_SendNetPacket_13(unsigned int rng_seed) {}

void Remote_SendNetPacket_14(int team, ResourceID unit_type, int grid_x, int grid_y) {}

void Remote_SendNetPacket_16(const char* file_name, const char* file_title) {}

void Remote_SendNetPacket_17() {}

void Remote_SendNetPacket_18(int sender_team, int addresse_team) {}

void Remote_SendNetPacket_20(UnitInfo* unit) {}

void Remote_SendNetPacket_22(UnitInfo* unit) {}

void Remote_SendNetPacket_28(int node) {}

void Remote_SendNetPacket_29(int node) {}

void Remote_SendNetPacket_31(int node) {}

void Remote_SendNetPacket_33() {}

void Remote_SendNetPacket_34() {}

void Remote_SendNetPacket_35() {}

void Remote_SendNetPacket_36() {}

void Remote_SendNetPacket_37() {}

void Remote_SendNetPacket_38(UnitInfo* unit) {}

void Remote_SendNetPacket_41(UnitInfo* unit) {}

void Remote_SendNetPacket_43(UnitInfo* unit, const char* name) {}

void Remote_SendNetPacket_44() {}

void Remote_SendNetPacket_50(UnitInfo* unit) {}
