---
layout: post
title:  "Status report"
date:   2022-12-22
categories:
excerpt_separator: <!--more-->
---
<!--more-->
Recently I finished work on the last remaining C++ module. 99% of the functions are reimplemented now. Only 34 functions remain out of 5704 and all of them are part of the MVE video decoder library.

Roughly 90% of the source code … yada yada… bla bla bla…

**Who cares?! Is M.A.X. Port playable now?** Unfortunately the answer is a definitive no. The source code consists of 365 source files that contain 84047 lines of code excluding comments and empty lines. The industry average is 50 defects per 1000 lines of code so there are hundreds, if not thousands, of defects in the source code. Mistakes I made during the reimplementation process, defects that were present even in the original game.

**So what’s next?** The reimplementation issues have to be identified and resolved one by one which will take an unknown amount of time. At least I have a plan on how to approach testing in a systematic way. The game uses pseudo random numbers to make a lot of decisions and the random number generator’s seed is changing with the date and time. I will hardcode the seed so that each run will play exactly the same way in two independent game instances. One will run the original implementation, the other will run the reimplementation. As soon as major game or player state variables start to differ I will know where to look for defects.
<br>
<br>
<br>

Now back to the yada yada talk that only interests me and perhaps a few like minded enthusiasts…

### **C++ dialect, code quality of the game specific code base**

So roughly 90% of the source code is written in an ancient C++ dialect that attempts to mimic the state of the art C++ language from 1996 or so that was available in the Watcom C/C++ compiler used by M.A.X. v1.04 at the time. The reimplementation replicates the original architecture of the game and of course inherits many of its technical limitations as well.

The first ISO C++ standard was released only in 1998. Standardization took many years and compiler vendors usually lagged behind to follow up the latest published draft papers. Even though the favored programming language was ISO C at Interplay Productions at the time, M.A.X. developers took the risks and realized most of the game specific functionality using C++. Based on various Dr. Dobb’s Journal articles, smart pointers appeared as a concept in C++ around 1995. Most of the game objects are stored in custom made smart containers like smart lists or smart arrays that were templates. Serialization of such smart objects is handled by smart file managers. Exception handling was not fully supported by compilers at the time either. Failing freestore memory allocations simply returned a null pointer which required specific error handling that does not work together with exceptions. Run-Time Type Information (RTTI) was also just a draft and was not fully supported by compilers.

The old custom made smart pointer class is fast. Based on my performance benchmark tests they are roughly 30% faster than ISO C++ shared pointers (`std::shared_ptr<>`). But the price of this could be costly. The custom one does not follow standard semantics, is not thread safe, only supports objects that inherit from the smart object base class, and they do not handle circular references well for which the ISO C++ standard later introduced unique pointers and weak pointers. Of course the custom semantics are easy to follow, M.A.X. is fundamentally a single threaded application if we do not consider DPMI services and interrupts or their modern replacements provided by multi threaded SDL features, the game architecture is a given so deriving from the smart object base class is not an issue at all, and circular references could be handled on a per case basis.

Most of the other custom smart containers are very similar. For example the smart object array template class could be replaced by something like `template <typename T> using SmartArray = std::vector<std::shared_ptr<T>>;`. In theory custom smart container iteration loops could also be used with auto types and new forms of for loops without considerable issues.

One negative trait of custom smart containers I can think of now is the steep learning curve. Even some of the original authors had trouble following the correct semantics. See defect 92.

Another one may be compatibility with 64 bit platforms. Currently I cannot change the GitHub Actions Linux build job to the Ubuntu 20.04 runner as it simply fails to install multilib to be able to build 32 bit applications. The current runner is already deprecated so Linux support will have to be dropped soon unless the source code is converted to be 64 bit platform compatible. This step is surely out of scope now considering the number of defects to be tracked down.

In my personal opinion the lack of exceptions was a good thing with respect to game development use cases. I am surely ignorant, but I do not know of any deterministic straightforward approach to enumerate which standard library API will raise an exception when, or whether there are platform or architecture specific exceptions in standard libraries. EASTL provides the EASTL_EXCEPTIONS_ENABLED build configuration option for a reason.

In the long run, I am sure that the architecture of the game should be updated to be more aligned to the state of the art. For example in one of the upcoming ISO C++ standards the new and delete keywords will be banned. Porting the pre ISO C++98 source code to ISO C++11 simply does not make any sense. Actually the source code already relies on some ISO C++17 standard library APIs so C++11 is anyways out of the question. Support for multiple threads, the Vulkan API for rendering, OpenCL for calculations, OpenAL for audio, immediate mode GUI for UI, and rollback netcode all seem reasonable goals.

### **C dialect, platform library**
10% of the source code is ANSI C relying on some non ISO C extensions which further hinders portability and maintainability. Basically GNW is the only original platform library that remains in the game still. GNW manages human machine interfaces, input locales, character fonts, software video rendering, windowing and some basic graphical user interface widgets.

