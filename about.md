---
layout: page
title: About
permalink: /about/
---

I always was a big fan of M.A.X. I first played the v1.0 interactive demo back in 1996 which I found on a local gamer magazine's demo CD and fell in love with it. Back then I did not know a single word in English. I had no clue about the storyline. But the graphics, sound effects and the music were all outstanding and captivating.

Most unfortunately the game has serious programming [defects](defects.md) even in its last official update, v1.04, that made it nearly impossible to reach even to one hundred turns without crashing or locking up in custom games against computer players. This always bugged me and driven me to fix it.

Back in 2009 I contacted several people from the original developer team to ask about source code, game design and whatever they remebered about the course of development.

A few intriguing quotes (the first one is from an actual Interplay associate from the time):

> Sorry, the source code for MAX is lost or unavailable. We've already tried looking for it.

> As I recall, the source was fairly quirky.  We had libraries from Fallout for some of the user interface / windowing system, and a significant number of assembler modules.  We used a command-line C compiler, the name of which escapes me right now, but it wasn't either Microsoft's or Borland's.  I think it may have been GCC, in fact, but I'm not sure.  Frankly, the source was a mess, and wouldn't port very easily.
>  
> Multiplayer was rather TCP/IP unfriendly, because it was frame-locked and didn't deal with delays in transmission very well.  Internet play was still very new at the time, and we thought strictly in terms of LAN play.  I think you'd have to re-design that from the ground up to get something reliable.
>  
> Honestly, the most you'd find useful to salvage would probably be core game logic and AI, since those are hard to reverse engineer.  Unfortunately, I can't help you even with that.

> The network support in M.A.X. was always spotty. The game is frame-locked, which means that all computers in a network game must agree at all times about
> the game data, and they exchange checks every animation frame. The synchronization error occurs because one of the two computers resolved some action
> differently than the other one. M.A.X. really predated the popularity of network games, and effort put into this aspect of the game was small compared to
> most games today.

> I'm glad a game that old still has interest in the gaming world.
> I did sprites for the vehicles and cut scenes for that version and MAX2.  Our producer might know where more assets are. Unfortunately I don't recall who did the coding for the game. I would only have the compiled code as well, sorry.

> MAX is also a favorite game of mine. Unfortunately, I just wrote up the game background. I'm a writer, not a programmer.
> 
> I do not recall if I kept any of my background material for MAX I (and a bunch for MAX II that was never used). I will look in my old files. Most of what I did was at Interplay, and I may not have made copies when I left Interplay.
> 
> Thanks for your interest in a favorite old project.

After learning that there is no hope to see a bug fixed version of the game I started my own research to find a solution and strangely enough one of the culprits for the memory assertion errors and game crashes was not related to the game itself. The original v1.04 M.A.X. linear executable is bound to Tenberry Software's DOS/4GW v1.97 DOS extender. A DOS extender, among many things, is a memory and file manager and according to the [DOS/4GW v2.01 release notes](https://web.archive.org/web/20180611050205/http://www.tenberry.com/dos4g/watcom/rn4gw.html) there were a couple of defects that could had affected M.A.X. as well.
A proof of that assumption is that after like 13 years, back in 2009, I was finally able, for the first time in history, to play an EPIC custom game against three compupter players for more than 266 turns without unrecoverable crashes or hangs until finally the AI started an endless loop of loading a unit into a depo building and then unloading the same again and again and... sad story.

<img src="/assets/images/utopia_city.jpg" alt="Utopia City" width="800" height="600">

At the time my biggest achievement was to figure out the **potentially** accurate formula for research centers.

<img src="/assets/images/rctc.jpg" alt="Research Center Formula">

But nobody seemed to care much from the community.

Ten years later I accidentally bumped into a presentation of the [Syndicate Wars Port](http://swars.vexillium.org/) which gave me strength and zeal to start this project.
