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

#include "transport_udp_default.hpp"

#include <SDL.h>
#include <SDL_thread.h>
#include <enet/enet.h>

#if defined(MAX_ENABLE_UPNP)
#include <miniupnpc.h>
#include <upnpcommands.h>
#endif

#include <utility>

#include "inifile.hpp"
#include "netlog.hpp"
#include "version.hpp"

enum : uint8_t {
    TRANSPORT_PACKET_00 = TRANSPORT_TP_PACKET_ID,
    TRANSPORT_PACKET_01,
    TRANSPORT_PACKET_02,
};

enum {
    TRANSPORT_NETSTATE_DEINITED,
    TRANSPORT_NETSTATE_INITED,
    TRANSPORT_NETSTATE_CONNECTED,
    TRANSPORT_NETSTATE_DISCONNECTED,
};

enum {
    TRANSPORT_TP_CHANNEL,
    TRANSPORT_APPL_CHANNEL,
    TRANSPORT_CHANNEL_COUNT,
};

static constexpr uint16_t TransportUdpDefault_ProtocolVersionId = 0x0002;
static constexpr uint16_t TransportUdpDefault_DefaultHostPort = 31554;
static constexpr uint32_t TransportUdpDefault_ServiceTickPeriod = 10;
static constexpr uint32_t TransportUdpDefault_DisconnectResponseTimeout = 3000;
static constexpr uint32_t TransportUdpDefault_MaximumPeers = 32;
static constexpr uint32_t TransportUdpDefault_Channels = TRANSPORT_CHANNEL_COUNT;

#if defined(MAX_ENABLE_UPNP)
static constexpr uint32_t TransportUdpDefault_UpnpDeviceResponseTimeout = 3000;

static_assert(MINIUPNPC_API_VERSION == 20, "API changes of MINIUPNP library shall be reviewed.");
#endif

static_assert(sizeof(NetAddress::host) == sizeof(ENetAddress::host));
static_assert(sizeof(NetAddress::port) == sizeof(ENetAddress::port));

enum {
    TRANSPORT_IGDSTATUS_NOIGD,
    TRANSPORT_IGDSTATUS_DISCONNECTED,
    TRANSPORT_IGDSTATUS_ERROR,
    TRANSPORT_IGDSTATUS_OK,
};

struct UpnpDevice {
    SmartString ControlUrl;
    SmartString ServiceType;
    SmartString HostAddress;
    SmartString ExternalAddress;
    int8_t Status;
};

struct TransportUdpDefault_Context {
    SDL_Thread* Thread;
    ENetHost* Host;
    SDL_SpinLock QueueLock;
    SmartObjectArray<ENetPeer*> Peers;
    SmartObjectArray<ENetPeer*> RemotePeers;
    SmartObjectArray<NetPacket*> TxPackets;
    SmartObjectArray<NetPacket*> RxPackets;
    struct UpnpDevice UpnpDevice;
    ENetAddress ServerAddress;
    bool ExitThread;
    int32_t NetState;
    int32_t NetRole;
    const char* LastError;
};

static int TransportUdpDefault_ClientFunction(void* data) noexcept;
static int TransportUdpDefault_ServerFunction(void* data) noexcept;
static inline void TransportUdpDefault_GetServerAddress(ENetAddress& address);
static inline bool TransportUdpDefault_SendTpPacket(struct TransportUdpDefault_Context* const context,
                                                    ENetPeer* const peer, const NetPacket& packet);
static inline bool TransportUdpDefault_SendVersionInfo(struct TransportUdpDefault_Context* const context,
                                                       ENetPeer* const peer);
static inline bool TransportUdpDefault_VersionCheck(struct TransportUdpDefault_Context* const context,
                                                    NetPacket& packet);
static inline bool TransportUdpDefault_SendPeersListToPeer(struct TransportUdpDefault_Context* const context,
                                                           ENetPeer* const peer);
static inline bool TransportUdpDefault_BroadcastNewPeerArrived(struct TransportUdpDefault_Context* const context,
                                                               ENetPeer* const peer);
static inline void TransportUdpDefault_RemovePeer(struct TransportUdpDefault_Context* const context,
                                                  ENetPeer* const peer);
