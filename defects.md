---
layout: page
title: Defects
permalink: /defects/
---

The article maintains a comprehensive list of game defects that is present in the original M.A.X. v1.04 (English) runtime.

1. **[Fixed]** M.A.X. is a 16/32 bit mixed linear executable that is bound to a dos extender stub from Tenberry Software called DOS/4G*W* 1.97. The W in the extender's name stands for Watcom which is the compiler used to build the original M.A.X. executable. A list of defects found in DOS/4GW 1.97 can be found in the [DOS/4GW v2.01 release notes](https://web.archive.org/web/20180611050205/http://www.tenberry.com/dos4g/watcom/rn4gw.html). By replacing DPMI service calls and basically the entire DOS extender stub with cross-platform [SDL library](https://wiki.libsdl.org/) the DOS/4GW 1.97 defects could be considered fixed.

2. If the game cannot play the intro movie, INTROFLC (maxint.mve), it tries to load and render a full screen Interplay logo image, ILOGO, which was removed from the game resource database max.res. The game continues gracefully after failing to open the ILOGO resource.
<br>
The game supports loading game resources from a secondary resource file called patches.res. The file does not exist in v1.04, but when created the contents of it are processed as expected. ILOGO is taken from the interactive demo of M.A.X. where the resource exists.
    <br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_2.mp4" type="video/mp4">
    <track label="English" kind="subtitles" srclang="en" src="{{ site.baseurl }}/assets/clips/defect_2.vtt" default>
    </video>

3. **[Fixed]** M.A.X. utilizes a user interface and OS-abstraction library called GNW for windowing, context menus, file system access, memory management, debug services, database management, human interface input recording and playback and many more. Several game menus create list controls for unit selection. When such list controls hold no items, the lists are empty, out of bounds access could occur. Based on dosbox debug sessions such accesses read random memory contents within low address space memory areas and operations conclude gracefully without crashes on DOS systems. On modern operating systems such out of bounds accesses cause segmentation faults and the game crashes immediately. UnitTypeSelector::Draw() (cseg01:00070CE3) is affected by this defect.

4. **[Fixed]** M.A.X. allocates 5 player objects. The fifth player is used for alien derelicts and is a very late addition to the game. Certain init routines (cseg01:0009DC7B) initialize arrays holding structures related to player data. Some arrays have four elements while the others have five elements. The structures held in these mixed size arrays are indexed from 4 to 0 in a loop. This creates an out of bounds write access to a totally unrelated memory area for the array that only holds four structure instances. The given structure is 19 bytes long and who knows what gets corrupted by the out of bounds write accesses.

5. **[Fixed]** M.A.X. has several units that were planned and never released for the final game or that were repurposed during game development. There is a mobile unit called the **Master Builder** which can be seen in one of the shorter movie clips of the game and has the following unit description text: *Specialized vehicle which transforms to become a new mining station.* The related game assets, like unit sprites, were removed from the game and all available demos. Never the less the unit is initialized by one of the UnitInfo class constructors (cseg01:000E9237) and when mandatory game resources are not found a NULL pointer dereference occurs. Like in case of defect 3 the game does not crash with out of bounds access under DOS environment but utterfy fails on modern operating systems with a segmentation fault.
The following resources are missing from max.res or patches.res: A_MASTER, I_MASTER, P_MASTER, F_MASTER, S_MASTER, MASTER.

6. **[Fixed]** The voice interface always accepts two resource indices. The reason for this is that in most cases several alternative voice samples are available for an event or action which improves immersion. So there is always a start marker, which is excluded, and a last included sample index. The game selects from the defined voice sample range in a pseudo random manner, but thanks to the implementation it is guaranteed that the start marker plus 1 is the smallest resource ID that could be chosen. There is a defect in one of the functions (cseg01:0008E881) where the play voice API is called without a valid range. Instead of a start marker and a valid resource ID the start marker is fed into the API function twice. E.g. play_voice(V_M283, V_M283). Probably the defect was not found by the original developers as in such a failure case the start marker plus 1 is played which coincidentally is the resource ID that is supposed to be played. Resource ID V_F283 corresponds to the "Mission Successful!" voice sample.

7. **[Fixed]** The load_font() function (cseg01:00109F6C) in GNW's text.c module leaks a file handle in case the requested amount of character descriptors cannot be read from the font file.

8. **[Fixed]** The text_remove_manager() function (cseg01:0010A164) in GNW's text.c module could perform out of bounds read access to an array of structures in corner cases.

9. **[Fixed]** The text_remove_manager() function (cseg01:0010A164) in GNW's text.c module is supposed to remove a previously created text manager instance. Instead of linked lists and dynamic memory allocation the module supports maximum ten managers that could be stored in a preallocated array. E.g. to remove a manager from array position 3 the function would simply shift managers 4-9 to positions 3-8 overwriting position 3 in the process. The function uses memmove() which allows overlapping source and destination addresses which is good. The problem is that the data size to move is defined by count * sizeof(FontMgrPtr) instead of count * sizeof(FontMgr). The structure is 20 bytes long while its pointer is just 4 on 32 bit platforms. In sort this GNW API function is broken. Maybe this is the reason why M.A.X. does not even call it.

10. The tm_click_response() internal GNW function within the interface.c module is a GNW button on mouse click release event handler which returns a result code in the original implementation. The button module's ButtonFunc function prototype defines void return type and thus the button module does not expect any return value. The b_value parameter is incremented in certain cases within the handler, but the changed value is basically lost. As M.A.X. does not use the services that rely on this handler it is not clear whether this could cause any negative side effects.

11. AI endlessly loads units into a depot and then unloads them to load another.
    <br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_11.mp4" type="video/mp4">
    </video>
    The depot contains 12 units which is the maximum it can hold. Moving a unit into and out from a depot does not cost movement points for the unit if it stands next to the depot. The AI is able to perform other tasks parallel which indicates that the issue is not related to code runaway. The game UI is partly unoperational though. For example clicking on the Preferences button pops up the GUI element, but afterwards clicking on the Files button crashes the game. The AI loop does not end even when the end turn timer counts down to zero.


12. Construction tape remains or is misplaced when AI constructor builds a building.
    <br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_12.mp4" type="video/mp4">
    </video>
    It is not allowed to instruct a unit to move to the coordinates found within the tape as the mouse hoover shows that the enemy constructor is found within those cells, but when a pathway is planned with shift + left mouse click the planned path crosses over the affected cells. When the constructor finishes the building the tape and the error remains. The tape and the unit referenced by the area remains even after the offending constructor is destroyed. Mouse hover also detects the constructor at cell 69-100. After finishing the building the unit might have left the construction area in that direction. This would indicate that the tape is not misplaced, but the process to remove the tape on finishing the building and moving the constructor out of the construction zone is bogus.

