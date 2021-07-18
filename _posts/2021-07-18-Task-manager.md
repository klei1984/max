---
layout: post
title:  "Debugging the AI in M.A.X."
date:   2021-07-18
driveId: 1Wu-iM3SdjAXzNGGYUmiSyEHQrvVi49Hz/preview
categories:
excerpt_separator: <!--more-->
---
The AI or Task Manager of M.A.X. distinguishes more than 30 different types of tasks and 5 or more reminder events that are related to tasks. There is a built-in AI debugger within the game with two major features. It is possible to inspect the task tree hierarchy at any given time and it is possible to log the timeline of task manager events into a file for later offline analysis. The video in this post demonstrates the first feature.
<!--more-->
<br><br>
{% include drive_player.html id=page.driveId %}
<br>
  
I always thought that the AI of M.A.X. was so advanced for its time. I still think that it is fun to play against, but now I also see so many of its quirks.

 One typical behavior that saddens me is that if we have an engineer at point A and another at point B then it is highly probable that engineer at point A will get to build something at point B while the engineer at point B will have to build something near point A. The distance between points A and B is huge.

 Another typical behavior I observed is that the AI builds stuff to satisfy demand that only exists because we want to build stuff. It is similar to the chicken or the egg problem. For example the Light Vehicle Plant needs a Power Generator. To build one an Engineer is needed. There is no free Engineer so lets build one. To get an Engineer a Light Vehicle Plant is needed which does not operate, as it has no power source, so lets build a new plant! Of course these kinds of loops resolve themselves most of the time but still.

 It is also a bit disappointing that the AI state is not saved within saved games. The AI rebuilds itself from scratch on every load changing the list of planned tasks and activities. Maybe the AI profile is saved, but not its planned actions. There are so-called threat maps that store information about attack power and scan coverage at each grid point, and stealth units have their own detection state map and these are all saved and restored with the game, but still… probably a lot of information is lost between game sessions.

These are the individual task and reminder object types in the game (actually there are a couple more as base and abstract classes are not listed): TaskAttackReserve TaskKillUnit TaskDefenseReserve TaskCheckAssaults TaskWaitToAttack TaskPlaceMines TaskAttack TaskSupportAttack TaskEscort TaskScavenge TaskRemoveRubble TaskRemoveMines TaskFrontalAttack TaskRadarAssistant TaskConnectionAssistant TaskHabitatAssistant TaskPowerAssistant TaskDefenseAssistant TaskCreateBuilding TaskCreateUnit TaskManageBuildings TaskAutoSurvey TaskSurvey TaskFindMines TaskExplore RemindTurnStart RemindTurnEnd RemindAvailable RemindMoveFinished RemindAttack TaskObtainUnits TaskClearZone.
