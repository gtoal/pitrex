XMAME 37B16 (0.52) Raspberry Pi mod (xMame37B16-Pi) modified for
                    the PiTrex (VeXMAME-Pi)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
XMAME-Pi is a fork of XMAME 37B16 (0.52) made by Slaanesh which is
designed for optimal performance on the original Raspberry Pi (same
SoC as the Raspberry Pi Zero). This modified "VeXMAME-Pi" version is
designed for use in combination with the modified "Xvectrex" version
of the XFree86 X server to display on the Vectrex using the PiTrex.
In the future it might also include a driver for direct control of the
Vectrex via the Vectrex Interface library.

This modified version of XMAME doesn't actually tie in directly with
any of the PiTrex code, hence it can be built without any of the
PiTrex libraries. What it does is implement accelerated vector
rendering when the X driver is used, which allows the X server to
use XAA acceleration, which is how the XFree86 PiTrex driver
accesses vector data.

xMame37B16-Pi by Slaanesh:
https://github.com/slaanesh-dev/xMame37B16-Pi

Building / Installing
^^^^^^^^^^^^^^^^^^^^^
The Git repo contains only the source files that were modified from
the original XMAME release. You need to get the original
sources and unpack them in the root of the git repo directory (where
the "xc" directory is), running tar with the -k option so that it
doesn't overwrite the modified files.

Download the required XMAME source archive (5MB) from here:
http://web.archive.org/web/20031128085707if_/http://x.mame.net:80/download/xmame-0.37b16.1.tar.bz2

Then unpack using "tar xvjkf xmame-0.37b16.1.tar.bz2"

Alternatively the original sources can be unpacked to an empty
location, and patched using the latest patch file available here:
http://www.ombertech.com/cnk/pitrex/ftp/cnk/VeXMAME-Pi/patches/

Enter the xmame-0.37b16.1 directory created when the XMAME sources
were unpacked and run the following command after downloading the
patch file to the above directory:
 xz -d -c ../xmame-0.37b16.1_pitrex_0.1.0.patch.xz | patch -p1

The Makefile is already configured for the Raspberry Pi and PiTrex
via X11.

Build by running:
 ./m68k.sh
 make

"sudo make install" will install everything to the locations
specified in the Makefile.

Running
^^^^^^^
After the X server running PiTrex drivers has been started on
the Raspberry Pi, run:

 DISPLAY=:0 xmame.x11.37b16 [game]

To use a bluetooth or USB controller connected to the Raspberry Pi
(not including the Vectrex controller), add "-jt 4" to the
command-line. For analogue controls in games that require them, use
"-analogstick" as well.

The veckeyboard XFree86 driver works well for many games. The
following button assignments for the keyboard section of the
XF86Config file will set a combination that works in many games:

        Option      "B2+3Key" "1"       # Esc (exit)
        Option      "B3+4Key" "6"       # 5 (insert coin)
        Option      "B1Key" "2"         # 1 (start)        
        Option      "B2Key" "57"        # SpaceBar (fire 2)
        Option      "B3Key" "56"        # Left-Alt (thrust)
        Option      "B4Key" "29"        # Left-Ctrl (fire 1)

Games designed for an analogue joystick, like Star Wars, are better
using the vecmouse driver. The veckeyboard driver still needs to be
loaded in order to use the buttons.

Example XF86Config files are provided, "xmame-0.37b16.1/XF86Config"
for games with digital controls, and
"xmame-0.37b16.1/XF86Config_analogue" for games that need analogue
controls and therefore require the vecmouse driver. Start the X
server with the "-xf86config [config file]" option so that it uses
one of these instead of the default at /etc/X11/XF86Config.

USB audio should be automatically enabled if it is configured. For
Bluetooth audio to play reliably with minimum lag, the Bluetooth
audio profile should be set to "sco" in /etc/asound.conf:
defaults.bluealsa.profile "sco"

The snd-aloop kernel module should be loaded:
 sudo modprobe snd-aloop

An audio loop-back stream should be created to feed the Bluetooth
speaker using Sox:
 AUDIODEV=hw:0,1,0 rec -q --buffer 150 -r 8000 -c 1 -p | AUDIODEV=bluealsa play --buffer 50 -r 8000 -c 1 -p -q &

Then XMAME should be started with the following options in addition
to any others that are desired:
 DISPLAY=:0 xmame.x11.37b16 -audiodevice hw:0,0,0 -samplefreq 8000 -nostereo [game]

Sound via Bluetooth will be a bit scratchy when used in combination
with the Vectrex display. Using a Bluetooth controller and speaker at
the same time may be unreliable.
