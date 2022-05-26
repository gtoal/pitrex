/*
   Ported to Arduino Due by PhOBoZ

   Vector Game "Gyrocks" auf dem Oszilloskop
   Carsten Wartmann 2016/2017 cw@blenderbuch.de
   Fürs Make-Magazin

   Gehackt und basierend auf Trammel Hudsons Arbeit:

   Vector display using the MCP4921 DAC on the teensy3.1.
   More info: https://trmm.net/V.st
*/

/*
   Todo:
   - Alles auf ein System/Skalierung umstellen, fixe integer Mathe
   - implement Bounding Box Collision
   + Enemies und Rocks trennen
   + weniger Schüsse gleichzeitig
   - Enemy und Rocks können Ship schaden
   - Bug in Collisions Erkennung?
    - Enemies schießen
   - Lebenszähler für Ship
   + einfache Punktezählung
   - Feinde Formationen fliegen lassen
   - Alles auf Polarkoord umstellen (Ship/Bullets) für Kollisionsabfrage
   - Explosionen

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define PI 3.14159265358979323846

#include <vectrex/vectrexInterface.h>



#include "std.i"
#include "hershey_font.h"
#include "objects.h"

#define SLOW_MOVE
#define BUFFER_SIZE 4096


/*  *********************** Graphics Stuff ***********************************************/
#ifndef FALSE
#define FALSE (0!=0)
#define TRUE (0==0)
#endif

