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

#include "resrcmgr.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"

#ifdef __unix__
#include <linux/limits.h>
#endif

struct res_index {
    char tag[8];
    int data_offset;
    int data_size;
};

static_assert(sizeof(struct res_index) == 16, "The structure needs to be packed.");

struct res_header {
    char id[4];
    int offset;
    int size;
};

static_assert(sizeof(struct res_header) == 12, "The structure needs to be packed.");

struct __attribute__((packed)) GameResourceMeta_s {
    short res_file_item_index;
    char *resource_buffer;
    unsigned char res_file_id;
};

static_assert(sizeof(struct GameResourceMeta_s) == 7, "The structure needs to be packed.");

FILE *res_file_handle_array[2];
struct res_index *res_item_index_lut;
GameResourceMeta *game_resource_meta_ptr;
unsigned char res_file_count;
unsigned short res_item_count_in_lut;
int resource_buffer_size;
char *color_animation_buffer;

const unsigned char byte_16B4F0[8] = {0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F};
const unsigned char byte_16B4F8[8] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27};
const unsigned char byte_16B500[8] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37};
const unsigned char byte_16B508[8] = {0xFF, 0xA1, 0xAC, 0xA9, 0xD8, 0xD5, 0xD4, 0xCF};
const unsigned char byte_16B510[8] = {0xD8, 0xD7, 0xD6, 0xD5, 0xD4, 0xD3, 0xD2, 0xD1};

const char *resource_id_list[RESOURCE_E] = {RESOURCE_LIST_INIT};

GAME_RESOURCE res_get_resource_id(int index) {
    char buffer[9];

    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, res_item_index_lut[index].tag, sizeof(((struct res_index){0}).tag));
    for (GAME_RESOURCE i = 0; i < RESOURCE_E; ++i) {
        if (!strncmp(buffer, resource_id_list[i], sizeof(buffer))) {
            return i;
        }
    }

    return INVALID_ID;
}

int get_attribs_param(const char *string, unsigned short *offset) {
    int number;

    while (string[*offset] == ' ') {
        ++*offset;
    }
    number = atoi(&string[*offset]);

    while (string[*offset] != ' ' && string[*offset] != '\0') {
        ++*offset;
    }

    return number;
}

void *load_game_resource(GAME_RESOURCE id) {
    void *resource_buffer;

    if (id == INVALID_ID) {
        resource_buffer = NULL;
    } else {
        SDL_assert(id < MEM_END);

        if (game_resource_meta_ptr[id].res_file_item_index == -1) {
            resource_buffer = NULL;
        } else {
            if ((resource_buffer = game_resource_meta_ptr[id].resource_buffer) == NULL) {
                FILE *fp = res_file_handle_array[game_resource_meta_ptr[id].res_file_id];
                int data_size = res_item_index_lut[game_resource_meta_ptr[id].res_file_item_index].data_size;
                int data_offset = res_item_index_lut[game_resource_meta_ptr[id].res_file_item_index].data_offset;

                fseek(fp, data_offset, SEEK_SET);

                char *buffer = malloc(data_size);
                if (!buffer) {
                    fatal_error(3);
                }

                if (!fread(buffer, data_size, 1, fp)) {
                    fatal_error(7);
                }

                game_resource_meta_ptr[id].resource_buffer = buffer;
                resource_buffer = buffer;
                resource_buffer_size += data_size;
            }
        }
    }

    return resource_buffer;
}

void realloc_game_resource(GAME_RESOURCE id, char *buffer, int data_size) {
    if (game_resource_meta_ptr[id].resource_buffer) {
        free(game_resource_meta_ptr[id].resource_buffer);
        resource_buffer_size -= res_item_index_lut[game_resource_meta_ptr[id].res_file_item_index].data_size;
    }

    game_resource_meta_ptr[id].resource_buffer = buffer;
    resource_buffer_size += data_size;
}

void *read_game_resource(GAME_RESOURCE id) {
    void *resource_buffer;

    if (id == INVALID_ID) {
        resource_buffer = NULL;
    } else {
        SDL_assert(id > MEM_END);

        if (game_resource_meta_ptr[id].res_file_item_index == -1) {
            resource_buffer = NULL;
        } else {
            FILE *fp = res_file_handle_array[game_resource_meta_ptr[id].res_file_id];
            int data_size = res_item_index_lut[game_resource_meta_ptr[id].res_file_item_index].data_size;
            int data_offset = res_item_index_lut[game_resource_meta_ptr[id].res_file_item_index].data_offset;

            fseek(fp, data_offset, SEEK_SET);

            char *buffer = malloc(data_size + sizeof('\0'));
            if (!buffer) {
                fatal_error(3);
            }

            if (!fread(buffer, data_size, 1, fp)) {
                fatal_error(7);
            }

            buffer[data_size] = '\0';
            resource_buffer = buffer;
        }
    }

    return resource_buffer;
}

