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

#ifndef KB_CODE_H
#define KB_CODE_H

/* key lock status flags */
#define GNW_KB_LOCK_NONE 0
#define GNW_KB_LOCK_NUM 1
#define GNW_KB_LOCK_CAPS 2
#define GNW_KB_LOCK_SCROLL 4

/* key modifiers */
#define GNW_KB_MOD_NONE 0
#define GNW_KB_MOD_NUM 2
#define GNW_KB_MOD_CAPS 1
#define GNW_KB_MOD_SCROLL 4

#define GNW_KB_MOD_LCTRL 0x80
#define GNW_KB_MOD_RCTRL 0x100
#define GNW_KB_MOD_CTRL (GNW_KB_MOD_LCTRL | GNW_KB_MOD_RCTRL)

#define GNW_KB_MOD_LSHIFT 0x08
#define GNW_KB_MOD_RSHIFT 0x10
#define GNW_KB_MOD_SHIFT (GNW_KB_MOD_LSHIFT | GNW_KB_MOD_RSHIFT)

#define GNW_KB_MOD_LALT 0x20
#define GNW_KB_MOD_RALT 0x40
#define GNW_KB_MOD_ALT (GNW_KB_MOD_LALT | GNW_KB_MOD_RALT)

/* dos/x86 scan codes */
#define GNW_KB_SCAN_BUFFER_FULL 0x00
#define GNW_KB_SCAN_ESCAPE 0x01
#define GNW_KB_SCAN_1 0x02
#define GNW_KB_SCAN_2 0x03
#define GNW_KB_SCAN_3 0x04
#define GNW_KB_SCAN_4 0x05
#define GNW_KB_SCAN_5 0x06
#define GNW_KB_SCAN_6 0x07
#define GNW_KB_SCAN_7 0x08
#define GNW_KB_SCAN_8 0x09
#define GNW_KB_SCAN_9 0x0A
#define GNW_KB_SCAN_0 0x0B
#define GNW_KB_SCAN_MINUS 0x0C
#define GNW_KB_SCAN_EQUALS 0x0D
#define GNW_KB_SCAN_BACKSPACE 0x0E
#define GNW_KB_SCAN_TAB 0x0F
#define GNW_KB_SCAN_Q 0x10
#define GNW_KB_SCAN_W 0x11
#define GNW_KB_SCAN_E 0x12
#define GNW_KB_SCAN_R 0x13
#define GNW_KB_SCAN_T 0x14
#define GNW_KB_SCAN_Y 0x15
#define GNW_KB_SCAN_U 0x16
#define GNW_KB_SCAN_I 0x17
#define GNW_KB_SCAN_O 0x18
#define GNW_KB_SCAN_P 0x19
#define GNW_KB_SCAN_LEFTBRACKET 0x1A
#define GNW_KB_SCAN_RIGHTBRACKET 0x1B
#define GNW_KB_SCAN_RETURN 0x1C
#define GNW_KB_SCAN_LCTRL 0x1D
#define GNW_KB_SCAN_A 0x1E
#define GNW_KB_SCAN_S 0x1F
#define GNW_KB_SCAN_D 0x20
#define GNW_KB_SCAN_F 0x21
#define GNW_KB_SCAN_G 0x22
#define GNW_KB_SCAN_H 0x23
#define GNW_KB_SCAN_J 0x24
#define GNW_KB_SCAN_K 0x25
#define GNW_KB_SCAN_L 0x26
#define GNW_KB_SCAN_SEMICOLON 0x27
#define GNW_KB_SCAN_APOSTROPHE 0x28
#define GNW_KB_SCAN_GRAVE 0x29
#define GNW_KB_SCAN_LSHIFT 0x2A
#define GNW_KB_SCAN_BACKSLASH 0x2B
#define GNW_KB_SCAN_Z 0x2C
#define GNW_KB_SCAN_X 0x2D
#define GNW_KB_SCAN_C 0x2E
#define GNW_KB_SCAN_V 0x2F
#define GNW_KB_SCAN_B 0x30
#define GNW_KB_SCAN_N 0x31
#define GNW_KB_SCAN_M 0x32
#define GNW_KB_SCAN_COMMA 0x33
#define GNW_KB_SCAN_PERIOD 0x34
#define GNW_KB_SCAN_DIVIDE 0x35
#define GNW_KB_SCAN_RSHIFT 0x36
#define GNW_KB_SCAN_KP_MULTIPLY 0x37
#define GNW_KB_SCAN_LALT 0x38
#define GNW_KB_SCAN_SPACE 0x39
#define GNW_KB_SCAN_CAPSLOCK 0x3A
#define GNW_KB_SCAN_F1 0x3B
#define GNW_KB_SCAN_F2 0x3C
#define GNW_KB_SCAN_F3 0x3D
#define GNW_KB_SCAN_F4 0x3E
#define GNW_KB_SCAN_F5 0x3F
#define GNW_KB_SCAN_F6 0x40
#define GNW_KB_SCAN_F7 0x41
#define GNW_KB_SCAN_F8 0x42
#define GNW_KB_SCAN_F9 0x43
#define GNW_KB_SCAN_F10 0x44
#define GNW_KB_SCAN_NUMLOCK 0x45
#define GNW_KB_SCAN_SCROLLLOCK 0x46
#define GNW_KB_SCAN_HOME 0x47
#define GNW_KB_SCAN_UP 0x48
#define GNW_KB_SCAN_PAGEUP 0x49
#define GNW_KB_SCAN_KP_MINUS 0x4A
#define GNW_KB_SCAN_LEFT 0x4B
#define GNW_KB_SCAN_KP_5 0x4C
#define GNW_KB_SCAN_RIGHT 0x4D
#define GNW_KB_SCAN_KP_PLUS 0x4E
#define GNW_KB_SCAN_END 0x4F
#define GNW_KB_SCAN_DOWN 0x50
#define GNW_KB_SCAN_PAGEDOWN 0x51
#define GNW_KB_SCAN_INSERT 0x52
#define GNW_KB_SCAN_DELETE 0x53
#define GNW_KB_SCAN_SYSREQ 0x54

#define GNW_KB_SCAN_F11 0x57
#define GNW_KB_SCAN_F12 0x58

#define GNW_KB_SCAN_PAUSE 0x61

#define GNW_KB_SCAN_KP_ENTER 0x9C
#define GNW_KB_SCAN_RCTRL 0x9D

#define GNW_KB_SCAN_KP_DIVIDE 0xB5

#define GNW_KB_SCAN_RALT 0xB8

#endif /* KB_CODE_H */
