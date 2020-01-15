---
layout: page
title: Defects
permalink: /defects/
---

The article maintains a comprehensive list of game defects that is present in the original M.A.X. v1.04 (English) runtime.

1. **\*\*FIXED\*\*** M.A.X. is a 16/32 bit mixed linear executable that is bound to a dos extender stub from Tenberry Software called DOS/4G*W* 1.97. The W in the extender's name stands for Watcom which is the compiler used to build the original M.A.X. executable. A list of defects found in DOS/4GW 1.97 can be found in the [DOS/4GW v2.01 release notes](https://web.archive.org/web/20180611050205/http://www.tenberry.com/dos4g/watcom/rn4gw.html). By replacing DPMI service calls and basically the entire DOS extender stub with cross-platform [SDL library](https://wiki.libsdl.org/) the DOS/4GW 1.97 defects could be considered fixed.

2. If the game cannot play the intro movie, INTROFLC (maxint.mve), it tries to load and render a full screen Interplay logo image, ILOGO, which was removed from the game resource database max.res. The game continues gracefully after failing to open the ILOGO resource.
<br>
**\*\*TODO\*\*** The game supports loading game resources from a secondary resource file called patches.res. The file does not exist in v1.04, but could be created to include ILOGO from the interactive demo of M.A.X. where the resource exists.

3. **\*\*FIXED\*\*** M.A.X. utilizes a user interface and OS-abstraction library called GNW for windowing, context menus, file system access, memory management, debug services, database management, human interface input recording and playback and many more. Several game menus create list controls for unit selection. When such list controls hold no items, the lists are empty, out of bounds access could occur. Based on dosbox debug sessions such accesses read random memory contents within low address space memory areas and operations conclude gracefully without crashes on DOS systems. On modern operating systems such out of bounds accesses cause segmentation faults and the game crashes immediately.

4. **\*\*FIXED\*\*** M.A.X. allocates 5 player objects. The fifth player is used for alien derelicts - *citation needed*. Certain init routines initialize arrays holding structures related to player data. Some arrays have four elements while the others have five elements. The structures held in these mixed size arrays are indexed from 4 to 0 in a loop. This creates an out of bounds write access to a totally unrelated memory area for the array that only holds four structure instances. The given structure is 19 bytes long and who knows what gets corrupted by the out of bounds write accesses.

5. **\*\*FIXED\*\*** M.A.X. has several units that were planned and never released for the final game or that were repurposed during game development. There is a mobile unit called the **Master Builder** which can be seen in one of the shorter movie clips of the game and has the following unit description text: *Specialized vehicle which transforms to become a new mining station.* The related game assets, like unit sprites, never made it into the game or they were not even created by the developers at all. Never the less the unit is initialized by one of the C++ constructors and when mandatory game resources are not found a NULL pointer dereference occurs. Like in case of defect 3 the game does not crash with out of bounds access under DOS environment but utterfy fails on modern operating system with a segmentation fault.
The following resources are missing from max.res or patches.res: A_MASTER, I_MASTER, P_MASTER, F_MASTER, S_MASTER, MASTER.

6. The sound manager in M.A.X. handles three different sound sources. Voices, a streaming music track and sound effects. The voice interface always accepts two sound samples. A male and a female voice. Male voice samples are not present in the game but the sound manager API requires resource IDs to be passed to it any ways.
In a certain unclarified game mode when the game concludes with either winning or losing, the sound manager is instructed to play WINR_MSC or LOSE_MSC that are distinct music tracks for winnind and losing. At the same time if the game was won a voice sample is also fed into the sound manager but instead of a femail and a male sample only two identical male samples are passed as arguments to digi_play_voice(). The male voice sample resource id is V_M283 which does not exist. But there is a V_F283 which references sound file F283.spw. The sound file is a female voice sample which says *Mission Successful!*.
<br>
**\*\*TODO\*\*** After figuring out which game mode the offending code belongs to the defect could be fixed if it is proven that the sound is played in a meaningful context when the proper input argument is passed.

7. 