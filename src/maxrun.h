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

/** \todo Auto generate the entire file */

/* Helper macros */
#define GLOBAL(sym) .global sym
#define REPLACE(sym) REMOVED_##sym

/* replaced start-up code and EIP */
#define _start __start
GLOBAL(main_);
GLOBAL(__InitRtns);

/* replaced game functions */
GLOBAL(_1794c8_data);
GLOBAL(word_1781C0);

/* replaced sound operating system functions */
#define sosTIMERInitSystem_ ac_sosTIMERInitSystem
#define sosTIMERRegisterEvent_ ac_sosTIMERRegisterEvent
#define sosTIMERRemoveEvent_ ac_sosTIMERRemoveEvent
#define sosTIMERUnInitSystem_ ac_sosTIMERUnInitSystem
#define sosDIGIDetectInit_ ac_sosDIGIDetectInit
#define sosDIGIDetectFindHardware_ ac_sosDIGIDetectFindHardware
#define sosDIGIDetectUnInit_ ac_sosDIGIDetectUnInit
#define sosDIGIInitSystem_ ac_sosDIGIInitSystem
#define sosDIGIUnInitSystem_ ac_sosDIGIUnInitSystem
#define sosDIGIInitDriver_ ac_sosDIGIInitDriver
#define sosDIGIUnInitDriver_ ac_sosDIGIUnInitDriver
#define sosDIGIStartSample_ ac_sosDIGIStartSample
#define sosDIGIStopSample_ ac_sosDIGIStopSample
#define sosDIGIContinueSample_ ac_sosDIGIContinueSample
#define sosDIGISampleDone_ ac_sosDIGISampleDone
#define sosDIGISetSampleVolume_ ac_sosDIGISetSampleVolume
#define sosDIGIGetPanLocation_ ac_sosDIGIGetPanLocation
#define sosDIGISetPanLocation_ ac_sosDIGISetPanLocation

/* replaced svga functions */
GLOBAL(scr_blit);
GLOBAL(scr_size);
#define vesa_screen_blit_ ac_vesa_screen_blit
#define init_vesa_mode_ ac_init_vesa_mode
#define reset_mode ac_reset_mode
#define get_start_mode_ ac_get_start_mode

/* replaced key functions */
#define key_init_ ac_key_init
#define key_close_ ac_key_close
#define kb_clear_ ac_kb_clear
#define kb_getch_ ac_kb_getch
#define kb_set_layout_ ac_kb_set_layout
#define kb_get_layout_ ac_kb_get_layout
#define kb_simulate_key_ ac_kb_simulate_key

/* replaced mouse functions */
GLOBAL(have_mouse);
GLOBAL(mouse_disabled);
GLOBAL(_mouse_is_hidden);
GLOBAL(mouse_blit);
GLOBAL(mouse_x);
GLOBAL(mouse_y);
GLOBAL(mouse_colorize);
GLOBAL(_mouse_sensitivity);
GLOBAL(mouse_simulate_input);
GLOBAL(mouse_set_shape);
GLOBAL(last_buttons);
#define GNW_mouse_init ac_GNW_mouse_init
#define mouse_info ac_mouse_info

/* replaced vcr functions */
GLOBAL(vcr_state);
GLOBAL(vcr_terminate_flags);
GLOBAL(vcr_terminated_condition);
GLOBAL(vcr_stop);

/* replaced timer functions */
#define timer_init_ ac_timer_init
#define timer_close_ ac_timer_close
#define timer_wait_ ac_timer_wait
#define timer_time_remaining_ms_ ac_timer_time_remaining_ms
#define timer_ch2_setup_ ac_timer_ch2_setup
#define timer_set_rate_ ac_timer_set_rate
#define timer_get_stamp32_ ac_timer_get_stamp32
#define get_time ac_get_time

/* replaced color palette functions */
GLOBAL(currentGammaTable);
GLOBAL(systemCmap);
#define setSystemPalette_ ac_setSystemPalette
#define setSystemPaletteEntry ac_setSystemPaletteEntry

/* replaced game functions */
GLOBAL(debug_register_env);
#define register_critical_error_handler ac_register_critical_error_handler
#define check_available_extended_memory_ ac_check_available_extended_memory
#define check_available_disk_space_ ac_check_available_disk_space

/* replaced DOS functions */
#define __delay_init_ ac_dos_delay_init
#define __init_387_emulator ac_dos_init_387_emulator
#define __fini_387_emulator ac_dos_fini_387_emulator
#define __Init_Argv_ ac_dos_init_argv
#define __setenvp_ ac_dos_setenvp
#define _dos_getvect_ ac_dos_getvect
#define int386_ ac_dos_int386
#define int386x_ ac_dos_int386x
#define get_dpmi_physical_memory ac_get_dpmi_physical_memory
#define _dos_open_ ac__dos_open
#define __assert_ ac_dos_assert

/* replaced Posix Library functions */
GLOBAL(strupr_);
#define lseek_ ac_lseek
#define open_ ac_posix_open

/* replaced C Library functions */
#define atoi_ ac_atoi
#define chdir_ ac_chdir
#define close_ ac_close
#define exit_ ac_exit
#define fclose_ ac_fclose
#define fflush_ ac_fflush
#define fgetc_ ac_fgetc
#define fgets_ ac_fgets
//#define fopen_ ac_fopen
#define fopen_ ac_dos_fopen
#define fprintf_ ac_fprintf
#define fputc_ ac_fputc
#define fputs_ ac_fputs
#define free_ ac_free
#define fread_ ac_fread
#define fseek_ ac_fseek
#define ftell_ ac_ftell
#define fwrite_ ac_fwrite
#define getenv_ ac_getenv
#define isatty_ ac_isatty
#define malloc_ ac_malloc
#define memcpy_ ac_memcpy
#define memmove_ ac_memmove
#define memset_ ac_memset
#define mktime_ ac_mktime
#define printf_ ac_printf
#define puts_ ac_puts
#define pow_ ac_pow
//#define rand_ ac_rand
#define read_ ac_read
#define realloc_ ac_realloc
#define sprintf_ ac_sprintf
//#define srand_ ac_srand
#define strcat_ ac_strcat
#define strcmp_ ac_strcmp
#define strcpy_ ac_strcpy
#define stricmp_ ac_stricmp
#define strlen_ ac_strlen
#define strncat_ ac_strncat
#define strncmp_ ac_strncmp
#define strncpy_ ac_strncpy
#define strnicmp_ ac_strnicmp
#define strrchr_ ac_strrchr
#define strstr_ ac_strstr
#define time_ ac_time
#define tolower_ ac_tolower
#define toupper_ ac_toupper
#define tzset_ ac_tzset
#define unlink_ ac_unlink
//#define vsprintf_ ac_vsprintf
#define write_ ac_write

.code32
.text
stub_function :
	movl $0x000000, % eax
	ret
