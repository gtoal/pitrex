//Drift control not working any more (no buzz),
/*
DEBUG with single step via terminal
with commands

why does that not work for small SMS:

        v_directMove32keepScale(x*128,(y-yy)*128);



"go"
go cycles
go till
"dis xxx-yyy"
show regs
set breakpoint


CLUSTER
A cluster is 1 or more vectors (preferable more), which are drawn
in one go without zeroing.
Additional attributes may be:
- all one color
- all draw (no move)

In battle zone the score - the last "digit" should be moved in front of the "score" cluster


SFX with priority - so no "forced" channel is needed.

NULL Waiter konfigurable

Raster routine (image) with CNTL in 2 cycles

Vector print with clipps...

// isn't that double?
		if (crankyFlag & CRANKY_BETWEEN_VIA_B)
		{
			afterYDelay += crankyFlag&0x0f;
		}

...
		if (crankyFlag & CRANKY_NULLING_WAIT)
		{
			// some crankies need additional waits here!
			afterYDelay += CRANKY_DELAY_Y_TO_NULL_VALUE;
		}



/*
possible hint for drift calibration value / or slot...



calibration draw on 1+2 second cluster "wobbles"


Gyrocks/ripoff and some others not working


black widow and space duel start

Pokey


Pipeline implementation:

a) Base Pipeline
Every time a vector is added to the interface one VectorPipelineBase is configured.

The Base pipeline is an array of MAX_PIPELINE elements held in the array:
  VectorPipelineBase pb[MAX_PIPELINE];

The global variable cpb (currentBasePipeline) always holds the next available item.
When it is used, the next item must be gotten with a sequence of:
(since we allways reuse the same array elements, we must ensure they are clean from start)

    cpb = &pb[++pipelineCounter];
    cpb->force = 0;
    cpb->pattern = 0;
    cpb->debug[0] = 0;

Once per round (or less, if double buffer is active) the contents of the
VectorPipelineBase is processed and ( handlePipeline() ) - and a second stage VectorPipeline
is built.
The goal is that "VectorPipeline" has all information needed to (repeatedly) be drawn as fast as possible.

handlePipeline()
Does following at the moment:
a) processes clipping if needed
b) sorts "stable" vectors to the beginning of the list
c) sorts dots to the end of the list
d) builds optimal vector scales
e) handles (some) hints
f) can handle (starting to) debug information and output these

The VectorPipeline pipeline itself is displayed with:
  void displayPipeline().

 */



// special PITREX command to write aword to VIA -> which is faster than two bytes

// SMS - or I handöle crankies better
// STO
// SSS


int myDebug;

/* What if we use for "timing" of the vectors the
 * smallest possible timer interrupt?
 * How many Nano is that?
 *
 * That way we could use "marked" timing...   to try...
 *
 *
 * */


/*
 T ODO                                       **
 Reset detection -> eleminate T1 -> RAMP, do with timer of Pi!

 problematic but actually *should* work (but doesn't in all cases...
 */




/*  Version 0.3
 D one by Malban for the PiTrex projec*t.    *
 last edit: 08th of Jamuary 2020
 ... not written down yet


 Version 0.2

 changes from 0.1
 - macro "syntax"
 - nano delays added, delay is not manually calibrated anymore
 - functions to access PSG registers
 - optimization for same VIA register states corrected (was using two different scales)
 - T1 timer differentiates whether T1 timer is used as 16 bit (very large scale) or usual 8bit
 (one access to IO port less)
 - read vectrex IO (joystick/buttons) only ONCE per wait recal call Possible
 (that also means you have to call waitrecal - or reset "ioDone" manually to zero before read)
 - probably some minor delay changes

 Possible enhancmence:
 - optimize scaling for very similar scale factors to use the same scale factor!
 (see optimization)
 - allow manually set of "no buzz vectrex" (calibration)
 - allow manually set of "cranky vectrex" (less cycles for Y integrator delay)
 - possibly allow manually settings of "draw lines in one go"
 since it is probably best, if different for each piTrex emulator
 - add float functions (draw move)
 - this would allow for easier transformations (screen scale, translocation, rotation etc...)

 WAIT STATES
 Are implemented using a busy loop "v_delayCycles(int cycles)" (vectrex cycles).
 Values of the delaya are in nano second and should be "666" for a vectrex cycles.
 On bare metal that is true. On raspbian seemingly some other optimization kicks in, which SET_OPTIMAL_SCALE
 increases the speed of even a hardcoded assembler loop.
 In non bare metal the waiter seems to need about 940 "fake" nano seconds for one vectrex cycle.

 Optimization
 If not optimized, every vector and every positioning is drawn/done from zero. So every drawing is done with zeroed coordinates. This is SLOW!
 If OPTIMIZED than:
 - vectors "near" each other and with a similar same scale are drawn/positioned directly after another without zeroing

 As of now, each vector might be drawn using a different scale, since scale is directly calculated from the given
 16bit coordinate, so that the strength of the vector is optimized.
 On the one hand this is "good" if there are large differences in scale.
 If the scale difference is very subtle - than it would be more performat just to use the same scale.
 This kind of optimization is on the TODO list!
 (also possibly better looking - if scales are clustered, perhaps zero when scale changes)

 */

/***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h> // these are fresstanding includes!
#include <stdint.h> // also "available":  <float.h>, <iso646.h>, <limits.h>, <stdarg.h>

#include <pitrex/pitrexio-gpio.h>
#include <pitrex/bcm2835.h>
#include "vectrexInterface.h"
#include "baremetalUtil.h"
#include "osWrapper.h"

#include "vectorFont.i" // includes font definition and string printing routines
#include "rasterFont.i" // includes font definition and string printing routines

// ff used for state saving
// state are saved in two 1024 byte blobs
// one for vectrexInterface, one for the "user"
// the blobs can be cast after loading/saving to anything one wants...
#ifdef FREESTANDING
#include <baremetal/rpi-aux.h>
#include <baremetal/rpi-base.h>
#include <baremetal/rpi-gpio.h>
#else
#include <errno.h>
#include <unistd.h>
#include <string.h>

#ifdef RTSCHED
#include <sched.h>
#endif

/* reverse:  reverse string s in place */
static void reverse(char s[])
 {
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }
// radix ignored
static void itoa(int n, char s[], int radix)
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }

typedef struct {
    volatile uint32_t control_status;
    volatile uint32_t counter_lo;
    volatile uint32_t counter_hi;
    volatile uint32_t compare0;
    volatile uint32_t compare1;
    volatile uint32_t compare2;
    volatile uint32_t compare3;
    } rpi_sys_timer_t;

rpi_sys_timer_t* rpiSystemTimer;

#ifdef AVOID_TICKS
unsigned int scaleTotal = 0;
#define MAP_FAILED	((void *) -1)
volatile uint32_t *bcm2835_int		= (uint32_t *)MAP_FAILED;
#endif

#endif

/***********************************************************************/




int32_t currentCursorX; // 16 bit positioning value X (stored in long), with sizing!
int32_t currentCursorY; // 16 bit positioning value Y (stored in long)
uint16_t currentScale; // currently active scale factor (T1 timer in VIA)
uint16_t lastScale; // last active scale factor (T1 timer in VIA)

uint8_t currentButtonState; // bit values as in vectrex
uint8_t internalButtonState; // bit values as in vectrex

int8_t currentJoy1X; // -127 left, 0 center, +127 right
int8_t currentJoy1Y; // -127 up, 0 center, +127 down
int8_t currentJoy2X; // not used yet
int8_t currentJoy2Y; // not used yet
int internalJoy1X; // -127 left, 0 center, +127 right
int internalJoy1Y; // -127 up, 0 center, +127 down
int internalJoy2X; // not used yet
int internalJoy2Y; // not used yet
int v_dotDwell;

int16_t consecutiveDraws; // how many lines/moves were drawn/made directly after another! (without zeroing)


// these are 16 bit to allow illegal values!
// illegal values = unkown state
int16_t currentPortA; // == portA (also X SH) = current X Strength
int16_t currentYSH; // Y SH = current Y Strength
int16_t currentZSH; // Z SH = Brightness (if bit 7 is set -> brightness is off!)

int inCalibration;

int selectedCalibrationMenu;
int selectionCalibrationMade;
uint8_t ioDone; // vectrex IO should only be done once per "round"

uint32_t timerMark;

int pipelineFilled;

void v_initSound();
void v_initDebug();
void handleUARTInterface();

// I don't think the "medians" really do anything
// but for now I leave them enabled.
#define MEDIAN_MAX 5
int medianX1[MEDIAN_MAX];
int medianY1[MEDIAN_MAX];
int medianX2[MEDIAN_MAX];
int medianY2[MEDIAN_MAX];


/* values that we remember for possible optimization purposes */
unsigned int MAX_USED_STRENGTH;
unsigned int MAX_CONSECUTIVE_DRAWS;
unsigned int DELAY_ZERO_VALUE; // 70 // probably less, this can be adjusted, by max x position, the nearer to the center the less waits
unsigned int DELAY_AFTER_T1_END_VALUE;
uint16_t SCALE_STRENGTH_DIF;
unsigned int Vec_Rfrsh; // 30000 cylces (vectrex) = $7530, little endian = $3075
unsigned int cycleEquivalent;
int optimizationON;
int commonHints;

int clipActive;
int clipminX;
int clipminY;
int clipmaxX;
int clipmaxY;
int clipMode;
int orientation;

int16_t offsetX;
int16_t offsetY;
float sizeX;
float sizeY;
uint8_t calibrationValue; // tut calibration

int currentMarkedMenu = 0;
int menuOffset = 0;  


CrankyFlags crankyFlag; // cranky should be checked during calibration! In "VecFever" terms cranky off = burst modus
int beamOffBetweenConsecutiveDraws;
unsigned int resetToZeroDifMax;

VectorPipeline P0[MAX_PIPELINE];
VectorPipeline P1[MAX_PIPELINE];
VectorPipeline *_P[]={P0,P1};
VectorPipeline *pl;
int pipelineAlt;


char *knownName;
unsigned char *knownBlob;
int knownBlobSize;

int bufferType;

int browseMode;
int currentBrowsline;
int currentDisplayedBrowseLine;
int valueChangeDelay;
int saveresetToZeroDifMax;
/***********************************************************************/
int (*executeDebugger)(int);
int noExecuteDebugger(int x)
{
  printf("Debugger not set!\r\n");
}

int customClipxMin;
int customClipyMin;
int customClipxMax;
int customClipyMax;
int customClippingEnabled;
// custom clippings
// are DIRECTLY
// in the coordinates given
// from caller!
// before repositioning, scaling, orientation ....
void setCustomClipping(int enabled, int x0, int y0, int x1, int y1)
{
  customClippingEnabled = enabled;
  customClipxMin = x0;
  customClipyMin = y0;
  customClipxMax = x1;
  customClipyMax = y1;
}
/* should be called once on startup / reset
 */
GlobalMemSettings *settings;

#ifdef FREESTANDING
#else
GlobalMemSettings _settings;
#endif
void v_init()
{ 
#ifdef FREESTANDING
  GlobalMemSettings **settingsPointer;
  settingsPointer = (GlobalMemSettings **)0x0000008c;
  settings = *settingsPointer; 

  printf("SettingPointer: %08x, settings: %0x08", settingsPointer, settings);
 
#else
 settings = &_settings;

#ifdef AVOID_TICKS
  bcm2835_int = bcm2835_peripherals + BCM2835_INT_BASE/4;
#endif
#ifdef RTSCHED
  struct sched_param sp = { .sched_priority = 99 };
   sched_setscheduler(0, SCHED_FIFO, &sp);
#endif

#endif

 
 
  unsigned char *buffer;
  buffer = malloc(200000);

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
#ifdef FREESTANDING
  initFileSystem();
#endif
  printf("v_init()\r\n");

  executeDebugger = noExecuteDebugger;
  v_initSound();
  #ifdef PITREX_DEBUG
  #ifdef FREESTANDING
  v_initDebug();
  myDebug = 0;
  #endif
  #endif

  // once to ENABLE in general!
  #ifdef FREESTANDING
  PMNC(CYCLE_COUNTER_ENABLE|CYCLE_COUNTER_RESET|COUNTER_ZERO);
  #endif

  MAX_CONSECUTIVE_DRAWS = 65;
  DELAY_ZERO_VALUE = 40; // 70 // probably less, this can be adjusted, by max x position, the nearer to the center the less waits
  DELAY_AFTER_T1_END_VALUE = 20; // last vectors should add 5!!!
  SCALE_STRENGTH_DIF = 2;
  bufferType = 0;
  MAX_USED_STRENGTH = 100;// 110;
  resetToZeroDifMax = 4000;
  customClippingEnabled = 0;
  commonHints = 0;
  pipelineFilled = 0;
  v_dotDwell = 10;
  browseMode=0;
  currentBrowsline=0;
  currentDisplayedBrowseLine=-1;


  for (int i=0;i<MEDIAN_MAX;i++)
  {
    medianX1[i] = 0;
    medianY1[i] = 0;
  }

  crankyFlag=0x14;//CRANKY_NULLING_CALIBRATE | CRANKY_BETWEEN_VIA_B+6; // for is the normal cranky delay after switching port b to MUX_y (or Z)

  cycleEquivalent = 666;
  beamOffBetweenConsecutiveDraws = 1;

  clipActive = 0;
  clipminX=-10000;
  clipminY= 00000;
  clipmaxX= 10000;
  clipmaxY= 15000;
  clipMode = 1; // 0 normal clipping, 1 = inverse clipping

  ioDone = 0;
  orientation = 0;
  timerMark = 0;
  calibrationValue = 0;
  optimizationON = 1;
  usePipeline = 1;
  pipelineCounter = 0;
  pipelineAlt = 0;
  pl = _P[pipelineAlt];
  cpb = &pb[pipelineCounter]; // current base pipeline

  inCalibration = 0;
  currentCursorX = 0; // 16 bit positioning value X
  currentCursorY = 0; // 16 bit positioning value Y
  currentButtonState = 0; // nothing pressed
  currentJoy1X = 0; // centered
  currentJoy1Y = 0; // centered
  currentJoy2X = 0; // centered
  currentJoy2Y = 0; // centered

  currentScale = 0; // currently active scale factor (T1 timer in VIA)
  lastScale = 1; // last active scale factor (T1 timer in VIA)
  consecutiveDraws = 0; // how many lines/moves were made after another!
  sizeX = 1.0;
  sizeY = 1.0;
  offsetX = 0;
  offsetY = 0;

  selectedCalibrationMenu = 0;
  selectionCalibrationMade = 0;


  // these are 16 bit to allow illegal values!
  currentPortA = 0x100; // == portA (also X SH)
  currentYSH = 0x100; // Y SH
  currentZSH = 0x100; // Z SH = brightness

  vectrexwrite (VIA_DDR_b, 0x9F); // All Outputs VIA Port B except COMPARE input and PB6 at Cartridge Port
  vectrexwrite (VIA_DDR_a, 0xFF); // All Outputs VIA Port A
  #ifdef BEAM_LIGHT_BY_SHIFT
  SET (VIA_aux_cntl, 0x98); //Shift Reg. Enabled, T1 PB7 Enabled
  #endif
  #ifdef BEAM_LIGHT_BY_CNTL
  SET (VIA_aux_cntl, 0x80); // Shift reg mode = 000 free disable, T1 PB7 enabled
  #endif

  Vec_Rfrsh = 0x3075;//0x3075;//12405; // 30000 = $7530 little endian = $3075 = 12405
  SETW (VIA_t2, Vec_Rfrsh); //Set T2 timer to 30000 cycles and start timer and reset IFlag

  v_readButtons(); // read buttons to check if we should enter calibration on init

  // calibrate if button 1 is pressed on startup
  if ((currentButtonState&0x08) == (0x08)) inCalibration=1;

  knownName = "";
  knownBlob = (unsigned char*)0;
  knownBlobSize = 0;

printf("1) Settings->flags = %02x\r\n", settings->flags);
  v_loadSettings(knownName, knownBlob, knownBlobSize);
  if ((settings->flags & GLOBAL_FLAG_IS_INIT) == 0)
  {
    printf("INIT Settings\r\n");
    // global settings not set yet
    settings->flags = settings->flags | GLOBAL_FLAG_IS_INIT;
    settings->orientation = orientation; // 0-3
    settings->lastSelection = 2; // start with tailgunner
  }
  else
  {
    printf("Settings were Init before \r\n");
    orientation = settings->orientation;
  }
printf("2) Settings->flags = %02x\r\n", settings->flags);

}

/***********************************************************************/


/*
 S ystem timer runs at 250 Mhz               * *
 -> 1 tick = 1/250000000

 0,000000004s
 0,000004 ms
 0,004 us
 4 ns

 1s
 0,1 = 1/10
 0,01 = 1/100
 0,001 = 1 milli
 0,000001 = 1 micro s
 0,000000001 = 1 nano s


 // 1 cycle = 1/1000000000
 0,000000001s
 0,000001 ms
 0,001 us
 1 ns
 */

// resolution "about" 4 nanoseconds
// but not less than OFFSET_CYCLE_OVERHEAD
void delayNano(uint32_t n)
{
  uint32_t nWait;
  nWait = n-OFFSET_CYCLE_OVERHEAD;
  if (nWait>((uint32_t)-20)) return;
  /*
   i f (n<1024)                             *  *
   {
   nWait = n-OFFSET_CYCLE_OVERHEAD;
   if (nWait<=0) return;
}
else
{
nWait = n-OFFSET_TIMER_OVERHEAD;
if (nWait>1024)
{
WAIT_MICRO_TIME(nWait>>10);
nWait = nWait&0x3ff;
}
}
*/
  WAIT_CYCLE_NANO(nWait);
}

// delay in vectrex cycles
void v_delayCycles(uint32_t n)
{
  uint32_t nWait;
  nWait = (n*DELAY_PI_CYCLE_EQUIVALENT)-(OFFSET_CYCLE_OVERHEAD-3);
  if (nWait>((uint32_t)-20)) return;
  WAIT_CYCLE_NANO(nWait);
}
void v_delayCyclesQuarter()
{
  uint32_t nWait;
  nWait = (DELAY_PI_CYCLE_EQUIVALENT/4)-(OFFSET_CYCLE_OVERHEAD-3);
  if (nWait>((uint32_t)-20)) return;
  WAIT_CYCLE_NANO(nWait);
}

void v_delayCyclesEighth()
{
  uint32_t nWait;
  nWait = (DELAY_PI_CYCLE_EQUIVALENT/8)-(OFFSET_CYCLE_OVERHEAD-3);
  if (nWait>((uint32_t)-20)) return;
  WAIT_CYCLE_NANO(nWait);
}
/***********************************************************************/

/* Set current brightness of the Vectorbeam (provided BLANK is inactive).
 I f bit 7 is set, vectrex does not show an*y* brightness!
 It is checked if brightness is already set to the
 given value, if so - the function returns immediately.

 Ends without a delay, last set: VIA port B!
 */
void v_setBrightness(uint8_t brightness)

{
  if (brightness == currentZSH) return; // beware -> this might cause brightness drift to zero on completely same intensities all the time!
  if (brightness != currentPortA)
  {
    SET(VIA_port_a, brightness);
    currentPortA = (int8_t) brightness;
  }
  SET(VIA_port_b, 0x84); // MUX to intensity
  currentZSH = brightness;
  DELAY_ZSH();
  SET(VIA_port_b, 0x81); // SET port b to no Mux
}

/***********************************************************************/

/* Moves from the current "cursor" position to given offsets.
 O ffsets are given in 16bit and are downsc*a*led by
 "optimal" scale factors to vectrex strength/scale values.
 Wait is done using the Vectrex Timer T1.
 The wait is not "finished" - so other "pi" things can be done in between.
 After these "other pi things" are done "v_directDeltaMoveEnd()" must be
 called to insure T1 timer has expired.
 */
void v_directDeltaMove32start(int32_t _xLen, int32_t _yLen)
{
  int32_t xLen = _xLen*sizeX;
  int32_t yLen = _yLen*sizeY;
  SET_OPTIMAL_SCALE(xLen, yLen);
  SET_YSH16(yLen);
  SET_XSH16(xLen);
  START_T1_TIMER();
  currentCursorX += xLen;
  currentCursorY += yLen;
}
// called from v_directDraw32, where sizing already occured!
void v_directDeltaMove32start_nosizing(int32_t xLen, int32_t yLen)
{
  SET_OPTIMAL_SCALE(xLen, yLen);
  SET_YSH16(yLen);
  SET_XSH16(xLen);
  START_T1_TIMER();
  currentCursorX += xLen;
  currentCursorY += yLen;
}
/*
 M oves to the given position "absolut".     * *
 at the beginning the vectrex beam is reset to zero (center of screen).

 The difference between a sequence of
 v_zeroWait();
 v_directDeltaMove32start(_xLen, _yLen);

 and a call to v_directMove32(...) is, that
 the absolut position call respects (can respect!) the
 chosen offsetX/offsetY.
 Delta move do not (can not) respect the offsets!
 */
