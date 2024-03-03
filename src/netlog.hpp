/* Copyright (c) 2024 M.A.X. Port Team
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

#ifndef NETLOG_HPP
#define NETLOG_HPP

#include "ailog.hpp"
#include "transport.hpp"

class NetLog {
    AiLog log;

    inline void ProcessLog(const char* format, va_list args) noexcept { log.VLog(format, args); }

    inline void ProcessLog(NetPacket& packet) noexcept {
        uint8_t packet_type;
        uint16_t entity_id;

        if (packet.Peek(0, &packet_type, sizeof(packet_type)) == sizeof(packet_type)) {
            if (packet_type < TRANSPORT_APPL_PACKET_ID) {
                log.Log("Tp. packet (type %i).", packet_type);

            } else {
                if (packet.GetDataSize() >= (sizeof(packet_type) + sizeof(entity_id)) &&
                    packet.Peek(sizeof(packet_type), &entity_id, sizeof(entity_id)) == sizeof(entity_id)) {
                    log.Log("Appl. packet (type %i, id %4X).", packet_type - TRANSPORT_APPL_PACKET_ID, entity_id);
                }
            }
        }
    }

public:
    NetLog() noexcept = delete;

    NetLog(const char* format, ...) noexcept : log("Network Event") {
        va_list args;

        va_start(args, format);
        ProcessLog(format, args);
        va_end(args);
    }

    NetLog(NetPacket& packet) noexcept : log("Network Packet") { ProcessLog(packet); }

    ~NetLog() noexcept {}

    inline void Log(const char* format, ...) noexcept {
        va_list args;

        va_start(args, format);
        ProcessLog(format, args);
        va_end(args);
    }

    inline void Log(NetPacket& packet) noexcept { ProcessLog(packet); }
};

inline void NetLog_Enable() noexcept {
    if (!AiLog_IsOpen()) {
        AiLog_Open();
    }
}

#endif /* NETLOG_HPP */
