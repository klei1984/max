---
layout: page
title: Build Instructions
permalink: /build/
---

<h3 class="no-toc">Table of Contents</h3>

* TOC
{:toc}

## How to build M.A.X. Port

The article documents the build instructions for M.A.X. Port v0.7.x. To actually run the game please read relevant sections of the [Installation Guideline](install.md).

### General Notes

The M.A.X. Port source code is compatible with the 32 bit x86 and 64 bit x86-64 architectures. Only Windows and Linux operating systems are targeted currently, but support for other platforms that are supported by SDL2 could potentially be added. A 64 bit build has generally better performance. E.g. there are more CPU registers that the compiler could use for general runtime optimizations.

The M.A.X. Port source code packages do not contain packages of required dependencies. The dependencies are downloaded once by the build system on demand. The dependencies can be downloaded manually as well and placed into the `<source root>/dependencies` source folder in which case the build system will use the local versions of the dependencies instead of downloading them on demand. The version and download location of depenencies are controlled by `<source root>/cmake/versions.cmake`. The file names of downloaded packages and their expected md5 hash can be found in `<source root>/dependencies/dependencies.md5`.

### Dependencies
- **CMake** Minimum requirement is currently `v3.28`.
- **ENet, Freetype, Miniaudio, SDL2** There is no exact version requirement, dependencies are managed by the build system. There are times when official package repositories are offline which could lead to failed builds. Keep this in mind when build errors occur.

SDL2 checks run time and build time dependencies available on the system during build configuration time and it will output a report within the terminal window that describes what features are found on the given system. E.g. on Linux systems the list will look something like this:

```
-- SDL2 was configured with the following options:

...

-- Options:
--   SDL_DISKAUDIO               (Wanted: ON): ON
--   SDL_DUMMYAUDIO              (Wanted: ON): ON
--   SDL_DUMMYVIDEO              (Wanted: ON): ON
--   SDL_FUSIONSOUND             (Wanted: OFF): OFF
--   SDL_FUSIONSOUND_SHARED      (Wanted: OFF): OFF
--   SDL_OPENGL                  (Wanted: ON): ON
--   SDL_OPENGLES                (Wanted: ON): ON
--   SDL_PIPEWIRE                (Wanted: ON): OFF
--   SDL_PIPEWIRE_SHARED         (Wanted: ON): OFF
--   SDL_PTHREADS                (Wanted: ON): ON
--   SDL_PTHREADS_SEM            (Wanted: ON): ON
--   SDL_PULSEAUDIO              (Wanted: ON): ON
--   SDL_PULSEAUDIO_SHARED       (Wanted: ON): ON
--   SDL_RENDER_D3D              (Wanted: OFF): OFF
--   SDL_SYSTEM_ICONV            (Wanted: ON): ON
--   SDL_VULKAN                  (Wanted: ON): ON
--   SDL_WAYLAND                 (Wanted: ON): ON
--   SDL_WAYLAND_LIBDECOR        (Wanted: ON): ON
--   SDL_WAYLAND_LIBDECOR_SHARED (Wanted: ON): ON
--   SDL_WAYLAND_QT_TOUCH        (Wanted: ON): ON
--   SDL_WAYLAND_SHARED          (Wanted: ON): ON
--   SDL_X11                     (Wanted: ON): ON
--   SDL_X11_SHARED              (Wanted: ON): ON
--   SDL_X11_XCURSOR             (Wanted: ON): ON
--   SDL_X11_XDBE                (Wanted: ON): ON
--   SDL_X11_XFIXES              (Wanted: ON): ON
--   SDL_X11_XINPUT              (Wanted: ON): ON
--   SDL_X11_XRANDR              (Wanted: ON): ON
--   SDL_X11_XSCRNSAVER          (Wanted: ON): ON
--   SDL_X11_XSHAPE              (Wanted: ON): ON
```

These settings determine what display and audio servers or drivers will be supported by the game so you want to make sure that at least the services used by your Linux system are found to be `ON`. For example if your Linux runs a Wayland based display server and SDL2 sets it to off, `SDL_WAYLAND (Wanted: ON): OFF`, then you will not be able to run the game. Each Linux distribution could use different package managers and package names, and the availability of the servers and drivers supported by SDL2 may differ too. It is up to the end user to make sure that their system specific dependencies are made available for SDL2. A simple but bloated way to install as many of these implicit dependencies on Linux is to install the SDL2 developer package via the package manager. E.g. on Ubuntu the 64 bit SDL2 developer package can be installed via a terminal window like `sudo apt install libsdl2-dev:amd64`. Keep in mind that the game does not use the operating system's SDL2 packages themselves as they tend to be outdated and configured in a way that is suboptimal for M.A.X. Port.