void v_directMove32(int32_t xEnd, int32_t yEnd)
{
  v_zeroWait();
  int32_t x = xEnd*sizeX+offsetX;
  int32_t y = yEnd*sizeY+offsetY;

  UNZERO(); // ensure vector beam can be moves
  SET_OPTIMAL_SCALE(x, y);
  SET_YSH16(y);
  SET_XSH16(x);
  START_T1_TIMER();
  consecutiveDraws=0;
  currentCursorX = x;
  currentCursorY = y;
  WAIT_T1_END();
}

/***********************************************************************/

/*
 W aits till T1 interrupt flag of vectrex VI**A is set.
 Instead the Macro: WAIT_T1_END can be used!
 See also:
 v_directDeltaMove32start(int32_t xLen, int32_t yLen)
 */
void v_directDeltaMoveEnd()
{
  WAIT_T1_END();
}

/***********************************************************************/

/*      Draws "directly" (not buffered) to vectrex screen.
 A ll given coordinates are absolut sc*reen c*oordinates.
 Assume coordinates are in the range: -32768 - +32767,
 assuming 0,0 is in the middle of the screen.
 "32" name tag denotes signed long range.

 This function acts "smart", it investigates given data (in regards to
 last used data) and determines:
 - whether a move/zeroing is necessary
 - translates data to "delta" data if appropriate
 - calculates the vectrex scale and strength
 - respects "OPTIMIZED" flag!

 Leaves non zeroed!
*/

int v_debug = 0;

void v_directDraw32HintedDebug(int32_t xStart, int32_t yStart, int32_t xEnd, int32_t yEnd, uint8_t brightness, int forced, char* debugInfo)
{
  cpb->force = forced;

  int i=0;
  if (debugInfo != 0)
  {
    for (;i<239; i++)
    {
      if (!(cpb->debug[i] = *debugInfo++)) break;
    }
  }
  cpb->debug[i] = (char)0;
  v_directDraw32(xStart, yStart, xEnd, yEnd, brightness);
}
void v_directDraw32Hinted(int32_t xStart, int32_t yStart, int32_t xEnd, int32_t yEnd, uint8_t brightness, int forced)
{
  cpb->force = forced;
  cpb->debug[0] = (char)0;
  v_directDraw32(xStart, yStart, xEnd, yEnd, brightness);
}

void cohen_sutherlandCustom(int32_t *x1, int32_t *y1,  int32_t *x2, int32_t *y2, int xmin, int ymin, int xmax, int ymax);

// only pipelined!
void v_directDraw32Patterned(int32_t xStart, int32_t yStart, int32_t xEnd, int32_t yEnd, uint8_t brightness, uint8_t pattern)
{
    cpb->pattern = pattern;
    v_directDraw32(xStart, yStart, xEnd, yEnd, brightness);
}

void v_directDraw32(int32_t xStart, int32_t yStart, int32_t xEnd, int32_t yEnd, uint8_t brightness)
{
  if (brightness==0) return;
  if (customClippingEnabled)
  {
    cohen_sutherlandCustom(&xStart, &yStart,  &xEnd, &yEnd, customClipxMin, customClipyMin, customClipxMax, customClipyMax);
    if (xStart == 1000000) return; // vector completely out of bounds
  }

  xStart = xStart*sizeX+offsetX;
  yStart = yStart*sizeY+offsetY;
  xEnd = xEnd*sizeX+offsetX;
  yEnd = yEnd*sizeY+offsetY;

  if (orientation == 0) ;// normal
  else  if (orientation == 1)
  {
    int32_t xStart_t = xStart;
    int32_t xEnd_t = xEnd;
    xStart = yStart;
    yStart = -xStart_t;
    xEnd = yEnd;
    yEnd = -xEnd_t;
  }
  else  if (orientation == 2)
  {
    xStart = -xStart;
    yStart = -yStart;
    xEnd = -xEnd;
    yEnd = -yEnd;
  }
  else  if (orientation == 3)
  {
    int32_t xStart_t = xStart;
    int32_t xEnd_t = xEnd;
    xStart = -yStart;
    yStart = xStart_t;
    xEnd = -yEnd;
    yEnd = xEnd_t;
  }
  if (usePipeline)
  {
    cpb->y0 = yStart & (~POSTION_MARGIN_AND);
    cpb->x0 = xStart & (~POSTION_MARGIN_AND);
    cpb->y1 = yEnd & (~POSTION_MARGIN_AND);
    cpb->x1 = xEnd & (~POSTION_MARGIN_AND);
    if (myDebug)
    {
      printf("x0,y0,x1,y2: %i,%i,%i,%i\r\n",cpb->x0,  cpb->y0,  cpb->x1,  cpb->y1);
    }
    cpb->intensity = brightness;
    cpb->force |= commonHints;
    cpb->sms = MAX_USED_STRENGTH;
    if (optimizationON==0) cpb->force |= PL_BASE_FORCE_ZERO;
    if (cpb->force & PL_BASE_FORCE_USE_DOT_DWELL) cpb->timingForced = v_dotDwell;
    if (cpb->force & PL_BASE_FORCE_USE_FIX_SIZE) cpb->timingForced = currentScale;

    cpb = &pb[++pipelineCounter];
    cpb->pattern = 0;
    cpb->force = 0;
    cpb->debug[0] = 0;
    return;
  }

  v_setBrightness(brightness);
  UNZERO(); // ensure vector beam can be moves

  // this will be the delta values for drawing
  int32_t xDrawDif;
  int32_t yDrawDif;

  // how far away is the cursor from the position we want to start drawing?
  int32_t xMoveDif = xStart-currentCursorX;
  int32_t yMoveDif = yStart-currentCursorY;

  // if not optimized, we always reposition!
  // test if the position of the last end - and the current start differs by more than our set margin
  if (((ABS(xMoveDif) > POSITION_MARGIN) || (ABS(yMoveDif) > POSITION_MARGIN) ) || (optimizationON==0))
  {
    // determine whether to zero and reposition (rather than do a move from here)
    // three facters
    // - did we already have to many consecutive moves? If so drift will be large - and it is better to zero once in a while
    // - will the next scale be considerably different from the current one? - if so - better start anew
    // - is the new position very far away from the old, if so, it might even be faster to zero and reposition
    //   (possibly redundant to reason two)

    int resetPos = consecutiveDraws > MAX_CONSECUTIVE_DRAWS;
    resetPos += ABS((currentScale-GET_OPTIMAL_SCALE(xMoveDif, yMoveDif)) > 20);
    resetPos += ((ABS(xMoveDif)>7000) || (ABS(yMoveDif)>7000) );

    // if not optimized, we always reposition!
    resetPos += optimizationON==0;
    if (resetPos)
    {
      // reset to zero - and position absolut
      ZERO_AND_WAIT();
      UNZERO();
      //v_resetIntegratorOffsets0();
      // reposition beam

      v_directDeltaMove32start_nosizing(xStart, yStart);
      consecutiveDraws = 0;
      xDrawDif = xEnd-xStart;
      yDrawDif = yEnd-yStart;
      SET_OPTIMAL_SCALE(xDrawDif, yDrawDif);
      WAIT_T1_END_LAST();//v_directDeltaMoveEnd();

      //if (calibrationValue!=0)    v_resetIntegratorOffsets();

    }
    else
    {
      // position relative
      // reposition beam
      v_directDeltaMove32start_nosizing(xMoveDif, yMoveDif);
      consecutiveDraws++;
      xDrawDif = xEnd-xStart;
      yDrawDif = yEnd-yStart;
      SET_OPTIMAL_SCALE(xDrawDif, yDrawDif);
      WAIT_T1_END_LAST();//v_directDeltaMoveEnd();
    }
  }
  else
  {
    // no repositioning -> draw directly again!
    xDrawDif = xEnd-xStart;
    yDrawDif = yEnd-yStart;
    SET_OPTIMAL_SCALE(xDrawDif, yDrawDif);
  }
  SET_YSH16(yDrawDif);
  SET_XSH16(xDrawDif);
  START_T1_TIMER();
  SWITCH_BEAM_ON();
  consecutiveDraws++;
  currentCursorX = xEnd;
  currentCursorY = yEnd;

  WAIT_T1_END();
  SWITCH_BEAM_OFF();
}

/***********************************************************************/

/*      Draws "directly" (not buffered) to vectrex screen.
 D raws from the current position to t*he giv*en delta values.
 Assume delta values are in the range: -32768 - +32767,
 assuming 0,0 is in the middle of the screen.
 "32" name tag denotes signed long range.

 This function acts "smart", it investigates given data (in regards to
 last used data) and determines:
 - calculates the vectrex scale and strength

 Leaves non zeroed!
 */
void v_directDeltaDraw32(int32_t _xLen, int32_t _yLen, uint8_t brightness)
{
  int32_t xLen = _xLen*sizeX;
  int32_t yLen = _yLen*sizeY;


  v_setBrightness(brightness);
  SET_OPTIMAL_SCALE(xLen, yLen);

  SET_YSH16(yLen);
  SET_XSH16(xLen);
  SWITCH_BEAM_ON();
  START_T1_TIMER();
  currentCursorX += xLen;
  currentCursorY += yLen;

  // if we have enough space and a correct pi timer
  // than we could wait with a PI timer in parallel and
  // resume "earlier", and switch the beam off
  // at EXACTLY the moment the timer finishes
  // lookup wait table 0x00 - 0xff (scale) for 100% correct timer values?
  WAIT_T1_END();
  SWITCH_BEAM_OFF();
}

/***********************************************************************/
// real "0", even to 0 when calibration was done!
inline static void v_resetIntegratorOffsets0()
{
  printf("CALIBRATION 0\r\n");
  SET (VIA_port_b, 0x81);
  DELAY_PORT_B_BEFORE_PORT_A();
  SET (VIA_port_a, 0x00);
  DELAY_CYCLES(4);
  SET (VIA_port_b, 0x80);
  DELAY_CYCLES(6);
  // reset integrators
  SET (VIA_port_b, 0x82);    // mux=1, enable mux - integrator offset = 0
  DELAY_CYCLES(6);
  SET (VIA_port_b, 0x81);    // disable mux
  DELAY_CYCLES(4);
  currentPortA=0x100;// non regular value!
}

/* This routine resets the integrator offsets to zero. This might be neccessary once in a while.
 *
 * The "opposite" (apart from natural degradation of the offsets) is Kristof Tuts like calibration.
 * Which actively sets a integrator offset to a very small value to compensate vectrex "drift".
 */
inline static void v_resetIntegratorOffsets()
{
  SET (VIA_port_b, 0x81);
  DELAY_PORT_B_BEFORE_PORT_A();
  if (calibrationValue==0)
  {
    SET (VIA_port_a, 0x00);
    DELAY_CYCLES(4);
    // reset integrators
    SET (VIA_port_b, 0x82);    // mux=1, enable mux - integrator offset = 0
    DELAY_CYCLES(6);
    SET (VIA_port_b, 0x81);    // disable mux
  }
  else
  {
    SET (VIA_port_b, 0x81);
    DELAY_PORT_B_BEFORE_PORT_A();
    SET (VIA_port_a, calibrationValue);
    DELAY_CYCLES(6);
    SET (VIA_port_b, 0x82);
    DELAY_PORT_B_BEFORE_PORT_A();
    SET (VIA_port_a, 0xff);
    DELAY_CYCLES(2);
    SET (VIA_port_b, 0x81);
  }
  DELAY_CYCLES(4);
  currentPortA=0x100;// non regular value!
}

void v_delayCycles1point5()
{
  uint32_t nWait;
  nWait = (DELAY_PI_CYCLE_EQUIVALENT*1.5)-(OFFSET_CYCLE_OVERHEAD-3);
  if (nWait>((uint32_t)-20)) return;
  WAIT_CYCLE_NANO(nWait);
}

 inline static void __v_resetIntegratorOffsets()
{
  SET (VIA_port_b, 0x81);
  DELAY_PORT_B_BEFORE_PORT_A();
  SET (VIA_port_a, 0x00);
  DELAY_CYCLES(4);
  SET (VIA_port_b, 0x80);
  DELAY_CYCLES(6);
  if (calibrationValue==0)
  {
    // reset integrators
    SET (VIA_port_b, 0x82);    // mux=1, enable mux - integrator offset = 0
    DELAY_CYCLES(6);
    SET (VIA_port_b, 0x81);    // disable mux
  }
  else
  {
    SET (VIA_port_b, 0x81);
    DELAY_PORT_B_BEFORE_PORT_A();
    SET (VIA_port_a, calibrationValue);
    DELAY_CYCLES(4);
    SET (VIA_port_b, 0x82);
    DELAY_CYCLES(4);
    SET (VIA_port_a, 0xff);
//    v_delayCycles1point5(); // delay of 1.5 cycles ensures a safety zone, and 0x81 is poked after a FIXED cycle count!
    SET (VIA_port_a, 0xff);
    SET (VIA_port_b, 0x81);
/*
int waited  = waitUntil(0);
if (waited > 4300)
printf("%06i*********\r\n", waited);
  else
printf("%06i\r\n", waited);
*/
  }
  DELAY_CYCLES(4);
  currentPortA=0x100;// non regular value!
}

/* ***********************************************************************/

/* This routine moves the vector beam to the extrem edges of the screen and zeros afterwards
 * this prevents "vector collaps".
 * If the "application" uses vectors which are far from the center anyway... this is not needed.
 */
void v_deflok()
{
  ZERO_AND_WAIT();
  UNZERO();
  v_setScale(255);
  SET_YSH_IMMEDIATE_8(127);
  SET_XSH_IMMEDIATE_8(127);
  START_WAIT_T1();
  ZERO_AND_WAIT();
  UNZERO();
  SET_YSH_IMMEDIATE_8(-127);
  SET_XSH_IMMEDIATE_8(-127);
  START_WAIT_T1();
  ZERO_AND_WAIT();
}

/***********************************************************************/

/*      Set vectrex refresh rate for T2 "WaitRecal".
 P arameter in in Hz.                 *      *
 -> so 50 Hz -> results in  Vec_Rfrsh = 0x3075
 */
void v_setRefresh(int hz)
{
  int32_t cycles = 1500000 / hz; // Hz to vectrex cycles
  Vec_Rfrsh = ((cycles&0xff)*256) + ((cycles>>8)&0xff); // swap hi / lo bytes for little endian VIA
}

/***********************************************************************/

/*      A vectrex BIOS like function that can be called "once per round".
 T his function waits till the Vectrex* Timer* T2 is finished (timing can be set
 using "v_setRefresh(int hz)".

 After the wait finishes, following things are further executed:
 - the Timer T2 will be reset to the set frequency.
 - v_deflok()
 - v_resetIntegratorOffsets()

 Leaves with scale set to 0xff.

 Resets ioDone (so that joystick reads are only done once per round).

 Also checks and reacts on PiTrex "button" sets (return to boot screen, calibrate etc)
 */
void handlePipeline();
void displayPipeline();
void v_WaitRecal_buffered(int buildBuffer);
void v_printBitmapUni(unsigned char *bitmapBlob, int width, int height, int size, int x, int y);


void v_WaitRecal()
{
  #ifdef FREESTANDING
  #ifdef PITREX_DEBUG
  while (browseMode)
  {
    /* display a bitmap) *
      ioDone = 0;
    v_printBitmapUni(0, 20, 20, 20, -70, 70);
  uint16_t timeLeftPrinting = ((Vec_Rfrsh&0xff)*256+(Vec_Rfrsh>>8)) - GET(VIA_t2_cnt_hi)*256;
  int usagePercentAll  =(timeLeftPrinting*100/((Vec_Rfrsh&0xff)*256+(Vec_Rfrsh>>8)));
  if (currentButtonState ==0xc) // button 3 + 4 pressed - print speed info
  {
    printf("Refresh usage: %i%%  \r\n", usagePercentAll );
  }

    */
    
    
    
    displayPipeline();
    handleUARTInterface();
    // wait for Via T2 to expire
    while ((GET (VIA_int_flags) & 0x20) == 0)
    {;}

    // reset T2 VIA timer to 50Hz
    SETW (VIA_t2, Vec_Rfrsh);
    currentZSH = 0x100;
    v_deflok();
    v_resetIntegratorOffsets();
  }
  #endif
  #endif


  if ((bufferType == 2) && (pipelineFilled))
  {
    if (pipelineCounter == 0)
    {
      v_WaitRecal_buffered(0);
      return;
    }
    v_WaitRecal_buffered(1);
    return;
  }
  v_WaitRecal_buffered(1);
  if (bufferType == 1)
  {
    v_WaitRecal_buffered(0);
  }
}
static int roundCounter =0;

void v_WaitRecal_buffered(int buildBuffer)
{
  // savety
  // within WaitRecal - neverchange the pipeline
  #ifdef PITREX_DEBUG
  uint16_t timeLeftCoding = ((Vec_Rfrsh&0xff)*256+(Vec_Rfrsh>>8)) - GET(VIA_t2_cnt_hi)*256;
  #endif
  if (usePipeline)
  {
    v_deflok();
    if (bufferType != 3)
    {
      if (buildBuffer)
      {
        if (inCalibration) v_calibrate();
        handlePipeline();
      }
      roundCounter++;
      v_resetIntegratorOffsets();
      displayPipeline();
    }
  }
  ioDone = 0;
  SWITCH_BEAM_OFF();
  #ifdef PITREX_DEBUG
  uint16_t timeLeftPrinting = ((Vec_Rfrsh&0xff)*256+(Vec_Rfrsh>>8)) - GET(VIA_t2_cnt_hi)*256;
  int usagePercentAll  =(timeLeftPrinting*100/((Vec_Rfrsh&0xff)*256+(Vec_Rfrsh>>8)));

  int usageCoding  =(timeLeftCoding*100/((Vec_Rfrsh&0xff)*256+(Vec_Rfrsh>>8)));
  int usageDrawing  =usagePercentAll-usageCoding;
  if (currentButtonState ==0xc) // button 3 + 4 pressed - print speed info
  {
    printf("Refresh usage: %i%% (Code: %i%% Draw: %i%%) \r\n", usagePercentAll, usageCoding, usageDrawing );
  }
  #ifdef FREESTANDING
  handleUARTInterface();

  #endif
  #endif

  v_resetDetection();
  if (currentButtonState ==0xf) // button 1+ 2 + 3+4 -> go menu
  {
    #ifndef FREESTANDING
    exit(0); // pressing all four buttons exits
    #else
    printf("Restarting kernel...%08x %08x\r\n",(int)settings->loader, (int)*settings->loader);
/*
    uint32_t progSapce = LOADER_START;;
    void (*progStart)(void) = (void (*)(void))progSapce;
    progStart();
      __asm__ __volatile__(
          "mov r5, #0x008c   \n\t"
//          "ldr pc, =0x400002c// 0x008c      \n\t"
          "ldr pc, [r5]      \n\t"
        );
*/    
    settings->loader();
    #endif
  }

  if (currentButtonState ==0x3) // button 1 + 2 pressed - enter calibration menu1
  {
    clipActive = 1;
    inCalibration = 1;
    valueChangeDelay = 0;


    currentMarkedMenu=0;
    selectionCalibrationMade = 0;
    menuOffset=0;
    selectedCalibrationMenu = 0;



    saveresetToZeroDifMax = resetToZeroDifMax;
    resetToZeroDifMax = 2000;
  }

  // wait for Via T2 to expire -> 50Hz
  while ((GET (VIA_int_flags) & 0x20) == 0)
  {
    v_resetDetection();
    ; // might do a usleep(1); here
  }

  if (!usePipeline)
  {
    v_deflok();
    v_resetIntegratorOffsets();
  }

  // reset T2 VIA timer to 50Hz
  SETW (VIA_t2, Vec_Rfrsh);

  currentZSH = 0x100;
  consecutiveDraws = 0;
}

