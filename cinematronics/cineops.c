#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "options.h"
#include <vectrex/osWrapper.h>

#ifdef NEVER
/*

// These are just here as a reference source while working on this file.

armor2:   Switches=1000000
armora:   Switches=1000000
armor:    Switches=1000000
armorp:   Switches=1000000
armorr:   Switches=1000000
barrier:  Switches=0000001
bbtest:   Switches=0001100
boxnbugs: Switches=0001100
demon2:   Switches=0000000
demon:    Switches=0000000
elettron: Switches=1000011
mottoeis: Switches=1000011
ripoff2:  Switches=0011101
ripoff:   Switches=0011101
solar:    Switches=1011000  Inputs=FFFF
spacewar: Switches=0000011  JMI=No
speedfrk: Switches=0000000  JMI=No  Inputs=FFE0     ; zero steering wheel, start in 1st gear
starcas3: Switches=1000011
starcas:  Switches=1000011
starhawk: Switches=0000000  Inputs=FFFF
sundance: Switches=0000101
tailg:    Switches=0000000  JMI=No ;Inputs=BFFF     ; uncomment to enable 8 way digital joystick mode
warrior:  Switches=1111111
wotw:     Switches=0000000

uchar  *OptKeyMap = 0;          // Key map array
uchar   OptMemSize = 1;         // 0=4k, 1=8k, 2=16k, 3=32k
uchar   OptJMI = 1;             // 0=JEI, 1=JMI
uchar   OptSpeed = 0;           // 0=Standard, 1=Full Speed
uchar   OptSwitches = 0x80;     // Bit0 = Switch 1, etc.
uint    OptInputs = 0xFFFFF;    // Initial Input values - probably should be FFFF or FFF ???
extern char   OptRomImages[8*128]; // Name of Rom images
extern char   OptStateFile[129];   // Name of CPU state file
int     OptWinXmin = 0;         // X Min size of Cine' window
int     OptWinYmin = 0;         // Y Min size of Cine' window
int     OptWinXmax = 1024;      // X Max size of Cine' window
int     OptWinYmax = 768;       // Y Max size of Cine' window
uchar   OptMonitor = 0;         // 0=BiLevel, 1=16Level, 2=64Level, 3=Color
uchar   OptTwinkle = 8;         // 1-9 Set twinkle level
uchar   OptRotate = 0;          // 0=Normal, 1=Rotate 90 degrees
uchar   OptFlipX = 0;           // 0=Don't flip, 1=Flip X image
uchar   OptFlipY = 0;           // 0=Don't flip, 1=Flip Y image
uchar   OptRedB = 100;          // RGB values for brightness
uchar   OptGreenB = 100;
uchar   OptBlueB = 100;
uchar   OptRedC = 67;           // RGB values for contrast
uchar   OptGreenC = 67;
uchar   OptBlueC = 67;
uchar  *OptMKeyMap = 0;         // Key map array
uchar   OptMouseType = 0;       // 0=None, 1=Tailgunner, 2=SpeedFreak, 3=BoxingBugs
uchar   OptMouseValid = 0;      // mask of valid mouse keys
uint    OptMouseSpeedX = 1024;  // mouse scaling
uint    OptMouseSpeedY = 1024;  // mouse scaling
*/
#endif

FILE *debugf = NULL;
#define DEBUG_OUT(...) do { if (!debugf) debugf = fopen("debug-cinema.log", "w"); if (debugf) { fprintf(debugf,__VA_ARGS__); fflush(debugf); } } while(0)

#include <vectrex/vectrexInterface.h>

#ifndef FALSE
#define FALSE (0!=0)
#define TRUE (0==0)
#endif

static int64_t ScaleXMul=1LL, ScaleXDiv=1LL, ScaleXOffsetPre=0LL, ScaleXOffsetPost=0LL,
               ScaleYMul=1LL, ScaleYDiv=1LL, ScaleYOffsetPre=0LL, ScaleYOffsetPost=0LL;
static int v_rotate = 0;
static int v_flip_x = 0;
static int v_flip_y = 0;
static int v_xl=0, v_yb=0, v_xr=0, v_yt=0;
int tx(int x) { // convert x from window to viewport
  if (v_flip_x) x = v_xr-(x-v_xl); 
  return (int)(((((int64_t)x)+ScaleXOffsetPre)*ScaleXMul)/ScaleXDiv + ScaleXOffsetPost);
}
int ty(int y) { // and y
  if (v_flip_y) y = v_yt-(y-v_yb); 
  return (int)(((((int64_t)y)+ScaleYOffsetPre)*ScaleYMul)/ScaleYDiv + ScaleYOffsetPost);
}
void line(int xl, int yb, int xr, int yt, int col) {
  //fprintf(stdout, "PRE:  line(%d,%d, %d,%d, %d);n", tx(xl),ty(yb), tx(xr),ty(yt), col);
  if (v_rotate) { int tmp; tmp = xl; xl = yb; yb = tmp; tmp = xr; xr = yt; yt = tmp; xl = v_xr-(xl-v_xl); xr = v_xr-(xr-v_xl); }
  //fprintf(stdout, "POST: line(%d,%d, %d,%d, %d);n", tx(xl),ty(yb), tx(xr),ty(yt), col);
  v_directDraw32(tx(xl),ty(yb), tx(xr),ty(yt), col);
}
void window(int xl, int yb, int xr, int yt) {
  // We will use normalised viewport coordinates of x: -18000 : 18000 and y: -24000 : 24000
  int64_t width, height;
  int xc, yc;
  int oxl = xl, oyb = yb, oxr = xr, oyt = yt;
  v_xl=xl; v_yb=yb; v_xr=xr; v_yt=yt;
  
  width = (int64_t)xr-(int64_t)xl;
  height = (int64_t)yt-(int64_t)yb;
  if (width*4 > height*3) {
    // window is wider than aspect ratio, so we will have black bars at the top and bottom
    height = (width * 4) / 3;
    yc = (yb+yt)/2;
    yb = yc - height/2;
    yt = yc + height/2;
  } else if (width*4 < height*3) {
    // window is taller than aspect ratio, so we will have black bars at the sides
    width = (height*3) / 4;
    xc = (xl+xr)/2;
    xl = xc - width/2;
    xr = xc + width/2;
  }
  ScaleXMul = 36000LL; ScaleXDiv = width; ScaleXOffsetPre = -width/2LL; ScaleXOffsetPost = 0LL; ScaleXOffsetPost = (tx(xr) - tx(oxr)) / 2LL;
  ScaleYMul = 48000LL; ScaleYDiv = height; ScaleYOffsetPre = -height/2LL; ScaleYOffsetPost = 0LL; ScaleYOffsetPost = (ty(yt) - ty(oyt)) / 2LL;
  // setCustomClipping(TRUE, tx(oxl), ty(oyb), tx(oxr), ty(oyt)); // transform world (window) coordinates to viewport (normalised device coordinates) before
                                                                  // clipping.  That way clipping code does not need to know about world coordinates.
  // not implemented as I don't have the updated library yet
}

//int tracing = TRUE;
//int debug = TRUE;
//int onscreen_debug = FALSE;
int ccpudebug = 0; /* default is off (0) */

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
/* Handy new operators ...
 */

/* for Zonn translation :) */
#define SAR16(var,arg)    ( ( (signed short int) var ) >> arg )

/* for setting/checking the A0 flag */
#define SETA0(var)    ( RCacc_a0 = var )
#define GETA0()       ( RCacc_a0 )

/* for setting/checking the Carry flag */
#define SETFC(val)    ( RCflag_C = val )
#define GETFC()       ( ( RCflag_C >> 8 ) & 0xFF )

/* Define new types for the c-cpu emulator
 */

#define SW_ABORT           0x100          /* for ioSwitches */
#define SW_COIN            0x080          /* for ioSwitches */

typedef short unsigned int RCCINEWORD;      /* 12bits on the C-CPU */
typedef unsigned char      RCCINEBYTE;      /* 8 (or less) bits on the C-CPU */
typedef short signed int   RCCINESWORD;     /* 12bits on the C-CPU */
typedef signed char        RCCINESBYTE;     /* 8 (or less) bits on the C-CPU */

typedef enum {
  RCstate_A = 0, RCstate_AA, RCstate_B, RCstate_BB
} RCCINESTATE;                              /* current RCstate */

#define UINT32 unsigned int
#define UINT16 unsigned short int
#define UINT8  unsigned char

#define INT32  signed int
#define INT16  signed short int
#define INT8   signed char

volatile static UINT32 dwElapsedTicks = 0;
static UINT8 bBailOut = FALSE;

/* C-CPU context information begins --
 */
RCCINEWORD RCregister_PC = 0; /* C-CPU registers; program counter */
RCCINEWORD RCregister_A = 0;  /* ; A-Register (accumulator) */
RCCINEWORD RCregister_B = 0;  /* ; B-Register (accumulator) */
RCCINEBYTE RCregister_I = 0;  /* ; I-Register (last access RAM location) */
RCCINEWORD RCregister_J = 0;  /* ; J-Register (target address for JMP opcodes) */
RCCINEBYTE RCregister_P = 0;  /* ; Page-Register (4 bits) */
RCCINEWORD RCFromX = 0;       /* ; X-Register (start of a vector) */
RCCINEWORD RCFromY = 0;       /* ; Y-Register (start of a vector) */
RCCINEWORD RCregister_T = 0;  /* ; T-Register (vector draw length timer) */
RCCINEWORD RCflag_C = 0;      /* C-CPU flags; carry. Is word sized, instead
                           * of RCCINEBYTE, so we can do direct assignment
                           * and then change to BYTE during inspection.
                           */

RCCINEWORD RCcmp_old = 0;     /* last accumulator value */
RCCINEWORD RCcmp_new = 0;     /* new accumulator value */
RCCINEBYTE RCacc_a0 = 0;      /* bit0 of A-reg at last accumulator access */

RCCINESTATE RCstate = RCstate_A; /* C-CPU RCstate machine current RCstate */
RCCINEWORD RCram [ 256 ];     /* C-CPU RCram (for all pages) */


static UINT8 rom[0x8000] ; //=  tailgunner was 0x2000 - others may be larger

static char  ccpu_game[32];     /* canonical name we now use as cinemu parameter */
static UINT8 ccpu_jmi_dip = 1;  /* as set by cineSetJMI */
static UINT8 ccpu_msize = 0;    /* as set by cineSetMSize */
static UINT8 ccpu_monitor = 0;  /* as set by cineSetMonitor */

/* cinem graphics junk */
RCCINEBYTE RCvgShiftLength = 0; /* number of shifts loaded into length reg */

/* -- Context information ends.
 */

UINT8 bFlipX;
UINT8 bFlipY;
UINT8 bSwapXY;
UINT8 bOverlay;

UINT16 ioSwitches = 0xFF;  /* high values for non-triggered switches */
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

/* Define new types for the c-cpu emulator */
typedef short unsigned int CINEWORD;      /* 12bits on the C-CPU */
typedef unsigned char      CINEBYTE;      /* 8 (or less) bits on the C-CPU */
typedef short signed int   CINESWORD;     /* 12bits on the C-CPU */
typedef signed char        CINESBYTE;     /* 8 (or less) bits on the C-CPU */
typedef unsigned long int  CINELONG;

typedef enum
{
	state_A = 0,
	state_AA,
	state_B,
	state_BB
} CINESTATE;

  /* C-CPU context information begins --  */
           CINEWORD  register_PC = 0;  /* C-CPU registers; program counter */
  /*register*/ CINEWORD  register_A = 0;   /* A-Register (accumulator) */
  /*register*/ CINEWORD  register_B = 0;   /* B-Register (accumulator) */
           CINEWORD /*CINEBYTE*/  register_I = 0;   /* I-Register (last access RAM location) */
           CINEWORD  register_J = 0;   /* J-Register (target address for JMP opcodes) */
           CINEWORD /*CINEBYTE*/  register_P = 0;   /* Page-Register (4 bits, shifts to high short nibble for code, hight byte nibble for ram) */
           CINEWORD  FromX = 0;        /* X-Register (start of a vector) */
           CINEWORD  FromY = 0;        /* Y-Register (start of a vector) */
           CINEWORD  register_T = 0;   /* T-Register (vector draw length timer) */
           CINEWORD  flag_C = 0;       /* C-CPU flags; carry. Is word sized, instead
                                        * of CINEBYTE, so we can do direct assignment
                                        * and then change to BYTE during inspection.
                                        */

           CINEWORD  cmp_old = 0;      /* last accumulator value */
           CINEWORD  cmp_new = 0;      /* new accumulator value */
           CINEWORD/*CINEBYTE*/  acc_a0 = 0;       /* bit0 of A-reg at last accumulator access */

           CINESTATE state = state_A;  /* C-CPU state machine current state */
           CINEWORD  ram[256];         /* C-CPU ram (for all pages) */

           CINEWORD  vgColour = 0;
           CINEBYTE  vgShiftLength = 0;   /* number of shifts loaded into length reg */
           int bailOut = 0;
           int       ccpu_ICount = 0;  /* */

  /* -- Context information ends. */

//static int Frames = 0;

// All the junk below comes from tailgunner.  We need to replace it and update all the variables
// with data from the ini files.


/* INP $8 ? */
#define IO_START   0x80

/* INP $7 ? */
#define IO_SHIELDS 0x40

#define IO_FIRE    0x20
#define IO_DOWN    0x10
#define IO_UP      0x08
#define IO_LEFT    0x04
#define IO_RIGHT   0x02

#define IO_COIN   0x80

#define IO_KNOWNBITS (IO_START | IO_SHIELDS | IO_FIRE | IO_DOWN | IO_LEFT | IO_RIGHT)

/* initial value of shields (on a DIP) */
#define SW_SHIELDS80 ((1<<1) | (1<<4) | (1<<5))
#define SW_SHIELDS40 ((0<<1) | (1<<4) | (1<<5))
#define SW_SHIELDS15 ((0<<1) | (0<<4) | (0<<5))
#define SW_SHIELDS    SW_SHIELDS80

static int startflag = IO_START;
static int coinflag  = 0;
static int shieldsflag = IO_SHIELDS;   /* Whether shields are up (mouse or key) */
static int fireflag = IO_FIRE;

void reset_coin_counter(int RCstate)
{
  /* RCstate = 0 or 1 */
  coinflag = IO_COIN;
}
int get_coin_state(void)
{
  return((coinflag >> 7) & 1);
}

int get_quarters_per_game(void)
{
/*   Bit 0 of switches = 1:  1 Quarter per game
     Bit 0 of switches = 0:  2 Quarters per game  ... these don't match the documentation!
 */
  return(1);
}

void set_watchdog(void)
{
}

//const int switches = SW_SHIELDS;  /* These don't change during a game */
//static int inputs = /* These two seem wrong but are in the comparison trace 0x10 | 0x08 | */ /*1*/0;

int get_shield_bit0(void)
{
  return(SW_SHIELDS&1);
}

int get_shield_bit1(void)
{
  return((SW_SHIELDS>>1)&1);
}

int get_shield_bit2(void)
{
  return((SW_SHIELDS>>2)&1);
}

int get_switch_bit(int n)
{
  return((ioSwitches >> n) & 1);
}

/* Mouse values currently sampled once per frame and set in WAI code.
   This is not ideal. */

const int xmousethresh = 2;
const int ymousethresh = 2;
static int mickeyx = 0;
static int mickeyy = 0;
static int mousecode = 0;

static int sound_addr = 0;
static int sound_addr_A = 0;
static int sound_addr_B = 0;
static int sound_addr_C = 0;
static int sound_data = 0;

void set_sound_data(int bit)
{
  sound_data = bit;
}

void set_sound_addr_A(int bit)
{
  sound_addr_A = bit;
}

void set_sound_addr_B(int bit)
{
  sound_addr_B = bit;
}

void set_sound_addr_C(int bit)
{
  sound_addr_C = bit;
}

/* These don't yet do anything obvious or useful.  Debugging needed... */
void strobe_sound_on(void)
{
  int sound_addr_tmp = (sound_addr_C<<2) | (sound_addr_B<<1) | sound_addr_A;
  sound_addr = sound_addr_tmp;
}

void strobe_sound_off(void)
{
  int sound_addr_tmp = (sound_addr_C<<2) | (sound_addr_B<<1) | sound_addr_A;
  sound_addr = sound_addr_tmp;
}
#ifndef FREESTANDING
FILE *dumpfile;
void PUTBYTE(int c)
{
  fputc(c, dumpfile);
}
void PUTWORD(int c)
{
  fputc(c>>8, dumpfile);
  fputc(c, dumpfile);
}
int xGETBYTE(void)
{
  return(fgetc(dumpfile));
}
#define GETBYTE(varname) varname = xGETBYTE()
int xGETWORD(void)
{
  return((fgetc(dumpfile)<<8) | fgetc(dumpfile));
}
#define GETWORD(varname) varname = xGETWORD()

#define GETSTATE GETBYTE
#define PUTSTATE PUTBYTE

void save_config(void)
{
  int i;
  dumpfile = fopen("tailgunr.dmp", "wb");
  if (dumpfile == NULL) return;
  PUTWORD(RCregister_PC); /* C-CPU registers; program counter */
  PUTWORD(RCregister_A);  /* ; A-Register (accumulator) */
  PUTWORD(RCregister_B);  /* ; B-Register (accumulator) */
  PUTBYTE(RCregister_I);  /* ; I-Register (last access RAM location) */
  PUTWORD(RCregister_J);  /* ; J-Register (target address for JMP opcodes) */
  PUTBYTE(RCregister_P);  /* ; Page-Register (4 bits) */
  PUTWORD(RCFromX);       /* ; X-Register (start of a vector) */
  PUTWORD(RCFromY);       /* ; Y-Register (start of a vector) */
  PUTWORD(RCregister_T);  /* ; T-Register (vector draw length timer) */
  PUTWORD(RCflag_C);      /* C-CPU flags; carry. Is word sized, instead
                         * of RCCINEBYTE, so we can do direct assignment
                         * and then change to BYTE during inspection.
                         */

  PUTWORD(RCcmp_old);     /* last accumulator value */
  PUTWORD(RCcmp_new);     /* new accumulator value */
  PUTBYTE(RCacc_a0);      /* bit0 of A-reg at last accumulator access */

  PUTSTATE(RCstate); /* C-CPU RCstate machine current RCstate */
  for (i = 0; i < 256; i++) {
    PUTWORD(RCram[i]);     /* C-CPU RCram (for all pages) */
  }
  PUTBYTE(RCvgShiftLength); /* number of shifts loaded into length reg */
  fclose(dumpfile);
}

int load_config(void)
{
  int i;
  dumpfile = fopen("tailgunr.dmp", "rb");
  if (dumpfile == NULL) return(FALSE);
  GETWORD(RCregister_PC); /* C-CPU registers; program counter */
  GETWORD(RCregister_A);  /* ; A-Register (accumulator) */
  GETWORD(RCregister_B);  /* ; B-Register (accumulator) */
  GETBYTE(RCregister_I);  /* ; I-Register (last access RAM location) */
  GETWORD(RCregister_J);  /* ; J-Register (target address for JMP opcodes) */
  GETBYTE(RCregister_P);  /* ; Page-Register (4 bits) */
  GETWORD(RCFromX);       /* ; X-Register (start of a vector) */
  GETWORD(RCFromY);       /* ; Y-Register (start of a vector) */
  GETWORD(RCregister_T);  /* ; T-Register (vector draw length timer) */
  GETWORD(RCflag_C);      /* C-CPU flags; carry. Is word sized, instead
                         * of RCCINEBYTE, so we can do direct assignment
                         * and then change to BYTE during inspection.
                         */

  GETWORD(RCcmp_old);     /* last accumulator value */
  GETWORD(RCcmp_new);     /* new accumulator value */
  GETBYTE(RCacc_a0);      /* bit0 of A-reg at last accumulator access */

  GETSTATE(RCstate); /* C-CPU RCstate machine current RCstate */
  for (i = 0; i < 256; i++) {
    GETWORD(RCram[i]);     /* C-CPU RCram (for all pages) */
  }
  GETBYTE(RCvgShiftLength); /* number of shifts loaded into length reg */
  fclose(dumpfile);
  return(TRUE);
}
#endif

void startFrame()
{
  static int counter=0;
  DEBUG_OUT( "v_WaitRecal(); v_setBrightness(64);v_readButtons();v_readJoystick1Analog(); // %d\n",++counter);
  v_WaitRecal();
  //v_doSound();
  v_readButtons();
  v_readJoystick1Analog();
  //v_playAllSFX();
}


char disassembly[ 256 ]; /* debugger buffer */
void CinemaClearScreen(void)
{
  static int parity = 0, slowdown = 0;
  static int second = 0;
  int mx, my;
  parity ^= 1;
  if (parity) { // this may also be for tailgunner only...

    /*fprintf(stderr, "CLEAR: %d\n", Frames++);*/
    startFrame();
  } else {
    /* Doing every second vsync() here makes the speed match that
       of Retrocade (and I presume the original?) almost exactly. */
  }
}

