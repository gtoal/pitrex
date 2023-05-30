XMAME v0.106 modified for the PiTrex (VeXMAME)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

XMAME is a fork of MAME v0.106, which is also what AdvanceMAME is
originally based on, except that they ripped out the direct X11
output support. This modified "VeXMAME" version is designed for use
in combination with the modified "Xvectrex" version of the XFree86 X
server to display on the Vectrex using the PiTrex. For optimal
performance, it should be run on a PC and access the X server on
the Raspberry Pi Zero as a remote X display.

This modified version of XMAME doesn't actually tie in directly with
any of the PiTrex code, hence it can be built without any of the
PiTrex libraries. What it does is implement accelerated vector
rendering when the X driver is used, which allows the X server to
use XAA acceleration, which is how the XFree86 PiTrex driver
accesses vector data. This is enabled using the "-hwvec 2" option,
without which many games will not display on the Vectrex.

This XMAME will still display on PC. The accelerated mode "-hwvec 2"
will look pretty terrible (this may be improved later), but the
normal bitmap rendered mode can be forced with the "-hwvec 0"
option. Games that don't use vector graphics will always display
normally on PC.

Building / Installing
^^^^^^^^^^^^^^^^^^^^^
For now the focus for this is mainly on building on PC, but this Pi
port of XMAME will be investigated later:
https://github.com/slaanesh-dev/xMame37B16-Pi

A bootable ISO may be made available later that can be directly
booted or run in a virtual machine or x86 emulator, so that not
everyone needs to run Linux and compile this themselves.

The Git repo contains only the source files that were modified from
the original XMAME release. You need to get the original
sources and unpack them in the root of the git repo directory (where
the "xc" directory is), running tar with the -k option so that it
doesn't overwrite the modified files.

Download the required XMAME source archive (16MB) from here:
http://web.archive.org/web/20090308015400/http://x.mame.net/download/xmame-0.106.tar.bz2
or here (20MB):
http://archive.debian.org/debian-archive/debian/pool/non-free/x/xmame/xmame_0.106.orig.tar.gz

Then unpack using "tar xvjkf xmame-0.106.tar.bz2" (or "tar xvzkf
xmame_0.106.orig.tar.gz)

Alternatively the original sources can be unpacked to an empty
location, and patched using the latest patch file available here:
http://www.ombertech.com/cnk/pitrex/ftp/cnk/VeXMAME/patches/

Enter the xmame-0.106 directory created when the XMAME sources were
unpacked and run the following command after downloading the patch
file to the above directory:
 xz -d -c ../xmame-0.106_pitrex_0.1.0.patch.xz | patch -p1

First step for building is to look over the Makefile (makefile.unix)
and adjust things to suit your environment. The graphics options are
all pre-set to build just the X11 driver that can display on the
PiTrex, but you may want to customise other settings to suit your
Linux distro and hardware.

The Xlib libraries on your system will do fine, you don't need to
install the libraries from the modified XFree86 package for this to
work with the PiTrex.

Run "make" to compile everything, it will take a while (you might
want to use the "-j" option with the number of CPU cores like
"-j[CPU cores]" to speed things up).

xmame.x11 is the binary, run it with "./xmame.x11". "sudo make
install" will install everything to the locations specified in the
Makefile.

Change settings by editing "~/.xmame/xmamerc".

ROMs
^^^^
MAME 0.106 ROMs should work, which are also what AdvanceMAME use. It
seems to work with the same ROMs as AAE as well.

The location where XMAME looks for ROMs can be configured in
"~/.xmame/xmamerc".

Running
^^^^^^^
From PC, after X server running PiTrex drivers has been started on
the Raspberry Pi, run:

DISPLAY=raspberrypi:0 xmame -hwvec 2 -skip_gameinfo -skip_warnings -skip_disclaimer omegrace

If your LAN doesn't have a DNS server that resolves the
"raspberrypi" host name, replace that with its IP address (or add it
to your hosts file).

The "-skip_" options are needed because the info screens are bitmaps
and won't display on the Vectrex.

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
loaded in order to access the "insert coin" and "start" buttons.
Pass the "-alwaysusemouse" option to xmame so that the mouse is
used. Left, Middle, and Right mouse buttons map to Vectrex
controller buttons 2, 3, and 4 respectively.

Example XF86Config files are provided, "xmame-0.106/XF86Config" for
games with digital controls, and "xmame-0.106/XF86Config_analogue"
for games that need analogue controls and therefore require the
vecmouse driver. Start the X server with the
"-xf86config [config file]" option so that it uses one of these
instead of the default at /etc/X11/XF86Config.

Developers
^^^^^^^^^^
The main thing is this file:
src/unix/video-drivers/x11vec.c

That is the bare-bones accelerated X11 vector video driver. Scaling
code there is to fit the display within the X window rather than to
the Vectrex display.

Remaining modifications are minor things. x11_window_update_display
in "src/unix/video-drivers/x11_window.c" is where the display
actually gets updated by XMAME, and may be a place for more
improvements.

It would be nice to pull in Graham's PiTrex driver for AdvanceMAME,
as an accelerated driver like this, or maybe even an alternative
mode for it. This would have to be disabled when building on PC of
course.

In theory Xlib and MAME can both be built for Windows, so a Windows
build could be possible. No current developers intend to attempt
this due to likely difficulties.

Why XMAME?
^^^^^^^^^^
XMAME hasn't been updated since 2006, whereas MAME itself is still in
active development. Current MAME is quite different though, with the
graphics code apparently undergoing a major re-write after version
0.106 (which I guess is why XMAME and AdvanceMAME are both stuck at
that base version). The code for current MAME was found to be much
more difficult to understand, and AdvanceMAME doesn't support X11
display directly, so XMAME was the easiest target. Other people who
can see how this could be done in current MAME are welcome to try,
and to share their results.

Unlike XFree86, the XMAME code seems to have aged fairly gracefully
and no changes were required to the code in order to make it compile
on a current (Debian Stable in 2022) Linux system.

Performance may have also dropped off after 0.106, which is
important for trying to run it as well as possible on the Pi:
https://aarongiles.com/old/?m=200704
