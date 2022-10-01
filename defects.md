---
layout: page
title: Defects
permalink: /defects/
---

The article maintains a comprehensive list of game defects that is present in the original M.A.X. v1.04 (English) runtime.

1. M.A.X. is a 16/32 bit mixed linear executable that is bound to a dos extender stub from Tenberry Software called DOS/4G*W* 1.97. The W in the extender's name stands for Watcom which is the compiler used to build the original M.A.X. executable. A list of defects found in DOS/4GW 1.97 can be found in the [DOS/4GW v2.01 release notes](https://web.archive.org/web/20180611050205/http://www.tenberry.com/dos4g/watcom/rn4gw.html). By replacing DPMI service calls and basically the entire DOS extender stub with cross-platform [SDL library](https://wiki.libsdl.org/) the DOS/4GW 1.97 defects could be considered fixed.

2. If the game cannot play the intro movie, INTROFLC (maxint.mve), it tries to load and render a full screen Interplay logo image, ILOGO, which was removed from the game resource database max.res. The game continues gracefully after failing to open the ILOGO resource.
<br>
The game supports loading game resources from a secondary resource file called patches.res. The file does not exist in v1.04, but when created the contents of it are processed as expected. ILOGO is taken from the interactive demo of M.A.X. where the resource exists.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_2.mp4" type="video/mp4">
    <track label="English" kind="subtitles" srclang="en" src="{{ site.baseurl }}/assets/clips/defect_2.vtt" default>
    </video>

3. M.A.X. utilizes a user interface and OS-abstraction library called GNW for windowing, context menus, file system access, memory management, debug services, database management, human interface input recording and playback and many more. Several game menus create list controls for unit selection. When such list controls hold no items, the lists are empty, out of bounds access could occur. Based on dosbox debug sessions such accesses read random memory contents within low address space memory areas and operations conclude gracefully without crashes on DOS systems. On modern operating systems such out of bounds accesses cause segmentation faults and the game crashes immediately. UnitTypeSelector::Draw() is affected by this defect.

4. M.A.X. allocates 5 player objects. The fifth player is used for alien derelicts - *citation needed*. Certain init routines initialize arrays holding structures related to player data. Some arrays have four elements while the others have five elements. The structures held in these mixed size arrays are indexed from 4 to 0 in a loop. This creates an out of bounds write access to a totally unrelated memory area for the array that only holds four structure instances. The given structure is 19 bytes long and who knows what gets corrupted by the out of bounds write accesses.

5. M.A.X. has several units that were planned and never released for the final game or that were repurposed during game development. There is a mobile unit called the **Master Builder** which can be seen in one of the shorter movie clips of the game and has the following unit description text: *Specialized vehicle which transforms to become a new mining station.* The related game assets, like unit sprites, were removed from the game and all available demos. Never the less the unit is initialized by one of the UnitInfo class constructors (cseg01:000E9237) and when mandatory game resources are not found a NULL pointer dereference occurs. Like in case of defect 3 the game does not crash with out of bounds access under DOS environment but utterfy fails on modern operating systems with a segmentation fault.
The following resources are missing from max.res or patches.res: A_MASTER, I_MASTER, P_MASTER, F_MASTER, S_MASTER, MASTER.

6. The voice interface always accepts two resource indices. The reason for this is that in most cases several alternative voice samples are available for an event or action which improves immersion. So there is always a start marker, which is excluded, and a last included sample index. The game selects from the defined voice sample range in a pseudo random manner, but thanks to the implementation it is guaranteed that the start marker plus 1 is the smallest resource ID that could be chosen. There is a defect in one of the functions (cseg01:0008E881) where the play voice API is called without a valid range. Instead of a start marker and a valid resource ID the start marker is fed into the API function twice. E.g. play_voice(V_M283, V_M283). Probably the defect was not found by the original developers as in such a failure case the start marker plus 1 is played which coincidentally is the resource ID that is supposed to be played. Resource ID V_F283 corresponds to the "Mission Successful!" voice sample.