//        !     "     #     $     %     &     '     (     )     *     +     ,     -     .     /   
//  0     1     2     3     4     5     6     7     8     9     :     ;     <     =     >     ?   
//  @     A     B     C     D     E     F     G     H     I     J     K     L     M     N     O   
//  P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _   
//  `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o   
//  p     q     r     s     t     u     v     w     x     y     z     {     |     }     ~      
unsigned char lcrasterline1[] = {
  0x00, 0x18, 0x6c, 0x6c, 0x30, 0x00, 0x38, 0x60, 0x18, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
  0x7c, 0x30, 0x78, 0x78, 0x1c, 0xfc, 0x38, 0xfc, 0x78, 0x78, 0x00, 0x00, 0x18, 0x00, 0x60, 0x78, 
  0x7c, 0x30, 0xfc, 0x3c, 0xf8, 0xfe, 0xfe, 0x3c, 0xcc, 0x78, 0x1e, 0xe6, 0xf0, 0xc6, 0xc6, 0x38, 
  0xfc, 0x78, 0xfc, 0x78, 0xfc, 0xcc, 0xcc, 0xc6, 0xc6, 0xcc, 0xfe, 0x78, 0xc0, 0x78, 0x10, 0x00, 
  0x30, 0x00, 0xe0, 0x00, 0x1c, 0x00, 0x38, 0x00, 0xe0, 0x30, 0x0c, 0xe0, 0x70, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x18, 0xe0, 0x76, 0x00, 
};
unsigned char lcrasterline2[] = {
  0x00, 0x3c, 0x6c, 0x6c, 0x7c, 0xc6, 0x6c, 0x60, 0x30, 0x30, 0x66, 0x30, 0x00, 0x00, 0x00, 0x0c, 
  0xc6, 0x70, 0xcc, 0xcc, 0x3c, 0xc0, 0x60, 0xcc, 0xcc, 0xcc, 0x30, 0x30, 0x30, 0x00, 0x30, 0xcc, 
  0xc6, 0x78, 0x66, 0x66, 0x6c, 0x62, 0x62, 0x66, 0xcc, 0x30, 0x0c, 0x66, 0x60, 0xee, 0xe6, 0x6c, 
  0x66, 0xcc, 0x66, 0xcc, 0xb4, 0xcc, 0xcc, 0xc6, 0xc6, 0xcc, 0xc6, 0x60, 0x60, 0x18, 0x38, 0x00, 
  0x30, 0x78, 0x60, 0x78, 0x0c, 0x78, 0x60, 0x76, 0x6c, 0x00, 0x00, 0x66, 0x30, 0xcc, 0xf8, 0x78, 
  0xdc, 0x76, 0xdc, 0x7c, 0x7c, 0xcc, 0xcc, 0xc6, 0xc6, 0xcc, 0xfc, 0x30, 0x18, 0x30, 0xdc, 0x00, 
};
unsigned char lcrasterline3[] = {
  0x00, 0x3c, 0x00, 0xfe, 0xc0, 0xcc, 0x38, 0xc0, 0x60, 0x18, 0x3c, 0x30, 0x00, 0x00, 0x00, 0x18, 
  0xce, 0x30, 0x0c, 0x0c, 0x6c, 0xf8, 0xc0, 0x0c, 0xcc, 0xcc, 0x30, 0x30, 0x60, 0xfc, 0x18, 0x0c, 
  0xde, 0xcc, 0x66, 0xc0, 0x66, 0x68, 0x68, 0xc0, 0xcc, 0x30, 0x0c, 0x6c, 0x60, 0xfe, 0xf6, 0xc6, 
  0x66, 0xcc, 0x66, 0xe0, 0x30, 0xcc, 0xcc, 0xc6, 0x6c, 0xcc, 0x8c, 0x60, 0x30, 0x18, 0x6c, 0x00, 
  0x18, 0x0c, 0x7c, 0xcc, 0x7c, 0xcc, 0xf0, 0xcc, 0x76, 0x70, 0x0c, 0x6c, 0x30, 0xfe, 0xcc, 0xcc, 
  0x66, 0xcc, 0x76, 0xc0, 0x30, 0xcc, 0xcc, 0xd6, 0x6c, 0xcc, 0x98, 0x30, 0x18, 0x30, 0x00, 0x00, 
};
unsigned char lcrasterline4[] = {
  0x00, 0x18, 0x00, 0x6c, 0x78, 0x18, 0x76, 0x00, 0x60, 0x18, 0xfe, 0xfc, 0x00, 0xfc, 0x00, 0x30, 
  0xde, 0x30, 0x38, 0x38, 0xcc, 0x0c, 0xf8, 0x18, 0x78, 0x7c, 0x00, 0x00, 0xc0, 0x00, 0x0c, 0x18, 
  0xde, 0xcc, 0x7c, 0xc0, 0x66, 0x78, 0x78, 0xce, 0xfc, 0x30, 0x0c, 0x78, 0x60, 0xfe, 0xde, 0xc6, 
  0x7c, 0xcc, 0x7c, 0x70, 0x30, 0xcc, 0xcc, 0xd6, 0x38, 0x78, 0x18, 0x60, 0x18, 0x18, 0xc6, 0x00, 
  0x00, 0x7c, 0x66, 0xc0, 0xcc, 0xfc, 0x60, 0xcc, 0x66, 0x30, 0x0c, 0x78, 0x30, 0xfe, 0xcc, 0xcc, 
  0x66, 0xcc, 0x66, 0x78, 0x30, 0xcc, 0xcc, 0xfe, 0x38, 0xcc, 0x30, 0xe0, 0x00, 0x1c, 0x00, 0x00, 
};
unsigned char lcrasterline5[] = {
  0x00, 0x18, 0x00, 0xfe, 0x0c, 0x30, 0xdc, 0x00, 0x60, 0x18, 0x3c, 0x30, 0x00, 0x00, 0x00, 0x60, 
  0xf6, 0x30, 0x60, 0x0c, 0xfe, 0x0c, 0xcc, 0x30, 0xcc, 0x0c, 0x00, 0x00, 0x60, 0x00, 0x18, 0x30, 
  0xde, 0xfc, 0x66, 0xc0, 0x66, 0x68, 0x68, 0xce, 0xcc, 0x30, 0xcc, 0x6c, 0x62, 0xd6, 0xce, 0xc6, 
  0x60, 0xdc, 0x6c, 0x1c, 0x30, 0xcc, 0xcc, 0xfe, 0x38, 0x30, 0x32, 0x60, 0x0c, 0x18, 0x00, 0x00, 
  0x00, 0xcc, 0x66, 0xcc, 0xcc, 0xc0, 0x60, 0x7c, 0x66, 0x30, 0xcc, 0x6c, 0x30, 0xd6, 0xcc, 0xcc, 
  0x7c, 0x7c, 0x60, 0x0c, 0x34, 0xcc, 0x78, 0xfe, 0x6c, 0x7c, 0x64, 0x30, 0x18, 0x30, 0x00, 0x00, 
};
unsigned char lcrasterline6[] = {
  0x00, 0x00, 0x00, 0x6c, 0xf8, 0x66, 0xcc, 0x00, 0x30, 0x30, 0x66, 0x30, 0x30, 0x00, 0x30, 0xc0, 
  0xe6, 0x30, 0xcc, 0xcc, 0x0c, 0xcc, 0xcc, 0x30, 0xcc, 0x18, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x00, 
  0xc0, 0xcc, 0x66, 0x66, 0x6c, 0x62, 0x60, 0x66, 0xcc, 0x30, 0xcc, 0x66, 0x66, 0xc6, 0xc6, 0x6c, 
  0x60, 0x78, 0x66, 0xcc, 0x30, 0xcc, 0x78, 0xee, 0x6c, 0x30, 0x66, 0x60, 0x06, 0x18, 0x00, 0x00, 
  0x00, 0x76, 0xdc, 0x78, 0x76, 0x78, 0xf0, 0x0c, 0xe6, 0x78, 0xcc, 0xe6, 0x78, 0xc6, 0xcc, 0x78, 
  0x60, 0x0c, 0xf0, 0xf8, 0x18, 0x76, 0x30, 0x6c, 0xc6, 0x0c, 0xfc, 0x30, 0x18, 0x30, 0x00, 0x00, 
};
unsigned char lcrasterline7[] = {
  0x00, 0x18, 0x00, 0x6c, 0x30, 0xc6, 0x76, 0x00, 0x18, 0x60, 0x00, 0x00, 0x70, 0x00, 0x30, 0x80, 
  0x7c, 0xfc, 0xfc, 0x78, 0x1e, 0x78, 0x78, 0x30, 0x78, 0x70, 0x30, 0x70, 0x18, 0x00, 0x60, 0x30, 
  0x78, 0xcc, 0xfc, 0x3c, 0xf8, 0xfe, 0xf0, 0x3e, 0xcc, 0x78, 0x78, 0xe6, 0xfe, 0xc6, 0xc6, 0x38, 
  0xf0, 0x1c, 0xe6, 0x78, 0x78, 0xfc, 0x30, 0xc6, 0xc6, 0x78, 0xfe, 0x78, 0x02, 0x78, 0x00, 0xfe, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xf0, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x1c, 0x18, 0xe0, 0x00, 0x00, 
};

