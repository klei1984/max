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

#include <format>

#include "access.hpp"
#include "ailog.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "hash.hpp"
#include "helpmenu.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "networkmenu.hpp"
#include "randomizer.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "sound_manager.hpp"
#include "ticktimer.hpp"
#include "transport.hpp"
#include "unit.hpp"
#include "unitevents.hpp"
#include "units_manager.hpp"
#include "version.hpp"
#include "window_manager.hpp"

#define REMOTE_RESPONSE_TIMEOUT 30000
#define REMOTE_RESPONSE_PENDING 3000
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
    int16_t move_to_grid_x;
    int16_t move_to_grid_y;
    int16_t fire_on_grid_x;
    int16_t fire_on_grid_y;
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
    uint16_t hits;
    uint16_t speed;
    uint8_t shots;
    int16_t storage;
    int16_t experience;
    int16_t transfer_cargo;
    uint8_t stealth_dice_roll;
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
bool Remote_TimeoutPendingActive;
uint64_t Remote_PauseTimeStamp;
uint64_t Remote_TimeoutTimeStamp;
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

static void Remote_NotifyTimeout(bool main_menu_state);
static void Remote_ClearNotifyTimeout(bool main_menu_state);

static void Remote_WriteGameSettings(NetPacket& packet, const std::shared_ptr<Mission> mission);
static void Remote_ReadGameSettings(NetPacket& packet);

static void Remote_OrderProcessor0_Write(UnitInfo* unit, NetPacket& packet);
static void Remote_OrderProcessor0_Read(UnitInfo* unit, NetPacket& packet);
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
        new_entity_id = Randomizer_Generate(31991) + 10;

        if (!Remote_Nodes.Find(new_entity_id)) {
            break;
        }
    }

    return new_entity_id;
}

void Remote_UpdateEntityId(NetAddress& address, uint16_t entity_id) {
    for (uint32_t i = 0; i < Remote_Nodes.GetCount(); ++i) {
        if (Remote_Nodes[i]->address == address) {
            Remote_Nodes[i]->entity_id = entity_id;
        }
    }
}

void Remote_WriteGameSettings(NetPacket& packet, const std::shared_ptr<Mission> mission) {
    SDL_assert(mission);

    uint32_t mission_category = mission->GetCategory();
    const auto extension = mission->GetMission().extension();

    if ((extension == ".mlt" || extension == ".MLT") && (mission_category == MISSION_CATEGORY_MULTI_PLAYER_SCENARIO)) {
        mission_category = MISSION_CATEGORY_MULTI;
    }

    switch (mission_category) {
        case MISSION_CATEGORY_MULTI: {
            packet << mission_category;
            packet << Remote_NetworkMenu->multi_scenario_id;
        } break;

        case MISSION_CATEGORY_MULTI_PLAYER_SCENARIO: {
            const std::vector<std::string> mission_hashes = mission->GetMissionHashes();

            packet << mission_category;
            packet << mission_hashes;
        } break;

        default: {
            SDL_assert(0);
        } break;
    }

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
    packet << Remote_NetworkMenu->rng_seed;
}

void Remote_ReadGameSettings(NetPacket& packet) {
    uint32_t mission_category;

    packet >> mission_category;

    switch (mission_category) {
        case MISSION_CATEGORY_MULTI: {
            packet >> Remote_NetworkMenu->multi_scenario_id;
        } break;

        case MISSION_CATEGORY_MULTI_PLAYER_SCENARIO: {
            std::vector<std::string> mission_hashes;

            packet >> mission_hashes;

            const auto mission_index = ResourceManager_GetMissionManager()->GetMissionIndex(
                static_cast<MissionCategory>(mission_category), mission_hashes);

            if (mission_index != MissionManager::InvalidID) {
                Remote_NetworkMenu->multi_scenario_id = mission_index + 1;
            } else {
                Remote_NetworkMenu->multi_scenario_id = 0;
            }
        } break;

        default: {
            SDL_assert(0);
        } break;
    }

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
    packet >> Remote_NetworkMenu->rng_seed;
}

void Remote_OrderProcessor0_Write(UnitInfo* unit, NetPacket& packet) {}

void Remote_OrderProcessor0_Read(UnitInfo* unit, NetPacket& packet) {}

void Remote_OrderProcessor1_Write(UnitInfo* unit, NetPacket& packet) {
    packet << unit->GetOrder();
    packet << unit->GetOrderState();
    packet << unit->GetPriorOrder();
    packet << unit->GetPriorOrderState();
    packet << unit->disabled_reaction_fire;
}

