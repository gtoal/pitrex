/* svga.c -- adapted for svgalib-Vectrex, Kevin Koster 2020 */
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <vectrex/vectrexInterface.h>
#include "lib/svgalib-vectrex/svgalib-vectrex.h"
#include "lib/svgalib-vectrex/vectrextokeyboard.h"

#include "misc.h"	/* for POINT */
/* #include "convxpm.h" -SVGAvec - unsupported */

#include "graphics.h"


/* bitmaps  -SVGAvec - unsupported
#include "xpm/num0.xpm"
#include "xpm/num1.xpm"
#include "xpm/num2.xpm"
#include "xpm/num3.xpm"
#include "xpm/num4.xpm"
#include "xpm/num5.xpm"
#include "xpm/num6.xpm"
#include "xpm/num7.xpm"
#include "xpm/num8.xpm"
#include "xpm/num9.xpm"
#include "xpm/blank.xpm"
#include "xpm/bomb.xpm"
#include "xpm/level.xpm"
#include "xpm/life.xpm"
#include "xpm/plus.xpm"
#include "xpm/score.xpm"
#include "xpm/shield.xpm"

#define NUM_BITMAPS	17
 */
/* conversions of above (all 16x16)  -SVGAvec - unsupported
unsigned char *bitmaps[NUM_BITMAPS];
 */
/* indicies in above array  -SVGAvec - unsupported
#define BMP_NUM0	0
#define BMP_NUM1	1
#define BMP_NUM2	2
#define BMP_NUM3	3
#define BMP_NUM4	4
#define BMP_NUM5	5
#define BMP_NUM6	6
#define BMP_NUM7	7
#define BMP_NUM8	8
#define BMP_NUM9	9
#define BMP_BLANK	10
#define BMP_BOMB	11
#define BMP_LEVEL	12
#define BMP_LIFE	13
#define BMP_PLUS	14
#define BMP_SCORE	15
#define BMP_SHIELD	16
*/

static int current_x=0,current_y=0;

#ifdef PITREX
/* this leaves 16 lines at the top for score etc. */
static int width=640,height=(640*4)/3,mindimhalf=((640*4)/3)/2 - 16;
#else
/* this leaves 16 lines at the top for score etc. */
static int width=640,height=464,mindimhalf=232;
#endif

/* this oddity is needed to simulate the mapping the original used a
 * Windows call for. MAX_COORD is a power of 2, so when optimised
 * this isn't too evil.
 */
#define convx(x)	(width/2+(x)*mindimhalf/MAX_COORD)
#define convy(y)	(16+height/2-(y)*mindimhalf/MAX_COORD)



int IsKeyDown(int key)
{
switch(key)
  {
  case KEY_F1:		return(keyboard_keypressed(SCANCODE_F1));
  case KEY_TAB:		return(keyboard_keypressed(SCANCODE_TAB));
  case KEY_S:		return(keyboard_keypressed(SCANCODE_S));
  case KEY_LEFT:	return(keyboard_keypressed(SCANCODE_CURSORBLOCKLEFT));
  case KEY_RIGHT:	return(keyboard_keypressed(SCANCODE_CURSORBLOCKRIGHT));
  case KEY_DOWN:	return(keyboard_keypressed(SCANCODE_CURSORBLOCKDOWN));
  case KEY_UP:		return(keyboard_keypressed(SCANCODE_CURSORBLOCKUP));
  case KEY_SPACE:	return(keyboard_keypressed(SCANCODE_SPACE));
  case KEY_ESC:		return(keyboard_keypressed(SCANCODE_ESCAPE));
  default:
    return(0);
  }
}


/* Used its own vector drawing routine, so redirect that to svgalib. */

void drawline(int x1,int y1,int x2,int y2)
{
vga_drawline(x1,y1,x2,y2);
}



void MoveTo(int x,int y)
{
current_x=convx(x);
current_y=convy(y);
}


void LineTo(int x,int y)
{
x=convx(x);
y=convy(y);
drawline(current_x,current_y,x,y);
current_x=x;
current_y=y;
}


void Polyline(POINT *pts,int n)
{
int f;

if(n<2) return;

MoveTo(pts->x,pts->y);
pts++;
for(f=1;f<n;f++,pts++)
  LineTo(pts->x,pts->y);
}


/* doesn't set current_[xy] because hyperoid.c doesn't need it to */
void SetPixel(int x,int y,int c)
{
vga_setcolor(c);
vga_drawpixel(convx(x),convy(y));
}


void set_colour(int c)
{
vga_setcolor(c);
}


