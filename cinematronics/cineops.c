#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//#define DEBUGGING_STARHAWK 1
#include <vectrex/osWrapper.h>
#include <vectrex/vectrexInterface.h>

#include "cines.h"
#include "options.h"

// these should be in the vectrex library
#define VEC_BUTTON_1_1 0x01
#define VEC_BUTTON_1_2 0x02
#define VEC_BUTTON_1_3 0x04
#define VEC_BUTTON_1_4 0x08
#define VEC_BUTTON_2_1 0x10
#define VEC_BUTTON_2_2 0x20
#define VEC_BUTTON_2_3 0x40
#define VEC_BUTTON_2_4 0x80

// Put any switch or IO bits that are common to all machines here.
#define SW_ABORT  0x100		/* for ioSwitches */

static int world_xl = -1800, world_yb = -2400, world_xr = 1800, world_yt = 2400; // dummy values should never be used

unsigned long isqrt(unsigned long x)
{
  register unsigned long op, res, one;

  op = x;
  res = 0;

  /* "one" starts at the highest power of four <= than the argument. */
  one = 1 << 30;  /* second-to-top bit set */
  while (one > op) one >>= 2;

  while (one != 0) {
    if (op >= res + one) {
      op -= res + one;
      res += one << 1;  // <-- faster than 2 * one
    }
    res >>= 1;
    one >>= 2;
  }
  return res;
}

int /*fp14*/ sine[] =
  {
    0,    402,    803,   1205,   1605,   2005,   2404,   2801,
    3196,   3589,   3980,   4369,   4756,   5139,   5519,   5896,
    6269,   6639,   7005,   7366,   7723,   8075,   8423,   8765,
    9102,   9434,   9759,  10079,  10393,  10701,  11002,  11297,
    11585,  11866,  12139,  12406,  12665,  12916,  13159,  13395,
    13622,  13842,  14053,  14255,  14449,  14634,  14810,  14978,
    15136,  15286,  15426,  15557,  15678,  15790,  15892,  15985,
    16069,  16142,  16206,  16260,  16305,  16339,  16364,  16379,
    16384,  16379,  16364,  16339,  16305,  16260,  16206,  16142,
    16069,  15985,  15892,  15790,  15678,  15557,  15426,  15286,
    15136,  14978,  14810,  14634,  14449,  14255,  14053,  13842,
    13622,  13395,  13159,  12916,  12665,  12406,  12139,  11866,
    11585,  11297,  11002,  10701,  10393,  10079,   9759,   9434,
    9102,   8765,   8423,   8075,   7723,   7366,   7005,   6639,
    6269,   5896,   5519,   5139,   4756,   4369,   3980,   3589,
    3196,   2801,   2404,   2005,   1605,   1205,    803,    402,
    0,   -402,   -803,  -1205,  -1605,  -2005,  -2404,  -2801,
    -3196,  -3589,  -3980,  -4369,  -4756,  -5139,  -5519,  -5896,
    -6269,  -6639,  -7005,  -7366,  -7723,  -8075,  -8423,  -8765,
    -9102,  -9434,  -9759, -10079, -10393, -10701, -11002, -11297,
    -11585, -11866, -12139, -12406, -12665, -12916, -13159, -13395,
    -13622, -13842, -14053, -14255, -14449, -14634, -14810, -14978,
    -15136, -15286, -15426, -15557, -15678, -15790, -15892, -15985,
    -16069, -16142, -16206, -16260, -16305, -16339, -16364, -16379,
    -16384, -16379, -16364, -16339, -16305, -16260, -16206, -16142,
    -16069, -15985, -15892, -15790, -15678, -15557, -15426, -15286,
    -15136, -14978, -14810, -14634, -14449, -14255, -14053, -13842,
    -13622, -13395, -13159, -12916, -12665, -12406, -12139, -11866,
    -11585, -11297, -11002, -10701, -10393, -10079,  -9759,  -9434,
    -9102,  -8765,  -8423,  -8075,  -7723,  -7366,  -7005,  -6639,
    -6269,  -5896,  -5519,  -5139,  -4756,  -4369,  -3980,  -3589,
    -3196,  -2801,  -2404,  -2005,  -1605,  -1205,   -803,   -402,
  };

FILE *debugf = NULL;

//#define DEBUG_OUT(...) do { if (!debugf) debugf = fopen("debug-cinema.log", "w"); if (debugf) { fprintf(debugf,__VA_ARGS__); fflush(debugf); } } while(0)
#define DEBUG_OUT(...)

#ifndef FALSE
#define FALSE (0!=0)
#define TRUE (0==0)
#endif

// Start of CCPU emulator

int ccpudebug = 0;		/* default is off (0) */

#define CARRYBIT (1<<12)
#define A0BIT 1

/* Contorted sign-extend macro only evaluates its parameter once, and
   executes in two shifts.  This is far more efficient that any other
   sign extend procedure I can think of, and relatively safe */

#define SEX(twelvebit) ((((int)twelvebit) << (int)((sizeof(int)*8)-12)) \
                          >> (int)((sizeof(int)*8)-12))

/* Mouse values currently sampled once per frame and set in WAI code.
   This is not ideal. */

/* RCstate setting macros to exit opcode handler */
#define jumpCineRet_A     do {RCstate = RCstate_A; goto cineExecBottom;} while (0)
#define jumpCineRet_B     do {RCstate = RCstate_B; goto cineExecBottom;} while (0)
#define jumpCineRet_AA    do {RCstate = RCstate_AA; goto cineExecBottom;} while (0)
#define jumpCineRet_BB    do {RCstate = RCstate_BB; goto cineExecBottom;} while (0)
#define jumpCineRet_WEIRD do (DEBUG_OUT( "Didn't bother yet.\n" ); exit ( 0 );} while (0)

/* Handy new operators ... */

/* for Zonn translation :) */
#define SAR16(var,arg)    ( ( (signed short int) var ) >> arg )

/* for setting/checking the A0 flag */
#define SETA0(var)    ( RCacc_a0 = var )
#define GETA0()       ( RCacc_a0 )

/* for setting/checking the Carry flag */
#define SETFC(val)    ( RCflag_C = val )
#define GETFC()       ( ( RCflag_C >> 8 ) & 0xFF )

/* Define new types for the c-cpu emulator */
typedef short unsigned int RCCINEWORD;	/* 12bits on the C-CPU */
typedef unsigned char RCCINEBYTE;	/* 8 (or less) bits on the C-CPU */
typedef short signed int RCCINESWORD;	/* 12bits on the C-CPU */
typedef signed char RCCINESBYTE;	/* 8 (or less) bits on the C-CPU */

typedef enum
{
   RCstate_A = 0, RCstate_AA, RCstate_B, RCstate_BB
} RCCINESTATE;			/* current RCstate */

volatile static UINT32 dwElapsedTicks = 0;
static UINT8 bBailOut = FALSE;

/* C-CPU context information begins -- */
RCCINEWORD RCregister_PC = 0;	/* C-CPU registers; program counter */
RCCINEWORD RCregister_A = 0;	/* ; A-Register (accumulator) */
RCCINEWORD RCregister_B = 0;	/* ; B-Register (accumulator) */
RCCINEBYTE RCregister_I = 0;	/* ; I-Register (last access RAM location) */
RCCINEWORD RCregister_J = 0;	/* ; J-Register (target address for JMP
				   opcodes) */
RCCINEBYTE RCregister_P = 0;	/* ; Page-Register (4 bits) */
RCCINEWORD RCFromX = 0;		/* ; X-Register (start of a vector) */
RCCINEWORD RCFromY = 0;		/* ; Y-Register (start of a vector) */
RCCINEWORD RCregister_T = 0;	/* ; T-Register (vector draw length timer) */
RCCINEWORD RCflag_C = 0;	/* C-CPU flags; carry. Is word sized, instead
				   of RCCINEBYTE, so we can do direct
				   assignment and then change to BYTE during
				   inspection. */

RCCINEWORD RCcmp_old = 0;	/* last accumulator value */
RCCINEWORD RCcmp_new = 0;	/* new accumulator value */
RCCINEBYTE RCacc_a0 = 0;	/* bit0 of A-reg at last accumulator access */

RCCINESTATE RCstate = RCstate_A;	/* C-CPU RCstate machine current
					   RCstate */
RCCINEWORD RCram[256];		/* C-CPU RCram (for all pages) */

static UINT8 rom[0x8000];	// = tailgunner was 0x2000 - others may be
				// larger

static char ccpu_game_name[32];	/* canonical name we now use as cinemu
				   parameter */
static int ccpu_game_id = 0;    /* as set by cineSetGame */
static UINT8 ccpu_jmi_dip = 1;	/* as set by cineSetJMI */
static UINT8 ccpu_msize = 0;	/* as set by cineSetMSize */
static UINT8 ccpu_monitor = 0;	/* as set by cineSetMonitor */

/* cinem graphics junk */
RCCINEBYTE RCvgShiftLength = 0;	/* number of shifts loaded into length reg */

/* -- Context information ends. */

UINT8 bFlipX;
UINT8 bFlipY;
UINT8 bSwapXY;
UINT8 bOverlay;

UINT16 ioSwitches = 0xFF;	/* high values for non-triggered switches */
UINT16 ioInputs = 0;
UINT8 ioOutputs = 0;

UINT16 RCvgColour = 0;

INT16 JoyX;
INT16 JoyY;
UINT8 bNewFrame;

INT32 sdwGameXSize;
INT32 sdwGameYSize;
INT32 sdwXOffset = 0;
INT32 sdwYOffset = 0;

// This will move to the vectrex graphics library.
static int64_t ScaleXMul = 1LL, ScaleXDiv = 1LL, ScaleXOffsetPre = 0LL, ScaleXOffsetPost = 0LL,
               ScaleYMul = 1LL, ScaleYDiv = 1LL, ScaleYOffsetPre = 0LL, ScaleYOffsetPost = 0LL;
static int v_rotate = 0, v_flip_x = 0, v_flip_y = 0;
static int v_xl = 0, v_yb = 0, v_xr = 0, v_yt = 0;

int tx (int x) {				// convert x from window to viewport
   if (v_flip_x) x = v_xr - (x - v_xl);
   return (int) (((((int64_t) x) + ScaleXOffsetPre) * ScaleXMul) / ScaleXDiv + ScaleXOffsetPost);
}

int ty (int y) {				// and y
   if (v_flip_y) y = v_yt - (y - v_yb);
   return (int) (((((int64_t) y) + ScaleYOffsetPre) * ScaleYMul) / ScaleYDiv + ScaleYOffsetPost);
}

int frameCounter = 0; // as long as this framework only runs one game once.
static int cineTwinkle = 255;	// off by default
void line (int xl, int yb, int xr, int yt, int col)
{
   if (v_rotate) {
      int tmp;
      tmp = xl; xl = yb; yb = tmp;
      tmp = xr; xr = yt; yt = tmp;
      xl = v_xr - (xl - v_xl);
      xr = v_xr - (xr - v_xl);
  }

   if ((ccpu_game_id == GAME_TAILGUNNER) && (xl==xr) && (yb==yt)) {
     // stars...  tweak the intensity according to the distance from the center
     int SCREEN_W = world_xr-world_xl, SCREEN_H = world_yt-world_yb;
     int dist = isqrt((SCREEN_W/2 - xl)*(SCREEN_W/2 - xl) + (SCREEN_H/2 - yb)*(SCREEN_H/2 - yb));
     int maxdist = isqrt((SCREEN_W/2)*(SCREEN_W/2) + (SCREEN_H/2)*(SCREEN_H/2));
     if (maxdist == 0) maxdist = 1;
     if (col == 7) {
       col = (col - (((maxdist-dist) * 7) / maxdist) + 1)*2+1;
     }
     v_directDraw32Patterned (tx (xl), ty (yb), tx (xr), ty (yt), (col + 1) * 8 - 1, random());
   } else if ((ccpu_game_id == GAME_TAILGUNNER) && (RCregister_PC == 0x03B2)) {
     // 0x1BBD - PUSH START
     // 0x03B2 - 3 lines in the shields have Twinkle values
     // Tailgunner's shields are funny and need fixing.
     v_directDraw32Patterned (tx (xl), ty (yb), tx (xr), ty (yt), 127, random ()); //one way to make sparky shields...
     //DEBUG_OUT("line(%d,%d, %d,%d, %02x) at PC=0x%04x;\n", xl,yb, xr,yt, col, RCregister_PC);
   } else {
     /* Twinkle may be 0 (sundance), or 7-9 */
     if (col <= cineTwinkle) {
       v_directDraw32 (tx (xl), ty (yb), tx (xr), ty (yt), (col + 1) * 8 - 1);
     } else { // twinkle parameter denotes flash.
       // check armor attack - cineTwinkle SHOULD be 255, but is it?
       v_directDraw32 (tx (xl), ty (yb), tx (xr), ty (yt), (127 * (sine[(frameCounter<<3)&255]+16384+32768)) / 65536); // brightness 63..127
     }
   }
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

   printf("old window: (%d,%d) to (%d,%d)\n", v_xl,v_yb, v_xr,v_yt);
   printf("new window: (%d,%d) to (%d,%d)\n", xl,yb, xr,yt);
   
   ScaleXMul = 36000LL;
   ScaleXDiv = width;

   //ScaleXOffsetPre = -width / 2LL;
   ScaleXOffsetPre = 0LL;
   ScaleXOffsetPost = 0LL;
   // adjust center of window to center of screen
   ScaleXOffsetPost =  -(tx (v_xr)+tx (v_xl)) / 2LL;

   
   //int tx (int x) {				// convert x from window to viewport
   //   if (v_flip_x) x = v_xr - (x - v_xl);
   //   return (int) (((((int64_t) x) + ScaleXOffsetPre) * ScaleXMul) / ScaleXDiv + ScaleXOffsetPost);
   //}

   ScaleYMul = 48000LL;
   ScaleYDiv = height;

   ScaleYOffsetPre = 0LL; // -height / 2LL;
   ScaleYOffsetPost = 0LL;
   // adjust center of window to center of screen
   ScaleYOffsetPost = -(ty (v_yt)+ty (v_yb)) / 2LL;

   printf("X: scale %d, pre %d, post %d\n", ScaleXMul/ScaleXDiv,  ScaleXOffsetPre,  ScaleXOffsetPost);
   printf("Y: scale %d, pre %d, post %d\n", ScaleYMul/ScaleYDiv,  ScaleYOffsetPre,  ScaleYOffsetPost);
   printf("new transformed window: (%d,%d) to (%d,%d)\n", tx(xl),ty(yb), tx(xr),ty(yt));
   //int ty (int y) {				// and y
   //   if (v_flip_y) y = v_yt - (y - v_yb);
   //   return (int) (((((int64_t) y) + ScaleYOffsetPre) * ScaleYMul) / ScaleYDiv + ScaleYOffsetPost);
   //}

   setCustomClipping(TRUE, tx(v_xl), ty(v_yb), tx(v_xr), ty(v_yt));
       // transform world (window) coordinates to viewport (normalised device
       // coordinates) before clipping.  That way clipping code does not need to know about world
       // coordinates. NOT SURE IF THIS IS WORKING. Speedfreak seems to draw lines outside the window.
}

// end of window library

void startFrame (void)		// generic default
{
   frameCounter += 1;
   DEBUG_OUT("v_WaitRecal(); v_setBrightness(64);v_readButtons();v_readJoystick1Analog(); // %d\n", frameCounter);
   v_WaitRecal ();
   // v_doSound();
   v_readButtons ();
   v_readJoystick1Analog ();
   // v_playAllSFX();
   if (frameCounter < 500) {
     if (v_rotate) {
       line(v_yb,v_xl, v_yt,v_xl, 6);
       line(v_yt,v_xl, v_yt,v_xr, 6);
       line(v_yt,v_xr, v_yb,v_xr, 6);
       line(v_yb,v_xr, v_yb,v_xl, 6);
     } else {
       line(v_xl,v_yb, v_xl,v_yt, 6);
       line(v_xl,v_yt, v_xr,v_yt, 6);
       line(v_xr,v_yt, v_xr,v_yb, 6);
       line(v_xr,v_yb, v_xl,v_yb, 6);
     }
   }
}

void startFrame_ripoff(void) {

#define RO_IO_P1START   0x02  // 1-player start
#define RO_IO_P1LEFT    0x1000
#define RO_IO_P1RIGHT   0x4000
#define RO_IO_P1THRUST  0x8000
#define RO_IO_P1FIRE    0x2000

#define RO_IO_P2START   0x08  // 2-player start
#define RO_IO_P2LEFT    0x01
#define RO_IO_P2RIGHT   0x04
#define RO_IO_P2THRUST  0x10
#define RO_IO_P2FIRE    0x20
  
#define RO_SW_ABORT   SW_ABORT	/* for ioSwitches */
#define RO_SW_COIN    0x080
  
   static int prevButtonState;	// for debouncing

   frameCounter += 1;
   DEBUG_OUT("// Frame %d\n", frameCounter);

   v_WaitRecal ();
   // v_doSound();
   prevButtonState = currentButtonState;
   v_readButtons ();		// update currentButtonState
   v_readJoystick1Analog ();
   //v_readJoystick2Analog ();  // Apparently we are getting joystick 2 data anyway?????
   // v_playAllSFX();

   // Unfortunately, it's quite common to press LEFT and RIGHT simultaneously by accident with this
   // layout, and accidentally invoke the configuration screen.  Need to think about how we will
   // handle this...
   
   // default inactive:
   ioInputs |= RO_IO_P1LEFT | RO_IO_P1RIGHT | RO_IO_P1THRUST | RO_IO_P1FIRE | RO_IO_P1START | 
               RO_IO_P2LEFT | RO_IO_P2RIGHT | RO_IO_P2THRUST | RO_IO_P2FIRE | RO_IO_P2START;
   ioSwitches |= RO_SW_COIN;

   // digital joysticks...
   if (currentJoy1X < -30) ioInputs &= ~RO_IO_P1LEFT;
   if (currentJoy1X > 30) ioInputs &= ~RO_IO_P1RIGHT;
   if (currentJoy1Y > 30) ioInputs &= ~RO_IO_P1THRUST;

   if (currentJoy2X < -30) ioInputs &= ~RO_IO_P2LEFT;
   if (currentJoy2X > 30) ioInputs &= ~RO_IO_P2RIGHT;
   if (currentJoy2Y > 30) ioInputs &= ~RO_IO_P2THRUST;

   // would be helpful to examine ram and make these context-dependent

   if ((currentButtonState & ~prevButtonState) & (VEC_BUTTON_1_1 | VEC_BUTTON_2_1)) ioSwitches &= ~RO_SW_COIN;	// only on rising edge
   if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~RO_IO_P1START; // needs 1 coin
   if (currentButtonState & VEC_BUTTON_2_2) ioInputs &= ~RO_IO_P2START; // needs 2 coins and second controller

   if (currentButtonState & VEC_BUTTON_1_1) ioInputs &= ~RO_IO_P1LEFT;
   if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~RO_IO_P1RIGHT;
   if (currentButtonState & VEC_BUTTON_1_3) ioInputs &= ~RO_IO_P1THRUST;
   if (currentButtonState & VEC_BUTTON_1_4) ioInputs &= ~RO_IO_P1FIRE;

   if (currentButtonState & VEC_BUTTON_2_1) ioInputs &= ~RO_IO_P2LEFT;
   if (currentButtonState & VEC_BUTTON_2_2) ioInputs &= ~RO_IO_P2RIGHT;
   if (currentButtonState & VEC_BUTTON_2_3) ioInputs &= ~RO_IO_P2THRUST;
   if (currentButtonState & VEC_BUTTON_2_4) ioInputs &= ~RO_IO_P2FIRE;

#ifdef NEVER
  /*
# Initialization file for Rip Off (Single Player)
>
>*** Rip Off (Single Player) ***
>
>Keyboard Mapping:
>
>   One Player  = F1
>   Two Players = F2
>   Coin        = F3
>   Reset Game  = F4
>   Exit        = <Esc>
>
>   Left        = 'Z'
>   Right       = 'X'
>   Thrust      = '.'
>   Fire        = '/'

 ; Switch definitions:
 ;
 ;   D------  0=Normal, 1=Diagnostics
 ;   -O-----  0=Individual Scores, 1=Combined Scores
 ;   --S----  0=Sound in attract mode, 1=No Sound (sound not supported)
 ;
 ;   ---CC--  11 = 1 credit per 1 quarter
 ;            10 = 1 credit per 2 quarters
 ;            01 = 3 credits per 2 quarters
 ;            00 = 3 credits per 4 quarters
 ;
 ;   -----TT  11 = 12 fuel pods
 ;            10 = 4 fuel pods
 ;            00 = 8 fuel pods
 ;            01 = 16 fuel pods

 Switches=0011101

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit 
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF

 Player1 = 00000000,FFFFFFFD,00000002,FFFFFFFF
 Player2 = 00000000,FFFFFFF7,00000008,FFFFFFFF

 RFire   = 00000000,FFFFDFFF,00002000,FFFFFFFF
 RThrust = 00000000,FFFF7FFF,00008000,FFFFFFFF
 RRight  = 00000000,FFFFBFFF,00004000,FFFFFFFF
 RLeft   = 00000000,FFFFEFFF,00001000,FFFFFFFF

 LFire   = 00000000,FFFFFFDF,00000020,FFFFFFFF
 LThrust = 00000000,FFFFFFEF,00000010,FFFFFFFF
 LRight  = 00000000,FFFFFFFB,00000004,FFFFFFFF
 LLeft   = 00000000,FFFFFFFE,00000001,FFFFFFFF
   */
#endif
   if (frameCounter < 500) {
     if (v_rotate) {
       line(v_yb,v_xl, v_yt,v_xl, 6);
       line(v_yt,v_xl, v_yt,v_xr, 6);
       line(v_yt,v_xr, v_yb,v_xr, 6);
       line(v_yb,v_xr, v_yb,v_xl, 6);
     } else {
       line(v_xl,v_yb, v_xl,v_yt, 6);
       line(v_xl,v_yt, v_xr,v_yt, 6);
       line(v_xr,v_yt, v_xr,v_yb, 6);
       line(v_xr,v_yb, v_xl,v_yb, 6);
     }
   }
}

#ifdef NEVER
void spacewar_input()
{
  //Spacewar inputs
  ioInputs = 0xffff;

  ioSwitches = 0x00cf; //00ff=45 00ef=2min  00df 1:30 00cf 1min
  // if (GkeyCheck(config.kcoin1)){ioSwitches &= ~SW_COIN;}

  if (key[config.kp1left])   {ioInputs -= 0x0100;}
  if (key[config.kp1right])  {ioInputs -= 0x2000;}
  if (key[config.kp1but1])   {ioSwitches -= 0x04;}
  if (key[config.kp1but2])   {ioInputs -= 0x8000;}
  if (key[config.kp1but3])   {ioSwitches -= 0x02;}

  if (key[config.kp2left])   {ioInputs -= 0x4000;}
  if (key[config.kp2right])  {ioInputs -= 0x1000;}
  if (key[config.kp2but1])   {ioSwitches -= 0x01;}
  if (key[config.kp2but2])   {ioInputs -= 0x0200;}
  if (key[config.kp2but3])   {ioSwitches -= 0x08;}

  if (key[config.pad9])   {ioInputs -= 0x0008;}
  if (key[config.pad8])   {ioInputs -= 0x0002;}
  if (key[config.pad7])   {ioInputs -= 0x0080;}
  if (key[config.pad6])   {ioInputs -= 0x0020;}
  if (key[config.pad5])   {ioInputs -= 0x0400;}
  if (key[config.pad4])   {ioInputs -= 0x0004;}
  if (key[config.pad3])   {ioInputs -= 0x0001;}
  if (key[config.pad2])   {ioInputs -= 0x0040;}
  if (key[config.pad1])   {ioInputs -= 0x0010;}
  if (key[config.pad0])   {ioInputs -= 0x0800;}
}
#endif

void startFrame_spacewars(void) {

#define SW_IO_P1LEFT    0x0100
#define SW_IO_P1RIGHT   0x2000
#define SW_IO_P1THRUST  0x8000

#define SW_SW_P1FIRE    0x04
#define SW_SW_P1HYPER   0x02  // not yet assigned to a key

#define SW_IO_P2LEFT    0x4000
#define SW_IO_P2RIGHT   0x1000
#define SW_IO_P2THRUST  0x0200

#define SW_SW_P2FIRE    0x01
#define SW_SW_P2HYPER   0x08  // not yet assigned to a key

// not yet assigned to keys:
#define SW_IO_Zero    0x0800
#define SW_IO_One     0x0010
#define SW_IO_Two     0x0040
#define SW_IO_Three   0x0001
#define SW_IO_Four    0x0004
#define SW_IO_Five    0x0400
#define SW_IO_Six     0x0020
#define SW_IO_Seven   0x0080
#define SW_IO_Eight   0x0002
#define SW_IO_Nine    0x0008

#define SW_SW_ABORT   SW_ABORT	/* for ioSwitches */
#define SW_SW_COIN    0x080

   // Default of Switches=0000011 doesn't seem compatible with SW_SW_P2FIRE=0x01 and SW_SW_P1HYPER=0x02
   // Code from AAE above suggests the duration bits are the high two bits rather than the low two,
   // so either we have a big/little-endian situation, or it's something to do with the shuffling of
   // switch bits also mentioned in the AAE code. (not shown)
  
   static int prevButtonState;	// for debouncing

   frameCounter += 1;
   DEBUG_OUT("// %d\n", frameCounter);

   v_WaitRecal ();
   // v_doSound();
   prevButtonState = currentButtonState;
   v_readButtons ();		// update currentButtonState
   v_readJoystick1Analog ();
   //v_readJoystick2Analog ();  // NOT YET IMPLEMENTED.
   // v_playAllSFX();

   // Unfortunately, it's quite common to press LEFT and RIGHT simultaneously by accident with this
   // layout, and accidentally invoke the configuration screen.  Need to think about how we will
   // handle this...
   
   // default inactive:
   ioInputs = 0xffff; ioSwitches = 0x00cf; // force for now...

   ioInputs |= SW_IO_P1LEFT | SW_IO_P1RIGHT | SW_IO_P1THRUST
             | SW_IO_P2LEFT | SW_IO_P2RIGHT | SW_IO_P2THRUST
             | SW_IO_Zero | SW_IO_One | SW_IO_Two | SW_IO_Three | SW_IO_Four | SW_IO_Five | SW_IO_Six | SW_IO_Seven | SW_IO_Eight | SW_IO_Nine;

   ioSwitches |= SW_SW_COIN  | SW_SW_P1FIRE | SW_SW_P1HYPER | SW_SW_P2FIRE | SW_SW_P2HYPER;

   if ((currentButtonState & ~prevButtonState) & (VEC_BUTTON_1_1|VEC_BUTTON_1_1)) ioSwitches &= ~SW_SW_COIN;	// only on rising edge

   // digital joysticks
   if (currentJoy1X < -30) ioInputs &= ~SW_IO_P1LEFT;
   if (currentJoy1X > 30) ioInputs &= ~SW_IO_P1RIGHT;
   if (currentJoy1Y < -30) ioSwitches &= ~SW_SW_P1HYPER;
   if (currentJoy1Y > 30) ioInputs &= ~SW_IO_P1THRUST;

   if (currentJoy2X < -30) ioInputs &= ~SW_IO_P2LEFT;
   if (currentJoy2X > 30) ioInputs &= ~SW_IO_P2RIGHT;
   if (currentJoy2Y < -30) ioSwitches &= ~SW_SW_P2HYPER;
   if (currentJoy2Y > 30) ioInputs &= ~SW_IO_P2THRUST;

   if (currentButtonState & VEC_BUTTON_1_1) ioInputs &= ~SW_IO_P1LEFT;
   if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~SW_IO_P1RIGHT;
   if (currentButtonState & VEC_BUTTON_1_3) ioInputs &= ~SW_IO_P1THRUST;
   if (currentButtonState & VEC_BUTTON_1_4) ioSwitches &= ~SW_SW_P1FIRE;

   if (currentButtonState & VEC_BUTTON_2_1) ioInputs &= ~SW_IO_P2LEFT;
   if (currentButtonState & VEC_BUTTON_2_2) ioInputs &= ~SW_IO_P2RIGHT;
   if (currentButtonState & VEC_BUTTON_2_3) ioInputs &= ~SW_IO_P2THRUST;
   if (currentButtonState & VEC_BUTTON_2_4) ioSwitches &= ~SW_SW_P2FIRE;

#ifdef NEVER
  /*
# Initialization file for Space Wars
>
>*** Space Wars ***
>
>Keyboard Mapping:
>
>   Coin       = F3
>   Reset Game = F4
>   Exit       = <Esc>
>
>   Left Player       Right Player
>   ------------      ----------------------------------
>   Left   = 'Q'       Left   = Keypad '7'
>   Right  = 'A'       Right  = Keypad '5'
>   Walk   = 'X'       Walk   = Keypad '9'
>   Fire   = 'C'       Fire   = Keypad '-' or <Sysreq>
>   HyperS = <Alt>     HyperS = Keypad '+' or <Enter>
>   HyperS = <Ctrl>
>
>   Option keys     = '0'-'9'
>   Space War RESET = <Backspace>

 ; Switch definitions:
 ;
 ;   XXXXX--  Unused (Must be 0)
 ;
 ;   -----TT  00 = 0:45 minutes per coin
 ;            11 = 1:00 minutes per coin
 ;            10 = 1:30 minutes per coin
 ;            01 = 2:00 minutes per coin

 Switches=0000011

[Inputs]
 RstCPU   = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating reset
 Exit     = 01000000,FFFFFFFF,00000000,FFFFFFFF ; Set bit indicating Exit 
 Coin     = 00000000,FFFFFFFF,00000000,FF7FFFFF

 ; Options

 Zero    = 00000000,FFFFF7FF,00000800,FFFFFFFF
 One     = 00000000,FFFFFFEF,00000010,FFFFFFFF
 Two     = 00000000,FFFFFFBF,00000040,FFFFFFFF
 Three   = 00000000,FFFFFFFE,00000001,FFFFFFFF
 Four    = 00000000,FFFFFFFB,00000004,FFFFFFFF
 Five    = 00000000,FFFFFBFF,00000400,FFFFFFFF
 Six     = 00000000,FFFFFFDF,00000020,FFFFFFFF
 Seven   = 00000000,FFFFFF7F,00000080,FFFFFFFF
 Eight   = 00000000,FFFFFFFD,00000002,FFFFFFFF
 Nine    = 00000000,FFFFFFF7,00000008,FFFFFFFF

 Reset   = 00000000,FFBFFFFF,00400000,FFFFFFFF

 ; Left player

 LLeft   = 00000000,FFFFFEFF,00000100,FFFFFFFF
 LRight  = 00000000,FFFFDFFF,00002000,FFFFFFFF
 LThrust = 00000000,FFFF7FFF,00008000,FFFFFFFF
 LFire   = 00000000,FFFBFFFF,00040000,FFFFFFFF
 LHyper  = 00000000,FFFDFFFF,00020000,FFFFFFFF

 ; Right player

 RLeft   = 00000000,FFFFBFFF,00004000,FFFFFFFF
 RRight  = 00000000,FFFFEFFF,00001000,FFFFFFFF
 RThrust = 00000000,FFFFFDFF,00000200,FFFFFFFF
 RFire   = 00000000,FFFEFFFF,00010000,FFFFFFFF
 RHyper  = 00000000,FFF7FFFF,00080000,FFFFFFFF
   */
#endif
   if (frameCounter < 500) {
     if (v_rotate) {
       line(v_yb,v_xl, v_yt,v_xl, 6);
       line(v_yt,v_xl, v_yt,v_xr, 6);
       line(v_yt,v_xr, v_yb,v_xr, 6);
       line(v_yb,v_xr, v_yb,v_xl, 6);
     } else {
       line(v_xl,v_yb, v_xl,v_yt, 6);
       line(v_xl,v_yt, v_xr,v_yt, 6);
       line(v_xr,v_yt, v_xr,v_yb, 6);
       line(v_xr,v_yb, v_xl,v_yb, 6);
     }
   }
}

