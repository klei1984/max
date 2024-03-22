---
layout: page
title: Installation Guideline
permalink: /install/
---

## How to play M.A.X. Port

M.A.X. Port requires the original M.A.X. game data files to work. Therefore the first step is to make the original game files available on your computer. Then [Download](download.md) M.A.X. Port or [build](build.md) the runtime using the source code. Finally install and configure M.A.X. Port.

The following article provides a step by step guide on how to install the game on Windows. If you have questions or need help with the installation process, you can visit the [discord](https://discord.gg/TCn8DpeBaY) server to chat, or could open a [discussion forum](https://github.com/klei1984/max/discussions) thread on GitHub.

### Installation of original M.A.X. game data files

#### Installation from an original CD-ROM

Interplay Productions released several versions of M.A.X. in CD-ROM format. Each version has a unique CD-ROM serial number printed onto the CD-ROM front. The various bundled CD-ROM re-releases of M.A.X. are not covered by this list.

<img src="{{ site.baseurl }}/assets/images/print_label.png" alt="M.A.X. CD-ROM Label CD-ICD-082-EU"> 

**Serial CD-ICD-082-EU**

This is the v1.00 version of the game released for the European market. The CD-ROM contains game data files for the English, French, German and Italian languages. The required game data files are found in the MAX folder of the CD-ROM's root folder. E.g. if the CD-ROM drive letter is D on Windows, then the folder path would be `D:\MAX`. The MAX folder has the French, German and Italian localization specific files in dedicated subfolders. E.g. the folder of the German localization would be in the previous example would be `D:\MAX\GERMAN`.

**Serial CD-ICD-082-G**

This is the v1.01 version of the game released for Germany. The CD-ROM contains game data files for the English, French, German and Italian languages. The required game data files are found in the MAX folder of the CD-ROM's root folder.

Compared to the v1.00 release of the game, this version corrects some problems in campaign mission 7, and fixes various localization issues in the French, German and Italian versions.

**Serial CD-ICD-082-0**

This is the v1.02 version of the game released for the North American market. The CD-ROM contains game data files for the English, French, German and Italian languages. The required game data files are found in the MAX folder of the CD-ROM's root folder.

Compared to the v1.01 release of the game, this version updates scenario missions 7.

**Serial Serial CD-ICD-082-SP**

This is the v1.03 version of the game released for the Spanish market. The CD-ROM contains game data files for the English and Spanish languages. The required game data files are found in the MAX folder of the CD-ROM's root folder. The MAX folder has the Spanish localization specific files in dedicated subfolders called Spanish.

Compared to the v1.02 release of the game, this version corrects the description of campaign mission 7, adds two new attract mode demos to the main menu that could trigger when the user is idle for more than a minute, and corrects various mistakes in the in-game help messages of the game. This version removes the French, German and Italian localizations and adds the Spanish localization in their place.

**Serial CD-ICD-082-1**

This is the v1.04 version of the game re-released for the North American market. The CD-ROM contains game data files for the English, French, German and Italian languages. The required game data files are found in the MAX folder of the CD-ROM's root folder.

Compared to the v1.03 release of the game, this version corrects more mistakes in the in-game help messages of the game.

**Windows step by step guide**

The following steps describe how to copy the required M.A.X. game data files from the original CD-ROMs to a user preferred location that will be used later on by M.A.X. Port.

1. Open the Windows Explorer (or File Explorer). Keyboard shortcut is `Windows + E`.
2. Put the M.A.X. CD-ROM into your CD-ROM drive and navigate to it within the File Explorer.
3. Select the MAX folder found on the CD-ROM and copy it. Keyboard shortcut to copy the selected folder is `Ctrl + C`. To copy the folder using the mouse pointer, right click on the selected folder and select the Copy option from the pop up context menu.
4. Navigate to a preferred location on a local drive where you have write permissions that has more than 1 GiB available free space.
5. Paste the previously selected MAX folder to your preferred folder. Keyboard shortcut to paste the copied folder is `Ctrl + V`. To paste the copied folder using the mouse pointer, right click inside the preferred folder and select the Paste option from the pop up context menu.
6. The CD-ROM can be removed from the CD-ROM drive.
7. If you want to use one of the supported non English languages all files found inside a language specific sub folder of the newly copied MAX folder have to be copied from the language specific sub folder into the MAX folder itself overwriting all existing files in the process.

#### Installation from GOG.com catalogue

tbd.

#### Installation from Steam catalogue

tbd.

#### Installation from Epic Games Store catalogue

tbd.

### Installation of M.A.X. Port

Ready to use M.A.X. Port guided installers are available for 32 bit and 64 bit Windows operating systems on the [Download](download.md) page. Portable packages that require no administrative privileges to install are also available at the same location. Finally M.A.X. Port can also be built by the end users using the source code found at the same location, using the following [guide](build.md).

tbd.

### Compatibility with Interactive Demo versions of M.A.X.

There were several demo versions published for M.A.X. Version v1.03a of the demo is marginally compatible with M.A.X. Port. Missing resources like audio files, unit sprites or images will simply not play or render.

tbd.
