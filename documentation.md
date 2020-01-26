---
layout: page
title: Documentation
permalink: /documentation/
---

# Preface

The article tries to document the technical aspects of M.A.X. that could either
be interesting for an enthusiast or could be relevant for an engineer.<br>
The information found herein is not guaranteed to be complete or technically accurate.

## Compiler Toolchain

M.A.X. v1.04 was built using the Watcom C/C++ 10.5 compiler. The original M.A.X.
runtime is a 16/32 bit mixed linear executable (LE) that is bound to a 32-bit DOS
extender stub.<br>
The Watcom compiler was shipped with the DOS/4GW 32-bit DOS extender.
The game was shipped with DOS/4GW 1.97.

The compiler supports various memory models from which M.A.X. used a Mixed
16/32-bit flat/small model.

| C/C++ Runtime Libraries  | Floating-Point Libraries (80x87) |
| ------------- | ------------- |
| clib3r.lib , plib3r.lib  | math387r.lib , emu387.lib  |

The **3r** suffix in the library names means that the compiler generates 386
instructions based on the 386 instruction timings and that the compiler uses
the Watcom register calling convention [\[1\]](#ref1)[\[2\]](#ref2).

In general the compiled code inherits the following characteristics:
- function arguments are passed in registers EAX, EDX, EBX and ECX as long as they fit and then on the stack.
- 48 bit __far pointers are passed 64 bit aligned in two registers or stack variables.
- all registers except EAX are preserved across function calls.
- functions with external linkage are suffixed with an underscore on machine-code level. E.g. **main_**.
- variables with external linkage are prefixed with an underscore on machine-code level. E.g. **__STACKLOW**.

## Libraries Used

M.A.X. is built upon several static link libraries. Out of the 5706 subroutines
1644 (28.81%) are coming from these libraries. Of course its also true that many
of the library functions are not even used by the game.

| Library Name  | Description |
| ------------- | ------------- |
| Watcom C 32-bit runtime (clib3r.lib) | Implements standard C library functions like fopen, memcpy, etc. At the time the Watcom C/C++ compiler supported several platforms like DOS, Linux, Netware, Windows and in many cases the compiler supported various flavors of the C standard like POSIX, ANSI and ISO. Many of the related functions are not portable. |
| Watcom C++ 32-bit runtime (plib3r.lib) | Implements *standard* C++ support. The library contains services that facilitate handling of C++ classes, global constructor and destructor lists, vtables, class inheritance and similar C++ "stuff".<br>M.A.X. calls 32 global constructors from which 12 are compiler or system related. E.g. there is a global constructor called __verify_pentium_fdiv_bug(). Interesting... or not. |
| Watcom floating-point libraries (math387r.lib & emu387r.lib) | M.A.X. is built with hardware floating point support, but if no x87 [\[3\]](#ref3) hardware is detected on the PC at run-time it is able to fall back to software emulation. |
| GNW | GNW is a user interface and OS-abstraction library designed and programmed by Timothy Cain [\[4\]](#ref4). Alternative versions of the library were used within Fallout 1, Fallout 2, Mapper 2 (Official Fallout 2 editor), Star Trek: Starfleet Academy, Atomic Bomberman and more.<br>GNW implements not only a windowing system, memory and file system abstraction layers and such. It also provides a streamlined interface for debugging and a human input device recorder and playback service for arcade like attract mode. This is not the way M.A.X. realizes the demo gameplay in the main menu after a 60 seconds idle timeout though. |
| HMI S.O.S. v3.x | HMI Sound Operating System [\[5\]](#ref5) is a sound card detection and sound card abstraction layer for DOS and many other platforms. It supports digital and MIDI playback, mixing more than 20 sound channels, streaming a music sample directly from HDD to conserve RAM space and many-many more. M.A.X., Descent 1 and 2, Blood & Magic and the MVE library rely on the v3.x branch of the library. M.A.X. does not support MIDI music. The music in M.A.X. is too awesome to be in mere MIDI format. Thanks Mr. Luzietti [\[6\]](#ref6)! M.A.X. wraps the SOS API layer into a game specific C++ Sound Manager component called **soundmgr**.|
| MVE | Interplay's own video player library [\[7\]](#ref7). Alternative versions of the library were used within many Interplay legends like Redneck Rampage Rides Again, Descent II, Descent II setup tool, Fallout, etc. The library integrates the HMI SOS library for sound playback. |
| LZSS | LZSS is used within the resource manager component of M.A.X which is also a library (probably part of GNW?). |
| VBE | VBE is a VESA BIOS Extension interface wrapper layer to the video display. The VBE library is not used by the game itself. Only the MVE library relies on the VBE library functions for video playback. The game itself uses GNW for video display. |

## Debug options in M.A.X. v1.04


## References
<a name="ref1"></a>\[1\] [Watcom C/C++ compiler options](https://users.pja.edu.pl/~jms/qnx/help/watcom/compiler-tools/cpopts.html#SW3RS)<br>
<a name="ref2"></a>\[2\] [x86 calling conventions](https://en.wikipedia.org/wiki/X86_calling_conventions)<br>
<a name="ref3"></a>\[3\] [x87](https://en.wikipedia.org/wiki/X87)<br>
<a name="ref4"></a>\[4\] [Timothy Cain biography at MobyGames](https://www.mobygames.com/developer/sheet/view/developerId,2720/)<br>
<a name="ref5"></a>\[5\] [HMI SOS home page](http://web.archive.org/web/19970225190838/http://www.humanmachine.com/dev.htm)<br>
<a name="ref6"></a>\[6\] [Brian Luzietti biography at MobyGames](https://www.mobygames.com/developer/sheet/view/developerId,5423/)<br>
<a name="ref7"></a>\[7\] [MVE format](https://wiki.multimedia.cx/index.php/Interplay_MVE)<br>
