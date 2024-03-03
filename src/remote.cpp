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

#include "access.hpp"
#include "ailog.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "hash.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "networkmenu.hpp"
#include "sound_manager.hpp"
#include "ticktimer.hpp"
#include "transport.hpp"
#include "unitevents.hpp"
#include "units_manager.hpp"
#include "version.hpp"
#include "window_manager.hpp"

#define REMOTE_RESPONSE_TIMEOUT 30000
#define REMOTE_PING_TIME_PERIOD 3000

enum {
    REMOTE_UNICAST,
    REMOTE_MULTICAST,
    REMOTE_BROADCAST,
};

enum {
    REMOTE_RECEIVED_ADDRESS,
};

struct OrderProcessor {
    void (*WritePacket)(UnitInfo* unit, NetPacket& packet);
    void (*ReadPacket)(UnitInfo* unit, NetPacket& packet);
};

struct Packet23Data {
    uint8_t orders;
    uint8_t state;
    uint8_t prior_orders;
    uint8_t prior_state;
    bool disabled_reaction_fire;
    uint16_t parent_unit_id;
    int16_t target_grid_x;
    int16_t target_grid_y;
    uint16_t enemy_unit_id;
    uint8_t total_mining;
    uint8_t raw_mining;
    uint8_t fuel_mining;
    uint8_t gold_mining;
    uint8_t build_time;
    uint16_t build_rate;
    uint16_t unit_type;
    uint8_t unit_id;
    int16_t grid_x;
    int16_t grid_y;
    uint8_t team;
    uint8_t hits;
    int8_t speed;
    uint8_t shots;
    int16_t storage;
    uint8_t ammo;
};

NetNodeArray Remote_Nodes;

NetNodeArray Remote_Hosts;
NetNodeArray Remote_Clients;

uint8_t Remote_GameState;
uint8_t Remote_RemotePlayerCount;
bool Remote_IsHostMode;
bool Remote_IsNetworkGame;
bool Remote_UpdatePauseTimer;
bool Remote_UnpauseGameEvent;
bool Remote_SendSynchFrame;
uint32_t Remote_PauseTimeStamp;
uint32_t Remote_TimeoutTimeStamp;
uint32_t Remote_RngSeed;
Transport* Remote_Transport;
NetworkMenu* Remote_NetworkMenu;

static uint8_t Remote_FrameSyncCounter2;
static uint8_t Remote_FrameSyncCounter[TRANSPORT_MAX_TEAM_COUNT];
static uint8_t Remote_FrameSyncCounter2values[TRANSPORT_MAX_TEAM_COUNT];
static uint8_t Remote_LeaveGameRequestId[TRANSPORT_MAX_TEAM_COUNT];
static uint8_t Remote_TurnIndices[TRANSPORT_MAX_TEAM_COUNT];
static uint8_t Remote_NextTurnIndices[TRANSPORT_MAX_TEAM_COUNT];
static uint16_t Remote_TeamDataCrc16[TRANSPORT_MAX_TEAM_COUNT];
static uint16_t Remote_P23_UnitId;
static NetPacket Remote_P23_Packet;
static bool Remote_P24_Signals[TRANSPORT_MAX_TEAM_COUNT];
static bool Remote_P49_Signal;
static bool Remote_P51_Signal;

static OrderProcessor Remote_OrderProcessors[ORDER_COUNT_MAX];

static uint16_t Remote_GenerateEntityId();
static void Remote_UpdateEntityId(NetAddress& address, uint16_t entity_id);

static void Remote_WriteGameSettings(NetPacket& packet);
static void Remote_ReadGameSettings(NetPacket& packet);

static void Remote_OrderProcessor1_Write(UnitInfo* unit, NetPacket& packet);
static void Remote_OrderProcessor1_Read(UnitInfo* unit, NetPacket& packet);
static void Remote_OrderProcessor2_Write(UnitInfo* unit, NetPacket& packet);
static void Remote_OrderProcessor2_Read(UnitInfo* unit, NetPacket& packet);
static void Remote_OrderProcessor3_Write(UnitInfo* unit, NetPacket& packet);
static void Remote_OrderProcessor3_Read(UnitInfo* unit, NetPacket& packet);
static void Remote_OrderProcessor4_Write(UnitInfo* unit, NetPacket& packet);
static void Remote_OrderProcessor4_Read(UnitInfo* unit, NetPacket& packet);
static void Remote_OrderProcessor5_Write(UnitInfo* unit, NetPacket& packet);
static void Remote_OrderProcessor5_Read(UnitInfo* unit, NetPacket& packet);

static int32_t Remote_SetupPlayers();
static void Remote_ResponseTimeout(uint16_t team, bool mode);
static bool Remote_AnalyzeDesyncHost(SmartList<UnitInfo>& units);
static void Remote_CreateNetPacket_23(UnitInfo* unit, NetPacket& packet);
static void Remote_ProcessNetPacket_23(struct Packet23Data& data, NetPacket& packet);
static void Remote_NetErrorUnknownUnit(uint16_t unit_id);
static void Remote_NetErrorUnitInfoOutOfSync(UnitInfo* unit, NetPacket& packet);

static void Remote_SendNetPacket_07(uint16_t team, bool mode);
static void Remote_SendNetPacket_23(UnitInfo* unit);
static void Remote_SendNetPacket_45(uint16_t team, uint8_t next_turn_index, uint16_t crc_checksum);

static void Remote_ReceiveNetPacket_00(NetPacket& packet);
static void Remote_ReceiveNetPacket_01(NetPacket& packet);
static void Remote_ReceiveNetPacket_05(NetPacket& packet);
static void Remote_ReceiveNetPacket_06(NetPacket& packet);
static void Remote_ReceiveNetPacket_07(NetPacket& packet);
static void Remote_ReceiveNetPacket_08(NetPacket& packet);
static void Remote_ReceiveNetPacket_09(NetPacket& packet);
static void Remote_ReceiveNetPacket_10(NetPacket& packet);
static void Remote_ReceiveNetPacket_11(NetPacket& packet);
static void Remote_ReceiveNetPacket_12(NetPacket& packet);
static void Remote_ReceiveNetPacket_13(NetPacket& packet);
static void Remote_ReceiveNetPacket_14(NetPacket& packet);
static void Remote_ReceiveNetPacket_16(NetPacket& packet);
static void Remote_ReceiveNetPacket_17(NetPacket& packet);
static void Remote_ReceiveNetPacket_18(NetPacket& packet);
static void Remote_ReceiveNetPacket_20(NetPacket& packet);
static void Remote_ReceiveNetPacket_21(NetPacket& packet);
static void Remote_ReceiveNetPacket_22(NetPacket& packet);
static void Remote_ReceiveNetPacket_23(NetPacket& packet);
static void Remote_ReceiveNetPacket_24(NetPacket& packet);
static void Remote_ReceiveNetPacket_26(NetPacket& packet);
static void Remote_ReceiveNetPacket_28(NetPacket& packet);
static void Remote_ReceiveNetPacket_29(NetPacket& packet);
static void Remote_ReceiveNetPacket_30(NetPacket& packet);
static void Remote_ReceiveNetPacket_31(NetPacket& packet);
static void Remote_ReceiveNetPacket_32(NetPacket& packet);
static void Remote_ReceiveNetPacket_33(NetPacket& packet);
static void Remote_ReceiveNetPacket_34(NetPacket& packet);
static void Remote_ReceiveNetPacket_35(NetPacket& packet);
static void Remote_ReceiveNetPacket_36(NetPacket& packet);
static void Remote_ReceiveNetPacket_37(NetPacket& packet);
static void Remote_ReceiveNetPacket_38(NetPacket& packet);
static void Remote_ReceiveNetPacket_39(NetPacket& packet);
static void Remote_ReceiveNetPacket_40(NetPacket& packet);
static void Remote_ReceiveNetPacket_41(NetPacket& packet);
static void Remote_ReceiveNetPacket_42(NetPacket& packet);
static void Remote_ReceiveNetPacket_43(NetPacket& packet);
static void Remote_ReceiveNetPacket_44(NetPacket& packet);
static void Remote_ReceiveNetPacket_45(NetPacket& packet);
static void Remote_ReceiveNetPacket_46(NetPacket& packet);
static void Remote_ReceiveNetPacket_48(NetPacket& packet);
static void Remote_ReceiveNetPacket_49(NetPacket& packet);
static void Remote_ReceiveNetPacket_50(NetPacket& packet);
static void Remote_ReceiveNetPacket_51(NetPacket& packet);
static void Remote_ReceiveNetPacket_52(NetPacket& packet);

uint16_t Remote_GenerateEntityId() {
    uint16_t new_entity_id;

    for (;;) {
        new_entity_id = ((dos_rand() * 31991) >> 15) + 10;

        if (!Remote_Nodes.Find(new_entity_id)) {
            break;
        }
    }

    return new_entity_id;
}

void Remote_UpdateEntityId(NetAddress& address, uint16_t entity_id) {
    for (int32_t i = 0; i < Remote_Nodes.GetCount(); ++i) {
        if (Remote_Nodes[i]->address == address) {
            Remote_Nodes[i]->entity_id = entity_id;
        }
    }
}

void Remote_WriteGameSettings(NetPacket& packet) {
    packet << Remote_NetworkMenu->world_name;
    packet << Remote_NetworkMenu->ini_world_index;
    packet << Remote_NetworkMenu->ini_play_mode;
    packet << Remote_NetworkMenu->ini_timer;
    packet << Remote_NetworkMenu->ini_endturn;
    packet << Remote_NetworkMenu->ini_opponent;
    packet << Remote_NetworkMenu->ini_start_gold;
    packet << Remote_NetworkMenu->ini_raw_resource;
    packet << Remote_NetworkMenu->ini_fuel_resource;
    packet << Remote_NetworkMenu->ini_gold_resource;
    packet << Remote_NetworkMenu->ini_alien_derelicts;
    packet << Remote_NetworkMenu->ini_victory_type;
    packet << Remote_NetworkMenu->ini_victory_limit;
    packet << Remote_NetworkMenu->is_map_changed;
    packet << Remote_NetworkMenu->is_multi_scenario;
    packet << Remote_NetworkMenu->default_team_names;
    packet << Remote_NetworkMenu->multi_scenario_id;
    packet << Remote_NetworkMenu->rng_seed;
}

void Remote_ReadGameSettings(NetPacket& packet) {
    packet >> Remote_NetworkMenu->world_name;
    packet >> Remote_NetworkMenu->ini_world_index;
    packet >> Remote_NetworkMenu->ini_play_mode;
    packet >> Remote_NetworkMenu->ini_timer;
    packet >> Remote_NetworkMenu->ini_endturn;
    packet >> Remote_NetworkMenu->ini_opponent;
    packet >> Remote_NetworkMenu->ini_start_gold;
    packet >> Remote_NetworkMenu->ini_raw_resource;
    packet >> Remote_NetworkMenu->ini_fuel_resource;
    packet >> Remote_NetworkMenu->ini_gold_resource;
    packet >> Remote_NetworkMenu->ini_alien_derelicts;
    packet >> Remote_NetworkMenu->ini_victory_type;
    packet >> Remote_NetworkMenu->ini_victory_limit;
    packet >> Remote_NetworkMenu->is_map_changed;
    packet >> Remote_NetworkMenu->is_multi_scenario;
    packet >> Remote_NetworkMenu->default_team_names;
    packet >> Remote_NetworkMenu->multi_scenario_id;
    packet >> Remote_NetworkMenu->rng_seed;
}

void Remote_OrderProcessor1_Write(UnitInfo* unit, NetPacket& packet) {
    packet << unit->orders;
    packet << unit->state;
    packet << unit->prior_orders;
    packet << unit->prior_state;
    packet << unit->disabled_reaction_fire;
}

