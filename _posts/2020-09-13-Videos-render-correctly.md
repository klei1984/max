---
layout: post
title:  "Videos render correctly"
date:   2020-09-13
driveId: 1tuDDLuV7aVlDQcbycH-vAOVGgN26x_Ij/preview
categories:
excerpt_separator: <!--more-->
---
Those wicked x86 opcodes tricked me...
<!--more-->
<br><br>
{% include drive_player.html id=page.driveId %}
<br>
  
I did not expect that some x86 instructions can be expressed in multiple ways. E.g. the opcode for `mov ax, cx` could be expressed as `668bc1h` as well as `6689c8h`. The Watcom compiler preferred the first notation while GCC the latter. This caused the block decoding issues seen in the previous post. Yet another reason not to design algorithms relying on self-modifying code for the x86 architecture in assembly.

Next steps are to add back audio support and to clean up all the MVE player related code.

The intro video is quite interesting. The control panels used by the Mech. Commander shows the Master Builder art, the in-game screen has the old dark UI, the engineer had the light gray sprite and the game was not called M.A.X. yet. What could be in the bottom left corner? I cannot recognise it.
