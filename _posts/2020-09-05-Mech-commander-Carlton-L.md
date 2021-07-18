---
layout: post
title:  "Mech Commander - Carlton L."
date:   2020-09-05
driveId: 1EpLDEChc6gvTS2owx74wHnf7WmtoIt4J/preview
categories:
excerpt_separator: <!--more-->
---
Who is Carlton L.? Why is he a Mech Commander instead of a M.A.X. Commander? Why would a mechanical engineer fix that sealing plate with so many screws? This is not cost effective... immersion ruined! :P
  
<!--more-->
<img src="{{ site.baseurl }}/assets/images/carlton_l.jpg" alt="Image - Mech Commander - Carlton L." width="740">
  
First visible progress in playing back in-game videos. Further details are inside the post.
<br><br>
{% include drive_player.html id=page.driveId %}
<br>
  
Frame rate is wrong, audio is disabled, some of the block decoder algorithms seem to be broken still, and the renderer seems to draw half of a previous frame as well, but at least MVE video files do not crash the game any more. Even more it will most probably be possible to add captions to the intro video for languages without audio support. It is quite interesting that the technology to support captions was already present in the MVE library used by M.A.X. and that the developers utilized this capability or feature within Fallout and Descent II, but ignored it in M.A.X.

M.A.X. uses Interplay's proprietary MVE video (and audio) format and related decoder library to render the intro and a couple of cutscenes. Some of the block decoder types go as far as self-modifying their own code at run-time to optimize for code footprint and execution speed. Its amazing how far developers had to, or dared to, go to get the maximum out of the available hardware (e.g. 80486 CPU). There are many problems with self-modifying code. Most importantly its quite difficult if not impossible to do it using standard C or comparable high level languages. Most JIT recompilers also work on assembly level or use an intermediate object code level presentation of the compiled code to be mutated. Another issue to solve is that modern operating systems introduced many security improvements and in general code segments are assumed to be read-only so when the self-modifying code tries to overwrite something inside the read-only output section the process exits with a segmentation fault. Processes are generally allowed to change their own access rules to their own memory space without administrative rights so there are workarounds to this of course (e.g. mprotect on linux and VirtualProtect on windows hosts at runtime or definition of custom output sections with custom flags at compilation time).