void Remote_OrderProcessor1_Read(UnitInfo* unit, NetPacket& packet) {
    UnitsManager_NewOrderWhileScaling(unit);

    packet >> unit->orders;
    packet >> unit->state;
    packet >> unit->prior_orders;
    packet >> unit->prior_state;
    packet >> unit->disabled_reaction_fire;
}

void Remote_OrderProcessor2_Write(UnitInfo* unit, NetPacket& packet) {
    Remote_OrderProcessor1_Write(unit, packet);

    if (unit->GetParent()) {
        packet << unit->GetParent()->GetId();

    } else {
        packet << static_cast<uint16_t>(0);
    }
}

void Remote_OrderProcessor2_Read(UnitInfo* unit, NetPacket& packet) {
    Remote_OrderProcessor1_Read(unit, packet);

    uint16_t unit_id;

    packet >> unit_id;

    if (unit_id) {
        unit->SetParent(Hash_UnitHash[unit_id]);

    } else {
        unit->SetParent(nullptr);
    }
}

void Remote_OrderProcessor3_Write(UnitInfo* unit, NetPacket& packet) {
    Remote_OrderProcessor2_Write(unit, packet);

    packet << unit->target_grid_x;
    packet << unit->target_grid_y;

    if (unit->GetEnemy()) {
        packet << unit->GetEnemy()->GetId();

    } else {
        packet << static_cast<uint16_t>(0);
    }
}

void Remote_OrderProcessor3_Read(UnitInfo* unit, NetPacket& packet) {
    Remote_OrderProcessor2_Read(unit, packet);

    packet >> unit->target_grid_x;
    packet >> unit->target_grid_y;
    uint16_t unit_id;

    packet >> unit_id;

    if (unit_id) {
        unit->SetEnemy(Hash_UnitHash[unit_id]);

    } else {
        unit->SetEnemy(nullptr);
    }
}

void Remote_OrderProcessor4_Write(UnitInfo* unit, NetPacket& packet) {
    SmartObjectArray<ResourceID> build_queue = unit->GetBuildList();

    Remote_OrderProcessor3_Write(unit, packet);

    packet << unit->GetRepeatBuildState();
    packet << unit->build_time;
    packet << static_cast<uint16_t>(unit->GetBuildRate());

    uint16_t unit_count = build_queue.GetCount();

    packet << unit_count;

    for (int32_t i = 0; i < unit_count; ++i) {
        packet << *build_queue[i];
    }
}

void Remote_OrderProcessor4_Read(UnitInfo* unit, NetPacket& packet) {
    SmartObjectArray<ResourceID> build_queue = unit->GetBuildList();
    bool repeat_build;
    uint16_t build_rate;
    uint16_t unit_count;
    ResourceID unit_type;

    Remote_OrderProcessor3_Read(unit, packet);

    packet >> repeat_build;
    unit->SetRepeatBuildState(repeat_build);
    packet >> unit->build_time;
    packet >> build_rate;
    unit->SetBuildRate(build_rate);
    packet >> unit_count;

    build_queue.Clear();

    for (int32_t i = 0; i < unit_count; ++i) {
        packet >> unit_type;
        build_queue.PushBack(&unit_type);
    }
}

void Remote_OrderProcessor5_Write(UnitInfo* unit, NetPacket& packet) {
    Remote_OrderProcessor3_Write(unit, packet);

    packet << unit->total_mining;
    packet << unit->raw_mining;
    packet << unit->fuel_mining;
    packet << unit->gold_mining;
}

void Remote_OrderProcessor5_Read(UnitInfo* unit, NetPacket& packet) {
    Remote_OrderProcessor3_Read(unit, packet);

    packet >> unit->total_mining;
    packet >> unit->raw_mining;
    packet >> unit->fuel_mining;
    packet >> unit->gold_mining;
}

int32_t Remote_SetupPlayers() {
    GameManager_PlayerTeam = Remote_NetworkMenu->player_team;

    ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + GameManager_PlayerTeam), TEAM_TYPE_PLAYER);

    UnitsManager_TeamInfo[GameManager_PlayerTeam].team_type = TEAM_TYPE_PLAYER;

    ini_config.SetStringValue(INI_PLAYER_NAME, Remote_NetworkMenu->player_name);

    int32_t player_clan = ini_get_setting(INI_PLAYER_CLAN);

    ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_CLAN + GameManager_PlayerTeam), player_clan);

    UnitsManager_TeamInfo[GameManager_PlayerTeam].team_clan = player_clan;

    int32_t remote_player_count = 0;

    for (int32_t team = 0; team < TRANSPORT_MAX_TEAM_COUNT; ++team) {
        if (Remote_NetworkMenu->team_nodes[team] > 4) {
            ini_config.SetStringValue(INI_RED_TEAM_NAME, Remote_NetworkMenu->team_names[team]);

            if (Remote_NetworkMenu->team_nodes[team] != Remote_NetworkMenu->player_node) {
                ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team), TEAM_TYPE_REMOTE);
                UnitsManager_TeamInfo[team].team_type = TEAM_TYPE_REMOTE;

                ++remote_player_count;
            }

        } else {
            ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team), TEAM_TYPE_NONE);
        }
    }

    Remote_RemotePlayerCount = remote_player_count;

    return remote_player_count;
}

void Remote_ResponseTimeout(uint16_t team, bool mode) {
    char message[100];

    ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team), TEAM_TYPE_ELIMINATED);

    UnitsManager_TeamInfo[team].team_type = TEAM_TYPE_ELIMINATED;

    if (mode) {
        sprintf(message, _(62a6), menu_team_names[team]);

    } else {
        if (GameManager_WrapUpGame) {
            return;
        }

        sprintf(message, _(9a28), menu_team_names[team]);

        SoundManager_PlayVoice(static_cast<ResourceID>(V_M025 + team * 4), static_cast<ResourceID>(V_F026 + team * 4));
    }

    if (GameManager_GameState != GAME_STATE_3_MAIN_MENU) {
        MessageManager_DrawMessage(message, 1, 1, true);
    }

    for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
        if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_REMOTE) {
            ++Remote_RngSeed;

            return;
        }
    }

    ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_CUSTOM);

    Remote_IsNetworkGame = false;
}

void Remote_Init() {
    Remote_GameState = 0;
    Remote_RngSeed = 0;
    GameManager_PlayerTeam = 0;
    Remote_FrameSyncCounter2 = 0;
    Remote_SendSynchFrame = true;
    Remote_RemotePlayerCount = 0;
    Remote_P51_Signal = false;

    for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
        Remote_FrameSyncCounter2values[i] = 0;
        Remote_FrameSyncCounter[i] = 0;
        Remote_NextTurnIndices[i] = 0;
        Remote_LeaveGameRequestId[i] = 0;
        Remote_TurnIndices[i] = 0;
        Remote_P24_Signals[i] = false;
    }

    Remote_Nodes.Clear();
    Remote_Hosts.Clear();

    for (int32_t i = 0; i < ORDER_COUNT_MAX; ++i) {
        switch (i) {
            case ORDER_POWER_ON:
            case ORDER_POWER_OFF:
            case ORDER_EXPLODE:
            case ORDER_SENTRY:
            case ORDER_LAND:
            case ORDER_TAKE_OFF:
            case ORDER_LOAD:
            case ORDER_IDLE:
            case ORDER_HALT_BUILDING:
            case ORDER_AWAIT_SCALING:
            case ORDER_AWAIT_TAPE_POSITIONING:
            case ORDER_LAY_MINE:
            case ORDER_HALT_BUILDING_2: {
                Remote_OrderProcessors[i].WritePacket = &Remote_OrderProcessor1_Write;
                Remote_OrderProcessors[i].ReadPacket = &Remote_OrderProcessor1_Read;
            } break;

            case ORDER_TRANSFORM:
            case ORDER_BUILD:
            case ORDER_CLEAR:
            case ORDER_DISABLE: {
                Remote_OrderProcessors[i].WritePacket = &Remote_OrderProcessor4_Write;
                Remote_OrderProcessors[i].ReadPacket = &Remote_OrderProcessor4_Read;
            } break;

            case ORDER_AWAIT:
            case ORDER_MOVE:
            case ORDER_FIRE:
            case ORDER_ACTIVATE:
            case ORDER_TRANSFER:
            case ORDER_AWAIT_STEAL_UNIT:
            case ORDER_AWAIT_DISABLE_UNIT:
            case ORDER_MOVE_TO_UNIT:
            case ORDER_MOVE_TO_ATTACK: {
                Remote_OrderProcessors[i].WritePacket = &Remote_OrderProcessor3_Write;
                Remote_OrderProcessors[i].ReadPacket = &Remote_OrderProcessor3_Read;
            } break;

            case ORDER_NEW_ALLOCATE: {
                Remote_OrderProcessors[i].WritePacket = &Remote_OrderProcessor5_Write;
                Remote_OrderProcessors[i].ReadPacket = &Remote_OrderProcessor5_Read;
            } break;

            case ORDER_UNLOAD:
            case ORDER_REPAIR:
            case ORDER_REFUEL:
            case ORDER_RELOAD:
            case ORDER_UPGRADE: {
                Remote_OrderProcessors[i].WritePacket = &Remote_OrderProcessor2_Write;
                Remote_OrderProcessors[i].ReadPacket = &Remote_OrderProcessor2_Read;
            } break;

            default: {
                SDL_assert(false);
            } break;
        }
    }
}

static bool Remote_ReceivePacket(NetPacket& packet) {
    bool result;

    if (Remote_Transport->ReceivePacket(packet)) {
        if (packet.GetDataSize() < 3) {
            SDL_Log("Remote: Dropped malformed packet (size: %i).\n", packet.GetDataSize());
            result = false;

        } else {
            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

static void Remote_TransmitPacket(NetPacket& packet, int32_t transmit_mode) {
    switch (transmit_mode) {
        case REMOTE_UNICAST: {
            SDL_assert(packet.GetAddressCount() == 1);
        } break;

        case REMOTE_MULTICAST: {
            for (int32_t i = 0; i < Remote_Nodes.GetCount(); ++i) {
                if (Remote_NetworkMenu->player_node != Remote_Nodes[i]->entity_id) {
                    packet.AddAddress(Remote_Nodes[i]->address);
                }
            }

            if (packet.GetAddressCount() == 0) {
                return;
            }
        } break;

        case REMOTE_BROADCAST: {
            packet.ClearAddressTable();
        } break;
    }

    if (!Remote_Transport->TransmitPacket(packet)) {
        /// \todo Handle transport layer errors
        Remote_Transport->GetError();
    }
}

void Remote_Deinit() {
    if (Remote_Transport) {
        Remote_Transport->Deinit();

        delete Remote_Transport;
        Remote_Transport = nullptr;
    }

    Remote_GameState = 0;
    Remote_IsNetworkGame = false;
}

bool Remote_AnalyzeDesyncHost(SmartList<UnitInfo>& units) {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).unit_type != MININGST) {
            for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
                Remote_P24_Signals[i] = false;
            }

            Remote_SendNetPacket_23(&*it);

            bool stay_in_loop = true;

            while (stay_in_loop && Remote_IsNetworkGame) {
                Remote_ProcessNetPackets();
                GameManager_ProcessTick(false);

                stay_in_loop = false;

                for (int32_t i = TRANSPORT_MAX_TEAM_COUNT - 1; i >= 0; --i) {
                    if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_REMOTE) {
                        if (!Remote_P24_Signals[i]) {
                            stay_in_loop = true;
                        }
                    }
                }

                if (get_input() == GNW_KB_KEY_ESCAPE) {
                    return false;
                }
            }
        }
    }

    return true;
}