/* SetIndicator - set a quantity indicator -SVGAvec - unsupported

int SetIndicator( char * npBuff, char bitmap, int nQuant )
{
	if (nQuant > 5)
	{
		*npBuff++ = bitmap; *npBuff++ = bitmap;
		*npBuff++ = bitmap; *npBuff++ = bitmap;
		*npBuff++ = BMP_PLUS;
	}
	else
	{
		int nBlank = 5 - nQuant;
		while (nQuant--) *npBuff++ = bitmap;
		while (nBlank--) *npBuff++ = BMP_BLANK;
	}
	return( 5 );
}
*/
const signed char shieldList[]=
{   (signed char) 9, // count of vectors
    (signed char) 0x16, (signed char) 0x1C, (signed char) 0xE8, (signed char) 0xFA, // y0, x0, y1, x1
    (signed char) 0xF3, (signed char) 0xDA, (signed char) 0x1D, (signed char) 0xFA, // y0, x0, y1, x1
    (signed char) 0x3A, (signed char) 0xCB, (signed char) 0x3A, (signed char) 0x33, // y0, x0, y1, x1
    (signed char) 0x3A, (signed char) 0x33, (signed char) 0xF9, (signed char) 0x34, // y0, x0, y1, x1
    (signed char) 0xF9, (signed char) 0x34, (signed char) 0xD2, (signed char) 0x21, // y0, x0, y1, x1
    (signed char) 0xD2, (signed char) 0x21, (signed char) 0xB2, (signed char) 0xFE, // y0, x0, y1, x1
    (signed char) 0xB2, (signed char) 0xFE, (signed char) 0xD0, (signed char) 0xDC, // y0, x0, y1, x1
    (signed char) 0xD0, (signed char) 0xDC, (signed char) 0xFA, (signed char) 0xCB, // y0, x0, y1, x1
    (signed char) 0xFA, (signed char) 0xCB, (signed char) 0x3A, (signed char) 0xCB, // y0, x0, y1, x1
};
const signed char heartList[]=
{   (signed char) 16, // count of vectors
    (signed char) 0x17, (signed char) 0x01, (signed char) 0x2F, (signed char) 0xEB, // y0, x0, y1, x1
    (signed char) 0x2F, (signed char) 0xEB, (signed char) 0x46, (signed char) 0xCE, // y0, x0, y1, x1
    (signed char) 0x46, (signed char) 0xCE, (signed char) 0x34, (signed char) 0xB3, // y0, x0, y1, x1
    (signed char) 0x34, (signed char) 0xB3, (signed char) 0x02, (signed char) 0xAE, // y0, x0, y1, x1
    (signed char) 0x02, (signed char) 0xAE, (signed char) 0xD6, (signed char) 0xCA, // y0, x0, y1, x1
    (signed char) 0xD6, (signed char) 0xCA, (signed char) 0xC1, (signed char) 0xE6, // y0, x0, y1, x1
    (signed char) 0xC1, (signed char) 0xE6, (signed char) 0xB3, (signed char) 0x00, // y0, x0, y1, x1
    (signed char) 0xB3, (signed char) 0x00, (signed char) 0xC1, (signed char) 0x1A, // y0, x0, y1, x1
    (signed char) 0xC1, (signed char) 0x1A, (signed char) 0xD6, (signed char) 0x36, // y0, x0, y1, x1
    (signed char) 0xD6, (signed char) 0x36, (signed char) 0x05, (signed char) 0x55, // y0, x0, y1, x1
    (signed char) 0x05, (signed char) 0x55, (signed char) 0x34, (signed char) 0x4D, // y0, x0, y1, x1
    (signed char) 0x34, (signed char) 0x4D, (signed char) 0x45, (signed char) 0x34, // y0, x0, y1, x1
    (signed char) 0x45, (signed char) 0x34, (signed char) 0x32, (signed char) 0x18, // y0, x0, y1, x1
    (signed char) 0x32, (signed char) 0x18, (signed char) 0x17, (signed char) 0x01, // y0, x0, y1, x1
    (signed char) 0x2E, (signed char) 0xCF, (signed char) 0x13, (signed char) 0xC1, // y0, x0, y1, x1
    (signed char) 0x13, (signed char) 0xC1, (signed char) 0xF1, (signed char) 0xCC, // y0, x0, y1, x1
};
const signed char bombList[]=
{   (signed char) 24, // count of vectors
    (signed char) 0x67, (signed char) 0x30, (signed char) 0x5A, (signed char) 0x27, // y0, x0, y1, x1
    (signed char) 0x5B, (signed char) 0x22, (signed char) 0x64, (signed char) 0x13, // y0, x0, y1, x1
    (signed char) 0x43, (signed char) 0x04, (signed char) 0x51, (signed char) 0x0F, // y0, x0, y1, x1
    (signed char) 0x51, (signed char) 0x0F, (signed char) 0x55, (signed char) 0x24, // y0, x0, y1, x1
    (signed char) 0x50, (signed char) 0x27, (signed char) 0x47, (signed char) 0x2E, // y0, x0, y1, x1
    (signed char) 0x0E, (signed char) 0x25, (signed char) 0xF2, (signed char) 0x25, // y0, x0, y1, x1
    (signed char) 0xF2, (signed char) 0x25, (signed char) 0xD9, (signed char) 0x15, // y0, x0, y1, x1
    (signed char) 0xD9, (signed char) 0x15, (signed char) 0xD2, (signed char) 0xF1, // y0, x0, y1, x1
    (signed char) 0x00, (signed char) 0xC6, (signed char) 0x18, (signed char) 0xCA, // y0, x0, y1, x1
    (signed char) 0x18, (signed char) 0xCA, (signed char) 0x26, (signed char) 0xD5, // y0, x0, y1, x1
    (signed char) 0x26, (signed char) 0xD5, (signed char) 0x38, (signed char) 0xEA, // y0, x0, y1, x1
    (signed char) 0x38, (signed char) 0xEA, (signed char) 0x3C, (signed char) 0x00, // y0, x0, y1, x1
    (signed char) 0x3C, (signed char) 0x00, (signed char) 0x38, (signed char) 0x14, // y0, x0, y1, x1
    (signed char) 0x38, (signed char) 0x14, (signed char) 0x26, (signed char) 0x2A, // y0, x0, y1, x1
    (signed char) 0x26, (signed char) 0x2A, (signed char) 0x18, (signed char) 0x36, // y0, x0, y1, x1
    (signed char) 0x18, (signed char) 0x36, (signed char) 0xFF, (signed char) 0x3A, // y0, x0, y1, x1
    (signed char) 0xFF, (signed char) 0x3A, (signed char) 0xE8, (signed char) 0x36, // y0, x0, y1, x1
    (signed char) 0xE8, (signed char) 0x36, (signed char) 0xDA, (signed char) 0x2A, // y0, x0, y1, x1
    (signed char) 0xDA, (signed char) 0x2A, (signed char) 0xC8, (signed char) 0x15, // y0, x0, y1, x1
    (signed char) 0xC8, (signed char) 0x15, (signed char) 0xC4, (signed char) 0x00, // y0, x0, y1, x1
    (signed char) 0xC4, (signed char) 0x00, (signed char) 0xC8, (signed char) 0xEB, // y0, x0, y1, x1
    (signed char) 0xC8, (signed char) 0xEB, (signed char) 0xDA, (signed char) 0xD6, // y0, x0, y1, x1
    (signed char) 0xDA, (signed char) 0xD6, (signed char) 0xE8, (signed char) 0xCA, // y0, x0, y1, x1
    (signed char) 0xE8, (signed char) 0xCA, (signed char) 0x00, (signed char) 0xC6, // y0, x0, y1, x1
};

