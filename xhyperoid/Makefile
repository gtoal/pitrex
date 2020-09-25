# makefile for xhyperoid

# You need an ANSI C compiler, e.g. gcc.
CC=gcc

#Pitrex Build environment:
BUILD_DIR_PURE := build.raspbian
BUILD_DIR := $(BUILD_DIR_PURE)/

PITREX_DIR := ../pitrex/pitrex/
VECTREX_DIR := ../pitrex/vectrex/

CFLAGS=-Ofast -I./lib -I../pitrex/ -DUART0 -DPIZERO -DRPI0 -DAVOID_TICKS -DDISABLE_FIQ
# -DRTSCHED

# You'll need to comment this out on most non-Linux systems to disable
# the sound. Linuxers should leave it as-is.
#
CFLAGS+=-DDO_SOUND

# Where things are installed by `make install':
#
PREFIX=/opt/pitrex
BINDIR=$(PREFIX)/bin
XBINDIR=$(BINDIR)
MANDIR=$(PREFIX)/man/man6
SOUNDSDIR=$(PREFIX)/share/xhyperoid
#
# if you want the X version to be installed in the usual X executables
# directory, uncomment this:
#
#XBINDIR=/usr/X11R6/bin

# Alternatively, you may prefer these less traditional but more
# FHS-friendly locations. :-) (You would probably want to uncomment
# these when installing in /usr.)
#
#BINDIR=$(PREFIX)/games
#XBINDIR=$(BINDIR)
#MANDIR=$(PREFIX)/share/man/man6
#SOUNDSDIR=$(PREFIX)/share/games/xhyperoid

# you shouldn't need to change anything below this point.
#--------------------------------------------------------------------

CFLAGS+=-DSOUNDSDIR=\"$(SOUNDSDIR)\"

.PHONY: pitrexLib

OBJS = $(PITREX_DIR)$(BUILD_DIR)bcm2835.o \
	$(PITREX_DIR)$(BUILD_DIR)pitrexio-gpio.o \
	$(VECTREX_DIR)$(BUILD_DIR)vectrexInterface.o \
	svgalib-vectrex.o vectrextokeyboard.o intensitypalette.o \
	hyperoid.o roidsupp.o sound.o svga.o window.o

XOBJS = hyperoid.o roidsupp.o sound.o gtk.o

all: vga

x: xhyperoid

vga: vhyperoid

pitrexLib:
	$(MAKE) -C $(PITREX_DIR) -f Makefile.raspbian all
	$(MAKE) -C $(VECTREX_DIR) -f Makefile.raspbian all

#bcm2835.o: lib/pitrex/bcm2835.c lib/pitrex/bcm2835.h
#	$(CC) $(CFLAGS) -c lib/pitrex/bcm2835.c -o bcm2835.o

#pitrexio-gpio.o: lib/pitrex/pitrexio-gpio.c lib/pitrex/pitrexio-gpio.h lib/pitrex/bcm2835.h
#	$(CC) $(CFLAGS) -c lib/pitrex/pitrexio-gpio.c -o pitrexio-gpio.o

#pitrex.10
#vectrexInterface.o: lib/vectrex/vectrexInterface.c lib/vectrex/vectrexInterface.h \
#	lib/vectrex/commands.i lib/vectrex/rasterFont.i lib/vectrex/vectorFont.i
#	$(CC) $(CFLAGS) -c lib/vectrex/vectrexInterface.c -o vectrexInterface.o

window.o: window.c
	$(CC) $(CFLAGS) -c window.c -o window.o

vectrextokeyboard.o: lib/svgalib-vectrex/vectrextokeyboard.c lib/svgalib-vectrex/vectrextokeyboard.h \
	lib/svgalib-vectrex/vectrexcontrollermap.h
	$(CC) $(CFLAGS) -c lib/svgalib-vectrex/vectrextokeyboard.c -o vectrextokeyboard.o

intensitypalette.o: lib/svgalib-vectrex/intensitypalette.c \
	lib/svgalib-vectrex/intensitypalette.h
	$(CC) $(CFLAGS) -c lib/svgalib-vectrex/intensitypalette.c -o intensitypalette.o

svgalib-vectrex.o: pitrexLib lib/svgalib-vectrex/svgalib-vectrex.c lib/svgalib-vectrex/svgalib-vectrex.h \
	lib/svgalib-vectrex/vectrextokeyboard.h lib/svgalib-vectrex/intensitypalette.h
	$(CC) $(CFLAGS) -c lib/svgalib-vectrex/svgalib-vectrex.c -o svgalib-vectrex.o

vhyperoid: $(OBJS)
	$(CC) -o vhyperoid $(OBJS) -l:libaoss.so -lm

xhyperoid: $(XOBJS)
	$(CC) -o xhyperoid $(XOBJS) `gtk-config --libs`

gtk.o: gtk.c
	$(CC) $(CFLAGS) `gtk-config --cflags` -c gtk.c -o gtk.o

installdirs:
	/bin/sh ./mkinstalldirs $(BINDIR) $(XBINDIR) $(MANDIR) $(SOUNDSDIR)

# check at least one executable has been made
check-made:
	@if [ ! -f vhyperoid -a ! -f xhyperoid ]; then \
	  echo 'do "make vga", "make x", or "make" first (see README).'; \
	  false; \
	fi

install: check-made installdirs
	if [ -f vhyperoid ]; then \
	install -o root -g games -m 4511 vhyperoid $(BINDIR); fi
	if [ -f xhyperoid ]; then \
	install -m 511 xhyperoid $(XBINDIR); fi
	install -m 444 xhyperoid.6 $(MANDIR)
	ln -f $(MANDIR)/xhyperoid.6 $(MANDIR)/vhyperoid.6
	chmod 555 $(SOUNDSDIR)
	install -m 444 sounds/*.ub $(SOUNDSDIR)

# I delete the sounds a slightly odd way to save having "rm -fr",
# which I'd prefer to avoid just in case SOUNDSDIR is bogus. :-)
uninstall:
	$(RM) $(BINDIR)/vhyperoid $(XBINDIR)/xhyperoid
	$(RM) $(MANDIR)/hyperoid.6* $(MANDIR)/[vx]hyperoid.6*
	$(RM) $(SOUNDSDIR)/*
	rmdir $(SOUNDSDIR)

clean:
	$(RM) vhyperoid xhyperoid *.o *~


# The stuff below makes the distribution tgz.
#
VERS=1.2

dist: tgz
tgz: ../xhyperoid-$(VERS).tar.gz
  
# Based on the example in ESR's Software Release Practice HOWTO.
#
../xhyperoid-$(VERS).tar.gz: clean
	$(RM) ../xhyperoid-$(VERS)
	@cd ..;ln -s xhyperoid xhyperoid-$(VERS)
	cd ..;tar zchvf xhyperoid-$(VERS).tar.gz xhyperoid-$(VERS)
	@cd ..;$(RM) xhyperoid-$(VERS)
