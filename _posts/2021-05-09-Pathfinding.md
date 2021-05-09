---
layout: post
title:  "Debugging the Pathfinding module in M.A.X."
date:   2021-05-09
driveId: 1jSE0Z0XtmdxB_uRdG9jkE5bRJmCNpLLy/preview
categories:
excerpt_separator: <!--more-->
---
The path finding algorithm of M.A.X. was very powerful compared to other strategy games of the time. Of course it is not fair to compare turn based, simultaneous real-time and real-time games, but I do not care. Pathfinding in M.A.X. is outstanding.
<!--more-->
<br><br>
{% include drive_player.html id=page.driveId %}
<br>
  
M.A.X. had three path finding algorithms. One for construction units, one for any land and sea units and one for air units.

According to the original author of the path finding algorithms, Gus Smedstad, he's design is similar to [Dijkstra’s Algorithm](https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm) even though he only found out about Dijkstra's work later. The algorithm considers path roughness as well as direction of movement. E.g. whether the unit can find a road or bridge, needs to cross water or turn around to reach the desired destination. Units also track fractional parts of their movement points to compensate for the limitation that units can only stand at a map grid aligned position.

Air units do not really use path finding in the normal sense. They move in a straight line till the destination is reached which must be aligned to the map grid.

One interesting aspect of the design is that there is a forward as well as a backwards searcher and there is some kind of weighting that prefers the direct straight line to the destination.

The path finding module is built using at least twelve C++ classes. There are a couple of virtual base classes involved as well. As data laid out within class objects is compiler specific it will be extremely difficult to reimplement the functionality in an iterative, step by step manner.

M.A.X. has built in path debugging services. The video demonstrates all of them switched on at the same time which is of course not the intended use case for them.