The original GNW library relied on operating system services, the VESA BIOS Extensions and similar low level hardware drivers. All these low level drivers are replaced by SDL library services. Basically GNW implements a 256 color palette soft renderer on top of a high performance hardware renderer which is nonsense. Keyboard input in game uses MS-DOS key or scan codes that are translated to USB scan codes that are converted to whatever nonsense. Raster fonts do not support but a few languages, non ASCII filenames could cause troubles and the list goes on.
It would make a lot of sense to either ditch SDL as-is and reimplement GNW to use GLFW directly or to ditch the GNW API and services as-is and rebuild them using SDL or GLFW or a game engine like Godot 4.x.

Historically 256 color palette based color animations were very powerful as the VESA BIOS did most work in hardware without consuming “any” CPU cycle time in a double buffered, seamless manner. Most effects in M.A.X. are nothing but rotated color codes within a system color palette. Even the water caustics are nothing but color manipulations. When M.A.X. 2 changed to its “exquisite 16-bit color” renderer color animations were not possible anymore. This is why the water does not wave in M.A.X. 2 anymore. They replaced color animated water with static low resolution textures and manipulated the map surface geometry instead. Now that hardware cannot be accessed directly anymore all the color palette transformations and the OpenGL surface color format conversions are done by SDL on the CPU and probably on the GPU which costs a lot of performance. Probably color animations could be done much more efficiently in a fragment shader or similar.

Rescaling and rendering map tiles, unit sprites and similar visual indicators like scan range circles or unit names, blending colors and handling transparency could be done much more efficiently on dedicated hardware. Even decade old hardware would perform better than the soft renderer that is available. I am not saying that the soft renderer is bad. It is doing an amazing job actually. Its selective rendering algorithm really does everything to only render the small section of the screen that actually changes. Of course color animations are not handled by the renderer, that was a totally different feature.

At 640 x 480 resolution the map surface is 448 x 448 pixels or 7 x 7 tactical map tiles. The tactical map surface is built up from about 15 layers. That means everything is drawn to screen layer by layer from bottom to top. For example the bottom layer contains the map tiles, the planet surface. On the next layer only torpedoes are drawn. Above that there are only water platforms. On the top layer there are only aerial explosions. Additionally there are 9 status indicator layers for unit names, unit hit points and similar and there is one more layer for status and event messages. Additionally the screen is split up into 59 window surfaces. The tactical map is one surface, the mini map is another, the File menu button is yet another.

Imagine that the 448 x 448 pixels tactical map could in theory hold 7 * 7 * 15 sprites that are drawn onto the screen one by one if they changed since the last draw call. Ignore the fact that there are 9 + 1 additional layers for status indications.

Now imagine that we have a screen resolution of 1920 x 1080 pixels with a map surface of 1344 x 960 pixels or 21 x 15 tactical map tiles. That means there could be 21 x 15 x 15 sprites to be drawn which is a 650% increase. A single threaded application like M.A.X. would need a lot more computational power from that single CPU core it uses.

M.A.X. scales every sprite on the fly without storing them for reuse to keep memory usage minimal as back in the day RAM usage was the biggest concern of the game. This constant rescaling activity also takes considerable computational power even if the algorithm and the size of sprites are relatively small. OpenGL would do this scaling in better quality, faster, and probably only once if a mipmap is used. RAM space is abundant now. M.A.X. Port runs with 26 MiB of allocated RAM on average. It would be possible to preload all 17 MiB of game assets into OpenGL optimized data structures, textures, whatever, and cache the data in non volatile memory between game sessions. The game map is always 112 x 112 tiles. Each tile is 64 x 64 pixels without an alpha channel which means that the entire map surface could be loaded into a single texture consuming about 147 MiB of system RAM. Yeah, maybe that is simply stupid… but technically possible still.

### **MVE video decoder**
As mentioned before the MVE video decoder library is not finished yet. The library uses self modifying code on assembly level. These highly optimized assembly routines are not portable and need to be replaced by potentially slower, but portable ISO C compliant alternatives. This is why the remaining 34 functions will be finished only much later on. There is a further problem, namely that the original MVE video decoder used its own custom video and audio hardware drivers and the architecture and capability of the original audio driver was better than what is available out of the box with SDL_audio and SDL_mixer. Eventually the video decoder will be made available, but it will remain on very low priority for some time.

### **Audio playback**
The original audio library of M.A.X. was HMI S.O.S which supported audio loop points for sound effects. Unfortunately SDL_mixer does not have such a feature. Many audio samples of M.A.X. have three sections. The first section is usually a ramp up or start up phase, the second section is a continuous operation or simply an on phase which is looped, the third is a ramp down or stop phase. Without support for loop points, the audio samples have to be cut up and the application needs to seamlessly put together the parts again via proper scheduling and management of additional metadata. A lot of work for something that is readily available in other audio libraries.

### **Network play**
The last missing feature of M.A.X. Port is the in-game netcode. The application layer of the networking protocol is ready, the Internet and Link layers are provided by SDL_net, only the transport layer is incomplete. The lobby and the game itself uses different protocols. The lobby is somewhat working, but the input-synchronous, lockstep peer-to-peer in-game protocol is broken completely. The original and the new architectures are completely different. This is the lowest priority for now.
