#define _GNU_SOURCE

#ifndef FREESTANDING
#include <sched.h>
#include <string.h>
#endif

#include <stdio.h>
#include <stdlib.h>


#include <time.h>   // for clock

#include <vectrex/vectrexInterface.h>
#include <vectrex/baremetalUtil.h>
extern int optimizationON;

#include "externs.h"
#include "mdep.h"
#include "mdep.i"

// It would have been nice to set this up so that it compiles for OpenGL *only* if USE_VECTERM
// is not defined, but in my rush to get something working I'm afraid there's quite a bit of
// Vectrex stuff in here now that is not ifdef'ed so if anyone wants a non-vectrex build, either
// fix this or contact gtoal@gtoal.com for an alternative starting point.  At various times
// I've made versions of this code for SDL, Allegro, OpenGL, GLUT, MS Windows, and even raw
// framebuffers on handheld consoles.  (But I wouldn't like to swear I could easily find any
//specific one of those in a hurry...)

/* Current TO DO list...
     can I automatically detect switched vs analog joystick hardware?
     what about using the vectrex joystick in either analog or digital mode?
     need to detect timeouts and go to reinit code until vecterm restarts
      - do I use both controllers - one being digital and the other analog?
        or do I make that a startup option (to be remembered between games)
     Add a keypress way of exiting the game? (all 4 buttons down perhaps)
        maybe 4 keys pauses the game and takes you in to a setup menu which
        also allows for changing joystick mode.
     MUST save high score
     maybe add front end to capture username at end of a game, and keep
        a net-wide global high score?
     key sequence to exit back to vectrex???
     clean up ifdefs so that this code can still compile for Linux *without* vectrex support
     re-do the vecfever scaling so that I can address the entire vectrex screen properly (to
        overlay new features)
     cheat modes: 1) auto re-generating shields
                  2) auto-fire
                  3) auto-shields when ships are about to pass
 */



#ifdef OPENGL
#define COLORS 16
GLubyte colors[COLORS][3] = {
  {0,0,0},
  {1,1,1},
  {3,3,3},
  {7,7,7},
  {15,15,15},
  {31,31,31},
  {63,63,63},
  {127,127,127},
  {0+128,0+128,0+128},
  {1+128,1+128,1+128},
  {3+128,3+128,3+128},
  {7+128,7+128,7+128},
  {15+128,15+128,15+128},
  {31+128,31+128,31+128},
  {63+128,63+128,63+128},
  {127+128,127+128,127+128},
};
#endif

#ifndef FALSE
#define FALSE (0!=0)
#define TRUE (0==0)
#endif

#ifdef USE_VECTERM
#define SCREEN_H 1023
#else
#define SCREEN_H 800
#endif
#define SCREEN_W 1023

int plaqueCounter=0;
static int MinX = 1000, MinY = 1000, MaxX = 0, MaxY = 0;

static int debounce_oneshots = 0, debounce_shields = 0, debounce_coin = -1;
void startFrame()
{
    v_WaitRecal();
v_doSound();
    v_setBrightness(64);        /* set intensity of vector beam... */
    v_readButtons();
    v_readJoystick1Analog();
v_playAllSFX();
plaqueCounter = 0;
}

