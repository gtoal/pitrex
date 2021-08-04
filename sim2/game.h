/*
 * game.h: Atari Vector game definitions & setup functions
 *
 * Copyright 1991, 1992, 1993, 1996 Hedley Rainnie and Eric Smith
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
 * $Id: game.h 2 2003-08-20 01:51:05Z eric $
 */

extern int game;

/* The following are B&W with the DVG: */
#define LUNAR_LANDER 1
#define ASTEROIDS 2
#define ASTEROIDS_DX 3
/* B&W with AVG: */
#define RED_BARON 4
#define BATTLEZONE 5  /* 2901 math box */
/* All the rest are color with AVG: */
#define TEMPEST 6     /* 2901 math box */
#define SPACE_DUEL 7
#define GRAVITAR 8
#define BLACK_WIDOW 9
#define MAJOR_HAVOC 10  /* extra 6502 for sound, quad POKEY */
/* The following use two 6809s, a math box, and four POKEYs: */
#define STAR_WARS 11
#define EMPIRE 12
/* The following uses a 68000: */
#define QUANTUM 13

#define FIRST_GAME LUNAR_LANDER
#define LAST_GAME MAJOR_HAVOC /* we don't do 6809 or 68000 (yet) */

extern int use_nmi;  /* set true to generate NMI instead of IRQ */

int pick_game (char *name);
void show_games (void);
char *game_name (int game);
void setup_game (void);

extern void (*gameCallback)(int);
