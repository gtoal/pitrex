all: tailgunr-ops.c tailgunner-data.c
	echo All up to date.

roms: ../../cinematronics/roms/tailgunner/tailg.t7 ../../cinematronics/roms/tailgunner/tailg.p7 ../../cinematronics/roms/tailgunner/tailg.u7 ../../cine\
matronics/roms/tailgunner/tailg.r7
	@cp `md5sum \`locate -i tailg.t7\` 2>/dev/null | fgrep a377b3b34e4a4ca8b0048ea4ba53c480 | head -1 | tr ' ' '\n' | tail -1` ../../cinematronics/roms/tailgunner/tailg.t7 2> /dev/null || ( echo You need to follow the instructions in README-ROMS.txt in the main directory && exit 1 )
	@cp `md5sum \`locate -i tailg.p7\` 2>/dev/null | fgrep 96bf9adaa9d21ab5fd5ed7fc26759486 | head -1 | tr ' ' '\n' | tail -1` ../../cinematronics/roms/tailgunner/tailg.p7 2> /dev/null || ( echo You need to follow the instructions in README-ROMS.txt in the main directory && exit 1 )
	@cp `md5sum \`locate -i tailg.u7\` 2>/dev/null | fgrep df68e2d4bd5e58513cb6e3a33768483b | head -1 | tr ' ' '\n' | tail -1` ../../cinematronics/roms/tailgunner/tailg.u7 2> /dev/null || ( echo You need to follow the instructions in README-ROMS.txt in the main directory && exit 1 )
	@cp `md5sum \`locate -i tailg.r7\` 2>/dev/null | fgrep 1d93d2e15e85ae30c9e2d850e3f3aae9 | head -1 | tr ' ' '\n' | tail -1` ../../cinematronics/roms/tailgunner/tailg.r7 2> /dev/null || ( echo You need to follow the instructions in README-ROMS.txt in the main directory && exit 1 )

../../cinematronics/roms/tailgunner/tailg.t7:
	@echo "If you already have tailg.t7 on this system (and use locate),"
	@echo "you can copy the files to the appropriate directory by"
	@echo "typing:  make -f Makefile.raspbian roms"
	@exit 1

../../cinematronics/roms/tailgunner/tailg.p7:
	@echo "If you already have tailg.p7 on this system (and use locate),"
	@echo "you can copy the files to the appropriate directory by"
	@echo "typing:  make -f Makefile.raspbian roms"
	@exit 1

../../cinematronics/roms/tailgunner/tailg.u7:
	@echo "If you already have tailg.u7 on this system (and use locate),"
	@echo "you can copy the files to the appropriate directory by"
	@echo "typing:  make -f Makefile.raspbian roms"
	@exit 1

../../cinematronics/roms/tailgunner/tailg.r7:
	@echo "If you already have tailg.r7 on this system (and use locate),"
	@echo "you can copy the files to the appropriate directory by"
	@echo "typing:  make -f Makefile.raspbian roms"
	@exit 1

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