static int MinX=1000, MinY=1000, MaxX=0, MaxY=0;
void xCinemaVectorData(int RCFromX, int RCFromY, int RCToX, int RCToY, int vgColour, int RCregister_PC)
{
  /*Remove stars: if ((RCFromX == RCToX) && (RCFromY == RCToY)) return;*/
  RCFromX = SEX(RCFromX);
  RCFromY = SEX(RCFromY);
  RCToX = SEX(RCToX);
  RCToY = SEX(RCToY);

  //if (debugf) fprintf(debugf, "LINE: From (%d,%d) To (%d,%d) Col %d [0x%04x]\n",
  //        RCFromX, RCFromY, RCToX, RCToY, vgColour, RCregister_PC);

  //RCFromX = (RCFromX * 2) / 3; RCToX = (RCToX * 2) / 3;
  //RCFromY = (RCFromY * 2) / 3; RCToY = (RCToY * 2) / 3;
  //RCFromX -= 32; RCToX -= 32;
  //RCFromY = SCREEN_H-RCFromY; RCToY=SCREEN_H-RCToY;

// /* HACK */ if (vgColour & 0x8) vgColour = 0xf; else vgColour = 0x7;

  line(RCFromX, RCFromY, RCToX, RCToY, 127 /* vgColour * 8 */);
  //v_directDraw32((RCFromX-256)*32, (RCFromY-(512+256))*32, (RCToX-256)*32,(RCToY-(512+256))*32, 127 /* vgColour * 8 */);

  if (RCFromX < MinX) MinX = RCFromX;
  if (RCToX < MinX) MinX = RCToX;
  if (RCFromY < MinY) MinY = RCFromY;
  if (RCToY < MinY) MinY = RCToY;
  if (RCFromX > MaxX) MaxX = RCFromX;
  if (RCToX > MaxX) MaxX = RCToX;
  if (RCFromY > MaxY) MaxY = RCFromY;
  if (RCToY > MaxY) MaxY = RCToY;
}
#define CinemaVectorData(RCFromX, RCFromY, RCToX, RCToY, vgColour) xCinemaVectorData(RCFromX, RCFromY, RCToX, RCToY, vgColour, RCregister_PC)

void xUNFINISHED(char *s, int RCregister_PC)
{
  DEBUG_OUT( "%04x DANGER: Unimplemented instruction - %s\n", RCregister_PC, s);
  exit(1);
}
#define UNFINISHED(s) xUNFINISHED(s, RCregister_PC)

/* Reset C-CPU registers, flags, etc to default starting values
 */
void cineReset(void) {

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

  /* reset RCram*/
  memset(RCram, 0, sizeof(RCram));

  /* reset internal RCstate */
  RCcmp_old = 0;
  RCcmp_new = 0;
  SETA0 ( 0 );

}

void cineSetGameName(char *name) {
  strcpy(ccpu_game, name);
}

void cineSetJMI(UINT8 j) {
  ccpu_jmi_dip = j;
  DEBUG_OUT("cineSetJMI(%u);\n", j);
}

void cineSetMSize(UINT8 m) {
  ccpu_msize = m;
  DEBUG_OUT("cineSetMSize(%u);\n", m);
}

void cineSetMonitor(UINT8 m) {
  ccpu_monitor = m;
  DEBUG_OUT("cineSetMonitor(%u);\n", m);
}

UINT32 cineGetElapsedTicks ( UINT32 dwClearIt ) {
  UINT32 dwTemp;

  dwTemp = dwElapsedTicks;
  if ( dwClearIt ) {
         dwElapsedTicks = 0;
  }

  return ( dwTemp );
}

void cineReleaseTimeslice ( void ) {
  bBailOut = TRUE;
}

/* extra functions
 */

/* ----------------------------------------------- Trace debugger cinedbg.c */

/* handy macros
 */

void disassemble ( char *buffer, unsigned int maxlen, unsigned int address );
#define START(x) DEBUG_OUT("Entered C-CPU function: %s\n", x )
#define STOP(x) DEBUG_OUT("Left C-CPU function: %s\n", x )
/* opcode tables for debugging
 */
typedef enum {
  CLR,  LDA,  INP,  ADD,  SUB,  LDJ,  LDP,  OUT,
  CMP,  LDI,  STA,  VIN,  VDR,  XLT,  MUL,  LLT,
  WAI,  AWD,  AND,  LSR,  LSL,  ASR,  ASRD, LSLD,
  JPPB, JMIB, JDRB, JLTB, JEQB, JNCB, JAOB, NOPB,
  JMPA, JMIA, JDRA, JLTA, JEQA, JNCA, JAOA, NOPA,
  JEIA, JEIB
} opcode_mnemonics;

typedef struct {
  opcode_mnemonics od_opcode;
  char *od_name;
} opcode_detail;

opcode_detail opcodes[] = {
  { CLR, "clr" },
  { LDA, "lda" },
  { INP, "inp" },
  { ADD, "add" },
  { SUB, "sub" },
  { LDJ, "ldj" },
  { LDP, "ldp" },
  { OUT, "out" },
  { CMP, "cmp" },
  { LDI, "ldi" },
  { STA, "sta" },
  { VIN, "vin" },
  { VDR, "vdr" },
  { XLT, "xlt" },
  { MUL, "mul" },
  { LLT, "llt" },
  { WAI, "wai" },
  { AWD, "awd" },
  { AND, "and" },
  { LSR, "lsr" },
  { LSL, "lsl" },
  { ASR, "asr" },
  { ASRD, "asrd" },
  { LSLD, "lsld" },
  { JPPB, "jppb" },
  { JMIB, "jmib" },
  { JDRB, "jdrb" },
  { JLTB, "jltb" },
  { JEQB, "jeqb" },
  { JNCB, "jncb" },
  { JAOB, "jaob" },
  { NOPB, "nopb" },
  { JMPA, "jmpa" },
  { JMIA, "jmia" },
  { JDRA, "jdra" },
  { JLTA, "jlta" },
  { JEQA, "jeqa" },
  { JNCA, "jnca" },
  { JAOA, "jaoa" },
  { NOPA, "nopa" },
  { JEIA, "jeia" },
  { JEIB, "jeib" }
};

typedef enum {
  ACC,          /* Accumulator */
  ADIR,         /* Acc Direct memory access */
  AIM4,         /* Acc 4 bit immediate */
  AIM4X,        /* Acc 4 bit immediate extended size */
  AIM8,         /* Acc 8 bit immediate */
  AINDM,        /* Acc indirect through memory */
  AIMX,         /* Acc immediate extended A-reg */
  AXLT,         /* Acc lookup ROM using Acc as pointer */
  AIRG,         /* Acc Through the I-reg */
  IRG,          /* Through the I-reg */
  IM4,          /* 4 bit immediate */
  IM12,         /* 12 bit immediate */
  DIRECT,          /* Direct memory access */
  IMP,          /* Implied */
  JUMP,         /* Acc selection/Jump instruction */
  JUMPX         /* Acc selection/Extended jump instruction */
} amode_mnemonics;

typedef struct {
  amode_mnemonics ad_amode;
  char *ad_name;
} amode_detail;

amode_detail amodes[] = {
  { ACC,   "acc" },
  { ADIR,  "adir" },
  { AIM4,  "aim4" },
  { AIM4X, "aim4x" },
  { AIM8,  "aim8" },
  { AINDM, "aindm" },
  { AIMX,  "aimx" },
  { AXLT,  "axlt" },
  { AIRG,  "airg" },
  { IRG,   "irg" },
  { IM4,   "im4" },
  { IM12,  "im12" },
  { DIRECT,   "dir" },
  { IMP,   "imp" },
  { JUMP,  "jump" },
  { JUMPX, "jumpx" }
};

typedef struct {
  opcode_mnemonics ot_opcode;
  amode_mnemonics  ot_amode;
} opcode_table_entry;

opcode_table_entry opcode_table[] = {
  { CLR, ACC   },                       /* 00 */
  { LDA, AIM4X },                       /* 01 */
  { LDA, AIM4X },                       /* 02 */
  { LDA, AIM4X },                       /* 03 */
  { LDA, AIM4X },                       /* 04 */
  { LDA, AIM4X },                       /* 05 */
  { LDA, AIM4X },                       /* 06 */
  { LDA, AIM4X },                       /* 07 */
  { LDA, AIM4X },                       /* 08 */
  { LDA, AIM4X },                       /* 09 */
  { LDA, AIM4X },                       /* 0A */
  { LDA, AIM4X },                       /* 0B */
  { LDA, AIM4X },                       /* 0C */
  { LDA, AIM4X },                       /* 0D */
  { LDA, AIM4X },                       /* 0E */
  { LDA, AIM4X },                       /* 0F */

  { INP, ADIR },                        /* 10 */
  { INP, ADIR },                        /* 11 */
  { INP, ADIR },                        /* 12 */
  { INP, ADIR },                        /* 13 */
  { INP, ADIR },                        /* 14 */
  { INP, ADIR },                        /* 15 */
  { INP, ADIR },                        /* 16 */
  { INP, ADIR },                        /* 17 */
  { INP, ADIR },                        /* 18 */
  { INP, ADIR },                        /* 19 */
  { INP, ADIR },                        /* 1A */
  { INP, ADIR },                        /* 1B */
  { INP, ADIR },                        /* 1C */
  { INP, ADIR },                        /* 1D */
  { INP, ADIR },                        /* 1E */
  { INP, ADIR },                        /* 1F */

  { ADD, AIM8 },                        /* 20 */
  { ADD, AIM4 },                        /* 21 */
  { ADD, AIM4 },                        /* 22 */
  { ADD, AIM4 },                        /* 23 */
  { ADD, AIM4 },                        /* 24 */
  { ADD, AIM4 },                        /* 25 */
  { ADD, AIM4 },                        /* 26 */
  { ADD, AIM4 },                        /* 27 */
  { ADD, AIM4 },                        /* 28 */
  { ADD, AIM4 },                        /* 29 */
  { ADD, AIM4 },                        /* 2A */
  { ADD, AIM4 },                        /* 2B */
  { ADD, AIM4 },                        /* 2C */
  { ADD, AIM4 },                        /* 2D */
  { ADD, AIM4 },                        /* 2E */
  { ADD, AIM4 },                        /* 2F */

  { SUB, AIM8 },                        /* 30 */
  { SUB, AIM4 },                        /* 31 */
  { SUB, AIM4 },                        /* 32 */
  { SUB, AIM4 },                        /* 33 */
  { SUB, AIM4 },                        /* 34 */
  { SUB, AIM4 },                        /* 35 */
  { SUB, AIM4 },                        /* 36 */
  { SUB, AIM4 },                        /* 37 */
  { SUB, AIM4 },                        /* 38 */
  { SUB, AIM4 },                        /* 39 */
  { SUB, AIM4 },                        /* 3A */
  { SUB, AIM4 },                        /* 3B */
  { SUB, AIM4 },                        /* 3C */
  { SUB, AIM4 },                        /* 3D */
  { SUB, AIM4 },                        /* 3E */
  { SUB, AIM4 },                        /* 3F */

  { LDJ, IM12 },                        /* 40 */
  { LDJ, IM12 },                        /* 41 */
  { LDJ, IM12 },                        /* 42 */
  { LDJ, IM12 },                        /* 43 */
  { LDJ, IM12 },                        /* 44 */
  { LDJ, IM12 },                        /* 45 */
  { LDJ, IM12 },                        /* 46 */
  { LDJ, IM12 },                        /* 47 */
  { LDJ, IM12 },                        /* 48 */
  { LDJ, IM12 },                        /* 49 */
  { LDJ, IM12 },                        /* 4A */
  { LDJ, IM12 },                        /* 4B */
  { LDJ, IM12 },                        /* 4C */
  { LDJ, IM12 },                        /* 4D */
  { LDJ, IM12 },                        /* 4E */
  { LDJ, IM12 },                        /* 4F */

  { JPPB, JUMP },                       /* 50 */
  { JMIB, JUMP },                       /* 51 */
  { JDRB, JUMP },                       /* 52 */
  { JLTB, JUMP },                       /* 53 */
  { JEQB, JUMP },                       /* 54 */
  { JNCB, JUMP },                       /* 55 */
  { JAOB, JUMP },                       /* 56 */
  { NOPB, IMP },                        /* 57 */

  { JMPA, JUMP },                       /* 58 */
  { JMIA, JUMP },                       /* 59 */
  { JDRA, JUMP },                       /* 5A */
  { JLTA, JUMP },                       /* 5B */
  { JEQA, JUMP },                       /* 5C */
  { JNCA, JUMP },                       /* 5D */
  { JAOA, JUMP },                       /* 5E */
  { NOPA, IMP },                        /* 5F */

  { ADD, ADIR },                        /* 60 */
  { ADD, ADIR },                        /* 61 */
  { ADD, ADIR },                        /* 62 */
  { ADD, ADIR },                        /* 63 */
  { ADD, ADIR },                        /* 64 */
  { ADD, ADIR },                        /* 65 */
  { ADD, ADIR },                        /* 66 */
  { ADD, ADIR },                        /* 67 */
  { ADD, ADIR },                        /* 68 */
  { ADD, ADIR },                        /* 69 */
  { ADD, ADIR },                        /* 6A */
  { ADD, ADIR },                        /* 6B */
  { ADD, ADIR },                        /* 6C */
  { ADD, ADIR },                        /* 6D */
  { ADD, ADIR },                        /* 6E */
  { ADD, ADIR },                        /* 6F */

  { SUB, ADIR },                        /* 70 */
  { SUB, ADIR },                        /* 71 */
  { SUB, ADIR },                        /* 72 */
  { SUB, ADIR },                        /* 73 */
  { SUB, ADIR },                        /* 74 */
  { SUB, ADIR },                        /* 75 */
  { SUB, ADIR },                        /* 76 */
  { SUB, ADIR },                        /* 77 */
  { SUB, ADIR },                        /* 78 */
  { SUB, ADIR },                        /* 79 */
  { SUB, ADIR },                        /* 7A */
  { SUB, ADIR },                        /* 7B */
  { SUB, ADIR },                        /* 7C */
  { SUB, ADIR },                        /* 7D */
  { SUB, ADIR },                        /* 7E */
  { SUB, ADIR },                        /* 7F */

  { LDP, IM4  },                        /* 80 */
  { LDP, IM4  },                        /* 81 */
  { LDP, IM4  },                        /* 82 */
  { LDP, IM4  },                        /* 83 */
  { LDP, IM4  },                        /* 84 */
  { LDP, IM4  },                        /* 85 */
  { LDP, IM4  },                        /* 86 */
  { LDP, IM4  },                        /* 87 */
  { LDP, IM4  },                        /* 88 */
  { LDP, IM4  },                        /* 89 */
  { LDP, IM4  },                        /* 8A */
  { LDP, IM4  },                        /* 8B */
  { LDP, IM4  },                        /* 8C */
  { LDP, IM4  },                        /* 8D */
  { LDP, IM4  },                        /* 8E */
  { LDP, IM4  },                        /* 8F */

  { OUT, ADIR },                        /* 90 */
  { OUT, ADIR },                        /* 91 */
  { OUT, ADIR },                        /* 92 */
  { OUT, ADIR },                        /* 93 */
  { OUT, ADIR },                        /* 94 */
  { OUT, ADIR },                        /* 95 */
  { OUT, ADIR },                        /* 96 */
  { OUT, ADIR },                        /* 97 */
  { OUT, ADIR },                        /* 98 */
  { OUT, ADIR },                        /* 99 */
  { OUT, ADIR },                        /* 9A */
  { OUT, ADIR },                        /* 9B */
  { OUT, ADIR },                        /* 9C */
  { OUT, ADIR },                        /* 9D */
  { OUT, ADIR },                        /* 9E */
  { OUT, ADIR },                        /* 9F */

  { LDA, ADIR },                        /* A0 */
  { LDA, ADIR },                        /* A1 */
  { LDA, ADIR },                        /* A2 */
  { LDA, ADIR },                        /* A3 */
  { LDA, ADIR },                        /* A4 */
  { LDA, ADIR },                        /* A5 */
  { LDA, ADIR },                        /* A6 */
  { LDA, ADIR },                        /* A7 */
  { LDA, ADIR },                        /* A8 */
  { LDA, ADIR },                        /* A9 */
  { LDA, ADIR },                        /* AA */
  { LDA, ADIR },                        /* AB */
  { LDA, ADIR },                        /* AC */
  { LDA, ADIR },                        /* AD */
  { LDA, ADIR },                        /* AE */
  { LDA, ADIR },                        /* AF */

  { CMP, ADIR },                        /* B0 */
  { CMP, ADIR },                        /* B1 */
  { CMP, ADIR },                        /* B2 */
  { CMP, ADIR },                        /* B3 */
  { CMP, ADIR },                        /* B4 */
  { CMP, ADIR },                        /* B5 */
  { CMP, ADIR },                        /* B6 */
  { CMP, ADIR },                        /* B7 */
  { CMP, ADIR },                        /* B8 */
  { CMP, ADIR },                        /* B9 */
  { CMP, ADIR },                        /* BA */
  { CMP, ADIR },                        /* BB */
  { CMP, ADIR },                        /* BC */
  { CMP, ADIR },                        /* BD */
  { CMP, ADIR },                        /* BE */
  { CMP, ADIR },                        /* BF */

  { LDI, DIRECT  },                     /* C0 */
  { LDI, DIRECT  },                        /* C1 */
  { LDI, DIRECT  },                        /* C2 */
  { LDI, DIRECT  },                        /* C3 */
  { LDI, DIRECT  },                        /* C4 */
  { LDI, DIRECT  },                        /* C5 */
  { LDI, DIRECT  },                        /* C6 */
  { LDI, DIRECT  },                        /* C7 */
  { LDI, DIRECT  },                        /* C8 */
  { LDI, DIRECT  },                        /* C9 */
  { LDI, DIRECT  },                        /* CA */
  { LDI, DIRECT  },                        /* CB */
  { LDI, DIRECT  },                        /* CC */
  { LDI, DIRECT  },                        /* CD */
  { LDI, DIRECT  },                        /* CE */
  { LDI, DIRECT  },                        /* CF */

  { STA, ADIR },                        /* D0 */
  { STA, ADIR },                        /* D1 */
  { STA, ADIR },                        /* D2 */
  { STA, ADIR },                        /* D3 */
  { STA, ADIR },                        /* D4 */
  { STA, ADIR },                        /* D5 */
  { STA, ADIR },                        /* D6 */
  { STA, ADIR },                        /* D7 */
  { STA, ADIR },                        /* D8 */
  { STA, ADIR },                        /* D9 */
  { STA, ADIR },                        /* DA */
  { STA, ADIR },                        /* DB */
  { STA, ADIR },                        /* DC */
  { STA, ADIR },                        /* DD */
  { STA, ADIR },                        /* DE */
  { STA, ADIR },                        /* DF */

  { VDR, IMP  },                        /* E0 */
  { LDJ, IRG  },                        /* E1 */
  { XLT, AXLT },                        /* E2 */
  { MUL, IRG  },                        /* E3 */
  { LLT, IMP  },                        /* E4 */
  { WAI, IMP  },                        /* E5 */
  { STA, AIRG },                        /* E6 */
  { ADD, AIRG },                        /* E7 */
  { SUB, AIRG },                        /* E8 */
  { AND, AIRG },                        /* E9 */
  { LDA, AIRG },                        /* EA */
  { LSR, ACC  },                        /* EB */
  { LSL, ACC  },                        /* EC */
  { ASR, ACC  },                        /* ED */
  { ASRD, IMP },                        /* EE */
  { LSLD, IMP },                        /* EF */

  { VIN, IMP  },                        /* F0 */
  { LDJ, IRG  },                        /* F1 */
  { XLT, AXLT },                        /* F2 */
  { MUL, IRG  },                        /* F3 */
  { LLT, IMP  },                        /* F4 */
  { WAI, IMP  },                        /* F5 */
  { STA, AIRG },                        /* F6 */
  { AWD, AIRG },                        /* F7 */
  { SUB, AIRG },                        /* F8 */
  { AND, AIRG },                        /* F9 */
  { LDA, AIRG },                        /* FA */
  { LSR, ACC  },                        /* FB */
  { LSL, ACC  },                        /* FC */
  { ASR, ACC  },                        /* FD */
  { ASRD, IMP },                        /* FE */
  { LSLD, IMP }                         /* FF */
};

/* disassemble() appends a disassembly of the specified address to
 * the end of the specified text buffer.
 */
