all:
	make -f Makefile.raspbian
	make -f Makefile.baremetal
	make translator

clean:
	make clean -f Makefile.raspbian
	make clean -f Makefile.baremetal
	rm -f translator

translator: translator.c game.c memory.c display.c no_interface.c mathbox.c pokey.c
	gcc -Wall -g -o translator translator.c game.c memory.c display.c no_interface.c mathbox.c pokey.c
