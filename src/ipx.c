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

#include <SDL_net.h>

#ifdef __unix__
#define _XOPEN_SOURCE
#include <unistd.h>
#endif

#include "game.h"

static_assert(sizeof(struct ipx_packet) == 630, "The structure needs to be packed.");

#define IPX_STREAM_BUFFER_SIZE 1024

static_assert(sizeof(intptr_t) == 4, "The type needs to be 32 bit wide.");

#define DPMI_real_segment(address) ((((intptr_t)(address)) >> 4) & 0xFFFF)
#define DPMI_real_offset(address) (((intptr_t)(address)) & 0xF)

static void ipx_listen_for_packet(ecb_header *ecb);
static int ipx_get_player_index(local_address *immediate_address);
static int ipx_got_new_ipx_packet(ecb_header *ecb);
static unsigned short ipx_get_crc16(unsigned char *data, unsigned short datasize, unsigned char packetnum);
static void ipx_process_packet(ecb_header *ecb);

static net_address ipx_address;
static ipx_packet *packets;
static unsigned int neterrors;

static unsigned short ipx_word_174DB6;

static int ipx_is_local_network;

static unsigned short ipx_socket;

UDPsocket udp_socket;

static unsigned char ipx_installed;
static unsigned char ipx_buffers_initialized;

local_address address_table[4];
unsigned char ipx_byte_1740B0[4];
unsigned char ipx_packetnum[4];
unsigned char ipx_byte_1740B8[4];

unsigned short ipx_packet_field4;

unsigned char byte_175630;
unsigned char byte_1737DA;
unsigned char byte_1759B8;

unsigned short ipx_crc;

static unsigned int ipx_dword_174130[4];
static unsigned char *ipx_buffer_lut_1[4];
static unsigned char *ipx_buffer_lut_2[4];
static unsigned int ipx_dword_174160[4];
static unsigned short ipx_word_174170[4][64];
static unsigned int ipx_dword_174370[4][64];
static unsigned int ipx_dword_174770[4];
static unsigned short ipx_word_174780[4][64];
static unsigned int ipx_dword_174980[4][64];
static unsigned char ipx_byte_174D80[4];
static int ipx_tx_time_stamp_list[4];

