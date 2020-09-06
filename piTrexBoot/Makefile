# -DLOADER_START=0x4000000
# must also be defined/changed in *.S files, since they do not parse defines
# REMEMBER in bcm2835
# MEM_COHERENT_REGION s defined now as 0x3f00000 !!!!

BUILD_DIR_PURE := build.baremetal
BUILD_DIR := $(BUILD_DIR_PURE)/

LIB_DIR :=../pitrex/baremetal
INC_DIR_PITREX := ../pitrex
INC_DIR_LIBRARY := $(LIB_DIR)/lib2835
SIM_DIR := ../sim/
PITREX_DIR := ../pitrex/pitrex/
VECTERX_DIR := ../pitrex/vectrex/
LOADER_DIR := ../piTrexBoot/

CFLAGS := -fuse-ld=bfd -Ofast -I$(INC_DIR_PITREX) -I$(INC_DIR_LIBRARY) -L$(LIB_DIR) -mfloat-abi=hard -nostartfiles -mfpu=vfp -march=armv6zk -mtune=arm1176jzf-s -DRPI0 -DFREESTANDING -DPITREX_DEBUG -DMHZ1000 -DLOADER_START=0x4000000
CC	= arm-none-eabi-gcc
AS	= $(CC)
AR	= arm-none-eabi-ar

.PHONY: dirCheck
.PHONY: pitrexLib loaderObjects

all:	dirCheck pitrexLib loaderObjects pitrex.img loader.pit

dirCheck:
	if [ ! -d $(BUILD_DIR_PURE) ]; then mkdir $(BUILD_DIR_PURE); fi

pitrexLib: dirCheck
	$(MAKE) -C $(PITREX_DIR) -f Makefile.baremetal all
	$(MAKE) -C $(VECTERX_DIR) -f Makefile.baremetal all

loaderObjects: dirCheck $(BUILD_DIR)baremetalEntry.o \
	$(BUILD_DIR)bareMetalMain.o \
	$(BUILD_DIR)cstubs.o \
	$(BUILD_DIR)rpi-armtimer.o \
	$(BUILD_DIR)rpi-aux.o \
	$(BUILD_DIR)rpi-gpio.o \
	$(BUILD_DIR)rpi-interrupts.o \
	$(BUILD_DIR)rpi-systimer.o \
	$(BUILD_DIR)bootStrapMain.o \
	$(BUILD_DIR)loaderEntry.o \
	$(BUILD_DIR)loaderMain.o

$(BUILD_DIR)baremetalEntry.o: $(LIB_DIR)/baremetalEntry.S
	$(AS) $(CFLAGS) -D__ASSEMBLY__ -c $(LIB_DIR)/baremetalEntry.S -o $(BUILD_DIR)baremetalEntry.o

$(BUILD_DIR)loaderEntry.o: loaderEntry.S
	$(AS) $(CFLAGS) -D__ASSEMBLY__ -c loaderEntry.S -o $(BUILD_DIR)loaderEntry.o

$(BUILD_DIR)bareMetalMain.o: $(LIB_DIR)/bareMetalMain.c $(LIB_DIR)/rpi-aux.h $(LIB_DIR)/rpi-gpio.h $(LIB_DIR)/rpi-armtimer.h $(LIB_DIR)/rpi-systimer.h $(LIB_DIR)/rpi-interrupts.h
	$(AS) $(CFLAGS) -D__ASSEMBLY__ -c $(LIB_DIR)/bareMetalMain.c -o $(BUILD_DIR)bareMetalMain.o

$(BUILD_DIR)loaderMain.o: loaderMain.c
	$(AS) $(CFLAGS) -D__ASSEMBLY__ -c loaderMain.c -o $(BUILD_DIR)loaderMain.o

$(BUILD_DIR)rpi-armtimer.o: $(LIB_DIR)/rpi-armtimer.c $(LIB_DIR)/rpi-armtimer.h $(LIB_DIR)/rpi-base.h
	$(CC) $(CFLAGS) -o $(BUILD_DIR)rpi-armtimer.o -c $(LIB_DIR)/rpi-armtimer.c

$(BUILD_DIR)rpi-aux.o: $(LIB_DIR)/rpi-aux.c $(LIB_DIR)/rpi-aux.h $(LIB_DIR)/rpi-base.h
	$(CC) $(CFLAGS) -o $(BUILD_DIR)rpi-aux.o -c $(LIB_DIR)/rpi-aux.c

$(BUILD_DIR)rpi-gpio.o: $(LIB_DIR)/rpi-gpio.c $(LIB_DIR)/rpi-gpio.h $(LIB_DIR)/rpi-base.h
	$(CC) $(CFLAGS) -o $(BUILD_DIR)rpi-gpio.o -c $(LIB_DIR)/rpi-gpio.c

