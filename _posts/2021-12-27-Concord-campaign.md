---
layout: post
title:  "First mission of an alternative Concord campaign"
date:   2021-12-27
videoId: IY7l4HM8A1k
categories:
excerpt_separator: <!--more-->
---
Ever wondered about what would have happened if you, as a M.A.X. Commander, would have taken side with the Concord and their faithful servants within the Circuit? Is it in humanity’s best interests to abandon the contract that the Dealer in Worlds offered to them? Is treachery and deceit the only options to reach one’s goals? Oh right… We are humans.

With the [save file format]({{ site.baseurl }}{% link save.md %}) fully documented it will eventually become possible to develop new missions for M.A.X. that could tell us the story of the Clans that did not join the Secret Council of Freedom. Crash those impudent rebels and bask in the glory of our Concord Masters!

Can you find Waldo?
<!--more-->
<br><br>
{% include yt_player.html id=page.videoId %}
<br>
  
There are severe limitations in the way the original game handles victory and loss conditions, campaign games, stand alone missions and save games in general.

Depending on game type the executable either hard codes or overrides certain save file content. Victory and loss conditions are not even saved into the save files, they are hard coded into the executable for most missions based on their mission index.

With a bit of tinkering and externalization we could add complex victory and loss conditions, filtered unit building capabilities, limitations to certain upgrades, conditional upgrades, timed events, optional goals with rewards that transition to future missions, conditional bonus missions that only get unlocked if an optional victory condition is met, diversified campaigns with multiple endings, side quests and more. An evil genius might even turn to pseudo randomization of gameplay and map elements to create an ever changing, but balanced storyline that “never” plays out in the exact same way unless desired so…

World files, or simply maps, currently store a complete tile set, pathfinding maps and similar. With a bit of restructuring part of the map data could be moved to the save files allowing an endless number of worlds to be generated from a preset number of tile sets.

So much potential… and so little time to do any of it xD
