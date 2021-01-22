#pragma once

#include <stddef.h>
#include <stdint.h>

void v_init(void); // also mounts sd card
void v_init_(int i); // also mounts sd card
void v_delayCycles(uint32_t cycles);
void v_delayCyclesQuarter(void);
void v_delayCyclesEighth(void);
void delayNano(uint32_t n);
void setMarkStart(void);
void waitMarkEnd(uint32_t offsetMicro);
void waitMarkEndMark(uint32_t offsetMicro);
void waitCycleMarkEnd(uint32_t cycles);
int waitFullMicro(void);
void measureTime(void); // debug and measure only

void v_setBrightness(uint8_t brightness);
void v_directDeltaMove32start(int32_t xLen, int32_t yLen);
void v_directDeltaMoveEnd(void);
void v_directMove32(int32_t xEnd, int32_t yEnd);
void v_directDraw32(int32_t xStart, int32_t yStart, int32_t xEnd, int32_t yEnd, uint8_t brightness);
void v_directDraw32Patterned(int32_t xStart, int32_t yStart, int32_t xEnd, int32_t yEnd, uint8_t brightness, uint8_t pattern); // only pipelined!
void v_directDraw32Hinted(int32_t xStart, int32_t yStart, int32_t xEnd, int32_t yEnd, uint8_t brightness, int forced);
void v_directDraw32HintedDebug(int32_t xStart, int32_t yStart, int32_t xEnd, int32_t yEnd, uint8_t brightness, int forced, char* debugInfo);
void v_directDeltaDraw32(int32_t xLen, int32_t yLen, uint8_t brightness);


void v_zeroWait(void);
void v_deflok(void);
void v_setRefresh(int hz);
void v_WaitRecal(void);
void v_resetDetection(void);
void v_calibrate(void);
uint8_t v_readButtons(void);
void v_readJoystick1Digital(void);
void v_readJoystick1Analog(void);
void v_printString(int8_t x, int8_t y, char* string, uint8_t textSize, uint8_t brightness);
int v_printStringRaster(int8_t x, int8_t y, char* _string, int8_t xSize, int8_t ySize, unsigned char delimiter); // returns 6809 cycles used
void v_set_font(unsigned char **userfont); // set the font for v_printStringRaster
// Parameter is a pointer to an array of 7 elements, each of which is a pointer to an array of 96 bytes
// A null parameter resets to the system font, which is only 64 characters and not the same layout
// as 7-bit ascii.


void v_setScale(uint16_t s);
void v_moveToImmediate8(int8_t xLen, int8_t yLen);
void v_drawToImmediate8(int8_t xLen, int8_t yLen);

void v_setName(char *name);
int v_loadSettings(char *name, unsigned char *blob, int blobSize);
int v_saveSettings(char *name, unsigned char *blob, int blobSize);

uint32_t v_millis(void); // only using low counters!
uint32_t v_micros(void); // only using low counters!

extern int (*executeDebugger)(int);

////////////////////////////////////////////////////////////////////////////
// Linux Interrupt stuff (Kevin Koster)
////////////////////////////////////////////////////////////////////////////

#ifdef AVOID_TICKS
#define BCM2835_ST_COMP(n)  (0x0c + (n) * 4)
#define SCALETOTAL_OFFSET	15 /* approximate number of vectrex cycles required for VIA writes/delays before/after each draw operation */
#define ST_GAP_END 4  /* Extra time to allow before next Linux interrupt after drawing finished (in Vectrex cycles) */
#define ST_GAP_TIMEOUT 5000 /* maximum delay waiting for gap, in microseconds - this the time after which a delay may be noticable */
#define BCM2835_INT_BASE  0xb000
#define BCM2835_INT_GPU_IER1  0x210
#define BCM2835_INT_GPU_IER2  0x214
#define BCM2835_INT_CPU_IER   0x218
#define BCM2835_INT_GPU_IDR1  0x21C
#define BCM2835_INT_GPU_IDR2  0x220
#define BCM2835_INT_CPU_IDR   0x224
#define BCM2835_INT_FIQ       0x20C
#endif

