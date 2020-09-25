/* PiTrex window calibration screen based on "window" demo
 *  Writes to the "default" settings file
 */

#include <stdio.h>
#include <string.h> // for strrchr
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include "window.h"

void startFrame()
{
  v_WaitRecal();
  //v_doSound();
  v_setBrightness(64);        /* set intensity of vector beam... */
  v_readButtons();
  v_readJoystick1Analog();
  //v_playAllSFX();
}

int main(int argc, char **argv) {
  //#define SETTINGS_SIZE 1024
  //static unsigned char settingsBlob[SETTINGS_SIZE];
  char *progname, *p;

  vectrexinit(1);
  v_setName("default");
  v_init();

  // orientation will be passed in by Malban. (Would be nice if done by v_init for us.)
  // until that's done, pass 0 / 1 / 2 / 3 in on command line (raspbian testing only)
  if (argc == 1) v_set_hardware_orientation(VECTREX_DEFAULT);
  else if (*argv[1] == '0') v_set_hardware_orientation(VECTREX_VERTICAL); 
  else if (*argv[1] == '1') v_set_hardware_orientation(VECTREX_HORIZONTAL);
  else if (*argv[1] == '2') v_set_hardware_orientation(VECTREX_INVERTED);
  else if (*argv[1] == '3') v_set_hardware_orientation(VECTREX_HORIZONTAL_INVERTED);
  else v_set_hardware_orientation(VECTREX_DEFAULT);

  usePipeline = 1;   // should create procedures for these rather than use global variables.
                     // doesn't matter for now but will be needed if we invent a serial
                     // terminal protocol and implement it using an RPC interface
  v_setRefresh(60);
  v_window(0,0, 360,480, NO_CLIP); // clipping currently broken except in default vertical orientation

  for (;;) {
    startFrame();
    v_line(  0,   0,   0, 480, 96); // settings should be adjusted interactively so that this frame exactly fits the screen in vertical mode
    v_line(  0, 480, 360, 480, 96); // Make the frame smaller than the screen (eg 2cm margin) to test clip window
    v_line(360, 480, 360,   0, 96);
    v_line(360,   0,   0,   0, 96);

    // equilateral triangle
    
    //         280,340
    // 180,313 |              height=sqrt(3)*base/2    1.73*200/2 = 173
    //     /\  |
    //    /  \ |
    //   /____\|
    //80,140  280,140
    
    v_line(80, 140, 280,140, 64); // upward-pointing triangle with a vertical on the right hand side
    v_line(280,140, 280,340, 64); // to confirm chiralty
    v_line(280,140, 180,140+173, 64);
    v_line(80, 140, 180,140+173, 64);

    // here is where current code leads to problems:
    //
    // 1) both of these should have used Malban's adjusted device coordinates in their original versions
    //    and virtual world coordinates in a version that supports the v_window() call.
    //
    // 2) and they should rotate appropriately along with the v_line() calls (and anything else that will
    //    eventually use world coordinates)
    
    v_printString(-30,-17, "CALIBRATE", 7, 64); // with 'CALIBRATE' in center of triangle.
    v_setBrightness(60); v_printStringRaster(-31,-40, "PRESS 1+2", 5*8, -7, '\0');
  }
  return 0;
}
