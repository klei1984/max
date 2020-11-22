---
layout: page
title: Documentation
permalink: /documentation/
---

## Preface

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
| Watcom floating-point libraries (math387r.lib & emu387r.lib) | M.A.X. is built with hardware floating point support, but if no x87 [\[3\]](#ref3) hardware is detected on the PC at runtime it is able to fall back to software emulation. |
| GNW | GNW is a user interface and OS-abstraction library designed and programmed by Timothy Cain [\[4\]](#ref4). Alternative versions of the library were used within Fallout 1, Fallout 2, Mapper 2 (Official Fallout 2 editor), Star Trek: Starfleet Academy, Atomic Bomberman and more.<br>GNW implements not only a windowing system, but memory and file system abstraction layers and more. It also provides a streamlined interface for debugging and a human input device recorder and playback service for arcade like attract mode. This is not the way M.A.X. realizes the demo gameplay in the main menu after a 60 seconds idle timeout though. |
| HMI S.O.S. v3.x | HMI Sound Operating System [\[5\]](#ref5) is a sound card detection and sound card abstraction layer for DOS and many other platforms. It supports digital and MIDI playback, mixing more than 20 sound channels, streaming a music sample directly from HDD to conserve RAM space and many-many more. M.A.X., Descent 1 and 2, Blood & Magic and the MVE library rely on the v3.x branch of the library. M.A.X. does not support MIDI music. The music in M.A.X. is too awesome to be in mere MIDI format. Thanks Mr. Luzietti [\[6\]](#ref6)! M.A.X. wraps the SOS API layer into a game specific C++ Sound Manager component called **soundmgr**.|
| MVE | Interplay's own video player library [\[7\]](#ref7). Alternative versions of the library were used within many Interplay legends like Redneck Rampage Rides Again, Descent II, Descent II setup tool, Fallout, etc. The library integrates the HMI SOS library for sound playback and the VBE library for video display. |
| LZSS | LZSS is used within the database manager component of GNW, but M.A.X does not rely on it. |
| VBE | VBE is a VESA BIOS Extension interface wrapper layer to the video display. The VBE library is not used by the game itself. Only the MVE library relies on the VBE library functions for video playback. The game itself uses GNW for video display. |

## Watcom v10.5 C++ 32 bit Constructs
### A bit of history
The first ISO C++ standard was published in September, 1998. ANSI standardization was proposed in 1989 and the first organizational meeting of the ANSI C++ committee took place already in December, 1989. The language specification continuously evolved during the time frame.  
  
A very good publication about the early history of C++ can be found in [\[8\]](#ref8).  
  
Watcom implemented the full AT&T C++ 3.0 specification with support for templates and exceptions in 1993 within Watcom C/C++32 v9.5. Watcom C/C++32 v10.5 was released two years later in 1995. At the time namespaces, or runtime type information (RTTI) were just concepts or even less.

The following sections attempt to document how the Watcom C/C++ compiler organizes data related to C++ constructs in 16/32 bit mixed linear executables (LE) targeting MS-DOS hosts.  
Most available publications about reversing C++ applications focus on state of the art compilers and related methods or data organization models. Most of those techniques or models simply do not apply to executables generated by the original Watcom C/C++ compilers and even the programming language was different.

The C++ user population doubled every 7.5 months or so between 1979 and 1991. The estimated number of C++ users was 400.000 in 1991. Why was C++ so popular?

One pretty obvious reason even for simple applications is semi automated resource deallocation or file handling via destructors. E.g. to load a character font using C, GNW opens a file, dynamically allocates a buffer for meta data, dynamically allocates further buffers for each individual character’s data, reads the file and fills all the buffers, and if any step fails the function enters dedicated else paths to deallocate all the resources necessary in a reverse order and finally it closes the file handle if necessary. All this is implemented by hand and every error condition has its own list of buffer deallocation and file handle operations depending on the already allocated and opened resources. This leads to bloated code with lots of conditional branches and repeated code snippets with minor differences only. Hard to read, error prone, difficult to maintain. On the other hand the generated code is efficient thanks to optimizing compilers that eliminate most of the duplicated code and many of the branches via various tricks and there are state of the art processes to eliminate or mitigate the risks of inroducing coding errors.  
The same can be achieved in C++ in a much cleaner way. But do not be fooled, this does not mean that C++ is memory or file system safe by design, quite the opposite.

### Used resources
* The binary and source code releases of the Open Watcom compiler, and the last professional version with its source code, could be downloaded from [\[9\]](#ref9).
* The older professional versions are required as well to be able to perform comparisons, analysis on libraries and such.
* The latest Open Watcom release for the favoured host OS is required for tools like wdis, wlib, wdump.

Analyzed code generator:  
&emsp; WATCOM C/C++32 Compile and Link Utility Version 10.5  
&emsp; wcl386.exe (1995-07-11)  
&emsp; MD5 hash: 2af860b0f1e431852f4989f4226f3938 *WCL386.EXE)  

### Identifying the compiler revision used
The simplest way to narrow down the list of potential compiler versions is the copyright notice which is embedded into the startup code: ```WATCOM C/C++32 Run-Time system. (c) Copyright by WATCOM International Corp. 1988-1995. All rights reserved.```. The string is very talkative. ```C++``` support was introduced in 1993 within v9.5. ```32``` indicates that the 32 bit runtime library is in use. The copyright date code ```1988-1995``` clearly indicates that the compiler is not older than 1995 which means at least v10.5. A brief version history is available on Wikipedia at [\[10\]](#ref10).  
  
A more precise way to identify the exact tool version is to take the C and C++ runtime libraries from the various compiler revisions, the library files that are linked into the executable like clib3r.lib or plib3s.lib, explode them using wlib tool, disassemble the resulting object files, like undefed.obj, using the wdis tool and compare the signatures of the resulting assembler routines and related variables or lookup tables with the disassembled executable's routines whether there are exact matches.  
  
<img src="{{ site.baseurl }}/assets/images/watcom_signature.svg" alt="workflow for matching library signatures">
  
For the signature checks a set of regex patterns based scripts were developed in Python. The output of the scripts cannot be 100% accurate, manual cross-verification is always required, but this is not a drawback. Having full control over the identification process guarantees that accuracy and completeness are kept high priority. The reversing tool used was IDA Freeware 7.0 which does not have built in signature checkers. The identified routines as well as variables were entered into the IDA database manually, but 100% accurate and as complete as possible with proper type definitions taken from the compiler's header files.

### Detecting the presence of C++ constructs in executables
The Watcom C++32 Runtime library provides services to manage global objects and their life cycles, exceptions and more. The easiest way to detect the presence of C++ constructs in an executable is to search for error messages related to the C++ runtime. A good example is the error message that is emitted when a non existent copy constructor is attempted to be invoked: ```undefined constructor or destructor called!```. There are many more such diagnostic messages.  
  

After basic identification, the previously described signature checks and setup of a reversing database allows identification of the C++ runtime library functions and their calling contexts can be analyzed further.  
  
| C++ Runtime Library Function  | Symbol Name | Description |
| ------------- | ------------- | ------------- |
| void CPPLIB(fatal_runtime_error)( char *msg, int code ) | \_\_wcpp_2_fatal_runtime_error\_\_ | Called on fatal runtime errors. Exit the application with diagnostic error message. |
| void CPPLIB( undefed_cdtor )( void ) | \_\_wcpp_2_undefed_cdtor\_\_ | The function is emitted by the compiler for undefined constructors and destructors as a placeholder in type signature tables. When a default constructor, copy constructor or destructor is not required by an application they are not generated instead this placeholder function is used. |
| void CPPLIB(mod_register)( RW_DTREG* rw ) | \_\_wcpp_2_mod_register\_\_ | Register constructed static objects into a linked list to be able to destruct them at application exit. |
| void CPPLIB(module_dtor)( void ) | \_\_wcpp_2_module_dtor\_\_ | Iterate the list of previously constructed static objects in reverse order and call their appropriate destructor. |
|  |  |  |

## Debug Options in M.A.X. v1.04


## References
<a name="ref1"></a>\[1\] [Watcom C/C++ compiler options](https://users.pja.edu.pl/~jms/qnx/help/watcom/compiler-tools/cpopts.html#SW3RS)<br>
<a name="ref2"></a>\[2\] [x86 calling conventions](https://en.wikipedia.org/wiki/X86_calling_conventions)<br>
<a name="ref3"></a>\[3\] [x87](https://en.wikipedia.org/wiki/X87)<br>
<a name="ref4"></a>\[4\] [Timothy Cain biography at MobyGames](https://www.mobygames.com/developer/sheet/view/developerId,2720/)<br>
<a name="ref5"></a>\[5\] [HMI SOS home page](http://web.archive.org/web/19970225190838/http://www.humanmachine.com/dev.htm)<br>
<a name="ref6"></a>\[6\] [Brian Luzietti biography at MobyGames](https://www.mobygames.com/developer/sheet/view/developerId,5423/)<br>
<a name="ref7"></a>\[7\] [MVE format](https://wiki.multimedia.cx/index.php/Interplay_MVE)<br>
<a name="ref8"></a>\[8\] A History of C++: 1979 - 1991, Bjarne Stroustrup - March, 1993<br>
<a name="ref9"></a>\[9\] [Open Watcom FTP](ftp://ftp.openwatcom.org/source)<br>
<a name="ref10"></a>\[10\] [Watcom C/C++ release history](https://en.wikipedia.org/wiki/Watcom_C/C%2B%2B)<br>