static inline void TransportUdpDefault_RemoveClients(struct TransportUdpDefault_Context* const context);
static inline void TransportUdpDefault_ConnectRemotePeer(struct TransportUdpDefault_Context* const context,
                                                         NetPacket& packet);
static inline void TransportUdpDefault_RemoveRemotePeer(struct TransportUdpDefault_Context* const context,
                                                        ENetPeer* const peer);
static inline void TransportUdpDefault_ProtocolErrorMessage(ENetPeer* const peer, uint8_t packet_type);
static inline void TransportUdpDefault_ProcessTpPacket(struct TransportUdpDefault_Context* const context,
                                                       ENetPeer* const peer, ENetPacket* const enet_packet);
static inline void TransportUdpDefault_ProcessApplPacket(struct TransportUdpDefault_Context* const context,
                                                         ENetPeer* const peer, ENetPacket* const enet_packet);
static inline void TransportUdpDefault_TransmitApplPackets(struct TransportUdpDefault_Context* const context);

#if defined(MAX_ENABLE_UPNP)
static void TransportUdpDefault_UpnpInit(struct TransportUdpDefault_Context* const context) noexcept;
static void TransportUdpDefault_UpnpDeinit(struct TransportUdpDefault_Context* const context) noexcept;
static bool TransportUdpDefault_UpnpAddPortMapping(struct UpnpDevice& device, ENetAddress& host_address) noexcept;
static bool TransportUdpDefault_UpnpRemovePortMapping(struct UpnpDevice& device, ENetAddress& host_address) noexcept;
#endif

TransportUdpDefault::~TransportUdpDefault() {
    Deinit();

    delete context;
    context = nullptr;
}

void TransportUdpDefault_GetServerAddress(ENetAddress& address) {
    char server_address[30];

    if (!ini_config.GetStringValue(INI_NETWORK_HOST_ADDRESS, server_address, sizeof(server_address)) ||
        enet_address_set_host_ip(&address, server_address) != 0) {
        (void)enet_address_set_host_ip(&address, "127.0.0.1");
    }

    int32_t server_port = ini_get_setting(INI_NETWORK_HOST_PORT);

    if (!server_port || server_port < 1024 || server_port > 65535) {
        server_port = TransportUdpDefault_DefaultHostPort;
    }

    address.port = server_port;
}

bool TransportUdpDefault::Init(int32_t mode) {
    bool result{false};

    if (context == nullptr) {
        context = new (std::nothrow) TransportUdpDefault_Context;

        context->Thread = nullptr;
        context->Host = nullptr;
        context->QueueLock = 0;
        context->Peers.Clear();
        context->RemotePeers.Clear();
        context->TxPackets.Clear();
        context->RxPackets.Clear();
        context->UpnpDevice.Status = TRANSPORT_IGDSTATUS_ERROR;
        context->ServerAddress.host = ENET_HOST_ANY;
        context->ServerAddress.port = TransportUdpDefault_DefaultHostPort;
        context->ExitThread = false;
        context->NetState = TRANSPORT_NETSTATE_DEINITED;
        context->NetRole = -1;
        context->LastError = "No error.";

        if (ini_get_setting(INI_LOG_FILE_DEBUG)) {
            NetLog_Enable();
        }
    }

    if (context->NetState == TRANSPORT_NETSTATE_DEINITED) {
        if (enet_initialize() == 0) {
            TransportUdpDefault_GetServerAddress(context->ServerAddress);

            // safe write access as thread cannot exist yet
            context->NetState = TRANSPORT_NETSTATE_INITED;

            SDL_assert(context->Thread == nullptr);

            if (mode == TRANSPORT_SERVER) {
                context->Thread = SDL_CreateThread(&TransportUdpDefault_ServerFunction, "TransportUdpDefault", context);

            } else {
                context->Thread = SDL_CreateThread(&TransportUdpDefault_ClientFunction, "TransportUdpDefault", context);
            }

            if (context->Thread) {
                result = true;

            } else {
                Deinit();
                SetError("ENET worker thread initialization error.");
            }

        } else {
            SetError("ENET initialization error.");
        }

    } else {
        result = true;
    }

    return result;
}

