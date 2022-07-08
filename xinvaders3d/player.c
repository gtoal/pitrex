/*------------------------------------------------------------------
  player.c:

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
#include "game.h"

/*================================================================*/

#define PLAYER_RADIUS          20.0f
#define PLAYER_RADIUS_SQUARED  400.0f
#define MISSLE_RADIUS          20.0f
#define MISSLE_RADIUS_SQUARED  400.0f

#define MAX_X 550.0f
#define MIN_X -550.0f
#define MAX_Y 450.0f
#define MIN_Y 50.0f

#define ROT_Z 0.087266463    /* 5 degrees per frame */
#define MAX_ROT 0.785398163  /* max of 45 degrees */

enum player_enum
{
   PLAYER_BLINK        = TRUE,
   PLAYER_BLINK_TIME   = 5000  /* 5.0 sec */
};

OBJECT player_1, *player;
OBJECT player_missile, *pm;

static VECTOR4 start_pos      = { 0.0f, 250.0f, -25.0f, 1.0f };
static VECTOR4 start_dir      = { 0.0f, 0.0f, -1.0f, 1.0f };
static VECTOR4 thrust_up      = { 0.0f, 10.0f, 0.0f, 1.0f };
static VECTOR4 thrust_down    = { 0.0f, -10.0f, 0.0f, 1.0f };
static VECTOR4 thrust_left    = { -10.0f, 0.0f, 0.0f, 1.0f };
static VECTOR4 thrust_right   = { 10.0f, 0.0f, 0.0f, 1.0f };
static VECTOR4 missile_offset = { 0.0f, 0.0f, 0.0f, 1.0f };
static VECTOR4 missile_thrust = { 0.0f, 0.0f, -50.0f, 1.0f };

static int pcolor; /* Player color */

/*================================================================*/

void Player_init ( void )
{
   gv->pblinking = TRUE;
   
   player = &player_1;

   Object_init ( player );
   Object_set_actionfn ( player, Player_update );
   Object_set_drawfn  ( player, Player_blink );

   Object_set_pos ( player, start_pos );
   Object_set_dir ( player, start_dir );

   player->active         = TRUE;
   player->radius         = PLAYER_RADIUS;
   player->radius_squared = PLAYER_RADIUS_SQUARED;
   player->frame          = 0;
   player->rot            = 0.0;
   pcolor                 = RED;
   Object_update_zone ( player );

   pm = &player_missile;
   Object_init         ( pm );
   Object_set_actionfn ( pm, Player_missile_update );
   Object_set_drawfn   ( pm, Player_missile_draw );

   pm->radius         = MISSLE_RADIUS;
   pm->radius_squared = MISSLE_RADIUS_SQUARED;
}

/*================================================================*/

void Player_update ( OBJECT *obj )
{
   if ( gv->key_UP )
   {
      obj->pos[YPOS] += thrust_up[YPOS] * gv->fadjust;
      Object_update_zone ( obj );
      if ( obj->pos[YPOS] > MAX_Y )
         obj->pos[YPOS] = MAX_Y;
   }

   if ( gv->key_DOWN )
   {
      obj->pos[YPOS] += thrust_down[YPOS] * gv->fadjust;
      Object_update_zone ( obj );
      if ( obj->pos[YPOS] < MIN_Y )
         obj->pos[YPOS] = MIN_Y;
   }

   if ( gv->key_LEFT )
   {
      obj->pos[XPOS] += thrust_left[XPOS] * gv->fadjust;
      player->rot += ROT_Z * gv->fadjust;
      
      if ( obj->pos[XPOS] < MIN_X )
         obj->pos[XPOS] = MIN_X;
      
      if ( player->rot > MAX_ROT )
         player->rot = MAX_ROT;
   }

   if ( gv->key_RIGHT )
   {
      obj->pos[XPOS] += thrust_right[XPOS] * gv->fadjust;
      player->rot -= ROT_Z * gv->fadjust;
      
      if ( obj->pos[XPOS] > MAX_X )
         obj->pos[XPOS] = MAX_X;

      if ( player->rot < -MAX_ROT )
         player->rot = -MAX_ROT;
   }

   if ( (gv->key_LEFT == FALSE) && (gv->key_RIGHT == FALSE) )
   {
      /* please re-do this major hackyness */
      if ( ( player->rot < -0.03 ) ||
            ( player->rot > 0.03 ) )
      {
         if ( player->rot < -0.03 )
         {
            player->rot += ROT_Z * gv->fadjust;
            if ( player->rot > 0.0 )
               player->rot = 0.0;
         }
         else
         {
            if ( player->rot > 0.03 )
            {
               player->rot -= ROT_Z * gv->fadjust;
               if ( player->rot < 0.0 )
                  player->rot = 0.0;
            }
         }
      }
   }

   if ( gv->key_FIRE && !pm->active )
   {
      pm->active      = TRUE;
      pm->zone        = obj->zone;
      pm->zheight     = obj->zheight;

      Vector_init ( pm->pos );
      Vector_copy ( obj->pos, pm->pos );
      Vector_copy ( obj->pos, pm->old_pos );
      Vector_addd ( pm->pos, missile_offset );
   }
}