void disassemble ( char *buffer, unsigned int maxlen, unsigned int address ) {
  char ambuffer [ 40 ];   /* text buffer for addressing mode values */
  char opbuffer [ 40 ];   /* text buffer for opcodes etc. */
  char flbuffer [ 80 ];   /* text buffer for flag dump */
  RCCINEBYTE opcode;
  RCCINEBYTE opsize = 1;
  int iter = 0;

  /* which opcode in opcode table? (includes immediate data) */
  opcode = rom [ address ];

  /* build addressing mode (value to opcode) */

        memset(ambuffer, 0, sizeof(ambuffer));
  switch ( amodes [ opcode_table [ opcode ].ot_amode ].ad_amode ) {

  /* 4-bit immediate value */
  case AIM4:
  case IM4:
    sprintf ( ambuffer, "#$%X", opcode & 0x0F );
    break;

  /* 4-bit extended immediate value */
  case AIM4X:
    sprintf ( ambuffer, "#$%X", ( opcode & 0x0F ) << 8 );
    break;

  /* 8-bit immediate value */
  case AIM8:
    sprintf ( ambuffer, "#$%02X", rom [ address + 1 ] );
    opsize ++; /* required second byte for value */
    break;

  /* [m] -- indirect through memory */
  case AINDM:
    sprintf ( ambuffer, "AINDM/Error" );
    break;

  /* [i] -- indirect through 'I' */
  case AIRG:
  case IRG:
    sprintf ( ambuffer, "[i]" );
    break;

  /* extended 12-bit immediate value */
  case AIMX:
    sprintf ( ambuffer, "AIMX/Error" );
    break;

  /* no special params */
  case ACC:
  case AXLT:
  case IMP:
    ambuffer [ 0 ] = '\0';
    break;

  /* 12-bit immediate value */
  case IM12:
    sprintf ( ambuffer, "#$%03X",
              ( rom [ address ] & 0x0F ) +
              ( rom [ address + 1 ] & 0xF0 ) +
              ( ( rom [ address + 1 ] & 0x0F ) << 8 ) );
    opsize ++;
    break;

  /* display address of direct addressing modes */
  case ADIR:
  case DIRECT:
    sprintf ( ambuffer, "$%X", opcode & 0x0F );
    break;

  /* jump through J register */
  case JUMP:
    sprintf ( ambuffer, "<jump>" );
    break;

  /* extended jump */
  case JUMPX:
    sprintf ( ambuffer, "JUMPX/Error" );
    break;

  } /* switch on addressing mode */

  /* build flags dump */
  sprintf ( flbuffer,
            "A=%03X B=%03X I=%03XZ J=%03X P=%X " \
            "A0=%02X %02X N%X O%X %c%c%c%c",
            RCregister_A, RCregister_B,
            RCregister_I /*<< 1*/,               /* use the <<1 for Zonn style */
            RCregister_J, RCregister_P, GETA0(),
            GETFC() /* ? 'C' : 'c' */,
            RCcmp_new, RCcmp_old,
            ( ( RCstate == RCstate_A ) || ( RCstate == RCstate_AA ) ) ? 'A' : ' ',
            ( RCstate == RCstate_AA ) ? 'A' : ' ',
            ( ( RCstate == RCstate_B ) || ( RCstate == RCstate_BB ) ) ? 'B' : ' ',
            ( RCstate == RCstate_BB ) ? 'B' : ' ' );

  /* build the final output string. Format is as follows:
   * ADDRESS: OPCODE-ROM-DUMP\tOPCODE VALUE\tFLAGS
   */

  /* append complete opcode ROM dump */
        memset(opbuffer, 0, sizeof(opbuffer));
  for ( iter = 0; iter < opsize; iter++ ) {
    sprintf ( opbuffer + ( iter * 3 ), " %02X", rom [ address + iter ] );
  }

  /* create final output */
  sprintf ( buffer, "%04X:%-6s : %-4s %-6s : %s",
            address, opbuffer,
            opcodes [ opcode_table [ opcode ].ot_opcode ].od_name,
            ambuffer,
            flbuffer );

  return;
}

/* cineExec() is what performs all the "processors" work; it will
 * continue to execute until something horrible happens, a watchpoint
 * is hit, cycle count exceeded, or other happy things.
 */