unsigned int get_resource_data_size(GAME_RESOURCE id) {
    unsigned int data_size;

    if (id == INVALID_ID || game_resource_meta_ptr[id].res_file_item_index == -1) {
        data_size = 0;
    } else {
        data_size = res_item_index_lut[game_resource_meta_ptr[id].res_file_item_index].data_size;
    }

    return data_size;
}

FILE *get_file_ptr_to_resource(GAME_RESOURCE id) {
    FILE *fp;

    if (id == INVALID_ID || game_resource_meta_ptr[id].res_file_item_index == -1) {
        fp = NULL;
    } else {
        fp = res_file_handle_array[game_resource_meta_ptr[id].res_file_id];
        int data_size = res_item_index_lut[game_resource_meta_ptr[id].res_file_item_index].data_size;
        int data_offset = res_item_index_lut[game_resource_meta_ptr[id].res_file_item_index].data_offset;

        fseek(fp, data_offset, SEEK_SET);
    }

    return fp;
}

int get_buffer_ptr_to_resource(GAME_RESOURCE id) {
    int data_offset;

    if (id == INVALID_ID || game_resource_meta_ptr[id].res_file_item_index == -1) {
        data_offset = 0;
    } else {
        data_offset = res_item_index_lut[game_resource_meta_ptr[id].res_file_item_index].data_offset;
    }

    return data_offset;
}

void deinit_mem_type_resources(void) {
    mouse_hide();

    for (short i = 0; i < MEM_END; ++i) {
        if (game_resource_meta_ptr[i].resource_buffer) {
            free(game_resource_meta_ptr[i].resource_buffer);
            game_resource_meta_ptr[i].resource_buffer = NULL;
        }
    }

    for (short j = 0; j < UNIT_END; ++j) {
        units2[j].sprite_ptr = NULL;
        units2[j].shadow_ptr = NULL;
        units2[j].unknown_5 = NULL;
    }

    resource_buffer_size = 0;
    init_mouse_gfx_luts();
    sub_C0D94(1);
    mouse_show();
}

int read_game_resource_into_buffer(GAME_RESOURCE id, void *buffer) {
    int result;

    if (id == INVALID_ID || game_resource_meta_ptr[id].res_file_item_index == -1) {
        result = 0;
    } else {
        FILE *fp = res_file_handle_array[game_resource_meta_ptr[id].res_file_id];
        int data_offset = res_item_index_lut[game_resource_meta_ptr[id].res_file_item_index].data_offset;

        fseek(fp, data_offset, SEEK_SET);

        if (!fread(buffer, 777, 1, fp)) {
            fatal_error(7);
        }

        result = 1;
    }

    return result;
}

int init_color_cycling_lut(void) {
    int result;

    color_animation_buffer = malloc(20 * 256 + 256);

    if (color_animation_buffer) {
        char *aligned_buffer = (char *)((((intptr_t)color_animation_buffer + 256) >> 8) << 8);

        dword_1770A0 = aligned_buffer + 0 * 256;
        dword_1770A4 = aligned_buffer + 1 * 256;
        dword_1770A8 = aligned_buffer + 2 * 256;
        dword_1770AC = aligned_buffer + 3 * 256;
        dword_1770B0 = aligned_buffer + 4 * 256;
        dword_1770B4 = aligned_buffer + 5 * 256;
        dword_1770B8 = aligned_buffer + 6 * 256;
        dword_1770BC = aligned_buffer + 7 * 256;
        dword_1770C4 = aligned_buffer + 8 * 256;
        dword_1770C0 = aligned_buffer + 9 * 256;
        dword_1770C8 = aligned_buffer + 10 * 256;
        dword_1770CC = aligned_buffer + 11 * 256;
        dword_17945C = aligned_buffer + 12 * 256;

        {
            char *buffer = aligned_buffer + 19 * 256;

            for (int i = 0; i < 256; ++i) {
                dword_1770B0[i] = i;
                dword_1770AC[i] = i;
                dword_1770A8[i] = i;
                dword_1770A4[i] = i;
                dword_1770A0[i] = i;
                buffer[i] = i;
            }
        }

        {
            int j = 0;
            int k = 32;

            while (j < 24) {
                if (j == 8) {
                    k += 8;
                }
                dword_1770A0[k] = byte_16B4F0[j & 7];
                dword_1770A4[k] = byte_16B4F8[j & 7];
                dword_1770A8[k] = byte_16B500[j & 7];
                dword_1770AC[k] = byte_16B508[j & 7];
                dword_1770B0[k] = byte_16B510[j & 7];
                k++;
                j++;
            }
        }

        word_1761B4 = 0x2000;
        dword_1761B6 = dword_1770A0;
        word_1764B0 = 0x1000;
        dword_1764B2 = dword_1770A4;
        word_1767AC = 0x800;
        dword_1767AE = dword_1770A8;
        word_176AA8 = 0x400;
        dword_176AAA = dword_1770AC;
        word_176DA4 = 0x8000;
        dword_176DA6 = dword_1770B0;

        result = 1;
    } else {
        result = 0;
    }

    return result;
}

