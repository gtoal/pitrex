// Linux specific functions for the Vector Mame Menu

#include "vmmstddef.h"
#include "LinuxVMM.h"
#ifdef PITREX
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <fcntl.h>
#include <unistd.h>
#endif

int LEDstate=0;

/******************************************************************
   Write to keyboard LEDs value held in global variable LEDstate
   only if state has changed
   Unfortunately the use of ioctl requires root access,
   or (apparently) the capability "CAP_SYS_TTY_CONFIG"
*******************************************************************/
void setLEDs(int leds)
{
   if (LEDstate != leds)
   {
#ifdef PITREX
     // Looks like the 3 keyboard LEDs are cycled through to indicate progress.
     // We might use the Pi Zero on-board LEDs instead or even allow users to
     // access special hardware such as addressible LEDs fitted to the screen.
     // However, for now, we'll just ignore them.
#else
      int fd;
      if ((fd = open("/dev/console", O_NOCTTY)) != -1)
      {
         ioctl(fd, KDSETLED, leds);
         close(fd);
      }
#endif
      LEDstate = leds;
   }
}
