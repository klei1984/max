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

#include <SDL3/SDL.h>
#include <SDL3/SDL_thread.h>
#include <enet/enet.h>

#if defined(MAX_ENABLE_UPNP)
#include <miniupnpc.h>
#include <upnpcommands.h>
#endif

#include <atomic>
#include <deque>
#include <utility>

#include "netlog.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "smartobjectarray.hpp"
#include "version.hpp"

enum : uint8_t {
    TRANSPORT_PACKET_00 = TRANSPORT_TP_PACKET_ID,
    TRANSPORT_PACKET_01,
    TRANSPORT_PACKET_02,
    TRANSPORT_PACKET_03,
};

enum {
    TRANSPORT_TP_CHANNEL,
    TRANSPORT_APPL_CHANNEL,
    TRANSPORT_CHANNEL_COUNT,
};

enum : intptr_t {
    TRANSPORT_PEER_UNVERIFIED,
    TRANSPORT_PEER_VERIFIED,
};

static constexpr uint16_t TransportUdpDefault_ProtocolVersionId = 0x0003;
static constexpr uint16_t TransportUdpDefault_DefaultHostPort = 31554;
static constexpr uint16_t TransportUdpDefault_MinimumHostPort = 1024;
static constexpr uint16_t TransportUdpDefault_MaximumHostPort = 65535;
static constexpr uint32_t TransportUdpDefault_ServiceTickPeriod = 10;
static constexpr uint32_t TransportUdpDefault_DisconnectResponseTimeout = 3000;
static constexpr uint32_t TransportUdpDefault_MaximumPeers = 32;
static constexpr uint32_t TransportUdpDefault_MaximumConnectAttempts = 5;
static constexpr size_t TransportUdpDefault_MaximumQueuedPackets = 1024;
static constexpr uint32_t TransportUdpDefault_Channels = TRANSPORT_CHANNEL_COUNT;

#if defined(MAX_ENABLE_UPNP)
static constexpr uint32_t TransportUdpDefault_UpnpDeviceResponseTimeout = 3000;

static_assert(MINIUPNPC_API_VERSION == 21, "API changes of MINIUPNP library shall be reviewed.");
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
    std::deque<NetPacket> TxPackets;
    std::deque<NetPacket> RxPackets;
    struct UpnpDevice UpnpDevice;
    ENetAddress ServerAddress;
    /* The client's link to the server. Null in the server role, and while a client is between
     * connection attempts. Control packets that only the server may send are checked against it.
     */
    ENetPeer* ServerPeer;
    std::atomic<bool> ExitThread;
    std::atomic<int32_t> NetState;
    std::atomic<int32_t> ErrorCode;
    int32_t NetRole;
    const char* LastError;
};

static int TransportUdpDefault_ClientFunction(void* data) noexcept;
static int TransportUdpDefault_ServerFunction(void* data) noexcept;
static inline uint16_t TransportUdpDefault_GetHostPort();
static inline void TransportUdpDefault_GetServerAddress(ENetAddress& address);
static inline bool TransportUdpDefault_SendTpPacket(struct TransportUdpDefault_Context* const context,
                                                    ENetPeer* const peer, const NetPacket& packet);
static inline bool TransportUdpDefault_SendVersionInfo(struct TransportUdpDefault_Context* const context,
                                                       ENetPeer* const peer);
static inline bool TransportUdpDefault_VersionCheck(struct TransportUdpDefault_Context* const context,
                                                    NetPacket& packet);
static inline bool TransportUdpDefault_SendVersionReject(struct TransportUdpDefault_Context* const context,
                                                         ENetPeer* const peer);
static inline void TransportUdpDefault_SetPeerState(ENetPeer* const peer, const intptr_t state) noexcept;
static inline bool TransportUdpDefault_IsPeerVerified(const ENetPeer* const peer) noexcept;
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
static ENetPeer* TransportUdpDefault_FindPeer(struct TransportUdpDefault_Context* const context,
                                              const NetAddress& address) noexcept;
static void TransportUdpDefault_SetErrorCode(struct TransportUdpDefault_Context* const context,
                                             int32_t error_code) noexcept;

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

uint16_t TransportUdpDefault_GetHostPort() {
    int32_t host_port = ResourceManager_GetSettings()->GetNumericValue("host_port");

    if (host_port < TransportUdpDefault_MinimumHostPort || host_port > TransportUdpDefault_MaximumHostPort) {
        host_port = TransportUdpDefault_DefaultHostPort;
    }

    return static_cast<uint16_t>(host_port);
}