void startFrame_boxingbugs(void) {
  startFrame();
#ifdef NEVER
  /*
# Initialization file for Boxing Bugs
>
>*** Boxing Bugs ***
>
>Keyboard Mapping:
>
>   One Player  = F1
>   Two Players = F2
>   Coin        = F3
>   Reset Game  = F4
>   Exit        = <Esc>
>
>   Cannon      = First (Left) Mouse Button
>   Glove       = Second Mouse Button
>   Panic       = Third Mouse Button
>   Panic       = <Spacebar>  (For the "third button challenged" mice)
>
>   Use mouse to aim Cannon/Glove.

; Switch definitions:
 ;
 ;   D------  0=Normal, 1=Diagnositic Mode
 ;   -F-----  0=Normal, 1=Free Play
 ;   --S----  0=No sound during attract, 1=Sound during attract (sound not supported)
 ;   ---B---  0=Bonus at 50k, 1=Bonus at 30k
 ;   ----P--  0=3 cannons per game, 1=5 cannons per game
 ;
 ;   -----CC  00 = 1 credit per 1 quarter
 ;            10 = 1 credit per 2 quarters
 ;            01 = 3 credits per 2 quarters
 ;            11 = 3 credits per 4 quarters

 Switches=0001100

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit 
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF

 ; Define Boxing Bug control panel

 LCannon = 00000000,FFFFFFFE,00000001,FFFFFFFF ; Left Cannon
 LGlove  = 00000000,FFFFFFFD,00000002,FFFFFFFF ; Left Glove
 LPanic  = 00000000,FFFFFFFB,00000004,FFFFFFFF ; Left Panic / Two Player
 RPanic  = 00000000,FFFFFFF7,00000008,FFFFFFFF ; Right Panic / One Player
 RGlove  = 00000000,FFFFFFEF,00000010,FFFFFFFF ; Right Glove
 RCannon = 00000000,FFFFFFDF,00000020,FFFFFFFF ; Right Cannon
 AcctOn  = 00000000,FFFFFFBF,00000000,FFFFFFFF ; Accounting Info On
 AcctOff = 00000040,FFFFFFFF,00000000,FFFFFFFF ; Accounting Info Off
   */
#endif
}

static int armor_cx = 0, armor_cy = 0;
void armor_moveto(int x, int y, int q) {
  int dx=2048, dy=2048;
  x -= dx; y -= dy;
  if (q&1) x = -x;
  if (q&2) y = -y;
  x+=dx; y+=dy;
  armor_cx = x; armor_cy = y;
}

void armor_lineto(int x, int y, int q) {
  // coordinates from pseudo-overlay code extracted from Mame
  // needed to be rescaled and a small Y offset applied to match
  // the internal coordinates used by the game.
  int dx=2048, dy=2048;
  x -= dx; y -= dy;
  if (q&1) x = -x;
  if (q&2) y = -y;
  x+=dx; y+=dy;
  
  line (armor_cx/4, armor_cy*2/11 + 10, x/4, y*2/11 + 10, 6);
  armor_cx = x; armor_cy = y;
}

void startFrame_armorattack(void) {

#define AA_IO_P1START   0x02  // 1-player start
#define AA_IO_P1LEFT    0x1000
#define AA_IO_P1RIGHT   0x4000
#define AA_IO_P1THRUST  0x8000
#define AA_IO_P1FIRE    0x2000

#define AA_IO_P2START   0x08  // 2-player start
#define AA_IO_P2LEFT    0x01
#define AA_IO_P2RIGHT   0x04
#define AA_IO_P2THRUST  0x10
#define AA_IO_P2FIRE    0x20
  
#define AA_SW_ABORT   SW_ABORT	/* for ioSwitches */
#define AA_SW_COIN    0x080
  
   static int prevButtonState;	// for debouncing

   frameCounter += 1;
   DEBUG_OUT("// Frame %d\n", frameCounter);

   v_WaitRecal ();
   // v_doSound();
   prevButtonState = currentButtonState;
   v_readButtons ();		// update currentButtonState
   v_readJoystick1Analog ();
   //v_readJoystick2Analog ();  // Apparently we are getting joystick 2 data anyway?????
   // v_playAllSFX();

   // Unfortunately, it's quite common to press LEFT and RIGHT simultaneously by accident with this
   // layout, and accidentally invoke the configuration screen.  Need to think about how we will
   // handle this...
   
   // default inactive:
   ioInputs |= AA_IO_P1LEFT | AA_IO_P1RIGHT | AA_IO_P1THRUST | AA_IO_P1FIRE | AA_IO_P1START | 
               AA_IO_P2LEFT | AA_IO_P2RIGHT | AA_IO_P2THRUST | AA_IO_P2FIRE | AA_IO_P2START;
   ioSwitches |= AA_SW_COIN;

   // digital joysticks...
   if (currentJoy1X < -30) ioInputs &= ~AA_IO_P1LEFT;
   if (currentJoy1X > 30) ioInputs &= ~AA_IO_P1RIGHT;
   if (currentJoy1Y > 30) ioInputs &= ~AA_IO_P1THRUST;

   if (currentJoy2X < -30) ioInputs &= ~AA_IO_P2LEFT;
   if (currentJoy2X > 30) ioInputs &= ~AA_IO_P2RIGHT;
   if (currentJoy2Y > 30) ioInputs &= ~AA_IO_P2THRUST;

   // would be helpful to examine ram and make these context-dependent

   if ((currentButtonState & ~prevButtonState) & (VEC_BUTTON_1_1 | VEC_BUTTON_2_1)) ioSwitches &= ~AA_SW_COIN;	// only on rising edge
   if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~AA_IO_P1START; // needs 1 coin
   if (currentButtonState & VEC_BUTTON_2_2) ioInputs &= ~AA_IO_P2START; // needs 2 coins and second controller

   if (currentButtonState & VEC_BUTTON_1_1) ioInputs &= ~AA_IO_P1LEFT;
   if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~AA_IO_P1RIGHT;
   if (currentButtonState & VEC_BUTTON_1_3) ioInputs &= ~AA_IO_P1THRUST;
   if (currentButtonState & VEC_BUTTON_1_4) ioInputs &= ~AA_IO_P1FIRE;

   if (currentButtonState & VEC_BUTTON_2_1) ioInputs &= ~AA_IO_P2LEFT;
   if (currentButtonState & VEC_BUTTON_2_2) ioInputs &= ~AA_IO_P2RIGHT;
   if (currentButtonState & VEC_BUTTON_2_3) ioInputs &= ~AA_IO_P2THRUST;
   if (currentButtonState & VEC_BUTTON_2_4) ioInputs &= ~AA_IO_P2FIRE;

#ifdef NEVER
  /*
>
>*** Armor Attack ***
>
>Keyboard Mapping:
>
>   Coin        = B1
>   One Player  = B2
>   Two Players = B3
>
>   Exit        = F1,F2,F3,F4
>   Calibrate   = F1,F2
>
>   Left        = F1
>   Right       = F2
>   Thrust      = F3
>   Fire        = F4
>

      
 ; Switch definitions:
 ;
 ;   D------  0=Diagnostics, 1=Normal
 ;   -X-----  Unused
 ;   --S----  0=No Sound in attract mode, 1=Sound (sound not supported)
 ;
 ;   ---CC--  00 = 1 credit per 1 quarter
 ;            10 = 1 credit per 2 quarters
 ;            01 = 3 credits per 2 quarters
 ;            11 = 3 credits per 4 quarters
 ;
 ;   -----JJ  00 = 5 jeeps per game
 ;   -----JJ  10 = 4 jeeps per game
 ;   -----JJ  01 = 3 jeeps per game
 ;   -----JJ  11 = 2 jeeps per game

 Switches=1000000      ; diagnostics, no sound, 1 credit per quarter, 5 jeeps per game ???

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF

 Player1 = 00000000,FFFFFFFD,00000002,FFFFFFFF
 Player2 = 00000000,FFFFFFF7,00000008,FFFFFFFF

 RFire   = 00000000,FFFFDFFF,00002000,FFFFFFFF
 RThrust = 00000000,FFFF7FFF,00008000,FFFFFFFF
 RRight  = 00000000,FFFFBFFF,00004000,FFFFFFFF
 RLeft   = 00000000,FFFFEFFF,00001000,FFFFFFFF

 LFire   = 00000000,FFFFFFDF,00000020,FFFFFFFF
 LThrust = 00000000,FFFFFFEF,00000010,FFFFFFFF
 LRight  = 00000000,FFFFFFFB,00000004,FFFFFFFF
 LLeft   = 00000000,FFFFFFFE,00000001,FFFFFFFF
   */
#endif
   {int q;
     for (q = 0; q < 4; q++) {
  // Upper Right Quadrant
  // Outer structure
  armor_moveto(3446, 2048, q);
  armor_lineto(3446, 2224, q); //
  armor_lineto(3958, 2224, q);
  armor_lineto(3958, 3059, q);
  armor_lineto(3323, 3059, q);
  armor_lineto(3323, 3225, q);
  armor_lineto(3194, 3225, q);
  armor_lineto(3194, 3393, q);
  armor_lineto(3067, 3393, q);
  armor_lineto(3067, 3901, q);
  armor_lineto(2304, 3901, q);
  armor_lineto(2304, 3225, q);
  armor_lineto(2048, 3225, q);
  // Center structure
  armor_moveto(2048, 2373, q);
  armor_lineto(2562, 2373, q); //
  armor_lineto(2562, 2738, q);
  armor_lineto(2430, 2738, q);
  armor_lineto(2430, 2893, q);
  armor_lineto(2306, 2893, q);
  armor_lineto(2306, 3065, q);
  armor_lineto(2048, 3065, q);
  // Big structure
  armor_moveto(2938, 2209, q);
  armor_lineto(3198, 2209, q); //
  armor_lineto(3198, 2383, q);
  armor_lineto(3706, 2383, q);
  armor_lineto(3706, 2738, q);
  armor_lineto(2938, 2738, q);
  armor_lineto(2938, 2209, q);
  // Small structure
  armor_moveto(2551, 3055, q);
  armor_lineto(2816, 3055, q); //
  armor_lineto(2816, 3590, q);
  armor_lineto(2422, 3590, q);
  armor_lineto(2422, 3231, q);
  armor_lineto(2555, 3231, q);
  armor_lineto(2555, 3055, q);
     }
   }
   if (frameCounter < 500) {
     if (v_rotate) {
       line(v_yb,v_xl, v_yt,v_xl, 6);
       line(v_yt,v_xl, v_yt,v_xr, 6);
       line(v_yt,v_xr, v_yb,v_xr, 6);
      line(v_yb,v_xr, v_yb,v_xl, 6);
     } else {
       line(v_xl,v_yb, v_xl,v_yt, 6);
       line(v_xl,v_yt, v_xr,v_yt, 6);
       line(v_xr,v_yt, v_xr,v_yb, 6);
       line(v_xr,v_yb, v_xl,v_yb, 6);
     }
   }
}

void startFrame_starcastle(void) {
  startFrame();
#ifdef NEVER
  /*
# Initialization file for Star Castle
>
>*** Star Castle ***
>
>Keyboard Mapping:
>
>   One Player  = F1
>   Two Players = F2
>   Coin        = F3
>   Reset Game  = F4
>   Exit        = <Esc>
>
>   Left        = 'Z'
>   Right       = 'X'
>   Thrust      = '.'
>   Fire        = '/'

 ; Switch definitions:
 ;
 ;   D------  0=Test Pattern, 1=Normal
 ;   -XX----  Unused
 ;
 ;   ---CC--  00 = 1 credit per 1 quarter
 ;            10 = 1 credit per 2 quarters
 ;            01 = 3 credit per 2 quarters
 ;            11 = 3 credit per 4 quarters
 ;
 ;   -----SS  00 = 3 ships per game
 ;            10 = 4 ships per game
 ;            01 = 5 ships per game
 ;            11 = 6 ships per game

 Switches=1000011

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit 
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF

 Player1 = 00000000,FFFFFFFE,00000001,FFFFFFFF
 Player2 = 00000000,FFFFFFFB,00000004,FFFFFFFF

 Left    = 00000000,FFFFFFBF,00000040,FFFFFFFF
 Right   = 00000000,FFFFFEFF,00000100,FFFFFFFF
 Thrust  = 00000000,FFFFFBFF,00000400,FFFFFFFF
 Fire    = 00000000,FFFFEFFF,00001000,FFFFFFFF
   */
#endif
}

void startFrame_starhawk(void) {
  static int prevButtonState;	// for debouncing

  frameCounter += 1;
  DEBUG_OUT("// Frame %d\n", frameCounter);

  v_WaitRecal ();
  // v_doSound();
  prevButtonState = currentButtonState;
  v_readButtons ();		// update currentButtonState
  v_readJoystick1Analog ();
  //v_readJoystick2Analog ();  // Apparently we are getting joystick 2 data anyway?????
  // v_playAllSFX();

#define SH_SW_P1_START 0x01
#define SH_IO_P1_LEFT  0x0002
#define SH_IO_P1_RIGHT 0x0004
#define SH_IO_P1_UP    0x0001
#define SH_IO_P1_DOWN  0x0008
#define SH_SW_P1_FIRE  0x08
#define SH_IO_P1_SLOW  0x4000
#define SH_IO_P1_MED   0x1000
#define SH_IO_P1_FAST  0x0200

#define SH_SW_P2_START 0x04
#define SH_IO_P2_LEFT  0x0400
#define SH_IO_P2_RIGHT 0x0010
#define SH_IO_P2_UP    0x0800
#define SH_IO_P2_DOWN  0x0020
#define SH_SW_P2_FIRE  0x02
#define SH_IO_P2_SLOW  0x0100
#define SH_IO_P2_MED   0x2000
#define SH_IO_P2_FAST  0x8000

#define SH_SW_COIN 0x80

   ioSwitches |= SH_SW_COIN | SH_SW_P1_START | SH_SW_P2_START | SH_SW_P1_FIRE | SH_SW_P2_FIRE;
   if ((currentButtonState & ~prevButtonState) & (VEC_BUTTON_1_1|VEC_BUTTON_2_1)) ioSwitches &= ~SH_SW_COIN;	// only on rising edge

   ioInputs |= SH_IO_P1_LEFT | SH_IO_P1_RIGHT | SH_IO_P1_UP | SH_IO_P1_DOWN | SH_IO_P1_SLOW | SH_IO_P1_MED | SH_IO_P1_FAST |
               SH_IO_P2_LEFT | SH_IO_P2_RIGHT | SH_IO_P2_UP | SH_IO_P2_DOWN | SH_IO_P2_SLOW | SH_IO_P2_MED | SH_IO_P2_FAST;

   if (currentButtonState & VEC_BUTTON_1_2) ioSwitches &= ~SH_SW_P1_START;
   if (currentButtonState & VEC_BUTTON_2_2) ioSwitches &= ~SH_SW_P2_START;

   // Digital joysticks.  Need to do the same sort of hack as
   // in Tailgunner to allow analog injection of cursor values
   // btw the P2 joystick for some reason seems much more responsive!
   
   if (currentJoy1X < -30) ioInputs &= ~SH_IO_P1_LEFT;
   if (currentJoy1X > 30) ioInputs &= ~SH_IO_P1_RIGHT;
   if (currentJoy1Y > 30) ioInputs &= ~SH_IO_P1_UP;
   if (currentJoy1Y < -30) ioInputs &= ~SH_IO_P1_DOWN;

   if (currentJoy2X < -30) ioInputs &= ~SH_IO_P2_LEFT;
   if (currentJoy2X > 30) ioInputs &= ~SH_IO_P2_RIGHT;
   if (currentJoy2Y > 30) ioInputs &= ~SH_IO_P2_UP;
   if (currentJoy2Y < -30) ioInputs &= ~SH_IO_P2_DOWN;

   // again, some context-sensitive decoding will be needed
   
   if (currentButtonState & VEC_BUTTON_1_1) ioInputs &= ~SH_IO_P1_SLOW;
   if (currentButtonState & VEC_BUTTON_2_1) ioInputs &= ~SH_IO_P2_SLOW;

   if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~SH_IO_P1_MED;
   if (currentButtonState & VEC_BUTTON_2_2) ioInputs &= ~SH_IO_P2_MED;

   if (currentButtonState & VEC_BUTTON_1_3) ioInputs &= ~SH_IO_P1_FAST;
   if (currentButtonState & VEC_BUTTON_2_3) ioInputs &= ~SH_IO_P2_FAST;

   if (currentButtonState & VEC_BUTTON_1_4) ioSwitches &= ~SH_SW_P1_FIRE;
   if (currentButtonState & VEC_BUTTON_2_4) ioSwitches &= ~SH_SW_P2_FIRE;

#ifdef NEVER
  /*
# Initialization file for Star Hawk
>
>*** Star Hawk ***
>
>Keyboard Mapping:
>
>   One Player  = F1
>   Two Players = F2
>   Coin        = F3
>   Reset Game  = F4
>   Exit        = <Esc>
>
>   Left Player            Right Player
>   ------------           ----------------------
>   Slow  = '1'            Slow  = '0'
>   Med   = '2'            Med   = '-'
>   Fast  = '3'            Fast  = '='
>   Left  = 'F'            Left  = Keypad '4'
>   Right = 'H'            Right = Keypad '6'
>   Up    = 'T'            Up    = Keypad '8'
>   Down  = 'G' or 'V'     Down  = Keypad '5' or Keypad '2'
>   Fire  = <Left Shift>   Fire  = <Right Shift>

 Switches=0000000

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit 
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF

 Player1 = 00000000,FFFEFFFF,00010000,FFFFFFFF
 Player2 = 00000000,FFFBFFFF,00040000,FFFFFFFF

 RSlow   = 00000000,FFFFBFFF,00004000,FFFFFFFF
 RMed    = 00000000,FFFFEFFF,00001000,FFFFFFFF
 RFast   = 00000000,FFFFFDFF,00000200,FFFFFFFF
 RLeft   = 00000000,FFFFFFFD,00000002,FFFFFFFF
 RRight  = 00000000,FFFFFFFB,00000004,FFFFFFFF
 RUp     = 00000000,FFFFFFFE,00000001,FFFFFFFF
 RDown   = 00000000,FFFFFFF7,00000008,FFFFFFFF
 RFire   = 00000000,FFF7FFFF,00080000,FFFFFFFF

 LSlow   = 00000000,FFFFFEFF,00000100,FFFFFFFF
 LMed    = 00000000,FFFFDFFF,00002000,FFFFFFFF
 LFast   = 00000000,FFFF7FFF,00008000,FFFFFFFF
 LLeft   = 00000000,FFFFFBFF,00000400,FFFFFFFF
 LRight  = 00000000,FFFFFFEF,00000010,FFFFFFFF
 LUp     = 00000000,FFFFF7FF,00000800,FFFFFFFF
 LDown   = 00000000,FFFFFFDF,00000020,FFFFFFFF
 LFire   = 00000000,FFFDFFFF,00020000,FFFFFFFF
   */
#endif
   if (frameCounter < 500) {
     if (v_rotate) {
       line(v_yb,v_xl, v_yt,v_xl, 6);
       line(v_yt,v_xl, v_yt,v_xr, 6);
       line(v_yt,v_xr, v_yb,v_xr, 6);
       line(v_yb,v_xr, v_yb,v_xl, 6);
     } else {
       line(v_xl,v_yb, v_xl,v_yt, 6);
       line(v_xl,v_yt, v_xr,v_yt, 6);
       line(v_xr,v_yt, v_xr,v_yb, 6);
       line(v_xr,v_yb, v_xl,v_yb, 6);
     }
   }
}

void startFrame_solarquest(void) {
  startFrame();
#ifdef NEVER
  /*
# Initialization file for Solar Quest
>
>*** Solar Quest ***
>
>Keyboard Mapping:
>
>   One Player  = F1
>   Two Players = F2
>   Coin        = F3
>   Reset Game  = F4
>   Exit        = <Esc>
>
>   Left        = 'Z'
>   Right       = 'X'
>   HyperSpace  = 'C'
>
>   Nova        = ','
>   Thrust      = '.'
>   Fire        = '/'

 ; Switch definitions:
 ;
 ;    D------  0=Diagnostics, 1=Normal
 ;    -F-----  0=Normal, 1=Free play
 ;
 ;    --SS---  00 = 2 ships
 ;             10 = 3 ships
 ;             01 = 4 ships
 ;             11 = 5 ships
 ;
 ;    ----C-C  0-0 = 1 coin 1 credit
 ;             1-0 = 2 coin 1 credit
 ;             0-1 = 2 coin 3 credit
 ;             1-1 = 4 coin 3 credit
 ;
 ;    -----E-  0=25 captures for extra ship, 1=40 captures

 Switches=1011000

 ; Default inputs (used to set difficulty level):
 ;
 ;    FFFF = Level 1 (Easiest)
 ;    EFFF = Level 2
 ;    DFFF = Level 3
 ;    CFFF = Level 4
 ;    BFFF = Level 5
 ;    AFFF = Level 6
 ;    9FFF = Level 7
 ;    8FFF = Level 8 (Hardest)
 ;
 ; Any other settings may cause erratic behaviour!

 Inputs=FFFF

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit 
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF

 Player1 = 00000000,FFFFFFF7,00000008,FFFFFFFF	; Also Hyperspace key
 Player2 = 00000000,FFFFFFFE,00000001,FFFFFFFF	; Also Nova key
 Left    = 00000000,FFFFFFDF,00000020,FFFFFFFF
 Right   = 00000000,FFFFFFEF,00000010,FFFFFFFF
 Thrust  = 00000000,FFFFFFFB,00000004,FFFFFFFF
 Fire    = 00000000,FFFFFFFD,00000002,FFFFFFFF
   */
#endif
}

void startFrame_cosmicchasm(void) {
  startFrame();
#ifdef NEVER
  /*
# Initialization file for Cosmic Chasm - best guess, under development
>
>***Cosmic Chasm ***
>
>Keyboard Mapping:
>
>   One Player  = F1
>   Two Players = F2
>   Coin        = F3
>   Reset Game  = F4
>   Exit        = <Esc>
>

; Switch definitions:
 ;
 ;   D------  0=Normal, 1=Diagnositic Mode
 ;   -F-----  0=Normal, 1=Free Play
 ;   --S----  0=No sound during attract, 1=Sound during attract (sound not supported)
 ;   ---B---  0=Bonus at 50k, 1=Bonus at 30k
 ;   ----P--  0=3 cannons per game, 1=5 cannons per game
 ;
 ;   -----CC  00 = 1 credit per 1 quarter
 ;            10 = 1 credit per 2 quarters
 ;            01 = 3 credits per 2 quarters
 ;            11 = 3 credits per 4 quarters

 Switches=0001100

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit 
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF
   */
#endif
}

void startFrame_waroftheworlds(void) {
  startFrame();
#ifdef NEVER
  /*
# Initialization file for War of the Worlds
>
>*** War of the Worlds ***
>
>Keyboard Mapping:
>
>   One Player  = F1
>   Two Players = F2
>   Coin        = F3
>   Reset Game  = F4
>   Exit        = <Esc>
>
>   Left        = 'Z'
>   Right       = 'X'
>   Shields     = '.'
>   Fire        = '/'

 ; Switch definitions:
 ;
 ;   D------  0=Normal, 1=Diagnostics
 ;   -F-----  0=Normal, 1=Free Play
 ;   ---C---  0=1 credit per 1 quarter, 1=3 credits per 2 quarters
 ;   -----S-  0=5 ships per game, 1=3 ships per game
 ;
 ;   --?-?-?  *unknown* (no manual exists)

 Switches=0000000

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit 
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF

 Player1 = 00000000,FFFFFFFE,00000001,FFFFFFFF
 Player2 = 00000000,FFFFFFFB,00000004,FFFFFFFF

 Left    = 00000000,FFFFFFBF,00000040,FFFFFFFF
 Right   = 00000000,FFFFFEFF,00000100,FFFFFFFF
 Shields = 00000000,FFFFFBFF,00000400,FFFFFFFF
 Fire    = 00000000,FFFFEFFF,00001000,FFFFFFFF

   */
#endif
}

static int warrior_cx = 0, warrior_cy = 0;
void warrior_moveto(int x, int y) {
  warrior_cx = x; warrior_cy = y;
}

void warrior_lineto(int x, int y) {
  line (warrior_cx/4, warrior_cy*2/11 + 10, x/4, y*2/11 + 10, 6);
  warrior_cx = x; warrior_cy = y;
}