13. Infiltrator could stuck and game could hang if mine is on a bridge that the unit wants to pass.
    <br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_13.mp4" type="video/mp4">
    </video>
    In case of water platforms the game correctly finds that there is no path to the destination. In case of bridges this is bogus. The infiltrator cannot take the path as there is a mine in the way, but the path finding algorith tells there is a valid path. When the issue occurs the game does not accept the end turn action, the affected infiltrator cannot be moved any more and it cannot be loaded by personnel carriers. The affected bridge at the same time is redrawn as if there would be a ship under the bridge. Interestingly it is possible to load back a game in this state and if done so the queued action to end the turn from the previous game activates. This also implies that command or event queues are not cleared on loading games.

14. AI does not consider to leave a free square for engineer to leave the construction site making it stuck.
<br>
    <img src="{{ site.baseurl }}/assets/images/defect_14.jpg" alt="defect 14" width="740"> 

15. The in-game help mouse spot for the map coordinate display holder arm is at the old top left location. The arm was on top in the old v1.00 interactive demo and was moved to the bottom later. The developers forgot to move the hot spot with the art to its new location.
<br>
    <img src="{{ site.baseurl }}/assets/images/defect_15.jpg" alt="defect 14" width="740"> 

16. **[Fixed]** Clicking the help button on the landing site selection screen tries to set the rest state for the button control before it is actually created. The GUI control buttons are initialized just before the M.A.X. control panels open up, after selecting the landing site. The on click event handler function (cseg01:00094A4C) does not check whether the help button is pressed in-game when the help button's rest state is tried to be set. This issue, like many others, does not lead to a crash under DOS, but it causes segmentation fault on modern operating systems.

17. Unit gets stuck at map square that is occupied by constructor's construction tape. This issue was observed only once so far. As soon as it could be reproduced a video will be added for it. The issue happened with a computer player that moved two units at the "same" time. One unit moved to a certain location while another one, a big constructor, moved next to it and started a construction job.

18. First player to take turns gets 0 raw materials within the initial mining station's storage container on turn 1. Rest of the players get 14 raw materials. This is potentially a defect.

19. Certain reports menu screens could dereference NULL (mostly at game startup as long as some of the data is not filled in yet). The issue was only observed once. Root cause is not known yet. The game does not crash with out of bounds access under DOS environment.

20. Targeting water tiles with the missile launcher's area attack hits underwater personnel carriers directly as well as indirectly, but the same cannot hit submarines at all. Ground attack planes can hit underwater personnel carriers as well as submarines. If any land or air unit hits an underwater personnel carrier it is revealed. An indirect area attack does not reveal the personnel carrier. In early builds the personnel carrier was not able to enter or pass the sea, it was a normal land unit only. It is assumed something is not working correctly here.

21. **[Fixed]** When the ini configuration file, MAX.INI, does not exist at game startup the original game (cseg01:000A5DB3) printed a message to standard output: "\nMANDER.INI File not found..  Using Defaults...\n". MANDER.INI should have been changed in the error message to MAX.INI after the developers changed the name of the game.

22. **[Fixed]** Configuration settings are not saved to non volatile storage medium unless the parsed MAX.INI file contains the relevant option already. This is odd as the original inifile manager would have supported addition and removal of ini file entries. Probably the default MAX.INI that is found on the CD-ROM contains the settings that the developers wanted the game to remember. On the other hand if the configuration file is not found by the game it dumps a new version which contains all the entires. The `enhanced_graphics` option is the only one which is lost on each startup as it was overridden in the original game by available system memory. The proposed defect fix allows users to save and load their user preference for the `enhanced_graphics` option. No other changes are made to the INI manager.

23. **[Fixed]** The game implements an exit(int status) like function (cseg01:0008F3A0) which prints exit code specific text to the standard output before the software terminates. One of the exit code outputs "\nMander INI not found.\n\n". Mander should have been changed in the exit code specific message to M.A.X. after the developers changed the name of the game.

24. The init function of the window manager (cseg01:00088923) sets outdated exit codes.

25. While enemy takes turn clicking an actively selected unit resets the flic animation's sequence. This is odd as the same cannot be observed while the player's turn is active.

26. **[Fixed]** The sound manager implements a method (cseg01:000DE1AE) to read meta data from audio files. If the audio file is not a RIFF WAVE file the method leaks its locally opened file handle. As all audio files in M.A.X. are RIFF WAVE format this is a latent defect only.

27. Unit names are typically constructed as follows Mark [version number] Type [Count]. The unit counter is stored by the game on a byte. In long games it is easy to have 300+ roads in which case the counter overflows. To fix this limitation the save file format version needs to be updated.

28. The game stores gold reserves for upgrades as a signed 16 bit word. Having a gold reserve of 32767+ does not crash the game, but it becomes impossible to purchase upgrades and it is also impossible to recover from this state via normal means.

29. The game tracks team casualties. These counters are stored as 8 bit words and could overflow in very long games. Interestingly fighters and infiltrators have the highest death rates.

30. The game implements lookup tables that map unit operating states to unit type and state specific sound effects. The size of lookup tables are dynamic. The first byte in the table defines the number of entries that follows. The default or common table, called UnitInfo_SfxDefaultUnit by the reimplementation, defines that the table has 20 entries even though there are only 18 entries provided in the table. There are 18 state specific sound effect types in total so it makes no sense to have tables bigger than that size. The sound manager searches in these tables and the method responsible to play sound effects (cseg01:000DD5C0) implements a for-loop to find the sound effect type specific sound effect in the lookup tables stopping the search as soon as a match is found. Due to this implementation it is guaranteed that a match will be found before the loop would do out of bounds access that may or may not lead to actual memory corruption or other issues.

31. The game implements a set of wrapper functions for file operations like fopen, fread, fseek. On the other hand there are no wrappers for fclose, ftell, fwrite and so on. This is not actually a defect, but a strange design decision as the wrappers add no functionality at all and the game uses them inconsistently all around. The reimplementation removes the wrappers.

32. The default mouse action for an infiltrator (and probably any other unit) is to attack a neutral bridge if an enemy flying entity is located in the cell.
    <br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
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

35. **[Fixed]** Similarly to defect 6 a cargo transfer related function (cseg01:00098F46) calls the play voice API with an invalid voice range. PlayVoice(V_M224, V_M224, PRIO_0) instead of PlayVoice(V_M224, V_F224, PRIO_0).

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
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_44.mp4" type="video/mp4">
    </video>
    In multiplayer the purchased unit count must remain below 134 otherwise buffer overflows or other wicked stuff could happen.

45. The TeamInfo class still allocates space for the map markers feature (40 bytes) which was removed by the original developers before release. To fix this technical debt the save file format version needs to be updated.

46. The TeamInfo class allocates 6 slots for screen locations while only 4 locations can be saved and restored by the game logic. To fix this technical debt the save file format version needs to be updated.

47. The save file format header allocates 5 team instances for team type and team clan parameters but only 4 team instances are filled up with valid data. To fix this technical debt the save file format version needs to be updated.

