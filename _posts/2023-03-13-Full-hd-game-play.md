---
layout: post
title:  "Full HD game play"
date:   2023-03-13
videoId: EwZb10FiTdc
categories:
excerpt_separator: <!--more-->
---
<!--more-->
The latest GitHub Actions builds are relatively playable in higher screen resolutions. Only a couple less frequently used screens are not rescaled and repositioned. The game still uses a software renderer which means there is high performance improvement potential for a hardware accelerated render pipeline which could additionally support shaders based post processing effects to simulate a CRT monitor or similar. Text is not rescaled yet.
<br><br>
{% include yt_player.html id=page.videoId %}
<br>

### How to enable higher resolutions within the latest GitHub Actions builds?

The user needs a text editor that could save text files using standard MS Windows line delimiters `CRLF`. The `max.ini` configuration file found next to the game executable contains a graphics settings specific section close to the bottom of the file.

```CPP
[GRAPHICS_SETTINGS]
screen_mode=2
scale_quality=1
window_width=640
window_height=480
dialog_center_mode=0
```

***screen_mode***:
- 0 - Windowed (still grabs mouse pointer)
- 1 - Full Screen
- 2 - Borderless Full Screen (best)
<br><br>

***scale_quality***:
- 0 - Nearest neighbor (crisp, looks good on CRT monitors)
- 1 - Linear interpolation (smoother, aliasing artifacts are not that visible)
- 2 - Best (whatever that is in SDL2)
<br><br>

***window_width & window_height***:

| Width | Height | Aspect ratio | Tactical map size | Notes |
|-------|--------|---------|---------|---------|
| 640 | 480 | 4:3 (1.33:1) | 7 x 7 tiles | Near pixel perfect original M.A.X. screen resolution. Text is rendered using original expected font size. |
| 853 | 480 | 16:9 (1.78:1)| 10 x 7 tiles | Near pixel perfect original M.A.X. screen resolution in widescreen mode. Text is rendered using original expected font size. |
| 768 | 480 | 16:10 (1.6:1) | 9 x 7 tiles | Near pixel perfect original M.A.X. screen resolution in widescreen mode. Text is rendered using original expected font size. |
| 1440 | 1080 | 4:3 (1.33:1) | 16 x 15 tiles | Full HD standard mode. |
| 1920 | 1080 | 16:9 (1.78:1) | 23 x 15 tiles | Full HD widescreen mode. |

***dialog_center_mode***:
- 0 - Center in-game popup windows to middle of tactical map.
- 1 - Center in-game popup windows to middle of game screen.
<br><br>

The video clip was created using the following settings:

```CPP
[GRAPHICS_SETTINGS]
screen_mode=2
scale_quality=2
window_width=1920
window_height=1080
dialog_center_mode=1
```


### Why text is so small at higher resolutions?

Text is still rendered by GNW's raster font manager which implements six fonts, but uses only three of them. The raster fonts' em size is optimized for 640 x 480 screen resolution. Raster fonts do not scale well and the font manager does not support DPI or em size correction. To be able to scale grapheme clusters in adequate quality three new TrueType fonts will be created and the font manager will be redesigned to work with SDL_ttf and TrueType fonts while keeping the ability to draw multi color shaded glyphs via GNW’s Text API to keep that old genuine look and feel.

The raster fonts are encoded according to code page 437. Half of the glyphs are not implemented from the code page while others are not recognizable. The fonts themselves are monochromatic and support no opacity contrary to some of the fonts that Fallout used. To support as many languages as possible the new TrueType fonts will use a Unicode code page and all text in-game will be externalized into UTF-8 encoded language files.

Keyboard input is locale dependent. As of today M.A.X. Port converts USB key or scan codes to a standard US MS-DOS keyboard layout. Keyboard hotkeys or shortcut keys are all hard coded in the source code. To support text input better the MS-DOS keyboard emulation needs to be removed, keyboard locales and keyboard layouts need to be considered by the game and text input needs to be converted to UTF-8 encoded grapheme clusters which is far from trivial considering the original saved game file format limitations for text fields like the player’s name.
