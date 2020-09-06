/*****************************************************************************
* CINEDB.CPP                                                                 *
*                                                                            *
* Cinematronics Emulator / Debugger.                                         *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
  
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include <vectrex/osWrapper.h>

#define   VERSION   "Version 1.3"

#include "cines.h"
#include "inifile.h"
#include "options.h"

// cineops.c:
extern uchar bNewFrame;
extern void CinemaClearScreen (void);

#define PROGSIZE  32768L
static uchar ProgData[PROGSIZE];	// static memory for CPU data

#define SCRASIZE  129
static char TempBfr[SCRASIZE];		// just a general use temporary buffer

/**************************** main **********************************/

void main (int argc, char **argv)
{
   int err;

   vectrexinit (1);
   v_init ();
   usePipeline = 1;
   v_setRefresh (60);

#ifdef FREESTANDING
   char *_argv[3];
   _argv[0] = "cine.img";
   _argv[1] = getLoadParameter ();
   _argv[2] = NULL;
   argc = 2;
   argv = _argv;
   printf ("Game selected: %s\r\n", _argv[1]);
#endif

   static char hdr[] =
    "CINEMU - Cinematronics Emulator. " VERSION
    "\n(c) Copyright 1997, Zonn Moore.  All rights reserved.\n";

   if ((argc == 2) && (strcmp(argv[1], "-V") == 0)) 
   {
     fputs (hdr, stderr);
     exit(0);
   }

   if ((argc != 2) || (strcmp(argv[1], "-h") == 0)) 
   {
      fprintf (stderr, "syntax: cinemu [-h | -V | <game>]  where the supported games are:\n");
      fprintf (stderr, "  armorattack  boxingbugs   demon  ripoff      spacewars   starcastle  sundance    waroftheworlds\n");
      fprintf (stderr, "  barrier      cosmicchasm  qb3    solarquest  speedfreak  starhawk    tailgunner  warrior\n");
  
      fprintf (stderr, "\n");
      exit (1);
   }
   /*
   * Games known to work 100% or nearly so, at some point or another of
   * the emulators development (but not necessarily right now :):
   * RipOff, Solar Quest, Spacewar, Speed Freak, Star Castle, Star Hawk,
   * Tail Gunner, War of the Worlds, Warrior
   *
   * For reference, all of the cinematronics games are:
   * Armor Attack, Barrier, Boxing Bugs, Demon, Ripoff, Solar Quest,
   * Spacewar, Speed Freak, Star Castle, Star Hawk, Sundance, Tail Gunner
   * War of the worlds, Warrior
   * (There is another, but it has not been made available yet)
   */
   if (strcasecmp (argv[1], "tailgunner") == 0) {
   } else if (strcasecmp (argv[1], "ripoff") == 0) {
   } else if (strcasecmp (argv[1], "spacewars") == 0) {
   } else if (strcasecmp (argv[1], "boxingbugs") == 0) {
   } else if (strcasecmp (argv[1], "armorattack") == 0) {
   } else if (strcasecmp (argv[1], "qb3") == 0) {
   } else if (strcasecmp (argv[1], "starcastle") == 0) {
   } else if (strcasecmp (argv[1], "starhawk") == 0) {
   } else if (strcasecmp (argv[1], "speedfreak") == 0) {
   } else if (strcasecmp (argv[1], "demon") == 0) {
   } else if (strcasecmp (argv[1], "solarquest") == 0) {
   } else if (strcasecmp (argv[1], "cosmicchasm") == 0) {
   } else if (strcasecmp (argv[1], "waroftheworlds") == 0) {
   } else if (strcasecmp (argv[1], "warrior") == 0) {
   } else if (strcasecmp (argv[1], "barrier") == 0) {
   } else if (strcasecmp (argv[1], "sundance") == 0) {
   } else {
      fprintf (stderr, "The supported games are:\n");
      fprintf (stderr, "  armorattack  boxingbugs   demon  ripoff      spacewars   starcastle  sundance    waroftheworlds\n");
      fprintf (stderr, "  barrier      cosmicchasm  qb3    solarquest  speedfreak  starhawk    tailgunner  warrior\n");
      fprintf (stderr, "\n");
      exit (1);
   }

   cineSetGameName (argv[1]);

   sprintf (TempBfr, "ini/%s.ini", argv[1]);

   err = openIniFile (TempBfr);
   if (err != iniErrOk)
      exit (err);

   err = execIniFile ();
   if (err != iniErrOk)

      exit (err);

   // read ROMs
   if (OptRomImages[0] == 0) {
      fputs ("\nError: No ROM image files specified in configuration file.\n", stderr);
      exit (1);
   }

   if (readRoms (OptRomImages, ProgData, OptMemSize))
      exit (1);

   pComments ();		// print all the printable comments. Maybe only if in linux mode, not freestanding?
   closeIniFile ();

   // now that we've got settings from ini file, pass them to the emulator:
   
   cineSetJMI (OptJMI);		// set JMI option
   cineSetMSize (OptMemSize);	// set memory size
   cineSetRate (60);            // Ini files support "Standard" vs "Full" but not used.
   cineSetSw (OptSwitches, OptInputs);	// set switches & inputs
   cineSetMonitor (OptMonitor);	// set up type of monitor - not really used but we may want to determine horiz vs vertical later...
   cineSetMouse (OptMouseType, OptMouseSpeedX, OptMouseSpeedY, OptMouseValid, OptMKeyMap);    // will probably want to remove mouse code
   vgSetCineSize (OptWinXmin, OptWinYmin, OptWinXmax, OptWinYmax);
   vgSetMonitor (OptMonitor,	// set up type of monitor and colors
		 OptRedB, OptGreenB, OptBlueB, OptRedC, OptGreenC, OptBlueC);
   vgSetTwinkle (OptTwinkle);	// set twinkle level
   vgSetRotateFlip (OptRotate, OptFlipX, OptFlipY);
   vgInit ();

   // #################################################################################
   // ##                                                                             ##
   // ##                               EMULATE!                                      ##
   // ##                                                                             ##
   // #################################################################################
   cineInit (ProgData, OptKeyMap);	// setup segment pointer, reset breakpoints
   cineReset ();		// called after ini file has been read.
   for (;;) {
      cineExec ();		/* either run one instruction or a lot ending in a vsync */
      if (bNewFrame != 0) 
      {	                        // vsync
        bNewFrame = 0;
        CinemaClearScreen ();	// This has to enforce execution speed of
                                //  (eg) 1/38 sec to generate a frame
                                // potentially could be sparated from drawing, eg at 50Hz for Vectrex
      }
   }
   exit(0); // probably never reached.
}