void CinemaClearScreen (void)
{
  static long Frames = 0L;
  int mx = 0, my = 0;
  int tmp_btns;
  static int alternating = 0;
  if (((alternating++)&1) == 1) {
  mousecode = 0;


  /*
     if (... fire button is pressed ...) fireflag &= ~IO_FIRE; else fireflag |= IO_FIRE;
     if (... shields button is pressed ...) shieldsflag &= ~IO_SHIELDS; else shieldsflag |= IO_SHIELDS;

     If the start button is pressed, set "startflag = 0;" here
     If the coin button is pressed, set
     ioSwitches &= ~SW_COIN; / * Clear coin counter the bodgy way * /
     coinflag = 0;
     if the shields button is pressed, set "shieldsflag = IO_SHIELDS;"

     HOWEVER if *nothing* is pressed,

     startflag = IO_START; / * Bodge.  Clear start button. Allows it to be down for 1/50sec * /

   */
  mx = currentJoy1X*5/2; // crude but the best I've found
  my = currentJoy1Y*5/2;
  // perhaps have a scheme where the right-hand joy port is analog and the left is digital?
  // check by seeing if the inputs do not ever change?
  ram[42] = mx;
  ram[43] = my;

  if (currentButtonState&1) startflag = 0; else startflag = IO_START;
  if (currentButtonState&4) shieldsflag &= ~IO_SHIELDS; else shieldsflag |= IO_SHIELDS;
  if (currentButtonState&8) fireflag &= ~IO_FIRE; else fireflag |= IO_FIRE;


  // unfortunately coining is happening on *release* of button 1 rather than on press of it.  Fix later.
  if ((currentButtonState&1) == 0)
  {
    if ((currentButtonState&1) != debounce_coin)
	{
      ioSwitches &= ~SW_COIN; coinflag = 0;
    }
	else
	{
      ioSwitches |= SW_COIN; coinflag = SW_COIN;  // only on down-press, for 1 frame
    }
  }
  else
  {
    ioSwitches |= SW_COIN; coinflag = SW_COIN;  // only on down-press, for 1 frame
  }
  debounce_coin = (currentButtonState&1);

  if (debounce_oneshots > 0) {
    debounce_oneshots -= 1;
    if (debounce_oneshots == 0) {
      startflag = IO_START; fireflag |= IO_FIRE;
    }
  }

  if (debounce_shields > 0) {
    debounce_shields -= 1;
    if (debounce_shields == 0) {
      shieldsflag |= IO_SHIELDS;
    }
  }

  ioInputs = (unsigned short) (mousecode | fireflag | shieldsflag | startflag);

  startFrame();
}
}


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

extern int myDebug;
extern int browseMode;

void CinemaVectorDataHinted (int FromX, int FromY, int ToX, int ToY, int vgColour, int hint)
{
  
  
  ToX = ToX << 16;
  ToX = ToX >> 16;
  FromX = FromX << 16;
  FromX = FromX >> 16;
  
  if (FromX < MinX)
    MinX = FromX;
  if (ToX < MinX)
    MinX = ToX;
  if (FromY < MinY)
    MinY = FromY;
  if (ToY < MinY)
    MinY = ToY;
  if (FromX > MaxX)
    MaxX = FromX;
  if (ToX > MaxX)
    MaxX = ToX;
  if (FromY > MaxY)
    MaxY = FromY;
  if (ToY > MaxY)
    MaxY = ToY;
  
  if (FromX==ToX && FromY==ToY)
  {
    int dist = isqrt((SCREEN_W/2.0 - FromX)*(SCREEN_W/2.0 - FromX) + (SCREEN_H/2.0 - FromY)*(SCREEN_H/2.0 - FromY));
    int maxdist = isqrt((SCREEN_W/2.0)*(SCREEN_W/2.0) + (SCREEN_H/2.0)*(SCREEN_H/2.0));
    if (vgColour == 7) vgColour = vgColour - (((maxdist-dist) * 7) / maxdist) + 1;
  }

  if ((myDebug)|| (browseMode))
  {
    char helperData[80];
    sprintf(helperData, "PC=%04x, I=%04x, J=%04x, P=%04x, T=%04x, A=%04x, B=%04x", register_PC, register_I, register_J, register_P, register_T, register_A, register_B);
    v_directDraw32HintedDebug((FromX - 512)*32, (FromY - 512)*32, (ToX- 512)*32,(ToY- 512)*32, vgColour * 8,hint, helperData);
  }
  else
    v_directDraw32Hinted((FromX - 512)*32, (FromY - 512)*32, (ToX- 512)*32,(ToY- 512)*32, vgColour * 8,hint);
}


