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
#include <miniupnpc.h>
#include <upnpcommands.h>

#include <utility>

#include "inifile.hpp"

enum : uint8_t {
    TRANSPORT_PACKET_00 = TRANSPORT_TP_PACKET_ID,
    TRANSPORT_PACKET_01,
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

/// \todo Test protocol version
static constexpr uint16_t TransportUdpDefault_ProtocolVersionId = 0x0001;
static constexpr uint16_t TransportUdpDefault_DefaultSessionId = 0x913F;
static constexpr uint16_t TransportUdpDefault_DefaultHostPort = 31554;
static constexpr uint32_t TransportUdpDefault_ServiceTickPeriod = 10;
static constexpr uint32_t TransportUdpDefault_MaximumPeers = 32;
static constexpr uint32_t TransportUdpDefault_Channels = TRANSPORT_CHANNEL_COUNT;

static constexpr uint32_t TransportUdpDefault_UpnpDeviceResponseTimeout = 3000;
static_assert(MINIUPNPC_API_VERSION == 17, "API changes of MINIUPNP library shall be reviewed.");

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
    uint16_t SessionId;
};

static int TransportUdpDefault_ClientFunction(void* data) noexcept;
static int TransportUdpDefault_ServerFunction(void* data) noexcept;
static inline void TransportUdpDefault_GetServerAddress(ENetAddress& address);
static inline void TransportUdpDefault_SendPeersListToPeer(struct TransportUdpDefault_Context* const context,
                                                           ENetPeer* const peer);
static inline void TransportUdpDefault_BroadcastNewPeerArrived(struct TransportUdpDefault_Context* const context,
                                                               ENetPeer* const peer);
static inline void TransportUdpDefault_RemoveClient(struct TransportUdpDefault_Context* const context,
                                                    ENetPeer* const peer);
static inline void TransportUdpDefault_ConnectRemotePeer(struct TransportUdpDefault_Context* const context,
                                                         NetPacket& packet);
static inline void TransportUdpDefault_RemoveRemotePeer(struct TransportUdpDefault_Context* const context,
                                                        ENetPeer* const peer);
static inline void TransportUdpDefault_ProcessTpPacket(struct TransportUdpDefault_Context* const context,
                                                       ENetPeer* const peer, ENetPacket* const enet_packet);
static inline void TransportUdpDefault_ProcessApplPacket(struct TransportUdpDefault_Context* const context,
                                                         ENetPeer* const peer, ENetPacket* const enet_packet);
static inline void TransportUdpDefault_TransmitApplPackets(struct TransportUdpDefault_Context* const context);
static void TransportUdpDefault_UpnpInit(struct TransportUdpDefault_Context* const context) noexcept;
static void TransportUdpDefault_UpnpDeinit(struct TransportUdpDefault_Context* const context) noexcept;
static void TransportUdpDefault_UpnpAddPortMapping(struct UpnpDevice& device, ENetAddress& host_address) noexcept;
static void TransportUdpDefault_UpnpRemovePortMapping(struct UpnpDevice& device, ENetAddress& host_address) noexcept;

TransportUdpDefault::~TransportUdpDefault() {
    Deinit();

    delete context;
    context = nullptr;
}

void TransportUdpDefault_GetServerAddress(ENetAddress& address) {
    char server_address[30];

    if (!ini_config.GetStringValue(INI_NETWORK_HOST_ADDRESS, server_address, sizeof(server_address)) ||
        enet_address_set_host_ip(&address, server_address) != 0) {
        address.host = ENET_HOST_ANY;
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
        context->SessionId = TransportUdpDefault_DefaultSessionId;
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
                SetError("Network initialization error.");
            }

        } else {
            SetError("Network initialization error.");
        }

    } else {
        result = true;
    }

    return result;
}

