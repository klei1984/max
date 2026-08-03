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

#include <array>
#include <string>
#include <vector>

#include "net_address.hpp"
#include "smartstring.hpp"

class NetPacket {
    std::array<NetAddress, NetAddress_MaximumCount> addresses;
    uint16_t address_count;
    char* buffer;
    uint32_t buffer_capacity;
    uint32_t buffer_read_position;
    uint32_t buffer_write_position;
    bool is_malformed;

    void GrowBuffer(int32_t length) noexcept;

public:
    /* Upper bounds for values that arrive from the network and drive an allocation or a loop.
     *
     * The real protection is CanRead(), which limits every field to the bytes actually received.
     * These add a second ceiling so a single packet cannot ask for an allocation far larger than
     * anything the protocol has a use for.
     */
    static constexpr uint32_t MaximumStringLength = 4096;
    static constexpr uint32_t MaximumArrayCount = 1024;

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

    /* A packet is malformed once any read has run past the received data, any length or count has
     * exceeded its bound, or an allocation has failed. The flag is sticky: a handler may keep
     * parsing without checking after every field, and the dispatcher tests it once at the end.
     */
    [[nodiscard]] bool IsMalformed() const noexcept;
    void SetMalformed() noexcept;
    [[nodiscard]] uint32_t GetRemainingSize() const noexcept;
    [[nodiscard]] bool CanRead(uint32_t length) const noexcept;

    void AddAddress(NetAddress& address) noexcept;
    [[nodiscard]] NetAddress& GetAddress(uint16_t index) noexcept;
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
