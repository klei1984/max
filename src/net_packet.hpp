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
    unsigned int buffer_capacity;
    unsigned int buffer_read_position;
    unsigned int buffer_write_position;

    void GrowBuffer(int length);

public:
    NetPacket();
    ~NetPacket();
    void Read(void* address, int length);
    void Write(const void* address, int length);
    unsigned int Peek(unsigned int offset, void* address, unsigned int length);
    void Reset();
    char* GetBuffer() const;
    int GetDataSize() const;

    void AddAddress(NetAddress& address);
    NetAddress& GetAddress(unsigned short index) const;
    unsigned short GetAddressCount() const;
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
