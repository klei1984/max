---
layout: page
title: Roadmap
permalink: /roadmap/
---

This article tries to maintain a high level overview of the work packages and challenges that need to be solved to step by step complete the port.

Last updated: 2022-12-21.

Reimplementation status: 5666 / 5704 (99%) functions.

The list is subject to change at any time. The outlined order of work packages, priorities, could be rearranged depending on the difficulty, available time or available help from others. I am new to many of the GitHub and open source toolings and hope to get help from friendly enthusiasts. Obvious work packages like fix all software defects identified and such are not mentioned explicitly in the list.

Items in the list are color-coded using the following interpretation:
- <span class="legend-done">Feature Complete</span> (all done but subject to bug fixes and future refinements)
- <span class="legend-close">Mostly Complete</span> (almost there, could be waiting on another system)
- <span class="legend-inwork">In Progress</span> (work started but still has some way to go)
- Not Started (work is yet to begin on this feature)


### 0.1 Build toolchain
- <span class="legend-done">
  Roll out basic CMAKE based build system that supports cross-platform toolchain configurations
  </span>
- <span class="legend-done">
  Refactor build scripts to support a proper folder structure or packaging concept
  </span>
- <span class="legend-done">
  Consolidate code generator scripts to simplify the configuration process, provide better configurability and support calls from and to old Watcom code
  </span>
-  <span class="legend-done">
  Roll out basic Github Actions setup as CI/CD tool
  </span>
-  <span class="legend-inwork">
  Document build toolchain
  </span>

### 0.2 Mock up basic OS dependent libraries and services
- <span class="legend-done">
  Replace the Watcom Standard C library
  </span>
- <span class="legend-done">
  Replace the Watcom Math library
  </span>
- <span class="legend-done">
  Replace or stub DPMI services
  </span>
- <span class="legend-done">
  Mock up VBE / VESA driver using SDL2 video
  (excluding driver variants used by MVE library)
  </span>
- <span class="legend-done">
  Mock up Mouse using SDL2 mouse
  </span>
- <span class="legend-done">
  Mock up Keyboard using SDL2 keyboard (far from perfect)
  </span>
- <span class="legend-done">
  Mock up timer services using SDL2
  </span>

### 0.3 Reimplement original platform library modules
The original platform libraries were implemented in pure C. The workflow within this package includes steps to reenginer the original GNW and other platform modules into ISO C99 compliant source code.
The netcode of M.A.X. relied on MS-DOS DPMI services and the IPX protocol for LAN play. These have to be fully replaced by an IP based netcode that should facilitate Internat play.

- <span class="legend-done">
  Reimplement the human machine interface related modules (kb, mouse, vcr, input)
  </span>
- <span class="legend-done">
  Reimplement the color management module (color)
  </span>
- <span class="legend-done">
  Reimplement the GUI controls related modules (gnw, rect, grbuf, interface, button)
  </span>
- <span class="legend-done">
  Reimplement the memory management module (memory)
  </span>
- <span class="legend-done">
  Reimplement the debug services module (debug)
  </span>
- <span class="legend-done">
  Reimplement the database management module (db, assoc)
  </span>
- <span class="legend-inwork">
  Replace the IPX driver and related dpmi modules (ipx, dpmi)
  </span>

### 0.4 Reimplement original MVE library modules
The original MVE library was implemented in C and Assembly languages using the Watcom C/C++ compiler. The library uses its own version of MS-DOS video and audio drivers. The library also relies on self modifying code which means that the assembly subroutines are modified on the fly by themselves. This makes the original library very difficult to port to different compiler or processor architectures. The audio data streams are interleaved with the video data streams and the audio frame rate is synchronized with the video frame rate. On low end computers this allows video frames to be skipped to be able to keep up with the audio. A choppy video stream is considered less annoying than a skipping audio stream. The library supported further performance optimization techniques like decreasing the resolution of the rendered video stream or interlacing the output effectively skipping every second horizontal line of the video data. The library itself would support closed captions too, but M.A.X. did not utilize this feature. The workflow within this package is similar to the one in package 0.3 except for the OS dependent drivers layer and the assembler subroutines.

