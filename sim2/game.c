/*
 * game.c: Atari Vector game definitions & setup functions
 *
 * Copyright 1991, 1992, 1993, 1996, 2003 Hedley Rainnie and Eric Smith
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: game.c 2 2003-08-20 01:51:05Z eric $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vectrex/vectrexInterface.h>
#include <vectrex/osWrapper.h>

#include "display.h"
#include "memory.h"
#include "game.h"

int use_nmi;
int game = 0;
void (*gameCallback)(int) = 0;

extern unsigned char avg_prom[256];


typedef struct { int show; char *kw1; char *kw2; char *name; } game_info;

game_info game_names [] =
{
  { 0, "unknown",    "??",    "Unknown" },
  { 1, "lunar",      "ll",    "Lunar Lander" },
  { 1, "asteroids",  "a",     "Asteroids" },
  { 1, "deluxe",     "ad",    "Asteroids Deluxe" },
  { 0, "redbaron",   "rb",    "Red Baron" },
  { 0, "battlezone", "bz",    "Battlezone" },
  { 0, "tempest",    "t",     "Tempest" },
  { 1, "spaceduel",  "sd",    "Space Duel" },
  { 1, "gravitar",   "g",     "Gravitar" },
  { 1, "blackwidow", "bw",    "Black Widow" },
  { 0, "majorhavoc", "mh",    "The Adventures of Major Havoc" },
  { 0, "starwars",   "sw",    "Star Wars" },
  { 0, "empire",     "tesb",  "The Empire Strikes Back" },
  { 0, "quantum",    "q",     "Quantum" }
};


int pick_game (char *name)
{
  int i;

  for (i = FIRST_GAME; i <= LAST_GAME; i++)
    if ((strcasecmp (name, game_names [i].kw1) == 0) ||
	(strcasecmp (name, game_names [i].kw2) == 0))
      {
	return (i);
      }

  fprintf (stderr, "ERROR: Unknown game \"%s\"\n", name);
  exit (1);
}


void show_games (void)
{
  int i;

  for (i = FIRST_GAME; i <= LAST_GAME; i++)
    if (game_names [i].show)
      fprintf (stderr, "    %-10s    %s\n", game_names [i].kw1, game_names [i].name);
}


char *game_name (int game)
{
  return (game_names [game].name);
}

/*
rom_info black_widow_roms [] =
{
  { "roms/BlackWidow/136017.101", 0x9000, 0x1000, 0 },
  { "roms/BlackWidow/136017.102", 0xa000, 0x1000, 0 },
  { "roms/BlackWidow/136017.103", 0xb000, 0x1000, 0 },
  { "roms/BlackWidow/136017.104", 0xc000, 0x1000, 0 },
  { "roms/BlackWidow/136017.105", 0xd000, 0x1000, 0 },
  { "roms/BlackWidow/136017.106", 0xe000, 0x1000, 0 },

  { "roms/BlackWidow/136017.107", 0x2800, 0x0800, 0 },
  { "roms/BlackWidow/136017.108", 0x3000, 0x1000, 0 },
  { "roms/BlackWidow/136017.109", 0x4000, 0x1000, 0 },
  { "roms/BlackWidow/136017.110", 0x5000, 0x1000, 0 },

  { NULL,   0,      0,      0 }
};
*/
rom_info2  black_widow_roms2 [] =
{
  { "roms/bwidow.zip","136017-101.d1", 0x9000, 0x1000, 0  , 0xFE3FEBB7},
  { "roms/bwidow.zip","136017-102.ef1", 0xa000, 0x1000, 0 , 0x10AD0376},
  { "roms/bwidow.zip","136017-103.h1", 0xb000, 0x1000, 0  , 0x8A1430EE},
  { "roms/bwidow.zip","136017-104.j1", 0xc000, 0x1000, 0  , 0x44F9943F},
  { "roms/bwidow.zip","136017-105.kl1", 0xd000, 0x1000, 0 , 0x1FDF801C},
  { "roms/bwidow.zip","136017-106.m1", 0xe000, 0x1000, 0  , 0xCCC9B26C},
  { "roms/bwidow.zip","136017-107.l7", 0x2800, 0x0800, 0  , 0x52322C9E},
  { "roms/bwidow.zip","136017-108.mn7", 0x3000, 0x1000, 0 , 0x3DA354ED},
  { "roms/bwidow.zip","136017-109.np7", 0x4000, 0x1000, 0 , 0x2FC4CE79},
  { "roms/bwidow.zip","136017-110.r7", 0x5000, 0x1000, 0  , 0x0DD52987},

  { NULL,   0,      0,      0, 0 }
};

tag_info black_widow_tags [] =
{
  { 0x0000, 0x0800, RD | WR, MEMORY }, /* RAM */

  { 0x2000, 0x0800, RD | WR, VECRAM }, /* vector RAM */

  /* they try to scribble on the vector ROM */
  { 0x2fac, 0x002c,      WR, IGNWRT },

  { 0x6000, 0x0800, RD | WR, POKEY1 },
  { 0x6800, 0x0800, RD | WR, POKEY2 },

  { 0x6008,      1, RD,      OPTSW1 },
  { 0x6808,      1, RD,      OPTSW2 },

  { 0x7000,      4, RD,      EAROMRD },  /* why 4 locs? */

  { 0x7800,      1, RD,      COININ },
  { 0x8000,      1, RD,      GRAVITAR_IN1 },
  { 0x8800,      1,      WR, COINOUT },
  { 0x8800,      1, RD,      GRAVITAR_IN2 },
  { 0x8840,      1,      WR, VGO },
  { 0x8880,      1,      WR, VGRST },
  { 0x88c0,      1,      WR, INTACK },
  { 0x8900,      1,      WR, EAROMCON },
  { 0x8940,   0x40,      WR, EAROMWR },

  /* they write to 8981..89ed */
  { 0x8980,   0x6e,      WR, WDCLR },

  { 0,           0, 0,       0 }
};


