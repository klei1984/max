---
layout: page
title: Roadmap
permalink: /roadmap/
---

This article tries to maintain a high level overview of the work packages and challenges that need to be solved to step by step complete the port.

Last updated: 2022-11-14.

Reimplementation status: 5362 / 5704 (94%) functions.

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
- <span class="legend-close">
  Discover and analyse original class hierarchies
  </span>
<br><br>

### 0.6 Reimplement original C++ classes
- Replace the AI movement classes (ai_move.cpp)
  - <span class="legend-done">PathRequest</span>
  - <span class="legend-done">TaskFindPath</span>
  - <span class="legend-done">TaskPathRequest</span>
  - <span class="legend-done">AdjustRequest</span>
  - <span class="legend-done">TaskMove</span>
  - <span class="legend-done">TaskDump</span>
  - <span class="legend-done">TaskRetreat</span>
  - <span class="legend-done">TaskActivate</span>
  - <span class="legend-done">TaskRendezvous</span>
  - <span class="legend-done">TaskMoveHome</span>
  - <span class="legend-done">TaskAssistMove</span>
  - <span class="legend-done">TaskTransport</span>
<br>
<br>
- Replace the computer player classes (ai_player.cpp)
  - <span class="legend-done">UnitValues</span>
  - <span class="legend-done">TransportOrder</span>
  - <span class="legend-done">SpottedUnit</span>
  - <span class="legend-done">TaskMineAssistant</span>
  - <span class="legend-done">TaskFrontierAssistant</span>
  - <span class="legend-done">TaskUpdateTerrain</span>
  - <span class="legend-done">Continent</span>
  - <span class="legend-done">ContinentFiller</span>
  - <span class="legend-done">ThreatMap</span>
  - <span class="legend-done">TerrainMap</span>
  - AiPlayer
<br>
<br>
- Replace the AI maintenance classes
  - <span class="legend-done">TaskRepair</span>
  - <span class="legend-done">TaskReload</span>
  - <span class="legend-done">TaskUpgrade</span>
<br>
<br>
- Replace the AI classes (ai_main.cpp, ai.cpp)
  - <span class="legend-close">TaskObtainUnits</span>
  - <span class="legend-done">RemindAvailable</span>
  - <span class="legend-done">RemindMoveFinished</span>
  - <span class="legend-done">RemindAttack</span>
  - <span class="legend-done">Zone</span>
  - <span class="legend-inwork">TaskClearZone</span>
  - <span class="legend-done">RemindTurnEnd</span>
  - <span class="legend-done">RemindTurnStart</span>
  - ProductionManager
  - <span class="legend-done">TaskManager</span>
  - <span class="legend-done">TaskGetResource</span>
  - <span class="legend-done">Reminder</span>
  - <span class="legend-close">Task</span>
<br>
<br>
- Replace the AI exploration classes (ai_explr.cpp)
  - <span class="legend-done">TaskSearchDestination</span>
  - <span class="legend-done">TaskSurvey</span>
  - <span class="legend-done">TaskFindMines</span>
  - <span class="legend-done">TaskExplore</span>
  - <span class="legend-done">TaskAutoSurvey</span>
  - <span class="legend-done">TaskAbstractSearch</span>
<br>
<br>
- Replace the AI builder classes (ai_build.cpp)
  - <span class="legend-done">TaskGetMaterials</span>
  - <span class="legend-done">TaskCreateBuilding</span>
  - <span class="legend-inwork">TaskManageBuildings</span>
  - <span class="legend-done">TaskCreateUnit</span>
  - <span class="legend-done">TaskDefenseAssistant</span>
  - <span class="legend-done">TaskRadarAssistant</span>
  - <span class="legend-done">TaskPowerAssistant</span>
  - <span class="legend-done">TaskHabitatAssistant</span>
  - <span class="legend-done">TaskConnectionAssistant</span>
  - <span class="legend-done">TaskRemoveRubble</span>
  - <span class="legend-done">TaskRemoveMines</span>
  - <span class="legend-done">TaskScavenge</span>
  - <span class="legend-done">SiteMarker</span>
  - <span class="legend-done">FloodRun</span>
  - <span class="legend-done">MAXFloodFill</span>
  - <span class="legend-done">TaskCreate</span>
<br>
<br>
- Replace the AI combat classes (ai_attk.cpp)
  - <span class="legend-done">DefenseManager</span>
  - <span class="legend-done">TaskAttackReserve</span>
  - <span class="legend-done">TaskKillUnit</span>
  - <span class="legend-done">TaskCheckAssaults</span>
  - <span class="legend-done">TaskDefenseReserve</span>
  - <span class="legend-done">TaskWaitToAttack</span>
  - <span class="legend-done">TaskFrontalAttack</span>
  - <span class="legend-done">TaskPlaceMines</span>
  - <span class="legend-done">TaskSupportAttack</span>
  - <span class="legend-done">TaskAttack</span>
  - <span class="legend-done">TaskEscort</span>
  - <span class="legend-done">AccessMap</span>
  - <span class="legend-done">WeightTable</span>
  - <span class="legend-done">TransporterMap</span>
<br>
<br>
- <span class="legend-done">
  Replace the GUI manager classes (commo.cpp)
  </span>
- <span class="legend-done">
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
- <span class="legend-done">
  Replace the message manager classes (mssgsmgr.cpp)
  </span>
- <span class="legend-done">
  Replace the path finding classes (paths.cpp)
  </span>
- <span class="legend-close">
  Replace the sound manager classes (soundmgr.cpp)
  </span>
- <span class="legend-done">
  Replace the unit values (attribs) classes (unitvalues.cpp)
  </span>
- <span class="legend-close">
  Replace the complex (building complexes) classes (complex.cpp)
  </span>
- <span class="legend-inwork">
  Replace the units manager classes (unitsmgr.cpp)
  </span>
- <span class="legend-inwork">
  Replace the unit info manager classes (unitinfo.cpp)
  </span>
- <span class="legend-done">
  Replace the game manager classes (gamemgr.cpp)
  </span>
- <span class="legend-done">
  Replace the map manager classes (drawmap.cpp)
  </span>
- <span class="legend-done">
  Replace the map hash classes (hash.cpp)
  </span>
- <span class="legend-done">
  Replace the access classes (access.cpp)
  </span>
- <span class="legend-inwork">
  Replace the remote classes (remote.cpp)
  </span>
- <span class="legend-done">
  Replace the reports classes (reports.cpp)
  </span>
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
