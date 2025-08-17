---
layout: page
title: Roadmap
permalink: /roadmap/
---

This article tries to maintain a high level overview of the work packages and challenges that need to be solved to step by step complete the port.

Last updated: 2025-08-17.

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

### 7 Support multiple languages
M.A.X. utilizes GNW's raster font manager and implements six fonts, but uses only three of them. The raster fonts' em size is optimized for 640 x 480 screen resolution. The fonts are encoded according to code page 437. Half of the glyphs are not implemented from the code page while others are not recognizable. The fonts themselves are monochromatic and support no opacity contrary to some of the fonts that Fallout used.
To support as many languages as possible and eventually high DPI displays and much higher resolutions the FreeType library is used in combination with utf8proc for text rendering. Three new TrueType fonts are created and the font manager is redesigned to work with unicode vector fonts while keeping the ability to draw multi color shaded glyphs via GNW’s Text API to keep that old genuine look and feel. To be able to support multiple languages a locale manager is realized and language dependent texts are externalized as they are all hard coded within the source code. In-game hints, in-game help and some other text, like mission briefings or planet descriptions, are handled by the game on a per use case basis. For example all mission and planet descriptions are hard coded. It is not possible to just add new planets that the game would recognize. Scenarios are just saved games that store chat and other system messages using the locale that was active at the time of creation.

- <span class="legend-done">
  Support TrueType fonts
  </span>
- <span class="legend-done">
  Support utf-8 encoded glyphs
  </span>
- Support keyboard locales
- Support custom key bindings
- <span class="legend-inwork">
  Support multiple languages
  </span>
- Support subtitles in MVE video clips
- <span class="legend-inwork">
  Externalize hard coded language dependent text
  </span>
- Develop translation tools

### 8 Improve code quality
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

### 9 Support multiple operating systems
Each operating system expects user specific files to be stored in a specific location following an OS distributor specific folder structure and file system standard. Each operating system has unique file system quirks that makes interoperability difficult. For example CD-ROMs could be mounted using ISO 9660 extensions like Rock Ridge, Joilet. One file system is case sensitive another is case insensitive. One supports utf8 encoded object names, another does not.

- <span class="legend-inwork">
  Support OS specific line delimiters
  </span>
- <span class="legend-inwork">
  Support long file system paths and utf8 file names
  </span>
- <span class="legend-done">
  Support OS specific save game locations
  </span>
- <span class="legend-close">
  Support both 32 and 64 bit builds
  </span>
- <span class="legend-close">
  Support game install from original media
  </span>

### 10 Support sandboxed scripting
Many missions have hard coded game rules, while configurability of normal custom missions is lacking. Main goal is to establish the framework to allow mission specific win & loss conditions, build and manufacturing capability lists, music scheduling and generation of in-game events.

- <span class="legend-close">
  Integrate a lua interpreter
  </span>
- <span class="legend-close">
  Expose interfaces to instanced lua contexts
  </span>
- Document the scripting engine capabilities in a programmer's manual

### 11 Support more missions
The list of missions and their game rules are all hard coded into the game executables. The English executable hardcodes English mission descriptions as each original MS-DOS executable used their own language specific MS-DOS code pages in text files. Main goal of this work package is to allow content creators to add new missions and to expand the versatility of missions via scripting and story telling.

- <span class="legend-inwork">
  Support arbitrary number of save game slots (9999 to be more precise)
  </span>
- <span class="legend-close">
  Support arbitrary number of missions
  </span>
- <span class="legend-close">
  Generalize and externalize mission win & loss conditions
  </span>
- <span class="legend-close">
  Generalize and externalize mission construction and manufacturing capability rules
  </span>
- <span class="legend-close">
  Externalize mission briefings
  </span>
- <span class="legend-close">
  Refactor user interfaces as necessary
  </span>

### 12 Support more worlds
The list of worlds and their descriptions are all hard coded into the game executables. The MS-DOS version only supports map sizes of 112 by 112 grid cells. The color schemes are hard coded for the four tile sets. The palette color animation cycles and timings are all common and hardcoded. WRL files self contain the tile set and the system palette.

- <span class="legend-done">
  Support arbitrary map sizes (within sane limits)
  </span>
- Support arbitrary number of world tile sets
- Generalize and externalize world color palette animation rules
- Externalize world titles, descriptions and similar text
- Introduce a new world file format
- Refactor user interfaces as necessary

### 13 Update the save file format to version 71
- <span class="legend-inwork">
  Develop new save file format
  </span>
- <span class="legend-inwork">
  Fix original defects caused by save file format version 70 quirks and limitations
  </span>
- <span class="legend-inwork">
  Fix original defects in original missions that were produced with save file format version 70
  </span>
- <span class="legend-inwork">
  Expand parameter limits to allow "infinite" progressions
  </span>
- Remove language dependencies from the message manager database

### 14 Replace the GNW engine
GNW is a 30 years old technology. Goal is to make the graphical user interfaces high DPI aware, support sane monitor aspect ratios out of the box, be highly flexible, preferably scriptable and render the assets using a modern GPU render pipeline. The game has two distinct rendering modes. System menus, and in-game menus with tactical map rendering.

- Switch to SDL3 backend
- Integrate the RmlUI library
- Develop GL 3.3 and GLES 3.0 rendering pipelines and relevant shaders
- Reimplement and redesign system menus
- Reimplement and redesign in-game menus and tactical map rendering
- Remove the GNW engine

### 15 Support more campaigns
The M.A.X. 1 campaign is very short and the campaign progression is linear. Main goals are to be able to add new campaigns, to make use of the extended game rules for missions, and to add branching to campaign progression.

- Support arbitrary number of campaigns
- Generalize and externalize campaign briefings
- Generalize and externalize campaign specific mission and progression rules
- Refactor user interfaces as necessary

### 16 - and beyond
To be defined

{% comment %}
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
- add built-in pseudo random map generator
- add built-in scenario and campaign editor
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