rom_info2 gravitar_roms2 [] =
{
  { "roms/gravitar.zip", "136010-301.d1", 0x9000, 0x1000, 0, 0xA2A55013 },
  { "roms/gravitar.zip", "136010-302.ef1", 0xa000, 0x1000, 0, 0xD3700B3C },
  { "roms/gravitar.zip", "136010-303.h1", 0xb000, 0x1000, 0, 0x8E12E3E0 },
  { "roms/gravitar.zip", "136010-304.j1", 0xc000, 0x1000, 0, 0x467AD5DA },
  { "roms/gravitar.zip", "136010-305.kl1", 0xd000, 0x1000, 0, 0x840603AF },
  { "roms/gravitar.zip", "136010-306.m1", 0xe000, 0x1000, 0, 0x3F3805AD },
  { "roms/gravitar.zip", "136010-210.l7", 0x2800, 0x0800, 0, 0xB763780F },
  { "roms/gravitar.zip", "136010-207.mn7", 0x3000, 0x1000, 0, 0x4135629A },
  { "roms/gravitar.zip", "136010-208.np7", 0x4000, 0x1000, 0, 0x358F25D9 },
  { "roms/gravitar.zip", "136010-309.r7", 0x5000, 0x1000, 0, 0x4AC78DF4 },

  { NULL,   0,      0,      0, 0 }
};

tag_info gravitar_tags [] =
{
  { 0x0000, 0x0800, RD | WR, MEMORY }, /* RAM */

  { 0x2000, 0x0800, RD | WR, VECRAM }, /* vector RAM */

  { 0x6000, 0x0800, RD | WR, POKEY1 },
  { 0x6800, 0x0800, RD | WR, POKEY2 },

  { 0x6008,      1, RD,      OPTSW1 },
  { 0x6808,      1, RD,      OPTSW2 },

  { 0x7000,      4, RD,      EAROMRD },  /* why 4 locs? */

  { 0x7800,      1, RD,      COININ },
  { 0x8000,      1, RD,      GRAVITAR_IN1 },
  { 0x8800,      1,      WR, COINOUT },
  { 0x8800,      1, RD,      GRAVITAR_IN2 },
  { 0x8840,      1,      WR, VGO },
  { 0x8880,      1,      WR, VGRST },
  { 0x88c0,      1,      WR, INTACK },
  { 0x8900,      1,      WR, EAROMCON },
  { 0x8940,   0x40,      WR, EAROMWR },
  { 0x8980,      1,      WR, WDCLR },

  { 0,           0, 0,       0 }
};


rom_info2 space_duel_roms2 [] =
{
  { "roms/spacduel.zip", "136006-201.r1", 0x4000, 0x1000, 0 , 0xF4037B6E},
  { "roms/spacduel.zip", "136006-102.np1", 0x5000, 0x1000, 0 , 0x4C451E8A},
  { "roms/spacduel.zip", "136006-103.m1", 0x6000, 0x1000, 0 , 0xEE72DA63},
  { "roms/spacduel.zip", "136006-104.kl1", 0x7000, 0x1000, 0 , 0xE41B38A3},
  { "roms/spacduel.zip", "136006-105.j1", 0x8000, 0x1000, 0 , 0x5652710F},
  { "roms/spacduel.zip", "136006-106.r7", 0x2800, 0x0800, 0 , 0x85EB9802},
  { "roms/spacduel.zip", "136006-107.np7", 0x3000, 0x1000, 0 , 0xD8DD0461},

  { NULL,         0,      0,      0, 0 }
};

tag_info space_duel_tags [] =
{
  { 0x0000, 0x0400, RD | WR, MEMORY },  /* RAM */
  { 0x1000, 0x0400, RD | WR, POKEY1 },
  { 0x1400, 0x0400, RD | WR, POKEY2 },
  { 0x2000, 0x0800, RD | WR, VECRAM },

  { 0x0800,      1, RD,      COININ },

  /* Space duel uses an ASL instruction to get bit 7 of some
     of its inputs into the carry flag.  The write may be safely
     ignored.  ELS 920718 */
  { 0x0900,      8, RD,      SD_INPUTS },
  { 0x0905,      2,      WR, IGNWRT },

  { 0x0a00,      1, RD,      EAROMRD },
  { 0x0c00,      1,      WR, COINOUT },
  { 0x0c80,      1,      WR, VGO },
  { 0x0d00,      1,      WR, WDCLR },
  { 0x0d80,      1,      WR, VGRST },
  { 0x0e00,      1,      WR, INTACK },
  { 0x0e80,      1,      WR, EAROMCON },
  { 0x0f00,   0x40,      WR, EAROMWR },
  { 0x1008,      1, RD,      OPTSW1 },
  { 0x1408,      1, RD,      OPTSW2 },

  { 0,           0, 0,       0 }
};


rom_info2 tempest_roms2 [] =
{
  { "roms/tempest.zip", "136002-133.d1", 0x9000, 0x1000, 0 , 0x1D0CC503},
  { "roms/tempest.zip", "136002-134.f1", 0xa000, 0x1000, 0 , 0xC88E3524},
  { "roms/tempest.zip", "136002-235.j1", 0xb000, 0x1000, 0 , 0xA4B2CE3F},  /* or .235 */
  { "roms/tempest.zip", "136002-136.lm1", 0xc000, 0x1000, 0 , 0x65A9A9F9},
  { "roms/tempest.zip", "136002-237.p1", 0xd000, 0x1000, 0 , 0xDE4E9E34},  /* or .237 */
  { "roms/tempest.zip", "136002-138.np3", 0x3000, 0x1000, 0 , 0x9995256D},
  { NULL,         0,      0,      0 , 0}
};
/*
rom_info tempest_roms [] =
{
  { "roms/Tempest/136002-133.d1", 0x9000, 0x1000, 0 },
  { "roms/Tempest/136002-134.f1", 0xa000, 0x1000, 0 },
  { "roms/Tempest/136002-235.j1", 0xb000, 0x1000, 0 },  / * or .235 * /
  { "roms/Tempest/136002-136.lm1", 0xc000, 0x1000, 0 },
  { "roms/Tempest/136002-237.p1", 0xd000, 0x1000, 0 },  / * or .237 * /
  { "roms/Tempest/136002-138.np3", 0x3000, 0x1000, 0 },
  { NULL,         0,      0,      0 }
};
*/
tag_info tempest_tags [] =
{
  { 0x0000, 0x0800, RD | WR, MEMORY }, /* RAM */
  { 0x0800, 0x0010, RD | WR, COLORRAM },

  { 0x0C00,      1, RD,      COININ },
  { 0x0d00,      1, RD,      OPTSW1 },
  { 0x0e00,      1, RD,      OPTSW2 },

  { 0x2000, 0x1000, RD | WR, VECRAM },

  { 0x4000,      1,      WR, COINOUT },
  { 0x4800,      1,      WR, VGO },
  { 0x5000,      1,      WR, WDCLR },
  { 0x5800,      1,      WR, VGRST },

  { 0x6000,   0x40,      WR, EAROMWR },
  { 0x6040,      1,      WR, EAROMCON },
  { 0x6050,      1, RD,      EAROMRD },

  { 0x6040,      1, RD,      MBSTAT },
  { 0x6060,      1, RD,      MBLO },
  { 0x6070,      1, RD,      MBHI },
  { 0x6080,   0x20,      WR, MBSTART },

  { 0x60C0,   0x10, RD | WR, POKEY1 },
  { 0x60D0,   0x10, RD | WR, POKEY2 },

  { 0x60e0,      1,      WR, TEMP_OUTPUTS },

  { 0,           0, 0,       0 }
};

