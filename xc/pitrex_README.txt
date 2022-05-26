XFree86 4.8.0 for PiTrex (Xvectrex)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In order to aid displaying the output of X-based programs on the
Vectrex, icluding those running on another computer to improve
performance, XFree86 has been modified to work with the PiTrex
software for display of vector graphics and to accept controller
inputs.

Installation and configuration requires decent familiarity with X.
Full details will in time be added here on the PiTrex Wiki:
http://www.ombertech.com/cnk/pitrex/wiki/index.php?wiki=Xvectrex

Key changes to the original XFree86 code are:

* Fixes to the original code so that it works in modern Linux
  running on a 32-bit ARM CPU (ARM was officially supported in
  XFree86 4.8.0, but actually broken).

* Hacking the Dummy graphics driver to support XAA acceleration
  and direct corresponding vector info to the Vectrex Interface
  library.

* Adding input drivers which access the Vectrex controller via the
  Vectrex Interface library and pretend to be either a keyboard or
  a mouse.

The reason for not using the more up-to-date Xorg version of X is
that Xorg removed the XAA acceleration layer which is used to
access vector display data.

Information and documentation about XFree86 can be found at the
official website:
http://www.xfree86.org/

Building / Installing
^^^^^^^^^^^^^^^^^^^^^
Note that non-developers should use a pre-packaged PiTrex Linux
image with X already installed and configured, when available.

Note that initially this has only been compiled with GCC 8.3.0 on
Raspbian version 10 (Buster) because this was current when work
was started. There may be compiling issues on more up-to-date
systems. Also note that some manual symlinking etc. may have been
used to fix some compiling problems. These potential issues will be
investigated at the next stage involving building on a clean
system.

Kernel header files are required for the build. In Raspbian 10 the
"raspberrypi-kernel-headers" package was installed for this.

The Git repo contains only the source files that were modified from
the original XFree86 4.8.0 release. You need to get the original
sources and unpack them in the root of the git repo directory (where
the "xc" directory is), running tar with the -k option so that it
doesn't overwrite the modified files.

Download the required XFree86 source archives (only 1-5 required, 46MB
total) from here:
ftp://ftp.xfree86.org:21/pub/XFree86/4.8.0/source/

Then unpack using "tar xvzk".

This one-liner can be used to do this automatically:
  for (( count = 1 ; count < 6 ; count++ )); do wget -q -O - ftp://ftp.xfree86.org/pub/XFree86/4.8.0/source/XFree86-4.8.0-src-$count.tgz | tar xvzk; done

Alternatively the original sources can be unpacked to an empty
location, and patched using the latest patch file available here:
http://www.ombertech.com/cnk/pitrex/ftp/cnk/Xvectrex/patches/

Enter the xc directory created when the XFree86 sources were
unpacked and run the following command after downloading the
patch file to the above directory:
  xz -d -c ../XFree86-4.8.0_pitrex_0.1.0.patch.xz | patch -p1

The standard instructions for compiling and installing XFree86
apply, as documented in the BUILD.txt file. The configuration
is already set to build only the PiTrex-related device drivers,
and to suit Raspberry Pi OS, so you should only have to run:
  make World
  sudo make install
Note that compiling XFree86 on a Raspberry Pi Zero will take hours.

Configuration
^^^^^^^^^^^^^
Important configuration is all in the XF86Config file for which
full documentation can be viewed after installation with
"man XF86Config" or at this link:
  http://www.xfree86.org/4.8.0/XF86Config.5.html

The default file is loaded from /etc/X11/XF86Config, but to allow
per-game button and scaling settings, it is recommended to load a
different one for each game by starting X with the
"-xf86config [file]" option. An example is given below using the
keyboard controller driver and both X and Y display scaling set to
0.5. Note that standard keyboard ("kbd") and mouse ("mouse") input
drivers can also be used for USB/bluetooth attached devices, and
"void" can be used where no device is to be available (used for the
mouse driver in the example).

-------------------------------------------------------------------
Section "ServerLayout"
        Identifier     "Layout0"
        Screen      0  "Screen0" 0 0
        InputDevice    "Keyboard0" "CoreKeyboard"
        InputDevice    "Mice" "CorePointer"
EndSection

Section "Module"
        Load "freetype"
        # Load "xtt"
        Load  "extmod"
        Load  "glx"
        Load  "dri"
        Load  "dbe"
        Load  "record"
        Load  "xtrap"
        Load  "type1"
        Load  "speedo"
EndSection
Section "InputDevice"
        Identifier  "Mice"
#       Driver      "vectrexmouse"
        Driver      "void"
EndSection

Section "InputDevice"
        Identifier  "Keyboard0"
        Driver      "vectrexkbd"

      ### Button to key assignments ###
        Option      "B2+3Key" "24"
        Option      "B2+4Key" "37"
        Option      "B1Key" "6"
        Option      "B2Key" "2"
        Option      "B3Key" "56"
        Option      "B4Key" "29"
EndSection

Section "Monitor"
        Identifier   "Monitor0"
        HorizSync    31.5 - 35.1
        VertRefresh  50.0 - 70.0
EndSection

Section "Device"
        Identifier  "PiTrex"
        Driver      "dummy"

      ###     Display Scaling      ###
        Option      "Xscale" "0.5"
        Option      "Yscale" "0.5"