void startFrame_warrior(void) {
  static int prevButtonState;	// for debouncing

  frameCounter += 1;
  DEBUG_OUT("// Frame %d\n", frameCounter);

  v_WaitRecal ();
  // v_doSound();
  prevButtonState = currentButtonState;
  v_readButtons ();		// update currentButtonState
  v_readJoystick1Analog ();
  //v_readJoystick2Analog ();  // Apparently we are getting joystick 2 data anyway?????
  // v_playAllSFX();

  // Don't have any input for this game as to Player 1 or Player 2 start....

  //#define WA_IO_P1_START 0x??
#define WA_IO_P1_LEFT  0x02
#define WA_IO_P1_RIGHT 0x01
#define WA_IO_P1_UP    0x04
#define WA_IO_P1_DOWN  0x08
#define WA_IO_P1_SWORD 0x10

  //#define WA_IO_P2_START 0x????
#define WA_IO_P2_LEFT  0x0200
#define WA_IO_P2_RIGHT 0x0100
#define WA_IO_P2_UP    0x0400
#define WA_IO_P2_DOWN  0x0800
#define WA_IO_P2_SWORD 0x1000

#define WA_SW_COIN 0x80

   ioSwitches |= WA_SW_COIN;
   if ((currentButtonState & ~prevButtonState) & (VEC_BUTTON_1_1|VEC_BUTTON_2_1)) ioSwitches &= ~WA_SW_COIN;	// only on rising edge

   ioInputs |= /*WA_IO_P1_START |*/ WA_IO_P1_LEFT | WA_IO_P1_RIGHT | WA_IO_P1_UP | WA_IO_P1_DOWN | WA_IO_P1_SWORD |
               /*WA_IO_P2_START |*/ WA_IO_P2_LEFT | WA_IO_P2_RIGHT | WA_IO_P2_UP | WA_IO_P2_DOWN | WA_IO_P2_SWORD;

   // How to start?
   //if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~WA_IO_P1_START;
   //if (currentButtonState & VEC_BUTTON_2_2) ioInputs &= ~WA_IO_P2_START;

   // USE JOYSTICK
   if (currentJoy1X < -30) ioInputs &= ~WA_IO_P1_LEFT;
   if (currentJoy1X > 30) ioInputs &= ~WA_IO_P1_RIGHT;
   if (currentJoy1Y > 30) ioInputs &= ~WA_IO_P1_UP;
   if (currentJoy1Y < -30) ioInputs &= ~WA_IO_P1_DOWN;

   if (currentJoy2X < -30) ioInputs &= ~WA_IO_P2_LEFT;
   if (currentJoy2X > 30) ioInputs &= ~WA_IO_P2_RIGHT;
   if (currentJoy2Y > 30) ioInputs &= ~WA_IO_P2_UP;
   if (currentJoy2Y < -30) ioInputs &= ~WA_IO_P2_DOWN;

   if (currentButtonState & VEC_BUTTON_1_4) ioInputs &= ~WA_IO_P1_SWORD;
   if (currentButtonState & VEC_BUTTON_2_4) ioInputs &= ~WA_IO_P2_SWORD;

#ifdef NEVER
  /*
# Initialization file for Warrior
>
>*** Warrior ***
>
>Keyboard Mapping:
>
>   One Player  = F1
>   Two Players = F2
>   Coin        = F3
>   Reset Game  = F4
>   Exit        = <Esc>
>
>   Left Player            Right Player
>   ------------           ----------------------
>   Left  = 'F'            Left  = Keypad '4'
>   Right = 'H'            Right = Keypad '6'
>   Up    = 'T'            Up    = Keypad '8'
>   Down  = 'G'            Down  = Keypad '5'
>   Sword = <Left Shift>   Sword = <Right Shift>

 Switches=1111111

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF

 LRight  = 00000000,FFFFFEFF,00000100,FFFFFFFF
 LLeft   = 00000000,FFFFFDFF,00000200,FFFFFFFF
 LUp     = 00000000,FFFFFBFF,00000400,FFFFFFFF
 LDown   = 00000000,FFFFF7FF,00000800,FFFFFFFF
 LSword  = 00000000,FFFFEFFF,00001000,FFFFFFFF

 RRight  = 00000000,FFFFFFFE,00000001,FFFFFFFF
 RLeft   = 00000000,FFFFFFFD,00000002,FFFFFFFF
 RUp     = 00000000,FFFFFFFB,00000004,FFFFFFFF
 RDown   = 00000000,FFFFFFF7,00000008,FFFFFFFF
 RSword  = 00000000,FFFFFFEF,00000010,FFFFFFFF

 Sw0  = 00000000,FFFEFFFF,00010000,FFFFFFFF
 Sw1  = 00000000,FFFDFFFF,00020000,FFFFFFFF
 Sw2  = 00000000,FFFBFFFF,00040000,FFFFFFFF
 Sw3  = 00000000,FFF7FFFF,00080000,FFFFFFFF
 Sw6  = 00000000,FFBFFFFF,00400000,FFFFFFFF

 Key5 = 00000000,FFFFFFDF,00000020,FFFFFFFF
 Key6 = 00000000,FFFFFFBF,00000040,FFFFFFFF
 Key7 = 00000000,FFFFFF7F,00000080,FFFFFFFF
 KeyD = 00000000,FFFFDFFF,00002000,FFFFFFFF
 KeyE = 00000000,FFFFBFFF,00004000,FFFFFFFF
 KeyF = 00000000,FFFF7FFF,00008000,FFFFFFFF
   */
#endif
  warrior_moveto(1187, 2232);
  warrior_lineto(1863, 2232);
  warrior_moveto(1187, 1372);
  warrior_lineto(1863, 1372);
  warrior_moveto(1187, 2232);
  warrior_lineto(1187, 1372);
  warrior_moveto(1863, 2232);
  warrior_lineto(1863, 1372);
  warrior_moveto(2273, 2498);
  warrior_lineto(2949, 2498);
  warrior_moveto(2273, 1658);
  warrior_lineto(2949, 1658);
  warrior_moveto(2273, 2498);
  warrior_lineto(2273, 1658);
  warrior_moveto(2949, 2498);
  warrior_lineto(2949, 1658);
   if (frameCounter < 500) {
     if (v_rotate) {
       line(v_yb,v_xl, v_yt,v_xl, 6);
       line(v_yt,v_xl, v_yt,v_xr, 6);
       line(v_yt,v_xr, v_yb,v_xr, 6);
       line(v_yb,v_xr, v_yb,v_xl, 6);
     } else {
       line(v_xl,v_yb, v_xl,v_yt, 6);
       line(v_xl,v_yt, v_xr,v_yt, 6);
       line(v_xr,v_yt, v_xr,v_yb, 6);
       line(v_xr,v_yb, v_xl,v_yb, 6);
     }
   }
}

void startFrame_barrier(void) {
  static int prevButtonState;	// for debouncing

   frameCounter += 1;
   DEBUG_OUT("// Frame %d\n", frameCounter);

   v_WaitRecal ();
   // v_doSound();
   prevButtonState = currentButtonState;
   v_readButtons ();		// update currentButtonState
   v_readJoystick1Analog ();
   //v_readJoystick2Analog ();  // Apparently we are getting joystick 2 data anyway?????
   // v_playAllSFX();

#define BA_IO_P1_START 0x0800
#define BA_IO_P1_LEFT  0x4000
#define BA_IO_P1_RIGHT 0x0200
#define BA_IO_P1_FWD   0x1000
#define BA_IO_P1_REV   0x0008

#define BA_IO_P2_START 0x0010
#define BA_IO_P2_LEFT  0x0100
#define BA_IO_P2_RIGHT 0x8000
#define BA_IO_P2_FWD   0x2000
#define BA_IO_P2_REV   0x0400

#define BA_IO_SKILL_A 0X01
#define BA_IO_SKILL_B 0X04
#define BA_IO_SKILL_C 0X40

#define BA_SW_COIN 0x80

   ioSwitches |= BA_SW_COIN;
   if ((currentButtonState & ~prevButtonState) & (VEC_BUTTON_1_1|VEC_BUTTON_2_1)) ioSwitches &= ~BA_SW_COIN;	// only on rising edge

   ioInputs |= BA_IO_P1_START | BA_IO_P1_LEFT | BA_IO_P1_RIGHT | BA_IO_P1_FWD | BA_IO_P1_REV |
               BA_IO_P2_START | BA_IO_P2_LEFT | BA_IO_P2_RIGHT | BA_IO_P2_FWD | BA_IO_P2_REV |
               BA_IO_SKILL_A | BA_IO_SKILL_B | BA_IO_SKILL_C;
// USE JOYSTICK
   if (currentJoy1X < -30) ioInputs &= ~BA_IO_P1_LEFT;
   if (currentJoy1X > 30) ioInputs &= ~BA_IO_P1_RIGHT;
   if (currentJoy1Y > 30) ioInputs &= ~BA_IO_P1_FWD;
   if (currentJoy1Y < -30) ioInputs &= ~BA_IO_P1_REV;

   if (currentJoy2X < -30) ioInputs &= ~BA_IO_P2_LEFT;
   if (currentJoy2X > 30) ioInputs &= ~BA_IO_P2_RIGHT;
   if (currentJoy2Y > 30) ioInputs &= ~BA_IO_P2_FWD;
   if (currentJoy2Y < -30) ioInputs &= ~BA_IO_P2_REV;

   if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~BA_IO_P1_START;
   if (currentButtonState & VEC_BUTTON_2_2) ioInputs &= ~BA_IO_P2_START;

   if (currentButtonState & (VEC_BUTTON_1_3|VEC_BUTTON_2_3)) ioInputs &= ~BA_IO_SKILL_A; // MAYBE SKILL++ AND SKILL-- ? ONE BUTTON SHORT
   if (currentButtonState & (VEC_BUTTON_1_4|VEC_BUTTON_2_4)) ioInputs &= ~BA_IO_SKILL_C;
  
#ifdef NEVER
  /*
# Initialization file for Barrier
>
>*** Barrier ***
>
>Keyboard Mapping:
>
>   One Player  = F1
>   Two Players = F2
>   Coin        = F3
>   Reset Game  = F4
>   Exit        = <Esc>
>
>   Left    = Keypad '4'     Skill A = '1'
>   Right   = Keypad '6'     Skill B = '2'
>   Forward = Keypad '8'     Skill C = '3'
>   Reverse = Keypad '2'
>
>   Use 'Forward', and any skill keys, to enter high score.

 ; Switch definitions:
 ;
 ;   XXXXX--  Unused
 ;   -----S-  0=Audio in attract mode, 1=No audio (sound not supported)
 ;   ------I  0=5 innings per game, 1=3 innings per game 

 Switches=0000001

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit 
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF

 Player1 = 00000000,FFFFF7FF,00000800,FFFFFFFF
 Player2 = 00000000,FFFFFFEF,00000010,FFFFFFFF

 SkillA  = 00000000,FFFFFFFE,00000001,FFFFFFFF
 SkillB  = 00000000,FFFFFFFB,00000004,FFFFFFFF
 SkillC  = 00000000,FFFFFFBF,00000040,FFFFFFFF

 ; Use these definitions if you want seperate player 1 & 2 controls
 ; (You'll have to define the player 2 keys you want to use in [KeyMapping])

; Left1    = 00000000,FFFFBFFF,00004000,FFFFFFFF
; Right1   = 00000000,FFFFFDFF,00000200,FFFFFFFF
; Fwd1     = 00000000,FFFFEFFF,00001000,FFFFFFFF
; Rev1     = 00000000,FFFFFFF7,00000008,FFFFFFFF

; Left2    = 00000000,FFFFFEFF,00000100,FFFFFFFF
; Right2   = 00000000,FFFF7FFF,00008000,FFFFFFFF
; Fwd2     = 00000000,FFFFDFFF,00002000,FFFFFFFF
; Rev2     = 00000000,FFFFFBFF,00000400,FFFFFFFF

 ; Use these definitions to have Player 1 and 2 share the same keys

 Left1&2  = 00000000,FFFFBEFF,00004100,FFFFFFFF
 Right1&2 = 00000000,FFFF7DFF,00008200,FFFFFFFF
 Fwd1&2   = 00000000,FFFFCFFF,00003000,FFFFFFFF
 Rev1&2   = 00000000,FFFFFBF7,00000408,FFFFFFFF

   */
#endif
   if (frameCounter < 500) {
     if (v_rotate) {
       line(v_yb,v_xl, v_yt,v_xl, 6);
       line(v_yt,v_xl, v_yt,v_xr, 6);
       line(v_yt,v_xr, v_yb,v_xr, 6);
       line(v_yb,v_xr, v_yb,v_xl, 6);
     } else {
       line(v_xl,v_yb, v_xl,v_yt, 6);
       line(v_xl,v_yt, v_xr,v_yt, 6);
       line(v_xr,v_yt, v_xr,v_yb, 6);
       line(v_xr,v_yb, v_xl,v_yb, 6);
     }
   }
}

void startFrame_sundance(void) {
  // need to check P1/P2 against L/R
#define SD_IO_P1START 0x04
#define SD_IO_P2START 0x08
#define SD_IO_GRID 0x20
#define SD_IO_2SUNS 0x0800
#define SD_IO_3SUNS 0x10
#define SD_IO_4SUNS 0x40
#define SD_IO_P1FIRE 0x02
#define SD_IO_P2FIRE 0x80

#define SD_SW_COIN    0x080
  static int prevButtonState;	// for debouncing
  int Square, X, Y;

   frameCounter += 1;
   DEBUG_OUT("// Frame %d\n", frameCounter);

   v_WaitRecal ();
   // v_doSound();
   prevButtonState = currentButtonState;
   v_readButtons ();		// update currentButtonState
   v_readJoystick1Analog ();
   //v_readJoystick2Analog ();  // Apparently we are getting joystick 2 data anyway?????
   // v_playAllSFX();

  ioSwitches |= SD_SW_COIN;
  
  if ((currentButtonState & ~prevButtonState) & VEC_BUTTON_1_1) ioSwitches &= ~SD_SW_COIN;	// only on rising edge

#define P1XLEFT 1
#define P1XCENT 2
#define P1XRIGHT 4
#define P1YTOP 8
#define P1YCENT 16
#define P1YBOT 32

  ioInputs |= 0x5201 | 0xA580;

  if (currentJoy1X > 30) X = P1XRIGHT; else if (currentJoy1X < -30) X = P1XLEFT; else X = P1XCENT;
  if (currentJoy1Y > 30) Y = P1YTOP; else if (currentJoy1Y < -30) Y = P1YBOT; else Y = P1YCENT;

  switch (X|Y) {
    /*
 Hatch1R = 00004000,FFFFEDFE,00005201,FFFFFFFF    1201
 Hatch2R = 00004201,FFFFEFFF,00005201,FFFFFFFF    1000
 Hatch3R = 00005200,FFFFFFFE,00005201,FFFFFFFF    0001
 Hatch4R = 00001201,FFFFBFFF,00005201,FFFFFFFF    4000
 Hatch5R = 00004200,FFFFEFFE,00005201,FFFFFFFF    1001
 Hatch6R = 00005001,FFFFFDFF,00005201,FFFFFFFF    0200
 Hatch7R = 00001200,FFFFBFFE,00005201,FFFFFFFF    4001
 Hatch8R = 00004001,FFFFEDFF,00005201,FFFFFFFF    1200
 Hatch9R = 00005000,FFFFFDFE,00005201,FFFFFFFF    0201
     */
  case P1XLEFT+P1YTOP: Square = 0x1201; break;
  case P1XCENT+P1YTOP: Square = 0x1000; break;
  case P1XRIGHT+P1YTOP: Square = 0x0001; break;

  case P1XLEFT+P1YCENT: Square = 0x4000; break;
  case P1XCENT+P1YCENT: Square = 0x1001; break;
  case P1XRIGHT+P1YCENT: Square = 0x0200; break;

  case P1XLEFT+P1YBOT: Square = 0x4001; break;
  case P1XCENT+P1YBOT: Square = 0x1200; break;
  case P1XRIGHT+P1YBOT: Square = 0x0201; break;
  }
  ioInputs &= ~Square;


#define P2XLEFT 1
#define P2XCENT 2
#define P2XRIGHT 4
#define P2YTOP 8
#define P2YCENT 16
#define P2YBOT 32


  if (currentJoy2X > 30) X = P2XRIGHT; else if (currentJoy2X < -30) X = P2XLEFT; else X = P2XCENT;
  if (currentJoy2Y > 30) Y = P2YTOP; else if (currentJoy2Y < -30) Y = P2YBOT; else Y = P2YCENT;

  switch (X|Y) {
    /*
 Hatch1L = 00008080,FFFFDAFF,0000A580,FFFFFFFF   2500
 Hatch2L = 00008580,FFFFDFFF,0000A580,FFFFFFFF   2000
 Hatch3L = 0000A180,FFFFFBFF,0000A580,FFFFFFFF   0400

 Hatch4L = 00002580,FFFF7FFF,0000A580,FFFFFFFF   8000
 Hatch5L = 00008180,FFFFDBFF,0000A580,FFFFFFFF   2400
 Hatch6L = 0000A480,FFFFFEFF,0000A580,FFFFFFFF   0100

 Hatch7L = 00002180,FFFF7BFF,0000A580,FFFFFFFF   9400  <-- possibly should be A400 or 8400 ???
 Hatch8L = 00008480,FFFFDEFF,0000A580,FFFFFFFF   2100
 Hatch9L = 0000A080,FFFFFAFF,0000A580,FFFFFFFF   0500
     */
  case P2XLEFT+P2YTOP: Square = 0x2500; break;
  case P2XCENT+P2YTOP: Square = 0x2000; break;
  case P2XRIGHT+P2YTOP: Square = 0x0400; break;

  case P2XLEFT+P2YCENT: Square = 0x8000; break;
  case P2XCENT+P2YCENT: Square = 0x2400; break;
  case P2XRIGHT+P2YCENT: Square = 0x0100; break;

  case P2XLEFT+P2YBOT: Square = 0x9400; break;
  case P2XCENT+P2YBOT: Square = 0x2100; break;
  case P2XRIGHT+P2YBOT: Square = 0x0500; break;
  }
  ioInputs &= ~Square;
  ioInputs |= SD_IO_P1START | SD_IO_P2START | SD_IO_GRID | SD_IO_2SUNS | SD_IO_3SUNS | SD_IO_4SUNS | SD_IO_P1FIRE | SD_IO_P2FIRE;

  if ((currentButtonState & ~prevButtonState) & (VEC_BUTTON_1_1|VEC_BUTTON_2_1)) ioSwitches &= ~SD_SW_COIN;	// only on rising edge
  if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~SD_IO_P1START;
  if (currentButtonState & VEC_BUTTON_2_2) ioInputs &= ~SD_IO_P2START;
  if (currentButtonState & (VEC_BUTTON_1_3|VEC_BUTTON_2_3)) ioInputs &= ~SD_IO_GRID;
  if (currentButtonState & VEC_BUTTON_1_4) ioInputs &= ~SD_IO_P1FIRE;
  if (currentButtonState & VEC_BUTTON_2_4) ioInputs &= ~SD_IO_P2FIRE;

#ifdef NEVER
  /*
# Initialization file for Sundance
>
>*** Sundance ***
>
>Keyboard Mapping:
>
>   One Player  = F1      Grid Control = '1'
>   Two Players = F2      2 Suns       = '2'
>   Coin        = F3      3 Suns       = '3'
>   Reset Game  = F4      4 Suns       = '4'
>   Exit        = <Esc>
>   
>   Left Player:          Right Player: 
>    -----------           -----------
>   | R | T | Y |         | 7 | 8 | 9 |
>   |---|---|---|         |---|---|---|
>   | F | G | H |         | 4 | 5 | 6 |
>   |---|---|---|         |---|---|---|
>   | V | B | N |         | 1 | 2 | 3 |
>    -----------           -----------
>   Fire = <Left Shift>   Fire = <Right Shift>

 ; Switch definitions:
 ;
 ;   XXX----  Unused
 ;   ---P---  0=2 players needs 2 coins, 1=2 players need only 1 coin
 ;   ----E--  0=Japanese, 1=English
 ;
 ;   -----TT  11 = 0:45 minutes per coin
 ;            01 = 1:00 minutes per coin
 ;            10 = 1:30 minutes per coin
 ;            00 = 2:00 minutes per coin

 Switches=0000101

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit 
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF

 Player1 = 00000000,FFFFFFFB,00000004,FFFFFFFF
 Player2 = 00000000,FFFFFFF7,00000008,FFFFFFFF

 Grid    = 00000000,FFFFFFDF,00000020,FFFFFFFF
 2Suns   = 00000000,FFFFF7FF,00000800,FFFFFFFF
 3Suns   = 00000000,FFFFFFEF,00000010,FFFFFFFF
 4Suns   = 00000000,FFFFFFBF,00000040,FFFFFFFF
 FireR   = 00000000,FFFFFFFD,00000002,FFFFFFFF
 FireL   = 00000000,FFFFFF7F,00000080,FFFFFFFF 

 Hatch1R = 00004000,FFFFEDFE,00005201,FFFFFFFF
 Hatch2R = 00004201,FFFFEFFF,00005201,FFFFFFFF
 Hatch3R = 00005200,FFFFFFFE,00005201,FFFFFFFF
 Hatch4R = 00001201,FFFFBFFF,00005201,FFFFFFFF
 Hatch5R = 00004200,FFFFEFFE,00005201,FFFFFFFF
 Hatch6R = 00005001,FFFFFDFF,00005201,FFFFFFFF
 Hatch7R = 00001200,FFFFBFFE,00005201,FFFFFFFF
 Hatch8R = 00004001,FFFFEDFF,00005201,FFFFFFFF
 Hatch9R = 00005000,FFFFFDFE,00005201,FFFFFFFF

 Hatch1L = 00008080,FFFFDAFF,0000A580,FFFFFFFF 
 Hatch2L = 00008580,FFFFDFFF,0000A580,FFFFFFFF 
 Hatch3L = 0000A180,FFFFFBFF,0000A580,FFFFFFFF 
 Hatch4L = 00002580,FFFF7FFF,0000A580,FFFFFFFF 
 Hatch5L = 00008180,FFFFDBFF,0000A580,FFFFFFFF 
 Hatch6L = 0000A480,FFFFFEFF,0000A580,FFFFFFFF 
 Hatch7L = 00002180,FFFF7BFF,0000A580,FFFFFFFF 
 Hatch8L = 00008480,FFFFDEFF,0000A580,FFFFFFFF 
 Hatch9L = 0000A080,FFFFFAFF,0000A580,FFFFFFFF 
   */
#endif
   if (frameCounter < 500) {
     if (v_rotate) {
       line(v_yb,v_xl, v_yt,v_xl, 6);
       line(v_yt,v_xl, v_yt,v_xr, 6);
       line(v_yt,v_xr, v_yb,v_xr, 6);
       line(v_yb,v_xr, v_yb,v_xl, 6);
     } else {
       line(v_xl,v_yb, v_xl,v_yt, 6);
       line(v_xl,v_yt, v_xr,v_yt, 6);
       line(v_xr,v_yt, v_xr,v_yb, 6);
       line(v_xr,v_yb, v_xl,v_yb, 6);
     }
   }
}

void startFrame_qb3(void) {
  startFrame();
#ifdef NEVER
  /*
# Initialization file for QB3 - under development
>
>*** QB3 ***
>
>Keyboard Mapping:
>
>   Start Game  = F1
>   Coin        = F3
>   Reset Game  = F4
>   Exit        = <Esc>
>


 Inputs=FFFF ; unknown for now # <input players="1" buttons="4" coins="1">
 Switches= 1001001000010   ; free play on
                      00   2 lives
                      01   4 lives
                      10   3 lives default
                      11   5 lives
                    1???   free play off  default
                    0???   free play on
                 1??????   service mode off  default
                 0??????   service mode on
              1?????????   debug off  default
              0?????????   debug on
           1????????????   infinite lives off  default
           0????????????   infinite lives on

[Inputs]
 RstCPU  = 02000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating RESET
 Exit    = 01000000,FFFFFFFF,00000000,FFFFFFFF	; Set bit indicating Exit 
 Coin    = 00000000,FFFFFFFF,00000000,FF7FFFFF

 Start   = 00000000,FFFFFF7F,00000080,FFFFFFFF	; Start game

#        	       	<rom name="qb3_le_t7.bin" size="8192" crc="adaaee4c" sha1="35c6bbb50646a3ddec12f115fcf3f2283e15b0a0" region="maincpu" offset="0"/>
#        	       	<rom name="qb3_lo_p7.bin" size="8192" crc="72f6199f" sha1="ae8f81f218940cfc3aef8f82dfe8cc14220770ce" region="maincpu" offset="1"/>
#        	       	<rom name="qb3_ue_u7.bin" size="8192" crc="050a996d" sha1="bf29236112746b5925b29fb231f152a4bde3f4f9" region="maincpu" offset="4000"/>
#        	       	<rom name="qb3_uo_r7.bin" size="8192" crc="33fa77a2" sha1="27a6853f8c2614a2abd7bfb9a62c357797312068" region="maincpu" offset="4001"/>
#        	       	<display tag="screen" type="vector" rotate="180" flipx="yes" refresh="38.000000" />
   */
#endif
}

void startFrame_tailgunner (void)
{
#define TG_IO_START   0x80	/* for ioInputs */
#define TG_IO_SHIELDS 0x40
#define TG_IO_FIRE    0x20
#define TG_IO_DOWN    0x10
#define TG_IO_UP      0x08
#define TG_IO_LEFT    0x04
#define TG_IO_RIGHT   0x02
  //#define TG_IO_COIN    0x01
#define TG_SW_ABORT   SW_ABORT	/* for ioSwitches */
#define TG_SW_COIN    0x080
   static int prevButtonState;	// for debouncing

   frameCounter += 1;
   DEBUG_OUT("v_WaitRecal(); v_setBrightness(64);v_readButtons();v_readJoystick1Analog(); // %d\n", frameCounter);

   v_WaitRecal ();
   // v_doSound();
   prevButtonState = currentButtonState;
   v_readButtons ();		// update currentButtonState
   v_readJoystick1Analog ();
   // v_playAllSFX();

   // default inactive:
   ioInputs |= TG_IO_LEFT | TG_IO_RIGHT | TG_IO_UP | TG_IO_DOWN | TG_IO_START | TG_IO_SHIELDS | TG_IO_FIRE;
   ioSwitches |= TG_SW_COIN;

   // set x and y from joystick
   RCram[42] = JoyX = (currentJoy1X * 5 / 2) & 0xfff;	// crude but the best I've found
   RCram[43] = JoyY = (currentJoy1Y * 5 / 2) & 0xfff;   // WHY ARE THE JOYSTICKS NON-RESPONSIVE IN STANDALONE MODE???

   if ((currentButtonState & ~prevButtonState) & VEC_BUTTON_1_1) ioSwitches &= ~TG_SW_COIN;	// only on rising edge
   if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~TG_IO_START;
   if (currentButtonState & VEC_BUTTON_1_3) ioInputs &= ~TG_IO_SHIELDS;
   if (currentButtonState & VEC_BUTTON_1_4) ioInputs &= ~TG_IO_FIRE;
   if (frameCounter < 500) {
     if (v_rotate) {
       line(v_yb,v_xl, v_yt,v_xl, 6);
       line(v_yt,v_xl, v_yt,v_xr, 6);
       line(v_yt,v_xr, v_yb,v_xr, 6);
       line(v_yb,v_xr, v_yb,v_xl, 6);
     } else {
       line(v_xl,v_yb, v_xl,v_yt, 6);
       line(v_xl,v_yt, v_xr,v_yt, 6);
       line(v_xr,v_yt, v_xr,v_yb, 6);
       line(v_xr,v_yb, v_xl,v_yb, 6);
     }
   }
}