/* These are useful if programs want to do real-time reads/writes directly
 * to the VIA via pitrexio-gpio intstead of via the Vectrex Interface
 * library functions. eg. VecX.direct.
 */
void disableLinuxInterrupts(unsigned int minOffset);
void enableLinuxInterrupts();

////////////////////////////////////////////////////////////////////////////
// sound stuff
////////////////////////////////////////////////////////////////////////////
typedef enum {
	NOT_PLAYING = 0,		///<
	PLAY_END,		///<
	PLAY_LOOP		///<
} SfxMode;

void v_noSound(void);
void v_playDirectSampleAll(char *ymBufferLoad, int fsize, int rate);
void v_doSound(void);
void v_playSFXCont(unsigned char *buffer, int channel, int loop);
void v_playSFXStop(unsigned char *buffer, int channel);
void v_playSFXStart(unsigned char *buffer, int channel, int loop);

void v_playAllSFX(void);
void v_initYM(unsigned char *ymBuffer, uint16_t length, int loop);
int v_playYM(void);

void v_writePSG_double_buffered(uint8_t reg, uint8_t data);
void v_writePSG_buffered(uint8_t reg, uint8_t data);
void v_writePSG(uint8_t reg, uint8_t data);
uint8_t v_readPSG_double_buffered(uint8_t reg);
uint8_t v_readPSG_buffered(uint8_t reg);
uint8_t v_readPSG(uint8_t reg);
int play_sfx1(void);
int play_sfx2(void);
int play_sfx3(void);
void v_PSG_writeDoubleBuffer(void);


////////////////////////////////////////////////////////////////////////////
// debug stuff
////////////////////////////////////////////////////////////////////////////

typedef struct {
	int id;
	char *command;
	char *commandShort;
	char *commandHelp;
	void (*commandHandler)(void);
} Command;
extern Command *userCommandList;
extern int browseMode;
extern int currentBrowsline;
extern int currentDisplayedBrowseLine;

void v_error(char *message); // halts the program and displays an the message on the vectrex!
void v_errori(char *message, int i);
char *getParameter(int p);

extern int32_t currentCursorX; // 32 bit positioning value X, "normal" values are 16 bit,
						// but enlarging the screen may leed to higher values
extern int32_t currentCursorY; // 32 bit positioning value Y
extern uint16_t currentScale; // currently active scale factor (T1 timer in VIA)
extern uint16_t lastScale; // last active scale factor (T1 timer in VIA)
extern int16_t consecutiveDraws; // how many lines/moves were made after another!
extern uint8_t currentButtonState;
extern int8_t currentJoy1X;
extern int8_t currentJoy1Y;
extern int8_t currentJoy2X;
extern int8_t currentJoy2Y;
extern int16_t currentPortA; // == portA (also X SH)
extern int16_t currentYSH; // Y SH
extern int16_t currentZSH; // Brightness
extern unsigned int cycleEquivalent;
extern int v_dotDwell;

// todo
typedef enum {
  CRANKY_CRANKY_VIA_B_DELAY_B0 = 0,               ///<
  CRANKY_CRANKY_VIA_B_DELAY_B1 = 0,               ///<
  CRANKY_CRANKY_VIA_B_DELAY_B2 = 0,               ///<
  CRANKY_CRANKY_VIA_B_DELAY_B3 = 0,               ///<
  CRANKY_BETWEEN_VIA_B = 16,               ///< uses above delay
  CRANKY_NULLING_WAIT = 32,               ///< uses fixed "6" cycles
  CRANKY_NULLING_CALIBRATE = 64,               ///< // does a calibrate after each "null" of Y
  CRANKY_T1_DELAYED = 128               ///< // not implemented
} CrankyFlags;
extern CrankyFlags crankyFlag;

