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

#ifndef TRANSPORT_HPP
#define TRANSPORT_HPP

#include "net_packet.hpp"

#define TRANSPORT_MAX_TEAM_COUNT 4
#define TRANSPORT_MAX_PACKET_SIZE 1440
#define TRANSPORT_DEFAULT_TYPE "udp_default"

enum {
    TRANSPORT_DEFAULT_UDP,
};

enum {
    TRANSPORT_SERVER,
    TRANSPORT_CLIENT,
};

enum : uint8_t { TRANSPORT_TP_PACKET_ID = 0x00, TRANSPORT_APPL_PACKET_ID = 0x50 };

/* Session state. Published by the network worker thread, polled by the main thread. */
enum : int32_t {
    TRANSPORT_NETSTATE_DEINITED,
    TRANSPORT_NETSTATE_INITED,
    TRANSPORT_NETSTATE_CONNECTED,
    TRANSPORT_NETSTATE_DISCONNECTED,
};

/* Failure reasons the network worker thread can report.
 *
 * An enum rather than a string: it crosses a thread boundary, so it must have no lifetime
 * questions, and it has to be translatable at the point of display rather than at the point of
 * failure. The first reported failure wins, so a cascade cannot bury the root cause.
 */
enum : int32_t {
    TRANSPORT_ERROR_NONE,
    TRANSPORT_ERROR_BIND_FAILED,
    TRANSPORT_ERROR_PEER_CONNECT_FAILED,
    TRANSPORT_ERROR_MESH_INCOMPLETE,
    TRANSPORT_ERROR_OUT_OF_MEMORY,
    TRANSPORT_ERROR_SEND_FAILED,
    TRANSPORT_ERROR_VERSION_MISMATCH,
    TRANSPORT_ERROR_UPNP_UNMAP_FAILED,
};

struct TransportStatus {
    int32_t state;
    int32_t error_code;

    [[nodiscard]] bool IsFatal() const noexcept {
        /* UPnP teardown failures are diagnostic only and must never abort a session. */
        return error_code != TRANSPORT_ERROR_NONE && error_code != TRANSPORT_ERROR_UPNP_UNMAP_FAILED;
    }
};

class Transport {
public:
    virtual ~Transport() {};
    virtual const char* GetError() const = 0;

    /* Reads the state the worker thread publishes. Atomic loads only -- never blocks, never waits,
     * so it is safe to call from the frame loop every frame. See the no-blocking rule in
     * transport_udp_default.cpp.
     */
    [[nodiscard]] virtual TransportStatus GetStatus() const = 0;
    virtual bool Init(int32_t mode) = 0;
    virtual bool Deinit() = 0;

    virtual bool Connect() = 0;
    virtual bool Disconnect() = 0;

    virtual bool TransmitPacket(NetPacket&& packet) = 0;
    virtual bool ReceivePacket(NetPacket& packet) = 0;
};

/* Initializes the process-global state of the underlying network library.
 *
 * Call once during game startup. The matching deinitializer is registered with atexit(), as the
 * game leaves through ResourceManager_Exit() -> exit() rather than by returning from main().
 *
 * A failure is not fatal to the process: only networked play becomes unavailable, which
 * Transport::Init() reports through Transport::GetError().
 */
bool Transport_Init() noexcept;

[[nodiscard]] bool Transport_IsInitialized() noexcept;

Transport* Transport_Create(int32_t type);

#endif /* TRANSPORT_HPP */
