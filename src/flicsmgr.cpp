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

#include "flicsmgr.hpp"

#include <SDL3/SDL.h>

#include "resource_manager.hpp"

#define FLICSMGR_FLI_TYPE 0xAF11u
#define FLICSMGR_FLC_TYPE 0xAF12u
#define FLICSMGR_FRAME_TYPE 0xF1FAu

#define FLICSMGR_FRAME_BUFFER 5

enum ChunkTypes { COLOR_256 = 4, DELTA_FLC = 7, COLOR_64 = 11, BLACK = 13, BYTE_RUN = 15, LITERAL = 16 };

typedef struct __attribute__((packed)) {
    int32_t size;
    uint16_t type;
    uint16_t frames;
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint16_t flags;
    int32_t speed;
    int16_t reserved1;
    uint32_t created;
    uint32_t creator;
    uint32_t updated;
    uint32_t updater;
    uint16_t aspect_dx;
    uint16_t aspect_dy;
    char reserved2[38];
    int32_t oframe1;
    int32_t oframe2;
    char reserved3[40];
} FlicHeader;

static_assert(sizeof(FlicHeader) == 128, "FLIC header is 128 bytes");

typedef struct {
    int32_t size;
    uint16_t type;
    int16_t chunks;
    char reserved[8];
} FrameHeader;

static_assert(sizeof(FrameHeader) == 16, "FLIC frame header is 16 bytes");

typedef struct __attribute__((packed)) {
    int32_t size;
    uint16_t type;
} FlicChunkHeader;

static_assert(sizeof(FlicChunkHeader) == 6, "FLIC chunk header is 6 bytes");

typedef struct {
    uint32_t size;
    uint16_t type;
    uint16_t chunks;
    char* buffer;
} FlicFrame;

struct Flic {
    FILE* fp;
    int16_t buffer_start_frame;
    FlicFrame frames[FLICSMGR_FRAME_BUFFER];
    int32_t file_pos;
    WinID wid;
    uint8_t* buffer;
    Rect bound;
    char animate;
    int16_t frame_count;
    int16_t width;
    int16_t height;
    int16_t frame_pos;
    int16_t full;
    int32_t speed;
    char load_flic_palette;
};

static void flicsmgr_decode_delta_flc(uint8_t* buffer, Flic* flc);
static void flicsmgr_decode_color(uint8_t* buffer, int32_t shift);
static void flicsmgr_decode_byte_run(uint8_t* buffer, Flic* flc);
static void flicsmgr_decode_frame(FlicFrame* frame, Flic* flc);
static char Flicsmgr_load_frame(FILE* file, FlicFrame* frame);
static char flicsmgr_fill_frame_buffer(Flic* flc);
static char flicsmgr_read(Flic* flc);
static char flicsmgr_load(ResourceID id, Flic* flc);

void flicsmgr_decode_delta_flc(uint8_t* buffer, Flic* flc) {
    int16_t opt_word;
    int16_t packet_count;
    uint8_t column_skip_count;
    int8_t packet_type;
    uint8_t* line_address;
    int16_t line_count;
    int32_t offset;

    line_count = *(int16_t*)buffer;
    buffer += sizeof(int16_t);

    line_address = flc->buffer;
    while (--line_count >= 0) {
        opt_word = *(int16_t*)buffer;
        buffer += sizeof(int16_t);

        /* process optional skip count words and last byte word */
        while ((uint16_t)opt_word & 0x8000) {
            if ((uint16_t)opt_word & 0x4000) {
                /* absolute value of optional word holds line skip count */
                opt_word = -opt_word;
                line_address += opt_word * flc->full;
            } else {
                /* low order byte of optional word holds last byte of current line */
                line_address[flc->width - 1] = opt_word;
            }

            opt_word = *(int16_t*)buffer;
            buffer += sizeof(int16_t);
        }

        packet_count = opt_word;
        offset = 0;

        while (--packet_count >= 0) {
            column_skip_count = buffer[0];
            packet_type = buffer[1];

            buffer += sizeof(int16_t);

            offset += column_skip_count;

            if (packet_type >= 0) {
                /* copy pocket type count of words */
                memcpy(&line_address[offset], buffer, packet_type * sizeof(int16_t));
                buffer += packet_type * sizeof(int16_t);
                offset += packet_type * sizeof(int16_t);
            } else {
                /* replicate absolute value of the pocket type count of words
                 * the word to replicate is the next word in the buffer
                 */
                int16_t word = *(int16_t*)buffer;
                buffer += sizeof(int16_t);

                packet_type = -packet_type;
                while (--packet_type >= 0) {
                    *(int16_t*)(&line_address[offset]) = word;
                    offset += sizeof(int16_t);
                }
            }
        }

        line_address += flc->full;
    }
}

