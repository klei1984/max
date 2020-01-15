---
layout: post
title:  "Winter holidays gone"
date:   2020-01-03
driveId: 16BczT14Us0HJCfsCSDuVGAQrd7oapXeZ/preview
categories:
---
I *wasted* my winter holidays, but made a lot of progress. A very basic sound manager is in place. Previously various screens did not work like the reports screen or the research selection screen, but now they are all working and the game can typically run without crashes without a debugger attached.
{% include drive_player.html id=page.driveId %}
<br><br>The sound manager does not mix sound samples, just queue them for playback. Music does not play as music samples are streamed by the game in small chunks and I did not figure out how to make SDL audio to work with pull and push approach at the same time.
<br><br>At the end of the video I loaded up my EPIC game from 2009 which lasted at the time for more than 250 turns.