### Optional dependencies

- **git** The source code can be obtained by cloning the git repository or by downloading the same as a compressed file from GitHub. If the first option is used the build system can take game version information from the cloned git repository.
- **MiniUPnP** UPnP client library.
- **NSIS** Install system required by Windows platform for packaging.

### Toolchain files

The build system relies on compiler configuration templates that are stored in CMake toolchain files. These files determine the compiler being used (e.g. GCC or CLang), activated compiler diagnostics, and the CPU architecture (e.g. 32 or 64 bit). For example `<source root>/cmake/toolchain-mingw-w64-i686-clang.cmake` is using the Windows MSYS/MinGW build system, sets the compiler to LLVM CLang, the CPU architecture to 32 bit and relies on the default compiler diagnostics profile. For end users it is recommended to use a GCC compiler based toolchain file instead of CLang.

### Build options

`BUILD_SHARED_LIBS`: Can be set to `ON` or `OFF`. Recommendation: on Linux set to `ON` to build with shared libraries. On Windows XP set to `OFF` to build with static libraries.

`CMAKE_BUILD_TYPE`: Can be set to `Debug`, `Release`, `RelWithDebInfo` or `MinSizeRel`. `Debug` builds enable various developer features in-game, but disable most compiler optimizations resulting in reduced performance, but better error diagnosability.

`MAX_BUILD_TESTS`: Can be set to `ON` or `OFF`. Enabled by default. If enabled, then unit tests are built and executed.

`MAX_ENABLE_UPNP`: Can be set to `ON` or `OFF`. Enabled by default. If enabled, then the UPnP client library is included into the build which attempts to open the configured `host_port` on a compliant router in case `trasport` is set to `udp_default`. The setting is only relevant for network play.

### Build on Linux (Ubuntu 22.04, x86 architecture, Debug configuration)

The user is supposed to have sudo access in a terminal window. A fresh install of 64 bit version of Ubuntu 22.04 is assumed.

For 32 bit x86 builds the first step is to enable the i386 architecture within the package manager. It is generally recommended to build the application in 64 bit x86-64 mode for a 64 bit operating system.

```
sudo dpkg --add-architecture i386
```

Now update the package lists.

```
sudo apt-get update
```

Next install all dependencies and required tools. The list includes gcc i386, the CMake build system and other libraries or tools. An up to date CMake version is required while Ubuntu's package manager usually offers an outdated version only so CMake's own package repository is added to the package manager's source list.

```
sudo apt-get install ca-certificates gpg wget
test -f /usr/share/doc/kitware-archive-keyring/copyright || wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
sudo apt-get update
test -f /usr/share/doc/kitware-archive-keyring/copyright || sudo rm /usr/share/keyrings/kitware-archive-keyring.gpg
sudo apt-get install kitware-archive-keyring
sudo apt-get install file build-essential cmake gcc-multilib g++-multilib gettext p7zip-full libsdl2-dev:i386
```

Now grab the M.A.X. Port source code from either a release baseline or from the latest master branch.

If downloaded as a zip file extract it somewhere and enter that folder. E.g. `unzip master.zip && cd master`.
If downloaded as a tar.gz source package the commands would look something like `tar -xf max-port-0.7.0-Source.tar.gz && cd max-port-0.7.0-Source`.

To clone the git repository instead we have to make sure that git is istalled first.

```
sudo apt-get install git
```

Then we could do

```
git clone https://github.com/klei1984/max.git && cd max
```

This way the master branch will be checked out initially which we can test with the `git branch` command.

At this stage we should be standing inside the source code's root folder. Here we should create a build folder where all the output and temporary files will be placed by the build environment. E.g. `mkdir Debug && cd Debug`.

After entering the newly created build folder configure CMake to setup a `Debug` build environment. The install prefix is defined as `/usr/local`. Locally installed software should be placed within `/usr/local` rather than `/usr` unless it is being installed to replace or upgrade software in `/usr`.