extern unsigned int Vec_Rfrsh;//0x3075;//12405; // 30000 = $7530 little endian = 3075 = 12405 50U;	// PLACEHOLDER FOR NOW -- FIX!
extern int Vec_Loop_Count;

extern uint16_t SCALE_STRENGTH_DIF;
extern unsigned int MAX_USED_STRENGTH;
extern unsigned int MAX_CONSECUTIVE_DRAWS;
extern unsigned int DELAY_ZERO_VALUE; // 70 // probably less, this can be adjusted, by max x position, the nearer to the center the less waits
extern unsigned int DELAY_AFTER_T1_END_VALUE;
extern int beamOffBetweenConsecutiveDraws;
extern unsigned int resetToZeroDifMax;

extern int bufferType; // 0 = none, 1 = double buffer, 2 = auto buffer (if pipeline is empty -> use previous (battlezone!)


#define VIA_port_b    ((unsigned short) 0xD000)	// VIA port B data I/O register
#define VIA_port_a    ((unsigned short) 0xD001)	// VIA port A data I/O register (handshaking)
#define VIA_DDR_b     ((unsigned short) 0xD002)	// VIA port B data direction register (0=input 1=output)
#define VIA_DDR_a     ((unsigned short) 0xD003)	// VIA port A data direction register (0=input 1=output)
#define VIA_t1        ((unsigned short) 0xD004)	// VIA timer 1 count
#define VIA_t1_cnt_lo ((unsigned short) 0xD004)	// VIA timer 1 count register lo (scale
#define VIA_t1_cnt_hi ((unsigned short) 0xD005)	// VIA timer 1 count register hi
#define VIA_t2        ((unsigned short) 0xD008)	// VIA timer 2 count register BOTH BYTES
#define VIA_t2_cnt_lo ((unsigned short) 0xD008)	// VIA timer 2 count register lo
#define VIA_t2_cnt_hi ((unsigned short) 0xD009)	// VIA timer 2 count register hi
#define VIA_shift_reg ((unsigned short) 0xD00A)	// VIA shift register
#define VIA_aux_cntl  ((unsigned short) 0xD00B)	// VIA auxiliary control
#define VIA_cntl      ((unsigned short) 0xD00C)	// VIA control register
#define VIA_int_flags ((unsigned short) 0xD00D)	// VIA interrupt flags register
#define VIA_int_enable ((unsigned short) 0xD00E)	// VIA interrupt enable register

#define MAX(A,B) ((A)>(B)?(A):(B))
#define ABS(A) ((A)>0?(A):(-(A)))

#define SETW_inverse(address, val) do {vectrexwrite((address), (val)&255);vectrexwrite((address)+1, ((val)>>8)&255); } while(0)
#define SETW(address, val) do {vectrexwrite(address, ((val)>>8)&255); vectrexwrite((address)+1, (val)&255);} while(0)
#define SET_WORD_REVERSE(adr, v1, v2) do {vectrexwrite((adr)+1, v2); CRANKY_DELAY_VIA_B; vectrexwrite(adr, v1); } while(0)
#define SET_WORD_ORDERED(adr, v1, v2) do {vectrexwrite((adr), v1); DELAY_CYCLES(DELAY_PORT_B_BEFORE_PORT_A_VALUE); vectrexwrite(adr+1, v2); } while(0)
#define SET(address, val) vectrexwrite(address, val)
#define GET(address) vectrexread(address)




/*

#define SETW_inverse(address, val) do {vectrexwrite_short((address), (val)&255);__sync_synchronize();vectrexwrite_short((address)+1, ((val)>>8)&255); } while(0)
#define SETW(address, val) do {vectrexwrite_short(address, ((val)>>8)&255); __sync_synchronize();vectrexwrite_short((address)+1, (val)&255);} while(0)
#define SET(address, val) vectrexwrite_short(address, val)
#define GET(address) vectrexread_short(address)
*/



#define V_BUTTONS_READ 1
#define V_JOY_DIGITAL_READ 2
#define V_JOY_ANALOG_READ 4


