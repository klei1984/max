---
layout: page
title: Build Instructions
permalink: /build/
---

## How to build M.A.X. Port

### Dependencies
- **SDL, SDL_mixer and SDL_net** There is no exact version requirement or it was not determined yet. Latest officially released versions are preferred over latest in development revisions. Avoid SDL `2.0.6` which is known to cause segmentation faults when playing sounds.
- **Python** The code generator scripts are developed using Python `3.7.x`, but anything should work above `3.6`.
- **CMake** Minimum requirement is currently `3.10`.

### Optional dependencies

- **git** The source code can be obtained by cloning the git repository or by downloading the same as a zip file from GitHub. If the first option is used the build system can take version information from the cloned git repository.

### General Notes

The M.A.X. Port source code is only *compatible* with the 32 bit x86 architecture. 64 bit builds cannot be created and in general x86_64 bit compilers cannot be used. The code generator scripts heavily rely on GCC extensions so compilers that are not GCC compatible will not work either. Only Windows and Linux operating systems are targeted currently, but potentially an x86 architecture based macOS build would be possible to do if self modifying code is supported by the platform. It is assumed that the OS is built for a 64 bit platform.

### Build options

### Build on Linux (Ubuntu 20.04 LTS)

The user is supposed to have sudo access in a terminal window. A fresh install of Ubuntu 20.04 LTS is assumed, e.g. Python3 is supposed to be pre-installed.

The first step is to enable the i386 architecture within the package manager so that a 32 bit x86 executable can be built. The stock x86_64 build framework will simply fail to work.

```
sudo dpkg --add-architecture i386
```

Now update the package lists.

```
sudo apt-get update
```

Next install all dependencies and required tools. The list includes gcc i386, SDL packages and the CMake build system.

```
sudo apt-get install cmake build-essential gcc-multilib g++-multilib libsdl2-dev:i386 libsdl2-mixer-dev:i386 libsdl2-net-dev:i386
```

Now grab the M.A.X. Port source code from either a release baseline or from the latest master branch.

If downloaded as a zip file extract it somewhere and enter that folder. E.g. `unzip master.zip && cd master`.

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

After entering the newly created build folder configure cmake to create a Debug build. Note that as long as the project is under heavy development and is full of assembler tricks it is best to keep to Debug builds with no compiler and link time optimizations of any kind.

```
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

and finally we can build our game executable

```
cmake --build .
```

If everything went well then the built executable will be found right inside the build folder called `maxrun`.

The new executable should be placed into the location where the original maxrun.exe file is found within the game's install folder.

### Build on Windows using MSYS2

The user may need administrative privileges so that installers could adapt system or user specific path environment variables.

First install [msys2](https://www.msys2.org/). Note that it is highly recommended not to change the default installation folder and especially do not use folder names with spaces or other special characters. Preferred install folder is `C:\msys64`.

Next open a mingw32 terminal window `c:\msys64\mingw32.exe` and update the msys2 package list. It might be requested by the package manager to close the terminal window and run the command again from a new one.

```
pacman -Syu
```

Next install all dependencies and required tools. The list includes gcc i386, SDL packages and the CMake build system.

```
pacman -S mingw-w64-i686-toolchain mingw32/mingw-w64-i686-SDL2 mingw32/mingw-w64-i686-SDL2_mixer mingw32/mingw-w64-i686-SDL2_net mingw32/mingw-w64-i686-cmake make
```

Python 3.7 or later is required for some build scripts. It can be installed from [here](https://www.python.org/downloads/) or it could be installed from within msys2. If a standalone installer is used, make sure that the python executable is found on the system path. The command to install python via msys2 is as follows `pacman -S mingw-w64-i686-python`.

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

After entering the newly created build folder configure cmake to create a Debug build. Note that as long as the project is under heavy development and is full of assembler tricks it is best to keep to Debug builds with no compiler and link time optimizations of any kind.

```
cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="C:/Program Files (x86)/max" -DSDL2_DIR=/mingw32/lib/cmake/SDL2 ..
```

and finally we can build our game executable

```
cmake --build .
```

If everything went well then the built executable will be found right inside the build folder called `maxrun.exe`.

The new executable should be placed into the location where the original maxrun.exe file is found within the game's install folder replacing the old one. It is recommended to create a backup copy of the old executable.

## References
