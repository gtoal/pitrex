Most PiTrex libraries live in the "pitrex" directory. These include a
variety of libraries from other projects that are used for building
Bare-Metal binaries.

Compiling Options (can be defined in Makefile):

FREESTANDING - Bare-Metal build.
PIZERO -
RPI0 -
PITREX_DEBUG - Print debugging messages to the terminal (serial
               port (UART) in Bare-Metal).

PiTrex-specific libraries are:

PiTrexIO
========
Location: pitrex/pitrex
Include: <pitrex/pitrexio-gpio.h>
Description:
This provides the functions for controlling the PiTrex interface
cartridge, including reading and writing bytes from/to the VIA
in the Vectrex.

It is used by the Vectrex Interface library for controlling the
Vectrex display, sound, and controller I/O. Reads and writes are
equivalent to reading/writing the VIA in code running on the
Vectrex itself.

Compiling Options:

USE_EDR - This enables use of the BCM2835 Event Detection Register
          for detecting transition of the RDY signal from the PiTrex.
UART0 - Current versions of the pitrexio-gpio library automatically
        detect the Pi model that they are running on and configure
        the GPIO accordingly. If this fails, this causes it to
        default to using the UART0 configuration (suits the Pi Zero).
        Otherwise it will default to the UART1 configuration (suits
        the Pi Zero W).

Vectrex Interface
=================
Location: pitrex/vectrex
Include: <vectrex/vectrexInterface.h>
Description:
This is effectively as an advanced C equivalent to the Vectrex's BIOS
ROM routines, providing functions for drawing on the display, reading
the controller inputs, and playing sound.

This is the primary interface for games to access the Vectrex.

Compiling Options:

AVOID_TICKS - Enable intelligent avoidance of Linux timer interrupts
              during Vectrex I/O operations and associated timing
              delays. This is required (in combination with other
              measures) in order to avoid glitches on the Vectrex
              display. This is not required in Bare-Metal.
DISABLE_FIQ - This disables FIQ interrupts ("fast" interrupts) at the
              same time as others disabled with AVOID_TICKS. If USB
              is used, Linux (or at least current Raspberry pi OS)
              must be using the "dwc2" USB driver. This is not
              required in Bare-Metal.
RTSCHED - Set the process priority to "real-time", or as close as we
          can get to it, to avoid glitches. This can cause some
          problems, and currently isn't required when the previous
          two options are used. This is not required in Bare-Metal.

Consequences of, and need for, using those options may vary a lot
depending on the Linux kernel version used. The Makefile is set for
best compatibility with the current Raspberry Pi OS (when suitably
pre-configured, as described on the PiTrex Wiki).

SVGAlib-Vectrex
===============
location [game directory]/lib/svgalib-vectrex
Include: "svgalib-vectrex/svgalib-vectrex.h"
         "svgalib-vectrex/vectrextokeyboard.h"
Description:
This is a "translation library" to aid porting vector games written
for Linux using SVGAlib to run on the Vectrex via the PiTrex. See
the PiTrex Wiki for more details.

The files "intensitypalette.c" and "vectrextokeyboard.h" contain
game-specific configuration of the colour to intensity translation,
and the Vectrex controller button assignments to keyboard keys.

The version used in Xhyperoid has been hacked a bit to improve
the display. The version used by Zblast follows the original aim
of minimising the need to modify the original game code.
