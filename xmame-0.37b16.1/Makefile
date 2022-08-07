#########################################################################
#                                                                       #
# makefile for xmame/xmess                                              #
#                                                                       #
# original by Juan Antonio Martinez                                     #
# current version by Hans de Goede                                      #
# http://x.mame.net                                                     #
# mailto:hans@highrise.nl                                               #
#                                                                       #
#									#
# READ COMMENTS AND MODIFY CONFIGURABLE OPTIONS WITH CARE...		#
#									#
# Members of Mame and X-Mame project assumes no responsability  	#
# implicit or explicit about illegal or incorrect use of this   	#
# sofware and about possible hard/soft damage derived of it   	  	#
#									#
# THIS SOFTWARE IS DISTRIBUTED "AS IS" . USE AT YOUR OWN RISK		#
#									#
# Mame and X-Mame are Copyright of Nicola Salmoria and Mirko buffoni	#
# and distributed under terms simular to the GNU General Public License #
# see doc/readme.unix for more details                                  #
#									#
# Arcade ROM images are copyright of their authors. DISTRIBUTING MAME   #
# OR X-MAME INCLUDING ROM IMAGES IN THE SAME PACKAGE IS ILLEGAL AND	#
# CONSTITUTES A VIOLATION OF TERMS OF COPYRIGHT				#
#									#
#########################################################################

##############################################################################
# xmame or xmess ?
##############################################################################

#set TARGET to "mame" (without the ") to compile xmame and to "mess" to
#compile xmess.
TARGET = mame
MAME_VERSION = 37b16


##############################################################################
# *** development environment options 
##############################################################################

# GNU MAKE is MANDATORY !!!

# *** Choose your compiler
# Use gcc if available
#CC	= arm-linux-gnueabihf-gcc
CC	= gcc
# otherwise try the default cc
# CC    = cc
# Use of `c89' is recommend for ultrix as it generates faster code (which
# means less frames to be skipped and better graphics) but `gcc' works just
# as well. However, stay away from the `cc' ultrix compiler if possible.
# CC	= c89

# *** Choose your compiler flags.
# Note 1:
# xmame compilation breaks when using egcs-1.1.x or gcc-2.95; use
# -fno-strict-aliasing to fix it.  gcc-2.95.1 does work with -fstrict-aliasing,
# and gives a nice speed increase.
#
# Note 2:
# linux-powerpc needs -fsigned-char
#
# Note 3:
# OpenStep needs -traditional-cpp
#
# Note 4:
# It has been reported that adding -pipe to the CFLAGS when using gcc will speed
# up build times a lot and reduce disk activity greatly.  Thanks to Pete French
# for the tip!
#
# Note 5:
# Pete French has reported that gcc can sometimes produce incorrect code for
# optimization levels above -O.  If you run into problems using -O2 or higher,
# try using -O instead. 
#
### for normal optimization, full warnings
# CFLAGS    = -O -Wall
### for common optimizations, full warnings except unused vars
# CFLAGS    = -O2 -Wall -Wno-unused
### this should work with all compilers 
# CFLAGS    =
### to get full optimization under gcc/x Intel based OS's..
# CFLAGS    = -O3 -m486 -Wall -Wno-unused -funroll-loops \
#  -fstrength-reduce -fomit-frame-pointer -ffast-math -malign-functions=2 \
#  -malign-jumps=2 -malign-loops=2
### for Linux/PowerPC use following opts
# CFLAGS    = -O -Wall  -Wno-unused -funroll-loops\
#  -fstrength-reduce -fomit-frame-pointer -ffast-math -fsigned-char
### for OpenStep/Intel use following opts
# CFLAGS    = -O -Wall -Wno-unused -finline-functions -ffast-math \
#  -fstrength-reduce -traditional-cpp
### for OpenStep/PPC use following opts
# CFLAGS    = -O -Wall  -Wno-unused -funroll-loops -traditional-cpp \
#  -fstrength-reduce -fomit-frame-pointer -ffast-math -fsigned-char
### for IRIX MIPSpro with real serious optimization for R10K O2
# CFLAGS = -fullwarn -n32 -mips4 -Ofast=ip32_10k -TARG:platform=ip32_10k \
#  -OPT:Olimit=0 -IPA
### for IRIX with more general optimization for R5+K Mips machines
# CFLAGS = -fullwarn -n32 -mips4 -Ofast -OPT:Olimit=0 -IPA
### for IRIX with R4K Mips chips (older Indys, Indigo2s, etc).
# CFLAGS = -fullwarn -n32 -mips3 -Ofast -OPT:Olimit=0 -IPA
### used by me, for full ansi testing and debugging (with gcc).
# CFLAGS    = -ggdb -ansi -pedantic -D_XOPEN_SOURCE -D_BSD_SOURCE -Wall \
#  -Wno-long-long -Wno-trigraphs -Wp,-Wno-paste \
#  -Dasm=__asm__ -O6 -march=pentiumpro -fstrength-reduce \
#  -ffast-math -malign-functions=2 -malign-jumps=2 -malign-loops=2