/*
rom_info battlezone_roms [] =
{
  { "roms/Battlezone/036414.01", 0x5000, 0x0800, 0 },
  { "roms/Battlezone/036413.01", 0x5800, 0x0800, 0 },
  { "roms/Battlezone/036412.01", 0x6000, 0x0800, 0 },
  { "roms/Battlezone/036411.01", 0x6800, 0x0800, 0 },
  { "roms/Battlezone/036410.01", 0x7000, 0x0800, 0 },
  { "roms/Battlezone/036409.01", 0x7800, 0x0800, 0 },
  { "roms/Battlezone/036422.01", 0x3000, 0x0800, 0 },
  { "roms/Battlezone/036421.01", 0x3800, 0x0800, 0 },

  { NULL,   0,      0,           0 }
};
*/
rom_info2 battlezone_roms2 [] =
{
  { "roms/bzone.zip","036414-02.e1", 0x5000, 0x0800, 0, 0x07BF9BF4 },
  { "roms/bzone.zip","036413-01.h1", 0x5800, 0x0800, 0, 0xD36718F8 },
  { "roms/bzone.zip","036412-01.j1", 0x6000, 0x0800, 0, 0x0C98FDDC },
  { "roms/bzone.zip","036411-01.k1", 0x6800, 0x0800, 0, 0xE1DF463A },
  { "roms/bzone.zip","036410-01.lm1", 0x7000, 0x0800, 0,0x095C08DE },
  { "roms/bzone.zip","036409-01.n1", 0x7800, 0x0800, 0, 0x18EBACD6 },
  { "roms/bzone.zip","036422-01.bc3", 0x3000, 0x0800, 0,0x0C2486B0 },
  { "roms/bzone.zip","036421-01.a3", 0x3800, 0x0800, 0, 0x50EA80FA },

  { NULL,   0,      0,           0 ,0}
};

tag_info battlezone_tags [] =
{
  { 0x0000, 0x0400, RD | WR, MEMORY },  /* RAM */

  { 0x0800,      1, RD,      COININ },
  { 0x0a00,      1, RD,      OPTSW1 },
  { 0x0c00,      1, RD,      OPTSW2 },
  { 0x1000,      1,      WR, COINOUT },
  { 0x1200,      1,      WR, VGO },
  { 0x1400,      1,      WR, WDCLR },
  { 0x1600,      1,      WR, VGRST },

  { 0x1800,      1, RD,      MBSTAT },
  { 0x1810,      1, RD,      MBLO },
  { 0x1818,      1, RD,      MBHI },

  { 0x1820,   0x10, RD | WR, POKEY1 },
  { 0x1828,      1, RD,      BZ_INPUTS },
  { 0x1840,      1,      WR, BZ_SOUND },

  { 0x1860,   0x20,      WR, MBSTART },
      
  { 0x2000, 0x1000, RD | WR, VECRAM },

  { 0,           0, 0,       0 }
};

rom_info2 red_baron_roms2 [] =
{
  { "roms/redbaron.zip","037587-01.fh1",  0x4800, 0x0800, 0 , 0x60F23983},
  { "roms/redbaron.zip","037000-01.e1", 0x5000, 0x0800, 0 , 0x870FB62F},
  { "roms/redbaron.zip","037587-01.fh1",  0x5800, 0x0800, 0x0800 , 0x60F23983},
  { "roms/redbaron.zip","036998-01.j1", 0x6000, 0x0800, 0 , 0xF90C45B3},
  { "roms/redbaron.zip","036997-01.k1", 0x6800, 0x0800, 0 , 0xEE85B789},
  { "roms/redbaron.zip","036996-01.lm1", 0x7000, 0x0800, 0, 0xE7829D53 },
  { "roms/redbaron.zip","036995-01.n1", 0x7800, 0x0800, 0 , 0x92DD38A6},
  { "roms/redbaron.zip","037006-01.bc3", 0x3000, 0x0800, 0, 0x890ED1A9},
  { "roms/redbaron.zip","037007-01.a3", 0x3800, 0x0800, 0 , 0xB83994DA},

  { NULL,         0,      0,      0 , 0 }
};

tag_info red_baron_tags [] =
{
  { 0x0000, 0x0400, RD | WR, MEMORY },  /* RAM */

  { 0x0800,      1, RD,      COININ },
  { 0x0a00,      1, RD,      OPTSW1 },
  { 0x0c00,      1, RD,      OPTSW2 },
  { 0x1000,      1,      WR, COINOUT },
  { 0x1200,      1,      WR, VGO },
  { 0x1400,      1,      WR, WDCLR },
  { 0x1600,      1,      WR, VGRST },

  { 0x1800,      1, RD,      MBSTAT },
  { 0x1802,      1, RD,      RB_SW },
  { 0x1804,      1, RD,      MBLO },
  { 0x1806,      1, RD,      MBHI },

  { 0x1808,      1,      WR, RB_SND },
  { 0x180a,      1,      WR, RB_SND_RST },
  { 0x180c,      1,      WR, EAROMCON },

  { 0x1810,   0x10, RD | WR, POKEY1 },
  { 0x1818,      1, RD,      RB_JOY },

  { 0x1820,   0x40,      WR, EAROMWR },
  { 0x1820,   0x40, RD,      EAROMRD },

  { 0x1860,   0x20,      WR, MBSTART },
      
  { 0x2000, 0x1000, RD | WR, VECRAM },

  { 0,           0, 0,       0 }
};