// strength needed for NON pipeline!
//#define SCALE_STRENGTH_DIF 4//2//4 //
#define MINSCALE 5 //15 to low might interfere with switching shift states to soon -> shift stalling
#define POSITION_MARGIN 100 // margin from where two positions are taken as equal 16bit
#define POSTION_MARGIN_AND 0x0//0x7f//0x3f // 64


// delay value in vectrex cycles
#ifdef FREESTANDING
  #define DELAY_NANO(n) delayNano((n))
  #define DELAY_AFTER_XSH_VALUE 0//0 // probably 0
  #define DELAY_AFTER_YSH_VALUE 8//6 might work too
  #define DELAY_PORT_B_BEFORE_PORT_A_VALUE 2 // not sure is this is a thing
  #ifdef MHZ1000
    #define DELAY_PI_CYCLE_EQUIVALENT 940//940 // nano seconds
  #else
    #define DELAY_PI_CYCLE_EQUIVALENT 660 // nano seconds
  #endif
#else
  #define DELAY_AFTER_XSH_VALUE 0 // probably 0
  #define DELAY_AFTER_YSH_VALUE 8 //4 //
  #define DELAY_PORT_B_BEFORE_PORT_A_VALUE 2 // not sure is this is a thing
  #define DELAY_PI_CYCLE_EQUIVALENT 940 //40 //930
#endif

/* possibly add:
  DELAY_AFTER_T1_START_VALUE 2 // (my personal stupid vectrex)
  DELAY_AFTER_T1_SET_VALUE 2 // (wait after scale is set - if started to fast??? dunno, is that a thing?)
*/

#define SET_SHIFT_REG(v) SET(VIA_shift_reg, (v))

// usually the beam (~BLANK) is switched using the SHIFT register
// but using other VIA settings the BEAM can also be switched on/off using the
// CNTL register
//#define BEAM_LIGHT_BY_SHIFT 1 //
#define BEAM_LIGHT_BY_CNTL

#ifdef BEAM_LIGHT_BY_SHIFT
#define SWITCH_BEAM_ON() SET_SHIFT_REG(0xff)
#define SWITCH_BEAM_OFF() SET_SHIFT_REG(0x00)
#endif
#ifdef BEAM_LIGHT_BY_CNTL
#define SWITCH_BEAM_ON() SET(VIA_cntl, 0xee)
#define SWITCH_BEAM_OFF() SET(VIA_cntl, 0xce)
#endif

#define CRANKY_DELAY_Y_TO_NULL_VALUE 6
#define CRANKY_DELAY_Y_TO_NULL do{if (crankyFlag & CRANKY_NULLING_WAIT) {DELAY_CYCLES(CRANKY_DELAY_Y_TO_NULL_VALUE);}}while (0)
#define CRANKY_DELAY_VIA_B     do{if (crankyFlag & CRANKY_BETWEEN_VIA_B){DELAY_CYCLES(crankyFlag&0x07);}} while (0) // cranky should be checked during calibration!

#define DELAY_CYCLES_QUARTER() v_delayCyclesQuarter()
#define DELAY_CYCLES_EIGHTH() v_delayCyclesEighth()
#define DELAY_CYCLES(cycles) do{if (cycles >1) v_delayCycles(cycles-1);v_delayCyclesQuarter();} while (0)
#define DELAY_PORT_B_BEFORE_PORT_A() do{DELAY_CYCLES(DELAY_PORT_B_BEFORE_PORT_A_VALUE);} while (0)// not sure is this is a thing
#define DELAY_T1_OFF() DELAY_CYCLES(DELAY_AFTER_T1_END_VALUE)
#define DELAY_XSH() DELAY_CYCLES(DELAY_AFTER_XSH_VALUE)
#define DELAY_ZSH() DELAY_YSH()
#define DELAY_YSH()  \
	do{DELAY_CYCLES(DELAY_AFTER_YSH_VALUE); \
	CRANKY_DELAY_VIA_B;} while (0)

