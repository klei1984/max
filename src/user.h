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

#define _text_to_buf ASM_PREFIX(_text_to_buf)
#define _text_height ASM_PREFIX(_text_height)
#define _text_width ASM_PREFIX(_text_width)
#define _text_char_width ASM_PREFIX(_text_char_width)
#define _text_mono_width ASM_PREFIX(_text_mono_width)
#define _text_spacing ASM_PREFIX(_text_spacing)
#define _text_size ASM_PREFIX(_text_size)
#define _text_max ASM_PREFIX(_text_max)

#undef CALLBACK
#undef ASM_PROLOGUE
#undef ASM_EPILOGUE
#undef ASM_INIT_CALLBACK

/* C++ specific manual overrides */

#if defined(__unix__)
#define ASM_CPP_FASTCALL_0_PARAM(func)  \
		push   %eax; \
		call   func;
#elif defined(_WIN32)
#define ASM_CPP_FASTCALL_0_PARAM(func)  \
		push   %ecx; \
		mov    %eax,%ecx; \
		call   func; \
		pop    %ecx
#else
#error "Platform is not supported"
#endif /* defined(platform) */

#if defined(__unix__)
#define ASM_CPP_FASTCALL_1_PARAM(func)  \
		push   %edx; \
		push   %edx; \
		push   %eax; \
		call   func; \
		pop    %edx
#else
#define ASM_CPP_FASTCALL_1_PARAM(func)  \
		push   %ecx; \
		push   %edx; \
		push   %edx; \
		mov    %eax,%ecx; \
		call   func; \
		pop    %edx; \
		pop    %ecx
#endif /* defined(platform) */

#if defined(__unix__)
#define ASM_CPP_FASTCALL_2_PARAM(func)  \
		push   %ebx; \
		push   %edx; \
		push   %ebx; \
		push   %edx; \
		push   %eax; \
		call   func; \
		pop    %edx; \
		pop    %ebx
#else
#define ASM_CPP_FASTCALL_2_PARAM(func)  \
		push   %ecx; \
		push   %ebx; \
		push   %edx; \
		push   %ebx; \
		push   %edx; \
		mov    %eax,%ecx; \
		call   func; \
		pop    %edx; \
		pop    %ebx; \
		pop    %ecx
#endif /* defined(platform) */

#if defined(__unix__)
#define ASM_CPP_FASTCALL_3_PARAM(func)  \
		push   %ecx; \
		push   %ebx; \
		push   %edx; \
		push   %ecx; \
		push   %ebx; \
		push   %edx; \
		push   %eax; \
		call   func; \
		pop    %edx; \
		pop    %ebx; \
		pop    %ecx
#else
#define ASM_CPP_FASTCALL_3_PARAM(func)  \
		push   %ecx; \
		push   %ebx; \
		push   %edx; \
		push   %ecx; \
		push   %ebx; \
		push   %edx; \
		mov    %eax,%ecx; \
		call   func; \
		pop    %edx; \
		pop    %ebx; \
		pop    %ecx
#endif /* defined(platform) */


/* ini_config object and class methods */
#define ini_config ASM_PREFIX(ini_config)
GLOBAL(ASM_PREFIX(ini_config))

#define _ZN11IniSettings4SaveEv ASM_PREFIX(_ZN11IniSettings4SaveEv)
GLOBAL(ASM_PREFIX(_ZN11IniSettings4SaveEv))

#define _ZN11IniSettings15GetNumericValueE10GAME_INI_e ASM_PREFIX(_ZN11IniSettings15GetNumericValueE10GAME_INI_e)
GLOBAL(ASM_PREFIX(_ZN11IniSettings15GetNumericValueE10GAME_INI_e))

#define _ZN11IniSettings14SetStringValueE10GAME_INI_ePc ASM_PREFIX(_ZN11IniSettings14SetStringValueE10GAME_INI_ePc)
GLOBAL(ASM_PREFIX(_ZN11IniSettings14SetStringValueE10GAME_INI_ePc))

#define _ZN11IniSettings14GetStringValueE10GAME_INI_ePci ASM_PREFIX(_ZN11IniSettings14GetStringValueE10GAME_INI_ePci)
GLOBAL(ASM_PREFIX(_ZN11IniSettings14GetStringValueE10GAME_INI_ePci))

#define _ZN11IniSettings11SaveSectionEPv10GAME_INI_e ASM_PREFIX(_ZN11IniSettings11SaveSectionEPv10GAME_INI_e)
GLOBAL(ASM_PREFIX(_ZN11IniSettings11SaveSectionEPv10GAME_INI_e))

#define _ZN11IniSettings11LoadSectionEPv10GAME_INI_ec ASM_PREFIX(_ZN11IniSettings11LoadSectionEPv10GAME_INI_ec)
GLOBAL(ASM_PREFIX(_ZN11IniSettings11LoadSectionEPv10GAME_INI_ec))