7. The load_font() function in GNW's text.c module leaks a file handle in case the requested amount of character descriptors cannot be read from the font file.

8. The text_remove_manager() function in GNW's text.c module could perform out of bounds read access to an array of structures in corner cases.

9. The text_remove_manager() function in GNW's text.c module is supposed to remove a previously created text manager instance. Instead of linked lists and dynamic memory allocation the module supports maximum ten managers that could be stored in a preallocated array. E.g. to remove a manager from array position 3 the function would simply shift managers 4-9 to positions 3-8 overwriting position 3 in the process. The function uses memmove() which allows overlapping source and destination addresses which is good. The problem is that the data size to move is defined by count * sizeof(FontMgrPtr) instead of count * sizeof(FontMgr). The structure is 20 bytes long while its pointer is just 4 on 32 bit platforms. In sort this GNW API function is broken. Maybe this is the reason why M.A.X. does not even call it.

10. The tm_click_response() internal GNW function within the interface.c module is a GNW button on mouse click release event handler which returns a result code in the original implementation. The button module's ButtonFunc function prototype defines void return type and thus the button module does not expect any return value. The b_value parameter is incremented in certain cases within the handler, but the changed value is basically lost. As M.A.X. does not use the services that rely on this handler it is not clear whether this could cause any negative side effects.

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

16. Clicking the help button on the landing site selection screen tries to set the rest state for the button control before it is actually created. The GUI control buttons are initialized just before the M.A.X. control panels open up, after selecting the landing site. The on click event handler function (cseg01:00094A4C) does not check whether the help button is pressed in-game when the help button's rest state is tried to be set. This issue, like many others, does not lead to a crash under DOS, but it causes segmentation fault on modern operating systems.

17. Unit gets stuck at map square that is occupied by constructor's construction tape. This issue was observed only once so far. As soon as it could be reproduced a video will be added for it. The issue happened with a computer player that moved two units at the "same" time. One unit moved to a certain location while another one, a big constructor, moved next to it and started a construction job.

18. First player to take turns gets 0 raw materials within the initial mining station's storage container on turn 1. Rest of the players get 14 raw materials. This is potentially a defect.

19. Certain reports menu screens could dereference NULL (mostly at game startup as long as some of the data is not filled in yet). The issue was only observed once. Root cause is not known yet. The game does not crash with out of bounds access under DOS environment.

20. Targeting water tiles with the missile launcher's area attack hits underwater personnel carriers directly as well as indirectly, but the same cannot hit submarines at all. Ground attack planes can hit underwater personnel carriers as well as submarines. If any land or air unit hits an underwater personnel carrier it is revealed. An indirect area attack does not reveal the personnel carrier. In early builds the personnel carrier was not able to enter or pass the sea, it was a normal land unit only. It is assumed something is not working correctly here.

21. When the ini configuration file, MAX.INI, does not exist at game startup the original game printed a message to standard output: "\nMANDER.INI File not found..  Using Defaults...\n". MANDER.INI should have been changed in the error message to MAX.INI after the developers changed the name of the game.

22. Configuration settings are not saved to non volatile storage medium unless the parsed MAX.INI file contains the relevant option already. This is odd as the original inifile manager would have supported addition and removal of ini file entries. Probably the default MAX.INI that is found on the CD-ROM contains the settings that the developers wanted the game to remember. On the other hand if the configuration file is not found by the game it dumps a new version which contains all the entires.

23. The game implements an exit(int status) like function (cseg01:0008F3A0) which prints exit code specific text to the standard output before the software terminates. One of the exit code outputs "\nMander INI not found.\n\n". Mander should have been changed in the exit code specific message to M.A.X. after the developers changed the name of the game.

24. The init function of the window manager (cseg01:00088923) sets outdated exit codes.

25. While enemy takes turn clicking an actively selected unit resets the flic animation's sequence. This is odd as the same cannot be observed while the player's turn is active.

26. The sound manager implements a method (cseg01:000DE1AE) to read meta data from audio files. If the audio file is not a RIFF WAVE file the method leaks its locally opened file handle. As all audio files in M.A.X. are RIFF WAVE format this is a latent defect only.