unsigned char *lcrasterlines[7] = {
  lcrasterline1, lcrasterline2, lcrasterline3, lcrasterline4, lcrasterline5, lcrasterline6, lcrasterline7
};

// This will move to the vectrex graphics library.
static int64_t ScaleXMul = 1LL, ScaleXDiv = 1LL, ScaleXOffsetPre = 0LL, ScaleXOffsetPost = 0LL,
               ScaleYMul = 1LL, ScaleYDiv = 1LL, ScaleYOffsetPre = 0LL, ScaleYOffsetPost = 0LL;
static int v_rotate = 0, v_flip_x = 0, v_flip_y = 0;
static int v_xl = 0, v_yb = 0, v_xr = 0, v_yt = 0;

int tx (int x) {				// convert x from window to viewport
   x = x * 2; // correct the seriously broken aspect ratio..!
   if (v_flip_x) x = v_xr - (x - v_xl);
   return (int) (((((int64_t) x) + ScaleXOffsetPre) * ScaleXMul) / ScaleXDiv + ScaleXOffsetPost);
}

int ty (int y) {				// and y
   if (v_flip_y) y = v_yt - (y - v_yb);
   return (int) (((((int64_t) y) + ScaleYOffsetPre) * ScaleYMul) / ScaleYDiv + ScaleYOffsetPost);
}

enum { TOP = 0x1, BOTTOM = 0x2, RIGHT = 0x4, LEFT = 0x8 };

typedef unsigned int outcode;

static outcode compute_outcode (int x, int y, int xmin, int ymin, int xmax, int ymax) {
  outcode oc = 0;

  if (y > ymax)
    oc |= TOP;
  else if (y < ymin)
    oc |= BOTTOM;
  if (x > xmax)
    oc |= RIGHT;
  else if (x < xmin)
    oc |= LEFT;
  return oc;
}

static int retain_after_clipping (int *x1p, int *y1p, int *x2p, int *y2p, int xmin, int ymin, int xmax, int ymax) {
#define x1 (*x1p)
#define y1 (*y1p)
#define x2 (*x2p)
#define y2 (*y2p)

  int accept;
  int done;
  outcode outcode1, outcode2;

  accept = FALSE;
  done = FALSE;
  outcode1 = compute_outcode (x1, y1, xmin, ymin, xmax, ymax);
  outcode2 = compute_outcode (x2, y2, xmin, ymin, xmax, ymax);
  do {
    if (outcode1 == 0 && outcode2 == 0) {
      accept = TRUE;
      done = TRUE;
    } else if (outcode1 & outcode2) {
      done = TRUE;
    } else {
      int x, y;
      int outcode_ex = outcode1 ? outcode1 : outcode2;

      if (outcode_ex & TOP) {
        x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
        y = ymax;
      } else if (outcode_ex & BOTTOM) {
        x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
        y = ymin;
      } else if (outcode_ex & RIGHT) {
        y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
        x = xmax;
      } else {
        y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
        x = xmin;
      }
      if (outcode_ex == outcode1) {
        x1 = x;
        y1 = y;
        outcode1 = compute_outcode (x1, y1, xmin, ymin, xmax, ymax);
      } else {
        x2 = x;
        y2 = y;
        outcode2 = compute_outcode (x2, y2, xmin, ymin, xmax, ymax);
      }
    }
  } while (done == FALSE);
  return accept && done;
#undef x1
#undef y1
#undef x2
#undef y2
}

void w_directMove32(int x, int y) {
       v_directMove32 (tx (x), ty (y));
}

