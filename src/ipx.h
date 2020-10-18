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

#ifndef IPX_H
#define IPX_H

#define IPX_MAX_PACKET_SIZE (sizeof(ipx_packet) - sizeof(ecb_header))
#define IPX_MAX_DATA_SIZE 550
#define IPX_META_DATA_SIZE (sizeof(packet_data) - IPX_MAX_DATA_SIZE)

typedef struct local_address {
    unsigned char address[6];
} local_address;

typedef struct net_address {
    unsigned char network_id[4];
    local_address node_id;
    unsigned short socket_id;
} net_address;

typedef struct ipx_header {
    unsigned short checksum;
    unsigned short length;
    unsigned char transport_control;
    unsigned char packet_type;
    net_address destination;
    net_address source;
} ipx_header;

typedef struct ecb_header {
    unsigned short link[2];
    unsigned short esr_address[2];
    unsigned char in_use;
    unsigned char completion_code;
    unsigned short socket_id;
    unsigned char ipx_reserved[14];
    unsigned short connection_id;
    local_address immediate_address;
    unsigned short fragment_count;
    unsigned short fragment_pointer[2];
    unsigned short fragment_size;
} ecb_header;

typedef struct __attribute__((packed)) packet_data {
    int packetnum;
    unsigned short field_4;
    unsigned short crc16;
    unsigned char data[IPX_MAX_DATA_SIZE];
} packet_data;

typedef struct ipx_packet {
    ecb_header ecb;
    ipx_header ipx;
    packet_data pd;
} ipx_packet;

int ipx_process_packets(unsigned char *data);
unsigned char *ipx_get_my_local_address(void);
unsigned char *ipx_get_my_server_address(void);
void ipx_send_packet(ecb_header *ecb);
void ipx_get_local_target(unsigned char *server, unsigned char *node, unsigned char *local_target);
void ipx_close(void);
int ipx_init(int socket_number);
void ipx_send_packet_data(unsigned char *data, int datasize, unsigned char *address, char unknown);
void ipx_send_broadcast_packet_data(unsigned char *data, int datasize);
void ipx_send_internetwork_packet_data(unsigned char *data, int datasize, unsigned char *server,
                                       unsigned char *address);

#endif /* IPX_H */