void CinemaVectorDataP (int FromX, int FromY, int ToX, int ToY, int vgColour, int pattern)
{
ToX = ToX << 16;
ToX = ToX >> 16;
FromX = FromX << 16;
FromX = FromX >> 16;

  if (FromX < MinX)
    MinX = FromX;
  if (ToX < MinX)
    MinX = ToX;
  if (FromY < MinY)
    MinY = FromY;
  if (ToY < MinY)
    MinY = ToY;
  if (FromX > MaxX)
    MaxX = FromX;
  if (ToX > MaxX)
    MaxX = ToX;
  if (FromY > MaxY)
    MaxY = FromY;
  if (ToY > MaxY)
    MaxY = ToY;

  v_directDraw32Patterned((FromX - 512)*32, (FromY - 512)*32, (ToX- 512)*32,(ToY- 512)*32, vgColour * 8, pattern);
}

void CinemaVectorData (int FromX, int FromY, int ToX, int ToY, int vgColour)
{
  if ((register_PC >= 0x1440) && (register_PC < 0x1454)) {
    plaqueCounter++; // reset on startFrame
    int force = 0;
    if ( (plaqueCounter == 9) || // after retangles -> T
	 (plaqueCounter == 11) ||   // -> A
	 (plaqueCounter == 14) ||   // -> IL
	 (plaqueCounter == 17) ||   // -> R
	 (plaqueCounter == 23) ||   // -> E
	 (plaqueCounter == 27) ||   // -> N
	 (plaqueCounter == 30) ||   // -> N
	 (plaqueCounter == 33) ||   // -> U
	 (plaqueCounter == 36)      // -> G
	 )
    {
	force = PL_BASE_FORCE_ZERO | PL_BASE_FORCE_RESET_ZERO_REF;
    }
    /* render the line */
#ifndef DUALCPU
    CinemaVectorDataHinted (FromX, FromY, ToX, ToY, vgColour, force);
#endif
    return;
  }
  
ToX = ToX << 16;
ToX = ToX >> 16;
FromX = FromX << 16;
FromX = FromX >> 16;

  if (FromX < MinX)
    MinX = FromX;
  if (ToX < MinX)
    MinX = ToX;
  if (FromY < MinY)
    MinY = FromY;
  if (ToY < MinY)
    MinY = ToY;
  if (FromX > MaxX)
    MaxX = FromX;
  if (ToX > MaxX)
    MaxX = ToX;
  if (FromY > MaxY)
    MaxY = FromY;
  if (ToY > MaxY)
    MaxY = ToY;

  if (FromX==ToX && FromY==ToY)
  {
    int dist = isqrt((SCREEN_W/2.0 - FromX)*(SCREEN_W/2.0 - FromX) + (SCREEN_H/2.0 - FromY)*(SCREEN_H/2.0 - FromY));
    int maxdist = isqrt((SCREEN_W/2.0)*(SCREEN_W/2.0) + (SCREEN_H/2.0)*(SCREEN_H/2.0));
    if (vgColour == 7) vgColour = vgColour - (((maxdist-dist) * 7) / maxdist) + 1;
  }
  
  if (browseMode)
  {
    char helperData[80];
    sprintf(helperData, "PC=%04x, I=%04x, J=%04x, P=%04x, T=%04x, A=%04x, B=%04x", register_PC, register_I, register_J, register_P, register_T, register_A, register_B);
    v_directDraw32HintedDebug((FromX - 512)*32, (FromY - 512)*32, (ToX- 512)*32,(ToY- 512)*32, vgColour * 8,0, helperData);
  }
  else
    v_directDraw32((FromX - 512)*32, (FromY - 512)*32, (ToX- 512)*32,(ToY- 512)*32, vgColour * 8);
}

#define UNFINISHED(s)		/* Do Nothing */

/* Reset C-CPU registers, flags, etc to default starting values
 */

void
cineSetJMI (UINT8 j)
{
  ccpu_jmi_dip = j;
}

void
cineSetMSize (UINT8 m)
{
  ccpu_msize = m;

}

