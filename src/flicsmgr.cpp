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

#include <new>

extern "C" {
#include <stdio.h>

#include "game.h"
}

#define FLICSMGR_FLI_TYPE 0xAF11u
#define FLICSMGR_FLC_TYPE 0xAF12u
#define FLICSMGR_FRAME_TYPE 0xF1FAu

#define FLICSMGR_FRAME_BUFFER 5

enum ChunkTypes { COLOR_256 = 4, DELTA_FLC = 7, COLOR_64 = 11, BLACK = 13, BYTE_RUN = 15, LITERAL = 16 };

typedef struct __attribute__((packed)) {
    long size;
    unsigned short type;
    unsigned short frames;
    unsigned short width;
    unsigned short height;
    unsigned short depth;
    unsigned short flags;
    long speed;
    short reserved1;
    unsigned long created;
    unsigned long creator;
    unsigned long updated;
    unsigned long updater;
    unsigned short aspect_dx;
    unsigned short aspect_dy;
    char reserved2[38];
    long oframe1;
    long oframe2;
    char reserved3[40];
} FlicHeader;

static_assert(sizeof(FlicHeader) == 128, "FLIC header is 128 bytes");

typedef struct {
    long size;
    unsigned short type;
    short chunks;
    char reserved[8];
} FrameHeader;

static_assert(sizeof(FrameHeader) == 16, "FLIC frame header is 16 bytes");

typedef struct __attribute__((packed)) {
    int size;
    unsigned short type;
} FlicChunkHeader;

static_assert(sizeof(FlicChunkHeader) == 6, "FLIC chunk header is 6 bytes");

typedef struct {
    unsigned int size;
    unsigned short type;
    unsigned short chunks;
    char *buffer;
} FlicFrame;

struct Flic_s {
    FILE *fp;
    short field_4;
    short field_6;
    FlicFrame frames[FLICSMGR_FRAME_BUFFER];
    long int file_pos;
    WinID wid;
    unsigned char *buffer;
    Rect bound;
    char animate;
    short frame_count;
    short width;
    short height;
    short frame_pos;
    short full;
    int speed;
    char load_flic_palette;
};

static void flicsmgr_decode_delta_flc(unsigned char *buffer, Flic *flc);
static void flicsmgr_decode_color(unsigned char *buffer, int shift);
static void flicsmgr_decode_byte_run(unsigned char *buffer, Flic *flc);
static void flicsmgr_decode_frame(FlicFrame *frame, Flic *flc);
static char Flicsmgr_load_frame(FILE *file, FlicFrame *frame);
static char flicsmgr_fill_frame_buffer(Flic *flc);
static char flicsmgr_read(Flic *flc);
static char flicsmgr_load(char *flic_file, Flic *flc);

void flicsmgr_decode_delta_flc(unsigned char *buffer, Flic *flc) {
    short opt_word;
    short packet_count;
    unsigned char column_skip_count;
    signed char packet_type;
    unsigned char *line_address;
    short line_count;
    int offset;

    line_count = *(short *)buffer;
    buffer += sizeof(short);

    line_address = flc->buffer;
    while (--line_count >= 0) {
        opt_word = *(short *)buffer;
        buffer += sizeof(short);

        /* process optional skip count words and last byte word */
        while ((unsigned short)opt_word & 0x8000) {
            if ((unsigned short)opt_word & 0x4000) {
                /* absolute value of optional word holds line skip count */
                opt_word = -opt_word;
                line_address += opt_word * flc->full;
            } else {
                /* low order byte of optional word holds last byte of current line */
                line_address[flc->width - 1] = opt_word;
            }

            opt_word = *(short *)buffer;
            buffer += sizeof(short);
        }

        packet_count = opt_word;
        offset = 0;

        while (--packet_count >= 0) {
            column_skip_count = buffer[0];
            packet_type = buffer[1];

            buffer += sizeof(short);

            offset += column_skip_count;

            if (packet_type >= 0) {
                /* copy pocket type count of words */
                memcpy(&line_address[offset], buffer, packet_type * sizeof(short));
                buffer += packet_type * sizeof(short);
                offset += packet_type * sizeof(short);
            } else {
                /* replicate absolute value of the pocket type count of words
                 * the word to replicate is the next word in the buffer
                 */
                short word = *(short *)buffer;
                buffer += sizeof(short);

                packet_type = -packet_type;
                while (--packet_type >= 0) {
                    *(short *)(&line_address[offset]) = word;
                    offset += sizeof(short);
                }
            }
        }

        line_address += flc->full;
    }
}

