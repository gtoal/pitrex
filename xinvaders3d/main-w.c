/*------------------------------------------------------------------
  main-w.c:

    XINVADERS 3D - 3d Shoot'em up
    Copyright (C) 2000 Don Llopis

	WIN32 port by Thomas Boutell

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

#include <windows.h>
#include <sys/types.h>
#include "game.h"

/*================================================================*/
/* WIN32 globals */

HINSTANCE	hInst;
HWND		win;

HPEN color_table_pens [MAX_COLORS];
HBRUSH color_table_brushes [MAX_COLORS];
unsigned short color_data [MAX_COLORS][3];
HPEN blackPen;
HBRUSH blackBrush;

/* Menu item IDs for the message boxes */
#define aboutItemId 1000
#define rulesItemId 1001

/* window buffers */
HDC			double_buffer_dc;
HBITMAP		double_buffer;
HBITMAP		double_buffer_old_bitmap;
/* misc window info */
char         *window_name = "3d";
unsigned int window_width, window_height,
             display_width, display_height;

static void showGameInfo(char **gameInfo, char *title, char *append);

/*================================================================*/

int PASCAL WinMain(HINSTANCE hInstCurrent, HINSTANCE hInstPrevious,
    LPSTR lpszCmdLine, int nCmdShow)

{   
   hInst = hInstCurrent;
   if ( !Graphics_init ( WIN_WIDTH, WIN_HEIGHT ) )
   {
      MessageBox(0,
		  "Error: could not initialize graphics!\n",
		  "XInvaders 3D Error",
		  MB_ICONEXCLAMATION);
      exit ( -1 );
   }

   if ( !Game_init ( WIN_WIDTH, WIN_HEIGHT ) )
   {
      MessageBox(0,
		  "Error: could not initialize game data!\n",
		  "XInvaders 3D Error",
		  MB_ICONEXCLAMATION);
      exit ( -1 );
   }

   /* run the game until ESC key is pressed */
   while ( Handle_events () )
   {
      /* get start-time of current frame */
      gv->msec    = Timer_msec ( gt );
      gv->ftime   = (double)gv->msec/1000L;
      gv->fps     = 1.0 / gv->ftime;
      gv->fadjust = gv->rfps / gv->fps;
      
      /* do game */
      (*Game_actionfn)(); 
      /* get end-time of current frame, msec elapsed, and calc fps */
      /* update display */
      Update_display (); 
   }

   Graphics_shutdown ();
   showGameInfo(game_about_info, "About XInvaders 3D", 
      "Ported to Windows by Thomas Boutell");
   return 0;
}


long FAR PASCAL winv3dWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*================================================================*/