void
cineSetMonitor (UINT8 m)
{
  ccpu_monitor = m;
}

void
cineReleaseTimeslice (void)
{
  bBailOut = TRUE;
}

/* main interpreter body */
extern int v_debug;
extern void cineExecute (void);
/*
1101 0110 0xd6
1010 1101 0xad
0101 1011 0x5b
1011 0110 0xb6
0110 1101 0x6d
*/

int patternData[] = {0xd6, 0xad, 0x5b, 0xb6, 0x6d};

#define PATTERN_DELAY 1
int currentPattern = 0;
int patternDelay = PATTERN_DELAY;




#define TO_RIGHT 85

void DrawShields(void) {
  static const int shieldvec[] = {
     -72,24,
      24,24,
      96,-96,
      24,24,
      -72,72,
      -72,-72,
      24,-24,
      96,96,
      24,-24,
      -72,-72,
      -72,72,
  };
  

 v_debug = 1;
//printf("---------------\r\n");
 // for cranky vectrex
 int oldMAX_USED_STRENGTH = MAX_USED_STRENGTH;
 MAX_USED_STRENGTH = 40;
 commonHints = PL_BASE_FORCE_ZERO |PL_BASE_FORCE_RESET_ZERO_REF;
  int i = 0, x0=0, y0=0, x1, y1;
  x0 = shieldvec[0]; y0 = shieldvec[1];
  
  int pattern = random();//patternData[currentPattern];
  if (--patternDelay<0) 
  {
    patternDelay = PATTERN_DELAY;
    currentPattern++;
    if (currentPattern>4) currentPattern = 0;
  }
  
  for (i = 1; i < 11; i++) {
    x1 = x0 + shieldvec[i*2];
    y1 = y0 + shieldvec[i*2+1];
    CinemaVectorDataP (y0*8+450+TO_RIGHT, x0*6+450, y1*8+450+TO_RIGHT, x1*6+450,  15,pattern);
    x0 = x1; y0 = y1;
  }
  MAX_USED_STRENGTH = oldMAX_USED_STRENGTH;
 commonHints = 0;
  v_debug = 0;
}

void cineExecuteFrame (void)
{
    if (!ram[0x6c] && !ram[0x6d]) {
      // Preset high score to 1024.
      ram[0x6c] = 0x102;
      ram[0x6d] = 0x4;
    }

    if (0==1) {
      // Set score to 1024.
      ram[0x6a] = 0x102;
      ram[0x6b] = 0x4;
    }


    bNewFrame = 0;
    if (ram[129] > 0 && !shieldsflag && ram[0x69] > 0)
    { // 129 is shields left, 0x69 is ships still to pass,
     // ie don't draw shields after the game is over
      DrawShields();
    }
    do cineExecute(); while (bNewFrame != 1); // bNewFrame is set TWICE per frame by the translated code
                                              // hence why the refresh rate is set to 76 rather than 38.
}

void initTailGunner (void)
{
  int i;
  init_graph ();

  bNewFrame = 0;
  sdwGameXSize = SCREEN_W;
  sdwGameYSize = SCREEN_H;
  bFlipX = 0;
  bFlipY = 0;
  bSwapXY = 0;
  ioInputs = 0xffff;

  ioSwitches = 0xffff;		/* Tweaked  testing bottom bit = quarters/game */

  ioSwitches = (ioSwitches & (~SW_SHIELDMASK)) | SW_SHIELDS;	/* GT */

  ioSwitches &= (unsigned short) ~SW_QUARTERS_PER_GAME;	/* One quarter per game */
  ioSwitches |= (unsigned short) quarterflag;

  cineSetJMI (0);		/* Use external input */
  cineSetMSize (1);		/* 8K */
  cineSetMonitor (0);		/* bi-level */

  CinemaClearScreen ();		/* Initialise ioInputs etc before game starts (optional) */

  for (i = 0; i < 256; i++) ram[i] = 0;

}


/* #define SCREEN_SAVER_MODE */