/***********************************************************************/
// detect VIA reset
// TODO
//-> this might work, if we don't use T1, but our own PI timer
// which atm we do...
void v_resetDetection()
{
  if ((GET(VIA_DDR_a) == 0) && (GET(VIA_DDR_b) == 0))
  {
    printf("Reset detected!\r\n");
    // wait till reset is finished
    while ((GET(VIA_DDR_a) == 0) && (GET(VIA_DDR_b) == 0))
    {
      SET(VIA_DDR_a, 0xff);
      DELAY_CYCLES(4);
    }
#ifdef FREESTANDING
/*
    uint32_t progSapce = LOADER_START;;
    void (*progStart)(void) = (void (*)(void))progSapce;
    progStart();
*/    
    settings->loader();
#else
    exit(0);
#endif
  }

}


/***********************************************************************/

/*      Reads the vectrex button states (port 1 and port 2).
 N eeds only be called once per round *(or ev*en only every second round)
 result also in variable: currentButtonState
 bit 0 = button 1 port 1
 bit 1 = button 2 port 1
 bit 2 = button 3 port 1
 bit 3 = button 4 port 1
 bit 4 = button 1 port 2
 bit 5 = button 2 port 2
 bit 6 = button 3 port 2
 bit 7 = button 4 port 2
 if bit == 1, than button is pressed
   if bit == 0, than button is not pressed
     */
uint8_t v_directReadButtons()
{
 int ibs=0;
  SET(VIA_port_a, 0x0e); // prepare access of psg port A (0x0e) by writing the register value to VIA port A
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x99); // set VIA port B to settings: sound BDIR on, BC1 on, mux off
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)
  DELAY_CYCLES(4);

  SET(VIA_DDR_a, 0x00); // set VIA DDR A to input
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x89); // set VIA port B to settings: sound BDIR on, BC1 on, mux off
  DELAY_CYCLES(6);
  ibs = ~GET(VIA_port_a); // Read buttons
  SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)
  DELAY_CYCLES(4);
  SET(VIA_DDR_a, 0xff); // set VIA DDR A to output
  DELAY_CYCLES(4);
  currentPortA = 0x100;

  return ibs;
}
uint8_t v_readButtons()
{
  if (ioDone & V_BUTTONS_READ) return currentButtonState;
  ioDone |= V_BUTTONS_READ;
  // read of buttons goes thru the PSG sound chip, PSG port A
  //
  SET(VIA_port_a, 0x0e); // prepare access of psg port A (0x0e) by writing the register value to VIA port A
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x99); // set VIA port B to settings: sound BDIR on, BC1 on, mux off
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)
  DELAY_CYCLES(4);

  SET(VIA_DDR_a, 0x00); // set VIA DDR A to input
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x89); // set VIA port B to settings: sound BDIR on, BC1 on, mux off
  DELAY_CYCLES(6);
  currentButtonState = ~GET(VIA_port_a); // Read buttons
  SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)
  DELAY_CYCLES(4);
  SET(VIA_DDR_a, 0xff); // set VIA DDR A to output
  DELAY_CYCLES(4);
  currentPortA = 0x100;

  internalButtonState = currentButtonState;
  if (inCalibration)
  {
    currentButtonState = 0;
  }


  return currentButtonState;
}

/***********************************************************************/

/*      Reads the vectrex joystick states (port 1) DIGITAL.
 ( Faster than analog call!)          *      *
 Needs only be called once per round (or even only every second round)
 result in variable:
 currentJoy1Y = -1 down
 currentJoy1Y = 0 centered
 currentJoy1Y = 1 up

 currentJoy1X = -1 left
 currentJoy1X = 0 centered
 currentJoy1X = 1 right
 */

void v_readJoystick1Digital()
{
  if (ioDone & V_JOY_DIGITAL_READ) return;
  ioDone |= V_JOY_DIGITAL_READ;

  // Y ANALOG
  SET(VIA_port_a, 0x00); // clear VIA port A
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x82); // set VIA port B mux enabled, mux sel = 01 (vertical pot port 0)

  // wait for joystick comparators to "settle"
  DELAY_CYCLES(60); // must be tested! can probably be less?

  currentJoy1Y = -1; // default down
  SET(VIA_port_b, 0x83); // set VIA port B mux
  DELAY_PORT_B_BEFORE_PORT_A();
  SET(VIA_port_a, 0x40); // load a with test value (positive y), test value to DAC
  DELAY_CYCLES(4);
  if ((GET(VIA_port_b) & 0x20) == 0x20)
  {
    currentJoy1Y = 1; //up
  }
  else
  {
    SET(VIA_port_a, -0x40); // load a with test value (negative y), test value to DAC
    DELAY_CYCLES(4);
    if ((GET(VIA_port_b) & 0x20) == 0x20)
    {
      currentJoy1Y = 0; // no direction
    }
  }

  // X ANALOG
  SET(VIA_port_a, 0x00); // clear VIA port A
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x80); // set VIA port B mux enabled, mux sel = 00 (horizontal pot port 0)
  // wait for joystick comparators to "settle"
  DELAY_CYCLES(60); // must be tested! can probably be less?

  currentJoy1X = -1; // default left
  SET(VIA_port_b, 0x83); // set VIA port B mux
  DELAY_PORT_B_BEFORE_PORT_A();
  SET(VIA_port_a, 0x40); // load a with test value (positive y), test value to DAC
  DELAY_CYCLES(2);
  if ((GET(VIA_port_b) & 0x20) == 0x20)
  {
    currentJoy1X = 1; //right
  }
  else
  {
    DELAY_CYCLES(4);
    SET(VIA_port_a, -0x40); // load a with test value (negative y), test value to DAC
    DELAY_CYCLES(4);
    if ((GET(VIA_port_b) & 0x20) == 0x20)
    {
      currentJoy1X = 0; // no direction
    }
    DELAY_CYCLES(4);
  }
  if (inCalibration)
  {
    currentJoy1X =0;
    currentJoy1Y =0;
  }



  // todo
  // if joyport 2  needed....




  // Y ANALOG
  SET(VIA_port_a, 0x00); // clear VIA port A
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x82+0x04); // set VIA port B mux enabled, mux sel = 03 (vertical pot port 1)

  // wait for joystick comparators to "settle"
  DELAY_CYCLES(60); // must be tested! can probably be less?

  currentJoy2Y = -1; // default down
  SET(VIA_port_b, 0x83); // set VIA port B mux
  DELAY_PORT_B_BEFORE_PORT_A();
  SET(VIA_port_a, 0x40); // load a with test value (positive y), test value to DAC
  DELAY_CYCLES(4);
  if ((GET(VIA_port_b) & 0x20) == 0x20)
  {
    currentJoy2Y = 1; //up
  }
  else
  {
    SET(VIA_port_a, -0x40); // load a with test value (negative y), test value to DAC
    DELAY_CYCLES(4);
    if ((GET(VIA_port_b) & 0x20) == 0x20)
    {
      currentJoy2Y = 0; // no direction
    }
  }

  // X ANALOG
  SET(VIA_port_a, 0x00); // clear VIA port A
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x84); // set VIA port B mux enabled, mux sel = 2 (horizontal pot port 1)
  // wait for joystick comparators to "settle"
  DELAY_CYCLES(60); // must be tested! can probably be less?

  currentJoy2X = -1; // default left
  SET(VIA_port_b, 0x83); // set VIA port B mux
  DELAY_PORT_B_BEFORE_PORT_A();
  SET(VIA_port_a, 0x40); // load a with test value (positive y), test value to DAC
  DELAY_CYCLES(2);
  if ((GET(VIA_port_b) & 0x20) == 0x20)
  {
    currentJoy2X = 1; //right
  }
  else
  {
    DELAY_CYCLES(4);
    SET(VIA_port_a, -0x40); // load a with test value (negative y), test value to DAC
    DELAY_CYCLES(4);
    if ((GET(VIA_port_b) & 0x20) == 0x20)
    {
      currentJoy2X = 0; // no direction
    }
    DELAY_CYCLES(4);
  }

  // set port A reference value to unkown
  currentYSH = currentPortA=0x100; // reset saved current values to unkown state
  v_resetIntegratorOffsets();
}

/***********************************************************************/

/*      Reads the vectrex joystick states (port 1) ANALOG.
 ( Slower than digital call!)         *      *
 Needs only be called once per round (or even only every second round)
 result in variable:
 currentJoy1Y = -1 - -127 down
 currentJoy1Y = 0 centered
 currentJoy1Y = +1 - +127 up

 currentJoy1X = -1 - -127  left
 currentJoy1X = 0 centered
 currentJoy1X = +1 - +127 right
 */



void v_readJoystick1Analog()
{
  if (ioDone & V_JOY_ANALOG_READ) return;
  ioDone |= V_JOY_ANALOG_READ;



  SET(VIA_port_a, 0x00); // clear VIA port A
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x82); // set VIA port B mux enabled, mux sel = 01 (vertical pot port 0)
  // wait for joystick comparators to "settle"
  DELAY_CYCLES(60); // must be tested! can probably be less?

  currentJoy1Y = 0; // default centered
  SET(VIA_port_b, 0x83); // set VIA port B mux disabled
  DELAY_PORT_B_BEFORE_PORT_A();
  uint8_t compareBit = 0x80;
  currentJoy1Y = 0;

  do
  {
    SET(VIA_port_a, currentJoy1Y); // load a with test value (positive y), test value to DAC
    DELAY_CYCLES(4);
    if ((GET(VIA_port_b) & 0x20) == 0)
    {
      currentJoy1Y = currentJoy1Y ^ compareBit;
    }
    DELAY_CYCLES(4);
    compareBit = compareBit>>1;
    currentJoy1Y = currentJoy1Y | compareBit;
  } while (compareBit!=0);

  SET(VIA_port_a, 0x00); // clear VIA port A
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x80); // set VIA port B mux enabled, mux sel = 01 (horizontal pot port 0)
  // wait for joystick comparators to "settle"
  DELAY_CYCLES(60); // must be tested! can probably be less?

  currentJoy1X = 0; // default centered
  SET(VIA_port_b, 0x81); // set VIA port B mux disabled
  DELAY_PORT_B_BEFORE_PORT_A();
  compareBit = 0x80;
  currentJoy1X = 0;

  do
  {
    SET(VIA_port_a, currentJoy1X); // load a with test value (positive y), test value to DAC
    DELAY_CYCLES(4);
    if ((GET(VIA_port_b) & 0x20) == 0)
    {
      currentJoy1X = currentJoy1X ^ compareBit;
    }
    DELAY_CYCLES(4);
    compareBit = compareBit>>1;
    currentJoy1X = currentJoy1X | compareBit;
  } while (compareBit!=0);

  int difx = currentJoy1X - internalJoy1X;
  int dify = currentJoy1Y - internalJoy1Y;

  currentJoy1X = internalJoy1X+(difx>>1);
  currentJoy1Y = internalJoy1Y+(dify>>1);
  internalJoy1X = currentJoy1X;
  internalJoy1Y = currentJoy1Y;
/*
  internalJoy1X = 0;
  internalJoy1Y = 0;
  for (int i=MEDIAN_MAX-1;i>0;i--)
  {
    medianX1[i] = medianX1[i-1];
    internalJoy1X+= medianX1[i];
    medianY1[i] = medianY1[i-1];
    internalJoy1Y+= medianY1[i];
  }

  internalJoy1X+=medianX1[0] = currentJoy1X;
  internalJoy1Y+=medianY1[0] = currentJoy1Y;
  internalJoy1X/=MEDIAN_MAX;
  internalJoy1Y/=MEDIAN_MAX;
  */

  if (inCalibration)
  {
    currentJoy1X =0;
    currentJoy1Y =0;
  }
  else
  {
    currentJoy1X = internalJoy1X;
    currentJoy1Y = internalJoy1Y;
  }








    // todo
  // if joyport 2  needed....

  SET(VIA_port_a, 0x00); // clear VIA port A
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x82+0x04); // set VIA port B mux enabled, mux sel = 03 (vertical pot port 1)
  // wait for joystick comparators to "settle"
  DELAY_CYCLES(60); // must be tested! can probably be less?

  currentJoy2Y = 0; // default centered
  SET(VIA_port_b, 0x83); // set VIA port B mux disabled
  DELAY_PORT_B_BEFORE_PORT_A();
  compareBit = 0x80;
  currentJoy2Y = 0;

  do
  {
    SET(VIA_port_a, currentJoy2Y); // load a with test value (positive y), test value to DAC
    DELAY_CYCLES(4);
    if ((GET(VIA_port_b) & 0x20) == 0)
    {
      currentJoy2Y = currentJoy2Y ^ compareBit;
    }
    DELAY_CYCLES(4);
    compareBit = compareBit>>1;
    currentJoy2Y = currentJoy2Y | compareBit;
  } while (compareBit!=0);

  SET(VIA_port_a, 0x00); // clear VIA port A
  DELAY_CYCLES(4);
  SET(VIA_port_b, 0x84); // set VIA port B mux enabled, mux sel = 2 (horizontal pot port 1)
  // wait for joystick comparators to "settle"
  DELAY_CYCLES(60); // must be tested! can probably be less?

  currentJoy2X = 0; // default centered
  SET(VIA_port_b, 0x81); // set VIA port B mux disabled
  DELAY_PORT_B_BEFORE_PORT_A();
  compareBit = 0x80;
  currentJoy2X = 0;

  do
  {
    SET(VIA_port_a, currentJoy2X); // load a with test value (positive y), test value to DAC
    DELAY_CYCLES(4);
    if ((GET(VIA_port_b) & 0x20) == 0)
    {
      currentJoy2X = currentJoy2X ^ compareBit;
    }
    DELAY_CYCLES(4);
    compareBit = compareBit>>1;
    currentJoy2X = currentJoy2X | compareBit;
  } while (compareBit!=0);






  // set port A reference value to unkown
  currentYSH = currentPortA=0x100; // reset saved current values to unkown state
  v_resetIntegratorOffsets();
}

/***********************************************************************/

/*      Sets the Vectrex Zero state - and waits for zeroing to have full effect.
 Z eroing is NOT disabled upon end of *functi*on!
 */
void v_zeroWait()
{
  ZERO_AND_WAIT();
}

/***********************************************************************/

/*      Sets the current scale value absolut to given value.
 D oes NOT change VIA!                    *  *
 */
void v_setScale(uint16_t s)
{
  lastScale = currentScale;
  currentScale = s;
}

/***********************************************************************/

/*      Moves from current location to
 d elta given by parameters.          *      *
 - does not set any internal variables
 - does not change scale
 - waits till position is reached

 Caller must ensure zeroing and similar states are taken care of beforehand!
 */
void v_moveToImmediate8(int8_t xLen, int8_t yLen)
{
  SET_YSH_IMMEDIATE_8(yLen);
  SET_XSH_IMMEDIATE_8(xLen);
  START_T1_TIMER();
  WAIT_T1_END();
}

/***********************************************************************/

/*      Draws from current location to
 d elta given by parameters.          *      *
 - does not set any internal variables
 - does not change scale
 - waits till position is reached

 Caller must ensure zeroing and similar states are taken care of!
 */
void v_drawToImmediate8(int8_t xLen, int8_t yLen)
{
  SET_YSH_IMMEDIATE_8(yLen);
  SET_XSH_IMMEDIATE_8(xLen);
  SWITCH_BEAM_ON();
  START_T1_TIMER();
  WAIT_T1_END();
  SWITCH_BEAM_OFF();
}

/***********************************************************************/
/* All things calibration follows                                      */
/***********************************************************************/
void v_driftCalibration()
{
  int doingSomething = 0;
  if ((internalButtonState&0x01) == (0x01)) calibrationValue=0;
  if ((internalButtonState&0x04) == (0x04)) selectedCalibrationMenu=0;
  if (valueChangeDelay>0)valueChangeDelay--;
  if ((internalJoy1X==0) && (internalJoy1Y==0)) valueChangeDelay =0;
  if (valueChangeDelay<=0)
  {
    if (internalJoy1X>80)
    {
      calibrationValue++;
      valueChangeDelay = 64-(internalJoy1X>>1);
      doingSomething=1;
    }
    if (internalJoy1X<-80)
    {
      calibrationValue--;
      valueChangeDelay = 64+(internalJoy1X>>1);
      doingSomething=1;
    }
  }
/*
  v_printString(-50, 90, "ALIGN THIS STRING ONLY!", 1,0x50);
  v_printString(-50, 80, "ALIGN THIS STRING ONLY!", 3,0x50);
*/
  v_printStringNP(-50, 70, "ALIGN THIS STRING ONLY!", 5,0x50);
  v_printStringNP(-50, 50, "LEFT <> RIGHT!", 5,0x50);
  v_printStringNP(-50, 30, "1 RESET!", 5,0x50);

  #ifdef PITREX_DEBUG
  if (doingSomething)
    printf("drift: %i (%i)\r\n", calibrationValue, internalJoy1X);
  #endif
}

/***********************************************************************/
// left is smaller X
// down is smaller Y
void v_sizeCalibration()
{
  int doingSomething = 0;
  if ((internalButtonState&0x04) == (0x04)) selectedCalibrationMenu=0;
  if (valueChangeDelay>0)valueChangeDelay--;
  if ((internalJoy1X==0) && (internalJoy1Y==0)) valueChangeDelay =0;
  if (valueChangeDelay<=0)
  {
    if (internalJoy1X>80)
    {
      doingSomething=1;
      sizeX+= 0.01;
      valueChangeDelay = 64-(internalJoy1X>>1);
    }
    if (internalJoy1X<-80)
    {
      doingSomething=1;
      sizeX-= 0.01;
      valueChangeDelay = 64+(internalJoy1X>>1);
    }
    if (internalJoy1Y>80)
    {
      doingSomething=1;
      sizeY+= 0.01;
      valueChangeDelay = 64-(internalJoy1Y>>1);
    }
    if (internalJoy1Y<-80)
    {
      doingSomething=1;
      sizeY-= 0.01;
      valueChangeDelay = 64+(internalJoy1Y>>1);
    }
  }
  v_printStringNP(-50, 20, "JOYSTICK", 5,0x50);
  #ifdef PITREX_DEBUG
  if (doingSomething)
    printf("stretch x, y: %f, %f\r\n", sizeX, sizeY);
  #endif

}

/***********************************************************************/

void v_positionCalibration()
{
  int doingSomething = 0;
  if ((internalButtonState&0x04) == (0x04)) selectedCalibrationMenu=0;
  if (valueChangeDelay>0)valueChangeDelay--;
  if ((internalJoy1X==0) && (internalJoy1Y==0)) valueChangeDelay =0;
  if (valueChangeDelay<=0)
  {
    if (internalJoy1X>80)
    {
      doingSomething=1;
      offsetX+= 100;
      valueChangeDelay = 64-(internalJoy1X>>1);
    }
    if (internalJoy1X<-80)
    {
      doingSomething=1;
      offsetX-= 100;
      valueChangeDelay = 64+(internalJoy1X>>1);
    }
    if (internalJoy1Y>80)
    {
      doingSomething=1;
      offsetY+= 100;
      valueChangeDelay = 64-(internalJoy1Y>>1);
    }
    if (internalJoy1Y<-80)
    {
      doingSomething=1;
      offsetY-= 100;
      valueChangeDelay = 64+(internalJoy1Y>>1);
    }
  }
  v_printStringNP(-50, 20, "JOYSTICK", 5,0x50);

  #ifdef PITREX_DEBUG
  if (doingSomething)
    printf("offset x, y: %i, %i\r\n", offsetX, offsetY);
  #endif
}
void v_SSSCalibration()
{
  int doingSomething = 0;
  char buf[12];

  if ((internalButtonState&0x04) == (0x04)) selectedCalibrationMenu=0;
  if (valueChangeDelay>0)valueChangeDelay--;
  if ((internalJoy1X==0) && (internalJoy1Y==0)) valueChangeDelay =0;
  if (valueChangeDelay<=0)
  {
    if (internalJoy1X>80)
    {
      doingSomething=1;
      SCALE_STRENGTH_DIF+= 1;
      valueChangeDelay = 64-(internalJoy1X>>1);
      if (SCALE_STRENGTH_DIF>10) SCALE_STRENGTH_DIF = 10;
    }
    if (internalJoy1X<-80)
    {
      doingSomething=1;
      SCALE_STRENGTH_DIF-=1;
      valueChangeDelay = 64+(internalJoy1X>>1);
      if (SCALE_STRENGTH_DIF<0) SCALE_STRENGTH_DIF = 0;
    }
  }
  itoa(SCALE_STRENGTH_DIF, buf, 10);
  v_printStringNP(-20, 80, "SSS", 5,0x50);
  v_printStringNP(-50, 50, "JOYSTICK", 5,0x50);
  v_printStringNP(-10, 30, buf, 5,0x50);
  #ifdef PITREX_DEBUG
  if (doingSomething)
    printf("SSS: %i\r\n", SCALE_STRENGTH_DIF);
  #endif
}
void v_STOCalibration()
{
  char buf[12];
  int doingSomething = 0;

  if ((internalButtonState&0x04) == (0x04)) selectedCalibrationMenu=0;
  if (valueChangeDelay>0)valueChangeDelay--;
  if ((internalJoy1X==0) && (internalJoy1Y==0)) valueChangeDelay =0;
  if (valueChangeDelay<=0)
  {
    if (internalJoy1X>80)
    {
      doingSomething=1;
      DELAY_AFTER_T1_END_VALUE+= 1;
      valueChangeDelay = 64-(internalJoy1X>>1);
      if (DELAY_AFTER_T1_END_VALUE>255) DELAY_AFTER_T1_END_VALUE = 255;
    }
    if (internalJoy1X<-80)
    {
      doingSomething=1;
      DELAY_AFTER_T1_END_VALUE-=1;
      valueChangeDelay = 64+(internalJoy1X>>1);
      if (DELAY_AFTER_T1_END_VALUE<1) DELAY_AFTER_T1_END_VALUE = 1;
    }
  }
  itoa(DELAY_AFTER_T1_END_VALUE, buf, 10);
  v_printStringNP(-20, 80, "STO", 5,0x50);
  v_printStringNP(-50, 50, "JOYSTICK", 5,0x50);
  v_printStringNP(-10, 30, buf, 5,0x50);
  #ifdef PITREX_DEBUG
  if (doingSomething)
    printf("STO: %i\r\n", DELAY_AFTER_T1_END_VALUE);
  #endif
}

