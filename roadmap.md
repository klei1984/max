---
layout: page
title: Roadmap
permalink: /roadmap/
---

This article tries to maintain a high level overview of the work packages and challenges that need to be solved to step by step complete the port.

Last updated: 2021-11-14.

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
- <span class="legend-done">
  Remove all replaced functions from maxrun.in
  </span>

### 0.3 Reimplement original platform library modules
The original platform library was implemented in pure C. The workflow within this package includes steps to
 reenginer the original GNW and related modules based on available information into ISO C99 source code,
 replace references to the original API calls within maxrun.in by the reimplemented versions,
 and finally remove the replaced old assembler code from maxrun.in.

- <span class="legend-done">
  Replace the human machine interface related modules (kb, mouse, vcr, input)
  </span>
- <span class="legend-done">
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
- <span class="legend-inwork">
  Replace the IPX driver and related dpmi modules (ipx, dpmi)
  </span>
- <span class="legend-close">
  Remove all replaced functions from maxrun.in
  </span>

### 0.4 Reimplement original MVE library modules
The original MVE library was implmeneted in C and Assembly. The MVE library uses its own version of MS-DOS svga and sound drivers.
The workflow within this package is similar to the one in package 0.3 except for the OS dependent drivers layer.

- <span class="legend-inwork">
  Replace the low level OS dependent drivers (sos, vbe, ...)
  </span>
- <span class="legend-inwork">
  Replace the MVE library modules (mvelib32, mveliba)
  </span>
- Implement feature to scale videos to match screen resolution
- Implement feature to render captions (subtitles)
- <span class="legend-none">
  Remove all replaced functions from maxrun.in
  </span>

### 0.5 Study Watcom C++, analyse original C++ classes and develop necessary tools
Most of the game itself was implemented in C++. Even though the Watcom C/C++ compiler became open source it is still very difficult to fully understand how the original compiler organizes constructor, destructor, operator overload and such behind the scenes C++ runtime specific data. Clear understanding of the Watcom C++ runtime library and related data that the compiler emits into executables is a prerequisit to be able to identify and understand the C++ class hierarchies from the game.

- <span class="legend-close">
  Study Watcom V11.0b and Open Watcom V1.1.0 source code and set up test environment
  </span>
- <span class="legend-inwork">
  Build reference classes with Watcom V10.5 and/or Open Watcom V1.1.0 with debug information to learn how C++ library specific meta data are emitted by the compiler and used by the C++ runtime library (plib3r.lib).
  </span>
- <span class="legend-inwork">
  Document findings in an article
  </span>
- Discover, analyse and document original class hierarchies
- Develop tools to automate the binding of original C++ method calls to reimplemented C++ class members.
<br><br>

### 0.6 Reimplement original C++ classes
- Replace the AI classes (task_manager.cpp, ai.cpp, ai_build.cpp, ai_main.cpp, ai_move.cpp, ai_playr.cpp, ai_explr.cpp, ai_attk.cpp)
- <span class="legend-inwork">
  Replace the GUI manager classes (commo.cpp)
  </span>
- <span class="legend-close">
  Replace the resource manager classes (resrcmgr.cpp)
  </span>
- <span class="legend-done">
  Replace the FLIC manager classes (flicsmgr.cpp)
  </span>
- <span class="legend-done">
  Replace the ini file manager classes (inifile.cpp)
  </span>
- <span class="legend-done">
  Replace the smart C string manager classes (strobj.cpp)
  </span>
- Replace the message manager classes (mssgsmgr.cpp)
- Replace the path finding classes (paths.cpp)
- <span class="legend-close">
  Replace the sound manager classes (soundmgr.cpp)
  </span>
- <span class="legend-done">
  Replace the unit values (attribs) classes (unitvalues.cpp)
  </span>
- <span class="legend-inwork">
  Replace the complex (building complexes) classes (complex.cpp)
  </span>
- <span class="legend-inwork">
  Replace the units manager classes (unitsmgr.cpp)
  </span>
- <span class="legend-inwork">
  Replace the unit info manager classes (unitinfo.cpp)
  </span>
- Replace the game manager classes (gamemgr.cpp)
- Replace the map manager classes (drawmap.cpp)
- <span class="legend-done">
  Replace the map hash classes (hash.cpp)
  </span>
- Replace the access classes (access.cpp)
- Replace the remote classes (remote.cpp)
- Replace the reports classes (reports.cpp)
- <span class="legend-done">
  Replace the smart pointer classes (smartptr.cpp)
  </span>
- <span class="legend-done">
  Replace the smart array classes (smrtarry.cpp)
  </span>
- <span class="legend-done">
  Replace the smart file classes (smrtfile.cpp)
  </span>
- <span class="legend-done">
  Replace the smart list classes (smartlst.cpp)
  </span>
- <span class="legend-done">
  Replace the textfile classes (textfile.cpp)
  </span>

### 0.7 - 1.0 and beyond
To be defined
