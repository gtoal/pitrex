/*------------------------------------------------------------------
  main-dos.c:

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

#include <allegro.h>
#include "game.h"

/* ahh the simplicity of DOS!!! */
BITMAP  *buffer;
PALETTE pal;

/*================================================================*/

/*------------------------------------------------------------------
 * main
 *  
 *
------------------------------------------------------------------*/
int main ( int argc, char **argv )
{
   int i;

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
      printf ( "%s\n", game_about_info[i] );
      i++;
   }
         
   return 0;
}

END_OF_MAIN();

/*================================================================*/

int Graphics_init ( unsigned int win_width, unsigned int win_height )
{
   int i, j;

   allegro_init ();
   install_keyboard ();

   if ( set_gfx_mode ( GFX_AUTODETECT, 640, 480, 0, 0 ) != 0 )
   {
      set_gfx_mode ( GFX_TEXT, 0, 0, 0, 0 );
      return FALSE;
   }

   buffer = create_bitmap ( win_width, win_height );
   set_clip ( buffer, 0, 0, win_width-1, win_height-1 );
   clear ( buffer );

   /* load default color scheme */
   /* red */
   for ( i=0, j=0; i<64; i++, j++ )
   {
      pal[i].r = j;
      pal[i].g = pal[i].b = 0;
   }

   /* green */
   for ( i=64, j=0; i<128; i++, j++ )
   {
      pal[i].g = j;
      pal[i].r = pal[i].b = 0;
   }
   
   /* blue */
   for ( i=128, j=0; i<192; i++, j++ )
   {
      pal[i].b = j;
      pal[i].r = pal[i].g = 0;
   }

   /* white */
   for ( i=192, j=0; i<256; i++, j++ )
   {
      pal[i].r = pal[i].g = pal[i].b = j;
   }

   /* yellow */
   pal[192].r = 63;
   pal[192].g = 63;
   pal[192].b = 32;
   
   set_palette ( pal );
   
   return TRUE;
}

/*================================================================*/

void Graphics_shutdown ( void )
{
   destroy_bitmap ( buffer );
   set_gfx_mode ( GFX_TEXT, 0, 0, 0, 0 );
}

/*================================================================*/

int Update_display ( void )
{
   vsync ();
   blit  ( buffer, screen, 0, 0, 0, 0, WIN_WIDTH, WIN_HEIGHT );
   clear ( buffer );
   return TRUE;
}

/*================================================================*/

int Handle_events ( void )
{
   gv->key_UP    = ( key[KEY_UP] ) ? TRUE : FALSE;
   gv->key_DOWN  = ( key[KEY_DOWN] ) ? TRUE : FALSE;
   gv->key_LEFT  = ( key[KEY_LEFT] ) ? TRUE : FALSE;
   gv->key_RIGHT = ( key[KEY_RIGHT] ) ? TRUE : FALSE;
   gv->key_FIRE  = ( key[KEY_SPACE] ) ? TRUE : FALSE;

   if ( key[KEY_F] )
      gv->display_fps ^= TRUE;
   
   if ( key[KEY_P] )
      Game_paused_toggle ();
   
   if ( key[KEY_Q] ) 
      Game_reset ();

   if ( key[KEY_ESC] )
      return FALSE;

   return TRUE;
}

/*================================================================*/

void Draw_line ( int x0, int y0, int x1, int y1, unsigned int color )
{
   line ( buffer, x0, y0, x1, y1, color );
}

/*================================================================*/

void Draw_point ( int x0, int y0, unsigned int color )
{
   rectfill ( buffer, x0-1, y0-1, x0+1, y0+1, color );
}

/*================================================================*/

void Draw_text ( char *message, int x0, int y0, unsigned int color )
{
   textout ( buffer, font, message, x0, y0-10, color );
} 

/*================================================================*/

/*------------------------------------------------------------------
 *
 * System msec & sec Timer functions
 *
------------------------------------------------------------------*/

void Timer_init ( TIMER *t )
{
   t->init_time_stamp = time ( NULL );

   while ( gettimeofday ( &(t->t0), NULL ) < 0 );
   while ( gettimeofday ( &(t->t1), NULL ) < 0 );
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
