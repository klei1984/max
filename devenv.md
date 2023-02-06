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

### ImHex

Used for research.

### MS-DOS

MS-DOS 6.22 runs well under [VirtualBox](https://www.virtualbox.org/). The MS-DOS development environment requires many legacy tools that are not marketed nor supported any more by their authors or owners. Fortunately ebay.com and archive.org are very good sources for legacy stuff.

### DOSBox & DBGL

[DBGL](https://dbgl.org/) is a feature rich GUI and manager for DOSBox.  
[DOSBox Debugger](https://www.vogons.org/viewtopic.php?t=7323) v0.74-3 is used for debugging.

Executable used under DOSBox: M.A.X. v1.04 (MD5 hash: 285d81dcd57e62390944c68a6bdcc54a *MAXRUN.EXE)  
Unbound linear executable analyzed in IDA: M.A.X. v1.04 (MD5 hash: fc51699f3c8e884cc6c2f0a7129f67ec *MAXRUN.LE)

#### Memory Mapping

Object Table as output by WDUMP from the Open Watcom compiler:
```
                                 Object Table
==============================================================================
object  1: virtual memory size             = 0012D469H
          relocation base address          = 00010000H
          object flag bits                 = 00002045H
          object page table index          = 00000001H
          # of object page table entries   = 0000012EH
          reserved                         = 00000000H
          flags = READABLE|EXECUTABLE|PRELOAD|BIG

object  2: virtual memory size             = 0000016BH
          relocation base address          = 00140000H
          object flag bits                 = 00000045H
          object page table index          = 0000012FH
          # of object page table entries   = 00000001H
          reserved                         = 00000000H
          flags = READABLE|EXECUTABLE|PRELOAD

object  3: virtual memory size             = 0006F170H
          relocation base address          = 00150000H
          object flag bits                 = 00002043H
          object page table index          = 00000130H
          # of object page table entries   = 00000024H
          reserved                         = 00000000H
          flags = READABLE|WRITABLE|PRELOAD|BIG
```

Segment definitions in IDA:
```
static Segments(void) {
	set_selector(0X1,0);
	set_selector(0X2,0X14000);
	set_selector(0X3,0);
	;
	add_segm_ex(0X10000,0X13E000,0X1,1,3,2,ADDSEG_NOSREG);
	SegRename(0X10000,"cseg01");
	SegClass (0X10000,"CODE");
	SegDefReg(0x10000,"ds",0x3);
	set_segm_type(0X10000,2);
	add_segm_ex(0X140000,0X141000,0X2,0,3,2,ADDSEG_NOSREG);
	SegRename(0X140000,"cseg02");
	SegClass (0X140000,"CODE");
	SegDefReg(0x140000,"ds",0x3);
	set_segm_type(0X140000,2);
	add_segm_ex(0X150000,0X1BF170,0X3,1,3,5,ADDSEG_NOSREG);
	SegRename(0X150000,"dseg03");
	SegClass (0X150000,"STACK");
	SegDefReg(0x150000,"ds",0x3);
	set_segm_type(0X150000,9);
	set_inf_attr(INF_LOW_OFF, 0x0);
	set_inf_attr(INF_HIGH_OFF, 0x1BF170);
}
```

Code segment offset from IDA to DOSBox: 0x00120010.  
Data segment offset from IDA to DOSBox: 0x0010D610.

Examples:

| DOSBox | IDA |
| ------------- | ------------- |
| 1038:00120010 | cseg01:00010000 |
| 1028:0010D610 | dseg03:00150000 |


A few useful debugger commands:  
**ALT-PAUSE** - Break into the debugger.  
**help** - List available commands.  
**F5** - Continue code execution.  
**Cursor Up**, **Cursor Down** - Move code window view up or down.  
**C \[segment\]:\[address\]** - Jump code view to segment:address.  
**Page Up**, **Page Down** - Move data window view up or down.  
**D \[segment\]:\[address\]** - Jump data view to segment:address.  
**BP \[segment\]:\[address\]** - Set breakpoint to segment:address.  

To break into the debugger within DOSBox it might be necessary to remap the key code associated to the debug break event. To enter the keymapper tool press CTRL-F1. Click the Debugger special event and add a new key binding.

## M.A.X. Port - Home Page Editing

tbd.

## GIMP Plug-in Development

tbd.
