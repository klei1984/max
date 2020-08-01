---
layout: page
title: Defects
permalink: /defects/
---

The article maintains a comprehensive list of game defects that is present in the original M.A.X. v1.04 (English) runtime.

1. **\*\*FIXED\*\*** M.A.X. is a 16/32 bit mixed linear executable that is bound to a dos extender stub from Tenberry Software called DOS/4G*W* 1.97. The W in the extender's name stands for Watcom which is the compiler used to build the original M.A.X. executable. A list of defects found in DOS/4GW 1.97 can be found in the [DOS/4GW v2.01 release notes](https://web.archive.org/web/20180611050205/http://www.tenberry.com/dos4g/watcom/rn4gw.html). By replacing DPMI service calls and basically the entire DOS extender stub with cross-platform [SDL library](https://wiki.libsdl.org/) the DOS/4GW 1.97 defects could be considered fixed.

2. **\*\*FIXED\*\*** If the game cannot play the intro movie, INTROFLC (maxint.mve), it tries to load and render a full screen Interplay logo image, ILOGO, which was removed from the game resource database max.res. The game continues gracefully after failing to open the ILOGO resource.
<br>
The game supports loading game resources from a secondary resource file called patches.res. The file does not exist in v1.04, but when created the contents of it are processed as expected. ILOGO is taken from the interactive demo of M.A.X. where the resource exists.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_2.mp4" type="video/mp4">
    <track label="English" kind="subtitles" srclang="en" src="{{ site.baseurl }}/assets/clips/defect_2.vtt" default>
    </video>

3. **\*\*FIXED\*\*** M.A.X. utilizes a user interface and OS-abstraction library called GNW for windowing, context menus, file system access, memory management, debug services, database management, human interface input recording and playback and many more. Several game menus create list controls for unit selection. When such list controls hold no items, the lists are empty, out of bounds access could occur. Based on dosbox debug sessions such accesses read random memory contents within low address space memory areas and operations conclude gracefully without crashes on DOS systems. On modern operating systems such out of bounds accesses cause segmentation faults and the game crashes immediately.

4. **\*\*FIXED\*\*** M.A.X. allocates 5 player objects. The fifth player is used for alien derelicts - *citation needed*. Certain init routines initialize arrays holding structures related to player data. Some arrays have four elements while the others have five elements. The structures held in these mixed size arrays are indexed from 4 to 0 in a loop. This creates an out of bounds write access to a totally unrelated memory area for the array that only holds four structure instances. The given structure is 19 bytes long and who knows what gets corrupted by the out of bounds write accesses.

5. **\*\*FIXED\*\*** M.A.X. has several units that were planned and never released for the final game or that were repurposed during game development. There is a mobile unit called the **Master Builder** which can be seen in one of the shorter movie clips of the game and has the following unit description text: *Specialized vehicle which transforms to become a new mining station.* The related game assets, like unit sprites, never made it into the game or they were not even created by the developers at all. Never the less the unit is initialized by one of the C++ constructors and when mandatory game resources are not found a NULL pointer dereference occurs. Like in case of defect 3 the game does not crash with out of bounds access under DOS environment but utterfy fails on modern operating system with a segmentation fault.
The following resources are missing from max.res or patches.res: A_MASTER, I_MASTER, P_MASTER, F_MASTER, S_MASTER, MASTER.

6. The sound manager in M.A.X. handles three different sound sources. Voices, a streaming music track and sound effects. The voice interface always accepts two sound samples. A male and a female voice. Male voice samples are not present in the game but the sound manager API requires resource IDs to be passed to it any ways.
In a certain unclarified game mode when the game concludes with either winning or losing, the sound manager is instructed to play WINR_MSC or LOSE_MSC that are distinct music tracks for winnind and losing. At the same time if the game was won a voice sample is also fed into the sound manager but instead of a femail and a male sample only two identical male samples are passed as arguments to digi_play_voice(). The male voice sample resource id is V_M283 which does not exist. But there is a V_F283 which references sound file F283.spw. The sound file is a female voice sample which says *Mission Successful!*.