/*
rom_info lunar_lander_roms [] =
{
  { "roms/LunarLander/034572.02", 0x6000, 0x0800, 0 },
  { "roms/LunarLander/034571.02", 0x6800, 0x0800, 0 },
  { "roms/LunarLander/034570.02", 0x7000, 0x0800, 0 },
  { "roms/LunarLander/034569.02", 0x7800, 0x0800, 0 },
  { "roms/LunarLander/034599.01", 0x4800, 0x0800, 0 },
  { "roms/LunarLander/034598.01", 0x5000, 0x0800, 0 },
  { "roms/LunarLander/034597.01", 0x5800, 0x0800, 0 },

  { NULL,        0,      0,      0 }
};
*/
rom_info2 lunar_lander_roms2 [] =
{
  { "roms/llander.zip", "034572-02.f1", 0x6000, 0x0800, 0, 0xEB23BAFE },
  { "roms/llander.zip", "034571-02.de1", 0x6800, 0x0800, 0, 0xABD92475 },
  { "roms/llander.zip", "034570-01.c1", 0x7000, 0x0800, 0, 0x9AE4F8F7 },
  { "roms/llander.zip", "034569-02.b1", 0x7800, 0x0800, 0, 0x8A0E2BEF },
  { "roms/llander.zip", "034599-01.r3", 0x4800, 0x0800, 0, 0x738B681C },
  { "roms/llander.zip", "034598-01.np3", 0x5000, 0x0800, 0, 0x26701CC2 },
  { "roms/llander.zip", "034597-01.m3", 0x5800, 0x0800, 0, 0x08A8F775 }, // newer MAME rom differs in two bytes!

  { NULL,        0,      0,      0, 0 }
};

tag_info lunar_lander_tags [] =
{
  { 0x0000, 0x0100, RD | WR, MEMORY },  /* RAM */
  { 0x0100, 0x0100, RD | WR, LUNAR_MEM },  /* copy of ZP for stack */

  { 0x2000,      1, RD,      LUNAR_SW1 },
  { 0x2400,      8, RD,      LUNAR_SW2 },
  { 0x2800,      4, RD,      OPT1_2BIT },
  { 0x2C00,      1, RD,      LUNAR_POT },

  { 0x3000,      1,      WR, VGO },
  { 0x3200,      1,      WR, LUNAR_OUT },
  { 0x3400,      1,      WR, WDCLR },
  { 0x3800,      1,      WR, DMACNT },
  { 0x3C00,      1,      WR, LUNAR_SND },
  { 0x3E00,      1,      WR, LUNAR_SND_RST },

  { 0x4000, 0x0800, RD | WR, VECRAM },

  /* they try to increment 0x5800 to test for presence of the 
     French/German/Spanish message ROM */
  { 0x5800,      1,      WR, IGNWRT },

  { 0,           0, 0,       0 }
};

rom_info2 asteroids_roms2 [] =
{
  { "roms/asteroid2.zip", "035145-02.ef2", 0x6800, 0x0800, 0, 0x5318BE7F },
  { "roms/asteroid2.zip", "035144-02.h2", 0x7000, 0x0800, 0, 0x49E9BB86 },
  { "roms/asteroid2.zip", "035143-02.j2", 0x7800, 0x0800, 0, 0x1902ADE7 },
  { "roms/asteroid2.zip", "035127-02.np3", 0x5000, 0x0800, 0, 0x526FC45A },
  { NULL,   0,      0,           0, 0 }
};
rom_info2 asteroids_roms3 [] =
{
  { "roms/asteroid2.zip", "035145-02.ef2", 0x6800, 0x0800, 0, 0x5318BE7F },
  { "roms/asteroid2.zip", "035144-02.h2", 0x7000, 0x0800, 0, 0x49E9BB86 },
  { "roms/asteroid.zip", "035143-02.j2", 0x7800, 0x0800, 0, 0x1902ADE7 },
  { "roms/asteroid.zip", "035127-02.np3", 0x5000, 0x0800, 0, 0x526FC45A },
  { NULL,   0,      0,           0, 0 }
};
/*
rom_info asteroids_roms [] =
{
  { "roms/Asteroids/035145.02", 0x6800, 0x0800, 0 },
  { "roms/Asteroids/035144.02", 0x7000, 0x0800, 0 },
  { "roms/Asteroids/035143.02", 0x7800, 0x0800, 0 },

  { "roms/Asteroids/035127.02", 0x5000, 0x0800, 0 },

  { NULL,   0,      0,           0 }
};
*/
tag_info asteroids_tags [] =
{
  { 0x0000, 0x0400, RD | WR, MEMORY },  /* RAM */

  { 0x2000,      8, RD,      ASTEROIDS_SW1 },
  { 0x2400,      8, RD,      ASTEROIDS_SW2 },
  { 0x2800,      4, RD,      OPT1_2BIT },

  /* Asteroids uses an LSR instruction to get bit 0 of some
     of its inputs into the carry flag.  The write may be safely
     ignored, so we mark it as memory.  ELS 920721 */
  { 0x2000,      8,      WR, MEMORY },
  { 0x2400,      8,      WR, MEMORY },
  { 0x2802,      4,      WR, MEMORY },

  { 0x3000,      1,      WR, VGO },
  { 0x3200,      1,      WR, ASTEROIDS_OUT },
  { 0x3400,      1,      WR, WDCLR },
  { 0x3600,      1,      WR, ASTEROIDS_EXP },
  { 0x3800,      1,      WR, DMACNT },
  { 0x3A00,      1,      WR, ASTEROIDS_THUMP },
  { 0x3C00,      6,      WR, ASTEROIDS_SND },
  { 0x3E00,      1,      WR, ASTEROIDS_SND_RST },

  { 0x4000, 0x0800, RD | WR, VECRAM },

  { 0,           0, 0,       0 }
};


rom_info2 asteroidsdx_roms2 [] =
{

  { "roms/astdelux.zip", "036430-02.d1", 0x6000, 0x0800, 0, 0x26E8AFCC },
  {  "roms/astdelux.zip", "036431-02.ef1", 0x6800, 0x0800, 0, 0x441AD34E },
  {  "roms/astdelux.zip", "036432-02.fh1", 0x7000, 0x0800, 0, 0x6412A267 },
  { "roms/astdelux.zip", "036433-03.j1", 0x7800, 0x0800, 0, 0xB851E618 },
  {  "roms/astdelux.zip", "036800-02.r2", 0x4800, 0x0800, 0, 0x42AC00ED },
  {  "roms/astdelux.zip", "036799-01.np2", 0x5000, 0x0800, 0, 0x8098F60E },
  { NULL,          0,      0,      0, 0 }
};