- <span class="legend-inwork">
  Replace the low level OS dependent drivers (sos, vbe, ...)
  </span>
- <span class="legend-inwork">
  Reimplement the MVE library modules (mvelib32, mveliba)
  </span>
- Implement feature to scale videos to match screen resolution
- Implement feature to render captions (subtitles)

### 0.5 Study Watcom C++, analyse original C++ classes and develop necessary tools
Most of the game itself was implemented in C++. Even though the Watcom C/C++ compiler became open source it is still very difficult to fully understand how the original compiler organizes constructor, destructor, operator overload and such behind the scenes C++ runtime specific data. Clear understanding of the Watcom C++ runtime library and related data that the compiler emits into executables is a prerequisit to be able to identify and understand the C++ class hierarchies from the game.

- <span class="legend-done">
  Study Watcom V11.0b and Open Watcom V1.1.0 source code and set up test environment
  </span>
- <span class="legend-done">
  Build reference classes with Watcom V10.5 and/or Open Watcom V1.1.0 with debug information to learn how C++ library specific meta data are emitted by the compiler and used by the C++ runtime library (plib3r.lib).
  </span>
- <span class="legend-done">
  Discover and analyse original class hierarchies
  </span>
- <span class="legend-inwork">
  Document findings in an article
  </span>

### 0.6 Reimplement original C++ classes
The first ISO C++ standard was released only in 1998. Standardization took many years and compiler vendors usually lagged behind to follow up the latest published draft papers. Based on various Dr. Dobb’s Journal articles smart pointers appeared as a concept in C++ around 1995. Even though the favored programming language was ISO C at Interplay Entertainment at the time, M.A.X. developers took the risks and realized most of the game specific functionality using C++. Most of the game objects are stored in custom made smart containers like smart lists or smart arrays that were templates. Serialization of such smart objects is handled by smart file managers. Exception handling was not fully supported by compilers at the time either. Failing freestore memory allocations simply returned a null pointer which required specific error handling that does not work together with exceptions. Run-Time Type Information (RTTI) was also just a draft and was not fully supported by compilers.

The massive amount of polymorphism that is involved with all the custom object containers and classes that encapsulate such polymorphic containers basically make it impossible to follow an incremental reimplementation approach that facilitates testing. All this means that reimplementation of the C++ part of the game follows the big bang approach. After finishing with all the reimplementation work a long testing period starts that attempts to verify that the original and the reimplemented functions behave comparably.

- <span class="legend-done">
  Reimplement the AI movement classes (ai_move.cpp)
  </span>
- <span class="legend-done">
  Reimplement the computer player classes (ai_player.cpp)
  </span>
- <span class="legend-done">
  Reimplement the AI generic classes (ai_main.cpp, ai.cpp)
  </span>
- <span class="legend-done">
  Reimplement the AI exploration classes (ai_explr.cpp)
  </span>
- <span class="legend-done">
  Reimplement the AI builder classes (ai_build.cpp)
  </span>
- <span class="legend-done">
  Reimplement the AI combat classes (ai_attk.cpp)
  </span>
- <span class="legend-done">
  Reimplement the GUI manager classes (commo.cpp)
  </span>
- <span class="legend-done">
  Reimplement the resource manager classes (resrcmgr.cpp)
  </span>
- <span class="legend-done">
  Reimplement the FLIC manager classes (flicsmgr.cpp)
  </span>
- <span class="legend-done">
  Reimplement the ini file manager classes (inifile.cpp)
  </span>
- <span class="legend-done">
  Reimplement the smart C string manager classes (strobj.cpp)
  </span>
- <span class="legend-done">
  Reimplement the message manager classes (mssgsmgr.cpp)
  </span>
- <span class="legend-done">
  Reimplement the path finding classes (paths.cpp)
  </span>
- <span class="legend-close">
  Reimplement the sound manager classes (soundmgr.cpp)
  </span>
