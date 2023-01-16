BUILD_DIR_PURE := build.raspbian
BUILD_DIR := $(BUILD_DIR_PURE)/
PITREX_DIR := ../../pitrex/

CFLAGS := -I ../..

.PHONY: dirCheck

all: dirCheck pitrexLib vpu_disPipe-loader

dirCheck:
	if [ ! -d $(BUILD_DIR_PURE) ]; then mkdir $(BUILD_DIR_PURE); fi

pitrexLib:
	$(MAKE) -C $(PITREX_DIR) -f Makefile.raspbian all

$(BUILD_DIR)mailbox.o: mailbox.c mailbox.c
	gcc $(CFLAGS) -c $< -o $(BUILD_DIR)$@

$(BUILD_DIR)gpuMem.o: gpuMem.c gpuMem.h
	gcc $(CFLAGS) -c $< -o $(BUILD_DIR)$@

vpu_disPipe-loader: main.c $(BUILD_DIR)mailbox.o $(BUILD_DIR)gpuMem.o
	gcc $(CFLAGS) -o vpu_disPipe-loader $^ $(PITREX_DIR)$(BUILD_DIR)bcm2835.o

clean:
	$(RM) vpu_disPipe-loader $(BUILD_DIR)*.*