int Graphics_init ( unsigned int win_width, unsigned int win_height )
{
   int i, j, width, height;

   HDC hdc;
   HPEN oldPen;
   HBRUSH oldBrush;
   WNDCLASS wc;    
   MENUITEMINFO menuitem;
   HMENU menu;
   int count;
   width = win_width;
   height = (int) win_height;

   window_width  = win_width;
   window_height = win_height;
	
   /* Create a window class */
	wc.style         = 0;
	wc.lpfnWndProc   = winv3dWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInst;
	wc.hIcon         = 0; /* LoadIcon(hInst, MAKEINTRESOURCE(WINV3D_ICON)); */
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW); 
	wc.hbrBackground = 0;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = "xinv3d";
	if (!RegisterClass(&wc)) {
		return FALSE;
	}

   /* create a simple window*/
	win = CreateWindow( 
		"xinv3d",
		"xinv3d",
		WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		window_width, window_height,
		HWND_DESKTOP,
		0,
		hInst,
		0);
   /* Get the system menu, so that we can install an
		about option that explains how to play */
	menu = GetSystemMenu(win, FALSE);
	count = GetMenuItemCount(menu);
	memset(&menuitem, 0, sizeof(menuitem));
	menuitem.fType = MFT_STRING;
	menuitem.fMask = MIIM_TYPE | MIIM_ID;
	menuitem.wID = aboutItemId;
	menuitem.dwTypeData = (DWORD) "&About xinv3d";
	menuitem.cch = strlen("&About xinv3d");
	menuitem.cbSize = sizeof(menuitem);
	InsertMenuItem(menu,
		count,
		TRUE,
		&menuitem);
	memset(&menuitem, 0, sizeof(menuitem));
	menuitem.fType = MFT_STRING;
	menuitem.fMask = MIIM_TYPE | MIIM_ID;
	menuitem.wID = rulesItemId;
	menuitem.dwTypeData = (DWORD) "&How to Play";
	menuitem.cch = strlen("&How to Play");
	menuitem.cbSize = sizeof(menuitem);
	InsertMenuItem(menu,
		count,
		TRUE,
		&menuitem);
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
      color_table_pens[i] = CreatePen(PS_SOLID, 1,
		RGB(color_data[i][0] >> 8, 
		color_data[i][1] >> 8, 
		color_data[i][2] >> 8));
      color_table_brushes[i] = CreateSolidBrush(
		RGB(color_data[i][0] >> 8, 
		color_data[i][1] >> 8, 
		color_data[i][2] >> 8));
   }
   blackPen = color_table_pens[BLACK];
   blackBrush = color_table_brushes[BLACK];
 
   /* create double buffer */
   hdc = GetDC(win);
   double_buffer = CreateCompatibleBitmap(
	   hdc, window_width, window_height);
   double_buffer_dc = CreateCompatibleDC(hdc);
   double_buffer_old_bitmap = SelectObject(
	   double_buffer_dc, double_buffer);
   SetBkColor(double_buffer_dc, RGB(0, 0, 0));
   ReleaseDC(win, hdc);
   
   oldPen = SelectObject(double_buffer_dc, blackPen);
   oldBrush = SelectObject(double_buffer_dc, blackBrush);

   Rectangle(double_buffer_dc,
		0, 0,
		window_width,
		window_height);
   SelectObject(double_buffer_dc, oldPen);
   SelectObject(double_buffer_dc, oldBrush);
   /* display the window */
   ShowWindow(win, SW_SHOW);

   return TRUE;
}

/*================================================================*/

void Graphics_shutdown ( void )
{
	int i;
	SelectObject(double_buffer_dc, double_buffer_old_bitmap);
	DeleteDC(double_buffer_dc);
	DeleteObject(double_buffer);
	for ( i=0; i<MAX_COLORS; i++ ) {
		DeleteObject(color_table_pens[i]);
		DeleteObject(color_table_brushes[i]);
	}
}

/*================================================================*/

int Update_display ( void )
{
	RECT r;
	HDC hdc = GetDC(win);
	BitBlt(hdc, 
		0, 
		0,
		window_width, 
		window_height,
		double_buffer_dc,
		0, 
		0,
		SRCCOPY);
	r.left = 0;
	r.top = 0;
	r.right = window_width;
	r.bottom = window_height;
	FillRect(double_buffer_dc,
		&r,
		blackBrush);
	return TRUE;
}

/*================================================================*/

void handleMessage(MSG *msg);

int Handle_events ( void )
{
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			return FALSE;
		}
    	handleMessage(&msg);
    }
    return TRUE;
}


void handleMessage(MSG *msg) {	
	/* Handle one message */
	TranslateMessage(msg); /* translates virtual key codes    */
	DispatchMessage(msg);  /* dispatches message to window    */
}

/*================================================================*/

void Draw_line ( int x0, int y0, int x1, int y1, unsigned int color )
{
	POINT p;
	HPEN oldPen = SelectObject(double_buffer_dc, color_table_pens[color]);
	MoveToEx(double_buffer_dc, x0, y0, &p);
	LineTo(double_buffer_dc, x1, y1);
	SelectObject(double_buffer_dc, oldPen);
}

/*================================================================*/

void Draw_point ( int x0, int y0, unsigned int color )
{
	RECT r;
	/* Consistent with the original odd logic */
	r.top = y0 - 3;
	r.left = x0 + 3;
	r.bottom = y0;
	r.right = x0;
	FillRect(double_buffer_dc, &r, color_table_brushes[color]);
}

/*================================================================*/

