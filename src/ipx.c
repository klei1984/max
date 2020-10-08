/* Copyright (c) 2020 M.A.X. Port Team
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

#include "ipx.h"

#ifdef __unix__
#define _XOPEN_SOURCE
#include <unistd.h>
#endif

#include "game.h"

static_assert(sizeof(struct ipx_packet) == 630, "The structure needs to be packed.");

unsigned int ipx_network;
local_address ipx_my_node;
unsigned short ipx_socket;
unsigned char ipx_socket_life;
unsigned char ipx_installed;

void ipx_listen_for_packet(ecb_header *ecb) {}

unsigned char *ipx_get_my_local_address(void) { return ipx_my_node.address; }

unsigned char *ipx_get_my_server_address(void) { return (unsigned char *)&ipx_network; }

void ipx_send_packet(ecb_header *ecb) {}

void ipx_get_local_target(unsigned char *server, unsigned char *node, unsigned char *local_target) {}

void ipx_close(void) {}

int ipx_init(int socket_number) {
    if (atexit(ipx_close)) {
        return -6;
    }

    swab((char *)&socket_number, (char *)&ipx_socket, sizeof(ipx_socket));
    ipx_socket_life = 0;

    return 0;
}

void ipx_send_packet_data(unsigned char *data, int datasize, unsigned char *address, char unknown) {}

void ipx_send_broadcast_packet_data(unsigned char *data, int datasize) {}

void ipx_send_internetwork_packet_data(unsigned char *data, int datasize, unsigned char *server,
                                       unsigned char *address) {}