UINT32 cineExec(void) {

  RCCINEBYTE temp_byte = 0;   /* cause it'll be handy */
  RCCINEBYTE temp_byte_2 = 0; /* cause it'll be handy */
  RCCINEWORD temp_word = 0;   /* cause it'll be handy */
  RCCINEWORD temp_word_2 = 0; /* cause it'll be handy */

  RCCINESWORD temp_sword = 0; /* in case its handy... */
  RCCINESBYTE temp_sbyte = 0; /* in case its handy... */

  RCCINEWORD original_PC = RCregister_PC;

  /* before opcode handlers ...
   */
  ioSwitches &= ( ~SW_ABORT ); /* ( ! SW_ABORT ) ? */

  /* label; after each opcode is finished running, execution returns
   * to this point, where the RCstate is examined and the proper
   * jump table used.
   */
cineExecTop:

  bBailOut = FALSE;

  /* examine the current RCstate, and jump down to the correct
   * opcode jump table.
   */
  switch ( RCstate ) {
  case RCstate_A:  goto opCodeTblA;
  case RCstate_AA: goto opCodeTblAA;
  case RCstate_B:  goto opCodeTblB;
  case RCstate_BB: goto opCodeTblBB;
  } /* switch on current RCstate */

  /* include the opcode jump tables; goto the correct piece of code
   * for the current opcode. That piece of code will set the RCstate
   * for the next run, as well. The opcode jumptables were generated
   * with a perl script and are kept separate for make and ugly sake.
   */

/* ---------------------------------------------- Main jump table cineops.c */


/* cineops.c is meant to be included directly into cinecpu.c, or turned
 * into a master dispatcher macro, or some other horrible thing. cineops.c
 * is the RCstate dispatcher, which also causes tate changes. This design
 * keeps things running very fast, since no costly flag calculations
 * need to be performed. Thank Zonn for this twisted but effective
 * idea.
 */

/* extern RCCINEBYTE opcode */

/* table for RCstate "A" -- Use this table if the last opcode was not
 * an ACC related opcode, and was not a B flip/flop operation.
 * Translation:
 *   Any ACC related routine will use A-reg and go on to opCodeTblAA
 *   Any B flip/flop instructions will jump to opCodeTblB
 *   All other instructions remain in opCodeTblA
 *   JMI will use the current sign of the A-reg
 */
opCodeTblA:

switch ( rom [ RCregister_PC ] ) {
case 0: goto opLDAimm_A_AA;
case 1: goto opLDAimm_A_AA;
case 2: goto opLDAimm_A_AA;
case 3: goto opLDAimm_A_AA;
case 4: goto opLDAimm_A_AA;
case 5: goto opLDAimm_A_AA;
case 6: goto opLDAimm_A_AA;
case 7: goto opLDAimm_A_AA;
case 8: goto opLDAimm_A_AA;
case 9: goto opLDAimm_A_AA;
case 10: goto opLDAimm_A_AA;
case 11: goto opLDAimm_A_AA;
case 12: goto opLDAimm_A_AA;
case 13: goto opLDAimm_A_AA;
case 14: goto opLDAimm_A_AA;
case 15: goto opLDAimm_A_AA;
case 16: goto opINP_A_AA;
case 17: goto opINP_A_AA;
case 18: goto opINP_A_AA;
case 19: goto opINP_A_AA;
case 20: goto opINP_A_AA;
case 21: goto opINP_A_AA;
case 22: goto opINP_A_AA;
case 23: goto opINP_A_AA;
case 24: goto opINP_A_AA;
case 25: goto opINP_A_AA;
case 26: goto opINP_A_AA;
case 27: goto opINP_A_AA;
case 28: goto opINP_A_AA;
case 29: goto opINP_A_AA;
case 30: goto opINP_A_AA;
case 31: goto opINP_A_AA;
case 32: goto opADDimmX_A_AA;
case 33: goto opADDimm_A_AA;
case 34: goto opADDimm_A_AA;
case 35: goto opADDimm_A_AA;
case 36: goto opADDimm_A_AA;
case 37: goto opADDimm_A_AA;
case 38: goto opADDimm_A_AA;
case 39: goto opADDimm_A_AA;
case 40: goto opADDimm_A_AA;
case 41: goto opADDimm_A_AA;
case 42: goto opADDimm_A_AA;
case 43: goto opADDimm_A_AA;
case 44: goto opADDimm_A_AA;
case 45: goto opADDimm_A_AA;
case 46: goto opADDimm_A_AA;
case 47: goto opADDimm_A_AA;
case 48: goto opSUBimmX_A_AA;
case 49: goto opSUBimm_A_AA;
case 50: goto opSUBimm_A_AA;
case 51: goto opSUBimm_A_AA;
case 52: goto opSUBimm_A_AA;
case 53: goto opSUBimm_A_AA;
case 54: goto opSUBimm_A_AA;
case 55: goto opSUBimm_A_AA;
case 56: goto opSUBimm_A_AA;
case 57: goto opSUBimm_A_AA;
case 58: goto opSUBimm_A_AA;
case 59: goto opSUBimm_A_AA;
case 60: goto opSUBimm_A_AA;
case 61: goto opSUBimm_A_AA;
case 62: goto opSUBimm_A_AA;
case 63: goto opSUBimm_A_AA;
case 64: goto opLDJimm_A_A;
case 65: goto opLDJimm_A_A;
case 66: goto opLDJimm_A_A;
case 67: goto opLDJimm_A_A;
case 68: goto opLDJimm_A_A;
case 69: goto opLDJimm_A_A;
case 70: goto opLDJimm_A_A;
case 71: goto opLDJimm_A_A;
case 72: goto opLDJimm_A_A;
case 73: goto opLDJimm_A_A;
case 74: goto opLDJimm_A_A;
case 75: goto opLDJimm_A_A;
case 76: goto opLDJimm_A_A;
case 77: goto opLDJimm_A_A;
case 78: goto opLDJimm_A_A;
case 79: goto opLDJimm_A_A;
case 80: goto tJPP_A_B;      /* redirector */
case 81: goto tJMI_A_B;      /* redirector */
case 82: goto opJDR_A_B;
case 83: goto opJLT_A_B;
case 84: goto opJEQ_A_B;
case 85: goto opJNC_A_B;
case 86: goto opJA0_A_B;
case 87: goto opNOP_A_B;
case 88: goto opJMP_A_A;
case 89: goto tJMI_A_A;      /* redirector */
case 90: goto opJDR_A_A;
case 91: goto opJLT_A_A;
case 92: goto opJEQ_A_A;
case 93: goto opJNC_A_A;
case 94: goto opJA0_A_A;
case 95: goto opNOP_A_A;
case 96: goto opADDdir_A_AA;
case 97: goto opADDdir_A_AA;
case 98: goto opADDdir_A_AA;
case 99: goto opADDdir_A_AA;
case 100: goto opADDdir_A_AA;
case 101: goto opADDdir_A_AA;
case 102: goto opADDdir_A_AA;
case 103: goto opADDdir_A_AA;
case 104: goto opADDdir_A_AA;
case 105: goto opADDdir_A_AA;
case 106: goto opADDdir_A_AA;
case 107: goto opADDdir_A_AA;
case 108: goto opADDdir_A_AA;
case 109: goto opADDdir_A_AA;
case 110: goto opADDdir_A_AA;
case 111: goto opADDdir_A_AA;
case 112: goto opSUBdir_A_AA;
case 113: goto opSUBdir_A_AA;
case 114: goto opSUBdir_A_AA;
case 115: goto opSUBdir_A_AA;
case 116: goto opSUBdir_A_AA;
case 117: goto opSUBdir_A_AA;
case 118: goto opSUBdir_A_AA;
case 119: goto opSUBdir_A_AA;
case 120: goto opSUBdir_A_AA;
case 121: goto opSUBdir_A_AA;
case 122: goto opSUBdir_A_AA;
case 123: goto opSUBdir_A_AA;
case 124: goto opSUBdir_A_AA;
case 125: goto opSUBdir_A_AA;
case 126: goto opSUBdir_A_AA;
case 127: goto opSUBdir_A_AA;
case 128: goto opLDPimm_A_A;
case 129: goto opLDPimm_A_A;
case 130: goto opLDPimm_A_A;
case 131: goto opLDPimm_A_A;
case 132: goto opLDPimm_A_A;
case 133: goto opLDPimm_A_A;
case 134: goto opLDPimm_A_A;
case 135: goto opLDPimm_A_A;
case 136: goto opLDPimm_A_A;
case 137: goto opLDPimm_A_A;
case 138: goto opLDPimm_A_A;
case 139: goto opLDPimm_A_A;
case 140: goto opLDPimm_A_A;
case 141: goto opLDPimm_A_A;
case 142: goto opLDPimm_A_A;
case 143: goto opLDPimm_A_A;
case 144: goto tOUT_A_A;         /* redirector */
case 145: goto tOUT_A_A;         /* redirector */
case 146: goto tOUT_A_A;         /* redirector */
case 147: goto tOUT_A_A;         /* redirector */
case 148: goto tOUT_A_A;         /* redirector */
case 149: goto tOUT_A_A;         /* redirector */
case 150: goto tOUT_A_A;         /* redirector */
case 151: goto tOUT_A_A;         /* redirector */
case 152: goto tOUT_A_A;         /* redirector */
case 153: goto tOUT_A_A;         /* redirector */
case 154: goto tOUT_A_A;         /* redirector */
case 155: goto tOUT_A_A;         /* redirector */
case 156: goto tOUT_A_A;         /* redirector */
case 157: goto tOUT_A_A;         /* redirector */
case 158: goto tOUT_A_A;         /* redirector */
case 159: goto tOUT_A_A;         /* redirector */
case 160: goto opLDAdir_A_AA;
case 161: goto opLDAdir_A_AA;
case 162: goto opLDAdir_A_AA;
case 163: goto opLDAdir_A_AA;
case 164: goto opLDAdir_A_AA;
case 165: goto opLDAdir_A_AA;
case 166: goto opLDAdir_A_AA;
case 167: goto opLDAdir_A_AA;
case 168: goto opLDAdir_A_AA;
case 169: goto opLDAdir_A_AA;
case 170: goto opLDAdir_A_AA;
case 171: goto opLDAdir_A_AA;
case 172: goto opLDAdir_A_AA;
case 173: goto opLDAdir_A_AA;
case 174: goto opLDAdir_A_AA;
case 175: goto opLDAdir_A_AA;
case 176: goto opCMPdir_A_AA;
case 177: goto opCMPdir_A_AA;
case 178: goto opCMPdir_A_AA;
case 179: goto opCMPdir_A_AA;
case 180: goto opCMPdir_A_AA;
case 181: goto opCMPdir_A_AA;
case 182: goto opCMPdir_A_AA;
case 183: goto opCMPdir_A_AA;
case 184: goto opCMPdir_A_AA;
case 185: goto opCMPdir_A_AA;
case 186: goto opCMPdir_A_AA;
case 187: goto opCMPdir_A_AA;
case 188: goto opCMPdir_A_AA;
case 189: goto opCMPdir_A_AA;
case 190: goto opCMPdir_A_AA;
case 191: goto opCMPdir_A_AA;
case 192: goto opLDIdir_A_A;
case 193: goto opLDIdir_A_A;
case 194: goto opLDIdir_A_A;
case 195: goto opLDIdir_A_A;
case 196: goto opLDIdir_A_A;
case 197: goto opLDIdir_A_A;
case 198: goto opLDIdir_A_A;
case 199: goto opLDIdir_A_A;
case 200: goto opLDIdir_A_A;
case 201: goto opLDIdir_A_A;
case 202: goto opLDIdir_A_A;
case 203: goto opLDIdir_A_A;
case 204: goto opLDIdir_A_A;
case 205: goto opLDIdir_A_A;
case 206: goto opLDIdir_A_A;
case 207: goto opLDIdir_A_A;
case 208: goto opSTAdir_A_A;
case 209: goto opSTAdir_A_A;
case 210: goto opSTAdir_A_A;
case 211: goto opSTAdir_A_A;
case 212: goto opSTAdir_A_A;
case 213: goto opSTAdir_A_A;
case 214: goto opSTAdir_A_A;
case 215: goto opSTAdir_A_A;
case 216: goto opSTAdir_A_A;
case 217: goto opSTAdir_A_A;
case 218: goto opSTAdir_A_A;
case 219: goto opSTAdir_A_A;
case 220: goto opSTAdir_A_A;
case 221: goto opSTAdir_A_A;
case 222: goto opSTAdir_A_A;
case 223: goto opSTAdir_A_A;
case 224: goto opVDR_A_A;
case 225: goto opLDJirg_A_A;
case 226: goto opXLT_A_AA;
case 227: goto opMULirg_A_AA;
case 228: goto opLLT_A_AA;
case 229: goto opWAI_A_A;
case 230: goto opSTAirg_A_A;
case 231: goto opADDirg_A_AA;
case 232: goto opSUBirg_A_AA;
case 233: goto opANDirg_A_AA;
case 234: goto opLDAirg_A_AA;
case 235: goto opLSRe_A_AA;
case 236: goto opLSLe_A_AA;
case 237: goto opASRe_A_AA;
case 238: goto opASRDe_A_AA;
case 239: goto opLSLDe_A_AA;
case 240: goto opVIN_A_A;
case 241: goto opLDJirg_A_A;
case 242: goto opXLT_A_AA;
case 243: goto opMULirg_A_AA;
case 244: goto opLLT_A_AA;
case 245: goto opWAI_A_A;
case 246: goto opSTAirg_A_A;
case 247: goto opAWDirg_A_AA;
case 248: goto opSUBirg_A_AA;
case 249: goto opANDirg_A_AA;
case 250: goto opLDAirg_A_AA;
case 251: goto opLSRf_A_AA;
case 252: goto opLSLf_A_AA;
case 253: goto opASRf_A_AA;
case 254: goto opASRDf_A_AA;
case 255: goto opLSLDf_A_AA;
} /* switch on opcode */

/* opcode table AA -- Use this table if the last opcode was an ACC
 * related opcode. Translation:
 *   Any ACC related routine will use A-reg and remain in OpCodeTblAA
 *   Any B flip/flop instructions will jump to opCodeTblB
 *   All other instructions will jump to opCodeTblA
 *   JMI will use the sign of acc_old
 */

opCodeTblAA:

switch ( rom [ RCregister_PC ] ) {
case 0: goto opLDAimm_AA_AA;
case 1: goto opLDAimm_AA_AA;
case 2: goto opLDAimm_AA_AA;
case 3: goto opLDAimm_AA_AA;
case 4: goto opLDAimm_AA_AA;
case 5: goto opLDAimm_AA_AA;
case 6: goto opLDAimm_AA_AA;
case 7: goto opLDAimm_AA_AA;
case 8: goto opLDAimm_AA_AA;
case 9: goto opLDAimm_AA_AA;
case 10: goto opLDAimm_AA_AA;
case 11: goto opLDAimm_AA_AA;
case 12: goto opLDAimm_AA_AA;
case 13: goto opLDAimm_AA_AA;
case 14: goto opLDAimm_AA_AA;
case 15: goto opLDAimm_AA_AA;
case 16: goto opINP_AA_AA;
case 17: goto opINP_AA_AA;
case 18: goto opINP_AA_AA;
case 19: goto opINP_AA_AA;
case 20: goto opINP_AA_AA;
case 21: goto opINP_AA_AA;
case 22: goto opINP_AA_AA;
case 23: goto opINP_AA_AA;
case 24: goto opINP_AA_AA;
case 25: goto opINP_AA_AA;
case 26: goto opINP_AA_AA;
case 27: goto opINP_AA_AA;
case 28: goto opINP_AA_AA;
case 29: goto opINP_AA_AA;
case 30: goto opINP_AA_AA;
case 31: goto opINP_AA_AA;
case 32: goto opADDimmX_AA_AA;
case 33: goto opADDimm_AA_AA;
case 34: goto opADDimm_AA_AA;
case 35: goto opADDimm_AA_AA;
case 36: goto opADDimm_AA_AA;
case 37: goto opADDimm_AA_AA;
case 38: goto opADDimm_AA_AA;
case 39: goto opADDimm_AA_AA;
case 40: goto opADDimm_AA_AA;
case 41: goto opADDimm_AA_AA;
case 42: goto opADDimm_AA_AA;
case 43: goto opADDimm_AA_AA;
case 44: goto opADDimm_AA_AA;
case 45: goto opADDimm_AA_AA;
case 46: goto opADDimm_AA_AA;
case 47: goto opADDimm_AA_AA;
case 48: goto opSUBimmX_AA_AA;
case 49: goto opSUBimm_AA_AA;
case 50: goto opSUBimm_AA_AA;
case 51: goto opSUBimm_AA_AA;
case 52: goto opSUBimm_AA_AA;
case 53: goto opSUBimm_AA_AA;
case 54: goto opSUBimm_AA_AA;
case 55: goto opSUBimm_AA_AA;
case 56: goto opSUBimm_AA_AA;
case 57: goto opSUBimm_AA_AA;
case 58: goto opSUBimm_AA_AA;
case 59: goto opSUBimm_AA_AA;
case 60: goto opSUBimm_AA_AA;
case 61: goto opSUBimm_AA_AA;
case 62: goto opSUBimm_AA_AA;
case 63: goto opSUBimm_AA_AA;
case 64: goto opLDJimm_AA_A;
case 65: goto opLDJimm_AA_A;
case 66: goto opLDJimm_AA_A;
case 67: goto opLDJimm_AA_A;
case 68: goto opLDJimm_AA_A;
case 69: goto opLDJimm_AA_A;
case 70: goto opLDJimm_AA_A;
case 71: goto opLDJimm_AA_A;
case 72: goto opLDJimm_AA_A;
case 73: goto opLDJimm_AA_A;
case 74: goto opLDJimm_AA_A;
case 75: goto opLDJimm_AA_A;
case 76: goto opLDJimm_AA_A;
case 77: goto opLDJimm_AA_A;
case 78: goto opLDJimm_AA_A;
case 79: goto opLDJimm_AA_A;
case 80: goto tJPP_AA_B;         /* redirector */
case 81: goto tJMI_AA_B;         /* redirector */
case 82: goto opJDR_AA_B;
case 83: goto opJLT_AA_B;
case 84: goto opJEQ_AA_B;
case 85: goto opJNC_AA_B;
case 86: goto opJA0_AA_B;
case 87: goto opNOP_AA_B;
case 88: goto opJMP_AA_A;
case 89: goto tJMI_AA_A;         /* redirector */
case 90: goto opJDR_AA_A;
case 91: goto opJLT_AA_A;
case 92: goto opJEQ_AA_A;
case 93: goto opJNC_AA_A;
case 94: goto opJA0_AA_A;
case 95: goto opNOP_AA_A;
case 96: goto opADDdir_AA_AA;
case 97: goto opADDdir_AA_AA;
case 98: goto opADDdir_AA_AA;
case 99: goto opADDdir_AA_AA;
case 100: goto opADDdir_AA_AA;
case 101: goto opADDdir_AA_AA;
case 102: goto opADDdir_AA_AA;
case 103: goto opADDdir_AA_AA;
case 104: goto opADDdir_AA_AA;
case 105: goto opADDdir_AA_AA;
case 106: goto opADDdir_AA_AA;
case 107: goto opADDdir_AA_AA;
case 108: goto opADDdir_AA_AA;
case 109: goto opADDdir_AA_AA;
case 110: goto opADDdir_AA_AA;
case 111: goto opADDdir_AA_AA;
case 112: goto opSUBdir_AA_AA;
case 113: goto opSUBdir_AA_AA;
case 114: goto opSUBdir_AA_AA;
case 115: goto opSUBdir_AA_AA;
case 116: goto opSUBdir_AA_AA;
case 117: goto opSUBdir_AA_AA;
case 118: goto opSUBdir_AA_AA;
case 119: goto opSUBdir_AA_AA;
case 120: goto opSUBdir_AA_AA;
case 121: goto opSUBdir_AA_AA;
case 122: goto opSUBdir_AA_AA;
case 123: goto opSUBdir_AA_AA;
case 124: goto opSUBdir_AA_AA;
case 125: goto opSUBdir_AA_AA;
case 126: goto opSUBdir_AA_AA;
case 127: goto opSUBdir_AA_AA;
case 128: goto opLDPimm_AA_A;
case 129: goto opLDPimm_AA_A;
case 130: goto opLDPimm_AA_A;
case 131: goto opLDPimm_AA_A;
case 132: goto opLDPimm_AA_A;
case 133: goto opLDPimm_AA_A;
case 134: goto opLDPimm_AA_A;
case 135: goto opLDPimm_AA_A;
case 136: goto opLDPimm_AA_A;
case 137: goto opLDPimm_AA_A;
case 138: goto opLDPimm_AA_A;
case 139: goto opLDPimm_AA_A;
case 140: goto opLDPimm_AA_A;
case 141: goto opLDPimm_AA_A;
case 142: goto opLDPimm_AA_A;
case 143: goto opLDPimm_AA_A;
case 144: goto tOUT_AA_A;          /* redirector */
case 145: goto tOUT_AA_A;          /* redirector */
case 146: goto tOUT_AA_A;          /* redirector */
case 147: goto tOUT_AA_A;          /* redirector */
case 148: goto tOUT_AA_A;          /* redirector */
case 149: goto tOUT_AA_A;          /* redirector */
case 150: goto tOUT_AA_A;          /* redirector */
case 151: goto tOUT_AA_A;          /* redirector */
case 152: goto tOUT_AA_A;          /* redirector */
case 153: goto tOUT_AA_A;          /* redirector */
case 154: goto tOUT_AA_A;          /* redirector */
case 155: goto tOUT_AA_A;          /* redirector */
case 156: goto tOUT_AA_A;          /* redirector */
case 157: goto tOUT_AA_A;          /* redirector */
case 158: goto tOUT_AA_A;          /* redirector */
case 159: goto tOUT_AA_A;          /* redirector */
case 160: goto opLDAdir_AA_AA;
case 161: goto opLDAdir_AA_AA;
case 162: goto opLDAdir_AA_AA;
case 163: goto opLDAdir_AA_AA;
case 164: goto opLDAdir_AA_AA;
case 165: goto opLDAdir_AA_AA;
case 166: goto opLDAdir_AA_AA;
case 167: goto opLDAdir_AA_AA;
case 168: goto opLDAdir_AA_AA;
case 169: goto opLDAdir_AA_AA;
case 170: goto opLDAdir_AA_AA;
case 171: goto opLDAdir_AA_AA;
case 172: goto opLDAdir_AA_AA;
case 173: goto opLDAdir_AA_AA;
case 174: goto opLDAdir_AA_AA;
case 175: goto opLDAdir_AA_AA;
case 176: goto opCMPdir_AA_AA;
case 177: goto opCMPdir_AA_AA;
case 178: goto opCMPdir_AA_AA;
case 179: goto opCMPdir_AA_AA;
case 180: goto opCMPdir_AA_AA;
case 181: goto opCMPdir_AA_AA;
case 182: goto opCMPdir_AA_AA;
case 183: goto opCMPdir_AA_AA;
case 184: goto opCMPdir_AA_AA;
case 185: goto opCMPdir_AA_AA;
case 186: goto opCMPdir_AA_AA;
case 187: goto opCMPdir_AA_AA;
case 188: goto opCMPdir_AA_AA;
case 189: goto opCMPdir_AA_AA;
case 190: goto opCMPdir_AA_AA;
case 191: goto opCMPdir_AA_AA;
case 192: goto opLDIdir_AA_A;
case 193: goto opLDIdir_AA_A;
case 194: goto opLDIdir_AA_A;
case 195: goto opLDIdir_AA_A;
case 196: goto opLDIdir_AA_A;
case 197: goto opLDIdir_AA_A;
case 198: goto opLDIdir_AA_A;
case 199: goto opLDIdir_AA_A;
case 200: goto opLDIdir_AA_A;
case 201: goto opLDIdir_AA_A;
case 202: goto opLDIdir_AA_A;
case 203: goto opLDIdir_AA_A;
case 204: goto opLDIdir_AA_A;
case 205: goto opLDIdir_AA_A;
case 206: goto opLDIdir_AA_A;
case 207: goto opLDIdir_AA_A;
case 208: goto opSTAdir_AA_A;
case 209: goto opSTAdir_AA_A;
case 210: goto opSTAdir_AA_A;
case 211: goto opSTAdir_AA_A;
case 212: goto opSTAdir_AA_A;
case 213: goto opSTAdir_AA_A;
case 214: goto opSTAdir_AA_A;
case 215: goto opSTAdir_AA_A;
case 216: goto opSTAdir_AA_A;
case 217: goto opSTAdir_AA_A;
case 218: goto opSTAdir_AA_A;
case 219: goto opSTAdir_AA_A;
case 220: goto opSTAdir_AA_A;
case 221: goto opSTAdir_AA_A;
case 222: goto opSTAdir_AA_A;
case 223: goto opSTAdir_AA_A;
case 224: goto opVDR_AA_A;
case 225: goto opLDJirg_AA_A;
case 226: goto opXLT_AA_AA;
case 227: goto opMULirg_AA_AA;
case 228: goto opLLT_AA_AA;
case 229: goto opWAI_AA_A;
case 230: goto opSTAirg_AA_A;
case 231: goto opADDirg_AA_AA;
case 232: goto opSUBirg_AA_AA;
case 233: goto opANDirg_AA_AA;
case 234: goto opLDAirg_AA_AA;
case 235: goto opLSRe_AA_AA;
case 236: goto opLSLe_AA_AA;
case 237: goto opASRe_AA_AA;
case 238: goto opASRDe_AA_AA;
case 239: goto opLSLDe_AA_AA;
case 240: goto opVIN_AA_A;
case 241: goto opLDJirg_AA_A;
case 242: goto opXLT_AA_AA;
case 243: goto opMULirg_AA_AA;
case 244: goto opLLT_AA_AA;
case 245: goto opWAI_AA_A;
case 246: goto opSTAirg_AA_A;
case 247: goto opAWDirg_AA_AA;
case 248: goto opSUBirg_AA_AA;
case 249: goto opANDirg_AA_AA;
case 250: goto opLDAirg_AA_AA;
case 251: goto opLSRf_AA_AA;
case 252: goto opLSLf_AA_AA;
case 253: goto opASRf_AA_AA;
case 254: goto opASRDf_AA_AA;
case 255: goto opLSLDf_AA_AA;
} /* switch on opcode */

/* opcode table B -- use this table if the last opcode was a B-reg flip/flop
 * Translation:
 *   Any ACC related routine uses B-reg, and goes to opCodeTblAA
 *   All other instructions will jump to table opCodeTblBB (including
 *     B flip/flop related instructions)
 *   JMI will use current sign of the A-reg
 */

opCodeTblB:

switch ( rom [ RCregister_PC ] ) {
case 0: goto opLDAimm_B_AA;
case 1: goto opLDAimm_B_AA;
case 2: goto opLDAimm_B_AA;
case 3: goto opLDAimm_B_AA;
case 4: goto opLDAimm_B_AA;
case 5: goto opLDAimm_B_AA;
case 6: goto opLDAimm_B_AA;
case 7: goto opLDAimm_B_AA;
case 8: goto opLDAimm_B_AA;
case 9: goto opLDAimm_B_AA;
case 10: goto opLDAimm_B_AA;
case 11: goto opLDAimm_B_AA;
case 12: goto opLDAimm_B_AA;
case 13: goto opLDAimm_B_AA;
case 14: goto opLDAimm_B_AA;
case 15: goto opLDAimm_B_AA;
case 16: goto opINP_B_AA;
case 17: goto opINP_B_AA;
case 18: goto opINP_B_AA;
case 19: goto opINP_B_AA;
case 20: goto opINP_B_AA;
case 21: goto opINP_B_AA;
case 22: goto opINP_B_AA;
case 23: goto opINP_B_AA;
case 24: goto opINP_B_AA;
case 25: goto opINP_B_AA;
case 26: goto opINP_B_AA;
case 27: goto opINP_B_AA;
case 28: goto opINP_B_AA;
case 29: goto opINP_B_AA;
case 30: goto opINP_B_AA;
case 31: goto opINP_B_AA;
case 32: goto opADDimmX_B_AA;
case 33: goto opADDimm_B_AA;
case 34: goto opADDimm_B_AA;
case 35: goto opADDimm_B_AA;
case 36: goto opADDimm_B_AA;
case 37: goto opADDimm_B_AA;
case 38: goto opADDimm_B_AA;
case 39: goto opADDimm_B_AA;
case 40: goto opADDimm_B_AA;
case 41: goto opADDimm_B_AA;
case 42: goto opADDimm_B_AA;
case 43: goto opADDimm_B_AA;
case 44: goto opADDimm_B_AA;
case 45: goto opADDimm_B_AA;
case 46: goto opADDimm_B_AA;
case 47: goto opADDimm_B_AA;
case 48: goto opSUBimmX_B_AA;
case 49: goto opSUBimm_B_AA;
case 50: goto opSUBimm_B_AA;
case 51: goto opSUBimm_B_AA;
case 52: goto opSUBimm_B_AA;
case 53: goto opSUBimm_B_AA;
case 54: goto opSUBimm_B_AA;
case 55: goto opSUBimm_B_AA;
case 56: goto opSUBimm_B_AA;
case 57: goto opSUBimm_B_AA;
case 58: goto opSUBimm_B_AA;
case 59: goto opSUBimm_B_AA;
case 60: goto opSUBimm_B_AA;
case 61: goto opSUBimm_B_AA;
case 62: goto opSUBimm_B_AA;
case 63: goto opSUBimm_B_AA;
case 64: goto opLDJimm_B_BB;
case 65: goto opLDJimm_B_BB;
case 66: goto opLDJimm_B_BB;
case 67: goto opLDJimm_B_BB;
case 68: goto opLDJimm_B_BB;
case 69: goto opLDJimm_B_BB;
case 70: goto opLDJimm_B_BB;
case 71: goto opLDJimm_B_BB;
case 72: goto opLDJimm_B_BB;
case 73: goto opLDJimm_B_BB;
case 74: goto opLDJimm_B_BB;
case 75: goto opLDJimm_B_BB;
case 76: goto opLDJimm_B_BB;
case 77: goto opLDJimm_B_BB;
case 78: goto opLDJimm_B_BB;
case 79: goto opLDJimm_B_BB;
case 80: goto tJPP_B_BB;          /* redirector */
case 81: goto tJMI_B_BB1;         /* redirector */
case 82: goto opJDR_B_BB;
case 83: goto opJLT_B_BB;
case 84: goto opJEQ_B_BB;
case 85: goto opJNC_B_BB;
case 86: goto opJA0_B_BB;
case 87: goto opNOP_B_BB;
case 88: goto opJMP_B_BB;
case 89: goto tJMI_B_BB2;         /* redirector */
case 90: goto opJDR_B_BB;
case 91: goto opJLT_B_BB;
case 92: goto opJEQ_B_BB;
case 93: goto opJNC_B_BB;
case 94: goto opJA0_B_BB;
case 95: goto opNOP_B_BB;
case 96: goto opADDdir_B_AA;
case 97: goto opADDdir_B_AA;
case 98: goto opADDdir_B_AA;
case 99: goto opADDdir_B_AA;
case 100: goto opADDdir_B_AA;
case 101: goto opADDdir_B_AA;
case 102: goto opADDdir_B_AA;
case 103: goto opADDdir_B_AA;
case 104: goto opADDdir_B_AA;
case 105: goto opADDdir_B_AA;
case 106: goto opADDdir_B_AA;
case 107: goto opADDdir_B_AA;
case 108: goto opADDdir_B_AA;
case 109: goto opADDdir_B_AA;
case 110: goto opADDdir_B_AA;
case 111: goto opADDdir_B_AA;
case 112: goto opSUBdir_B_AA;
case 113: goto opSUBdir_B_AA;
case 114: goto opSUBdir_B_AA;
case 115: goto opSUBdir_B_AA;
case 116: goto opSUBdir_B_AA;
case 117: goto opSUBdir_B_AA;
case 118: goto opSUBdir_B_AA;
case 119: goto opSUBdir_B_AA;
case 120: goto opSUBdir_B_AA;
case 121: goto opSUBdir_B_AA;
case 122: goto opSUBdir_B_AA;
case 123: goto opSUBdir_B_AA;
case 124: goto opSUBdir_B_AA;
case 125: goto opSUBdir_B_AA;
case 126: goto opSUBdir_B_AA;
case 127: goto opSUBdir_B_AA;
case 128: goto opLDPimm_B_BB;
case 129: goto opLDPimm_B_BB;
case 130: goto opLDPimm_B_BB;
case 131: goto opLDPimm_B_BB;
case 132: goto opLDPimm_B_BB;
case 133: goto opLDPimm_B_BB;
case 134: goto opLDPimm_B_BB;
case 135: goto opLDPimm_B_BB;
case 136: goto opLDPimm_B_BB;
case 137: goto opLDPimm_B_BB;
case 138: goto opLDPimm_B_BB;
case 139: goto opLDPimm_B_BB;
case 140: goto opLDPimm_B_BB;
case 141: goto opLDPimm_B_BB;
case 142: goto opLDPimm_B_BB;
case 143: goto opLDPimm_B_BB;
case 144: goto tOUT_B_BB;         /* redirector */
case 145: goto tOUT_B_BB;         /* redirector */
case 146: goto tOUT_B_BB;         /* redirector */
case 147: goto tOUT_B_BB;         /* redirector */
case 148: goto tOUT_B_BB;         /* redirector */
case 149: goto tOUT_B_BB;         /* redirector */
case 150: goto tOUT_B_BB;         /* redirector */
case 151: goto tOUT_B_BB;         /* redirector */
case 152: goto tOUT_B_BB;         /* redirector */
case 153: goto tOUT_B_BB;         /* redirector */
case 154: goto tOUT_B_BB;         /* redirector */
case 155: goto tOUT_B_BB;         /* redirector */
case 156: goto tOUT_B_BB;         /* redirector */
case 157: goto tOUT_B_BB;         /* redirector */
case 158: goto tOUT_B_BB;         /* redirector */
case 159: goto tOUT_B_BB;         /* redirector */
case 160: goto opLDAdir_B_AA;
case 161: goto opLDAdir_B_AA;
case 162: goto opLDAdir_B_AA;
case 163: goto opLDAdir_B_AA;
case 164: goto opLDAdir_B_AA;
case 165: goto opLDAdir_B_AA;
case 166: goto opLDAdir_B_AA;
case 167: goto opLDAdir_B_AA;
case 168: goto opLDAdir_B_AA;
case 169: goto opLDAdir_B_AA;
case 170: goto opLDAdir_B_AA;
case 171: goto opLDAdir_B_AA;
case 172: goto opLDAdir_B_AA;
case 173: goto opLDAdir_B_AA;
case 174: goto opLDAdir_B_AA;
case 175: goto opLDAdir_B_AA;
case 176: goto opCMPdir_B_AA;
case 177: goto opCMPdir_B_AA;
case 178: goto opCMPdir_B_AA;
case 179: goto opCMPdir_B_AA;
case 180: goto opCMPdir_B_AA;
case 181: goto opCMPdir_B_AA;
case 182: goto opCMPdir_B_AA;
case 183: goto opCMPdir_B_AA;
case 184: goto opCMPdir_B_AA;
case 185: goto opCMPdir_B_AA;
case 186: goto opCMPdir_B_AA;
case 187: goto opCMPdir_B_AA;
case 188: goto opCMPdir_B_AA;
case 189: goto opCMPdir_B_AA;
case 190: goto opCMPdir_B_AA;
case 191: goto opCMPdir_B_AA;
case 192: goto opLDIdir_B_BB;
case 193: goto opLDIdir_B_BB;
case 194: goto opLDIdir_B_BB;
case 195: goto opLDIdir_B_BB;
case 196: goto opLDIdir_B_BB;
case 197: goto opLDIdir_B_BB;
case 198: goto opLDIdir_B_BB;
case 199: goto opLDIdir_B_BB;
case 200: goto opLDIdir_B_BB;
case 201: goto opLDIdir_B_BB;
case 202: goto opLDIdir_B_BB;
case 203: goto opLDIdir_B_BB;
case 204: goto opLDIdir_B_BB;
case 205: goto opLDIdir_B_BB;
case 206: goto opLDIdir_B_BB;
case 207: goto opLDIdir_B_BB;
case 208: goto opSTAdir_B_BB;
case 209: goto opSTAdir_B_BB;
case 210: goto opSTAdir_B_BB;
case 211: goto opSTAdir_B_BB;
case 212: goto opSTAdir_B_BB;
case 213: goto opSTAdir_B_BB;
case 214: goto opSTAdir_B_BB;
case 215: goto opSTAdir_B_BB;
case 216: goto opSTAdir_B_BB;
case 217: goto opSTAdir_B_BB;
case 218: goto opSTAdir_B_BB;
case 219: goto opSTAdir_B_BB;
case 220: goto opSTAdir_B_BB;
case 221: goto opSTAdir_B_BB;
case 222: goto opSTAdir_B_BB;
case 223: goto opSTAdir_B_BB;
case 224: goto opVDR_B_BB;
case 225: goto opLDJirg_B_BB;
case 226: goto opXLT_B_AA;
case 227: goto opMULirg_B_AA;
case 228: goto opLLT_B_AA;
case 229: goto opWAI_B_BB;
case 230: goto opSTAirg_B_BB;
case 231: goto opADDirg_B_AA;
case 232: goto opSUBirg_B_AA;
case 233: goto opANDirg_B_AA;
case 234: goto opLDAirg_B_AA;
case 235: goto opLSRe_B_AA;
case 236: goto opLSLe_B_AA;
case 237: goto opASRe_B_AA;
case 238: goto opASRDe_B_AA;
case 239: goto opLSLDe_B_AA;
case 240: goto opVIN_B_BB;
case 241: goto opLDJirg_B_BB;
case 242: goto opXLT_B_AA;
case 243: goto opMULirg_B_AA;
case 244: goto opLLT_B_AA;
case 245: goto opWAI_B_BB;
case 246: goto opSTAirg_B_BB;
case 247: goto opAWDirg_B_AA;
case 248: goto opSUBirg_B_AA;
case 249: goto opANDirg_B_AA;
case 250: goto opLDAirg_B_AA;
case 251: goto opLSRf_B_AA;
case 252: goto opLSLf_B_AA;
case 253: goto opASRf_B_AA;
case 254: goto opASRDf_B_AA;
case 255: goto opLSLDf_B_AA;
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

switch ( rom [ RCregister_PC ] ) {
case 0: goto opLDAimm_BB_AA;
case 1: goto opLDAimm_BB_AA;
case 2: goto opLDAimm_BB_AA;
case 3: goto opLDAimm_BB_AA;
case 4: goto opLDAimm_BB_AA;
case 5: goto opLDAimm_BB_AA;
case 6: goto opLDAimm_BB_AA;
case 7: goto opLDAimm_BB_AA;
case 8: goto opLDAimm_BB_AA;
case 9: goto opLDAimm_BB_AA;
case 10: goto opLDAimm_BB_AA;
case 11: goto opLDAimm_BB_AA;
case 12: goto opLDAimm_BB_AA;
case 13: goto opLDAimm_BB_AA;
case 14: goto opLDAimm_BB_AA;
case 15: goto opLDAimm_BB_AA;
case 16: goto opINP_BB_AA;
case 17: goto opINP_BB_AA;
case 18: goto opINP_BB_AA;
case 19: goto opINP_BB_AA;
case 20: goto opINP_BB_AA;
case 21: goto opINP_BB_AA;
case 22: goto opINP_BB_AA;
case 23: goto opINP_BB_AA;
case 24: goto opINP_BB_AA;
case 25: goto opINP_BB_AA;
case 26: goto opINP_BB_AA;
case 27: goto opINP_BB_AA;
case 28: goto opINP_BB_AA;
case 29: goto opINP_BB_AA;
case 30: goto opINP_BB_AA;
case 31: goto opINP_BB_AA;
case 32: goto opADDimmX_BB_AA;
case 33: goto opADDimm_BB_AA;
case 34: goto opADDimm_BB_AA;
case 35: goto opADDimm_BB_AA;
case 36: goto opADDimm_BB_AA;
case 37: goto opADDimm_BB_AA;
case 38: goto opADDimm_BB_AA;
case 39: goto opADDimm_BB_AA;
case 40: goto opADDimm_BB_AA;
case 41: goto opADDimm_BB_AA;
case 42: goto opADDimm_BB_AA;
case 43: goto opADDimm_BB_AA;
case 44: goto opADDimm_BB_AA;
case 45: goto opADDimm_BB_AA;
case 46: goto opADDimm_BB_AA;
case 47: goto opADDimm_BB_AA;
case 48: goto opSUBimmX_BB_AA;
case 49: goto opSUBimm_BB_AA;
case 50: goto opSUBimm_BB_AA;
case 51: goto opSUBimm_BB_AA;
case 52: goto opSUBimm_BB_AA;
case 53: goto opSUBimm_BB_AA;
case 54: goto opSUBimm_BB_AA;
case 55: goto opSUBimm_BB_AA;
case 56: goto opSUBimm_BB_AA;
case 57: goto opSUBimm_BB_AA;
case 58: goto opSUBimm_BB_AA;
case 59: goto opSUBimm_BB_AA;
case 60: goto opSUBimm_BB_AA;
case 61: goto opSUBimm_BB_AA;
case 62: goto opSUBimm_BB_AA;
case 63: goto opSUBimm_BB_AA;
case 64: goto opLDJimm_BB_A;
case 65: goto opLDJimm_BB_A;
case 66: goto opLDJimm_BB_A;
case 67: goto opLDJimm_BB_A;
case 68: goto opLDJimm_BB_A;
case 69: goto opLDJimm_BB_A;
case 70: goto opLDJimm_BB_A;
case 71: goto opLDJimm_BB_A;
case 72: goto opLDJimm_BB_A;
case 73: goto opLDJimm_BB_A;
case 74: goto opLDJimm_BB_A;
case 75: goto opLDJimm_BB_A;
case 76: goto opLDJimm_BB_A;
case 77: goto opLDJimm_BB_A;
case 78: goto opLDJimm_BB_A;
case 79: goto opLDJimm_BB_A;
case 80: goto tJPP_BB_B;         /* redirector */
case 81: goto tJMI_BB_B;         /* redirector */
case 82: goto opJDR_BB_B;
case 83: goto opJLT_BB_B;
case 84: goto opJEQ_BB_B;
case 85: goto opJNC_BB_B;
case 86: goto opJA0_BB_B;
case 87: goto opNOP_BB_B;
case 88: goto opJMP_BB_A;
case 89: goto tJMI_BB_A;         /* redirector */
case 90: goto opJDR_BB_A;
case 91: goto opJLT_BB_A;
case 92: goto opJEQ_BB_A;
case 93: goto opJNC_BB_A;
case 94: goto opJA0_BB_A;
case 95: goto opNOP_BB_A;
case 96: goto opADDdir_BB_AA;
case 97: goto opADDdir_BB_AA;
case 98: goto opADDdir_BB_AA;
case 99: goto opADDdir_BB_AA;
case 100: goto opADDdir_BB_AA;
case 101: goto opADDdir_BB_AA;
case 102: goto opADDdir_BB_AA;
case 103: goto opADDdir_BB_AA;
case 104: goto opADDdir_BB_AA;
case 105: goto opADDdir_BB_AA;
case 106: goto opADDdir_BB_AA;
case 107: goto opADDdir_BB_AA;
case 108: goto opADDdir_BB_AA;
case 109: goto opADDdir_BB_AA;
case 110: goto opADDdir_BB_AA;
case 111: goto opADDdir_BB_AA;
case 112: goto opSUBdir_BB_AA;
case 113: goto opSUBdir_BB_AA;
case 114: goto opSUBdir_BB_AA;
case 115: goto opSUBdir_BB_AA;
case 116: goto opSUBdir_BB_AA;
case 117: goto opSUBdir_BB_AA;
case 118: goto opSUBdir_BB_AA;
case 119: goto opSUBdir_BB_AA;
case 120: goto opSUBdir_BB_AA;
case 121: goto opSUBdir_BB_AA;
case 122: goto opSUBdir_BB_AA;
case 123: goto opSUBdir_BB_AA;
case 124: goto opSUBdir_BB_AA;
case 125: goto opSUBdir_BB_AA;
case 126: goto opSUBdir_BB_AA;
case 127: goto opSUBdir_BB_AA;
case 128: goto opLDPimm_BB_A;
case 129: goto opLDPimm_BB_A;
case 130: goto opLDPimm_BB_A;
case 131: goto opLDPimm_BB_A;
case 132: goto opLDPimm_BB_A;
case 133: goto opLDPimm_BB_A;
case 134: goto opLDPimm_BB_A;
case 135: goto opLDPimm_BB_A;
case 136: goto opLDPimm_BB_A;
case 137: goto opLDPimm_BB_A;
case 138: goto opLDPimm_BB_A;
case 139: goto opLDPimm_BB_A;
case 140: goto opLDPimm_BB_A;
case 141: goto opLDPimm_BB_A;
case 142: goto opLDPimm_BB_A;
case 143: goto opLDPimm_BB_A;
case 144: goto tOUT_BB_A;         /* redirector */
case 145: goto tOUT_BB_A;         /* redirector */
case 146: goto tOUT_BB_A;         /* redirector */
case 147: goto tOUT_BB_A;         /* redirector */
case 148: goto tOUT_BB_A;         /* redirector */
case 149: goto tOUT_BB_A;         /* redirector */
case 150: goto tOUT_BB_A;         /* redirector */
case 151: goto tOUT_BB_A;         /* redirector */
case 152: goto tOUT_BB_A;         /* redirector */
case 153: goto tOUT_BB_A;         /* redirector */
case 154: goto tOUT_BB_A;         /* redirector */
case 155: goto tOUT_BB_A;         /* redirector */
case 156: goto tOUT_BB_A;         /* redirector */
case 157: goto tOUT_BB_A;         /* redirector */
case 158: goto tOUT_BB_A;         /* redirector */
case 159: goto tOUT_BB_A;         /* redirector */
case 160: goto opLDAdir_BB_AA;
case 161: goto opLDAdir_BB_AA;
case 162: goto opLDAdir_BB_AA;
case 163: goto opLDAdir_BB_AA;
case 164: goto opLDAdir_BB_AA;
case 165: goto opLDAdir_BB_AA;
case 166: goto opLDAdir_BB_AA;
case 167: goto opLDAdir_BB_AA;
case 168: goto opLDAdir_BB_AA;
case 169: goto opLDAdir_BB_AA;
case 170: goto opLDAdir_BB_AA;
case 171: goto opLDAdir_BB_AA;
case 172: goto opLDAdir_BB_AA;
case 173: goto opLDAdir_BB_AA;
case 174: goto opLDAdir_BB_AA;
case 175: goto opLDAdir_BB_AA;
case 176: goto opCMPdir_BB_AA;
case 177: goto opCMPdir_BB_AA;
case 178: goto opCMPdir_BB_AA;
case 179: goto opCMPdir_BB_AA;
case 180: goto opCMPdir_BB_AA;
case 181: goto opCMPdir_BB_AA;
case 182: goto opCMPdir_BB_AA;
case 183: goto opCMPdir_BB_AA;
case 184: goto opCMPdir_BB_AA;
case 185: goto opCMPdir_BB_AA;
case 186: goto opCMPdir_BB_AA;
case 187: goto opCMPdir_BB_AA;
case 188: goto opCMPdir_BB_AA;
case 189: goto opCMPdir_BB_AA;
case 190: goto opCMPdir_BB_AA;
case 191: goto opCMPdir_BB_AA;
case 192: goto opLDIdir_BB_A;
case 193: goto opLDIdir_BB_A;
case 194: goto opLDIdir_BB_A;
case 195: goto opLDIdir_BB_A;
case 196: goto opLDIdir_BB_A;
case 197: goto opLDIdir_BB_A;
case 198: goto opLDIdir_BB_A;
case 199: goto opLDIdir_BB_A;
case 200: goto opLDIdir_BB_A;
case 201: goto opLDIdir_BB_A;
case 202: goto opLDIdir_BB_A;
case 203: goto opLDIdir_BB_A;
case 204: goto opLDIdir_BB_A;
case 205: goto opLDIdir_BB_A;
case 206: goto opLDIdir_BB_A;
case 207: goto opLDIdir_BB_A;
case 208: goto opSTAdir_BB_A;
case 209: goto opSTAdir_BB_A;
case 210: goto opSTAdir_BB_A;
case 211: goto opSTAdir_BB_A;
case 212: goto opSTAdir_BB_A;
case 213: goto opSTAdir_BB_A;
case 214: goto opSTAdir_BB_A;
case 215: goto opSTAdir_BB_A;
case 216: goto opSTAdir_BB_A;
case 217: goto opSTAdir_BB_A;
case 218: goto opSTAdir_BB_A;
case 219: goto opSTAdir_BB_A;
case 220: goto opSTAdir_BB_A;
case 221: goto opSTAdir_BB_A;
case 222: goto opSTAdir_BB_A;
case 223: goto opSTAdir_BB_A;
case 224: goto opVDR_BB_A;
case 225: goto opLDJirg_BB_A;
case 226: goto opXLT_BB_AA;
case 227: goto opMULirg_BB_AA;
case 228: goto opLLT_BB_AA;
case 229: goto opWAI_BB_A;
case 230: goto opSTAirg_BB_A;
case 231: goto opADDirg_BB_AA;
case 232: goto opSUBirg_BB_AA;
case 233: goto opANDirg_BB_AA;
case 234: goto opLDAirg_BB_AA;
case 235: goto opLSRe_BB_AA;
case 236: goto opLSLe_BB_AA;
case 237: goto opASRe_BB_AA;
case 238: goto opASRDe_BB_AA;
case 239: goto opLSLDe_BB_AA;
case 240: goto opVIN_BB_A;
case 241: goto opLDJirg_BB_A;
case 242: goto opXLT_BB_AA;
case 243: goto opMULirg_BB_AA;
case 244: goto opLLT_BB_AA;
case 245: goto opWAI_BB_A;
case 246: goto opSTAirg_BB_A;
case 247: goto opAWDirg_BB_AA;
case 248: goto opSUBirg_BB_AA;
case 249: goto opANDirg_BB_AA;
case 250: goto opLDAirg_BB_AA;
case 251: goto opLSRf_BB_AA;
case 252: goto opLSLf_BB_AA;
case 253: goto opASRf_BB_AA;
case 254: goto opASRDf_BB_AA;
case 255: goto opLSLDf_BB_AA;
} /* switch on opcode */

  /* the actual opcode code; each piece should be careful to
   * (1) set the correct RCstate
   * (2) increment the program counter as necessary
   * (3) piss with the flags as needed
   * otherwise the next opcode will be completely buggered.
   */

opINP_A_AA:
opINP_AA_AA:
opINP_BB_AA:
  /* bottom 4 bits of opcode are the position of the bit we want;
   * obtain input value, shift over that no, and truncate to last bit.
   * NOTE: Masking 0x07 does interesting things on Sundance and
   * others, but masking 0x0F makes RipOff and others actually work :)
   */
  RCcmp_new = ( ioInputs >> ( rom [ RCregister_PC ] & 0x0F ) ) & 0x01;

  SETA0 ( RCregister_A );               /* save old accA bit0 */
  SETFC ( RCregister_A );

  RCcmp_old = RCregister_A;               /* save old accB */
  RCregister_A = RCcmp_new;               /* load new accB; zero other bits */

  RCregister_PC ++;
  jumpCineRet_AA;

opINP_B_AA:
  /* bottom 3 bits of opcode are the position of the bit we want;
   * obtain Switches value, shift over that no, and truncate to last bit.
   */
  RCcmp_new = ( ioSwitches >> ( rom [ RCregister_PC ] & 0x07 ) ) & 0x01;

  SETA0 ( RCregister_A );               /* save old accA bit0 */
  SETFC ( RCregister_A );

  RCcmp_old = RCregister_B;               /* save old accB */
  RCregister_B = RCcmp_new;               /* load new accB; zero other bits */

  RCregister_PC ++;
  jumpCineRet_AA;

opOUTbi_A_A:
opOUTbi_AA_A:
opOUTbi_BB_A:

  temp_byte = rom [ RCregister_PC ] & 0x07;
  RCregister_PC++;

  if ( temp_byte - 0x06 ) {
    goto opOUTsnd_A;
  }

  RCvgColour = ( ( RCregister_A & 0x01 ) << 3 ) | 0x07;

  jumpCineRet_A;

opOUT16_A_A:
opOUT16_AA_A:
opOUT16_BB_A:

  temp_byte = rom [ RCregister_PC ] & 0x07;
  RCregister_PC++;

  if ( temp_byte - 0x06 ) {
    goto opOUTsnd_A;
  }

  if ( ( RCregister_A & 0xFF ) != 1 ) {
    RCvgColour = RCFromX & 0x0F;

    if ( ! RCvgColour ) {
      RCvgColour = 1;
    }

  }

  jumpCineRet_A;

opOUTsnd_A:
  temp_byte = 0x01 << ( rom [ RCregister_PC ] & 0x07 );

  if ( ! ( RCregister_A & 0x01 ) ) {
    goto opOUT_Aset;
  }

  temp_byte = ( ! temp_byte );   /* BUG?  Should this not be ~temp_byte */
  ioOutputs &= temp_byte;

  if ( ( rom [ RCregister_PC ] & 0x07 ) == 0x05 ) {
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
  temp_byte = rom [ RCregister_PC ] & 0x07;
  RCregister_PC++;

  if ( temp_byte - 0x06 ) {
    goto opOUTsnd_A;
  }

  if ( ( RCregister_A & 0xFF ) == 1 ) {
    temp_word = ( ! RCFromX ) & 0xFFF;

    if ( ! temp_word ) {   /* black */

      RCvgColour = 0;

    } else {              /* non-black */

      if ( temp_word & 0x0888 ) {
        /* bright */

        temp_word_2 = ( ( temp_word << 4 ) & 0x8000 ) >> 15;
        temp_byte = ( temp_byte << 1 ) + temp_word_2;

        temp_word_2 = ( ( temp_word << 3 ) & 0x8000 ) >> 15;
        temp_byte = ( temp_byte << 1 ) + temp_word_2;

        temp_word_2 = ( ( temp_word << 3 ) & 0x8000 ) >> 15;
        temp_byte = ( temp_byte << 1 ) + temp_word_2;

        RCvgColour = ( temp_byte & 0x07 ) + 7;

      } else if ( temp_word & 0x0444 ) {
        /* dim bits */

        temp_word_2 = ( ( temp_word << 5 ) & 0x8000 ) >> 15;
        temp_byte = ( temp_byte << 1 ) + temp_word_2;

        temp_word_2 = ( ( temp_word << 3 ) & 0x8000 ) >> 15;
        temp_byte = ( temp_byte << 1 ) + temp_word_2;

        temp_word_2 = ( ( temp_word << 3 ) & 0x8000 ) >> 15;
        temp_byte = ( temp_byte << 1 ) + temp_word_2;

        RCvgColour = ( temp_byte & 0x07 );

      } else {
        /* dim white */
        RCvgColour = 0x0F;

      }

    }

  } /* colour change? == 1 */

  jumpCineRet_A;

opOUTbi_B_BB:
  temp_byte = rom [ RCregister_PC ] & 0x07;
  RCregister_PC++;

  if ( temp_byte - 0x06 ) {
    goto opOUTsnd_B;
  }

  RCvgColour = ( ( RCregister_B & 0x01 ) << 3 ) | 0x07;

  jumpCineRet_BB;

opOUT16_B_BB:
  temp_byte = rom [ RCregister_PC ] & 0x07;
  RCregister_PC++;

  if ( temp_byte - 0x06 ) {
    goto opOUTsnd_B;
  }

  if ( ( RCregister_B & 0xFF ) != 1 ) {
    RCvgColour = RCFromX & 0x0F;

    if ( ! RCvgColour ) {
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
  temp_word = rom [ RCregister_PC ] & 0x0F;   /* pick up immediate value */
  temp_word <<= 8;                          /* LDAimm is the HIGH nibble! */

  RCcmp_new = temp_word;                      /* set new comparison flag */

  SETA0 ( RCregister_A );                     /* save old accA bit0 */
  SETFC ( RCregister_A );                     /* ??? clear carry? */

  RCcmp_old = RCregister_A;                     /* step back cmp flag */
  RCregister_A = temp_word;                   /* set the register */

  RCregister_PC++;                            /* increment PC */
  jumpCineRet_AA;                           /* swap RCstate and end opcode */

opLDAimm_B_AA:
  temp_word = rom [ RCregister_PC ] & 0x0F;   /* pick up immediate value */
  temp_word <<= 8;                          /* LDAimm is the HIGH nibble! */

  RCcmp_new = temp_word;                      /* set new comparison flag */

  SETA0 ( RCregister_A );                     /* save old accA bit0 */
  SETFC ( RCregister_A );

  RCcmp_old = RCregister_B;                     /* step back cmp flag */
  RCregister_B = temp_word;                   /* set the register */

  RCregister_PC++;                            /* increment PC */
  jumpCineRet_AA;

opLDAdir_A_AA:
opLDAdir_AA_AA:
opLDAdir_BB_AA:

  temp_byte = rom [ RCregister_PC ] & 0x0F;        /* snag imm value */
  RCregister_I = ( RCregister_P << 4 ) + temp_byte;  /* set I register */

  RCcmp_new = RCram[ RCregister_I ];                  /* new acc value */

  SETA0 ( RCregister_A );                          /* back up bit0 */
  SETFC ( RCregister_A );

  RCcmp_old = RCregister_A;                          /* store old acc */
  RCregister_A = RCcmp_new;                          /* store new acc */

  RCregister_PC ++;                                /* bump PC */
  jumpCineRet_AA;

opLDAdir_B_AA:

  temp_byte = rom [ RCregister_PC ] & 0x0F;        /* snag imm value */
  RCregister_I = ( RCregister_P << 4 ) + temp_byte;  /* set I register */

  RCcmp_new = RCram[ RCregister_I ];                  /* new acc value */

  SETA0 ( RCregister_A );                          /* back up bit0 */
  SETFC ( RCregister_A );

  RCcmp_old = RCregister_B;                          /* store old acc */
  RCregister_B = RCcmp_new;                          /* store new acc */

  RCregister_PC ++;                                /* bump PC */
  jumpCineRet_AA;

opLDAirg_A_AA:
opLDAirg_AA_AA:
opLDAirg_BB_AA:

  RCcmp_new = RCram[ RCregister_I ];

  SETA0 ( RCregister_A );
  SETFC ( RCregister_A );

  RCcmp_old = RCregister_A;
  RCregister_A = RCcmp_new;

  RCregister_PC++;
  jumpCineRet_AA;

opLDAirg_B_AA:
  RCcmp_new = RCram[ RCregister_I ];

  SETA0 ( RCregister_A );
  SETFC ( RCregister_A );

  RCcmp_old = RCregister_B;
  RCregister_B = RCcmp_new;

  RCregister_PC++;
  jumpCineRet_AA;

  /* ADD imm */
opADDimm_A_AA:
opADDimm_AA_AA:
opADDimm_BB_AA:
  temp_word = rom [ RCregister_PC ] & 0x0F;     /* get imm value */

  RCcmp_new = temp_word;                        /* new acc value */
  SETA0 ( RCregister_A );                       /* save old accA bit0 */
  RCcmp_old = RCregister_A;                       /* store old acc for later */

  RCregister_A += temp_word;                    /* add values */
  SETFC ( RCregister_A );                       /* store carry and extra */
  RCregister_A &= 0xFFF;                        /* toss out >12bit carry */

  RCregister_PC ++;
  jumpCineRet_AA;

opADDimm_B_AA:
  temp_word = rom [ RCregister_PC ] & 0x0F;     /* get imm value */

  RCcmp_new = temp_word;                        /* new acc value */
  SETA0 ( RCregister_A );                       /* save old accA bit0 */
  RCcmp_old = RCregister_B;                       /* store old acc for later */

  RCregister_B += temp_word;                    /* add values */
  SETFC ( RCregister_B );                       /* store carry and extra */
  RCregister_B &= 0xFFF;                        /* toss out >12bit carry */

  RCregister_PC ++;
  jumpCineRet_AA;

  /* ADD imm extended */
opADDimmX_A_AA:
opADDimmX_AA_AA:
opADDimmX_BB_AA:
  RCcmp_new = rom [ RCregister_PC + 1 ];          /* get extended value */
  SETA0 ( RCregister_A );                       /* save old accA bit0 */
  RCcmp_old = RCregister_A;                       /* store old acc for later */

  RCregister_A += RCcmp_new;                      /* add values */
  SETFC ( RCregister_A );                       /* store carry and extra */
  RCregister_A &= 0xFFF;                        /* toss out >12bit carry */

  RCregister_PC += 2;                           /* bump PC */
  jumpCineRet_AA;

opADDimmX_B_AA:
  RCcmp_new = rom [ RCregister_PC + 1 ];          /* get extended value */
  SETA0 ( RCregister_A );                       /* save old accA bit0 */
  RCcmp_old = RCregister_B;                       /* store old acc for later */

  RCregister_B += RCcmp_new;                      /* add values */
  SETFC ( RCregister_B );                       /* store carry and extra */
  RCregister_B &= 0xFFF;                        /* toss out >12bit carry */

  RCregister_PC += 2;                           /* bump PC */
  jumpCineRet_AA;

opADDdir_A_AA:
opADDdir_AA_AA:
opADDdir_BB_AA:

  temp_byte = rom [ RCregister_PC ] & 0x0F;         /* fetch imm value */
  RCregister_I = ( RCregister_P << 4 ) + temp_byte;   /* set regI addr */

  RCcmp_new = RCram[ RCregister_I ];                   /* fetch imm real value */
  SETA0 ( RCregister_A );                           /* store bit0 */
  RCcmp_old = RCregister_A;                           /* store old acc value */

  RCregister_A += RCcmp_new;                          /* do acc operation */
  SETFC ( RCregister_A );                           /* store carry and extra */
  RCregister_A &= 0xFFF;

  RCregister_PC ++;                                 /* bump PC */
  jumpCineRet_AA;

opADDdir_B_AA:
  temp_byte = rom [ RCregister_PC ] & 0x0F;         /* fetch imm value */
  RCregister_I = ( RCregister_P << 4 ) + temp_byte;   /* set regI addr */

  RCcmp_new = RCram[ RCregister_I ];                   /* fetch imm real value */
  SETA0 ( RCregister_A );                           /* store bit0 */
  RCcmp_old = RCregister_B;                           /* store old acc value */

  RCregister_B += RCcmp_new;                          /* do acc operation */
  SETFC ( RCregister_B );                           /* store carry and extra */
  RCregister_B &= 0xFFF;

  RCregister_PC ++;                                 /* bump PC */
  jumpCineRet_AA;

opAWDirg_A_AA:
opAWDirg_AA_AA:
opAWDirg_BB_AA:
opADDirg_A_AA:
opADDirg_AA_AA:
opADDirg_BB_AA:

  RCcmp_new = RCram[ RCregister_I ];
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_A;

  RCregister_A += RCcmp_new;
  SETFC ( RCregister_A );
  RCregister_A &= 0xFFF;

  RCregister_PC++;
  jumpCineRet_AA;

opAWDirg_B_AA:
opADDirg_B_AA:
  RCcmp_new = RCram[ RCregister_I ];
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_B;

  RCregister_B += RCcmp_new;
  SETFC ( RCregister_B );
  RCregister_B &= 0xFFF;

  RCregister_PC++;
  jumpCineRet_AA;

opSUBimm_A_AA:
opSUBimm_AA_AA:
opSUBimm_BB_AA:
  /* SUBtractions are negate-and-add instructions of the CCPU; what
   * a pain in the ass.
   */
  temp_word = rom [ RCregister_PC ] & 0x0F;

  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_A;

  temp_word = ( temp_word ^ 0xFFF ) + 1;         /* ones compliment */
  RCregister_A += temp_word;                       /* add */
  SETFC ( RCregister_A );                          /* pick up top bits */
  RCregister_A &= 0xFFF;                          /* mask final regA value */

  RCregister_PC ++;                                /* bump PC */
  jumpCineRet_AA;

opSUBimm_B_AA:
  /* SUBtractions are negate-and-add instructions of the CCPU; what
   * a pain in the ass.
   */
  temp_word = rom [ RCregister_PC ] & 0x0F;

  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_B;

  temp_word = ( temp_word ^ 0xFFF ) + 1;         /* ones compliment */
  RCregister_B += temp_word;                       /* add */
  SETFC ( RCregister_B );                          /* pick up top bits */
  RCregister_B &= 0xFFF;                          /* mask final regA value */

  RCregister_PC ++;                                /* bump PC */
  jumpCineRet_AA;

opSUBimmX_A_AA:
opSUBimmX_AA_AA:
opSUBimmX_BB_AA:

  temp_word = rom [ RCregister_PC + 1 ];          /* snag imm value */

  RCcmp_new = temp_word;                          /* save cmp value */
  SETA0 ( RCregister_A );                         /* store bit0 */
  RCcmp_old = RCregister_A;                         /* back up regA */

  temp_word = ( temp_word ^ 0xFFF ) + 1;        /* ones compliment */
  RCregister_A += temp_word;                      /* add */
  SETFC ( RCregister_A );                         /* pick up top bits */
  RCregister_A &= 0xFFF;                         /* mask final regA value */

  RCregister_PC += 2;                             /* bump PC */
  jumpCineRet_AA;

opSUBimmX_B_AA:

  temp_word = rom [ RCregister_PC + 1 ];          /* snag imm value */

  RCcmp_new = temp_word;                          /* save cmp value */
  SETA0 ( RCregister_A );                         /* store bit0 */
  RCcmp_old = RCregister_B;                         /* back up regA */

  temp_word = ( temp_word ^ 0xFFF ) + 1;        /* ones compliment */
  RCregister_B += temp_word;                      /* add */
  SETFC ( RCregister_B );                         /* pick up top bits */
  RCregister_B &= 0xFFF;                         /* mask final regA value */

  RCregister_PC += 2;                             /* bump PC */
  jumpCineRet_AA;

opSUBdir_A_AA:
opSUBdir_AA_AA:
opSUBdir_BB_AA:
  temp_word = rom [ RCregister_PC ] & 0x0F;         /* fetch imm value */
  RCregister_I = ( RCregister_P << 4 ) + temp_word;   /* set regI addr */

  RCcmp_new = RCram[ RCregister_I ];
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_A;

  temp_word = ( RCcmp_new ^ 0xFFF ) + 1;           /* ones compliment */
  RCregister_A += temp_word;                       /* add */
  SETFC ( RCregister_A );                          /* pick up top bits */
  RCregister_A &= 0xFFF;                          /* mask final regA value */

  RCregister_PC ++;
  jumpCineRet_AA;

opSUBdir_B_AA:

  temp_byte = rom [ RCregister_PC ] & 0x0F;         /* fetch imm value */
  RCregister_I = ( RCregister_P << 4 ) + temp_byte;   /* set regI addr */

  RCcmp_new = RCram[ RCregister_I ];
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_B;

  temp_word = ( RCcmp_new ^ 0xFFF ) + 1;           /* ones compliment */
  RCregister_B += temp_word;                       /* add */
  SETFC ( RCregister_B );                          /* pick up top bits */
  RCregister_B &= 0xFFF;                          /* mask final regA value */

  RCregister_PC ++;
  jumpCineRet_AA;

opSUBirg_A_AA:
opSUBirg_AA_AA:
opSUBirg_BB_AA:
  /* sub [i] */
  RCcmp_new = RCram[ RCregister_I ];
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_A;

  temp_word = ( RCcmp_new ^ 0xFFF ) + 1;           /* ones compliment */
  RCregister_A += temp_word;                       /* add */
  SETFC ( RCregister_A );                          /* pick up top bits */
  RCregister_A &= 0xFFF;                          /* mask final regA value */

  RCregister_PC ++;
  jumpCineRet_AA;

opSUBirg_B_AA:
  /* sub [i] */
  RCcmp_new = RCram[ RCregister_I ];
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_B;

  temp_word = ( RCcmp_new ^ 0xFFF ) + 1;           /* ones compliment */
  RCregister_B += temp_word;                       /* add */
  SETFC ( RCregister_B );                          /* pick up top bits */
  RCregister_B &= 0xFFF;                          /* mask final regA value */

  RCregister_PC ++;
  jumpCineRet_AA;

  /* CMP dir */
opCMPdir_A_AA:
opCMPdir_AA_AA:
opCMPdir_BB_AA:
  /* compare direct mode; don't modify regs, just set carry flag or not.
   */

  temp_byte = rom [ RCregister_PC ] & 0x0F;       /* obtain relative addr */
  RCregister_I = ( RCregister_P << 4 ) + temp_byte; /* build real addr */

  temp_word = RCram[ RCregister_I ];
  RCcmp_new = temp_word;                          /* new acc value */
  SETA0 ( RCregister_A );                         /* backup bit0 */
  RCcmp_old = RCregister_A;                         /* backup old acc */

  temp_word = ( temp_word ^ 0xFFF ) + 1;        /* ones compliment */
  temp_word += RCregister_A;
  SETFC ( temp_word );                          /* pick up top bits */

  RCregister_PC ++;                               /* bump PC */
  jumpCineRet_AA;

opCMPdir_B_AA:
  temp_byte = rom [ RCregister_PC ] & 0x0F;       /* obtain relative addr */
  RCregister_I = ( RCregister_P << 4 ) + temp_byte; /* build real addr */

  temp_word = RCram[ RCregister_I ];
  RCcmp_new = temp_word;                          /* new acc value */
  SETA0 ( RCregister_A );                         /* backup bit0 */
  RCcmp_old = RCregister_B;                         /* backup old acc */

  temp_word = ( temp_word ^ 0xFFF ) + 1;        /* ones compliment */
  temp_word += RCregister_B;
  SETFC ( temp_word );                          /* pick up top bits */

  RCregister_PC ++;                               /* bump PC */
  jumpCineRet_AA;

  /* AND [i] */
opANDirg_A_AA:
opANDirg_AA_AA:
opANDirg_BB_AA:
  RCcmp_new = RCram[ RCregister_I ];                /* new acc value */
  SETA0 ( RCregister_A );
  SETFC ( RCregister_A );
  RCcmp_old = RCregister_A;

  RCregister_A &= RCcmp_new;

  RCregister_PC ++;                              /* bump PC */
  jumpCineRet_AA;

opANDirg_B_AA:
  RCcmp_new = RCram[ RCregister_I ];                /* new acc value */
  SETA0 ( RCregister_A );
  SETFC ( RCregister_A );
  RCcmp_old = RCregister_B;

  RCregister_B &= RCcmp_new;

  RCregister_PC ++;                              /* bump PC */
  jumpCineRet_AA;

  /* LDJ imm */
opLDJimm_A_A:
opLDJimm_AA_A:
opLDJimm_BB_A:
  temp_byte = rom [ RCregister_PC + 1 ];         /* upper part of address */
  temp_byte = ( temp_byte << 4 ) |             /* Silly CCPU; Swap */
              ( temp_byte >> 4 );              /* nibbles */

  /* put the upper 8 bits above the existing 4 bits */
  RCregister_J = ( rom [ RCregister_PC ] & 0x0F ) | ( temp_byte << 4 );

  RCregister_PC += 2;
  jumpCineRet_A;

opLDJimm_B_BB:
  temp_byte = rom [ RCregister_PC + 1 ];         /* upper part of address */
  temp_byte = ( temp_byte << 4 ) |             /* Silly CCPU; Swap */
              ( temp_byte >> 4 );              /* nibbles */

  /* put the upper 8 bits above the existing 4 bits */
  RCregister_J = ( rom [ RCregister_PC ] & 0x0F ) | ( temp_byte << 4 );

  RCregister_PC += 2;
  jumpCineRet_BB;

  /* LDJ irg */
opLDJirg_A_A:
opLDJirg_AA_A:
opLDJirg_BB_A:
  /* load J reg from value at last dir addr */
  RCregister_J = RCram[ RCregister_I ];
  RCregister_PC++;                             /* bump PC */
  jumpCineRet_A;

opLDJirg_B_BB:
  RCregister_J = RCram[ RCregister_I ];
  RCregister_PC++;                             /* bump PC */
  jumpCineRet_BB;

  /* LDP imm */
opLDPimm_A_A:
opLDPimm_AA_A:
opLDPimm_BB_A:
  /* load page register from immediate */
  RCregister_P = rom [ RCregister_PC ] & 0x0F;  /* set page register */
  RCregister_PC ++;  /* inc PC */
  jumpCineRet_A;

opLDPimm_B_BB:
  /* load page register from immediate */
  RCregister_P = rom [ RCregister_PC ] & 0x0F;  /* set page register */
  RCregister_PC ++;  /* inc PC */
  jumpCineRet_BB;

  /* LDI dir */
opLDIdir_A_A:
opLDIdir_AA_A:
opLDIdir_BB_A:
  /* load regI directly .. */

  temp_byte = ( RCregister_P << 4 ) +           /* get rampage ... */
              ( rom [ RCregister_PC ] & 0x0F ); /* and imm half of ramaddr.. */

  RCregister_I = RCram[ temp_byte ] & 0xFF;      /* set/mask new RCregister_I */

  RCregister_PC ++;
  jumpCineRet_A;

opLDIdir_B_BB:
  temp_byte = ( RCregister_P << 4 ) +           /* get rampage ... */
              ( rom [ RCregister_PC ] & 0x0F ); /* and imm half of ramaddr.. */

  RCregister_I = RCram[ temp_byte ] & 0xFF;      /* set/mask new RCregister_I */

  RCregister_PC ++;
  jumpCineRet_BB;

  /* STA dir */
opSTAdir_A_A:
opSTAdir_AA_A:
opSTAdir_BB_A:
  temp_byte = rom [ RCregister_PC ] & 0x0F;        /* snag imm value */
  RCregister_I = ( RCregister_P << 4 ) + temp_byte;  /* set I register */

  RCram[ RCregister_I ] = RCregister_A;               /* store acc to RCram*/

  RCregister_PC ++;                                /* inc PC */
  jumpCineRet_A;

opSTAdir_B_BB:
  temp_byte = rom [ RCregister_PC ] & 0x0F;        /* snag imm value */
  RCregister_I = ( RCregister_P << 4 ) + temp_byte;  /* set I register */

  RCram[ RCregister_I ] = RCregister_B;               /* store acc to RCram*/

  RCregister_PC ++;                                /* inc PC */
  jumpCineRet_BB;

  /* STA irg */
opSTAirg_A_A:
opSTAirg_AA_A:
opSTAirg_BB_A:
  /* STA into address specified in regI. Nice and easy :)
   */

  RCram[ RCregister_I ] = RCregister_A;               /* store acc */

  RCregister_PC ++;                                /* bump PC */
  jumpCineRet_A;

opSTAirg_B_BB:

  RCram[ RCregister_I ] = RCregister_B;               /* store acc */

  RCregister_PC ++;                                /* bump PC */
  jumpCineRet_BB;

  /* XLT */
opXLT_A_AA:
opXLT_AA_AA:
opXLT_BB_AA:
  /* XLT is weird; it loads the current accumulator with the bytevalue
   * at ROM location pointed to by the accumulator; this allows the
   * programto read the programitself..
   *   NOTE! Next opcode is *IGNORED!* because of a twisted side-effect
   */

  RCcmp_new = rom [ ( RCregister_PC & 0xF000 ) + RCregister_A ];   /* store new acc value */
  SETA0 ( RCregister_A );           /* store bit0 */
  SETFC ( RCregister_A );
  RCcmp_old = RCregister_A;           /* back up acc */

  RCregister_A = RCcmp_new;           /* new acc value */

  RCregister_PC += 2;               /* bump PC twice because XLT is fucked up */
  jumpCineRet_AA;

opXLT_B_AA:

  RCcmp_new = rom [ ( RCregister_PC & 0xF000 ) + RCregister_B ];   /* store new acc value */
  SETA0 ( RCregister_A );           /* store bit0 */
  SETFC ( RCregister_A );
  RCcmp_old = RCregister_B;           /* back up acc */

  RCregister_B = RCcmp_new;           /* new acc value */

  RCregister_PC += 2;               /* bump PC twice because XLT is fucked up */
  jumpCineRet_AA;

  /* MUL [i] */
opMULirg_A_AA:
opMULirg_AA_AA:
opMULirg_BB_AA:
  /* MUL's usually happen in batches, so a slight speed bump can be
   * gained by checking for multiple instances and handling in here,
   * without going through the main loop for each.
   */
  temp_word = RCram[ RCregister_I ];               /* pick up ramvalue */
  RCcmp_new = temp_word;

  temp_word <<= 4;                              /* shift into ADD position */
  RCregister_B <<= 4;                             /* get sign bit 15 */
  RCregister_B |= ( RCregister_A >> 8 );            /* bring in A high nibble */

  RCregister_A = ( ( RCregister_A & 0xFF ) << 8 ) | /* shift over 8 bits */
               ( rom [ RCregister_PC ] & 0xFF );  /* pick up opcode */

  temp_byte = rom [ RCregister_PC ] & 0xFF;       /* (for ease and speed) */

  /* handle multiple consecutive MUL's
   */

  RCregister_PC++;                               /* inc PC */

#ifdef DUALCPU
  goto opMUL1;
#endif

  if ( rom [ RCregister_PC ] != temp_byte ) {    /* next opcode is a MUL? */
    goto opMUL1;                               /* no? skip multiples... */
  }

  RCregister_PC++;                               /* repeat above */
  if ( rom [ RCregister_PC ] != temp_byte ) {
    goto opMUL2;
  }

  RCregister_PC++;                               /* repeat above */
  if ( rom [ RCregister_PC ] != temp_byte ) {
    goto opMUL3;
  }

  RCregister_PC++;                               /* repeat above */
  if ( rom [ RCregister_PC ] != temp_byte ) {
    goto opMUL4;
  }

  RCregister_PC++;                               /* repeat above */
  if ( rom [ RCregister_PC ] != temp_byte ) {
    goto opMUL5;
  }

  RCregister_PC++;                               /* repeat above */
  if ( rom [ RCregister_PC ] != temp_byte ) {
    goto opMUL6;
  }

  RCregister_PC++;                               /* repeat above */
  if ( rom [ RCregister_PC ] != temp_byte ) {
    goto opMUL7;
  }

  RCregister_PC++;                               /* repeat above */
  if ( rom [ RCregister_PC ] != temp_byte ) {
    goto opMUL8;
  }

  RCregister_PC++;                               /* repeat above */
  if ( rom [ RCregister_PC ] != temp_byte ) {
    goto opMUL9;
  }

  RCregister_PC++;                               /* repeat above */
  if ( rom [ RCregister_PC ] != temp_byte ) {
    goto opMUL10;
  }

  RCregister_PC++;                               /* repeat above */
  if ( rom [ RCregister_PC ] != temp_byte ) {
    goto opMUL11;
  }

opMUL12:
  RCregister_PC++;                               /* we don't bother to check
                                                * for multiple occurances more
                                                * than 12, so just inc the PC
                                                * and not worry about it.
                                                */

  temp_word_2 = ( RCregister_B & 0x01 ) << 15;   /* get low bit for rotation */
  RCregister_B = SAR16(RCregister_B,1);              /* signed arith right 1 */
  RCregister_A = ( RCregister_A >> 1 ) |           /* rotate right ... */
               temp_word_2;                    /*   ... via carry flag */
  if ( ! ( RCregister_A & 0x80 ) ) {
    goto opMUL11;
  }
  RCregister_B += temp_word;

opMUL11:
  temp_word_2 = ( RCregister_B & 0x01 ) << 15;   /* get low bit for rotation */
  RCregister_B = SAR16(RCregister_B,1);              /* signed arith right 1 */
  RCregister_A = ( RCregister_A >> 1 ) |           /* rotate right ... */
               temp_word_2;                    /*   ... via carry flag */
  if ( ! ( RCregister_A & 0x80 ) ) {
    goto opMUL10;
  }
  RCregister_B += temp_word;

opMUL10:
  temp_word_2 = ( RCregister_B & 0x01 ) << 15;   /* get low bit for rotation */
  RCregister_B = SAR16(RCregister_B,1);              /* signed arith right 1 */
  RCregister_A = ( RCregister_A >> 1 ) |           /* rotate right ... */
               temp_word_2;                    /*   ... via carry flag */
  if ( ! ( RCregister_A & 0x80 ) ) {
    goto opMUL9;
  }
  RCregister_B += temp_word;

opMUL9:
  temp_word_2 = ( RCregister_B & 0x01 ) << 15;   /* get low bit for rotation */
  RCregister_B = SAR16(RCregister_B,1);              /* signed arith right 1 */
  RCregister_A = ( RCregister_A >> 1 ) |           /* rotate right ... */
               temp_word_2;                    /*   ... via carry flag */
  if ( ! ( RCregister_A & 0x80 ) ) {
    goto opMUL8;
  }
  RCregister_B += temp_word;

opMUL8:
  temp_word_2 = ( RCregister_B & 0x01 ) << 15;   /* get low bit for rotation */
  RCregister_B = SAR16(RCregister_B,1);              /* signed arith right 1 */
  RCregister_A = ( RCregister_A >> 1 ) |           /* rotate right ... */
               temp_word_2;                    /*   ... via carry flag */
  if ( ! ( RCregister_A & 0x80 ) ) {
    goto opMUL7;
  }
  RCregister_B += temp_word;

opMUL7:
  temp_word_2 = ( RCregister_B & 0x01 ) << 15;   /* get low bit for rotation */
  RCregister_B = SAR16(RCregister_B,1);              /* signed arith right 1 */
  RCregister_A = ( RCregister_A >> 1 ) |           /* rotate right ... */
               temp_word_2;                    /*   ... via carry flag */
  if ( ! ( RCregister_A & 0x80 ) ) {
    goto opMUL6;
  }
  RCregister_B += temp_word;

opMUL6:
  temp_word_2 = ( RCregister_B & 0x01 ) << 15;   /* get low bit for rotation */
  RCregister_B = SAR16(RCregister_B,1);              /* signed arith right 1 */
  RCregister_A = ( RCregister_A >> 1 ) |           /* rotate right ... */
               temp_word_2;                    /*   ... via carry flag */
  if ( ! ( RCregister_A & 0x80 ) ) {
    goto opMUL5;
  }
  RCregister_B += temp_word;

opMUL5:
  temp_word_2 = ( RCregister_B & 0x01 ) << 15;   /* get low bit for rotation */
  RCregister_B = SAR16(RCregister_B,1);              /* signed arith right 1 */
  RCregister_A = ( RCregister_A >> 1 ) |           /* rotate right ... */
               temp_word_2;                    /*   ... via carry flag */
  if ( ! ( RCregister_A & 0x80 ) ) {
    goto opMUL4;
  }
  RCregister_B += temp_word;

opMUL4:
  temp_word_2 = ( RCregister_B & 0x01 ) << 15;   /* get low bit for rotation */
  RCregister_B = SAR16(RCregister_B,1);              /* signed arith right 1 */
  RCregister_A = ( RCregister_A >> 1 ) |           /* rotate right ... */
               temp_word_2;                    /*   ... via carry flag */
  if ( ! ( RCregister_A & 0x80 ) ) {
    goto opMUL3;
  }
  RCregister_B += temp_word;

opMUL3:
  temp_word_2 = ( RCregister_B & 0x01 ) << 15;   /* get low bit for rotation */
  RCregister_B = SAR16(RCregister_B,1);              /* signed arith right 1 */
  RCregister_A = ( RCregister_A >> 1 ) |           /* rotate right ... */
               temp_word_2;                    /*   ... via carry flag */
  if ( ! ( RCregister_A & 0x80 ) ) {
    goto opMUL2;
  }
  RCregister_B += temp_word;

opMUL2:
  temp_word_2 = ( RCregister_B & 0x01 ) << 15;   /* get low bit for rotation */
  RCregister_B = SAR16(RCregister_B,1);              /* signed arith right 1 */
  RCregister_A = ( RCregister_A >> 1 ) |           /* rotate right ... */
               temp_word_2;                    /*   ... via carry flag */
  if ( ! ( RCregister_A & 0x80 ) ) {
    goto opMUL1;
  }
  RCregister_B += temp_word;

opMUL1:

  if ( RCregister_A & 0x100 ) {           /* 1bit shifted out? */
    goto opMULshf;
  }

  RCregister_A = ( RCregister_A >> 8 ) |    /* Bhigh | Alow */
               ( ( RCregister_B & 0xFF ) << 8 );

  temp_word = RCregister_A & 0xFFF;

  SETA0 ( temp_word & 0xFF );                   /* store bit0 */
  RCcmp_old = temp_word;

  temp_word += RCcmp_new;
  SETFC ( temp_word );

  RCregister_A >>= 1;
  RCregister_A &= 0xFFF;

  RCregister_B = SAR16(RCregister_B,5);
  RCregister_B &= 0xFFF;

  jumpCineRet_AA;

opMULshf: /* part of opMULirg */

  RCregister_A = ( RCregister_A >> 8 ) |
               ( ( RCregister_B & 0xFF ) << 8 );

  SETA0 ( RCregister_A & 0xFF );                  /* store bit0 */

  RCregister_A >>= 1;
  RCregister_A &= 0xFFF;

  RCregister_B = SAR16(RCregister_B,4);
  RCcmp_old = RCregister_B & 0x0F;

  RCregister_B = SAR16(RCregister_B,1);

  RCregister_B &= 0xFFF;
  RCregister_B += RCcmp_new;

  SETFC ( RCregister_B );

  RCregister_B &= 0xFFF;

  jumpCineRet_AA;

opMULirg_B_AA:
  RCregister_PC++;

  temp_word = RCram[ RCregister_I ];
  RCcmp_new = temp_word;
  RCcmp_old = RCregister_B;
  SETA0 ( RCregister_A & 0xFF );

  RCregister_B <<= 4;

  RCregister_B = SAR16(RCregister_B,5);

  if ( RCregister_A & 0x01 ) {
    goto opMULirgB1;
  }

  temp_word += RCregister_B;
  SETFC ( temp_word );

  jumpCineRet_AA;

opMULirgB1:
  RCregister_B += temp_word;
  SETFC ( RCregister_B );
  RCregister_B &= 0xFFF;
  jumpCineRet_AA;

  /* LSRe */
opLSRe_A_AA:
opLSRe_AA_AA:
opLSRe_BB_AA:
  /* EB; right shift pure; fill new bit with zero.
   */
  temp_word = 0x0BEB;

  RCregister_PC++;

#ifndef DUALCPU
  if ( rom [ RCregister_PC ] == 0xEB ) {
    goto opLSRe_A0; /* multiples */
  }
#endif

  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_A;

  temp_word += RCregister_A;
  SETFC ( temp_word );

  RCregister_A >>= 1;
  jumpCineRet_AA;

opLSRe_A0:
  RCregister_A >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_A1;
  }

  RCregister_A >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_A1;
  }

  RCregister_A >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_A1;
  }

  RCregister_A >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_A1;
  }

  RCregister_A >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_A1;
  }

  RCregister_A >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_A1;
  }

  RCregister_A >>= 1;
  RCregister_PC ++;

