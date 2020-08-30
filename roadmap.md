---
layout: page
title: Roadmap
permalink: /roadmap/
---

This article tries to maintain a high level overview of the work packages and challenges that need to be solved to step by step complete the port.

Last updated: 2020-08-30.

The list is subject to change at any time. The outlined order of work packages, priorities, could be rearranged depending on the difficulty, available time or available help from others. I am new to many of the Github and open source toolings and hope to get help from friendly enthusiasts. Obvious work packages like fix all software defects identified and such are not mentioned explicitly in the list.

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
- Document build toolchain

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
- <span class="legend-done">
  Remove all replaced functions from maxrun.in
  </span>

### 0.3 Reimplement original GNW library modules
The original GNW library was implemented in pure C. The workflow within this package includes steps to
 reenginer the original GNW modules based on available information into ISO C99 source code,
 replace references to the original GNW API calls within maxrun.in by the reimplemented versions,
 and finally remove the replaced old assembler code from maxrun.in.

- <span class="legend-done">
  Replace the human machine interface related modules (kb, mouse, vcr, input)
  </span>
- <span class="legend-inwork">
  Replace the color management module (color)
  </span>
- <span class="legend-done">
  Replace the GUI controls related modules (gnw, rect, grbuf, interface, button)
  </span>
- <span class="legend-done">
  Replace the memory management module (memory)
  </span>
- <span class="legend-done">
  Replace the debug services module (debug)
  </span>
- <span class="legend-done">
  Replace the database management module (db, assoc)
  </span>
- <span class="legend-close">
  Remove all replaced functions from maxrun.in
  </span>

### 0.4 Reimplement original MVE library modules
The original MVE library was implmeneted in C and Assembly. The MVE library uses its own version of MS-DOS svga and sound drivers.
The workflow within this package is similar to the one in package 0.3 except for the OS dependent drivers layer.

- Replace the low level OS dependent drivers (sos, vbe, ...)
- Replace the MVE library modules (mvelib32, mveliba)
- <span class="legend-none">
  Remove all replaced functions from maxrun.in
  </span>

### 0.5 Reimplement Game Menu loops
Most of the menus are implemented in C, but they use several C++ resources. Original module boundaries still need to be determined.

- Reimplement game menu loops (main, clan select, planet select, etc.)
- Reimplement game menu specific on-click event handlers
- <span class="legend-none">
  Remove all replaced functions from maxrun.in
  </span>

### 0.6 Reimplement original C++ modules
Most of the game itself was implemented in C++. Even though the Watcom C/C++ compiler became open source it is still very difficult to fully understand how the original compiler organizes constructor, destructor, operator overload and such behind the scenes C++ runtime specific data. Clear understanding of the Watcom C++ runtime library and related data that the compiler emits into executables is a prerequisit to be able to reimplement the C++ modules from the game.

- <span class="legend-inwork">
  Study Open Watcom V1.1.0 source code and set up test environment
  </span>
- Build reference classes with Watcom V10.5 and/or Open Watcom V1.1.0 with debug information to learn how C++ library specific meta data are emitted by the compiler and used by the C++ runtime library (plib3r.lib).
- Develop tools to automate the binding of original C++ method calls to reimplemented C++ class members.
<br><br>
- Replace the AI modules (ai.cpp, ai_build.cpp, ai_main.cpp, ai_move.cpp, ai_playr.cpp, ai_explr.cpp, ai_attk.cpp)
- Replace the context menu manager module (commo.cpp)
- <span class="legend-inwork">
  Replace the resource manager module (resrcmgr.cpp)
  </span>
- Replace the FLIC manager module (flicsmgr.cpp)
- Replace the ini file manager module (inifile.cpp)
- <span class="legend-inwork">
  Replace the smart C string manager module (strobj.cpp)
  </span>
- Replace the message manager module (mssgsmgr.cpp)
- Replace the path finding module (paths.cpp)
- <span class="legend-inwork">
  Replace the sound manager module (soundmgr.cpp)
  </span>
- Replace the units manager module (unitsmgr.cpp)
- Replace the unit info manager module (unitinfo.cpp)
- Replace the game manager module (gamemgr.cpp)
- Replace the map manager module (drawmap.cpp)
- Replace the map hash module (hash.cpp)
- Replace the access module (access.cpp)
- Replace the remote module (remote.cpp)
- Replace the reports module (reports.cpp)
- Replace the smart memory manager modules (smartlst.cpp, smartptr.cpp, smrtarry.cpp, smrtfile.cpp)
- Replace the textfile module (textfile.cpp)

### 0.7 - 1.0 and beyond
To be defined
