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

#include "enums.hpp"
#include "net_address.hpp"
#include "net_node.hpp"
#include "networkmenu.hpp"
#include "unitinfo.hpp"

enum : uint8_t {
    REMOTE_PACKET_00,
    REMOTE_PACKET_01,
    REMOTE_PACKET_02,
    REMOTE_PACKET_03,
    REMOTE_PACKET_04,
    REMOTE_PACKET_05,
    REMOTE_PACKET_06,
    REMOTE_PACKET_07,
    REMOTE_PACKET_08,
    REMOTE_PACKET_09,
    REMOTE_PACKET_10,
    REMOTE_PACKET_11,
    REMOTE_PACKET_12,
    REMOTE_PACKET_13,
    REMOTE_PACKET_14,
    REMOTE_PACKET_15,
    REMOTE_PACKET_16,
    REMOTE_PACKET_17,
    REMOTE_PACKET_18,
    REMOTE_PACKET_19,
    REMOTE_PACKET_20,
    REMOTE_PACKET_21,
    REMOTE_PACKET_22,
    REMOTE_PACKET_23,
    REMOTE_PACKET_24,
    REMOTE_PACKET_25,
    REMOTE_PACKET_26,
    REMOTE_PACKET_27,
    REMOTE_PACKET_28,
    REMOTE_PACKET_29,
    REMOTE_PACKET_30,
    REMOTE_PACKET_31,
    REMOTE_PACKET_32,
    REMOTE_PACKET_33,
    REMOTE_PACKET_34,
    REMOTE_PACKET_35,
    REMOTE_PACKET_36,
    REMOTE_PACKET_37,
    REMOTE_PACKET_38,
    REMOTE_PACKET_39,
    REMOTE_PACKET_40,
    REMOTE_PACKET_41,
    REMOTE_PACKET_42,
    REMOTE_PACKET_43,
    REMOTE_PACKET_44,
    REMOTE_PACKET_45,
    REMOTE_PACKET_46,
    REMOTE_PACKET_47,
    REMOTE_PACKET_48,
    REMOTE_PACKET_49,
    REMOTE_PACKET_50,
    REMOTE_PACKET_51,
    REMOTE_PACKET_52,
};

void Remote_Deinit();
void Remote_SetupConnection();
int32_t Remote_Lobby(bool is_host_mode);
bool Remote_UiProcessNetPackets();
bool Remote_UiProcessTick(bool mode = false);
bool Remote_CheckRestartAfterDesyncEvent();
void Remote_RegisterMenu(NetworkMenu* menu);
void Remote_ProcessNetPackets();
void Remote_AnalyzeDesync();
int32_t Remote_CheckUnpauseEvent();
void Remote_Synchronize(bool mode = false);
void Remote_WaitBeginTurnAcknowledge();
void Remote_WaitEndTurnAcknowledge();
int32_t Remote_SiteSelectMenu();
void Remote_LeaveGame(uint16_t team, bool mode);
bool Remote_CheckDesync(uint16_t team, uint16_t crc_checksum);

void Remote_SendNetPacket_Signal(int32_t packet_type, int32_t team, int32_t parameter);
void Remote_SendNetPacket_05(uint16_t random_number, int32_t team);
void Remote_SendNetPacket_08(UnitInfo* unit);
void Remote_SendNetPacket_09(int32_t team);
void Remote_SendNetPacket_10(int32_t team, ResourceID unit_type);
void Remote_SendNetPacket_11(int32_t team, Complex* complex);
void Remote_SendNetPacket_12(int32_t team);
void Remote_SendNetPacket_13(uint32_t rng_seed);
void Remote_SendNetPacket_14(int32_t team, ResourceID unit_type, int32_t grid_x, int32_t grid_y);
void Remote_SendNetPacket_16(const char* file_name, const char* file_title);
void Remote_SendNetPacket_17();
void Remote_SendNetPacket_18(int32_t sender_team, int32_t addresse_team, const char* message);
void Remote_SendNetPacket_20(UnitInfo* unit);
void Remote_SendNetPacket_28(int32_t node);
void Remote_SendNetPacket_29(int32_t node);
void Remote_SendNetPacket_30(NetAddress& address);
void Remote_SendNetPacket_31(int32_t node);
void Remote_SendNetPacket_32(uint16_t random_number, int32_t team);
void Remote_SendNetPacket_33();
void Remote_SendNetPacket_34();
void Remote_SendNetPacket_35();
void Remote_SendNetPacket_36();
void Remote_SendNetPacket_37();
void Remote_SendNetPacket_38(UnitInfo* unit);
void Remote_SendNetPacket_41(UnitInfo* unit);
void Remote_SendNetPacket_43(UnitInfo* unit, const char* name);
void Remote_SendNetPacket_44(NetAddress& address);
void Remote_SendNetPacket_46(uint16_t team, bool state, uint32_t counter);
void Remote_SendNetPacket_50(UnitInfo* unit);

extern uint8_t Remote_GameState;
extern bool Remote_IsHostMode;
extern bool Remote_IsNetworkGame;
extern bool Remote_UpdatePauseTimer;
extern uint32_t Remote_PauseTimeStamp;
extern uint32_t Remote_RngSeed;
extern NetNodeArray Remote_Nodes;
extern NetNodeArray Remote_Hosts;

#endif /* REMOTE_HPP */