27. Unit names are typically constructed as follows Mark [version number] Type [Count]. The unit counter is stored by the game on a byte. In long games it is easy to have 300+ roads in which case the counter overflows. To fix this limitation the save file format version needs to be updated.

28. The game stores gold reserves for upgrades as a signed 16 bit word. Having a gold reserve of 32767+ does not crash the game, but it becomes impossible to purchase upgrades and it is also impossible to recover from this state via normal means.

29. The game tracks team casualties. These counters are stored as 8 bit words and could overflow in very long games. Interestingly fighters and infiltrators have the highest death rates.

30. The game implements lookup tables that map unit operating states to unit type and state specific sound effects. The size of lookup tables are dynamic. The first byte in the table defines the number of entries that follows. The default or common table, called UnitInfo_SfxDefaultUnit by the reimplementation, defines that the table has 20 entries even though there are only 18 entries provided in the table. There are 18 state specific sound effect types in total so it makes no sense to have tables bigger than that size. The sound manager searches in these tables and the method responsible to play sound effects (cseg01:000DD5C0) implements a for-loop to find the sound effect type specific sound effect in the lookup tables stopping the search as soon as a match is found. Due to this implementation it is guaranteed that a match will be found before the loop would do out of bounds access that may or may not lead to actual memory corruption or other issues.

31. The game implements a set of wrapper functions for file operations like fopen, fread, fseek. On the other hand there are no wrappers for fclose, ftell, fwrite and so on. This is not actually a defect, but a strange design decision as the wrappers add no functionality at all and the game uses them inconsistently all around. The reimplementation removes the wrappers.

32. The default mouse action for an infiltrator (and probably any other unit) is to attack a neutral bridge if an enemy flying entity is located in the cell.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_32.mp4" type="video/mp4">
    </video>
    The video clip demonstrates that the cell below the enemy air transport is free to unload a land unit, but moving a stealth unit back to the free cell is not possible as the default action for the cell is to attack instead of move. For a cell where a neutral bridge is present only the default action is to allow movement there. As an infiltrator cannot attack a flying target and the bridge is a neutral entity the default action for such a cell should be to allow movment there, but the game incorrectly interprets the situation that the default action should be the attack.

33. If a cell is occupied by an enemy land unit and an enemy air unit, possibly sea unit works as well, the game does not allow selection of the air unit. In case a cell is occupied by a neutral bridge and an enemy air unit, the air unit is selectable while the neutral bridge is not. This is more of a limitation or problem instead of a defect, but it should be solved.

34. Previously accepted AI kill unit task retriggers several times at the beginning of each team's turn to search for a valid path to the target in case there is no valid path to the moved target's new location. At the same time the voice advisor interface is spammed with "no path to destination" type of voice samples.
    <br>
    <video class="embed-video" controls loop playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_33.mp4" type="video/mp4">
    </video>
    Opinion: it would be better to cancel the AI kill unit task in case there is no path to the destination any more and to add a new AI attack plan task instead. Rationale: The overall goal does not change this way. Problem: The game does not allow to issue a new AI kill unit or AI attack plan task if there is no valid path to the destination. To at least avoid the advisor voice spamming it would be possible to filter multiple voice events from the same voice category for the actively selected unit.

35. Similarly to defect 6 a cargo transfer related function calls the play voice API with an invalid voice range. PlayVoice(V_M224, V_M224, PRIO_0) instead of PlayVoice(V_M224, V_F224, PRIO_0).

36. The ObjectArray base class uses memcpy() to shift data in its internally allocated buffer after removal of an array element. In case the destination and source addresses overlap memmove() is supposed to be used instead of memcpy() as the latter does not guarantee proper handling of overlapping memory areas. The original compiler's memcpy() implementation for example performs 32 bit word copy operations and only the final non 32 bit sized data chunk is copied per byte. This means that if the instantiated derived array class holds a type which has an element size that is smaller than 32 bits each removal could result in data corruption in all the elements that were held after the removed item. It is not yet confirmed whether there were any offending types though.