bool TransportUdpDefault::Deinit() {
    if (context) {
        if (context->Thread) {
            int result;

            context->ExitThread = true;
            SDL_WaitThread(context->Thread, &result);
            context->Thread = nullptr;

            /// \todo Handle error.
        }

        if (context->NetState != TRANSPORT_NETSTATE_DEINITED) {
            enet_deinitialize();
            // safe write access as thread cannot exist anymore
            context->NetState = TRANSPORT_NETSTATE_DEINITED;
        }
    }

    return true;
}

bool TransportUdpDefault::Connect() { return false; }

bool TransportUdpDefault::Disconnect() { return false; }

void TransportUdpDefault::SetError(const char* error) {
    if (context) {
        context->LastError = error;
    }
}

const char* TransportUdpDefault::GetError() const {
    const char* error{""};

    if (context) {
        error = context->LastError;
    }

    return error;
}

bool TransportUdpDefault::TransmitPacket(NetPacket& packet) {
    SDL_AtomicLock(&context->QueueLock);
    {
        NetPacket* local = new (std::nothrow) NetPacket(std::move(packet));

        context->TxPackets.PushBack(&local);

        SDL_AtomicUnlock(&context->QueueLock);

        NetLog log("Transmit");
        log.Log(*local);
    }

    return true;
}

bool TransportUdpDefault::ReceivePacket(NetPacket& packet) {
    bool result{false};

    packet.Reset();

    SDL_AtomicLock(&context->QueueLock);
    {
        if (context->RxPackets.GetCount() > 0) {
            NetPacket* local = *context->RxPackets[0];

            packet = std::move(*local);
            delete local;
            context->RxPackets.Remove(0);

            result = true;
        }

        SDL_AtomicUnlock(&context->QueueLock);
    }

    if (result) {
        NetLog log("Receive from %4X", packet.GetAddress(0).port);
        log.Log(packet);
    }

    return result;
}

