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

    void GrowBuffer(int32_t length);

public:
    NetPacket();
    ~NetPacket();
    void Read(void* address, int32_t length);
    void Write(const void* address, int32_t length);
    uint32_t Peek(uint32_t offset, void* address, uint32_t length);
    void Reset();
    char* GetBuffer() const;
    int32_t GetDataSize() const;

    void AddAddress(NetAddress& address);
    NetAddress& GetAddress(uint16_t index) const;
    uint16_t GetAddressCount() const;
    void ClearAddressTable();

    friend NetPacket& operator<<(NetPacket& packet, const SmartString& string);
    friend NetPacket& operator<<(NetPacket& packet, SmartString& string);
    friend NetPacket& operator>>(NetPacket& packet, SmartString& string);

    friend bool operator==(NetPacket& left, NetPacket& right);
    friend bool operator!=(NetPacket& left, NetPacket& right);

    template <typename T>
    friend NetPacket& operator<<(NetPacket& packet, const T& data) {
        packet.Write(&data, sizeof(T));
        return packet;
    }

    template <typename T>
    friend NetPacket& operator<<(NetPacket& packet, T& data) {
        packet.Write(&data, sizeof(T));
        return packet;
    }

    template <typename T>
    friend NetPacket& operator>>(NetPacket& packet, T& data) {
        packet.Read(&data, sizeof(T));
        return packet;
    }
};

#endif /* NET_PACKET_HPP */