void v_SMSCalibration()
{
  char buf[12];
  int doingSomething = 0;

  if ((internalButtonState&0x04) == (0x04)) selectedCalibrationMenu=0;
  if (valueChangeDelay>0)valueChangeDelay--;
  if ((internalJoy1X==0) && (internalJoy1Y==0)) valueChangeDelay =0;
  if (valueChangeDelay<=0)
  {
    if (internalJoy1X>80)
    {
      doingSomething=1;
      MAX_USED_STRENGTH+= 1;
      valueChangeDelay = 64-(internalJoy1X>>1);
      if (MAX_USED_STRENGTH>127) MAX_USED_STRENGTH = 127;
    }
    if (internalJoy1X<-80)
    {
      doingSomething=1;
      MAX_USED_STRENGTH-=1;
      valueChangeDelay = 64+(internalJoy1X>>1);
      if (MAX_USED_STRENGTH<10) MAX_USED_STRENGTH = 10;
    }
  }
  itoa(MAX_USED_STRENGTH, buf, 10);
  v_printStringNP(-20, 80, "SMS", 5,0x50);
  v_printStringNP(-50, 50, "JOYSTICK", 5,0x50);
  v_printStringNP(-10, 30, buf, 5,0x50);
  #ifdef PITREX_DEBUG
  if (doingSomething)
    printf("SMS: %i\r\n", MAX_USED_STRENGTH);
  #endif
}
void v_OrientationCalibration()
{
  char buf[12];
  int doingSomething = 0;

  if ((internalButtonState&0x04) == (0x04)) selectedCalibrationMenu=0;
  if (valueChangeDelay>0)valueChangeDelay--;
  if ((internalJoy1X==0) && (internalJoy1Y==0)) valueChangeDelay =0;
  if (valueChangeDelay<=0)
  {
    if (internalJoy1X>80)
    {
      orientation+= 1;
      valueChangeDelay = 64;
      if (orientation>3) orientation = 0;
    }
    if (internalJoy1X<-80)
    {
      orientation-= 1;
      valueChangeDelay = 64;
      if (orientation<0) orientation = 3;
    }
  }
  v_printStringNP(-35, 80, "ORIENTATION", 5,0x50);
  v_printStringNP(-50, 50, "JOYSTICK", 5,0x50);
  settings->orientation = orientation;
}

/***********************************************************************/

typedef struct MenuItem {
  char *name;
  void (*calibrateFunction)(void);
} MenuItem;
MenuItem menuItems[] = {
  {"DRIFT   ", v_driftCalibration},
  {"SIZE    ", v_sizeCalibration},
  {"POSITION", v_positionCalibration},
  {"SMS     ", v_SMSCalibration},
  {"STO     ", v_STOCalibration},
  {"SSS     ", v_SSSCalibration},
  {"BIG O   ", v_OrientationCalibration},

  {"DONE    ", 0},
};

void v_calibrationMenu()
{
  char *selected = ">";
  if (!usePipeline) v_setBrightness(80);
  for (int i=0; i<7; i++)
  {
    v_printStringNP(-40, 80-i*10, menuItems[menuOffset+i].name, 4,0x50);
    if (menuOffset+i == currentMarkedMenu)
      v_printStringNP(-60, 80-i*10, selected, 5,0x50);
  }
  
  if ((internalJoy1Y<-50) && (selectionCalibrationMade==0))
  {
    if (menuItems[currentMarkedMenu].calibrateFunction != 0)
    {
      currentMarkedMenu++;
      selectionCalibrationMade = 1;
      if (currentMarkedMenu-menuOffset>6) menuOffset++;
      
    }
  }
  if ((internalJoy1Y>50) && (selectionCalibrationMade==0))
  {
    currentMarkedMenu--;
    if (currentMarkedMenu<0) currentMarkedMenu=0;

    if (menuOffset>currentMarkedMenu) menuOffset--;
    selectionCalibrationMade = 1;
  }
  if (ABS(internalJoy1Y)<45) selectionCalibrationMade =0;

  if ((internalButtonState&0x08) == (0x08))
  {
    selectedCalibrationMenu = currentMarkedMenu+1;
    if (menuItems[currentMarkedMenu].calibrateFunction == 0)
    {
      inCalibration=0;
      clipActive = 0;
      resetToZeroDifMax = saveresetToZeroDifMax;
      v_saveSettings(knownName, knownBlob, knownBlobSize);
    }
  }
}

/***********************************************************************/

/*      Calibration "switcher", called from "WaitRecal"
 */

// todo possibly display some funny stuff
void v_calibrate()
{
  commonHints = PL_BASE_FORCE_NOT_CLIPPED | PL_BASE_FORCE_STABLE;
  char *message = "CALIBRATION";
  v_printStringNP(-57, 95, message, 10,0x50);
  v_readButtons();
  v_readJoystick1Analog();

  if (selectedCalibrationMenu == 0)
  {
    v_calibrationMenu();
  }
  else 
  {
      menuItems[selectedCalibrationMenu-1].calibrateFunction();
  }

  commonHints = 0;
}

/***********************************************************************/

#ifdef FREESTANDING
#include <baremetal/rpi-systimer.h>
static rpi_sys_timer_t* rpiSystemTimer = (rpi_sys_timer_t*)RPI_SYSTIMER_BASE;
#endif

uint32_t markStartLo = 0;
uint32_t markStartHi = 0;
uint32_t markRollOverExpected = 0;



/* sets a "mark" for the system timer
 * returns just after a timer "tick" occured
 */
    static int errdone = 0;
void setMarkStart(void)
{
  #ifndef FREESTANDING
  if (errdone) return;
  if (bcm2835_st==((void *) -1))
  {
    if (!errdone)
    {
      errdone = 1;
      printf ("Read system time failed - try running with SUDO \r\n");
    }
    return;
  }
  rpi_sys_timer_t* rpiSystemTimer = (rpi_sys_timer_t*)bcm2835_st;
  #endif

  volatile uint32_t ts = rpiSystemTimer->counter_lo;
  // ensure the timer "just" switched
  // the timer has only a resolution of 1 micro second
  // one instruction takes 1 nano second - soooo actually
  // the resolution is only exact for 1000 pi instructions!!!
  markStartHi = rpiSystemTimer->counter_hi;
  while (ts == rpiSystemTimer->counter_lo);

  if (markStartHi != rpiSystemTimer->counter_hi)
    markStartHi = rpiSystemTimer->counter_hi;
  markStartLo = rpiSystemTimer->counter_lo;
  return ;
}

/* waits till # of microseconds passed since mark was set */
void waitMarkEnd(uint32_t offsetMicro)
{
  #ifndef FREESTANDING
  if (errdone) return;
  if (bcm2835_st==((void *) -1))
  {
    if (!errdone)
    {
      errdone = 1;
      printf ("Read system time failed - try running with SUDO \r\n");
    }
    return;
  }
  rpi_sys_timer_t* rpiSystemTimer = (rpi_sys_timer_t*)bcm2835_st;
  #endif
  volatile uint32_t ts = markStartLo + offsetMicro;
  //  printf("mMarkEnd: %i, %i, %i\r\n", ts, rpiSystemTimer->counter_lo, offsetMicro);
  if (ts < markStartLo)
  {
    // wait till rollover
    while (markStartHi == rpiSystemTimer->counter_hi)
      ;
  }

  // return when the timer has "just" switched
  while (ts > rpiSystemTimer->counter_lo) ;
  return;
}
/* waits till the next "full" micro second tick is done
 then returns the full tick counter since *la*st mark */
int waitFullMicro()
{
  #ifndef FREESTANDING
  if (errdone) return 0;
  if (bcm2835_st==((void *) -1))
  {
    if (!errdone)
    {
      errdone = 1;
      printf ("Read system time failed - try running with SUDO \r\n");
    }
    return 0;
  }
  rpi_sys_timer_t* rpiSystemTimer = (rpi_sys_timer_t*)bcm2835_st;
  #endif

  int ticks =  rpiSystemTimer->counter_lo-markStartLo+1;

  volatile uint32_t ts = rpiSystemTimer->counter_lo;

  // return when the timer has "just" switched
  while (ts == rpiSystemTimer->counter_lo) ;
  return ticks;
}

/* waits till # of microseconds passed since mark was set */
/* and sets new mark */
void waitMarkEndMark(uint32_t offsetMicro)
{
  #ifndef FREESTANDING
  if (errdone) return;
  if (bcm2835_st==((void *) -1))
  {
    if (!errdone)
    {
      errdone = 1;
      printf ("Read system time failed - try running with SUDO \r\n");
    }
    return;
  }
  rpi_sys_timer_t* rpiSystemTimer = (rpi_sys_timer_t*)bcm2835_st;
  #endif
  volatile uint32_t ts = markStartLo + offsetMicro;
  if (ts < markStartLo)
  {
    // wait till rollover
    while (markStartHi == rpiSystemTimer->counter_hi) ;
  }

  // return when the timer has "just" switched
  while (ts > rpiSystemTimer->counter_lo) ;
  markStartHi = rpiSystemTimer->counter_hi;
  markStartLo = rpiSystemTimer->counter_lo;
  return;
}

// obsolete!
// -> the same as "waitMarkEnd" now
//
/* Waits till # of vectrex cycles have passed
 * uses Systimer for "full" microseconds
 * and Nano Wait for partials.
 *
 * One Vectrex cycle = 0,666 useconds
 *
 * ...passed since mark was set */
void waitCycleMarkEnd(uint32_t cycles)
{
  //   markStartLo = 0;
  //  if(markStartLo==0) return;
  /*
   u int32_t micros = cycles * 0.840;         **
   uint32_t nanos = cycles * 840 - micros*1000;
   waitMarkEnd(micros);
   WAIT_CYCLE_NANO(nanos);
   */
  waitMarkEnd((cycles*cycleEquivalent)/1000);
  return;
}

void measureTime()
{
  SET(VIA_t1_cnt_lo, 100);
  DELAY_CYCLES(4);
  setMarkStart();
  SET(VIA_t1_cnt_hi, 0);
  while ((GET (VIA_int_flags) & 0x40) == 0);

  #ifndef FREESTANDING
  if (errdone) return;
  if (bcm2835_st==((void *) -1))
  {
    if (!errdone)
    {
      errdone = 1;
      printf ("Read system time failed - try running with SUDO \r\n");
    }
    return;
  }
//  rpi_sys_timer_t* rpiSystemTimer = (rpi_sys_timer_t*)bcm2835_st;
  #endif
/*
  uint32_t lmarkStartHi = rpiSystemTimer->counter_hi;
  if (lmarkStartHi != rpiSystemTimer->counter_hi)
    lmarkStartHi = rpiSystemTimer->counter_hi;
  uint32_t lmarkStartLo = rpiSystemTimer->counter_lo;
  //  printf("LoDif: %i\r\n", lmarkStartLo-markStartLo);
  */
}

// only using low counters!
uint32_t v_millis()
{
  #ifndef FREESTANDING
  if (errdone) return 0;
  if (bcm2835_st==((void *) -1))
  {
    if (!errdone)
    {
      errdone = 1;
      printf ("Read system time failed - try running with SUDO \r\n");
    }
    return 0;
  }
  rpi_sys_timer_t* rpiSystemTimer = (rpi_sys_timer_t*)bcm2835_st;
  #endif
  uint32_t micros= rpiSystemTimer->counter_lo;
  return micros/1000;
}

// only using low counters!
uint32_t v_micros()
{
  #ifndef FREESTANDING
  if (errdone) return 0;
  if (bcm2835_st==((void *) -1))
  {
    if (!errdone)
    {
      errdone = 1;
      printf ("Read system time failed - try running with SUDO \r\n");
    }
    return 0;
  }
  rpi_sys_timer_t* rpiSystemTimer = (rpi_sys_timer_t*)bcm2835_st;
  #endif
  return rpiSystemTimer->counter_lo;
}

// blocking error message!
void v_error(char *message)
{
  printf("Vectrex/PiTrex bailing out - error:\r\n\t%s\r\n" , message);
  while (1)
  {
    v_WaitRecal();
    v_setBrightness(64);        /* set intensity of vector beam... */
    v_printStringRaster(-30, 80, "ERROR", 40, -7, 0);
    v_printStringRaster(-100, 0, message, 40, -7, 0);
    v_readButtons();
  }
}
void v_errori(char *message, int i)
{
  printf("Vectrex/PiTrex bailing out - error:\r\n\t%s (%i)\r\n" , message, i);
  char buf[16];
  itoa(i,buf,10);
  while (1)
  {
    v_WaitRecal();
    v_setBrightness(64);        /* set intensity of vector beam... */
    v_printStringRaster(-30, 80, "ERROR", 40, -7, 0);
    v_printStringRaster(-100, 0, message, 40, -7, 0);
    v_printStringRaster(-100, -20, buf, 40, -7, 0);
    v_readButtons();
  }
}


////////////////////////////////////////////////////////////////////////////
// PSG and samples -> all stuff sound
////////////////////////////////////////////////////////////////////////////
/*
 A ccessing PSG is "expensive" (to set* one r*egister 6/8 (write/read) VIA access are necessary!)!

 To "directly" access PSG registers use functions:
 void            v_writePSG(uint8_t reg, uint8_t data)
 uint8_t         v_readPSG(uint8_t reg)

 However since it is so "expansive" there is a buffer thru which all PSG acccess should be done:
 Therefor we use a buffer system, if all reads writes are done using the buffer functions
 void v_writePSG_buffered(uint8_t reg, uint8_t data)
 uint8_t v_readPSG_buffered(uint8_t reg)

 we create two significant improvements:
 a) all reads are done via the buffer (saves 8 VIA accesses)
 b) only data is written to the PSG, that is not the same as in the buffer already (and thus in the PSG)

 Even more however:
 Also a double buffer is implemented!

 This can be used if you
 e.g.
 - play a YM file
 - and want to "overlay" distinct channels with a sound effect

 The functions are:
 void v_writePSG_double_buffered(uint8_t reg, uint8_t data)
 uint8_t v_readPSG_double_buffered(uint8_t reg)
 void v_PSG_writeDoubleBuffer()

 The read/write functions are more or less the same as the "buffered" ones, but the data is not
 written to VIA when necessary but written to another (double) buffer.
 So you can "overwrite" PSG data in the order you intend, and not actually put the data to the PSG directly.

 When all PSG stuff is done in your "round" - than a call to "v_PSG_writeDoubleBuffer()" must be made, which
 transports the complete contents of double buffer to the PSG (with respect to already buffered data - meaning
 if the PSG already contains a value, that also is not overwritten!)

   -------------------

   In conclusion:
   - use the double buffer functions!
   - once per round write the double buffer!
   */

unsigned int psgDoubleBuffer[16];
unsigned int psgShadow[16];
uint16_t ymLength;
uint16_t ymPos;
uint8_t *ymBuffer;
int ymloop;
unsigned int sfx_status_1;
unsigned int sfx_status_2;
unsigned int sfx_status_3;
uint8_t *sfx_pointer_1;
uint8_t *sfx_pointer_2;
uint8_t *sfx_pointer_3;

uint8_t *sfx_pointer_1_org;
uint8_t *sfx_pointer_2_org;
uint8_t *sfx_pointer_3_org;
int sfx_priority1;
int sfx_priority2;
int sfx_priority3;

typedef struct {
	unsigned int *status;
	int *priority;
} SFX;

SFX ayfx[3] =
{
	{&sfx_status_1, &sfx_priority1},
	{&sfx_status_2, &sfx_priority2},
	{&sfx_status_3, &sfx_priority3}
};

// used to cut of thrust in asteroids
int v_addSFXForced(unsigned char *buffer, int channel, int loop)
{
	v_playSFXCont(buffer, channel, loop);
	*ayfx[channel].priority=0;
	return channel;
}

// allows the same effect to be played multiple times on// different channels
// return -1 on failure
// channel on success
int v_addSFX(unsigned char *buffer, int priority, int loop, int whenEqualPriorityPlayNew)
{
	int use = -1;
	int possibleUse1 = -1;
	int possibleUse2 = -1;
	for (int i=0;i<3;i++)
	{
		if (*ayfx[i].status == NOT_PLAYING)
		{
			use = i;
			break;
		}
		if (*ayfx[i].priority < priority)
		{
			possibleUse1 = i;
		}
		if (whenEqualPriorityPlayNew)
		{
			if (*ayfx[i].priority == priority)
			{
				possibleUse2 = i;
			}
		}
	}
	int reallyUse = -1;
	if (use != -1) reallyUse = use;
	else if (possibleUse1 != -1) reallyUse = possibleUse1;
	else if (possibleUse2 != -1) reallyUse = possibleUse2;
	if (reallyUse == -1) return -1;

	v_playSFXCont(buffer, reallyUse, loop);
	return reallyUse;
}

void v_noSound();

void v_writePSG_double_buffered(uint8_t reg, uint8_t data);
void v_writePSG_buffered(uint8_t reg, uint8_t data);
void v_writePSG(uint8_t reg, uint8_t data);
uint8_t v_readPSG_double_buffered(uint8_t reg);
uint8_t v_readPSG_buffered(uint8_t reg);
uint8_t v_readPSG(uint8_t reg);
void v_PSG_writeDoubleBuffer();

void v_initYM(uint8_t *b, uint16_t length, int l);
int v_playYM();
int play_sfx1();
int play_sfx2();
int play_sfx3();

void v_playDirectSampleAll(char *ymBufferLoad, int fsize, int rate);



// set PSG sound output to "none"
void v_noSound()
{
  v_writePSG_buffered(8,0); // volume 0
  v_writePSG_buffered(9,0);
  v_writePSG_buffered(10,0);
  v_writePSG_buffered(10,0x3f); // all channel off
}
void v_doSound()
{
  v_PSG_writeDoubleBuffer();
}

void v_initSound()
{
  for (int i=0;i<16;i++)
  {
    psgShadow[i] = 0;
    psgDoubleBuffer[i] = 0;
  }
  sfx_status_1 = NOT_PLAYING;
  sfx_status_2 = NOT_PLAYING;
  sfx_status_3 = NOT_PLAYING;
  sfx_pointer_1 = (uint8_t *) 0;
  sfx_pointer_2 = (uint8_t *) 0;
  sfx_pointer_3 = (uint8_t *) 0;
  sfx_pointer_1_org = (uint8_t *) 0;
  sfx_pointer_2_org = (uint8_t *) 0;
  sfx_pointer_3_org = (uint8_t *) 0;
  sfx_priority1 =0;
  sfx_priority2 =0;
  sfx_priority3 =0;
  ymLength = 0;
  ymBuffer = 0;
  ymloop = 1;
  v_noSound();
}