48. **[Fixed]** There is a typo in the description of Valentine's Planet. "Two to four can be easily accomodated." -> accommodated.

49. The resource levels panel of the game options menu allows to configure the amount of alien derelicts. The built-in help entry for the panel does not explain the meaning of alien derelicts indicating that the English help is older than the concept of alien derelicts. Additionally the "Alien Derelicts" caption is the only one without a colon within the panel.

50. The built-in help menu's popup window supports rendering multiple pages of text. If more than one page is needed to display the help entry arrow buttons are added to the window. The up arrow registers an sfx, the down arrow does not. Interestingly neither is played by the game for some reason.

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
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_63.mp4" type="video/mp4">
    </video>
    In this case the AI thinks that the infiltrator stands at the old grid cell position and offloads all shots onto the empty ground. Eventually the AI will "learn" the correct location too and may terminate the revealed infiltrator.

64. Disabled alien attack planes placed as alien derelicts onto the maps do not set the map pass tables to unpassable.
    <br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_64.mp4" type="video/mp4">
    </video>
    The path generator lets ground units pass these planes. This defect could be used as an exploit to give an edge against human players. If a unit's remaining moves in a turn are calculated well, then it is possible to stop exactly under the plane. The mini map will show the alien derelict's team color (team 5, yellow) instead of the player unit's color. If the grid cell gets attacked, the player unit is not damaged as the alien unit is targeted by the game.

65. **[Fixed]** The ResourceManager_InitTeamInfo() function (cseg01:000D6316) initializes 5 CTinfo structs. The last belongs to the alien derelicts team. The CTinfo structure's TeamType and ClanType fields are initialized from the MAX.INI file's red_team_player to gray_team_player and red_team_clan to gray_team_clan parameters. There is no alien_team_player nor alien_team_clan INI parameter so the alien CTinfo TeamType field is initialized from the red_team_clan INI parameter and the ClanType field is initialized from the DIGITAL INI group ID (returns 0 which corresponds to RANDOM clan).

66. If a unit is selected the primary selection marker starts to blink at a 1 second rate. If a multiselect rectangle is redrawn by clicking and dragging the mouse pointer the primary marker gets redrawn as well and its draw function toggles the marker state much faster. The primary marker should not start to blink like crazy in such events.

67. When a supply truck tries to reload a unit and it's storage container is empty the following alert message is shown: "insufficient material in storage to reload unit.". The message should start with capital letter. Same goes for repairing a unit.

68. The path generator erroneously cannot find path to a cell location in corner cases. Note: the audio can be unmuted on this video clip.
    <br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_68.mp4" type="video/mp4">
    <track label="English" kind="subtitles" srclang="en" src="{{ site.baseurl }}/assets/clips/defect_68.vtt" default>
    </video>
    In the above case an airplane is hovering above a bridge which confuses the path generator.

69. The path generator inconsistently finds a path to a cell where it is questionable whether the given unit should be allowed to move at all. Note: the audio can be unmuted on this video clip.
    <br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_69.mp4" type="video/mp4">
    <track label="English" kind="subtitles" srclang="en" src="{{ site.baseurl }}/assets/clips/defect_69.vtt" default>
    </video>
    Is it intentional to allow ships to coast type cells in case there is a bridge at the same cell?

70. The allocation menu uses a function (cseg01:0008A5FB) to change color temperature of highlighted text background. When the algorithm finds a better color match in the system color palette based on color distance the worse distance is saved instead of the better one.
    <br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
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
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_80.mp4" type="video/mp4">
    </video>
    The transporter's activate event should check the destination cell type.

81. The algorithm (cseg01:00076144) that draws attack and scan ranges on screen misbehaves at extreme ranges combined with high level of zoom.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_81.mp4" type="video/mp4">
    </video>

82. Redraw order of the attack and scan range markers and the message area is indeterministic in case a hoovering unit overlaps with them.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_82.mp4" type="video/mp4">
    </video>
    This is not the only flickering behaviour.

83. The function (cseg01:000CE775) responsible for the reports menu's mouse left click event handling tests against incorrect window boundaries. The active draw area is 459 x 448 pixels. The on click event handler checks for 459 x 459 pixels by calling Image::GetWidth() for both x and y axis dimensions. this must be a simple copy paste error.

84. The reports menu constructor (cseg01:000CC8D0) calls GNW win_draw() twice right after each other. First a call is enabled by the class specific draw function via function parameter, then right after the draw function there is another call.

85. Enemy land units do not sink or get destroyed after a water platform or similar is demolished beneath them. The units can also move from a water cell back to a land cell if it is next to them. Land mines do not get destroyed if the structure below them gets demolished. Sea mine layers can detect floating land mines too. Building a landing pad above a road above a water platform makes the road disappear. Further investigation is required whether the road unit gets "properly" destroyed or is it leaking memory. Destoying a landing pad above a water platform while an aircraft is landed there leaves the aircraft on the ground in awaiting state. The aircraft is targetable in this state by land units. The aircraft can take off into air from the water mass.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_85.mp4" type="video/mp4">
    </video>
    In my opinion land mines should also blow up just like connectors and roads built above water platforms and bridges do. Any non hybrid land unit should sink to the bottom of the sea too. If an aircraft is landed on a landing pad on a water platform and the landing pad gets demolished the aircraft should either take off into air, or should sink to the bottom of the sea.

86. The maxsuper cheat code increases the scan and attack ranges. The game redraws the markers incorrectly until there is a movement of screen or unit.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_86.mp4" type="video/mp4">
    </video>

87. Both the TaskMineAssisstant and TaskFrontierAssistant classes have the same type id. It is not known yet whether this is a copy paste error or done on purpose in which case it would be an unnecessary design decision not a defect. There are other tasks with equal type identifiers too.

88. The TaskRemoveRubble task implements a function (cseg01:0003A57F) to dump cargo to the closest complex. This task is used by computer players. The algorithm first searches for a building A that could store some of the raw materials that the bulldozer or mine layer or similar holds, then searches for the closest building B within the same complex of the previously found building A and orders the bulldozer to move to **building A** instead of the closest building within the complex which would be building B. It is another topic why Supply Trucks with free capacity are not even considered by the AI or that it is not even checked how much raw materials can be stored within the selected complex.

89. Both the TaskConnectionAssistant and TaskPowerAssistant classes have the same type id. It is not known yet whether this is a copy paste error or done on purpose in which case it would be an unnecessary design decision not a defect. There are other tasks with equal type identifiers too.

90. The TaskCreateUnit task implements a function (cseg01:00038CC9) that tries to optimize resource consumption so that demanded units could be created (faster). If necessary the algorithm tries to increase fuel reserves by shutting down inessential infrastructure, namely Eco-Spheres and Gold Refineries. There is a copy paste error when counting the inessential building types so Eco-Spheres are tested twice, counted once, and Gold Refineries do not add to the count. Due to this defect basically Gold Refineries are not considered at all by the algorithm.