int32_t Remote_Lobby(bool is_host_mode) {
    int32_t result;
    char ini_transport[30];
    int32_t transort_type;

    Remote_IsHostMode = is_host_mode;

    ResourceManager_InitTeamInfo();
    Remote_Init();

    if (ini_config.GetStringValue(INI_NETWORK_TRANSPORT, ini_transport, sizeof(ini_transport))) {
        if (!strcmp(ini_transport, TRANSPORT_DEFAULT_TYPE)) {
            transort_type = TRANSPORT_DEFAULT_UDP;

        } else {
            SmartString error_message;

            ini_config.SetStringValue(INI_NETWORK_TRANSPORT, TRANSPORT_DEFAULT_TYPE);

            error_message.Sprintf(
                100, "Unknown network transport layer type (%s), using default (" TRANSPORT_DEFAULT_TYPE ").\n",
                ini_transport);
            MessageManager_DrawMessage(error_message.GetCStr(), 2, 1, true);

            transort_type = TRANSPORT_DEFAULT_UDP;
        }

    } else {
        transort_type = TRANSPORT_DEFAULT_UDP;
    }

    SDL_assert(Remote_Transport == nullptr);
    Remote_Transport = Transport_Create(transort_type);

    if (Remote_Transport) {
        if (Remote_Transport->Init(Remote_IsHostMode ? TRANSPORT_SERVER : TRANSPORT_CLIENT)) {
            Remote_IsNetworkGame = NetworkMenu_MenuLoop(Remote_IsHostMode);

            result = Remote_IsNetworkGame;

        } else {
            WindowManager_LoadBigImage(MAINPIC, WindowManager_GetWindow(WINDOW_MAIN_WINDOW),
                                       WindowManager_GetWindow(WINDOW_MAIN_WINDOW)->width, false, true, -1, -1, true);
            MessageManager_DrawMessage(Remote_Transport->GetError(), 2, 1);

            result = false;
        }

    } else {
        WindowManager_LoadBigImage(MAINPIC, WindowManager_GetWindow(WINDOW_MAIN_WINDOW),
                                   WindowManager_GetWindow(WINDOW_MAIN_WINDOW)->width, false, true, -1, -1, true);
        MessageManager_DrawMessage("Unable to initialize network transport layer.\n", 2, 1);

        result = false;
    }

    return result;
}

void Remote_SetupConnection() {
    int32_t remote_player_count;

    remote_player_count = Remote_SetupPlayers();

    Remote_GameState = 2;
    Remote_NetworkMenu->remote_player_count = 0;

    Remote_SendNetPacket_32(REMOTE_BROADCAST);

    while (Remote_NetworkMenu->remote_player_count != remote_player_count) {
        Remote_ProcessNetPackets();
        if (get_input() == GNW_KB_KEY_ESCAPE) {
            return;
        }
    }

    Remote_SendNetPacket_34();

    Remote_NetworkMenu->connection_state = 1;
}

bool Remote_UiProcessNetPackets() {
    Remote_ProcessNetPackets();

    return Remote_NetworkMenu->is_gui_update_needed;
}

bool Remote_UiProcessTick(bool mode) {
    bool result;

    Remote_ProcessNetPackets();

    if (!TickTimer_HaveTimeToThink(TIMER_FPS_TO_MS(24))) {
        Remote_Synchronize(mode);

        TickTimer_SetLastTimeStamp(timer_get());

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool Remote_CheckRestartAfterDesyncEvent() {
    Remote_ProcessNetPackets();

    return Remote_P51_Signal;
}

void Remote_RegisterMenu(NetworkMenu* menu) {
    Remote_NetworkMenu = menu;
    Remote_GameState = 1;

    if (!Remote_NetworkMenu->is_host_mode) {
        Remote_SendNetPacket_28(0);
    }
}

void Remote_ProcessNetPackets() {
    for (;;) {
        NetPacket packet;
        uint8_t packet_type;

        if (Remote_ReceivePacket(packet)) {
            packet >> packet_type;

            switch (packet_type) {
                case REMOTE_PACKET_00: {
                    Remote_ReceiveNetPacket_00(packet);
                } break;

                case REMOTE_PACKET_01: {
                    Remote_ReceiveNetPacket_01(packet);
                } break;

                case REMOTE_PACKET_05: {
                    Remote_ReceiveNetPacket_05(packet);
                } break;

                case REMOTE_PACKET_06: {
                    Remote_ReceiveNetPacket_06(packet);
                } break;

                case REMOTE_PACKET_07: {
                    Remote_ReceiveNetPacket_07(packet);
                } break;

                case REMOTE_PACKET_08: {
                    Remote_ReceiveNetPacket_08(packet);
                } break;

                case REMOTE_PACKET_09: {
                    Remote_ReceiveNetPacket_09(packet);
                } break;

                case REMOTE_PACKET_10: {
                    Remote_ReceiveNetPacket_10(packet);
                } break;

                case REMOTE_PACKET_11: {
                    Remote_ReceiveNetPacket_11(packet);
                } break;

                case REMOTE_PACKET_12: {
                    Remote_ReceiveNetPacket_12(packet);
                } break;

                case REMOTE_PACKET_13: {
                    Remote_ReceiveNetPacket_13(packet);
                } break;

                case REMOTE_PACKET_14: {
                    Remote_ReceiveNetPacket_14(packet);
                } break;

                case REMOTE_PACKET_16: {
                    Remote_ReceiveNetPacket_16(packet);
                } break;

                case REMOTE_PACKET_17: {
                    Remote_ReceiveNetPacket_17(packet);
                } break;

                case REMOTE_PACKET_18: {
                    Remote_ReceiveNetPacket_18(packet);
                } break;

                case REMOTE_PACKET_20: {
                    Remote_ReceiveNetPacket_20(packet);
                } break;

                case REMOTE_PACKET_21: {
                    Remote_ReceiveNetPacket_21(packet);
                } break;

                case REMOTE_PACKET_22: {
                    Remote_ReceiveNetPacket_22(packet);
                } break;

                case REMOTE_PACKET_23: {
                    Remote_ReceiveNetPacket_23(packet);
                } break;

                case REMOTE_PACKET_24: {
                    Remote_ReceiveNetPacket_24(packet);
                } break;

                case REMOTE_PACKET_26: {
                    Remote_ReceiveNetPacket_26(packet);
                } break;

                case REMOTE_PACKET_28: {
                    Remote_ReceiveNetPacket_28(packet);
                } break;

                case REMOTE_PACKET_29: {
                    Remote_ReceiveNetPacket_29(packet);
                } break;

                case REMOTE_PACKET_30: {
                    Remote_ReceiveNetPacket_30(packet);
                } break;

                case REMOTE_PACKET_31: {
                    Remote_ReceiveNetPacket_31(packet);
                } break;

                case REMOTE_PACKET_32: {
                    Remote_ReceiveNetPacket_32(packet);
                } break;

                case REMOTE_PACKET_33: {
                    Remote_ReceiveNetPacket_33(packet);
                } break;

                case REMOTE_PACKET_34: {
                    Remote_ReceiveNetPacket_34(packet);
                } break;

                case REMOTE_PACKET_35: {
                    Remote_ReceiveNetPacket_35(packet);
                } break;

                case REMOTE_PACKET_36: {
                    Remote_ReceiveNetPacket_36(packet);
                } break;

                case REMOTE_PACKET_37: {
                    Remote_ReceiveNetPacket_37(packet);
                } break;

                case REMOTE_PACKET_38: {
                    Remote_ReceiveNetPacket_38(packet);
                } break;

                case REMOTE_PACKET_39: {
                    Remote_ReceiveNetPacket_39(packet);
                } break;

                case REMOTE_PACKET_40: {
                    Remote_ReceiveNetPacket_40(packet);
                } break;

                case REMOTE_PACKET_41: {
                    Remote_ReceiveNetPacket_41(packet);
                } break;

                case REMOTE_PACKET_42: {
                    Remote_ReceiveNetPacket_42(packet);
                } break;

                case REMOTE_PACKET_43: {
                    Remote_ReceiveNetPacket_43(packet);
                } break;

                case REMOTE_PACKET_44: {
                    Remote_ReceiveNetPacket_44(packet);
                } break;

                case REMOTE_PACKET_45: {
                    Remote_ReceiveNetPacket_45(packet);
                } break;

                case REMOTE_PACKET_46: {
                    Remote_ReceiveNetPacket_46(packet);
                } break;

                case REMOTE_PACKET_48: {
                    Remote_ReceiveNetPacket_48(packet);
                } break;

                case REMOTE_PACKET_49: {
                    Remote_ReceiveNetPacket_49(packet);
                } break;

                case REMOTE_PACKET_50: {
                    Remote_ReceiveNetPacket_50(packet);
                } break;

                case REMOTE_PACKET_51: {
                    Remote_ReceiveNetPacket_51(packet);
                } break;

                case REMOTE_PACKET_52: {
                    Remote_ReceiveNetPacket_52(packet);
                } break;

                default: {
                    AiLog log("Received unknown packet type (%i).", packet_type - TRANSPORT_APPL_PACKET_ID);
                } break;
            }
        } else {
            break;
        }
    };
}

void Remote_NetErrorUnknownUnit(uint16_t unit_id) {
    char message[100];

    sprintf(message, _(ef35), unit_id);

    AiLog log(message);

    MessageManager_DrawMessage(message, 2, 1, false, true);
}

void Remote_NetErrorUnitInfoOutOfSync(UnitInfo* unit, NetPacket& packet) {
    char message[100];
    AiLog log("Units are out of sync:  Host, Peer");
    struct Packet23Data data;
    const char* const team_names[PLAYER_TEAM_MAX + 1] = {"Red", "Green", "Blue", "Gray", "Neutral", "Unknown"};

    Remote_ProcessNetPacket_23(data, packet);

    log.Log(" team          %s, %s",
            (unit->team < PLAYER_TEAM_MAX) ? team_names[unit->team] : team_names[PLAYER_TEAM_MAX],
            (data.team < PLAYER_TEAM_MAX) ? team_names[data.team] : team_names[PLAYER_TEAM_MAX]);

    log.Log(" type          %s, %s",
            (unit->unit_type < UNIT_END) ? UnitsManager_BaseUnits[unit->unit_type].singular_name : "?",
            (data.unit_type < UNIT_END) ? UnitsManager_BaseUnits[unit->unit_type].singular_name : "?");

    log.Log(" unit id       %i, %i", unit->unit_id, data.unit_id);
    log.Log(" parent id     %X, %X", unit->GetParent() ? unit->GetParent()->GetId() : 0, data.parent_unit_id);
    log.Log(" enemy id      %X, %X", unit->GetEnemy() ? unit->GetEnemy()->GetId() : 0, data.enemy_unit_id);
    log.Log(" orders        %i, %i", unit->orders, data.orders);
    log.Log(" order state   %i, %i", unit->state, data.state);
    log.Log(" prior orders  %i, %i", unit->prior_orders, data.prior_orders);
    log.Log(" prior state   %i, %i", unit->prior_state, data.prior_state);
    log.Log(" grid x        %i, %i", unit->grid_x, data.grid_x);
    log.Log(" grid y        %i, %i", unit->grid_y, data.grid_y);

    if (unit->orders == ORDER_BUILD || data.orders == ORDER_BUILD) {
        log.Log(" build turns   %i, %i", unit->build_time, data.build_time);
        log.Log(" build rate    %i, %i", unit->build_rate, data.build_rate);
    }

    log.Log(" target grid x %i, %i", unit->target_grid_x, data.target_grid_x);
    log.Log(" target grid y %i, %i", unit->target_grid_y, data.target_grid_y);
    log.Log(" reaction fire %i, %i", unit->disabled_reaction_fire, data.disabled_reaction_fire);
    log.Log(" total mining  %i, %i", unit->total_mining, data.total_mining);
    log.Log(" raw mining    %i, %i", unit->raw_mining, data.raw_mining);
    log.Log(" fuel mining   %i, %i", unit->fuel_mining, data.fuel_mining);
    log.Log(" gold mining   %i, %i", unit->gold_mining, data.gold_mining);
    log.Log(" hits          %i, %i", unit->hits, data.hits);
    log.Log(" speed         %i, %i", unit->speed, data.speed);
    log.Log(" rounds        %i, %i", unit->shots, data.shots);
    log.Log(" storage       %i, %i", unit->storage, data.storage);
    log.Log(" ammo          %i, %i", unit->ammo, data.ammo);

    sprintf(message, "Unit, id %i, is in different state in remote packet.", unit->GetId());

    MessageManager_DrawMessage(message, 2, 1, false, true);
}

void Remote_AnalyzeDesync() {
    Remote_UpdatePauseTimer = false;

    if (Remote_IsHostMode) {
        if (Remote_AnalyzeDesyncHost(UnitsManager_MobileLandSeaUnits) &&
            Remote_AnalyzeDesyncHost(UnitsManager_MobileAirUnits) &&
            Remote_AnalyzeDesyncHost(UnitsManager_StationaryUnits) &&
            Remote_AnalyzeDesyncHost(UnitsManager_GroundCoverUnits)) {
            Remote_AnalyzeDesyncHost(UnitsManager_ParticleUnits);
        }

        Remote_SendNetPacket_Signal(REMOTE_PACKET_49, PLAYER_TEAM_RED, 1);

    } else {
        Remote_P49_Signal = false;

        while (!Remote_P49_Signal) {
            Remote_ProcessNetPackets();
            GameManager_ProcessTick(false);

            if (Remote_P23_UnitId != 0) {
                UnitInfo* unit = Hash_UnitHash[Remote_P23_UnitId];

                if (unit) {
                    NetPacket packet;

                    packet << static_cast<uint8_t>(REMOTE_PACKET_23);
                    packet << unit->GetId();

                    Remote_CreateNetPacket_23(unit, packet);

                    if (Remote_P23_Packet != packet) {
                        Remote_NetErrorUnitInfoOutOfSync(unit, Remote_P23_Packet);
                    }

                } else {
                    Remote_NetErrorUnknownUnit(Remote_P23_UnitId);
                }

                Remote_P23_UnitId = 0;

                Remote_SendNetPacket_Signal(REMOTE_PACKET_24, GameManager_PlayerTeam, 1);
            }

            if (get_input() == GNW_KB_KEY_ESCAPE) {
                break;
            }
        }
    }

    SaveLoadMenu_Save("save.dbg", "debug save", false);

    if (ini_get_setting(INI_TIMER)) {
        Remote_UpdatePauseTimer = true;
    }
}

int32_t Remote_CheckUnpauseEvent() {
    Remote_UnpauseGameEvent = false;

    Remote_ProcessNetPackets();
    Remote_TimeoutTimeStamp = timer_get();

    return Remote_UnpauseGameEvent;
}

void Remote_Synchronize(bool async_mode) {
    Remote_ProcessNetPackets();

    if (GameManager_GameState == GAME_STATE_3_MAIN_MENU || GameManager_GameState == GAME_STATE_7_SITE_SELECT ||
        GameManager_GameState == GAME_STATE_10 || GameManager_GameState == GAME_STATE_12 ||
        GameManager_GameState == GAME_STATE_13) {
        Remote_TimeoutTimeStamp = timer_get();
        Remote_SendSynchFrame = true;

    } else {
        uint32_t time_stamp = timer_get();
        bool stay_in_loop = true;

        while (stay_in_loop && Remote_IsNetworkGame) {
            Remote_ProcessNetPackets();
            MouseEvent::ProcessInput();

            if (Remote_SendSynchFrame || timer_elapsed_time(time_stamp) > TIMER_FPS_TO_MS(2)) {
                Remote_SendNetPacket_Signal(REMOTE_PACKET_01, GameManager_PlayerTeam, Remote_FrameSyncCounter2);
                time_stamp = timer_get();
                Remote_SendSynchFrame = false;
            }

            stay_in_loop = false;

            for (int32_t team = 0; team < TRANSPORT_MAX_TEAM_COUNT; ++team) {
                if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_REMOTE) {
                    if (((Remote_FrameSyncCounter2 - 1) & 0x3F) == Remote_FrameSyncCounter2values[team]) {
                        if (timer_elapsed_time(Remote_TimeoutTimeStamp) > REMOTE_RESPONSE_TIMEOUT) {
                            Remote_ResponseTimeout(team, true);
                            break;

                        } else {
                            stay_in_loop = true;
                        }
                    }
                }
            }

            if (stay_in_loop && async_mode) {
                Remote_TimeoutTimeStamp = timer_get();

                return;
            }
        }

        Remote_FrameSyncCounter2 = (Remote_FrameSyncCounter2 + 1) & 0x3F;
        Remote_TimeoutTimeStamp = timer_get();
        Remote_SendSynchFrame = true;
    }
}

void Remote_WaitBeginTurnAcknowledge() {
    Cursor_SetCursor(CURSOR_UNIT_NO_GO);

    Remote_P51_Signal = false;

    ++Remote_FrameSyncCounter[GameManager_PlayerTeam];

    Remote_SendNetPacket_Signal(REMOTE_PACKET_00, GameManager_PlayerTeam,
                                Remote_FrameSyncCounter[GameManager_PlayerTeam]);

    uint32_t time_stamp_timeout = timer_get();
    uint32_t time_stamp_ping = timer_get();

    bool stay_in_loop = true;

    while (stay_in_loop && Remote_IsNetworkGame) {
        Remote_UiProcessTick(true);

        if (timer_elapsed_time(time_stamp_ping) > REMOTE_PING_TIME_PERIOD) {
            Remote_SendNetPacket_Signal(REMOTE_PACKET_00, GameManager_PlayerTeam,
                                        Remote_FrameSyncCounter[GameManager_PlayerTeam]);

            time_stamp_ping = timer_get();
        }

        stay_in_loop = false;

        for (int32_t team = 0; team < TRANSPORT_MAX_TEAM_COUNT; ++team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_REMOTE) {
                if (Remote_FrameSyncCounter[team] != Remote_FrameSyncCounter[GameManager_PlayerTeam]) {
                    stay_in_loop = true;

                    if (timer_elapsed_time(time_stamp_timeout) > REMOTE_RESPONSE_TIMEOUT) {
                        Remote_ResponseTimeout(team, true);
                    }
                }
            }
        }

        if (get_input() == GNW_KB_KEY_ESCAPE) {
            stay_in_loop = false;
        }
    }
}