/***********************************************************************/
uint8_t v_readPSG_double_buffered(uint8_t reg)
{
  return psgDoubleBuffer[reg];
}

void v_writePSG_double_buffered(uint8_t reg, uint8_t data)
{
  psgDoubleBuffer[reg] = data;
}

void v_PSG_writeDoubleBuffer()
{
  for (int i=0;i<15;i++)
  {
    uint8_t data = psgDoubleBuffer[i];
    if (psgShadow[i] == data) continue;
    psgShadow[i] = data;

    SET(VIA_port_a, i); // prepare access of psg port A (0x0e) by writing the register value to VIA port A
    SET(VIA_port_b, 0x99); // set VIA port B to settings: sound BDIR on, BC1 on, mux off
    SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)

    SET(VIA_port_a, data); // write data to port a of via -> and than to psg
    SET(VIA_port_b, 0x91); // set VIA port B to settings: sound BDIR on, BC1 on, mux off, write to PSG
    SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)
  }
  currentPortA = 0x100; // undefined
}

void v_writePSG_buffered(uint8_t reg, uint8_t data)
{
  if (psgShadow[reg] == data) return;
  psgShadow[reg] = data;

  SET(VIA_port_a, reg); // prepare access of psg port A (0x0e) by writing the register value to VIA port A
  SET(VIA_port_b, 0x99); // set VIA port B to settings: sound BDIR on, BC1 on, mux off
  SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)

  SET(VIA_port_a, data); // write data to port a of via -> and than to psg
  SET(VIA_port_b, 0x91); // set VIA port B to settings: sound BDIR on, BC1 on, mux off, write to PSG
  SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)
  currentPortA = 0x100; // undefined
}
uint8_t v_readPSG_buffered(uint8_t reg)
{
  return psgShadow[reg];
}

void v_writePSG(uint8_t reg, uint8_t data)
{
  SET(VIA_port_a, reg); // prepare access of psg port A (0x0e) by writing the register value to VIA port A
  SET(VIA_port_b, 0x99); // set VIA port B to settings: sound BDIR on, BC1 on, mux off
  SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)

  SET(VIA_port_a, data); // write data to port a of via -> and than to psg
  SET(VIA_port_b, 0x91); // set VIA port B to settings: sound BDIR on, BC1 on, mux off, write to PSG
  SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)
  currentPortA = 0x100; // undefined
}

/***********************************************************************/

uint8_t v_readPSG(uint8_t reg)
{
  SET(VIA_port_a, reg); // prepare access of psg port A (0x0e) by writing the register value to VIA port A
  SET(VIA_port_b, 0x99); // set VIA port B to settings: sound BDIR on, BC1 on, mux off
  SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)

  SET(VIA_DDR_a, 0x00); // set VIA DDR A to input
  SET(VIA_port_b, 0x89); // set VIA port B to settings: sound BDIR on, BC1 on, mux off, read from psg
  uint8_t directData = GET(VIA_port_a); // Read buttons
  SET(VIA_port_b, 0x81); // set VIA Port B = 81, mux disabled, RAMP disabled, BC1/BDIR = 00 (PSG inactive)
  SET(VIA_DDR_a, 0xff); // set VIA DDR A to output
  currentPortA = 0x100; // undefined
  return directData;
}

void v_initYM(uint8_t *b, uint16_t length, int l)
{
  ymBuffer = b;
  ymloop = l;
  ymLength = length;
  ymPos = 0;
}

int v_playYM()
{
  if (ymPos>=ymLength)
  {
    if (ymloop==1)
      ymPos = 0;
    else
      return 0;
  }
  uint8_t *currentPointer = ymBuffer+ymPos*14;
  ymPos++;
  for (int i=0; i<14; i++)
  {
    v_writePSG_double_buffered(i, *currentPointer);
    currentPointer++;
  }
  return 1;
}

// does play a complete sample on vectrex, does not return in between... nothing is displayed!
// sample rate is e.g. 15000
// one sample each 0,000066 seconds
// = 0,0666 milli seconds
// = each 66 micro seconds
void v_playDirectSampleAll(char *ymBufferLoad, int fsize, int rate)
{
  int counter = 0;
  int microsToWait = (1000*1000) / rate;

  SET(VIA_port_a, 0);
  DELAY_CYCLES(4);

  SET(VIA_port_b, 0x86);
  DELAY_CYCLES(4);
  while (counter < fsize)
  {
    char b = *ymBufferLoad++;
    SET(VIA_port_a, b);
    int endTime = v_micros()+microsToWait;
    counter++;
    while (v_micros()<endTime) ;
  }
  SET(VIA_port_b, 0x81);
  DELAY_CYCLES(4);
}

void v_playAllSFX()
{
  play_sfx1();
  play_sfx2();
  play_sfx3();
}

// plays a SFX
// if the sfx is already playing... does nothing
void v_playSFXCont(unsigned char *buffer, int channel, int loop)
{
  if (channel == 0)
  {
    if (sfx_pointer_1_org == buffer) return;
    sfx_pointer_1_org = buffer;
    sfx_pointer_1 = buffer;
    if (loop) sfx_status_1 = PLAY_LOOP; else  sfx_status_1 = PLAY_END;
  }
  if (channel == 1)
  {
    if (sfx_pointer_2_org == buffer) return;
    sfx_pointer_2_org = buffer;
    sfx_pointer_2 = buffer;
    if (loop) sfx_status_2 = PLAY_LOOP; else  sfx_status_2 = PLAY_END;
  }
  if (channel == 2)
  {
    if (sfx_pointer_3_org == buffer) return;
    sfx_pointer_3_org = buffer;
    sfx_pointer_3 = buffer;
    if (loop) sfx_status_3 = PLAY_LOOP; else  sfx_status_3 = PLAY_END;
  }

}
// plays a SFX
// if the sfx is already playing... restart it
void v_playSFXStart(unsigned char *buffer, int channel, int loop)
{
  if (channel == 0)
  {
    sfx_pointer_1_org = buffer;
    sfx_pointer_1 = buffer;
    if (loop) sfx_status_1 = PLAY_LOOP; else  sfx_status_1 = PLAY_END;
  }
  if (channel == 1)
  {
    sfx_pointer_2_org = buffer;
    sfx_pointer_2 = buffer;
    if (loop) sfx_status_2 = PLAY_LOOP; else  sfx_status_2 = PLAY_END;
  }
  if (channel == 2)
  {
    sfx_pointer_3_org = buffer;
    sfx_pointer_3 = buffer;
    if (loop) sfx_status_3 = PLAY_LOOP; else  sfx_status_3 = PLAY_END;
  }
}

void v_playSFXStop(unsigned char *buffer, int channel)
{
  if (channel == 0)
  {
    if (sfx_pointer_1_org != buffer) return;
    sfx_pointer_1_org = 0;
    sfx_status_1 = NOT_PLAYING;
    v_writePSG_double_buffered(0+8, 0);

    uint8_t enable = v_readPSG_double_buffered(7);
    // disable tone
    enable = enable | (1<<0); // channel 0
    // disable noise
    enable = enable | (1<<(3+0)); // channel 0
    v_writePSG_double_buffered(7, enable);
  }
  if (channel == 1)
  {
    if (sfx_pointer_2_org != buffer) return;
    sfx_pointer_2_org = 0;
    sfx_status_2 = NOT_PLAYING;
    v_writePSG_double_buffered(1+8, 0);
    uint8_t enable = v_readPSG_double_buffered(7);
    // disable tone
    enable = enable | (1<<1); // channel 0
    // disable noise
    enable = enable | (1<<(3+1)); // channel 0
    v_writePSG_double_buffered(7, enable);
  }
  if (channel == 2)
  {
    if (sfx_pointer_3_org != buffer) return;
    sfx_pointer_3_org = 0;
    sfx_status_3 = NOT_PLAYING;
    v_writePSG_double_buffered(2+8, 0);
    uint8_t enable = v_readPSG_double_buffered(7);
    // disable tone
    enable = enable | (1<<2); // channel 0
    // disable noise
    enable = enable | (1<<(3+2)); // channel 0
    v_writePSG_double_buffered(7, enable);
  }
}

#undef PSG_CHANNEL
#undef SFX_STATUS
#undef SFX_POINTER
#undef SFX_POINTER_ORG
#define PSG_CHANNEL 0
#define SFX_STATUS sfx_status_1
#define SFX_POINTER sfx_pointer_1
#define SFX_POINTER_ORG sfx_pointer_1_org
// return 0 on finish or nothing
int play_sfx1()
{
  if (!SFX_STATUS) return 0;

  uint8_t b = *SFX_POINTER++;

  if (b == 0xd0)
  {
    if ((*SFX_POINTER) == 0x20)
    {
      if (SFX_STATUS == PLAY_LOOP)
      {
        SFX_POINTER = SFX_POINTER_ORG;
        b = *SFX_POINTER++;
      }
      else
      {
        SFX_POINTER_ORG = 0;
        SFX_STATUS = 0;
        return 0;
      }
    }
  }
  if ((b & (1 << 5)) == (1 << 5))
  {
    // tone frequency
    v_writePSG_double_buffered(PSG_CHANNEL*2+0, *SFX_POINTER++);
    v_writePSG_double_buffered(PSG_CHANNEL*2+1, *SFX_POINTER++);
  }
  if ((b & (1 << 6)) == (1 << 6))
  {
    // noise frequency
    v_writePSG_double_buffered(6, *SFX_POINTER++);
  }
  uint8_t volume = b&0xf;
  v_writePSG_double_buffered(PSG_CHANNEL+8, volume);
  uint8_t enable = v_readPSG_double_buffered(7);
  if ((b & (1 << 4)) == (1 << 4))
  {
    // disable tone
    enable = enable | (1<<PSG_CHANNEL); // channel 0
  }
  else
  {
    // enable tone
    enable = enable & (0xff - (1<<PSG_CHANNEL)); // channel 0
  }

  if ((b & (1 << 7)) == (1 << 7))
  {
    // disable noise
    enable = enable | (1<<(3+PSG_CHANNEL)); // channel 0
  }
  else
  {
    // enable tone
    enable = enable & (0xff - (1<<(3+PSG_CHANNEL))); // channel 0
  }
  v_writePSG_double_buffered(7, enable);
  return 1;
}


#undef PSG_CHANNEL
#undef SFX_STATUS
#undef SFX_POINTER
#undef SFX_POINTER_ORG
#define PSG_CHANNEL 1
#define SFX_STATUS sfx_status_2
#define SFX_POINTER sfx_pointer_2
#define SFX_POINTER_ORG sfx_pointer_2_org
// return 0 on finish or nothing
int play_sfx2()
{
  if (!SFX_STATUS) return 0;

  uint8_t b = *SFX_POINTER++;

  if (b == 0xd0)
  {
    if ((*SFX_POINTER) == 0x20)
    {
      if (SFX_STATUS == PLAY_LOOP)
      {
        SFX_POINTER = SFX_POINTER_ORG;
        b = *SFX_POINTER++;
      }
      else
      {
        SFX_POINTER_ORG = 0;
        SFX_STATUS = 0;
        return 0;
      }
    }
  }
  if ((b & (1 << 5)) == (1 << 5))
  {
    // tone frequency
    v_writePSG_double_buffered(PSG_CHANNEL*2+0, *SFX_POINTER++);
    v_writePSG_double_buffered(PSG_CHANNEL*2+1, *SFX_POINTER++);
  }
  if ((b & (1 << 6)) == (1 << 6))
  {
    // noise frequency
    v_writePSG_double_buffered(6, *SFX_POINTER++);
  }
  uint8_t volume = b&0xf;
  v_writePSG_double_buffered(PSG_CHANNEL+8, volume);
  uint8_t enable = v_readPSG_double_buffered(7);
  if ((b & (1 << 4)) == (1 << 4))
  {
    // disable tone
    enable = enable | (1<<PSG_CHANNEL); // channel 0
  }
  else
  {
    // enable tone
    enable = enable & (0xff - (1<<PSG_CHANNEL)); // channel 0
  }

  if ((b & (1 << 7)) == (1 << 7))
  {
    // disable noise
    enable = enable | (1<<(3+PSG_CHANNEL)); // channel 0
  }
  else
  {
    // enable tone
    enable = enable & (0xff - (1<<(3+PSG_CHANNEL))); // channel 0
  }
  v_writePSG_double_buffered(7, enable);
  return 1;
}



#undef PSG_CHANNEL
#undef SFX_STATUS
#undef SFX_POINTER
#undef SFX_POINTER_ORG
#define PSG_CHANNEL 2
#define SFX_STATUS sfx_status_3
#define SFX_POINTER sfx_pointer_3
#define SFX_POINTER_ORG sfx_pointer_3_org
// return 0 on finish or nothing
int play_sfx3()
{
  if (!SFX_STATUS) return 0;

  uint8_t b = *SFX_POINTER++;

  if (b == 0xd0)
  {
    if ((*SFX_POINTER) == 0x20)
    {
      if (SFX_STATUS == PLAY_LOOP)
      {
        SFX_POINTER = SFX_POINTER_ORG;
        b = *SFX_POINTER++;
      }
      else
      {
        SFX_POINTER_ORG = 0;
        SFX_STATUS = 0;
        return 0;
      }
    }
  }
  if ((b & (1 << 5)) == (1 << 5))
  {
    // tone frequency
    v_writePSG_double_buffered(PSG_CHANNEL*2+0, *SFX_POINTER++);
    v_writePSG_double_buffered(PSG_CHANNEL*2+1, *SFX_POINTER++);
  }
  if ((b & (1 << 6)) == (1 << 6))
  {
    // noise frequency
    v_writePSG_double_buffered(6, *SFX_POINTER++);
  }
  uint8_t volume = b&0xf;
  v_writePSG_double_buffered(PSG_CHANNEL+8, volume);
  uint8_t enable = v_readPSG_double_buffered(7);
  if ((b & (1 << 4)) == (1 << 4))
  {
    // disable tone
    enable = enable | (1<<PSG_CHANNEL); // channel 0
  }
  else
  {
    // enable tone
    enable = enable & (0xff - (1<<PSG_CHANNEL)); // channel 0
  }

  if ((b & (1 << 7)) == (1 << 7))
  {
    // disable noise
    enable = enable | (1<<(3+PSG_CHANNEL)); // channel 0
  }
  else
  {
    // enable tone
    enable = enable & (0xff - (1<<(3+PSG_CHANNEL))); // channel 0
  }
  v_writePSG_double_buffered(7, enable);
  return 1;
}
#undef PSG_CHANNEL
#undef SFX_STATUS
#undef SFX_POINTER
#undef SFX_POINTER_ORG


/*
 s etOptimalScale()                          * *
 startDraw
 startMove
 continueTo
 doZero
 */


void handleVectorRequest(int type, int x, int y)
{
}
#ifdef FREESTANDING
#ifdef PITREX_DEBUG
#include "commands.i"
#endif
#endif

#define V_SETTINGS_SIZE 1024
unsigned char v_settingsBlob[V_SETTINGS_SIZE];

typedef struct {
  int8_t crankyFlag; // boolean: cranky should be checked during calibration! In "VecFever" terms cranky off = burst modus
  unsigned int Vec_Rfrsh; // 30000 cylces (vectrex) = $7530, little endian = $3075
  int optimizationON;
  uint8_t calibrationValue; // tut calibration
  float sizeX;
  float sizeY;
  int16_t offsetX;
  int16_t offsetY;
  unsigned int  orientation;
  unsigned int MAX_USED_STRENGTH;
  unsigned int MAX_CONSECUTIVE_DRAWS;
  unsigned int DELAY_ZERO_VALUE; // 70 // probably less, this can be adjusted, by max x position, the nearer to the center the less waits
  unsigned int DELAY_AFTER_T1_END_VALUE;
  uint16_t SCALE_STRENGTH_DIF;
  unsigned int cycleEquivalent;
  int beamOffBetweenConsecutiveDraws;
} Settings;


/* values that we remember for possible optimization purposes */



Settings *currentSettings;

void applyLoadedSettings()
{
  currentSettings = (Settings *)v_settingsBlob;
  crankyFlag = currentSettings->crankyFlag;
  Vec_Rfrsh = currentSettings->Vec_Rfrsh;
  optimizationON = currentSettings->optimizationON;
  calibrationValue = currentSettings->calibrationValue;
  sizeX = currentSettings->sizeX;
  sizeY = currentSettings->sizeY;
  offsetX = currentSettings->offsetX;
  offsetY = currentSettings->offsetY;
  MAX_USED_STRENGTH = currentSettings->MAX_USED_STRENGTH;
  MAX_CONSECUTIVE_DRAWS = currentSettings->MAX_CONSECUTIVE_DRAWS;
  DELAY_ZERO_VALUE = currentSettings->DELAY_ZERO_VALUE;
  DELAY_AFTER_T1_END_VALUE = currentSettings->DELAY_AFTER_T1_END_VALUE;
  orientation = currentSettings->orientation;
  SCALE_STRENGTH_DIF = currentSettings->SCALE_STRENGTH_DIF;
  cycleEquivalent = currentSettings->cycleEquivalent;
  beamOffBetweenConsecutiveDraws = currentSettings->beamOffBetweenConsecutiveDraws;
}
void prepareSaveSettings()
{
  currentSettings = (Settings *)v_settingsBlob;
  currentSettings->crankyFlag = crankyFlag;
  currentSettings->Vec_Rfrsh = Vec_Rfrsh;
  currentSettings->optimizationON = optimizationON;
  currentSettings->calibrationValue = calibrationValue;
  currentSettings->sizeX = sizeX;
  currentSettings->sizeY = sizeY;
  currentSettings->offsetX = offsetX;
  currentSettings->offsetY = offsetY;
  currentSettings->MAX_USED_STRENGTH = MAX_USED_STRENGTH;
  currentSettings->MAX_CONSECUTIVE_DRAWS = MAX_CONSECUTIVE_DRAWS;
  currentSettings->DELAY_ZERO_VALUE = DELAY_ZERO_VALUE;
  currentSettings->DELAY_AFTER_T1_END_VALUE = DELAY_AFTER_T1_END_VALUE;
  currentSettings->orientation = orientation;
  currentSettings->SCALE_STRENGTH_DIF = SCALE_STRENGTH_DIF;
  currentSettings->cycleEquivalent = cycleEquivalent;
  currentSettings->beamOffBetweenConsecutiveDraws = beamOffBetweenConsecutiveDraws;
}

// if defined each "game" has a complete config file
#define ONE_FILE_CONFIG


#ifndef ONE_FILE_CONFIG

// expects to be in root directData
// expects filesystem to be initialized


// expects to be in root directData
// expects filesystem to be initialized
int v_loadSettings(char *name, unsigned char *blob, int blobSize)
{

  knownName = name;
  knownBlob = blob;
  knownBlobSize = blobSize;

  name = "default";

  if (knownName[0] != (char) 0)
    printf("Loading settings for: %s!\r\n", knownName);
  else
    printf("Loading settings for: %s!\r\n", name);

  char *settingsDir = "settings";

  int err=0;
  err = chdir (settingsDir);
  if (err)
  {
    printf("NO settings directory found...(%i)!\r\n", errno);
    return 0;
  }

  FILE *fileRead;
  fileRead = fopen(name, "rb");
  if (fileRead == 0)
  {
    printf("Could not open file %s (%i) \r\n", name, errno);
    err = chdir("..");
    return 0;
  }

  unsigned int lenLoaded=0;
  lenLoaded = fread(v_settingsBlob, V_SETTINGS_SIZE, 1, fileRead);
  if (1 != lenLoaded)
  {
    printf("Read(1) of %s fails (len loaded: %i) (Error: %i)\r\n", name, lenLoaded, errno);
    fclose(fileRead);
    err = chdir("..");
    return 0;
  }
  applyLoadedSettings();
  if (knownName[0] != (char) 0)
  {
    fileRead = fopen(knownName, "rb");
    if (fileRead == 0)
    {
      printf("Could not open file %s (%i) \r\n", knownName, errno);
      err = chdir("..");
      return 0;
    }
    if (blobSize != 0)
    {
      lenLoaded = fread(blob, blobSize, 1, fileRead);
      if (1 != lenLoaded)
      {
        printf("Read(2) of %s fails (len loaded: %i) (Error: %i)\r\n", knownName, lenLoaded, errno);
        fclose(fileRead);
        err = chdir("..");
        return 0;
      }
    }
    fclose(fileRead);
  }
  err = chdir("..");
  return 1;
}
int v_saveSettings(char *name, unsigned char *blob, int blobSize)
{
  knownName = name;
  knownBlob = blob;
  knownBlobSize = blobSize;

  name = "default";

  if (knownName[0] != (char) 0)
    printf("Saving settings for: %s!\r\n", knownName);
  else
    printf("Saving settings for: %s!\r\n", name);

  char *settingsDir = "settings";



  int err=0;
  err = chdir (settingsDir);
  if (err)
  {
    printf("NO settings directory found...(%i)!\r\n", errno);
    return 0;
  }
  prepareSaveSettings();

  FILE *fileWrite;
  // always as a "new file"
  fileWrite = fopen(name, "wb");
  if (fileWrite == 0)
  {
    printf("Could not open file %s (%i) \r\n", name, errno);
    err = chdir("..");
    return 0;
  }
  unsigned int lenSaved=0;
  lenSaved = fwrite(v_settingsBlob, V_SETTINGS_SIZE, 1, fileWrite);
  if ( lenSaved != 1)
  {
    printf("File not saved (1) (size written = %i) (error: %i)\r\n", lenSaved, errno);
    fclose(fileWrite);
    err = chdir("..");
    return 0;
  }
  fclose(fileWrite);
  if (knownName[0] != (char) 0)
  {
    fileWrite = fopen(knownName, "wb");
    if (fileWrite == 0)
    {
      printf("Could not open file %s (%i) \r\n", knownName, errno);
      err = chdir("..");
      return 0;
    }
    if (blobSize != 0)
    {
      lenSaved = fwrite(blob, blobSize, 1, fileWrite);
      if ( lenSaved != 1)
      {
        printf("File not saved (2) (size written = %i) (error: %i)\r\n", lenSaved, errno);
        fclose(fileWrite);
        err = chdir("..");
        return 0;
      }
    }
    fclose(fileWrite);
  }
  err = chdir("..");
  return 1;
}
#else // ONE_FILE_CONFIG

