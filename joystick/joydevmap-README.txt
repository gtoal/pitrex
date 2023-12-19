
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
(about $19 shipped)

  https://www.ebay.com/sch/i.html?_from=R40&_nkw=sidewinder+precision+pro&_sacat=0&_sop=15&rt=nc&LH_BIN=1

  https://www.amazon.com/Belkin-Joystick-Adapter-SideWinder-F3U200-08/dp/B000067RIV/
(The adapter will cost roughly another $20 shipped)
  
I think the Sidewinder Pro has enough axes and buttons to handle Battlezone pretty nicely.

Another use for this interface is to build a converter from USB joysticks to
Vectrex native. This will require analogue hardware to produce the appropriate
voltages to input to the Vectrex's two I/O ports.

If we do that, the joytest program should be used to document any new joysticks so
that we know which inputs to map to which Vectrex joystick axis or button.

We have not yet added linux joystick support to the Pitrex ports of arcade
video games.