# Raspberry Pi Linux
# 
# RASPI: improvements or changes required for compatibility with rapberry pi
# ENABLE_AUTOFIRE: include the autofire option
# SPEEDHACK: include the speedhack option
# VERSION: used for some config paths
# 
# Two display method versions exist:
#
# SDL: Standard version with no hardware color palette or scaling
# Dispmanx: Hardware scaling

CFLAGS = -pipe -g \
		-Ofast -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -mstructure-size-boundary=32 \
        -ffast-math -fsingle-precision-constant \
        -fomit-frame-pointer -fno-builtin -fno-exceptions -fno-common \
        -fstrict-aliasing  -fmerge-all-constants  \
        -finline -fweb -frename-registers -falign-functions=16 \
        -DRASPI -DSPEEDHACK -DUSE_8BIT

# *** Does your compiler supports inline funcs?. use appropiate item:
# do not use IL = inline : wont work due to Z80 and 6809 stuffs
# if strict ansi used set IL to "static"
# This can very noticably increase xmame's performance, so enabling this is
# encouraged, it works fine with gcc and probably with most other compilers.
IL	= '-DINLINE=static __inline__'
# IL     = -DINLINE=static

# *** Choose your linker & linker flags (some OSes need native linker
# instead GNU one)
# normal
LD	= $(CC) -Wl,-s
# to profile with gcc and gprof
# LD	= $(CC) -pg
# no profiling (less strace output), but with debug info
# LD	= $(CC)
# for debugging only, only works with gnu-binutils !!
# LD	= $(CC) -Wl,-warn-common
# for IRIX R5+K MIPSpro, optimization at link time, takes nearly a gig of 
# memory!!
# LD    = $(CC) -fullwarn -n32 -mips4 -IPA
# for IRIX R4K MIPSpro, optimization at link time, takes nearly a gig of
# memory!!
# LD  = $(CC) -fullwarn -n32 -mips3 -IP

# *** Does your platform have a separate libm ? only comment this out if you
# have maths libraries included within libc. This is very rare, you will
# probably only need it for OpenStep systems
SEPARATE_LIBM = 1

# *** Does your platform have ranlib (leave this untouched unless you are
# using irix or Unixware 7
RANLIB = ranlib
# RANLIB = true

# *** How to install executable, man pages, and data?
# OSes that don't have install command should use provided install.sh script
# INSTALL =            install-sh
# INSTALL =            /bin/install
INSTALL =              /usr/bin/install
INSTALL_PROGRAM_DIR =  $(INSTALL) -d    -o root -g bin -m  755
INSTALL_MAN_DIR =      $(INSTALL) -d    -o root -g 0   -m  755
INSTALL_DATA_DIR =     $(INSTALL) -d    -o root -g bin -m  755
INSTALL_PROGRAM =      $(INSTALL) -c -s -o root -g bin -m  555
INSTALL_PROGRAM_SUID = $(INSTALL) -c -s -o root -g bin -m 4555
INSTALL_MAN =          $(INSTALL) -c    -o root -g bin -m  444
INSTALL_DATA =         $(INSTALL) -c    -o root -g bin -m  644