void flicsmgr_decode_color(uint8_t* buffer, int32_t shift) {
    int32_t entry;
    uint8_t* cbuf;
    int16_t* wp;
    int16_t ops;
    int32_t count;

    entry = 0;
    cbuf = buffer;
    wp = (int16_t*)buffer;

    ops = *wp;
    cbuf += sizeof(*wp);

    while (--ops >= 0) {
        entry += *cbuf++;

        if ((count = *cbuf++) == 0) {
            count = 256;
        }

        while (--count >= 0) {
            Color_SetSystemPaletteEntry(entry, cbuf[0] >> shift, cbuf[1] >> shift, cbuf[2] >> shift);

            cbuf += 3;
            entry++;
        }
    }
}

void flicsmgr_decode_byte_run(uint8_t* buffer, Flic* flc) {
    int32_t x;
    int32_t y;
    int32_t height;
    int32_t offset;
    char psize;
    uint8_t* cpt;

    height = flc->height;
    cpt = buffer;
    y = 0;

    while (--height >= 0) {
        x = 0;
        psize = 0;
        offset = y;
        cpt++;

        while ((x += psize) < flc->width) {
            psize = *cpt++;

            if (psize >= 0) {
                memset(&flc->buffer[offset], *cpt++, psize);
            } else {
                psize = -psize;
                memcpy(&flc->buffer[offset], cpt, psize);
                cpt += psize;
            }

            offset += psize;
        }

        y += flc->full;
    }
}

void flicsmgr_decode_frame(FlicFrame* frame, Flic* flc) {
    FlicChunkHeader* chunk;
    char* next_chunk;

    next_chunk = frame->buffer;

    for (int32_t i = 0; i < frame->chunks; i++) {
        chunk = (FlicChunkHeader*)next_chunk;
        next_chunk += chunk->size;

        switch (chunk->type) {
            case COLOR_256:
                if (flc->load_flic_palette) {
                    flicsmgr_decode_color((uint8_t*)&chunk[1], 2);
                }
                break;
            case DELTA_FLC:
                flicsmgr_decode_delta_flc((uint8_t*)&chunk[1], flc);
                break;
            case COLOR_64:
                if (flc->load_flic_palette) {
                    flicsmgr_decode_color((uint8_t*)&chunk[1], 0);
                }
                break;
            case BLACK:
                win_fill(flc->wid, flc->bound.ulx, flc->bound.uly, flc->width, flc->height, 0);
                break;
            case BYTE_RUN:
                flicsmgr_decode_byte_run((uint8_t*)&chunk[1], flc);
                break;
            case LITERAL:
                buf_to_buf(reinterpret_cast<uint8_t*>(&chunk[1]), flc->width, flc->width, flc->height, flc->buffer,
                           flc->full);
                break;
            default:
                break;
        }
    }

    if (flc->wid) {
        win_draw_rect(flc->wid, (Rect*)&flc->bound);
    }
}

char Flicsmgr_load_frame(FILE* file, FlicFrame* frame) {
    FrameHeader flic_frame;

    if (frame->buffer) {
        SDL_free(frame->buffer);
        frame->buffer = nullptr;
    }

    SDL_assert(file);

    if (!fread(&flic_frame, 1, sizeof(FrameHeader), file)) {
        return 0;
    }

    frame->size = flic_frame.size - sizeof(FrameHeader);
    frame->type = flic_frame.type;
    frame->chunks = flic_frame.chunks;

    SDL_assert(frame->type == FLICSMGR_FRAME_TYPE);

    frame->buffer = (char*)SDL_malloc(frame->size);

    if (fread(frame->buffer, 1, frame->size, file)) {
        return 1;
    }

    SDL_free(frame->buffer);
    frame->buffer = nullptr;

    return 0;
}

char flicsmgr_fill_frame_buffer(Flic* flc) {
    int32_t v2;

    flc->buffer_start_frame = flc->frame_pos;
    v2 = flc->frame_count - flc->frame_pos + 1;

    if (v2 > FLICSMGR_FRAME_BUFFER) {
        v2 = FLICSMGR_FRAME_BUFFER;
    }

    for (int32_t i = 0; i < v2; i++) {
        if (!Flicsmgr_load_frame(flc->fp, &flc->frames[i])) {
            return 0;
        }
    }

    return 1;
}