static unsigned char ipx_packet_data[5] = {0xFF, 0x00, 0x00, 0x00, 0x00};
static unsigned char ipx_broadcast_address[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static enum GAME_INI_e ini_player_team_lut[4] = {ini_red_team_player, ini_green_team_player, ini_blue_team_player,
                                                 ini_gray_team_player};

void ipx_listen_for_packet(ecb_header *ecb) {}

int ipx_get_player_index(local_address *immediate_address) {
    for (int i = 0; i < 4; i++) {
        if (!memcmp(immediate_address->address, address_table[i].address, sizeof(local_address))) {
            return i;
        }
    }

    return -1;
}

int ipx_got_new_ipx_packet(ecb_header *ecb) {
    ipx_packet *p;
    int result;

    p = (ipx_packet *)ecb;

    if (memcmp(&p->ipx.source.node_id.address, ipx_address.node_id.address, sizeof(local_address)) && byte_175630) {
        result = p->pd.field_4 == ipx_packet_field4;
    } else {
        result = 0;
    }

    return result;
}

unsigned short ipx_get_crc16(unsigned char *data, unsigned short datasize, unsigned char packetnum) {
    int i;

    ipx_crc = 0;

    for (i = 0; i < datasize; i++) {
        crc16(&ipx_crc, data[i]);
    }

    crc16(&ipx_crc, packetnum);

    return ipx_crc;
}

void ipx_verify_packet(unsigned short player_index, ecb_header *ecb, char skip_crc) {
    ipx_packet *p;
    short length;
    unsigned char packetnum;
    short data_size;
    char *data_pointer;
    short buffer_position;

    p = (ipx_packet *)ecb;

    length = SDL_SwapBE16(p->ipx.length) - sizeof(ipx_header);

    if ((length > 0) && (length <= IPX_MAX_DATA_SIZE)) {
        data_pointer = p->pd.data;
        buffer_position = ipx_dword_174130[player_index];

        packetnum = p->pd.packetnum % 64;

        data_size = length - IPX_META_DATA_SIZE;
        SDL_assert(data_size < 0);

        if (skip_crc || ((data_size > 0) && ipx_get_crc16(data_pointer, data_size, packetnum) == p->pd.crc16)) {
            ipx_dword_174160[player_index]++;
            ipx_word_174170[player_index][packetnum] = data_size;
            ipx_dword_174370[player_index][packetnum] = buffer_position;

            SDL_assert(ipx_buffer_lut_1[player_index]);

            if ((buffer_position + length) > IPX_STREAM_BUFFER_SIZE) {
                memcpy(&ipx_buffer_lut_1[player_index][buffer_position], data_pointer,
                       IPX_STREAM_BUFFER_SIZE - buffer_position);
                data_pointer += IPX_STREAM_BUFFER_SIZE - buffer_position;
                length -= IPX_STREAM_BUFFER_SIZE - buffer_position;
                buffer_position = 0;
            }

            memcpy(&ipx_buffer_lut_1[player_index][buffer_position], data_pointer, length);
            ipx_dword_174130[player_index] = buffer_position + length;
        }
    } else {
        neterrors++;
    }
}

void ipx_process_packet(ecb_header *ecb) {
    ipx_packet *p;
    int player_index;
    int buffer_position;
    int data_length;
    int datasize;
    unsigned char buffer[IPX_MAX_DATA_SIZE];
    unsigned char packetnum;
    unsigned char new_packetnum;

    p = (ipx_packet *)ecb;

    if (p->ecb.in_use) {
        neterrors++;
    } else if (p->ecb.completion_code) {
        neterrors++;
    } else {
        if (ipx_got_new_ipx_packet(ecb)) {
            player_index = ipx_get_player_index(&p->ipx.source.node_id);

            if ((byte_175630 != 1) && (player_index != -1)) {
                if (p->pd.data[0] == 0xFF) {
                    new_packetnum = *(unsigned short *)&p->pd.data[1];
                    datasize = ipx_word_174780[player_index][new_packetnum];
                    data_length = 0;
                    buffer_position = ipx_dword_174980[player_index][new_packetnum];
                    packetnum = ipx_packetnum[player_index];

                    if ((buffer_position + datasize) > IPX_STREAM_BUFFER_SIZE) {
                        data_length = IPX_STREAM_BUFFER_SIZE - buffer_position;

                        memcpy(buffer, &ipx_buffer_lut_2[player_index][buffer_position],
                               IPX_STREAM_BUFFER_SIZE - buffer_position);

                        buffer_position = 0;
                        datasize -= data_length;
                    }

                    memcpy(&buffer[data_length], &ipx_buffer_lut_2[player_index][buffer_position], datasize);

                    ipx_packetnum[player_index] = new_packetnum;

                    ipx_send_packet_data(buffer, ipx_word_174780[player_index][new_packetnum],
                                         address_table[player_index].address, 1);

                    ipx_packetnum[player_index] = packetnum;

                    ipx_verify_packet(player_index, ecb, 0);
                } else {
                    ipx_verify_packet(player_index, ecb, 0);
                }

            } else {
                player_index = byte_1737DA;
                p->pd.packetnum = ipx_byte_1740B8[player_index];

                ipx_verify_packet(player_index, ecb, 1);

                ipx_byte_1740B8[player_index]++;
                ipx_byte_1740B8[player_index] = ipx_byte_1740B8[player_index] % 64;
            }
        }

        p->ecb.in_use = 0;
    }
}

int ipx_process_packets(unsigned char *data) {
    int i;
    int player_index;
    unsigned int buffer_position;
    unsigned int data_length;

    SDL_assert(data);

    for (i = 4; i < 32; i++) {
        if (!packets[i].ecb.in_use) {
            ipx_process_packet(&packets[i].ecb);
            packets[i].ecb.in_use = 0;

            ipx_listen_for_packet(&packets[i].ecb);
        }
    }

    player_index = 4;
    while (1) {
        do {
            if (--player_index == -1) {
                return 0;
            }
        } while (!ipx_dword_174160[player_index]);

        if (ipx_word_174170[player_index][ipx_byte_1740B0[player_index]]) {
            break;
        }

        if (!ipx_byte_174D80[player_index]) {
            ipx_byte_174D80[player_index] = 1;

            *(unsigned short *)&ipx_packet_data[1] = ipx_byte_1740B0[player_index];

            ipx_send_packet_data(ipx_packet_data, 5, address_table[player_index].address, 0);

            ipx_tx_time_stamp_list[player_index] = timer_get_stamp32();
        }
    }

    buffer_position = ipx_dword_174370[player_index][ipx_byte_1740B0[player_index]];
    data_length = ipx_word_174170[player_index][ipx_byte_1740B0[player_index]];

    if ((buffer_position + data_length) <= IPX_STREAM_BUFFER_SIZE) {
        memcpy(data, &ipx_buffer_lut_1[player_index][buffer_position], data_length);
    } else {
        memcpy(data, &ipx_buffer_lut_1[player_index][buffer_position], IPX_STREAM_BUFFER_SIZE - buffer_position);
        memcpy(&data[IPX_STREAM_BUFFER_SIZE - buffer_position], ipx_buffer_lut_1[player_index],
               data_length - (IPX_STREAM_BUFFER_SIZE - buffer_position));
    }

    ipx_word_174170[player_index][ipx_byte_1740B0[player_index]] = 0;
    ipx_dword_174370[player_index][ipx_byte_1740B0[player_index]] = 0;

    ipx_dword_174160[player_index]--;

    ipx_byte_1740B0[player_index]++;
    ipx_byte_1740B0[player_index] %= 64;

    ipx_byte_174D80[player_index] = 0;

    if (byte_1759B8 && players[player_index].unknown[40] != 3) {
        players[player_index].unknown[40] = ini_get_setting(ini_player_team_lut[player_index]);
    }

    return data_length;
}

unsigned char *ipx_get_my_local_address(void) { return ipx_address.node_id.address; }

unsigned char *ipx_get_my_server_address(void) { return ipx_address.network_id; }

void ipx_send_packet(ecb_header *ecb) {
    ipx_packet *p;
    UDPpacket *packet;

    p = (ipx_packet *)ecb;

    p->pd.crc16 =
        ipx_get_crc16(p->pd.data, p->ecb.fragment_size - sizeof(ipx_header) - IPX_META_DATA_SIZE, p->pd.packetnum);

    SDL_assert(udp_socket);

    packet = SDLNet_AllocPacket(p->ecb.fragment_size);

    SDL_assert(packet);

    memcpy(packet->data, p, p->ecb.fragment_size);
    packet->len = p->ecb.fragment_size;

    if (!SDLNet_UDP_Send(udp_socket, -1, packet)) {
        SDL_Log("Unable to send packet: %s\n", SDLNet_GetError());
    }

    SDLNet_FreePacket(packet);
}

void ipx_get_local_target(unsigned char *server, unsigned char *node, unsigned char *local_target) {
    SDL_assert(0 /** \todo implement unused code */);
}

void ipx_close(void) {
    if (ipx_installed) {
        ipx_installed = 0;

        SDLNet_Quit();
    }

    if (packets) {
        free(packets);
        packets = NULL;
    }

    if (ipx_buffers_initialized) {
        for (int index = 0; index < 4; index++) {
            free(ipx_buffer_lut_1[index]);
            ipx_buffer_lut_1[index] = NULL;

            free(ipx_buffer_lut_2[index]);
            ipx_buffer_lut_2[index] = NULL;
        }

        ipx_buffers_initialized = 0;
    }
}

int ipx_init(int socket_number) {
    if (SDLNet_Init()) {
        SDL_Log("Unable to initialize SDL_net: %s\n", SDLNet_GetError());
        return -3;
    }

    if (atexit(ipx_close)) {
        return -6;
    }

    swab((char *)&socket_number, (char *)&ipx_socket, sizeof(ipx_socket));

    udp_socket = SDLNet_UDP_Open(socket_number);

    if (!udp_socket) {
        SDL_Log("Unable to initialize SDL_net UDP socket: %s\n", SDLNet_GetError());
        return -2;
    }

    ipx_installed = 1;

    memset(&ipx_address, 0, sizeof(net_address));

    for (int i = 0; i < 4; i++) {
        memset(address_table[i].address, 0, sizeof(local_address));

        ipx_byte_1740B0[i] = 0;
        ipx_packetnum[i] = 0;
        ipx_byte_1740B8[i] = 0;
        ipx_dword_174130[i] = 0;
        ipx_dword_174770[i] = 0;
        ipx_dword_174160[i] = 0;
        memset(ipx_dword_174370[i], 0, sizeof(unsigned int) * 64);
        ipx_byte_174D80[i] = 0;

        ipx_buffer_lut_1[i] = (unsigned char *)malloc(IPX_STREAM_BUFFER_SIZE);
        ipx_buffer_lut_2[i] = (unsigned char *)malloc(IPX_STREAM_BUFFER_SIZE);

        SDL_assert(ipx_buffer_lut_1[i] && ipx_buffer_lut_2[i]);

        ipx_buffers_initialized = 1;
    }

    packets = (ipx_packet *)malloc(32 * sizeof(ipx_packet));

    if (!packets) {
        return -4;
    }

    for (int i = 0; i < 4; i++) {
        memset(&packets[i], 0, sizeof(ipx_packet));

        packets[i].ecb.socket_id = ipx_socket;
        packets[i].ecb.fragment_count = 1;
        packets[i].ecb.fragment_pointer[0] = DPMI_real_offset(&packets[i].ipx);
        packets[i].ecb.fragment_pointer[1] = DPMI_real_segment(&packets[i].ipx);
        packets[i].ipx.packet_type = 4;
        packets[i].ipx.destination.socket_id = ipx_socket;

        memcpy(&packets[i].ipx.destination.network_id, ipx_address.network_id, sizeof(((net_address *)0)->network_id));
    }

    ipx_word_174DB6 = 0;

    for (int i = 4; i < 32; i++) {
        memset(&packets[i], 0, sizeof(ipx_packet));

        packets[i].ecb.in_use = 0;
        packets[i].ecb.socket_id = ipx_socket;
        packets[i].ecb.fragment_count = 1;
        packets[i].ecb.fragment_pointer[0] = DPMI_real_offset(&packets[i].ipx);
        packets[i].ecb.fragment_pointer[1] = DPMI_real_segment(&packets[i].ipx);
        packets[i].ecb.fragment_size = IPX_MAX_PACKET_SIZE;

        ipx_listen_for_packet(&packets[i].ecb);
    }

    return 0;
}

void ipx_send_packet_data(unsigned char *data, int datasize, unsigned char *address, char unknown) {
    int player_index;
    unsigned int buffer_position;

    player_index = ipx_get_player_index((local_address *)address);

    SDL_assert(datasize < IPX_MAX_DATA_SIZE);

    // wait for ipx packet slot to be free
    // exit if packet was not sent successfully

    memcpy(&packets[ipx_word_174DB6].ipx.destination.node_id, address, sizeof(local_address));
    memcpy(&packets[ipx_word_174DB6].ecb.immediate_address, address, sizeof(local_address));

    packets[ipx_word_174DB6].pd.field_4 = ipx_packet_field4;
    packets[ipx_word_174DB6].ecb.fragment_size = datasize + sizeof(ipx_header) + IPX_META_DATA_SIZE;
    memcpy(packets[ipx_word_174DB6].pd.data, data, datasize);

    if (player_index < 0) {
        packets[ipx_word_174DB6].pd.packetnum = 0;
    } else {
        packets[ipx_word_174DB6].pd.packetnum = ipx_packetnum[player_index];
    }

    ipx_send_packet(&packets[ipx_word_174DB6].ecb);

    ipx_word_174DB6 = (ipx_word_174DB6 + 1) % 4;

    if (!unknown && player_index != -1) {
        buffer_position = ipx_dword_174770[player_index];
        ipx_word_174780[player_index][ipx_packetnum[player_index]] = datasize;
        ipx_dword_174980[player_index][ipx_packetnum[player_index]] = ipx_dword_174770[player_index];

        if (datasize + buffer_position > IPX_STREAM_BUFFER_SIZE) {
            memcpy(&ipx_buffer_lut_2[player_index][buffer_position], data, IPX_STREAM_BUFFER_SIZE - buffer_position);
            data = &data[IPX_STREAM_BUFFER_SIZE - buffer_position];
            datasize -= IPX_STREAM_BUFFER_SIZE - buffer_position;
            buffer_position = 0;
        }

        memcpy(&ipx_buffer_lut_2[player_index][buffer_position], data, datasize);
        ipx_dword_174770[player_index] = datasize + buffer_position;
        ipx_packetnum[player_index] = (ipx_packetnum[player_index] + 1) % 64;
    }
}

void ipx_send_broadcast_packet_data(unsigned char *data, int datasize) {
    if (ipx_is_local_network) {
        unsigned char local_address[6];

        memcpy(local_address, ipx_broadcast_address, sizeof(ipx_broadcast_address));
        ipx_send_packet_data(data, datasize, local_address, 0);

    } else {
        // ipx_send_internetwork_packet_data(data, datasize, &server, &address);
    }
}

void ipx_send_internetwork_packet_data(unsigned char *data, int datasize, unsigned char *server,
                                       unsigned char *address) {
    unsigned char local_address[6];

    if ((*(unsigned int *)server) != 0) {
        ipx_get_local_target(server, address, local_address);
        ipx_send_packet_data(data, datasize, local_address, 0);
    } else {
        ipx_send_packet_data(data, datasize, address, 0);
    }
}