91. The TaskManager implements a method (cseg01:00043F3F) to estimate the total heap memory usage by the AI. The game introduced a priority list for reminders in one of the post release patches and instead of considering the dynamic memory usage of the priority reminders list it considers the count of reminders stored by the list by calling the wrong smart list function. Due to this and other similar defects the overall memory usage was probably underestimated by the developers. The game places huge buffers on the stack as well which makes stack usage hard to estimate. GNW supported custom C memory allocators that could measure dynamic memory usage, but as M.A.X. is mostly implemented in C++ this GNW feature was somewhat useless.

92. The DefenseManager class tracks the unit types it needs in a SmartObjectArray\<ResourceID\> object and the units it already manages in a SmartList\<UnitInfo\> object. If a new unit is added to the manager the given unit type is removed from the array of needed unit types and the unit is added to the list of managed units. The method (cseg01:0001D2C8) responsible for this operation calls the SmartObjectArray\<ResourceID\>::Remove() method which attempts to remove an array element by index or position while the manager method passes the unit type which is obviously not equal to the array position where the unit type was originally stored. No memory corruption occurs as the SmartObjectArray is intelligent enough not to remove elements from beyond the allocated size of the array. But the algorithm is totally broken that relies on the unit type array contents. The root cause of the defect is probably a misunderstood interface as while the SmartList\<UnitInfo\>::Remove() API searches for the passed element instance and if found it is removed from the linked list the SmartObjectArray\<ResourceID\>::Remove() behaves totally different.

93. There is a function to evaluate assaults (cseg01:00018D4A) which first reads the team member from a UnitInfo object pointer and afterwards tests whether the pointer was null.

94. **[Fixed]** The TaskManageBuildings class implements a method (cseg01:00030C63) which processes a two dimensional matrix. The matrix dimensions are supposed to be the same as the game map grid dimensions. Instead of the X and Y axes the Y axis is used twice in a loop. As the game only implements fixed 112 x 112 grid maps the defect is latent.

95. **[Fixed]** The TaskManageBuildings class implements a method (cseg01:0003177D) which marks path ways around buildings. The function also attempts to mark the path ways around buildings that are scheduled to be built, but the tasks list iterator variable is used in two loops and the second loop does not reinitialize the iterator to the beginning of the list effectively skipping the entire list of planned buildings.

96. The TaskManageBuildings class implements a method (cseg01:0003285C) which determines whether a site is reasonably safe for construction purposes. There is a minor risk that `++marker_index` might lead to out of bounds access that could smash the stack and depending on the calling convention of the ABI this could lead to code runaway.

97. **[Fixed]** The TaskManageBuildings class implements a method (cseg01:00033E27) to get the amount of units of a particular type a team has and plans to build. The method returns the count on 8 bits. Over long game sessions it is possible to have more than 255 units of the same type in which case the result gets truncated that could lead to unintended AI behaviors in corner cases.

98. Depending on the zoom level the draw order of shadows could become incorrect in corner cases.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_98.mp4" type="video/mp4">
    </video>

99. The unit status displayed in the top left corner of the screen shows transferring state when a mine layer laying state is changed to place or remove until an actual lay mine or remove mine action is executed. Similar issues can be observed with the exploding status on hit.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_99.mp4" type="video/mp4">
    </video>

100. It is possible to take control over non friendly mobile units or reactivate friendly disabled units and exploit this in various ways. Thanks for Sal from the M.A.X. Reloaded community for reporting this issue.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_100.mp4" type="video/mp4">
    </video>
<br>
The defect is closely related to the group command. Steps to reproduce the issue: left click an enemy mobile unit that has unspent movement points and is not building. Press down the shift key and left click a friendly mobile unit with unspent movement points. This creates a new unit group which places the enemy and the friendly units into the same player controlled group. Left click once more on the friendly unit to make it the actively selected unit instead of the enemy. Now the shift key can be released. Order the friendly unit to move somewhere and the enemy unit will follow the movement direction and distance as long as this is possible.
- If the enemy unit is a bulldozer that is clearing a site the game will let the bulldozer unit leave the work zone corrupting the game and unit states affecting game save files too.
- If the enemy unit is a mine layer that is laying or removing mines, the enemy unit will continue to lay or remove mines.
- If the enemy unit is actually a disabled alien derelict unit, it could be reactivated this way and computer players will consider them as threats typically destroying their own potential assets.
- If the enemy unit reserves shots as a defensive measure, we can deplete their reserves unless the unit has the Move & Fire attribute.
- The enemy unit can be deliberately moved over a mine to get it blown to pieces.
- The enemy unit can be moved to a location where it is not anymore protected by defensive structures like missile launchers or stationary anti-aircraft turrets or units that could detect infiltrators.
- A disabled friendly mobile unit could be immediately reactivated this way.
<br>
<br>

101. **[Fixed]** The units manager module implements a function (cseg01:00100464) to transfer materials. The function sets up two smart pointers. One for the cargo source unit which is initialized to the UnitInfo object that is given as function argument and another for the cargo destination unit which is initialized using the previous UnitInfo object's parent unit. Depending on the sign of the cargo amount to transfer between the two units the source and destination smart pointers to the units may be swapped. After performing the transfer the function checks whether the unit information panel on the GUI needs to be updated. An update to the GUI is required if the currently selected unit is affected by the transfer. The function compares the currently selected unit against the unit which was the input argument to the function and the destination unit which is held by one of the smart pointers. As in case of negative transfer amounts the two smart pointers are swapped, it could happen that the unit held by the source smart pointer is not tested by the function. The function should have simply tested the source and destination smart pointers instead of the input argument plus one of the smart pointers. The defect could be considered latent as the GUI is rendered in every frame anyways.

102. **[Fixed]** The AiPlayer class implements a method (cseg01:000627F0) to filter out stationary or regenerating units from weight tables. When weight tables are initialized unit types that are not allowed to be built during a scenario or training mission are saved into the tables with the unit type set to INVALID_ID. The method iterates through the weight table entries and uses the unit type as index into the BaseUnit base units array. The INVALID_ID has a value of -1 represented as 0xFFFF on 16 bits or 0xFFFFFFFF on 32 bits. On modern operating systems such out of bounds accesses, `UnitsManager_BaseUnits[INVALID_ID].flags`, could cause segmentation faults and the game could crash immediately. The original game under DOS could potentially corrupt a byte by setting it to zero at a semi random address.

103. **[Fixed]** The AiPlayer class implements a method (cseg01:0006317C) that enumerates unit types that could fight against a targeted enemy unit type and stores the results into a weight table. When weight tables are initialized unit types that are not allowed to be built during a scenario or training mission are saved into the tables with the unit type set to INVALID_ID. The method filters out unit types that cannot get within range of the enemy unit type due to the map surface by iterating through the weight table entries and testing the unit type flags using the BaseUnit base units array. The INVALID_ID has a value of -1 represented as 0xFFFF on 16 bits or 0xFFFFFFFF on 32 bits. On modern operating systems such out of bounds accesses, `UnitsManager_BaseUnits[INVALID_ID].flags`, could cause segmentation faults and the game could crash immediately. The original game under DOS could potentially corrupt a byte by setting it to zero at a semi random address.