#ifdef FREESTANDING
#define START_T1_TIMER()  \
do { \
	setMarkStart(); \
	if (lastScale == currentScale) \
	{ \
 		SET(VIA_t1_cnt_hi, currentScale>>8);  \
	} \
	else \
	{ \
 		SETW_inverse(VIA_t1, currentScale);  /* scale due to "enlargement" is 16 bit! */ \
	}  \
} while (0)
#define WAIT_T1_END_LAST() do{waitCycleMarkEnd(lastScale); DELAY_T1_OFF();} while (0)
#define WAIT_T1_END() do{waitCycleMarkEnd(currentScale); DELAY_T1_OFF();} while (0)
#else
#define START_T1_TIMER()  \
do { \
	if (lastScale == currentScale) \
	{ \
 		SET(VIA_t1_cnt_hi, currentScale>>8);  \
	} \
	else \
	{ \
 		SETW_inverse(VIA_t1, currentScale);  /* scale due to "enlargement" is 16 bit! */ \
	}  \
} while (0)
#define WAIT_T1_END() do{while ((GET (VIA_int_flags) & 0x40) == 0); DELAY_T1_OFF();} while (0)
#define WAIT_T1_END_LAST WAIT_T1_END
#endif

#define START_WAIT_T1() do{START_T1_TIMER();WAIT_T1_END();} while (0)


// chose a good scale for the max size given
// depending on "currentScale" (since it also needs time to switch scales)
// and on size, size should be so that
// it should maxVSizeWord/scale == very near to 127 (8 bit max)
//
// attention! for cranky vectrex large (SH values in Y) values (>100?) need additional waits
// when written to Port A of VIA
#define GET_OPTIMAL_SCALE(a,b) GET_OPTIMAL_SCALE_SMS(a,b,MAX_USED_STRENGTH)
#define SET_OPTIMAL_SCALE(a,b) \
	do {lastScale = currentScale;currentScale = GET_OPTIMAL_SCALE(a,b); \
	if (currentScale <= MINSCALE) currentScale = MINSCALE;\
	} while (0)
	
#define GET_OPTIMAL_SCALE_SMS(a,b,sms) ((MAX(ABS(a), ABS(b))+(sms-1))/sms)
//#define GET_OPTIMAL_SCALE_SMS(a,b,sms) ((MAX(ABS(a), ABS(b))+(MAX(ABS(a), ABS(b))-1))/sms)
#define SET_OPTIMAL_SCALE_SMS(a,b,sms) \
    do {lastScale = currentScale;currentScale = GET_OPTIMAL_SCALE_SMS(a,b, sms); \
    if (currentScale <= MINSCALE) currentScale = MINSCALE;\
    } while (0)

#define UNZERO() SET(VIA_cntl, 0xCE) // disable zeroing, otherwise no positioning can be done
#define ZERO_AND_CONTINUE() do{SET(VIA_cntl, 0xCC);currentCursorX = 0; currentCursorY = 0;} while (0)
#define ZERO_AND_WAIT() do{ \
	ZERO_AND_CONTINUE(); \
	DELAY_CYCLES(DELAY_ZERO_VALUE);} while (0)

#define SET_YSH_IMMEDIATE_8(v) do{ \
	SET(VIA_port_b, 0x80); \
	DELAY_PORT_B_BEFORE_PORT_A(); \
	currentYSH=currentPortA=(uint8_t)(v); \
	SET(VIA_port_a, currentPortA); \
	DELAY_YSH();} while (0)

#define SET_XSH_IMMEDIATE_8(v) do{ \
	SET(VIA_port_b, 0x81); \
	DELAY_PORT_B_BEFORE_PORT_A(); \
	currentPortA=(uint8_t)(v); \
	SET(VIA_port_a, currentPortA); \
	DELAY_XSH();} while (0)