void startFrame_demon (void)
{
#define DE_IO_P1START   0x01  // 1-player start
#define DE_IO_P1WALK    0x10
#define DE_IO_P1FIRE    0x20
#define DE_IO_P1LEFT    0x04
#define DE_IO_P1RIGHT   0x08
#define DE_IO_P1PANIC   0x0200

#define DE_IO_P2START   0x02  // 2-player start
#define DE_IO_P2WALK    0x2000
#define DE_IO_P2FIRE    0x4000
#define DE_IO_P2LEFT    0x0800
#define DE_IO_P2RIGHT   0x1000
#define DE_IO_P2PANIC   0x0400
  
#define DE_SW_ABORT   SW_ABORT	/* for ioSwitches */
#define DE_SW_COIN    0x080     // Not used in freeplay mode
   static int prevButtonState;	// for debouncing

   frameCounter += 1;
   DEBUG_OUT("v_WaitRecal(); v_setBrightness(64);v_readButtons();v_readJoystick1Analog(); // %d\n", frameCounter);

   v_WaitRecal ();
   // v_doSound();
   prevButtonState = currentButtonState;
   v_readButtons ();		// update currentButtonState
   v_readJoystick1Analog ();
   //v_readJoystick2Analog ();  // NOT YET IMPLEMENTED.
   // v_playAllSFX();

   // Unfortunately, it's quite common to press LEFT and RIGHT simultaneously by accident with this
   // layout, and accidentally invoke the configuration screen.  Need to think about how we will
   // handle this...
   
   // default inactive:
   ioInputs |= DE_IO_P1LEFT | DE_IO_P1RIGHT | DE_IO_P1START | DE_IO_P1WALK | DE_IO_P1FIRE | DE_IO_P1PANIC
             | DE_IO_P2LEFT | DE_IO_P2RIGHT | DE_IO_P2START | DE_IO_P2WALK | DE_IO_P2FIRE | DE_IO_P2PANIC;

   // ioSwitches |= DE_SW_COIN;
   // not used in freeplay mode
   // if ((currentButtonState & ~prevButtonState) & VEC_BUTTON_1_1) ioSwitches &= ~DE_SW_COIN;	// only on rising edge

   // digital joysticks...
   if (currentJoy1X < -30) ioInputs &= ~DE_IO_P1LEFT;
   if (currentJoy1X > 30) ioInputs &= ~DE_IO_P1RIGHT;
   if (currentJoy1Y > 30) ioInputs &= ~DE_IO_P1WALK;
   if (currentJoy1Y < -30) ioInputs &= ~DE_IO_P1PANIC;

   if (currentJoy2X < -30) ioInputs &= ~DE_IO_P2LEFT;
   if (currentJoy2X > 30) ioInputs &= ~DE_IO_P2RIGHT;
   if (currentJoy2Y > 30) ioInputs &= ~DE_IO_P2WALK;
   if (currentJoy2Y < -30) ioInputs &= ~DE_IO_P2PANIC;

   if (currentButtonState & VEC_BUTTON_1_1) ioInputs &= ~DE_IO_P1START;
   if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~DE_IO_P2START;

   if (currentButtonState & VEC_BUTTON_1_1) ioInputs &= ~DE_IO_P1LEFT;
   if (currentButtonState & VEC_BUTTON_1_2) ioInputs &= ~DE_IO_P1RIGHT;
   if (currentButtonState & VEC_BUTTON_1_3) ioInputs &= ~DE_IO_P1WALK;
   if (currentButtonState & VEC_BUTTON_1_4) ioInputs &= ~DE_IO_P1FIRE;

   if (currentButtonState & VEC_BUTTON_2_1) ioInputs &= ~DE_IO_P2LEFT;
   if (currentButtonState & VEC_BUTTON_2_2) ioInputs &= ~DE_IO_P2RIGHT;
   if (currentButtonState & VEC_BUTTON_2_3) ioInputs &= ~DE_IO_P2WALK;
   if (currentButtonState & VEC_BUTTON_2_4) ioInputs &= ~DE_IO_P2FIRE;
   if (frameCounter < 500) {
     if (v_rotate) {
       line(v_yb,v_xl, v_yt,v_xl, 6);
       line(v_yt,v_xl, v_yt,v_xr, 6);
       line(v_yt,v_xr, v_yb,v_xr, 6);
       line(v_yb,v_xr, v_yb,v_xl, 6);
     } else {
       line(v_xl,v_yb, v_xl,v_yt, 6);
       line(v_xl,v_yt, v_xr,v_yt, 6);
       line(v_xr,v_yt, v_xr,v_yb, 6);
       line(v_xr,v_yb, v_xl,v_yb, 6);
     }
   }
}

void startFrame_speedfreak (void)
{
   static int coinKey = 0;
   static int startKey = 0;
   static int gasKey = 0;
   static int gear1Key = 0;
   static int currentGear = -1;

   static int counter = 0;

   DEBUG_OUT("v_WaitRecal(); v_setBrightness(64);v_readButtons();v_readJoystick1Analog(); // %d\n", ++counter);
   v_WaitRecal ();
   // v_doSound();
   v_readButtons ();
   v_readJoystick1Analog ();
   // v_playAllSFX();

   // coin is button 1 joyport 0
   if (currentButtonState & 1) {
      // Coin = 00000000,FFFFFFFF,00000000,FF7FFFFF
      if (!coinKey) {
	 coinKey = 1;
	 ioSwitches = ioSwitches | 0x0000;
	 ioInputs = ioInputs | 0x0000;

	 ioSwitches = ioSwitches & 0xffff;
	 ioInputs = ioInputs & 0xffff;
      }
   } else {
      if (coinKey) {
	 coinKey = 0;
	 ioSwitches = ioSwitches | 0x0000;
	 ioInputs = ioInputs | 0x0000;

	 ioSwitches = ioSwitches & 0xff7f;
	 ioInputs = ioInputs & 0xffff;
      }
   }

   // start is button 2 joyport 0
   if (currentButtonState & 2) {
      // Start = 00000000,FFFFFF7F,00000080,FFFFFFFF ; Start game
      if (!startKey) {
	 startKey = 1;
	 ioSwitches = ioSwitches | 0x0000;
	 ioInputs = ioInputs | 0x0000;

	 ioSwitches = ioSwitches & 0xffff;
	 ioInputs = ioInputs & 0xFF7F;
      }
   } else {
      if (startKey) {
	 startKey = 0;
	 ioSwitches = ioSwitches | 0x0000;
	 ioInputs = ioInputs | 0x0080;

	 ioSwitches = ioSwitches & 0xffff;
	 ioInputs = ioInputs & 0xffff;
      }
      currentGear = -1;

   }

   // gas is button 4 joyport 0
   if (currentButtonState & 8) {
      // Gas = 00000000,FFFFFEFF,00000100,FFFFFFFF ; Gas
      if (!gasKey) {
	 gasKey = 1;
	 ioInputs = ioInputs & 0xFEFF;
      }
   } else {
      if (gasKey) {
	 gasKey = 0;
	 ioInputs = ioInputs | 0x0100;
      }
   }

   // gear 1 is button 3 joyport 0
   if (currentButtonState & 4) {
      // Gear1 = 000000E0,FFFFFFEF,00000000,FFFFFFFF ; First Gear
      if (!gear1Key) {
	 gear1Key = 1;
	 ioInputs = ioInputs | 0x00E0;
	 ioInputs = ioInputs & 0xFFEF;
      }
   } else {
      if (gear1Key) {
	 gear1Key = 0;
      }
   }

   static int shiftCounter = 0;

// bit 0 indicates 0 =  negative/ right
// bit 0 indicates 1 =  positive/ left
// bit 1-3 indicate speed in the direction
// so min speed = 001, max = 111 -> 1-7  

   if ((currentJoy1X > 0) && (currentJoy1X > 30)) {
      // range from 30-127 -> 0-97 / 16 -> 0-6 (+1)
      // left test
      ioInputs = ioInputs & 0xfff0;
      ioInputs = ioInputs | (((((currentJoy1X - 30) / 16) + 1) << 1) + 1);	/* between 1-7 */
   } else if ((currentJoy1X < 0) && (currentJoy1X < -30)) {
      // right test
      ioInputs = ioInputs & 0xfff0;
      ioInputs = ioInputs | (((((-currentJoy1X) - 30) / 16) + 1) << 1);	/* between 1-7 */
   } else {
      // release should not be necessary, shift is cleared each round!
      ioInputs = ioInputs & 0xfff0;
   }
   if (frameCounter < 500) {
     if (v_rotate) {
       line(v_yb,v_xl, v_yt,v_xl, 6);
       line(v_yt,v_xl, v_yt,v_xr, 6);
       line(v_yt,v_xr, v_yb,v_xr, 6);
       line(v_yb,v_xr, v_yb,v_xl, 6);
     } else {
       line(v_xl,v_yb, v_xl,v_yt, 6);
       line(v_xl,v_yt, v_xr,v_yt, 6);
       line(v_xr,v_yt, v_xr,v_yb, 6);
       line(v_xr,v_yb, v_xl,v_yb, 6);
     }
   }
}

void CinemaVectorData (int RCFromX, int RCFromY, int RCToX, int RCToY, int vgColour)
{
   RCFromX = SEX (RCFromX); RCFromY = SEX (RCFromY); RCToX = SEX (RCToX); RCToY = SEX (RCToY);
   line (RCFromX, RCFromY, RCToX, RCToY, vgColour);
}

void xUNFINISHED (char *s, int RCregister_PC)
{
   DEBUG_OUT("%04x DANGER: Unimplemented instruction - %s\n", RCregister_PC, s);
   exit (1);
}
#define UNFINISHED(s) xUNFINISHED(s, RCregister_PC)

/* Reset C-CPU registers, flags, etc to default starting values */
void cineReset (void)
{

   /* zero registers */
   RCregister_PC = 0;
   RCregister_A = 0;
   RCregister_B = 0;
   RCregister_I = 0;
   RCregister_J = 0;
   RCregister_P = 0;
   RCFromX = 0;
   RCFromY = 0;
   RCregister_T = 0;

   /* zero flags */
   RCflag_C = 0;

   /* reset RCstate */
   RCstate = 0;

   /* reset RCram */
   memset (RCram, 0, sizeof (RCram));

   /* reset internal RCstate */
   RCcmp_old = 0;
   RCcmp_new = 0;
   SETA0 (0);

}

void cineSetGame (char *name, int id)
{
   strcpy (ccpu_game_name, name);
   ccpu_game_id = id;
}

void cineSetJMI (UINT8 j)
{
   ccpu_jmi_dip = j;
   DEBUG_OUT("cineSetJMI(%u);\n", j);
}

void cineSetMSize (UINT8 m)
{
   ccpu_msize = m;
   DEBUG_OUT("cineSetMSize(%u);\n", m);
}

void cineSetMonitor (UINT8 m)
{
   ccpu_monitor = m;
   DEBUG_OUT("cineSetMonitor(%u);\n", m);
}

UINT32 cineGetElapsedTicks (UINT32 dwClearIt)
{
   UINT32 dwTemp;
   dwTemp = dwElapsedTicks;
   if (dwClearIt) dwElapsedTicks = 0;
   return (dwTemp);
}

void cineReleaseTimeslice (void)
{
   bBailOut = TRUE;
}

/* extra functions */

/* ----------------------------------------------- Trace debugger cinedbg.c */

/* handy macros */

void disassemble (char *buffer, unsigned int maxlen, unsigned int address);

#define START(x) DEBUG_OUT("Entered C-CPU function: %s\n", x )
#define STOP(x) DEBUG_OUT("Left C-CPU function: %s\n", x )

/* opcode tables for debugging */
typedef enum
{
   CLR, LDA, INP, ADD, SUB, LDJ, LDP, OUT,
   CMP, LDI, STA, VIN, VDR, XLT, MUL, LLT,
   WAI, AWD, AND, LSR, LSL, ASR, ASRD, LSLD,
   JPPB, JMIB, JDRB, JLTB, JEQB, JNCB, JAOB, NOPB,
   JMPA, JMIA, JDRA, JLTA, JEQA, JNCA, JAOA, NOPA,
   JEIA, JEIB
} opcode_mnemonics;

typedef struct
{
   opcode_mnemonics od_opcode;
   char *od_name;
} opcode_detail;

opcode_detail opcodes[] = {
   {CLR, "clr"},
   {LDA, "lda"},
   {INP, "inp"},
   {ADD, "add"},
   {SUB, "sub"},
   {LDJ, "ldj"},
   {LDP, "ldp"},
   {OUT, "out"},
   {CMP, "cmp"},
   {LDI, "ldi"},
   {STA, "sta"},
   {VIN, "vin"},
   {VDR, "vdr"},
   {XLT, "xlt"},
   {MUL, "mul"},
   {LLT, "llt"},
   {WAI, "wai"},
   {AWD, "awd"},
   {AND, "and"},
   {LSR, "lsr"},
   {LSL, "lsl"},
   {ASR, "asr"},
   {ASRD, "asrd"},
   {LSLD, "lsld"},
   {JPPB, "jppb"},
   {JMIB, "jmib"},
   {JDRB, "jdrb"},
   {JLTB, "jltb"},
   {JEQB, "jeqb"},
   {JNCB, "jncb"},
   {JAOB, "jaob"},
   {NOPB, "nopb"},
   {JMPA, "jmpa"},
   {JMIA, "jmia"},
   {JDRA, "jdra"},
   {JLTA, "jlta"},
   {JEQA, "jeqa"},
   {JNCA, "jnca"},
   {JAOA, "jaoa"},
   {NOPA, "nopa"},
   {JEIA, "jeia"},
   {JEIB, "jeib"}
};

typedef enum
{
   ACC,				/* Accumulator */
   ADIR,			/* Acc Direct memory access */
   AIM4,			/* Acc 4 bit immediate */
   AIM4X,			/* Acc 4 bit immediate extended size */
   AIM8,			/* Acc 8 bit immediate */
   AINDM,			/* Acc indirect through memory */
   AIMX,			/* Acc immediate extended A-reg */
   AXLT,			/* Acc lookup ROM using Acc as pointer */
   AIRG,			/* Acc Through the I-reg */
   IRG,				/* Through the I-reg */
   IM4,				/* 4 bit immediate */
   IM12,			/* 12 bit immediate */
   DIRECT,			/* Direct memory access */
   IMP,				/* Implied */
   JUMP,			/* Acc selection/Jump instruction */
   JUMPX			/* Acc selection/Extended jump instruction */
} amode_mnemonics;

typedef struct
{
   amode_mnemonics ad_amode;
   char *ad_name;
} amode_detail;

amode_detail amodes[] = {
   {ACC, "acc"},
   {ADIR, "adir"},
   {AIM4, "aim4"},
   {AIM4X, "aim4x"},
   {AIM8, "aim8"},
   {AINDM, "aindm"},
   {AIMX, "aimx"},
   {AXLT, "axlt"},
   {AIRG, "airg"},
   {IRG, "irg"},
   {IM4, "im4"},
   {IM12, "im12"},
   {DIRECT, "dir"},
   {IMP, "imp"},
   {JUMP, "jump"},
   {JUMPX, "jumpx"}
};

typedef struct
{
   opcode_mnemonics ot_opcode;
   amode_mnemonics ot_amode;
} opcode_table_entry;

opcode_table_entry opcode_table[] = {
   {CLR, ACC},			/* 00 */
   {LDA, AIM4X},		/* 01 */
   {LDA, AIM4X},		/* 02 */
   {LDA, AIM4X},		/* 03 */
   {LDA, AIM4X},		/* 04 */
   {LDA, AIM4X},		/* 05 */
   {LDA, AIM4X},		/* 06 */
   {LDA, AIM4X},		/* 07 */
   {LDA, AIM4X},		/* 08 */
   {LDA, AIM4X},		/* 09 */
   {LDA, AIM4X},		/* 0A */
   {LDA, AIM4X},		/* 0B */
   {LDA, AIM4X},		/* 0C */
   {LDA, AIM4X},		/* 0D */
   {LDA, AIM4X},		/* 0E */
   {LDA, AIM4X},		/* 0F */

   {INP, ADIR},			/* 10 */
   {INP, ADIR},			/* 11 */
   {INP, ADIR},			/* 12 */
   {INP, ADIR},			/* 13 */
   {INP, ADIR},			/* 14 */
   {INP, ADIR},			/* 15 */
   {INP, ADIR},			/* 16 */
   {INP, ADIR},			/* 17 */
   {INP, ADIR},			/* 18 */
   {INP, ADIR},			/* 19 */
   {INP, ADIR},			/* 1A */
   {INP, ADIR},			/* 1B */
   {INP, ADIR},			/* 1C */
   {INP, ADIR},			/* 1D */
   {INP, ADIR},			/* 1E */
   {INP, ADIR},			/* 1F */

   {ADD, AIM8},			/* 20 */
   {ADD, AIM4},			/* 21 */
   {ADD, AIM4},			/* 22 */
   {ADD, AIM4},			/* 23 */
   {ADD, AIM4},			/* 24 */
   {ADD, AIM4},			/* 25 */
   {ADD, AIM4},			/* 26 */
   {ADD, AIM4},			/* 27 */
   {ADD, AIM4},			/* 28 */
   {ADD, AIM4},			/* 29 */
   {ADD, AIM4},			/* 2A */
   {ADD, AIM4},			/* 2B */
   {ADD, AIM4},			/* 2C */
   {ADD, AIM4},			/* 2D */
   {ADD, AIM4},			/* 2E */
   {ADD, AIM4},			/* 2F */

   {SUB, AIM8},			/* 30 */
   {SUB, AIM4},			/* 31 */
   {SUB, AIM4},			/* 32 */
   {SUB, AIM4},			/* 33 */
   {SUB, AIM4},			/* 34 */
   {SUB, AIM4},			/* 35 */
   {SUB, AIM4},			/* 36 */
   {SUB, AIM4},			/* 37 */
   {SUB, AIM4},			/* 38 */
   {SUB, AIM4},			/* 39 */
   {SUB, AIM4},			/* 3A */
   {SUB, AIM4},			/* 3B */
   {SUB, AIM4},			/* 3C */
   {SUB, AIM4},			/* 3D */
   {SUB, AIM4},			/* 3E */
   {SUB, AIM4},			/* 3F */

   {LDJ, IM12},			/* 40 */
   {LDJ, IM12},			/* 41 */
   {LDJ, IM12},			/* 42 */
   {LDJ, IM12},			/* 43 */
   {LDJ, IM12},			/* 44 */
   {LDJ, IM12},			/* 45 */
   {LDJ, IM12},			/* 46 */
   {LDJ, IM12},			/* 47 */
   {LDJ, IM12},			/* 48 */
   {LDJ, IM12},			/* 49 */
   {LDJ, IM12},			/* 4A */
   {LDJ, IM12},			/* 4B */
   {LDJ, IM12},			/* 4C */
   {LDJ, IM12},			/* 4D */
   {LDJ, IM12},			/* 4E */
   {LDJ, IM12},			/* 4F */

   {JPPB, JUMP},		/* 50 */
   {JMIB, JUMP},		/* 51 */
   {JDRB, JUMP},		/* 52 */
   {JLTB, JUMP},		/* 53 */
   {JEQB, JUMP},		/* 54 */
   {JNCB, JUMP},		/* 55 */
   {JAOB, JUMP},		/* 56 */
   {NOPB, IMP},			/* 57 */

   {JMPA, JUMP},		/* 58 */
   {JMIA, JUMP},		/* 59 */
   {JDRA, JUMP},		/* 5A */
   {JLTA, JUMP},		/* 5B */
   {JEQA, JUMP},		/* 5C */
   {JNCA, JUMP},		/* 5D */
   {JAOA, JUMP},		/* 5E */
   {NOPA, IMP},			/* 5F */

   {ADD, ADIR},			/* 60 */
   {ADD, ADIR},			/* 61 */
   {ADD, ADIR},			/* 62 */
   {ADD, ADIR},			/* 63 */
   {ADD, ADIR},			/* 64 */
   {ADD, ADIR},			/* 65 */
   {ADD, ADIR},			/* 66 */
   {ADD, ADIR},			/* 67 */
   {ADD, ADIR},			/* 68 */
   {ADD, ADIR},			/* 69 */
   {ADD, ADIR},			/* 6A */
   {ADD, ADIR},			/* 6B */
   {ADD, ADIR},			/* 6C */
   {ADD, ADIR},			/* 6D */
   {ADD, ADIR},			/* 6E */
   {ADD, ADIR},			/* 6F */

   {SUB, ADIR},			/* 70 */
   {SUB, ADIR},			/* 71 */
   {SUB, ADIR},			/* 72 */
   {SUB, ADIR},			/* 73 */
   {SUB, ADIR},			/* 74 */
   {SUB, ADIR},			/* 75 */
   {SUB, ADIR},			/* 76 */
   {SUB, ADIR},			/* 77 */
   {SUB, ADIR},			/* 78 */
   {SUB, ADIR},			/* 79 */
   {SUB, ADIR},			/* 7A */
   {SUB, ADIR},			/* 7B */
   {SUB, ADIR},			/* 7C */
   {SUB, ADIR},			/* 7D */
   {SUB, ADIR},			/* 7E */
   {SUB, ADIR},			/* 7F */

   {LDP, IM4},			/* 80 */
   {LDP, IM4},			/* 81 */
   {LDP, IM4},			/* 82 */
   {LDP, IM4},			/* 83 */
   {LDP, IM4},			/* 84 */
   {LDP, IM4},			/* 85 */
   {LDP, IM4},			/* 86 */
   {LDP, IM4},			/* 87 */
   {LDP, IM4},			/* 88 */
   {LDP, IM4},			/* 89 */
   {LDP, IM4},			/* 8A */
   {LDP, IM4},			/* 8B */
   {LDP, IM4},			/* 8C */
   {LDP, IM4},			/* 8D */
   {LDP, IM4},			/* 8E */
   {LDP, IM4},			/* 8F */

   {OUT, ADIR},			/* 90 */
   {OUT, ADIR},			/* 91 */
   {OUT, ADIR},			/* 92 */
   {OUT, ADIR},			/* 93 */
   {OUT, ADIR},			/* 94 */
   {OUT, ADIR},			/* 95 */
   {OUT, ADIR},			/* 96 */
   {OUT, ADIR},			/* 97 */
   {OUT, ADIR},			/* 98 */
   {OUT, ADIR},			/* 99 */
   {OUT, ADIR},			/* 9A */
   {OUT, ADIR},			/* 9B */
   {OUT, ADIR},			/* 9C */
   {OUT, ADIR},			/* 9D */
   {OUT, ADIR},			/* 9E */
   {OUT, ADIR},			/* 9F */

   {LDA, ADIR},			/* A0 */
   {LDA, ADIR},			/* A1 */
   {LDA, ADIR},			/* A2 */
   {LDA, ADIR},			/* A3 */
   {LDA, ADIR},			/* A4 */
   {LDA, ADIR},			/* A5 */
   {LDA, ADIR},			/* A6 */
   {LDA, ADIR},			/* A7 */
   {LDA, ADIR},			/* A8 */
   {LDA, ADIR},			/* A9 */
   {LDA, ADIR},			/* AA */
   {LDA, ADIR},			/* AB */
   {LDA, ADIR},			/* AC */
   {LDA, ADIR},			/* AD */
   {LDA, ADIR},			/* AE */
   {LDA, ADIR},			/* AF */

   {CMP, ADIR},			/* B0 */
   {CMP, ADIR},			/* B1 */
   {CMP, ADIR},			/* B2 */
   {CMP, ADIR},			/* B3 */
   {CMP, ADIR},			/* B4 */
   {CMP, ADIR},			/* B5 */
   {CMP, ADIR},			/* B6 */
   {CMP, ADIR},			/* B7 */
   {CMP, ADIR},			/* B8 */
   {CMP, ADIR},			/* B9 */
   {CMP, ADIR},			/* BA */
   {CMP, ADIR},			/* BB */
   {CMP, ADIR},			/* BC */
   {CMP, ADIR},			/* BD */
   {CMP, ADIR},			/* BE */
   {CMP, ADIR},			/* BF */

   {LDI, DIRECT},		/* C0 */
   {LDI, DIRECT},		/* C1 */
   {LDI, DIRECT},		/* C2 */
   {LDI, DIRECT},		/* C3 */
   {LDI, DIRECT},		/* C4 */
   {LDI, DIRECT},		/* C5 */
   {LDI, DIRECT},		/* C6 */
   {LDI, DIRECT},		/* C7 */
   {LDI, DIRECT},		/* C8 */
   {LDI, DIRECT},		/* C9 */
   {LDI, DIRECT},		/* CA */
   {LDI, DIRECT},		/* CB */
   {LDI, DIRECT},		/* CC */
   {LDI, DIRECT},		/* CD */
   {LDI, DIRECT},		/* CE */
   {LDI, DIRECT},		/* CF */

   {STA, ADIR},			/* D0 */
   {STA, ADIR},			/* D1 */
   {STA, ADIR},			/* D2 */
   {STA, ADIR},			/* D3 */
   {STA, ADIR},			/* D4 */
   {STA, ADIR},			/* D5 */
   {STA, ADIR},			/* D6 */
   {STA, ADIR},			/* D7 */
   {STA, ADIR},			/* D8 */
   {STA, ADIR},			/* D9 */
   {STA, ADIR},			/* DA */
   {STA, ADIR},			/* DB */
   {STA, ADIR},			/* DC */
   {STA, ADIR},			/* DD */
   {STA, ADIR},			/* DE */
   {STA, ADIR},			/* DF */

   {VDR, IMP},			/* E0 */
   {LDJ, IRG},			/* E1 */
   {XLT, AXLT},			/* E2 */
   {MUL, IRG},			/* E3 */
   {LLT, IMP},			/* E4 */
   {WAI, IMP},			/* E5 */
   {STA, AIRG},			/* E6 */
   {ADD, AIRG},			/* E7 */
   {SUB, AIRG},			/* E8 */
   {AND, AIRG},			/* E9 */
   {LDA, AIRG},			/* EA */
   {LSR, ACC},			/* EB */
   {LSL, ACC},			/* EC */
   {ASR, ACC},			/* ED */
   {ASRD, IMP},			/* EE */
   {LSLD, IMP},			/* EF */

   {VIN, IMP},			/* F0 */
   {LDJ, IRG},			/* F1 */
   {XLT, AXLT},			/* F2 */
   {MUL, IRG},			/* F3 */
   {LLT, IMP},			/* F4 */
   {WAI, IMP},			/* F5 */
   {STA, AIRG},			/* F6 */
   {AWD, AIRG},			/* F7 */
   {SUB, AIRG},			/* F8 */
   {AND, AIRG},			/* F9 */
   {LDA, AIRG},			/* FA */
   {LSR, ACC},			/* FB */
   {LSL, ACC},			/* FC */
   {ASR, ACC},			/* FD */
   {ASRD, IMP},			/* FE */
   {LSLD, IMP}			/* FF */
};

/* disassemble() appends a disassembly of the specified address to
 * the end of the specified text buffer. */
void disassemble (char *buffer, unsigned int maxlen, unsigned int address)
{
   char ambuffer[40];		/* text buffer for addressing mode values */
   char opbuffer[40];		/* text buffer for opcodes etc. */
   char flbuffer[80];		/* text buffer for flag dump */
   RCCINEBYTE opcode;
   RCCINEBYTE opsize = 1;
   int iter = 0;

   /* which opcode in opcode table? (includes immediate data) */
   opcode = rom[address];

   /* build addressing mode (value to opcode) */

   memset (ambuffer, 0, sizeof (ambuffer));
   switch (amodes[opcode_table[opcode].ot_amode].ad_amode) {

      /* 4-bit immediate value */
   case AIM4:
   case IM4:
      sprintf (ambuffer, "#$%X", opcode & 0x0F);
      break;

      /* 4-bit extended immediate value */
   case AIM4X:
      sprintf (ambuffer, "#$%X", (opcode & 0x0F) << 8);
      break;

      /* 8-bit immediate value */
   case AIM8:
      sprintf (ambuffer, "#$%02X", rom[address + 1]);
      opsize++;			/* required second byte for value */
      break;

      /* [m] -- indirect through memory */
   case AINDM:
      sprintf (ambuffer, "AINDM/Error");
      break;

      /* [i] -- indirect through 'I' */
   case AIRG:
   case IRG:
      sprintf (ambuffer, "[i]");
      break;

      /* extended 12-bit immediate value */
   case AIMX:
      sprintf (ambuffer, "AIMX/Error");
      break;

      /* no special params */
   case ACC:
   case AXLT:
   case IMP:
      ambuffer[0] = '\0';
      break;

      /* 12-bit immediate value */
   case IM12:
      sprintf (ambuffer, "#$%03X",
	       (rom[address] & 0x0F) +
	       (rom[address + 1] & 0xF0) + ((rom[address + 1] & 0x0F) << 8));
      opsize++;
      break;

      /* display address of direct addressing modes */
   case ADIR:
   case DIRECT:
      sprintf (ambuffer, "$%X", opcode & 0x0F);
      break;

      /* jump through J register */
   case JUMP:
      sprintf (ambuffer, "<jump>");
      break;

      /* extended jump */
   case JUMPX:
      sprintf (ambuffer, "JUMPX/Error");
      break;

   }				/* switch on addressing mode */

   /* build flags dump */
   sprintf (flbuffer, "A=%03X B=%03X I=%03XZ J=%03X P=%X " "A0=%02X %02X N%X O%X %c%c%c%c",
	    RCregister_A, RCregister_B, RCregister_I /* << 1 */,     /* use the <<1 for Zonn style */
	    RCregister_J, RCregister_P, GETA0 (), GETFC () /* ? 'C' : 'c' */,
	    RCcmp_new, RCcmp_old,
	    ((RCstate == RCstate_A) || (RCstate == RCstate_AA)) ? 'A' : ' ',
	    (RCstate == RCstate_AA) ? 'A' : ' ',
	    ((RCstate == RCstate_B) || (RCstate == RCstate_BB)) ? 'B' : ' ',
	    (RCstate == RCstate_BB) ? 'B' : ' ');

   /* build the final output string. Format is as follows: ADDRESS: OPCODE-ROM-DUMP\tOPCODE VALUE\tFLAGS */

   /* append complete opcode ROM dump */
   memset (opbuffer, 0, sizeof (opbuffer));
   for (iter = 0; iter < opsize; iter++) {
      sprintf (opbuffer + (iter * 3), " %02X", rom[address + iter]);
   }

   /* create final output */
   sprintf (buffer, "%04X:%-6s : %-4s %-6s : %s",
	    address, opbuffer,
	    opcodes[opcode_table[opcode].ot_opcode].od_name,
	    ambuffer, flbuffer);

   return;
}

