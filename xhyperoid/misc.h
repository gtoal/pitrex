/*
 * HYPEROID.H - hyperoid internal header information
 *
 * Version: 1.1  Copyright (C) 1990,91 Hutchins Software
 *      This software is licenced under the GNU General Public Licence
 *      Please read the associated legal documentation
 * Author: Edward Hutchins
 * Revisions:
 * 2000 Mar 22 massively cut down to remove as much Windows
 *		and Pascal-ish bogosity as possible -rjm
 */


/* extra data types and defines */

typedef unsigned char BYTE;

typedef struct { int x,y; } POINT;
typedef struct { int left,right,top,bottom; } RECT;


/* typedefs and defines */
#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

/* color stuff */
#define PALETTE_SIZE 16
typedef enum
{
	BLACK, DKGREY, GREY, WHITE,
	DKRED, RED, DKGREEN, GREEN, DKBLUE, BLUE,
	DKYELLOW, YELLOW, DKCYAN, CYAN, DKMAGENTA, MAGENTA
} HYPEROID_COLORS;

enum
{
	KEY_F1, KEY_TAB, KEY_S,
	KEY_LEFT, KEY_RIGHT, KEY_DOWN, KEY_UP,
	KEY_SPACE, KEY_ESC
};

/* degrees scaled to integer math */
#define DEGREE_SIZE 256
#define DEGREE_MASK 255
#define DEGREE_MAX 0x4000

/* object limits */
#define MAX_PTS 8
#define MAX_OBJS 70 //100
#define MAX_COORD 0x2000
#define CLIP_COORD (MAX_COORD+300)

/* timer stuff */
#define DRAW_DELAY 50
#define RESTART_DELAY_FRAMES 100

/* restart modes */
typedef enum { RESTART_GAME, RESTART_LEVEL, RESTART_NEXTLEVEL } RESTART_MODE;

/* letter scaling */
#define LETTER_MAX 256

/* extra life every */
#define EXTRA_LIFE 100000

/* list node */
typedef struct tagNODE
{
	struct tagNODE  *npNext, *npPrev;
} NODE;

/* list header */
typedef struct
{
	NODE *npHead, *npTail;
} LIST;

/* object descriptor */
typedef struct
{
	NODE    Link;               /* for object list */
	POINT   Pos;                /* position of center of object */
	POINT   Vel;                /* velocity in logical units/update */
	int     nMass;              /* mass of object */
	int     nDir;               /* direction in degrees */
	int     nSpin;              /* angular momentum degrees/update */
	int     nCount;             /* used by different objects */
	int     nDelay;             /* used by different objects */
	BYTE    byColor;            /* palette color */
	BYTE    byPts;              /* number of points in object */
	POINT   Pts[MAX_PTS];       /* points making up an object */
	POINT   Old[MAX_PTS];       /* last plotted location */
} OBJ;


/* inline macro functions */

/* function aliases */
#define AddHeadObj(l,o) AddHead((l),((NODE *)o))
#define RemHeadObj(l) ((OBJ *)RemHead(l))
#define RemoveObj(l,o) Remove((l),((NODE *)o))
#define HeadObj(l) ((OBJ *)((l)->npHead))
#define NextObj(o) ((OBJ *)((o)->Link.npNext))


/* size of an array */
#define DIM(x) (sizeof(x)/sizeof((x)[0]))

/* faster than MulDiv! */
#define MULDEG(x,y) ((int)(((long)(x)*(y))/DEGREE_MAX))

/* DEG - convert an integer into a degree lookup index */
#define DEG(x) ((int)(x)&DEGREE_MASK)

/* ACCEL - accelerate an object in a given direction */
#define ACCEL(o,d,s) \
	(((o)->Vel.x += MULDEG((s),nCos[DEG(d)])), \
	((o)->Vel.y += MULDEG((s),nSin[DEG(d)])))

/* PTINRECT - a faster PtInRect */
#define PTINRECT(r,p) \
	(((r)->left <= (p).x) && ((r)->right > (p).x) && \
	((r)->top <= (p).y) && ((r)->bottom > (p).y))

/* INTRECT - a faster IntersectRect that just returns the condition */
#define INTRECT(r1,r2) \
	(((r1)->right >= (r2)->left) && \
	((r1)->left < (r2)->right) && \
	((r1)->bottom >= (r2)->top) && \
	((r1)->top < (r2)->bottom))

/* MKRECT - make a rect around a point */
#define MKRECT(r,p,s) \
	(((r)->left = ((p).x-(s))), ((r)->right = ((p).x+(s))), \
	((r)->top = ((p).y-(s))), ((r)->bottom = ((p).y+(s))))

/* this seems to be what MulDiv does -rjm */
/* muldiv is supposed to be x*y/z but with a double-precision intermediate result
   so that it is not possible to have overflow.  For example if x,y,z are 32 bit
   integers then x*y should be calculated as a 64-bit integer and that divided
   by z is then expected to fit in a 32 bit result.  - GT */
#define MulDiv(x,y,z) ((x)*(y)/(z))
/* so... assuming always called with ints, on a 32-bit system where long is also 32
   bits, perhaps a better macro would be:
#define MulDiv(x,y,z) ((int)((long long)(x) * (long long)(y) / (long long)(z)))
   Before long long was invented, this was traditionally implemented as a procedure
   written in assembly language.
*/