opLSRe_A1:
  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_A;

  temp_word += RCregister_A;
  SETFC ( temp_word );

  RCregister_A >>= 1;
  jumpCineRet_AA;

 opLSRe_B_AA: // 235
  //UNFINISHED ( "opLSRe B 1\n" );
  //GTOAL: trying new code. Hope I get it right. ---------------------- NEW CODE:
  
  /* EB; right shift pure; fill new bit with zero.
   */
  temp_word = 0x0BEB;

  RCregister_PC++;

  //if ( rom [ RCregister_PC ] == 0xEB ) {
  //  goto opLSRe_B0; /* multiples */
  //}

  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_B;

  temp_word += RCregister_B;
  SETFC ( temp_word );

  RCregister_B >>= 1;
  jumpCineRet_AA;

opLSRe_B0:
  RCregister_B >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_B1;
  }

  RCregister_B >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_B1;
  }

  RCregister_B >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_B1;
  }

  RCregister_B >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_B1;
  }

  RCregister_B >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_B1;
  }

  RCregister_B >>= 1;
  RCregister_PC ++;
  if ( rom [ RCregister_PC ] != 0xEB ) {
    goto opLSRe_B1;
  }

  RCregister_B >>= 1;
  RCregister_PC ++;

opLSRe_B1:
  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_B;

  temp_word += RCregister_B;
  SETFC ( temp_word );

  RCregister_B >>= 1;
  jumpCineRet_AA;