104. **[Fixed]** The TaskKillUnit class implements a method (cseg01:0001BAD5) to gather friendly unit types that can fight against an enemy unit type. The method uses a weight table which could contain INVALID_ID as unit type. Weights of unit types are set to 0 if the unit type cannot be built before the final turn of the mission or in case the unit types are regenerating types. The conditions are tested using the BaseUnit base units array and the unit type as index. Using INVALID_ID as array index, `UnitsManager_BaseUnits[INVALID_ID].flags`, could lead to segmentation faults and the game could crash immediately. The original game under DOS could potentially corrupt a byte by setting it to zero at a semi random address.

105. **[Fixed]** The master builder unit type has no sprite and shadow assets. Due to this the unit asset loader method does not set the sprite_bounds and shadow_bounds members of the UnitInfo class before use. The members are not initialized by the UnitInfo constructors either. There is a function (cseg01:000E9FA8) to report new dirty zones for the renderer. As the master builder sprite and shadow bounds are uninitialized random garbage, the renderer is presented with potentially ill formed data. The sprite and shadow bounds should be initialized to safe default values by the UnitInfo constructors. Additionally the function that takes new dirty zones could potentially discard any bounds that have no valid size. The master builder unit is used by the game during the landing zone selection screen. A master builder unit is spawned at the selected landing spot.

106. The ini file module implements a function (cseg01:000C36AC) to save an ini configuration file and release internal buffers. The internal buffer memory is leaked in case the ini file cannot be opened.

107. The main menu setup and in-game preferences windows do not exit active edit menus when the user presses the help button. Pressing most buttons emit keyboard key or scan codes. In case of the help button a question mark is emitted which is fed into the active edit control instead of switching to active help mode in the given context menu. Pressing the help button shall exit the edit control just like enter and escape and similar buttons do.

108. **[Fixed]** The in-game credits spells incorrectly `Albert LIoyd Olson for Four Bars Entertainment` as `Albert Olsen for Four Bars Intertainment`. The game manual (MC-ICD-082-GEN) is correct.

109. **[Fixed]** There is a function (cseg01:000CEB0D) to enumerate team units to be considered for the reports menu. Only such units are added to the list that fulfill certain conditions. There is a condition that says if the unit order is IDLE then the unit can be added to the list if the unit order is not MOVE_TO_ATTACK which is always true. There are many tests that check IDLE order and BUILDING_READY state in combination. The MOVE_TO_ATTACK order code and the BUILDING_READY state code are the same.

110. **[Fixed]** The units report window in the reports menu renders (cseg01:000CC088) unit statistics for the RESEARCH unit tpye incorrectly. The unit statistics shown should be Hits, Usage, Total while Hits, Usage, Usage is shown.

111. There is a function (cseg01:000FAA72) to initialize in-game popup context menu event handlers and some other possibly related resources that have something to do with delayed reactions. The team specific UnitsManager_TeamInfo structures are tested for their unit_type members even though the objects are initialized only at a later stage. Thus a delayed reactions related globally allocated static variable, UnitsManager_Team, is left to its default value which is PLAYER_TEAM_RED or set to a stale value from a previous game session.

112. Certain crater inland tiles use coastline color animation color palette indices which is assumed to be unintentional based on the visual look of the affected landscape elements.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_112.mp4" type="video/mp4">
    </video>
<br>

113. There is a function to load saved games (cseg01:000D8EB6) that reads the saved game file header which redundantly contains the team name, type and clan specifications. Only the team name is used from the saved game file header. The function calls a resource manager function (cseg01:000D6316) to initialize TeamInfo objects which initializes TeamInfo objects with stale ini configuration settings for team type and clan specifications instead of using the specifications from the already read saved game file header. The init function is called from three different call sites implementing different game setup scenarios, thus to eliminate the use of stale data in case of loading a previously saved game the ini configuration settings needs to be updated based on the file header before the call to the TeamInfo initialization function. Defect 65 is also relevant for this issue.

114. The game logic does not consider whether a stealth unit would be revealed at a target location after stepping aside from an enemy unit. The sole purpose of stepping aside is to remain hidden. In the demonstrated scenario the enemy unit would have had sufficient movement or speed points to move to the west instead of east. Movement to the east is favored due to the cheaper movement costs even if that reveals the stealth unit.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_114.mp4" type="video/mp4">
    </video>
<br>

115. Redraw order of the hits, names and survey markers are indeterministic in case of context menu redraws.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_115.mp4" type="video/mp4">
    </video>

116. The scan and range markers of selected unit is not cleared if another unit is selected via the reports menu without moving the main map window in the process.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_116.mp4" type="video/mp4">
    </video>

117. Generic mission loss conditions do not consider the case when no mining stations remain, but we have at least one constructor unit and materials left to build a new one, although the constructor is not allowed to build mining stations in the given mission.

118. **[Fixed]** The in-game preferences menu implements a method (cseg01:000C29AD) to process human input. There is a corner case that leads to an out of bounds access. In a turn based game end the player's turn to start a computer turn. Press the in-game preferences button to open up the preferences window while the computer player is thinking. Close the preferences window by using the Done button. Open up the in-game preferences window a second time. Now press the files button in the background while the preferences window is still active. The preferences menu receives a key code of 4 while the minimum should be the virtual key code 1002 which results in an out of bounds access to a menu button array and the game crashes. The root cause of the issue is that there is a mechanism to disable the in-game menu buttons while popup windows are active which is triggered by the preferences window as well, but it does not work properly while computer players take turns. On opening up the preferences window the mechanism (cseg01:0009FC13) checks whether in-game menu buttons are enabled and if so the algorithm disables the in-game menu buttons and sets a state variable to disabled state. On exiting the preferences window the mechanism (cseg01:0009FA70) checks the state variable and if it is disabled state it enables back all buttons, but only sets the state variable to enabled again if the player that is taking its turn is a human one. The next time the preferences window is opened up the mechanism finds that the state variable is set to disabled state already and buttons in the background all remain active.

119. **[Fixed]** Certain missions, like most training missions, limit the type of units that could be constructed or manufactured. Computer players build weight tables that help them decide what type of units and buildings they need the most depending on the features or characteristics of the world and other aspects. In case a unit type is disabled in a given mission the INVALID_ID unit type value is used replacing the original unit type. There is a function (cseg01:000198DE) that ignores the fact that list elements may use the INVALID_ID unit type value which is -1 or 0xFFFF if the value is interpreted as an unsigned array index in which case out of bounds access is attempted to a local stack array that leads to random crashes or even worse, to memory corruption.

