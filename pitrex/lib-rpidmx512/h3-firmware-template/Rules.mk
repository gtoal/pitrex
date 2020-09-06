PREFIX ?= arm-none-eabi-

CC	= $(PREFIX)gcc
CPP	= $(PREFIX)g++
AS	= $(CC)
LD	= $(PREFIX)ld
AR	= $(PREFIX)ar

PLATFORM?=ORANGE_PI
CONSOLE?=

SUFFIX=orangepi_zero

ifeq ($(findstring ORANGE_PI_ONE,$(PLATFORM)),ORANGE_PI_ONE)
	SUFFIX=orangepi_one
endif

COND=
ifeq ($(findstring ORANGE_PI,$(PLATFORM)),ORANGE_PI)
	COND=1
endif

ifeq ($(findstring ENABLE_SPIFLASH,$(DEFINES)),ENABLE_SPIFLASH)
	COND=1
endif

ifeq ($(findstring NO_EMAC,$(DEFINES)),NO_EMAC)
else
	ifdef COND
		LIBS:=remoteconfig tftpdeamon $(LIBS)
	endif
endif

ifdef COND
	LIBS+=artnet4 artnet e131 dmxsend dmx ws28xxdmx ws28xx tlc59711dmx tlc59711 spiflashinstall spiflashstore spiflash
endif

LIBS+=network

ifeq ($(findstring ESP8266,$(DEFINES)),ESP8266)
	LIBS+=esp8266
endif

# Output 
TARGET = $(SUFFIX).img
LIST = $(SUFFIX).list
MAP = $(SUFFIX).map
BUILD=build_h3/

# Input
SOURCE = ./
FIRMWARE_DIR = ./../h3-firmware-template/
LINKER = $(FIRMWARE_DIR)memmap

LIBS+=lightset ledblink properties display c++ hal bob i2c utils console ff12c h3 debug arm

DEFINES:=$(addprefix -D,$(DEFINES))

ifneq ($(CONSOLE),)
	DEFINES+=-D$(CONSOLE)
endif

# The variable for the firmware include directories
INCDIRS=../include $(wildcard ./include) $(wildcard ./*/include)
INCDIRS:=$(addprefix -I,$(INCDIRS))

# The variable for the libraries include directory
LIBINCDIRS=$(addprefix -I../lib-,$(LIBS))
LIBINCDIRS:=$(addsuffix /include, $(LIBINCDIRS))

# The variables for the ld -L flag
LIBH3=$(addprefix -L../lib-,$(LIBS))
LIBH3:=$(addsuffix /lib_h3, $(LIBH3))

# The variable for the ld -l flag 
LDLIBS:=$(addprefix -l,$(LIBS))

# The variables for the dependency check 
LIBDEP=$(addprefix ../lib-,$(LIBS))
LIBSDEP=$(addsuffix /lib_h3/lib, $(LIBDEP))
LIBSDEP:=$(join $(LIBSDEP), $(LIBS))
LIBSDEP:=$(addsuffix .a, $(LIBSDEP))

COPS=-DBARE_METAL -DH3 -D$(PLATFORM) $(DEFINES)
COPS+=$(INCDIRS) $(LIBINCDIRS) $(addprefix -I,$(EXTRA_INCLUDES))
COPS+=-Wall -Werror -O2 -nostartfiles -nostdinc -nostdlib -ffreestanding -mhard-float -mfloat-abi=hard
COPS+=-mfpu=neon-vfpv4 -march=armv7-a -mtune=cortex-a7

C_OBJECTS=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.c,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.c)))
C_OBJECTS+=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.cpp,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.cpp)))
ASM_OBJECTS=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.S,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.S)))

BUILD_DIRS:=$(addprefix $(BUILD),$(SRCDIR))

OBJECTS:=$(ASM_OBJECTS) $(C_OBJECTS)

define compile-objects
$(BUILD)$1/%.o: $(SOURCE)$1/%.cpp
	$(CPP) -pedantic -fno-exceptions -fno-unwind-tables -fno-rtti -std=c++11 -nostdinc++ $(COPS) -c $$< -o $$@	

$(BUILD)$1/%.o: $(SOURCE)$1/%.c
	$(CC) $(COPS) -c $$< -o $$@
	
$(BUILD)$1/%.o: $(SOURCE)$1/%.S
	$(CC) $(COPS) -D__ASSEMBLY__ -c $$< -o $$@
endef

all : clearlibs builddirs prerequisites $(TARGET)
	
.PHONY: clean builddirs

clearlibs:
	$(MAKE) -f Makefile.H3 clean --directory=../lib-console
	$(MAKE) -f Makefile.H3 clean --directory=../lib-h3
	$(MAKE) -f Makefile.H3 clean --directory=../lib-hal
	$(MAKE) -f Makefile.H3 clean --directory=../lib-remoteconfig

builddirs:
	mkdir -p $(BUILD_DIRS)

clean:
	rm -rf $(BUILD)
	rm -f $(TARGET)
	rm -f $(MAP)
	rm -f $(LIST)
	rm -f $(SUFFIX).uImage
	for d in $(LIBDEP); \
		do                               \
			$(MAKE) -f Makefile.H3 clean --directory=$$d;       \
		done
#
# Build libraries
#
$(LIBSDEP):
	for d in $(LIBDEP); \
		do                               \
			$(MAKE) -f Makefile.H3 'PLATFORM=$(PLATFORM)' 'MAKE_FLAGS=$(DEFINES)' --directory=$$d;       \
		done

# Build uImage

$(BUILD)vectors.o : $(FIRMWARE_DIR)/vectors.S
	$(AS) $(COPS) -D__ASSEMBLY__ -c $(FIRMWARE_DIR)/vectors.S -o $(BUILD)vectors.o
	
$(BUILD)main.elf: Makefile.H3 $(LINKER) $(BUILD)vectors.o $(OBJECTS) $(LIBSDEP)
	$(LD) $(BUILD)vectors.o $(OBJECTS) -Map $(MAP) -T $(LINKER) -o $(BUILD)main.elf $(LIBH3) $(LDLIBS)
	$(PREFIX)objdump -D $(BUILD)main.elf | $(PREFIX)c++filt > $(LIST)
	$(PREFIX)size -A $(BUILD)main.elf

$(TARGET) : $(BUILD)main.elf 
	$(PREFIX)objcopy $(BUILD)main.elf -O binary $(TARGET)
	mkimage -n 'http://www.orangepi-dmx.org' -A arm -O u-boot -T standalone -C none -a 0x40000000 -d $(TARGET) $(SUFFIX).uImage

$(foreach bdir,$(SRCDIR),$(eval $(call compile-objects,$(bdir))))