bool TransportUdpDefault::Deinit() {
    if (context) {
        if (context->Thread) {
            context->ExitThread = true;
            SDL_WaitThread(context->Thread, nullptr);
            context->Thread = nullptr;
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

void TransportUdpDefault::SetSessionId(uint16_t session_id) {
    /// \todo The SessionID was part of the IPX Protocol Header. Should it be removed?

    if (context) {
        context->SessionId = session_id;
    }
}

void TransportUdpDefault::SetError(const char* error) {
    if (context) {
        context->LastError = error;
    }
}

const char* TransportUdpDefault::GetError() const {
    const char* error{nullptr};

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

    return result;
}

void TransportUdpDefault_SendPeersListToPeer(struct TransportUdpDefault_Context* const context, ENetPeer* const peer) {
    NetPacket packet;
    const uint16_t peer_count = context->Peers.GetCount();

    packet << static_cast<uint8_t>(TRANSPORT_PACKET_00);
    packet << peer_count;

    for (auto i = 0; i < peer_count; ++i) {
        packet << (*context->Peers[i])->address;
    }

    ENetPacket* enet_packet = enet_packet_create(packet.GetBuffer(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);

    if (enet_packet) {
        if (enet_peer_send(peer, TRANSPORT_TP_CHANNEL, enet_packet) != 0) {
            /// \todo Handle error
        }

        enet_packet = nullptr;
    }
}

void TransportUdpDefault_BroadcastNewPeerArrived(struct TransportUdpDefault_Context* const context,
                                                 ENetPeer* const peer) {
    NetPacket packet;

    packet << static_cast<uint8_t>(TRANSPORT_PACKET_01);
    packet << peer->address;

    ENetPacket* enet_packet = enet_packet_create(packet.GetBuffer(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);

    if (enet_packet) {
        enet_host_broadcast(context->Host, TRANSPORT_TP_CHANNEL, enet_packet);
        enet_packet = nullptr;
    }
}

void TransportUdpDefault_RemoveClient(struct TransportUdpDefault_Context* const context, ENetPeer* const peer) {
    auto position = context->Peers->Find(&peer);

    if (position != -1) {
        context->Peers.Remove(position);
    }

    peer->data = nullptr;
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

void TransportUdpDefault_ProcessTpPacket(struct TransportUdpDefault_Context* const context, ENetPeer* const peer,
                                         ENetPacket* const enet_packet) {
    NetPacket packet;
    uint8_t packet_type;

    packet.Write(enet_packet->data, enet_packet->dataLength);

    packet >> packet_type;

    if (context->NetRole == TRANSPORT_SERVER) {
        switch (packet_type) {
            default: {
            } break;
        }

    } else if (context->NetRole == TRANSPORT_CLIENT) {
        switch (packet_type) {
            case TRANSPORT_PACKET_00: {
                uint16_t peer_count;

                packet >> peer_count;

                for (auto i = 0; i < peer_count; ++i) {
                    TransportUdpDefault_ConnectRemotePeer(context, packet);
                }
            } break;

            case TRANSPORT_PACKET_01: {
                TransportUdpDefault_ConnectRemotePeer(context, packet);
            } break;

            default: {
                /// \todo Handle error
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
            enet_host_broadcast(context->Host, TRANSPORT_APPL_CHANNEL, enet_packet);
        }
    }
}

void TransportUdpDefault_UpnpInit(struct TransportUdpDefault_Context* const context) noexcept {
    struct UPNPDev* device_list{nullptr};
    int discovery_result{UPNPDISCOVER_SUCCESS};

    device_list = upnpDiscover(TransportUdpDefault_UpnpDeviceResponseTimeout, nullptr, nullptr, UPNP_LOCAL_PORT_ANY, 0,
                               2, &discovery_result);

    if (UPNPDISCOVER_SUCCESS == discovery_result) {
        struct UPNPUrls UpnpUrls;
        struct IGDdatas UpnpIgdData;

        char network_address[64];

        SDL_memset(&UpnpUrls, 0, sizeof(struct UPNPUrls));
        SDL_memset(&UpnpIgdData, 0, sizeof(struct IGDdatas));

        const int igd_search_result =
            UPNP_GetValidIGD(device_list, &UpnpUrls, &UpnpIgdData, network_address, sizeof(network_address));

        switch (igd_search_result) {
            case 1:
            case 2: {
                char ip_string[40];

                context->UpnpDevice.ControlUrl = UpnpUrls.controlURL;
                context->UpnpDevice.ServiceType = UpnpIgdData.first.servicetype;
                context->UpnpDevice.HostAddress = network_address;

                context->UpnpDevice.Status = TRANSPORT_IGDSTATUS_OK;

                TransportUdpDefault_UpnpAddPortMapping(context->UpnpDevice, context->Host->address);

                if (UPNPCOMMAND_SUCCESS == UPNP_GetExternalIPAddress(context->UpnpDevice.ControlUrl.GetCStr(),
                                                                     context->UpnpDevice.ServiceType.GetCStr(),
                                                                     ip_string)) {
                    context->UpnpDevice.ExternalAddress = ip_string;
                }
            } break;

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
    TransportUdpDefault_UpnpRemovePortMapping(context->UpnpDevice, context->Host->address);
}

void TransportUdpDefault_UpnpAddPortMapping(struct UpnpDevice& device, ENetAddress& host_address) noexcept {
    if (device.Status == TRANSPORT_IGDSTATUS_OK) {
        SmartString port;
        int result;

        port.Sprintf(10, "%i", host_address.port);

        result = UPNP_AddPortMapping(device.ControlUrl.GetCStr(), device.ServiceType.GetCStr(), port.GetCStr(),
                                     port.GetCStr(), device.HostAddress.GetCStr(), "M.A.X.", "UDP", nullptr, "0");
    }

    /// \todo Handle error
}

void TransportUdpDefault_UpnpRemovePortMapping(struct UpnpDevice& device, ENetAddress& host_address) noexcept {
    if (device.Status == TRANSPORT_IGDSTATUS_OK) {
        SmartString port;
        int result;

        port.Sprintf(10, "%i", host_address.port);

        result = UPNP_DeletePortMapping(device.ControlUrl.GetCStr(), device.ServiceType.GetCStr(), port.GetCStr(),
                                        "UDP", nullptr);
    }
    /// \todo Handle error
}

int TransportUdpDefault_ServerFunction(void* data) noexcept {
    auto context = reinterpret_cast<struct TransportUdpDefault_Context*>(data);
    ENetEvent event;

    context->NetRole = TRANSPORT_SERVER;
    context->Host =
        enet_host_create(&context->ServerAddress, TransportUdpDefault_MaximumPeers, TransportUdpDefault_Channels, 0, 0);

    if (context->Host) {
        TransportUdpDefault_UpnpInit(context);

        context->NetState = TRANSPORT_NETSTATE_CONNECTED;

        for (;;) {
            if (enet_host_service(context->Host, &event, TransportUdpDefault_ServiceTickPeriod) > 0) {
                switch (event.type) {
                    case ENET_EVENT_TYPE_CONNECT: {
                        TransportUdpDefault_SendPeersListToPeer(context, event.peer);
                        TransportUdpDefault_BroadcastNewPeerArrived(context, event.peer);

                        context->Peers.PushBack(&event.peer);
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
                        TransportUdpDefault_RemoveClient(context, event.peer);
                    } break;
                }
            }

            TransportUdpDefault_TransmitApplPackets(context);

            if (context->ExitThread) {
                break;
            }
        }

        TransportUdpDefault_UpnpDeinit(context);

        enet_host_destroy(context->Host);

        context->Peers.Clear();
        context->NetRole = -1;
        context->NetState = TRANSPORT_NETSTATE_DISCONNECTED;
    }

    return 0;
}

int TransportUdpDefault_ClientFunction(void* data) noexcept {
    struct TransportUdpDefault_Context* context = reinterpret_cast<struct TransportUdpDefault_Context*>(data);
    ENetAddress host_address;

    context->NetRole = TRANSPORT_CLIENT;

    host_address.host = ENET_HOST_ANY;
    host_address.port = ENET_PORT_ANY;

    context->Host =
        enet_host_create(&host_address, TransportUdpDefault_MaximumPeers, TransportUdpDefault_Channels, 0, 0);

    if (context->Host) {
        TransportUdpDefault_UpnpInit(context);

        ENetPeer* server_peer =
            enet_host_connect(context->Host, &context->ServerAddress, TransportUdpDefault_Channels, 0);

        if (server_peer) {
            for (;;) {
                ENetEvent event;

                while (enet_host_service(context->Host, &event, TransportUdpDefault_ServiceTickPeriod) > 0) {
                    switch (event.type) {
                        case ENET_EVENT_TYPE_CONNECT: {
                            if (event.peer == server_peer) {
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
                    break;
                }
            }
        }

        TransportUdpDefault_UpnpDeinit(context);

        enet_host_destroy(context->Host);

        context->RemotePeers.Clear();
        context->NetRole = -1;
        context->NetState = TRANSPORT_NETSTATE_DISCONNECTED;
    }

    return 0;
}
