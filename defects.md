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

5. **\*\*FIXED\*\*** M.A.X. has several units that were planned and never released for the final game or that were repurposed during game development. There is a mobile unit called the **Master Builder** which can be seen in one of the shorter movie clips of the game and has the following unit description text: *Specialized vehicle which transforms to become a new mining station.* The related game assets, like unit sprites, never made it into the game or they were not even created by the developers at all. Never the less the unit is initialized by one of the C++ constructors and when mandatory game resources are not found a NULL pointer dereference occurs. Like in case of defect 3 the game does not crash with out of bounds access under DOS environment but utterfy fails on modern operating systems with a segmentation fault.
The following resources are missing from max.res or patches.res: A_MASTER, I_MASTER, P_MASTER, F_MASTER, S_MASTER, MASTER.

6. **\*\*FIXED\*\*** The voice interface always accepts two resource indices. The reason for this is that in most cases several alternative voice samples are available for an event or action which improves immersion. So there is always a start marker, which is excluded, and a last included sample index. The game selects from the defined voice sample range in a pseudo random manner, but thanks to the implementation it is guaranteed that the start marker plus 1 is the smallest resource ID that could be chosen. There is a defect in one of the functions where the play voice API is called without a valid range. Instead of a start marker and a valid resource ID the start marker is fed into the API function twice. E.g. play_voice(Marker_283, Marker_283). Probably the defect was not found by the original developers as in such a failure case the start marker plus 1 is played which coincidentally is the resource ID that is supposed to be played. Resource ID V_F283 which corresponds to the "Mission Successful!" voice sample.

7. **\*\*FIXED\*\*** The load_font() function in GNW's text.c module leaks a file handle in case the requested amount of character descriptors cannot be read from the font file.

8. **\*\*FIXED\*\*** The text_remove_manager() function in GNW's text.c module could perform out of bounds read access to an array of structures in corner cases.

9. **\*\*FIXED\*\*** The text_remove_manager() function in GNW's text.c module is supposed to remove a previously created text manager instance. Instead of linked lists and dynamic memory allocation the module supports maximum ten managers that could be stored in a preallocated array. E.g. to remove a manager from array position 3 the function would simply shift managers 4-9 to positions 3-8 overwriting position 3 in the process. The function uses memmove() which allows overlapping source and destination addresses which is good. The problem is that the data size to move is defined by count * sizeof(FontMgrPtr) instead of count * sizeof(FontMgr). The structure is 20 bytes long while its pointer is just 4 on 32 bit platforms. In sort this GNW API function is broken. Maybe this is the reason why M.A.X. does not even call it.

10. **\*\*FIXED\*\*** The tm_click_response() internal GNW function within the interface.c module is a GNW button on mouse click release event handler which returns a result code in the original implementation. The button module's ButtonFunc function prototype defines void return type and thus the button module does not expect any return value. The b_value parameter is incremented in certain cases within the handler, but the changed value is basically lost. As M.A.X. does not use the services that rely on this handler it is not clear whether this could cause any negative side effects.

11. AI endlessly loads units into a depot and then unloads them to load another.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_11.mp4" type="video/mp4">
    </video>
    The depot contains 12 units which is the maximum it can hold. Moving a unit into and out from a depot does not cost movement points for the unit if it stands next to the depot. The AI is able to perform other tasks parallel which indicates that the issue is not related to code runaway. The game UI is partly unoperational though. For example clicking on the Preferences button pops up the GUI element, but afterwards clicking on the Files button crashes the game. The AI loop does not end even when the end turn timer counts down to zero.


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

15. The in-game help mouse spot for the map coordinate display holder arm is at the old top left location. The arm was on top in the old v1.00 interactive demo and was moved to the bottom later. The developers forgot to move the hot spot with the art to its new location.
    
    <img src="{{ site.baseurl }}/assets/images/defect_15.jpg" alt="defect 14" width="740"> 

16. Clicking the Landing Zone help button seamingly dereference NULL. The Landing Zone has a dedicated Cancel and Help button. This Help button is not the same as the one found on the normal in-game GUI control panel. While the Landing Zone Help button works without any issues there is a GUI on click event handler function which manages all the 24 normal in-game menu elements and this function incorrectly calls the rest state setter function for the wrong Help button. This issue, like many others, does not lead to a crash under DOS, but it causes segmentation fault on modern operating systems.

17. Unit gets stuck at map square that is occupied by constructor's construction tape. This issue was observed only once so far. As soon as it could be reproduced a video will be added for it. The issue happened with a computer player that moved two units at the "same" time. One unit moved to a certain location while another one, a big constructor, moved next to it and started a construction job.

18. First player to take turns gets 0 raw materials within the initial mining station's storage container on turn 1. Rest of the players get 14 raw materials. This is potentially a defect.

20. Targeting water tiles with the missile launcher's area attack hits underwater personnel carriers directly as well as indirectly, but the same cannot hit submarines at all. Ground attack planes can hit underwater personnel carriers as well as submarines. If any land or air unit hits an underwater personnel carrier it is revealed. An indirect area attack does not reveal the personnel carrier. In early builds the personnel carrier was not able to enter or pass the sea, it was a normal land unit only. It is assumed something is not working correctly here.

21. **\*\*FIXED\*\*** When the ini configuration file, MAX.INI, does not exist at game startup the original game printed a message to standard output: "\nMANDER.INI File not found..  Using Defaults...\n". MANDER.INI should have been changed in the error message to MAX.INI after the developers changed the name of the game.

22. Configuration settings are not saved to non volatile storage medium unless the parsed MAX.INI file contains the relevant option already. This is odd as the original inifile manager would have supported addition and removal of ini file entries. Probably the default MAX.INI that is found on the CD-ROM contains the settings that the developers wanted the game to remember. On the other hand if the configuration file is not found by the game it dumps a new version which contains all the entires.

25. While enemy takes turn clicking an actively selected unit resets the flic animation's sequence. This is odd as the same cannot be observed while the player's turn is active.

{% comment %}

19. Reports screens dereference NULL (mostly at game startup as long as some of the data is not filled in yet).

23. gexit: mander ini error code

24. gwin_init: wrong exit code

26. Soundmgr_init_sound_data_info: leak file handle if opening non RIFF file

27. Unit names are typically constructed as follows Mark [version number] Type [Count]. The unit counter is stored by the game on a byte. In long games it is easy to have 300+ roads in which case the counter overflows and indexing starts again from 0 even though at init the counter starts from 1.

28. The game stores gold reserves for upgrades an a signed 16 bit word. Having a gold reserve of 32767+ crashes the game?

29. The game tracks a statistic about how much gold is spent on upgrades. The counter is 16 bit?

30. The unitinfo common structure size is 20, while it has only 18 or 19? elements!

31. no abstractrion for fclose in adaption layer in seg32:000DD5C0 file_open + fclose?

{% endcomment %}