bool TransportUdpDefault_SendTpPacket(struct TransportUdpDefault_Context* const context, ENetPeer* const peer,
                                      const NetPacket& packet) {
    bool result{false};

    ENetPacket* enet_packet = enet_packet_create(packet.GetBuffer(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    if (enet_packet) {
        if (peer) {
            if (enet_peer_send(peer, TRANSPORT_TP_CHANNEL, enet_packet) == 0) {
                result = true;
            }

        } else {
            enet_host_broadcast(context->Host, TRANSPORT_TP_CHANNEL, enet_packet);

            result = true;
        }
    }

    return result;
}

bool TransportUdpDefault_SendVersionInfo(struct TransportUdpDefault_Context* const context, ENetPeer* const peer) {
    NetPacket packet;
    uint32_t enet_version{static_cast<uint32_t>(enet_linked_version())};
    uint16_t protocol_version{TransportUdpDefault_ProtocolVersionId};
    uint32_t game_version{GAME_VERSION};

    packet << static_cast<uint8_t>(TRANSPORT_PACKET_00);
    packet << game_version;
    packet << enet_version;
    packet << protocol_version;

    return TransportUdpDefault_SendTpPacket(context, peer, packet);
}

bool TransportUdpDefault_VersionCheck(struct TransportUdpDefault_Context* const context, NetPacket& packet) {
    uint32_t enet_version{static_cast<uint32_t>(enet_linked_version())};
    uint32_t remote_enet_version{0};
    uint32_t game_version{GAME_VERSION};
    uint32_t remote_game_version{0};
    uint16_t protocol_version{TransportUdpDefault_ProtocolVersionId};
    uint16_t remote_protocol_version{0};

    packet >> remote_game_version;
    packet >> remote_enet_version;
    packet >> remote_protocol_version;

    return game_version == remote_game_version && enet_version == remote_enet_version &&
           protocol_version == remote_protocol_version;
}

bool TransportUdpDefault_SendPeersListToPeer(struct TransportUdpDefault_Context* const context, ENetPeer* const peer) {
    NetPacket packet;
    const uint16_t peer_count = context->Peers.GetCount();
    bool result{true};

    if (peer_count) {
        packet << static_cast<uint8_t>(TRANSPORT_PACKET_01);
        packet << peer_count;

        for (auto i = 0; i < peer_count; ++i) {
            packet << (*context->Peers[i])->address;
        }

        result = TransportUdpDefault_SendTpPacket(context, peer, packet);
    }

    return result;
}

bool TransportUdpDefault_BroadcastNewPeerArrived(struct TransportUdpDefault_Context* const context,
                                                 ENetPeer* const peer) {
    NetPacket packet;
    const uint16_t peer_count = context->Peers.GetCount();
    bool result{true};

    packet << static_cast<uint8_t>(TRANSPORT_PACKET_02);
    packet << peer->address;

    for (auto i = 0; i < peer_count; ++i) {
        if (!TransportUdpDefault_SendTpPacket(context, *context->Peers[i], packet)) {
            /// \todo Handle error
        }
    }

    return result;
}

void TransportUdpDefault_RemovePeer(struct TransportUdpDefault_Context* const context, ENetPeer* const peer) {
    auto position = context->Peers->Find(&peer);

    if (position != -1) {
        context->Peers.Remove(position);
    }

    peer->data = nullptr;
}

void TransportUdpDefault_RemoveClients(struct TransportUdpDefault_Context* const context) {
    ENetEvent event;
    const uint32_t time_stamp = SDL_GetTicks() + TransportUdpDefault_DisconnectResponseTimeout;
    auto& peers = context->NetRole == TRANSPORT_SERVER ? context->Peers : context->RemotePeers;

    enet_host_flush(context->Host);

    for (auto i = 0; i < peers.GetCount(); ++i) {
        enet_peer_disconnect(*peers[i], 0);
    }

    for (; peers.GetCount();) {
        if (enet_host_service(context->Host, &event, TransportUdpDefault_ServiceTickPeriod) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                } break;

                case ENET_EVENT_TYPE_RECEIVE: {
                    enet_packet_destroy(event.packet);
                } break;

                case ENET_EVENT_TYPE_DISCONNECT: {
                    TransportUdpDefault_RemovePeer(context, event.peer);
                } break;
            }
        }

        if (time_stamp < SDL_GetTicks()) {
            for (auto i = 0; i < peers.GetCount(); ++i) {
                enet_peer_disconnect_now(*peers[i], 0);
                if (context->NetRole == TRANSPORT_SERVER) {
                    TransportUdpDefault_RemovePeer(context, *peers[i]);

                } else {
                    TransportUdpDefault_RemoveRemotePeer(context, *peers[i]);
                }
            }
        }
    }

    peers.Clear();
}

void TransportUdpDefault_ConnectRemotePeer(struct TransportUdpDefault_Context* const context, NetPacket& packet) {
    ENetAddress address{0, 0};
    ENetPeer* remote_peer{nullptr};

    packet >> address;

    remote_peer = enet_host_connect(context->Host, &address, TransportUdpDefault_Channels, 0);

    if (remote_peer) {
        context->RemotePeers.PushBack(&remote_peer);

    } else {
        /// \todo Handle error
    }
}

void TransportUdpDefault_RemoveRemotePeer(struct TransportUdpDefault_Context* const context, ENetPeer* const peer) {
    auto position = context->RemotePeers->Find(&peer);

    if (position != -1) {
        context->RemotePeers.Remove(position);
    }

    peer->data = nullptr;
}

void TransportUdpDefault_ProtocolErrorMessage(ENetPeer* const peer, uint8_t packet_type) {
    char peer_ip[40];

    if (enet_address_get_host_ip(&peer->address, peer_ip, sizeof(peer_ip))) {
        peer_ip[0] = '\0';
    }

    AiLog log("Transport protocol error: Unknown packet type received (%i) from '%s'.\n", packet_type, peer_ip);
}