void Player_blink ( OBJECT *obj, MATRIX4 r )
{
   static long blink = 0;
   static int color = 0;

   blink += gv->msec;
   if ( blink > 50 )
   {
      blink -= 250;
      color++;
      if ( color == 3 )
         color = 0;
   }

   pcolor = RED - ( color * 15 );

   obj->frame += gv->msec;
   if ( obj->frame > PLAYER_BLINK_TIME )
   {
      pcolor        = RED;
      gv->pblinking = FALSE;
      Object_set_drawfn ( obj, Player_draw );
   }

   Player_draw ( obj, r );
}

void Player_draw ( OBJECT *obj, MATRIX4 r )
{
   int p[32];
   VECTOR4 tmp[16], rot_vert[16];
   MATRIX4 tmp_mat, rot_mat;
   VECTOR4 offset;


   VECTOR4 player_vert [16] = { { -5.0f, 0.0f, 15.0f, 1.0f },  /*body*/
                             { -10.0f, 0.0f, -20.0f, 1.0f },
                             { 10.0f, 0.0f, -20.0f, 1.0f },
                             { 5.0f, 0.0f, 15.0f, 1.0f },
                             { -5.0f, 10.0f, -18.0f, 1.0f },
                             { 5.0f, 10.0f, -18.0f, 1.0f }, 
                             { -5.0f, 0.0f, 20.0f, 1.0f }, /* left e*/
                             { -10.0f, 0.0f, -15.0f, 1.0f },
                             { -12.0f, 0.0f, -12.0f, 1.0f },
                             { -10.0f, 0.0f, 25.0f, 1.0f }, 
                             { 5.0f, 0.0f, 20.0f, 1.0f }, /* re */
                             { 10.0f, 0.0f, -15.0f, 1.0f },
                             { 12.0f, 0.0f, -12.0f, 1.0f },
                             { 10.0f, 0.0f, 25.0f, 1.0f },
                             { -20.0f, -5.0f, 35.0f, 1.0f }, /*lw*/
                             { 20.0f, -5.0f, 35.0f, 1.0f } }; /*rw*/



   VECTOR4 cross_hairs[4] = { { -15.0f, 0.0f, 0.0f, 1.0f },
                           { 0.0f, 15.0f, 0.0f, 1.0f },
                           { 15.0f, 0.0f, 0.0f, 1.0f },
                           { 0.0f, -15.0f, 0.0f, 1.0f } };

   Matrix_vec_mult ( r, obj->pos, tmp[0] );
   Matrix_copy ( r, tmp_mat );
   Matrix_set_trans ( tmp_mat, tmp[0] );

   Matrix_z_rot ( rot_mat, player->rot );
   Matrix_vec_multn ( rot_mat, player_vert, rot_vert, 16 );
   Matrix_vec_multn ( tmp_mat, rot_vert, tmp, 16 );
   Camera_project_points ( tmp, p, 16 );

   /* body bottom */
   Draw_line ( p[0], p[1], p[2], p[3], pcolor );
   Draw_line ( p[2], p[3], p[4], p[5], pcolor );
   Draw_line ( p[4], p[5], p[6], p[7], pcolor );
   Draw_line ( p[6], p[7], p[0], p[1], pcolor );
   
   /* body top */
   Draw_line ( p[0], p[1], p[8], p[9], pcolor );
   Draw_line ( p[8], p[9], p[2], p[3], pcolor );
   Draw_line ( p[4], p[5], p[10], p[11], pcolor );
   Draw_line ( p[10], p[11], p[6], p[7], pcolor );
   Draw_line ( p[8], p[9], p[10], p[11], pcolor );

   /* left e*/
   Draw_line ( p[12], p[13], p[14], p[15], pcolor );
   Draw_line ( p[14], p[15], p[16], p[17], pcolor );
   Draw_line ( p[16], p[17], p[18], p[19], pcolor );
   Draw_line ( p[18], p[19], p[12], p[13], pcolor );

   /* right e */
   Draw_line ( p[20], p[21], p[22], p[23], pcolor );
   Draw_line ( p[22], p[23], p[24], p[25], pcolor );
   Draw_line ( p[24], p[25], p[26], p[27], pcolor );
   Draw_line ( p[26], p[27], p[20], p[21], pcolor );

   /* left wing */
   Draw_line ( p[28], p[29], p[16], p[17], pcolor );
   Draw_line ( p[28], p[29], p[18], p[19], pcolor );

   /* right wing */
   Draw_line ( p[30], p[31], p[24], p[25], pcolor );
   Draw_line ( p[30], p[31], p[26], p[27], pcolor );

  /* update cross-hairs */ 
   Vector_copy ( obj->pos, tmp[0] );
   Vector_init ( offset );
   offset[ZPOS] = -((gv->formation_zone * 100.0f) + 50.0f);
   
   Vector_addd ( tmp[0], offset );
   Matrix_vec_mult ( r, tmp[0], tmp[1] );
   Matrix_copy ( r, tmp_mat );
   Matrix_set_trans ( tmp_mat, tmp[1] );

   Matrix_vec_multn ( rot_mat, cross_hairs, rot_vert, 4 );
   Matrix_vec_multn ( tmp_mat, rot_vert, tmp, 4 );
   Camera_project_points ( tmp, p, 4 );
   Draw_line ( p[0], p[1], p[2], p[3], WHITE );
   Draw_line ( p[2], p[3], p[4], p[5], WHITE );
   Draw_line ( p[4], p[5], p[6], p[7], WHITE );
   Draw_line ( p[6], p[7], p[0], p[1], WHITE );
}