void w_directDraw32(int xl, int yb, int xr, int yt, int col) {
       if (!retain_after_clipping(&xl,&yb,&xr,&yt, v_xl,v_yb,v_xr,v_yt)) return;
       v_directDraw32 (tx (xl), ty (yb), tx (xr), ty (yt), col);
}

void window (int xl, int yb, int xr, int yt)
{
   // We will use normalised viewport coordinates of x = -18000:18000 and y = -24000:24000 for consistency
   int64_t width, height, owidth, oheight;
   int xc, yc;

   v_xl = xl; v_yb = yb; v_xr = xr; v_yt = yt;
   owidth = width = (int64_t) xr - (int64_t) xl;
   oheight = height = (int64_t) yt - (int64_t) yb;

   // However, if OS tells us that vectrex is on its side, we'll handle these differently.
   // For now, though, Malban's orientation code is doing the rotation behind the scenes
   // so we don't want to do it twice.  Need to think about whether that solution is OK
   // or has to be changed. Although it doesn't matter too much in terms of what is displayed,
   // it makes a different with respect to loading and saving configs and using the same
   // default for multiple games.
   if (width * 4 >= height * 3) {
      // window is wider than aspect ratio, so we will have black bars at the top and bottom
      height = (width * 4) / 3;
      yc = (yb + yt) / 2;
      yb = yc - height / 2;
      yt = yc + height / 2;
   } else if (width * 4 < height * 3) {
      // window is taller than aspect ratio, so we will have black bars at
      // the sides
      width = (height * 3) / 4;
      xc = (xl + xr) / 2;
      xl = xc - width / 2;
      xr = xc + width / 2;
   }

   //printf("old window: (%d,%d) to (%d,%d)\n", v_xl,v_yb, v_xr,v_yt);
   //printf("new window: (%d,%d) to (%d,%d)\n", xl,yb, xr,yt);
   
   ScaleXMul = 36000LL;
   ScaleXDiv = width;

   //ScaleXOffsetPre = -width / 2LL;
   ScaleXOffsetPre = 0LL;
   ScaleXOffsetPost = 0LL;
   // adjust center of window to center of screen
   ScaleXOffsetPost =  -(tx (v_xr)+tx (v_xl)) / 2LL;

   ScaleYMul = 48000LL;
   ScaleYDiv = height;

   ScaleYOffsetPre = 0LL; // -height / 2LL;
   ScaleYOffsetPost = 0LL;
   // adjust center of window to center of screen
   ScaleYOffsetPost = -(ty (v_yt)+ty (v_yb)) / 2LL;

   //printf("X: scale %lld, pre %lld, post %lld\n", ScaleXMul/ScaleXDiv,  ScaleXOffsetPre,  ScaleXOffsetPost);
   //printf("Y: scale %lld, pre %lld, post %lld\n", ScaleYMul/ScaleYDiv,  ScaleYOffsetPre,  ScaleYOffsetPost);
   //printf("new transformed window: (%d,%d) to (%d,%d)\n", tx(xl),ty(yb), tx(xr),ty(yt));

   //setCustomClipping(TRUE, tx(v_xl), ty(v_yb), tx(v_xr), ty(v_yt));
       // transform world (window) coordinates to viewport (normalised device
       // coordinates) before clipping.  That way clipping code does not need to know about world
       // coordinates. NOT SURE IF THIS IS WORKING. Speedfreak seems to draw lines outside the window.
       // I think setCustomClipping must only apply to one of the buffered drawing modes.
   // I've added a local clipping procedure to ensure we have complete local control over clipping.
}

// end of window library
/*  *********************** Game Stuff ***************************************************/

// Rock
typedef struct
{
  int16_t t;
  int16_t r;
  int16_t p;
  int16_t x;    // x/y Merker für Kollisionsabfrage
  int16_t y;
  int16_t d;
  int16_t vr;
  int16_t vp;
} rock_t;

// Enemy
typedef struct
{
  int16_t t;
  int16_t r;
  int16_t p;
  int16_t vr;
  int16_t vp;
} enemy_t;

// Ship
typedef struct
{
  int16_t x;
  int16_t y;
  int16_t ax;
  int16_t ay;
  unsigned long firedelay;
} ship_t;

// Star
typedef struct
{
  int16_t x;
  int16_t y;
  int16_t vx;
  int16_t vy;
  int16_t age;
} star_t;

// Bullet
typedef struct
{
  int16_t x;
  int16_t y;
  int16_t rot;
  int16_t vx;
  int16_t vy;
  int16_t age;
} bullet_t;


#define boolean int
#define true 1
#define false 0
#define byte unsigned char

#define HALT  // Auskommentieren um "Handbremse" für zweiten Knopf/Schalter zu lösen (Debug&Screenshot)

