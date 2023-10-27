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

#include "inifile.hpp"

#define TRANSPORT_DEFAULT_SESSION_ID 0x913F
#define TRANSPORT_DEFAULT_HOST_PORT 213

enum {
    TRANSPORT_NETSTATE_DEINITED,
    TRANSPORT_NETSTATE_INITED,
    TRANSPORT_NETSTATE_CONNECTED,
    TRANSPORT_NETSTATE_DISCONNECTED,
};

TransportUdpDefault::TransportUdpDefault()
    : NetState(TRANSPORT_NETSTATE_DEINITED),
      network_role(-1),
      LastError("No error\n"),
      SessionId(TRANSPORT_DEFAULT_SESSION_ID),
      socket(nullptr),
      channel(-1),
      UdpPacket(nullptr) {
    for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
        channels[i] = -1;
    }

    BroadcastAddress.host = INADDR_BROADCAST;
    BroadcastAddress.port = TRANSPORT_DEFAULT_HOST_PORT;
}

TransportUdpDefault::~TransportUdpDefault() { Deinit(); }

bool TransportUdpDefault::SetupHost() {
    bool result;
    IPaddress address;
    Uint16 host_port;

    network_role = TRANSPORT_SERVER;

    host_port = ini_get_setting(INI_NETWORK_HOST_PORT);
    if (!host_port) {
        host_port = TRANSPORT_DEFAULT_HOST_PORT;
    }

    address.host = INADDR_BROADCAST;
    address.port = host_port;

    socket = SDLNet_UDP_Open(host_port);
    if (!socket) {
        SDL_Log("Transport: Unable to open UDP socket. %s\n", SDLNet_GetError());
        SetError("Network socket error.");
        result = false;
    } else {
        channel = SDLNet_UDP_Bind(socket, -1, &address);
        if (channel == -1) {
            SDL_Log("Transport: Unable to open UDP socket. %s\n", SDLNet_GetError());
            SetError("Network channel error.");
            result = false;
        } else {
            NetState = TRANSPORT_NETSTATE_INITED;
            result = true;
        }
    }

    return result;
}

bool TransportUdpDefault::SetupClient() {
    char host_address[30];
    IPaddress address;
    Uint16 host_port;
    bool result;

    network_role = TRANSPORT_CLIENT;

    host_port = ini_get_setting(INI_NETWORK_HOST_PORT);
    if (!host_port) {
        host_port = TRANSPORT_DEFAULT_HOST_PORT;
    }

    if (!ini_config.GetStringValue(INI_NETWORK_HOST_ADDRESS, host_address, sizeof(host_address))) {
        strcpy(host_address, "255.255.255.255");
    }

    socket = SDLNet_UDP_Open(0);
    if (!socket) {
        SDL_Log("Transport: Unable to open UDP socket. %s\n", SDLNet_GetError());
        SetError("Network socket error.");
        result = false;
    } else {
        if (SDLNet_ResolveHost(&address, host_address, host_port) == -1) {
            address.host = INADDR_BROADCAST;
            address.port = host_port;
        }

        BroadcastAddress.host = address.host;
        BroadcastAddress.port = address.port;

        channel = SDLNet_UDP_Bind(socket, -1, &address);
        if (channel == -1) {
            SDL_Log("Transport: Unable to open UDP socket. %s\n", SDLNet_GetError());
            SetError("Network channel error.");
            result = false;
        } else {
            NetState = TRANSPORT_NETSTATE_INITED;
            result = true;
        }
    }

    return result;
}

bool TransportUdpDefault::Init(int32_t mode) {
    bool result;

    if (SDLNet_Init() == -1) {
        SDL_Log("Transport: Unable to initialize SDL Net. %s\n", SDLNet_GetError());

        SetError("Network not available.");

        result = false;

    } else {
        UdpPacket = SDLNet_AllocPacket(TRANSPORT_MAX_PACKET_SIZE);
        SDL_assert(UdpPacket);

        if (mode == TRANSPORT_SERVER) {
            result = SetupHost();

        } else {
            result = SetupClient();
        }
    }

    return result;
}

bool TransportUdpDefault::Deinit() {
    if (socket) {
        if (channel != -1) {
            SDLNet_UDP_Unbind(socket, channel);
            channel = -1;
        }

        SDLNet_UDP_Close(socket);
        socket = nullptr;
    }

    if (UdpPacket) {
        SDLNet_FreePacket(UdpPacket);
        UdpPacket = nullptr;
    }

    SDLNet_Quit();
    NetState = TRANSPORT_NETSTATE_DEINITED;

    return true;
}

bool TransportUdpDefault::Connect() { return false; }

bool TransportUdpDefault::Disconnect() { return false; }

void TransportUdpDefault::SetSessionId(uint16_t session_id) { SessionId = session_id; }

void TransportUdpDefault::SetError(const char* error) { LastError = error; }

const char* TransportUdpDefault::GetError() const { return LastError; }

bool inline TransportUdpDefault_TransmitPacket(UDPsocket socket, UDPpacket& udp_packet) {
    bool result;

    if (SDLNet_UDP_Send(socket, udp_packet.channel, &udp_packet) == 0) {
        SDL_Log("Transport: Transmit failed. %s\n", SDLNet_GetError());
        result = false;

    } else if (udp_packet.len != udp_packet.status) {
        SDL_Log("Transport: Packet size mismatch (%i/%i).\n", udp_packet.len, udp_packet.status);
        result = false;

    } else {
        result = true;
    }

    return result;
}

bool TransportUdpDefault::TransmitPacket(NetPacket& packet) {
    bool result = true;
    UDPpacket udp_packet;

    if (!packet.GetAddressCount()) {
        SDL_assert(network_role == TRANSPORT_CLIENT);
        packet.AddAddress(BroadcastAddress);
    }

    udp_packet.channel = -1;
    udp_packet.data = reinterpret_cast<Uint8*>(packet.GetBuffer());
    udp_packet.len = packet.GetDataSize();
    udp_packet.maxlen = packet.GetDataSize();

    for (int32_t i = 0; i < packet.GetAddressCount(); ++i) {
        udp_packet.address.host = packet.GetAddress(i).host;
        udp_packet.address.port = packet.GetAddress(i).port;

        result = TransportUdpDefault_TransmitPacket(socket, udp_packet);
        if (!result) {
            break;
        }
    }

    return result;
}

bool TransportUdpDefault::ReceivePacket(NetPacket& packet) {
    int32_t state;
    bool result;

    state = SDLNet_UDP_Recv(socket, UdpPacket);

    if (state == -1) {
        SDL_Log("Transport: Receive error. %s\n", SDLNet_GetError());
        result = false;

    } else if (state > 0) {
        NetAddress address;

        address.host = UdpPacket->address.host;
        address.port = UdpPacket->address.port;

        packet.Reset();
        packet.AddAddress(address);
        packet.Write(UdpPacket->data, UdpPacket->len);

        result = true;

    } else {
        result = false;
    }

    return result;
}