void flicsmgr_decode_color(unsigned char *buffer, int shift) {
    int entry;
    unsigned char *cbuf;
    short *wp;
    short ops;
    int count;

    entry = 0;
    cbuf = buffer;
    wp = (short *)buffer;

    ops = *wp;
    cbuf += sizeof(*wp);

    while (--ops >= 0) {
        entry += *cbuf++;

        if ((count = *cbuf++) == 0) {
            count = 256;
        }

        while (--count >= 0) {
            setSystemPaletteEntry(entry, cbuf[0] >> shift, cbuf[1] >> shift, cbuf[2] >> shift);

            cbuf += 3;
            entry++;
        }
    }
}

void flicsmgr_decode_byte_run(unsigned char *buffer, Flic *flc) {
    int x;
    int y;
    int height;
    int offset;
    char psize;
    unsigned char *cpt;

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

void flicsmgr_decode_frame(FlicFrame *frame, Flic *flc) {
    FlicChunkHeader *chunk;
    char *next_chunk;

    next_chunk = frame->buffer;

    for (int i = 0; i < frame->chunks; i++) {
        chunk = (FlicChunkHeader *)next_chunk;
        next_chunk += chunk->size;

        switch (chunk->type) {
            case COLOR_256:
                if (flc->load_flic_palette) {
                    flicsmgr_decode_color((unsigned char *)&chunk[1], 2);
                }
                break;
            case DELTA_FLC:
                flicsmgr_decode_delta_flc((unsigned char *)&chunk[1], flc);
                break;
            case COLOR_64:
                if (flc->load_flic_palette) {
                    flicsmgr_decode_color((unsigned char *)&chunk[1], 0);
                }
                break;
            case BLACK:
                win_fill(flc->wid, flc->bound.ulx, flc->bound.uly, flc->width, flc->height, 0);
                break;
            case BYTE_RUN:
                flicsmgr_decode_byte_run((unsigned char *)&chunk[1], flc);
                break;
            case LITERAL:
                buf_to_buf((unsigned char *)(&chunk[1]), flc->width, flc->width, flc->height, flc->buffer, flc->full);
                break;
            default:
                break;
        }
    }

    if (flc->wid) {
        win_draw_rect(flc->wid, (Rect *)&flc->bound);
    }
}

char Flicsmgr_load_frame(FILE *file, FlicFrame *frame) {
    FrameHeader flic_frame;

    if (frame->buffer) {
        free(frame->buffer);
        frame->buffer = NULL;
    }

    SDL_assert(file);

    if (!fread(&flic_frame, 1, sizeof(FrameHeader), file)) {
        return 0;
    }

    frame->size = flic_frame.size - sizeof(FrameHeader);
    frame->type = flic_frame.type;
    frame->chunks = flic_frame.chunks;

    SDL_assert(frame->type == FLICSMGR_FRAME_TYPE);

    frame->buffer = (char *)malloc(frame->size);

    if (fread(frame->buffer, 1, frame->size, file)) {
        return 1;
    }

    free(frame->buffer);
    frame->buffer = NULL;

    return 0;
}

char flicsmgr_fill_frame_buffer(Flic *flc) {
    int v2;

    flc->field_4 = flc->frame_pos;
    v2 = flc->frame_count - flc->frame_pos + 1;

    if (v2 > FLICSMGR_FRAME_BUFFER) {
        v2 = FLICSMGR_FRAME_BUFFER;
    }

    for (int i = 0; i < v2; i++) {
        if (!Flicsmgr_load_frame(flc->fp, &flc->frames[i])) {
            return 0;
        }
    }

    return 1;
}

char flicsmgr_read(Flic *flc) {
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

    free(flc->frames[0].buffer);
    flc->frames[0].buffer = NULL;

    if (is_max_cd_in_use && strcmp(file_path_flc, file_path_game_install)) {
        flc->frame_count = 1;
    }

    if (flc->frame_count > 1 && flc->animate) {
        flc->frame_pos = 1;
        fseek(flc->fp, flc->file_pos, SEEK_SET);

        if (flicsmgr_fill_frame_buffer(flc)) {
            flc->frame_pos = 0;
            if ((flc->frame_count - 1) <= FLICSMGR_FRAME_BUFFER) {
                fclose(flc->fp);
                flc->fp = NULL;
            }

            result = 1;
        } else {
            result = 0;
        }
    } else {
        flc->animate = 0;
        flc->frame_count = 0;
        fclose(flc->fp);
        flc->fp = NULL;

        result = 1;
    }

    return result;
}

char flicsmgr_load(char *flic_file, Flic *flc) {
    char filename[PATH_MAX];

    if (!flic_file) {
        return 0;
    }

    strcpy(filename, file_path_flc);
    strcat(filename, flic_file);

    /* allocated by resource manager */
    free(flic_file);

    flc->fp = fopen(filename, "rb");

    if (!flc->fp) {
        return 0;
    }

    if (flicsmgr_read(flc)) {
        return 1;
    }

    fclose(flc->fp);
    flc->fp = NULL;

    return 0;
}

Flic *flicsmgr_construct(GAME_RESOURCE id, Window *w, int width, int ulx, int uly, char animate,
                         char load_flic_palette) {
    Flic *flc = new (std::nothrow) Flic();

    flc->fp = NULL;

    flc->bound.ulx = ulx;
    flc->bound.uly = uly;
    flc->wid = w->id;
    flc->buffer = &w->buffer[ulx + width * uly];
    flc->full = width;
    flc->frame_pos = 0;
    flc->animate = animate;
    flc->load_flic_palette = load_flic_palette;

    for (int i = 0; i < FLICSMGR_FRAME_BUFFER; i++) {
        flc->frames[i].buffer = NULL;
    }

    if (flicsmgr_load((char *)read_game_resource(id), flc)) {
        if (!flc->animate) {
            delete flc;
            flc = NULL;
        }

    } else {
        Rect bound;

        bound.ulx = ulx;
        bound.uly = uly;
        bound.lrx = ulx + 128;
        bound.lry = uly + 128;

        buf_fill(&w->buffer[ulx + uly * width], 128, 128, width, 0);

        if (w->id) {
            win_draw_rect(w->id, &bound);
        }

        delete flc;
        flc = NULL;
    }

    return flc;
}

char flicsmgr_advance_animation(Flic *flc) {
    if (++flc->frame_pos > flc->frame_count) {
        if (flc->animate != 2) {
            flicsmgr_delete(flc);

            return 0;
        }

        flc->frame_pos = 1;

        if (flc->field_4 != 1) {
            fseek(flc->fp, flc->file_pos, SEEK_SET);
            flicsmgr_fill_frame_buffer(flc);
        }
    }

    if (flc->frame_pos >= (flc->field_4 + FLICSMGR_FRAME_BUFFER)) {
        flicsmgr_fill_frame_buffer(flc);
    }

    flicsmgr_decode_frame(&flc->frames[flc->frame_pos - flc->field_4], flc);

    return 1;
}

void flicsmgr_delete(Flic *flc) {
    for (int i = 0; i < FLICSMGR_FRAME_BUFFER; i++) {
        if (flc->frames[i].buffer) {
            free(flc->frames[i].buffer);
            flc->frames[i].buffer = NULL;
        }
    }

    if (flc->fp) {
        fclose(flc->fp);
        flc->fp = NULL;
    }

    flc->wid = 0;
}
