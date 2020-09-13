This code came from https://www.sthu.org/code/joydevmap.html

For use with https://www.kernel.org/doc/Documentation/input/joystick-api.txt

We may be able to map the Vectrex controller to a Linux joystick.  That would
mean that if our code which access the Vectrex controller uses this interface,
we could trivially allow USB joysticks to be used with PiTrex programs (including
the Vectrex emulator)