```
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-linux-i686.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

And finally we can build and install the application. The installation step may require administrative privileges to install software under `/usr/local`.

```
cmake --build . --parallel
sudo cmake --install .
```

After the installation process is complete the game can be started with the `max-port` command from the terminal window, or with the `M.A.X. Port` desktop icon from the Apps menu. To configure and actually run the game please read relevant sections of the [Installation Guideline](install.md).

To uninstall the installed files use the following command from the build directory

```
sudo xargs rm < install_manifest.txt
```

### Build on Linux (Ubuntu 22.04, x86-64 architecture, RelWithDebInfo configuration)

The user is supposed to have sudo access in a terminal window. A fresh install of 64 bit version of Ubuntu 22.04 is assumed.

It is generally recommended to build the application in 64 bit x86-64 mode for a 64 bit operating system. The `RelWithDebInfo` build configuration enables various compiler optimizations.

Update the package lists.

```
sudo apt-get update
```

Next install all dependencies and required tools. The list includes gcc, the CMake build system and other libraries or tools. An up to date CMake version is required while Ubuntu's package manager usually offers an outdated version only so CMake's own package repository is added to the package manager's source list.

```
sudo apt-get install ca-certificates gpg wget
test -f /usr/share/doc/kitware-archive-keyring/copyright || wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
sudo apt-get update
test -f /usr/share/doc/kitware-archive-keyring/copyright || sudo rm /usr/share/keyrings/kitware-archive-keyring.gpg
sudo apt-get install kitware-archive-keyring
sudo apt-get install file build-essential cmake wget gettext p7zip-full libsdl2-dev
```

Now grab the M.A.X. Port source code from either a release baseline or from the latest master branch.

If downloaded as a zip file extract it somewhere and enter that folder. E.g. `unzip master.zip && cd master`.
If downloaded as a tar.gz source package the commands would look something like `tar -xf max-port-0.7.0-Source.tar.gz && cd max-port-0.7.0-Source`.

To clone the git repository instead we have to make sure that git is istalled first.

```
sudo apt-get install git
```

Then we could do

```
git clone https://github.com/klei1984/max.git && cd max
```

This way the master branch will be checked out initially which we can test with the `git branch` command.

At this stage we should be standing inside the source code's root folder. Here we should create a build folder where all the output and temporary files will be placed by the build environment. E.g. `mkdir RelWithDebInfo && cd RelWithDebInfo`.

After entering the newly created build folder configure CMake to setup a `RelWithDebInfo` build environment. The install prefix is defined as `/usr/local`. Locally installed software should be placed within `/usr/local` rather than `/usr` unless it is being installed to replace or upgrade software in `/usr`.

```
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-linux-x86_64.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

And finally we can build and install the application. The installation step may require administrative privileges to install software under `/usr/local`.

```
cmake --build . --parallel
sudo cmake --install .
```

After the installation process is complete the game can be started with the `max-port` command from the terminal window, or with the `M.A.X. Port` desktop icon from the Apps menu. To configure and actually run the game please read relevant sections of the [Installation Guideline](install.md).

To uninstall the installed files use the following command from the build directory

```
sudo xargs rm < install_manifest.txt
```

### Build on Linux for Windows XP (Ubuntu 22.04, x86 architecture, Release configuration)

The user is supposed to have sudo access in a terminal window. A fresh install of 64 bit version of Ubuntu 22.04 is assumed.

For 32 bit x86 builds the first step is to enable the i386 architecture within the package manager. The `Release` build configuration enables various compiler optimizations.

```
sudo dpkg --add-architecture i386
```

Now update the package lists.

```
sudo apt-get update
```

Next install all dependencies and required tools. The list includes gcc i386, the CMake build system and other libraries or tools. An up to date CMake version is required while Ubuntu's package manager usually offers an outdated version only so CMake's own package repository is added to the package manager's source list.

```
sudo apt-get install ca-certificates gpg wget
test -f /usr/share/doc/kitware-archive-keyring/copyright || wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
sudo apt-get update
test -f /usr/share/doc/kitware-archive-keyring/copyright || sudo rm /usr/share/keyrings/kitware-archive-keyring.gpg
sudo apt-get install kitware-archive-keyring
sudo apt-get install file build-essential binutils-mingw-w64-i686 g++-mingw-w64-i686 gcc-mingw-w64-i686 cmake nsis gettext p7zip-full libsdl2-dev:i386
```

Now grab the M.A.X. Port source code from either a release baseline or from the latest master branch.

If downloaded as a zip file extract it somewhere and enter that folder. E.g. `unzip master.zip && cd master`.
If downloaded as a tar.gz source package the commands would look something like `tar -xf max-port-0.7.0-Source.tar.gz && cd max-port-0.7.0-Source`.

To clone the git repository instead we have to make sure that git is istalled first.

```
sudo apt-get install git
```

Then we could do

```
git clone https://github.com/klei1984/max.git && cd max
```

This way the master branch will be checked out initially which we can test with the `git branch` command.

At this stage we should be standing inside the source code's root folder. Here we should create a build folder where all the output and temporary files will be placed by the build environment. E.g. `mkdir Release && cd Release`.