120. **[Fixed]** The constructor of the TransferMenu class (cseg01:00104C63) instantiates a Button class object for an arrow icon which is not destructed by the destructor (cseg01:0010564C). This defect leaks at least 56 bytes of heap memory after each cargo transfer.

121. **[Fixed]** Heat maps of the units manager class are allocated in heap memory by various initialization functions. The alien derelicts team is handled in a special way by most initialization routines probably because aliens were added to the game late in the development process (alien derelicts are just a hack in the code base). The function responsible to clean up heat maps forget to delete the alien heat maps. This defect leaks at least 36.75 KiB of heap memory per game session.

122. **[Fixed]** Computer players may request new or alternative ground paths with a maximum path cost set to zero in case the unit's actual speed points are all depleted. The paths manager processes given ground path requests in two stages. First a viable path is generated to the destination and if found the unit gets the part of the path that it can take in the given turn up to the limit of the maximum path cost. If the maximum cost is set to zero it means that the unit cannot move at all which means that if a viable path is found to the destination a GroundPath object that is instantiated will contain an empty steps array. Dereferencing an empty smart array results in an out of bounds access. Such accesses read random memory contents and operations conclude gracefully without crashes on DOS systems. On modern operating systems such out of bounds accesses cause segmentation faults and the game crashes immediately. The GroundPath method (cseg01:000BA570) that dereferences the steps array checks the unit speed points, which is zero, and stops processing the already read random garbage step data.
The proposed defect fix will not instantiate a ground path if the determinted path (cseg01:000BCE26) steps up to the maximum path cost is zero and the maximum path cost will be based upon the unit's base speed value instead of the actual speed points remaining.

123. **[Fixed]** The MenuLandingSequence class is instantiated by the game as a static global object. The class implements a method to initialize user interface controls, like buttons or images, on demand. Destruction of the static global object is performed at some point by the C++ runtime while the application is being terminated. If the user interface controls were not deinitialized by the application, the destructor (cseg01:0009133A) attempts to do so by calling the related class method. When GNW or the application detects an unrecoverable error an exit-like function is called that cleans up after GNW freeing all window manager resources in the process. In corner cases the MenuLandingSequence class interface controls are initialized by the application, but then the application detects an error before calling the deinitialization method. GNW calls the exit-like function that frees the window manager resources and finally the static global class object destructor is called by the C++ runtime that deletes the previously initialized user interface controls, like buttons, that attempt to read from the already freed window surface they belonged to and also attempt to write pixels to the window surface from within their destructors. This leads to out of bounds read and write accesses under DOS systems that lead to segmentation faults on modern operating systems.

124. **[Fixed]** The function (cseg01:000198DE) that is described in defect 119 has another similar defect. The function checks whether the manufacturer of a desired unit type is available already or one has to be manufacted as well. If the assessed team already has a unit from the manufacturer unit type it is removed from the list of desired unit types. The weight of the unit type is set to zero. There are unit types, like the alien gun boat (juggernaut), that cannot be produced by any of the plants. In such cases the manufacturer unit type is set to INVALID_ID in which case the function again attempts an out of bounds access to a local stack array that leads to random crashes or even worse, to memory corruption. Disabled alien gun boats can be captured by infiltrators.

125. **[Fixed]** If defect 65 sets the alien derelict team_type coincidentally to `TEAM_TYPE_COMPUTER` or a decimal value of 2 based on the red_team_clan ini parameter which corresponds to `TEAM_CLAN_CRIMSON_PATH` than an AI function (cseg01:00048D5F) tries to manipulate `AiPlayer_Teams[unit->team]` where the team index is the alien derelict team with a decimal value of 4 while the maximum array index of the AiPlayer_Teams array is three as only four objects are allocated. There is no fifth object for the alien team. This out of bounds access corrupts some other global statically allocated memory contents within the `AiPlayer_ThreatMaps` array which in turn crashes the game whenever the affected threat map objects are attempted to be destructed.

126. **[Fixed]** There is a TaskAttack class method (cseg01:00023755) which issues conditional unit movement orders to move closer to target enemy units if viable. In case the target unit is owned by the alien derelict team, which team does not have heat maps allocated, the algorithm dereferences a null pointer offset by map coordinates which leads to segmentation fault on modern operating systems. The proposed defect fix handles non allocated heat maps as if the heat map would indicate no risks for the pursuer unit.

127. Deploying new unit with either attack or scan range marker enabled renders an incomplete circle. As soon as the screen changes the problem disappears and the newly rendered circle is complete.

128. **[Fixed]** The TaskAttack class has a method (cseg01:0002685E) to determine whether pursued enemy units are within attack range. The TaskKillUnit child tasks could in corner cases be assigned to recently destroyed units in which case the function dereferences null which leads to segmentation faults on modern operating systems. The proposed defect fix will not garbage collect the completed TaskKillUnit child task here by removing itself as this is surely performed by some other function somewhere else. Instead the affected method will simply report that the non existent target unit does not need to be attacked.

129. The game forgets all previously detected enemy mines on loading a previously saved game. It is assumed that this is not the intended behavior. As an end user I would expect the game to continue from where I left off.

130. In multiplayer games cheaters are supposed to be punished by the game. But enabled cheats are not reset between game sessions. If a cheater starts the game, loads a single player mission, enables a cheat code to see enemy armies, then quits the single player mission and enters a multiplayer game the cheat code is still active without receiving any punishment for the ill gotten advantages. This can be used as an exploit in multiplayer games.

131. The UnitInfo class member `image_index_max` of MK I Infiltrator 6 in training mission 12 (MD5 hash: bfca7a73ad7d2c927b4110f16803d417 \*SAVE12.TRA) is initialized to 0xFFFF or -1 at file offset 0xFD7B. Due to this the walk animation of the infiltrator unit does not work.

132. **[Fixed]** There is a typo in the description of the Mine Layer land unit. "They cannot remove enemy minefields - those most be exploded with gunfire and rockets." -> must be.

133. Land and sea mines do not blow up if enemy units are deployed or activated upon them.

134. **[Fixed]** The TaskCheckAssaults task implements the RemoveUnit (cseg01:0001CB93) interface. The implementation checks whether the passed unit instance is held by a member variable of the task. This task is unique in that it holds a SmartList<UnitInfo> iterator. The RemoveUnit implementation assumes that the iterator is always pointing to a valid ListNode object which seems not to be true. The method dereferences null which leads to segmentation faults on modern operating systems. This happens most of the time when the player reloads a previously saved game in which case the ongoing game is first cleaned up including the task manager. It is unclear whether the list iterator should never be null by design, thus the proposed defect fix is to simply check whether the iterator is null.

135. Adding a unit to a group by mouse pointer selection or shift + left click prefers ground units in case air and ground units are both present at a grid cell which is counterintuitive.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_135.mp4" type="video/mp4">
    </video>

136. It is possible to obtain near infinite speed points for air units moving in groups which could be used as an exploit.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_136.mp4" type="video/mp4">
    </video>
