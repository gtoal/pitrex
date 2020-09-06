all: tailgunr-ops.c tailgunner-data.c
	echo All up to date.

tailgunr.rom: convert ../../cinematronics/roms/tailgunner/tailg.t7 ../../cinematronics/roms/tailgunner/tailg.p7 ../../cinematronics/roms/tailgunner/tailg.u7 ../../cinematronics/roms/tailgunner/tailg.r7
	./convert ../../cinematronics/roms/tailgunner/tailg.t7 ../../cinematronics/roms/tailgunner/tailg.p7 ../../cinematronics/roms/tailgunner/tailg.u7 ../../cine\
matronics/roms/tailgunner/tailg.r7 > tailgunr.rom

convert: convert.c
	cc -o convert convert.c

tailgunner-data.c: tailgunr.rom makearray
	./makearray tailgunr.rom > tailgunner-data.c

makearray: makearray.c
	cc -o makearray makearray.c

tailgunr-ops.c: translate
	./translate

translate: translate.c macros.h disasm.h tailgunner-data.c disopc.h ccpu.h opcodes.h tailgunr.jmp tailgunr.opt
	cc -DIS_LONG64 -o translate -DUSE_INVARIANTS translate.c
	#cc -o translate -DSUPEROPTIMISE translate.c
	#cc -o translate translate.c

clean:
	rm -f translate makearray convert tailgunner-data.c tailgunr-ops.c dispatch-ops.h tailgunr.rom *~