char flicsmgr_read(Flic* flc) {
    FlicHeader flic_header;
    char result;

    if (!fread(&flic_header, 1, sizeof(FlicHeader), flc->fp)) {
        return 0;
    }

    if (flic_header.type != FLICSMGR_FLC_TYPE) {
        return 0;
    }

    flc->frame_count = flic_header.frames;
    flc->width = flic_header.width;
    flc->height = flic_header.height;
    flc->speed = flic_header.speed >> 3;

    flc->bound.lrx = flc->bound.ulx + flc->width - 1;
    flc->bound.lry = flc->bound.uly + flc->height - 1;

    fseek(flc->fp, flic_header.oframe1, SEEK_SET);

    if (!Flicsmgr_load_frame(flc->fp, &flc->frames[0])) {
        return 0;
    }

    flc->file_pos = ftell(flc->fp);

    flicsmgr_decode_frame(&flc->frames[0], flc);

    SDL_free(flc->frames[0].buffer);
    flc->frames[0].buffer = nullptr;

    if (flc->frame_count > 1 && flc->animate) {
        flc->frame_pos = 1;
        fseek(flc->fp, flc->file_pos, SEEK_SET);

        if (flicsmgr_fill_frame_buffer(flc)) {
            flc->frame_pos = 0;
            if ((flc->frame_count - 1) <= FLICSMGR_FRAME_BUFFER) {
                fclose(flc->fp);
                flc->fp = nullptr;
            }

            result = 1;
        } else {
            result = 0;
        }
    } else {
        flc->animate = 0;
        flc->frame_count = 0;
        fclose(flc->fp);
        flc->fp = nullptr;

        result = 1;
    }

    return result;
}

char flicsmgr_load(ResourceID id, Flic* flc) {
    flc->fp = ResourceManager_OpenFileResource(id, ResourceType_GameData);

    if (!flc->fp) {
        return 0;
    }

    if (flicsmgr_read(flc)) {
        return 1;
    }

    fclose(flc->fp);
    flc->fp = nullptr;

    return 0;
}

Flic* flicsmgr_construct(ResourceID id, WindowInfo* w, int32_t width, int32_t ulx, int32_t uly, char animate,
                         char load_flic_palette) {
    Flic* flc = static_cast<struct Flic*>(SDL_malloc(sizeof(struct Flic)));

    flc->fp = nullptr;

    flc->bound.ulx = ulx;
    flc->bound.uly = uly;
    flc->wid = w->id;
    flc->buffer = &w->buffer[ulx + width * uly];
    flc->full = width;
    flc->frame_pos = 0;
    flc->animate = animate;
    flc->load_flic_palette = load_flic_palette;

    for (int32_t i = 0; i < FLICSMGR_FRAME_BUFFER; i++) {
        flc->frames[i].buffer = nullptr;
    }

    if (flicsmgr_load(id, flc)) {
        if (!flc->animate) {
            SDL_free(flc);
            flc = nullptr;
        }

    } else {
        Rect bound;

        bound.ulx = ulx;
        bound.uly = uly;
        bound.lrx = ulx + FLICSMGR_FLIC_SIZE;
        bound.lry = uly + FLICSMGR_FLIC_SIZE;

        buf_fill(&w->buffer[ulx + uly * width], FLICSMGR_FLIC_SIZE, FLICSMGR_FLIC_SIZE, width, 0);

        if (w->id) {
            win_draw_rect(w->id, &bound);
        }

        SDL_free(flc);
        flc = nullptr;
    }

    return flc;
}

char flicsmgr_advance_animation(Flic* flc) {
    if (++flc->frame_pos > flc->frame_count) {
        if (flc->animate != 2) {
            flicsmgr_delete(flc);

            return 0;
        }

        flc->frame_pos = 1;

        if (flc->buffer_start_frame != 1) {
            fseek(flc->fp, flc->file_pos, SEEK_SET);
            flicsmgr_fill_frame_buffer(flc);
        }
    }

    if (flc->frame_pos >= (flc->buffer_start_frame + FLICSMGR_FRAME_BUFFER)) {
        flicsmgr_fill_frame_buffer(flc);
    }

    flicsmgr_decode_frame(&flc->frames[flc->frame_pos - flc->buffer_start_frame], flc);

    return 1;
}

void flicsmgr_delete(Flic* flc) {
    for (int32_t i = 0; i < FLICSMGR_FRAME_BUFFER; i++) {
        if (flc->frames[i].buffer) {
            SDL_free(flc->frames[i].buffer);
            flc->frames[i].buffer = nullptr;
        }
    }

    if (flc->fp) {
        fclose(flc->fp);
        flc->fp = nullptr;
    }

    flc->wid = 0;
}
