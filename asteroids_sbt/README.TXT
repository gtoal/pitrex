You need to find the rom files

035145.02
035144.02
035143.02
035127.02

on your own, which are the Asteroids rom files used by MAME and others.

Unzip them here.  

trans.exe takes the rom files and translates them to C code, namely
game.c.  game.c is #included in gmain.c.

For both, I probably dont have hyperspace mapped, so when you get the high score
after the first game just hit the start button again and it will bypass the 
initials and go into a new game.

I hope to do another writeup, for now this is the writeup from the last time I 
did this translation:
http://www.dwelch.com/ipod/source.htm

dwelch@dwelch.com
