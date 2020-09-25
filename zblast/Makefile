# makefile for zblast/xzb
#
# `make' makes only vga (PiTrex) version;
# `make vga' does vga only;
# `make x' does X only.
# Pitrex Mod: Built instead of "vga" target.

# NB: if you're on a Linux box with both svgalib and X installed (most
# Linux boxes are like this), you shouldn't need to edit this at all;
# just do `make' and then, as root, `make install'.

# You need an ANSI C compiler, e.g. gcc.
CC=gcc

#Pitrex Build environment:
BUILD_DIR_PURE := build.raspbian
BUILD_DIR := $(BUILD_DIR_PURE)/

PITREX_DIR := ../pitrex/pitrex/
VECTREX_DIR := ../pitrex/vectrex/

# optimisation and other CFLAGS stuff
# these settings are for gcc; try just "-O" if you're using some other
# compiler.
#
OPTOPT=-Ofast -I./lib -I../pitrex/ -DPIZERO -DRPI0 -DAVOID_TICKS -DDISABLE_FIQ
# -DWINDOW
# -DRTSCHED
# comment this out if you're not compiling on Linux.
#
MUSICOPT=-DMUSIC_SUPPORT

# same here. You should also comment it out if you don't want joystick
# support. :-)
#
#JOYSTICKOPT=-DJOYSTICK_SUPPORT

# this next option is only of concern on Linux - ignore it on
# non-Linux systems.
#
# On slower systems (e.g. 486s), you may need more sound buffering to
# try and avoid sound breaking up. However, this delays the sound a
# bit, which is suboptimal. Still, you may want to uncomment the
# following if you have a really old machine. :-)
#
#FRAGBUFOPT=-DMORE_SOUNDBUF

# set to where you have X installed (the X installation base dir).
# leave it as is if you're only compiling the vga version.
#
XROOT=/usr/X11R6

# change these if you want anything anywhere else.
# Note that the default puts some files in slightly old-fashioned
# places; you can uncomment the commented alternatives below to get a
# more `modern' setup, but you may have trouble with directories like
# /usr/share/games not existing.
#
PREFIX=/opt/pitrex

BINDIR=$(PREFIX)/bin
XBINDIR=$(BINDIR)
MANDIR=$(PREFIX)/man/man6
SOUNDSDIR=$(PREFIX)/share/zblast
SCORESFILE=$(SOUNDSDIR)/zblast.scores

# alternatively, you may prefer more FHS-friendly locations :-)
#
#BINDIR=$(PREFIX)/games
#XBINDIR=$(BINDIR)
#MANDIR=$(PREFIX)/share/man/man6
#SOUNDSDIR=$(PREFIX)/share/games/zblast
#SCORESFILE=/var/games/zblast.scores

# and if you want the X version to be installed in the usual X
# executables directory, uncomment this:
#
#XBINDIR=$(XROOT)/bin

# you shouldn't need to change anything below this point.
#--------------------------------------------------------------------

SOUNDOPT=-DSOUNDSDIR=\"$(SOUNDSDIR)\" $(MUSICOPT) $(JOYSTICKOPT) $(FRAGBUFOPT)

CFLAGS= $(OPTOPT) -I$(XROOT)/include -L$(XROOT)/lib $(SOUNDOPT) \
	-DSCORES_FILE=\"$(SCORESFILE)\"

.PHONY: pitrexLib

all : zblast

vga: zblast
x: xzb

ZBLASTOBJS = $(PITREX_DIR)$(BUILD_DIR)bcm2835.o \
	$(PITREX_DIR)$(BUILD_DIR)pitrexio-gpio.o \
	$(VECTREX_DIR)$(BUILD_DIR)vectrexInterface.o \
	svgalib-vectrex.o vectrextokeyboard.o intensitypalette.o \
	window.o \
	zblast.o font.o levels.o hiscore.o anim3d.o sod2.o grokfile.o joy.o

XZBOBJS = zblast-x.o font-x.o levels.o hiscore.o anim3d.o \
	sod2.o grokfile.o joy.o

pitrexLib:
	$(MAKE) -C $(PITREX_DIR) -f Makefile.raspbian all
	$(MAKE) -C $(VECTREX_DIR) -f Makefile.raspbian all


#bcm2835.o: ../pitrex/pitrex/bcm2835.c ../pitrex/pitrex/bcm2835.h
#	$(CC) $(CFLAGS) -c ../pitrex/pitrex/bcm2835.c -o bcm2835.o

#pitrexio-gpio.o: ../pitrex/pitrex/pitrexio-gpio.c ../pitrex/pitrex/pitrexio-gpio.h ../pitrex/pitrex/bcm2835.h
#	$(CC) $(CFLAGS) -c ../pitrex/pitrex/pitrexio-gpio.c -o pitrexio-gpio.o

