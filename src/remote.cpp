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

#include "game_manager.hpp"
#include "inifile.hpp"
#include "message_manager.hpp"
#include "networkmenu.hpp"
#include "transport.hpp"
#include "units_manager.hpp"
#include "version.hpp"
#include "window_manager.hpp"

enum {
    REMOTE_UNICAST,
    REMOTE_MULTICAST,
    REMOTE_BROADCAST,
};

enum {
    REMOTE_RECEIVED_ADDRESS,
};

NetNodeArray Remote_Nodes;

NetNodeArray Remote_Hosts;
NetNodeArray Remote_Clients;

unsigned char Remote_GameState;
bool Remote_IsHostMode;
bool Remote_IsNetworkGame;
bool Remote_UpdatePauseTimer;
bool Remote_UnpauseGameEvent;
unsigned int Remote_PauseTimeStamp;
unsigned int Remote_TimeoutTimeStamp;
unsigned int Remote_RngSeed;
Transport* Remote_Transport;
NetworkMenu* Remote_NetworkMenu;

static void Remote_ReceiveNetPacket_28(NetPacket& packet);
static void Remote_ReceiveNetPacket_29(NetPacket& packet);
static void Remote_ReceiveNetPacket_30(NetPacket& packet);
static void Remote_ReceiveNetPacket_31(NetPacket& packet);
static void Remote_ReceiveNetPacket_33(NetPacket& packet);
static void Remote_ReceiveNetPacket_35(NetPacket& packet);
static void Remote_ReceiveNetPacket_36(NetPacket& packet);
static void Remote_ReceiveNetPacket_37(NetPacket& packet);
static void Remote_ReceiveNetPacket_44(NetPacket& packet);

static unsigned short Remote_GenerateEntityId() {
    unsigned short new_entity_id;

    for (;;) {
        new_entity_id = ((dos_rand() * 31991) >> 15) + 10;

        if (!Remote_Nodes.Find(new_entity_id)) {
            break;
        }
    }

    return new_entity_id;
}

static void Remote_UpdateEntityId(NetAddress& address, unsigned short entity_id) {
    for (int i = 0; i < Remote_Nodes.GetCount(); ++i) {
        if (Remote_Nodes[i]->address == address) {
            Remote_Nodes[i]->entity_id = entity_id;
        }
    }
}