void reshape(int width, int height)
{
}

void display(void)
{
    int i;
    static int color = 0;
    static int odd = 0;

    if (odd == 0) {
    }
}

void idle(void)
{
}

void bail(int code)
{
    ;//exit(code);
}

int startTailgunner()
{
    int i;


}
//#endif

int vectrexinit (char viaconfig);

#ifdef FREESTANDING
//#define SETTINGS_SIZE 1024
//unsigned char settingsBlob[SETTINGS_SIZE];



int main(int argc, char **argv)
{
  vectrexinit(1);
  optimizationON = 0; // don't optimise, ie reset to 0 for every line...
  initTailGunner();

  char * ppp = getLoadParameter();
  printf("Load Param = %s\r\n", ppp);
  
  //v_loadSettings("tailgunner", settingsBlob, SETTINGS_SIZE);
  v_setName("tailgunner_sbt");
  
  rom[0x03CD] = 0xff; // suppress the shield pattern - we'll redraw it ourselves...

  for (;;)
  {
    cineExecuteFrame ();
  }
  return 0;
}

#else
#ifdef PIZERO
/** Main function - we'll never return from here */
extern int bufferType; // 0 = none, 1 = double buffer, 2 = auto buffer (if pipeline is empty -> use previous) [this should be in header file]
int main(int argc, char **argv)
{
  vectrexinit(1);
  // v_init() is called in initGraph which is called from initTailGunner
  // - might be better to move that here?
  initTailGunner();
  v_setRefresh(76);
  usePipeline = 1; // translated code calls wait for refresh twice per frame in rapid succession
  bufferType = 2; // 0 = none, 1 = double buffer, 2 = auto buffer (if pipeline is empty -> use previous
  optimizationON = 0; // don't optimise, ie reset to 0 for every line... - game is more than fast enough to handle it
  v_setName("tailgunner_sbt");

  // need to add refresh rate and correct double buffering option!
  
  rom[0x03CD] = 0xff; // suppress the shield pattern - we'll redraw it ourselves...

  for (;;) {
    cineExecuteFrame ();
  }

  exit(0);
  return 0;
}

#else
#ifdef PI3 // NO LONGER USED but keep the code in case we try this again some time...
//int i;

extern void _enable_interrupts(void);
extern int startTailgunner();

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

/** Main function - we'll never return from here */
void main()
{
  static volatile uint32_t *regs = NULL;
  int   fd;
  cpu_set_t cpus;

  CPU_ZERO(&cpus);

  CPU_SET(3, &cpus);

  if (sched_setaffinity(0, sizeof(cpus), &cpus)) {
    perror("sched_setaffinity");
    exit(1);
  }

  if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0) exit(1);
  regs = (uint32_t *)mmap(0, 0x10000 /* BLOCK_SIZE */, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  if ((int32_t)regs == -1) exit(1);

  regs[0x4C/sizeof(uint32_t)] = 0x00000000; // TURN OFF IRQs

  vectrexinit(1);
  optimizationON = 0; // don't optimise, ie reset to 0 for every line...

  for (int i=0;i<8;i++)
  {
    sound_reg[i]=0;
    sound_reg_old[i]=0;
  }


    initTailGunner();
    // apply any rom hacking here

    // truncate some vector lists
    //rom[0x1A35] = 0xff; // suppress the boxes
    //rom[0x1A70] = 0xff; // suppress TAIL
    //rom[0x1AAC] = 0xff; // suppress NER
    //rom[0x1AE7] = 0xff; // suppress GUN
    rom[0x03CD] = 0xff; // suppress the shield pattern - we'll redraw it ourselves...


    while(1)
    {

        display();
    }
}
#else
#error You must define one of PIZERO or PI3, and possibly PIZERO and FREESTANDING
#endif // PI3
#endif
#endif