# pitrex.03
#vectrexInterface.o: lib/vectrex/vectrexInterface.c lib/vectrex/vectrexInterface.h lib/vectrex/vectorFont.i
#	$(CC) $(CFLAGS) -c lib/vectrex/vectrexInterface.c -o vectrexInterface.o

#pitrex.10
#vectrexInterface.o: ../pitrex/vectrex/vectrexInterface.c ../pitrex/vectrex/vectrexInterface.h \
#	../pitrex/vectrex/commands.i ../pitrex/vectrex/rasterFont.i ../pitrex/vectrex/vectorFont.i
#	$(CC) $(CFLAGS) -c ../pitrex/vectrex/vectrexInterface.c -o vectrexInterface.o

window.o: pitrexLib lib/window/window.c lib/window/window.h
	$(CC) $(CFLAGS) -c lib/window/window.c -o window.o

vectrextokeyboard.o: lib/svgalib-vectrex/vectrextokeyboard.c lib/svgalib-vectrex/vectrextokeyboard.h \
	lib/svgalib-vectrex/vectrexcontrollermap.h
	$(CC) $(CFLAGS) -c lib/svgalib-vectrex/vectrextokeyboard.c -o vectrextokeyboard.o

intensitypalette.o: lib/svgalib-vectrex/intensitypalette.c \
	lib/svgalib-vectrex/intensitypalette.h
	$(CC) $(CFLAGS) -c lib/svgalib-vectrex/intensitypalette.c -o intensitypalette.o

svgalib-vectrex.o: pitrexLib lib/svgalib-vectrex/svgalib-vectrex.c lib/svgalib-vectrex/svgalib-vectrex.h \
	lib/svgalib-vectrex/vectrextokeyboard.h \
	lib/svgalib-vectrex/intensitypalette.h
	$(CC) $(CFLAGS) -c lib/svgalib-vectrex/svgalib-vectrex.c -o svgalib-vectrex.o

zblast-x.o: zblast.c
	$(CC) $(CFLAGS) -DUSE_X -c zblast.c -o zblast-x.o

font-x.o: font.c
	$(CC) $(CFLAGS) -DUSE_X -c font.c -o font-x.o

zblast: $(ZBLASTOBJS)
	$(CC) $(CFLAGS) -o zblast $(ZBLASTOBJS) -lm -l:libaoss.so

xzb: $(XZBOBJS)
	$(CC) $(CFLAGS) -o xzb $(XZBOBJS) -lX11 -lm

installdirs:
	/bin/sh ./mkinstalldirs $(BINDIR) $(XBINDIR) $(MANDIR) $(SOUNDSDIR)

# this also copies over any old (pre-1.2) "Scores" file installed in the
# old default location, if no new scores file exists yet.
install: installdirs
	if [ -f zblast ]; then \
	  install -o root -g games -m 6555 zblast $(BINDIR); fi
	if [ -f xzb ]; then \
	  install -g games -m 2555 xzb $(XBINDIR); fi
	install -m 444 zblast.6 $(MANDIR)
	ln -sf $(MANDIR)/zblast.6 $(MANDIR)/xzb.6
	chmod 555 $(SOUNDSDIR)
	install -m 444 sounds/* $(SOUNDSDIR)
	if [ -f /usr/games/lib/zblast/Scores -a \
	     ! -f $(SCORESFILE) ]; then \
	  cp /usr/games/lib/zblast/Scores $(SCORESFILE); \
	else \
	  touch $(SCORESFILE); \
	fi
	chown root:games $(SCORESFILE)
	chmod 664 $(SCORESFILE)

# deliberately *does not* remove old high scores; do that by hand
# if you're sure you want to do that.
uninstall:
	$(RM) $(BINDIR)/zblast $(XBINDIR)/xzb
	$(RM) $(MANDIR)/{zblast.6*,xzb.6*}
	for i in sounds/*; do $(RM) $(SOUNDSDIR)/`basename $$i`; done
	@echo 'NB: scores file $(SCORESFILE) left intact.'

clean:
	$(RM) zblast xzb *.o *~


# The stuff below makes the distribution tgz.

VERS=1.3

dist: tgz
tgz: ../zblast-$(VERS).tar.gz
  
../zblast-$(VERS).tar.gz: clean
	$(RM) ../zblast-$(VERS)
	@cd ..;ln -s zblast zblast-$(VERS)
	cd ..;tar zchvf zblast-$(VERS).tar.gz zblast-$(VERS)
	@cd ..;$(RM) zblast-$(VERS)
