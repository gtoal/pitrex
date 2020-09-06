#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>

void startFrame()
{
  v_WaitRecal();
  v_readButtons();
  v_readJoystick1Analog();
}

int main(int argc, char **argv) {
  vectrexinit(1);
  v_init();
  v_setRefresh(60);
  for (;;)
  {
    startFrame();
    v_directDraw32(-10000, -10000,  -10000,  10000, 64);
    v_directDraw32(-10000,  10000,   10000,  10000, 64);
    v_directDraw32( 10000,  10000,   10000, -10000, 64);
    v_directDraw32( 10000, -10000,  -10000, -10000, 64);
    v_printString(-10,0, "HELLO", 7, 64);
    v_setBrightness(60);
    v_printStringRaster(-8,-4, "WORLD", 5*8, -7, '\0');
  }
  return 0;
}
