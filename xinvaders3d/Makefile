BUILD_DIR_PURE := build.raspbian
BUILD_DIR := $(BUILD_DIR_PURE)/

PITREX_DIR := ../pitrex/pitrex/
VECTREX_DIR := ../pitrex/vectrex/

.PHONY: pitrexLib

all: game pitrex

CC = gcc
XLIB = -L/usr/X11R6/lib -I/usr/X11R6/include
LIBS = -lm
XOBJS = game.o aliens.o player.o effects.o \
object.o camera.o main-x11.o vec4x1.o mat4x4.o
PITREXOBJS = $(PITREX_DIR)$(BUILD_DIR)bcm2835.o \
$(PITREX_DIR)$(BUILD_DIR)pitrexio-gpio.o \
$(VECTREX_DIR)$(BUILD_DIR)vectrexInterface.o \
$(VECTREX_DIR)$(BUILD_DIR)osWrapper.o \
$(VECTREX_DIR)$(BUILD_DIR)baremetalUtil.o \
game.o aliens.o player.o effects.o \
object.o camera.o main-pitrex.o vec4x1.o mat4x4.o
DEPENDS = 
#CFLAGS = -g -Wall -pedantic -ansi
CFLAGS = -g -I../pitrex/ -DPIZERO -DRPI0 -Wall
#OPTIONS = -D_GNU_SOURCE -DGAME_LINUX_X11 -DGAME_DEBUG
OPTIONS = -DGAME_LINUX_X11

pitrexLib:
	$(MAKE) -C $(PITREX_DIR) -f Makefile.raspbian all
	$(MAKE) -C $(VECTREX_DIR) -f Makefile.raspbian all

.c.o: $(DEPENDS)
	$(CC) -c $(CFLAGS) $(OPTIONS) $< -o $@

game: $(XOBJS) $(DEPENDS)
	$(CC) $(CFLAGS) -o xinv3d $(XOBJS) $(XLIB) $(LIBS) -lX11 $(OPTIONS)

pitrex: $(PITREXOBJS) $(DEPENDS) pitrexLib
	$(CC) $(CFLAGS) -o xinv3d_pitrex $(PITREXOBJS) $(LIBS) $(OPTIONS)

clean:
	$(RM) *.o xinv3d xinv3d_pitrex

aliens.o: aliens.c game.h vec4x1.h mat4x4.h camera.h system.h defines.h \
 object.h externs.h
camera.o: camera.c mat4x4.h vec4x1.h camera.h
effects.o: effects.c game.h vec4x1.h mat4x4.h camera.h system.h \
 defines.h object.h externs.h
game.o: game.c game.h vec4x1.h mat4x4.h camera.h system.h defines.h \
 object.h externs.h
object.o: object.c mat4x4.h vec4x1.h defines.h object.h
main-x11.o: main-x11.c game.h vec4x1.h mat4x4.h camera.h system.h \
 defines.h object.h externs.h
main-pitrex.o: main-pitrex.c game.h vec4x1.h mat4x4.h camera.h system.h \
 defines.h object.h externs.h
mat4x4.o: mat4x4.c mat4x4.h vec4x1.h
player.o: player.c game.h vec4x1.h mat4x4.h camera.h system.h defines.h \
 object.h externs.h
vec4x1.o: vec4x1.c vec4x1.h
