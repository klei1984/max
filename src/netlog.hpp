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

#include <format>

#include "ailog.hpp"
#include "transport.hpp"

class NetLog {
    const char* m_file;
    const char* m_function;
    AiLog log;

    inline void ProcessLog(NetPacket& packet) noexcept {
        uint8_t packet_type;
        uint16_t entity_id;

        if (packet.Peek(0, &packet_type, sizeof(packet_type)) == sizeof(packet_type)) {
            if (packet_type < TRANSPORT_APPL_PACKET_ID) {
                log.Log(m_file, m_function, "Tp. packet (type {}).", packet_type);

            } else {
                if (packet.GetDataSize() >= static_cast<int32_t>(sizeof(packet_type) + sizeof(entity_id)) &&
                    packet.Peek(sizeof(packet_type), &entity_id, sizeof(entity_id)) == sizeof(entity_id)) {
                    log.Log(m_file, m_function, "Appl. packet (type {}, id {:4X}).",
                            packet_type - TRANSPORT_APPL_PACKET_ID, entity_id);
                }
            }
        }
    }

public:
    NetLog() noexcept = delete;

    template <typename... Args>
    NetLog(const char* file, const char* function, std::format_string<Args...> fmt, Args&&... args) noexcept
        : m_file(file), m_function(function), log(file, function, "Network Event") {
        auto message = std::format(fmt, std::forward<Args>(args)...);
        log.Log(file, function, "{}", message);
    }

    NetLog(const char* file, const char* function, NetPacket& packet) noexcept
        : m_file(file), m_function(function), log(file, function, "Network Packet") {
        ProcessLog(packet);
    }

    ~NetLog() noexcept {}

    template <typename... Args>
    inline void Log(std::format_string<Args...> fmt, Args&&... args) noexcept {
        auto message = std::format(fmt, std::forward<Args>(args)...);
        log.Log(m_file, m_function, "{}", message);
    }

    inline void Log(NetPacket& packet) noexcept { ProcessLog(packet); }
};

#if !defined(NDEBUG)
#define NETLOG(log, ...) NetLog log(__FILE__, __func__, __VA_ARGS__)
#define NETLOG_LOG(log, ...) (log).Log(__FILE__, __func__, __VA_ARGS__)
#else
#define NETLOG(log, ...)
#define NETLOG_LOG(log, ...)
#endif /* !defined(NDEBUG) */

inline void NetLog_Enable() noexcept { AiLog_Open(); }

#endif /* NETLOG_HPP */
