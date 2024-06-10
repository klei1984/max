---
layout: page
title: Roadmap
permalink: /roadmap/
---

This article tries to maintain a high level overview of the work packages and challenges that need to be solved to step by step complete the port.

Last updated: 2024-06-01.

Reimplementation status: 5704 / 5704 (100%) functions.

The list is subject to change at any time. The outlined order of work packages, priorities, could be rearranged depending on the difficulty, available time or available help from others. I am new to many of the GitHub and open source toolings and hope to get help from friendly enthusiasts. Obvious work packages like fix all software defects identified and such are not mentioned explicitly in the list.

Items in the list are color-coded using the following interpretation:
- <span class="legend-done">Feature Complete</span> (all done but subject to bug fixes and future refinements)
- <span class="legend-close">Mostly Complete</span> (almost there, could be waiting on another system)
- <span class="legend-inwork">In Progress</span> (work started but still has some way to go)
- Not Started (work is yet to begin on this feature)


### 1 Build toolchain
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

### 2 Mock up basic OS dependent libraries and services
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

### 3 Reimplement original platform library modules
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
- <span class="legend-done">
  Replace the IPX driver and related dpmi modules (ipx, dpmi)
  </span>

### 4 Reimplement original MVE library modules
The original MVE library was implemented in C and Assembly languages using the Watcom C/C++ compiler. The library uses its own version of MS-DOS video and audio drivers. The library also relies on self modifying code which means that the assembly subroutines are modified on the fly by themselves. This makes the original library very difficult to port to different compiler or processor architectures. The audio data streams are interleaved with the video data streams and the audio frame rate is synchronized with the video frame rate. On low end computers this allows video frames to be skipped to be able to keep up with the audio. A choppy video stream is considered less annoying than a skipping audio stream. The library supported further performance optimization techniques like decreasing the resolution of the rendered video stream or interlacing the output effectively skipping every second horizontal line of the video data. The library itself would support closed captions too, but M.A.X. did not utilize this feature. The workflow within this package is similar to the one in package 0.3 except for the OS dependent drivers layer and the assembler subroutines.

- <span class="legend-done">
  Replace the low level OS dependent drivers (sos, vbe, ...)
  </span>
- <span class="legend-done">
  Reimplement the MVE library modules (mvelib32, mveliba)
  </span>
- <span class="legend-done">
  Implement feature to scale videos to match screen resolution
  </span>

### 5 Study Watcom C++, analyse original C++ classes and develop necessary tools
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

### 6 Reimplement original C++ classes
The first ISO C++ standard was released only in 1998. Standardization took many years and compiler vendors usually lagged behind to follow up the latest published draft papers. Based on various Dr. Dobb’s Journal articles, smart pointers appeared as a concept in C++ around 1995. Even though the favored programming language was ISO C at Interplay Productions at the time, M.A.X. developers took the risks and realized most of the game specific functionality using C++. Most of the game objects are stored in custom made smart containers like smart lists or smart arrays that were templates. Serialization of such smart objects is handled by smart file managers. Exception handling was not fully supported by compilers at the time either. Failing freestore memory allocations simply returned a null pointer which required specific error handling that does not work together with exceptions. Run-Time Type Information (RTTI) was also just a draft and was not fully supported by compilers.

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
- <span class="legend-done">
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
- <span class="legend-done">
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
- <span class="legend-done">
  Verify the correctness of the reimplemented C++ modules (fix reimplementation issues).
  </span>

### 7 Localization
M.A.X. utilizes GNW's raster font manager and implements six fonts, but uses only three of them. The raster fonts' em size is optimized for 640 x 480 screen resolution. The fonts are encoded according to code page 437. Half of the glyphs are not implemented from the code page while others are not recognizable. The fonts themselves are monochromatic and support no opacity contrary to some of the fonts that Fallout used.
To support as many languages as possible and eventually high DPI displays and much higher resolutions the FreeType library is used for text rendering. Three new TrueType fonts are created and the font manager is redesigned to work with unicode vector fonts while keeping the ability to draw multi color shaded glyphs via GNW’s Text API to keep that old genuine look and feel. To be able to support multiple languages a locale manager is realized and language dependent texts are externalized as they are all hard coded within the source code.
In-game hints, in-game help and some other text, like mission briefings or planet descriptions, are handled by the game on a per use case basis. For example all missions and planets are hard coded. It is not possible to just add new planets that the game would recognize. Scenarios are just saved games that store chat and other system messages using the locale that was active at the time of creation. Solving these problems and supporting multiple voice overs are out of the scope of this work package.

- <span class="legend-done">
  Support TrueType fonts
  </span>
- <span class="legend-done">
  Support utf-8 encoded glyphs
  </span>
- Support keyboard locales
- Support custom key bindings
- <span class="legend-done">
  Support multiple languages
  </span>
- Support subtitles in MVE video clips
- <span class="legend-done">
  Externalize hard coded language dependent text
  </span>
- Develop translation tools

### 8 Code cleanup
- <span class="legend-inwork">
  Setup workflows to improve code quality
  </span>
- <span class="legend-inwork">
  Fix original game defects and remaining reimplementation issues (retain backwards compatibility with save game format version 70)
  </span>
- <span class="legend-done">
  Implement missing text mode class serializers and enumerator constants or remove these incomplete features as-is (preferred)
  </span>
- <span class="legend-done">
  Implement AI debug and log modules
  </span>
- <span class="legend-inwork">
  Make smart containers reentrant, thread safe and improve ISO C++ compliance
  </span>
- <span class="legend-done">
  Use an audio library that supports loop points (SFX) and streaming (MVE) for digital samples
  </span>
- <span class="legend-done">
  Refactor MVE player to eliminate self modifying code from it
  </span>

### 9 - and beyond
To be defined

{% comment %}
Cross-platform
- support OS specific line delimiters
- support long file system paths and utf8 file names
- support OS specific save game locations
- <span class="legend-done">
  support both 32 and 64 bit builds
  </span>
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