tag_info asteroidsdx_tags [] =
{
  { 0x0000, 0x0400, RD | WR, MEMORY },  /* RAM */
  { 0x2000,      8, RD,      ASTEROIDS_SW1 },
  { 0x2000,      8,      WR, IGNWRT }, /* they use an LSR to read the switches */
  { 0x2400,      8, RD,      ASTEROIDS_SW2 },
  { 0x2400,      8,      WR, IGNWRT }, /* they use an LSR to read the switches */
  { 0x2800,      4, RD,      OPT1_2BIT },
  { 0x2C00,     16, RD | WR, POKEY1 },
  { 0x2C08,      1, RD,      OPTSW2 },
  { 0x2C40,     64, RD,      EAROMRD },
  { 0x3000,      1,      WR, VGO },
  { 0x3200,     64,      WR, EAROMWR }, 
  { 0x3400,      1,      WR, WDCLR },
  { 0x3600,      1,      WR, ASTEROIDS_EXP },
  { 0x3800,      1,      WR, VGRST },
  { 0x3A00,      1,      WR, EAROMCON },
  { 0x3c00,      8,      WR, AST_DEL_OUT },
  { 0x3e00,      1,      WR, ASTEROIDS_SND_RST },
  { 0x4000, 0x0800, RD | WR, VECRAM },

  { 0,           0, 0,       0 }
};


rom_info major_havoc_roms [] =
{
  /* this is copied from Gravitar and hasn't yet been updated! */
  { "roms/MajorHavoc/136025.104", 0x9000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.103", 0xa000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.109", 0xb000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.101", 0xc000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.106", 0xd000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.107", 0xe000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.108", 0x3000, 0x4000, 0 },

  /* vector generator */
  { "roms/MajorHavoc/136010.110", 0x2000, 0x2000, 0 },

  { NULL,   0,      0,      0 }
};

tag_info major_havoc_tags [] =
{
  /* this is copied from Gravitar and hasn't yet been updated! */
  { 0x0000, 0x0800, RD | WR, MEMORY }, /* RAM */

  { 0x2000, 0x0800, RD | WR, VECRAM }, /* vector RAM */

  { 0x6000, 0x0800, RD | WR, POKEY1 },
  { 0x6800, 0x0800, RD | WR, POKEY2 },

  { 0x6008,      1, RD,      OPTSW1 },
  { 0x6808,      1, RD,      OPTSW2 },

  { 0x7000,      4, RD,      EAROMRD },  /* why 4 locs? */

  { 0x7800,      1, RD,      COININ },
  { 0x8000,      1, RD,      GRAVITAR_IN1 },
  { 0x8800,      1,      WR, COINOUT },
  { 0x8800,      1, RD,      GRAVITAR_IN2 },
  { 0x8840,      1,      WR, VGO },
  { 0x8880,      1,      WR, VGRST },
  { 0x88c0,      1,      WR, INTACK },
  { 0x8900,      1,      WR, EAROMCON },
  { 0x8940,   0x40,      WR, EAROMWR },
  { 0x8980,      1,      WR, WDCLR },

  { 0,           0, 0,       0 }
};

