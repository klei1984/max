---
layout: page
permalink: /download/
title: Downloads
dl_url: https://github.com/klei1984/max/releases/download
release: 0.7.2
releaseDate: 2025-06-24
tag: 0.7.2
nightly_tag: nightly
nightly: max-port-nightly
---

### Current Release: v{{ page.release }} ({{ page.releaseDate }})

Read the [Installation Guideline](install.md) on how to use.<br>
Read the [FAQ](faq.md) if in doubt.

- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows.svg" width="18px" height="18px"/> &ensp; **[Windows installer 64 bit]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-Windows_x86_64.exe)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows.svg" width="18px" height="18px"/> &ensp; **[Windows 7-Zip file 64 bit]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-Windows_x86_64.7z)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows.svg" width="18px" height="18px"/> &ensp; **[Windows installer 32 bit]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-Windows_x86.exe)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows.svg" width="18px" height="18px"/> &ensp; **[Windows 7-Zip file 32 bit]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-Windows_x86.7z)**

- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows_xp.svg" width="22px" height="22px"/> &ensp; **[Windows XP installer 64 bit]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-WindowsXP_x86_64.exe)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows_xp.svg" width="22px" height="22px"/> &ensp; **[Windows XP 7-Zip file 64 bit]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-WindowsXP_x86_64.7z)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows_xp.svg" width="22px" height="22px"/> &ensp; **[Windows XP installer 32 bit]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-WindowsXP_x86.exe)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows_xp.svg" width="22px" height="22px"/> &ensp; **[Windows XP 7-Zip file 32 bit]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-WindowsXP_x86.7z)**

- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/linux.svg" width="22px" height="22px"/> &ensp; **[Linux DEB package 64 bit for x86-64 systems]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-Linux_x86_64.deb)**

- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/flatpak.svg" width="22px" height="22px"/> &ensp; **[Linux Flatpak package 64 bit for x86-64 systems]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-Linux_x86_64.flatpak)**

- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/arch_linux.svg" width="22px" height="22px"/> &ensp; **[Arch Linux package 64 bit for x86-64 systems]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-Linux_x86_64.pkg.tar.zst)**

Read the [Build Instructions](build.md) on how to build.

- **[Source .7z]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-Source.7z)**
- **[Source .tar.gz]({{ page.dl_url }}/v{{ page.release }}/max-port-{{ page.tag }}-Source.tar.gz)**

### Latest Nightly Build

Nightly builds are rebuilt automatically from the tip of the `master` branch after every successful CI build, so they carry the newest fixes and features long before they reach a formal release. The links below are permanent, they always point to the most recent successful build.

**These are development builds, not releases.** They are compiled without optimizations, they run noticeably slower than the release above and they enable developer only debug features. They may also be unstable or contain regressions. See the [FAQ](faq.md#nightly-builds) before you use them.

**Saved games are not guaranteed to be interoperable between development builds!** The `V71` save file format is still under development and changes without notice. Saved games in the original `V70` format are loadable by all development builds.

- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows.svg" width="18px" height="18px"/> &ensp; **[Windows installer 64 bit]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-Windows_x86_64.exe)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows.svg" width="18px" height="18px"/> &ensp; **[Windows 7-Zip file 64 bit]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-Windows_x86_64.7z)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows.svg" width="18px" height="18px"/> &ensp; **[Windows installer 32 bit]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-Windows_x86.exe)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows.svg" width="18px" height="18px"/> &ensp; **[Windows 7-Zip file 32 bit]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-Windows_x86.7z)**

- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows_xp.svg" width="22px" height="22px"/> &ensp; **[Windows XP installer 64 bit]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-WindowsXP_x86_64.exe)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows_xp.svg" width="22px" height="22px"/> &ensp; **[Windows XP 7-Zip file 64 bit]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-WindowsXP_x86_64.7z)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows_xp.svg" width="22px" height="22px"/> &ensp; **[Windows XP installer 32 bit]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-WindowsXP_x86.exe)**
- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/windows_xp.svg" width="22px" height="22px"/> &ensp; **[Windows XP 7-Zip file 32 bit]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-WindowsXP_x86.7z)**

- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/linux.svg" width="22px" height="22px"/> &ensp; **[Linux DEB package 64 bit for x86-64 systems]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-Linux_x86_64.deb)**

- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/flatpak.svg" width="22px" height="22px"/> &ensp; **[Linux Flatpak package 64 bit for x86-64 systems]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-Linux_x86_64.flatpak)**

- <img class="themed-svg" style="float: left" src="{{ site.baseurl }}/assets/images/arch_linux.svg" width="22px" height="22px"/> &ensp; **[Arch Linux package 64 bit for x86-64 systems]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-Linux_x86_64.pkg.tar.zst)**

- **[Source .7z]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-Source.7z)**
- **[Source .tar.gz]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-Source.tar.gz)**

Debug symbol archives, the Debian `.ddeb` debug package and the [SHA-256 checksums]({{ page.dl_url }}/{{ page.nightly_tag }}/{{ page.nightly }}-SHA256SUMS.txt) of all the above files are attached to the [nightly release](https://github.com/klei1984/max/releases/tag/{{ page.nightly_tag }}) page, which also states the exact commit the build was made from.

### Previous Releases

Download previous releases from [GitHub](https://github.com/klei1984/max/releases).