void Remote_OrderProcessor1_Read(UnitInfo* unit, NetPacket& packet) {
    UnitsManager_NewOrderWhileScaling(unit);

    UnitOrderType order;
    UnitOrderStateType order_state;
    UnitOrderType prior_order;
    UnitOrderStateType prior_order_state;

    packet >> order;
    packet >> order_state;
    packet >> prior_order;
    packet >> prior_order_state;
    packet >> unit->disabled_reaction_fire;

    unit->SetOrder(order);
    unit->SetOrderState(order_state);
    unit->SetPriorOrder(prior_order);
    unit->SetPriorOrderState(prior_order_state);
}

void Remote_OrderProcessor2_Write(UnitInfo* unit, NetPacket& packet) {
    Remote_OrderProcessor1_Write(unit, packet);

    if (unit->GetParent()) {
        packet << unit->GetParent()->GetId();

    } else {
        packet << static_cast<uint16_t>(0);
    }

    packet << unit->experience;
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

    packet >> unit->experience;
}

void Remote_OrderProcessor3_Write(UnitInfo* unit, NetPacket& packet) {
    Remote_OrderProcessor2_Write(unit, packet);

    packet << unit->move_to_grid_x;
    packet << unit->move_to_grid_y;
    packet << unit->fire_on_grid_x;
    packet << unit->fire_on_grid_y;

    if (unit->GetEnemy()) {
        packet << unit->GetEnemy()->GetId();

    } else {
        packet << static_cast<uint16_t>(0);
    }

    packet << unit->transfer_cargo;
    packet << unit->stealth_dice_roll;
}

void Remote_OrderProcessor3_Read(UnitInfo* unit, NetPacket& packet) {
    Remote_OrderProcessor2_Read(unit, packet);

    packet >> unit->move_to_grid_x;
    packet >> unit->move_to_grid_y;
    packet >> unit->fire_on_grid_x;
    packet >> unit->fire_on_grid_y;
    uint16_t unit_id;

    packet >> unit_id;

    if (unit_id) {
        unit->SetEnemy(Hash_UnitHash[unit_id]);

    } else {
        unit->SetEnemy(nullptr);
    }

    packet >> unit->transfer_cargo;
    packet >> unit->stealth_dice_roll;
}

void Remote_OrderProcessor4_Write(UnitInfo* unit, NetPacket& packet) {
    SmartObjectArray<ResourceID> build_queue = unit->GetBuildList();

    Remote_OrderProcessor3_Write(unit, packet);

    packet << unit->GetRepeatBuildState();
    packet << unit->build_time;
    packet << static_cast<uint16_t>(unit->GetBuildRate());

    uint32_t unit_count = build_queue.GetCount();

    packet << unit_count;

    for (uint32_t i = 0; i < unit_count; ++i) {
        packet << *build_queue[i];
    }
}