void TransportUdpDefault_ProcessTpPacket(struct TransportUdpDefault_Context* const context, ENetPeer* const peer,
                                         ENetPacket* const enet_packet) {
    NetPacket packet;
    uint8_t packet_type;

    packet.Write(enet_packet->data, enet_packet->dataLength);

    packet >> packet_type;

    if (context->NetRole == TRANSPORT_SERVER) {
        switch (packet_type) {
            case TRANSPORT_PACKET_00: {
                if (TransportUdpDefault_VersionCheck(context, packet) &&
                    TransportUdpDefault_SendPeersListToPeer(context, peer) &&
                    TransportUdpDefault_BroadcastNewPeerArrived(context, peer)) {
                    context->Peers.PushBack(&peer);

                } else {
                    enet_peer_disconnect(peer, 0);
                }

            } break;

            default: {
                TransportUdpDefault_ProtocolErrorMessage(peer, packet_type);
            } break;
        }

    } else if (context->NetRole == TRANSPORT_CLIENT) {
        switch (packet_type) {
            case TRANSPORT_PACKET_01: {
                uint16_t peer_count;

                packet >> peer_count;

                for (auto i = 0; i < peer_count; ++i) {
                    TransportUdpDefault_ConnectRemotePeer(context, packet);
                }
            } break;

            case TRANSPORT_PACKET_02: {
                TransportUdpDefault_ConnectRemotePeer(context, packet);
            } break;

            default: {
                TransportUdpDefault_ProtocolErrorMessage(peer, packet_type);
            } break;
        }
    }
}

void TransportUdpDefault_ProcessApplPacket(struct TransportUdpDefault_Context* const context, ENetPeer* const peer,
                                           ENetPacket* const enet_packet) {
    NetPacket* packet = new (std::nothrow) NetPacket();
    uint8_t packet_type;
    NetAddress address;

    address.host = peer->address.host;
    address.port = peer->address.port;

    packet->AddAddress(address);
    packet->Write(enet_packet->data, enet_packet->dataLength);

    SDL_AtomicLock(&context->QueueLock);
    {
        context->RxPackets.PushBack(&packet);

        SDL_AtomicUnlock(&context->QueueLock);
    }
}

void TransportUdpDefault_TransmitApplPackets(struct TransportUdpDefault_Context* const context) {
    for (bool packets_pending = true; packets_pending;) {
        ENetPacket* enet_packet{nullptr};

        SDL_AtomicLock(&context->QueueLock);
        {
            if (context->TxPackets.GetCount() > 0) {
                NetPacket* local = *context->TxPackets[0];

                enet_packet = enet_packet_create(local->GetBuffer(), local->GetDataSize(), ENET_PACKET_FLAG_RELIABLE);

                context->TxPackets.Remove(0);

            } else {
                packets_pending = false;
            }

            SDL_AtomicUnlock(&context->QueueLock);
        }

        if (enet_packet) {
            /// \todo Properly support Unicast, Multicast and Broadcast messaging
            /// \todo Flush buffered packets as soon as a synch packet is found or simply exit loop
            enet_host_broadcast(context->Host, TRANSPORT_APPL_CHANNEL, enet_packet);
        }
    }
}

#if defined(MAX_ENABLE_UPNP)
void TransportUdpDefault_UpnpInit(struct TransportUdpDefault_Context* const context) noexcept {
    struct UPNPDev* device_list{nullptr};
    int discovery_result{UPNPDISCOVER_SUCCESS};

    device_list = upnpDiscover(TransportUdpDefault_UpnpDeviceResponseTimeout, nullptr, nullptr, UPNP_LOCAL_PORT_ANY, 0,
                               2, &discovery_result);

    if (UPNPDISCOVER_SUCCESS == discovery_result) {
        struct UPNPUrls UpnpUrls;
        struct IGDdatas UpnpIgdData;

        char lan_address[64];
        char wan_address[64];

        SDL_memset(&UpnpUrls, 0, sizeof(struct UPNPUrls));
        SDL_memset(&UpnpIgdData, 0, sizeof(struct IGDdatas));

        const int igd_search_result = UPNP_GetValidIGD(device_list, &UpnpUrls, &UpnpIgdData, lan_address,
                                                       sizeof(lan_address), wan_address, sizeof(wan_address));

        context->UpnpDevice.ExternalAddress = wan_address;

        switch (igd_search_result) {
            case 1:
            case 2: {
                context->UpnpDevice.ControlUrl = UpnpUrls.controlURL;
                context->UpnpDevice.ServiceType = UpnpIgdData.first.servicetype;
                context->UpnpDevice.HostAddress = lan_address;

                context->UpnpDevice.Status = TRANSPORT_IGDSTATUS_OK;

                if (!TransportUdpDefault_UpnpAddPortMapping(context->UpnpDevice, context->Host->address)) {
                    context->UpnpDevice.Status = TRANSPORT_IGDSTATUS_ERROR;
                }

                if (UPNPCOMMAND_SUCCESS == UPNP_GetExternalIPAddress(context->UpnpDevice.ControlUrl.GetCStr(),
                                                                     context->UpnpDevice.ServiceType.GetCStr(),
                                                                     wan_address)) {
                    context->UpnpDevice.ExternalAddress = wan_address;
                }
            } break;

            case 0:
            case 3: {
                context->UpnpDevice.Status = TRANSPORT_IGDSTATUS_NOIGD;
            } break;

            default: {
                context->UpnpDevice.Status = TRANSPORT_IGDSTATUS_ERROR;
            } break;
        }

        FreeUPNPUrls(&UpnpUrls);

    } else {
        context->UpnpDevice.Status = TRANSPORT_IGDSTATUS_ERROR;
    }

    freeUPNPDevlist(device_list);
}

