/*------------------------------------------------------------------
  main-x11.c:

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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <sys/types.h>

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif

#include "game.h"

/*================================================================*/

/* window structs */
Display     *display;
int         screen_num;
Window      win;
Screen      *screen_ptr;
XFontStruct *font_info;

/* gc's & color data */
GC             black_gc, color_gc, text_gc;
XGCValues      gc_values;
unsigned long  gc_valuemask = 0;
XColor         color_info;
unsigned int   color_table [MAX_COLORS];
unsigned short color_data [MAX_COLORS][3];

/* window buffers */
Pixmap       double_buffer;

/* misc window info */
char          *display_name = NULL;
char          *window_name = "XInvaders 3D";
XTextProperty wname;
char          *font_name = "10x20";
unsigned int  window_width, window_height,
              display_width, display_height;

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
   int i, j;

   window_width  = win_width;
   window_height = win_height;

   display = XOpenDisplay ( display_name );
   if ( !display )
      return FALSE;

   screen_num = DefaultScreen ( display );
   screen_ptr = DefaultScreenOfDisplay ( display );

   win = XCreateSimpleWindow ( display, 
         RootWindow ( display, screen_num ),
         0, 0,
         window_width,
         window_height,
         1,
         BlackPixel ( display, screen_num ),
         WhitePixel ( display, screen_num ) );

   XSelectInput ( display, win, ExposureMask | KeyPressMask |
         KeyReleaseMask | ButtonPressMask | 
         ButtonReleaseMask | EnterWindowMask |
         LeaveWindowMask );
   
   XStringListToTextProperty ( &window_name, 1, &wname );
   XSetWMProperties ( display, win, &wname, NULL,
         NULL, 0, NULL, NULL, NULL );

   /* create GCs for clearing, text, & drawing */
   black_gc = XCreateGC ( display, win, gc_valuemask, &gc_values );
   color_gc = XCreateGC ( display, win, gc_valuemask, &gc_values );
   text_gc  = XCreateGC ( display, win, gc_valuemask, &gc_values );

   
   /* load default font */
   font_info = XLoadQueryFont ( display, font_name );
   if ( !font_info )
      perror ( "WARNING: could not load default font.\n" );
   else
      XSetFont ( display, text_gc, font_info->fid );

   /* load default color scheme */

   /* red */
   for ( i=0, j=0; i<64; i++, j++ )
   {
      color_data[i][0] = 1024 * j;
      color_data[i][1] = color_data[i][2] = 0;
   }

   /* green */
   for ( i=64, j=0; i<128; i++, j++ )
   {
      color_data[i][1] = 1024 * j;
      color_data[i][0] = color_data[i][2] = 0;
   }
   
   /* blue */
   for ( i=128, j=0; i<192; i++, j++ )
   {
      color_data[i][2] = 1024 * j;
      color_data[i][0] = color_data[i][1] = 0;
   }

   /* white */
   for ( i=192, j=0; i<256; i++, j++ )
   {
      color_data[i][0] = color_data[i][1] = color_data[i][2] = j * 1024;
   }

   /* yellow */
   color_data[192][0] = 63 * 1024;
   color_data[192][1] = 63 * 1024;
   color_data[192][2] = 32 * 1024;

   
   for ( i=0; i<MAX_COLORS; i++ )
   {
      color_info.red   = color_data[i][0];
      color_info.green = color_data[i][1];
      color_info.blue  = color_data[i][2];

      XAllocColor ( display,
            DefaultColormap ( display, screen_num ),
            &color_info );

      color_table[i] = color_info.pixel;
   }

   XSetForeground ( display, black_gc, 
         color_table[BLACK] );
   XSetForeground ( display, color_gc, 
         color_table[GREEN] );
   XSetForeground ( display, text_gc, 
         color_table[GREEN] );
/*
   double_buffer = XCreatePixmap ( display, win,
         window_width,
         window_height,
         DefaultDepth ( display, screen_num ) );
*/
#define double_buffer	win
   XFillRectangle ( display,
         double_buffer,
         black_gc,
         0, 0, 
         window_width,
         window_height );

   XAutoRepeatOff ( display );

   XMapWindow ( display, win );