37. The TeamUnits class specific TextLoad method patches one of the unit ATTRIBS entires to disable the unit specific move & fire capability via code. Most probably the Infantry was the unit in question as it lost this capability during development. There are other inconsistencies between the human readable and binary parsers of TextfileObject based classes which indicates that after switching to binary format from the initial text based one the text parsers were not fully maintained.

38. The ScrollBar base class and therefore the derived LimitedScrollbar class forget to delete the Button class instance in their destructor leaking heap memory at least 56 bytes at a time.

39. The play voice API is called with wrong voice range. PlayVoice(V_M229, V_F256, PRIO_0) is called when a unit is under attack and it does not gets destroyed. There are only four under attack type voice samples: V_F229 - V_F232 so the correct range would be PlayVoice(V_M229, V_F232, PRIO_0). It is not easy to test the failure scenario as a unit might be destroyed in a single shot and the API call could still randomly select a correct sample or a sample that does not exists so no incorrect sound would play.

40. The function which builds network packet 41 (path blocked) forgets to set the packet header's *data_size* field so random garbage in random size is sent over the network. Typically a single byte of data is sent as the most common packet on the network has one data byte.

41. On the Network Game lobby screen clients cannot see the maps, load, scenarios and options buttons as these are only available for the host. The in-game help mouse spot areas for these buttons are not deactivated for clients.

42. Saving a game in multiplayer game modes via network packet 16 does not verify whether peers are already desynchronized. The game also does not check for desynchronization when a previously saved multiplayer game is loaded. This leads to situations where previously saved games are basically corrupted already at the time of loading.

43. The help button and the help feature in general does not work on the IPX multiplayer initial cargo setup screen.

44. When initial upgrades are made during mission loadout and the player proceeds to the landing zone selection screen then goes back to the purchase menu by pushing the cancel button the upgrades can be cancelled to get back the upgrade costs, but the game keeps the upgrades still. This can be used as an exploit in both single and multiplayer games where the purchase menu is available.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_44.mp4" type="video/mp4">
    </video>
    In multiplayer the purchased unit count must remain below 134 otherwise buffer overflows or other wicked stuff could happen.

45. The TeamInfo class still allocates space for the map markers feature (40 bytes) which was removed by the original developers before release. To fix this technical debt the save file format version needs to be updated.

46. The TeamInfo class allocates 6 slots for screen locations while only 4 locations can be saved and restored by the game logic. To fix this technical debt the save file format version needs to be updated.

47. The save file format header allocates 5 team instances for team type and team clan parameters but only 4 team instances are filled up with valid data. To fix this technical debt the save file format version needs to be updated.

48. There is a typo in the description of Valentine's Planet. "Two to four can be easily accomodated." -> accommodated.

49. The resource levels panel of the game options menu allows to configure the amount of alien derelicts. The built-in help entry for the panel does not explain the meaning of alien derelicts indicating that the English help is older than the concept of alien derelicts. Additionally the "Alien Derelicts" caption is the only one without a colon within the panel.

50. The built-in help menu's pop-up window supports rendering multiple pages of text. If more than one page is needed to display the help entry arrow buttons are added to the window. The up arrow registers an sfx, the down arrow does not. Interestingly neither is played by the game for some reason.

51. The game setup menu allocates char buffers to hold mission titles using new char[], but deletes the buffers using the delete operator instead of the delete[] operator which is undefined behavior.

52. The event handler of the scroll up button on mission selection screens tests against an uninitialized local (stack) variable. In the extremely unlikely case when the test would pass the function does nothing to the GameSetupMenu class object's state as the function prematurely exits.

53. If a multiplayer game desyncs on the beginning of the first round before a new autosave is made, thus no autosave exists for the given game session, the game still asks whether players, any of them, wants to reload the last save.

54. The main menu setup and in-game preferences menu is implemented in one class. The game pause menu is not deactivated in the main menu by the setup window variant where it makes no sense to be active.