void Draw_text ( const char *message, int x0, int y0, unsigned int color )
{
	SetTextColor(double_buffer_dc, 
		RGB(color_data[color][0] >> 8, 
		color_data[color][1] >> 8, 
		color_data[color][2] >> 8));
   /* draw text */
   TextOut(double_buffer_dc,
	   x0, y0,
	   message,
	   strlen(message));
} 

/*================================================================*/

long FAR PASCAL winv3dWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{       
	switch (msg) {
        /*
         * Process whatever messages you want here and send the
         * rest to DefWindowProc.
         */           
		case WM_KEYDOWN:
		if (lParam & 0x4000000) {
			/* Repeat */
			break;
		}
		/* Not Repeat */
		switch (wParam)
		{
	           case VK_SPACE:
		          gv->key_FIRE = TRUE;
		          break;

               case VK_UP:
                  gv->key_UP = TRUE;
                  break;

               case VK_DOWN:
                  gv->key_DOWN = TRUE;
                  break;

               case VK_LEFT:
                  gv->key_LEFT = TRUE;
                  break;

               case VK_RIGHT:
                  gv->key_RIGHT = TRUE;
                  break;
               case VK_ESCAPE:
                  /* quit! */
				  PostMessage(win, WM_QUIT, 0, 0L); 
                  return FALSE; 
                  break;

               default:
                  break;
		}
        break;
		case WM_CHAR:
		switch (wParam)
		{
               case 'f':
                  /* display frames per second */
                  gv->display_fps ^= TRUE;
                  break;

               case 'p':
                  /* pause */
                  Game_paused_toggle ();
                  break;

               case 'q':
                  Game_reset ();
                  break;
			   default:
				   break;
		}
		break;
		case WM_KEYUP:
		switch (wParam)
		{
	           case VK_SPACE:
		          gv->key_FIRE = FALSE;
		          break;

               case VK_UP:
                  gv->key_UP = FALSE;
                  break;

               case VK_DOWN:
                  gv->key_DOWN = FALSE;
                  break;

               case VK_LEFT:
                  gv->key_LEFT = FALSE;
                  break;

               case VK_RIGHT:
                  gv->key_RIGHT = FALSE;
                  break;
               default:
                  break;
		}
        break;
		case WM_SYSCOMMAND:
		switch (wParam) {
			case SC_CLOSE:
			PostMessage(win, WM_QUIT, 0, 0L);
			break;
			case aboutItemId:
			showGameInfo(game_about_info,
				"About XInvaders 3D",
				"Ported to Windows by Thomas Boutell");
			break;
			case rulesItemId:
			showGameInfo(game_rules_info,
				"How to Play XInvaders 3D",
				0);
			break;
			default:
			/* Inherit default behavior */
			return (DefWindowProc(hwnd, msg, wParam, lParam));
		}
 		default:
		/* Inherit default behavior */
        return (DefWindowProc(hwnd, msg, wParam, lParam));
	}
	return 0L;
}

static void showGameInfo(char **gameInfo, char *title, char *append)
{
	/* Must be less than 16K to work properly. */
	char message[16384];
	strcpy(message, "");
	while (*gameInfo) {
		strcat(message, *gameInfo);
		strcat(message, "\r\n");
		gameInfo++;
	}
	if (append) {
		strcat(message, append);
	}	
	MessageBox(win,
		message,
		title ? title : "XInvaders 3D",
		MB_ICONINFORMATION);
}

void Timer_init ( TIMER *t )
{
   long msec;
   t->init_time_stamp = time ( NULL );
   msec = GetTickCount();
   t->t0.tv_sec = msec / 1000;
   t->t0.tv_usec = (msec % 1000) * 1000; 

   t->t1 = t->t0;
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
   long msec = GetTickCount();
   t->t1.tv_sec = msec / 1000;
   t->t1.tv_usec = (msec % 1000) * 1000; 
  
   msec = ((t->t1.tv_sec-t->t0.tv_sec)*1000L)+
      ((t->t1.tv_usec-t->t0.tv_usec)/1000L);

   t->t0.tv_sec = t->t1.tv_sec;
   t->t0.tv_usec = t->t1.tv_usec;

   return msec;
}