/* score_graphics - draw score and rest of status display */

void displayList(int x,int y, const signed char list[])
{
  int count = *list++;
  x = x<<7;
  y = y<<7;
  
  
  while (count >0)  
  {
    int y0 = *list++;
    int x0 = *list++;
    int y1 = *list++;
    int x1 = *list++;
    v_directDraw32(x+(x0<<3), y+(y0<<3),x+(x1<<3),y+(y1<<3), 0x30);
    count --;
  }
}

void score_graphics(int level,int score,int lives,int shield,int bomb)
{
static char scoreLine1[128];
static char scoreLine2[128];
static char scoreLine3[128];
sprintf(scoreLine1, "LEV %2.2d", level);
sprintf(scoreLine2, "%7.7d", score);
sprintf(scoreLine3, "LIVES %1.1d  SHIELDS %1.1d  BOMBS %1.1d", lives, shield, bomb);
//v_printStringRaster(-8, -4, scoreLine, 5 * 8, -7, '\0'); /* - looks bad with Linux glitches */
//v_printString(-127, 127, scoreLine, 10, 85); // NEEDS TO BE SCALED ETC BY window.c

 v_printString(-110 /* x */, 110 /* y */, scoreLine1, 8 /* size */, 70 /* brightness */); // NEEDS TO BE SCALED ETC BY window.c
 v_printString(50 /* x */, 110 /* y */, scoreLine2, 8 /* size */, 80 /* brightness */); // NEEDS TO BE SCALED ETC BY window.c
// v_printString(-60 /* x */, -127 /* y */, scoreLine3, 4 /* size */, 55 /* brightness */); // NEEDS TO BE SCALED ETC BY window.c

if (lives >=3)
  displayList(-72,-110, heartList);
if (lives >1)
  displayList(-60,-110, heartList);
if (lives >0)
  displayList(-48,-110, heartList);
 
if (shield >=3)
  displayList(-12,-110, shieldList);
if (shield >1)
  displayList(-00,-110, shieldList);
if (shield >0)
  displayList( 12,-110, shieldList);
 
if (bomb >=3)
  displayList(48,-113, bombList);
if (bomb >1)
  displayList(60,-113, bombList);
if (bomb >0)
  displayList(72,-113, bombList);

/* -SVGAvec - unsupported -- Possible to draw xpms via Vectrex raster routines?
static char szScore[40];
char szBuff[sizeof(szScore)];
char *npBuff = szBuff;
int nLen, x, y;

*npBuff++ = BMP_LEVEL;
sprintf( npBuff, "%2.2d", level );
while (isdigit( *npBuff ))
	*npBuff = (char)(*npBuff + BMP_NUM0 - '0'), ++npBuff;
*npBuff++ = BMP_BLANK;
*npBuff++ = BMP_SCORE;
sprintf( npBuff, "%7.7d", score );
while (isdigit( *npBuff ))
	*npBuff = (char)(*npBuff + BMP_NUM0 - '0'), ++npBuff;
*npBuff++ = BMP_BLANK;
npBuff += SetIndicator( npBuff, BMP_LIFE, lives );
npBuff += SetIndicator( npBuff, BMP_SHIELD, shield );
npBuff += SetIndicator( npBuff, BMP_BOMB, bomb );
nLen = npBuff - szBuff;

for(y=0;y<16;y++)
  for(x=0;x<nLen;x++)
    vga_drawscansegment(bitmaps[(int)szBuff[x]]+y*16,x*16,y,16);
*/
}