/*









		// *** load sound files

		// Sound effect numbers:
		// 0 = thrust
		// 1 = not used? (sample guessed)
		// 2 = shield
		// 3 = laser
		// 4 = line to the start button led (no sound)
		// 5 = explosion
		// 6 = warp out (at end of attack wave)
		// 7 = bounce (in reality on pin for value 4 on the LS259 on the sound board)

		loadSoundEffect(0, "thrust1.au", true);		//LS259: 1 001
		loadSoundEffect(1, "star.au", false);		//LS259: ? not used
		loadSoundEffect(2, "shield.au", true);		//LS259: 3 011
		loadSoundEffect(3, "slaser.au", false);		//LS259: 2 010
		loadSoundEffect(5, "sexplode.au", false);	//LS259: 0 000
		loadSoundEffect(6, "hypersp.au", false);	//LS259: 5 101
		loadSoundEffect(7, "bounce.au", false);		//LS259: 4 100  // nr. 7 is fake, sound_reg is 3, like shield

		soundCoin = getAudioClip(getCodeBase(), "coin.au");




38 Hz original

switch to

76 hz _> double buffer

gaol:
    each ship 1 draw
	each text 1 draw
	shield x draw
	packete x draw
	stars 1 draw
*/


// Sound effect numbers:
// 0 = thrust
// 1 = not used? (sample guessed)
// 2 = shield
// 3 = laser
// 4 = line to the start button led (no sound)
// 5 = explosion
// 6 = warp out (at end of attack wave)
// 7 = bounce (in reality on pin for value 4 on the LS259 on the sound board)
unsigned char TailGunnerSFX_111[] = // EXPLODE - todo
{    0x6F,  0xB0,  0x0A,  0x1C,  0x2F,  0x04,  0x08,  0x2F,  0x03,  0x06,
   0x2F,  0x83,  0x04,  0x2F,  0x63,  0x03,  0x2F,  0x8B,  0x02,  0x2F,
   0xE9,  0x01,  0x2F,  0x6F,  0x01,  0x6F,  0x58,  0x05,  0x1B,  0x2E,
   0x02,  0x04,  0x2D,  0x02,  0x03,  0x2C,  0x42,  0x02,  0x2E,  0x58,
   0x05,  0x2D,  0x02,  0x04,  0x2C,  0x02,  0x03,  0x2B,  0x42,  0x02,
   0x2D,  0x58,  0x05,  0x2C,  0x02,  0x04,  0x2B,  0x02,  0x03,  0x2A,
   0x42,  0x02,  0x2C,  0x58,  0x05,  0x2B,  0x02,  0x04,  0x2A,  0x02,
   0x03,  0x29,  0x42,  0x02,  0x2B,  0x58,  0x05,  0x2A,  0x02,  0x04,
   0x29,  0x02,  0x03,  0x28,  0x42,  0x02,  0x2A,  0x58,  0x05,  0x29,
   0x02,  0x04,  0x28,  0x02,  0x03,  0x27,  0x42,  0x02,  0x29,  0x58,
   0x05,  0x28,  0x02,  0x04,  0x27,  0x02,  0x03,  0x26,  0x42,  0x02,
   0x28,  0x58,  0x05,  0x27,  0x02,  0x04,  0x26,  0x02,  0x03,  0x25,
   0x42,  0x02,  0x60,  0x00,  0x00,  0x00,  0xD0,  0x20};