55. The game pause menu is not deactivated in the popup help menus from within the main menu.

56. Single player custom games test for at least two active players of any kind (computer and human) to be able to start a game. Hot seat games test for at least two human players to be selected. This means that it is possible to start a single player custom game without a human player. It is unclear whether this is a feature or a defect.

57. The custom scenarios menu title is "Multiplayer Scenarios" even in single player mode.

58. If a high-tech unit is captured, and then it gets upgraded in the depot, the unit would lose any of its superior unit values. For example if an enemy tank had 34 attack power and the player's own latest tank upgrades would only grant 20 attack power to an upgraded tank then the captured tank would lose the 34 attack power and would get the inferior 20 "as an upgrade". This behavior was considered to be a defect in M.A.X. 2 back in March, 1998.

59. The SaveSlot class has two image resources, FILEx_UP and FILEx_DN. The SaveLoadMenu_Init() function allocates memory for the images' data. The buffer size is determined for both images, but the allocation uses the size of FILEx_UP data for both. As both images have the same dimensions this defect does not cause any issues.

60. The function which builds network packet 13 (update RNG seed) forgets to set the packet header's *entity_id* field so the previously configured packet's field value is used.

61. If an IPX client exits abnormally while it is in an IPX network lobby the host and its other peers will not be able to proceed with their game creation process. The only way to continue game creation is to exit the lobby by the host and start the connection process again from scratch. Adding a visual marker for ready state and adding the ability for the host to kick an unresponsive client could resolve the issue.

62. The ButtonManager class allocates ButtonID buffers with new[] but deletes the buffer with free which is undefined behavior. It worked under Watcom 10.x compiler.

63. The AI misbehaves when a hidden infiltrator steps aside, but doing so reveals its presence to the enemy.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_63.mp4" type="video/mp4">
    </video>
    In this case the AI thinks that the infiltrator stands at the old grid cell position and offloads all shots onto the empty ground. Eventually the AI will "learn" the correct location too and may terminate the revealed infiltrator.

64. Disabled alien attack planes placed as alien derelicts onto the maps do not set the map pass tables to unpassable.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_64.mp4" type="video/mp4">
    </video>
    The path generator lets ground units pass these planes. This defect could be used as an exploit to give an edge against human players. If a unit's remaining moves in a turn are calculated well, then it is possible to stop exactly under the plane. The mini map will show the alien derelict's team color (team 5, yellow) instead of the player unit's color. If the grid cell gets attacked, the player unit is not damaged as the alien unit is targeted by the game.

65. The ResourceManager_InitTeamInfo() function at cseg01:000D6316 initializes 5 CTinfo structs. The last belongs to the alien derelicts team. The CTinfo structure's TeamType and ClanType fields are initialized from the MAX.INI file's red_team_player to gray_team_player and red_team_clan to gray_team_clan parameters. There is no alien_team_player nor alien_team_clan INI parameter so the alien CTinfo TeamType field is initialized from the red_team_clan INI parameter and the ClanType field is initialized from the DIGITAL INI group ID (returns 0 which corresponds to RANDOM clan). It is not yet clear what negative side effects these defects create.

66. If a unit is selected the primary selection marker starts to blink at a 1 second rate. If a multiselect rectangle is redrawn by clicking and dragging the mouse pointer the primary marker gets redrawn as well and its draw function toggles the marker state much faster. The primary marker should not start to blink like crazy in such events.

67. When a supply truck tries to reload a unit and it's storage container is empty the following alert message is shown: "insufficient material in storage to reload unit.". The message should start with capital letter. Same goes for repairing a unit.

68. The path generator erroneously cannot find path to a cell location in corner cases. Note: the audio can be unmuted on this video clip.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_68.mp4" type="video/mp4">
    <track label="English" kind="subtitles" srclang="en" src="{{ site.baseurl }}/assets/clips/defect_68.vtt" default>
    </video>
    In the above case an airplane is hovering above a bridge which confuses the path generator.