// --------------------------- end of gtoal addition

opLSRf_A_AA:
opLSRf_AA_AA:
opLSRf_BB_AA:
  UNFINISHED ( "opLSRf 1\n" );
  jumpCineRet_AA;

opLSRf_B_AA:
  UNFINISHED ( "opLSRf 2\n" );
  jumpCineRet_AA;

opLSLe_A_AA:
opLSLe_AA_AA:
opLSLe_BB_AA:
  /* EC; left shift pure; fill new bit with zero */
  /* This version supports multiple consecutive LSLe's; the older
   * version only did one at a time. I'm changing it to make tracing
   * easier (as its comperable to Zonn's)
   */

  RCregister_PC++;
  temp_word = 0x0CEC;

#ifndef DUALCPU
  if ( rom [ RCregister_PC ] == 0xEC ) {
    goto opLSLe_A0; /* do multiples */
  }
#endif

  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_A;

  temp_word += RCregister_A;
  SETFC ( temp_word );

  RCregister_A <<= 1;
  RCregister_A &= 0xFFF;

  jumpCineRet_AA;

opLSLe_A0:
  RCregister_A <<= 1; /* unit begin */
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != 0xEC ) {
    goto opLSLe_A1; /* no more, do last one */
  }

  RCregister_A <<= 1; /* unit begin */
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != 0xEC ) {
    goto opLSLe_A1; /* no more, do last one */
  }

  RCregister_A <<= 1; /* unit begin */
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != 0xEC ) {
    goto opLSLe_A1; /* no more, do last one */
  }

  RCregister_A <<= 1; /* unit begin */
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != 0xEC ) {
    goto opLSLe_A1; /* no more, do last one */
  }

  RCregister_A <<= 1; /* unit begin */
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != 0xEC ) {
    goto opLSLe_A1; /* no more, do last one */
  }

  RCregister_A <<= 1; /* unit begin */
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != 0xEC ) {
    goto opLSLe_A1; /* no more, do last one */
  }

  RCregister_A <<= 1;
  RCregister_PC++;

