PROVISIONAL Instructions for how to set up the bare-metal environment, so
that you can boot straight into a game or a menu.  Better instruction
will probably be available on the wiki by the time you read this.

The stand-alone boot environment goes in the first partition on the SD.
If you installed using NOOBS then that is /boot, but if you did a manual
partitioning, it may not be.  But if you did that, you know what you're
doing, and you'll be able to work out where the *.img files need to go.

Add these to /boot/config.txt - the gpio commands are to avoid bringing up
gpio pins in the wrong mode and potentially doing electrical damage.

The dtoverlay is to use alternative USB handling code - that's actually needed
more for linux-hosted code than baremetal, but you might as well put it in
there now before you forget.

gpio=0-5,16-24,26-29=ip
gpio=6-13,25=op
gpio=24=np
dtoverlay=dwc2,dr_mode=host

You'll need some subdirectories in /boot: ini settings roms vectrex
we may rationalise this structure before final release so be ready to
handle changes in your own programs.