After entering the newly created build folder configure CMake to setup a `Release` build environment. The install prefix is defined as `c:/Program Files/M.A.X. Port`.

```
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-cross-mingw-w64-i686.cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX="c:/Program Files/M.A.X. Port" ..
```

And finally we can generate our easy to use binary distribution packages.

```
cmake --build . --parallel --target package
```

An executable installer and a 7-Zip compressed archive is created by the build process with names similar to `max-port-x.y.z-win32.exe` and `max-port-x.y.z-win32.7z`. To install, configure and actually run the game using the generated installer application please read relevant sections of the [Installation Guideline](install.md).

### Build on Windows using MSYS2 (Windows 10, x86 architecture, Debug configuration)

The user may need administrative privileges so that installers could adapt system or user specific path environment variables.

First install [msys2](https://www.msys2.org/). Note that it is highly recommended not to change the default installation folder and especially do not use folder names with spaces or other special characters. Preferred install folder is `C:\msys64`.

Next open a mingw32 terminal window `c:\msys64\mingw32.exe` and update the msys2 package list. It might be requested by the package manager to close the terminal window and run the command again from a new one.

```
pacman -Syu
```

Next install all dependencies and required tools. The list includes gcc i386, NSIS and the CMake build system.

```
pacman -S mingw-w64-i686-toolchain mingw-w64-i686-nsis mingw32/mingw-w64-i686-cmake gettext make
```

Now grab the M.A.X. Port source code from either a release baseline or from the latest master branch.

If downloaded as a zip file extract it somewhere and enter that folder. E.g. `unzip master.zip && cd master`. To install the unzip tool do `pacman -S unzip` from the msys2 terminal.

To clone the git repository instead we have to make sure that git is istalled first.

```
pacman -S git
```

Then we could do the following from a folder of our preference

```
git clone https://github.com/klei1984/max.git && cd max
```

This way the master branch will be checked out initially which we can test with the `git branch` command.

At this stage we should be standing inside the source code's root folder. Here we should create a build folder where all the output and temporary files will be placed by the build environment. E.g. `mkdir Debug && cd Debug`.

After entering the newly created build folder configure CMake to setup a `Debug` build environment. The install prefix is defined as `/usr/local`, but we will not install the application using MSYS.

```
cmake -G "MSYS Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw-w64-i686.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

And finally we can generate our easy to use binary distribution packages.

```
cmake --build . --parallel --target package
```

An executable installer and a 7-Zip compressed archive is created by the build process with names similar to `max-port-x.y.z-win32.exe` and `max-port-x.y.z-win32.7z`. To install, configure and actually run the game using the generated installer application please read relevant sections of the [Installation Guideline](install.md).

### Build on Linux (Arch Linux, x64 architecture, None configuration)

The user is supposed to have sudo access in a terminal window (Apps/Utilities/Console). A fresh install of Arch Linux with desktop, audio, etc. support is assumed.

We will build the software using a preconfigured PKGBUILD script that generates binary packages. The script is configured to determine the architecture and build type by the operating system. The PKGBUILD script is configured to download the master branch of the game from GitHub. If the user wants to build a particular release baseline from a downloaded source package, then the script should be adapted according to instructions found in the official Arch package guidelines [\[1\]](#ref1).

First update the package lists.

```
sudo pacman -Syu
```

Next install the build system.

```
sudo pacman -S base-devel
```

Now grab the `PKGBUILD` script from the M.A.X. Port source code. E.g. navigate to `https://github.com/klei1984/max/blob/master/cmake/scripts/linux/PKGBUILD` and left click with the mouse on the download icon. Or using the terminal window from a folder of our preference

```
wget https://raw.githubusercontent.com/klei1984/max/master/cmake/scripts/linux/PKGBUILD
```

To install the wget tool do `sudo pacman -S wget`.

At this stage we should be standing inside a folder with the `PKGBUILD` script ready to build the application using

```
makepkg --noconfirm --syncdeps --holdver
```

And finally we can install the application with something like

```
sudo pacman -U ./max-port-0*.pkg.tar.zst
```

After the installation process is complete the game can be started with the `max-port` command from the terminal window, or with the `M.A.X. Port` desktop icon from the Apps menu. To configure and actually run the game please read relevant sections of the [Installation Guideline](install.md).

To uninstall the installed application do

```
sudo pacman -R max-port
```

## References
<a name="ref1"></a>\[1\] [Arch Package Guidelines](https://wiki.archlinux.org/title/Arch_package_guidelines)<br>