EndSection

Section "Screen"
        Identifier "Screen0"
        Device     "PiTrex"
        Monitor    "Monitor0"
        DefaultDepth     16
        SubSection "Display"
                Viewport   0 0
                Depth     16
                Modes    "800x600"
        EndSubSection
EndSection
-------------------------------------------------------------------

Drivers
^^^^^^^
Options for the new/modified drivers follow.

-- dummy (hacked display driver, to be replaced) --

"Xscale" - scale factor for the X axis (default "1")
"Yscale" - scale factor for the Y axis  (default "1")
"Xoffset" - offset value for the X axis (default "0")
"Yoffset" - offset value for the Y axis (default "0")

-- vectrexkbd --

Button Assignments
"B1Key" (default: "29" (Left Ctrl))
"B2Key" (default: "56" (Left Alt))
"B3Key" (default: "28" (Enter))
"B4Key" (default: "57" (SpaceBar))
"B1+2Key" (default: unused)
"B1+3Key" (default: unused)
"B1+4Key" (default: unused)
"B2+3Key" (default: unused)
"B2+4Key" (default: unused)
"B3+4Key" (default: unused)
"UpKey" (default: "90" (Up))
"DownKey" (default: "96" (Down))
"LeftKey" (default: "92" (Left))
"RightKey" (default: "94" (Right))
  - These options only accept X key numbers. See the list in the
    file "xc/programs/Xserver/hw/xfree86/common/atKeynames.h" and
    copy matching numbers from the "Pressed value (dec)" column.
"Timeout" - Interval in miliseconds between checking for change in
            button state. (default: "20")
"ReadAnalogue" - Read Vectrex controller joystick in analogue mode
                 so that spurious "down" signals are avoided
                 (workaround for a bug in the Vectrex Interface
                 lib). (default: "true")
"AnalogueThreshold" - Threshold for press/release of keyboard
                      button corresponding to joystick movement
                      in analogue mode. (default: "75")
"AnalogueDebounce" - 'Debouncing' for press/release of keyboard
                      button corresponding to joystick movement
                      in analogue mode. Corresponds to the number
                      of times that a state change is detected
                      before it is taken seriously. (default: "2")
"debug_level" - Set level of debugging messages printed by server
                to stdout and log. (default: "0" = disabled)

-- vectrexmouse --

"MaxX" - Max X value (default: "1000")
"MinX" - Min X value (default: "0")
"MaxY" - Max Y value (default: "1000")
"MinY" - Min Y value (default: "0")
"CenterX"  - X center value (default: "-1")
"CenterY"  - Y center value (default: "-1")
"Delta" - delta cursor (default: "100")
"RelativeMovement" - Relative vs absolute movement mode.
                     (default: "true")
"Timeout" - Interval in miliseconds between checking for change in
            button state. (default: "20")
"DebugLevel" - Set level of debugging messages printed by server
                to stdout and log. (default: "0" = disabled)

Running
^^^^^^^
Hardware auto-detection is disabled because it was causing a seg.
fault and serves no purpose on the Pi. In other words the "-nohw"
option is now default when built for ARM32.

When starting the X server from the command prompt, it is
recommended to use the "-keeptty" option so that the X server can
be shut down with Ctrl-C. Note that at the moment the server seg.
faults during shut-down, which isn't ideal.

To enable remote connections, add the name or IP address on your
network of the computer you want to connect to the Pi to the file
/etc/X0.hosts (newline-separated). Or use the "xhost" program for
temporary changes to permissions.

After loading the X server, set the DISPLAY environment variable
to ":0" before attempting to launch programs, eg.
  export DISPLAY=:0

When a program stops updating the display (eg. closes), the PiTrex
display driver will continue displaying the last complete frame on
the screen.

See the PiTrex Wiki for instructions on configuring Linux so that
the vector drawing routines are never interrupted, causing
glitches.

Developers
^^^^^^^^^^
PiTrex-specific code is all in the locations of the individual
driver sources:

dummy: programs/Xserver/hw/xfree86/drivers/dummy
vectrexkbd: programs/Xserver/hw/xfree86/input/vectrexkbd
vectrexmouse: programs/Xserver/hw/xfree86/input/vectrexmouse

Testing the display is easily done with the ico program installed
with XFree86:
  ico -sleep 0.02

Testing controller to keyboard or mouse drivers is done with the
xev program:
  xev -geometry 640x480

These can be run either from the Pi, or on another computer and
displaying as a remote X window, eg:
  DISPLAY=raspberrypi:0 xev -geometry 640x480

BONUS - Xfbdev
^^^^^^^^^^^^^^
Besides the PiTrex stuff, in getting XFree86 to compile for the
Raspberry Pi the regular X display via the framebuffer using the
fbdev driver also works. This includes the Xfbdev "TinyX" server,
which is a self-contained X server program optimised for smaller
size and lower system requirements. This is potentially useful for
faster start-up and running of normal X-based graphical systems
compared with Xorg.

After "make World", the Xfbdev binary can be found at:
programs/Xserver/Xfbdev

TinyX does not include XAA acceleration, so it's not useful for the
PiTrex driver. It also lacks GLX support, which is required by some
3D (OpenGL) applications.