<br>
The defect is closely related to the group command and to partial speed points stored for later use to compensate for earlier non-integer movement costs. Even though the unit statistics screen indicates that there are always maximum speed points left after the issue occurs, in reality the speed or group_speed member of the UnitInfo object underflows to -1 which is stored as 0xFF in the `unsigned char` containers. So in fact the player that exploits the issue gains additional 255+ speed points to move the affected units around. The issue is potentially reproducible using ground and sea units as well but there it is much more difficult due to the more sophisticated movement cost calculations.

137. Most hand crafted missions store incorrect unit counter values in team info structures. This causes summary reports at the end of missions to show incorrect unit statistics. For example campaign mission 3 unit counters indicate that at some point the designers gave the red team 9 armoured personnel carriers and 17 infiltrators before they changed their minds.

138. **[Fixed]** Heat maps are only allocated for teams that are in play (aliens never have heat maps). Unallocated heat maps are set to null. The TaskMove class has a method (cseg01:0004D6E4) that tries to speed up movement of infantry and infiltrator units by using an armoured personnel carrier unless enemy heat maps indicate that the unit to be transported is in danger in which case air transport is attempted. The method checks all enemy heat maps even if they are null which leads to segmentation faults on modern operating systems.

139. **[Fixed]** The chat / goal button label is wrongly set to chat on loading (cseg01:000D8EB6) any game type that has a set goal if this is the first started game in the game session. Steps to reproduce: start a campaign or training game which creates an autosave in save slot 10. Fully exit the game. Start the game again and load save slot 10. The goal button will print chat until redraw due to mouse hover over or similar event. The root cause is that the saved game loader first initializes the in-game GUI buttons and sets the mission index from the saved file to non zero which would normally set the label of the button to be goal.

140. There is a typo in the campaign mission 5 description. "You land in in an out-of-the-way island." -> You land on an...

141. The hover member of the UnitInfo class member's `flags` bitfield of most air units in campaign mission 5 (09376d9fbd57a1637cb22d09676d61e0 \*SAVE5.CAM) is initialized to 0 which means that the air unit is landed.
<br>
    <video class="embed-video" preload="metadata" controls loop muted playsinline>
    <source src="{{ site.baseurl }}/assets/clips/defect_141.mp4" type="video/mp4">
    </video>
<br>
By normal means air units cannot land on plain ground. It is assumed that it was not intentional to allow the player to destroy the enemy's only two AWACs right at the beginning of the mission in case of turn based game mode. In simultaneous moves mode the computer has a chance to move the AWACs away before the player could react.

142. **[Fixed]** There is a function (cseg01:0001284B) to check whether there is a second selectable unit in a grid cell that does not consider that the used query function (cseg01:000136D8) could return null in which case the earlier function dereferences null which leads to segmentation faults on modern operating systems.

143. **[Fixed]** There is a function (cseg01:00087004) to draw a glowing green text box above units on the tactical map in case a research topic or a construction is ready. In case of construction jobs the function receives the builder unit, like an engineer, as function parameter and the parent of the builder unit is set to the newly constructed unit. When an engineer finishes building a connector unit it is automatically deployed by the game without waiting for the player to issue the deployment. Due to this the engineer automatically releases the new unit from being its parent and as such this function dereferences null which leads to segmentation faults on modern operating systems.

144. **[Fixed]** There is a function (cseg01:000970AE) to determine the mouse cursor sprite. When a builder unit is the actively selected unit and the unit just finished construction of another unit and the mouse cursor is next to this another unit the cursor is supposed to be set to a certain type. In case the engineer is building connectors in path build mode the builder unit’s parent is set to null in which case the function dereferences null which leads to segmentation faults on modern operating systems.

145. The bottom and right sides of the tactical map have a 1 pixel wide line of corrupted pixels that are not updaing correctly when the screen moves.

146. There is a function to set a new mouse cursor shape (cseg01:000C0D94). The function conditionally calls the mouse_hide() and mouse_show() GNW API functions. The GNW API functions copy pixels from the active window surface into the bounds of the newly selected cursor shape to handle transparent cursor pixels. In case the cursor is changed while a new window is being rendered into the window back buffer the monitor screen may still show the old window contents to the player while the game already redrawn part of the contents in the back buffer for the new window. In such corner cases the GNW API functions copy the new unfinished window's already rendered pixels and blits the new cursor with the wrong window background around it to the monitor screen which creates visual glitches until the new window gets fully rendered and blitted to the monitor screen.

147. **[Fixed]** The top right instrument image (`PNLSEQ_5`) is 24 pixels high while all other images in the sequence were 17 pixels only. The respective Window Manager window boundaries are defined to be `{.ulx=384, .uly=0, .lrx=639, .lry=22}`. The bottom pixel row is cropped which effectively cuts the transition part between the shaded and bright window frame area.

148. The scan range of a working constructor is not centered on the unit sprite.
<br>
    <img src="{{ site.baseurl }}/assets/images/defect_148.jpg" alt="defect 148" width="740"> 
<br>

149. Campaign mission 6 and 7 do not end even if the player's victory is obvious. Mission 6 & 7 do not have defined rules to win contrary to most other campaign missions. There is a function (cseg01:0008EFA5) that checks end game conditions. First the loss conditions are tested (cseg01:0008C1BD) which evaluates the enemy to have lost the game already. Then win conditions (cseg01:0008CD4D) are tested and as there are no explicit rules defined the function returns state pending. State pending instead of state generic means that generic rules do not apply thus the algorithm concludes that if the win conditions are pending then the fact that the enemy is utterly destroyed does not matter. After the turns limit ends at 100 turns the game makes a final conclusion that the player has won the game in case of mission 6 while for mission 7 the highest team score wins.

150. In case of (hot seat) multiplayer games if one of the human players cancel landing zone selection with the ESC key while exclusion zones overlap the game progresses the game loop one frame and tries to determine the mouse cursor which is guaranteed to point over the actively selected master builder unit which is (mis)used as the landing zone's exclusion and warning zone markers. As the master builder unit has an IDLE order the cursor selection algorithm (cseg01:00097FB2) assumes that the unit should have a parent which is not true and the game dereferences a null pointer which leads to segmentation faults on modern operating systems.

151. Two connectors at grid positions 45,70 and 45,71 overlap a depot in campaign mission 8 (MD5 hash: 97379ca828ca8017ea00f728b3b86941 \*SAVE8.CAM) which creates visual glitches. Normally connectors are removed by the game when a building is built over them.

152. Air units are not allowed to move to a grid cell where an enemy mine is detected by the mouse cursor selection algorithm. This is counter intuitive and creates a tactical disadvantage if the enemy creates a huge minefield along its borders as in such cases the player's air units are simply unable to go near the enemy base.