# *** Extra include and or library paths, sometimes needed for zlib,
# with suse linux -L/usr/X11R6/lib is needed for example,
# and with unixware7 -lsocket is needed
# LIBS     = -L/usr/X11R6/lib -L/usr/local/lib
# INCLUDES = -I/usr/X11R6/include -I/usr/local/include

# *** If your system doesn't have zlib, or your systems zlib is giving you
# troubles, you can use the one distributed with xmame.
# Uncomment the following line to use xmame's own version of zlib
ZLIB = 1

# *** Does your system support gettimeofday() function? If so, we encourage
# you to enable this feature . Else .... comment it :-(
HAVE_GETTIMEOFDAY = 1

# *** Uncomment this if you want to link with ElectricFence developers only !
# EFENCE = 1


##############################################################################
# *** Configuration files and directories
##############################################################################

# *** Select destination directory for your compiled program, manual page
# and binary distribution ( if you want to... )
# ( only needed to install, not to compile... )
ifndef PREFIX
PREFIX = /usr/local
endif
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/man/man6

# *** xmameroot, this is the default rompath, place where the highscores are
# stored, global rc location etc. Since most of these are configurable through
# the rc-file anyway, there are no longer seperate defines for each.
XMAMEROOT = $(PREFIX)/share/x$(TARGET)37b16


##############################################################################
# *** architecture, Choose your CPU (!!only one!!) 
##############################################################################

### i386 + gnu-asm
# MY_CPU = i386
### i386 no asm
# MY_CPU = i386_noasm
### ia64
# MY_CPU = ia64
### alpha
# MY_CPU = alpha
### m68k
# MY_CPU = m68k
### generic risc (powerpc, sparc, hppa, ibm)
# MY_CPU = risc
### generic risc, lsb-first (RISC (ultrix machines))
MY_CPU = risc_lsb
### mips (generic risc + SGI compiler bug workarounds)
# MY_CPU = mips

##############################################################################
# *** architecture, Choose your OS (!!only one!!) 
##############################################################################

### Linux 
ARCH  = linux
### FreeBSD
# ARCH  = freebsd
### NetBSD
# ARCH  = netbsd
### OpenBSD
# ARCH = openbsd
### Solaris / SunOS
# ARCH  = solaris
### QNX Neutrino (QNX4\QNX6)
# ARCH = nto
### OpenStep on NeXT systems
# ARCH  = next
### OpenStep on Apple systems (Cocoa)
# ARCH  = macosx
### IRIX ( with sound requires the dmedia package ) (*)
# ARCH  = irix
### IRIX ( with sound using the new al package) (*)
# ARCH  = irix_al
### aix ( with sound, you'll need the UMS and SOM lpp's installed ( under AIX4 ))
# ARCH  = aix
### generic unix, no sound
# ARCH  = generic

# *) For IRIX 6.5 or higher add -DHAVE_SNPRINTF to CFLAGS.irix(_al) in
# src/unix/Makefile


##############################################################################
# *** Input Devices
##############################################################################

# *** uncomments any joystick types which you want to use, which one is
# actually used can be selected runtime with the -joytype switch.
# X Input Extensions based joystick, this is known to not work right now ;|
# JOY_X11 = 1
# On iX86 based OS's, if supported, you can use standard joystick driver.
JOY_I386 = 1
# Linux FM-TOWNS game PAD joystick emulation support, thanks to Osamu Kurati.
# JOY_PAD = 1
# NetBSD/FreeBSD USB joystick support.
#JOY_USB = 1
# SDL joystick support.
#JOY_SDL = 1


##############################################################################
# *** Sound Devices
##############################################################################

# *** any additonal sound drivers you want to include, besided the native one
# sound driver for your system
# SOUND_ESOUND = 1
SOUND_ALSA = 1
# SOUND_ARTS_TEIRA = 1
# SOUND_ARTS_SMOTEK = 1
# SOUND_SDL = 1


##############################################################################
# *** Select your display method, choose only one
# note: x11 is the only one supported on all platforms
##############################################################################

