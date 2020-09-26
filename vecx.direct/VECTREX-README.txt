This emulator is only expected to work in bare-metal mode for now.

It'll compile and run under linux but we have no way at the moment of avoiding
system interrupts that would cause bad vector positioning etc.
 - EDIT: Actually now it will run without such glitches if built with
         "-DAVOID_TICKS", however exiting with Ctrl-C doesn't work and Linux
         crashes if you exit by pressing Reset on the Vectrex. I'm having a
         lot of trouble fixing this problem. - Kevin

1+2+3+4 exit command not supported.