// Joystick
#define BUTT 14   // Digital
#define TRIG 15   // Digital
#define THRU 16    // Analog
#define POTX 17   // Analog
#define POTY 18    // Analog
#define DEADX 30  // Deadband X
#define DEADY 30  // Deadband Y

#define FIREDELAY 100   // Zweitverzögerung zwischen zwei Schüssen

// Hintergrundsterne
#define MAX_STARS 30
star_t s[MAX_STARS];

// max. Anzahl der Schüsse
#define MAX_BULLETS 5
bullet_t b[MAX_BULLETS];

// max. Zahl der Asteroiden/Rocks
#define MAX_ROCK 5
rock_t r[MAX_ROCK];

// max. Zahl der Feinde
#define MAX_ENEMY 5
enemy_t e[MAX_ENEMY];

// Infos zum Schiff/Ship speichern
ship_t ship;

// Punktezähler
unsigned int score;

// Frames per Second/Framerate Merker
long fps;

// Schnelle aber ungenaue Sinus Tabelle
const  uint8_t isinTable8[] = {
  0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44,
  49, 53, 57, 62, 66, 70, 75, 79, 83, 87,
  91, 96, 100, 104, 108, 112, 116, 120, 124, 128,

  131, 135, 139, 143, 146, 150, 153, 157, 160, 164,
  167, 171, 174, 177, 180, 183, 186, 190, 192, 195,
  198, 201, 204, 206, 209, 211, 214, 216, 219, 221,

  223, 225, 227, 229, 231, 233, 235, 236, 238, 240,
  241, 243, 244, 245, 246, 247, 248, 249, 250, 251,
  252, 253, 253, 254, 254, 254, 255, 255, 255, 255,
};

double sinTable[360];
void buildSinTable()
{
  for (int i=0;i<360; i++)
    sinTable[i] = sin(i*(PI/180))*255;
}

void initVars()
{
 for (int i=0; i<MAX_ROCK; i++)
 {
	r[i].t = -1;
	r[i].r = 0;
	r[i].p = 0;
	r[i].x = 0;
	r[i].y = 0;
	r[i].d = 0;
 }
 for (int i=0; i<MAX_ENEMY; i++)
 {
	e[i].t = -1;
	e[i].r = 0;
	e[i].p = 0;
 }
 for (int i=0; i<MAX_BULLETS; i++)
 {
	b[i].age = -1;
 }
 ship.x = 2048;
 ship.y = 1000;
 buildSinTable();

}


double isin(int angle)
{
  if (angle < 0)
  {
    angle = -angle;
    if (angle >= 360) angle %= 360;
    return -sinTable[angle];
  }
  if (angle >= 360) angle %= 360;
  return sinTable[angle];
}
double icos(int x)
{
  return (isin(x + 90));
}




/*
// Schnelle aber ungenaue Sinus Funktion
int isin(int x)
{
  boolean pos = true;  // positive - keeps an eye on the sign.
  uint8_t idx;
  // remove next 6 lines for fastest execution but without error/wraparound
  if (x < 0)
  {
    x = -x;
    pos = !pos;
  }
  if (x >= 360) x %= 360;
  if (x > 180)
  {
    idx = x - 180;
    pos = !pos;
  }
  else idx = x;
  if (idx > 90) idx = 180 - idx;
  if (pos) return isinTable8[idx] / 2 ;
  return -(isinTable8[idx] / 2);
}

// Cosinus
int icos(int x)
{
  return (isin(x + 90));
}
*/


/* ************************************** vector output stuff **************************************/

// relative functions are not clipped anymore "here"
// clipping could be done in vectrex support
static int old_x=0, old_y=0, new_x=0,new_y=0;
void movetoRelative(int x, int y)
{
  int16_t px, py;
  px = x;
  py = y;

  py = py*16 ;
  px = px*16 ;
  v_directMove32(new_x = old_x + px, new_y = old_y + py); old_x = new_x; old_y = new_y;
}

void linetoRelative(int x, int y)
{
  int16_t px, py;

  px = x;
  py = y;
  py = py*16 ;
  px = px*16 ;
  w_directDraw32(old_x, old_y, new_x = old_x + px, new_y = old_y + py, currentZSH); old_x = new_x; old_y = new_y;
}

// absolut from 0,0 (center)
void moveto(int x, int y)
{
  int16_t px, py;

  //Test!  Very stupid "Clipping"
  if (x >= 4096) x = 4095;
  if (y >= 4096) y = 4095;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  px = x & 0xFFF;
  py = y & 0xFFF;

  py = py*16 -32768;
  px = px*16 -32768;
  w_directMove32(new_x = px, new_y = py); old_x = new_x; old_y = new_y;
}