void Remote_OrderProcessor4_Read(UnitInfo* unit, NetPacket& packet) {
    SmartObjectArray<ResourceID> build_queue = unit->GetBuildList();
    bool repeat_build;
    uint16_t build_rate;
    uint32_t unit_count;
    ResourceID unit_type;

    Remote_OrderProcessor3_Read(unit, packet);

    packet >> repeat_build;
    unit->SetRepeatBuildState(repeat_build);
    packet >> unit->build_time;
    packet >> build_rate;
    unit->SetBuildRate(build_rate);
    packet >> unit_count;

    build_queue.Clear();

    for (uint32_t i = 0; i < unit_count; ++i) {
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

    ResourceManager_GetSettings()->SetNumericValue(menu_team_player_setting[GameManager_PlayerTeam], TEAM_TYPE_PLAYER);

    UnitsManager_TeamInfo[GameManager_PlayerTeam].team_type = TEAM_TYPE_PLAYER;

    ResourceManager_GetSettings()->SetStringValue("player_name", Remote_NetworkMenu->player_name);

    int32_t player_clan = ResourceManager_GetSettings()->GetNumericValue("player_clan");

    ResourceManager_GetSettings()->SetNumericValue(menu_team_clan_setting[GameManager_PlayerTeam], player_clan);

    UnitsManager_TeamInfo[GameManager_PlayerTeam].team_clan = player_clan;

    int32_t remote_player_count = 0;

    for (int32_t team = 0; team < TRANSPORT_MAX_TEAM_COUNT; ++team) {
        if (Remote_NetworkMenu->team_nodes[team] > 4) {
            ResourceManager_GetSettings()->SetStringValue(menu_team_name_setting[team],
                                                          Remote_NetworkMenu->team_names[team]);

            if (Remote_NetworkMenu->team_nodes[team] != Remote_NetworkMenu->player_node) {
                ResourceManager_GetSettings()->SetNumericValue(menu_team_player_setting[team], TEAM_TYPE_REMOTE);
                UnitsManager_TeamInfo[team].team_type = TEAM_TYPE_REMOTE;

                ++remote_player_count;
            }

        } else {
            ResourceManager_GetSettings()->SetNumericValue(menu_team_player_setting[team], TEAM_TYPE_NONE);
            UnitsManager_TeamInfo[team].team_type = TEAM_TYPE_NONE;
        }
    }

    Remote_RemotePlayerCount = remote_player_count;

    return remote_player_count;
}

void Remote_ResponseTimeout(uint16_t team, bool mode) {
    char message[100];
    const char* menu_team_names[] = {_(f394), _(a8a6), _(a3ee), _(319d), ""};

    ResourceManager_GetSettings()->SetNumericValue(menu_team_player_setting[team], TEAM_TYPE_ELIMINATED);

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
            case ORDER_17: {
                // unused order slots
                Remote_OrderProcessors[i].WritePacket = &Remote_OrderProcessor0_Write;
                Remote_OrderProcessors[i].ReadPacket = &Remote_OrderProcessor0_Read;
            } break;

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
            AILOG(log, "Remote: Dropped malformed packet (size: {}).\n", packet.GetDataSize());
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
            for (uint32_t i = 0; i < Remote_Nodes.GetCount(); ++i) {
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
        if ((*it).GetUnitType() != MININGST) {
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
    int32_t transort_type;
    std::string transport_setting = ResourceManager_GetSettings()->GetStringValue("transport");

    Remote_IsHostMode = is_host_mode;

    ResourceManager_InitTeamInfo();
    Remote_Init();

    if (!transport_setting.empty()) {
        if (!strcmp(transport_setting.c_str(), TRANSPORT_DEFAULT_TYPE)) {
            transort_type = TRANSPORT_DEFAULT_UDP;

        } else {
            SmartString error_message;

            ResourceManager_GetSettings()->SetStringValue("transport", TRANSPORT_DEFAULT_TYPE);

            error_message.Sprintf(
                100, "Unknown network transport layer type (%s), using default (" TRANSPORT_DEFAULT_TYPE ").\n",
                transport_setting.c_str());
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
                    AILOG(log, "Received unknown packet type ({}).", packet_type - TRANSPORT_APPL_PACKET_ID);
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

    AILOG(log, "{}", message);

    MessageManager_DrawMessage(message, 2, 1, false, true);
}

void Remote_NetErrorUnitInfoOutOfSync(UnitInfo* unit, NetPacket& packet) {
    struct Packet23Data data;
    const char* const team_names[PLAYER_TEAM_MAX + 1] = {"Red", "Green", "Blue", "Gray", "Neutral", "Unknown"};

    Remote_ProcessNetPacket_23(data, packet);

    AILOG(log, "Units are out of sync:  Host, Peer");

    AILOG_LOG(log, " team          {}, {}",
              (unit->team < PLAYER_TEAM_MAX) ? team_names[unit->team] : team_names[PLAYER_TEAM_MAX],
              (data.team < PLAYER_TEAM_MAX) ? team_names[data.team] : team_names[PLAYER_TEAM_MAX]);

    AILOG_LOG(
        log, " type          {}, {}",
        (unit->GetUnitType() < UNIT_END) ? ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data() : "?",
        (data.unit_type < UNIT_END) ? ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data() : "?");

    AILOG_LOG(log, " unit id       {}, {}", unit->unit_id, data.unit_id);
    AILOG_LOG(log, " parent id     {}, {}", unit->GetParent() ? unit->GetParent()->GetId() : 0, data.parent_unit_id);
    AILOG_LOG(log, " enemy id      {}, {}", unit->GetEnemy() ? unit->GetEnemy()->GetId() : 0, data.enemy_unit_id);
    AILOG_LOG(log, " orders        {}, {}", static_cast<int>(unit->GetOrder()), static_cast<int>(data.orders));
    AILOG_LOG(log, " order state   {}, {}", static_cast<int>(unit->GetOrderState()), static_cast<int>(data.state));
    AILOG_LOG(log, " prior orders  {}, {}", static_cast<int>(unit->GetPriorOrder()),
              static_cast<int>(data.prior_orders));
    AILOG_LOG(log, " prior state   {}, {}", static_cast<int>(unit->GetPriorOrderState()),
              static_cast<int>(data.prior_state));
    AILOG_LOG(log, " grid x        {}, {}", unit->grid_x, data.grid_x);
    AILOG_LOG(log, " grid y        {}, {}", unit->grid_y, data.grid_y);

    if (unit->GetOrder() == ORDER_BUILD || data.orders == ORDER_BUILD) {
        AILOG_LOG(log, " build turns   {}, {}", unit->build_time, data.build_time);
        AILOG_LOG(log, " build rate    {}, {}", unit->build_rate, data.build_rate);
    }

    AILOG_LOG(log, " move to grid x {}, {}", unit->move_to_grid_x, data.move_to_grid_x);
    AILOG_LOG(log, " move to grid y {}, {}", unit->move_to_grid_y, data.move_to_grid_y);
    AILOG_LOG(log, " fire on grid x {}, {}", unit->fire_on_grid_x, data.fire_on_grid_x);
    AILOG_LOG(log, " fire on grid y {}, {}", unit->fire_on_grid_y, data.fire_on_grid_y);
    AILOG_LOG(log, " reaction fire {}, {}", unit->disabled_reaction_fire, data.disabled_reaction_fire);
    AILOG_LOG(log, " total mining  {}, {}", unit->total_mining, data.total_mining);
    AILOG_LOG(log, " raw mining    {}, {}", unit->raw_mining, data.raw_mining);
    AILOG_LOG(log, " fuel mining   {}, {}", unit->fuel_mining, data.fuel_mining);
    AILOG_LOG(log, " gold mining   {}, {}", unit->gold_mining, data.gold_mining);
    AILOG_LOG(log, " hits          {}, {}", unit->hits, data.hits);
    AILOG_LOG(log, " speed         {}, {}", unit->speed, data.speed);
    AILOG_LOG(log, " rounds        {}, {}", unit->shots, data.shots);
    AILOG_LOG(log, " storage       {}, {}", unit->storage, data.storage);
    AILOG_LOG(log, " experience    {}, {}", unit->experience, data.experience);
    AILOG_LOG(log, " x-fer cargo   {}, {}", unit->transfer_cargo, data.transfer_cargo);
    AILOG_LOG(log, " stealth dice  {}, {}", unit->stealth_dice_roll, data.stealth_dice_roll);
    AILOG_LOG(log, " ammo          {}, {}", unit->ammo, data.ammo);

    std::string message = std::format("Unit, id {}, is in different state in remote packet.", unit->GetId());

    MessageManager_DrawMessage(message.c_str(), 2, 1, false, true);
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

    if (ResourceManager_GetSettings()->GetNumericValue("timer")) {
        Remote_UpdatePauseTimer = true;
    }
}

int32_t Remote_CheckUnpauseEvent() {
    Remote_UnpauseGameEvent = false;

    Remote_ProcessNetPackets();
    Remote_TimeoutTimeStamp = timer_get();

    return Remote_UnpauseGameEvent;
}

void Remote_NotifyTimeout(bool main_menu_state) {
    if (timer_elapsed_time(Remote_TimeoutTimeStamp) > REMOTE_RESPONSE_PENDING) {
        if (!Remote_TimeoutPendingActive) {
            const auto window = WindowManager_GetWindow(WINDOW_MESSAGE_BOX);
            Remote_TimeoutPendingActive = true;

            if (main_menu_state) {
                GameManager_DisableMainMenu();
            }

            Cursor_SetCursor(CURSOR_UNIT_NO_GO);
            MessageManager_ClearMessageBox();
            win_draw_rect(window->id, &window->window);
            MessageManager_DrawMessage(_(17ee), 2, 0);
            MessageManager_DrawMessageBox();
            win_draw_rect(window->id, &window->window);
        }

        GNW_process_message();
    }
}

void Remote_ClearNotifyTimeout(bool main_menu_state) {
    if (Remote_TimeoutPendingActive) {
        Remote_TimeoutPendingActive = false;
        MessageManager_ClearMessageBox();
        const auto window = WindowManager_GetWindow(WINDOW_MESSAGE_BOX);
        win_draw_rect(window->id, &window->window);
    }

    if (main_menu_state) {
        GameManager_EnableMainMenu(nullptr);
    }
}

void Remote_Synchronize(bool async_mode) {
    Remote_ProcessNetPackets();

    if (GameManager_GameState == GAME_STATE_3_MAIN_MENU || GameManager_GameState == GAME_STATE_7_SITE_SELECT ||
        GameManager_GameState == GAME_STATE_10_LOAD_GAME || GameManager_GameState == GAME_STATE_12_DEPLOYING_UNITS ||
        GameManager_GameState == GAME_STATE_13_SITE_SELECTED) {
        Remote_TimeoutTimeStamp = timer_get();
        Remote_SendSynchFrame = true;

    } else {
        uint64_t time_stamp = timer_get();
        bool stay_in_loop = true;
        bool main_menu_state = GameManager_IsMainMenuEnabled;

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
                            Remote_TimeoutPendingActive = false;
                            Remote_ResponseTimeout(team, true);
                            break;

                        } else {
                            stay_in_loop = true;
                        }

                        Remote_NotifyTimeout(main_menu_state);
                    }
                }
            }

            if (stay_in_loop && async_mode) {
                Remote_TimeoutTimeStamp = timer_get();

                Remote_ClearNotifyTimeout(main_menu_state);

                return;
            }
        }

        Remote_FrameSyncCounter2 = (Remote_FrameSyncCounter2 + 1) & 0x3F;
        Remote_TimeoutTimeStamp = timer_get();
        Remote_SendSynchFrame = true;

        Remote_ClearNotifyTimeout(main_menu_state);
    }
}