void Remote_WaitEndTurnAcknowledge() {
    Cursor_SetCursor(CURSOR_UNIT_NO_GO);

    Remote_P51_Signal = false;

    ++Remote_TurnIndices[GameManager_PlayerTeam];

    Remote_SendNetPacket_Signal(REMOTE_PACKET_52, GameManager_PlayerTeam, Remote_TurnIndices[GameManager_PlayerTeam]);

    uint32_t time_stamp_timeout = timer_get();
    uint32_t time_stamp_ping = timer_get();

    bool stay_in_loop = true;

    while (stay_in_loop && Remote_IsNetworkGame) {
        Remote_UiProcessTick(true);

        if (timer_elapsed_time(time_stamp_ping) > REMOTE_PING_TIME_PERIOD) {
            Remote_SendNetPacket_Signal(REMOTE_PACKET_52, GameManager_PlayerTeam,
                                        Remote_TurnIndices[GameManager_PlayerTeam]);

            time_stamp_ping = timer_get();
        }

        stay_in_loop = false;

        for (int32_t team = 0; team < TRANSPORT_MAX_TEAM_COUNT; ++team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_REMOTE) {
                if (Remote_TurnIndices[team] != Remote_TurnIndices[GameManager_PlayerTeam]) {
                    stay_in_loop = true;

                    if (timer_elapsed_time(time_stamp_timeout) > REMOTE_RESPONSE_TIMEOUT) {
                        Remote_ResponseTimeout(team, true);
                    }
                }
            }
        }

        if (get_input() == GNW_KB_KEY_ESCAPE) {
            stay_in_loop = false;
        }
    }
}

int32_t Remote_SiteSelectMenu() {
    bool stay_in_loop = false;

    Remote_ProcessNetPackets();

    for (int32_t team = 0; team < TRANSPORT_MAX_TEAM_COUNT; ++team) {
        if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_REMOTE) {
            if (!UnitsManager_TeamMissionSupplies[team].units.GetCount()) {
                stay_in_loop = true;
            }
        }
    }

    if (stay_in_loop && Remote_IsNetworkGame) {
        MessageManager_DrawMessage(_(6da5), 0, 0);

        Cursor_SetCursor(CURSOR_HAND);

        while (stay_in_loop && Remote_IsNetworkGame) {
            Remote_ProcessNetPackets();
            GameManager_ProcessTick(false);
            MouseEvent::ProcessInput();

            stay_in_loop = false;

            for (int32_t team = 0; team < TRANSPORT_MAX_TEAM_COUNT; ++team) {
                if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_REMOTE) {
                    if (!UnitsManager_TeamMissionSupplies[team].units.GetCount()) {
                        stay_in_loop = true;
                    }
                }
            }

            int32_t key_press = get_input();

            switch (key_press) {
                case GNW_KB_KEY_ESCAPE: {
                    GameManager_GameState = GAME_STATE_14;

                    Remote_SendNetPacket_Signal(REMOTE_PACKET_42, GameManager_PlayerTeam, false);

                    return 6;

                } break;

                case GNW_KB_KEY_LALT_X: {
                    GameManager_GameState = GAME_STATE_3_MAIN_MENU;

                    Remote_SendNetPacket_Signal(REMOTE_PACKET_42, GameManager_PlayerTeam, false);

                    return 6;

                } break;

                case GNW_KB_KEY_F10: {
                    GameManager_GameState = GAME_STATE_15_FATAL_ERROR;

                    Remote_SendNetPacket_Signal(REMOTE_PACKET_42, GameManager_PlayerTeam, false);

                    return 6;

                } break;

                case GNW_KB_KEY_SHIFT_DIVIDE: {
                    HelpMenu_Menu(HELPMENU_SITE_SELECT_SETUP, WINDOW_MAIN_MAP);
                } break;
            }
        }

        MessageManager_ClearMessageBox();
    }

    for (int32_t team = 0; team < TRANSPORT_MAX_TEAM_COUNT; ++team) {
        if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_REMOTE) {
            int32_t proximity_state = GameManager_CheckLandingZones(GameManager_PlayerTeam, team);

            if (proximity_state != 0) {
                UnitsManager_TeamMissionSupplies[team].units.Clear();

                return proximity_state;
            }
        }
    }

    for (int32_t team = 0; team < TRANSPORT_MAX_TEAM_COUNT; ++team) {
        if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_REMOTE) {
            for (int32_t team2 = 0; team2 < TRANSPORT_MAX_TEAM_COUNT; ++team2) {
                if (team != team2) {
                    if (UnitsManager_TeamInfo[team2].team_type == TEAM_TYPE_REMOTE) {
                        if (GameManager_CheckLandingZones(team, team2)) {
                            UnitsManager_TeamMissionSupplies[team].units.Clear();
                            UnitsManager_TeamMissionSupplies[team2].units.Clear();

                            return 5;
                        }
                    }
                }
            }
        }
    }

    dos_srand(Remote_RngSeed);

    return 0;
}

