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
   #ifdef _DVGTIMER_H_
      printf(">>> DVG Hardware Version using port: %s <<<\n",DVGPort);
   #else
      printf(">>> ZVG Hardware Version <<<");
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
      tmrSetFrameRate(FRAMES_PER_SEC);
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
   gSFury = Mix_LoadWAV( "VMMsnd/sfury9.wav" );
   if( gSFury == NULL )
   {
       printf( "Failed to load sfury9 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gFire1 = Mix_LoadWAV( "VMMsnd/elim2.wav" );
   if( gFire1 == NULL )
   {
       printf( "Failed to load elim2 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gExplode1 = Mix_LoadWAV( "VMMsnd/explode1.wav" );
   if( gExplode1 == NULL )
   {
       printf( "Failed to load explode1 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gExplode2 = Mix_LoadWAV( "VMMsnd/explode2.wav" );
   if( gExplode2 == NULL )
   {
       printf( "Failed to load explode2 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gExplode3 = Mix_LoadWAV( "VMMsnd/explode3.wav" );
   if( gExplode3 == NULL )
   {
       printf( "Failed to load explode3 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gFire2 = Mix_LoadWAV( "VMMsnd/efire.wav" );
   if( gFire2 == NULL )
   {
       printf( "Failed to load efire sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   gNuke = Mix_LoadWAV( "VMMsnd/nuke1.wav" );
   if( gNuke == NULL )
   {
      printf( "Failed to load nuke1 sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
   }
   Mix_Volume(-1, optz[o_volume]);
}


/********************************************************************
	Play a sound sample
********************************************************************/
void playsound(int picksound)
{
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
   if (optz[o_dovga] || !ZVGPresent)
   {
      SDL_RenderPresent(screenRender);                    // Flip to rendered screen
      SDL_SetRenderDrawColor(screenRender, 0, 0, 0, 255); // Set render colour to black
      SDL_RenderClear(screenRender);                      // Clear screen
   }
   timenow = SDL_GetTicks();
   duration = (timenow-timestart);
   //printf("Time now: %i Loop duration: %i Wait time:%i\n", timenow, duration, fps_ms-duration);
   if (!ZVGPresent)
   {
      if ((duration < fps_ms) && ((fps_ms-duration)>0)) SDL_Delay(fps_ms-duration);
   }
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
      err = zvgFrameSend();     // send next frame
      if (err)
      {
         zvgError( err);
         zvgFrameClose();       // fix up all the ZVG stuff
         exit(1);
      }
   }
   FrameSendSDL();
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
   int r, g, b;
   if (clr > 7) clr = vwhite;
   if (bright > 31) bright = 31;
   GetRGBfromColour(clr, &r, &g, &b);
   if (ZVGPresent)
   {
      zvgFrameSetRGB15(r*bright, g*bright, b*bright);
   }
   SDL_VC = clr;      // a bit hacky, it was a late addition.
   SDL_VB = bright;   // should pass as parameters to draw functions
}


/*******************************************************************
 Draw a vector - pass the start, end points, and the x and y offsets
 Uses global rotation variable to determine orientation
********************************************************************/
void drawvector(point p1, point p2, float x_trans, float y_trans)
{
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
      sprintf(command, "./vmm.sh '%s' '%s'", gameargs, zvgargs);
   }
   else
   {
      sprintf(command, "./vmm.sh \"%s\"", gameargs);
   }
   printf("Launching: [%s]\n", command);
   err = system(command);
   if (optz[o_redozvg] && ZVGPresent)  // Re-open the ZVG if MAME closed it
   {
      err = zvgFrameOpen();            // initialize everything
      if (err)
      {
         zvgError( err);               // if it went wrong print error
         exit(0);                      // and return to OS
      }
   }
   InitialiseSDL(1);                   // re-open windows etc
}
