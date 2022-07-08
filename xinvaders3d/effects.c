/*------------------------------------------------------------------
  effects.c:

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

/*------------------------------------------------------------------
 * Stars
 *  
 *
------------------------------------------------------------------*/

enum stars_enum
{
   MAX_STARS = 100
};

struct STARSSTRUCT
{
   VECTOR4      pos;
   VECTOR4      thrustz;
   int          thrust;
   unsigned int color;
} stars[MAX_STARS];

VECTOR4 star_start     = { 0.0f, 0.0f, -2000.0f, 1.0f };
VECTOR4 star_thrust[4] = {{ 0.0f, 0.0f, 50.0f, 1.0f },
                         { 0.0f, 0.0f, 25.0f, 1.0f },
                         { 0.0f, 0.0f, 15.0f, 1.0f },
                         { 0.0f, 0.0f, 10.0f, 1.0f } };
                 

void Stars_init ( void )
{
   int i, j, k, l, m;
   for ( i=0; i<MAX_STARS; i++ )
   {
      m = rand () % 4;
      j = rand () % 800;
      k = rand () % 800;
      l = rand () % 1000;
      Vector_copy ( star_start, stars[i].pos );

      stars[i].pos[XPOS] = (float)j;
      stars[i].pos[YPOS] = (float)k;

      if ( (i % 3) == 0 )
      {
         stars[i].pos[XPOS] *= -1.0f;
         if ( (i%2) == 0 )
         {
            stars[i].pos[YPOS] *= -1.0f;
         }
      }
      
      stars[i].thrust = m;
      stars[i].color = WHITE - (m*15);
   }
}

void Stars_draw ( MATRIX4 r )
{
   VECTOR4 tmp;
   int i, j, m, p[2];

   for ( i=0; i<MAX_STARS; i++ )
   {
      j = stars[i].thrust;
      stars[i].pos[ZPOS] += star_thrust[j][ZPOS] * gv->fadjust; 
      
      if ( stars[i].pos[ZPOS] > -1.0f )
      {
         m = rand() % 4;
         stars[i].thrust = m;
         stars[i].color = WHITE - (m*15);
         stars[i].pos[ZPOS] = -2000.0f;
      }
      Matrix_vec_mult ( r, stars[i].pos, tmp );
      Camera_project_point ( tmp, p );
      Draw_point ( p[0], p[1], stars[i].color );
   }
}

/*------------------------------------------------------------------
 * Explosions
 *  
 *
------------------------------------------------------------------*/

#define EXPLOSION_ROT 0.23f

enum explosions_enum
{
   MAX_EXPLOSIONS        = 10,
   MAX_PARTICLES         = 4,
   EXPLOSIONS_LIFE       = 1500, /* 1.5 sec */
   EXPLOSION_BLEND_TIME  = 375,  /* 0.375 sec */
   EXPLOSION_COLOR_INC   = 15
};

struct EXPLOSIONSTRUCT 
{
   VECTOR4  pos[MAX_PARTICLES];
   int  thrust[MAX_PARTICLES];
   long frame;
   long blend;
   int  color;
   int  active;
} explosions[MAX_EXPLOSIONS];

static int   ecur;   /* EXPLOSION index */
static int   ecount; /* EXPLOSION count */
static int   pcur;   /* THURST index */
static float erot;   /* EXPLOSIONS rotation */

/* shard/particle thrust vectors */
VECTOR4 pthrust[8] = { {-2.0f, 0.0f, 0.0f, 1.0f},
                      {0.0f, -2.0f, 0.0f, 1.0f},
                      {2.0f, 0.0f, 0.0f, 1.0f},
                      {0.0f, 2.0f, 0.0f, 1.0f},
                      {-2.0f, 2.0f, 0.0f, 1.0f},
                      {2.0f, -2.0f, 0.0f, 1.0f},
                      {-2.0f, -2.0f, 0.0f, 1.0f},
                      {2.0f, 2.0f, 0.0f, 1.0f} };


VECTOR4 shard[3] = { {-5.0f, 0.0f, 0.0f, 1.0f},
                    { 0.0f, 5.0f, 0.0f, 1.0f},
                    { 5.0f, -5.0f, 0.0f, 1.0f}}; 


