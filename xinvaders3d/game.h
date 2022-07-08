/*------------------------------------------------------------------
  game.h: Game main header

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
#ifndef GAME_MAIN_HEADER
#define GAME_MAIN_HEADER

/*------------------------------------------------------------------
 *
 * Standard Includes
 *
------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <math.h>

/*------------------------------------------------------------------
 *
 * Vector, Matrix, and Camera functions
 *
------------------------------------------------------------------*/
#include "vec4x1.h"
#include "mat4x4.h"
#include "camera.h"

/*------------------------------------------------------------------
 *
 * System specific Functions, Graphics primitives, and Timming
 *
------------------------------------------------------------------*/

#include "system.h"

/*------------------------------------------------------------------
 *
 * Game Defines 
 *
------------------------------------------------------------------*/

#include "defines.h"

/*------------------------------------------------------------------
 *
 * Object Managment Struct & Funtions  : object.c 
 *
------------------------------------------------------------------*/

#include "object.h"

/*------------------------------------------------------------------
 *
 * Game-Logic Funtions : game.c
 *
------------------------------------------------------------------*/

extern void Game_main ( void );

/* initialization functions */
extern int  Game_init      ( float, float );
extern void Game_init_vars ( int );
extern void Game_init_keys ( void );

/* Game action function(s) */
extern void (*Game_actionfn)();
extern void Game_newlevel      ( void );
extern void Game_gameover      ( void );
extern void Game_paused_toggle ( void );
extern void Game_paused        ( void );
extern void Game_reset         ( void );

extern void Game_menu    ( void );
extern void Game_ready   ( void );
extern void Game_run     ( void );
extern void Game_overlay ( void );

/* misc game related functions */
extern void Object_update_zone ( OBJECT *obj );

/*------------------------------------------------------------------
 *
 * Player's Functions  : player.c
 *
------------------------------------------------------------------*/
extern void Player_init    ( void );
extern void Player_blink   ( OBJECT *, MATRIX4 );
extern void Player_draw    ( OBJECT *, MATRIX4 );
extern void Player_update  ( OBJECT * );

extern void Player_missile_update  ( OBJECT * );
extern void Player_missile_draw    ( OBJECT *, MATRIX4 );

/*------------------------------------------------------------------
 *
 * Aliens' Functions  : aliens.c
 *
------------------------------------------------------------------*/
extern void Aliens_init        ( void );
extern void Aliens_move        ( void );
extern void Aliens_update      ( OBJECT * );
extern void Aliens_draw        ( OBJECT *, MATRIX4 );
extern void Alien_missile_draw ( OBJECT *, MATRIX4 );
extern void Ufo_spawn          ( void );
extern void Ufo_draw           ( OBJECT *, MATRIX4 );

/*------------------------------------------------------------------
 *
 * Special-Effects Functions  : effects.c
 *
------------------------------------------------------------------*/
/* stars */
extern void Stars_init ( void );
extern void Stars_draw ( MATRIX4 );

/* explosions */
extern void Explosions_add   ( OBJECT * );
extern void Explosions_clear ( void );
extern void Explosions_draw  ( MATRIX4 );
extern int  Explosions_count ( void );

/* jump-gate */
extern void Jumpgate_init    ( void );
extern void Jumpgate_open    ( VECTOR4, int );
extern void Jumpgate_animate ( MATRIX4 );

/* one-up */
extern void One_up_init ( void );
extern void One_up_add  ( OBJECT * );
extern void One_up_draw ( MATRIX4 );

/*------------------------------------------------------------------
 *
 * Game Variables
 *
------------------------------------------------------------------*/

#include "externs.h"

#endif
