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

#define PROGSIZE  32768L
static uchar ProgData[PROGSIZE];	// static memory for CPU data

#define SCRASIZE  129
static char TempBfr[SCRASIZE];		// just a general use temporary buffer

/**************************** main **********************************/

void main (int argc, char **argv)
{
   int Game = 0;
   int err;

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
    "CINEMU - Cinematronics Emulator. " VERSION "\n"
    "(c) Copyright 1997, Zonn Moore.  All rights reserved.\n";

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
   */
   if (strcasecmp (argv[1], "tailgunner") == 0) {
     Game = GAME_TAILGUNNER;
   } else if (strcasecmp (argv[1], "ripoff") == 0) {
     Game = GAME_RIPOFF;
   } else if (strcasecmp (argv[1], "spacewars") == 0) {
     Game = GAME_SPACEWARS;
   } else if (strcasecmp (argv[1], "boxingbugs") == 0) {
     Game = GAME_BOXINGBUGS;
   } else if (strcasecmp (argv[1], "armorattack") == 0) {
     Game = GAME_ARMORATTACK;
   } else if (strcasecmp (argv[1], "starcastle") == 0) {
     Game = GAME_STARCASTLE;
   } else if (strcasecmp (argv[1], "starhawk") == 0) {
     Game = GAME_STARHAWK;
   } else if (strcasecmp (argv[1], "speedfreak") == 0) {
     Game = GAME_SPEEDFREAK;
   } else if (strcasecmp (argv[1], "demon") == 0) {
     Game = GAME_DEMON;
   } else if (strcasecmp (argv[1], "solarquest") == 0) {
     Game = GAME_SOLARQUEST;
   } else if (strcasecmp (argv[1], "cosmicchasm") == 0) {
     Game = GAME_COSMICCHASM;
   } else if (strcasecmp (argv[1], "waroftheworlds") == 0) {
     Game = GAME_WAROFTHEWORLDS;
   } else if (strcasecmp (argv[1], "warrior") == 0) {
     Game = GAME_WARRIOR;
   } else if (strcasecmp (argv[1], "barrier") == 0) {
     Game = GAME_BARRIER;
   } else if (strcasecmp (argv[1], "sundance") == 0) {
     Game = GAME_SUNDANCE;
   } else if (strcasecmp (argv[1], "qb3") == 0) {
     Game = GAME_QB3;
   } else {
      fprintf (stderr, "The supported games are:\n");
      fprintf (stderr, "  armorattack  boxingbugs   demon  ripoff      spacewars   starcastle  sundance    waroftheworlds\n");
      fprintf (stderr, "  barrier      cosmicchasm  qb3    solarquest  speedfreak  starhawk    tailgunner  warrior\n");
      fprintf (stderr, "\n");
      exit (1);
   }

   cineSetGame (argv[1], Game);

   sprintf (TempBfr, "/opt/pitrex/ini/%s.ini", argv[1]); // temporarily hard-coded until we have something better such as 'pathopen()'

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

   vectrexinit (1);
   v_init ();
   usePipeline = 1;
   v_setRefresh (60); // need to work on 38/76 to match hardware...

   switch (Game) {

   case GAME_RIPOFF:     
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_ripoff();
         }
       }
     }
     return;

   case GAME_SPACEWARS:     
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_spacewars();
         }
       }
     }
     return;

   case GAME_BOXINGBUGS:     
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_boxingbugs();
         }
       }
     }
     return;

   case GAME_ARMORATTACK:     
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_armorattack();
         }
       }
     }
     return;

   case GAME_STARCASTLE:     
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_starcastle();
         }
       }
     }
     return;

   case GAME_STARHAWK:     
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_starhawk();
         }
       }
     }
     return;

   case GAME_SOLARQUEST:     
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_solarquest();
         }
       }
     }
     return;

   case GAME_COSMICCHASM:     
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_cosmicchasm();
         }
       }
     }
     return;

   case GAME_WAROFTHEWORLDS:     
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_waroftheworlds();
         }
       }
     }
     return;

   case GAME_WARRIOR:     
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_warrior();
         }
       }
     }
     return;

   case GAME_BARRIER:     
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_barrier();
         }
       }
     }
     return;

   case GAME_SUNDANCE:
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_sundance();
         }
       }
     }
     return;

     
   case GAME_QB3:
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_qb3();
         }
       }
     }
     return;
     

   case GAME_TAILGUNNER:
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_tailgunner(); // Tailgunner calls the instruction that we use to determine vsync twice in a row without intervening drawing code.
         }
       }
     }
     return;

   case GAME_DEMON:
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_demon(); // Demon calls the instruction that we use to determine vsync twice in a row without intervening drawing code.
         }
       }
     }
     return;

   case GAME_SPEEDFREAK:
     {
       static int parity = 0;
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_speedfreak(); // Speedfreak calls the instruction that we use to determine vsync twice in a row without intervening drawing code.
         }
       }
     }
     return;

   default:
     {
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           startFrame();
         }
       }
     }
   }
   exit(0); // probably never reached.
}
