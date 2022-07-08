/*------------------------------------------------------------------
  defines.h: Game Constants

    XINVADERS 3D - 3d Shoot'em up
    Copyright (C) 2000 Don Llopis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

------------------------------------------------------------------*/
#ifndef GAME_DEFINES
#define GAME_DEFINES


#define REFERENCE_FRAMERATE 30.0 /* 30 fps */

enum color_names 
{
   BLACK=0, 
   RED=63,
   GREEN=127, 
   BLUE=191, 
   YELLOW=192,
   WHITE=255
};

enum zone_enum
{
   ZONE_0 = 0,
   ZONE_1 = 1,
   ZONE_2 = 2,
   ZONE_3 = 3,
   ZONE_4 = 4,
   ZONE_5 = 5,
   ZONE_6 = 6,
   ZONE_7 = 7,
   ZONE_8 = 8,
   ZONE_9 = 9,
   ZONE_MAX        = 10,
   ZONE_HEIGHT_MAX = 5,
   ZONE_WIDTH      = 100,
   ZONE_HEIGHT     = 100
};

enum game_enum
{
   LOGO_TIME          = 10000,  /* 10 sec */
   READY_TIME         = 3000,   /* 3  sec */
   GAMEOVER_TIME      = 15000,  /* 15 sec */
   PLAYER_LIVES_START = 3,
   HISCORE            = 10000,  /* 10,000 pts */
   UFO_BONUS          = 500,    /* 500 pts */
   PLAYER_LIFE_BONUS  = 7500,   /* every 7500pts */
   PLAYER_LIFE_MAX    = 5,
   INITGAME           = 0,
   NEWGAME            = 1,
   NEWLEVEL           = 2,
   XPOS               = 0,
   YPOS               = 1,
   ZPOS               = 2,
   MAX_OBJECTS        = 70 /* was: 100 */
};

/* TRUE and FALSE defined using #ifdefs instead, and
	only if they aren't defined already. Please stick to this,
	as the include files needed for timers on at least one
	platform define TRUE and FALSE, resulting in an error
	if they are part of an enumeration. TBB */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

#endif