// leaves with mux set to y integrator!
// sets 16 bit values divided by current scale!
// actually: due to possible display scaling, input values can now be larger 16bit
#define SET_YSH16(v) \
  do { \
	int16_t t = (uint8_t)((v)/(currentScale+SCALE_STRENGTH_DIF));  \
	if (t != currentYSH) \
	{ \
		if (t != currentPortA) \
		{ \
			SET(VIA_port_b, 0x80); \
			DELAY_PORT_B_BEFORE_PORT_A(); \
			currentPortA = currentYSH = t; \
			SET(VIA_port_a, currentPortA); \
		} \
		else \
		{ \
			SET(VIA_port_b, 0x80); \
			currentYSH = currentPortA; \
		} \
	} \
	DELAY_YSH(); \
} while (0)

// sets 16 bit values divided by current scale!
// actually: due to possible display scaling, input values can now be larger 16bit
// if no value set, this "at least" switches the MUX off
#define SET_XSH16(v) \
  do { \
	int16_t t = (uint8_t)((v)/(currentScale+SCALE_STRENGTH_DIF));  \
	if (t != currentPortA) \
	{ \
		SET(VIA_port_b, 0x81); \
		DELAY_PORT_B_BEFORE_PORT_A(); \
		currentPortA = t; \
		SET(VIA_port_a, currentPortA); \
	}  \
	else \
  { \
		SET(VIA_port_b, 0x81); \
  } \
	DELAY_XSH(); \
} while (0)


#define SET_YSH8(t) \
do { \
  if (t != currentYSH) \
  { \
  if (t != currentPortA) \
  { \
  SET(VIA_port_b, 0x80); \
  DELAY_PORT_B_BEFORE_PORT_A(); \
  currentPortA = currentYSH = t; \
  SET(VIA_port_a, currentPortA); \
  } \
  else \
  { \
  SET(VIA_port_b, 0x80); \
  currentYSH = currentPortA; \
  } \
  } \
  DELAY_YSH(); \
  } while (0)

#define SET_XSH8(t) \
do { \
  if (t != currentPortA) \
  { \
  SET(VIA_port_b, 0x81); \
  DELAY_PORT_B_BEFORE_PORT_A(); \
  currentPortA = t; \
  SET(VIA_port_a, currentPortA); \
  }  \
  else \
  { \
  SET(VIA_port_b, 0x81); \
  } \
  DELAY_XSH(); \
  } while (0)


// 4 cycles per loop?
// 1 cycle = 1 nano second
#define CYCLE_PER_LOOP 4
#define WAIT_CYCLE_NANO(n) do{ \
    uint32_t l = ((n)>>2)-((n)>>5)+((n)>>7)-((n)>>9); /* divided by 4 */  \
    if (!l) {l=1;} \
    else { l+=(n)>>7; \
    }asm volatile( "0:" "SUBS %[count], #2;" "SUBS %[count], #-1;" "BNE 0b;" :[count]"+r"(l) ); \
    } while(0)

// need an enclosing () below? - check usages first
#define GET_SYSTEM_TIMER_LO *(bcm2835_st + BCM2835_ST_CLO/4)
#define SET_TIMER_MARK do{timerMark = (volatile uint32_t)*(bcm2835_st + BCM2835_ST_CLO/4);} while(0)

// 32 bit timer, perhaps reset each timing?
// adjust overhead accordingly
// 1 tick = 0,001 us
// ticks * 1000 = 1000 us
#define WAIT_MICRO_TIME(utime) do{ \
       __sync_synchronize(); \
       uint32_t tstart = GET_SYSTEM_TIMER_LO; \
       uint32_t tend = tstart +(utime); \
       __sync_synchronize(); \
       while (GET_SYSTEM_TIMER_LO < tend);} while (0)

#define WAIT_MICRO_MARK_DELTA(utime)  do{ \\
       uint32_t tend = timerMark +(utime); \
       __sync_synchronize(); \
       while (GET_SYSTEM_TIMER_LO < tend);} while (0)


#define OFFSET_CYCLE_OVERHEAD 17// # of cycles "wasted" by function calling etc
#define OFFSET_TIMER_OVERHEAD 24// # of cycles "wasted" by function calling etc