int draw_character(char c, int x, int y, int size)
{
  const hershey_char_t * const f = &hershey_simplex[c - ' '];
  int next_moveto = 1;

  moveto(x , y );
  int posx = 0;
  int posy = 0;

  for (int i = 0 ; i < f->count ; i++)
  {
    int dx = f->points[2 * i + 0];
    int dy = f->points[2 * i + 1];
    if (dx == -1)
    {
      next_moveto = 1;
      continue;
    }
    dx = (dx * size) * 3 / 4;
    dy = (dy * size) * 3 / 4; //??
    if (next_moveto)
      movetoRelative( dx-posx,  dy-posy);
    else
      linetoRelative( dx-posx,  dy-posy);

   posx=(dx);
   posy=(dy);

   next_moveto = 0;
  }
  return (f->width * size) * 3 / 4;
}


void draw_string(const char * s, int x, int y, int size)
{
  while (*s)
  {
    char c = *s++;
    x += draw_character(c, x, y, size);
  }
}



static void  init_stars(star_t * const stars)
{
  for (uint8_t i = 0 ; i < MAX_STARS ; i++)
  {
    star_t * const s = &stars[i];
    s->x = rand() % 500 + 1750;
    s->y = rand() % 500 + 1750;

    s->vx = rand() % 8 - 4 ;
    s->vy = rand() % 8 - 4 ;
    s->age = rand() % 300;
  }
}

// Debug draw
void draw_rect(int x0, int y0, int x1, int y1)
{
  return;    //Debug!
/*
  moveto(x0, y0);
  lineto(x1, y0);
  lineto(x1, y1);
  lineto(x0, y1);
  lineto(x0, y0);
*/
}

/* ***************************** Game Stuff **************************************************/

// Ähnlich draw_string aber mit definierter Rotation
void draw_object(byte c, int x, int y, int size, int rot)
{
  const objects_char_t * const f = &gobjects[c];
      moveto(x , y );

  int next_moveto = 1;
  int dxx, dyy;
  int posx = 0;
  int posy = 0;

  for (int i = 0 ; i < f->count ; i++)
  {
    int dx = f->points[2 * i + 0];
    int dy = f->points[2 * i + 1];
    if (dx == -127)
    {
      next_moveto = 1;
      continue;
    }
    dxx = ((int)((dx * icos(rot)) - dy * isin(rot))) >> 7 ; // Würg irgendwie nicht Standard, KoordSys komisch?
    dyy = ((int)((dy * icos(rot)) + dx * isin(rot))) >> 7 ;

    dx = (dxx * size) * 3 / 4 ;
    dy = (dyy * size) ;

    if (next_moveto)
      movetoRelative( dx-posx,  dy-posy);
    else
      linetoRelative( dx-posx,  dy-posy);

   posx=(dx);
   posy=(dy);

    next_moveto = 0;
  }
}



// Irgndwie verallgemeinern?
int collision_rock(int x, int y, int d)
{
  int x0, y0, x1, y1;

  d = d * 10;
  x0 = x - d;
  y0 = y - d ;
  x1 = x + d;
  y1 = y + d ;

  draw_rect(x0, y0, x1, y1);
  for (uint8_t i = 0 ; i < MAX_ROCK ; i++)
  {
    if (r[i].t >= 0)
    {
      if (r[i].x > x0 && r[i].x < x1 && r[i].y > y0 && r[i].y < y1)
      {
        //        r[i].t = -1;    //Kill rock also?
        return 1; //Collision with Bullet
      }
    }
  }
  return 0; // No Collision
}



int collision_bullet(int x, int y, int d)
{
  int x0, y0, x1, y1;

  d = d * 10;
  x0 = x - d / 2;
  y0 = y - d / 2;
  x1 = x + d / 2;
  y1 = y + d / 2;

  for (uint8_t i = 0 ; i < MAX_BULLETS ; i++)
  {
    //    bullet_t * const b = &bullets[i];
    if (b[i].age >= 0)
    {
      if (b[i].x > x0 && b[i].x < x1 && b[i].y > y0 && b[i].y < y1)
      {
        b[i].age = -1;
        return 1; //Kollision mit Schuss/Bullet
      }
    }
  }
  return 0; // Keine Kollision
}


// Neuer Schuß wenn eine Slot frei (age==-1)
static void add_bullet(bullet_t * const bullets, ship_t * const ship, int rot)
{
  for (uint8_t i = 0 ; i < MAX_BULLETS ; i++)
  {
    bullet_t * const b = &bullets[i];
    if (b->age < 0)
    {
      b->x = ship->x;
      b->y = ship->y;

      b->rot = rot;
      b->vx = ship->ax ;
      b->vy = ship->ay ;
      b->age = 1;
      break;
    }
  }
}