void Remote_LeaveGame(uint16_t team, bool mode) {
    if (UnitsManager_TeamInfo[UnitsManager_DelayedReactionsTeam].team_type != TEAM_TYPE_REMOTE) {
        uint16_t source_team = UnitsManager_DelayedReactionsTeam;

        do {
            UnitsManager_DelayedReactionsTeam = (UnitsManager_DelayedReactionsTeam + 1) % TRANSPORT_MAX_TEAM_COUNT;

        } while (UnitsManager_DelayedReactionsTeam != source_team &&
                 UnitsManager_TeamInfo[UnitsManager_DelayedReactionsTeam].team_type != TEAM_TYPE_REMOTE);

        if (UnitsManager_DelayedReactionsTeam != source_team) {
            ++UnitsManager_DelayedReactionsSyncCounter;

            Remote_SendNetPacket_46(UnitsManager_DelayedReactionsTeam, false, UnitsManager_DelayedReactionsSyncCounter);
        }
    }

    ++Remote_LeaveGameRequestId[team];

    Remote_SendNetPacket_07(team, mode);

    uint32_t time_stamp = timer_get();
    bool stay_in_loop = true;

    while (stay_in_loop && Remote_IsNetworkGame) {
        Remote_ProcessNetPackets();
        MouseEvent::ProcessInput();

        stay_in_loop = false;

        for (int32_t team2 = 0; team2 < TRANSPORT_MAX_TEAM_COUNT; ++team2) {
            if (UnitsManager_TeamInfo[team2].team_type == TEAM_TYPE_REMOTE) {
                if (Remote_LeaveGameRequestId[team2] < Remote_LeaveGameRequestId[team]) {
                    if (timer_elapsed_time(time_stamp) < 5000) {
                        stay_in_loop = true;
                    }
                }
            }
        }

        if (get_input() == GNW_KB_KEY_ESCAPE) {
            stay_in_loop = false;
        }
    }

    Remote_GameState = 0;
    Remote_IsNetworkGame = false;
}

bool Remote_CheckDesync(uint16_t team, uint16_t crc_checksum) {
    ++Remote_NextTurnIndices[team];

    Remote_SendNetPacket_45(team, Remote_NextTurnIndices[team], crc_checksum);

    uint32_t time_stamp_timeout = timer_get();
    uint32_t time_stamp_ping = timer_get();

    bool stay_in_loop = true;

    while (stay_in_loop && Remote_IsNetworkGame) {
        Remote_UiProcessTick(true);

        if (timer_elapsed_time(time_stamp_ping) > REMOTE_PING_TIME_PERIOD) {
            Remote_SendNetPacket_45(team, Remote_NextTurnIndices[team], crc_checksum);
            time_stamp_ping = timer_get();
        }

        stay_in_loop = false;

        for (int32_t i = TRANSPORT_MAX_TEAM_COUNT - 1; i >= 0; --i) {
            if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_REMOTE) {
                if (Remote_NextTurnIndices[i] != Remote_NextTurnIndices[team]) {
                    stay_in_loop = true;

                    if (timer_elapsed_time(time_stamp_timeout) > REMOTE_RESPONSE_TIMEOUT) {
                        Remote_ResponseTimeout(i, true);
                    }
                }
            }
        }

        if (get_input() == GNW_KB_KEY_ESCAPE) {
            stay_in_loop = false;
        }
    }

    for (int32_t i = TRANSPORT_MAX_TEAM_COUNT - 1; i >= 0; --i) {
        if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_REMOTE) {
            if (Remote_TeamDataCrc16[i] != crc_checksum && Remote_IsNetworkGame) {
                return false;
            }
        }
    }

    return true;
}

void Remote_SendNetPacket_Signal(int32_t packet_type, int32_t team, uint8_t parameter) {
    NetPacket packet;

    packet << static_cast<uint8_t>(packet_type);
    packet << static_cast<uint16_t>(team);

    packet << parameter;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_00(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;
    packet >> Remote_FrameSyncCounter[entity_id];
}

void Remote_ReceiveNetPacket_01(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;
    packet >> Remote_FrameSyncCounter2values[entity_id];
}

void Remote_SendNetPacket_05(int32_t transmit_mode) {
    NetPacket packet;
    NetNode* node{nullptr};

    packet << static_cast<uint8_t>(REMOTE_PACKET_05);
    packet << static_cast<uint16_t>(Remote_NetworkMenu->host_node);

    node = Remote_Nodes.Find(Remote_NetworkMenu->host_node);

    packet.AddAddress(node->address);

    Remote_TransmitPacket(packet, transmit_mode);
}

void Remote_ReceiveNetPacket_05(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    if (Remote_NetworkMenu->host_node == entity_id) {
        ++Remote_NetworkMenu->remote_player_count;
    }
}

void Remote_ReceiveNetPacket_06(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    UnitsManager_TeamInfo[entity_id].finished_turn = 1;

    if (GameManager_PlayMode == PLAY_MODE_TURN_BASED) {
        GameManager_GameState = GAME_STATE_9_END_TURN;

    } else {
        bool is_found = false;

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
                UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED &&
                !UnitsManager_TeamInfo[team].finished_turn) {
                is_found = true;
            }
        }

        GameManager_HandleTurnTimer();

        if (is_found) {
            char message[100];

            sprintf(message, _(2abe), menu_team_names[entity_id]);

            MessageManager_DrawMessage(message, 1, 0);

            SoundManager_PlayVoice(static_cast<ResourceID>(V_M279 + entity_id * 2),
                                   static_cast<ResourceID>(V_F279 + entity_id * 2));
        }
    }
}

void Remote_SendNetPacket_07(uint16_t team, bool mode) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_07);
    packet << static_cast<uint16_t>(team);

    packet << Remote_LeaveGameRequestId[team];
    packet << mode;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_07(NetPacket& packet) {
    uint16_t entity_id;
    uint8_t request_id;
    bool request_mode;

    packet >> entity_id;

    packet >> request_id;
    packet >> request_mode;

    ++Remote_LeaveGameRequestId[GameManager_PlayerTeam];

    Remote_SendNetPacket_Signal(REMOTE_PACKET_48, GameManager_PlayerTeam,
                                Remote_LeaveGameRequestId[GameManager_PlayerTeam]);

    if (request_mode) {
        Remote_ResponseTimeout(entity_id, false);
    }
}

void Remote_SendNetPacket_08(UnitInfo* unit) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_08);
    packet << static_cast<uint16_t>(unit->GetId());

    Remote_OrderProcessors[unit->orders].WritePacket(unit, packet);

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_08(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    UnitInfo* unit = Hash_UnitHash[entity_id];

    if (unit) {
        {
            NetPacket local;

            local.Write(packet.GetBuffer(), packet.GetDataSize());

            Remote_OrderProcessor1_Read(unit, local);
        }

        Remote_OrderProcessors[unit->orders].ReadPacket(unit, packet);

    } else {
        Remote_NetErrorUnknownUnit(entity_id);
    }
}

void Remote_SendNetPacket_09(int32_t team) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_09);
    packet << static_cast<uint16_t>(team);

    packet << UnitsManager_TeamInfo[team].team_units->GetGold();
    packet << UnitsManager_TeamInfo[team].stats_gold_spent_on_upgrades;

    packet << UnitsManager_TeamInfo[team].markers;
    packet << UnitsManager_TeamInfo[team].research_topics;

    char team_name[30];

    if (!ini_config.GetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), team_name, sizeof(team_name))) {
        SDL_assert(0);
    }

    packet << SmartString(team_name);

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_09(NetPacket& packet) {
    uint16_t entity_id;
    uint16_t gold;
    SmartString team_name;

    packet >> entity_id;

    packet >> gold;
    UnitsManager_TeamInfo[entity_id].team_units->SetGold(gold);

    packet >> UnitsManager_TeamInfo[entity_id].stats_gold_spent_on_upgrades;
    packet >> UnitsManager_TeamInfo[entity_id].markers;
    packet >> UnitsManager_TeamInfo[entity_id].research_topics;

    packet >> team_name;

    ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + entity_id), team_name.GetCStr());
}

void Remote_SendNetPacket_10(int32_t team, ResourceID unit_type) {
    NetPacket packet;
    SmartPointer<UnitValues> unit_values(UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type));

    packet << static_cast<uint8_t>(REMOTE_PACKET_10);
    packet << static_cast<uint16_t>(team);

    packet << unit_type;
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_TURNS));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_HITS));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_ARMOR));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_ATTACK));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_SPEED));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_RANGE));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_ROUNDS));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_MOVE_AND_FIRE));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_SCAN));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_STORAGE));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_AMMO));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_ATTACK_RADIUS));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_AGENT_ADJUST));

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_10(NetPacket& packet) {
    uint16_t entity_id;
    ResourceID unit_type;
    uint16_t value;

    packet >> entity_id;

    packet >> unit_type;

    SmartPointer<UnitValues> unit_values(new (std::nothrow) UnitValues(
        *UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[entity_id], unit_type)));

    unit_values->UpdateVersion();
    unit_values->SetUnitsBuilt(0);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_TURNS, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_HITS, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_ARMOR, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_ATTACK, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_SPEED, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_RANGE, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_ROUNDS, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_MOVE_AND_FIRE, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_SCAN, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_STORAGE, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_AMMO, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_ATTACK_RADIUS, value);

    packet >> value;
    unit_values->SetAttribute(ATTRIB_AGENT_ADJUST, value);

    UnitsManager_TeamInfo[entity_id].team_units->SetCurrentUnitValues(unit_type, *unit_values);
}

void Remote_SendNetPacket_11(int32_t team, Complex* complex) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetComplex() == complex && (*it).unit_type == MININGST && (*it).orders != ORDER_POWER_OFF &&
            (*it).orders != ORDER_DISABLE && (*it).orders != ORDER_IDLE) {
            UnitsManager_SetNewOrder(&*it, ORDER_NEW_ALLOCATE, ORDER_STATE_INIT);
        }
    }

    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_11);
    packet << static_cast<uint16_t>(team);

    UnitsManager_TeamInfo[team].team_units->WriteComplexPacket(complex->GetId(), packet);

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_11(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    UnitsManager_TeamInfo[entity_id].team_units->ReadComplexPacket(packet);
}

void Remote_SendNetPacket_12(int32_t team) {
    TeamMissionSupplies* supplies = &UnitsManager_TeamMissionSupplies[team];
    NetPacket packet;
    uint16_t unit_count = supplies->units.GetCount();

    packet << static_cast<uint8_t>(REMOTE_PACKET_12);
    packet << static_cast<uint16_t>(team);

    packet << supplies->team_gold;
    packet << unit_count;
    packet << UnitsManager_TeamInfo[team].stats_gold_spent_on_upgrades;
    packet << supplies->starting_position;
    packet << supplies->proximity_alert_ack;

    for (int32_t i = 0; i < unit_count; ++i) {
        packet << *supplies->units[i];
        packet << *supplies->cargos[i];
    }

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_12(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    TeamMissionSupplies* supplies = &UnitsManager_TeamMissionSupplies[entity_id];

    supplies->units.Clear();
    supplies->cargos.Clear();

    uint16_t unit_count;

    packet >> supplies->team_gold;
    packet >> unit_count;
    packet >> UnitsManager_TeamInfo[entity_id].stats_gold_spent_on_upgrades;
    packet >> supplies->starting_position;
    packet >> supplies->proximity_alert_ack;

    for (int32_t i = 0; i < unit_count; ++i) {
        ResourceID unit_type;
        uint16_t cargo;

        packet >> unit_type;
        supplies->units.PushBack(&unit_type);

        packet >> cargo;
        supplies->cargos.PushBack(&cargo);
    }
}

void Remote_SendNetPacket_13(uint32_t rng_seed) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_13);
    packet << static_cast<uint16_t>(0);

    packet << rng_seed;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_13(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    packet >> Remote_RngSeed;
}

void Remote_SendNetPacket_14(int32_t team, ResourceID unit_type, int32_t grid_x, int32_t grid_y) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_14);
    packet << static_cast<uint16_t>(team);

    packet << unit_type;
    packet << grid_x;
    packet << grid_y;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_14(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    ResourceID unit_type;
    int32_t grid_x;
    int32_t grid_y;

    packet >> unit_type;
    packet >> grid_x;
    packet >> grid_y;

    bool state_backup = GameManager_UnknownFlag3;

    GameManager_UnknownFlag3 = true;

    GameManager_DeployUnit(entity_id, unit_type, grid_x, grid_y);

    GameManager_UnknownFlag3 = state_backup;
}

