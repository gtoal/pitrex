/*------------------------------------------------------------------
  system.h: 

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
#ifndef GAME_SYSTEM_FUNCTIONS
#define GAME_SYSTEM_FUNCTIONS

/*------------------------------------------------------------------
 *
 * Misc Functions & Graphics primitives: 
 * LINUX/X11 : main-x11.c
 *
------------------------------------------------------------------*/
#define WIN_WIDTH  640
#define WIN_HEIGHT 480
#define MAX_COLORS 256

extern int  Graphics_init     ( unsigned int, unsigned int );
extern void Graphics_shutdown ( void );
extern int  Update_display    ( void );
extern int  Handle_events     ( void );
extern void Draw_line         ( int, int, int, int, unsigned int );
extern void Draw_point        ( int, int, unsigned int );
extern void Draw_text         ( char *, int, int, unsigned int );

/*------------------------------------------------------------------
 *
 * System msec & sec Timer functions:
 * LINUX/X11 : main-x11.c
 *
------------------------------------------------------------------*/

#ifdef GAME_LINUX_X11

    #include <sys/time.h>
    #include <sys/times.h>
    #include <sys/types.h>
    #define TIME_T  time_t
    #define TIMEVAL struct timeval
    #define CLOCK_T clock_t

#elif GAME_DOS_DJGPP
    
    #include <sys/time.h>
    #include <sys/times.h>
    #include <sys/types.h>
    #define TIME_T  time_t
    #define TIMEVAL struct timeval
    #define CLOCK_T clock_t


#elif GAME_WIN32

    #include <winsock.h>
    #define TIME_T  time_t
    #define TIMEVAL struct timeval
    #define CLOCK_T clock_t

#endif

typedef struct TIMERSTRUCT TIMER;
struct TIMERSTRUCT
{
   TIME_T  init_time_stamp;
   TIMEVAL t0;
   TIMEVAL t1;
};

extern void    Timer_init  ( TIMER * );
extern CLOCK_T Timer_ticks ( void );
extern double  Timer_sec   ( TIMER * );
extern long    Timer_msec  ( TIMER * );

#endif