static void Remote_WriteGameSettings(NetPacket& packet) {
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

static void Remote_ReadGameSettings(NetPacket& packet) {
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

struct packetcb {
    void* read;
    void* write;
};

packetcb ipx_packet_cb_array[32];

static void ipx_packet_cb_11(UnitInfo& unit, NetPacket& packet) {
    packet << unit.orders;
    packet << unit.state;
    packet << unit.prior_orders;
    packet << unit.prior_state;
    packet << unit.disabled_reaction_fire;
}

static void ipx_packet_cb_12(UnitInfo& unit, NetPacket& packet) {
    //    sub_10186C(unit);

    packet >> unit.orders;
    packet >> unit.state;
    packet >> unit.prior_orders;
    packet >> unit.prior_state;
    packet >> unit.disabled_reaction_fire;
}

static int Remote_SetupPlayers() { return 0; }

void Remote_Init() {
    WindowInfo* window;

    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);

    Remote_GameState = 0;
    Remote_RngSeed = 0;
    GameManager_PlayerTeam = 0;
    //    Remote_FrameSyncCounter2 = 0;
    //    Remote_send_p1_sync_frame = 1;
    //    Remote_byte_1759C5 = 0;
    //    Remote_remote_player_count = 0;
    //    Remote_P51_signal = 0;

    for (int i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
        //        Remote_FrameSyncCounter2values[i] = 0;
        //        Remote_FrameSyncCounter[i] = 0;
        //        Remote_NextTurnIndexArray[i] = 0;
        //        Remote_LeaveGameRequestId[i] = 0;
        //        Remote_TurnIndexArray[i] = 0;
        //        Remote_P24_signal[i] = 0;
    }

    Remote_Nodes.Clear();
    Remote_Hosts.Clear();

    //    for (int i = 0; i < 32; ++i) {
    //        switch (i) {
    //            case 0:
    //            case 7:
    //            case 8:
    //            case 9:
    //            case 12:
    //            case 13:
    //            case 14:
    //            case 15:
    //            case 16:
    //            case 21:
    //            case 22:
    //            case 23:
    //            case 29:
    //            case 31: {
    //                ipx_packet_cb_array[i].read = ipx_packet_cb_11;
    //                ipx_packet_cb_array[i].write = ipx_packet_cb_12;
    //            } break;
    //
    //            case 1:
    //            case 4:
    //            case 11:
    //            case 26: {
    //                ipx_packet_cb_array[i].read = ipx_packet_cb_41;
    //                ipx_packet_cb_array[i].write = ipx_packet_cb_42;
    //            } break;
    //
    //            case 2:
    //            case 3:
    //            case 5:
    //            case 20:
    //            case 24:
    //            case 25:
    //            case 27:
    //            case 30: {
    //                ipx_packet_cb_array[i].read = ipx_packet_cb_31;
    //                ipx_packet_cb_array[i].write = ipx_packet_cb_32;
    //            } break;
    //
    //            case 6: {
    //                ipx_packet_cb_array[i].read = ipx_packet_cb_51;
    //                ipx_packet_cb_array[i].write = ipx_packet_cb_52;
    //            } break;
    //
    //            case 10:
    //            case 17:
    //            case 18:
    //            case 19:
    //            case 28: {
    //                ipx_packet_cb_array[i].read = ipx_packet_cb_21;
    //                ipx_packet_cb_array[i].write = ipx_packet_cb_22;
    //            } break;
    //        }
    //    }
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

static void Remote_TransmitPacket(NetPacket& packet, int transmit_mode) {
    switch (transmit_mode) {
        case REMOTE_UNICAST: {
            SDL_assert(packet.GetAddressCount() == 1);
        } break;

        case REMOTE_MULTICAST: {
            for (int i = 0; i < Remote_Nodes.GetCount(); ++i) {
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
    } else {
        unsigned char packet_type;
        packet.Peek(0, &packet_type, sizeof(packet_type));
        SDL_Log("Remote: Transmit packet (%i).\n", packet_type);
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

int Remote_Lobby(bool is_host_mode) {
    WindowInfo* window;
    int result;
    char ini_transport[30];
    int transort_type;

    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);

    Remote_IsHostMode = is_host_mode;

    /// \todo Implement missing stuff
    // CTInfo_Init();
    Remote_Init();

    if (ini_config.GetStringValue(INI_NETWORK_TRANSPORT, ini_transport, sizeof(ini_transport))) {
        if (!strcmp(ini_transport, "udp_default")) {
            transort_type = TRANSPORT_DEFAULT_UDP;
        } else {
            SDL_Log("Remote: Unknown transport type (%s).\n", ini_transport);
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
            WindowManager_LoadImage(MAINPIC, WindowManager_GetWindow(WINDOW_MAIN_WINDOW), 640, false, true);
            MessageManager_DrawMessage(Remote_Transport->GetError(), 2, 1);

            result = false;
        }

    } else {
        SDL_Log("Remote: Unable to initialize network transport layer.\n");
        result = false;
    }

    return result;
}

void Remote_SetupConnection() {
    unsigned int rng_number;
    int remote_player_count;

    rng_number = dos_rand();
    remote_player_count = Remote_SetupPlayers();

    Remote_GameState = 2;
    Remote_NetworkMenu->remote_player_count = 0;

    Remote_SendNetPacket_32(rng_number, REMOTE_BROADCAST);

    Remote_Transport->SetSessionId(rng_number);

    while (Remote_NetworkMenu->remote_player_count != remote_player_count) {
        Remote_ProcessNetPackets();
        if (get_input() == GNW_KB_KEY_ESCAPE) {
            return;
        }
    }

    Remote_SendNetPacket_34();

    for (int i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
        if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_REMOTE) {
            //            ipx_rx_packetnum[i] = 1;
        }
    }

    Remote_NetworkMenu->connection_state = 1;
}

bool Remote_NetSync() {
    Remote_ProcessNetPackets();

    return Remote_NetworkMenu->is_gui_update_needed;
}

bool Remote_sub_C8835(bool mode) { return false; }

bool Remote_CheckRestartAfterDesyncEvent() { return false; }

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
        unsigned char packet_type;

        if (Remote_ReceivePacket(packet)) {
            packet >> packet_type;

            SDL_Log("Remote: Received packet (%i).\n", packet_type);

            switch (packet_type) {
                case REMOTE_PACKET_00: {
                } break;

                case REMOTE_PACKET_01: {
                } break;

                case REMOTE_PACKET_02: {
                } break;

                case REMOTE_PACKET_03: {
                } break;

                case REMOTE_PACKET_04: {
                } break;

                case REMOTE_PACKET_05: {
                } break;

                case REMOTE_PACKET_06: {
                } break;

                case REMOTE_PACKET_07: {
                } break;

                case REMOTE_PACKET_08: {
                } break;

                case REMOTE_PACKET_09: {
                } break;

                case REMOTE_PACKET_10: {
                } break;

                case REMOTE_PACKET_11: {
                } break;

                case REMOTE_PACKET_12: {
                } break;

                case REMOTE_PACKET_13: {
                } break;

                case REMOTE_PACKET_14: {
                } break;

                case REMOTE_PACKET_15: {
                } break;

                case REMOTE_PACKET_16: {
                } break;

                case REMOTE_PACKET_17: {
                } break;

                case REMOTE_PACKET_18: {
                } break;

                case REMOTE_PACKET_19: {
                } break;

                case REMOTE_PACKET_20: {
                } break;

                case REMOTE_PACKET_21: {
                } break;

                case REMOTE_PACKET_22: {
                } break;

                case REMOTE_PACKET_23: {
                } break;

                case REMOTE_PACKET_24: {
                } break;

                case REMOTE_PACKET_25: {
                } break;

                case REMOTE_PACKET_26: {
                } break;

                case REMOTE_PACKET_27: {
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
                } break;

                case REMOTE_PACKET_33: {
                    Remote_ReceiveNetPacket_33(packet);
                } break;

                case REMOTE_PACKET_34: {
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
                } break;

                case REMOTE_PACKET_39: {
                } break;

                case REMOTE_PACKET_40: {
                } break;

                case REMOTE_PACKET_41: {
                } break;

                case REMOTE_PACKET_42: {
                } break;

                case REMOTE_PACKET_43: {
                } break;

                case REMOTE_PACKET_44: {
                    Remote_ReceiveNetPacket_44(packet);
                } break;

                case REMOTE_PACKET_45: {
                } break;

                case REMOTE_PACKET_46: {
                } break;

                case REMOTE_PACKET_47: {
                } break;

                case REMOTE_PACKET_48: {
                } break;

                case REMOTE_PACKET_49: {
                } break;

                case REMOTE_PACKET_50: {
                } break;

                case REMOTE_PACKET_51: {
                } break;

                case REMOTE_PACKET_52: {
                } break;

                default: {
                    SDL_Log("Remote: Received unknown packet (%i).\n", packet_type);
                } break;
            }
        } else {
            break;
        }
    };
}

void Remote_sub_C9753() {
    /// \todo
}

void Remote_AnalyzeDesync() {
    /// \todo
}

int Remote_CheckUnpauseEvent() {
    Remote_UnpauseGameEvent = false;

    Remote_ProcessNetPackets();
    Remote_TimeoutTimeStamp = timer_get_stamp32();

    return Remote_UnpauseGameEvent;
}

void Remote_ProcessTick(bool mode) {
    /// \todo
}

int Remote_SiteSelectMenu() {
    /// \todo
}

void Remote_SendNetPacket_Signal(int packet_type, int team, int parameter) {
    NetPacket packet;

    packet << static_cast<unsigned char>(packet_type);
    packet << static_cast<unsigned short>(team);

    packet << parameter;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_SendNetPacket_05(unsigned short random_number, int transmit_mode) {
    NetPacket packet;

    packet << static_cast<unsigned char>(REMOTE_PACKET_05);
    packet << static_cast<unsigned short>(Remote_NetworkMenu->host_node);

    packet << Remote_NetworkMenu->player_team;
    packet << random_number;

    /// \todo Copy player address from TP layer?

    Remote_TransmitPacket(packet, transmit_mode);
}

void Remote_SendNetPacket_08(UnitInfo* unit) {}

void Remote_SendNetPacket_09(int team) {}

void Remote_SendNetPacket_10(int team, ResourceID unit_type) {}

void Remote_SendNetPacket_11(int team, Complex* complex) {}

void Remote_SendNetPacket_12(int team) {}

void Remote_SendNetPacket_13(unsigned int rng_seed) {
    NetPacket packet;

    packet << static_cast<unsigned char>(REMOTE_PACKET_13);
    packet << static_cast<unsigned short>(0);

    packet << rng_seed;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_SendNetPacket_14(int team, ResourceID unit_type, int grid_x, int grid_y) {
    NetPacket packet;

    packet << static_cast<unsigned char>(REMOTE_PACKET_14);
    packet << static_cast<unsigned short>(team);

    packet << unit_type;
    packet << grid_x;
    packet << grid_y;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_SendNetPacket_16(const char* file_name, const char* file_title) {
    NetPacket packet;

    packet << static_cast<unsigned char>(REMOTE_PACKET_16);
    packet << static_cast<unsigned short>(0);

    packet << SmartString(file_name);
    packet << SmartString(file_title);

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_SendNetPacket_17() {
    NetPacket packet;

    int world;
    int game_file_number;
    int game_file_type;
    int play_mode;
    int all_visible;
    int quick_build;
    int real_time;
    int log_file_debug;
    int disable_fire;
    int fast_movement;
    int timer;
    int endturn;
    int start_gold;
    int auto_save;
    int victory_type;
    int victory_limit;
    int raw_normal_low;
    int raw_normal_high;
    int raw_concentrate_low;
    int raw_concentrate_high;
    int raw_concentrate_seperation;
    int raw_concentrate_diffusion;
    int fuel_normal_low;
    int fuel_normal_high;
    int fuel_concentrate_low;
    int fuel_concentrate_high;
    int fuel_concentrate_seperation;
    int fuel_concentrate_diffusion;
    int gold_normal_low;
    int gold_normal_high;
    int gold_concentrate_low;
    int gold_concentrate_high;
    int gold_concentrate_seperation;
    int gold_concentrate_diffusion;
    int mixed_resource_seperation;
    int min_resources;
    int max_resources;
    int alien_seperation;
    int alien_unit_value;

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

    packet << static_cast<unsigned char>(REMOTE_PACKET_17);
    packet << static_cast<unsigned short>(0);

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

void Remote_SendNetPacket_18(int sender_team, int addresse_team, const char* message) {
    int message_length;

    message_length = strlen(message);

    if (message_length && Remote_IsNetworkGame) {
        NetPacket packet;

        packet << static_cast<unsigned char>(REMOTE_PACKET_18);
        packet << static_cast<unsigned short>(sender_team);

        packet << SmartString(message);

        Remote_TransmitPacket(packet, addresse_team);
    }
}

void Remote_SendNetPacket_20(UnitInfo* unit) {}

void Remote_SendNetPacket_28(int node) {
    NetPacket packet;

    packet << static_cast<unsigned char>(REMOTE_PACKET_28);
    packet << static_cast<unsigned short>(node);

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

void Remote_SendNetPacket_29(int node) {
    NetPacket packet;
    NetNode* host_node;

    packet << static_cast<unsigned char>(REMOTE_PACKET_29);
    packet << static_cast<unsigned short>(node);

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
    unsigned short host_node;

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

    packet << static_cast<unsigned char>(REMOTE_PACKET_30);
    packet << node->entity_id;

    packet << Remote_NetworkMenu->player_node;
    packet << Remote_NetworkMenu->team_nodes;
    packet << Remote_NetworkMenu->team_names;

    packet << Remote_Nodes.GetCount();

    for (int i = 0; i < Remote_Nodes.GetCount(); ++i) {
        packet << *Remote_Nodes[i];
    }

    Remote_WriteGameSettings(packet);

    packet.AddAddress(address);

    Remote_TransmitPacket(packet, REMOTE_UNICAST);
}

void Remote_ReceiveNetPacket_30(NetPacket& packet) {
    if (Remote_GameState == 1) {
        unsigned short player_node;
        unsigned short host_node;
        unsigned short node_count;

        packet >> player_node;
        Remote_NetworkMenu->player_node = player_node;

        packet >> host_node;
        Remote_NetworkMenu->host_node = host_node;

        packet >> Remote_NetworkMenu->team_nodes;
        packet >> Remote_NetworkMenu->team_names;

        packet >> node_count;

        for (int i = 0; i < node_count; ++i) {
            NetNode peer_node;

            packet >> peer_node;
            Remote_Nodes.Add(peer_node);
        }

        SDL_assert(Remote_Nodes.Find(player_node) && Remote_Nodes.Find(host_node));

        Remote_ReadGameSettings(packet);

        Remote_NetworkMenu->is_gui_update_needed = true;
    }
}

void Remote_SendNetPacket_31(int node) {
    NetPacket packet;

    packet << static_cast<unsigned char>(REMOTE_PACKET_31);
    packet << static_cast<unsigned short>(node);

    if (Remote_NetworkMenu->is_host_mode) {
        for (int i = 0; i < Remote_Clients.GetCount(); ++i) {
            packet.AddAddress(Remote_Clients[i]->address);
        }

        Remote_Clients.Clear();
    }

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_31(NetPacket& packet) {
    if (Remote_GameState == 1) {
        unsigned short entity_id;

        packet >> entity_id;

        Remote_NetworkMenu->LeaveGame(entity_id);
    }
}

void Remote_SendNetPacket_32(unsigned short random_number, int transmit_mode) {
    NetPacket packet;

    packet << static_cast<unsigned char>(REMOTE_PACKET_32);
    packet << static_cast<unsigned short>(Remote_NetworkMenu->host_node);

    packet << Remote_NetworkMenu->player_team;
    packet << random_number;

    /// \todo Copy player address from TP layer?

    Remote_TransmitPacket(packet, transmit_mode);
}

void Remote_SendNetPacket_33() {
    if (strlen(Remote_NetworkMenu->chat_input_buffer)) {
        NetPacket packet;

        packet << static_cast<unsigned char>(REMOTE_PACKET_33);
        packet << static_cast<unsigned short>(Remote_NetworkMenu->host_node);

        SmartString string(Remote_NetworkMenu->player_name);
        string += ": ";
        string += Remote_NetworkMenu->chat_input_buffer;

        packet << string;

        Remote_TransmitPacket(packet, REMOTE_MULTICAST);
    }
}

void Remote_ReceiveNetPacket_33(NetPacket& packet) {
    unsigned short entity_id;

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

    packet << static_cast<unsigned char>(REMOTE_PACKET_34);
    packet << static_cast<unsigned short>(Remote_NetworkMenu->host_node);

    /// \todo Copy address table from TP layer?

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_SendNetPacket_35() {
    if (Remote_Nodes.GetCount()) {
        NetPacket packet;

        packet << static_cast<unsigned char>(REMOTE_PACKET_35);
        packet << static_cast<unsigned short>(Remote_NetworkMenu->host_node);

        packet << Remote_NetworkMenu->player_node;
        packet << Remote_NetworkMenu->player_team;
        packet << (Remote_NetworkMenu->client_state == 2);
        packet << SmartString(Remote_NetworkMenu->player_name);

        Remote_TransmitPacket(packet, REMOTE_MULTICAST);
    }
}

void Remote_ReceiveNetPacket_35(NetPacket& packet) {
    unsigned short entity_id;

    packet >> entity_id;

    if (Remote_GameState == 1 && Remote_NetworkMenu->host_node == entity_id) {
        unsigned short source_node;
        char team_slot;
        bool ready_state;
        SmartString team_name;

        packet >> source_node;

        for (int i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
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

    packet << static_cast<unsigned char>(REMOTE_PACKET_36);
    packet << static_cast<unsigned short>(Remote_NetworkMenu->player_node);

    packet << Remote_NetworkMenu->player_name;

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_36(NetPacket& packet) {
    if (Remote_GameState == 1) {
        unsigned short entity_id;
        char player_name[30];
        NetNode* host_node;

        packet >> entity_id;
        packet >> player_name;

        host_node = Remote_Hosts.Find(entity_id);
        if (host_node) {
            strcpy(host_node->name, player_name);

            Remote_NetworkMenu->is_gui_update_needed = true;
        }

        for (int i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
            if (Remote_NetworkMenu->team_nodes[i] == entity_id) {
                strcpy(Remote_NetworkMenu->team_names[i], player_name);

                Remote_NetworkMenu->is_gui_update_needed = true;
            }
        }
    }
}

void Remote_SendNetPacket_37() {
    NetPacket packet;

    packet << static_cast<unsigned char>(REMOTE_PACKET_37);
    packet << static_cast<unsigned short>(Remote_NetworkMenu->player_node);

    Remote_WriteGameSettings(packet);

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_37(NetPacket& packet) {
    unsigned short entity_id;

    packet >> entity_id;

    if (Remote_GameState == 1 && Remote_NetworkMenu->host_node == entity_id) {
        Remote_ReadGameSettings(packet);

        Remote_NetworkMenu->is_gui_update_needed = true;
    }
}

void Remote_SendNetPacket_38(UnitInfo* unit) {}

void Remote_SendNetPacket_41(UnitInfo* unit) {}

void Remote_SendNetPacket_43(UnitInfo* unit, const char* name) {}

void Remote_SendNetPacket_44(NetAddress& address) {
    NetPacket packet;

    packet.AddAddress(address);

    packet << static_cast<unsigned char>(REMOTE_PACKET_44);
    packet << static_cast<unsigned short>(Remote_NetworkMenu->player_node);
    packet << SmartString(GAME_VERSION);
    packet << SmartString(Remote_NetworkMenu->player_name);

    Remote_TransmitPacket(packet, REMOTE_UNICAST);
}

void Remote_ReceiveNetPacket_44(NetPacket& packet) {
    if (Remote_GameState == 1) {
        unsigned short entity_id;

        packet >> entity_id;

        if (!Remote_Hosts.Find(entity_id)) {
            SmartString game_version;

            packet >> game_version;
            if (!strcmp(GAME_VERSION, game_version.GetCStr())) {
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

void Remote_SendNetPacket_50(UnitInfo* unit) {}