69. The path generator inconsistently finds a path to a cell where it is questionable whether the given unit should be allowed to move at all. Note: the audio can be unmuted on this video clip.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_69.mp4" type="video/mp4">
    <track label="English" kind="subtitles" srclang="en" src="{{ site.baseurl }}/assets/clips/defect_69.vtt" default>
    </video>
    Is it intentional to allow ships to coast type cells in case there is a bridge at the same cell?

70. The allocation menu uses a function (cseg01:0008A5FB) to change color temperature of highlighted text background. When the algorithm finds a better color match in the system color palette based on color distance the worse distance is saved instead of the better one.
    <br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_70.mp4" type="video/mp4">
    <track label="English" kind="subtitles" srclang="en" src="{{ site.baseurl }}/assets/clips/defect_70.vtt" default>
    </video>
    This makes the text background brighter now, but more importantly more detail is visible. Maybe in the future the blue color temperature could be made darker to match the original aesthetic keeping the higher level of detail.

71. There is a typo in the briefing text of stand alone mission 20 (DESCR20.SCE). "Can you affort to shut down" -> afford.

72. There is a function (cseg01:0008E881) to wrap up games and display the mission's end briefing. The function takes an ordered array with the places of teams and builds another local array later on with very similar content. In the beginning of the function the local yet to be initialized array is used to test which is the winner team in case of custom games instead of the array which was taken as a function argument by mistake.

73. The order of initialization of global variables across translation units is unspecified behavior in standard C++. The original SmartString class has a default constructor (cseg01:000E1AE8) which uses a globally constructed SmartString object instantiated by an overloaded constructor. The idea behind is that if new SmartString objects are created with the default constructor the class just increments internally its reference counter to a default empty string object. The problem with this approach is that nothing guarantees that the SmartString module's internal global variable will be initialized first before another module's global SmartString variable in which case the object for which the reference counter would be incremented does not exist yet and the application crashes with a null pointer dereference related segmentation fault at some point. It seems that the original solution worked in the old Watcom C/C++ compiler while it fails with GCC.

74. The function (cseg01:0009D3A7) that spawns all derelict alien units on the map uses rand() which generates pseudo random numbers. There is a code snippet that seeks a pseudo randomly selected alien unit that fits within a predetermined value budget. When such a unit is found the function deploys another pseudo randomly selected alien unit instead of reusing the unit type that was tested to actually fit into the budget. The code clearly wanted to select a unit within budget so spawning another one which may not fit must be a defect.

75. The functions (cseg01:0009CAE7, cseg01:0009C6F6) responsible for resource distribution on maps do not check whether the calculated grid coordinates are within map boundaries. In corner cases this could lead to heap memory corruption under DOS or in worst case could cause segmentation faults on modern operating systems (ResourceManager_CargoMap[]).

76. Dot is missing from end of sentence "%i %s upgraded to mark %s for %i raw material" in Upgrade All event handler function (cseg01:000F80FD). There are three possible messages: no upgrade, single unit upgrade, more then one unit upgrade. The messages are ended with dots for the first two, only the third one is inconsistent.

