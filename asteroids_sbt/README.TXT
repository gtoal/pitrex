
You need to find the rom files

035145.02
035144.02
035143.02
035127.02

on your own, which are the Asteroids rom files used by MAME and others.

Unzip them here.  

Check build.bat or the makefile to see that you have the right
build tools.  This probably will not build with GPHs gcc you may
need one of mine (and may have downloaded it with this file/release).

trans.exe takes the rom files and translates it to C code, namely
game.c.  game.c is #included in gmain.c.

copy the aload940t.gpe program and the 00000000.bin file to an SD card
(both in the same directory) aload940t will read 00000000.bin, load it into
the 940 and put the 920 in an infinite loop.  The 940 will take over
and run the game.  You will have to cycle power to get back to linux.
The A and Y buttons rotate left.  B and X rotate right.  Start is the one
player start.  volume down is fire.  Volume up is thrust.  

The winast directory contains a windows version.  You dont need to build the
gp2x binary to use the windows version.  Trans.exe will create game.c and
rom.h (as run in Makefile or build.bat).  You will need MinGW installed to
build the rest of winast.  ESC to exit, the number 1 to start one player,
left and right rotate left and right.  Ctrl fires.

For both, I probably dont have hyperspace mapped, so when you get the high score
after the first game just hit the start button again and it will bypass the 
initials and go into a new game.

I hope to do another writeup, for now this is the writeup from the last time I 
did this translation:
http://www.dwelch.com/ipod/source.htm

dwelch@dwelch.com