opLSLe_A1:
  RCregister_A &= 0xFFF;

opLSLe_A2:
  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_A;

  temp_word += RCregister_A;
  SETFC ( temp_word );

  RCregister_A <<= 1;
  RCregister_A &= 0xFFF;

  jumpCineRet_AA;

#if 0 /* non-multiple-consecutive-LSLe's version */
  temp_word = 0x0CEC;                          /* data register */

  RCcmp_new = temp_word;                         /* magic value */
  SETA0 ( RCregister_A );                        /* back up bit0 */
  RCcmp_old = RCregister_A;                        /* store old acc */

  temp_word += RCregister_A;                     /* add to acc */
  SETFC ( temp_word );                         /* store carry flag */
  RCregister_A <<= 1;                            /* add regA to itself */
  RCregister_A &= 0xFFF;                         /* toss excess bits */

  RCregister_PC ++;                              /* bump PC */
  jumpCineRet_AA;
#endif

opLSLe_B_AA:
  temp_word = 0x0CEC;                          /* data register */

  RCcmp_new = temp_word;                         /* magic value */
  SETA0 ( RCregister_A );                        /* back up bit0 */
  RCcmp_old = RCregister_B;                        /* store old acc */

  temp_word += RCregister_B;                     /* add to acc */
  SETFC ( temp_word );                         /* store carry flag */
  RCregister_B <<= 1;                            /* add regA to itself */
  RCregister_B &= 0xFFF;                         /* toss excess bits */

  RCregister_PC ++;                              /* bump PC */
  jumpCineRet_AA;

opLSLf_A_AA:
opLSLf_AA_AA:
opLSLf_BB_AA:
  UNFINISHED ( "opLSLf 1\n" );
  // RCregister_PC ++;                              /* bump PC */  // GTOAL testing to see how bad the damage is

  jumpCineRet_AA;

opLSLf_B_AA:
  UNFINISHED ( "opLSLf 2\n" );
  jumpCineRet_AA;

opASRe_A_AA:
opASRe_AA_AA:
opASRe_BB_AA:
  /* agh! I dislike these silly 12bit processors :P */

  temp_word = 0xDED;

  RCregister_PC ++;

#ifndef DUALCPU /* DON'T OPTIMISE WHEN RUNNING DUAL CPUS */
  if ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) {
    goto opASRe_A0;
  }
#endif
  RCcmp_new = temp_word;

  SETA0 ( RCregister_A );           /* store bit0 */
  SETFC ( RCregister_A );

  RCcmp_old = RCregister_A;

  RCregister_A <<= 4; /* get sign bit */
  RCregister_A = SAR16(RCregister_A,5);
  RCregister_A &= 0xFFF;

  jumpCineRet_AA;

opASRe_A0:
  /* multiple ASRe's ... handle 'em in a batch, for efficiency
   */

  RCregister_A <<= 4;
  RCregister_A = SAR16(RCregister_A,1);

  RCregister_PC++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opASRe_A1;
  }                               /* end of unit */

  RCregister_A = SAR16(RCregister_A,1);
  RCregister_PC++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opASRe_A1;
  }                               /* end of unit */

  RCregister_A = SAR16(RCregister_A,1);
  RCregister_PC++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opASRe_A1;
  }                               /* end of unit */

  RCregister_A = SAR16(RCregister_A,1);
  RCregister_PC++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opASRe_A1;
  }                               /* end of unit */

  RCregister_A = SAR16(RCregister_A,1);
  RCregister_PC++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opASRe_A1;
  }                               /* end of unit */

  RCregister_A = SAR16(RCregister_A,1);
  RCregister_PC++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opASRe_A1;
  }                               /* end of unit */

  RCregister_A = SAR16(RCregister_A,1);
  RCregister_PC++;

opASRe_A1:
  /* no more multiples left; finish off.
   */
  RCregister_A >>= 4;

opASRe_A2:
  /* shift once with flags */
  RCcmp_new = temp_word;

  SETA0 ( RCregister_A );           /* store bit0 */
  SETFC ( RCregister_A );

  RCcmp_old = RCregister_A;

  RCregister_A <<= 4; /* get sign bit */
  RCregister_A = SAR16(RCregister_A,5);
  RCregister_A &= 0xFFF;

  jumpCineRet_AA;

opASRe_B_AA:
  RCregister_PC++;

  if ( ( rom [ RCregister_PC ] == 0xED ) &&
       ( rom [ RCregister_PC + 1 ] == 0x57 ) )
  {
    goto opASRe_B0; /* another one follows, do multiples */
  }

  RCcmp_new = 0x0DED;
  SETA0 ( RCregister_A );
  SETFC ( RCregister_A );
  RCcmp_old = RCregister_B;

  RCregister_B <<= 4;
  RCregister_B = SAR16(RCregister_B,5);
  RCregister_B &= 0xFFF;

  jumpCineRet_AA;

opASRe_B0:
  RCregister_B <<= 4;  /* get sign bit */

  RCregister_B = SAR16(RCregister_B,1);
  RCregister_PC += 2;
  if ( ! ( ( rom [ RCregister_PC ] == 0xED ) &&
           ( rom [ RCregister_PC + 1 ] == 0x57 ) ) )
  {
    goto opASRe_B1;
  }

  RCregister_B = SAR16(RCregister_B,1);
  RCregister_PC += 2;
  if ( ! ( ( rom [ RCregister_PC ] == 0xED ) &&
           ( rom [ RCregister_PC + 1 ] == 0x57 ) ) )
  {
    goto opASRe_B1;
  }

  RCregister_B = SAR16(RCregister_B,1);
  RCregister_PC += 2;
  if ( ! ( ( rom [ RCregister_PC ] == 0xED ) &&
           ( rom [ RCregister_PC + 1 ] == 0x57 ) ) )
  {
    goto opASRe_B1;
  }

  RCregister_B = SAR16(RCregister_B,1);
  RCregister_PC += 2;
  if ( ! ( ( rom [ RCregister_PC ] == 0xED ) &&
           ( rom [ RCregister_PC + 1 ] == 0x57 ) ) )
  {
    goto opASRe_B1;
  }

  RCregister_B = SAR16(RCregister_B,1);
  RCregister_PC += 2;
  if ( ! ( ( rom [ RCregister_PC ] == 0xED ) &&
           ( rom [ RCregister_PC + 1 ] == 0x57 ) ) )
  {
    goto opASRe_B1;
  }

  RCregister_B = SAR16(RCregister_B,1);
  RCregister_PC += 2;
  if ( ! ( ( rom [ RCregister_PC ] == 0xED ) &&
           ( rom [ RCregister_PC + 1 ] == 0x57 ) ) )
  {
    goto opASRe_B1;
  }

  RCregister_B = SAR16(RCregister_B,1);
  RCregister_PC += 2;

opASRe_B1:
  RCregister_B >>= 4; /* fix register */

opASRe_B2:
  RCcmp_new = 0x0DED;
  SETA0 ( RCregister_A );
  SETFC ( RCregister_A );
  RCcmp_old = RCregister_B;

  RCregister_B <<= 4;
  RCregister_B = SAR16(RCregister_B,5);
  RCregister_B &= 0xFFF;

  jumpCineRet_AA;

opASRf_A_AA:
opASRf_AA_AA:
opASRf_BB_AA:
  UNFINISHED ( "opASRf 1\n" );
  jumpCineRet_AA;

opASRf_B_AA:
  UNFINISHED ( "opASRf 2\n" );
  jumpCineRet_AA;

opASRDe_A_AA:
opASRDe_AA_AA:
opASRDe_BB_AA:
  /* Arithmetic shift right of D (A+B) .. B is high (sign bits).
   * divide by 2, but leave the sign bit the same. (ie: 1010 -> 1001)
   */
  temp_word = 0x0EEE;
  RCregister_PC++;

#ifndef DUALCPU
  if ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) {
    goto opASRDe_A0; /* multiples, do the batch */
  }
#endif

  RCcmp_new = temp_word;          /* save new acc value */
  SETA0 ( RCregister_A & 0xFF );  /* save old accA bit0 */
  RCcmp_old = RCregister_A;         /* save old acc */

  temp_word += RCregister_A;
  SETFC ( temp_word );

  RCregister_A <<= 4;
  RCregister_B <<= 4;

  temp_word_2 = ( RCregister_B >> 4 ) << 15;
  RCregister_B = SAR16(RCregister_B,5);
  RCregister_A = ( RCregister_A >> 1 ) | temp_word_2;
  RCregister_A >>= 4;

  RCregister_B &= 0xFFF;
  jumpCineRet_AA;

opASRDe_A0:
  RCregister_A <<= 4;
  RCregister_B <<= 4;

  temp_word_2 = ( RCregister_B >> 4 ) << 15;
  RCregister_B = SAR16(RCregister_B,5);
  RCregister_A = ( RCregister_A >> 1 ) | temp_word_2;

  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opASRDe_A1; /* no more, do last one */
  }

  temp_word_2 = ( RCregister_B & 0x01 ) << 15;
  RCregister_B = SAR16(RCregister_B,1);
  RCregister_A = ( RCregister_A >> 1 ) | temp_word_2;
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opASRDe_A1; /* no more */
  }

  temp_word_2 = ( RCregister_B & 0x01 ) << 15;
  RCregister_B = SAR16(RCregister_B,1);
  RCregister_A = ( RCregister_A >> 1 ) | temp_word_2;
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opASRDe_A1; /* no more */
  }

  temp_word_2 = ( RCregister_B & 0x01 ) << 15;
  RCregister_B = SAR16(RCregister_B,1);
  RCregister_A = ( RCregister_A >> 1 ) | temp_word_2;
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opASRDe_A1; /* no more */
  }

  temp_word_2 = ( RCregister_B & 0x01 ) << 15;
  RCregister_B = SAR16(RCregister_B,1);
  RCregister_A = ( RCregister_A >> 1 ) | temp_word_2;
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opASRDe_A1; /* no more */
  }

  temp_word_2 = ( RCregister_B & 0x01 ) << 15;
  RCregister_B = SAR16(RCregister_B,1);
  RCregister_A = ( RCregister_A >> 1 ) | temp_word_2;
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opASRDe_A1; /* no more */
  }

  temp_word_2 = ( RCregister_B & 0x01 ) << 15;
  RCregister_B = SAR16(RCregister_B,1);
  RCregister_A = ( RCregister_A >> 1 ) | temp_word_2;

  RCregister_PC++;

opASRDe_A1:  /* do last shift with flags */
  RCregister_A >>= 4;

  RCcmp_new = temp_word;
  SETA0 ( RCregister_A & 0xFF );
  RCcmp_old = RCregister_A;

  temp_word += RCregister_A;
  SETFC ( temp_word );

  RCregister_A <<= 4;
  temp_word_2 = ( RCregister_B & 0x01 ) << 15;
  RCregister_B = SAR16(RCregister_B,1);
  RCregister_A = ( RCregister_A >> 1 ) | temp_word_2;
  RCregister_A >>= 4;

  RCregister_B &= 0xFFF;
  jumpCineRet_AA;

opASRDe_B_AA:
  RCregister_PC++;
  temp_word = 0x0EEE;
  RCcmp_new = temp_word;
  SETA0 ( RCregister_A & 0xFF );
  RCcmp_old = RCregister_B;

  temp_word += RCregister_B;
  SETFC ( temp_word );
  RCregister_B <<= 4;
  RCregister_B = SAR16(RCregister_B,5);
  RCregister_B &= 0xFFF;

  jumpCineRet_AA;

opASRDf_A_AA:
opASRDf_AA_AA:
opASRDf_BB_AA:
  UNFINISHED ( "opASRDf 1\n" );
  //RCregister_PC++;        // GTOAL again testing hw much damage from a missing opcode
  jumpCineRet_AA;

opASRDf_B_AA:
  UNFINISHED ( "opASRDf 2\n" );
  jumpCineRet_AA;

opLSLDe_A_AA:
opLSLDe_AA_AA:
opLSLDe_BB_AA:
  /* LSLDe -- Left shift through both accumulators; lossy in middle. */

  temp_word = 0x0FEF;

  RCregister_PC++;
  if ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) {
    goto opLSLDe_A0; /* multiples.. go to it. */
  }

  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_A;

  temp_word += RCregister_A;
  SETFC ( temp_word );
  RCregister_A <<= 1;                             /* logical shift left */
  RCregister_A &= 0xFFF;

  RCregister_B <<= 1;
  RCregister_B &= 0xFFF;

  jumpCineRet_AA;

opLSLDe_A0:
  RCregister_A <<= 1;
  RCregister_B <<= 1;

  RCregister_PC ++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opLSLDe_A1; /* nope, go do last one */
  }

  RCregister_A <<= 1;
  RCregister_B <<= 1;

  RCregister_PC ++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opLSLDe_A1; /* nope, go do last one */
  }

  RCregister_A <<= 1;
  RCregister_B <<= 1;

  RCregister_PC ++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opLSLDe_A1; /* nope, go do last one */
  }

  RCregister_A <<= 1;
  RCregister_B <<= 1;

  RCregister_PC ++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opLSLDe_A1; /* nope, go do last one */
  }

  RCregister_A <<= 1;
  RCregister_B <<= 1;

  RCregister_PC ++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opLSLDe_A1; /* nope, go do last one */
  }

  RCregister_A <<= 1;
  RCregister_B <<= 1;

  RCregister_PC ++;
  if ( ! ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) ) {
    goto opLSLDe_A1; /* nope, go do last one */
  }

  RCregister_A <<= 1;
  RCregister_B <<= 1;

  RCregister_PC++;

opLSLDe_A1:
  RCregister_A &= 0xFFF;
  RCregister_B &= 0xFFF;

  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_A;

  temp_word += RCregister_A;
  SETFC ( temp_word );
  RCregister_A <<= 1;                             /* logical shift left */
  RCregister_A &= 0xFFF;

  RCregister_B <<= 1;
  RCregister_B &= 0xFFF;

  jumpCineRet_AA;

