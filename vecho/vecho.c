/* Display the argument string on the Vectrex.
 * Or the string defined as DISPLAY_STRING if no arguments.
 * Exits when button pressed.
 * TODO:
 * * Allow exit controlled by another process in Linux
 * * Support newlines (centred vertically)
 * * More eye candy
 *
 */

#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>

#ifndef DISPLAY_STRING
#define DISPLAY_STRING "PITREX"
#endif

int main(int argc, char **argv)
{
  unsigned int tlength;
  unsigned int tstart = 0;
  unsigned int strings = 0;
  char defaulttext[] = DISPLAY_STRING;
  char *argchar = argv[0];

  vectrexinit(1);
  v_setName("vecho");
  v_init();
  v_setRefresh(60);
  if (argc > 1)
  {
    for (tlength=0; strings < argc; tlength++)
    {
      if (*(argchar + tlength) == '\0')
      {
       strings++;
       if (strings < argc)
        *(argchar + tlength) = ' ';
      }
      if (strings == 1 && tstart == 0)
       tstart = tlength;
    }

    tlength = (tlength - 1) - tstart;
  }

  else
  {
    tlength = sizeof(DISPLAY_STRING) - 1;
  }

  char b = 1;
  char bDir = 1;
  unsigned int s = 0;
  while (1)
  {
    v_WaitRecal();
    v_readButtons();
    b=b+bDir;
    if (b==70)
     bDir = -1;
    if (b==50)
     bDir = 1;

    if (argc > 1)
    {
      v_setBrightness(b);
      v_printStringRaster((tlength/2)*-6, 0, (argchar + tstart), 5*8, -7, '\0');
    }
    else
    {
      v_setBrightness(b);
      v_printStringRaster((tlength/2)*-6, 0, defaulttext, 5*8, -7, '\0');
    }

    if (currentButtonState)
     return 0;
  }
}