void TransportUdpDefault_UpnpDeinit(struct TransportUdpDefault_Context* const context) noexcept {
    if (TRANSPORT_IGDSTATUS_OK == context->UpnpDevice.Status) {
        if (!TransportUdpDefault_UpnpRemovePortMapping(context->UpnpDevice, context->Host->address)) {
            /// \todo Handle error
        }
    }
}

bool TransportUdpDefault_UpnpAddPortMapping(struct UpnpDevice& device, ENetAddress& host_address) noexcept {
    bool result{true};

    if (device.Status == TRANSPORT_IGDSTATUS_OK) {
        SmartString port;
        char reserved_port[6]{'\0'};

        port.Sprintf(10, "%i", host_address.port);

        // If a control point uses the value 0 to indicate an infinite lease time mapping, it is REQUIRED that
        // gateway uses the maximum value instead (e.g. 604800 seconds) according to
        // WANIPConnection service 2.0 for UPnP Version 1.0.

        if (UPNP_AddAnyPortMapping(device.ControlUrl.GetCStr(), device.ServiceType.GetCStr(), port.GetCStr(),
                                   port.GetCStr(), device.HostAddress.GetCStr(), "M.A.X.", "UDP", nullptr, "0",
                                   reserved_port) == UPNPCOMMAND_SUCCESS) {
            host_address.port = std::strtol(reserved_port, nullptr, 10);

        } else {
            if (UPNP_AddPortMapping(device.ControlUrl.GetCStr(), device.ServiceType.GetCStr(), port.GetCStr(),
                                    port.GetCStr(), device.HostAddress.GetCStr(), "M.A.X.", "UDP", nullptr,
                                    "0") != UPNPCOMMAND_SUCCESS) {
                device.Status = TRANSPORT_IGDSTATUS_ERROR;

                result = false;
            }
        }
    }

    return result;
}

bool TransportUdpDefault_UpnpRemovePortMapping(struct UpnpDevice& device, ENetAddress& host_address) noexcept {
    bool result{true};

    if (device.Status == TRANSPORT_IGDSTATUS_OK) {
        SmartString port;

        port.Sprintf(10, "%i", host_address.port);

        if (UPNP_DeletePortMapping(device.ControlUrl.GetCStr(), device.ServiceType.GetCStr(), port.GetCStr(), "UDP",
                                   nullptr) != UPNPCOMMAND_SUCCESS) {
            result = false;
        }
    }

    return result;
}
#endif