- <span class="legend-done">
  Reimplement the unit values (attribs) classes (unitvalues.cpp)
  </span>
- <span class="legend-done">
  Reimplement the complex (building complexes) classes (complex.cpp)
  </span>
- <span class="legend-done">
  Reimplement the units manager classes (unitsmgr.cpp)
  </span>
- <span class="legend-done">
  Reimplement the unit info manager classes (unitinfo.cpp)
  </span>
- <span class="legend-done">
  Reimplement the game manager classes (gamemgr.cpp)
  </span>
- <span class="legend-done">
  Reimplement the map manager classes (drawmap.cpp)
  </span>
- <span class="legend-done">
  Reimplement the map hash classes (hash.cpp)
  </span>
- <span class="legend-done">
  Reimplement the access classes (access.cpp)
  </span>
- <span class="legend-close">
  Reimplement the remote classes (remote.cpp)
  </span>
- <span class="legend-done">
  Reimplement the reports classes (reports.cpp)
  </span>
- <span class="legend-done">
  Reimplement the smart pointer classes (smartptr.cpp)
  </span>
- <span class="legend-done">
  Reimplement the smart array classes (smrtarry.cpp)
  </span>
- <span class="legend-done">
  Reimplement the smart file classes (smrtfile.cpp)
  </span>
- <span class="legend-done">
  Reimplement the smart list classes (smartlst.cpp)
  </span>
- <span class="legend-done">
  Reimplement the textfile classes (textfile.cpp)
  </span>
<br>
<br>
- <span class="legend-inwork">
  Verify the correctness of the reimplemented C++ modules (fix reimplementation issues).
  </span>

### 0.7 - 1.0 and beyond
To be defined

{% comment %}
Code cleanup
- setup workflows to improve code quality
- fix original game defects (retain backwards compatibility with save game format version 70)
- implement missing text mode class serializers and enumerator constants or remove these incomplete features as-is (preferred)
- implement AI debug and log modules or remove these incomplete features as-is (preferred)
- make smart containers reentrant, thread safe and improve ISO C++ compliance
- create or use an audio library that supports loop points (SFX) and streaming (MVE) for digital samples
- refactor MVE player to eliminate self modifying code from it
- make GNW and MVE player standalone libraries

Internationalization
- support utf8 glyphs
- support vector fonts
- support keyboard locales
- support custom key bindings
- support multiple languages
- support subtitles in MVE video clips
- develop translation tools

Cross-platform
- support OS specific line delimiters
- support long file system paths and utf8 file names
- support OS specific save game locations
- support both 32 and 64 bit builds
- support game install from original CD-ROMs (reimplement the MS-DOS installer)

Scenario and campaign editor
- support arbitrary number of save game slots
- support arbitrary number of missions (generalize and externalize mission win & loss conditions)
- support arbitrary number of map tile sets
- support arbitrary map sizes
- add built-in pseudo random map generator
- add built-in scenario and campaign editor

Asset development
- support a high fidelity, lossless audio format that supports loop points (e.g. FLAC with RIFF chunks)
- complete the GIMP plugin
- create a res file encoder and decoder
- create tools to generate the user manual
- create tools to generate map tile sets (including water caustics)

Quality of life improvements
- support mouse wheel and middle mouse button
- support higher resolutions and wide screens (1024x768, 1024x576, 1920x1080)
- support bigger, easier to read UI on higher resolutions
- calibrate monitor brightness

New features
- support various CRT shaders
- improve computer players
- support multiplayer (remote) games against computer players
- port back adaptive music from M.A.X. 2
- port back game mode specific UI variations from M.A.X. 2
- port back colorized map accessibility indicators from M.A.X. 2
- port back alliances feature from M.A.X. 2
- port back 6 players feature from M.A.X. 2
- port back line of sight feature from M.A.X. 2
- port back real-time mode with interrupt (pause) feature from M.A.X. 2
- port back waypoints feature from M.A.X. 2
- port back advanced group movement feature from M.A.X. 2
{% endcomment %}
