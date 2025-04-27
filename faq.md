---
layout: page
title: Frequently Asked Questions
permalink: /faq/
toc: true
---

<h3 class="no-toc">Preface</h3>

The article mainly focuses on M.A.X. Port related topics.

<h3 class="no-toc">Table of Contents</h3>

* TOC
{:toc}

<br>
<br>
<h3 class="no-toc">Q&A List</h3>

#### **Q:** Is M.A.X. Port stable?
**A:** It depends. Based on user feedback the game performs better in many ways than the MS-DOS version. More than 60% of the original game [defects](defects.md) are already fixed, but there are many more to iron out and now that users can play much longer games then ever before they tend to attribute never before seen issues to M.A.X. Port even though there is a fair chance that they experience original defects for the very first times in history. There are several issues that cannot be resolved due to limitations of the original save file format and there are also very complex architectural or fundamental issues with the grand game design or game logic itself. You can report [issues](https://github.com/klei1984/max/issues) on GitHub or could visit the [discord](https://discord.gg/TCn8DpeBaY) server to chat, or could open a [discussion forum](https://github.com/klei1984/max/discussions) thread on GitHub.
<br>
<br>

#### **Q:** When will M.A.X. Port be released?
**A:** While the game did not reach all targeted v1.0.0 goals and will not reach them for years to come, it is already available for download from the [downloads](download.md) page or from the GitHub [releases](https://github.com/klei1984/max/releases) page. Please keep in mind that formal releases are made only when considerable improvements are achieved. So if you want to see the latest improvements you need to have a GitHub account to download CI/CD builds or you need to [build the game](build.md) yourself from source code.
<br>
<br>

#### **Q:** Can I help?
**A:** If you find a reproducible defect, please report it. Reproducible means that even if you quit the game and restart it the same issue happens often enough. Make sure to consult the [defects list](defects.md) first before reporting an issue as there is a good chance somebody reported the same already. Attaching your saved game files to issue reports, e.g. SAVE10.DTA and SAVE10.BAK, are highly appreciated as it helps root cause analysis a lot. You can report [issues](https://github.com/klei1984/max/issues) on GitHub or could visit the [discord](https://discord.gg/TCn8DpeBaY) server for help.<br>
There are cases where the defect is already found on the defects list, but the root cause of the defect is not yet known. For example, defect 12 from the list has been reported many times, but we still do not know how to induce or reproduce the issue. In such cases please report any new information and share your affected, or to be affected saved game files with us. These are most appreciated.<br>
There are many M.A.X. 1 and M.A.X. 2 game releases, merchandise and marketing materials out there. We are always searching for such artefacts. If you treasure any M.A.X. related relics, feel encouraged to show these off at our [discord](https://discord.gg/TCn8DpeBaY) server.
<br>
<br>

#### **Q:** Are there many missing or planned features?
**A:** A high level roadmap can be found [here](roadmap.md). The actual work package list is much bigger, but the rest of the packages are not yet committed to.
<br>
<br>

#### **Q:** My keyboard arrow keys do not work in-game.
**A:** For the time being M.A.X. Port simulates an MS-DOS US101 keyboard and receives USB scan codes from the SDL2 OS abstraction layer backend. To have arrow keys on laptops and similar reduced key count keyboards the Num Lock state may need to be toggled.
<br>
<br>

#### **Q:** Using wide screen, or higher window resolutions or non square shaped maps I cannot scroll the tactical map on the landing site selection screen so I cannot use part of the map as a landing spot.
**A:** The landing site selection screen does not support mouse scrolling currently. The arrow keys should be used instead to move around on the tactical map.
<br>
<br>

#### **Q:** I experience mouse lag and stutter in menus. In cases light bulbs appear above units and I cannot move them around.
**A:** M.A.X. uses the GNW engine that does not have a conventional render loop. The game does not use vertical synchronization and could attempt to flip video card back buffers to redraw part of the screen at variable frame rates that try to stay below 30 FPS. Using frame rate limiter applications will cause hick ups, stutters and delays or could completely rob all CPU cycle time from path finding and computer AI. Please disable frame rate limiters and similar tools while you play with M.A.X. Port.
<br>
<br>

#### **Q:** If I installed the game using an original M.A.X. CD-ROM that is not patched to v1.04, do I have to install game patches before I install M.A.X. Port?
**A:** No, that is not required. The game v1.01-04 game patches only updated the game executable and the readme files. Just install M.A.X. Port and it will be comparable to v1.04 feature set of the original. More details on original CD-ROM differences can be read in the [installation guide](install.md).
<br>
<br>

#### **Q:** I read about a built in mission editor and other cool features, but I cannot find them.
**A:** There are distinct **release** and **debug** builds. Debug or developer builds contain additional features that can be used to diagnose issues easier or to create new scenarios or missions. These debug features are error prone, they are not meant to be end user proof. Additionally these features can be abused, they also make it possible to cheat and use exploits in ways that the game does not know about which is not fair so normal users are encouraged to use release builds. If you want to use debug builds you need to create a free GitHub account to download CI/CD builds or you need to [build the game](build.md) yourself from source code. You can always visit the [discord](https://discord.gg/TCn8DpeBaY) server for help where the debug features are also described in more details.
<br>
<br>

### **Q:** The settings.ini file contains many settings but I do not know what they mean.
**A:** All settings that are meant to be configured by users are documented in the [installation guide](install.md) within the **Configuration of M.A.X. Port** section. Note that there are many settings in the ini configuration file that are handled and dynamically overwritten by the game and are not meant to be edited by end users. It is important that the settings.ini file uses `crlf` line endings so on non Windows operating systems make sure to use a text editor that can handle Windows line endings properly.
<br>
<br>
