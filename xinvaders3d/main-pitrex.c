/*------------------------------------------------------------------
  main-pitrex.c:

    XINVADERS 3D - 3d Shoot'em up
    Copyright (C) 2000 Don Llopis, 2022 Kevin Koster

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
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif

#include "game.h"

/*------------------------------------------------------------------
 * main
 *  
 *
------------------------------------------------------------------*/
int main ( int argc, char **argv )
{
   int i;

#ifdef __FreeBSD__
   fpsetmask(0);
#endif

   if ( !Graphics_init ( WIN_WIDTH, WIN_HEIGHT ) )
   {
      fprintf ( stderr, "Error: could not initialize graphics!\n" );
      exit ( -1 );
   }

   if ( !Game_init ( WIN_WIDTH, WIN_HEIGHT ) )
   {
      fprintf ( stderr, "Error: could not initialize game data!\n" );
      exit ( -1 );
   }

   Game_main ();

   Graphics_shutdown ();
   
   /* print contact information */
   i = 0;
   while ( game_about_info[i] )
   {
      fprintf ( stderr, "%s\n", game_about_info[i] );
      i++;
   }
         
   return 0;
}

/*================================================================*/

int Graphics_init ( unsigned int win_width, unsigned int win_height )
{
   if (!vectrexinit(1) )
   {
    printf("Could Not Initialise Vectrex Connection\n");
    return FALSE;
   }
   v_setName("xinvaders3d");
   v_init();

   return TRUE;
}

/*================================================================*/

void Graphics_shutdown ( void )
{
   vectrexclose();
}

/*================================================================*/

int Update_display ( void )
{
   v_WaitRecal();
   return TRUE;
}

/*================================================================*/

int Handle_events ( void )
{
  v_readButtons();
  v_readJoystick1Digital();

 if (currentButtonState & 0x01)
  Game_reset ();
 if (currentButtonState & 0x02)
  gv->display_fps ^= TRUE;
 if (currentButtonState & 0x04)
  Game_paused_toggle ();
 if (currentButtonState & 0x08)
  gv->key_FIRE = TRUE;
 else
  gv->key_FIRE = FALSE;

 if (currentJoy1Y == -1)
  gv->key_DOWN = TRUE;
 else
  gv->key_DOWN = FALSE;
 if (currentJoy1Y == 1)
  gv->key_UP = TRUE;
 else
  gv->key_UP = FALSE;
 if (currentJoy1X == -1)
  gv->key_LEFT = TRUE;
 else
  gv->key_LEFT = FALSE;
 if (currentJoy1X == 1)
  gv->key_RIGHT = TRUE;
 else
  gv->key_RIGHT = FALSE;

   return TRUE;
}

/*================================================================*/

void Draw_line ( int x0, int y0, int x1, int y1, unsigned int color )
{
   v_directDraw32 ((x0*20)-7000, ~(y0*25)+6000, (x1*20)-7000, ~(y1*25)+6000, (color/4)+50);
}

/*================================================================*/

void Draw_point ( int x0, int y0, unsigned int color )
{
/*
   v_directDraw32 ((x0*28)-9000, (~(y0*28)-12000)+1, ((x0*28)-9000)+1, (~(y0*28)-12000)+1, (color/4)+50);
*/
}

/*================================================================*/

void Draw_text ( char *message, int x0, int y0, unsigned int color )
{
   v_setBrightness((color/4)+50);
   v_printStringRaster (-20, -80, message, 5*8, -7, '\0');
} 

/*================================================================*/

/*------------------------------------------------------------------
 *
 * System msec & sec Timer functions
 *  NOT BARE-METAL COMPATIBLE
------------------------------------------------------------------*/

void Timer_init ( TIMER *t )
{
   t->init_time_stamp = time ( NULL );

   gettimeofday ( &(t->t0), NULL );
   gettimeofday ( &(t->t1), NULL );
}

/*================================================================*/

CLOCK_T Timer_ticks ( void )
{
   return clock ();
}

/*================================================================*/

double Timer_sec ( TIMER *t )
{
   return difftime ( time(NULL), t->init_time_stamp );
}

/*================================================================*/

long Timer_msec ( TIMER *t )
{
   long msec;
  
   if ( gettimeofday ( &(t->t1), NULL ) < 0 ) return -1;

   msec = ((t->t1.tv_sec-t->t0.tv_sec)*1000L)+
      ((t->t1.tv_usec-t->t0.tv_usec)/1000L);

   t->t0.tv_sec = t->t1.tv_sec;
   t->t0.tv_usec = t->t1.tv_usec;

   return msec;
}

/*================================================================*/