extern unsigned long irq_cycle;
void setup_game (void)
{
  tag_area (0x0000, 0x10000, RD | WR, UNKNOWN);

  switch (game)
  {
    case BLACK_WIDOW:
    {
      int error = setup_roms_and_tags2 (black_widow_roms2, black_widow_tags);
      if (error != 0)
      {
        v_message("TROUBLE WITH BLACK WIDOW ROMS!");
      }
      /* copy_rom (0xe000, 0xf000, 0x1000); */
      copy_rom (0xeffa, 0xfffa, 6);

      vector_mem_offset = 0x2000;

#ifdef MAGIC_PC
      mem [0x963c].magic = 1;
      mem [0x98a6].magic = 1;
      mem [0x9a4c].magic = 1;
#endif

      optionreg [0] = 0x02;  /* switch D4, 1..8, off = 0, on = 1 */
      optionreg [1] = 0xcf;  /* switch B4, 1..8, off = 0, on = 1 */
/*
    -------------------------------------------------------------------------------
    Settings of 8-Toggle Switch on Black Widow CPU PCB (at D4)
     8   7   6   5   4   3   2   1   Option
    -------------------------------------------------------------------------------
    Off Off                          1 coin/1 credit <
    On  On                           1 coin/2 credits
    On  Off                          2 coins/1 credit
    Off On                           Free play

            Off Off                  Right coin mechanism x 1 <
            On  Off                  Right coin mechanism x 4
            Off On                   Right coin mechanism x 5
            On  On                   Right coin mechanism x 6

                    Off              Left coin mechanism x 1 <
                    On               Left coin mechanism x 2

                        Off Off Off  No bonus coins (0)* <
                        Off On  On   No bonus coins (6)
                        On  On  On   No bonus coins (7)

                        On  Off Off  For every 2 coins inserted,
                                     logic adds 1 more coin (1)
                        Off On  Off  For every 4 coins inserted,
                                     logic adds 1 more coin (2)
                        On  On  Off  For every 4 coins inserted,
                                     logic adds 2 more coins (3)
                        Off Off On   For every 5 coins inserted,
                                     logic adds 1 more coin (4)
                        On  Off On   For every 3 coins inserted,
                                     logic adds 1 more coin (5)

    -------------------------------------------------------------------------------

    * The numbers in parentheses will appear on the BONUS ADDER line in the
      Operator Information Display (Figure 2-1) for these settings.
    < Manufacturer's recommended setting

    -------------------------------------------------------------------------------
    Settings of 8-Toggle Switch on Black Widow CPU PCB (at B4)
     8   7   6   5   4   3   2   1   Option

    Note: The bits are the exact opposite of the switch numbers - switch 8 is bit 0.
    -------------------------------------------------------------------------------
    Off Off                          Maximum start at level 13
    On  Off                          Maximum start at level 21 <
    Off On                           Maximum start at level 37
    On  On                           Maximum start at level 53

            Off Off                  3 spiders per game <
            On  Off                  4 spiders per game
            Off On                   5 spiders per game
            On  On                   6 spiders per game

                    Off Off          Easy game play
                    On  Off          Medium game play <
                    Off On           Hard game play
                    On  On           Demonstration mode

                            Off Off  Bonus spider every 20,000 points <
                            On  Off  Bonus spider every 30,000 points
                            Off On   Bonus spider every 40,000 points
                            On  On   No bonus

    -------------------------------------------------------------------------------
*/ 
    }
      break;

    case GRAVITAR:
    {
//      setup_roms_and_tags (gravitar_roms, gravitar_tags);
      int error = setup_roms_and_tags2 (gravitar_roms2, gravitar_tags);
      if (error != 0)
      {
        v_message("TROUBLE WITH GRAVITAR ROMS!");
      }

      /* copy_rom (0xe000, 0xf000, 0x1000); */
      copy_rom (0xeffa, 0xfffa, 6);

      vector_mem_offset = 0x2000;

#ifdef MAGIC_PC
      mem [0xe8a6].magic = 1;
      mem [0xcd66].magic = 1;
      /* magicPC3 = 0xeccb; tried this for self-test, doesn't seem to help */
#endif

      optionreg [0] = 0x10;  /* switch D4, 1..8, off = 0, on = 1 */
      optionreg [1] = 0x02;  /* switch B4, 1..8, off = 0, on = 1 */
    }
      break;

    case SPACE_DUEL:
    {
//      setup_roms_and_tags (space_duel_roms, space_duel_tags);
      int error = setup_roms_and_tags2 (space_duel_roms2, space_duel_tags);
      if (error != 0)
      {
        v_message("TROUBLE WITH SPACE DUEL ROMS!");
      }

      /*
      copy_rom (0x8000, 0x9000, 0x1000);
      copy_rom (0x8000, 0xa000, 0x1000);
      copy_rom (0x8000, 0xb000, 0x1000);
      copy_rom (0x8000, 0xc000, 0x1000);
      copy_rom (0x8000, 0xd000, 0x1000);
      copy_rom (0x8000, 0xe000, 0x1000);
      copy_rom (0x8000, 0xf000, 0x1000);
      */
      copy_rom (0x8ffa, 0xfffa, 6);

      vector_mem_offset = 0x2000;
//irq_cycle = 931918;

#ifdef MAGIC_PC
      mem [0x4027].magic = 1;
      mem [0x80ae].magic = 1;
#endif


      optionreg [0] = 0x84; /* switch D4 8..1, off = 0, on = 1 */
      optionreg [1] = 0x02; /* switch B4 8..1, off = 0, on = 1 */
                            /* set to 0x02 for free play */

      optionreg [2] = 0x00; /* switch P10/11 4..2, off = 0, on = 1 */

/*

    -------------------------------------------------------------------------------

    < Manufacturer's recommended setting

    Space Duel Settings
    -------------------

    (Settings of 8-Toggle Switch on Space Duel game PCB at D4)
    Note: The bits are the exact opposite of the switch numbers - switch 8 is bit 0.
     1   2   3   4   5   6   7   8
     8   7   6   5   4   3   2   1       Option
    On  Off                         3 ships per game
    Off Off                         4 ships per game $
    On  On                          5 ships per game
    Off On                          6 ships per game
            On  Off                *Easy game difficulty
            Off Off                 Normal game difficulty $
            On  On                  Medium game difficulty
            Off On                  Hard game difficulty
                    Off Off         English $
                    On  Off         German
                    On  On          Spanish
                    Off On          French
                                    Bonus life granted every:
                            Off On  8,000 points
                            Off Off 10,000 points
                            On  Off 15,000 points
                            On  On  No bonus life

    $Manufacturer's suggested settings
    *Easy-In the beginning of the first wave, 3 targets appear on the
    screen.  Targets increase by one in each new wave.
    Normal-Space station action is the same as 'Easy'.  Fighter action has
    4 targets in the beginning of the first wave.  Targets increase by 2
    in each new wave.  Targets move faster and more targets enter.
    Medium and Hard-In the beginning of the first wave, 4 targets appear
    on the screen.  Targets increase by 2 in each new wave.  As difficulty
    increases, targets move faster, and more targets enter.


    (Settings of 8-Toggle Switch on Space Duel game PCB at B4)
     8   7   6   5   4   3   2   1       Option
    Off On                          Free play
    Off Off                        *1 coin for 1 game (or 1 player) $
    On  On                          1 coin for 2 game (or 2 players)
    On  Off                         2 coins for 1 game (or 1 player)
            Off Off                 Right coin mech x 1 $
            On  Off                 Right coin mech x 4
            Off On                  Right coin mech x 5
            On  On                  Right coin mech x 6
                    Off             Left coin mech x 1 $
                    On              Left coin mech x 2
                        Off Off Off No bonus coins $
                        Off On  Off For every 4 coins, game logic adds 1 more coin
                        On  On  Off For every 4 coins, game logic adds 2 more coin
                        Off On  On  For every 5 coins, game logic adds 1 more coin
                        On  Off On**For every 3 coins, game logic adds 1 more coin

    $Manufacturer's suggested settings

    **In operator Information Display, this option displays same as no bonus.
*/      
    }
      break;

    case TEMPEST:
    {
      init_earom();
//      setup_roms_and_tags (tempest_roms, tempest_tags);

      
//      setup_roms_and_tags (space_duel_roms, space_duel_tags);
      int error = setup_roms_and_tags2 (tempest_roms2, tempest_tags);
      if (error != 0)
      {
        v_message("TROUBLE WITH TEMPEST ROMS!");
      }
      
      
      
      mem [0x11b].tagr = TEMPEST_PROTECTTION_0;
      mem [0x455].tagr = TEMPEST_PROTECTTION_0;
      mem [0x11f].tagr = TEMPEST_PROTECTTION_0;
      mem [0x720].tagr = TEMPEST_PROTECTTION_0;
/*      
00011B  1  xx           copyr_vid_cksum1
000455  1  xx           copyr_vid_cksum2

00011F  1  xx           pokey_piracy_detected
000720  1  xx           pokey_piracy_detected2
*/
      
      
      
      
	  read_rom_image_zip_to("roms/tempest.zip","136002-125.d7", 0, 256, 0, avg_prom);


      vector_mem_offset = 0x2000;
	  avg_init(vector_mem_offset, 0x800);
	  
      
      /* copy_rom (0xc000, 0xe000, 0x2000); */
      copy_rom (0xdffa, 0xfffa, 6);


#ifdef MAGIC_PC
      mem[0xc7a7].magic = 1;
#endif

      
      
      optionreg [0] = 0x02;//N13 INVERTED and backwards
      optionreg [1] = 0x00;//L12 INVERTED and backwards
/*
   
    GAME OPTIONS:
    (8-position switch at N13 on Analog Vector-Generator PCB)

    1   2   3   4   5   6   7   8   Meaning
    -------------------------------------------------------------------------
    Off Off                         2 lives per game
    On  On                          3 lives per game
    On  Off                         4 lives per game
    Off On                          5 lives per game
            On  On  Off             Bonus life every 10000 pts
            On  On  On              Bonus life every 20000 pts
            On  Off On              Bonus life every 30000 pts
            On  Off Off             Bonus life every 40000 pts
            Off On  On              Bonus life every 50000 pts
            Off On  Off             Bonus life every 60000 pts
            Off Off On              Bonus life every 70000 pts
            Off Off Off             No bonus lives
                        On  On      English
                        On  Off     French
                        Off On      German
                        Off Off     Spanish
                                On  1-credit minimum
                                Off 2-credit minimum

 PRICING OPTIONS:
    (8-position switch at L12 on Analog Vector-Generator PCB)
    1   2   3   4   5   6   7   8   Meaning
    -------------------------------------------------------------------------
    On  On  On                      No bonus coins
    On  On  Off                     For every 2 coins, game adds 1 more coin
    On  Off On                      For every 4 coins, game adds 1 more coin
    On  Off Off                     For every 4 coins, game adds 2 more coins
    Off On  On                      For every 5 coins, game adds 1 more coin
    Off On  Off                     For every 3 coins, game adds 1 more coin
    On  Off                 Off On  Demonstration Mode (see notes)
    Off Off                 Off On  Demonstration-Freeze Mode (see notes)
                On                  Left coin mech * 1
                Off                 Left coin mech * 2
                    On  On          Right coin mech * 1
                    On  Off         Right coin mech * 4
                    Off On          Right coin mech * 5
                    Off Off         Right coin mech * 6
                            Off On  Free Play
                            Off Off 1 coin 2 plays
                            On  On  1 coin 1 play
                            On  Off 2 coins 1 play * 
 * 
    GAME OPTIONS:
    (4-position switch at D/E2 on Math Box PCB)

    1   2   3   4                   Meaning
    -------------------------------------------------------------------------
        Off                         Minimum rating range: 1, 3, 5, 7, 9
        On                          Minimum rating range tied to high score
            Off Off                 Medium difficulty (see notes)
            Off On                  Easy difficulty (see notes)
            On  Off                 Hard difficulty (see notes)
            On  On                  Medium difficulty (see notes)



*/
      portrait = 1;
    }
      break;

    case BATTLEZONE:
    {
//      setup_roms_and_tags (battlezone_roms, battlezone_tags);
      int error = setup_roms_and_tags2 (battlezone_roms2, battlezone_tags);
      if (error != 0)
      {
        v_message("TROUBLE WITH BATTLE ZONE ROMS!");
      }
      copy_rom (0x7ffa, 0xfffa, 6);

      vector_mem_offset = 0x2000;

#ifdef MAGIC_PC
      mem [0x5000].magic = 1;
#endif

      optionreg [0] = 0x15; /* M10 8..1 */
      optionreg [1] = 0x60; /* P10 8..1 */

      use_nmi = 1;
    }
      break;

    case RED_BARON:
    {

//      setup_roms_and_tags (red_baron_roms, red_baron_tags);
      int error = setup_roms_and_tags2 (red_baron_roms2, red_baron_tags);
      if (error != 0)
      {
        v_message("TROUBLE WITH RED BARON ROMS!");
      }
      
      

      copy_rom (0x7ffa, 0xfffa, 6);

      vector_mem_offset = 0x2000;

      optionreg [0] = 0xff; /* M10 8..1 coins/credit */
      optionreg [1] = 0xeb; /* P10 8..1 language/# planes/bonus points*/
/*
 * 
    RED BARON DIP SWITCH SETTINGS
    Donated by Dana Colbert


    $=Default
    "K" = 1,000

    Switch at position P10
                                      8    7    6    5    4    3    2    1
                                    _________________________________________
    English                        $|    |    |    |    |    |    |Off |Off |
    Spanish                         |    |    |    |    |    |    |Off | On |
    French                          |    |    |    |    |    |    | On |Off |
    German                          |    |    |    |    |    |    | On | On |
                                    |    |    |    |    |    |    |    |    |
     Bonus airplane granted at:     |    |    |    |    |    |    |    |    |
    Bonus at 2K, 10K and 30K        |    |    |    |    |Off |Off |    |    |
    Bonus at 4K, 15K and 40K       $|    |    |    |    |Off | On |    |    |
    Bonus at 6K, 20K and 50K        |    |    |    |    | On |Off |    |    |
    No bonus airplanes              |    |    |    |    | On | On |    |    |
                                    |    |    |    |    |    |    |    |    |
    2 aiplanes per game             |    |    |Off |Off |    |    |    |    |
    3 airplanes per game           $|    |    |Off | On |    |    |    |    |
    4 airplanes per game            |    |    | On |Off |    |    |    |    |
    5 airplanes per game            |    |    | On | On |    |    |    |    |
                                    |    |    |    |    |    |    |    |    |
    1-play minimum                 $|    |Off |    |    |    |    |    |    |
    2-play minimum                  |    | On |    |    |    |    |    |    |
                                    |    |    |    |    |    |    |    |    |
    Self-adj. game difficulty: on  $|Off |    |    |    |    |    |    |    |
    Self-adj. game difficulty: off  | On |    |    |    |    |    |    |    |
                                    -----------------------------------------

      If self-adjusting game difficulty feature is
    turned on, the program strives to maintain the
    following average game lengths (in seconds):

                                            Airplanes per game:
         Bonus airplane granted at:          2   3     4     5
    2,000, 10,000 and 30,000 points         90  105$  120   135
    4,000, 15,000 and 40,000 points         75   90   105   120
    6,000, 20,000 and 50,000 points         60   75    90   105
                 No bonus airplanes         45   60    75    90



    Switch at position M10
                                      8    7    6    5    4    3    2    1
                                    _________________________________________
        50  PER PLAY                |    |    |    |    |    |    |    |    |
     Straight 25  Door:             |    |    |    |    |    |    |    |    |
    No Bonus Coins                  |Off |Off |Off |Off |Off |Off | On | On |
    Bonus $1= 3 plays               |Off | On | On |Off |Off |Off | On | On |
    Bonus $1= 3 plays, 75 = 2 plays |Off |Off | On |Off |Off |Off | On | On |
                                    |    |    |    |    |    |    |    |    |
     25 /$1 Door or 25 /25 /$1 Door |    |    |    |    |    |    |    |    |
    No Bonus Coins                  |Off |Off |Off |Off |Off | On | On | On |
    Bonus $1= 3 plays               |Off | On | On |Off |Off | On | On | On |
    Bonus $1= 3 plays, 75 = 2 plays |Off |Off | On |Off |Off | On | On | On |
                                    |    |    |    |    |    |    |    |    |
        25  PER PLAY                |    |    |    |    |    |    |    |    |
     Straight 25  Door:             |    |    |    |    |    |    |    |    |
    No Bonus Coins                  |Off |Off |Off |Off |Off |Off | On |Off |
    Bonus 50 = 3 plays              |Off |Off | On |Off |Off |Off | On |Off |
    Bonus $1= 5 plays               |Off | On |Off |Off |Off |Off | On |Off |
                                    |    |    |    |    |    |    |    |    |
     25 /$1 Door or 25 /25 /$1 Door |    |    |    |    |    |    |    |    |
    No Bonus Coins                  |Off |Off |Off |Off |Off | On | On |Off |
    Bonus 50 = 3 plays              |Off |Off | On |Off |Off | On | On |Off |
    Bonus $1= 5 plays               |Off | On |Off |Off |Off | On | On |Off |
                                    -----------------------------------------

    Switch at position L11
                                                          1    2    3    4
                                                        _____________________
    All 3 mechs same denomination                       | On | On |    |    |
    Left and Center same, right different denomination  | On |Off |    |    |
    Right and Center same, left different denomination  |Off | On |    |    |
    All different denominations                         |Off |Off |    |    |
                                                        ---------------------


    2008-07
    Dip locations added from the notes above (factory settings for bzone
    from the manual).
 * */
      use_nmi = 1;
    }
      break;

    case LUNAR_LANDER:
    {
//      setup_roms_and_tags (lunar_lander_roms, lunar_lander_tags);
      
      
      int error = setup_roms_and_tags2 (lunar_lander_roms2, lunar_lander_tags);
      if (error != 0)
      {
        v_message("TROUBLE WITH LUNAR LANDER ROMS!");
      }
      
      
      copy_rom (0x7ffa, 0xfffa, 6);

      vector_mem_offset = 0x4000;

      /* they try to increment 0x5800 to test for presence of the 
	 French/German/Spanish message ROM */
      /*
      mem [0x5800].cell = 0xff;
      */

#ifdef MAGIC_PC
      mem [0x652d].magic = 1;
#endif

      optionreg [0] = 0xff-12;
/*
 * 
    Lunar Lander settings:

    0 = OFF  1 = ON  x = Don't Care  $ = Atari suggests


    8 SWITCH DIP (P8) with -01 ROMs on PCB
    87654321
    --------
    11xxxxxx   450 fuel units per coin
    10xxxxxx   600 fuel units per coin
    01xxxxxx   750 fuel units per coin  $
    00xxxxxx   900 fuel units per coin
    xxx0xxxx   Free play
    xxx1xxxx   Coined play as determined by toggles 7 & 8  $
    xxxx00xx   German instructions
    xxxx01xx   Spanish instructions
    xxxx10xx   French instructions
    xxxx11xx   English instructions  $
    xxxxxx11   Right coin == 1 credit/coin  $
    xxxxxx10   Right coin == 4 credit/coin
    xxxxxx01   Right coin == 5 credit/coin
    xxxxxx00   Right coin == 6 credit/coin
               (Left coin always registers 1 credit/coin)


    8 SWITCH DIP (P8) with -02 ROMs on PCB
    87654321
    --------
    11x1xxxx   450 fuel units per coin
    10x1xxxx   600 fuel units per coin
    01x1xxxx   750 fuel units per coin  $
    00x1xxxx   900 fuel units per coin
    11x0xxxx   1100 fuel units per coin
    10x0xxxx   1300 fuel units per coin
    01x0xxxx   1550 fuel units per coin
    00x0xxxx   1800 fuel units per coin
    xx0xxxxx   Free play
    xx1xxxxx   Coined play as determined by toggles 5, 7, & 8  $
    xxxx00xx   German instructions
    xxxx01xx   Spanish instructions
    xxxx10xx   French instructions
    xxxx11xx   English instructions  $
    xxxxxx11   Right coin == 1 credit/coin  $
    xxxxxx10   Right coin == 4 credit/coin
    xxxxxx01   Right coin == 5 credit/coin
    xxxxxx00   Right coin == 6 credit/coin
               (Left coin always registers 1 credit/coin)

 */
      dvg = 1;
      use_nmi = 1;
    }
      break;

    case ASTEROIDS:
    {
//      setup_roms_and_tags (asteroids_roms, asteroids_tags);
      int error = setup_roms_and_tags2 (asteroids_roms2, asteroids_tags);
      if (error) error = setup_roms_and_tags2 (asteroids_roms3, asteroids_tags);
      if (error != 0)
      {
        v_message("TROUBLE WITH ASTEROIDS ROMS!");
      }
      
      
      copy_rom (0x7ffa, 0xfffa, 6);

      vector_mem_offset = 0x4000;

#ifdef MAGIC_PC
      mem [0x680c].magic = 1;
#endif

      optionreg [0] = 0x00;

      dvg = 1;
      use_nmi = 1;
    }
      break;

    case ASTEROIDS_DX:
    {
//      setup_roms_and_tags (asteroidsdx_roms, asteroidsdx_tags);
      int error = setup_roms_and_tags2 (asteroidsdx_roms2, asteroidsdx_tags);
      if (error != 0)
      {
        v_message("TROUBLE WITH ASTEROIDS DELUXE ROMS!");
      }

      copy_rom (0x7ffa, 0xfffa, 6);

      vector_mem_offset = 0x4000;

#ifdef MAGIC_PC
      mem [0x601c].magic = 1;
      mem [0x601e].magic = 1;
#endif

      optionreg [0] = (~0xD3) & 0xff;

      dvg = 1;
      use_nmi = 1;

#ifdef OLD_AD
      if (mem [0x7cf8].cell != 0x9d)
	{
	  fprintf (stderr, "Bad opcode at 7cf8!\n");
	  mem [0x7cf8].cell = 0x9d;
	}

      if (mem [0x7d15].cell != 0xc9)
	{
	  fprintf (stderr, "Bad branch offset at 7d15!\n");
	  mem [0x7d15].cell = 0xc9;
	}
#endif
    }
      break;

    default:
      fprintf (stderr, "ERROR: Unknown game\n");
      exit (1);
    }
}
