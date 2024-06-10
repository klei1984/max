---
layout: page
title: Installation Guideline
permalink: /install/
---

## How to play M.A.X. Port

M.A.X. Port requires the original M.A.X. game data files to work. Therefore the first step is to make the original game files available on your computer. Then [Download](download.md) M.A.X. Port or [build](build.md) the runtime using the source code. Finally install and configure M.A.X. Port.

The following article provides a step by step guide on how to install the game on various operating systems. If you have questions or need help with the installation process, you can visit the [discord](https://discord.gg/TCn8DpeBaY) server to chat, or could open a [discussion forum](https://github.com/klei1984/max/discussions) thread on GitHub.

### Installation of original M.A.X. game data files

#### Installation from an original CD-ROM

Interplay Productions released several versions of M.A.X. in CD-ROM format. Each version has a unique CD-ROM serial number printed onto the CD-ROM front. The various bundled CD-ROM re-releases of M.A.X. are not covered by this list.

<img src="{{ site.baseurl }}/assets/images/print_label.png" alt="M.A.X. CD-ROM Label CD-ICD-082-EU"> 

**Serial CD-ICD-082-EU**

This is the v1.00 version of the game released for the European market. The CD-ROM contains game data files for the English, French, German and Italian languages. The required game data files are found in the MAX folder of the CD-ROM's root folder. E.g. if the CD-ROM drive letter is D on Windows, then the folder path would be `D:\MAX`. The MAX folder has the French, German and Italian localization specific files in dedicated subfolders. E.g. the folder of the German localization would be in `D:\MAX\GERMAN`.

**Serial CD-ICD-082-G**

This is the v1.01 version of the game released for Germany. The CD-ROM contains game data files for the English, French, German and Italian languages. The required game data files are found in the MAX folder of the CD-ROM's root folder.

Compared to the v1.00 release of the game, this version corrects some problems in campaign mission 7, and fixes various localization issues in the French, German and Italian versions.

**Serial CD-ICD-082-0**

This is the v1.02 version of the game released for the North American market. The CD-ROM contains game data files for the English, French, German and Italian languages. The required game data files are found in the MAX folder of the CD-ROM's root folder.

Compared to the v1.01 release of the game, this version updates scenario missions 7.

**Serial CD-ICD-082-SP**

This is the v1.03 version of the game released for the Spanish market. The CD-ROM contains game data files for the English and Spanish languages. The required game data files are found in the MAX folder of the CD-ROM's root folder. The MAX folder has the Spanish localization specific files in dedicated subfolders called Spanish.

Compared to the v1.02 release of the game, this version corrects the description of campaign mission 7, adds two new attract mode demos to the main menu that could trigger when the user is idle for more than a minute, and corrects various mistakes in the in-game help messages of the game. This version removes the French, German and Italian localizations and adds the Spanish localization in their place.

**Serial CD-ICD-082-1**

This is the v1.04 version of the game re-released for the North American market. The CD-ROM contains game data files for the English, French, German and Italian languages. The required game data files are found in the MAX folder of the CD-ROM's root folder.

Compared to the v1.03 release of the game, this version corrects more mistakes in the in-game help messages of the game.

The following steps describe how to copy the required M.A.X. game data files from the original CD-ROMs to a user preferred location that will be used later on by M.A.X. Port.

**Windows step by step guide**

1. Open the Windows Explorer (or File Explorer). Keyboard shortcut is `Windows + E`.
2. Put the M.A.X. CD-ROM into your CD-ROM drive and navigate to it within the File Explorer.
3. Select the MAX folder found on the CD-ROM and copy it. Keyboard shortcut to copy the selected folder is `Ctrl + C`. To copy the folder using the mouse pointer, right click on the selected MAX folder, e.g. on `D:\MAX`, and select the Copy option from the pop up context menu.
4. Navigate to a preferred location on a local drive where you have write permissions that has more than 1 GiB available free space.
5. Paste the previously selected MAX folder to your preferred folder. Keyboard shortcut to paste the copied folder is `Ctrl + V`. To paste the copied folder using the mouse pointer, right click inside the preferred folder and select the Paste option from the pop up context menu.
6. If you want to use one of the supported non English languages all files found inside a language specific sub folder, e.g. `D:\MAX\GERMAN`, of the newly copied MAX folder have to be copied from the language specific sub folder into the MAX folder itself overwriting all existing files in the process.
7. After all copy operations are completed, the CD-ROM can be removed from the CD-ROM drive.

**Linux step by step guide**

It is assumed that the Linux distribution runs a desktop client and that the user is logged into that system as a sudoer user. It is assumed that the user is familiar with the `mount` system administration command. CD-ROM devices under Linux could have arbitrary names, in this tutorial it is assumed that the CD-ROM has the name `/dev/sr1`. Default mount points for CD-ROMs under Linux could have arbitrary names, in this tutorial it is assumed that the default mount point is called `/media/<logged in user name>`, e.g. `/media/max` for user `max`.

The M.A.X. CD-ROMs contain a basic ISO 9660 file system which means that file names appear in MS-DOS 8.3 format and all characters are in upper case. If the given Linux distribution automatically mounts CD-ROMs on insertion, it could easily happen that the operating system mounts the CD-ROM file system with name translation mapping enabled in which case all folder and file names are converted to be lower case. M.A.X. Port should be able to handle original M.A.X. game data files even if they are in lower case. In case the user wants to have the original ISO 9660 file system representation, the CD-ROM file system needs to be unmounted and remounted with different settings. E.g. with `sudo mount -o map=off,ro /dev/sr1 /media/max`.

1. Open a terminal window. On some Linux distributions the keyboard shortcut is `Ctrl + Alt + T`.
2. Put the M.A.X. CD-ROM into your CD-ROM drive. If it is not automatically showing up at the desktop, try to figure out whether the CD-ROM had been automatically mounted or what could be the CD-ROM drive's device name in the first place.
   E.g. run the `mount` command which might output something like `dev/sr0 on /media/max/MAX type iso9660 (ro,nosuid,nodev,relatime,nojoliet,check=s,map=n,blocksize=2048,uid=1000,gid=1000,dmode=500,fmode=400,iocharset=utf8,uhelper=udisks2)`. In the given case the device is called `dev/sr0` and the CD-ROM file system is mounted to the path `/media/max/MAX` where `max` is the user name and `MAX` is the CD-ROM's name and `map=n` means that all folder and file names are converted to lower case. Another command that may be available on the system is `lsblk` which might output something like `sr0     11:0    1   639M  0 rom  /media/max/MAX` where `/media/max/MAX` is the mounted CD-ROM file system path.
3. If the CD-ROM file system is not automatically mounted, but at least the device name had been identified in the previous step, then mount the CD-ROM file system with the command `sudo mount -o map=off,ro <device name> <mount point>`, e.g. `sudo mount -m -o map=off,ro /dev/sr0 /media/max/MAX`.
4. The files found in the MAX folder of the CD-ROM needs to be copied to a user owned location, e.g. with access mode `0664` permissions or similar, without the sub folders. It is recommended to copy the files under the user's home folder to an empty new folder. E.g. to `~/MAX` which is same as `/home/<user name>/MAX`. First create an empty new folder: `mkdir ~/MAX`, then copy the files with the user's default file system access mode permissions: `cp --no-preserve=all /media/max/MAX/*.* ~/MAX`.
5. you want to use one of the supported non English languages all files found inside a language specific sub folder, e.g. `/media/max/MAX/GERMAN`, of the newly copied `~/MAX` folder have to be copied from the language specific sub folder into the `~/MAX` folder itself overwriting all existing files in the process. E.g. to copy the German files run command `cp --no-preserve=all /media/max/MAX/GERMAN/*.* ~/MAX`. Note that if the CD-ROM mounted the CD-ROM file system with name translation mapping enabled, the folder names to copy may be in lower case in the cp command lines.
6. After all copy operations are completed, the CD-ROM can be removed from the CD-ROM drive.

#### Installation from GOG.com catalogue

GOG supports the English, German, French and Italian versions of the game. GOG does not have the Spanish version. The game data files installed by GOG correspond to the CD-ICD-082-0 retail CD-ROM contents which is the v1.02 version of the game released for the North American market, but in addition the v1.04 retail patch is applied. This means that the improvements from CD-ICD-082-SP and CD-ICD-082-1 are missing from this release.

**Windows step by step guide**

Under Windows operating system it is recommended to use the GOG Galaxy client to simply install the game as any other and select the preferred language during the installation process. The default installation folder under Windows is `c:\Program Files (x86)\GOG Galaxy\Games\MAX`. The game files do not need to be copied anywhere, the given folder path could be used by M.A.X. Port directly.

**Linux step by step guide**

Under Linux the offline Windows installer needs to be downloaded and extracted using a 3rd party tool. At the time of writing this guide the installer is called `setup_m.a.x._1.04_(21516).exe` which may be saved as `setup_m_a_x__1_04__21516_.exe` by the Web browser.

1. Download the offline Windows installer from GOG.com using your preferred Web browser application.
2. Open a terminal window and navigate to the download location of the offline installer.
2. Install the `innoextract` tool if it is not yet available on the system. On Ubuntu and the likes the package can be installed with `sudo apt install innoextract`. On Arch Linux and the likes the package can be installed with `sudo pacman -S innoextract`.
3. To determine which languages are supported by the installer, run `innoextract --list-languages setup_m_a_x__1_04__21516_.exe`.
4. Install the game in the preferred language to the desired folder. E.g. to install the English version into a folder called MAX within the user's home folder, call `innoextract --language en-US --gog --quiet --exclude-temp --output-dir ~/MAX setup_m_a_x__1_04__21516_.exe`.

Note: at the time of writing, innosetup failed to extract any non en-US language specific files with `innoextract v1.9`.

#### Installation from Steam catalogue

Steam supports the English version of the game. Steam does not have the German, French, Italian and Spanish version. The game data files installed by Steam correspond to the CD-ICD-082-EU retail CD-ROM contents which is the v1.00 version of the game released for the European market, but in addition the v1.04 retail patch is applied. This means that none of the improvements from any of the later retail CD-ROM releases are present in this release.

**Windows step by step guide**

The default installation folder under Windows is `c:\Program Files (x86)\Steam\steamapps\common\M.A.X. Mechanized Assault & Exploration\max`. The game files do not need to be copied anywhere, the given folder path could be used by M.A.X. Port directly.

**Linux step by step guide**

The default installation folder under Linux is distribution specific. E.g. the game data files might be located here: `~/.steam/steam/steamapps/common/M.A.X. Mechanized Assault & Exploration/max`. The game files do not need to be copied anywhere, the given folder path could be used by M.A.X. Port directly.

#### Installation from Epic Games Store catalogue

EGS supports the English version of the game. EGS does not have the German, French, Italian and Spanish version. The game data files installed by EGS correspond to the CD-ICD-082-EU retail CD-ROM contents which is the v1.00 version of the game released for the European market, but in addition the v1.04 retail patch is applied. This means that none of the improvements from any of the later retail CD-ROM releases are present in this release.

**Windows step by step guide**

The default installation folder under Windows is `c:\Program Files\Epic Games\MAX\max`. The game files do not need to be copied anywhere, the given folder path could be used by M.A.X. Port directly.

**Linux step by step guide**

Not supported.

### Installation of M.A.X. Port

**Windows step by step guide**

Ready to use M.A.X. Port guided installers are available for 32 bit and 64 bit Windows operating systems on the [Download](download.md) page.


Portable packages that require no administrative privileges to install are also available at the same location. Finally M.A.X. Port can also be built by the end users using the source code found at the same location, using the following [guide](build.md).

tbd.

**Linux step by step guide**

***Ubuntu 64 bit (x86-64, amd64) using Deb package***

The guide will use as example Deb package name `max-port_0.7.0-1_x86-64.deb`. The original M.A.X. game data is assumed to be copied to `~/MAX` in previous steps.

1. Download the corresponding package from the [Download](download.md) page using your preferred Web browser application.
2. Open a terminal window. On some Linux distributions the keyboard shortcut is `Ctrl + Alt + T`.
3. Navigate to the download location of the downloaded Deb package.
4. In case a previous install of the `max-port` package is installed on the system, remove it using command `sudo apt remove -y max-port`.
5. Install the package with command `sudo apt install -y ./max-port_0.7.0-1_x86-64.deb`.
6. The game can be started with the `max-port` command, or with the `M.A.X. Port` desktop icon.
7. On the first run by the given user the game needs to be configured.
tbd.

***Ubuntu 64 bit (x86-64, amd64) using Flatpak package***

The guide will use as example Flatpak package name `max-port_0.7.0-1_x86-64.flatpak`. The original M.A.X. game data is assumed to be copied to `~/MAX` in previous steps.

1. Download the corresponding package from the [Download](download.md) page using your preferred Web browser application.
2. Open a terminal window. On some Linux distributions the keyboard shortcut is `Ctrl + Alt + T`.
3. Navigate to the download location of the downloaded Flatpak package.
tbd.

***Arch Linux 64 bit (x86-64, amd64) using pkg.tar.zst binary package***

The guide will use as example binary package name `max-port_0.7.0-1_x86-64.pkg.tar.zst`. The original M.A.X. game data is assumed to be copied to `~/MAX` in previous steps.

1. Download the corresponding package from the [Download](download.md) page using your preferred Web browser application.
2. Open a terminal window.
3. Navigate to the download location of the downloaded binary package.
tbd.

### Compatibility with Interactive Demo versions of M.A.X.

Interplay published several versions of the M.A.X. demo. Version 1.03a of the demo is **marginally** compatible with M.A.X. Port. Missing resources like audio files, unit sprites or images will simply not play or render.
