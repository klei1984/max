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

#ifndef USER_H
#define USER_H

#if defined(_WIN32)
#define ASM_PREFIX(symbol) _##symbol
#else /* defined(_WIN32) */
#define ASM_PREFIX(symbol) symbol
#endif /* defined(_WIN32) */

GLOBAL(ASM_PREFIX(text_width_func_wrapper_))
GLOBAL(ASM_PREFIX(init_callbacks))
GLOBAL(ASM_PREFIX(text_width))

#define CALLBACK(name) name##_wrapper_

#define ASM_PROLOGUE  \
    push % ebp;       \
    mov % esp, % ebp; \
    push % ecx;       \
    push % edx

#define ASM_EPILOGUE \
    pop % edx;       \
    pop % ecx;       \
    leave;           \
    ret

#define ASM_INIT_CALLBACK(cb, var) \
    lea CALLBACK(cb), % eax;       \
    movl % eax, var

.code32
.text
.align 4
ASM_PREFIX(init_callbacks): /* watcall void init_callbacks(void) */
		ASM_PROLOGUE

		ASM_INIT_CALLBACK(text_to_buf_func, ASM_PREFIX(_text_to_buf))
		ASM_INIT_CALLBACK(text_height_func, ASM_PREFIX(_text_height))
		ASM_INIT_CALLBACK(text_width_func, ASM_PREFIX(_text_width))
		ASM_INIT_CALLBACK(text_char_width_func, ASM_PREFIX(_text_char_width))
		ASM_INIT_CALLBACK(text_mono_width_func, ASM_PREFIX(_text_mono_width))
		ASM_INIT_CALLBACK(text_spacing_func, ASM_PREFIX(_text_spacing))
		ASM_INIT_CALLBACK(text_size_func, ASM_PREFIX(_text_size))
		ASM_INIT_CALLBACK(text_max_func, ASM_PREFIX(_text_max))
		ASM_INIT_CALLBACK(openFunc_func, ASM_PREFIX(_openFunc))
		ASM_INIT_CALLBACK(readFunc_func, ASM_PREFIX(_readFunc))
		ASM_INIT_CALLBACK(closeFunc_func, ASM_PREFIX(_closeFunc))

		ASM_EPILOGUE

.code32
.text
.align 4
CALLBACK(text_to_buf_func): /* watcall void (*text_to_buf_func)(unsigned char*, char*, int, int, int) */
		ASM_PROLOGUE

		push	0x8(%ebp) /* int */
		push	%ecx /* int */
		push	%ebx /* int */
		push	%edx /* char* */
		push	%eax /* unsigned char * */

		call	*ASM_PREFIX(text_to_buf)
		add	$0x18,%esp

		ASM_EPILOGUE	$0x000004

.code32
.text
.align 4
CALLBACK(text_height_func): /* watcall int (*text_height_func)(void) */
		ASM_PROLOGUE

		call	*ASM_PREFIX(text_height)

		ASM_EPILOGUE

.code32
.text
.align 4
CALLBACK(text_width_func): /* watcall int (*text_width_func)(char*) */
		ASM_PROLOGUE

		push	%eax /* char * */

		call	*ASM_PREFIX(text_width)
		add	$0x4,%esp

		ASM_EPILOGUE

.code32
.text
.align 4
CALLBACK(text_char_width_func): /* watcall int (*text_char_width_func)(char) */
		ASM_PROLOGUE

		push	%eax /* char */

		call	*ASM_PREFIX(text_char_width)
		add	$0x4,%esp

		ASM_EPILOGUE

.code32
.text
.align 4
CALLBACK(text_mono_width_func): /* watcall int (*text_mono_width_func)(char*) */
		ASM_PROLOGUE

		push	%eax /* char * */

		call	*ASM_PREFIX(text_mono_width)
		add	$0x4,%esp

		ASM_EPILOGUE

.code32
.text
.align 4
CALLBACK(text_spacing_func): /* watcall int (*text_spacing_func)(void) */
		ASM_PROLOGUE

		call	*ASM_PREFIX(text_spacing)

		ASM_EPILOGUE

.code32
.text
.align 4
CALLBACK(text_size_func): /* watcall int (*text_size_func)(char*) */
		ASM_PROLOGUE

		push	%eax /* char * */

		call	*ASM_PREFIX(text_size)
		add	$0x4,%esp

		ASM_EPILOGUE

.code32
.text
.align 4
CALLBACK(text_max_func): /* watcall int (*text_max_func)(void) */
		ASM_PROLOGUE

		call	*ASM_PREFIX(text_max)

		ASM_EPILOGUE

.code32
.text
.align 4
CALLBACK(openFunc_func): /* watcall unsigned long (*openFunc_func)(char *file, int mode) */
		ASM_PROLOGUE

		push	%edx /* int mode */
		push	%eax /* char *file */

		call	ASM_PREFIX(colorOpen)
		add	$0x8,%esp

		ASM_EPILOGUE


.code32
.text
.align 4
CALLBACK(readFunc_func): /* watcall unsigned long (*readFunc_func)(unsigned long handle, void *buf, unsigned long size) */
		ASM_PROLOGUE

		push	%ebx /* unsigned long size */
		push	%edx /* void *buf */
		push	%eax /* unsigned long handle */

		call	ASM_PREFIX(colorRead)
		add	$0xC,%esp

		ASM_EPILOGUE


.code32
.text
.align 4
CALLBACK(closeFunc_func): /* watcall unsigned long (*closeFunc_func)(unsigned long handle) */
		ASM_PROLOGUE

		push	%eax /* unsigned long handle */

		call	ASM_PREFIX(colorClose)
		add	$0x4,%esp

		ASM_EPILOGUE


#define _text_to_buf ASM_PREFIX(_text_to_buf)
#define _text_height ASM_PREFIX(_text_height)
#define _text_width ASM_PREFIX(_text_width)
#define _text_char_width ASM_PREFIX(_text_char_width)
#define _text_mono_width ASM_PREFIX(_text_mono_width)
#define _text_spacing ASM_PREFIX(_text_spacing)
#define _text_size ASM_PREFIX(_text_size)
#define _text_max ASM_PREFIX(_text_max)
#define _openFunc ASM_PREFIX(_openFunc)
#define _readFunc ASM_PREFIX(_readFunc)
#define _closeFunc ASM_PREFIX(_closeFunc)

#undef CALLBACK
#undef ASM_PROLOGUE
#undef ASM_EPILOGUE
#undef ASM_INIT_CALLBACK

#endif /* USER_H */