void Explosions_clear ( void )
{
   int i, j;
   
   ecur = 0;
   ecount = 0;
   pcur = 0;

   erot = 0.0f;

   for ( i=0; i<MAX_EXPLOSIONS; i++ )
   {
      explosions[i].active = FALSE;
      explosions[i].frame = 0L;
      for (j=0; j<MAX_PARTICLES; j++ )
         Vector_init ( explosions[i].pos[j] );
   }
}

int Explosions_count ( void )
{
   return ecount;
}

void Explosions_add ( OBJECT *obj )
{
   explosions[ecur].active = TRUE;
   explosions[ecur].frame = 0;
   explosions[ecur].color = GREEN;
   explosions[ecur].blend = 0;

   /* ok there are currently only 4 */
   Vector_copy ( obj->pos, explosions[ecur].pos[0] );
   Vector_copy ( obj->pos, explosions[ecur].pos[1] );
   Vector_copy ( obj->pos, explosions[ecur].pos[2] );
   Vector_copy ( obj->pos, explosions[ecur].pos[3] );

   explosions[ecur].thrust[0] = pcur;
   explosions[ecur].thrust[1] = pcur+1;
   explosions[ecur].thrust[2] = pcur+2;
   explosions[ecur].thrust[3] = pcur+3;
   
   ecur++;
   pcur += 4;
   ecount++;

   if ( ecur > MAX_EXPLOSIONS-1 )
      ecur = 0;

   if ( pcur == 8 )
      pcur = 0;
}

void Explosions_draw ( MATRIX4 r )
{
   int i, j, k, p0[6];
   VECTOR4 tmp[3], shard_tmp[3];
   MATRIX4 tmp_mat, tmp_mat2, erot_mat;

   erot += EXPLOSION_ROT * gv->fadjust;
   Matrix_x_rot ( tmp_mat, erot );
   Matrix_z_rot ( tmp_mat2, erot );
   Matrix_mult  ( tmp_mat, tmp_mat2, erot_mat );
   Matrix_vec_multn ( erot_mat, shard, shard_tmp, 3 );

   for ( i=0; i<MAX_EXPLOSIONS; i++ )
   {
      if ( explosions[i].active )
      {
         explosions[i].frame += gv->msec;
         if ( explosions[i].frame > EXPLOSIONS_LIFE )
         {
            explosions[i].active = FALSE;
            ecount--;
         }
         explosions[i].blend += gv->msec;
         if ( explosions[i].blend > EXPLOSION_BLEND_TIME )
         {
            explosions[i].blend -= EXPLOSION_BLEND_TIME;
            explosions[i].color -= EXPLOSION_COLOR_INC;
         }

         for ( j=0; j<MAX_PARTICLES; j++ )
         {
            k = explosions[i].thrust[j];
            explosions[i].pos[j][XPOS] += pthrust[k][XPOS] * gv->fadjust;
            explosions[i].pos[j][YPOS] += pthrust[k][YPOS] * gv->fadjust;
            explosions[i].pos[j][ZPOS] += pthrust[k][ZPOS] * gv->fadjust;
   
            Matrix_vec_mult ( r, explosions[i].pos[j], tmp[0] );
            Matrix_copy ( r, tmp_mat );
            Matrix_set_trans ( tmp_mat, tmp[0] );

            Matrix_vec_multn ( tmp_mat, shard_tmp, tmp, 3 );
            Camera_project_points ( tmp, p0, 3 );

            Draw_line ( p0[0], p0[1], p0[2], p0[3], explosions[i].color );
            Draw_line ( p0[2], p0[3], p0[4], p0[5], explosions[i].color );
            Draw_line ( p0[4], p0[5], p0[0], p0[1], explosions[i].color );
         }
      }
   }
}

/*------------------------------------------------------------------
 * Jump-gate
 *  
 *
------------------------------------------------------------------*/

enum jumpgate_enum
{
   MAX_JUMPGATES   = 4,
   JUMPGATE_TIME   = 5000,
   JUMPGATE_ANIM   = 250,
   JUMPGATE_FRAMES = 3
};

struct JUMPGATESTRUCT
{
   int  active;
   long time;
   long anim;
   long frame;
   int  dir;
   VECTOR4  pos;
} jgates[MAX_JUMPGATES];

