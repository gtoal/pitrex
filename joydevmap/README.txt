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