/* -SVGAvec - unsupported
void conv_xpms(int *palrgb)
{
int f;

bitmaps[BMP_NUM0]=xpm2bytemap(num0_xpm,palrgb);
bitmaps[BMP_NUM1]=xpm2bytemap(num1_xpm,palrgb);
bitmaps[BMP_NUM2]=xpm2bytemap(num2_xpm,palrgb);
bitmaps[BMP_NUM3]=xpm2bytemap(num3_xpm,palrgb);
bitmaps[BMP_NUM4]=xpm2bytemap(num4_xpm,palrgb);
bitmaps[BMP_NUM5]=xpm2bytemap(num5_xpm,palrgb);
bitmaps[BMP_NUM6]=xpm2bytemap(num6_xpm,palrgb);
bitmaps[BMP_NUM7]=xpm2bytemap(num7_xpm,palrgb);
bitmaps[BMP_NUM8]=xpm2bytemap(num8_xpm,palrgb);
bitmaps[BMP_NUM9]=xpm2bytemap(num9_xpm,palrgb);

bitmaps[BMP_BLANK ]=xpm2bytemap(blank_xpm ,palrgb);
bitmaps[BMP_BOMB  ]=xpm2bytemap(bomb_xpm  ,palrgb);
bitmaps[BMP_LEVEL ]=xpm2bytemap(level_xpm ,palrgb);
bitmaps[BMP_LIFE  ]=xpm2bytemap(life_xpm  ,palrgb);
bitmaps[BMP_PLUS  ]=xpm2bytemap(plus_xpm  ,palrgb);
bitmaps[BMP_SCORE ]=xpm2bytemap(score_xpm ,palrgb);
bitmaps[BMP_SHIELD]=xpm2bytemap(shield_xpm,palrgb);

for(f=0;f<NUM_BITMAPS;f++)
  if(bitmaps[f]==NULL)
    fprintf(stderr,"bad XPM data, conversion failed!\n"),exit(1);
}
*/

/* note that this actually changes palrgb[] (ouch :-)) */
void graphics_init(int argc,char *argv[],int *palrgb)
{
int f;

vga_init();
/* conv_xpms(palrgb); -SVGAvec - unsupported */

#ifdef PITREX
 vga_setmode(G640x854x128); // approximate vectrex capabilities
#else
if(vga_hasmode(G640x480x256))
  vga_setmode(G640x480x256);
else
  vga_setmode(G640x480x16);
#endif
for(f=0;f<16*3;f++) palrgb[f]>>=2;
vga_setpalvec(0,16,palrgb);

keyboard_init();
}


void graphics_update(void)
{
  // keyboard_update(); only needs to done once per frame at top level next to waitrecal
}


void graphics_exit(void)
{
keyboard_close();
vga_setmode(TEXT);
}