#ifdef GAME_DEBUG_KEYBOARD
   fprintf ( stderr, "SPACE is %d\n", XKeysymToKeycode ( display, XK_space ) );
   fprintf ( stderr, "Left Shift is %d\n", XKeysymToKeycode ( display, XK_Shift_L ) );
   fprintf ( stderr, "UP is %d\n", XKeysymToKeycode ( display, XK_Up ) );
   fprintf ( stderr, "DOWN is %d\n", XKeysymToKeycode ( display, XK_Down ) );
   fprintf ( stderr, "LEFT is %d\n", XKeysymToKeycode ( display, XK_Left ) );
   fprintf ( stderr, "KP UP is %d\n", XKeysymToKeycode ( display, XK_KP_Up ) );
   fprintf ( stderr, "KP DOWN is %d\n", XKeysymToKeycode ( display, XK_KP_Down ) );
   fprintf ( stderr, "KP LEFT is %d\n", XKeysymToKeycode ( display, XK_KP_Left ) );
   fprintf ( stderr, "KP RIGHT is %d\n", XKeysymToKeycode ( display, XK_KP_Right ) );
   fprintf ( stderr, "p is %d\n", XKeysymToKeycode ( display, XK_p ) );
   fprintf ( stderr, "P is %d\n", XKeysymToKeycode ( display, XK_P ) );
   fprintf ( stderr, "q is %d\n", XKeysymToKeycode ( display, XK_q ) );
   fprintf ( stderr, "Q is %d\n", XKeysymToKeycode ( display, XK_Q ) );
   fprintf ( stderr, "f is %d\n", XKeysymToKeycode ( display, XK_f ) );
   fprintf ( stderr, "F is %d\n", XKeysymToKeycode ( display, XK_F ) );
   fprintf ( stderr, "ESC is %d\n", XKeysymToKeycode ( display, XK_Escape ) );
#endif

   return TRUE;
}

/*================================================================*/

void Graphics_shutdown ( void )
{
   if ( font_info )
      XUnloadFont ( display, font_info->fid );

   XFreeGC ( display, black_gc );
   XFreeGC ( display, color_gc );
   XFreeGC ( display, text_gc );

   XAutoRepeatOn ( display );

   XCloseDisplay ( display );
}

/*================================================================*/

int Update_display ( void )
{
#ifndef double_buffer
   XCopyArea ( display, double_buffer, win, black_gc,
         0, 0, window_width, window_height, 0, 0 );
#endif
   XFillRectangle ( display, double_buffer, black_gc,
         0, 0, window_width, window_height );

   
   return TRUE;
}

/*================================================================*/

int Handle_events ( void )
{
   XEvent event;
   KeySym keysym;
   
   XSync ( display, False );
   while ( XPending ( display ) )
   {
      XNextEvent ( display, &event );

      switch ( event.type )
      {
         case Expose:
            break;

         case EnterNotify:
            /* turn off auto-repeat */
            XAutoRepeatOff ( display );
            break;

         case LeaveNotify:
            /* turn on auto-repeat */
            XAutoRepeatOn ( display );
            break;

         case KeyPress:
            keysym = XLookupKeysym ( &(event.xkey), 0 );
            switch ( keysym )
            {
               case XK_space:
                  gv->key_FIRE = TRUE;
                  break;

               case XK_Up:
               case XK_KP_Up:
                  gv->key_UP = TRUE;
                  break;

               case XK_Down:
               case XK_KP_Down:
                  gv->key_DOWN = TRUE;
                  break;

               case XK_Left:
               case XK_KP_Left:
                  gv->key_LEFT = TRUE;
                  break;

               case XK_Right:
               case XK_KP_Right:
                  gv->key_RIGHT = TRUE;
                  break;

               case XK_f:
               case XK_F:
                  /* display frames per second */
                  gv->display_fps ^= TRUE;
                  break;

               case XK_p:
               case XK_P:
                  /* pause */
                  Game_paused_toggle ();
                  break;

               case XK_q:
               case XK_Q:
                     Game_reset ();
                  break;

               case XK_Escape:
                  /* quit! */
                  return FALSE; 
                  break;

               default:
                  break;
            }
            break;
         
         case KeyRelease:
            keysym = XLookupKeysym ( &(event.xkey), 0 );
            switch ( keysym )
            {
               case XK_space:
                  gv->key_FIRE = FALSE;
                  break;

               case XK_Up:
               case XK_KP_Up:
                  gv->key_UP = FALSE;
                  break;

               case XK_Down:
               case XK_KP_Down:
                  gv->key_DOWN = FALSE;
                  break;

               case XK_Left:
               case XK_KP_Left:
                  gv->key_LEFT = FALSE;
                  break;

               case XK_Right:
               case XK_KP_Right:
                  gv->key_RIGHT = FALSE;
                  break;
               
               default:
                  break;
            }
            break;

         default:
            break;
      }
   }

   return TRUE;
}

/*================================================================*/

void Draw_line ( int x0, int y0, int x1, int y1, unsigned int color )
{
   XSetForeground ( display, color_gc, 
         color_table[color] );

   XDrawLine ( display, double_buffer, color_gc,
               x0, y0, x1, y1 );
}

/*================================================================*/

void Draw_point ( int x0, int y0, unsigned int color )
{
   XSetForeground ( display, color_gc, 
         color_table[color] );

   XFillRectangle ( display, double_buffer, color_gc,
         x0-3, y0+3, 3, 3 );
}

/*================================================================*/

void Draw_text ( char *message, int x0, int y0, unsigned int color )
{
   XSetForeground ( display, text_gc, 
         color_table[color] );

   XDrawString ( display, double_buffer, text_gc,
                 x0, y0,
                 message, strlen ( message ) );
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