int build_res_file_index_db(const char *file_path) {
    int result;
    FILE *fp;
    struct res_header header;

    fp = fopen(file_path, "rb");
    res_file_handle_array[res_file_count] = fp;

    if (fp) {
        if (fread(&header, sizeof(header), 1, fp)) {
            if (!strncmp("RES0", header.id, sizeof(((struct res_header){0}).id))) {
                if (res_item_index_lut) {
                    res_item_index_lut =
                        realloc(res_item_index_lut, header.size + res_item_count_in_lut * sizeof(struct res_index));
                } else {
                    res_item_index_lut = malloc(header.size);
                }

                if (res_item_index_lut) {
                    fseek(fp, header.offset, SEEK_SET);
                    if (fread(&res_item_index_lut[res_item_count_in_lut], header.size, 1, fp)) {
                        short new_item_count = res_item_count_in_lut + header.size / sizeof(struct res_index);

                        for (short i = res_item_count_in_lut; i < new_item_count; ++i) {
                            GAME_RESOURCE id = res_get_resource_id(i);
                            if (id != INVALID_ID && game_resource_meta_ptr[id].res_file_item_index == -1) {
                                game_resource_meta_ptr[id].res_file_item_index = i;
                                game_resource_meta_ptr[id].res_file_id = res_file_count;
                            }
                        }
                        res_item_count_in_lut = new_item_count;
                        ++res_file_count;
                        result = 0;
                    } else {
                        result = 7;
                    }
                } else {
                    result = 3;
                }
            } else {
                result = 8;
            }
        } else {
            result = 7;
        }
    } else {
        result = 6;
    }

    return result;
}

int init_game_resources(void) {
    char file_path[PATH_MAX];
    int result;

    game_resource_meta_ptr = (GameResourceMeta *)malloc(RESOURCE_E * sizeof(GameResourceMeta));

    if (game_resource_meta_ptr) {
        for (short i = 0; i < RESOURCE_E; i++) {
            game_resource_meta_ptr[i].resource_buffer = NULL;
            game_resource_meta_ptr[i].res_file_item_index = INVALID_ID;
        }
        res_item_count_in_lut = 0;

        strcpy(file_path, unknown_path1);
        strcat(file_path, "PATCHES.RES");

        result = build_res_file_index_db(file_path);

        if (result == 0 || result == 6) {
            strcpy(file_path, unknown_path2);
            strcat(file_path, "MAX.RES");

            result = build_res_file_index_db(file_path);

            if (result == 0) {
                dword_1770E8 = (char *)malloc(112 * 112);
                dword_1770E0 = (char *)malloc(112 * 112);
                dword_1770E4 = (char *)malloc(112 * 112);
                if (dword_1770E8 && dword_1770E0 && dword_1770E4) {
                    if (init_color_cycling_lut()) {
                        init_mouse_gfx_luts();
                        for (short j = 0; j < UNIT_END; ++j) {
                            unit2_init(&units2[j], &units[j]);
                        }
                        result = 0;
                    } else {
                        result = 3;
                    }
                } else {
                    result = 3;
                }
            }
        }
    } else {
        result = 3;
    }

    return result;
}

unsigned int sub_A697A(void *unknown, GAME_RESOURCE id) {
    unsigned int unknown2;
    unsigned int result;

    if (*(int *)((char *)unknown + 132)) {
        if (sub_C36FA(unknown, "Unit Volumes") && (game_resource_meta_ptr[id].res_file_item_index != INVALID_ID) &&
            sub_C4113(unknown, resource_id_list[id], &unknown2)) {
            result = 0x7FFF * unknown2 / 100;
        } else {
            result = 0x7FFF;
        }
    } else {
        result = 0x7FFF;
    }

    return result;
}
