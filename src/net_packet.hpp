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

#ifndef NET_PACKET_HPP
#define NET_PACKET_HPP

#include "net_address.hpp"
#include "smartobjectarray.hpp"
#include "smartstring.hpp"

class NetPacket {
    SmartObjectArray<NetAddress> addresses;
    char* buffer;
    uint32_t buffer_capacity;
    uint32_t buffer_read_position;
    uint32_t buffer_write_position;

    void GrowBuffer(int32_t length) noexcept;

public:
    NetPacket() noexcept;
    ~NetPacket() noexcept;
    NetPacket(NetPacket&& other) noexcept;
    NetPacket& operator=(NetPacket&& other) noexcept;
    void Read(void* address, int32_t length) noexcept;
    void Write(const void* address, int32_t length) noexcept;
    uint32_t Peek(uint32_t offset, void* address, uint32_t length) noexcept;
    void Reset() noexcept;
    [[nodiscard]] char* GetBuffer() const noexcept;
    [[nodiscard]] int32_t GetDataSize() const noexcept;

    void AddAddress(NetAddress& address) noexcept;
    [[nodiscard]] NetAddress& GetAddress(uint16_t index) const noexcept;
    [[nodiscard]] uint16_t GetAddressCount() const noexcept;
    void ClearAddressTable() noexcept;

    friend NetPacket& operator<<(NetPacket& packet, const SmartString& string) noexcept;
    friend NetPacket& operator<<(NetPacket& packet, SmartString& string) noexcept;
    friend NetPacket& operator>>(NetPacket& packet, SmartString& string) noexcept;

    friend NetPacket& operator<<(NetPacket& packet, const std::string& string) noexcept;
    friend NetPacket& operator<<(NetPacket& packet, std::string& string) noexcept;
    friend NetPacket& operator>>(NetPacket& packet, std::string& string) noexcept;

    friend NetPacket& operator<<(NetPacket& packet, const std::vector<std::string>& strings) noexcept;
    friend NetPacket& operator<<(NetPacket& packet, std::vector<std::string>& strings) noexcept;
    friend NetPacket& operator>>(NetPacket& packet, std::vector<std::string>& strings) noexcept;

    friend bool operator==(NetPacket& left, NetPacket& right) noexcept;
    friend bool operator!=(NetPacket& left, NetPacket& right) noexcept;

    template <typename T>
    friend NetPacket& operator<<(NetPacket& packet, const T& data) noexcept {
        packet.Write(&data, sizeof(T));
        return packet;
    }

    template <typename T>
    friend NetPacket& operator<<(NetPacket& packet, T& data) noexcept {
        packet.Write(&data, sizeof(T));
        return packet;
    }

    template <typename T>
    friend NetPacket& operator>>(NetPacket& packet, T& data) noexcept {
        packet.Read(&data, sizeof(T));
        return packet;
    }
};

#endif /* NET_PACKET_HPP */
