# PiTrex

Support for controlling a Vectrex display and other I/O from a Raspberry Pi Zero WH.

PiTrex is a open source/open hardware project.

This repository is currently aimed only at users of the PiTrex hardware - 'Developer release'.

Our intention is that everything you need to develop software for the PiTrex will be in this
repository and that you should fetch the entire repository from git. eg.

sudo apt-get install -y gcc-arm-none-eabi git-core libsdl2-dev libsdl2-2.0 libsdl2-mixer-2.0-0 libsdl2-mixer-dev locate
mkdir ~/src
cd ~/src
git clone https://github.com/gtoal/pitrex.git
cd ~/src/pitrex
make -f Makefile.raspbian
# if you intend to use the bare metal environment as well:
make -f Makefile.baremetal

We realise that there are some imported projects here from other repositories which we have
duplicated, but for the moment it is simpler just to have everything together.  When we have
more experience with git and packaging software, we'll remove the redundant copies and link
to the original repositories.

You'll need to add these to /boot/config.txt and reboot before using the linux executables:

gpio=0-5,16-24,26-29=ip
gpio=6-13,25=op
gpio=24=np
dtoverlay=dwc2,dr_mode=host

Add this line to boot to the PiTrex Bare-Metal menu:

kernel pitrex.img

More comprehensive installation instructions, along with lots of other documentation, can be
found at the PiTrex Wiki.

Some things that aren't working yet include:
* Tempest, and some other arcade emulations, don't display.
* Vectrex button mapping is poor or not working at all for some arcade emulations.
* The Calibrate program doesn't work in Bare-Metal (nor do any other programs using the functions from the window.c example).

PiTrex Links:

Wiki: http://computernerdkev.heliohost.org/pitrex/wiki/

Developer's Forum / Mailing List: https://groups.io/g/pitrex-dev