static int jgcur;   /* JUMPGATE index */
static int jcount;  /* JUMPGATE counter */

static VECTOR4 jgvert[32] =
{
   {-10.0f, 0.0f, 10.0f, 1.0f},
   {-0.0f, 10.0f, -10.0f, 1.0f},
   {10.0f, 0.0f, -10.0f, 1.0f},
   {0.0f, -10.0f, 10.0f, 1.0f},
   
   {-30.0f, 0.0f, 30.0f, 1.0f},
   {0.0f, 30.0f, -30.0f, 1.0f},
   {30.0f, 0.0f, -30.0f, 1.0f},
   {0.0f, -30.0f, 30.0f, 1.0f},
  
   {-50.0f, 0.0f, 50.0f, 1.0f},
   {0.0f, 50.0f, -50.0f, 1.0f},
   {50.0f, 0.0f, -50.0f, 1.0f},
   {0.0f, -50.0f, 50.0f, 1.0f},
   
   {-70.0f, 0.0f, 70.0f, 1.0f},
   {0.0f, 70.0f, -70.0f, 1.0f},
   {70.0f, 0.0f, -70.0f, 1.0f},
   {0.0f, -70.0f, 70.0f, 1.0f},
   
   {-10.0f, 0.0f, -10.0f, 1.0f},
   {0.0f, 10.0f, 10.0f, 1.0f},
   {10.0f, 0.0f, 10.0f, 1.0f},
   {0.0f, -10.0f, -10.0f, 1.0f},
   
   {-30.0f, 0.0f, -30.0f, 1.0f},
   {0.0f, 30.0f, 30.0f, 1.0f},
   {30.0f, 0.0f, 30.0f, 1.0f},
   {0.0f, -30.0f, -30.0f, 1.0f},
  
   {-50.0f, 0.0f, -50.0f, 1.0f},
   {0.0f, 50.0f, 50.0f, 1.0f},
   {50.0f, 0.0f, 50.0f, 1.0f},
   {0.0f, -50.0f, -50.0f, 1.0f},
   
   {-70.0f, 0.0f, -70.0f, 1.0f},
   {0.0f, 70.0f, 70.0f, 1.0f},
   {70.0f, 0.0f, 70.0f, 1.0f},
   {0.0f, -70.0f, -70.0f, 1.0f}
};

void Jumpgate_init ( void )
{
   int i;
   for ( i=0; i<MAX_JUMPGATES; i++ )
   {
      jgates[i].active = FALSE;
      jgates[i].time   = 0;
      jgates[i].anim   = 0;
      jgates[i].frame  = 0;
      jgates[i].dir    = 0;
      Vector_init ( jgates[i].pos );
   }
   jgcur   = 0;
   jcount  = 0;
}
   
void Jumpgate_open ( VECTOR4 pos, int dir )
{
   if ( jcount > MAX_JUMPGATES-1 ) return;

   jgates[jgcur].active = TRUE;
   jgates[jgcur].time   = 0;
   jgates[jgcur].anim   = 0;
   jgates[jgcur].frame  = 0;
   jgates[jgcur].dir    = dir * 16;
   Vector_copy  ( pos, jgates[jgcur].pos );

   jgcur++;
   if ( jgcur > MAX_JUMPGATES-1 )
      jgcur = 0;

   jcount++;
}

void Jumpgate_animate ( MATRIX4 r )
{
   MATRIX4 tmp_mat;
   VECTOR4 tmp[16];
   int    i, j, p[32], f0;

   for ( i=0; i<MAX_JUMPGATES; i++ )
   {

      if ( jgates[i].active )
      {
         jgates[i].time += gv->msec;
         jgates[i].anim += gv->msec;

         if ( jgates[i].anim > JUMPGATE_ANIM )
         {
            jgates[i].anim -= JUMPGATE_ANIM;
            jgates[i].frame += 1;
            if ( jgates[i].frame > JUMPGATE_FRAMES )
               jgates[i].frame = 0;
         }

         if ( jgates[i].time > JUMPGATE_TIME )
         {
            jgates[i].active = FALSE;
            jcount--;
         }

         /* draw jumpgate */
         Matrix_vec_mult ( r, jgates[i].pos, tmp[0] );
         Matrix_copy ( r, tmp_mat );
         Matrix_set_trans ( tmp_mat, tmp[0] );

         f0 = ( jgates[i].frame + 1 ) * 4;
         Matrix_vec_multn ( tmp_mat, 
               &jgvert[jgates[i].dir], tmp, f0 );
         Camera_project_points ( tmp, p, f0 ); 
         for ( j=0; j<((f0*2)-4); j+=8 )
         {
            Draw_line ( p[0+j], p[1+j], p[2+j], p[3+j], GREEN );
            Draw_line ( p[2+j], p[3+j], p[4+j], p[5+j], GREEN );
            Draw_line ( p[4+j], p[5+j], p[6+j], p[7+j], GREEN );
            Draw_line ( p[6+j], p[7+j], p[0+j], p[1+j], GREEN );
         }
      }
   }
}

