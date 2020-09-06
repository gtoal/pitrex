all:
	bash -c "pushd translate; make; popd"
	make -f Makefile.raspbian
	make -f Makefile.baremetal

clean:
	bash -c "pushd translate; make clean; popd"
	make -f Makefile.raspbian clean
	make -f Makefile.baremetal clean
