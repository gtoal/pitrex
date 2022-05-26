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
extern int optimizationON; // should be in header. preferably with a procedure to set it.

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
      // NOTE: Cosmic Chasm removed because it is a 68000 game!
      fprintf (stderr, "syntax: cinemu [-h | -V | <game>]  where the supported games are:\n");
      fprintf (stderr, "  armorattack  boxingbugs   qb3*   solarquest* speedfreak  starhawk  tailgunner     warrior\n");
      fprintf (stderr, "  barrier      demon        ripoff spacewars   starcastle  sundance  waroftheworlds\n");
      fprintf (stderr, "\n");
      fprintf (stderr, "[*: qb3 and solarquest not yet working]\n");
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
      fprintf (stderr, "  armorattack  boxingbugs   qb3*    solarquest* speedfreak  starhawk  tailgunner     warrior\n");
      fprintf (stderr, "  barrier      demon        ripoff  spacewars   starcastle  sundance  waroftheworlds\n");
      fprintf (stderr, "\n");
      fprintf (stderr, "[*: qb3 and solarquest not yet working]\n");
      fprintf (stderr, "\n");
      exit (1);
   }

   cineSetGame (argv[1], Game);

   sprintf (TempBfr, INI_DIR"/%s.ini", argv[1]); // temporarily hard-coded (now in header) until we have something better such as 'pathopen()'

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

   bufferType = 2; // 0 = none, 1 = double buffer, 2 = auto buffer (if pipeline is empty -> use previous
   optimizationON = 1; // don't optimise, ie reset to 0 for every line... - game is more than fast enough to handle it
   
   // I've moved the speed control into each game so they can be tweaked separately.
   // I think the games are generally supposed to run at 38Hz which is slow for the
   // vectrex (shorter persistence phosphor) so maybe the best plan is to run at
   // 76Hz and fill in every alternate frame.

   // (Note: This is *not* related to the fact that the cinematronics code calls WAI
   //  twice in a row between frames and that one of those calls has to be skipped.
   //  That's just a distracting coincidence.)

   // btw Malban recommends displaying at 50Hz if possible to get a smoother update.
   // I'm running several (most) of the games at that speed for the moment but that
   // does cause them to run at the wrong rate - probably 25% too fast.  So I really
   // do need to come back and revisit this.  In Malban's interrupt-driven code in the
   // bare-metal environment, he separates the frame generation rate from the display
   // rate, which is the best option and one I'ld like to do here, but I don't think
   // our current system code supports that properly.  The auto buffer mode is the
   // closest we have to it which is what is used here, but to make it work we have
   // to run at double the rate and redraw every second frame, which we don't yet do.
   
   switch (Game) {

   case GAME_RIPOFF:     
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_ripoff(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   case GAME_SPACEWARS:     
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_spacewars(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   case GAME_BOXINGBUGS:     
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_boxingbugs(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   case GAME_ARMORATTACK:     
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_armorattack(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   case GAME_STARCASTLE:     
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_starcastle(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   case GAME_STARHAWK:     
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_starhawk(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   case GAME_SOLARQUEST:     
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_solarquest(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   case GAME_WAROFTHEWORLDS:     
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_waroftheworlds(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   case GAME_WARRIOR:     
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_warrior(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   case GAME_BARRIER:     
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_barrier(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   case GAME_SUNDANCE:
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_sundance(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

     
   case GAME_QB3:
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_qb3(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;
     

   case GAME_TAILGUNNER:
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_tailgunner(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
	   // Tailgunner calls the instruction that we use to determine vsync twice in a row without intervening drawing code.
	   // ... turns out so does everything else ...
         }
       }
     }
     return;

   case GAME_DEMON:
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_demon(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   case GAME_SPEEDFREAK:
     {
       static int parity = 0;
       v_setRefresh (76);
       for (;;) {
         cineExec ();		/* either run one instruction or a lot ending in a vsync */
         if (bNewFrame != 0) {
           bNewFrame = 0;
           parity ^= 1;
           if (parity) startFrame_speedfreak(); else v_WaitRecal (); // halve the game speed, keep the refresh rate
         }
       }
     }
     return;

   default:
     {
       v_setRefresh (76);
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
