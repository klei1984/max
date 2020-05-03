---
layout: page
title: Roadmap
permalink: /roadmap/
---

This article tries to maintain a high level overview of the work packages and challenges that need to be solved to step by step complete the port.

Last updated: 2020-05-03.

The list is subject to change at any time. The outlined order of work packages, priorities, could be rearranged depending on the difficulty, available time or available help from others. I am new to many of the Github and open source toolings and hope to get help from friendly enthusiasts. Obvious work packages like fix all software defects identified and such are not mentioned explicitly in the list.

Items in the list are color-coded using the following interpretation:
- <span class="legend-done">Feature Complete</span> (all done but subject to bug fixes and future refinements)
- <span class="legend-close">Mostly Complete</span> (almost there, could be waiting on another system)
- <span class="legend-inwork">In Progress</span> (work started but still has some way to go)
- Not Started (work is yet to begin on this feature)


### 0.1 (Build toolchain)
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

### 0.2 (Mock up basic OS dependent libraries and services)
- <span class="legend-inwork">
  Replace the Watcom Standard C/C++ library
  </span>
- <span class="legend-inwork">
  Replace or stub DPMI memory and file server services
  </span>
- <span class="legend-inwork">
  Mock up VBE / VESA driver using SDL2 video
  </span>
- <span class="legend-inwork">
  Mock up Mouse using SDL2 mouse
  </span>
- <span class="legend-inwork">
  Mock up Keyboard using SDL2 keyboard
  </span>

### 0.3 (Make it sound)
- <span class="legend-inwork">
  Reimplement HMI SOS API interface (low level layer) or directly the game's own sound manager interface (high level layer)
  </span>
- Integrate SDL_mixer for both music and sound effects playback

### 0.4 (Reimplement DOS game loop with a modern event loop)
- Replace the DOS game loop with an SDL2 event loop

### 0.5 (Enable built in advanced debug services - if possible)
- Enable built in advanced debug services to log AI task manager activities and unit info

### 0.6 (Make it networking)
- Replace the various network modes of the game by SDL_net

### 0.7 (Move to a 32 / 64 bit code base)
- Remove the 16 bit code segment (currently required by original network code)
- Replace or stub all the remaining real mode direct interrupt or memory mapped resource accesses
- Make the game code 32 / 64 bit compliant
- Migrate the build toolchain to a 64 bit stock GCC or comparable basis (due to 16 bit code only 32 bit build environment is viable currently)

### 0.8 (Split the assembly code into components along original module boundaries)
- Split out each static link library from the core assembler source file
- Remove/replace all obsolete static link library code dependencies (e.g. standard C/C++ library, VBE driver, ...)
- Split out game specific components from the core assembler source file

### 0.9 (Reimplement individual assembler components in modern cross-platform ISO C/C++ as needed)
- Replace the keyboard driver
- Replace the mouse driver
- Replace the VESA driver
- Replace VBE driver
- Replace HMI SOS
- Reimplement GNW library (debug support, windowing, memory manager, ...)
- Reimplement the color palette manager library
- Reimplement the database library
- Reimplement the MVE library
- Reimplement VCR (input recorder, player library)
- Reimplement the game specific components

### 1.0 (Convert game into a modern multi-threaded application to balance performance)
- Refactor code into various threads

### 1.x (Enhance certain game services without affecting game play logic)
- Support high screen resolutions
- Support virtually limitless number of sound channels
- Add support for configurable key bindings