/*------------------------------------------------------------------
 * One-up!!!
 *  
 *
------------------------------------------------------------------*/

enum oneup_enum
{ 
   ONEUP_LIFE       = 2000,
   ONEUP_BLEND_TIME = 200
};

struct ONEUPSTRUCT
{
   VECTOR4  pos;
   long frame;
   long blend;
   long color;
   int  active;
}one_up;

static VECTOR4 one_up_vert[10] =
{
   {-40.0f, 20.0f, 0.0f, 1.0f} ,  /* 1 */
   {-40.0f, -20.0f, 0.0f, 1.0f }, 
   {-20.0f, 20.0f, 0.0f, 1.0f  }, /* U */
   {-20.0f, -20.0f, 0.0f, 1.0f },
   { 10.0f, -20.0f, 0.0f, 1.0f },
   { 10.0f, 20.0f, 0.0f, 1.0f  }, 
   { 20.0f, 20.0f, 0.0f, 1.0f  }, /* P */
   { 20.0f, -20.0f, 0.0f, 1.0f },
   { 60.0f, 0.0f, 0.0f, 1.0f },
   { 20.0f, 0.0f, 0.0, 1.0f }
};

static VECTOR4 oneup_thrust = { 0.0f, 0.0f, 10.0f, 1.0f };

void One_up_init ( void )
{
   one_up.active = FALSE;
}

void One_up_add  ( OBJECT *obj )
{
   if ( one_up.active == FALSE )
   {
      one_up.active = TRUE;
      Vector_copy ( obj->pos, one_up.pos );
      one_up.frame = 0;
      one_up.blend = 0;
      one_up.color = WHITE;
   }
}

void One_up_draw ( MATRIX4 r )
{
   int p0[20];
   VECTOR4 tmp[10];
   MATRIX4 tmp_mat;

   if ( one_up.active )
   {
      one_up.pos[ZPOS] += oneup_thrust[ZPOS] * gv->fadjust;
      if ( one_up.pos[ZPOS] > -50.0f )
         one_up.active = FALSE;
      one_up.frame += gv->msec;
      if ( one_up.frame > ONEUP_LIFE )
      {
         one_up.active = FALSE;
      }
      one_up.blend += gv->msec;
      if ( one_up.blend > ONEUP_BLEND_TIME )
      {
         one_up.blend -= ONEUP_BLEND_TIME;
         one_up.color -= 3;
      }

      /* draw one_up */
      Matrix_vec_mult ( r, one_up.pos, tmp[0] );
      Matrix_copy ( r, tmp_mat );
      Matrix_set_trans ( tmp_mat, tmp[0] );

      Matrix_vec_multn ( tmp_mat, one_up_vert, tmp, 10 );
      Camera_project_points ( tmp, p0, 10 );

      /* draw 1 */
      Draw_line ( p0[0], p0[1], p0[2], p0[3], one_up.color );

      /* draw U */
      Draw_line ( p0[4], p0[5], p0[6], p0[7], one_up.color );
      Draw_line ( p0[6], p0[7], p0[8], p0[9], one_up.color );
      Draw_line ( p0[8], p0[9], p0[10], p0[11], one_up.color );

      /* draw P */
      Draw_line ( p0[12], p0[13], p0[14], p0[15], one_up.color );
      Draw_line ( p0[12], p0[13], p0[16], p0[17], one_up.color );
      Draw_line ( p0[16], p0[17], p0[18], p0[19], one_up.color );
   }
}
