# M.A.X. Port 

M.A.X. Port is an [SDL library](https://wiki.libsdl.org/) based runtime executable for the 1996 DOS game M.A.X.: Mechanized Assault & Exploration developed and published by Interplay Productions.

Official home page: [https://klei1984.github.io/max/](https://klei1984.github.io/max/) 

The goal of the project is to fix the game breaking [defects](https://klei1984.github.io/max/defects/) present in the original game executable and to create a runtime that natively runs on modern operating systems.

M.A.X. Port requires the original game data. The port only provides a modernized executable.

### How to play M.A.X. Port

1. Install the original M.A.X. game onto your computer.
   - If you still have the original M.A.X. CD-ROM you have to use a DOS emulator to install the game as only an MS-DOS compatible installer is found on the CD. Make sure to select **full** installation within the installer if you do not wish to keep the CD in your CD-ROM drive all the time. The game needs to be patched to v1.04 after installation. 
   - If you have the gog.com release of the game it is already patched to v1.04.
   - If you have the steam release of the game it is already patched to v1.04.

2. [Download](x) M.A.X. Port or [build](x) the runtime using the source code.
   - Copy the runtime to your M.A.X. installation folder, e.g. C:\MAX.

3. The mve video player library is not yet ported to the SDL2 backend. Due to this the following game files shall be renamed to something else in the M.A.X. installation folder: MAXINT.MVE MAXMVE1.MVE MAXMVE2.MVE.
