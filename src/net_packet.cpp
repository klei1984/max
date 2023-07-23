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

#include "net_packet.hpp"

#include <SDL.h>

#include <algorithm>
#include <new>

void NetPacket::GrowBuffer(int32_t length) {
    while (buffer_write_position + length >= buffer_capacity) {
        buffer_capacity *= 2;
    }

    char* new_buffer = new (std::nothrow) char[buffer_capacity];
    memcpy(new_buffer, buffer, buffer_write_position);
    delete[] buffer;
    buffer = new_buffer;
}

void NetPacket::Read(void* address, int32_t length) {
    SDL_assert(address);
    SDL_assert(buffer);
    SDL_assert(length);
    SDL_assert(buffer_read_position + length <= buffer_write_position);

    memcpy(address, &buffer[buffer_read_position], length);
    buffer_read_position += length;
}

void NetPacket::Write(const void* address, int32_t length) {
    if (!buffer) {
        buffer = new (std::nothrow) char[100];
        buffer_capacity = 100;
    }

    if (buffer_write_position + length > buffer_capacity) {
        GrowBuffer(length);
    }

    memcpy(&buffer[buffer_write_position], address, length);
    buffer_write_position += length;
}

uint32_t NetPacket::Peek(uint32_t offset, void* data_address, uint32_t length) {
    uint32_t size;

    if (buffer) {
        uint32_t end_position;

        end_position = std::min(offset + length, buffer_write_position);
        if (offset >= end_position) {
            size = 0;
        } else {
            size = end_position - offset;

            memcpy(data_address, &buffer[offset], std::min(size, length));
        }

    } else {
        size = 0;
    }

    return size;
}

void NetPacket::Reset() {
    buffer_read_position = 0;
    buffer_write_position = 0;
    addresses.Clear();
}

NetPacket::NetPacket()
    : buffer(nullptr), buffer_capacity(0), buffer_read_position(0), buffer_write_position(0), addresses(5) {}

NetPacket::~NetPacket() { delete[] buffer; }

char* NetPacket::GetBuffer() const { return buffer; }

int32_t NetPacket::GetDataSize() const { return buffer_write_position; }

NetPacket& operator<<(NetPacket& packet, SmartString& string) {
    uint16_t length = string.GetLength() + sizeof('\0');
    packet.Write(&length, sizeof(length));
    packet.Write(string.GetCStr(), length);
    return packet;
}

NetPacket& operator<<(NetPacket& packet, const SmartString& string) {
    uint16_t length = string.GetLength() + sizeof('\0');
    packet.Write(&length, sizeof(length));
    packet.Write(string.GetCStr(), length);
    return packet;
}

void NetPacket::AddAddress(NetAddress& address) { addresses.PushBack(&address); }

NetAddress& NetPacket::GetAddress(uint16_t index) const { return *addresses[index]; }

uint16_t NetPacket::GetAddressCount() const { return addresses.GetCount(); }

void NetPacket::ClearAddressTable() { addresses.Clear(); }

NetPacket& operator>>(NetPacket& packet, SmartString& string) {
    uint16_t length;
    packet.Read(&length, sizeof(length));
    char* text_buffer = new (std::nothrow) char[length];
    packet.Read(text_buffer, length);
    string = text_buffer;
    delete[] text_buffer;
    return packet;
}

bool operator==(NetPacket& left, NetPacket& right) {
    return left.GetDataSize() == right.GetDataSize() &&
           !memcmp(left.GetBuffer(), right.GetBuffer(), left.GetDataSize());
}

bool operator!=(NetPacket& left, NetPacket& right) { return !(left == right); }
