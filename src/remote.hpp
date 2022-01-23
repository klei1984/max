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

#ifndef REMOTE_HPP
#define REMOTE_HPP

#include "networkmenu.hpp"
#include "resource_manager.hpp"
#include "unitinfo.hpp"

void Remote_Deinit();
void Remote_SetupConnection();
int Remote_Lobby(bool is_host_mode);
bool Remote_sub_CAC94();
bool Remote_sub_C8835(bool mode = false);
bool Remote_CheckRestartAfterDesyncEvent();
void Remote_RegisterMenu(NetworkMenu* menu);
void Remote_ProcessNetPackets();
void Remote_sub_C9753();

void Remote_SendNetPacket_signal(int packet_type, int team, int parameter);
void Remote_SendNetPacket_08(UnitInfo* unit);
void Remote_SendNetPacket_09(int team);
void Remote_SendNetPacket_10(int team, ResourceID unit_type);
void Remote_SendNetPacket_11(int team, Complex* complex);
void Remote_SendNetPacket_12(int team);
void Remote_SendNetPacket_13(unsigned int rng_seed);
void Remote_SendNetPacket_14(int team, ResourceID unit_type, int grid_x, int grid_y);
void Remote_SendNetPacket_16(const char* file_name, const char* file_title);
void Remote_SendNetPacket_17();
void Remote_SendNetPacket_18(int sender_team, int addresse_team);
void Remote_SendNetPacket_20(UnitInfo* unit);
void Remote_SendNetPacket_22(UnitInfo* unit);
void Remote_SendNetPacket_28(int node);
void Remote_SendNetPacket_29(int node);
void Remote_SendNetPacket_31(int node);
void Remote_SendNetPacket_33();
void Remote_SendNetPacket_34();
void Remote_SendNetPacket_35();
void Remote_SendNetPacket_36();
void Remote_SendNetPacket_37();
void Remote_SendNetPacket_38(UnitInfo* unit);
void Remote_SendNetPacket_41(UnitInfo* unit);
void Remote_SendNetPacket_43(UnitInfo* unit, const char* name);
void Remote_SendNetPacket_44();
void Remote_SendNetPacket_50(UnitInfo* unit);

extern unsigned char Remote_GameState;
extern bool Remote_IsHostMode;
extern bool Remote_IsNetworkGame;
extern unsigned int Remote_PauseTimeStamp;
extern unsigned int Remote_RngSeed;

#endif /* REMOTE_HPP */