/* Resolves the address of the server to connect to.
 *
 * This is a client side setting only. A server does not need to know its own address, and must not bind one, as that
 * would restrict it to a single local interface. See Init().
 *
 * The setting takes either a host name or an IPv4 literal. enet_address_set_host() resolves names through the operating
 * system resolver, so whether a bare LAN host name works depends on that host being resolvable there, through DNS, the
 * hosts file, or a local name service such as mDNS or NetBIOS. It parses IPv4 literals too, so it covers both forms.
 */
void TransportUdpDefault_GetServerAddress(ENetAddress& address) {
    std::string server_address = ResourceManager_GetSettings()->GetStringValue("host_address");

    if (server_address.empty() || enet_address_set_host(&address, server_address.c_str()) != 0) {
        (void)enet_address_set_host_ip(&address, "127.0.0.1");
    }

    address.port = TransportUdpDefault_GetHostPort();
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
        context->TxPackets.clear();
        context->RxPackets.clear();
        context->UpnpDevice.Status = TRANSPORT_IGDSTATUS_ERROR;
        context->ServerAddress.host = ENET_HOST_ANY;
        context->ServerAddress.port = TransportUdpDefault_DefaultHostPort;
        context->ServerPeer = nullptr;
        context->ExitThread.store(false, std::memory_order_release);
        context->NetState.store(TRANSPORT_NETSTATE_DEINITED, std::memory_order_release);
        context->ErrorCode.store(TRANSPORT_ERROR_NONE, std::memory_order_release);
        context->NetRole = -1;
        context->LastError = "No error.";

        if (ResourceManager_GetSettings()->GetNumericValue("log_file_debug")) {
            NetLog_Enable();
        }
    }

    if (context->NetState.load(std::memory_order_acquire) == TRANSPORT_NETSTATE_DEINITED) {
        if (Transport_IsInitialized()) {
            context->ErrorCode.store(TRANSPORT_ERROR_NONE, std::memory_order_release);
            context->NetState.store(TRANSPORT_NETSTATE_INITED, std::memory_order_release);

            SDL_assert(context->Thread == nullptr);

            if (mode == TRANSPORT_SERVER) {
                context->ServerAddress.host = ENET_HOST_ANY;
                context->ServerAddress.port = TransportUdpDefault_GetHostPort();

                context->Thread = SDL_CreateThread(&TransportUdpDefault_ServerFunction, "TransportUdpDefault", context);

            } else {
                TransportUdpDefault_GetServerAddress(context->ServerAddress);

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

            context->ExitThread.store(true, std::memory_order_release);
            SDL_WaitThread(context->Thread, &result);
            context->Thread = nullptr;

            /// \todo Handle error.
        }

        if (context->NetState.load(std::memory_order_acquire) != TRANSPORT_NETSTATE_DEINITED) {
            context->NetState.store(TRANSPORT_NETSTATE_DEINITED, std::memory_order_release);
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

void TransportUdpDefault_SetErrorCode(struct TransportUdpDefault_Context* const context, int32_t error_code) noexcept {
    int32_t expected{TRANSPORT_ERROR_NONE};

    (void)context->ErrorCode.compare_exchange_strong(expected, error_code, std::memory_order_acq_rel,
                                                     std::memory_order_relaxed);
}

const char* TransportUdpDefault::GetError() const {
    const char* error{""};

    if (context) {
        switch (context->ErrorCode.load(std::memory_order_acquire)) {
            case TRANSPORT_ERROR_BIND_FAILED: {
                error = "Could not bind the network port.";
            } break;

            case TRANSPORT_ERROR_PEER_CONNECT_FAILED: {
                error = "Could not connect to a remote peer.";
            } break;

            case TRANSPORT_ERROR_MESH_INCOMPLETE: {
                error = "Could not announce a peer to every participant.";
            } break;

            case TRANSPORT_ERROR_OUT_OF_MEMORY: {
                error = "Out of memory in the network layer.";
            } break;

            case TRANSPORT_ERROR_SEND_FAILED: {
                error = "Could not send to a remote peer.";
            } break;

            case TRANSPORT_ERROR_VERSION_MISMATCH: {
                error = "The other player runs a different version of the game.";
            } break;

            default: {
                error = context->LastError;
            } break;
        }
    }

    return error;
}

TransportStatus TransportUdpDefault::GetStatus() const {
    TransportStatus status{TRANSPORT_NETSTATE_DEINITED, TRANSPORT_ERROR_NONE};

    if (context) {
        status.state = context->NetState.load(std::memory_order_acquire);
        status.error_code = context->ErrorCode.load(std::memory_order_acquire);
    }

    return status;
}

bool TransportUdpDefault::TransmitPacket(NetPacket&& packet) {
    bool result{false};

    NETLOG(log, "Transmit");
    NETLOG_LOG(log, packet);

    SDL_LockSpinlock(&context->QueueLock);
    {
        if (context->TxPackets.size() < TransportUdpDefault_MaximumQueuedPackets) {
            context->TxPackets.push_back(std::move(packet));

            result = true;
        }

        SDL_UnlockSpinlock(&context->QueueLock);
    }

    if (!result) {
        NETLOG_LOG(log, "Transmit queue is full, packet rejected.");
    }

    return result;
}

bool TransportUdpDefault::ReceivePacket(NetPacket& packet) {
    bool result{false};

    packet.Reset();

    SDL_LockSpinlock(&context->QueueLock);
    {
        if (!context->RxPackets.empty()) {
            packet = std::move(context->RxPackets.front());
            context->RxPackets.pop_front();

            result = true;
        }

        SDL_UnlockSpinlock(&context->QueueLock);
    }

    if (result) {
        NETLOG(log, "Receive from {:4X}", packet.GetAddress(0).port);
        NETLOG_LOG(log, packet);
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

            if (enet_packet->referenceCount == 0) {
                enet_packet_destroy(enet_packet);
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

    const bool result{game_version == remote_game_version && enet_version == remote_enet_version &&
                      protocol_version == remote_protocol_version};

    if (!result) {
        AILOG(log,
              "Transport: Version mismatch. Local game {:08X} enet {:08X} protocol {:04X}, remote game {:08X} enet "
              "{:08X} protocol {:04X}.\n",
              game_version, enet_version, protocol_version, remote_game_version, remote_enet_version,
              remote_protocol_version);
    }

    return result;
}

/* Tells a peer why it is about to be dropped.
 *
 * A bare disconnect is indistinguishable from a network fault, which is what left a mismatched
 * client retrying forever. The rejected side reads this and stops.
 */
bool TransportUdpDefault_SendVersionReject(struct TransportUdpDefault_Context* const context, ENetPeer* const peer) {
    NetPacket packet;
    uint32_t enet_version{static_cast<uint32_t>(enet_linked_version())};
    uint16_t protocol_version{TransportUdpDefault_ProtocolVersionId};
    uint32_t game_version{GAME_VERSION};

    packet << static_cast<uint8_t>(TRANSPORT_PACKET_03);
    packet << game_version;
    packet << enet_version;
    packet << protocol_version;

    return TransportUdpDefault_SendTpPacket(context, peer, packet);
}

void TransportUdpDefault_SetPeerState(ENetPeer* const peer, const intptr_t state) noexcept {
    peer->data = reinterpret_cast<void*>(state);
}

bool TransportUdpDefault_IsPeerVerified(const ENetPeer* const peer) noexcept {
    return reinterpret_cast<intptr_t>(peer->data) == TRANSPORT_PEER_VERIFIED;
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
            result = false;

            NETLOG(log, "Failed to announce new peer {:8X}:{:4X} to peer {:8X}:{:4X}.", peer->address.host,
                   peer->address.port, (*context->Peers[i])->address.host, (*context->Peers[i])->address.port);
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
    const uint64_t time_stamp = SDL_GetTicks() + TransportUdpDefault_DisconnectResponseTimeout;
    auto& peers = context->NetRole == TRANSPORT_SERVER ? context->Peers : context->RemotePeers;

    enet_host_flush(context->Host);

    for (uint32_t i = 0; i < peers.GetCount(); ++i) {
        enet_peer_disconnect_later(*peers[i], 0);
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
                    if (context->NetRole == TRANSPORT_SERVER) {
                        TransportUdpDefault_RemovePeer(context, event.peer);

                    } else {
                        TransportUdpDefault_RemoveRemotePeer(context, event.peer);
                    }
                } break;
            }
        }

        if (time_stamp < SDL_GetTicks()) {
            for (uint32_t i = 0; i < peers.GetCount(); ++i) {
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

    context->ServerPeer = nullptr;
}

void TransportUdpDefault_ConnectRemotePeer(struct TransportUdpDefault_Context* const context, NetPacket& packet) {
    ENetAddress address{0, 0};
    ENetPeer* remote_peer{nullptr};

    packet >> address;

    remote_peer = enet_host_connect(context->Host, &address, TransportUdpDefault_Channels, 0);

    if (remote_peer) {
        context->RemotePeers.PushBack(&remote_peer);

    } else {
        NETLOG(log, "Failed to allocate a connection to peer {:8X}:{:4X}.", address.host, address.port);

        TransportUdpDefault_SetErrorCode(context, TRANSPORT_ERROR_PEER_CONNECT_FAILED);
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

    AILOG(log, "Transport protocol error: Unknown packet type received ({}) from '{}'.\n", packet_type, peer_ip);
}

void TransportUdpDefault_ProcessTpPacket(struct TransportUdpDefault_Context* const context, ENetPeer* const peer,
                                         ENetPacket* const enet_packet) {
    NetPacket packet;
    uint8_t packet_type;

    packet.Write(enet_packet->data, enet_packet->dataLength);

    packet >> packet_type;

    switch (packet_type) {
        case TRANSPORT_PACKET_00: {
            if (!TransportUdpDefault_VersionCheck(context, packet)) {
                (void)TransportUdpDefault_SendVersionReject(context, peer);

                if (peer == context->ServerPeer) {
                    TransportUdpDefault_SetErrorCode(context, TRANSPORT_ERROR_VERSION_MISMATCH);
                }

                enet_peer_disconnect_later(peer, 0);

                return;
            }

            TransportUdpDefault_SetPeerState(peer, TRANSPORT_PEER_VERIFIED);

            if (context->NetRole == TRANSPORT_SERVER) {
                if (TransportUdpDefault_SendPeersListToPeer(context, peer) &&
                    TransportUdpDefault_BroadcastNewPeerArrived(context, peer)) {
                    context->Peers.PushBack(&peer);

                } else {
                    enet_peer_disconnect(peer, 0);
                }

            } else if (peer == context->ServerPeer) {
                context->NetState.store(TRANSPORT_NETSTATE_CONNECTED, std::memory_order_release);

            } else if (context->RemotePeers->Find(&peer) == -1) {
                context->RemotePeers.PushBack(&peer);
            }

        } break;

        case TRANSPORT_PACKET_03: {
            (void)TransportUdpDefault_VersionCheck(context, packet);

            if (peer == context->ServerPeer) {
                TransportUdpDefault_SetErrorCode(context, TRANSPORT_ERROR_VERSION_MISMATCH);
            }

            enet_peer_disconnect_later(peer, 0);

        } break;

        case TRANSPORT_PACKET_01: {
            if (context->NetRole != TRANSPORT_CLIENT || peer != context->ServerPeer ||
                !TransportUdpDefault_IsPeerVerified(peer)) {
                TransportUdpDefault_ProtocolErrorMessage(peer, packet_type);

                return;
            }

            uint16_t peer_count;

            packet >> peer_count;

            for (auto i = 0; i < peer_count; ++i) {
                TransportUdpDefault_ConnectRemotePeer(context, packet);
            }
        } break;

        case TRANSPORT_PACKET_02: {
            if (context->NetRole != TRANSPORT_CLIENT || peer != context->ServerPeer ||
                !TransportUdpDefault_IsPeerVerified(peer)) {
                TransportUdpDefault_ProtocolErrorMessage(peer, packet_type);

                return;
            }

            TransportUdpDefault_ConnectRemotePeer(context, packet);
        } break;

        default: {
            TransportUdpDefault_ProtocolErrorMessage(peer, packet_type);
        } break;
    }
}

void TransportUdpDefault_ProcessApplPacket(struct TransportUdpDefault_Context* const context, ENetPeer* const peer,
                                           ENetPacket* const enet_packet) {
    NetPacket packet;
    NetAddress address;

    if (!TransportUdpDefault_IsPeerVerified(peer)) {
        NETLOG(log, "Dropped {} application bytes from unverified peer {:8X}:{:4X}.", enet_packet->dataLength,
               peer->address.host, peer->address.port);

        return;
    }

    address.host = peer->address.host;
    address.port = peer->address.port;

    packet.AddAddress(address);
    packet.Write(enet_packet->data, enet_packet->dataLength);

    bool accepted{false};

    SDL_LockSpinlock(&context->QueueLock);
    {
        if (context->RxPackets.size() < TransportUdpDefault_MaximumQueuedPackets) {
            context->RxPackets.push_back(std::move(packet));

            accepted = true;
        }

        SDL_UnlockSpinlock(&context->QueueLock);
    }

    if (!accepted) {
        NETLOG(log, "Receive queue is full, packet from {:8X}:{:4X} dropped.", peer->address.host, peer->address.port);
    }
}

/* Resolves a recipient address to a connected peer.
 *
 * The peer mesh can hold more than one connection to the same remote host, as both ends dial each
 * other during the join handshake. Returning the first match collapses those duplicates, so a
 * packet is delivered once per remote host rather than once per connection.
 */
ENetPeer* TransportUdpDefault_FindPeer(struct TransportUdpDefault_Context* const context,
                                       const NetAddress& address) noexcept {
    ENetPeer* result{nullptr};

    for (size_t i = 0; i < context->Host->peerCount; ++i) {
        ENetPeer* const peer = &context->Host->peers[i];

        if (peer->state == ENET_PEER_STATE_CONNECTED && peer->address.host == address.host &&
            peer->address.port == address.port) {
            result = peer;
            break;
        }
    }

    return result;
}

void TransportUdpDefault_TransmitApplPackets(struct TransportUdpDefault_Context* const context) {
    bool packets_transmitted{false};

    for (;;) {
        NetPacket local;
        bool packet_pending{false};

        SDL_LockSpinlock(&context->QueueLock);
        {
            if (!context->TxPackets.empty()) {
                local = std::move(context->TxPackets.front());
                context->TxPackets.pop_front();

                packet_pending = true;
            }

            SDL_UnlockSpinlock(&context->QueueLock);
        }

        if (!packet_pending) {
            break;
        }

        packets_transmitted = true;

        ENetPacket* const enet_packet =
            enet_packet_create(local.GetBuffer(), local.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);

        if (!enet_packet) {
            NETLOG(log, "Failed to allocate a packet of {} bytes, packet lost.", local.GetDataSize());

            TransportUdpDefault_SetErrorCode(context, TRANSPORT_ERROR_OUT_OF_MEMORY);

            continue;
        }

        const uint16_t address_count = local.GetAddressCount();

        if (address_count == 0) {
            /* An empty recipient table means broadcast. enet_host_broadcast() releases the packet
             * itself when no peer took a reference.
             */
            enet_host_broadcast(context->Host, TRANSPORT_APPL_CHANNEL, enet_packet);

        } else {
            for (uint16_t index = 0; index < address_count; ++index) {
                ENetPeer* const target = TransportUdpDefault_FindPeer(context, local.GetAddress(index));

                if (target) {
                    if (enet_peer_send(target, TRANSPORT_APPL_CHANNEL, enet_packet) < 0) {
                        NETLOG(log, "Failed to queue a packet of {} bytes to peer {:8X}:{:4X}.", local.GetDataSize(),
                               local.GetAddress(index).host, local.GetAddress(index).port);

                        TransportUdpDefault_SetErrorCode(context, TRANSPORT_ERROR_SEND_FAILED);
                    }

                } else {
                    NETLOG(log, "No connected peer for recipient {:8X}:{:4X}, packet dropped.",
                           local.GetAddress(index).host, local.GetAddress(index).port);
                }
            }

            if (enet_packet->referenceCount == 0) {
                enet_packet_destroy(enet_packet);
            }
        }
    }

    if (packets_transmitted) {
        enet_host_flush(context->Host);
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
            NETLOG(log, "Failed to remove the UPnP port mapping for port {}.", context->Host->address.port);

            TransportUdpDefault_SetErrorCode(context, TRANSPORT_ERROR_UPNP_UNMAP_FAILED);
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
        context->NetState.store(TRANSPORT_NETSTATE_CONNECTED, std::memory_order_release);

        for (;;) {
            if (enet_host_service(context->Host, &event, TransportUdpDefault_ServiceTickPeriod) > 0) {
                switch (event.type) {
                    case ENET_EVENT_TYPE_CONNECT: {
                        TransportUdpDefault_SetPeerState(event.peer, TRANSPORT_PEER_UNVERIFIED);
                        (void)TransportUdpDefault_SendVersionInfo(context, event.peer);
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

            if (context->ExitThread.load(std::memory_order_acquire)) {
                TransportUdpDefault_TransmitApplPackets(context);
                TransportUdpDefault_RemoveClients(context);
                break;
            }
        }

#if defined(MAX_ENABLE_UPNP)
        TransportUdpDefault_UpnpDeinit(context);
#endif

        enet_host_destroy(context->Host);

        context->NetRole = -1;
        context->NetState.store(TRANSPORT_NETSTATE_DISCONNECTED, std::memory_order_release);

    } else {
        NETLOG(log, "Failed to bind the server host to port {}.", context->ServerAddress.port);

        TransportUdpDefault_SetErrorCode(context, TRANSPORT_ERROR_BIND_FAILED);

        context->NetRole = -1;
        context->NetState.store(TRANSPORT_NETSTATE_DISCONNECTED, std::memory_order_release);
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

        uint32_t connect_attempts{1};

        context->ServerPeer =
            enet_host_connect(context->Host, &context->ServerAddress, TransportUdpDefault_Channels, 0);

        if (context->ServerPeer) {
            context->RemotePeers.PushBack(&context->ServerPeer);

            for (;;) {
                ENetEvent event;

                while (enet_host_service(context->Host, &event, TransportUdpDefault_ServiceTickPeriod) > 0) {
                    switch (event.type) {
                        case ENET_EVENT_TYPE_CONNECT: {
                            TransportUdpDefault_SetPeerState(event.peer, TRANSPORT_PEER_UNVERIFIED);
                            (void)TransportUdpDefault_SendVersionInfo(context, event.peer);

                            if (event.peer == context->ServerPeer) {
                                connect_attempts = 0;
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
                            TransportUdpDefault_SetPeerState(event.peer, TRANSPORT_PEER_UNVERIFIED);
                            TransportUdpDefault_RemoveRemotePeer(context, event.peer);

                            if (event.peer == context->ServerPeer) {
                                context->NetState.store(TRANSPORT_NETSTATE_DISCONNECTED, std::memory_order_release);

                                context->ServerPeer = nullptr;

                                if (context->ErrorCode.load(std::memory_order_acquire) != TRANSPORT_ERROR_NONE) {
                                    break;
                                }

                                if (connect_attempts >= TransportUdpDefault_MaximumConnectAttempts) {
                                    NETLOG(log, "Giving up on the server after {} connection attempts.",
                                           connect_attempts);

                                    TransportUdpDefault_SetErrorCode(context, TRANSPORT_ERROR_PEER_CONNECT_FAILED);

                                    break;
                                }

                                ++connect_attempts;

                                context->ServerPeer = enet_host_connect(context->Host, &context->ServerAddress,
                                                                        TransportUdpDefault_Channels, 0);

                                if (context->ServerPeer) {
                                    context->RemotePeers.PushBack(&context->ServerPeer);

                                } else {
                                    NETLOG(log, "Failed to allocate a peer for the server reconnection.");

                                    TransportUdpDefault_SetErrorCode(context, TRANSPORT_ERROR_PEER_CONNECT_FAILED);
                                }
                            }
                        } break;
                    }
                }

                TransportUdpDefault_TransmitApplPackets(context);

                if (context->ExitThread.load(std::memory_order_acquire)) {
                    TransportUdpDefault_TransmitApplPackets(context);
                    TransportUdpDefault_RemoveClients(context);
                    break;
                }
            }

        } else {
            NETLOG(log, "Failed to allocate a peer for the server connection.");

            TransportUdpDefault_SetErrorCode(context, TRANSPORT_ERROR_PEER_CONNECT_FAILED);
        }

#if defined(MAX_ENABLE_UPNP)
        TransportUdpDefault_UpnpDeinit(context);
#endif

        enet_host_destroy(context->Host);

        context->RemotePeers.Clear();
        context->NetRole = -1;
        context->NetState.store(TRANSPORT_NETSTATE_DISCONNECTED, std::memory_order_release);

    } else {
        NETLOG(log, "Failed to create the client host.");

        TransportUdpDefault_SetErrorCode(context, TRANSPORT_ERROR_BIND_FAILED);

        context->NetRole = -1;
        context->NetState.store(TRANSPORT_NETSTATE_DISCONNECTED, std::memory_order_release);
    }

    return 0;
}
