---
layout: page
title: Build Instructions
permalink: /build/
---

## How to build M.A.X. Port

### Dependencies
- **Iconv, Freetype, SDL2, Miniaudio and SDL2_net** There is no exact version requirement, dependencies are managed by the build system.
- **CMake** Minimum requirement is currently `3.24`.
- **NSIS** Install system required by Windows platform for packaging.

### Optional dependencies

- **git** The source code can be obtained by cloning the git repository or by downloading the same as a zip file from GitHub. If the first option is used the build system can take game version information from the cloned git repository.

### General Notes

The M.A.X. Port source code is compatible with the 32 bit x86 and 64 bit x86-64 architectures. Only Windows and Linux operating systems are targeted currently, but support for other platforms could potentially be added.

The M.A.X. Port source code packages do not contain packages of required dependencies. The dependencies are downloaded once by the build system on demand. The dependencies can be downloaded manually as well and placed into the `/dependencies` folder in which case the build system will use the local versions of the dependencies instead of downloading them on demand. The version and download location of depenencies are controlled by `/cmake/versions.cmake`.

### Build options

`BUILD_SHARED_LIBS`: Can be set to `ON` or `OFF`. On Linux only `ON` should be used for shared libraries. For the Windows XP builds only `OFF` should be used for static libraries.

`CMAKE_BUILD_TYPE`: Can be set to `Debug`, `Release`, `RelWithDebInfo` or `MinSizeRel`. Only use `Debug` option for the time being.

### Build on Linux (Ubuntu 23.10, x86 architecture, Debug configuration)

The user is supposed to have sudo access in a terminal window. A fresh install of Ubuntu 23.10 is assumed, e.g. the installable cmake package is 3.24 or newer.

For 32 bit x86 builds the first step is to enable the i386 architecture within the package manager so that a 32 bit x86 executable can be built. It is generally recommended to build the application in 64 bit x86-64 mode for a 64 bit operating system.

```
sudo dpkg --add-architecture i386
```

Now update the package lists.

```
sudo apt-get update
```

Next install all dependencies and required tools. The list includes gcc i386, the CMake build system and other libraries or tools.

```
sudo apt-get install build-essential gcc-multilib g++-multilib ninja-build:i386 cmake gettext p7zip-full
```

Now grab the M.A.X. Port source code from either a release baseline or from the latest master branch.

If downloaded as a zip file extract it somewhere and enter that folder. E.g. `unzip master.zip && cd master`.
If downloaded as a tar.gz source package the commands would look something like `tar -xf max-port-0.5.0-Source.tar.gz && cd max-port-0.5.0-Source`.

To clone the git repository instead we have to make sure that git is istalled first.

```
sudo apt-get install git
```

Then we could do

```
git clone https://github.com/klei1984/max.git && cd max
```

This way the master branch will be checked out initially which we can test with the `git branch` command.

At this stage we should be standing inside the source code's root folder. Here we should create a build folder where all the output and temporary files will be placed by the build environment. E.g. `mkdir build && cd build`.

After entering the newly created build folder configure cmake to create a Debug build. Note that as long as the project is under heavy development it is best to keep to Debug builds with no compiler and link time optimizations of any kind.

```
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-linux-i686.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

And finally we can generate our easy to use binary distribution package.

```
cmake --build . --parallel --target package
```

If everything went well then a Linux DEB package is created with a name similar to `max-port-x.y.z-Linux.deb`.

### Build on Linux for Windows XP (Ubuntu 23.10, x86 architecture, Debug configuration)

The user is supposed to have sudo access in a terminal window. A fresh install of Ubuntu 23.10 is assumed, e.g. the installable cmake package is 3.24 or newer.

The first step is to enable the i386 architecture within the package manager so that a 32 bit x86 executable can be built.

```
sudo dpkg --add-architecture i386
```

Now update the package lists.

```
sudo apt-get update
```

Next install all dependencies and required tools. The list includes gcc i386, the CMake build system, the mingw cross compiler and other libraries or tools.

```
sudo apt-get install build-essential binutils-mingw-w64-i686 g++-mingw-w64-i686 gcc-mingw-w64-i686 ninja-build:i386 cmake gettext p7zip-full nsis
```

Now grab the M.A.X. Port source code from either a release baseline or from the latest master branch.

If downloaded as a zip file extract it somewhere and enter that folder. E.g. `unzip master.zip && cd master`.
If downloaded as a tar.gz source package the commands would look something like `tar -xf max-port-0.5.0-Source.tar.gz && cd max-port-0.5.0-Source`.

To clone the git repository instead we have to make sure that git is istalled first.

```
sudo apt-get install git
```

Then we could do

```
git clone https://github.com/klei1984/max.git && cd max
```

This way the master branch will be checked out initially which we can test with the `git branch` command.

At this stage we should be standing inside the source code's root folder. Here we should create a build folder where all the output and temporary files will be placed by the build environment. E.g. `mkdir build && cd build`.

After entering the newly created build folder configure cmake to create a Debug build. Note that as long as the project is under heavy development it is best to keep to Debug builds with no compiler and link time optimizations of any kind.

```
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-cross-mingw-w64-i686.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

And finally we can generate our easy to use binary distribution package.

```
cmake --build . --parallel --target package
```

If everything went well then an executable installer and a 7-Zip compressed archive is created with names similar to `max-port-x.y.z-win32.exe` and `max-port-x.y.z-win32.7z`. Either run the installer and follow the on screen instructions or unpack the archive. M.A.X. Port is an update for the original M.A.X. game which means that the update needs to be installed or copied into the root folder of an existing M.A.X. installation folder where the `maxrun.exe` game executable is found. Some examples: `c:\Program Files (x86)\GOG Galaxy\Games\MAX`, `c:\Program Files (x86)\Steam\steamapps\common\M.A.X. Mechanized Assault & Exploration\max`, `C:\INTRPLAY\MAX`.

### Build on Windows using MSYS2 (x86 architecture, Debug configuration)

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

At this stage we should be standing inside the source code's root folder. Here we should create a build folder where all the output and temporary files will be placed by the build environment. E.g. `mkdir build && cd build`.

After entering the newly created build folder configure cmake to create a Debug build. Note that as long as the project is under heavy development it is best to keep to Debug builds with no compiler and link time optimizations of any kind.

```
cmake -G "MSYS Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw-w64-i686.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

And finally we can generate our easy to use binary distribution packages.

```
cmake --build . --parallel --target package
```

If everything went well then an executable installer and a 7-Zip compressed archive is created with names similar to `max-port-x.y.z-win32.exe` and `max-port-x.y.z-win32.7z`. Either run the installer and follow the on screen instructions or unpack the archive. M.A.X. Port is an update for the original M.A.X. game which means that the update needs to be installed or copied into the root folder of an existing M.A.X. installation folder where the `maxrun.exe` game executable is found. Some examples: `c:\Program Files (x86)\GOG Galaxy\Games\MAX`, `c:\Program Files (x86)\Steam\steamapps\common\M.A.X. Mechanized Assault & Exploration\max`, `C:\INTRPLAY\MAX`.

## References
