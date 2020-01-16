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

#ifndef UNITS_H
#define UNITS_H

struct __attribute__((packed)) UnitInfo_s {
    unsigned int flags;
    unsigned short data;
    unsigned int unknown_1;
    unsigned short animation;
    unsigned short portrait;
    unsigned short interface_icon;
    unsigned short stored_portrait;
    unsigned short unknown_2;
    unsigned char cargo_type;
    unsigned char land_type;
    unsigned char gender;
    char* singular_name;
    char* plural_name;
    char* description;
    char* alt_description;
    unsigned short sprite;
    unsigned short shadows;
};

typedef struct UnitInfo_s* UnitInfoPtr;

struct __attribute__((packed)) UnitInfo2_s {
    unsigned int flags;
    unsigned short data;
    void* resource_buffer;
    unsigned short animation;
    unsigned short portrait;
    unsigned short interface_icon;
    unsigned short stored_portrait;
    unsigned short unknown_2;
    unsigned char cargo_type;
    unsigned char land_type;
    unsigned char gender;
    char* singular_name;
    char* plural_name;
    char* description;
    char* alt_description;
    int unknown_3;
    int unknown_4;
    int unknown_5;
};

typedef struct UnitInfo2_s* UnitInfo2Ptr;

#endif /* UNITS_H */
