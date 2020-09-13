This module is under development.

Recently got graphics starting to work, have not yet added joystick/button input to actually do anything with it!

The module as written was displaying both on the raster display (with SDL) and the vector display (originally the ZVG,
then the DVG, and now the PiTrex) but in order for the linux-hosted version to work, we have to disable the HDMI
display ... and this causes the SDL code to fail to initialise.  So it looks like we'll have to remove the SDL
calls.  This wouldn't be a problem in a bare metal implementation but we'll concentrate on the Linux version first.

