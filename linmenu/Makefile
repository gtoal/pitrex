# I prefer "handmade" make files, it is a little bit of more worktime to start with
# but I can read them, others can, and I know what they do!
#

BUILD_DIR_PURE := build.raspbian
BUILD_DIR := $(BUILD_DIR_PURE)/

PITREX_DIR := ../pitrex/pitrex/
VECTREX_DIR := ../pitrex/vectrex/
CFLAGS := -Ofast -I../pitrex/ -DPIZERO -DRPI0
CC := gcc

.PHONY: pitrexLib
.PHONY: dirCheck

all:	dirCheck pitrexLib linmenu

dirCheck:
	if [ ! -d $(BUILD_DIR_PURE) ]; then mkdir $(BUILD_DIR_PURE); fi

pitrexLib: dirCheck
	$(MAKE) -C $(PITREX_DIR) -f Makefile.raspbian all
	$(MAKE) -C $(VECTREX_DIR) -f Makefile.raspbian all

$(BUILD_DIR)linmenu.o: linmenu.c
	$(CC) $(CFLAGS) -o $(BUILD_DIR)linmenu.o -c linmenu.c

linmenu: $(BUILD_DIR)linmenu.o pitrexLib 
	$(RM) linmenu
	$(CC) $(CFLAGS) -o linmenu \
	$(PITREX_DIR)$(BUILD_DIR)bcm2835.o \
	$(PITREX_DIR)$(BUILD_DIR)pitrexio-gpio.o \
	$(VECTREX_DIR)$(BUILD_DIR)baremetalUtil.o \
	$(VECTREX_DIR)$(BUILD_DIR)osWrapper.o \
	$(VECTREX_DIR)$(BUILD_DIR)vectrexInterface.o \
	$(BUILD_DIR)linmenu.o -lm

clean:
	$(RM) $(BUILD_DIR)*.*
	$(RM) linmenu