// expects to be in root directData
// expects filesystem to be initialized


// expects to be in root directData
// expects filesystem to be initialized
int v_loadSettings(char *name, unsigned char *blob, int blobSize)
{
  knownName = name;
  knownBlob = blob;
  knownBlobSize = blobSize;

  if (knownName[0] == (char) 0)
  {
    name = "default";
  }
  printf("Loading settings for: %s!\r\n", name);

  char *settingsDir = "settings";

  int err=0;
  err = chdir (settingsDir);
  if (err)
  {
    printf("NO settings directory found...(%i)!\r\n", errno);
    return 0;
  }

  FILE *fileRead;
  fileRead = fopen(name, "rb");
  if (fileRead == 0)
  {
    printf("Could not open file %s (%i) \r\n", name, errno);
    err = chdir("..");
    return 0;
  }

  unsigned int lenLoaded=0;
  lenLoaded = fread(v_settingsBlob, V_SETTINGS_SIZE, 1, fileRead);
  if (1 != lenLoaded)
  {
    printf("Read(1) of %s fails (len loaded: %i) (Error: %i)\r\n", name, lenLoaded, errno);
    fclose(fileRead);
    err = chdir("..");
    return 0;
  }
  applyLoadedSettings();
  if (knownName[0] != (char) 0)
  {
    if (blobSize != 0)
    {
      lenLoaded = fread(blob, blobSize, 1, fileRead);
      if (1 != lenLoaded)
      {
        printf("Read(2) of %s fails (len loaded: %i) (Error: %i)\r\n", name, lenLoaded, errno);
        fclose(fileRead);
        err = chdir("..");
        return 0;
      }
    }
  }
  fclose(fileRead);
  err  = chdir("..");
  return 1;
}
int v_saveSettings(char *name, unsigned char *blob, int blobSize)
{
  knownName = name;
  knownBlob = blob;
  knownBlobSize = blobSize;

  if (knownName[0] == (char) 0)
    name = "default";
  printf("Saving settings for: %s!\r\n", name);

  char *settingsDir = "settings";


  int err=0;
  err = chdir (settingsDir);
  if (err)
  {
    printf("NO settings directory found...(%i)!\r\n", errno);
    return 0;
  }
  prepareSaveSettings();

  FILE *fileWrite;
  // always as a "new file"
  fileWrite = fopen(name, "wb");
  if (fileWrite == 0)
  {
    printf("Could not open file %s (%i) \r\n", name, errno);
    err = chdir("..");
    return 0;
  }
  unsigned int lenSaved=0;
  lenSaved = fwrite(v_settingsBlob, V_SETTINGS_SIZE, 1, fileWrite);
  if ( lenSaved != 1)
  {
    printf("File not saved (1) (size written = %i) (error: %i)\r\n", lenSaved, errno);
    fclose(fileWrite);
    err = chdir("..");
    return 0;
  }
  if (knownName[0] != (char) 0)
  {
    if (blobSize != 0)
    {
      lenSaved = fwrite(blob, blobSize, 1, fileWrite);
      if ( lenSaved != 1)
      {
        printf("File not saved (2) (size written = %i) (error: %i)\r\n", lenSaved, errno);
        fclose(fileWrite);
        err = chdir("..");
        return 0;
      }
    }
  }
  fclose(fileWrite);
  err = chdir("..");
  return 1;
}


#endif











// if zeroing is enabled -we can not moveor draw
// ensure it is off!


// if zeroing is enabled -we can not moveor draw
// ensure it is off!

// moving from the current position, without bothering about the beam illumination state
// zeroing off is assumed
#define PL_CONTINUE_TO(_x, _y) \
do{ \
  if (cpb->force & PL_BASE_FORCE_USE_FIX_SIZE) \
  { \
    timingNow = cpb->timingForced; \
    if ((ABS((_y)/(timingNow+SCALE_STRENGTH_DIF))>127) || (ABS((_x)/(timingNow+SCALE_STRENGTH_DIF))>127)) \
    { /* we must use another scaling!!! */ \
      SET_OPTIMAL_SCALE_SMS(_x, _y,cpb->sms); \
      timingNow = currentScale; \
    } \
  } \
  else \
  { \
    SET_OPTIMAL_SCALE_SMS(_x, _y,cpb->sms); \
    timingNow = currentScale; \
  } \
  rampingNow = 1; \
  /* if (myDebug) printf("BASE 1: x,y: %i,%i \r\n", _x, _y); */ \
  pl[fpc].y = ((_y)/(timingNow+SCALE_STRENGTH_DIF)); \
  pl[fpc].x = ((_x)/(timingNow+SCALE_STRENGTH_DIF)); \
  /* if (myDebug) printf("BASE 2: x,y: %i,%i :%i\r\n",  pl[fpc].x*timingNow,  pl[fpc].y*timingNow, timingNow); */ \
  if (currentY == pl[fpc].y) \
  { \
  /* not need to set Y at all */  \
  } \
  else \
  { \
    pl[fpc].flags = pl[fpc].flags | PL_Y_MUST_BE_SET; \
    if (currentMUX != MUX_Y) \
      pl[fpc].flags = pl[fpc].flags | PL_MUX_Y_MUST_BE_SET; \
    currentMUX = MUX_Y; \
    /* we must set Y */ \
    if ((pl[fpc].y==0) && (currentY!=0)) \
    {  \
        if ((crankyFlag&CRANKY_NULLING_CALIBRATE) && (!calibDone)) \
        { \
            if (calibrationValue == 0) \
            { \
              pl[fpc].flags = pl[fpc].flags | PL_CALIBRATE_0 | PL_MUX_Y_MUST_BE_SET; /* because integrator reset, when y = 0 */  \
              currentA =0x0; \
            } \
            else \
            { /* costs much time - perhaps we can leave that out? */ \
              /* pl[fpc].flags = pl[fpc].flags | PL_CALIBRATE | PL_MUX_Y_MUST_BE_SET; */ /* because integrator reset, when y = 0 */  \
              /* currentA =0x100;*/ \
            } \
        } \
        else if (crankyFlag&CRANKY_NULLING_WAIT) \
        { \
            pl[fpc].flags = pl[fpc].flags | PL_Y_DELAY_TO_NULL; /* because integrator reset, when y = 0 */  \
        } \
    } \
    if (currentA == pl[fpc].y) \
    { \
      ;/* no need to load A */ \
    } \
    else \
    { \
      /* we must set VIA_a */ \
      currentA = pl[fpc].y; \
      pl[fpc].flags = pl[fpc].flags | PL_Y_A_MUST_BE_SET; \
    } \
    currentY = currentA; /* store A to Y */ \
  } \
  if (currentA != pl[fpc].x) \
  { \
    pl[fpc].flags = pl[fpc].flags | PL_X_A_MUST_BE_SET; \
  } \
  currentA = pl[fpc].x; \
  if (currentMUX != MUX_X) \
    pl[fpc].flags = pl[fpc].flags | PL_MUX_X_MUST_BE_SET; \
  currentMUX = MUX_X; \
  if ((t1_timingSet&0xff) == (timingNow&0xff)) \
    pl[fpc].flags = pl[fpc].flags | PL_T1_LO_EQUALS; \
  t1_timingSet=timingNow; \
} while (0)

#ifndef AVIOD_TICKS
#define INIT_NEXT_PIPELINE_ITEM \
  pl[fpc].this_timing = timingNow; \
  pl[fpc].last_timing = timingLast; \
  fpc++; \
  pl[fpc].base=0; \
  pl[fpc].flags = 0; \
  if (lastMustFinish) \
    pl[fpc].flags = pl[fpc].flags | PL_LAST_MUST_FINISH; \
  if (rampingNow) \
    pl[fpc].flags = pl[fpc].flags | PL_LAST_IS_RAMPING;  \
  timingLast = timingNow; \
  timingNow = 0; \
  rampingLast = rampingNow;  \
  rampingNow = 0;
#else
#define INIT_NEXT_PIPELINE_ITEM \
  scaleTotal += timingNow + SCALETOTAL_OFFSET; \
  pl[fpc].this_timing = timingNow; \
  pl[fpc].last_timing = timingLast; \
  fpc++; \
  pl[fpc].base=0; \
  pl[fpc].flags = 0; \
  if (lastMustFinish) \
    pl[fpc].flags = pl[fpc].flags | PL_LAST_MUST_FINISH; \
  if (rampingNow) \
    pl[fpc].flags = pl[fpc].flags | PL_LAST_IS_RAMPING;  \
  timingLast = timingNow; \
  timingNow = 0; \
  rampingLast = rampingNow;  \
  rampingNow = 0;
#endif

#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

#define ADD_CLIPPED_VECTOR(_x0,_y0,_x1,_y1, baseVector) \
do { \
  cpb->x0 = _x0; \
  cpb->y0 = _y0; \
  cpb->x1 = _x1; \
  cpb->y1 = _y1; \
  cpb->intensity = baseVector->intensity; \
  cpb->force = baseVector->force | PL_BASE_FORCE_NOT_CLIPPED; \
  cpb->force &= ~PL_BASE_FORCE_EMPTY; \
  cpb->sms = MAX_USED_STRENGTH; \
  cpb = &pb[++pipelineCounter]; \
  cpb->debug[0] = 0; \
  cpb->pattern = 0; \
  cpb->force = 0; \
  } while (0)

#define ADD_PIPELINE(_x0,_y0,_x1,_y1, intens) \
do { \
  cpb->x0 = _x0; \
  cpb->y0 = _y0; \
  cpb->x1 = _x1; \
  cpb->y1 = _y1; \
  cpb->intensity = intens; \
  cpb->force = cpb->force | PL_BASE_FORCE_NOT_CLIPPED | PL_BASE_FORCE_STABLE; \
  cpb->sms = MAX_USED_STRENGTH; \
  cpb = &pb[++pipelineCounter]; \
  cpb->debug[0] = 0; \
  cpb->pattern = 0; \
  cpb->force = 0; \
  } while (0)