void Remote_WaitBeginTurnAcknowledge() {
    Cursor_SetCursor(CURSOR_UNIT_NO_GO);

    Remote_P51_Signal = false;

    ++Remote_FrameSyncCounter[GameManager_PlayerTeam];

    Remote_SendNetPacket_Signal(REMOTE_PACKET_00, GameManager_PlayerTeam,
                                Remote_FrameSyncCounter[GameManager_PlayerTeam]);

    uint64_t time_stamp_timeout = timer_get();
    uint64_t time_stamp_ping = timer_get();

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

    uint64_t time_stamp_timeout = timer_get();
    uint64_t time_stamp_ping = timer_get();

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
                    GameManager_GameState = GAME_STATE_14_EXIT_SITE_SELECT;

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
                    HelpMenu_Menu("SITE_SELECT_SETUP", WINDOW_MAIN_MAP);
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

    Randomizer_SetSeed(Remote_RngSeed);

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

    uint64_t time_stamp = timer_get();
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

    uint64_t time_stamp_timeout = timer_get();
    uint64_t time_stamp_ping = timer_get();

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
    const char* menu_team_names[] = {_(f394), _(a8a6), _(a3ee), _(319d), ""};

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

    Remote_OrderProcessors[unit->GetOrder()].WritePacket(unit, packet);

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

        Remote_OrderProcessors[unit->GetOrder()].ReadPacket(unit, packet);

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
    packet << UnitsManager_TeamInfo[team].research_topics;

    const auto team_name = ResourceManager_GetSettings()->GetStringValue(menu_team_name_setting[team]);

    if (team_name.empty()) {
        SDL_assert(0);
    }

    packet << SmartString(team_name.c_str());

    Remote_TransmitPacket(packet, REMOTE_MULTICAST);
}