77. One of the access module functions (cseg01:000136D8) reads already released heap memory which leads to random crashes or even worse, to memory corruption.
```cpp
    UnitInfo* unit;
    SmartList<UnitInfo>::Iterator it;
    
    // the iterator is a smart pointer to a list node object which holds the list item and a reference to the next
    // and previous list node. The next and prev members are smart pointers to list nodes
    for (it = Hash_MapHash[Point(grid_x, grid_y)], unit = &*it; it != nullptr;) {
        if (((*it).flags & flags) && !((*it).flags & GROUND_COVER) && (*it).orders != ORDER_IDLE) {
            return &*it;
        }
        
        // the following code snippet removes the list item, the list node, from the hash map which means that the
        // last reference to the list item, which is held by the list node, is held by the iterator which is again
        // a reference counted smart pointer
        Hash_MapHash.Remove(&*it);
        Hash_MapHash.Add(&*it);
        
        // this is where the issue occurs as the iterator is moved which means that the last reference to our
        // list node is gone somewhere within the iterator's overloaded ++ operator
        ++it;
        
        ...
    }
    
    // this is how the iterator's ++ operator is implemented. The next member of the list node object
    // is a smart pointer which means that the smart pointer class uses the overloaded = operator that
    // takes another smart pointer by reference (by its address practically)
    Iterator& operator++() {
        SmartPointer<ListNode<T>>::operator=(this->object_pointer->next);
        return *this;
    }
    
    // this is how the = operator is implemented. The implementation takes into account the use case where the
    // other object could be the same as the this object. But it does not take into account that the object
    // that is held by the smart pointer might be a smart pointer like object.
    SmartPointer<T>& operator=(const SmartPointer<T>& other) {
        // the other object in this case is the list node's next member which has a reference count >= 1
        other.object_pointer->Increment();
        // the decrement operation decreases the current list node's reference count to zero so the held list node
        // object gets deleted. But that list node object's next member, which is a smart pointer to the next list
        // node also gets deleted! So the other argument, which is a reference, now points to released heap memory.
        // The heap deallocator implementation may fill up the released memory with a test pattern like 0xFEEEFEEE or
        // 0xDEADBEEF or similar. 
        object_pointer->Decrement();
        // So when the object_pointer is assigned to other.object_pointer potentially 0xFEEEFEEE offset by the object_pointer
        // member gets dereferenced and the software segmentation faults or worse.
        object_pointer = other.object_pointer;
    
        return *this;
    }
```
The issue can be resolved if the iterator ++ and \-\- operators pass object T reference to the smart pointer instead of the smartpointer itself as follows.
```cpp
    // bad, passes next which is a smart pointer to the list node object
    SmartPointer<ListNode<T>>::operator=(this->object_pointer->next);
    
    // good, passes the list node object (T) held by next so even if the list node is deleted and the next member of it gets destroyed the object T is still valid
    SmartPointer<ListNode<T>>::operator=(&*this->object_pointer->next);
```
On modern operating systems the deallocated heap memory could be reallocated by another thread or process before the next list node gets assigned so memory corruption or crashes might still occur even if the heap deallocator does not fill the released memory with nasty bug trapping test patterns. On DOS this is highly unlikely so there this defect was assumably latent.

78. The Engineer and Constructor specific popup context menu button list initializer function (cseg01:000F9304) tests whether the units are building `order = build and order_state != 13 and order_state != 46` and either adds the Stop or the Build button to the button list. The last test in the original code is `order != 46` which is always true of course as there is no such order ID. In case the order state would be 46, whatever that means, the wrong button would be presented to the user. Interestingly the button on click event handler is the same function so the test and the button label is defective, but the outcome of the event will be the same action.

79. The function (cseg01:0009C6F6) responsible for resource distribution on maps uses an uninitialized variable by mistake. Fixing this issue potentially alters the original resource distribution probabilities.

80. Units that can pass land as well as water do not update the sprite base frame index after they are picked up from water by air transport and dropped down onto land.
<br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_80.mp4" type="video/mp4">
    </video>
    The transporter's activate event should check the destination cell type.

81. The algorithm (cseg01:00076144) that draws attack and scan ranges on screen misbehaves at extreme ranges combined with high level of zoom.
<br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_81.mp4" type="video/mp4">
    </video>

82. Redraw order of the attack and scan range markers and the message area is indeterministic in case a hoovering unit overlaps with them.
<br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_82.mp4" type="video/mp4">
    </video>
    This is not the only flickering behaviour.

83. The function (cseg01:000CE775) responsible for the reports menu's mouse left click event handling tests against incorrect window boundaries. The active draw area is 459 x 448 pixels. The on click event handler checks for 459 x 459 pixels by calling Image::GetWidth() for both x and y axis dimensions. this must be a simple copy paste error.

84. The reports menu constructor (cseg01:000CC8D0) calls GNW win_draw() twice right after each other. First a call is enabled by the class specific draw function via function parameter, then right after the draw function there is another call.