void Remote_SendNetPacket_16(const char* file_name, const char* file_title) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_16);
    packet << static_cast<uint16_t>(0);

    packet << SmartString(file_name);
    packet << SmartString(file_title);

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_16(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    SmartString file_name;
    SmartString file_title;

    packet >> file_name;
    packet >> file_title;

    SaveLoadMenu_Save(file_name.GetCStr(), file_title.GetCStr(), true);

    MessageManager_DrawMessage(_(87d7), 1, 0);
}

void Remote_SendNetPacket_17() {
    NetPacket packet;

    int32_t world;
    int32_t game_file_number;
    int32_t game_file_type;
    int32_t play_mode;
    int32_t all_visible;
    int32_t quick_build;
    int32_t real_time;
    int32_t log_file_debug;
    int32_t disable_fire;
    int32_t fast_movement;
    int32_t timer;
    int32_t endturn;
    int32_t start_gold;
    int32_t auto_save;
    int32_t victory_type;
    int32_t victory_limit;
    int32_t raw_normal_low;
    int32_t raw_normal_high;
    int32_t raw_concentrate_low;
    int32_t raw_concentrate_high;
    int32_t raw_concentrate_seperation;
    int32_t raw_concentrate_diffusion;
    int32_t fuel_normal_low;
    int32_t fuel_normal_high;
    int32_t fuel_concentrate_low;
    int32_t fuel_concentrate_high;
    int32_t fuel_concentrate_seperation;
    int32_t fuel_concentrate_diffusion;
    int32_t gold_normal_low;
    int32_t gold_normal_high;
    int32_t gold_concentrate_low;
    int32_t gold_concentrate_high;
    int32_t gold_concentrate_seperation;
    int32_t gold_concentrate_diffusion;
    int32_t mixed_resource_seperation;
    int32_t min_resources;
    int32_t max_resources;
    int32_t alien_seperation;
    int32_t alien_unit_value;

    world = ini_get_setting(INI_WORLD);
    game_file_number = ini_get_setting(INI_GAME_FILE_NUMBER);
    game_file_type = ini_get_setting(INI_GAME_FILE_TYPE);
    play_mode = ini_get_setting(INI_PLAY_MODE);
    all_visible = ini_get_setting(INI_ALL_VISIBLE);
    quick_build = ini_get_setting(INI_QUICK_BUILD);
    real_time = ini_get_setting(INI_REAL_TIME);
    log_file_debug = ini_get_setting(INI_LOG_FILE_DEBUG);
    disable_fire = ini_get_setting(INI_DISABLE_FIRE);
    fast_movement = ini_get_setting(INI_FAST_MOVEMENT);
    timer = ini_get_setting(INI_TIMER);
    endturn = ini_get_setting(INI_ENDTURN);
    start_gold = ini_get_setting(INI_START_GOLD);
    auto_save = ini_get_setting(INI_AUTO_SAVE);
    victory_type = ini_get_setting(INI_VICTORY_TYPE);
    victory_limit = ini_get_setting(INI_VICTORY_LIMIT);
    raw_normal_low = ini_get_setting(INI_RAW_NORMAL_LOW);
    raw_normal_high = ini_get_setting(INI_RAW_NORMAL_HIGH);
    raw_concentrate_low = ini_get_setting(INI_RAW_CONCENTRATE_LOW);
    raw_concentrate_high = ini_get_setting(INI_RAW_CONCENTRATE_HIGH);
    raw_concentrate_seperation = ini_get_setting(INI_RAW_CONCENTRATE_SEPERATION);
    raw_concentrate_diffusion = ini_get_setting(INI_RAW_CONCENTRATE_DIFFUSION);
    fuel_normal_low = ini_get_setting(INI_FUEL_NORMAL_LOW);
    fuel_normal_high = ini_get_setting(INI_FUEL_NORMAL_HIGH);
    fuel_concentrate_low = ini_get_setting(INI_FUEL_CONCENTRATE_LOW);
    fuel_concentrate_high = ini_get_setting(INI_FUEL_CONCENTRATE_HIGH);
    fuel_concentrate_seperation = ini_get_setting(INI_FUEL_CONCENTRATE_SEPERATION);
    fuel_concentrate_diffusion = ini_get_setting(INI_FUEL_CONCENTRATE_DIFFUSION);
    gold_normal_low = ini_get_setting(INI_GOLD_NORMAL_LOW);
    gold_normal_high = ini_get_setting(INI_GOLD_NORMAL_HIGH);
    gold_concentrate_low = ini_get_setting(INI_GOLD_CONCENTRATE_LOW);
    gold_concentrate_high = ini_get_setting(INI_GOLD_CONCENTRATE_HIGH);
    gold_concentrate_seperation = ini_get_setting(INI_GOLD_CONCENTRATE_SEPERATION);
    gold_concentrate_diffusion = ini_get_setting(INI_GOLD_CONCENTRATE_DIFFUSION);
    mixed_resource_seperation = ini_get_setting(INI_MIXED_RESOURCE_SEPERATION);
    min_resources = ini_get_setting(INI_MIN_RESOURCES);
    max_resources = ini_get_setting(INI_MAX_RESOURCES);
    alien_seperation = ini_get_setting(INI_ALIEN_SEPERATION);
    alien_unit_value = ini_get_setting(INI_ALIEN_UNIT_VALUE);

    packet << static_cast<uint8_t>(REMOTE_PACKET_17);
    packet << static_cast<uint16_t>(0);

    packet << GameManager_GameState;
    packet << world;
    packet << game_file_number;
    packet << game_file_type;
    packet << play_mode;
    packet << all_visible;
    packet << quick_build;
    packet << real_time;
    packet << log_file_debug;
    packet << disable_fire;
    packet << fast_movement;
    packet << timer;
    packet << endturn;
    packet << start_gold;
    packet << auto_save;
    packet << victory_type;
    packet << victory_limit;
    packet << raw_normal_low;
    packet << raw_normal_high;
    packet << raw_concentrate_low;
    packet << raw_concentrate_high;
    packet << raw_concentrate_seperation;
    packet << raw_concentrate_diffusion;
    packet << fuel_normal_low;
    packet << fuel_normal_high;
    packet << fuel_concentrate_low;
    packet << fuel_concentrate_high;
    packet << fuel_concentrate_seperation;
    packet << fuel_concentrate_diffusion;
    packet << gold_normal_low;
    packet << gold_normal_high;
    packet << gold_concentrate_low;
    packet << gold_concentrate_high;
    packet << gold_concentrate_seperation;
    packet << gold_concentrate_diffusion;
    packet << mixed_resource_seperation;
    packet << min_resources;
    packet << max_resources;
    packet << alien_seperation;
    packet << alien_unit_value;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_17(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    char game_state;
    int32_t world;
    int32_t game_file_number;
    int32_t game_file_type;
    int32_t play_mode;
    int32_t all_visible;
    int32_t quick_build;
    int32_t real_time;
    int32_t log_file_debug;
    int32_t disable_fire;
    int32_t fast_movement;
    int32_t timer;
    int32_t endturn;
    int32_t backup_timer;
    int32_t backup_endturn;
    int32_t start_gold;
    int32_t auto_save;
    int32_t victory_type;
    int32_t victory_limit;
    int32_t raw_normal_low;
    int32_t raw_normal_high;
    int32_t raw_concentrate_low;
    int32_t raw_concentrate_high;
    int32_t raw_concentrate_seperation;
    int32_t raw_concentrate_diffusion;
    int32_t fuel_normal_low;
    int32_t fuel_normal_high;
    int32_t fuel_concentrate_low;
    int32_t fuel_concentrate_high;
    int32_t fuel_concentrate_seperation;
    int32_t fuel_concentrate_diffusion;
    int32_t gold_normal_low;
    int32_t gold_normal_high;
    int32_t gold_concentrate_low;
    int32_t gold_concentrate_high;
    int32_t gold_concentrate_seperation;
    int32_t gold_concentrate_diffusion;
    int32_t mixed_resource_seperation;
    int32_t min_resources;
    int32_t max_resources;
    int32_t alien_seperation;
    int32_t alien_unit_value;

    backup_timer = ini_get_setting(INI_TIMER);
    backup_endturn = ini_get_setting(INI_ENDTURN);

    packet >> game_state;
    packet >> world;
    packet >> game_file_number;
    packet >> game_file_type;
    packet >> play_mode;
    packet >> all_visible;
    packet >> quick_build;
    packet >> real_time;
    packet >> log_file_debug;
    packet >> disable_fire;
    packet >> fast_movement;
    packet >> timer;
    packet >> endturn;
    packet >> start_gold;
    packet >> auto_save;
    packet >> victory_type;
    packet >> victory_limit;
    packet >> raw_normal_low;
    packet >> raw_normal_high;
    packet >> raw_concentrate_low;
    packet >> raw_concentrate_high;
    packet >> raw_concentrate_seperation;
    packet >> raw_concentrate_diffusion;
    packet >> fuel_normal_low;
    packet >> fuel_normal_high;
    packet >> fuel_concentrate_low;
    packet >> fuel_concentrate_high;
    packet >> fuel_concentrate_seperation;
    packet >> fuel_concentrate_diffusion;
    packet >> gold_normal_low;
    packet >> gold_normal_high;
    packet >> gold_concentrate_low;
    packet >> gold_concentrate_high;
    packet >> gold_concentrate_seperation;
    packet >> gold_concentrate_diffusion;
    packet >> mixed_resource_seperation;
    packet >> min_resources;
    packet >> max_resources;
    packet >> alien_seperation;
    packet >> alien_unit_value;

    GameManager_PlayMode = play_mode;
    GameManager_RealTime = real_time;
    GameManager_FastMovement = fast_movement;

    ini_setting_victory_type = victory_type;
    ini_setting_victory_limit = victory_limit;

    ini_set_setting(INI_PLAY_MODE, play_mode);
    ini_set_setting(INI_ALL_VISIBLE, all_visible);
    ini_set_setting(INI_QUICK_BUILD, quick_build);
    ini_set_setting(INI_REAL_TIME, real_time);
    ini_set_setting(INI_LOG_FILE_DEBUG, log_file_debug);
    ini_set_setting(INI_DISABLE_FIRE, disable_fire);
    ini_set_setting(INI_FAST_MOVEMENT, fast_movement);
    ini_set_setting(INI_TIMER, timer);
    ini_set_setting(INI_ENDTURN, endturn);
    ini_set_setting(INI_START_GOLD, start_gold);
    ini_set_setting(INI_AUTO_SAVE, auto_save);
    ini_set_setting(INI_RAW_NORMAL_LOW, raw_normal_low);
    ini_set_setting(INI_RAW_NORMAL_HIGH, raw_normal_high);
    ini_set_setting(INI_RAW_CONCENTRATE_LOW, raw_concentrate_low);
    ini_set_setting(INI_RAW_CONCENTRATE_HIGH, raw_concentrate_high);
    ini_set_setting(INI_RAW_CONCENTRATE_SEPERATION, raw_concentrate_seperation);
    ini_set_setting(INI_RAW_CONCENTRATE_DIFFUSION, raw_concentrate_diffusion);
    ini_set_setting(INI_FUEL_NORMAL_LOW, fuel_normal_low);
    ini_set_setting(INI_FUEL_NORMAL_HIGH, fuel_normal_high);
    ini_set_setting(INI_FUEL_CONCENTRATE_LOW, fuel_concentrate_low);
    ini_set_setting(INI_FUEL_CONCENTRATE_HIGH, fuel_concentrate_high);
    ini_set_setting(INI_FUEL_CONCENTRATE_SEPERATION, fuel_concentrate_seperation);
    ini_set_setting(INI_FUEL_CONCENTRATE_DIFFUSION, fuel_concentrate_diffusion);
    ini_set_setting(INI_GOLD_NORMAL_LOW, gold_normal_low);
    ini_set_setting(INI_GOLD_NORMAL_HIGH, gold_normal_high);
    ini_set_setting(INI_GOLD_CONCENTRATE_LOW, gold_concentrate_low);
    ini_set_setting(INI_GOLD_CONCENTRATE_HIGH, gold_concentrate_high);
    ini_set_setting(INI_GOLD_CONCENTRATE_SEPERATION, gold_concentrate_seperation);
    ini_set_setting(INI_GOLD_CONCENTRATE_DIFFUSION, gold_concentrate_diffusion);
    ini_set_setting(INI_MIXED_RESOURCE_SEPERATION, mixed_resource_seperation);
    ini_set_setting(INI_MIN_RESOURCES, min_resources);
    ini_set_setting(INI_MAX_RESOURCES, max_resources);
    ini_set_setting(INI_ALIEN_SEPERATION, alien_seperation);
    ini_set_setting(INI_ALIEN_UNIT_VALUE, alien_unit_value);

    if (GameManager_GameState == GAME_STATE_3_MAIN_MENU) {
        GameManager_GameState = game_state;

        ini_set_setting(INI_WORLD, world);
        ini_set_setting(INI_GAME_FILE_NUMBER, game_file_number);
        ini_set_setting(INI_GAME_FILE_TYPE, game_file_type);

    } else {
        if (GameManager_AllVisible != all_visible) {
            Access_UpdateVisibilityStatus(all_visible);
        }

        if (backup_timer != timer || backup_endturn != endturn) {
            MessageManager_DrawMessage(_(7be4), 1, 0);
        }
    }
}

void Remote_SendNetPacket_18(int32_t sender_team, int32_t addresse_team, const char* message) {
    int32_t message_length;

    message_length = strlen(message);

    if (message_length && Remote_IsNetworkGame) {
        NetPacket packet;

        packet << static_cast<uint8_t>(REMOTE_PACKET_18);
        packet << static_cast<uint16_t>(sender_team);

        packet << SmartString(message);

        Remote_TransmitPacket(packet, addresse_team);
    }
}

void Remote_ReceiveNetPacket_18(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    char team_name[30];

    ini_config.GetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + entity_id), team_name, sizeof(team_name));

    SmartString message_text;
    SmartString message(team_name);

    packet >> message;

    message += ": ";
    message += message_text;

    MessageManager_DrawMessage(message.GetCStr(), 1, 0, true, false);
    MessageManager_AddMessage(message.GetCStr(), LIPS);
}