void Remote_ReceiveNetPacket_09(NetPacket& packet) {
    uint16_t entity_id;
    uint32_t gold;
    SmartString team_name;

    packet >> entity_id;
    packet >> gold;

    UnitsManager_TeamInfo[entity_id].team_units->SetGold(gold);

    packet >> UnitsManager_TeamInfo[entity_id].stats_gold_spent_on_upgrades;
    packet >> UnitsManager_TeamInfo[entity_id].research_topics;
    packet >> team_name;

    ResourceManager_GetSettings()->SetStringValue(menu_team_name_setting[entity_id], team_name.GetCStr());
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
        if ((*it).GetComplex() == complex && (*it).GetUnitType() == MININGST && (*it).GetOrder() != ORDER_POWER_OFF &&
            (*it).GetOrder() != ORDER_DISABLE && (*it).GetOrder() != ORDER_IDLE) {
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
    uint32_t unit_count = supplies->units.GetCount();

    packet << static_cast<uint8_t>(REMOTE_PACKET_12);
    packet << static_cast<uint16_t>(team);

    packet << supplies->start_gold;
    packet << supplies->team_gold;
    packet << unit_count;
    packet << UnitsManager_TeamInfo[team].stats_gold_spent_on_upgrades;
    packet << supplies->starting_position;
    packet << supplies->proximity_alert_ack;

    for (uint32_t i = 0; i < unit_count; ++i) {
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

    uint32_t unit_count;

    packet >> supplies->start_gold;
    packet >> supplies->team_gold;
    packet >> unit_count;
    packet >> UnitsManager_TeamInfo[entity_id].stats_gold_spent_on_upgrades;
    packet >> supplies->starting_position;
    packet >> supplies->proximity_alert_ack;

    for (uint32_t i = 0; i < unit_count; ++i) {
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

    bool state_backup = GameManager_QuickBuildMenuActive;

    GameManager_QuickBuildMenuActive = true;

    GameManager_DeployUnit(entity_id, unit_type, grid_x, grid_y);

    GameManager_QuickBuildMenuActive = state_backup;
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
    const auto settings = ResourceManager_GetSettings();
    NetPacket packet;

    int32_t world;
    int32_t game_file_number;
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

    world = settings->GetNumericValue("world");
    game_file_number = settings->GetNumericValue("game_file_number");
    play_mode = settings->GetNumericValue("play_mode");
    all_visible = settings->GetNumericValue("all_visible");
    quick_build = settings->GetNumericValue("quick_build");
    real_time = settings->GetNumericValue("real_time");
    log_file_debug = settings->GetNumericValue("log_file_debug");
    disable_fire = settings->GetNumericValue("disable_fire");
    fast_movement = settings->GetNumericValue("fast_movement");
    timer = settings->GetNumericValue("timer");
    endturn = settings->GetNumericValue("endturn");
    start_gold = settings->GetNumericValue("start_gold");
    auto_save = settings->GetNumericValue("auto_save");
    victory_type = settings->GetNumericValue("victory_type");
    victory_limit = settings->GetNumericValue("victory_limit");
    raw_normal_low = settings->GetNumericValue("raw_normal_low");
    raw_normal_high = settings->GetNumericValue("raw_normal_high");
    raw_concentrate_low = settings->GetNumericValue("raw_concentrate_low");
    raw_concentrate_high = settings->GetNumericValue("raw_concentrate_high");
    raw_concentrate_seperation = settings->GetNumericValue("raw_concentrate_seperation");
    raw_concentrate_diffusion = settings->GetNumericValue("raw_concentrate_diffusion");
    fuel_normal_low = settings->GetNumericValue("fuel_normal_low");
    fuel_normal_high = settings->GetNumericValue("fuel_normal_high");
    fuel_concentrate_low = settings->GetNumericValue("fuel_concentrate_low");
    fuel_concentrate_high = settings->GetNumericValue("fuel_concentrate_high");
    fuel_concentrate_seperation = settings->GetNumericValue("fuel_concentrate_seperation");
    fuel_concentrate_diffusion = settings->GetNumericValue("fuel_concentrate_diffusion");
    gold_normal_low = settings->GetNumericValue("gold_normal_low");
    gold_normal_high = settings->GetNumericValue("gold_normal_high");
    gold_concentrate_low = settings->GetNumericValue("gold_concentrate_low");
    gold_concentrate_high = settings->GetNumericValue("gold_concentrate_high");
    gold_concentrate_seperation = settings->GetNumericValue("gold_concentrate_seperation");
    gold_concentrate_diffusion = settings->GetNumericValue("gold_concentrate_diffusion");
    mixed_resource_seperation = settings->GetNumericValue("mixed_resource_seperation");
    min_resources = settings->GetNumericValue("min_resources");
    max_resources = settings->GetNumericValue("max_resources");
    alien_seperation = settings->GetNumericValue("alien_seperation");
    alien_unit_value = settings->GetNumericValue("alien_unit_value");

    packet << static_cast<uint8_t>(REMOTE_PACKET_17);
    packet << static_cast<uint16_t>(0);

    packet << GameManager_GameState;
    packet << world;
    packet << game_file_number;
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
    const auto settings = ResourceManager_GetSettings();
    uint16_t entity_id;

    packet >> entity_id;

    char game_state;
    int32_t world;
    int32_t game_file_number;
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

    backup_timer = settings->GetNumericValue("timer");
    backup_endturn = settings->GetNumericValue("endturn");

    packet >> game_state;
    packet >> world;
    packet >> game_file_number;
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

    settings->SetNumericValue("play_mode", play_mode);
    settings->SetNumericValue("all_visible", all_visible);
    settings->SetNumericValue("quick_build", quick_build);
    settings->SetNumericValue("real_time", real_time);
    settings->SetNumericValue("log_file_debug", log_file_debug);
    settings->SetNumericValue("disable_fire", disable_fire);
    settings->SetNumericValue("fast_movement", fast_movement);
    settings->SetNumericValue("timer", timer);
    settings->SetNumericValue("endturn", endturn);
    settings->SetNumericValue("start_gold", start_gold);
    settings->SetNumericValue("auto_save", auto_save);
    settings->SetNumericValue("raw_normal_low", raw_normal_low);
    settings->SetNumericValue("raw_normal_high", raw_normal_high);
    settings->SetNumericValue("raw_concentrate_low", raw_concentrate_low);
    settings->SetNumericValue("raw_concentrate_high", raw_concentrate_high);
    settings->SetNumericValue("raw_concentrate_seperation", raw_concentrate_seperation);
    settings->SetNumericValue("raw_concentrate_diffusion", raw_concentrate_diffusion);
    settings->SetNumericValue("fuel_normal_low", fuel_normal_low);
    settings->SetNumericValue("fuel_normal_high", fuel_normal_high);
    settings->SetNumericValue("fuel_concentrate_low", fuel_concentrate_low);
    settings->SetNumericValue("fuel_concentrate_high", fuel_concentrate_high);
    settings->SetNumericValue("fuel_concentrate_seperation", fuel_concentrate_seperation);
    settings->SetNumericValue("fuel_concentrate_diffusion", fuel_concentrate_diffusion);
    settings->SetNumericValue("gold_normal_low", gold_normal_low);
    settings->SetNumericValue("gold_normal_high", gold_normal_high);
    settings->SetNumericValue("gold_concentrate_low", gold_concentrate_low);
    settings->SetNumericValue("gold_concentrate_high", gold_concentrate_high);
    settings->SetNumericValue("gold_concentrate_seperation", gold_concentrate_seperation);
    settings->SetNumericValue("gold_concentrate_diffusion", gold_concentrate_diffusion);
    settings->SetNumericValue("mixed_resource_seperation", mixed_resource_seperation);
    settings->SetNumericValue("min_resources", min_resources);
    settings->SetNumericValue("max_resources", max_resources);
    settings->SetNumericValue("alien_seperation", alien_seperation);
    settings->SetNumericValue("alien_unit_value", alien_unit_value);

    if (GameManager_GameState == GAME_STATE_3_MAIN_MENU) {
        GameManager_GameState = game_state;

        settings->SetNumericValue("world", world);
        settings->SetNumericValue("game_file_number", game_file_number);

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

    const auto team_name = ResourceManager_GetSettings()->GetStringValue(menu_team_name_setting[entity_id]);

    SmartString message_text;
    SmartString message(team_name.c_str());

    packet >> message_text;

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

void Remote_CreateNetPacket_23(UnitInfo* unit, NetPacket& packet) {
    Remote_OrderProcessor5_Write(unit, packet);

    const auto unit_type{unit->GetUnitType()};

    packet << unit->build_time;
    packet << unit->build_rate;
    packet << unit_type;
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

    local >> data.move_to_grid_x;
    local >> data.move_to_grid_y;
    local >> data.fire_on_grid_x;
    local >> data.fire_on_grid_y;
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
    local >> data.experience;
    local >> data.transfer_cargo;
    local >> data.stealth_dice_roll;
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

    ResourceManager_GetSettings()->SetNumericValue(menu_team_clan_setting[entity_id], team_clan);

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
        AILOG(log, "Remote: Attempted to register to host that does not exist.\n");
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
    const auto mission = ResourceManager_GetMissionManager()->GetMission();

    if (mission) {
        NetPacket packet;
        NetNode* node;

        node = Remote_Nodes.Find(address);

        packet << static_cast<uint8_t>(REMOTE_PACKET_30);
        packet << node->entity_id;

        packet << Remote_NetworkMenu->player_node;
        packet << Remote_NetworkMenu->team_nodes;
        packet << Remote_NetworkMenu->team_names;

        packet << Remote_Nodes.GetCount();

        for (uint32_t i = 0; i < Remote_Nodes.GetCount(); ++i) {
            packet << *Remote_Nodes[i];
        }

        Remote_WriteGameSettings(packet, mission);

        packet.AddAddress(address);

        Remote_TransmitPacket(packet, REMOTE_UNICAST);

    } else {
        SDL_assert(0);
    }
}

void Remote_ReceiveNetPacket_30(NetPacket& packet) {
    if (Remote_GameState == 1) {
        uint16_t player_node;
        uint16_t host_node;
        uint32_t node_count;

        packet >> player_node;
        Remote_NetworkMenu->player_node = player_node;

        packet >> host_node;
        Remote_NetworkMenu->host_node = host_node;

        packet >> Remote_NetworkMenu->team_nodes;
        packet >> Remote_NetworkMenu->team_names;

        packet >> node_count;

        for (uint32_t i = 0; i < node_count; ++i) {
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
        for (uint32_t i = 0; i < Remote_Clients.GetCount(); ++i) {
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

        const std::string string = Remote_NetworkMenu->player_name + ": " + Remote_NetworkMenu->chat_input_buffer;

        packet << string;

        Remote_TransmitPacket(packet, REMOTE_MULTICAST);
    }
}

void Remote_ReceiveNetPacket_33(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    if (Remote_GameState == 1 && Remote_NetworkMenu->host_node == entity_id) {
        std::string string;

        packet >> string;

        SDL_utf8strlcpy(Remote_NetworkMenu->chat_message_buffer, string.c_str(),
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
        const std::string team_name = Remote_NetworkMenu->player_name;

        packet << static_cast<uint8_t>(REMOTE_PACKET_35);
        packet << static_cast<uint16_t>(Remote_NetworkMenu->host_node);

        packet << Remote_NetworkMenu->player_node;
        packet << Remote_NetworkMenu->player_team;
        packet << (Remote_NetworkMenu->client_state == 2);
        packet << team_name;

        Remote_TransmitPacket(packet, REMOTE_MULTICAST);
    }
}

void Remote_ReceiveNetPacket_35(NetPacket& packet) {
    uint16_t entity_id;

    packet >> entity_id;

    if (Remote_GameState == 1 && Remote_NetworkMenu->host_node == entity_id) {
        uint16_t source_node;
        uint8_t team_slot;
        bool ready_state;
        std::string team_name;

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

        strcpy(Remote_NetworkMenu->team_names[team_slot], team_name.c_str());
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
        std::string player_name;
        NetNode* host_node;

        packet >> entity_id;
        packet >> player_name;

        host_node = Remote_Hosts.Find(entity_id);
        if (host_node) {
            SDL_strlcpy(host_node->name, player_name.c_str(), sizeof(host_node->name));

            Remote_NetworkMenu->is_gui_update_needed = true;
        }

        for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
            if (Remote_NetworkMenu->team_nodes[i] == entity_id) {
                SDL_strlcpy(Remote_NetworkMenu->team_names[i], player_name.c_str(),
                            sizeof(Remote_NetworkMenu->team_names[i]));

                Remote_NetworkMenu->is_gui_update_needed = true;
            }
        }
    }
}

void Remote_SendNetPacket_37() {
    const auto mission = ResourceManager_GetMissionManager()->GetMission();

    if (mission) {
        NetPacket packet;

        packet << static_cast<uint8_t>(REMOTE_PACKET_37);
        packet << static_cast<uint16_t>(Remote_NetworkMenu->player_node);

        Remote_WriteGameSettings(packet, mission);

        Remote_TransmitPacket(packet, REMOTE_MULTICAST);

    } else {
        SDL_assert(0);
    }
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

    packet << unit->GetOrder();
    packet << unit->GetOrderState();
    packet << unit->move_to_grid_x;
    packet << unit->move_to_grid_y;
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

        UnitOrderType order;
        UnitOrderStateType order_state;

        packet >> order;
        packet >> order_state;

        unit->SetOrder(order);
        unit->SetOrderState(order_state);

        packet >> unit->move_to_grid_x;
        packet >> unit->move_to_grid_y;
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
                unit->path = new (std::nothrow) GroundPath(unit->move_to_grid_x, unit->move_to_grid_y);

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
    const std::string host_name = Remote_NetworkMenu->player_name;

    packet.AddAddress(address);

    packet << static_cast<uint8_t>(REMOTE_PACKET_44);
    packet << static_cast<uint16_t>(Remote_NetworkMenu->player_node);
    packet << static_cast<uint32_t>(GAME_VERSION);
    packet << host_name;

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
                std::string host_name;

                packet >> host_name;
                host_node.address = packet.GetAddress(REMOTE_RECEIVED_ADDRESS);
                host_node.entity_id = entity_id;
                SDL_strlcpy(host_node.name, host_name.c_str(), sizeof(host_node.name));
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