int TransportUdpDefault_ServerFunction(void* data) noexcept {
    auto context = reinterpret_cast<struct TransportUdpDefault_Context*>(data);
    ENetEvent event;

    context->NetRole = TRANSPORT_SERVER;
    context->Host =
        enet_host_create(&context->ServerAddress, TransportUdpDefault_MaximumPeers, TransportUdpDefault_Channels, 0, 0);

    if (context->Host) {
#if defined(MAX_ENABLE_UPNP)
        TransportUdpDefault_UpnpInit(context);
#endif
        context->NetState = TRANSPORT_NETSTATE_CONNECTED;

        for (;;) {
            if (enet_host_service(context->Host, &event, TransportUdpDefault_ServiceTickPeriod) > 0) {
                switch (event.type) {
                    case ENET_EVENT_TYPE_CONNECT: {
                    } break;

                    case ENET_EVENT_TYPE_RECEIVE: {
                        switch (event.channelID) {
                            case TRANSPORT_TP_CHANNEL: {
                                TransportUdpDefault_ProcessTpPacket(context, event.peer, event.packet);
                            } break;

                            case TRANSPORT_APPL_CHANNEL: {
                                TransportUdpDefault_ProcessApplPacket(context, event.peer, event.packet);
                            } break;
                        }

                        enet_packet_destroy(event.packet);
                    } break;

                    case ENET_EVENT_TYPE_DISCONNECT: {
                        TransportUdpDefault_RemovePeer(context, event.peer);
                    } break;
                }
            }

            TransportUdpDefault_TransmitApplPackets(context);

            if (context->ExitThread) {
                TransportUdpDefault_RemoveClients(context);
                break;
            }
        }

#if defined(MAX_ENABLE_UPNP)
        TransportUdpDefault_UpnpDeinit(context);
#endif

        enet_host_destroy(context->Host);

        context->NetRole = -1;
        context->NetState = TRANSPORT_NETSTATE_DISCONNECTED;
    }

    return 0;
}

int TransportUdpDefault_ClientFunction(void* data) noexcept {
    struct TransportUdpDefault_Context* context = reinterpret_cast<struct TransportUdpDefault_Context*>(data);

    context->NetRole = TRANSPORT_CLIENT;

    context->Host = enet_host_create(nullptr, TransportUdpDefault_MaximumPeers, TransportUdpDefault_Channels, 0, 0);

    if (context->Host) {
#if defined(MAX_ENABLE_UPNP)
        TransportUdpDefault_UpnpInit(context);
#endif

        ENetPeer* server_peer =
            enet_host_connect(context->Host, &context->ServerAddress, TransportUdpDefault_Channels, 0);

        if (server_peer) {
            for (;;) {
                ENetEvent event;

                while (enet_host_service(context->Host, &event, TransportUdpDefault_ServiceTickPeriod) > 0) {
                    switch (event.type) {
                        case ENET_EVENT_TYPE_CONNECT: {
                            if (event.peer == server_peer) {
                                TransportUdpDefault_SendVersionInfo(context, event.peer);

                                context->NetState = TRANSPORT_NETSTATE_CONNECTED;
                            }
                        } break;

                        case ENET_EVENT_TYPE_RECEIVE: {
                            switch (event.channelID) {
                                case TRANSPORT_TP_CHANNEL: {
                                    TransportUdpDefault_ProcessTpPacket(context, event.peer, event.packet);
                                } break;

                                case TRANSPORT_APPL_CHANNEL: {
                                    TransportUdpDefault_ProcessApplPacket(context, event.peer, event.packet);
                                } break;
                            }

                            enet_packet_destroy(event.packet);
                        } break;

                        case ENET_EVENT_TYPE_DISCONNECT: {
                            if (event.peer == server_peer) {
                                context->NetState = TRANSPORT_NETSTATE_DISCONNECTED;

                                server_peer = enet_host_connect(context->Host, &context->ServerAddress,
                                                                TransportUdpDefault_Channels, 0);

                            } else {
                                TransportUdpDefault_RemoveRemotePeer(context, event.peer);
                            }
                        } break;
                    }
                }

                TransportUdpDefault_TransmitApplPackets(context);

                if (context->ExitThread) {
                    TransportUdpDefault_RemoveClients(context);
                    break;
                }
            }
        }

#if defined(MAX_ENABLE_UPNP)
        TransportUdpDefault_UpnpDeinit(context);
#endif

        enet_host_destroy(context->Host);

        context->RemotePeers.Clear();
        context->NetRole = -1;
        context->NetState = TRANSPORT_NETSTATE_DISCONNECTED;
    }

    return 0;
}