/* ini_clans object and class methods */
#define ini_clans ASM_PREFIX(ini_clans)
GLOBAL(ASM_PREFIX(ini_clans))

#define _ZN8IniClans8SeekUnitEii ASM_PREFIX(_ZN8IniClans8SeekUnitEii)
GLOBAL(ASM_PREFIX(_ZN8IniClans8SeekUnitEii))

#define _ZN8IniClans18GetNextUnitUpgradeEPsS0_ ASM_PREFIX(_ZN8IniClans18GetNextUnitUpgradeEPsS0_)
GLOBAL(ASM_PREFIX(_ZN8IniClans18GetNextUnitUpgradeEPsS0_))

#define _ZN8IniClans14GetStringValueEPci ASM_PREFIX(_ZN8IniClans14GetStringValueEPci)
GLOBAL(ASM_PREFIX(_ZN8IniClans14GetStringValueEPci))

#define _ZN8IniClans11GetClanGoldEi ASM_PREFIX(_ZN8IniClans11GetClanGoldEi)
GLOBAL(ASM_PREFIX(_ZN8IniClans11GetClanGoldEi))

#define _ZN8IniClans11GetClanTextEiPci ASM_PREFIX(_ZN8IniClans11GetClanTextEiPci)
GLOBAL(ASM_PREFIX(_ZN8IniClans11GetClanTextEiPci))

#define _ZN8IniClans11GetClanNameEiPci ASM_PREFIX(_ZN8IniClans11GetClanNameEiPci)
GLOBAL(ASM_PREFIX(_ZN8IniClans11GetClanNameEiPci))

/* soundmgr object and class methods */
#define soundmgr ASM_PREFIX(soundmgr)
GLOBAL(ASM_PREFIX(soundmgr))

#define _ZN8SoundMgr7PlaySfxE15GAME_RESOURCE_e ASM_PREFIX(_ZN8SoundMgr7PlaySfxE15GAME_RESOURCE_e)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr7PlaySfxE15GAME_RESOURCE_e))

#define _ZN8SoundMgr9PlayVoiceE15GAME_RESOURCE_eS0_s ASM_PREFIX(_ZN8SoundMgr9PlayVoiceE15GAME_RESOURCE_eS0_s)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr9PlayVoiceE15GAME_RESOURCE_eS0_s))

#define _ZN8SoundMgr9PlayMusicE15GAME_RESOURCE_eb ASM_PREFIX(_ZN8SoundMgr9PlayMusicE15GAME_RESOURCE_eb)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr9PlayMusicE15GAME_RESOURCE_eb))

#define _ZN8SoundMgr14FreeAllSamplesEv ASM_PREFIX(_ZN8SoundMgr14FreeAllSamplesEv)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr14FreeAllSamplesEv))

#define _ZN8SoundMgr17HaltMusicPlaybackEb ASM_PREFIX(_ZN8SoundMgr17HaltMusicPlaybackEb)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr17HaltMusicPlaybackEb))

#define _ZN8SoundMgr15HaltSfxPlaybackEb ASM_PREFIX(_ZN8SoundMgr15HaltSfxPlaybackEb)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr15HaltSfxPlaybackEb))

#define _ZN8SoundMgr17HaltVoicePlaybackEb ASM_PREFIX(_ZN8SoundMgr17HaltVoicePlaybackEb)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr17HaltVoicePlaybackEb))

#define _ZN8SoundMgr9SetVolumeEii ASM_PREFIX(_ZN8SoundMgr9SetVolumeEii)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr9SetVolumeEii))

#define _ZN8SoundMgr9FreeMusicEv ASM_PREFIX(_ZN8SoundMgr9FreeMusicEv)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr9FreeMusicEv))

#define _ZN8SoundMgr7PlaySfxEP8UnitInfoNS_8SFX_TYPEEb ASM_PREFIX(_ZN8SoundMgr7PlaySfxEP8UnitInfoNS_8SFX_TYPEEb)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr7PlaySfxEP8UnitInfoNS_8SFX_TYPEEb))

#define _ZN8SoundMgr17UpdateSfxPositionEv ASM_PREFIX(_ZN8SoundMgr17UpdateSfxPositionEv)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr17UpdateSfxPositionEv))

#define _ZN8SoundMgr17UpdateSfxPositionEP8UnitInfo ASM_PREFIX(_ZN8SoundMgr17UpdateSfxPositionEP8UnitInfo)
GLOBAL(ASM_PREFIX(_ZN8SoundMgr17UpdateSfxPositionEP8UnitInfo))

#define PATH_DEBUGGER 0

#endif /* USER_H */