/*================================================================*/

void Player_missile_update ( OBJECT *obj )
{
   Vector_copy ( obj->pos, obj->old_pos );
   obj->pos[ZPOS] += missile_thrust[ZPOS] * gv->fadjust; 
   Object_update_zone ( obj );

   if ( obj->zone > ZONE_9 )
      obj->active = FALSE;
}

void Player_missile_draw ( OBJECT *obj, MATRIX4 r )
{
   VECTOR4 tmp[4];
   MATRIX4 tmp_mat;
   int p[8];

   VECTOR4 missile_vert[4] = { {-10.0f, 0.0f, -5.0f, 1.0f},
                            {0.0f, 0.0f, -10.0f, 1.0f},
                            {10.0f, 0.0f, -5.0f, 1.0f},
                            {0.0f, 0.0f, 20.0f, 1.0f}};

   Matrix_vec_mult  ( r, obj->pos, tmp[0] );
   Matrix_copy      ( r, tmp_mat );
   Matrix_set_trans ( tmp_mat, tmp[0] );

   Matrix_vec_multn ( tmp_mat, missile_vert, tmp, 4 );

   Camera_project_points ( tmp, p, 4 );

   Draw_line ( p[0], p[1], p[2], p[3], YELLOW );
   Draw_line ( p[2], p[3], p[4], p[5], YELLOW );
   Draw_line ( p[4], p[5], p[6], p[7], YELLOW );
   Draw_line ( p[6], p[7], p[0], p[1], YELLOW );
}

/*================================================================*/
