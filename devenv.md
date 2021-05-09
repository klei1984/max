---
layout: page
title: Development Environments
permalink: /devenv/
---

## M.A.X. Port Development

Most tools are open source and cross-platform. Platform specific tools and processes are clearly marked as such.

### Integrated Development Environment

In general any IDE could do, but only an Eclipse C/C++ Makefile project is maintained in the project.

Eclipse can be obtained from eclipse.org. Just use the Eclipse Installer from the main download section. At the time of writing this is Eclipse IDE 2021-03. Of course it is also possible to just put together your own bundle from scratch to get rid of all the nice to have stuff that nobody ever uses any ways and just wastes memory and slows down the IDE.

The installer bundles its own JRE, it just works. How to set up proxy settings can be found on the Internet if required. The basic `Eclipse IDE for C/C++ Developers` bundle fits for the project's basic needs. Select that bundle to install.

The bundle comes with the Eclipse Marketplace feature so plugins are easy to install. Start Eclipse and to install a marketplace plugin just drag & drop the Install control into the Eclipse workspace from the marketplace page.

- [ANSI Escape in Console](https://marketplace.eclipse.org/content/ansi-escape-console) to pretty print cmake build status information.
- [CMake Editor](https://marketplace.eclipse.org/content/cmake-editor) to edit CMake configuration files.
- [CppStyle](https://marketplace.eclipse.org/content/cppstyle) for its clang-format integration. The git repository comes with its auto code formatting profile for clang-format.
- [JSON Editor](https://marketplace.eclipse.org/content/json-editor-plugin) The code generator scripts use a JSON configuration file as input. The editor comes with auto code formatting and format parsing; it is very useful.
- [YAML Editor](https://marketplace.eclipse.org/content/yaml-editor) GitHub Actions jobs are configured via YAML files.

And that is all.

The installed Eclipse bundle comes with Egit which is based on JGit which means there is no need for a command line git SCM client. The project git repositories can be cloned or added from the Git Perspective.

After checking out the latest master, or a feature branch, the Eclipse project can be added to the Eclipse workspace via the File menu `Open Projects from File System...` menu item.

### CMake

CMake generates system specific build configuration and makefiles. Follow the OS specific [build instructions](build.md) to install CMake and CMake GUI. The CMake GUI tool is a convenient way to configure packages, various build configurations and such.

### NSIS (Windows specific)

CMake's CPack module can build OS specific installers. The Windows specific CPack configuration depends on [NSIS](https://nsis.sourceforge.io/Main_Page). Packaging is not part of normal builds. There is a dedicated build target in the Eclipse project to create the installer.

### Intel Inspector

Used for profiling and debugging purposes. E.g. to find memory allocation and thread safety issues. Unfortunately SDL tends to let the OS clean up after itself so there are many memory leaks unrelated to actual M.A.X. Port code.

## M.A.X. Research

### Open Watcom

[OW](http://open-watcom.github.io/) is used for research.

### GIMP

GIMP will later become the cornerstone of the graphics asset pipeline. But currently it is used for research mostly.

### Wireshark

Used for network analysis. E.g. it is used to debug the IPX/SPX tunneling over UDP of M.A.X. v1.04 running under DOSBox.

### IDA Freeware 7.0

Used for debugging and research.

### wxHexEditor

Used for research.

### MS-DOS

MS-DOS 6.22 runs well under [VirtualBox](https://www.virtualbox.org/). The MS-DOS development environment requires many legacy tools that are not marketed nor supported any more by their authors or owners. Fortunately ebay.com and archive.org are very good sources for legacy stuff.

### DOSBox & DBGL

[DBGL](https://dbgl.org/) is a feature ritch GUI and manager for DOSBox.

## M.A.X. Port - Home Page Editing

tbd.

## GIMP Plug-in Development

tbd.