void Remote_SendNetPacket_20(UnitInfo* unit) {
    NetPacket packet;
    SmartPointer<UnitValues> unit_values(unit->GetBaseValues());

    packet << static_cast<uint8_t>(REMOTE_PACKET_20);
    packet << static_cast<uint16_t>(unit->GetId());

    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_TURNS));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_HITS));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_ARMOR));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_ATTACK));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_SPEED));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_RANGE));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_ROUNDS));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_MOVE_AND_FIRE));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_SCAN));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_STORAGE));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_AMMO));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_ATTACK_RADIUS));
    packet << static_cast<uint16_t>(unit_values->GetAttribute(ATTRIB_AGENT_ADJUST));

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_20(NetPacket& packet) {
    uint16_t entity_id;
    uint16_t value;

    packet >> entity_id;

    SmartPointer<UnitInfo> unit(Hash_UnitHash[entity_id]);

    if (unit) {
        SmartPointer<UnitValues> unit_values(unit->GetBaseValues());

        unit_values->UpdateVersion();
        unit_values->SetUnitsBuilt(1);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_TURNS, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_HITS, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_ARMOR, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_ATTACK, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_SPEED, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_RANGE, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_ROUNDS, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_MOVE_AND_FIRE, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_SCAN, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_STORAGE, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_AMMO, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_ATTACK_RADIUS, value);

        packet >> value;
        unit_values->SetAttribute(ATTRIB_AGENT_ADJUST, value);

        unit->SetBaseValues(&*unit_values);

    } else {
        Remote_NetErrorUnknownUnit(entity_id);
    }
}

void Remote_ReceiveNetPacket_21(NetPacket& packet) {
    uint16_t entity_id;
    uint16_t complex_id;
    uint16_t material;
    uint16_t fuel;
    uint16_t gold;

    packet >> entity_id;

    packet >> complex_id;
    packet >> material;
    packet >> fuel;
    packet >> gold;

    UnitsManager_TeamInfo[entity_id].team_units->GetComplex(complex_id)->Transfer(material, fuel, gold);
}

void Remote_ReceiveNetPacket_22(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    UnitInfo* unit = Hash_UnitHash[entity_id];

    if (unit) {
        unit->ReadPacket(packet);

    } else {
        Remote_NetErrorUnknownUnit(entity_id);
    }
}

void Remote_CreateNetPacket_23(UnitInfo* unit, NetPacket& packet) {
    Remote_OrderProcessor5_Write(unit, packet);

    packet << unit->build_time;
    packet << unit->build_rate;
    packet << unit->unit_type;
    packet << unit->unit_id;
    packet << unit->grid_x;
    packet << unit->grid_y;
    packet << unit->team;
    packet << unit->hits;
    packet << unit->speed;
    packet << unit->shots;
    packet << unit->storage;
    packet << unit->ammo;
}

void Remote_ProcessNetPacket_23(struct Packet23Data& data, NetPacket& packet) {
    NetPacket local;
    uint8_t packet_type;
    uint16_t entity_id;

    local.Write(packet.GetBuffer(), packet.GetDataSize());

    local >> packet_type;
    local >> entity_id;

    local >> data.orders;
    local >> data.state;
    local >> data.prior_orders;
    local >> data.prior_state;
    local >> data.disabled_reaction_fire;

    local >> data.parent_unit_id;

    local >> data.target_grid_x;
    local >> data.target_grid_y;
    local >> data.enemy_unit_id;

    local >> data.total_mining;
    local >> data.raw_mining;
    local >> data.fuel_mining;
    local >> data.gold_mining;

    local >> data.build_time;
    local >> data.build_rate;
    local >> data.unit_type;
    local >> data.unit_id;
    local >> data.grid_x;
    local >> data.grid_y;
    local >> data.team;
    local >> data.hits;
    local >> data.speed;
    local >> data.shots;
    local >> data.storage;
    local >> data.ammo;
}

void Remote_SendNetPacket_23(UnitInfo* unit) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_23);
    packet << unit->GetId();

    Remote_CreateNetPacket_23(unit, packet);

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_23(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    Remote_P23_UnitId = entity_id;

    Remote_P23_Packet.Reset();

    Remote_P23_Packet << static_cast<uint8_t>(REMOTE_PACKET_23);
    Remote_P23_Packet << static_cast<uint16_t>(entity_id);

    Remote_P23_Packet.Write(packet.GetBuffer(), packet.GetDataSize());
}

void Remote_ReceiveNetPacket_24(NetPacket& packet) {
    uint16_t entity_id;
    bool status;

    packet >> entity_id;

    packet >> status;

    Remote_P24_Signals[entity_id] = status;
}

void Remote_ReceiveNetPacket_26(NetPacket& packet) {
    uint16_t entity_id;
    uint8_t team_clan;

    packet >> entity_id;

    packet >> team_clan;

    ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_CLAN + entity_id), team_clan);

    ResourceManager_InitClanUnitValues(entity_id);
}

void Remote_SendNetPacket_28(int32_t node) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_28);
    packet << static_cast<uint16_t>(node);

    Remote_TransmitPacket(packet, REMOTE_BROADCAST);
}

void Remote_ReceiveNetPacket_28(NetPacket& packet) {
    if (Remote_GameState == 1 && Remote_NetworkMenu->is_host_mode) {
        NetNode client_node;
        client_node.address = packet.GetAddress(REMOTE_RECEIVED_ADDRESS);
        client_node.entity_id = 0;
        client_node.name[0] = '\0';
        client_node.is_host = false;

        Remote_Clients.Add(client_node);

        Remote_SendNetPacket_44(packet.GetAddress(REMOTE_RECEIVED_ADDRESS));
    }
}

void Remote_SendNetPacket_29(int32_t node) {
    NetPacket packet;
    NetNode* host_node;

    packet << static_cast<uint8_t>(REMOTE_PACKET_29);
    packet << static_cast<uint16_t>(node);

    host_node = Remote_Hosts.Find(node);
    if (host_node) {
        Remote_Nodes.Add(*host_node);

        packet.AddAddress(host_node->address);

        Remote_TransmitPacket(packet, REMOTE_UNICAST);
    } else {
        SDL_Log("Remote: Attempted to register to host that does not exist.\n");
    }
}

void Remote_ReceiveNetPacket_29(NetPacket& packet) {
    uint16_t host_node;

    packet >> host_node;

    if (Remote_GameState == 1 && Remote_NetworkMenu->player_node == host_node) {
        NetNode client_node;

        client_node.address = packet.GetAddress(REMOTE_RECEIVED_ADDRESS);
        client_node.entity_id = Remote_GenerateEntityId();
        client_node.is_host = false;
        client_node.name[0] = '\0';

        Remote_Nodes.Add(client_node);
        Remote_SendNetPacket_30(packet.GetAddress(REMOTE_RECEIVED_ADDRESS));
    }
}

void Remote_SendNetPacket_30(NetAddress& address) {
    NetPacket packet;
    NetNode* node;

    node = Remote_Nodes.Find(address);

    packet << static_cast<uint8_t>(REMOTE_PACKET_30);
    packet << node->entity_id;

    packet << Remote_NetworkMenu->player_node;
    packet << Remote_NetworkMenu->team_nodes;
    packet << Remote_NetworkMenu->team_names;

    packet << Remote_Nodes.GetCount();

    for (int32_t i = 0; i < Remote_Nodes.GetCount(); ++i) {
        packet << *Remote_Nodes[i];
    }

    Remote_WriteGameSettings(packet);

    packet.AddAddress(address);

    Remote_TransmitPacket(packet, REMOTE_UNICAST);
}

void Remote_ReceiveNetPacket_30(NetPacket& packet) {
    if (Remote_GameState == 1) {
        uint16_t player_node;
        uint16_t host_node;
        uint16_t node_count;

        packet >> player_node;
        Remote_NetworkMenu->player_node = player_node;

        packet >> host_node;
        Remote_NetworkMenu->host_node = host_node;

        packet >> Remote_NetworkMenu->team_nodes;
        packet >> Remote_NetworkMenu->team_names;

        packet >> node_count;

        for (int32_t i = 0; i < node_count; ++i) {
            NetNode peer_node;

            packet >> peer_node;
            Remote_Nodes.Add(peer_node);
        }

        SDL_assert(Remote_Nodes.Find(player_node) && Remote_Nodes.Find(host_node));

        Remote_ReadGameSettings(packet);

        Remote_NetworkMenu->is_gui_update_needed = true;
    }
}

void Remote_SendNetPacket_31(int32_t node) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_31);
    packet << static_cast<uint16_t>(node);

    if (Remote_NetworkMenu->is_host_mode) {
        for (int32_t i = 0; i < Remote_Clients.GetCount(); ++i) {
            packet.AddAddress(Remote_Clients[i]->address);
        }

        Remote_Clients.Clear();
    }

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_31(NetPacket& packet) {
    if (Remote_GameState == 1) {
        uint16_t entity_id;

        packet >> entity_id;

        if (Remote_NetworkMenu->host_node == entity_id) {
            Remote_NetworkMenu->host_node = 0;
            Remote_NetworkMenu->client_state = 0;
            Remote_NetworkMenu->player_team = -1;
        }

        Remote_NetworkMenu->LeaveGame(entity_id);
    }
}