unsigned char TailGunnerSFX_110[] = // THRUST
{
     0xE6, 0xBC, 0x03, 0x00, 0x86, 0x03, 0x03, 0x01, 0x01, 0x86,
  0x86, 0x03, 0x03, 0x01, 0x01, 0xD0, 0x20
};
unsigned char TailGunnerSFX_101[] = // LASER
{
  0xEA, 0x25, 0x00, 0x00, 0xAB, 0x40, 0x00, 0xAA, 0x25, 0x00,
  0xAB, 0x40, 0x00, 0xA0, 0x00, 0x00, 0xAB, 0x5B, 0x00, 0xAB,
  0x80, 0x00, 0xA0, 0x00, 0x00, 0xAB, 0xB0, 0x00, 0xAB, 0xDB,
  0x00, 0xAC, 0x43, 0x01, 0xAC, 0x84, 0x01, 0xAB, 0x00, 0x02,
  0xAB, 0x00, 0x03, 0xAA, 0xB0, 0x00, 0xAA, 0xDB, 0x00, 0xAB,
  0x43, 0x01, 0xAB, 0x84, 0x01, 0xAA, 0x00, 0x02, 0xAA, 0x00,
  0x03, 0xA9, 0x05, 0x04, 0xA9, 0xB0, 0x00, 0xA9, 0xDB, 0x00,
  0xAA, 0x43, 0x01, 0xAA, 0x84, 0x01, 0xA9, 0x00, 0x02, 0xA9,
  0x00, 0x03, 0xA8, 0x05, 0x04, 0xA8, 0x50, 0x05, 0xA8, 0x01,
  0x00, 0x20, 0x00, 0x00, 0xD0, 0x20
  /*
    0xEA,   0xAB,   0x00,   0x00,   0xEA,   0x81,   0x00,   0x00,   0xA0,   0x61,
     0x00,   0xAA,   0xAB,   0x00,   0xA0,   0x81,   0x00,   0xAA,   0xAB,   0x00,
     0xAA,   0x81,   0x00,   0xAA,   0x61,   0x00,   0xAA,   0x49,   0x00,   0xAA,
     0x37,   0x00,   0xAA,   0x2A,   0x00,   0xAA,   0x20,   0x00,   0xAA,   0x18,
     0x00,   0x20,   0x00,   0x00,   0xD0,   0x20
 */
};
unsigned char TailGunnerSFX_100[] = // SHIELD
{ 0xED, 0xCC, 0x08, 0x00, 0xE0, 0x00, 0x08, 0x00, 0xD0, 0x20};

unsigned char TailGunnerSFX_011[] = // BOUNCE nr. 7 is fake, sound_reg is 3, like shield
{  0x6F, 0x5B, 0x02, 0x00, 0x2E, 0xF1, 0x02, 0x2C, 0xAD, 0x03,
  0x2E, 0x19, 0x02, 0x2C, 0x9F, 0x02, 0x2B, 0x46, 0x03, 0x2C,
  0x5B, 0x02, 0x2B, 0xF1, 0x02, 0x29, 0xAD, 0x03, 0x2B, 0x19,
  0x02, 0x29, 0x9F, 0x02, 0x28, 0x46, 0x03, 0x29, 0x5B, 0x02,
  0x28, 0xF1, 0x02, 0x26, 0xAD, 0x03, 0x28, 0x19, 0x02, 0x26,
  0x9F, 0x02, 0x25, 0x46, 0x03, 0x26, 0x5B, 0x02, 0x25, 0xF1,
  0x02, 0x24, 0xAD, 0x03, 0x26, 0x19, 0x02, 0x25, 0x9F, 0x02,
  0x24, 0x46, 0x03, 0x20, 0x00, 0x00, 0xD0, 0x20};

