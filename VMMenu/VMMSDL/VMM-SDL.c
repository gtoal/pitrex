// SDL specific functions for the Vector Mame Menu

#include <fcntl.h>
#include <SDL.h>
#include <SDL2/SDL_mixer.h>
#include "vmmstddef.h"
#include "VMM-SDL.h"
#if defined(linux) || defined(__linux)
   #include "LinuxVMM.h"
#elif defined(__WIN32__) || defined(_WIN32)
   #include "WinVMM.h"
#endif
#include "zvgFrame.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef PITREX
#include <unistd.h>
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include "../PiTrex/window.h"
extern int PiTrex_init;
extern void PiTrexInit(void);
#endif

extern void   GetRGBfromColour(int, int*, int*, int*);               // Get R, G and B components of a passed colour

SDL_Window     *window = NULL;
SDL_Renderer   *screenRender = NULL;
int            WINDOW_WIDTH  = 768;
int            WINDOW_HEIGHT; // = 600;
#define        WINDOW_SCALE (1024.0/WINDOW_WIDTH)

#if defined(linux) || defined(__linux)
const          char* WINDOW_TITLE = "Vector Mame Menu for Linux";
#elif defined(__WIN32__) || defined(_WIN32)
const          char* WINDOW_TITLE = "Vector Mame Menu for Win32";
#endif
int            mdx=0, mdy=0;
int            MouseX=0, MouseY=0;
extern         int ZVGPresent;
int            SDL_VB, SDL_VC;            // SDL_Vector "Brightness" and "Colour"
int            optz[16];                  // array of user defined menu preferences
int            keyz[11];                  // array of key press codes
extern int     mousefound;
extern char    DVGPort[15];
uint32_t       timestart = 0, timenow, duration;

enum vsounds
{ 
  sSFury,
  sNuke,
  sExplode1,
  sFire1,
  sExplode2,
  sFire2,
  sExplode3
};


//The sound effects that will be used
Mix_Chunk      *gSFury    = NULL;
Mix_Chunk      *gFire1    = NULL;
Mix_Chunk      *gFire2    = NULL;
Mix_Chunk      *gExplode1 = NULL;
Mix_Chunk      *gExplode2 = NULL;
Mix_Chunk      *gExplode3 = NULL;
Mix_Chunk      *gNuke     = NULL;

#define		fps     60
#define		fps_ms  1000/fps


/******************************************************************
   Start up the DVG if poss and use SDL if necessary
*******************************************************************/
void startZVG(void)
{
   unsigned int error;
   #if DEBUG
      printf("Key UP:       0x%04x\n", UP);
      printf("Key DOWN:     0x%04x\n", DOWN);
      printf("Key LEFT:     0x%04x\n", LEFT);
      printf("Key RIGHT:    0x%04x\n", RIGHT);
      printf("Key SETTINGS: 0x%04x\n", GRAVE);
      printf("Key ESCAPE:   0x%04x\n", ESC);
      printf("Key CREDIT:   0x%04x\n", CREDIT);
      printf("Key START1:   0x%04x\n", START1);
      printf("Key START2:   0x%04x\n", START2);
      printf("Key FIRE:     0x%04x\n", FIRE);
      printf("Key THRUST:   0x%04x\n", THRUST);
      printf("Key HYPSPACE: 0x%04x\n", HYPSPACE);
      printf("Key RSHIFT:   0x%04x\n", RSHIFT);
      printf("Key LSHIFT:   0x%04x\n", LSHIFT);
   #endif

   InitialiseSDL(1);
#ifdef PITREX
      printf(">>> PiTrex Hardware Version <<<");
#else
   #ifdef _DVGTIMER_H_
      printf(">>> DVG Hardware Version using port: %s <<<\n",DVGPort);
   #else
      printf(">>> ZVG Hardware Version <<<");
   #endif
#endif
   error = zvgFrameOpen();         // initialize ZVG/DVG
   if (error)
   {
      zvgError(error);             // print error
      printf("Vector Generator hardware not found, rendering to SDL window only.\n");
      ZVGPresent = 0;
   }
   else
   {
#ifndef PITREX
      tmrSetFrameRate(FRAMES_PER_SEC);
#endif
      zvgFrameSetClipWin( X_MIN, Y_MIN, X_MAX, Y_MAX);
   }
//   #if DEBUG
//      //print out a ZVG banner, indicating version etc.
//      zvgBanner( ZvgSpeeds, &ZvgID);
//      printf("\n\n");
//   #endif
}

