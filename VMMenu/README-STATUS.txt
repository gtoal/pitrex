This module is under development.

The module as written was displaying both on the raster display (with SDL) and the vector display (originally the ZVG,
then the DVG, and now the PiTrex) but in order for the linux-hosted version to work, we have to disable the HDMI
display ... and this causes the SDL code to fail to initialise.  At first it looked like we'ld have to remove the SDL
calls, but then we found a sneaky hack where we started the menu with HDMI enabled and then rapidly shut it off.

At some point it'll be better to remove the SDL entirely.

It would be nice if we could add the PiTrex code completely conditionally so that this still works with both
the ZVG and DVG, but be prepared for an expedient decision to remove the legacy code if need be.

We need to move the sounds from the Pi side to the Vectrex side.  Or at a minimum, disable them since the Pi Zero
doesn't have sound output anyway (at least not when the HDMI is off)

The graphics.c code duplicates some code handled by this package.  Unless window.c moves into the library,
it might be worth cutting down considerably.

The menu should either be cut down to what we support - or we should implement advmame asap!

The menu script needs to be in a loop so that exiting a program doesn't exit the menu.  Or just disable the
1+2+3+4 mechanism for the menu itself.

The display is a bit flickery when there is a lot of text on the screen.  Maybe change the number of games
that can be listed at once from 13 to 8 or less?  If you force a reset0-ref on every vector the flicker
becomes unacceptable on all screens.  I currently only do it when there's a move, but not for abutting vectors.
Would be nicer to do it at a higher level only when actually necessary.

(Slightly tangential: add a linux command line as a menu option.)