unsigned char TailGunnerSFX_010[] = // HYPERSPACE
{ 0x6B, 0x38, 0x02, 0x02, 0x0B, 0x0B, 0x2B, 0xC8, 0x01, 0x0B,
  0x0B, 0x2B, 0x7C, 0x01, 0x0B, 0x0B, 0x2B, 0x40, 0x01, 0x0B,
  0x0B, 0x2B, 0x1C, 0x01, 0x0B, 0x0B, 0x2B, 0xE4, 0x00, 0x0B,
  0x0B, 0x2B, 0xBE, 0x00, 0x0B, 0x0B, 0x2B, 0xA0, 0x00, 0x0B,
  0x0B, 0x2B, 0x72, 0x00, 0x0B, 0x0B, 0x2B, 0x5F, 0x00, 0x0B,
  0x0B, 0x2B, 0x50, 0x00, 0x0B, 0x0B, 0x2B, 0x47, 0x00, 0x0B,
  0x0B, 0x29, 0x72, 0x00, 0x09, 0x09, 0x29, 0x5F, 0x00, 0x09,
  0x09, 0x29, 0x50, 0x00, 0x09, 0x09, 0x29, 0x47, 0x00, 0x09,
  0x09, 0x28, 0x72, 0x00, 0x08, 0x08, 0x28, 0x5F, 0x00, 0x08,
  0x08, 0x28, 0x50, 0x00, 0x08, 0x08, 0x28, 0x47, 0x00, 0x08,
  0x08, 0x26, 0x72, 0x00, 0x06, 0x06, 0x26, 0x5F, 0x00, 0x06,
  0x06, 0x26, 0x50, 0x00, 0x06, 0x06, 0x26, 0x47, 0x00, 0x06,
  0x06, 0x24, 0x72, 0x00, 0x04, 0x04, 0x24, 0x5F, 0x00, 0x04,
  0x04, 0x24, 0x50, 0x00, 0x04, 0x04, 0x24, 0x47, 0x00, 0x04,
  0x04, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01,
  0x60, 0x00, 0x00, 0x00, 0xD0, 0x20};
unsigned char TailGunnerSFX_001[] = // - PUSH START
{ 0xD0,   0x20};
unsigned char TailGunnerSFX_000[] = // - UNKOWN
{ 0xD0,   0x20};



typedef struct {
	unsigned char *effectData;
	int loop;
	int channel;
	int valid;
} SoundEffect;


SoundEffect TailgunnerFX[] =
{
  {TailGunnerSFX_000, 0, 0 ,0}, // UNKOWN
  {TailGunnerSFX_001, 0, 0 ,0}, // PUSH START
  {TailGunnerSFX_010, 0, 0 ,1}, // HYPERSPACE (end of wave warp)
  {TailGunnerSFX_011, 0, 1 ,1}, // BOUNCE
  {TailGunnerSFX_100, 1, 2 ,1}, // SHIELD
  {TailGunnerSFX_101, 0, 0 ,1}, // LASER
  {TailGunnerSFX_110, 1, 1 ,1}, // THRUST
  {TailGunnerSFX_111, 0 ,0 ,1}, // EXPLODE
};




// 0 unkown
// 1 PUSH START
// 2 WARP
// 3 unkown
// 4 SHIELD
// 5 Laser
// 6 Thrust (allways)
// 7 Explosion

int sound_reg[8];
int sound_reg_old[8];

  void update_sound (int strobe)
  {
	  sound_reg[sound_addr] = sound_data;
	  // Hack for shield bounce sound: bounce uses sound reg value 3 (like laser!)
	  // bounce is assumed, if shield is on also (i.e. sound value 2)
	  if ( (sound_reg[3] != 0) && (sound_reg_old[3] == 0) )
	  {
		  if (sound_reg[2] != 0)
		  {	// if shield is on,
			  sound_reg[7] = 1;  	// 	set bounce on and
			  sound_reg[3] = 0;	// 	set laser off
		  }
		  else
          {
              sound_reg[7] = 0; // else set bounce off
          }
	  };
      if ((sound_reg_old[3]!=0) && (sound_reg[3]==0))
      {
        sound_reg_old[6] = 0; // make it so that thrust returns after a bounce
      }

	  // process sound registers
	  for (int i = 0; i < 8; i++)
	  {
		  if (TailgunnerFX[i].valid == 0)
			  continue;
		  if ( (sound_reg[i] != 0) && (sound_reg_old[i] == 0) )
		  {
			  v_playSFXCont(TailgunnerFX[i].effectData, TailgunnerFX[i].channel, TailgunnerFX[i].loop);
		  }
		  if ( (sound_reg[i] == 0) && (sound_reg_old[i] != 0) )
		  {
		    if (TailgunnerFX[i].loop)
			  v_playSFXStop(TailgunnerFX[i].effectData, TailgunnerFX[i].channel);
		  };
		  sound_reg_old[i] = sound_reg[i];
	  };
  };