/******************************************************************
   itoa function, which isn't ANSI though is part of DJGPP
*******************************************************************/
char* itoa(int value, char* result, int base)
{
   // check that the base if valid
   if (base < 2 || base > 36) { *result = '\0'; return result; }

   char* ptr = result, *ptr1 = result, tmp_char;
   int tmp_value;

   do {
      tmp_value = value;
      value /= base;
      *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
   } while ( value );

   // Apply negative sign
   if (tmp_value < 0) *ptr++ = '-';
   *ptr-- = '\0';
   while(ptr1 < ptr) {
      tmp_char = *ptr;
      *ptr--= *ptr1;
      *ptr1++ = tmp_char;
   }
   return result;
}


/********************************************************************
 Initialise SDL and screen
********************************************************************/
void InitialiseSDL(int start)
{
   /* Initialise SDL */
   if (start)
   {
      if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0)
      {
         fprintf( stderr, "Could not initialise SDL: %s\n", SDL_GetError() );
         exit( -1 );
      }
      //else printf("SDL Initialised\n");
   }
   /* Set a video mode */
   WINDOW_HEIGHT=((WINDOW_WIDTH/4)*3); // this ensure the window is 4:3
   window = SDL_CreateWindow( WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIDDEN);

   screenRender = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE); // Don't use accelerated as it is tied to the screen refresh rate
   SDL_ShowWindow(window);

   SDL_SetRelativeMouseMode(SDL_TRUE);
   SDL_Event event;
   while (SDL_PollEvent(&event)) {}            // clear event buffer

   MouseX = 0;
   MouseY = 0;
   
   
   //Initialize SDL_mixer
   if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 )
   {
      printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   //else printf("SDL Mixer initialised\n");
   
   //Load sound effects
   gSFury = Mix_LoadWAV( SND_DIR "/sfury9.wav" );
   if( gSFury == NULL )
   {
       printf( "Failed to load sfury9 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gFire1 = Mix_LoadWAV( SND_DIR "/elim2.wav" );
   if( gFire1 == NULL )
   {
       printf( "Failed to load elim2 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gExplode1 = Mix_LoadWAV( SND_DIR "/explode1.wav" );
   if( gExplode1 == NULL )
   {
       printf( "Failed to load explode1 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gExplode2 = Mix_LoadWAV( SND_DIR "/explode2.wav" );
   if( gExplode2 == NULL )
   {
       printf( "Failed to load explode2 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gExplode3 = Mix_LoadWAV( SND_DIR "/explode3.wav" );
   if( gExplode3 == NULL )
   {
       printf( "Failed to load explode3 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gFire2 = Mix_LoadWAV( SND_DIR "/efire.wav" );
   if( gFire2 == NULL )
   {
       printf( "Failed to load efire sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gNuke = Mix_LoadWAV( SND_DIR "/nuke1.wav" );
   if( gNuke == NULL )
   {
      printf( "Failed to load nuke1 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   Mix_Volume(-1, optz[o_volume]);
}


/********************************************************************
	Play a sound sample
  Currently not audible on default PiZero hardware.  Would be nice
  to create compatible tunes and punt them over to the Vectrex.
********************************************************************/
void playsound(int picksound)
{
  
  // just returning here doesn't work - causes menu to hang at startup trying to open some audio device:
  // open("/dev/snd/pcmC0D0p", O_RDWR|O_NONBLOCK|O_CLOEXEC
  
   if (optz[o_volume] > 0)
   {
      Mix_Volume(-1, optz[o_volume]);
      switch(picksound)
      {
         case 0:
            Mix_PlayChannel( -1, gSFury, 0 );
            break;
         case 1:
            Mix_PlayChannel( -1, gNuke, 0 );
            break;
         case 2:
            Mix_PlayChannel( -1, gExplode1, 0 );
            break;
         case 3:
            Mix_PlayChannel( -1, gFire1, 0 );
            break;
         case 4:
            Mix_PlayChannel( -1, gExplode2, 0 );
            break;
         case 5:
            Mix_PlayChannel( -1, gFire2, 0 );
            break;
         case 6:
            Mix_PlayChannel( -1, gExplode3, 0 );
            break;
         default:
            break;
      }
   }
}


/********************************************************************
   Close off SDL cleanly
********************************************************************/
void CloseSDL(int done)
{
   SDL_SetWindowGrab(window, SDL_FALSE);
   SDL_ShowCursor(SDL_ENABLE);
   SDL_DestroyWindow(window);
   Mix_FreeChunk( gSFury );
   Mix_FreeChunk( gFire1 );
   Mix_FreeChunk( gExplode1 );
   Mix_FreeChunk( gExplode2 );
   Mix_FreeChunk( gExplode3 );
   Mix_FreeChunk( gFire2 );
   Mix_FreeChunk( gNuke );
   gSFury    = NULL;
   gFire1    = NULL;
   gExplode1 = NULL;
   gExplode2 = NULL;
   gExplode3 = NULL;
   gFire2    = NULL;
   gNuke     = NULL;
   Mix_CloseAudio();
   if (done)
   {
      Mix_Quit();
      SDL_Quit();
   }
}


/********************************************************************
   Send a vector to the SDL surface - adjust co-ords from ZVG format
********************************************************************/
void SDLvector(float x1, float y1, float x2, float y2, int clr, int bright)
{
   int r, g, b;

   if (optz[o_dovga] || !ZVGPresent)
   {
      if (clr > 7) clr = vwhite;
      if (bright > 31) bright = 31;
      GetRGBfromColour(clr, &r, &g, &b);
      SDL_SetRenderDrawColor(screenRender, r*8*bright, g*8*bright, b*8*bright, 127); //SDL_ALPHA_OPAQUE);
      SDL_RenderDrawLine(screenRender, x1/WINDOW_SCALE+(WINDOW_WIDTH/2), -y1/WINDOW_SCALE+(WINDOW_HEIGHT/2),
                                       x2/WINDOW_SCALE+(WINDOW_WIDTH/2), -y2/WINDOW_SCALE+(WINDOW_HEIGHT/2));
   }
}


/********************************************************************
   Send a screen to the SDL surface
********************************************************************/
void FrameSendSDL()
{
#ifndef PITREX
   if (optz[o_dovga] || !ZVGPresent)
   {
      SDL_RenderPresent(screenRender);                    // Flip to rendered screen
      SDL_SetRenderDrawColor(screenRender, 0, 0, 0, 255); // Set render colour to black
      SDL_RenderClear(screenRender);                      // Clear screen
   }
#endif
   timenow = SDL_GetTicks();
   duration = (timenow-timestart);
   //printf("Time now: %i Loop duration: %i Wait time:%i\n", timenow, duration, fps_ms-duration);
#ifndef PITREX
   if (!ZVGPresent)
   {
      if ((duration < fps_ms) && ((fps_ms-duration)>0)) SDL_Delay(fps_ms-duration);
   }
#endif
   timestart=SDL_GetTicks();
}


/******************************************************************
Check whether a mouse driver is installed
*******************************************************************/
int initmouse(void)
{
   return 1;                                 // Let's just say yes...
}


/******************************************************************
Get the amount by which the mouse has been moved since last check
   deltas are read and accumulated by keyboard event function
   Mouse types:   0 = No device
                  1 = X-Axis Spinner
                  2 = Y-Axis Spinner
                  3 = Trackball/Mouse
*******************************************************************/
void processmouse(void)
{
   if (optz[o_mouse])
   {
      MouseX = mdx/optz[o_msens];           // use the integer part
      MouseY = mdy/optz[o_msens];
      mdx = mdx%optz[o_msens];               // retain the fractional part
      mdy = mdy%optz[o_msens];
      //printf("mdx: %d mdy: %d\n", mdx, mdy);

      // If spinner selected, discard the axis not in use, the spinner might really be a mouse
      if (optz[o_mouse] == 1) MouseY = 0;   // Spinner which moves X-axis
      if (optz[o_mouse] == 2)                // Spinner which moves Y-axis
      {
         MouseX = MouseY;                  // Convert to X axis
         MouseY = 0;                        // Discard Y axis
      }

      if (optz[o_mrevX]) MouseX = -MouseX; // Reverse X axis if selected
      if (optz[o_mrevY]) MouseY = -MouseY; // Reverse Y axis if selected
   }
   else
   {
      MouseX = 0;                           // if we said we don't have a mouse then
      MouseY = 0;                           // set all the values to zero - there might
      mdx=0;                                 // still be a mouse connected giving values
      mdy=0;                                 // which we don't want to have an effect
   }
}


/******************************************************************
Get keypress - SDL implementation. Returns scancode of pressed key
Also updates mouse x and y movements
*******************************************************************/
int getkey(void)
{
#ifdef PITREX
  static int bit[5] = { 0x88, 0x44, 0x22, 0x11, 0xFF };
  static int KeyPressOn = 0, PrevMouseX = 0, PrevMouseY = 0;
#define BUTTON1 0
#define BUTTON2 1
#define BUTTON3 2
#define BUTTON4 3
#define BUTTONS1234 4
#define controller(code) (currentButtonState & bit[code])
#define sgn(x) ((x) ? ((x) > 0 ? 1 : -1) : 0)
#define JOYSTICK_CENTER_MARGIN 0x60
  
  MouseX = currentJoy1X;  // 0x80   0  0x7f
  if ((MouseX>0) && (MouseX<JOYSTICK_CENTER_MARGIN)) MouseX = 0;
  if ((MouseX<0) && (MouseX>-JOYSTICK_CENTER_MARGIN)) MouseX = 0;
  MouseY = -currentJoy1Y;  // 0x80   0  0x7f  // first release of this had Y axis inverted
  if ((MouseY>0) && (MouseY<JOYSTICK_CENTER_MARGIN)) MouseY = 0;
  if ((MouseY<0) && (MouseY>-JOYSTICK_CENTER_MARGIN)) MouseY = 0;

  /*
k_togglemenu                   = 0x002c
k_quit                         = 0x0029
k_options                      = 0x0035
k_prevman                      = 0x0050
k_nextman                      = 0x004f
k_prevgame                     = 0x00e0
k_nextgame                     = 0x00e2
k_prevclone                    = 0x0050
k_nextclone                    = 0x004f
k_startgame                    = 0x001e
   */
  
  if (sgn(MouseX) != sgn(PrevMouseX)) {
    PrevMouseX = MouseX;
    if (MouseX < 0) return SDL_SCANCODE_LEFT;
    if (MouseX > 0) return SDL_SCANCODE_RIGHT;
  } else if (sgn(MouseY) != sgn(PrevMouseY)) {
    PrevMouseY = MouseY;
    if (MouseY < 0) return 0x00e0; // prevgame
    if (MouseY > 0) return 0x00e2; // nextgame
  } else if (KeyPressOn && !controller(BUTTONS1234)) {
    KeyPressOn = 0;
  } else if (controller(BUTTON2)) {
    if (KeyPressOn == 0) {
      KeyPressOn = 1;
      return 0x0035; // options SDL_SCANCODE_LEFT;
    }
  } else if (controller(BUTTON1)) {
    if (KeyPressOn == 0) {
      KeyPressOn = 1;
      return 0x001e; // start game SDL_SCANCODE_RIGHT;
    }
  } else if (controller(BUTTON3)) {
    if (KeyPressOn == 0) {
      KeyPressOn = 1;
      return 0x002c; // toggle menu SDL_SCANCODE_UP;
    }
  } else if (controller(BUTTON4)) {
    if (KeyPressOn == 0) {
      KeyPressOn = 1;
      return 0x0029; // quit SDL_SCANCODE_DOWN;
    }
  }
  return 0;
#else
   int key=0;
   SDL_Event event;
   while(SDL_PollEvent(&event))
   {
      //printf("Event: %d\n", event.type);
      switch(event.type)
      {
         case SDL_MOUSEMOTION:
            mdx += event.motion.xrel;
            mdy += event.motion.yrel;
            break;
         case SDL_KEYDOWN:
            key = event.key.keysym.scancode;
            //printf("Key: %X\n", key);
            break;
         default:
            break;
      }
   }

   if (mousefound) processmouse();              // 3 Feb 2020, read every frame, ignore sample rate
   // convert mouse movement into key presses. Now built into the getkey function
   if (MouseY < 0 && optz[o_mouse]==3) key = keyz[k_pgame];       // Trackball Up    = Up
   if (MouseY > 0 && optz[o_mouse]==3) key = keyz[k_ngame];       // Trackball Down  = Down
   if (MouseX < 0 && optz[o_mouse]==3) key = keyz[k_pclone];      // Trackball Left  = Left
   if (MouseX > 0 && optz[o_mouse]==3) key = keyz[k_nclone];      // Trackball Right = Right
   if (MouseX < 0 && optz[o_mouse]!=3) key = keyz[k_pgame];       // Spinner   Left  = Up
   if (MouseX > 0 && optz[o_mouse]!=3) key = keyz[k_ngame];       // Spinner   Right = Down

   if (key == keyz[k_ngame])   playsound(sFire1);
   if (key == keyz[k_pgame])   playsound(sFire1);
   if (key == keyz[k_nclone])  playsound(sFire2);
   if (key == keyz[k_pclone])  playsound(sFire2);
   if (key == keyz[k_start])   playsound(NewScale());
   if (key == keyz[k_options]) playsound(sNuke);
   if (key == keyz[k_quit])    playsound(NewScale());
   if (key == keyz[k_menu])    playsound(sNuke);
   return key;
#endif
}


/******************************************************************
   Set Mouse Position
*******************************************************************/
void mousepos(int *mx, int *my)
{
   SDL_GetMouseState(mx, my);
   *mx = (*mx - (WINDOW_WIDTH/2 -22));
   *my = -(*my - (WINDOW_HEIGHT/2 -22));
}


/******************************************************
Send the frame to the ZVG, exit if it went pear shaped
*******************************************************/
int sendframe(void)
{
   unsigned int   err=0;
   if (ZVGPresent)
   {
      //tmrWaitForFrame();      // wait for next frame time
//fprintf(stderr, "Z1\n");
      err = zvgFrameSend();     // send next frame
//fprintf(stderr, "Z2\n");
      if (err)
      {
//fprintf(stderr, "Z3\n");
         zvgError( err);
         zvgFrameClose();       // fix up all the ZVG stuff
//fprintf(stderr, "Z4\n");
         exit(1);
      }
   }
//fprintf(stderr, "Z5\n");
   FrameSendSDL();
//fprintf(stderr, "Z6\n");
   return err;
}


/******************************************************
Close everything off for a graceful exit
*******************************************************/
void ShutdownAll(void)
{
   CloseSDL(1);
   #if defined(linux) || defined(__linux)
      setLEDs(8);
   #else
      setLEDs(0);                                 // restore LED status
   #endif
   if (ZVGPresent)
   {
      zvgFrameClose();                            // fix up all the ZVG stuff
   }
}


/***********************************************
 Set colour to one of red, green, blue, magenta,
 cyan, yellow, white
************************************************/
void setcolour(int clr, int bright)
{
#ifndef PITREX
   int r, g, b;
#endif
   if (clr > 7) clr = vwhite;
   if (bright > 31) bright = 31;
#ifdef PITREX
   if (PiTrex_init == 0) {
     //fprintf(stderr, "INIT in zvgFrameSend\n");
     PiTrex_init = 1;
     PiTrexInit();
     //fprintf(stderr, "PITREX INITIALISED!\n");
   }
   //fprintf(stderr, "v_brightness(bright=%d*4);\n", bright);
   v_brightness(bright*4);
#else
   GetRGBfromColour(clr, &r, &g, &b);
   if (ZVGPresent)
   {
      zvgFrameSetRGB15(r*bright, g*bright, b*bright);
   }
#endif
   SDL_VC = clr;      // a bit hacky, it was a late addition.
   SDL_VB = bright;   // should pass as parameters to draw functions
}


/*******************************************************************
 Draw a vector - pass the start, end points, and the x and y offsets
 Uses global rotation variable to determine orientation
********************************************************************/
void drawvector(point p1, point p2, float x_trans, float y_trans)
{
#ifdef PITREX
  //fprintf(stderr, "v_brightness(SDL_VB=%d*4);\n", SDL_VB);
  v_brightness(SDL_VB * 4);
#endif
  // Standard - no rotation
   if (optz[o_rot] == 0)
   {
      if (ZVGPresent)
      {
         zvgFrameVector(p1.x + x_trans, p1.y + y_trans, p2.x + x_trans, p2.y + y_trans);
      }
      SDLvector(p1.x + x_trans, p1.y + y_trans, p2.x + x_trans, p2.y + y_trans, SDL_VC, SDL_VB);
   }
   // rotated LEFT (90° CW)
   if (optz[o_rot] == 1)
   {
      if (ZVGPresent)
      {
         zvgFrameVector(p1.y + y_trans, -(p1.x + x_trans), p2.y + y_trans, -(p2.x + x_trans));
      }
      SDLvector(p1.y + y_trans, -(p1.x + x_trans), p2.y + y_trans, -(p2.x + x_trans), SDL_VC, SDL_VB);
   }
   // Rotated 180°
   if (optz[o_rot] == 2)
   {
      if (ZVGPresent)
      {
         zvgFrameVector(-(p1.x + x_trans), -(p1.y + y_trans), -(p2.x + x_trans), -(p2.y + y_trans));
      }
      SDLvector(-(p1.x + x_trans), -(p1.y + y_trans), -(p2.x + x_trans), -(p2.y + y_trans), SDL_VC, SDL_VB);
   }
   // rotated RIGHT (90° CCW)
   if (optz[o_rot] == 3)
   {
      if (ZVGPresent)
      {
         zvgFrameVector(-(p1.y + y_trans), (p1.x + x_trans), -(p2.y + y_trans), (p2.x + x_trans));
      }
      SDLvector(-(p1.y + y_trans), (p1.x + x_trans), -(p2.y + y_trans), (p2.x + x_trans), SDL_VC, SDL_VB);
   }
}

/********************************************************************
   Close SDL and execute MAME, restart SDL when done
********************************************************************/
void RunGame(char *gameargs, char *zvgargs)
{
   unsigned int   err;
   char   command[200];

   setLEDs(0);
   CloseSDL(0);                        // Close windows etc but don't quit SDL
   if (ZVGPresent)
   {
      if (optz[o_redozvg])
      {
         zvgFrameClose();              // Close the ZVG
      }
      sprintf(command, "/opt/pitrex/bin/vmm.sh '%s' '%s'", gameargs, zvgargs);
   }
   else
   {
     sprintf(command, "/opt/pitrex/bin/vmm.sh \"%s\"", gameargs);
   }
   printf("Launching: [%s]\n", command);
#ifdef PITREX
   v_detectExitEvents = 0;
#endif
   err = system(command);
   if (optz[o_redozvg] && ZVGPresent)  // Re-open the ZVG if MAME closed it
   {
#ifdef PITREX
      usleep(500000); //Should just disable 1+2+3+4 exit function for 1s instead.
      v_init();
#endif
      err = zvgFrameOpen();            // initialize everything
      if (err)
      {
         zvgError( err);               // if it went wrong print error
         exit(0);                      // and return to OS
      }
   }
   InitialiseSDL(1);                   // re-open windows etc
}