opLSLDe_B_AA:
  UNFINISHED ( "opLSLD 1\n" );
  jumpCineRet_AA;

opLSLDf_A_AA:
opLSLDf_AA_AA:
opLSLDf_BB_AA:
  /* LSLDf */

  temp_word = 0xFFF;

  RCregister_PC++;
  if ( rom [ RCregister_PC ] == ( temp_word & 0xFF ) ) {
    goto opLSLDf_A0; /* do multiple batches */
  }

  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_A;

  temp_word += RCregister_A;
  SETFC ( temp_word );

  RCregister_A <<= 1;
  RCregister_A &= 0xFFF;

  RCregister_B <<= 1;
  RCregister_B &= 0xFFF;

  jumpCineRet_AA;

opLSLDf_A0:

  RCregister_A <<= 1;  /* unit begin */
  RCregister_B <<= 1;
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opLSLDf_A1;
  }                  /* unit end */

  RCregister_A <<= 1;  /* unit begin */
  RCregister_B <<= 1;
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opLSLDf_A1;
  }                  /* unit end */

  RCregister_A <<= 1;  /* unit begin */
  RCregister_B <<= 1;
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opLSLDf_A1;
  }                  /* unit end */

  RCregister_A <<= 1;  /* unit begin */
  RCregister_B <<= 1;
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opLSLDf_A1;
  }                  /* unit end */

  RCregister_A <<= 1;  /* unit begin */
  RCregister_B <<= 1;
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opLSLDf_A1;
  }                  /* unit end */

  RCregister_A <<= 1;  /* unit begin */
  RCregister_B <<= 1;
  RCregister_PC++;
  if ( rom [ RCregister_PC ] != ( temp_word & 0xFF ) ) {
    goto opLSLDf_A1;
  }                  /* unit end */

  RCregister_A <<= 1;
  RCregister_B <<= 1;

  RCregister_PC++;

opLSLDf_A1:
  RCregister_A &= 0xFFF;
  RCregister_B &= 0xFFF;

  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  SETFC ( RCregister_A );
  RCcmp_old = RCregister_A;

  temp_word += RCregister_A;
  SETFC ( temp_word );

  RCregister_A <<= 1;
  RCregister_A &= 0xFFF;

  RCregister_B <<= 1;
  RCregister_B &= 0xFFF;

  jumpCineRet_AA;

opLSLDf_B_AA: /* not 'the same' as the A->AA version above */

  RCregister_PC++;

  temp_word = 0xFFF;
  RCcmp_new = temp_word;
  SETA0 ( RCregister_A );
  RCcmp_old = RCregister_B;

  temp_word += RCregister_B;
  SETFC ( temp_word );

  RCregister_B <<= 1;
  RCregister_B &= 0xFFF;

  jumpCineRet_AA;

opJMP_A_A:
opJMP_AA_A:
opJMP_BB_A:
  /* simple jump; change PC and continue..
   */

  /* Use 0xF000 so as to keep the current page, since it may well
   * have been changed with JPP.
   */
  RCregister_PC = ( RCregister_PC & 0xF000 ) + RCregister_J;  /* pick up new PC */
  jumpCineRet_A;

opJMP_B_BB:
  RCregister_PC = ( RCregister_PC & 0xF000 ) + RCregister_J;  /* pick up new PC */
  jumpCineRet_BB;

opJEI_A_A:
opJEI_AA_A:
opJEI_BB_A:

  if ( ! ( ioOutputs & 0x80 ) ) {
    goto opja1;
  }

  if ( ( RCFromX - JoyX ) > 0 ) {
    goto opJMP_A_A;
  }

  RCregister_PC ++;   /* increment PC */
  jumpCineRet_A;

opja1:

  if ( ( RCFromX - JoyY ) > 0 ) {
    goto opJMP_A_A;
  }

  RCregister_PC++;
  jumpCineRet_A;

opJEI_B_BB:

  if ( ! ( ioOutputs & 0x80 ) ) {
    goto opjbb1;
  }

  if ( ( RCFromX - JoyX ) > 0 ) {
    goto opJMP_B_BB;
  }

  RCregister_PC ++;   /* increment PC */
  jumpCineRet_BB;

opjbb1:

  if ( ( RCFromX - JoyY ) > 0 ) {
    goto opJMP_B_BB;
  }

  RCregister_PC++;
  jumpCineRet_BB;

opJEI_A_B:
opJEI_AA_B:
opJEI_BB_B:

  if ( ! ( ioOutputs & 0x80 ) ) {
    goto opjb1;
  }

  if ( ( RCFromX - JoyX ) > 0 ) {
    goto opJMP_A_B;
  }

  RCregister_PC ++;   /* increment PC */
  jumpCineRet_B;

opjb1:

  if ( ( RCFromX - JoyY ) > 0 ) {
    goto opJMP_A_B;
  }

  RCregister_PC++;
  jumpCineRet_B;

opJMI_A_A:
  /* previous instruction was not an ACC instruction, nor was the
   * instruction twice back a USB, therefore minus flag test the
   * current A-reg
   */

  /* negative acc? */
  if ( RCregister_A & 0x800 ) {
    goto opJMP_A_A;  /* yes -- do jump */
  }

  RCregister_PC ++;   /* increment PC */
  jumpCineRet_A;

opJMI_AA_A:
  /* previous acc negative? Jump if so... */
  if ( RCcmp_old & 0x800 ) {
    goto opJMP_AA_A;
  }
  RCregister_PC++;
  jumpCineRet_A;

opJMI_BB_A:
  if ( RCregister_B & 0x800 ) {
    goto opJMP_BB_A;
  }
  RCregister_PC++;
  jumpCineRet_A;

opJMI_B_BB:
  if ( RCregister_A & 0x800 ) {
    goto opJMP_B_BB;
  }
  RCregister_PC++;
  jumpCineRet_BB;

opJLT_A_A:
opJLT_AA_A:
opJLT_BB_A:
  /* jump if old acc equals new acc */

  if ( RCcmp_new < RCcmp_old ) {
    goto opJMP_A_A;
  }

  RCregister_PC ++;
  jumpCineRet_A;

opJLT_B_BB:
  if ( RCcmp_new < RCcmp_old ) {
    goto opJMP_B_BB;
  }
  RCregister_PC ++;
  jumpCineRet_BB;

opJEQ_A_A:
opJEQ_AA_A:
opJEQ_BB_A:
  /* jump if equal */

  if ( RCcmp_new == RCcmp_old ) {
    goto opJMP_A_A;
  }
  RCregister_PC++;                            /* bump PC */
  jumpCineRet_A;

opJEQ_B_BB:

  if ( RCcmp_new == RCcmp_old ) {
    goto opJMP_B_BB;
  }
  RCregister_PC++;                            /* bump PC */
  jumpCineRet_BB;

opJA0_A_A:
opJA0_AA_A:
opJA0_BB_A:

  if ( RCacc_a0 & 0x01 ) {
    goto opJMP_A_A;
  }

  RCregister_PC ++;                               /* bump PC */
  jumpCineRet_A;

opJA0_B_BB:
  if ( RCacc_a0 & 0x01 ) {
    goto opJMP_B_BB;
  }

  RCregister_PC ++;                               /* bump PC */
  jumpCineRet_BB;

opJNC_A_A:
opJNC_AA_A:
opJNC_BB_A:

  if ( ! ( GETFC() & 0xF0 ) ) {
    goto opJMP_A_A; /* no carry, so jump */
  }
  RCregister_PC ++;

  jumpCineRet_A;

opJNC_B_BB:
  if ( ! ( GETFC() & 0xF0 ) ) {
    goto opJMP_B_BB; /* no carry, so jump */
  }
  RCregister_PC ++;

  jumpCineRet_BB;

opJDR_A_A:
opJDR_AA_A:
opJDR_BB_A:
  /*
   * ; Calculate number of cycles executed since
   * ; last 'VDR' instruction, add two and use as
   * ; cycle count, never branch
   */
  RCregister_PC ++;
  jumpCineRet_A;

opJDR_B_BB:
  /*
   * ; Calculate number of cycles executed since
   * ; last 'VDR' instruction, add two and use as
   * ; cycle count, never branch
   */
  RCregister_PC ++;
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
  /* ; 00 = Offset 0000h
   * ; 01 = Offset 1000h
   * ; 02 = Offset 2000h
   * ; 03 = Offset 3000h
   * ; 04 = Offset 4000h
   * ; 05 = Offset 5000h
   * ; 06 = Offset 6000h
   * ; 07 = Offset 7000h
   */
  temp_word = ( RCregister_P & 0x07 ) << 12;  /* rom offset */
  RCregister_PC = RCregister_J + temp_word;
  jumpCineRet_B;

opJPP32_B_BB:
  temp_word = ( RCregister_P & 0x07 ) << 12;  /* rom offset */
  RCregister_PC = RCregister_J + temp_word;
  jumpCineRet_BB;

opJPP16_A_B:
opJPP16_AA_B:
opJPP16_BB_B:
  /* ; 00 = Offset 0000h
   * ; 01 = Offset 1000h
   * ; 02 = Offset 2000h
   * ; 03 = Offset 3000h
   */
  temp_word = ( RCregister_P & 0x03 ) << 12;  /* rom offset */
  RCregister_PC = RCregister_J + temp_word;
  jumpCineRet_B;

opJPP16_B_BB:
  temp_word = ( RCregister_P & 0x03 ) << 12;  /* rom offset */
  RCregister_PC = RCregister_J + temp_word;
  jumpCineRet_BB;

opJMP_A_B:
  RCregister_PC = ( RCregister_PC & 0xF000 ) + RCregister_J;  /* pick up new PC */
  jumpCineRet_B;

opJPP8_A_B:
opJPP8_AA_B:
opJPP8_BB_B:
  /* "long jump"; combine P and J to jump to a new far location (that can
   * be more than 12 bits in address). After this jump, further jumps
   * are local to this new page.
   */
  temp_word = ( ( RCregister_P & 0x03 ) - 1 ) << 12;  /* rom offset */
  RCregister_PC = RCregister_J + temp_word;
  jumpCineRet_B;

opJPP8_B_BB:
  temp_word = ( ( RCregister_P & 0x03 ) - 1 ) << 12;  /* rom offset */
  RCregister_PC = RCregister_J + temp_word;
  jumpCineRet_BB;

opJMI_A_B:
  if ( RCregister_A & 0x800 ) {
    goto opJMP_A_B;
  }
  RCregister_PC++;
  jumpCineRet_B;

opJMI_AA_B:
  UNFINISHED ( "opJMI 3\n" );
  jumpCineRet_B;

opJMI_BB_B:
  UNFINISHED ( "opJMI 4\n" );
  jumpCineRet_B;

opJLT_A_B:
opJLT_AA_B:
opJLT_BB_B:
  if ( RCcmp_new < RCcmp_old ) {
    goto opJMP_A_B;
  }
  RCregister_PC ++;
  jumpCineRet_B;

opJEQ_A_B:
opJEQ_AA_B:
opJEQ_BB_B:
  if ( RCcmp_new == RCcmp_old ) {
    goto opJMP_A_B;
  }
  RCregister_PC++;                            /* bump PC */
  jumpCineRet_B;

opJA0_A_B:
opJA0_AA_B:
opJA0_BB_B:

  if ( GETA0() & 0x01 ) {
    goto opJMP_A_B;
  }
  RCregister_PC++;

  jumpCineRet_B;

opJNC_A_B:
opJNC_AA_B:
opJNC_BB_B:

  if ( ! ( GETFC() & 0x0F0 ) ) {
    goto opJMP_A_B; /* if no carry, jump */
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
  RCregister_PC++;                          /* NOP; bump PC only */
  jumpCineRet_B;

opLLT_A_AA:
opLLT_AA_AA:
opLLT_BB_AA:

  RCregister_PC++;
  temp_byte = 0;

opLLTa1:
  temp_word = RCregister_A >> 8;         /* RCregister_A's high bits */
  temp_word &= 0x0A;                   /* only want PA11 and PA9 */

  if ( ! temp_word ) {
    goto opLLTa2;                      /* zero, no mismatch */
  }

  temp_word ^= 0x0A;                   /* flip the bits */

  if ( temp_word ) {
    goto opLLTa4;                      /* if not zero, mismatch found */
  }

opLLTa2:
  temp_word = RCregister_B >> 8;         /* regB's top bits */
  temp_word &= 0x0A;                   /* only want SA11 and SA9 */

  if ( ! temp_word ) {
    goto opLLTa3;                      /* if zero, no mismatch */
  }

  temp_word ^= 0x0A;                   /* flip bits */

  if ( temp_word ) {
    goto opLLTa4;                      /* if not zero, mismatch found */
  }

opLLTa3:
  RCregister_A <<= 1;                    /* shift regA */
  RCregister_B <<= 1;                    /* shift regB */

  temp_byte ++;
  if ( temp_byte ) {
    goto opLLTa1;                      /* try again */
  }
  jumpCineRet_AA;

opLLTa4:
  RCvgShiftLength = temp_byte;
  RCregister_A &= 0xFFF;
  RCregister_B &= 0xFFF;

opLLTaErr:
  jumpCineRet_AA;

opLLT_B_AA:
  UNFINISHED ( "opLLT 1\n" );
  jumpCineRet_AA;

opVIN_A_A:
opVIN_AA_A:
opVIN_BB_A:
  /* set the starting address of a vector */

  RCFromX = RCregister_A & 0xFFF;            /* regA goes to x-coord */
  RCFromY = RCregister_B & 0xFFF;            /* regB goes to y-coord */

  RCregister_PC ++;                             /* bump PC */
  jumpCineRet_A;

opVIN_B_BB:

  RCFromX = RCregister_A & 0xFFF;            /* regA goes to x-coord */
  RCFromY = RCregister_B & 0xFFF;            /* regB goes to y-coord */

  RCregister_PC ++;                             /* bump PC */
  jumpCineRet_BB;

opWAI_A_A:
opWAI_AA_A:
opWAI_BB_A:
  /* wait for a tick on the watchdog
   */
  bNewFrame = 1;
  RCregister_PC ++;
  goto cineExit;

opWAI_B_BB:
  bNewFrame = 1;
  RCregister_PC ++;
  goto cineExit;

opVDR_A_A:
opVDR_AA_A:
opVDR_BB_A:
  /* set ending points and draw the vector, or buffer for a later draw.
   */
  {
    int RCToX = RCregister_A & 0xFFF;
    int RCToY = RCregister_B & 0xFFF;

    /* shl 20, sar 20; this means that if the CCPU reg should be -ve,
     * we should be negative as well.. sign extended.
     */
    if ( RCFromX & 0x800 ) {
      RCFromX |= 0xFFFFF000;
    }
    if ( RCToX & 0x800 ) {
      RCToX |= 0xFFFFF000;
    }
    if ( RCFromY & 0x800 ) {
      RCFromY |= 0xFFFFF000;
    }
    if ( RCToY & 0x800 ) {
      RCToY |= 0xFFFFF000;
    }

    /* figure out the vector
     */

    RCToX -= RCFromX;
    RCToX = SAR16(RCToX,RCvgShiftLength);
    RCToX += RCFromX;

    RCToY -= RCFromY;
    RCToY = SAR16(RCToY,RCvgShiftLength);
    RCToY += RCFromY;

    /* do orientation flipping, etc.
     */
    if ( ! bFlipX ) {
      goto noFlipX1;
    }

    RCToX = sdwGameXSize - RCToX;
    RCFromX = sdwGameXSize - RCFromX;

  noFlipX1:
    if ( ! bFlipY ) {
      goto noFlipY1;
    }

    RCToY = sdwGameYSize - RCToY;
    RCFromY = sdwGameYSize - RCFromY;

  noFlipY1:
    RCFromX += sdwXOffset;
    RCToX += sdwXOffset;

    RCFromY += sdwYOffset;
    RCToY += sdwYOffset;

    /* check real coords */
    if ( ! bSwapXY ) {
      goto noSwapXY2;
    }

    temp_word = RCToY;
    RCToY = RCToX;
    RCToX = temp_word;

    temp_word = RCFromY;
    RCFromY = RCFromX;
    RCFromX = temp_word;

  noSwapXY2:
    /* render the line
     */

    CinemaVectorData ( RCFromX, RCFromY,
                       RCToX, RCToY,
                       RCvgColour + 0x0F );

  }

  RCregister_PC++;
  jumpCineRet_A;

opVDR_B_BB:
  UNFINISHED ( "opVDR B 1\n" );
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
  switch ( ccpu_msize ) {
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
  switch ( ccpu_msize ) {
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
  switch ( ccpu_msize ) {
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
  switch ( ccpu_msize ) {
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


/* JMI series of opcodes
 */

tJMI_A_B:
  if ( ccpu_jmi_dip ) {
    goto opJMI_A_B;
  } else {
    goto opJEI_A_B;
  }

tJMI_A_A:
  if ( ccpu_jmi_dip ) {
    goto opJMI_A_A;
  } else {
    goto opJEI_AA_B;
  }

tJMI_AA_B:
  if ( ccpu_jmi_dip ) {
    goto opJMI_AA_B;
  } else {
    goto opJEI_AA_B;
  }

tJMI_AA_A:
  if ( ccpu_jmi_dip ) {
    goto opJMI_AA_A;
  } else {
    goto opJEI_AA_A;
  }

tJMI_B_BB1:
  if ( ccpu_jmi_dip ) {
    goto opJMI_B_BB;
  } else {
    goto opJEI_B_BB;
  }

tJMI_B_BB2:
  if ( ccpu_jmi_dip ) {
    goto opJMI_B_BB;
  } else {
    goto opJEI_B_BB;
  }

tJMI_BB_B:
  if ( ccpu_jmi_dip ) {
    goto opJMI_BB_B;
  } else {
    goto opJEI_BB_B;
  }

tJMI_BB_A:
  if ( ccpu_jmi_dip ) {
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
  switch ( ccpu_monitor ) {
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
  switch ( ccpu_monitor ) {
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
  switch ( ccpu_monitor ) {
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
  switch ( ccpu_monitor ) {
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

  /* opcode exit point (so we can catch and do debugging, etc.
   */
cineExecBottom:

  /* handle debugger */
  if ( ccpudebug ) {
    disassemble ( disassembly, sizeof ( disassembly ), original_PC );
    fprintf(stderr, "DEBUG: %s\n", disassembly);
  }

  /* the opcode code has set a RCstate and done mischief with flags and
   * the programcounter; now jump back to the top and run through another
   * opcode.
   */

cineExit:
  /* return control to main system
   */
  dwElapsedTicks = 100000; /* some magic number */

  return ( 0x80000000 );
}



void vgSetRotateFlip( int rotate, int flipX, int flipY) {
  DEBUG_OUT ("vgSetRotateFlip( int rotate=%d, int flipX=%d, int flipY=%d);\n", rotate, flipX, flipY);
  v_rotate = rotate; v_flip_x = flipX; v_flip_y = flipY;
}

void cineInit( unsigned char *progdata, unsigned char *optkeymap ) {
#define SETTINGS_SIZE 1024
  static unsigned char settingsBlob[SETTINGS_SIZE];
  DEBUG_OUT( "cineInit( unsigned char *progdata, unsigned char *optkeymap );\n");

  // copy from progdata[] to rom[] NEEDS TO KNOW ROM SIZE
  {int i; for (i = 0; i < 0x8000; i++) rom[i] = progdata[i];}
//fprintf(stderr, "Calling v_loadSettings\n");
  v_loadSettings(ccpu_game, settingsBlob, SETTINGS_SIZE);
//fprintf(stderr, "Returned from v_loadSettings\n");
  DEBUG_OUT ( "v_loadSettings(\n\"%s\"\n);\n", settingsBlob);
}

void vgInit(unsigned char *vectBfr) {
}

void vgSetTwinkle( int aTwinkle) {
  DEBUG_OUT( "vgSetTwinkle( int aTwinkle=%d);\n", aTwinkle);
}

void vgSetMonitor( int aMonitor, int RB, int GB, int BB, int RC, int GC, int BC) {
  DEBUG_OUT( "vgSetMonitor( int aMonitor, int RB, int GB, int BB, int RC, int GC, int BC);\n");
}

void cineSetRate( unsigned int aRefresh ) {
  DEBUG_OUT( "cineSetRate( unsigned int aRefresh=%d );\n", aRefresh);
  v_setRefresh(aRefresh);
}

void cineSetSw( unsigned int aSwitches, unsigned int aInputs) {
  DEBUG_OUT("cineSetSw( unsigned int aSwitches=%0x, unsigned int aInputs=%0x); // TO DO!\n",  aSwitches, aInputs);
  ioSwitches = aSwitches; ioInputs = aInputs;
}


void cineSetMouse(
        unsigned int    aMouseType,
        unsigned int    aMouseSpeedX,
        unsigned int    aMouseSpeedY,
        unsigned int    aKeyMask,
        unsigned char   *aMouseSeg
) {
  DEBUG_OUT( "cineSetMouse( aMouseType=%d, ...);\n", aMouseType);
}

void vgSetMode( int mode) {
  DEBUG_OUT( "vgSetMode( int mode=%d);\n", mode);
}


void vgSetCineSize( int Xmin, int Ymin, int Xmax, int Ymax) {
  DEBUG_OUT("vgSetCineSize( int Xmin=%d, int Ymin=%d, int Xmax=%d, int Ymax=%d);\n", Xmin, Ymin,  Xmax, Ymax);
  window(Xmin, Ymin, Xmax, Ymax);
}

void vsetmode( unsigned int mode ) {
  DEBUG_OUT("vsetmode( unsigned int mode=%d );\n", mode);
}