enum {TOP = 0x1, BOTTOM = 0x2, RIGHT = 0x4, LEFT = 0x8};
enum {FALSE, TRUE};
typedef unsigned int outcode;
outcode compute_outcode(int x, int y, int xmin, int ymin, int xmax, int ymax)
{
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

// returns x1 = 1000000 on complete outside!
void cohen_sutherlandCustom(int32_t *x1, int32_t *y1,  int32_t *x2, int32_t *y2, int xmin, int ymin, int xmax, int ymax)
{
  int accept;
  int done;
  outcode outcode1, outcode2;
  accept = FALSE;
  done = FALSE;
  outcode1 = compute_outcode(*x1, *y1, xmin, ymin, xmax, ymax);
  outcode2 = compute_outcode(*x2, *y2, xmin, ymin, xmax, ymax);
  do
  {
    if (outcode1 == 0 && outcode2 == 0)
    {
      accept = TRUE;
      done = TRUE;
    }
    else if (outcode1 & outcode2)
    {
      done = TRUE;
    }
    else
    {
      int x, y;
      int outcode_ex = outcode1 ? outcode1 : outcode2;
      if (outcode_ex & TOP)
      {
        x = *x1 + (*x2 - *x1) * (ymax - *y1) / (*y2 - *y1);
        y = ymax;
      }
      else if (outcode_ex & BOTTOM)
      {
        x = *x1 + (*x2 - *x1) * (ymin - *y1) / (*y2 - *y1);
        y = ymin;
      }
      else if (outcode_ex & RIGHT)
      {
        y = *y1 + (*y2 - *y1) * (xmax - *x1) / (*x2 - *x1);
        x = xmax;
      }
      else
      {
        y = *y1 + (*y2 - *y1) * (xmin - *x1) / (*x2 - *x1);
        x = xmin;
      }
      if (outcode_ex == outcode1)
      {
        *x1 = x;
        *y1 = y;
        outcode1 = compute_outcode(*x1, *y1, xmin, ymin, xmax, ymax);
      }
      else
      {
        *x2 = x;
        *y2 = y;
        outcode2 = compute_outcode(*x2, *y2, xmin, ymin, xmax, ymax);
      }
    }
  } while (done == FALSE);
  if (accept == TRUE)
  {
    return;
  }
  *x1 = 1000000;
  return;
}

// reuses current baseline
void cohen_sutherland(VectorPipelineBase *baseVector, int xmin, int ymin, int xmax, int ymax)
{
  int x1 = baseVector->x0;
  int y1 = baseVector->y0;
  int x2 = baseVector->x1;
  int y2 = baseVector->y1;

  int accept;
  int done;
  outcode outcode1, outcode2;
  accept = FALSE;
  done = FALSE;
  outcode1 = compute_outcode(x1, y1, xmin, ymin, xmax, ymax);
  outcode2 = compute_outcode(x2, y2, xmin, ymin, xmax, ymax);
  do
  {
    if (outcode1 == 0 && outcode2 == 0)
    {
      accept = TRUE;
      done = TRUE;
    }
    else if (outcode1 & outcode2)
    {
      done = TRUE;
    }
    else
    {
      int x, y;
      int outcode_ex = outcode1 ? outcode1 : outcode2;
      if (outcode_ex & TOP)
      {
        x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
        y = ymax;
      }
      else if (outcode_ex & BOTTOM)
      {
        x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
        y = ymin;
      }
      else if (outcode_ex & RIGHT)
      {
        y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
        x = xmax;
      }
      else
      {
        y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
        x = xmin;
      }
      if (outcode_ex == outcode1)
      {
        x1 = x;
        y1 = y;
        outcode1 = compute_outcode(x1, y1, xmin, ymin, xmax, ymax);
      }
      else
      {
        x2 = x;
        y2 = y;
        outcode2 = compute_outcode(x2, y2, xmin, ymin, xmax, ymax);
      }
    }
  } while (done == FALSE);
  if (accept == TRUE)
  {
    baseVector->x0 =x1;
    baseVector->y0 =y1;
    baseVector->x1 =x2;
    baseVector->y1 =y2;
    return;
  }
  baseVector->force |= PL_BASE_FORCE_EMPTY;
  return;
}

// https://stackoverflow.com/questions/47884592/how-to-reverse-cohen-sutherland-algorithm
// adds new baselines
// invalidates old baseline
void reverse_cohen_sutherland(VectorPipelineBase *baseVector, int xmin, int ymin, int xmax, int ymax)
{
  int x1 = baseVector->x0;
  int y1 = baseVector->y0;
  int x2 = baseVector->x1;
  int y2 = baseVector->y1;
  baseVector->force |= PL_BASE_FORCE_EMPTY;

  int accept;
  int done;
  outcode outcode1, outcode2;
  accept = FALSE;
  done = FALSE;
  outcode1 = compute_outcode(x1, y1, xmin, ymin, xmax, ymax);
  outcode2 = compute_outcode(x2, y2, xmin, ymin, xmax, ymax);
  do
  {
    if (outcode1 == 0 && outcode2 == 0)
    {
      done = TRUE;
    }
    else if (outcode1 & outcode2)
    {
      accept = TRUE;
      done = TRUE;
    }
    else
    {
      int x, y;
      int outcode_ex = outcode1 ? outcode1 : outcode2;

      if (outcode_ex & TOP)
      {
        x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
        y = ymax;
      }
      else if (outcode_ex & BOTTOM)
      {
        x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
        y = ymin;
      }
      else if (outcode_ex & RIGHT)
      {
        y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
        x = xmax;
      }
      else
      {
        y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
        x = xmin;
      }

      if (outcode_ex == outcode1)
      {
        ADD_CLIPPED_VECTOR(x1,y1,x,y,baseVector);
        x1 = x;
        y1 = y;
        outcode1 = compute_outcode(x1, y1, xmin, ymin, xmax, ymax);
      }
      else
      {
        ADD_CLIPPED_VECTOR(x,y,x2,y2,baseVector);
        x2 = x;
        y2 = y;
        outcode2 = compute_outcode(x2, y2, xmin, ymin, xmax, ymax);
      }
    }
  } while (done == FALSE);
  if (accept == TRUE)
  {
    ADD_CLIPPED_VECTOR(x1,y1,x2,y2,baseVector);
    return;
  }
  return;
}

void handlePipeline()
{
  // build a list of all items
  // and put "dots" last
  // and stables first (hint)
  if (pipelineCounter>0)
     pipelineFilled = 1;
  // the array is converted to a double linked list
  // that way we can later easily manipulate the "array"
  VectorPipelineBase *head=(VectorPipelineBase *)0;
  VectorPipelineBase *element=(VectorPipelineBase *)0;

  // move dots to be done "last" - possibly without ZERO REF
  // in between
  // dots are often fast moving small objects
  // a) they desturb the integrators
  // b) exact position is not so important!
  //
  // last head is for "dots" -> for a short time an own linked list
  // which is later put at the end of the "normal" vectors
  VectorPipelineBase *lastHead=(VectorPipelineBase *)0;
  VectorPipelineBase *lastElement=(VectorPipelineBase *)0;

  VectorPipelineBase *firstHead=(VectorPipelineBase *)0;
  VectorPipelineBase *firstElement=(VectorPipelineBase *)0;


  // clipping is "costly"
  // and is done on the array
  // this way "reverse" clipping can more easily
  // add new vectors to the end, since
  // we still are in "addition" mode of the pipeline
  if (clipActive)
  {
    // add a "window"
    ADD_PIPELINE(clipminX, clipminY, clipminX, clipmaxY, 90);
    ADD_PIPELINE(clipminX, clipmaxY, clipmaxX, clipmaxY, 90);
    ADD_PIPELINE(clipmaxX, clipmaxY, clipmaxX, clipminY, 90);
    ADD_PIPELINE(clipmaxX, clipminY, clipminX, clipminY, 90);

    VectorPipelineBase *c_cpb;
    for (int i=0; i<pipelineCounter; i++)
    {
      c_cpb = &pb[i];
      // we can "hint" not to clip
      // e.g. already inverse clipped and added reverse results!
      if (!(c_cpb->force & PL_BASE_FORCE_NOT_CLIPPED))
      {
        if (clipMode==0)
        {
          cohen_sutherland(c_cpb, clipminX, clipminY, clipmaxX, clipmaxY);
        }
        else if (clipMode==1)
        {
          reverse_cohen_sutherland(c_cpb, clipminX, clipminY, clipmaxX, clipmaxY);
        }
      }
    }
  }

  // now we build the linked list
  //
  // adding "stables" to the front
  // in case "stables" are sorted (which is likely)
  // backwards ensured, that stables are correctly ordered!
  // when we move them to the front!
  for (int i=0; i<pipelineCounter; i++)
  {
    cpb = &pb[i];

    // stables are added in front of the list head!
    if (cpb->force & PL_BASE_FORCE_STABLE)
    {
      if (firstHead == (VectorPipelineBase *)0)
      {
        firstHead = cpb;
        firstElement = cpb;
        firstElement->next = (VectorPipelineBase *)0;
        firstElement->previous = (VectorPipelineBase *)0;
      }
      else
      {
        cpb->next = (VectorPipelineBase *)0;
        cpb->previous = firstElement;
        firstElement->next = cpb;
        firstElement = cpb;
      }
      continue;
    }

    // MOVE dots to the back of the list!
    if ((cpb->x0 == cpb->x1) && (cpb->y0 == cpb->y1))
    {
      // in some cases it might be ok
      // to not zero "stars"
      // for now - we let the algorythm decide
      // and do not force to NOT ZERO
      //      if (!(cpb->force & PL_BASE_FORCE_ZERO))
      //        cpb->force |= PL_BASE_FORCE_NO_ZERO; // not sure if that is always a good thing!
      if (lastHead == (VectorPipelineBase *)0)
      {
        lastHead = cpb;
        lastElement = cpb;
        lastElement->next = (VectorPipelineBase *)0;
        lastElement->previous = (VectorPipelineBase *)0;
      }
      else
      {
        cpb->previous = lastElement;
        lastElement->next = cpb;
        lastElement = cpb;
        lastElement->next = (VectorPipelineBase *)0;
      }
      continue;
    }

    // everything not dot and not stable
    // is just added to the normal list
    if (head == (VectorPipelineBase *)0)
    {
      head = cpb;
      element = cpb;
      element->next = (VectorPipelineBase *)0;
      element->previous = (VectorPipelineBase *)0;
    }
    else
    {
      cpb->previous = element;
      element->next = cpb;
      element = cpb;
      element->next = (VectorPipelineBase *)0;
    }
  }
  if (lastHead != 0)
  {
    // add dots last...
    if (head == (VectorPipelineBase *)0)
    {
      head = lastHead;
    }
    if (element != (VectorPipelineBase *)0)
    {
      element->next = lastHead;
      lastHead->previous = element;
    }
  }
  if (firstHead != 0)
  {
    if (head != 0)
    {
      firstElement->next = head;
      head->previous = firstElement;
    }
    head = firstHead;
  }
  cpb = head;

  // todo
  // might sort for brightness
  // supposedly all same brightnesses will build a (or several) "clusters"

  // now we calculate our "optimal"
  // VectorPipeline
  // for that we keep track of a hell of a lot
  // single items
  // like position, and contents of registers...
  // starts with position 0,0 assumed
  int cx = 0;
  int cy = 0;

  int beamState = 0; // off
  int currentY = 0x100;  // value in reg A (illegal)
  int currentA = 0x100;  // value in reg A (illegal)
  int zeroActive = 1;
  int currentBrightness = 0x100; // invalid
  int timingNow = 0;
  int timingLast = 0;
  int t1_timingSet = 0xffff;
  int rampingNow = 0;
  int rampingLast;
  int currentMUX = 0x100;
  int lastMustFinish = 0;
  #define  MUX_BRIGHTNESS 4
  #define  MUX_Y 0
  #define  MUX_X 1

  // (since we allways reuse the same array elements, we must ensure they are clean from start)
  int fpc = 0; // final pipeline counter
  pl[fpc].flags = 0;

  // not used anymore...
  consecutiveDraws = 0;

  // when clipping thats not true anymore - but who cares
  // clippin is special anyway
  if (myDebug)  printf("Base pipeline has %i items!\r\n", pipelineCounter);
  while (cpb != (VectorPipelineBase *)0)
  {
    int calibDone = 0;
    // invalidated element... do nothing
    if (cpb->force & PL_BASE_FORCE_EMPTY)
    {
      if (myDebug)  printf("Base EMPTY!\r\n");
      // cleanup and next!
      cpb->force = 0;
      cpb = cpb->next;
      continue;
    }
    if (cpb->force & PL_BASE_FORCE_DEFLOK)
    {
      if (myDebug)  printf("Base DEFLOK!\r\n");
      pl[fpc].type = PL_DEFLOK;
      timingNow = 0;
      rampingNow = 0;
      lastMustFinish = 0;
      currentMUX = MUX_X;
      currentA = 0x100;
      calibDone = 1;
      if (beamState)
      {
        pl[fpc].flags = pl[fpc].flags | PL_SWITCH_BEAM_OFF;
        beamState = 0;
      }
      zeroActive = 1;
      cx = 0;
      cy = 0;
      consecutiveDraws = 0;
      INIT_NEXT_PIPELINE_ITEM;
      continue;
    }


    /************ HANDLE brightness of Vector ************/
    if (currentBrightness != cpb->intensity)
    {
      if (myDebug)  printf("Base BRIGHTNESS!\r\n");
      pl[fpc].type = PL_SET_BRIGHTNESS;
      pl[fpc].intensity = cpb->intensity;
      currentBrightness = cpb->intensity;

      if (currentA != currentBrightness)
      {
        pl[fpc].flags = pl[fpc].flags | PL_I_A_MUST_BE_SET;
        currentA = currentBrightness;
      }

      // if we don't switch the beam off
      // while setting the brightness -
      // bright dots might be possible!
      if (beamState)
      {
        pl[fpc].flags = pl[fpc].flags | PL_SWITCH_BEAM_OFF;
        beamState = 0;
      }

      currentMUX = MUX_BRIGHTNESS;
      timingNow = 0;
      rampingNow = 0;
      lastMustFinish = 0;
      INIT_NEXT_PIPELINE_ITEM;
    }

    // forcing zero prevents algorythm!
    if (((cpb->force & PL_BASE_FORCE_ZERO )== PL_BASE_FORCE_ZERO) && (!zeroActive))
    {
      if (myDebug)  printf("Base ZERO!\r\n");
      pl[fpc].type = PL_ZERO;
      timingNow = DELAY_ZERO_VALUE;
// for now ZEROING
// allways recalibs!

/*
      if (cpb->force & PL_BASE_FORCE_RESET_ZERO_REF )
      {
        pl[fpc].flags = pl[fpc].flags | PL_CALIBRATE_0;
        calibDone = 1;
        currentMUX = MUX_X;
        currentA = 0;
      }
      if (cpb->force & PL_BASE_FORCE_CALIBRATE_INTEGRATORS )
*/
{
//        pl[fpc].flags = pl[fpc].flags | PL_CALIBRATE;
  calibDone = 1;
  currentMUX = MUX_X;
  currentA = 0x100;
//  currentY = 0;
}

      // todo
      // zero timer in relation to current position
      // the less far from zero -> the less time needed!
      rampingNow = 0;
      lastMustFinish = 1;
      if (beamState)
      {
        pl[fpc].flags = pl[fpc].flags | PL_SWITCH_BEAM_OFF;
        beamState = 0;
      }
      zeroActive = 1;
      cx = 0;
      cy = 0;
      consecutiveDraws = 0;
      INIT_NEXT_PIPELINE_ITEM;
    }
    else if (!(cpb->force & PL_BASE_FORCE_NO_ZERO )) // do not force the algorythm if we are not prepared to eventually ZERO
    {
      // VERY EASY algorythm
      // like the "old" one

      // how far away is the cursor from the position we want to start drawing?
      int32_t xMoveDif = cpb->x0-cx;
      int32_t yMoveDif = cpb->y0-cy;

      // test if the position of the last end - and the current start differs by more than our set margin
      if ((ABS(xMoveDif) > POSITION_MARGIN) || (ABS(yMoveDif) > POSITION_MARGIN) )
      {
        // not on the same position, so we either do a MOVE
        // or do a Zero

        // the only left over criteria a the moment is - how far away from the last cursor position
        // if too far away, we throw in a zero ref
        // old:
        //    consecutiveDraws > MAX_CONSECUTIVE_DRAWS;
        //    resetPos += ABS((currentScale-GET_OPTIMAL_SCALE(xMoveDif, yMoveDif)) > 20);
        int resetPos = ((ABS(xMoveDif)>resetToZeroDifMax) || (ABS(yMoveDif)>resetToZeroDifMax) );

        if ((resetPos) && (!zeroActive))
        {
          if (myDebug)  printf("Base CALC ZERO!\r\n");
          // do a zeroing!
          // copy paste from above
          pl[fpc].type = PL_ZERO;
          timingNow = DELAY_ZERO_VALUE;

    // zero allways recalibs
{
//        pl[fpc].flags = pl[fpc].flags | PL_CALIBRATE;
  calibDone = 1;
  currentMUX = MUX_X;
  currentA = 0x100;
//  currentY = 0;
}




          // todo
          // zero timer in relation to current position
          // the less far from zero -> the less time needed!
          rampingNow = 0;
          lastMustFinish = 1;
          if (beamState)
          {
            pl[fpc].flags = pl[fpc].flags | PL_SWITCH_BEAM_OFF;
            beamState = 0;
          }
          zeroActive = 1;
          cx = 0;
          cy = 0;
          consecutiveDraws = 0;
          INIT_NEXT_PIPELINE_ITEM;
        }
      }
    }

    // the result from above is either
    // a) we are not correctly positioned -> do a MOVE
    // b) we ARE correctly positioned -> do a DRAW
    // both are handled below
    /*****************************************************/
    if ((cpb->force & PL_BASE_FORCE_RESET_ZERO_REF ) && (!calibDone))
    {
        if (myDebug)  printf("Base PL_CALIBRATE_0!\r\n");
        pl[fpc].flags = pl[fpc].flags | PL_CALIBRATE_0;
        currentA = 0;  // value in reg A (illegal)
        currentMUX = MUX_X;
    }
    if ((cpb->force & PL_BASE_FORCE_CALIBRATE_INTEGRATORS ) && (!calibDone))
    {
        if (myDebug)  printf("Base PL_CALIBRATE!\r\n");
        pl[fpc].flags = pl[fpc].flags | PL_CALIBRATE;
        currentA = 0x100;  // value in reg A (illegal)
        currentMUX = MUX_X;
    }

    /************ HANDLE start position of Vector ************/
    // fill secondary "optimized" pipeline with values
    if ((cx != cpb->x0) || (cy != cpb->y0))
    {
      if (myDebug)  printf("Base MOVE!\r\n");
      pl[fpc].type = PL_MOVE;

      // we nust move to start the new vector
      if (zeroActive)
      {
        zeroActive = 0;
        pl[fpc].flags = pl[fpc].flags | PL_DEACTIVATE_ZERO;
      }
      if (beamState)
      {
        beamState = 0;
        pl[fpc].flags = pl[fpc].flags | PL_SWITCH_BEAM_OFF;
      }

      PL_CONTINUE_TO((cpb->x0-cx), (cpb->y0-cy));
      cx = cpb->x0;
      cy = cpb->y0;

      lastMustFinish = 1;
      INIT_NEXT_PIPELINE_ITEM;
      consecutiveDraws++;
    }
    /*********************************************************/

    /************ HANDLE draw position of Vector ************/
    if ((cx != cpb->x1) || (cy != cpb->y1))
    {
      pl[fpc].type = PL_DRAW;

      // we must move to start the new vector
      if (zeroActive)
      {
        zeroActive = 0;
        pl[fpc].flags = pl[fpc].flags | PL_DEACTIVATE_ZERO;
      }

      // before timer
      if (beamOffBetweenConsecutiveDraws)
      {
        if (beamState)
          pl[fpc].flags = pl[fpc].flags | PL_SWITCH_BEAM_OFF;
        beamState = 0;
      }
      // after timer
      if (!beamState)
      {
        beamState = 1;
        pl[fpc].flags = pl[fpc].flags | PL_SWITCH_BEAM_ON;
      }
      PL_CONTINUE_TO(cpb->x1-cpb->x0, cpb->y1-cpb->y0);
      cx = cpb->x1;
      cy = cpb->y1;

      // remember the "base" for possible debug informations
      // todo:
      // while debugging - prevent baselist switching!
      pl[fpc].base=cpb;
      lastMustFinish = 1;

      if ((cpb->pattern != 0) && (cpb->pattern != 0xff))
      {
        if (myDebug)  printf("Base PATTERN!\r\n");
        pl[fpc].type = PL_DRAW_PATTERN;
        pl[fpc].pattern = cpb->pattern;
        lastMustFinish = 0;
        rampingNow = 0;
      }
      else
      {
        if (myDebug)  printf("Base DRAW!\r\n");
      }

      INIT_NEXT_PIPELINE_ITEM;
      consecutiveDraws++;
    }
    else
    {
      // start and end coordinates are the same
      // do a "dot"
      if (myDebug)  printf("Base DOT!\r\n");
      // remember the "base" for possible debug informations
      // todo:
      // while debugging - prevent baselist switching!
      pl[fpc].base=cpb;

      pl[fpc].type = PL_DRAW_DOT;
      if (!beamState)
      {
        beamState = 1;
        pl[fpc].flags = pl[fpc].flags | PL_SWITCH_BEAM_ON;
      }
      // dot dwell?
      // todo dwell for this_timing?
      if (cpb->force & PL_BASE_FORCE_USE_DOT_DWELL)
        timingNow = cpb->timingForced;
      else
        timingNow = v_dotDwell;
      cx = cpb->x1;
      cy = cpb->y1;

      rampingNow = 0;
      lastMustFinish = 1;
      INIT_NEXT_PIPELINE_ITEM;
    }

    /*********************************************************/

    // when calibrate?
    // when calibareted calibration
    // user commands to SYNC etc

    // reset base -> ready to be reused next "round"
    cpb->force = 0;
    cpb->pattern = 0;
    cpb->debug[0] = 0;
    cpb = cpb->next;
  }
  // clean up after last item of the list
  if (beamState)
    pl[fpc].flags = pl[fpc].flags | PL_SWITCH_BEAM_OFF_AFTER | PL_SWITCH_ZERO_AFTER | PL_SWITCH_BEAM_OFF;

  pl[fpc].type = PL_END;
  pl[fpc].last_timing = timingLast;

  // setup next round
  pipelineCounter = 0;
  pipelineAlt = pipelineAlt?0:1;
  pl = _P[pipelineAlt];
  cpb = &pb[0];
}
#define LINE_DEBUG_OUT(...) \
        if (((browseMode) && (lineNo==currentBrowsline) && (currentDisplayedBrowseLine != currentBrowsline)) || (myDebug))  \
        { \
          printf(__VA_ARGS__); \
        }


void displayPipeline()
{
  int c = 0;
  VectorPipeline *dpl = _P[pipelineAlt?0:1];
  int delayedBeamOff=0;
  if (myDebug) printf("Display pipeline started...!\r\n");

#ifdef AVOID_TICKS
	volatile uint32_t* paddr;
	uint32_t fiqtemp;
	uint32_t clk;
	uint32_t gap0;
//	uint32_t gap1;
	uint32_t gap2;
	uint32_t gap3;
	uint32_t comp0;
//	uint32_t comp1;
	uint32_t comp2;
	uint32_t comp3;
	uint32_t gpu_ier1=0, gpu_ier2=0, cpu_ier=0;

	scaleTotal = (scaleTotal * DELAY_PI_CYCLE_EQUIVALENT) + ST_GAP_END;

	/* Wait until no timer interrupts are due within the expected drawing time,
	   otherwise the system clock will get messed up: */
	do
	{
	 clk = bcm2835_st_read() & 0xFFFFFFFF;
         paddr = bcm2835_st + BCM2835_ST_COMP(0)/4;
	 comp0 = bcm2835_peri_read(paddr);
//       paddr = bcm2835_st + BCM2835_ST_COMP(1)/4;
//	 comp1 = bcm2835_peri_read(paddr);
         paddr = bcm2835_st + BCM2835_ST_COMP(2)/4;
	 comp2 = bcm2835_peri_read(paddr);
         paddr = bcm2835_st + BCM2835_ST_COMP(3)/4;
	 comp3 = bcm2835_peri_read(paddr);

	 gap0 = (comp0 - clk);
//	 gap1 = (comp1 - clk);
	 gap2 = (comp2 - clk);
	 gap3 = (comp3 - clk);
	} while ( gap0 < scaleTotal || gap2 < scaleTotal || gap3 < scaleTotal);

	/* Save interrupt configuration and disable interrupts */
	if (bcm2835_int != MAP_FAILED)
	{
	 paddr = bcm2835_int + BCM2835_INT_GPU_IER1/4;
	 gpu_ier1 = bcm2835_peri_read(paddr);
	 paddr = bcm2835_int + BCM2835_INT_GPU_IER2/4;
	 gpu_ier2 = bcm2835_peri_read(paddr);
	 paddr = bcm2835_int + BCM2835_INT_CPU_IER/4;
	 cpu_ier = bcm2835_peri_read(paddr);

	 paddr = bcm2835_int + BCM2835_INT_GPU_IDR1/4;
	 bcm2835_peri_write(paddr,0xFFFFFFFF);
	 paddr = bcm2835_int + BCM2835_INT_GPU_IDR2/4;
	 bcm2835_peri_write(paddr,0xFFFFFFFF);
	 paddr = bcm2835_int + BCM2835_INT_CPU_IDR/4;
	 bcm2835_peri_write(paddr,0x0000007F);

#ifdef DISABLE_FIQ
	 /* Disable Fast Interrupt Requests: */
	 paddr = bcm2835_int + BCM2835_INT_FIQ/4;
	 fiqtemp = bcm2835_peri_read(paddr); // read FIQ control register 0x20C
	 fiqtemp &= ~(1 << 7);              // zero FIQ enable bit 7
	 bcm2835_peri_write(paddr,fiqtemp);/* write back to register
                                          attempting to clear bit 7 of *(intrupt+131) directly
                                          will crash the system
									   */
#endif
	}
	else
	{
	 printf("Interrupt address mapping failed\r\n");
	}
#endif

  if (browseMode)
  {
    v_setBrightness(50);
  }
  int lineNo = 0;

  while (dpl[c].type != PL_END)
  {
    if (browseMode)
    {
        if (lineNo ==currentBrowsline)
        {
          v_setBrightness(127);
        }
        if (lineNo ==currentBrowsline+1)
        {
          v_setBrightness(50);
        }
    }

    switch (dpl[c].type)
    {
      case PL_DEFLOK:
      {
        LINE_DEBUG_OUT("PL DEFLOK\r\n");
        // loop ensures that there are no pending draws/moves
        if (dpl[c].flags & PL_SWITCH_BEAM_OFF) SWITCH_BEAM_OFF();
        v_deflok();
        LINE_DEBUG_OUT("PL DEFLOK CALIB \r\n");
        v_resetIntegratorOffsets();
        break;
      }
      case PL_ZERO:
      {
        LINE_DEBUG_OUT("PL ZERO: %i, %i\r\n", dpl[c+1].last_timing, dpl[c].this_timing);
        #ifndef BEAM_LIGHT_BY_CNTL
        if (dpl[c].flags & PL_SWITCH_BEAM_OFF)
          SWITCH_BEAM_OFF();
        #endif
        ZERO_AND_CONTINUE();
        {
          int timeDone =24;
          // TODO enable calib again when zeroing!
          v_resetIntegratorOffsets();
          dpl[c+1].last_timing = (dpl[c].this_timing - timeDone);
          if (dpl[c+1].last_timing<0) dpl[c+1].last_timing=0;
        }
        break;
      }
      case PL_SET_BRIGHTNESS:
      {
        LINE_DEBUG_OUT("PL Brightness  A = %x\r\n", dpl[c].intensity);
        if (browseMode) break;
        if (dpl[c].flags & PL_I_A_MUST_BE_SET)
        {
          SET_WORD_ORDERED(VIA_port_b, 0x084, dpl[c].intensity);
        }
        else
        {
          SET(VIA_port_b, 0x84); // MUX to intensity
        }
        DELAY_ZSH();
        break;
      }
      case PL_DRAW_DOT:
      {
        LINE_DEBUG_OUT ("PL DOT  %i, %i :%i\r\n    %s\r\n", dpl[c].x*dpl[c].this_timing, dpl[c].y*dpl[c].this_timing, dpl[c].this_timing, ((dpl[c].base != 0)?dpl[c].base->debug:""));
        if (dpl[c].flags & PL_SWITCH_BEAM_ON)
        {
          SWITCH_BEAM_ON();
        }
        break;
      }
      case PL_MOVE:
      {
        LINE_DEBUG_OUT("PL MOVE %i, %i :%i\r\n", dpl[c].x*dpl[c].this_timing, dpl[c].y*dpl[c].this_timing, dpl[c].this_timing);
      }
      case PL_DRAW_PATTERN:
      {
        if (dpl[c].type == PL_DRAW_PATTERN)
        {
          LINE_DEBUG_OUT ("PL DRAW PATTERN \r\n    %s\r\n", ((dpl[c].base != 0)?dpl[c].base->debug:""));
        }
      }
      case PL_DRAW:
      {
        int delayed = 1;
        if (dpl[c].type ==PL_DRAW)
        {
          LINE_DEBUG_OUT ("PL DRAW \r\n    %s\r\n", ((dpl[c].base != 0)?dpl[c].base->debug:""));
        }

        if (dpl[c].flags & PL_CALIBRATE_0)
        {
          LINE_DEBUG_OUT ("    PL D CALIB 0\r\n");
          SET (VIA_port_b, 0x81);
          DELAY_PORT_B_BEFORE_PORT_A();
          SET (VIA_port_a, 0x00);
          DELAY_CYCLES(2);
          SET (VIA_port_b, 0x82);    // mux=1, enable mux - integrator offset = 0
          DELAY_CYCLES(2);
          SET (VIA_port_b, 0x81);
          DELAY_CYCLES(2);
          delayed+=14;
        }
        else if (dpl[c].flags & PL_CALIBRATE)
        {
          LINE_DEBUG_OUT ("    PL D CALIB \r\n");
          v_resetIntegratorOffsets();
          delayed+=24;
        }

        int afterYDelay = 0;
        if (dpl[c].flags & PL_Y_MUST_BE_SET)
        {
          LINE_DEBUG_OUT("     Y MUST BE SET\r\n");
          afterYDelay = 2; // cranky dependend
          if (crankyFlag & CRANKY_BETWEEN_VIA_B)
          {
              afterYDelay += crankyFlag&0x0f;
          }

          delayed+=2;

          if (dpl[c].flags & PL_Y_A_MUST_BE_SET)
          {
            LINE_DEBUG_OUT("     YA MUST BE SET\r\n");
            if (dpl[c].flags & PL_MUX_Y_MUST_BE_SET)
            {
              LINE_DEBUG_OUT("     YMUX MUST BE SET\r\n");
              SET(VIA_port_b, 0x80);
              DELAY_PORT_B_BEFORE_PORT_A();
              SET(VIA_port_a, dpl[c].y);
              delayed+=2+DELAY_PORT_B_BEFORE_PORT_A_VALUE;
            }
            else
            {
              SET(VIA_port_a, dpl[c].y);
            }
          }
          else
          {
            if (dpl[c].flags & PL_MUX_Y_MUST_BE_SET)
            {
              SET(VIA_port_b, 0x80); // MUX to y integrator
            }
          }
        }
        else
          LINE_DEBUG_OUT("     Y NEED NOT BE SET\r\n");


        if (dpl[c].flags & PL_DEACTIVATE_ZERO)
        {
            // attention!
            // UNZERO is also a BEAM_OFF!
          UNZERO();
          delayed+=2;
          afterYDelay -= 2;
        }

        if (dpl[c].flags & PL_Y_DELAY_TO_NULL)
        {
            if (crankyFlag & CRANKY_NULLING_WAIT)
            {
                // some crankies need additional waits here!
                afterYDelay += CRANKY_DELAY_Y_TO_NULL_VALUE;
            }

        }

        // after the last set Y value
        // We have to wait for "afterYDelay" cycles
        // so Y can settly

        // for the beam to be switched off from last drawing we have
        // to wait alltogether for "DELAY_AFTER_T1_END_VALUE" cycles
        // now we do the wait / wait we still need
        if (afterYDelay<0) afterYDelay=0;
        if (delayedBeamOff==3)
        {
          int toDelayOff = DELAY_AFTER_T1_END_VALUE-delayed;
          if (dpl[c].type == PL_DRAW) toDelayOff-=5; // consecutive draws wait less ;-)

          if (toDelayOff >= afterYDelay)
          {
            // todo
            // theoretically we could check the difference
            // do a part delay here
            // and set the X value as a "delayer"

            if (toDelayOff>0)
              DELAY_CYCLES(toDelayOff);
            SWITCH_BEAM_OFF();
          }
          else
          {
            if (toDelayOff>0)
              DELAY_CYCLES(toDelayOff);
            else
              toDelayOff = 0;
            SWITCH_BEAM_OFF();
            DELAY_CYCLES(afterYDelay - toDelayOff);
          }
        }
        else
        {
          DELAY_CYCLES(afterYDelay); // EQ DELAY_AFTER_YSH_VALUE this is cranky dependend!
        }

        // not checking - since we also do ramp with B
        // if (dpl[c].flags & PL_MUX_X_MUST_BE_SET;
        SET(VIA_port_b, 0x81);
        if (dpl[c].flags & PL_X_A_MUST_BE_SET)
        {
          // to test only if cranky?dpl[c].x
          DELAY_PORT_B_BEFORE_PORT_A();
          SET(VIA_port_a, dpl[c].x);
        }
        // sync()
        setMarkStart();
        // start T1 timer
        if (dpl[c].flags & PL_T1_LO_EQUALS)
        {
          SET(VIA_t1_cnt_hi, (dpl[c].this_timing)>>8);
        }
        else
        {
          SETW_inverse(VIA_t1, dpl[c].this_timing);  /* scale due to "enlargement" is 16 bit! */
        }



        if (dpl[c].type ==PL_DRAW)
        {
            LINE_DEBUG_OUT ("     %i, %i :%i \r\n", dpl[c].x*dpl[c].this_timing, dpl[c].y*dpl[c].this_timing, dpl[c].this_timing);
            if (dpl[c].flags & PL_SWITCH_BEAM_ON)
            {
              SWITCH_BEAM_ON();
            }
        }
        else if (dpl[c].type == PL_DRAW_PATTERN)// this must be  (dpl[c].type == PL_DRAW_PATTERN))
        {
            LINE_DEBUG_OUT ("     %i, %i :%i - $%02x\r\n", dpl[c].x*dpl[c].this_timing, dpl[c].y*dpl[c].this_timing, dpl[c].this_timing, dpl[c].pattern);
            int patternAnds[] = {128,64,32,16,8,4,2,1};
            int pCount = 0;
            while ((GET (VIA_int_flags) & 0x40) == 0)
            {
#ifdef BEAM_LIGHT_BY_CNTL
                if (dpl[c].pattern & patternAnds[pCount])
                    SWITCH_BEAM_ON();
                else
                    SWITCH_BEAM_OFF();
                pCount=pCount+1;
                if (pCount==8) pCount=0;
#endif
#ifdef BEAM_LIGHT_BY_SHIFT
                if (pCount==0)
                {
                    SET_SHIFT_REG(dpl[c].pattern);
                    pCount=18;
                }
                else
                    pCount-=2;
#endif
            }
            int delayT1 = DELAY_AFTER_T1_END_VALUE;
            while (delayT1>0)
            {
#ifdef BEAM_LIGHT_BY_CNTL
                if (dpl[c].pattern & patternAnds[pCount])
                    SWITCH_BEAM_ON();
                else
                    SWITCH_BEAM_OFF();
                pCount=pCount+1;
                if (pCount==8) pCount=0;
#endif
#ifdef BEAM_LIGHT_BY_SHIFT
                if (pCount==0)
                {
                    SET_SHIFT_REG(dpl[c].pattern);
                    pCount=18;
                }
                else
                    pCount-=2;
#endif
                delayT1-=2;
            }
            SWITCH_BEAM_OFF();
            if ((browseMode) && (dpl[c].type == PL_DRAW_PATTERN))
            {
              if (lineNo==currentBrowsline)
                currentDisplayedBrowseLine = currentBrowsline;
              lineNo++;
            }
        }
        break;
      }
      default:
        break;
    }
    delayedBeamOff = 0;
    c++;
    if (myDebug)   printf("handled %i display elements...!\r\n", c);

    if (dpl[c].flags & PL_LAST_MUST_FINISH )
    {
      if (dpl[c].flags & PL_LAST_IS_RAMPING)
      {
        while ((GET (VIA_int_flags) & 0x40) == 0);

        if ((dpl[c].flags & PL_SWITCH_BEAM_OFF)/* && (dpl[c].type != PL_ZERO)*/)
        {
          // is drawing
          if ((dpl[c].type == PL_MOVE) || (dpl[c].type == PL_DRAW))
          {
            delayedBeamOff = 1;
          }
          else
          {
            DELAY_T1_OFF();
          }
        }
        else
        {
          // is Moving
          // finish move - is not as bad is BEAM
          //          DELAY_T1_OFF();
          DELAY_CYCLES(8);
          if (dpl[c].type == PL_DRAW_DOT)
          {
            // since we switch the light DIRECTLY ON...
            // better wait some cycles more!
            DELAY_CYCLES(4);
          }
        }
      }
      else
      {
        // e.g. zeroing
        // dots
        DELAY_CYCLES(dpl[c].last_timing);
      }
      if ((browseMode) && ((dpl[c-1].type ==PL_DRAW)||(dpl[c-1].type ==PL_DRAW_DOT)||(dpl[c-1].type ==PL_DRAW_PATTERN)   ))
      {
        if (lineNo==currentBrowsline)
          currentDisplayedBrowseLine = currentBrowsline;
        lineNo++;
      }
    }

    if ((dpl[c].flags & PL_SWITCH_BEAM_OFF)/* && (dpl[c].type != PL_ZERO)*/)
    {
      if (delayedBeamOff == 0)
      {
        SWITCH_BEAM_OFF();
      }
      else
      {
        delayedBeamOff+=2;
      }

    }
  }

  // safety only
  SWITCH_BEAM_OFF();
  ZERO_AND_CONTINUE();
  
#ifdef AVOID_TICKS
//	printf("clk: %u | comp0: %u, comp2: %u, comp3: %u\nGap0: %u, Gap2: %u, Gap3: %u\n"
//	 ,clk,comp0,comp2,comp3,gap0,gap2,gap3);

	/* Re-enable interrupts with the previous settings */
	if (bcm2835_int != MAP_FAILED)
	{
	 paddr = bcm2835_int + BCM2835_INT_GPU_IER1/4;
	 bcm2835_peri_write(paddr,gpu_ier1);
	 paddr = bcm2835_int + BCM2835_INT_GPU_IER2/4;
	 bcm2835_peri_write(paddr,gpu_ier2);
	 paddr = bcm2835_int + BCM2835_INT_CPU_IER/4;
	 bcm2835_peri_write(paddr,cpu_ier);
	 
#ifdef DISABLE_FIQ
	 /* Enable Fast Interrupt Requests: */
	 paddr = bcm2835_int + BCM2835_INT_FIQ/4;
	 fiqtemp = bcm2835_peri_read(paddr); // read FIQ control register 0x20C
	 fiqtemp |= (1 << 7);               // set FIQ enable bit
	 bcm2835_peri_write(paddr,fiqtemp);// write back to register
#endif
//	 printf("GPU IER1: 0x%X | GPU IER2: 0x%X | CPU IER: 0x%X\n",gpu_ier1,gpu_ier2,cpu_ier);
	}
	scaleTotal = 0;
#endif
}


//  0x50, 0x09
unsigned char uniDirectional[] =
{
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000101, 0b00000000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00001111, 0b11100000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00111111, 0b11110000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000001, 0b10000000, 0b01111111, 0b11111000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000011, 0b11111001, 0b11111111, 0b11111100, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000111, 0b11111111, 0b11111111, 0b11111110, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00001111, 0b11111111, 0b11111111, 0b11111111, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00001111, 0b11111111, 0b11111111, 0b11111111, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00011111, 0b11111111, 0b11111111, 0b11111111, 0b10000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00011111, 0b11111111, 0b11111111, 0b11111111, 0b11000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111111, 0b11111111, 0b11111111, 0b11111111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111111, 0b11111111, 0b11111111, 0b11111111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111111, 0b11111111, 0b11111111, 0b11111111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b01111111, 0b11111111, 0b11111111, 0b11111111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b01111111, 0b11111111, 0b11111111, 0b11111111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b01111111, 0b11111111, 0b11111111, 0b11111111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111111, 0b11111111, 0b11111111, 0b11111111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111111, 0b11111111, 0b11111111, 0b11111111, 0b11110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111111, 0b11111111, 0b11111111, 0b11111111, 0b11110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111111, 0b11111111, 0b11111111, 0b11111111, 0b11110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00011111, 0b11111111, 0b11111111, 0b11111111, 0b11111000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00011111, 0b11111111, 0b11111111, 0b11111111, 0b11111000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00011111, 0b11111111, 0b11111111, 0b11110111, 0b11111000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111111, 0b11111111, 0b11111111, 0b00000000, 0b10111000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00001111, 0b11111111, 0b11111000, 0b00000000, 0b00111000, 0b11000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b01111111, 0b11111000, 0b00011111, 0b01111000, 0b11000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00111111, 0b11111000, 0b00000111, 0b11111000, 0b11000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111000, 0b00001111, 0b11110000, 0b00000001, 0b11111000, 0b11100000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00100000, 0b00000111, 0b11100010, 0b00010001, 0b11111101, 0b11100000, 0b00000000, //forward
  0b00000000, 0b00010000, 0b11000000, 0b01000011, 0b11000011, 0b00011111, 0b11111101, 0b11000000, 0b00000000, //forward
  0b00000000, 0b00010000, 0b11000100, 0b01001011, 0b11001111, 0b11111111, 0b11111101, 0b11000000, 0b00000000, //forward
  0b00000000, 0b00010000, 0b11100100, 0b11111001, 0b11011111, 0b11111111, 0b11111101, 0b11000000, 0b00000000, //forward
  0b00000000, 0b00001001, 0b11111111, 0b11111001, 0b11011111, 0b11111111, 0b11111001, 0b11000000, 0b00000000, //forward
  0b00000000, 0b00001001, 0b11111111, 0b11111101, 0b11111111, 0b11111111, 0b11111001, 0b11000000, 0b00000000, //forward
  0b00000000, 0b00001001, 0b11111111, 0b11111001, 0b11111111, 0b11111111, 0b11111001, 0b11000000, 0b00000000, //forward
  0b00000000, 0b00001001, 0b11111111, 0b11111001, 0b11001111, 0b11111111, 0b11110001, 0b11000000, 0b00000000, //forward
  0b00000000, 0b00001001, 0b11111111, 0b11111001, 0b11001111, 0b11111111, 0b11110001, 0b11000000, 0b00000000, //forward
  0b00000000, 0b00000001, 0b11111111, 0b11111001, 0b11101111, 0b11111111, 0b11110001, 0b10000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b11111111, 0b11111011, 0b11100011, 0b11111111, 0b11110011, 0b10000000, 0b00000000, //forward
  0b00000000, 0b00000100, 0b11111111, 0b11110011, 0b11110011, 0b11111111, 0b11110011, 0b10000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b11111111, 0b11100011, 0b11111001, 0b11111111, 0b11110011, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000010, 0b01111111, 0b11100111, 0b11111111, 0b11111111, 0b11110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000010, 0b01111111, 0b11101011, 0b11100011, 0b11111111, 0b11110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b01111111, 0b11110000, 0b11000111, 0b11111111, 0b11110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111111, 0b11111000, 0b00011111, 0b11111111, 0b11110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111111, 0b11111111, 0b11111111, 0b11111111, 0b11110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00111111, 0b11111111, 0b11111001, 0b10111111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00011111, 0b10000111, 0b11111101, 0b11011111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00011111, 0b10011111, 0b11111111, 0b11111111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00011111, 0b11111111, 0b11111110, 0b00111111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00011111, 0b11110111, 0b11100000, 0b00111111, 0b11000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00011111, 0b11000000, 0b00000001, 0b11111111, 0b10010000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00001111, 0b11100000, 0b00111111, 0b11111111, 0b10010000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000111, 0b11111111, 0b11111111, 0b11111111, 0b00110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000011, 0b11111111, 0b11101111, 0b11111110, 0b00110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000001, 0b11111100, 0b00000011, 0b11111110, 0b01110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b11111110, 0b00011111, 0b11111100, 0b01100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b01111111, 0b11111111, 0b11111000, 0b11110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b01111111, 0b11111111, 0b11110001, 0b11110000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00111111, 0b11111111, 0b11100011, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00011111, 0b11111111, 0b11000111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00001111, 0b11111111, 0b10001111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000011, 0b11111101, 0b00011111, 0b11100000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000000, 0b00000001, 0b11111000, 0b00111111, 0b11000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000001, 0b00000000, 0b00000000, 0b01111111, 0b11000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000001, 0b10000000, 0b00000001, 0b11111111, 0b10000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000001, 0b10000000, 0b00111111, 0b11111111, 0b10000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000001, 0b11000011, 0b11111111, 0b11111110, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000001, 0b11100001, 0b11111111, 0b11110000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000001, 0b11110001, 0b11111111, 0b11000000, 0b00000000, 0b00000000, 0b00000000, //forward
  0b00000000, 0b00000000, 0b00000001, 0b11111100, 0b11111110, 0b00000000, 0b00000000, 0b00000000, 0b00000000, //forward
};

void v_directMove32n(int32_t xEnd, int32_t yEnd)
{
  v_zeroWait();
  int32_t x = xEnd*sizeX+offsetX;
  int32_t y = yEnd*sizeY+offsetY;

  UNZERO(); // ensure vector beam can be moves
  SET_OPTIMAL_SCALE(x, y);
  SET_YSH16(y);
  SET_XSH16(x);
  START_T1_TIMER();
  consecutiveDraws=0;
  currentCursorX = x;
  currentCursorY = y;
  WAIT_T1_END();
}

// 8 bit for now
void v_printBitmapUni(unsigned char *bitmapBlob, int width, int height, int size, int x, int y)
{
  v_readButtons();
  bitmapBlob=uniDirectional;
  width = 0x09;
  height = 0x50;
  x = -(9*4);
  y = 0x28;
  
	int patternAnds[] = {128,64,32,16,8,4,2,1};
	v_setBrightness(64);
	// uni directional
    
	for (int yy=0;yy<height;yy++)
	{
    currentYSH=currentPortA=0x100;

      v_directMove32n(x*128,(y-yy-yy)*128);
////////////////
// Prepare line print


        int afterYDelay = 6; // cranky dependend
		if (crankyFlag & CRANKY_BETWEEN_VIA_B)
		{
			afterYDelay += crankyFlag&0x0f;
		}

		SET(VIA_port_b, 0x80);
		DELAY_PORT_B_BEFORE_PORT_A();
		SET(VIA_port_a, 0);

//        UNZERO();
        afterYDelay -= 2;

		if (crankyFlag & CRANKY_NULLING_WAIT)
		{
			// some crankies need additional waits here!
			afterYDelay += CRANKY_DELAY_Y_TO_NULL_VALUE;
		}
		if (afterYDelay>0)
          DELAY_CYCLES(afterYDelay);

        SET(VIA_port_b, 0x81);
        DELAY_PORT_B_BEFORE_PORT_A();
        SET(VIA_port_a, 120);
////////////////
        // prepare for raster output

        SET(VIA_aux_cntl, 0x00);
//DELAY_CYCLES(1);
		// now print one line pattern

		SET(VIA_port_b, 0x01); // enable ramp, mux = y integrator, disable mux
		for (int xx=0;xx<width;xx++)
		{
			for (int bit=0;bit<8;bit++)
			{
				// output one raster dot - or not
                if (*bitmapBlob & patternAnds[bit])
                {
                  vectrexwrite_short(VIA_cntl, 0xee);
//                    SWITCH_BEAM_ON();
                }
                else
                {
                  vectrexwrite_short(VIA_cntl, 0xce);
//                    SWITCH_BEAM_OFF();
                }
			}
			bitmapBlob++;
		}
		SET(VIA_port_b, 0x81); // disable ramp, mux = y integrator, disable mux
		// assume lightning is done CNTL
		SET (VIA_aux_cntl, 0x80); // Shift reg mode = 000 free disable, T1 PB7 enabled
	}
}

// TODO
// increase STO with higher scales slightly

// y to 0 can be better
// first drawn on small scales could be sooner

// in calibration put in a cranky checker
// sms value to be adjusted, till it still looks good!

// OR

// test for cranky values and THAN internally fix to values that are OK!
// even with SMS = 127, only 1/80 of all vectors fall into the real cranky category!!!

/*
1) Config Window for the three items - using Bresenham
2) PiTrex - Raspbian
3) possibly hint input, scrolling thrus lists out of values (Terminal mode only)
4) configure dot dwell
-
Possible dotted lists!
*/
