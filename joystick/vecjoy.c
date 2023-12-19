//#include <vectrex/graphics_lib.h>
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for exit()
#include <errno.h>
void
startFrame ()
{
  v_WaitRecal ();
  v_readButtons ();
  v_readJoystick1Analog ();
}

int main(int argc, char **argv) {
  int tick = 0;
  int x, y;

  vectrexinit (1);
  v_setName("vecjoy");
  v_init ();
  v_setRefresh (60);

  for (;;) {
    // read joysticks, buttons ...

    startFrame();

    if (tick++ == 2000) { // temporary failsafe to exit if I mess up with menu actions
      exit(0);
    }

    x = currentJoy1X;
    y = currentJoy1Y;
    fprintf(stderr, "x=%d y=%d\n", x, y);
    
    x *= 200; y *= 150;
    
    v_directDraw32(x-500,    y, x+500,    y, 127);
    v_directDraw32(   x, y-500,    x, y+500, 127);

    v_directDraw32(-10000, -10000,  -10000,  10000, 64);
    v_directDraw32(-10000,  10000,   10000,  10000, 64);
    v_directDraw32( 10000,  10000,   10000, -10000, 64);
    v_directDraw32( 10000, -10000,  -10000, -10000, 64);
    v_printString(-10,0, "HELLO", 7, 64);
    v_setBrightness(60);
    v_printStringRaster(-8,-4, "WORLD", 5*8, -7, '\0');
    
#ifdef NEVER
    v_directDraw32Patterned(xl, yb, xr, yt, pattern, bri);
#endif

  }
  exit(0);
  return 0;
}