$(BUILD_DIR)rpi-interrupts.o: $(LIB_DIR)/rpi-interrupts.c $(LIB_DIR)/rpi-interrupts.h $(LIB_DIR)/rpi-base.h
	$(CC) $(CFLAGS) -o $(BUILD_DIR)rpi-interrupts.o -c $(LIB_DIR)/rpi-interrupts.c

$(BUILD_DIR)rpi-systimer.o: $(LIB_DIR)/rpi-systimer.c $(LIB_DIR)/rpi-systimer.h $(LIB_DIR)/rpi-base.h
	$(CC) $(CFLAGS) -o $(BUILD_DIR)rpi-systimer.o -c $(LIB_DIR)/rpi-systimer.c

$(BUILD_DIR)cstubs.o: $(LIB_DIR)/cstubs.c
	$(CC) $(CFLAGS) -o $(BUILD_DIR)cstubs.o -c $(LIB_DIR)/cstubs.c

$(BUILD_DIR)bootStrapMain.o: bootStrapMain.c
	$(CC) $(CFLAGS) -o $(BUILD_DIR)bootStrapMain.o -c bootStrapMain.c

pitrex.img: pitrexLib loaderObjects $(PITREX_DIR)$(BUILD_DIR)bcm2835.o $(PITREX_DIR)$(BUILD_DIR)pitrexio-gpio.o $(VECTERX_DIR)$(BUILD_DIR)baremetalUtil.o $(VECTERX_DIR)$(BUILD_DIR)vectrexInterface.o 
	$(CC) $(CFLAGS) -o $(BUILD_DIR)pitrex.elf.img \
	$(BUILD_DIR)baremetalEntry.o \
	$(BUILD_DIR)bareMetalMain.o \
	$(BUILD_DIR)cstubs.o \
	$(BUILD_DIR)rpi-armtimer.o \
	$(BUILD_DIR)rpi-aux.o \
	$(BUILD_DIR)rpi-gpio.o \
	$(BUILD_DIR)rpi-interrupts.o \
	$(BUILD_DIR)rpi-systimer.o \
	$(PITREX_DIR)$(BUILD_DIR)bcm2835.o \
	$(PITREX_DIR)$(BUILD_DIR)pitrexio-gpio.o \
	$(VECTERX_DIR)$(BUILD_DIR)baremetalUtil.o \
	$(VECTERX_DIR)$(BUILD_DIR)osWrapper.o \
	$(VECTERX_DIR)$(BUILD_DIR)vectrexInterface.o \
	$(BUILD_DIR)bootStrapMain.o -lff12c  -ldebug  -lhal -lutils -lconsole -lff12c -lbob -li2c -lbcm2835 -larm
	$(RM) pitrex.img
	arm-none-eabi-objcopy $(BUILD_DIR)pitrex.elf.img -O binary pitrex.img
ifdef MAKE_IMAGE
	cp pitrex.img /media/psf/Home/ubuntu64
	cp pitrex.img /media/psf/macRoot/Volumes/baremetal
endif

loader.pit: pitrexLib loaderObjects 
	$(CC) $(CFLAGS) -o $(BUILD_DIR)loader.elf.img \
	$(BUILD_DIR)loaderEntry.o \
	$(BUILD_DIR)cstubs.o \
	$(BUILD_DIR)rpi-armtimer.o \
	$(BUILD_DIR)rpi-aux.o \
	$(BUILD_DIR)rpi-gpio.o \
	$(BUILD_DIR)rpi-interrupts.o \
	$(BUILD_DIR)rpi-systimer.o \
	$(PITREX_DIR)$(BUILD_DIR)bcm2835.o \
	$(PITREX_DIR)$(BUILD_DIR)pitrexio-gpio.o \
	$(VECTERX_DIR)$(BUILD_DIR)baremetalUtil.o \
	$(VECTERX_DIR)$(BUILD_DIR)osWrapper.o \
	$(VECTERX_DIR)$(BUILD_DIR)vectrexInterface.o \
	$(BUILD_DIR)loaderMain.o -lff12c  -ldebug  -lhal -lutils -lconsole -lff12c -lbob -li2c -lbcm2835 -larm
	$(RM) loader.pit
	arm-none-eabi-objcopy $(BUILD_DIR)loader.elf.img -O binary loader.img
	arm-none-eabi-objdump -l -S -D $(BUILD_DIR)loader.elf.img > $(BUILD_DIR)loader.elf.img.s
	dd if=loader.img of=loader.pit ibs=1 skip=67076096
	rm loader.img
ifdef MAKE_IMAGE
	cp loader.pit /media/psf/Home/ubuntu64
	cp loader.pit /media/psf/macRoot/Volumes/baremetal
endif

clean:
	$(RM) $(BUILD_DIR)*.*
	$(RM) pitrex.img
	$(RM) loader.img
	$(RM) loader.pit