// Updating bullets/Schüsse
static void update_bullets(bullet_t * const bullets)
{
  for (uint8_t i = 0 ; i < MAX_BULLETS ; i++)
  {
    bullet_t * const b = &bullets[i];
    if (b->age >= 0)
    {
      if (b->age > 100 || (b->x > 2000 - 96 && b->x < 2000 + 96 && b->y > 2000 - 96 && b->y < 2000 + 96))
      {
        b->age = -1;
      }
      else
      {
        b->age++;
        b->x = b->x + (b->vx >> 1);
        b->y = b->y + (b->vy >> 1);
        draw_object(6, b->x, b->y, 10, b->rot);
      }
    }
  }
}


// Schiff Verwaltung
static void update_ship(ship_t * const ship)
{
  long d;
  int rot;

  d = ((2048 - ship->x) * (2048 - ship->x) + (2048 - ship->y) * (2048 - ship->y)) / 75000;
  if (d > 15) d = 15;
  if (d < 1) d = 1;

  if (collision_rock(ship->x, ship->y, d))
  {
    score = 0;
  }

  rot = atan2(2048 - ship->y, 2048 - ship->x) * 180.0 / PI - 90;  // different coord sys...?! Float... hmm

  // button 4 fires
  if ((((currentButtonState&0x08) == (0x08))) && v_millis() > (ship->firedelay + FIREDELAY))
  {
    ship->firedelay = v_millis();
    ship->ax = ((int)-isin(rot)) >> 1  ;
    ship->ay = ((int)icos(rot)) >> 1  ;
    add_bullet(b, ship, rot);
  }

  draw_object(3, ship->x, ship->y, d, rot);               // Ship
  draw_object(4, ship->x, ship->y, d + rand() % d, rot);  // Engine
}


// Hintergrund
static void  update_stars(star_t * const stars)
{
  int age2;
  for (uint8_t i = 0 ; i < MAX_STARS ; i++)
  {
    star_t * const s = &stars[i];
    s->age++;
    age2 = s->age * s->age >> 12;
    s->x = s->x + (s->vx * age2);
    s->y = s->y + (s->vy * age2);
    if (s->x > 4000 || s->x < 96 || s->y > 4000 || s->y < 96 || s->age > 200)
    {
      s->x = rand() % 50 + 2000;
      s->y = rand() % 50 + 2000;
      s->vx = rand() % 8 - 4 ;
      s->vy = rand() % 8 - 4 ;
      s->age = 0;
    }
    draw_character(43, s->x, s->y, age2 >> 1);  // Using a "+" char...
  }
}


// Felsen/Rock/Asteroid
static void  add_rock(rock_t * const rock)
{
  for (uint8_t i = 0 ; i < MAX_ROCK ; i++)
  {
    rock_t * const rr = &rock[i];
    if (rr->t == -1)
    {
      rr->t = rand() % 2 + 13;
      rr->r = rand() % 1000 + 1;
      rr->p = rand() % 359 * 16;

      rr->vr = rand() % 30 + 15;
      rr->vp = rand() % 10 - 5  ;
      break;
    }
  }
}

static void  update_rocks(rock_t * const rr)
{
  int x, y;
  for (uint8_t i = 0 ; i < MAX_ROCK ; i++)
  {
    //rock_t * const rr = &rock[i];
    if (rr[i].t >= 0) // nur wenn live/type gesetzt
    {
      if (rr[i].r < 30000 )
      {
        rr[i].r += rr[i].vr;
      }
      else
      {
        rr[i].t = -1;
        continue;
      }
      rr[i].p = (rr[i].p + rr[i].vp) % (360 * 16) ;
      x = 2048 + (rr[i].r / 16 * icos(rr[i].p / 16)) / 100;
      y = 2048 + (rr[i].r / 16 * isin(rr[i].p / 16)) / 100;
      rr[i].x = x;
      rr[i].y = y;  // Keep track, x,y ToDo: raus oder auf Polarkoords
      rr[i].d = rr[i].r / 512;

      if (collision_bullet(x, y, rr[i].r / 512))
      {
        rr[i].t = -1;
        rr[i].r = 0;
        score += 10;
      }
      else
      {
        draw_object(rr[i].t, x, y, rr[i].d, -rr[i].p / 4);
        draw_rect(r[i].x - r[i].d * 6, r[i].y - r[i].d * 6, r[i].x + r[i].d * 6, r[i].y + r[i].d * 6); // debug
      }
    }
  }
}


// Enemies
static void  add_enemy(enemy_t * const enemy)
{
  for (uint8_t i = 0 ; i < MAX_ENEMY ; i++)
  {
    enemy_t * const rr = &enemy[i];
    if (rr->t == -1)
    {
      rr->t = rand() % 4 + 18;
      rr->r = rand() % 1000 + 1;
      rr->p = rand() % 359 * 16;

      rr->vr = rand() % 30 + 15;
      rr->vp = rand() % 10 - 5  ;
      break;
    }
  }
}