### X11
DISPLAY_METHOD = x11
### svgalib, only supported under linux
# DISPLAY_METHOD = svgalib
### ggi, only tested under linux
# DISPLAY_METHOD = ggi
### OpenGL under X11
# DISPLAY_METHOD = xgl
### glide under X11
# DISPLAY_METHOD = xfx
### glide in console mode
# DISPLAY_METHOD = svgafx
### OpenStep bitmaps
# DISPLAY_METHOD = openstep
### SDL library
#DISPLAY_METHOD = SDL
### Photon 2.x (QNX6), currently buggy but working...                
# DISPLAY_METHOD = photon2
### SDL + dispmanx
#DISPLAY_METHOD = dispmanx


##############################################################################
# *** X windows options, only needed for X as display method
##############################################################################

# *** uncomment any extensions you wish to use.
# MIT-Shared Memory X Extensions, comment the
# X11_MITSHM = 1
# XFree86 DGA
# X11_DGA = 1
# The XIL library (Solaris 2.5.1 and higher), uncomment
# X11_XIL = 1

# *** Choice the location of your X headers & libs
# standard location for X11 
# X11INC		= -I/usr/include/X11
# X11LIB		= -L/usr/lib/X11
# standard location for XFree86
X11INC		= -I/usr/X11R6/include
X11LIB		= -L/usr/X11R6/lib
# for Sun systems
# X11INC		= -I/usr/openwin/include
# X11LIB		= -L/usr/openwin/lib
# for non-standard locations
# X11INC		= -I/usr/local/include/X11
# X11LIB		= -L/usr/local/lib

# *** The default name for the input device used by the X11 joystick driver.
# use provided program "xlistdev" to list available ones
# X11_JOYNAME = "Joystick"

##############################################################################
# SDL options (only needed when SDL is the display method)
##############################################################################

# The name of your sdl-config executable
SDL_CONFIG      = sdl-config


##############################################################################
# *** OpenGL info -- configure if you are using the xgl display option
##############################################################################
#GLLIBS= -ldl

#
# General dynamical loading OpenGL (GL/GLU) support for:
#
# <OS - System>          <#define>  commentary
# -----------------------------------------------
# GNU/Linux, Unices/X11  _X11_      (loads glx also)
# Macinstosh OS9         _MAC_OS9_
# Macinstosh OSX         _MAC_OSX_
# Win32                  _WIN32_    (load wgl also)
#
# GLU version 1.2 is recommended (e.g., from SGI).  If you 
# use Mesa's GLU, you may have to add -DGLU_VERSION_1_2 to 
# the GLCFLAGS below.
#
#GLCFLAGS= -D_X11_

##############################################################################
# *** Special features
##############################################################################

# *** Uncommenty the next lines to build xmame with build in debugger, only
# supported for x11 and svgalib. Only enable this if you need it, if you
# want to develop drivers that is, since it slows down xmame considerably.
# MAME_DEBUG = 1

# *** Uncomment the next lines to use x86-asm cpu cores. This is only
# supported for some 386 machines with nasm-0.98 or higher available in your
# path! the asm 68k core could cause problem with certain games, please test
# with the c-core before reporting any problems.
# uncomment next line to use Assembler 68000 engine
# X86_ASM_68000 = 1
# uncomment next line to use Assembler 68020 engine
# X86_ASM_68020 = 1
#
# And choose elf / or aout
# ELF settings: no leading underscores + elf object format Tested with
# linux/i386. Also for e.g. Solaris/x86
ASM_STRIP = contrib/porting/strip_
NASM_FMT  = -f elf
# aout settings: with leading underscores + aout object format. Tested with
# netBSD i386. Also for older linux installs, freebsd and others.
# ASM_STRIP = true  # no strip command
# NASM_FMT  = -f aoutb

ifdef MAME_DEBUG
NAME=x$(TARGET)d
else
NAME=x$(TARGET)
endif
MAP=-Wl,-Map=x$(TARGET).map

##############################################################################
# All done, type make -f makefile.unix and enjoy xmame/xmess ;)
# No configurable options below this line....
##############################################################################
include src/unix/unix.mak