/* cineExec() is what performs all the "processors" work; it will
 * continue to execute until something horrible happens, a watchpoint
 * is hit, cycle count exceeded, or other happy things. */
UINT32 cineExec (void)
{

   RCCINEBYTE temp_byte = 0;	/* cause it'll be handy */
   RCCINEBYTE temp_byte_2 = 0;	/* cause it'll be handy */
   RCCINEWORD temp_word = 0;	/* cause it'll be handy */
   RCCINEWORD temp_word_2 = 0;	/* cause it'll be handy */

   RCCINESWORD temp_sword = 0;	/* in case its handy... */
   RCCINESBYTE temp_sbyte = 0;	/* in case its handy... */

   RCCINEWORD original_PC = RCregister_PC;

   /* before opcode handlers ... */
   ioSwitches &= (~SW_ABORT);	/* ( ! SW_ABORT ) ? */

   /* label; after each opcode is finished running, execution returns to this 
      point, where the RCstate is examined and the proper jump table used. */
 cineExecTop:

   bBailOut = FALSE;

   /* examine the current RCstate, and jump down to the correct opcode jump table. */
   switch (RCstate) {
   case RCstate_A:
      goto opCodeTblA;
   case RCstate_AA:
      goto opCodeTblAA;
   case RCstate_B:
      goto opCodeTblB;
   case RCstate_BB:
      goto opCodeTblBB;
   } /* switch on current RCstate */

   /* include the opcode jump tables; goto the correct piece of code for the
      current opcode. That piece of code will set the RCstate for the next
      run, as well. The opcode jumptables were generated with a perl script
      and are kept separate for make and ugly sake. */

/* ---------------------------------------------- Main jump table cineops.c */

/* cineops.c is meant to be included directly into cinecpu.c, or turned
 * into a master dispatcher macro, or some other horrible thing. cineops.c
 * is the RCstate dispatcher, which also causes state changes. This design
 * keeps things running very fast, since no costly flag calculations
 * need to be performed. Thank Zonn for this twisted but effective
 * idea.
 */

/* table for RCstate "A" -- Use this table if the last opcode was not
 * an ACC related opcode, and was not a B flip/flop operation.
 * Translation:
 *   Any ACC related routine will use A-reg and go on to opCodeTblAA
 *   Any B flip/flop instructions will jump to opCodeTblB
 *   All other instructions remain in opCodeTblA
 *   JMI will use the current sign of the A-reg
 */
 opCodeTblA:

   switch (rom[RCregister_PC]) {
   case 0:
      goto opLDAimm_A_AA;
   case 1:
      goto opLDAimm_A_AA;
   case 2:
      goto opLDAimm_A_AA;
   case 3:
      goto opLDAimm_A_AA;
   case 4:
      goto opLDAimm_A_AA;
   case 5:
      goto opLDAimm_A_AA;
   case 6:
      goto opLDAimm_A_AA;
   case 7:
      goto opLDAimm_A_AA;
   case 8:
      goto opLDAimm_A_AA;
   case 9:
      goto opLDAimm_A_AA;
   case 10:
      goto opLDAimm_A_AA;
   case 11:
      goto opLDAimm_A_AA;
   case 12:
      goto opLDAimm_A_AA;
   case 13:
      goto opLDAimm_A_AA;
   case 14:
      goto opLDAimm_A_AA;
   case 15:
      goto opLDAimm_A_AA;
   case 16:
      goto opINP_A_AA;
   case 17:
      goto opINP_A_AA;
   case 18:
      goto opINP_A_AA;
   case 19:
      goto opINP_A_AA;
   case 20:
      goto opINP_A_AA;
   case 21:
      goto opINP_A_AA;
   case 22:
      goto opINP_A_AA;
   case 23:
      goto opINP_A_AA;
   case 24:
      goto opINP_A_AA;
   case 25:
      goto opINP_A_AA;
   case 26:
      goto opINP_A_AA;
   case 27:
      goto opINP_A_AA;
   case 28:
      goto opINP_A_AA;
   case 29:
      goto opINP_A_AA;
   case 30:
      goto opINP_A_AA;
   case 31:
      goto opINP_A_AA;
   case 32:
      goto opADDimmX_A_AA;
   case 33:
      goto opADDimm_A_AA;
   case 34:
      goto opADDimm_A_AA;
   case 35:
      goto opADDimm_A_AA;
   case 36:
      goto opADDimm_A_AA;
   case 37:
      goto opADDimm_A_AA;
   case 38:
      goto opADDimm_A_AA;
   case 39:
      goto opADDimm_A_AA;
   case 40:
      goto opADDimm_A_AA;
   case 41:
      goto opADDimm_A_AA;
   case 42:
      goto opADDimm_A_AA;
   case 43:
      goto opADDimm_A_AA;
   case 44:
      goto opADDimm_A_AA;
   case 45:
      goto opADDimm_A_AA;
   case 46:
      goto opADDimm_A_AA;
   case 47:
      goto opADDimm_A_AA;
   case 48:
      goto opSUBimmX_A_AA;
   case 49:
      goto opSUBimm_A_AA;
   case 50:
      goto opSUBimm_A_AA;
   case 51:
      goto opSUBimm_A_AA;
   case 52:
      goto opSUBimm_A_AA;
   case 53:
      goto opSUBimm_A_AA;
   case 54:
      goto opSUBimm_A_AA;
   case 55:
      goto opSUBimm_A_AA;
   case 56:
      goto opSUBimm_A_AA;
   case 57:
      goto opSUBimm_A_AA;
   case 58:
      goto opSUBimm_A_AA;
   case 59:
      goto opSUBimm_A_AA;
   case 60:
      goto opSUBimm_A_AA;
   case 61:
      goto opSUBimm_A_AA;
   case 62:
      goto opSUBimm_A_AA;
   case 63:
      goto opSUBimm_A_AA;
   case 64:
      goto opLDJimm_A_A;
   case 65:
      goto opLDJimm_A_A;
   case 66:
      goto opLDJimm_A_A;
   case 67:
      goto opLDJimm_A_A;
   case 68:
      goto opLDJimm_A_A;
   case 69:
      goto opLDJimm_A_A;
   case 70:
      goto opLDJimm_A_A;
   case 71:
      goto opLDJimm_A_A;
   case 72:
      goto opLDJimm_A_A;
   case 73:
      goto opLDJimm_A_A;
   case 74:
      goto opLDJimm_A_A;
   case 75:
      goto opLDJimm_A_A;
   case 76:
      goto opLDJimm_A_A;
   case 77:
      goto opLDJimm_A_A;
   case 78:
      goto opLDJimm_A_A;
   case 79:
      goto opLDJimm_A_A;
   case 80:
      goto tJPP_A_B;		/* redirector */
   case 81:
      goto tJMI_A_B;		/* redirector */
   case 82:
      goto opJDR_A_B;
   case 83:
      goto opJLT_A_B;
   case 84:
      goto opJEQ_A_B;
   case 85:
      goto opJNC_A_B;
   case 86:
      goto opJA0_A_B;
   case 87:
      goto opNOP_A_B;
   case 88:
      goto opJMP_A_A;
   case 89:
      goto tJMI_A_A;		/* redirector */
   case 90:
      goto opJDR_A_A;
   case 91:
      goto opJLT_A_A;
   case 92:
      goto opJEQ_A_A;
   case 93:
      goto opJNC_A_A;
   case 94:
      goto opJA0_A_A;
   case 95:
      goto opNOP_A_A;
   case 96:
      goto opADDdir_A_AA;
   case 97:
      goto opADDdir_A_AA;
   case 98:
      goto opADDdir_A_AA;
   case 99:
      goto opADDdir_A_AA;
   case 100:
      goto opADDdir_A_AA;
   case 101:
      goto opADDdir_A_AA;
   case 102:
      goto opADDdir_A_AA;
   case 103:
      goto opADDdir_A_AA;
   case 104:
      goto opADDdir_A_AA;
   case 105:
      goto opADDdir_A_AA;
   case 106:
      goto opADDdir_A_AA;
   case 107:
      goto opADDdir_A_AA;
   case 108:
      goto opADDdir_A_AA;
   case 109:
      goto opADDdir_A_AA;
   case 110:
      goto opADDdir_A_AA;
   case 111:
      goto opADDdir_A_AA;
   case 112:
      goto opSUBdir_A_AA;
   case 113:
      goto opSUBdir_A_AA;
   case 114:
      goto opSUBdir_A_AA;
   case 115:
      goto opSUBdir_A_AA;
   case 116:
      goto opSUBdir_A_AA;
   case 117:
      goto opSUBdir_A_AA;
   case 118:
      goto opSUBdir_A_AA;
   case 119:
      goto opSUBdir_A_AA;
   case 120:
      goto opSUBdir_A_AA;
   case 121:
      goto opSUBdir_A_AA;
   case 122:
      goto opSUBdir_A_AA;
   case 123:
      goto opSUBdir_A_AA;
   case 124:
      goto opSUBdir_A_AA;
   case 125:
      goto opSUBdir_A_AA;
   case 126:
      goto opSUBdir_A_AA;
   case 127:
      goto opSUBdir_A_AA;
   case 128:
      goto opLDPimm_A_A;
   case 129:
      goto opLDPimm_A_A;
   case 130:
      goto opLDPimm_A_A;
   case 131:
      goto opLDPimm_A_A;
   case 132:
      goto opLDPimm_A_A;
   case 133:
      goto opLDPimm_A_A;
   case 134:
      goto opLDPimm_A_A;
   case 135:
      goto opLDPimm_A_A;
   case 136:
      goto opLDPimm_A_A;
   case 137:
      goto opLDPimm_A_A;
   case 138:
      goto opLDPimm_A_A;
   case 139:
      goto opLDPimm_A_A;
   case 140:
      goto opLDPimm_A_A;
   case 141:
      goto opLDPimm_A_A;
   case 142:
      goto opLDPimm_A_A;
   case 143:
      goto opLDPimm_A_A;
   case 144:
      goto tOUT_A_A;		/* redirector */
   case 145:
      goto tOUT_A_A;		/* redirector */
   case 146:
      goto tOUT_A_A;		/* redirector */
   case 147:
      goto tOUT_A_A;		/* redirector */
   case 148:
      goto tOUT_A_A;		/* redirector */
   case 149:
      goto tOUT_A_A;		/* redirector */
   case 150:
      goto tOUT_A_A;		/* redirector */
   case 151:
      goto tOUT_A_A;		/* redirector */
   case 152:
      goto tOUT_A_A;		/* redirector */
   case 153:
      goto tOUT_A_A;		/* redirector */
   case 154:
      goto tOUT_A_A;		/* redirector */
   case 155:
      goto tOUT_A_A;		/* redirector */
   case 156:
      goto tOUT_A_A;		/* redirector */
   case 157:
      goto tOUT_A_A;		/* redirector */
   case 158:
      goto tOUT_A_A;		/* redirector */
   case 159:
      goto tOUT_A_A;		/* redirector */
   case 160:
      goto opLDAdir_A_AA;
   case 161:
      goto opLDAdir_A_AA;
   case 162:
      goto opLDAdir_A_AA;
   case 163:
      goto opLDAdir_A_AA;
   case 164:
      goto opLDAdir_A_AA;
   case 165:
      goto opLDAdir_A_AA;
   case 166:
      goto opLDAdir_A_AA;
   case 167:
      goto opLDAdir_A_AA;
   case 168:
      goto opLDAdir_A_AA;
   case 169:
      goto opLDAdir_A_AA;
   case 170:
      goto opLDAdir_A_AA;
   case 171:
      goto opLDAdir_A_AA;
   case 172:
      goto opLDAdir_A_AA;
   case 173:
      goto opLDAdir_A_AA;
   case 174:
      goto opLDAdir_A_AA;
   case 175:
      goto opLDAdir_A_AA;
   case 176:
      goto opCMPdir_A_AA;
   case 177:
      goto opCMPdir_A_AA;
   case 178:
      goto opCMPdir_A_AA;
   case 179:
      goto opCMPdir_A_AA;
   case 180:
      goto opCMPdir_A_AA;
   case 181:
      goto opCMPdir_A_AA;
   case 182:
      goto opCMPdir_A_AA;
   case 183:
      goto opCMPdir_A_AA;
   case 184:
      goto opCMPdir_A_AA;
   case 185:
      goto opCMPdir_A_AA;
   case 186:
      goto opCMPdir_A_AA;
   case 187:
      goto opCMPdir_A_AA;
   case 188:
      goto opCMPdir_A_AA;
   case 189:
      goto opCMPdir_A_AA;
   case 190:
      goto opCMPdir_A_AA;
   case 191:
      goto opCMPdir_A_AA;
   case 192:
      goto opLDIdir_A_A;
   case 193:
      goto opLDIdir_A_A;
   case 194:
      goto opLDIdir_A_A;
   case 195:
      goto opLDIdir_A_A;
   case 196:
      goto opLDIdir_A_A;
   case 197:
      goto opLDIdir_A_A;
   case 198:
      goto opLDIdir_A_A;
   case 199:
      goto opLDIdir_A_A;
   case 200:
      goto opLDIdir_A_A;
   case 201:
      goto opLDIdir_A_A;
   case 202:
      goto opLDIdir_A_A;
   case 203:
      goto opLDIdir_A_A;
   case 204:
      goto opLDIdir_A_A;
   case 205:
      goto opLDIdir_A_A;
   case 206:
      goto opLDIdir_A_A;
   case 207:
      goto opLDIdir_A_A;
   case 208:
      goto opSTAdir_A_A;
   case 209:
      goto opSTAdir_A_A;
   case 210:
      goto opSTAdir_A_A;
   case 211:
      goto opSTAdir_A_A;
   case 212:
      goto opSTAdir_A_A;
   case 213:
      goto opSTAdir_A_A;
   case 214:
      goto opSTAdir_A_A;
   case 215:
      goto opSTAdir_A_A;
   case 216:
      goto opSTAdir_A_A;
   case 217:
      goto opSTAdir_A_A;
   case 218:
      goto opSTAdir_A_A;
   case 219:
      goto opSTAdir_A_A;
   case 220:
      goto opSTAdir_A_A;
   case 221:
      goto opSTAdir_A_A;
   case 222:
      goto opSTAdir_A_A;
   case 223:
      goto opSTAdir_A_A;
   case 224:
      goto opVDR_A_A;
   case 225:
      goto opLDJirg_A_A;
   case 226:
      goto opXLT_A_AA;
   case 227:
      goto opMULirg_A_AA;
   case 228:
      goto opLLT_A_AA;
   case 229:
      goto opWAI_A_A;
   case 230:
      goto opSTAirg_A_A;
   case 231:
      goto opADDirg_A_AA;
   case 232:
      goto opSUBirg_A_AA;
   case 233:
      goto opANDirg_A_AA;
   case 234:
      goto opLDAirg_A_AA;
   case 235:
      goto opLSRe_A_AA;
   case 236:
      goto opLSLe_A_AA;
   case 237:
      goto opASRe_A_AA;
   case 238:
      goto opASRDe_A_AA;
   case 239:
      goto opLSLDe_A_AA;
   case 240:
      goto opVIN_A_A;
   case 241:
      goto opLDJirg_A_A;
   case 242:
      goto opXLT_A_AA;
   case 243:
      goto opMULirg_A_AA;
   case 244:
      goto opLLT_A_AA;
   case 245:
      goto opWAI_A_A;
   case 246:
      goto opSTAirg_A_A;
   case 247:
      goto opAWDirg_A_AA;
   case 248:
      goto opSUBirg_A_AA;
   case 249:
      goto opANDirg_A_AA;
   case 250:
      goto opLDAirg_A_AA;
   case 251:
      goto opLSRf_A_AA;
   case 252:
      goto opLSLf_A_AA;
   case 253:
      goto opASRf_A_AA;
   case 254:
      goto opASRDf_A_AA;
   case 255:
      goto opLSLDf_A_AA;
   } /* switch on opcode */

/* opcode table AA -- Use this table if the last opcode was an ACC
 * related opcode. Translation:
 *   Any ACC related routine will use A-reg and remain in OpCodeTblAA
 *   Any B flip/flop instructions will jump to opCodeTblB
 *   All other instructions will jump to opCodeTblA
 *   JMI will use the sign of acc_old
 */

 opCodeTblAA:

   switch (rom[RCregister_PC]) {
   case 0:
      goto opLDAimm_AA_AA;
   case 1:
      goto opLDAimm_AA_AA;
   case 2:
      goto opLDAimm_AA_AA;
   case 3:
      goto opLDAimm_AA_AA;
   case 4:
      goto opLDAimm_AA_AA;
   case 5:
      goto opLDAimm_AA_AA;
   case 6:
      goto opLDAimm_AA_AA;
   case 7:
      goto opLDAimm_AA_AA;
   case 8:
      goto opLDAimm_AA_AA;
   case 9:
      goto opLDAimm_AA_AA;
   case 10:
      goto opLDAimm_AA_AA;
   case 11:
      goto opLDAimm_AA_AA;
   case 12:
      goto opLDAimm_AA_AA;
   case 13:
      goto opLDAimm_AA_AA;
   case 14:
      goto opLDAimm_AA_AA;
   case 15:
      goto opLDAimm_AA_AA;
   case 16:
      goto opINP_AA_AA;
   case 17:
      goto opINP_AA_AA;
   case 18:
      goto opINP_AA_AA;
   case 19:
      goto opINP_AA_AA;
   case 20:
      goto opINP_AA_AA;
   case 21:
      goto opINP_AA_AA;
   case 22:
      goto opINP_AA_AA;
   case 23:
      goto opINP_AA_AA;
   case 24:
      goto opINP_AA_AA;
   case 25:
      goto opINP_AA_AA;
   case 26:
      goto opINP_AA_AA;
   case 27:
      goto opINP_AA_AA;
   case 28:
      goto opINP_AA_AA;
   case 29:
      goto opINP_AA_AA;
   case 30:
      goto opINP_AA_AA;
   case 31:
      goto opINP_AA_AA;
   case 32:
      goto opADDimmX_AA_AA;
   case 33:
      goto opADDimm_AA_AA;
   case 34:
      goto opADDimm_AA_AA;
   case 35:
      goto opADDimm_AA_AA;
   case 36:
      goto opADDimm_AA_AA;
   case 37:
      goto opADDimm_AA_AA;
   case 38:
      goto opADDimm_AA_AA;
   case 39:
      goto opADDimm_AA_AA;
   case 40:
      goto opADDimm_AA_AA;
   case 41:
      goto opADDimm_AA_AA;
   case 42:
      goto opADDimm_AA_AA;
   case 43:
      goto opADDimm_AA_AA;
   case 44:
      goto opADDimm_AA_AA;
   case 45:
      goto opADDimm_AA_AA;
   case 46:
      goto opADDimm_AA_AA;
   case 47:
      goto opADDimm_AA_AA;
   case 48:
      goto opSUBimmX_AA_AA;
   case 49:
      goto opSUBimm_AA_AA;
   case 50:
      goto opSUBimm_AA_AA;
   case 51:
      goto opSUBimm_AA_AA;
   case 52:
      goto opSUBimm_AA_AA;
   case 53:
      goto opSUBimm_AA_AA;
   case 54:
      goto opSUBimm_AA_AA;
   case 55:
      goto opSUBimm_AA_AA;
   case 56:
      goto opSUBimm_AA_AA;
   case 57:
      goto opSUBimm_AA_AA;
   case 58:
      goto opSUBimm_AA_AA;
   case 59:
      goto opSUBimm_AA_AA;
   case 60:
      goto opSUBimm_AA_AA;
   case 61:
      goto opSUBimm_AA_AA;
   case 62:
      goto opSUBimm_AA_AA;
   case 63:
      goto opSUBimm_AA_AA;
   case 64:
      goto opLDJimm_AA_A;
   case 65:
      goto opLDJimm_AA_A;
   case 66:
      goto opLDJimm_AA_A;
   case 67:
      goto opLDJimm_AA_A;
   case 68:
      goto opLDJimm_AA_A;
   case 69:
      goto opLDJimm_AA_A;
   case 70:
      goto opLDJimm_AA_A;
   case 71:
      goto opLDJimm_AA_A;
   case 72:
      goto opLDJimm_AA_A;
   case 73:
      goto opLDJimm_AA_A;
   case 74:
      goto opLDJimm_AA_A;
   case 75:
      goto opLDJimm_AA_A;
   case 76:
      goto opLDJimm_AA_A;
   case 77:
      goto opLDJimm_AA_A;
   case 78:
      goto opLDJimm_AA_A;
   case 79:
      goto opLDJimm_AA_A;
   case 80:
      goto tJPP_AA_B;		/* redirector */
   case 81:
      goto tJMI_AA_B;		/* redirector */
   case 82:
      goto opJDR_AA_B;
   case 83:
      goto opJLT_AA_B;
   case 84:
      goto opJEQ_AA_B;
   case 85:
      goto opJNC_AA_B;
   case 86:
      goto opJA0_AA_B;
   case 87:
      goto opNOP_AA_B;
   case 88:
      goto opJMP_AA_A;
   case 89:
      goto tJMI_AA_A;		/* redirector */
   case 90:
      goto opJDR_AA_A;
   case 91:
      goto opJLT_AA_A;
   case 92:
      goto opJEQ_AA_A;
   case 93:
      goto opJNC_AA_A;
   case 94:
      goto opJA0_AA_A;
   case 95:
      goto opNOP_AA_A;
   case 96:
      goto opADDdir_AA_AA;
   case 97:
      goto opADDdir_AA_AA;
   case 98:
      goto opADDdir_AA_AA;
   case 99:
      goto opADDdir_AA_AA;
   case 100:
      goto opADDdir_AA_AA;
   case 101:
      goto opADDdir_AA_AA;
   case 102:
      goto opADDdir_AA_AA;
   case 103:
      goto opADDdir_AA_AA;
   case 104:
      goto opADDdir_AA_AA;
   case 105:
      goto opADDdir_AA_AA;
   case 106:
      goto opADDdir_AA_AA;
   case 107:
      goto opADDdir_AA_AA;
   case 108:
      goto opADDdir_AA_AA;
   case 109:
      goto opADDdir_AA_AA;
   case 110:
      goto opADDdir_AA_AA;
   case 111:
      goto opADDdir_AA_AA;
   case 112:
      goto opSUBdir_AA_AA;
   case 113:
      goto opSUBdir_AA_AA;
   case 114:
      goto opSUBdir_AA_AA;
   case 115:
      goto opSUBdir_AA_AA;
   case 116:
      goto opSUBdir_AA_AA;
   case 117:
      goto opSUBdir_AA_AA;
   case 118:
      goto opSUBdir_AA_AA;
   case 119:
      goto opSUBdir_AA_AA;
   case 120:
      goto opSUBdir_AA_AA;
   case 121:
      goto opSUBdir_AA_AA;
   case 122:
      goto opSUBdir_AA_AA;
   case 123:
      goto opSUBdir_AA_AA;
   case 124:
      goto opSUBdir_AA_AA;
   case 125:
      goto opSUBdir_AA_AA;
   case 126:
      goto opSUBdir_AA_AA;
   case 127:
      goto opSUBdir_AA_AA;
   case 128:
      goto opLDPimm_AA_A;
   case 129:
      goto opLDPimm_AA_A;
   case 130:
      goto opLDPimm_AA_A;
   case 131:
      goto opLDPimm_AA_A;
   case 132:
      goto opLDPimm_AA_A;
   case 133:
      goto opLDPimm_AA_A;
   case 134:
      goto opLDPimm_AA_A;
   case 135:
      goto opLDPimm_AA_A;
   case 136:
      goto opLDPimm_AA_A;
   case 137:
      goto opLDPimm_AA_A;
   case 138:
      goto opLDPimm_AA_A;
   case 139:
      goto opLDPimm_AA_A;
   case 140:
      goto opLDPimm_AA_A;
   case 141:
      goto opLDPimm_AA_A;
   case 142:
      goto opLDPimm_AA_A;
   case 143:
      goto opLDPimm_AA_A;
   case 144:
      goto tOUT_AA_A;		/* redirector */
   case 145:
      goto tOUT_AA_A;		/* redirector */
   case 146:
      goto tOUT_AA_A;		/* redirector */
   case 147:
      goto tOUT_AA_A;		/* redirector */
   case 148:
      goto tOUT_AA_A;		/* redirector */
   case 149:
      goto tOUT_AA_A;		/* redirector */
   case 150:
      goto tOUT_AA_A;		/* redirector */
   case 151:
      goto tOUT_AA_A;		/* redirector */
   case 152:
      goto tOUT_AA_A;		/* redirector */
   case 153:
      goto tOUT_AA_A;		/* redirector */
   case 154:
      goto tOUT_AA_A;		/* redirector */
   case 155:
      goto tOUT_AA_A;		/* redirector */
   case 156:
      goto tOUT_AA_A;		/* redirector */
   case 157:
      goto tOUT_AA_A;		/* redirector */
   case 158:
      goto tOUT_AA_A;		/* redirector */
   case 159:
      goto tOUT_AA_A;		/* redirector */
   case 160:
      goto opLDAdir_AA_AA;
   case 161:
      goto opLDAdir_AA_AA;
   case 162:
      goto opLDAdir_AA_AA;
   case 163:
      goto opLDAdir_AA_AA;
   case 164:
      goto opLDAdir_AA_AA;
   case 165:
      goto opLDAdir_AA_AA;
   case 166:
      goto opLDAdir_AA_AA;
   case 167:
      goto opLDAdir_AA_AA;
   case 168:
      goto opLDAdir_AA_AA;
   case 169:
      goto opLDAdir_AA_AA;
   case 170:
      goto opLDAdir_AA_AA;
   case 171:
      goto opLDAdir_AA_AA;
   case 172:
      goto opLDAdir_AA_AA;
   case 173:
      goto opLDAdir_AA_AA;
   case 174:
      goto opLDAdir_AA_AA;
   case 175:
      goto opLDAdir_AA_AA;
   case 176:
      goto opCMPdir_AA_AA;
   case 177:
      goto opCMPdir_AA_AA;
   case 178:
      goto opCMPdir_AA_AA;
   case 179:
      goto opCMPdir_AA_AA;
   case 180:
      goto opCMPdir_AA_AA;
   case 181:
      goto opCMPdir_AA_AA;
   case 182:
      goto opCMPdir_AA_AA;
   case 183:
      goto opCMPdir_AA_AA;
   case 184:
      goto opCMPdir_AA_AA;
   case 185:
      goto opCMPdir_AA_AA;
   case 186:
      goto opCMPdir_AA_AA;
   case 187:
      goto opCMPdir_AA_AA;
   case 188:
      goto opCMPdir_AA_AA;
   case 189:
      goto opCMPdir_AA_AA;
   case 190:
      goto opCMPdir_AA_AA;
   case 191:
      goto opCMPdir_AA_AA;
   case 192:
      goto opLDIdir_AA_A;
   case 193:
      goto opLDIdir_AA_A;
   case 194:
      goto opLDIdir_AA_A;
   case 195:
      goto opLDIdir_AA_A;
   case 196:
      goto opLDIdir_AA_A;
   case 197:
      goto opLDIdir_AA_A;
   case 198:
      goto opLDIdir_AA_A;
   case 199:
      goto opLDIdir_AA_A;
   case 200:
      goto opLDIdir_AA_A;
   case 201:
      goto opLDIdir_AA_A;
   case 202:
      goto opLDIdir_AA_A;
   case 203:
      goto opLDIdir_AA_A;
   case 204:
      goto opLDIdir_AA_A;
   case 205:
      goto opLDIdir_AA_A;
   case 206:
      goto opLDIdir_AA_A;
   case 207:
      goto opLDIdir_AA_A;
   case 208:
      goto opSTAdir_AA_A;
   case 209:
      goto opSTAdir_AA_A;
   case 210:
      goto opSTAdir_AA_A;
   case 211:
      goto opSTAdir_AA_A;
   case 212:
      goto opSTAdir_AA_A;
   case 213:
      goto opSTAdir_AA_A;
   case 214:
      goto opSTAdir_AA_A;
   case 215:
      goto opSTAdir_AA_A;
   case 216:
      goto opSTAdir_AA_A;
   case 217:
      goto opSTAdir_AA_A;
   case 218:
      goto opSTAdir_AA_A;
   case 219:
      goto opSTAdir_AA_A;
   case 220:
      goto opSTAdir_AA_A;
   case 221:
      goto opSTAdir_AA_A;
   case 222:
      goto opSTAdir_AA_A;
   case 223:
      goto opSTAdir_AA_A;
   case 224:
      goto opVDR_AA_A;
   case 225:
      goto opLDJirg_AA_A;
   case 226:
      goto opXLT_AA_AA;
   case 227:
      goto opMULirg_AA_AA;
   case 228:
      goto opLLT_AA_AA;
   case 229:
      goto opWAI_AA_A;
   case 230:
      goto opSTAirg_AA_A;
   case 231:
      goto opADDirg_AA_AA;
   case 232:
      goto opSUBirg_AA_AA;
   case 233:
      goto opANDirg_AA_AA;
   case 234:
      goto opLDAirg_AA_AA;
   case 235:
      goto opLSRe_AA_AA;
   case 236:
      goto opLSLe_AA_AA;
   case 237:
      goto opASRe_AA_AA;
   case 238:
      goto opASRDe_AA_AA;
   case 239:
      goto opLSLDe_AA_AA;
   case 240:
      goto opVIN_AA_A;
   case 241:
      goto opLDJirg_AA_A;
   case 242:
      goto opXLT_AA_AA;
   case 243:
      goto opMULirg_AA_AA;
   case 244:
      goto opLLT_AA_AA;
   case 245:
      goto opWAI_AA_A;
   case 246:
      goto opSTAirg_AA_A;
   case 247:
      goto opAWDirg_AA_AA;
   case 248:
      goto opSUBirg_AA_AA;
   case 249:
      goto opANDirg_AA_AA;
   case 250:
      goto opLDAirg_AA_AA;
   case 251:
      goto opLSRf_AA_AA;
   case 252:
      goto opLSLf_AA_AA;
   case 253:
      goto opASRf_AA_AA;
   case 254:
      goto opASRDf_AA_AA;
   case 255:
      goto opLSLDf_AA_AA;
   } /* switch on opcode */

/* opcode table B -- use this table if the last opcode was a B-reg flip/flop
 * Translation:
 *   Any ACC related routine uses B-reg, and goes to opCodeTblAA
 *   All other instructions will jump to table opCodeTblBB (including
 *     B flip/flop related instructions)
 *   JMI will use current sign of the A-reg
 */

 opCodeTblB:

   switch (rom[RCregister_PC]) {
   case 0:
      goto opLDAimm_B_AA;
   case 1:
      goto opLDAimm_B_AA;
   case 2:
      goto opLDAimm_B_AA;
   case 3:
      goto opLDAimm_B_AA;
   case 4:
      goto opLDAimm_B_AA;
   case 5:
      goto opLDAimm_B_AA;
   case 6:
      goto opLDAimm_B_AA;
   case 7:
      goto opLDAimm_B_AA;
   case 8:
      goto opLDAimm_B_AA;
   case 9:
      goto opLDAimm_B_AA;
   case 10:
      goto opLDAimm_B_AA;
   case 11:
      goto opLDAimm_B_AA;
   case 12:
      goto opLDAimm_B_AA;
   case 13:
      goto opLDAimm_B_AA;
   case 14:
      goto opLDAimm_B_AA;
   case 15:
      goto opLDAimm_B_AA;
   case 16:
      goto opINP_B_AA;
   case 17:
      goto opINP_B_AA;
   case 18:
      goto opINP_B_AA;
   case 19:
      goto opINP_B_AA;
   case 20:
      goto opINP_B_AA;
   case 21:
      goto opINP_B_AA;
   case 22:
      goto opINP_B_AA;
   case 23:
      goto opINP_B_AA;
   case 24:
      goto opINP_B_AA;
   case 25:
      goto opINP_B_AA;
   case 26:
      goto opINP_B_AA;
   case 27:
      goto opINP_B_AA;
   case 28:
      goto opINP_B_AA;
   case 29:
      goto opINP_B_AA;
   case 30:
      goto opINP_B_AA;
   case 31:
      goto opINP_B_AA;
   case 32:
      goto opADDimmX_B_AA;
   case 33:
      goto opADDimm_B_AA;
   case 34:
      goto opADDimm_B_AA;
   case 35:
      goto opADDimm_B_AA;
   case 36:
      goto opADDimm_B_AA;
   case 37:
      goto opADDimm_B_AA;
   case 38:
      goto opADDimm_B_AA;
   case 39:
      goto opADDimm_B_AA;
   case 40:
      goto opADDimm_B_AA;
   case 41:
      goto opADDimm_B_AA;
   case 42:
      goto opADDimm_B_AA;
   case 43:
      goto opADDimm_B_AA;
   case 44:
      goto opADDimm_B_AA;
   case 45:
      goto opADDimm_B_AA;
   case 46:
      goto opADDimm_B_AA;
   case 47:
      goto opADDimm_B_AA;
   case 48:
      goto opSUBimmX_B_AA;
   case 49:
      goto opSUBimm_B_AA;
   case 50:
      goto opSUBimm_B_AA;
   case 51:
      goto opSUBimm_B_AA;
   case 52:
      goto opSUBimm_B_AA;
   case 53:
      goto opSUBimm_B_AA;
   case 54:
      goto opSUBimm_B_AA;
   case 55:
      goto opSUBimm_B_AA;
   case 56:
      goto opSUBimm_B_AA;
   case 57:
      goto opSUBimm_B_AA;
   case 58:
      goto opSUBimm_B_AA;
   case 59:
      goto opSUBimm_B_AA;
   case 60:
      goto opSUBimm_B_AA;
   case 61:
      goto opSUBimm_B_AA;
   case 62:
      goto opSUBimm_B_AA;
   case 63:
      goto opSUBimm_B_AA;
   case 64:
      goto opLDJimm_B_BB;
   case 65:
      goto opLDJimm_B_BB;
   case 66:
      goto opLDJimm_B_BB;
   case 67:
      goto opLDJimm_B_BB;
   case 68:
      goto opLDJimm_B_BB;
   case 69:
      goto opLDJimm_B_BB;
   case 70:
      goto opLDJimm_B_BB;
   case 71:
      goto opLDJimm_B_BB;
   case 72:
      goto opLDJimm_B_BB;
   case 73:
      goto opLDJimm_B_BB;
   case 74:
      goto opLDJimm_B_BB;
   case 75:
      goto opLDJimm_B_BB;
   case 76:
      goto opLDJimm_B_BB;
   case 77:
      goto opLDJimm_B_BB;
   case 78:
      goto opLDJimm_B_BB;
   case 79:
      goto opLDJimm_B_BB;
   case 80:
      goto tJPP_B_BB;		/* redirector */
   case 81:
      goto tJMI_B_BB1;		/* redirector */
   case 82:
      goto opJDR_B_BB;
   case 83:
      goto opJLT_B_BB;
   case 84:
      goto opJEQ_B_BB;
   case 85:
      goto opJNC_B_BB;
   case 86:
      goto opJA0_B_BB;
   case 87:
      goto opNOP_B_BB;
   case 88:
      goto opJMP_B_BB;
   case 89:
      goto tJMI_B_BB2;		/* redirector */
   case 90:
      goto opJDR_B_BB;
   case 91:
      goto opJLT_B_BB;
   case 92:
      goto opJEQ_B_BB;
   case 93:
      goto opJNC_B_BB;
   case 94:
      goto opJA0_B_BB;
   case 95:
      goto opNOP_B_BB;
   case 96:
      goto opADDdir_B_AA;
   case 97:
      goto opADDdir_B_AA;
   case 98:
      goto opADDdir_B_AA;
   case 99:
      goto opADDdir_B_AA;
   case 100:
      goto opADDdir_B_AA;
   case 101:
      goto opADDdir_B_AA;
   case 102:
      goto opADDdir_B_AA;
   case 103:
      goto opADDdir_B_AA;
   case 104:
      goto opADDdir_B_AA;
   case 105:
      goto opADDdir_B_AA;
   case 106:
      goto opADDdir_B_AA;
   case 107:
      goto opADDdir_B_AA;
   case 108:
      goto opADDdir_B_AA;
   case 109:
      goto opADDdir_B_AA;
   case 110:
      goto opADDdir_B_AA;
   case 111:
      goto opADDdir_B_AA;
   case 112:
      goto opSUBdir_B_AA;
   case 113:
      goto opSUBdir_B_AA;
   case 114:
      goto opSUBdir_B_AA;
   case 115:
      goto opSUBdir_B_AA;
   case 116:
      goto opSUBdir_B_AA;
   case 117:
      goto opSUBdir_B_AA;
   case 118:
      goto opSUBdir_B_AA;
   case 119:
      goto opSUBdir_B_AA;
   case 120:
      goto opSUBdir_B_AA;
   case 121:
      goto opSUBdir_B_AA;
   case 122:
      goto opSUBdir_B_AA;
   case 123:
      goto opSUBdir_B_AA;
   case 124:
      goto opSUBdir_B_AA;
   case 125:
      goto opSUBdir_B_AA;
   case 126:
      goto opSUBdir_B_AA;
   case 127:
      goto opSUBdir_B_AA;
   case 128:
      goto opLDPimm_B_BB;
   case 129:
      goto opLDPimm_B_BB;
   case 130:
      goto opLDPimm_B_BB;
   case 131:
      goto opLDPimm_B_BB;
   case 132:
      goto opLDPimm_B_BB;
   case 133:
      goto opLDPimm_B_BB;
   case 134:
      goto opLDPimm_B_BB;
   case 135:
      goto opLDPimm_B_BB;
   case 136:
      goto opLDPimm_B_BB;
   case 137:
      goto opLDPimm_B_BB;
   case 138:
      goto opLDPimm_B_BB;
   case 139:
      goto opLDPimm_B_BB;
   case 140:
      goto opLDPimm_B_BB;
   case 141:
      goto opLDPimm_B_BB;
   case 142:
      goto opLDPimm_B_BB;
   case 143:
      goto opLDPimm_B_BB;
   case 144:
      goto tOUT_B_BB;		/* redirector */
   case 145:
      goto tOUT_B_BB;		/* redirector */
   case 146:
      goto tOUT_B_BB;		/* redirector */
   case 147:
      goto tOUT_B_BB;		/* redirector */
   case 148:
      goto tOUT_B_BB;		/* redirector */
   case 149:
      goto tOUT_B_BB;		/* redirector */
   case 150:
      goto tOUT_B_BB;		/* redirector */
   case 151:
      goto tOUT_B_BB;		/* redirector */
   case 152:
      goto tOUT_B_BB;		/* redirector */
   case 153:
      goto tOUT_B_BB;		/* redirector */
   case 154:
      goto tOUT_B_BB;		/* redirector */
   case 155:
      goto tOUT_B_BB;		/* redirector */
   case 156:
      goto tOUT_B_BB;		/* redirector */
   case 157:
      goto tOUT_B_BB;		/* redirector */
   case 158:
      goto tOUT_B_BB;		/* redirector */
   case 159:
      goto tOUT_B_BB;		/* redirector */
   case 160:
      goto opLDAdir_B_AA;
   case 161:
      goto opLDAdir_B_AA;
   case 162:
      goto opLDAdir_B_AA;
   case 163:
      goto opLDAdir_B_AA;
   case 164:
      goto opLDAdir_B_AA;
   case 165:
      goto opLDAdir_B_AA;
   case 166:
      goto opLDAdir_B_AA;
   case 167:
      goto opLDAdir_B_AA;
   case 168:
      goto opLDAdir_B_AA;
   case 169:
      goto opLDAdir_B_AA;
   case 170:
      goto opLDAdir_B_AA;
   case 171:
      goto opLDAdir_B_AA;
   case 172:
      goto opLDAdir_B_AA;
   case 173:
      goto opLDAdir_B_AA;
   case 174:
      goto opLDAdir_B_AA;
   case 175:
      goto opLDAdir_B_AA;
   case 176:
      goto opCMPdir_B_AA;
   case 177:
      goto opCMPdir_B_AA;
   case 178:
      goto opCMPdir_B_AA;
   case 179:
      goto opCMPdir_B_AA;
   case 180:
      goto opCMPdir_B_AA;
   case 181:
      goto opCMPdir_B_AA;
   case 182:
      goto opCMPdir_B_AA;
   case 183:
      goto opCMPdir_B_AA;
   case 184:
      goto opCMPdir_B_AA;
   case 185:
      goto opCMPdir_B_AA;
   case 186:
      goto opCMPdir_B_AA;
   case 187:
      goto opCMPdir_B_AA;
   case 188:
      goto opCMPdir_B_AA;
   case 189:
      goto opCMPdir_B_AA;
   case 190:
      goto opCMPdir_B_AA;
   case 191:
      goto opCMPdir_B_AA;
   case 192:
      goto opLDIdir_B_BB;
   case 193:
      goto opLDIdir_B_BB;
   case 194:
      goto opLDIdir_B_BB;
   case 195:
      goto opLDIdir_B_BB;
   case 196:
      goto opLDIdir_B_BB;
   case 197:
      goto opLDIdir_B_BB;
   case 198:
      goto opLDIdir_B_BB;
   case 199:
      goto opLDIdir_B_BB;
   case 200:
      goto opLDIdir_B_BB;
   case 201:
      goto opLDIdir_B_BB;
   case 202:
      goto opLDIdir_B_BB;
   case 203:
      goto opLDIdir_B_BB;
   case 204:
      goto opLDIdir_B_BB;
   case 205:
      goto opLDIdir_B_BB;
   case 206:
      goto opLDIdir_B_BB;
   case 207:
      goto opLDIdir_B_BB;
   case 208:
      goto opSTAdir_B_BB;
   case 209:
      goto opSTAdir_B_BB;
   case 210:
      goto opSTAdir_B_BB;
   case 211:
      goto opSTAdir_B_BB;
   case 212:
      goto opSTAdir_B_BB;
   case 213:
      goto opSTAdir_B_BB;
   case 214:
      goto opSTAdir_B_BB;
   case 215:
      goto opSTAdir_B_BB;
   case 216:
      goto opSTAdir_B_BB;
   case 217:
      goto opSTAdir_B_BB;
   case 218:
      goto opSTAdir_B_BB;
   case 219:
      goto opSTAdir_B_BB;
   case 220:
      goto opSTAdir_B_BB;
   case 221:
      goto opSTAdir_B_BB;
   case 222:
      goto opSTAdir_B_BB;
   case 223:
      goto opSTAdir_B_BB;
   case 224:
      goto opVDR_B_BB;
   case 225:
      goto opLDJirg_B_BB;
   case 226:
      goto opXLT_B_AA;
   case 227:
      goto opMULirg_B_AA;
   case 228:
      goto opLLT_B_AA;
   case 229:
      goto opWAI_B_BB;
   case 230:
      goto opSTAirg_B_BB;
   case 231:
      goto opADDirg_B_AA;
   case 232:
      goto opSUBirg_B_AA;
   case 233:
      goto opANDirg_B_AA;
   case 234:
      goto opLDAirg_B_AA;
   case 235:
      goto opLSRe_B_AA;
   case 236:
      goto opLSLe_B_AA;
   case 237:
      goto opASRe_B_AA;
   case 238:
      goto opASRDe_B_AA;
   case 239:
      goto opLSLDe_B_AA;
   case 240:
      goto opVIN_B_BB;
   case 241:
      goto opLDJirg_B_BB;
   case 242:
      goto opXLT_B_AA;
   case 243:
      goto opMULirg_B_AA;
   case 244:
      goto opLLT_B_AA;
   case 245:
      goto opWAI_B_BB;
   case 246:
      goto opSTAirg_B_BB;
   case 247:
      goto opAWDirg_B_AA;
   case 248:
      goto opSUBirg_B_AA;
   case 249:
      goto opANDirg_B_AA;
   case 250:
      goto opLDAirg_B_AA;
   case 251:
      goto opLSRf_B_AA;
   case 252:
      goto opLSLf_B_AA;
   case 253:
      goto opASRf_B_AA;
   case 254:
      goto opASRDf_B_AA;
   case 255:
      goto opLSLDf_B_AA;
   } /* switch on opcode */

/* opcode table BB -- use this table if the last opcode was not an ACC
 * related opcode, but instruction before that was a B-flip/flop instruction.
 * Translation:
 *   Any ACC related routine will use A-reg and go to opCodeTblAA
 *   Any B flip/flop instructions will jump to opCodeTblB
 *   All other instructions will jump to table opCodeTblA
 *   JMI will use the current RCstate of the B-reg
 */

 opCodeTblBB:

   switch (rom[RCregister_PC]) {
   case 0:
      goto opLDAimm_BB_AA;
   case 1:
      goto opLDAimm_BB_AA;
   case 2:
      goto opLDAimm_BB_AA;
   case 3:
      goto opLDAimm_BB_AA;
   case 4:
      goto opLDAimm_BB_AA;
   case 5:
      goto opLDAimm_BB_AA;
   case 6:
      goto opLDAimm_BB_AA;
   case 7:
      goto opLDAimm_BB_AA;
   case 8:
      goto opLDAimm_BB_AA;
   case 9:
      goto opLDAimm_BB_AA;
   case 10:
      goto opLDAimm_BB_AA;
   case 11:
      goto opLDAimm_BB_AA;
   case 12:
      goto opLDAimm_BB_AA;
   case 13:
      goto opLDAimm_BB_AA;
   case 14:
      goto opLDAimm_BB_AA;
   case 15:
      goto opLDAimm_BB_AA;
   case 16:
      goto opINP_BB_AA;
   case 17:
      goto opINP_BB_AA;
   case 18:
      goto opINP_BB_AA;
   case 19:
      goto opINP_BB_AA;
   case 20:
      goto opINP_BB_AA;
   case 21:
      goto opINP_BB_AA;
   case 22:
      goto opINP_BB_AA;
   case 23:
      goto opINP_BB_AA;
   case 24:
      goto opINP_BB_AA;
   case 25:
      goto opINP_BB_AA;
   case 26:
      goto opINP_BB_AA;
   case 27:
      goto opINP_BB_AA;
   case 28:
      goto opINP_BB_AA;
   case 29:
      goto opINP_BB_AA;
   case 30:
      goto opINP_BB_AA;
   case 31:
      goto opINP_BB_AA;
   case 32:
      goto opADDimmX_BB_AA;
   case 33:
      goto opADDimm_BB_AA;
   case 34:
      goto opADDimm_BB_AA;
   case 35:
      goto opADDimm_BB_AA;
   case 36:
      goto opADDimm_BB_AA;
   case 37:
      goto opADDimm_BB_AA;
   case 38:
      goto opADDimm_BB_AA;
   case 39:
      goto opADDimm_BB_AA;
   case 40:
      goto opADDimm_BB_AA;
   case 41:
      goto opADDimm_BB_AA;
   case 42:
      goto opADDimm_BB_AA;
   case 43:
      goto opADDimm_BB_AA;
   case 44:
      goto opADDimm_BB_AA;
   case 45:
      goto opADDimm_BB_AA;
   case 46:
      goto opADDimm_BB_AA;
   case 47:
      goto opADDimm_BB_AA;
   case 48:
      goto opSUBimmX_BB_AA;
   case 49:
      goto opSUBimm_BB_AA;
   case 50:
      goto opSUBimm_BB_AA;
   case 51:
      goto opSUBimm_BB_AA;
   case 52:
      goto opSUBimm_BB_AA;
   case 53:
      goto opSUBimm_BB_AA;
   case 54:
      goto opSUBimm_BB_AA;
   case 55:
      goto opSUBimm_BB_AA;
   case 56:
      goto opSUBimm_BB_AA;
   case 57:
      goto opSUBimm_BB_AA;
   case 58:
      goto opSUBimm_BB_AA;
   case 59:
      goto opSUBimm_BB_AA;
   case 60:
      goto opSUBimm_BB_AA;
   case 61:
      goto opSUBimm_BB_AA;
   case 62:
      goto opSUBimm_BB_AA;
   case 63:
      goto opSUBimm_BB_AA;
   case 64:
      goto opLDJimm_BB_A;
   case 65:
      goto opLDJimm_BB_A;
   case 66:
      goto opLDJimm_BB_A;
   case 67:
      goto opLDJimm_BB_A;
   case 68:
      goto opLDJimm_BB_A;
   case 69:
      goto opLDJimm_BB_A;
   case 70:
      goto opLDJimm_BB_A;
   case 71:
      goto opLDJimm_BB_A;
   case 72:
      goto opLDJimm_BB_A;
   case 73:
      goto opLDJimm_BB_A;
   case 74:
      goto opLDJimm_BB_A;
   case 75:
      goto opLDJimm_BB_A;
   case 76:
      goto opLDJimm_BB_A;
   case 77:
      goto opLDJimm_BB_A;
   case 78:
      goto opLDJimm_BB_A;
   case 79:
      goto opLDJimm_BB_A;
   case 80:
      goto tJPP_BB_B;		/* redirector */
   case 81:
      goto tJMI_BB_B;		/* redirector */
   case 82:
      goto opJDR_BB_B;
   case 83:
      goto opJLT_BB_B;
   case 84:
      goto opJEQ_BB_B;
   case 85:
      goto opJNC_BB_B;
   case 86:
      goto opJA0_BB_B;
   case 87:
      goto opNOP_BB_B;
   case 88:
      goto opJMP_BB_A;
   case 89:
      goto tJMI_BB_A;		/* redirector */
   case 90:
      goto opJDR_BB_A;
   case 91:
      goto opJLT_BB_A;
   case 92:
      goto opJEQ_BB_A;
   case 93:
      goto opJNC_BB_A;
   case 94:
      goto opJA0_BB_A;
   case 95:
      goto opNOP_BB_A;
   case 96:
      goto opADDdir_BB_AA;
   case 97:
      goto opADDdir_BB_AA;
   case 98:
      goto opADDdir_BB_AA;
   case 99:
      goto opADDdir_BB_AA;
   case 100:
      goto opADDdir_BB_AA;
   case 101:
      goto opADDdir_BB_AA;
   case 102:
      goto opADDdir_BB_AA;
   case 103:
      goto opADDdir_BB_AA;
   case 104:
      goto opADDdir_BB_AA;
   case 105:
      goto opADDdir_BB_AA;
   case 106:
      goto opADDdir_BB_AA;
   case 107:
      goto opADDdir_BB_AA;
   case 108:
      goto opADDdir_BB_AA;
   case 109:
      goto opADDdir_BB_AA;
   case 110:
      goto opADDdir_BB_AA;
   case 111:
      goto opADDdir_BB_AA;
   case 112:
      goto opSUBdir_BB_AA;
   case 113:
      goto opSUBdir_BB_AA;
   case 114:
      goto opSUBdir_BB_AA;
   case 115:
      goto opSUBdir_BB_AA;
   case 116:
      goto opSUBdir_BB_AA;
   case 117:
      goto opSUBdir_BB_AA;
   case 118:
      goto opSUBdir_BB_AA;
   case 119:
      goto opSUBdir_BB_AA;
   case 120:
      goto opSUBdir_BB_AA;
   case 121:
      goto opSUBdir_BB_AA;
   case 122:
      goto opSUBdir_BB_AA;
   case 123:
      goto opSUBdir_BB_AA;
   case 124:
      goto opSUBdir_BB_AA;
   case 125:
      goto opSUBdir_BB_AA;
   case 126:
      goto opSUBdir_BB_AA;
   case 127:
      goto opSUBdir_BB_AA;
   case 128:
      goto opLDPimm_BB_A;
   case 129:
      goto opLDPimm_BB_A;
   case 130:
      goto opLDPimm_BB_A;
   case 131:
      goto opLDPimm_BB_A;
   case 132:
      goto opLDPimm_BB_A;
   case 133:
      goto opLDPimm_BB_A;
   case 134:
      goto opLDPimm_BB_A;
   case 135:
      goto opLDPimm_BB_A;
   case 136:
      goto opLDPimm_BB_A;
   case 137:
      goto opLDPimm_BB_A;
   case 138:
      goto opLDPimm_BB_A;
   case 139:
      goto opLDPimm_BB_A;
   case 140:
      goto opLDPimm_BB_A;
   case 141:
      goto opLDPimm_BB_A;
   case 142:
      goto opLDPimm_BB_A;
   case 143:
      goto opLDPimm_BB_A;
   case 144:
      goto tOUT_BB_A;		/* redirector */
   case 145:
      goto tOUT_BB_A;		/* redirector */
   case 146:
      goto tOUT_BB_A;		/* redirector */
   case 147:
      goto tOUT_BB_A;		/* redirector */
   case 148:
      goto tOUT_BB_A;		/* redirector */
   case 149:
      goto tOUT_BB_A;		/* redirector */
   case 150:
      goto tOUT_BB_A;		/* redirector */
   case 151:
      goto tOUT_BB_A;		/* redirector */
   case 152:
      goto tOUT_BB_A;		/* redirector */
   case 153:
      goto tOUT_BB_A;		/* redirector */
   case 154:
      goto tOUT_BB_A;		/* redirector */
   case 155:
      goto tOUT_BB_A;		/* redirector */
   case 156:
      goto tOUT_BB_A;		/* redirector */
   case 157:
      goto tOUT_BB_A;		/* redirector */
   case 158:
      goto tOUT_BB_A;		/* redirector */
   case 159:
      goto tOUT_BB_A;		/* redirector */
   case 160:
      goto opLDAdir_BB_AA;
   case 161:
      goto opLDAdir_BB_AA;
   case 162:
      goto opLDAdir_BB_AA;
   case 163:
      goto opLDAdir_BB_AA;
   case 164:
      goto opLDAdir_BB_AA;
   case 165:
      goto opLDAdir_BB_AA;
   case 166:
      goto opLDAdir_BB_AA;
   case 167:
      goto opLDAdir_BB_AA;
   case 168:
      goto opLDAdir_BB_AA;
   case 169:
      goto opLDAdir_BB_AA;
   case 170:
      goto opLDAdir_BB_AA;
   case 171:
      goto opLDAdir_BB_AA;
   case 172:
      goto opLDAdir_BB_AA;
   case 173:
      goto opLDAdir_BB_AA;
   case 174:
      goto opLDAdir_BB_AA;
   case 175:
      goto opLDAdir_BB_AA;
   case 176:
      goto opCMPdir_BB_AA;
   case 177:
      goto opCMPdir_BB_AA;
   case 178:
      goto opCMPdir_BB_AA;
   case 179:
      goto opCMPdir_BB_AA;
   case 180:
      goto opCMPdir_BB_AA;
   case 181:
      goto opCMPdir_BB_AA;
   case 182:
      goto opCMPdir_BB_AA;
   case 183:
      goto opCMPdir_BB_AA;
   case 184:
      goto opCMPdir_BB_AA;
   case 185:
      goto opCMPdir_BB_AA;
   case 186:
      goto opCMPdir_BB_AA;
   case 187:
      goto opCMPdir_BB_AA;
   case 188:
      goto opCMPdir_BB_AA;
   case 189:
      goto opCMPdir_BB_AA;
   case 190:
      goto opCMPdir_BB_AA;
   case 191:
      goto opCMPdir_BB_AA;
   case 192:
      goto opLDIdir_BB_A;
   case 193:
      goto opLDIdir_BB_A;
   case 194:
      goto opLDIdir_BB_A;
   case 195:
      goto opLDIdir_BB_A;
   case 196:
      goto opLDIdir_BB_A;
   case 197:
      goto opLDIdir_BB_A;
   case 198:
      goto opLDIdir_BB_A;
   case 199:
      goto opLDIdir_BB_A;
   case 200:
      goto opLDIdir_BB_A;
   case 201:
      goto opLDIdir_BB_A;
   case 202:
      goto opLDIdir_BB_A;
   case 203:
      goto opLDIdir_BB_A;
   case 204:
      goto opLDIdir_BB_A;
   case 205:
      goto opLDIdir_BB_A;
   case 206:
      goto opLDIdir_BB_A;
   case 207:
      goto opLDIdir_BB_A;
   case 208:
      goto opSTAdir_BB_A;
   case 209:
      goto opSTAdir_BB_A;
   case 210:
      goto opSTAdir_BB_A;
   case 211:
      goto opSTAdir_BB_A;
   case 212:
      goto opSTAdir_BB_A;
   case 213:
      goto opSTAdir_BB_A;
   case 214:
      goto opSTAdir_BB_A;
   case 215:
      goto opSTAdir_BB_A;
   case 216:
      goto opSTAdir_BB_A;
   case 217:
      goto opSTAdir_BB_A;
   case 218:
      goto opSTAdir_BB_A;
   case 219:
      goto opSTAdir_BB_A;
   case 220:
      goto opSTAdir_BB_A;
   case 221:
      goto opSTAdir_BB_A;
   case 222:
      goto opSTAdir_BB_A;
   case 223:
      goto opSTAdir_BB_A;
   case 224:
      goto opVDR_BB_A;
   case 225:
      goto opLDJirg_BB_A;
   case 226:
      goto opXLT_BB_AA;
   case 227:
      goto opMULirg_BB_AA;
   case 228:
      goto opLLT_BB_AA;
   case 229:
      goto opWAI_BB_A;
   case 230:
      goto opSTAirg_BB_A;
   case 231:
      goto opADDirg_BB_AA;
   case 232:
      goto opSUBirg_BB_AA;
   case 233:
      goto opANDirg_BB_AA;
   case 234:
      goto opLDAirg_BB_AA;
   case 235:
      goto opLSRe_BB_AA;
   case 236:
      goto opLSLe_BB_AA;
   case 237:
      goto opASRe_BB_AA;
   case 238:
      goto opASRDe_BB_AA;
   case 239:
      goto opLSLDe_BB_AA;
   case 240:
      goto opVIN_BB_A;
   case 241:
      goto opLDJirg_BB_A;
   case 242:
      goto opXLT_BB_AA;
   case 243:
      goto opMULirg_BB_AA;
   case 244:
      goto opLLT_BB_AA;
   case 245:
      goto opWAI_BB_A;
   case 246:
      goto opSTAirg_BB_A;
   case 247:
      goto opAWDirg_BB_AA;
   case 248:
      goto opSUBirg_BB_AA;
   case 249:
      goto opANDirg_BB_AA;
   case 250:
      goto opLDAirg_BB_AA;
   case 251:
      goto opLSRf_BB_AA;
   case 252:
      goto opLSLf_BB_AA;
   case 253:
      goto opASRf_BB_AA;
   case 254:
      goto opASRDf_BB_AA;
   case 255:
      goto opLSLDf_BB_AA;
   } /* switch on opcode */

   /* the actual opcode code; each piece should be careful to (1) set the
      correct RCstate (2) increment the program counter as necessary (3) piss 
      with the flags as needed otherwise the next opcode will be completely
      buggered. */

 opINP_A_AA:
 opINP_AA_AA:
 opINP_BB_AA:
   /* bottom 4 bits of opcode are the position of the bit we want; obtain
      input value, shift over that no, and truncate to last bit. NOTE:
      Masking 0x07 does interesting things on Sundance and others, but
      masking 0x0F makes RipOff and others actually work :) */
#ifdef DEBUGGING_STARHAWK
   RCcmp_new = (ioInputs >> (rom[RCregister_PC] & 0x0FF)) & 0x01; // some of the ioInputs care > 0x10000 - don't know how to extact data.
#else
   RCcmp_new = (ioInputs >> (rom[RCregister_PC] & 0x0F)) & 0x01;
#endif
   
   SETA0 (RCregister_A);	/* save old accA bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_A;	/* save old accB */
   RCregister_A = RCcmp_new;	/* load new accB; zero other bits */

   RCregister_PC++;
   jumpCineRet_AA;

 opINP_B_AA:
   /* bottom 3 bits of opcode are the position of the bit we want; obtain
      Switches value, shift over that no, and truncate to last bit. */
   RCcmp_new = (ioSwitches >> (rom[RCregister_PC] & 0x07)) & 0x01;

   SETA0 (RCregister_A);	/* save old accA bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_B;	/* save old accB */
   RCregister_B = RCcmp_new;	/* load new accB; zero other bits */

   RCregister_PC++;
   jumpCineRet_AA;

 opOUTbi_A_A:
 opOUTbi_AA_A:
 opOUTbi_BB_A:

   temp_byte = rom[RCregister_PC] & 0x07;
   RCregister_PC++;

   if (temp_byte - 0x06) {
      goto opOUTsnd_A;
   }

   RCvgColour = ((RCregister_A & 0x01) << 3) | 0x07;

   jumpCineRet_A;

 opOUT16_A_A:
 opOUT16_AA_A:
 opOUT16_BB_A:

   temp_byte = rom[RCregister_PC] & 0x07;
   RCregister_PC++;

   if (temp_byte - 0x06) {
      goto opOUTsnd_A;
   }

   if ((RCregister_A & 0xFF) != 1) {
      RCvgColour = RCFromX & 0x0F;

      if (!RCvgColour) {
	 RCvgColour = 1;
      }

   }

   jumpCineRet_A;

 opOUTsnd_A:
   temp_byte = 0x01 << (rom[RCregister_PC] & 0x07);

   if (!(RCregister_A & 0x01)) {
      goto opOUT_Aset;
   }

   temp_byte = (!temp_byte);	/* BUG? Should this not be ~temp_byte */
   ioOutputs &= temp_byte;

   if ((rom[RCregister_PC] & 0x07) == 0x05) {
      goto opOUT_Aq;
   }

   jumpCineRet_A;

 opOUT_Aq:
   /* reset coin counter */
   jumpCineRet_A;

 opOUT_Aset:
   ioOutputs |= temp_byte;
   jumpCineRet_A;

 opOUT64_A_A:
 opOUT64_AA_A:
 opOUT64_BB_A:
   jumpCineRet_A;

 opOUTWW_A_A:
 opOUTWW_AA_A:
 opOUTWW_BB_A:
   temp_byte = rom[RCregister_PC] & 0x07;
   RCregister_PC++;

   if (temp_byte - 0x06) {
      goto opOUTsnd_A;
   }

   if ((RCregister_A & 0xFF) == 1) {
      temp_word = (!RCFromX) & 0xFFF;
      if (!temp_word) {		/* black */
	 RCvgColour = 0;
      } else {			/* non-black */
	 if (temp_word & 0x0888) {
	    /* bright */
	    temp_word_2 = ((temp_word << 4) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    temp_word_2 = ((temp_word << 3) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    temp_word_2 = ((temp_word << 3) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    RCvgColour = (temp_byte & 0x07) + 7;
	 } else if (temp_word & 0x0444) {
	    /* dim bits */
	    temp_word_2 = ((temp_word << 5) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    temp_word_2 = ((temp_word << 3) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    temp_word_2 = ((temp_word << 3) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    RCvgColour = (temp_byte & 0x07);
	 } else {
	    /* dim white */
	    RCvgColour = 0x0F;
	 }
      }
   }
   /* colour change? == 1 */
   jumpCineRet_A;

 opOUTbi_B_BB:
   temp_byte = rom[RCregister_PC] & 0x07;
   RCregister_PC++;

   if (temp_byte - 0x06) {
      goto opOUTsnd_B;
   }

   RCvgColour = ((RCregister_B & 0x01) << 3) | 0x07;

   jumpCineRet_BB;

 opOUT16_B_BB:
   temp_byte = rom[RCregister_PC] & 0x07;
   RCregister_PC++;

   if (temp_byte - 0x06) {
      goto opOUTsnd_B;
   }

   if ((RCregister_B & 0xFF) != 1) {
      RCvgColour = RCFromX & 0x0F;

      if (!RCvgColour) {
	 RCvgColour = 1;
      }

   }

   jumpCineRet_BB;

 opOUTsnd_B:
   jumpCineRet_BB;

 opOUT64_B_BB:
   jumpCineRet_BB;

 opOUTWW_B_BB:
   jumpCineRet_BB;

   /* LDA imm (0x) */
 opLDAimm_A_AA:
 opLDAimm_AA_AA:
 opLDAimm_BB_AA:
   temp_word = rom[RCregister_PC] & 0x0F;	/* pick up immediate value */
   temp_word <<= 8;		/* LDAimm is the HIGH nibble! */

   RCcmp_new = temp_word;	/* set new comparison flag */

   SETA0 (RCregister_A);	/* save old accA bit0 */
   SETFC (RCregister_A);	/* ??? clear carry? */

   RCcmp_old = RCregister_A;	/* step back cmp flag */
   RCregister_A = temp_word;	/* set the register */

   RCregister_PC++;		/* increment PC */
   jumpCineRet_AA;		/* swap RCstate and end opcode */

 opLDAimm_B_AA:
   temp_word = rom[RCregister_PC] & 0x0F;	/* pick up immediate value */
   temp_word <<= 8;		/* LDAimm is the HIGH nibble! */

   RCcmp_new = temp_word;	/* set new comparison flag */

   SETA0 (RCregister_A);	/* save old accA bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_B;	/* step back cmp flag */
   RCregister_B = temp_word;	/* set the register */

   RCregister_PC++;		/* increment PC */
   jumpCineRet_AA;

 opLDAdir_A_AA:
 opLDAdir_AA_AA:
 opLDAdir_BB_AA:

   temp_byte = rom[RCregister_PC] & 0x0F;	/* snag imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set I register */

   RCcmp_new = RCram[RCregister_I];	/* new acc value */

   SETA0 (RCregister_A);	/* back up bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_A;	/* store old acc */
   RCregister_A = RCcmp_new;	/* store new acc */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opLDAdir_B_AA:

   temp_byte = rom[RCregister_PC] & 0x0F;	/* snag imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set I register */

   RCcmp_new = RCram[RCregister_I];	/* new acc value */

   SETA0 (RCregister_A);	/* back up bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_B;	/* store old acc */
   RCregister_B = RCcmp_new;	/* store new acc */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opLDAirg_A_AA:
 opLDAirg_AA_AA:
 opLDAirg_BB_AA:

   RCcmp_new = RCram[RCregister_I];

   SETA0 (RCregister_A);
   SETFC (RCregister_A);

   RCcmp_old = RCregister_A;
   RCregister_A = RCcmp_new;

   RCregister_PC++;
   jumpCineRet_AA;

 opLDAirg_B_AA:
   RCcmp_new = RCram[RCregister_I];

   SETA0 (RCregister_A);
   SETFC (RCregister_A);

   RCcmp_old = RCregister_B;
   RCregister_B = RCcmp_new;

   RCregister_PC++;
   jumpCineRet_AA;

   /* ADD imm */
 opADDimm_A_AA:
 opADDimm_AA_AA:
 opADDimm_BB_AA:
   temp_word = rom[RCregister_PC] & 0x0F;	/* get imm value */

   RCcmp_new = temp_word;	/* new acc value */
   SETA0 (RCregister_A);	/* save old accA bit0 */
   RCcmp_old = RCregister_A;	/* store old acc for later */

   RCregister_A += temp_word;	/* add values */
   SETFC (RCregister_A);	/* store carry and extra */
   RCregister_A &= 0xFFF;	/* toss out >12bit carry */

   RCregister_PC++;
   jumpCineRet_AA;

 opADDimm_B_AA:
   temp_word = rom[RCregister_PC] & 0x0F;	/* get imm value */

   RCcmp_new = temp_word;	/* new acc value */
   SETA0 (RCregister_A);	/* save old accA bit0 */
   RCcmp_old = RCregister_B;	/* store old acc for later */

   RCregister_B += temp_word;	/* add values */
   SETFC (RCregister_B);	/* store carry and extra */
   RCregister_B &= 0xFFF;	/* toss out >12bit carry */

   RCregister_PC++;
   jumpCineRet_AA;

   /* ADD imm extended */
 opADDimmX_A_AA:
 opADDimmX_AA_AA:
 opADDimmX_BB_AA:
   RCcmp_new = rom[RCregister_PC + 1];	/* get extended value */
   SETA0 (RCregister_A);	/* save old accA bit0 */
   RCcmp_old = RCregister_A;	/* store old acc for later */

   RCregister_A += RCcmp_new;	/* add values */
   SETFC (RCregister_A);	/* store carry and extra */
   RCregister_A &= 0xFFF;	/* toss out >12bit carry */

   RCregister_PC += 2;		/* bump PC */
   jumpCineRet_AA;

 opADDimmX_B_AA:
   RCcmp_new = rom[RCregister_PC + 1];	/* get extended value */
   SETA0 (RCregister_A);	/* save old accA bit0 */
   RCcmp_old = RCregister_B;	/* store old acc for later */

   RCregister_B += RCcmp_new;	/* add values */
   SETFC (RCregister_B);	/* store carry and extra */
   RCregister_B &= 0xFFF;	/* toss out >12bit carry */

   RCregister_PC += 2;		/* bump PC */
   jumpCineRet_AA;

 opADDdir_A_AA:
 opADDdir_AA_AA:
 opADDdir_BB_AA:

   temp_byte = rom[RCregister_PC] & 0x0F;	/* fetch imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set regI addr */

   RCcmp_new = RCram[RCregister_I];	/* fetch imm real value */
   SETA0 (RCregister_A);	/* store bit0 */
   RCcmp_old = RCregister_A;	/* store old acc value */

   RCregister_A += RCcmp_new;	/* do acc operation */
   SETFC (RCregister_A);	/* store carry and extra */
   RCregister_A &= 0xFFF;

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opADDdir_B_AA:
   temp_byte = rom[RCregister_PC] & 0x0F;	/* fetch imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set regI addr */

   RCcmp_new = RCram[RCregister_I];	/* fetch imm real value */
   SETA0 (RCregister_A);	/* store bit0 */
   RCcmp_old = RCregister_B;	/* store old acc value */

   RCregister_B += RCcmp_new;	/* do acc operation */
   SETFC (RCregister_B);	/* store carry and extra */
   RCregister_B &= 0xFFF;

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opAWDirg_A_AA:
 opAWDirg_AA_AA:
 opAWDirg_BB_AA:
 opADDirg_A_AA:
 opADDirg_AA_AA:
 opADDirg_BB_AA:

   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   RCregister_A += RCcmp_new;
   SETFC (RCregister_A);
   RCregister_A &= 0xFFF;

   RCregister_PC++;
   jumpCineRet_AA;

 opAWDirg_B_AA:
 opADDirg_B_AA:
   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_B;

   RCregister_B += RCcmp_new;
   SETFC (RCregister_B);
   RCregister_B &= 0xFFF;

   RCregister_PC++;
   jumpCineRet_AA;

 opSUBimm_A_AA:
 opSUBimm_AA_AA:
 opSUBimm_BB_AA:
   /* SUBtractions are negate-and-add instructions of the CCPU; what a pain in the ass. */
   temp_word = rom[RCregister_PC] & 0x0F;

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_A += temp_word;	/* add */
   SETFC (RCregister_A);	/* pick up top bits */
   RCregister_A &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opSUBimm_B_AA:
   /* SUBtractions are negate-and-add instructions of the CCPU; what a pain in the ass. */
   temp_word = rom[RCregister_PC] & 0x0F;

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_B;

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_B += temp_word;	/* add */
   SETFC (RCregister_B);	/* pick up top bits */
   RCregister_B &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opSUBimmX_A_AA:
 opSUBimmX_AA_AA:
 opSUBimmX_BB_AA:

   temp_word = rom[RCregister_PC + 1];	/* snag imm value */

   RCcmp_new = temp_word;	/* save cmp value */
   SETA0 (RCregister_A);	/* store bit0 */
   RCcmp_old = RCregister_A;	/* back up regA */

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_A += temp_word;	/* add */
   SETFC (RCregister_A);	/* pick up top bits */
   RCregister_A &= 0xFFF;	/* mask final regA value */

   RCregister_PC += 2;		/* bump PC */
   jumpCineRet_AA;

 opSUBimmX_B_AA:

   temp_word = rom[RCregister_PC + 1];	/* snag imm value */

   RCcmp_new = temp_word;	/* save cmp value */
   SETA0 (RCregister_A);	/* store bit0 */
   RCcmp_old = RCregister_B;	/* back up regA */

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_B += temp_word;	/* add */
   SETFC (RCregister_B);	/* pick up top bits */
   RCregister_B &= 0xFFF;	/* mask final regA value */

   RCregister_PC += 2;		/* bump PC */
   jumpCineRet_AA;

 opSUBdir_A_AA:
 opSUBdir_AA_AA:
 opSUBdir_BB_AA:
   temp_word = rom[RCregister_PC] & 0x0F;	/* fetch imm value */
   RCregister_I = (RCregister_P << 4) + temp_word;	/* set regI addr */

   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word = (RCcmp_new ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_A += temp_word;	/* add */
   SETFC (RCregister_A);	/* pick up top bits */
   RCregister_A &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;
   jumpCineRet_AA;

 opSUBdir_B_AA:

   temp_byte = rom[RCregister_PC] & 0x0F;	/* fetch imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set regI addr */

   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_B;

   temp_word = (RCcmp_new ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_B += temp_word;	/* add */
   SETFC (RCregister_B);	/* pick up top bits */
   RCregister_B &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;
   jumpCineRet_AA;

 opSUBirg_A_AA:
 opSUBirg_AA_AA:
 opSUBirg_BB_AA:
   /* sub [i] */
   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word = (RCcmp_new ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_A += temp_word;	/* add */
   SETFC (RCregister_A);	/* pick up top bits */
   RCregister_A &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;
   jumpCineRet_AA;

 opSUBirg_B_AA:
   /* sub [i] */
   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_B;

   temp_word = (RCcmp_new ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_B += temp_word;	/* add */
   SETFC (RCregister_B);	/* pick up top bits */
   RCregister_B &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;
   jumpCineRet_AA;

   /* CMP dir */
 opCMPdir_A_AA:
 opCMPdir_AA_AA:
 opCMPdir_BB_AA:
   /* compare direct mode; don't modify regs, just set carry flag or not. */

   temp_byte = rom[RCregister_PC] & 0x0F;	/* obtain relative addr */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* build real addr */

   temp_word = RCram[RCregister_I];
   RCcmp_new = temp_word;	/* new acc value */
   SETA0 (RCregister_A);	/* backup bit0 */
   RCcmp_old = RCregister_A;	/* backup old acc */

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   temp_word += RCregister_A;
   SETFC (temp_word);		/* pick up top bits */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opCMPdir_B_AA:
   temp_byte = rom[RCregister_PC] & 0x0F;	/* obtain relative addr */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* build real addr */

   temp_word = RCram[RCregister_I];
   RCcmp_new = temp_word;	/* new acc value */
   SETA0 (RCregister_A);	/* backup bit0 */
   RCcmp_old = RCregister_B;	/* backup old acc */

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   temp_word += RCregister_B;
   SETFC (temp_word);		/* pick up top bits */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

   /* AND [i] */
 opANDirg_A_AA:
 opANDirg_AA_AA:
 opANDirg_BB_AA:
   RCcmp_new = RCram[RCregister_I];	/* new acc value */
   SETA0 (RCregister_A);
   SETFC (RCregister_A);
   RCcmp_old = RCregister_A;

   RCregister_A &= RCcmp_new;

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opANDirg_B_AA:
   RCcmp_new = RCram[RCregister_I];	/* new acc value */
   SETA0 (RCregister_A);
   SETFC (RCregister_A);
   RCcmp_old = RCregister_B;

   RCregister_B &= RCcmp_new;

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

   /* LDJ imm */
 opLDJimm_A_A:
 opLDJimm_AA_A:
 opLDJimm_BB_A:
   temp_byte = rom[RCregister_PC + 1];	/* upper part of address */
   temp_byte = (temp_byte << 4) | (temp_byte >> 4); /* Silly CCPU; Swap nibbles */

   /* put the upper 8 bits above the existing 4 bits */
   RCregister_J = (rom[RCregister_PC] & 0x0F) | (temp_byte << 4);

   RCregister_PC += 2;
   jumpCineRet_A;

 opLDJimm_B_BB:
   temp_byte = rom[RCregister_PC + 1];	/* upper part of address */
   temp_byte = (temp_byte << 4) | (temp_byte >> 4); /* Silly CCPU; Swap nibbles */

   /* put the upper 8 bits above the existing 4 bits */
   RCregister_J = (rom[RCregister_PC] & 0x0F) | (temp_byte << 4);

   RCregister_PC += 2;
   jumpCineRet_BB;

   /* LDJ irg */
 opLDJirg_A_A:
 opLDJirg_AA_A:
 opLDJirg_BB_A:
   /* load J reg from value at last dir addr */
   RCregister_J = RCram[RCregister_I];
   RCregister_PC++;		/* bump PC */
   jumpCineRet_A;

 opLDJirg_B_BB:
   RCregister_J = RCram[RCregister_I];
   RCregister_PC++;		/* bump PC */
   jumpCineRet_BB;

   /* LDP imm */
 opLDPimm_A_A:
 opLDPimm_AA_A:
 opLDPimm_BB_A:
   /* load page register from immediate */
   RCregister_P = rom[RCregister_PC] & 0x0F;	/* set page register */
   RCregister_PC++;		/* inc PC */
   jumpCineRet_A;

 opLDPimm_B_BB:
   /* load page register from immediate */
   RCregister_P = rom[RCregister_PC] & 0x0F;	/* set page register */
   RCregister_PC++;		/* inc PC */
   jumpCineRet_BB;

   /* LDI dir */
 opLDIdir_A_A:
 opLDIdir_AA_A:
 opLDIdir_BB_A:
   /* load regI directly .. */

   temp_byte = (RCregister_P << 4) +	/* get rampage ... */
    (rom[RCregister_PC] & 0x0F);	/* and imm half of ramaddr.. */

   RCregister_I = RCram[temp_byte] & 0xFF;	/* set/mask new RCregister_I */

   RCregister_PC++;
   jumpCineRet_A;

 opLDIdir_B_BB:
   temp_byte = (RCregister_P << 4) + (rom[RCregister_PC] & 0x0F);	/* get rampage ... and imm half of ramaddr.. */
   RCregister_I = RCram[temp_byte] & 0xFF;	/* set/mask new RCregister_I */
   RCregister_PC++;
   jumpCineRet_BB;

   /* STA dir */
 opSTAdir_A_A:
 opSTAdir_AA_A:
 opSTAdir_BB_A:
   temp_byte = rom[RCregister_PC] & 0x0F;	/* snag imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set I register */
   RCram[RCregister_I] = RCregister_A;	/* store acc to RCram */
   RCregister_PC++;		/* inc PC */
   jumpCineRet_A;

 opSTAdir_B_BB:
   temp_byte = rom[RCregister_PC] & 0x0F;	/* snag imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set I register */
   RCram[RCregister_I] = RCregister_B;	/* store acc to RCram */
   RCregister_PC++;		/* inc PC */
   jumpCineRet_BB;

   /* STA irg */
 opSTAirg_A_A:
 opSTAirg_AA_A:
 opSTAirg_BB_A:
   /* STA into address specified in regI. Nice and easy :) */
   RCram[RCregister_I] = RCregister_A;	/* store acc */
   RCregister_PC++;		/* bump PC */
   jumpCineRet_A;

 opSTAirg_B_BB:
   RCram[RCregister_I] = RCregister_B;	/* store acc */
   RCregister_PC++;		/* bump PC */
   jumpCineRet_BB;

   /* XLT */
 opXLT_A_AA:
 opXLT_AA_AA:
 opXLT_BB_AA:
   /* XLT is weird; it loads the current accumulator with the bytevalue at
      ROM location pointed to by the accumulator; this allows the programto
      read the programitself..  NOTE! Next opcode is *IGNORED!* because of a
      twisted side-effect */

   RCcmp_new = rom[(RCregister_PC & 0xF000) + RCregister_A];	/* store new acc value */
   SETA0 (RCregister_A);	/* store bit0 */
   SETFC (RCregister_A);
   RCcmp_old = RCregister_A;	/* back up acc */
   RCregister_A = RCcmp_new;	/* new acc value */
   RCregister_PC += 2;		/* bump PC twice because XLT is fucked up */
   jumpCineRet_AA;

 opXLT_B_AA:

   RCcmp_new = rom[(RCregister_PC & 0xF000) + RCregister_B];	/* store new acc value */
   SETA0 (RCregister_A);	/* store bit0 */
   SETFC (RCregister_A);
   RCcmp_old = RCregister_B;	/* back up acc */
   RCregister_B = RCcmp_new;	/* new acc value */
   RCregister_PC += 2;		/* bump PC twice because XLT is fucked up */
   jumpCineRet_AA;

   /* MUL [i] */
 opMULirg_A_AA:
 opMULirg_AA_AA:
 opMULirg_BB_AA:
   /* MUL's usually happen in batches, so a slight speed bump can be gained
      by checking for multiple instances and handling in here, without going
      through the main loop for each. */
   temp_word = RCram[RCregister_I];	/* pick up ramvalue */
   RCcmp_new = temp_word;
   temp_word <<= 4;		/* shift into ADD position */
   RCregister_B <<= 4;		/* get sign bit 15 */
   RCregister_B |= (RCregister_A >> 8);	/* bring in A high nibble */
   RCregister_A = ((RCregister_A & 0xFF) << 8) | (rom[RCregister_PC] & 0xFF);	/* shift over 8 bits and pick up opcode */
   temp_byte = rom[RCregister_PC] & 0xFF;	/* (for ease and speed) */

   /* handle multiple consecutive MUL's */

   RCregister_PC++;		/* inc PC */

   if (rom[RCregister_PC] != temp_byte) {	/* next opcode is a MUL? */
      goto opMUL1;		/* no? skip multiples... */
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL2;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL3;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL4;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL5;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL6;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL7;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL8;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL9;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL10;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL11;
   }

 opMUL12:
   RCregister_PC++;		/* we don't bother to check for more than 12 multiple
				   occurances, so just inc the PC and don't worry about it. */

   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	temp_word_2; /* rotate right ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL11;
   }
   RCregister_B += temp_word;

 opMUL11:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL10;
   }
   RCregister_B += temp_word;

 opMUL10:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL9;
   }
   RCregister_B += temp_word;

 opMUL9:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL8;
   }
   RCregister_B += temp_word;

 opMUL8:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL7;
   }
   RCregister_B += temp_word;

 opMUL7:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL6;
   }
   RCregister_B += temp_word;

 opMUL6:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL5;
   }
   RCregister_B += temp_word;

 opMUL5:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL4;
   }
   RCregister_B += temp_word;

 opMUL4:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL3;
   }
   RCregister_B += temp_word;

 opMUL3:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL2;
   }
   RCregister_B += temp_word;

 opMUL2:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL1;
   }
   RCregister_B += temp_word;

 opMUL1:

   if (RCregister_A & 0x100) {	/* 1bit shifted out? */
      goto opMULshf;
   }

   RCregister_A = (RCregister_A >> 8) |	/* Bhigh | Alow */
    ((RCregister_B & 0xFF) << 8);

   temp_word = RCregister_A & 0xFFF;

   SETA0 (temp_word & 0xFF);	/* store bit0 */
   RCcmp_old = temp_word;

   temp_word += RCcmp_new;
   SETFC (temp_word);

   RCregister_A >>= 1;
   RCregister_A &= 0xFFF;

   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opMULshf:			/* part of opMULirg */

   RCregister_A = (RCregister_A >> 8) | ((RCregister_B & 0xFF) << 8);

   SETA0 (RCregister_A & 0xFF);	/* store bit0 */

   RCregister_A >>= 1;
   RCregister_A &= 0xFFF;

   RCregister_B = SAR16 (RCregister_B, 4);
   RCcmp_old = RCregister_B & 0x0F;

   RCregister_B = SAR16 (RCregister_B, 1);

   RCregister_B &= 0xFFF;
   RCregister_B += RCcmp_new;

   SETFC (RCregister_B);

   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opMULirg_B_AA:
   RCregister_PC++;

   temp_word = RCram[RCregister_I];
   RCcmp_new = temp_word;
   RCcmp_old = RCregister_B;
   SETA0 (RCregister_A & 0xFF);

   RCregister_B <<= 4;

   RCregister_B = SAR16 (RCregister_B, 5);

   if (RCregister_A & 0x01) {
      goto opMULirgB1;
   }

   temp_word += RCregister_B;
   SETFC (temp_word);

   jumpCineRet_AA;

 opMULirgB1:
   RCregister_B += temp_word;
   SETFC (RCregister_B);
   RCregister_B &= 0xFFF;
   jumpCineRet_AA;

   /* LSRe */
 opLSRe_A_AA:
 opLSRe_AA_AA:
 opLSRe_BB_AA:
   /* EB; right shift pure; fill new bit with zero. */
   temp_word = 0x0BEB;

   RCregister_PC++;

   if (rom[RCregister_PC] == 0xEB) {
      goto opLSRe_A0;		/* multiples */
   }

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A >>= 1;
   jumpCineRet_AA;

 opLSRe_A0:
   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;

 opLSRe_A1:
   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A >>= 1;
   jumpCineRet_AA;

 opLSRe_B_AA:
   UNFINISHED ("opLSRe B 1\n");
   jumpCineRet_AA;

 opLSRf_A_AA:
 opLSRf_AA_AA:
 opLSRf_BB_AA:
   UNFINISHED ("opLSRf 1\n");
   jumpCineRet_AA;

 opLSRf_B_AA:
   UNFINISHED ("opLSRf 2\n");
   jumpCineRet_AA;

 opLSLe_A_AA:
 opLSLe_AA_AA:
 opLSLe_BB_AA:
   /* EC; left shift pure; fill new bit with zero */
   /* This version supports multiple consecutive LSLe's; the older version
      only did one at a time. I'm changing it to make tracing easier (as its
      comperable to Zonn's) */

   RCregister_PC++;
   temp_word = 0x0CEC;

   if (rom[RCregister_PC] == 0xEC) {
      goto opLSLe_A0;		/* do multiples */
   }

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 1;
   RCregister_A &= 0xFFF;

   jumpCineRet_AA;

 opLSLe_A0:
   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;
   RCregister_PC++;

 opLSLe_A1:
   RCregister_A &= 0xFFF;

 opLSLe_A2:
   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 1;
   RCregister_A &= 0xFFF;

   jumpCineRet_AA;

#if 0				/* non-multiple-consecutive-LSLe's version */
   temp_word = 0x0CEC;		/* data register */

   RCcmp_new = temp_word;	/* magic value */
   SETA0 (RCregister_A);	/* back up bit0 */
   RCcmp_old = RCregister_A;	/* store old acc */

   temp_word += RCregister_A;	/* add to acc */
   SETFC (temp_word);		/* store carry flag */
   RCregister_A <<= 1;		/* add regA to itself */
   RCregister_A &= 0xFFF;	/* toss excess bits */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;
#endif

 opLSLe_B_AA:
   temp_word = 0x0CEC;		/* data register */

   RCcmp_new = temp_word;	/* magic value */
   SETA0 (RCregister_A);	/* back up bit0 */
   RCcmp_old = RCregister_B;	/* store old acc */

   temp_word += RCregister_B;	/* add to acc */
   SETFC (temp_word);		/* store carry flag */
   RCregister_B <<= 1;		/* add regA to itself */
   RCregister_B &= 0xFFF;	/* toss excess bits */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opLSLf_A_AA:
 opLSLf_AA_AA:
 opLSLf_BB_AA:
   UNFINISHED ("opLSLf 1\n");
   jumpCineRet_AA;

 opLSLf_B_AA:
   UNFINISHED ("opLSLf 2\n");
   jumpCineRet_AA;

 opASRe_A_AA:
 opASRe_AA_AA:
 opASRe_BB_AA:
   /* agh! I dislike these silly 12bit processors :P */

   temp_word = 0xDED;

   RCregister_PC++;

   if (rom[RCregister_PC] == (temp_word & 0xFF)) {
      goto opASRe_A0;
   }
   RCcmp_new = temp_word;

   SETA0 (RCregister_A);	/* store bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_A;

   RCregister_A <<= 4;		/* get sign bit */
   RCregister_A = SAR16 (RCregister_A, 5);
   RCregister_A &= 0xFFF;

   jumpCineRet_AA;

 opASRe_A0:
   /* multiple ASRe's ... handle 'em in a batch, for efficiency */

   RCregister_A <<= 4;
   RCregister_A = SAR16 (RCregister_A, 1);

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;

 opASRe_A1:
   /* no more multiples left; finish off. */
   RCregister_A >>= 4;

 opASRe_A2:
   /* shift once with flags */
   RCcmp_new = temp_word;

   SETA0 (RCregister_A);	/* store bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_A;

   RCregister_A <<= 4;		/* get sign bit */
   RCregister_A = SAR16 (RCregister_A, 5);
   RCregister_A &= 0xFFF;

   jumpCineRet_AA;

 opASRe_B_AA:
   RCregister_PC++;

   if ((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57)) {
      goto opASRe_B0;		/* another one follows, do multiples */
   }

   RCcmp_new = 0x0DED;
   SETA0 (RCregister_A);
   SETFC (RCregister_A);
   RCcmp_old = RCregister_B;

   RCregister_B <<= 4;
   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opASRe_B0:
   RCregister_B <<= 4;		/* get sign bit */

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;

 opASRe_B1:
   RCregister_B >>= 4;		/* fix register */

 opASRe_B2:
   RCcmp_new = 0x0DED;
   SETA0 (RCregister_A);
   SETFC (RCregister_A);
   RCcmp_old = RCregister_B;

   RCregister_B <<= 4;
   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opASRf_A_AA:
 opASRf_AA_AA:
 opASRf_BB_AA:
   UNFINISHED ("opASRf 1\n");
   jumpCineRet_AA;

 opASRf_B_AA:
   UNFINISHED ("opASRf 2\n");
   jumpCineRet_AA;

 opASRDe_A_AA:
 opASRDe_AA_AA:
 opASRDe_BB_AA:
   /* Arithmetic shift right of D (A+B) .. B is high (sign bits). divide by
      2, but leave the sign bit the same. (ie: 1010 -> 1001) */
   temp_word = 0x0EEE;
   RCregister_PC++;

   if (rom[RCregister_PC] == (temp_word & 0xFF)) {
      goto opASRDe_A0;		/* multiples, do the batch */
   }

   RCcmp_new = temp_word;	/* save new acc value */
   SETA0 (RCregister_A & 0xFF);	/* save old accA bit0 */
   RCcmp_old = RCregister_A;	/* save old acc */

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 4;
   RCregister_B <<= 4;

   temp_word_2 = (RCregister_B >> 4) << 15;
   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_A >>= 4;

   RCregister_B &= 0xFFF;
   jumpCineRet_AA;

 opASRDe_A0:
   RCregister_A <<= 4;
   RCregister_B <<= 4;

   temp_word_2 = (RCregister_B >> 4) << 15;
   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;

   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more, do last one */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;

   RCregister_PC++;

 opASRDe_A1:			/* do last shift with flags */
   RCregister_A >>= 4;

   RCcmp_new = temp_word;
   SETA0 (RCregister_A & 0xFF);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 4;
   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_A >>= 4;

   RCregister_B &= 0xFFF;
   jumpCineRet_AA;

 opASRDe_B_AA:
   RCregister_PC++;
   temp_word = 0x0EEE;
   RCcmp_new = temp_word;
   SETA0 (RCregister_A & 0xFF);
   RCcmp_old = RCregister_B;

   temp_word += RCregister_B;
   SETFC (temp_word);
   RCregister_B <<= 4;
   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opASRDf_A_AA:
 opASRDf_AA_AA:
 opASRDf_BB_AA:
   UNFINISHED ("opASRDf 1\n");
   jumpCineRet_AA;

 opASRDf_B_AA:
   UNFINISHED ("opASRDf 2\n");
   jumpCineRet_AA;

 opLSLDe_A_AA:
 opLSLDe_AA_AA:
 opLSLDe_BB_AA:
   /* LSLDe -- Left shift through both accumulators; lossy in middle. */

   temp_word = 0x0FEF;

   RCregister_PC++;
   if (rom[RCregister_PC] == (temp_word & 0xFF)) {
      goto opLSLDe_A0;		/* multiples.. go to it. */
   }

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);
   RCregister_A <<= 1;		/* logical shift left */
   RCregister_A &= 0xFFF;

   RCregister_B <<= 1;
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opLSLDe_A0:
   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;

 opLSLDe_A1:
   RCregister_A &= 0xFFF;
   RCregister_B &= 0xFFF;

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);
   RCregister_A <<= 1;		/* logical shift left */
   RCregister_A &= 0xFFF;

   RCregister_B <<= 1;
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opLSLDe_B_AA:
   UNFINISHED ("opLSLD 1\n");
   jumpCineRet_AA;

 opLSLDf_A_AA:
 opLSLDf_AA_AA:
 opLSLDf_BB_AA:
   /* LSLDf */

   temp_word = 0xFFF;

   RCregister_PC++;
   if (rom[RCregister_PC] == (temp_word & 0xFF)) {
      goto opLSLDf_A0;		/* do multiple batches */
   }

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 1;
   RCregister_A &= 0xFFF;

   RCregister_B <<= 1;
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opLSLDf_A0:

   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;

 opLSLDf_A1:
   RCregister_A &= 0xFFF;
   RCregister_B &= 0xFFF;

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   SETFC (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 1;
   RCregister_A &= 0xFFF;

   RCregister_B <<= 1;
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opLSLDf_B_AA:			/* not 'the same' as the A->AA version above */

   RCregister_PC++;

   temp_word = 0xFFF;
   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_B;

   temp_word += RCregister_B;
   SETFC (temp_word);

   RCregister_B <<= 1;
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opJMP_A_A:
 opJMP_AA_A:
 opJMP_BB_A:
   /* simple jump; change PC and continue.. */

   /* Use 0xF000 so as to keep the current page, since it may well have been
      changed with JPP. */
   RCregister_PC = (RCregister_PC & 0xF000) + RCregister_J;	/* pick up new PC */
   jumpCineRet_A;

 opJMP_B_BB:
   RCregister_PC = (RCregister_PC & 0xF000) + RCregister_J;	/* pick up new PC */
   jumpCineRet_BB;

 opJEI_A_A:
 opJEI_AA_A:
 opJEI_BB_A:

   if (!(ioOutputs & 0x80)) {
      goto opja1;
   }

   if ((RCFromX - JoyX) > 0) {
      goto opJMP_A_A;
   }

   RCregister_PC++;		/* increment PC */
   jumpCineRet_A;

 opja1:

   if ((RCFromX - JoyY) > 0) {
      goto opJMP_A_A;
   }

   RCregister_PC++;
   jumpCineRet_A;

 opJEI_B_BB:

   if (!(ioOutputs & 0x80)) {
      goto opjbb1;
   }

   if ((RCFromX - JoyX) > 0) {
      goto opJMP_B_BB;
   }

   RCregister_PC++;		/* increment PC */
   jumpCineRet_BB;

 opjbb1:

   if ((RCFromX - JoyY) > 0) {
      goto opJMP_B_BB;
   }

   RCregister_PC++;
   jumpCineRet_BB;

 opJEI_A_B:
 opJEI_AA_B:
 opJEI_BB_B:

   if (!(ioOutputs & 0x80)) {
      goto opjb1;
   }

   if ((RCFromX - JoyX) > 0) {
      goto opJMP_A_B;
   }

   RCregister_PC++;		/* increment PC */
   jumpCineRet_B;

 opjb1:

   if ((RCFromX - JoyY) > 0) {
      goto opJMP_A_B;
   }

   RCregister_PC++;
   jumpCineRet_B;

 opJMI_A_A:
   /* previous instruction was not an ACC instruction, nor was the
      instruction twice back a USB, therefore minus flag test the current
      A-reg */

   /* negative acc? */
   if (RCregister_A & 0x800) {
      goto opJMP_A_A;		/* yes -- do jump */
   }

   RCregister_PC++;		/* increment PC */
   jumpCineRet_A;

 opJMI_AA_A:
   /* previous acc negative? Jump if so... */
   if (RCcmp_old & 0x800) {
      goto opJMP_AA_A;
   }
   RCregister_PC++;
   jumpCineRet_A;

 opJMI_BB_A:
   if (RCregister_B & 0x800) {
      goto opJMP_BB_A;
   }
   RCregister_PC++;
   jumpCineRet_A;

 opJMI_B_BB:
   if (RCregister_A & 0x800) {
      goto opJMP_B_BB;
   }
   RCregister_PC++;
   jumpCineRet_BB;

 opJLT_A_A:
 opJLT_AA_A:
 opJLT_BB_A:
   /* jump if old acc equals new acc */

   if (RCcmp_new < RCcmp_old) {
      goto opJMP_A_A;
   }

   RCregister_PC++;
   jumpCineRet_A;

 opJLT_B_BB:
   if (RCcmp_new < RCcmp_old) {
      goto opJMP_B_BB;
   }
   RCregister_PC++;
   jumpCineRet_BB;

 opJEQ_A_A:
 opJEQ_AA_A:
 opJEQ_BB_A:
   /* jump if equal */

   if (RCcmp_new == RCcmp_old) {
      goto opJMP_A_A;
   }
   RCregister_PC++;		/* bump PC */
   jumpCineRet_A;

 opJEQ_B_BB:

   if (RCcmp_new == RCcmp_old) {
      goto opJMP_B_BB;
   }
   RCregister_PC++;		/* bump PC */
   jumpCineRet_BB;

 opJA0_A_A:
 opJA0_AA_A:
 opJA0_BB_A:

   if (RCacc_a0 & 0x01) {
      goto opJMP_A_A;
   }

   RCregister_PC++;		/* bump PC */
   jumpCineRet_A;

 opJA0_B_BB:
   if (RCacc_a0 & 0x01) {
      goto opJMP_B_BB;
   }

   RCregister_PC++;		/* bump PC */
   jumpCineRet_BB;

 opJNC_A_A:
 opJNC_AA_A:
 opJNC_BB_A:

   if (!(GETFC () & 0xF0)) {
      goto opJMP_A_A;		/* no carry, so jump */
   }
   RCregister_PC++;
   jumpCineRet_A;

 opJNC_B_BB:
   if (!(GETFC () & 0xF0)) {
      goto opJMP_B_BB;		/* no carry, so jump */
   }
   RCregister_PC++;
   jumpCineRet_BB;

 opJDR_A_A:
 opJDR_AA_A:
 opJDR_BB_A:
   /* 
    * ; Calculate number of cycles executed since
    * ; last 'VDR' instruction, add two and use as
    * ; cycle count, never branch
    */
   RCregister_PC++;
   jumpCineRet_A;

 opJDR_B_BB:
   /* 
    * ; Calculate number of cycles executed since
    * ; last 'VDR' instruction, add two and use as
    * ; cycle count, never branch
    */
   RCregister_PC++;
   jumpCineRet_BB;

 opNOP_A_A:
 opNOP_AA_A:
 opNOP_BB_A:
   RCregister_PC++;
   jumpCineRet_A;

 opNOP_B_BB:
   RCregister_PC++;
   jumpCineRet_BB;

 opJPP32_A_B:
 opJPP32_AA_B:
 opJPP32_BB_B:
   /* ; 00 = Offset 0000h ; 01 = Offset 1000h ; 02 = Offset 2000h ; 03 =
      Offset 3000h ; 04 = Offset 4000h ; 05 = Offset 5000h ; 06 = Offset
      6000h ; 07 = Offset 7000h */
   temp_word = (RCregister_P & 0x07) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_B;

 opJPP32_B_BB:
   temp_word = (RCregister_P & 0x07) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_BB;

 opJPP16_A_B:
 opJPP16_AA_B:
 opJPP16_BB_B:
   /* ; 00 = Offset 0000h ; 01 = Offset 1000h ; 02 = Offset 2000h ; 03 = Offset 3000h */
   temp_word = (RCregister_P & 0x03) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_B;

 opJPP16_B_BB:
   temp_word = (RCregister_P & 0x03) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_BB;

 opJMP_A_B:
   RCregister_PC = (RCregister_PC & 0xF000) + RCregister_J;	/* pick up
								   new PC */
   jumpCineRet_B;

 opJPP8_A_B:
 opJPP8_AA_B:
 opJPP8_BB_B:
   /* "long jump"; combine P and J to jump to a new far location (that can be 
      more than 12 bits in address). After this jump, further jumps are local 
      to this new page. */
   temp_word = ((RCregister_P & 0x03) - 1) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_B;

 opJPP8_B_BB:
   temp_word = ((RCregister_P & 0x03) - 1) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_BB;

 opJMI_A_B:
   if (RCregister_A & 0x800) {
      goto opJMP_A_B;
   }
   RCregister_PC++;
   jumpCineRet_B;

 opJMI_AA_B:
   UNFINISHED ("opJMI 3\n");
   jumpCineRet_B;

 opJMI_BB_B:
   UNFINISHED ("opJMI 4\n");
   jumpCineRet_B;

 opJLT_A_B:
 opJLT_AA_B:
 opJLT_BB_B:
   if (RCcmp_new < RCcmp_old) {
      goto opJMP_A_B;
   }
   RCregister_PC++;
   jumpCineRet_B;

 opJEQ_A_B:
 opJEQ_AA_B:
 opJEQ_BB_B:
   if (RCcmp_new == RCcmp_old) {
      goto opJMP_A_B;
   }
   RCregister_PC++;		/* bump PC */
   jumpCineRet_B;

 opJA0_A_B:
 opJA0_AA_B:
 opJA0_BB_B:

   if (GETA0 () & 0x01) {
      goto opJMP_A_B;
   }
   RCregister_PC++;

   jumpCineRet_B;

 opJNC_A_B:
 opJNC_AA_B:
 opJNC_BB_B:

   if (!(GETFC () & 0x0F0)) {
      goto opJMP_A_B;		/* if no carry, jump */
   }
   RCregister_PC++;

   jumpCineRet_B;

 opJDR_A_B:
 opJDR_AA_B:
 opJDR_BB_B:
   /* RCregister_PC++; */

   jumpCineRet_B;

   /* NOP */
 opNOP_A_B:
 opNOP_AA_B:
 opNOP_BB_B:
   RCregister_PC++;		/* NOP; bump PC only */
   jumpCineRet_B;

 opLLT_A_AA:
 opLLT_AA_AA:
 opLLT_BB_AA:
   RCregister_PC++;
   temp_byte = 0;

 opLLTa1:
   temp_word = RCregister_A >> 8;	/* RCregister_A's high bits */
   temp_word &= 0x0A;		/* only want PA11 and PA9 */

   if (!temp_word) {
      goto opLLTa2;		/* zero, no mismatch */
   }

   temp_word ^= 0x0A;		/* flip the bits */

   if (temp_word) {
      goto opLLTa4;		/* if not zero, mismatch found */
   }

 opLLTa2:
   temp_word = RCregister_B >> 8;	/* regB's top bits */
   temp_word &= 0x0A;		/* only want SA11 and SA9 */

   if (!temp_word) {
      goto opLLTa3;		/* if zero, no mismatch */
   }

   temp_word ^= 0x0A;		/* flip bits */

   if (temp_word) {
      goto opLLTa4;		/* if not zero, mismatch found */
   }

 opLLTa3:
   RCregister_A <<= 1;		/* shift regA */
   RCregister_B <<= 1;		/* shift regB */

   temp_byte++;
   if (temp_byte) {
      goto opLLTa1;		/* try again */
   }
   jumpCineRet_AA;

 opLLTa4:
   RCvgShiftLength = temp_byte;
   RCregister_A &= 0xFFF;
   RCregister_B &= 0xFFF;

 opLLTaErr:
   jumpCineRet_AA;

 opLLT_B_AA:
   UNFINISHED ("opLLT 1\n");
   jumpCineRet_AA;

 opVIN_A_A:
 opVIN_AA_A:
 opVIN_BB_A:
   /* set the starting address of a vector */

   RCFromX = RCregister_A & 0xFFF;	/* regA goes to x-coord */
   RCFromY = RCregister_B & 0xFFF;	/* regB goes to y-coord */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_A;

 opVIN_B_BB:

   RCFromX = RCregister_A & 0xFFF;	/* regA goes to x-coord */
   RCFromY = RCregister_B & 0xFFF;	/* regB goes to y-coord */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_BB;

 opWAI_A_A:
 opWAI_AA_A:
 opWAI_BB_A:
   /* wait for a tick on the watchdog */
   bNewFrame = 1;
   RCregister_PC++;
   goto cineExit;

 opWAI_B_BB:
   bNewFrame = 1;
   RCregister_PC++;
   goto cineExit;

 opVDR_A_A:
 opVDR_AA_A:
 opVDR_BB_A:
   {
      int RCToX = SEX (RCregister_A & 0xFFF);
      int RCToY = SEX (RCregister_B & 0xFFF);

      RCFromX = SEX (RCFromX);
      RCFromY = SEX (RCFromY);

      /* figure out the vector */
      RCToX -= RCFromX;
      RCToX = SAR16 (RCToX, RCvgShiftLength);
      RCToX += RCFromX;

      RCToY -= RCFromY;
      RCToY = SAR16 (RCToY, RCvgShiftLength);
      RCToY += RCFromY;

      /* do orientation flipping, etc. */
      if (bFlipX) {
	 RCToX = sdwGameXSize - RCToX;
	 RCFromX = sdwGameXSize - RCFromX;
      }

      if (bFlipY) {
	 RCToY = sdwGameYSize - RCToY;
	 RCFromY = sdwGameYSize - RCFromY;
      }

      RCFromX += sdwXOffset;
      RCToX += sdwXOffset;

      RCFromY += sdwYOffset;
      RCToY += sdwYOffset;

      /* check real coords */
      if (bSwapXY) {
	 temp_word = RCToY;
	 RCToY = RCToX;
	 RCToX = temp_word;
	 temp_word = RCFromY;
	 RCFromY = RCFromX;
	 RCFromX = temp_word;
      }

      /* render the line */
      CinemaVectorData (RCFromX, RCFromY, RCToX, RCToY, RCvgColour);
   }

   RCregister_PC++;
   jumpCineRet_A;

 opVDR_B_BB:
   UNFINISHED ("opVDR B 1\n");
   jumpCineRet_BB;

/* some code needs to be changed based on the machine or switches set.
 * Instead of getting disorganized, I'll put the extra dispatchers
 * here. The main dispatch loop jumps here, checks options, and
 * redispatches to the actual opcode handlers.
 */

/* JPP series of opcodes
 */
 tJPP_A_B:
   /* MSIZE -- 0 = 4k, 1 = 8k, 2 = 16k, 3 = 32k */
   switch (ccpu_msize) {
   case 0:
   case 1:
      goto opJPP8_A_B;
      break;
   case 2:
      goto opJPP16_A_B;
      break;
   case 3:
      goto opJPP32_A_B;
      break;
   }

 tJPP_AA_B:
   /* MSIZE -- 0 = 4k, 1 = 8k, 2 = 16k, 3 = 32k */
   switch (ccpu_msize) {
   case 0:
   case 1:
      goto opJPP8_AA_B;
      break;
   case 2:
      goto opJPP16_AA_B;
      break;
   case 3:
      goto opJPP32_AA_B;
      break;
   }

 tJPP_B_BB:
   /* MSIZE -- 0 = 4k, 1 = 8k, 2 = 16k, 3 = 32k */
   switch (ccpu_msize) {
   case 0:
   case 1:
      goto opJPP8_B_BB;
      break;
   case 2:
      goto opJPP16_B_BB;
      break;
   case 3:
      goto opJPP32_B_BB;
      break;
   }

 tJPP_BB_B:
   /* MSIZE -- 0 = 4k, 1 = 8k, 2 = 16k, 3 = 32k */
   switch (ccpu_msize) {
   case 0:
   case 1:
      goto opJPP8_BB_B;
      break;
   case 2:
      goto opJPP16_BB_B;
      break;
   case 3:
      goto opJPP32_BB_B;
      break;
   }

/* JMI series of opcodes */

 tJMI_A_B:
   if (ccpu_jmi_dip) {
      goto opJMI_A_B;
   } else {
      goto opJEI_A_B;
   }

 tJMI_A_A:
   if (ccpu_jmi_dip) {
      goto opJMI_A_A;
   } else {
      goto opJEI_AA_B;
   }

 tJMI_AA_B:
   if (ccpu_jmi_dip) {
      goto opJMI_AA_B;
   } else {
      goto opJEI_AA_B;
   }

 tJMI_AA_A:
   if (ccpu_jmi_dip) {
      goto opJMI_AA_A;
   } else {
      goto opJEI_AA_A;
   }

 tJMI_B_BB1:
   if (ccpu_jmi_dip) {
      goto opJMI_B_BB;
   } else {
      goto opJEI_B_BB;
   }

 tJMI_B_BB2:
   if (ccpu_jmi_dip) {
      goto opJMI_B_BB;
   } else {
      goto opJEI_B_BB;
   }

 tJMI_BB_B:
   if (ccpu_jmi_dip) {
      goto opJMI_BB_B;
   } else {
      goto opJEI_BB_B;
   }

 tJMI_BB_A:
   if (ccpu_jmi_dip) {
      goto opJMI_BB_A;
   } else {
      goto opJEI_BB_A;
   }

/* OUT series of opcodes:
 * ccpu_monitor can be one of:
 * 1 -- 16-level colour
 * 2 -- 64-level colour
 * 3 -- War of the Worlds colour
 * other -- bi-level
 */
 tOUT_A_A:
   switch (ccpu_monitor) {
   case 1:
      goto opOUT16_A_A;
      break;
   case 2:
      goto opOUT64_A_A;
      break;
   case 3:
      goto opOUTWW_A_A;
      break;
   default:
      goto opOUTbi_A_A;
   }

   goto opOUTbi_A_A;

 tOUT_AA_A:
   switch (ccpu_monitor) {
   case 1:
      goto opOUT16_AA_A;
      break;
   case 2:
      goto opOUT64_AA_A;
      break;
   case 3:
      goto opOUTWW_AA_A;
      break;
   default:
      goto opOUTbi_AA_A;
   }

   goto opOUTbi_A_A;

 tOUT_B_BB:
   switch (ccpu_monitor) {
   case 1:
      goto opOUT16_B_BB;
      break;
   case 2:
      goto opOUT64_B_BB;
      break;
   case 3:
      goto opOUTWW_B_BB;
      break;
   default:
      goto opOUTbi_B_BB;
   }

   goto opOUTbi_A_A;

 tOUT_BB_A:
   switch (ccpu_monitor) {
   case 1:
      goto opOUT16_BB_A;
      break;
   case 2:
      goto opOUT64_BB_A;
      break;
   case 3:
      goto opOUTWW_BB_A;
      break;
   default:
      goto opOUTbi_BB_A;
   }

   goto opOUTbi_A_A;

   /* opcode exit point (so we can catch and do debugging, etc. */
 cineExecBottom:

   /* handle debugger */
   if (ccpudebug) {
      static char disassembly[256];	/* debugger buffer */

      disassemble (disassembly, sizeof (disassembly), original_PC);
      fprintf (stderr, "DEBUG: %s\n", disassembly);
   }

   /* the opcode code has set a RCstate and done mischief with flags and the
      programcounter; now jump back to the top and run through another
      opcode. */

 cineExit:
   /* return control to main system */
   dwElapsedTicks = 100000;	/* some magic number */

   return (0x80000000);
}

void vgSetRotateFlip (int rotate, int flipX, int flipY)
{
   DEBUG_OUT("vgSetRotateFlip( int rotate=%d, int flipX=%d, int flipY=%d);\n", rotate, flipX, flipY);
   v_rotate = rotate;
   v_flip_x = flipX;
   v_flip_y = flipY;
}

void cineInit (unsigned char *progdata, unsigned char *optkeymap)
{
  //#define SETTINGS_SIZE 1024
  // static unsigned char settingsBlob[SETTINGS_SIZE];

   DEBUG_OUT("cineInit( unsigned char *progdata, unsigned char *optkeymap );\n");

   // copy from progdata[] to rom[] NEEDS TO KNOW ROM SIZE
   {int i; for (i = 0; i < 0x8000; i++) rom[i] = progdata[i];}
   //fprintf(stderr, "Calling v_loadSettings\n");
   // v_loadSettings (ccpu_game_name, settingsBlob, SETTINGS_SIZE);
   v_setName(ccpu_game_name);
   //fprintf(stderr, "Returned from v_loadSettings\n");
   DEBUG_OUT("v_loadSettings(\n\"%s\"\n);\n", settingsBlob);
}

void vgInit (void)
{
}

void vgSetTwinkle (int aTwinkle)
{
   DEBUG_OUT("vgSetTwinkle( int aTwinkle=%d);\n", aTwinkle);
   cineTwinkle = aTwinkle;
}

void vgSetMonitor (int aMonitor, int RB, int GB, int BB, int RC, int GC,
		   int BC)
{
   DEBUG_OUT("vgSetMonitor( int aMonitor, int RB, int GB, int BB, int RC, int GC, int BC);\n");
}

void cineSetRate (unsigned int aRefresh)
{
   DEBUG_OUT("cineSetRate( unsigned int aRefresh=%d );\n", aRefresh);
   v_setRefresh (aRefresh);
}

void cineSetSw (unsigned int aSwitches, unsigned int aInputs)
{
   DEBUG_OUT("cineSetSw( unsigned int aSwitches=%0x, unsigned int aInputs=%0x); // TO DO!\n", aSwitches, aInputs);
   ioSwitches = aSwitches;
   ioInputs = aInputs;
}

void cineSetMouse (unsigned int aMouseType,
		   unsigned int aMouseSpeedX,
		   unsigned int aMouseSpeedY,
		   unsigned int aKeyMask, unsigned char *aMouseSeg)
{
   DEBUG_OUT("cineSetMouse( aMouseType=%d, ...);\n", aMouseType);
}

void vgSetMode (int mode)
{
   DEBUG_OUT("vgSetMode( int mode=%d);\n", mode);
}

void vgSetCineSize (int Xmin, int Ymin, int Xmax, int Ymax)
{
   DEBUG_OUT("vgSetCineSize( int Xmin=%d, int Ymin=%d, int Xmax=%d, int Ymax=%d);\n", Xmin, Ymin, Xmax, Ymax);
   world_xl = Xmin; world_yb = Ymin; world_xr = Xmax; world_yt = Ymax;
   window (Xmin, Ymin, Xmax, Ymax);
}

void vsetmode (unsigned int mode)
{
   DEBUG_OUT("vsetmode( unsigned int mode=%d );\n", mode);
}