static void  update_enemies(enemy_t * const rr)
{
  int x, y;
  for (uint8_t i = 0 ; i < MAX_ENEMY ; i++)
  {
    if (rr[i].t >= 0) // nur wenn live/type gesetzt
    {
      if (rr[i].r < 10000 )
      {
        rr[i].r += rr[i].vr;
      }

      rr[i].p = (rr[i].p + rr[i].vp) % (360 * 16) ;
      x = 2048 + (rr[i].r / 16 * icos(rr[i].p / 16)) / 100;
      y = 2048 + (rr[i].r / 16 * isin(rr[i].p / 16)) / 100;

      if (collision_bullet(x, y, rr[i].r / 512))
      {
        rr[i].t = -1;
        rr[i].r = 0;
        score += 100;
      }
      else
      {
        draw_object(rr[i].t, x, y, rr[i].r / 512, -rr[i].p / 4);
      }
    }
  }
}

void draw_field()
{
#define CORNER 500

  moveto(0, CORNER/16);
  linetoRelative(0, (int)(-CORNER/16));
  linetoRelative((int)(CORNER/16), 0);

  moveto(4095 - CORNER/16, 0);
  linetoRelative((int)(CORNER/16), 0);
  linetoRelative(0, (int)(CORNER/16));

  moveto(4095, 4095 - CORNER/16);
  linetoRelative(0, (int)(CORNER/16));
  linetoRelative((int)(-CORNER/16), 0);

  moveto(CORNER/16, 4095);
  linetoRelative( (int)(-CORNER/16), 0);
  linetoRelative(0,  (int)(-CORNER/16));
}




int constrain(int in, int min, int max)
{
	return in < min ?min :(in > max ? max : in);
}


// Anzeige Funktion
void video()
{
  // Joystick auslesen
  if (currentJoy1X > DEADX || currentJoy1X < -DEADX)
  {
    ship.x = ship.x + currentJoy1X;
  }
  if (currentJoy1Y > DEADY || currentJoy1Y < -DEADY)
  {
    ship.y = ship.y + currentJoy1Y;
  }

  ship.x = constrain(ship.x, 400, 3700);
  ship.y = constrain(ship.y, 400, 3700);

  v_setBrightness(60);
  update_stars(s);
  v_setBrightness(127);
  update_bullets(b);
  v_setBrightness(74);
  update_rocks(r);
  if (rand() % 500 == 1) add_rock(r);
  v_setBrightness(68);
  update_enemies(e);
  if (rand() % 500 == 1) add_enemy(e);
  v_setBrightness(100);
  update_ship(&ship);
  v_setBrightness(80);
  draw_field();
}

extern int vectrexinit (char viaconfig);

//#define SETTINGS_SIZE 1024
//unsigned char settingsBlob[SETTINGS_SIZE];

/* Setup all */
void setup()
{
  vectrexinit(1);
  v_setName("gyrocks");
  v_init();
  v_set_font(lcrasterlines);
  bufferType = 2; usePipeline = 1; 
  usePipeline = 2; //usePipeline = 0 => no size adjustment, glitches
  v_setRefresh(50);
  window(-28500,-40000,28500,40000);
  initVars();
  init_stars(s);
}

void startFrame()
{
    v_readButtons();
    v_readJoystick1Analog();
    v_WaitRecal(); old_x = 0; old_y = 0; new_x = 0; new_y = 0;
    v_setBrightness(80);        /* set intensity of vector beam... */
}

// Hauptfunktion
void main()
{
  printf("Gyrocks Main\r\n");
  setup();

  int clockAvailable = 0;

  long start_time = v_micros();
  long tick = 0;
  char FPSBuf[32];
  char ScoreBuf[32];

  clockAvailable = start_time!=0;
  sprintf(FPSBuf, "FPS: ");
  sprintf(ScoreBuf, "Points: ");

  if (!clockAvailable) {
    sprintf(FPSBuf+5, "n/a ");
  }

  while (1) {
    video();

    // Punktezähler ausgeben

    itoa(score, ScoreBuf+8, 10); strcat(ScoreBuf, " ");
    v_printStringRaster (-115, 100, ScoreBuf, 5*8, -7, '\0');

    v_printStringRaster (70, 100, FPSBuf, 5*8, -7, '\0');

    if (clockAvailable) {
      //draw_string(itoa(fps, buf, 10), 3400, 150, 6);
      if ((tick&31) == 31) {itoa(fps, FPSBuf+5, 10); strcat(FPSBuf, " ");}
      // slow down the rate at which the fps display is updated.
      fps = 1000000 / (v_micros() - start_time);
      start_time = v_micros();
    }

    startFrame();
    tick++;
  }
}
