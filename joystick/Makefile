# I prefer "handmade" make files, it is a little bit of more worktime to start with
# but I can read them, others can, and I know what they do!

BUILD_DIR_PURE := build.raspbian
BUILD_DIR := $(BUILD_DIR_PURE)/

PITREX_DIR := ../pitrex/pitrex/
VECTREX_DIR := ../pitrex/vectrex/
CFLAGS := -g -I../pitrex/ -DPIZERO -DRPI0
# CFLAGS := -Ofast -I../pitrex/ -DPIZERO -DRPI0
CC := gcc

.PHONY: pitrexLib
.PHONY: dirCheck


all:	dirCheck pitrexLib vecjoy vec2linux joyplug joydevmap sidewinderpro joytest linjoy
	echo All up to date

dirCheck:
	if [ ! -d $(BUILD_DIR_PURE) ]; then mkdir $(BUILD_DIR_PURE); fi

clean:
	$(RM) $(BUILD_DIR)*.* *~ vecjoy vec2linux joyplug joydevmap sidewinderpro joytest linjoy

pitrexLib:
	$(MAKE) -C $(PITREX_DIR) -f Makefile.raspbian all
	$(MAKE) -C $(VECTREX_DIR) -f Makefile.raspbian all

$(BUILD_DIR)vecjoy.o: vecjoy.c
	$(CC) $(CFLAGS) -o $(BUILD_DIR)vecjoy.o -c vecjoy.c

$(BUILD_DIR)linjoy.o: linjoy.c
	$(CC) $(CFLAGS) -o $(BUILD_DIR)linjoy.o -c linjoy.c

sidewinderpro: sidewinderpro.c
	$(CC) sidewinderpro.c -std=c99 `sdl2-config --cflags --libs` -o sidewinderpro

joytest: joytest.c
	$(CC) joytest.c -std=c99 `sdl2-config --cflags --libs` -o joytest

joyplug: joyplug.c
	$(CC) joyplug.c -std=c99 `sdl2-config --cflags --libs` -o joyplug

joydevmap: joydevmap.c
	$(CC) -o joydevmap joydevmap.c

vecjoy: $(BUILD_DIR)vecjoy.o pitrexLib
	$(RM) vecjoy
	$(CC) $(CFLAGS) -o vecjoy \
	$(PITREX_DIR)$(BUILD_DIR)bcm2835.o \
	$(PITREX_DIR)$(BUILD_DIR)pitrexio-gpio.o \
	$(VECTREX_DIR)$(BUILD_DIR)vectrexInterface.o \
	$(VECTREX_DIR)$(BUILD_DIR)osWrapper.o \
	$(VECTREX_DIR)$(BUILD_DIR)baremetalUtil.o \
	$(BUILD_DIR)vecjoy.o

linjoy: $(BUILD_DIR)linjoy.o pitrexLib
	$(RM) linjoy
	$(CC) $(CFLAGS) `sdl2-config --cflags --libs` -o linjoy \
	$(PITREX_DIR)$(BUILD_DIR)bcm2835.o \
	$(PITREX_DIR)$(BUILD_DIR)pitrexio-gpio.o \
	$(VECTREX_DIR)$(BUILD_DIR)vectrexInterface.o \
	$(VECTREX_DIR)$(BUILD_DIR)osWrapper.o \
	$(VECTREX_DIR)$(BUILD_DIR)baremetalUtil.o \
	$(BUILD_DIR)linjoy.o

$(BUILD_DIR)vec2linux.o: vec2linux.c
	$(CC) $(CFLAGS) -o $(BUILD_DIR)vec2linux.o -c vec2linux.c

vec2linux: $(BUILD_DIR)vec2linux.o \
	$(PITREX_DIR)$(BUILD_DIR)bcm2835.o \
	$(PITREX_DIR)$(BUILD_DIR)pitrexio-gpio.o \
	$(VECTREX_DIR)$(BUILD_DIR)vectrexInterface.o
	$(CC) $(CFLAGS) -o vec2linux $(BUILD_DIR)vec2linux.o $(PITREX_DIR)$(BUILD_DIR)bcm2835.o $(PITREX_DIR)$(BUILD_DIR)pitrexio-gpio.o $(VECTREX_DIR)$(BUILD_DIR)vectrexInterface.o