7. **\*\*FIXED\*\*** The load_font() function in GNW's text.c module leaks a file handle in case the requested amount of character descriptors cannot be read from the font file.

8. **\*\*FIXED\*\*** The text_remove_manager() function in GNW's text.c module could perform out of bounds read access to an array of structures in corner cases.

9. **\*\*FIXED\*\*** The text_remove_manager() function in GNW's text.c module is supposed to remove a previously created text manager instance. Instead of linked lists and dynamic memory allocation the module supports maximum ten managers that could be stored in a preallocated array. E.g. to remove a manager from array position 3 the function would simply shift managers 4-9 to positions 3-8 overwriting position 3 in the process. The function uses memmove() which allows overlapping source and destination addresses which is good. The problem is that the data size to move is defined by count * sizeof(FontMgrPtr) instead of count * sizeof(FontMgr). The structure is 20 bytes long while its pointer is just 4 on 32 bit platforms. In sort this GNW API function is broken. Maybe this is the reason why M.A.X. does not even call it.

10. **\*\*FIXED\*\*** The tm_click_response() internal GNW function within the interface.c module is a GNW button on mouse click release event handler which returns a result code in the original implementation. The button module's ButtonFunc function prototype defines void return type and thus the button module does not expect any return value. The b_value parameter is incremented in certain cases within the handler, but the changed value is basically lost. As M.A.X. does not use the services that rely on this handler it is not clear whether this could cause any negative side effects.

11. AI endlessly loads units into a deport and then unloads them to load another.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_11.mp4" type="video/mp4">
    </video>
    The deport contains 12 units which is the maximum it can hold. Moving a unit into and out from a depot does not cost movement points for the unit if it stands next to the depot. The AI is able to perform other tasks parallel which indicates that the issue is not related to code runaway. The game UI is partly unoperational though. For example clicking on the Preferences button pops up the GUI element, but afterwards clicking on the Files button crashes the game. The AI loop does not end even when the end turn timer counts down to zero.


12. Construction tape remains or is misplaced when AI constructor builds a building.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_12.mp4" type="video/mp4">
    </video>
    It is not allowed to instruct a unit to move to the coordinates found within the tape as the mouse hoover shows that the enemy constructor is found within those cells, but when a pathway is planned with shift + left mouse click the planned path crosses over the affected cells. When the constructor finishes the building the tape and the error remains. The tape and the unit referenced by the area remains even after the offending constructor is destroyed. Mouse hover also detects the constructor at cell 69-100. After finishing the building the unit might have left the construction area in that direction. This would indicate that the tape is not misplaced, but the process to remove the tape on finishing the building and moving the constructor out of the construction zone is bogus.

13. Infiltrator could stuck and game could hang if mine is on a bridge that the unit wants to pass.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_13.mp4" type="video/mp4">
    </video>
    In case of water platforms the game correctly finds that there is no path to the destination. In case of bridges this is bogus. The infiltrator cannot take the path as there is a mine in the way, but the path finding algorith tells there is a valid path. When the issue occurs the game does not accept the end turn action, the affected infiltrator cannot be moved any more and it cannot be loaded by personnel carriers. The affected bridge at the same time is redrawn as if there would be a ship under the bridge. Interestingly it is possible to load back a game in this state and if done so the queued action to end the turn from the previous game activates. This also implies that command or event queues are not cleared on loading games.

14. AI does not consider to leave a free square for engineer to leave the construction site making it stuck.
    <img src="{{ site.baseurl }}/assets/images/defect_14.jpg" alt="defect 14" width="740"> 

{% comment %}
15. Help mouse spot is at wrong position.

16. Landing Zone help button dereferences NULL.

17. Unit gets stuck at square that is occupied by constructor's construction zone.

18. First player to take turns gets 0 raw materials within the initial mining station's storage container on turn 1. Rest of the players get 14 raw materials. This is potentially a defect.
{% endcomment %}