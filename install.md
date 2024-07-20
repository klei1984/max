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

<br>
*Microsoft Defender SmartScreen [\[1\]](#ref1) may block the application and its installer from running as the executables are not signed by a Microsoft-approved certifate authority. Microsoft's tool may determine whether a downloaded application or application installer is potentially malicious by:*
- *Checking downloaded files against a list of reported malicious software sites and programs known to be unsafe. If it finds a match, Microsoft Defender SmartScreen shows a warning to let the user know that the site might be malicious.*
- *Checking downloaded files against a list of files that are well known and downloaded frequently. If the file isn't on that list, Microsoft Defender SmartScreen shows a warning, advising caution.*
<br><br>


The guide assumes a Windows 10 operating system and will use as example NSIS installer name `max-port-0.7.0-Windows_x86_64.exe`. The original M.A.X. game data is assumed to be installed from the GOG.com catalogue to `c:\Program Files (x86)\GOG Galaxy\Games\MAX`.

1. Download the corresponding installer from the [Download](download.md) page using your preferred Web browser application.
2. The installer is not signed by a certificate authority. In case the Microsoft Defender SmartScreen tool opens a new window with title `Windows protected your PC`, left click on `More info` and then left click on `Run anyway` on that window. Alternatively right click on the installer's executable and on the General tab check the `Unblock` checkbox in the `Security` section.
3. As the installer is not signed by a certificate authority the Publisher of the installer is identified by Microsoft as an Unknown source. In case the Microsoft Defender SmartScreen tool opens a new window with title `Do you want to allow this app from an unknown publisher to make changes to your device?`, left click on the `Yes` button to enable the installer to run.
4. Follow the on screen guide of the installer tool. The first few screens are trivial. On the Welcome screen, left click on `Next` button. On the License Agreement screen read the agreement and left click on `I Agree` button in case the license terms are acceptable for you otherwise exit the installer by left clicking on the `Cancel` button.
5. On the Choose Install Location screen configure where to install the application. This example will use the default location of the 64 bit version which is `C:\Program Files\M.A.X. Port`. Note that the install location of the application is independent from the location of the original M.A.X. game data. They can be the same locations, but could be different too. When ready, left click on `Next` button.
6. On the Choose Start Menu Folder screen the location of shortcuts in the start menu can be selected or the feature can be disabled completely. In case the feature gets disabled a desktop icon is not created either. When ready, left click on `Next` button.
7. On the Configure Game Settings screen select the location of the original M.A.X. game data. In this example the location is `c:\Program Files (x86)\GOG Galaxy\Games\MAX`. This folder holds original M.A.X. files like `MAX.RES` or `MAXINT.MVE`. The `Install` button is disabled as long as a valid M.A.X. game data folder is not configured. To proceed with the installation left click on `Install` button. There is one more configuration setting on this screen. If the checkbox is checked the game will store user data like saved game files, screenshots, log files and the `settings.ini` configuration file in the install location of the application which is `C:\Program Files\M.A.X. Port` in this example.<br><br>*It is recommended to leave the checkbox unchecked in which case user data is saved into the user specific roaming application data folder. Note that if there are existing saved game files in the original M.A.X. game data folder, then they are copied to the user specific roaming application data folder. Note that game settings are not transferred from the original M.A.X. game data folder. E.g. the campaign game progress is not taken over. Note that under Program Files and similar Windows folders the user may not have administrative privileges to write or modify files after installation without running M.A.X. Port with administrative privileges which is not recommended so in case the game is installed under Program Files or similar make sure to remove the check mark from the checkbox to save user data to the user specific application data folder. To find the user specific roaming application data folder location open the Windows Run dialog, keyboard shortcut is `Windows + R`, type in `"%appdata%"` and left click on `OK` button. An example location on Windows 10 would be `C:\Users\<user name>\AppData\Roaming\M.A.X. Port` where \<user name\> is the logged in operating system user's account name.*<br><br>
8. After the installation process is complete left click on `Finish` button which closes the installer application.
9. To Uninstall M.A.X. Port use the standard Windows procedure. E.g. on Windows 10 navigate to `Apps & Features` dialog, search for a list entry called `M.A.X. Port`, select the entry and then left click on `Uninstall` button. This will open the uninstaller application of M.A.X. Port. There you need to follow the on screen guide of the uninstaller application.

**Linux step by step guide**

***Ubuntu 64 bit (x86-64, amd64) using Deb package***

The guide will use as example Deb package name `max-port_0.7.0-1_x86-64.deb`. The original M.A.X. game data is assumed to be extracted from the GOG.com offline Windows installer to `~/MAX` in previous steps. The package is compliant to v0.8 of the XDG Base Directory Specification [\[2\]](#ref2).

1. Download the corresponding package from the [Download](download.md) page using your preferred Web browser application.
2. Open a terminal window. On some Linux distributions the keyboard shortcut is `Ctrl + Alt + T`.
3. Navigate to the download location of the downloaded Deb package.
4. In case a previous version of the `max-port` package is installed on the system, remove it using command `sudo apt remove -y max-port`.
5. Install the package with command `sudo apt install -y ./max-port_0.7.0-1_x86-64.deb`.
6. After the installation process is complete the game can be started with the `max-port` command, or with the `M.A.X. Port` desktop icon.
7. On the first run by the given user the game needs to be configured unless appropriate resources are found by the configuration tool. The configuration tool uses a terminal window and a text interface dialog. Use the `Arrow` keys to navigate in the menus and press the `Enter` key to proceed to the next configuration setting or the `Escape (ESC)` key to exit from the configuration tool.
8. The first configuration dialog window prompts the user to select the language of the configuration interface. Note that this is not setting the game language itself or changing the active keyboard locale. Press `Enter` to proceed to the next dialog window after a selection is made.
9. The next configuration dialog window asks whether to use "portable mode". In the case of portable mode, the game stores user data like saved game files, screenshots, log files and the `settings.ini` configuration file in the same location where the configuration script is located. In non portable mode the game stores user data at `$XDG_DATA_HOME/max-port`. Unless specified explicitly by the operating system or the user, the `$XDG_DATA_HOME` environment variable will be set to `$HOME/.local/share` so user data would be saved to `$HOME/.local/share/max-port` where `$HOME` is the currently logged in operating system user's home folder. It is strongly recommended to disable portable mode when the user installs the game via a package manager so select the `No` option and press `Enter` to proceed. Rationale: it makes no sense to install the game via a package manager following the XDG Base Directory Specification just to violate the same by instructing the game to put user data to a location that is designated read-only.
10. The next configuration dialog window asks for the location of the original M.A.X. game data files. The given guide uses the example location `~/MAX`, but that notation will not be recognized by the configuration tool so the full path needs to be typed in like `/home/<user name>/MAX`. Note that if there are existing saved game files in the original M.A.X. game data folder, then they are copied to the previously configured user data location. Press `Enter` to conclude the configuration and to start the game. Note that if the provided path is not found or it does not contain mandatory original M.A.X. assets, the configuration tool will not leave the given configuration dialog window.
11. To uninstall M.A.X. Port, follow the instructions from step 4. Note that the operating system's package manager will not remove user data. E.g. files stored in `$XDG_DATA_HOME/max-port`, in the case of a non portable mode configuration, will stay intact and should be removed manually by the user.

***Ubuntu 64 bit (x86-64, amd64) using Flatpak package***

The guide will use as example Flatpak package name `max-port_0.7.0-1_x86-64.flatpak`. The original M.A.X. game data is assumed to be copied to `~/MAX` in previous steps. Make sure that the Flatpak tool is available on the system. There are guidelines on the Internet on how to install the tool.

1. Download the corresponding package from the [Download](download.md) page using your preferred Web browser application.
2. Open a terminal window. On some Linux distributions the keyboard shortcut is `Ctrl + Alt + T`.
3. Navigate to the download location of the downloaded Flatpak package.
4. Install the package with command `flatpak install max-port_0.7.0-1_x86-64.flatpak`. This will also install all required platform dependencies.
5. After the installation process is complete the game can be started with the `flatpak run io.github.max-port` command, or with the `M.A.X. Port` desktop icon.
6. On the first run the game needs to be configured unless appropriate resources are found by the configuration tool. The configuration tool uses a terminal window and a text interface dialog. Use the `Arrow` keys to navigate in the menus and press the `Enter` key to proceed to the next configuration setting or the `Escape (ESC)` key to exit from the configuration tool.
7. The first configuration dialog window prompts the user to select the language of the configuration interface. Note that this is not setting the game language itself or changing the active keyboard locale. Press `Enter` to proceed to the next dialog window after a selection is made.
8. The next configuration dialog window asks for the location of the original M.A.X. game data files that is expected to be at a well defined location within the environment that Flatpak grants access to for the game. E.g. the tool will show a viable path similar to `home/<user name>/.var/app/io.github.max-port/data/max-port/MAX` and the user is expected to copy the original M.A.X. game data files there. E.g. create the missing destination folders with command `mkdir -p ~/.var/app/io.github.max-port/data/max-port/MAX` and then copy the original M.A.X. game data with command `cp -r ~/MAX ~/.var/app/io.github.max-port/data/max-port/`. Press `Enter` to conclude the configuration and to start the game. Note that if the provided path is not found or it does not contain mandatory original M.A.X. assets, the configuration tool will not leave the given configuration dialog window.
9. To uninstall M.A.X. Port, run command `flatpak remove io.github.max-port`. Note that Flatpak will not remove user data. The `io.github.max-port` folder inside `/home/<user data>/.var/app/` should be removed manually by the user.

***Arch Linux 64 bit (x86-64, amd64) using pkg.tar.zst binary package***

The guide will use as example binary package name `max-port_0.7.0-1_x86-64.pkg.tar.zst`. The original M.A.X. game data is assumed to be copied to `~/MAX` in previous steps.

1. Download the corresponding package from the [Download](download.md) page using your preferred Web browser application.
2. Open a terminal window.
3. Navigate to the download location of the downloaded binary package.
4. In case a previous version of the `max-port` package is installed on the system, remove it using command `sudo pacman -R max-port`.
5. Install the package with command `sudo pacman -U ./max-port_0.7.0-1_x86-64.pkg.tar.zst`.
6. After the installation process is complete the game can be started with the `max-port` command, or with the `M.A.X. Port` desktop icon under the Games category.
7. On the first run by the given user the game needs to be configured unless appropriate resources are found by the configuration tool. The configuration tool uses a terminal window and a text interface dialog. Use the `Arrow` keys to navigate in the menus and press the `Enter` key to proceed to the next configuration setting or the `Escape (ESC)` key to exit from the configuration tool. On certain systems the desktop icon may not start the configuration tool in that case use the `max-port` command from the console window and configure the game from there. After the configuration is finished, the desktop icon should work.
8. The first configuration dialog window prompts the user to select the language of the configuration interface. Note that this is not setting the game language itself or changing the active keyboard locale. Press `Enter` to proceed to the next dialog window after a selection is made.
9. The next configuration dialog window asks whether to use "portable mode". In the case of portable mode, the game stores user data like saved game files, screenshots, log files and the `settings.ini` configuration file in the same location where the configuration script is located. In non portable mode the game stores user data at `$XDG_DATA_HOME/max-port`. Unless specified explicitly by the operating system or the user, the `$XDG_DATA_HOME` environment variable will be set to `$HOME/.local/share` so user data would be saved to `$HOME/.local/share/max-port` where `$HOME` is the currently logged in operating system user's home folder. It is strongly recommended to disable portable mode when the user installs the game via a package manager so select the `No` option and press `Enter` to proceed. Rationale: it makes no sense to install the game via a package manager following the XDG Base Directory Specification just to violate the same by instructing the game to put user data to a location that is designated read-only.
10. The next configuration dialog window asks for the location of the original M.A.X. game data files. The given guide uses the example location `~/MAX`, but that notation will not be recognized by the configuration tool so the full path needs to be typed in like `/home/<user name>/MAX`. Note that if there are existing saved game files in the original M.A.X. game data folder, then they are copied to the previously configured user data location. Press `Enter` to conclude the configuration and to start the game. Note that if the provided path is not found or it does not contain mandatory original M.A.X. assets, the configuration tool will not leave the given configuration dialog window.
11. To uninstall M.A.X. Port, follow the instructions from step 4. Note that the operating system's package manager will not remove user data. E.g. files stored in `$XDG_DATA_HOME/max-port`, in the case of a non portable mode configuration, will stay intact and should be removed manually by the user.

### Configuration of M.A.X. Port

Various game settings can only be configured via the `settings.ini` configuration file. The following section documents the ini parameters that are relevant for end users. Note that there are many parameters that should not be tampered with unless the user wants to break the game.

Location of the `settings.ini` configuration file depends on the operating system and the mode of installation. In general the file can be found where "user data" are located based on the above guidelines. E.g. on Windows it may be located at `C:\Users\<user name>\AppData\Roaming\M.A.X. Port/settings.ini` or on Linux it may be found at `$XDG_DATA_HOME/max-port/settings.ini` or at `$HOME/.var/app/io.github.max-port/data/max-port/settings.ini`.

It is important that the ini file uses Windows line delimiters (`\r\n` or `CR LF` ) and UTF-8 encoding, thus it must be edited by a text editor that properly recognises such text file properties.

***[SETUP] section***

**language** Set the game's language. The game data folder contains `lang_<language code>.ini` parameter files. E.g. to select `lang_english.ini` set the value of this ini parameter to `english`. It is very important to use the language specific original M.A.X. game data files. This means that to set `language=spanish`, the user must have the Spanish version of the original game (CD-ROM serial number CD-ICD-082-SP).

Warning: if the configured language and the language of the MAX.RES file does not match, then initial Clan Upgrades will not work and other game anomalies could occur too.<br>
Warning: the in-game help system, game tips, planet and mission descriptions do not encode text in UTF-8 format and will not render correctly for non US ASCII characters. Currently it is up to the end users to convert all the original resource files to UTF-8 encoded variants.<br>
Warning: certain game sub systems do not fully support UTF-8 encoded text rendering and text input does not support any non US ASCII characters.

Recommended (default) value: `english`.

**game_data** Full normalized path to original M.A.X. game data. The files at the configured location are handled as read-only. Normally the ini parameter is configured once during installation. If the parameter is incorrectly set the game will not work.

***[GRAPHICS_SETTINGS] section***

**display_index** Index of monitor screen. The index starts from 0.

**screen_mode** Sets the display mode of the game.
- 0 - Windowed
- 1 - Full Screen
- 2 - Borderless Full Screen (default)

**scale_quality**
- 0 - Nearest neighbor. Crisp, looks good on CRT monitors.
- 1 - Linear interpolation. Smoother graphics, aliasing artifacts are not that visible (default).
- 2 - Best. Smooth graphics, no aliasing artifacts.

Note: SDL2 may override the configured parameter value.

**window_width & window_height**

| Width | Height | Aspect ratio | Tactical map size | Notes |
|-------|--------|---------|---------|---------|
| 640 | 480 | 4:3 (1.33:1) | 7 x 7 tiles | Near pixel perfect original M.A.X. screen resolution. Text is rendered using original expected font size. |
| 853 | 480 | 16:9 (1.78:1)| 10 x 7 tiles | Near pixel perfect original M.A.X. screen resolution in widescreen mode. Text is rendered using original expected font size. |
| 768 | 480 | 16:10 (1.6:1) | 9 x 7 tiles | Near pixel perfect original M.A.X. screen resolution in widescreen mode. Text is rendered using original expected font size. |
| 1440 | 1080 | 4:3 (1.33:1) | 16 x 15 tiles | Full HD standard mode. |
| 1920 | 1080 | 16:9 (1.78:1) | 23 x 15 tiles | Full HD widescreen mode. |

Note: if automatic aspect ratio correction is enabled, then one of the configured dimensions may be overridden by the game. E.g. on a monitor with 16:9 aspect ratio the configuration of 640 x 480 (AR 4:3) resolution will be overridden to match the 16:9 aspect ratio. The auto aspect ratio correction feature can be disabled.<br>
Note: if the display mode is windowed, then the application window size is set to the configured value. If it is full screen, then the monitor's resolution is attempted to be changed to the configured value. In borderless mode the game's internal resolution is set to the configured value and SDL2 scales the internal resolution to match the monitor's configured resolution using a scaling method requested  by the scale quality setting.
Warning: certain uncommon resolutions may crash the game.

Recommended (default) value: `640 x 480`.

**disable_ar_correction**
- 0 - Auto aspect ratio correction is enabled (default).
- 1 - Auto aspect ratio correction is disabled.

**dialog_center_mode**
- 0 - Center in-game popup windows to middle of tactical map.
- 1 - Center in-game popup windows to middle of game screen.

***[NETWORK_SETTINGS] section***

M.A.X. Port implements an input-synchronous, lockstep peer-to-peer networking model. One player takes the application protocol layer role of a game host while up to three other players become clients. The Host as well as the Clients need to set the same `host_address` and `host_port` parameter value. It is up to the users to figure out how their networking hardware and operating system works, but the game supports UPNP NAT to aid the users.

Warning: The implemented networking model is not robust against hugh network latencies.<br>
Warning: Network play is not thoroughly tested.

**transport** Transport protocol layer implementation of the networking module. Only supported value is `udp_default`.

**host_address** IP address of the user that is hosting a network game. The default value is a dummy value, it is set to `127.0.0.1` which is the localhost address that clients will not be able to connect to. The users need to set this to the public IP address of the host computer.

**host_port** Network port number of the host. If the transport protocol layer uses UDP protocol, then this is a UDP port. The default value is 31554 (UDP port).

### Compatibility with Interactive Demo versions of M.A.X.

Interplay published several versions of the M.A.X. demo. Version 1.03a of the demo is **marginally** compatible with M.A.X. Port. Missing resources like audio files, unit sprites or images will simply not play or render.

## References
<a name="ref1"></a>\[1\] [Microsoft Defender SmartScreen](https://learn.microsoft.com/en-us/windows/security/operating-system-security/virus-and-threat-protection/microsoft-defender-smartscreen/)<br>
<a name="ref2"></a>\[2\] [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/basedir-spec-0.8.html)<br>