void Remote_SendNetPacket_32(int32_t transmit_mode) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_32);
    packet << static_cast<uint16_t>(Remote_NetworkMenu->host_node);

    Remote_TransmitPacket(packet, transmit_mode);
}

void Remote_ReceiveNetPacket_32(NetPacket& packet) {
    if (Remote_GameState == 1) {
        uint16_t entity_id;

        packet >> entity_id;

        if (Remote_NetworkMenu->host_node == entity_id) {
            if (Remote_NetworkMenu->player_team == -1) {
                Remote_NetworkMenu->host_node = 0;
                Remote_NetworkMenu->client_state = 0;
                Remote_NetworkMenu->player_team = -1;

                Remote_NetworkMenu->LeaveGame(entity_id);

            } else {
                Remote_SetupPlayers();

                Remote_GameState = 2;

                Remote_SendNetPacket_05(REMOTE_UNICAST);
            }
        } else {
            Remote_NetworkMenu->LeaveGame(entity_id);
        }
    }
}

void Remote_SendNetPacket_33() {
    if (strlen(Remote_NetworkMenu->chat_input_buffer)) {
        NetPacket packet;

        packet << static_cast<uint8_t>(REMOTE_PACKET_33);
        packet << static_cast<uint16_t>(Remote_NetworkMenu->host_node);

        SmartString string(Remote_NetworkMenu->player_name);
        string += ": ";
        string += Remote_NetworkMenu->chat_input_buffer;

        packet << string;

        Remote_TransmitPacket(packet, REMOTE_MULTICAST);
    }
}

void Remote_ReceiveNetPacket_33(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    if (Remote_GameState == 1 && Remote_NetworkMenu->host_node == entity_id) {
        SmartString string;

        packet >> string;

        strncpy(Remote_NetworkMenu->chat_message_buffer, string.GetCStr(),
                sizeof(Remote_NetworkMenu->chat_message_buffer));

        Remote_NetworkMenu->is_gui_update_needed = true;
    }
}

void Remote_SendNetPacket_34() {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_34);
    packet << static_cast<uint16_t>(Remote_NetworkMenu->host_node);

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_34(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    if (Remote_NetworkMenu->host_node == entity_id) {
        Remote_NetworkMenu->connection_state = true;
    }
}

void Remote_SendNetPacket_35() {
    if (Remote_Nodes.GetCount()) {
        NetPacket packet;

        packet << static_cast<uint8_t>(REMOTE_PACKET_35);
        packet << static_cast<uint16_t>(Remote_NetworkMenu->host_node);

        packet << Remote_NetworkMenu->player_node;
        packet << Remote_NetworkMenu->player_team;
        packet << (Remote_NetworkMenu->client_state == 2);
        packet << SmartString(Remote_NetworkMenu->player_name);

        Remote_TransmitPacket(packet, REMOTE_MULTICAST);
    }
}

void Remote_ReceiveNetPacket_35(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    if (Remote_GameState == 1 && Remote_NetworkMenu->host_node == entity_id) {
        uint16_t source_node;
        char team_slot;
        bool ready_state;
        SmartString team_name;

        packet >> source_node;

        for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
            if (Remote_NetworkMenu->team_nodes[i] == source_node) {
                Remote_NetworkMenu->ResetJar(i);
                --Remote_NetworkMenu->remote_player_count;
            }
        }

        packet >> team_slot;
        packet >> ready_state;

        Remote_NetworkMenu->team_nodes[team_slot] = source_node;
        Remote_NetworkMenu->team_jar_in_use[team_slot] = ready_state;

        packet >> team_name;

        strcpy(Remote_NetworkMenu->team_names[team_slot], team_name.GetCStr());
        ++Remote_NetworkMenu->remote_player_count;

        Remote_NetworkMenu->is_gui_update_needed = true;
    }
}

void Remote_SendNetPacket_36() {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_36);
    packet << static_cast<uint16_t>(Remote_NetworkMenu->player_node);

    packet << Remote_NetworkMenu->player_name;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_36(NetPacket& packet) {
    if (Remote_GameState == 1) {
        uint16_t entity_id;
        char player_name[30];
        NetNode* host_node;

        packet >> entity_id;
        packet >> player_name;

        host_node = Remote_Hosts.Find(entity_id);
        if (host_node) {
            strcpy(host_node->name, player_name);

            Remote_NetworkMenu->is_gui_update_needed = true;
        }

        for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
            if (Remote_NetworkMenu->team_nodes[i] == entity_id) {
                strcpy(Remote_NetworkMenu->team_names[i], player_name);

                Remote_NetworkMenu->is_gui_update_needed = true;
            }
        }
    }
}

void Remote_SendNetPacket_37() {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_37);
    packet << static_cast<uint16_t>(Remote_NetworkMenu->player_node);

    Remote_WriteGameSettings(packet);

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_37(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    if (Remote_GameState == 1 && Remote_NetworkMenu->host_node == entity_id) {
        Remote_ReadGameSettings(packet);

        Remote_NetworkMenu->is_gui_update_needed = true;
    }
}

void Remote_SendNetPacket_38(UnitInfo* unit) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_38);
    packet << static_cast<uint16_t>(unit->GetId());

    packet << unit->orders;
    packet << unit->state;
    packet << unit->target_grid_x;
    packet << unit->target_grid_y;
    packet << unit->group_speed;

    uint16_t steps_count;

    if (unit->path) {
        if (unit->flags & MOBILE_AIR_UNIT) {
            steps_count = 1;

            packet << steps_count;

            packet << unit->path->GetEndX();
            packet << unit->path->GetEndY();
            packet << unit->path->GetDistanceX();
            packet << unit->path->GetDistanceY();
            packet << unit->path->GetEuclideanDistance();

            packet << unit->speed;
            packet << unit->move_fraction;
            packet << unit->max_velocity;

        } else {
            unit->path->WritePacket(packet);
        }
    } else {
        steps_count = 0;

        packet << steps_count;
    }

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_38(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    UnitInfo* unit = Hash_UnitHash[entity_id];

    if (unit) {
        uint16_t steps_count;

        if (unit->path) {
            unit->path = nullptr;
        }

        UnitsManager_NewOrderWhileScaling(unit);

        packet >> unit->orders;
        packet >> unit->state;
        packet >> unit->target_grid_x;
        packet >> unit->target_grid_y;
        packet >> unit->group_speed;

        packet >> steps_count;

        if (steps_count) {
            if (unit->flags & MOBILE_AIR_UNIT) {
                int16_t end_x;
                int16_t end_y;
                int32_t distance_x;
                int32_t distance_y;
                int16_t euclidean_distance;

                packet >> end_x;
                packet >> end_y;
                packet >> distance_x;
                packet >> distance_y;
                packet >> euclidean_distance;

                unit->Redraw();

                unit->path = new (std::nothrow) AirPath(unit, distance_x, distance_y, euclidean_distance, end_x, end_y);

                packet >> unit->speed;
                packet >> unit->move_fraction;
                packet >> unit->max_velocity;

            } else {
                unit->path = new (std::nothrow) GroundPath(unit->target_grid_x, unit->target_grid_y);

                unit->path->ReadPacket(packet, steps_count);
            }
        }

        unit->Redraw();

    } else {
        Remote_NetErrorUnknownUnit(entity_id);
    }
}

void Remote_ReceiveNetPacket_39(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    DialogMenu_Menu(_(1dc1));
}

void Remote_ReceiveNetPacket_40(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    Remote_UnpauseGameEvent = true;
}

void Remote_SendNetPacket_41(UnitInfo* unit, bool mode) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_41);
    packet << static_cast<uint16_t>(unit->GetId());
    packet << mode;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_41(NetPacket& packet) {
    uint16_t entity_id;
    bool mode;

    packet >> entity_id;
    packet >> mode;

    UnitInfo* unit = Hash_UnitHash[entity_id];

    if (unit) {
        unit->BlockedOnPathRequest(mode, true);

    } else {
        Remote_NetErrorUnknownUnit(entity_id);
    }
}

void Remote_ReceiveNetPacket_42(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    TeamMissionSupplies* supplies = &UnitsManager_TeamMissionSupplies[entity_id];

    supplies->units.Clear();
    supplies->cargos.Clear();
}

void Remote_SendNetPacket_43(UnitInfo* unit, const char* name) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_43);
    packet << static_cast<uint16_t>(unit->GetId());

    packet << SmartString(name);

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_43(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    UnitInfo* unit = Hash_UnitHash[entity_id];

    if (unit) {
        SmartString unit_name;

        packet >> unit_name;

        unit->SetName(unit_name.GetCStr());

    } else {
        Remote_NetErrorUnknownUnit(entity_id);
    }
}

void Remote_SendNetPacket_44(NetAddress& address) {
    NetPacket packet;

    packet.AddAddress(address);

    packet << static_cast<uint8_t>(REMOTE_PACKET_44);
    packet << static_cast<uint16_t>(Remote_NetworkMenu->player_node);
    packet << static_cast<uint32_t>(GAME_VERSION);
    packet << SmartString(Remote_NetworkMenu->player_name);

    Remote_TransmitPacket(packet, REMOTE_UNICAST);
}

void Remote_ReceiveNetPacket_44(NetPacket& packet) {
    if (Remote_GameState == 1) {
        uint16_t entity_id;

        packet >> entity_id;

        if (!Remote_Hosts.Find(entity_id)) {
            uint32_t game_version;

            packet >> game_version;
            if (game_version == GAME_VERSION) {
                NetNode host_node;
                SmartString host_name;

                packet >> host_name;
                host_node.address = packet.GetAddress(REMOTE_RECEIVED_ADDRESS);
                host_node.entity_id = entity_id;
                strcpy(host_node.name, host_name.GetCStr());
                host_node.is_host = true;

                Remote_Hosts.Add(host_node);

                Remote_NetworkMenu->is_gui_update_needed = true;
            }
        }
    }
}

void Remote_SendNetPacket_45(uint16_t team, uint8_t next_turn_index, uint16_t crc_checksum) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_45);
    packet << static_cast<uint16_t>(team);

    packet << next_turn_index;
    packet << crc_checksum;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_45(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    packet >> Remote_NextTurnIndices[entity_id];
    packet >> Remote_TeamDataCrc16[entity_id];
}

void Remote_SendNetPacket_46(uint16_t team, bool state, uint32_t counter) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_46);
    packet << static_cast<uint16_t>(team);

    packet << state;
    packet << counter;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_46(NetPacket& packet) {
    uint16_t entity_id;
    bool state;
    uint32_t counter;

    packet >> entity_id;
    packet >> state;
    packet >> counter;

    if (state) {
        UnitsManager_DelayedReactionsPending = true;
    }

    if (counter > UnitsManager_DelayedReactionsSyncCounter) {
        UnitsManager_DelayedReactionsTeam = entity_id;
        UnitsManager_DelayedReactionsSyncCounter = counter;
    }
}

void Remote_ReceiveNetPacket_48(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    packet >> Remote_LeaveGameRequestId[entity_id];
}

void Remote_ReceiveNetPacket_49(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    Remote_P49_Signal = true;
}

void Remote_SendNetPacket_50(UnitInfo* unit) {
    NetPacket packet;

    packet << static_cast<uint8_t>(REMOTE_PACKET_50);
    packet << static_cast<uint16_t>(unit->GetId());

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_50(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    UnitInfo* unit = Hash_UnitHash[entity_id];

    if (unit) {
        UnitEventEmergencyStop* unit_event = new (std::nothrow) UnitEventEmergencyStop(unit);

        UnitEvent_UnitEvents.PushBack(*unit_event);

    } else {
        Remote_NetErrorUnknownUnit(entity_id);
    }
}

void Remote_ReceiveNetPacket_51(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    Remote_P51_Signal = true;
}

void Remote_ReceiveNetPacket_52(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    packet >> Remote_TurnIndices[entity_id];
}
