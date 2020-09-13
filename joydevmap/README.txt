This code came from https://www.sthu.org/code/joydevmap.html
and https://github.com/abedra/geekfest-linux-kernel-joystick-api/blob/master/fun-with-the-linux-kernel.org

For use with https://www.kernel.org/doc/Documentation/input/joystick-api.txt

Primarily this will allow PiTrex programs to use more input devices than
just the Vectrex controllers.

We may eventually be able to map the Vectrex controller as a Linux joystick.
That would allow PiTrex programs to trivially switch between Vectrex
controller input and USB Joystick input.

The biggest potential win from this might be in Vectrex emulators, allowing
an expanded range of USB controllers to be used with existing Vectrex games.

Personally I use an old Sidewinder with a USB adapter, which can be had relatively cheaply nowadays:

  https://www.ebay.com/sch/i.html?_from=R40&_nkw=sidewinder+precision+pro&_sacat=0&_sop=15&rt=nc&LH_BIN=1

  https://www.amazon.com/Belkin-Joystick-Adapter-SideWinder-F3U200-08/dp/B000067RIV/

I think this one has enough axes and buttons to handle Battlezone pretty nicely.
