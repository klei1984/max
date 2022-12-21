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

enum {
    TRANSPORT_DEFAULT_UDP,
};

enum {
    TRANSPORT_SERVER,
    TRANSPORT_CLIENT,
};

class Transport {
public:
    virtual ~Transport(){};
    virtual const char* GetError() const = 0;
    virtual bool Init(int mode) = 0;
    virtual bool Deinit() = 0;
    virtual bool Connect() = 0;
    virtual bool Disconnect() = 0;
    virtual bool TransmitPacket(NetPacket& packet) = 0;
    virtual bool ReceivePacket(NetPacket& packet) = 0;
    virtual void SetSessionId(unsigned short session_id) = 0;
};

Transport* Transport_Create(int type);

#endif /* TRANSPORT_HPP */