// max 4000 micro
#define delayMicro(u) delayNano(((u)*1000))

// max 4 milli
#define delayMilli(u) delayNano(((u)*10000000))




typedef struct {
  int y0;
  int x0;
  int y1;
  int x1;
  int intensity;
  int pattern; // if 0, than treated as ff -> no pattern
  int sms;
  int timingForced;
  int force;
  char debug[240];
  void *next;
  void *previous;
} VectorPipelineBase;

typedef enum {
  PL_BASE_NO_FORCE = 0,
  PL_BASE_FORCE_ZERO = 1,
  PL_BASE_FORCE_NO_ZERO = 2,
  PL_BASE_FORCE_DEFLOK = 4,
  PL_BASE_FORCE_RESET_ZERO_REF = 8, 
  PL_BASE_FORCE_CALIBRATE_INTEGRATORS = 16, 
  PL_BASE_FORCE_STABLE = 32,               	// < does never "move" - should be at the start of  drawingsr
  PL_BASE_FORCE_EMPTY = 64,
  PL_BASE_FORCE_NOT_CLIPPED = 128,
  PL_BASE_FORCE_USE_FIX_SIZE = 256, // optimal scale is not calculated, the currently set timing is used!
  PL_BASE_FORCE_USE_DOT_DWELL = 512 // optimal scale is not calculated, the currently set timing is used!
} BaseActions;

typedef enum {
  PL_ZERO = 0,   
  PL_DEFLOK,
  PL_MOVE,             
  PL_DRAW_PATTERN,
  PL_DRAW,             
  PL_DRAW_DOT,
  PL_SET_BRIGHTNESS, 
  PL_END             
} VectorActions;

typedef enum {
  PL_DEACTIVATE_ZERO   = 1,               ///<
  PL_SWITCH_ZERO_AFTER = 2,
  PL_Y_MUST_BE_SET     = 4,   // when moving, Y integrator must be set
  PL_Y_A_MUST_BE_SET   = 8,  // when moving, Y integrator must be set and reg A must be loaded with y value
  PL_X_A_MUST_BE_SET   = 16,
  PL_I_A_MUST_BE_SET   = 32,  // when moving, Y integrator must be set and reg A must be loaded with y value
  PL_LAST_MUST_FINISH  = 64,
  PL_SWITCH_BEAM_ON    = 128,
  PL_SWITCH_BEAM_OFF   = 256,
  PL_SWITCH_BEAM_OFF_AFTER = 512,
  PL_MUX_Y_MUST_BE_SET = 1024,
  PL_MUX_X_MUST_BE_SET = 2048,
  PL_IS_RAMPING        = 4096,
  PL_LAST_IS_RAMPING   = 8192,
  PL_Y_DELAY_TO_NULL   = 8192*2,
  PL_T1_LO_EQUALS      = (8192*2)*2,
  PL_CALIBRATE_0       = (8192*2)*2*2,
  PL_CALIBRATE         = (8192*2)*2*2*2
} ActionFlags;

typedef struct {
  VectorActions type;
  int flags;
  int y;
  int x;
  int pattern; // if 0, than treated as ff -> no pattern
  int intensity;
  int this_timing; // scale or cycle delay
  int last_timing; // scale or cycle delay
  VectorPipelineBase *base;
} VectorPipeline;

#define MAX_PIPELINE 3000

VectorPipelineBase pb[MAX_PIPELINE];
int usePipeline;
int pipelineCounter;
VectorPipelineBase *cpb;
int pipelineAlt;

extern int commonHints;

#define GLOBAL_FLAG_IS_INIT 1  // bit 0

// 0x0000008c is a pointer to a structur of->
typedef struct {
  unsigned char flags;
  void (*loader)(void);
  unsigned char orientation;
  unsigned char lastSelection; // in loader menu
  char parameter1[16];
} GlobalMemSettings;

extern GlobalMemSettings *settings;