85. Enemy land units do not sink or get destroyed after a water platform or similar is demolished beneath them. The units can also move from a water cell back to a land cell if it is next to them. Land mines do not get destroyed if the structure below them gets demolished. Sea mine layers can detect floating land mines too. Building a landing pad above a road above a water platform makes the road disappear. Further investigation is required whether the road unit gets "properly" destroyed or is it leaking memory. Destoying a landing pad above a water platform while an aircraft is landed there leaves the aircraft on the ground in awaiting state. The aircraft is targetable in this state by land units. The aircraft can take off into air from the water mass.
<br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_85.mp4" type="video/mp4">
    </video>
    In my opinion land mines should also blow up just like connectors and roads built above water platforms and bridges do. Any non hybrid land unit should sink to the bottom of the sea too. If an aircraft is landed on a landing pad on a water platform and the landing pad gets demolished the aircraft should either take off into air, or should sink to the bottom of the sea.

86. The maxsuper cheat code increases the scan and attack ranges. The game redraws the markers incorrectly until there is a movement of screen or unit.
<br>
    <video class="embed-video" autoplay loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_86.mp4" type="video/mp4">
    </video>

87. Both the TaskMineAssisstant and TaskFrontierAssistant classes have the same type id. It is not known yet whether this is a copy paste error or done on purpose in which case it would be an unnecessary design decision not a defect. There are other tasks with equal type identifiers too.

88. The TaskRemoveRubble task implements a function (cseg01:0003A57F) to dump cargo to the closest complex. This task is used by computer players. The algorithm first searches for a building A that could store some of the raw materials that the bulldozer or mine layer or similar holds, then searches for the closest building B within the same complex of the previously found building A and orders the bulldozer to move to **building A** instead of the closest building within the complex which would be building B. It is another topic why Supply Trucks with free capacity are not even considered by the AI or that it is not even checked how much raw materials can be stored within the selected complex.

89. Both the TaskConnectionAssistant and TaskPowerAssistant classes have the same type id. It is not known yet whether this is a copy paste error or done on purpose in which case it would be an unnecessary design decision not a defect. There are other tasks with equal type identifiers too.

90. The TaskCreateUnit task implements a function (cseg01:00038CC9) that tries to optimize resource consumption so that demanded units could be created (faster). If necessary the algorithm tries to increase fuel reserves by shutting down inessential infrastructure, namely Eco-Spheres and Gold Refineries. There is a copy paste error when counting the inessential building types so Eco-Spheres are tested twice, counted once, and Gold Refineries do not add to the count. Due to this defect basically Gold Refineries are not considered at all by the algorithm.

91. The TaskManager implements a method (cseg01:00043F3F) to estimate the total heap memory usage by the AI. The game introduced a priority list for reminders in one of the post release patches and instead of considering the dynamic memory usage of the priority reminders list it considers the count of reminders stored by the list by calling the wrong smart list function. Due to this and other similar defects the overall memory usage was probably underestimated by the developers. The game places huge buffers on the stack as well which makes stack usage hard to estimate. GNW supported custom C memory allocators that could measure dynamic memory usage, but as M.A.X. is mostly implemented in C++ this GNW feature was somewhat useless.

92. The DefenseManager class tracks the unit types it needs in a SmartObjectArray\<ResourceID\> object and the units it already manages in a SmartList\<UnitInfo\> object. If a new unit is added to the manager the given unit type is removed from the array of needed unit types and the unit is added to the list of managed units. The method (cseg01:0001D2C8) responsible for this operation calls the SmartObjectArray\<ResourceID\>::Remove() method which attempts to remove an array element by index or position while the manager method passes the unit type which is obviously not equal to the array position where the unit type was originally stored. No memory corruption occurs as the SmartObjectArray is intelligent enough not to remove elements from beyond the allocated size of the array. But the algorithm is totally broken that relies on the unit type array contents. The root cause of the defect is probably a misunderstood interface as while the SmartList\<UnitInfo\>::Remove() API searches for the passed element instance and if found it is removed from the linked list the SmartObjectArray\<ResourceID\>::Remove() behaves totally different.