153. Campaign missions 2, 5, 9, multiplayer scenario 6, single player scenario 1, 2, 4, 11, 12, 14, 16, 17, 21, 22, 23 and 24 contain message log entires that should have been removed by the authors. For example in campaign mission 9 red player (MD5 hash: 30a8de876d55f3cabb4bc96f8cb31aab \*SAVE9.CAM) has three logged messages: 1) `Begin turn 1.` 2) `Begin turn 2.` and 3) `Begin turn 2.` yet again. Probably the mission was edited at least twice.

154. Computer players evaluate whether their four primary recon unit types have sufficient scan range to avoid enemy fire coming from their targeted enemy team in every turn and request scan unit attribute upgrades if necessary. The algorithm that determines the targeted enemy (cseg01:00066677) loops through all four teams from red to gray and only filters out `TEAM_TYPE_COMPUTER` teams. At any time there are less than four teams in play and the gray team is not human nor computer the algorithm selects gray team, having team type `TEAM_TYPE_NONE` or `TEAM_TYPE_ELIMINATED`, as the targeted enemy even if that team never existed or is already gone. This effectively means that in many scenarios the computer players do not upgrade their recon units that will be blown to pieces by their enemies due to their limited scan ranges thanks to this defect. If the red team is human while only the green and blue are computers then the two computers will not attack each other as their arch enemy is set to gray that does not exist. If only two to three computers are in play they will not attack each other. This feature was introduced into the game in one of the late patches. For example there is another algorithm that cancels ongoing building constructions if any enemy unit has damage potential overlapping with the affected construction unit. A loitering enemy scout around the computer team's base that cannot be attacked by the computer player could force constructions to be canceled.

155. **[Fixed]** The AI generates so-called damage potential maps and a minefield map. To determine the damage potential at a given grid cell all possible attack ratings that could hit the cell is summed up. The number of shots that are required to inflict the summed up damage from the attack ratings is also calculated. The armor rating of the friendly unit for which the damage potential map is calculated is used to reduce the damage potential by the number of shots suffered multiplied by the armor rating. The minefield map is taken into consideration by adding plus one shot and the inflicted damage of the mine, reducing the damage total by one shot multiplied by the friendly unit's armor rating. To optimize this time consuming process the AI caches a maximum of 10 previously calculated damage potential maps. The maps are categorized into use cases or unit classes. If a friendly unit needs to calculate the damage potential map and the friendly unit's class is found in the cache, then the only parameter that could be different is the armor rating. There is a function (cseg01:000593E1) to update a damage potential map by the difference in armor ratings between two units of the same class. Unfortunately the given function is defective. The function uses a nested for loop to walk two 2D matrices using pointer arithmetic. In pseudo code the nested loop looks as follows:
```cpp
    for (int grid_x = 0; grid_x < dimension.x; ++grid_x) {
            for (int grid_y = 0; grid_y < dimension.y; ++grid_y) {
                *damage_potential_map[grid_x][0] += shots_map[grid_x][grid_y] * difference;
            }
    }
```
The pseudo code does not use pointer arithmetic for better clarity. Obviously this means that friendly units of the same class with huge armor rating differences will think that they are either near invincible or insufferably vulnerable in the topmost cell row of the tactical map.

156. **[Fixed]** There is a function (cseg01:00023EAC) to determine the nearest destination for an attacker that does not require support from a transporter unit. The function attempts to use heat maps to select safe destinations. The alien derelict player does not have heat maps in which case the game dereferences null which leads to segmentation faults on modern operating systems.

157. **[Fixed]** There is a function (cseg01:000F49C5) to change the team of a unit. The source team loses sight of its stolen unit depending on the team specific heat maps. When an alien derelict unit is stolen by an infiltrator the source team has no heat maps in which case the game dereferences null which leads to segmentation faults on modern operating systems.

158. Smarter computer players do not destroy alien derelict units instead always attempt to steal them. There is no dedicated task manager task type to infiltrate enemy bases or steal enemy units. The game creates a `TaskKillUnit` type task and assigns an infiltrator as the leader. The below screenshot depicts a corner case where infiltrators are close enough to the target enemy unit, an alien ship, so transporter units cannot be asked for help, and other friendly units participating in the kill unit task as supporters are actually not allowed to attack the alien derelict unit by the “smart” AI, but gather around the kill unit task target regardless. The infiltrator cannot access the grid cell to steal the alien derelict, the supporter units do not know that they are in the way, air transporters are not allowed to move the infiltrator from the near vicinity to the right spot for doing its infiltration business.
<br>
    <img src="{{ site.baseurl }}/assets/images/defect_158.jpg" alt="defect 158" width="740"> 
<br>
This is a good example for a complex soft lock situation.

159. The English in-game help entry for TOPICS_ICONS within the RESEARCH_SETUP group contains a typo. "appropreiate" -> appropriate.

160. The English in-game help entries UPGRADE_1 to UPGRADE_4 within the HANGAR_SETUP group are copy pasted from the repair help menu entries.

161. In case an enemy infiltrator is visible only due to cheat code and the unit stands on a friendly road or similar ground cover class unit, it is not possible to select the enemy infiltrator. It is assumed that cheaters are not intentionally penalized by the game in single player modes, thus the behavior is considered to be a defect.

162. **[Fixed]** The TaskManager class implements a function (cseg01:000453AC) to free up UnitInfo objects for tasks to use. In corner cases, for example when a new mission is started after a previous one is finished and the AI module resets the TaskManager instance, UnitInfo objects reference count tracked by SmartPointers reach zero references eventually within the TaskManager function's body and the raw pointer passed to it is deleted early, thus already freed up memory is getting dereferenced later which leads to segmentation faults on modern operating systems. It is unclear whether the TaskManager or the affected Task derived classes are supposed to protect against early deletions that call the TaskManager API. The proposed defect fix protects against the proven call site which causes segmentation faults, thus protects the called TaskManager API function instead of the caller Task derived class types.

163. **[Fixed]** When a new mission is started after a previous one and the AI module resets the TaskManager instance, tasks of type TaskRemoveMines attempt to check the map surface via an Access module function (cseg01:00013CFD) after the mission start-up code already deleted the map object as part of its reinitialization sequence. The function does not check whether the map object is valid which leads to segmentation faults on modern operating systems. It is difficult to tell what would be the appropriate way to initialize, reinitialize and uninitialize all the game assets and game objects, thus the defect fix does not attempt to fix the architecture design of the original game. Instead a simple defect fix is proposed that simply tests for null pointer within the Access module function that caused the detected segmentation fault.

164. **[Fixed]** The TaskCreateBuilding class implements a method (cseg01:0002E07B) to finish build jobs. In corner cases the last SmartPointer reference to the task object itself gets removed and the object is prematurely deleted during the execution of another class virtual method (cseg01:0002CF2C). When the earlier method gets back control and tries to use object specific member variables that hold UnitInfo object references there is a chance that the already freed up heap memory is already reallocated for something else and a new corrupted activation order is issued based on garbage data. In other cases invalid memory addresses could be dereferenced which leads to segmentation faults on modern systems. This defect may be related to defect 12.
