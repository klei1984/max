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

static constexpr int32_t NetPacket_DefaultPacketSize = 100;

void NetPacket::GrowBuffer(int32_t length) noexcept {
    while (buffer_write_position + length >= buffer_capacity) {
        buffer_capacity *= 2;
    }

    char* new_buffer = new (std::nothrow) char[buffer_capacity];
    memcpy(new_buffer, buffer, buffer_write_position);
    delete[] buffer;
    buffer = new_buffer;
}

void NetPacket::Read(void* address, int32_t length) noexcept {
    SDL_assert(address);
    SDL_assert(buffer);
    SDL_assert(length > 0);
    SDL_assert(buffer_read_position + length <= buffer_write_position);

    memcpy(address, &buffer[buffer_read_position], length);
    buffer_read_position += length;
}

void NetPacket::Write(const void* address, int32_t length) noexcept {
    if (!buffer) {
        buffer = new (std::nothrow) char[NetPacket_DefaultPacketSize];
        buffer_capacity = NetPacket_DefaultPacketSize;
    }

    if (buffer_write_position + length > buffer_capacity) {
        GrowBuffer(length);
    }

    memcpy(&buffer[buffer_write_position], address, length);
    buffer_write_position += length;
}

uint32_t NetPacket::Peek(uint32_t offset, void* data_address, uint32_t length) noexcept {
    uint32_t size;

    if (buffer) {
        const uint32_t end_position = std::min(buffer_read_position + offset + length, buffer_write_position);
        const uint32_t start_position = buffer_read_position + offset;

        if (start_position >= end_position) {
            size = 0;
        } else {
            size = end_position - start_position;

            memcpy(data_address, &buffer[start_position], std::min(size, length));
        }

    } else {
        size = 0;
    }

    return size;
}

void NetPacket::Reset() noexcept {
    buffer_read_position = 0;
    buffer_write_position = 0;
    addresses.Clear();
}

NetPacket::NetPacket() noexcept
    : buffer(nullptr), buffer_capacity(0), buffer_read_position(0), buffer_write_position(0), addresses(5) {}

NetPacket::~NetPacket() noexcept { delete[] buffer; }

[[nodiscard]] char* NetPacket::GetBuffer() const noexcept { return &buffer[buffer_read_position]; }

[[nodiscard]] int32_t NetPacket::GetDataSize() const noexcept { return buffer_write_position - buffer_read_position; }

NetPacket& operator<<(NetPacket& packet, SmartString& string) noexcept {
    uint16_t length = string.GetLength() + sizeof('\0');
    packet.Write(&length, sizeof(length));
    packet.Write(string.GetCStr(), length);
    return packet;
}

NetPacket& operator<<(NetPacket& packet, const SmartString& string) noexcept {
    uint16_t length = string.GetLength() + sizeof('\0');
    packet.Write(&length, sizeof(length));
    packet.Write(string.GetCStr(), length);
    return packet;
}

NetPacket& operator<<(NetPacket& packet, std::string& string) noexcept {
    uint32_t length = string.size();
    packet.Write(&length, sizeof(length));
    packet.Write(string.c_str(), length);
    return packet;
}

NetPacket& operator<<(NetPacket& packet, const std::string& string) noexcept {
    uint32_t length = string.size();
    packet.Write(&length, sizeof(length));
    packet.Write(string.c_str(), length);
    return packet;
}

void NetPacket::AddAddress(NetAddress& address) noexcept { addresses.PushBack(&address); }

[[nodiscard]] NetAddress& NetPacket::GetAddress(uint16_t index) const noexcept { return *addresses[index]; }

[[nodiscard]] uint16_t NetPacket::GetAddressCount() const noexcept { return addresses.GetCount(); }

void NetPacket::ClearAddressTable() noexcept { addresses.Clear(); }

NetPacket& operator>>(NetPacket& packet, SmartString& string) noexcept {
    uint16_t length;
    packet.Read(&length, sizeof(length));
    char* text_buffer = new (std::nothrow) char[length];
    packet.Read(text_buffer, length);
    string = text_buffer;
    delete[] text_buffer;
    return packet;
}

NetPacket& operator>>(NetPacket& packet, std::string& string) noexcept {
    uint32_t length;
    packet.Read(&length, sizeof(length));
    if (length) {
        char* text_buffer = new (std::nothrow) char[length];
        packet.Read(text_buffer, length);
        string.assign(text_buffer, length);
        delete[] text_buffer;

    } else {
        string.clear();
    }
    return packet;
}

NetPacket& operator<<(NetPacket& packet, const std::vector<std::string>& strings) noexcept {
    uint32_t count = strings.size();
    packet << count;
    for (const auto& string : strings) {
        packet << string;
    }
    return packet;
}

NetPacket& operator<<(NetPacket& packet, std::vector<std::string>& strings) noexcept {
    uint32_t count = strings.size();
    packet << count;
    for (const auto& string : strings) {
        packet << string;
    }
    return packet;
}

NetPacket& operator>>(NetPacket& packet, std::vector<std::string>& strings) noexcept {
    uint32_t count;
    strings.clear();
    packet >> count;
    for (uint32_t i = 0; i < count; ++i) {
        std::string string;
        packet >> string;
        strings.push_back(string);
    }
    return packet;
}

bool operator==(NetPacket& left, NetPacket& right) noexcept {
    return left.GetDataSize() == right.GetDataSize() &&
           !memcmp(left.GetBuffer(), right.GetBuffer(), left.GetDataSize());
}

NetPacket::NetPacket(NetPacket&& other) noexcept
    : addresses(other.addresses, true),
      buffer(other.buffer),
      buffer_capacity(other.buffer_capacity),
      buffer_read_position(other.buffer_read_position),
      buffer_write_position(other.buffer_write_position) {
    other.Reset();
    other.buffer = nullptr;
    other.buffer_capacity = 0;
}

NetPacket& NetPacket::operator=(NetPacket&& other) noexcept {
    addresses = SmartObjectArray<NetAddress>(other.addresses, true);
    delete[] buffer;
    buffer = other.buffer;
    buffer_capacity = other.buffer_capacity;
    buffer_read_position = other.buffer_read_position;
    buffer_write_position = other.buffer_write_position;

    other.Reset();
    other.buffer = nullptr;
    other.buffer_capacity = 0;

    return *this;
}

bool operator!=(NetPacket& left, NetPacket& right) noexcept { return !(left == right); }
