---
layout: post
title:  "Graphics renderer work in progress"
date:   2023-02-19
videoId: Co4y4w4A008
categories:
excerpt_separator: <!--more-->
---
<!--more-->
M.A.X. is based on GNW, a user interface and OS-abstraction library designed and programmed by Timothy Cain. The same is used in Fallout 1, Fallout 2, Atomic Bomberman and probably other games from Interplay Productions. Even though the library supported any VESA VBE interface enabled Super VGA graphics modes, M.A.X. was hardcoded to use 640 x 480 resolution in 256 color palette mode.

To support higher screen resolutions in M.A.X. each game module that draws onto the screen needs to be updated one way or another. The graphics assets, like buttons and other GUI elements, are all pre rendered 2D sprites that are optimized for the 640 x 480 screen resolution which means that to be able to add proper widescreen support new assets need to be created as well.

After many weeks of research and preparation and then many nights of mind boggling trial and error development and debugging, the first in-game screen appeared in Full HD resolution. On higher resolutions the GUI is so small that it is barely readable on my small laptop screen.
<br><br>
{% include yt_player.html id=page.videoId %}
<br>
  
It is so satisfying to see such a big portion of the tactical map instead of that teeny-weeny area from the original game.
