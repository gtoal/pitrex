/**************************************
DOSvmm.c
Chad Gray, 18 Jun 2011
DOS Specific Functions
**************************************/

#include <bios.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include "vmmstddef.h"
#include "DOSVMM.h"
#include "zvgFrame.h"
#include <audio.h>		// SEAL library for Sounds

#define Mouse_INT 0X33

extern void GetRGBfromColour(int, int*, int*, int*);   // Get R, G and B components of a passed colour
extern void writecfg(void);                            // write the cfg file

union       REGS in, out;
int         LEDstate=0;
int         MouseX=0, MouseY=0;
int         mdx=0, mdy=0;
int         optz[15];                        // array of user defined menu preferences
int         f1_press = 0, f2_press = 0, f3_press = 0, f4_press = 0;
//char      zvgargs[30];
extern int  ZVGPresent;
extern char auth1[], auth2[];
int         keyz[11];                        // array of key press codes
extern int  mousefound;

//=========================================
// SEAL Audio variables
int         SoundActivated;
AUDIOINFO   info;
HAC         hVoice;

// enum sounds so we can call them by name
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
LPAUDIOWAVE    gSFury;
LPAUDIOWAVE    gFire1;
LPAUDIOWAVE    gFire2;
LPAUDIOWAVE    gExplode1;
LPAUDIOWAVE    gExplode2;
LPAUDIOWAVE    gExplode3;
LPAUDIOWAVE    gNuke;
//=========================================


/******************************************************************
Try to open the ZVG
*******************************************************************/
void startZVG(void)
{
   SoundActivated=InitialiseSEAL();
   uint error;
   error = zvgFrameOpen();                   // initialize everything
   if (error)
   {
      zvgError(error);                       // print error
      printf("\nVector Generator hardware not found! Quitting...\n");
      printf("\n%s, (c) 2009-2020\n", auth1);
      printf("%s\n", auth2);
      exit(0);                               // return to DOS
   }
   else
   {
      tmrSetFrameRate( FRAMES_PER_SEC);
      zvgFrameSetClipWin( X_MIN, Y_MIN, X_MAX, Y_MAX);
   }
   #if DEBUG
      //print out a ZVG banner, indicating version etc.
      zvgBanner( ZvgSpeeds, &ZvgID);
      printf("\n\n");
   #endif
}


/******************************************************************
Check for a keypress, return code of key (or modifier)
*******************************************************************/
int getkey(void)
{
   int key=0, shift=0;
   if (bioskey(1))                              // There is a key waiting to be pressed
   {
      key=bioskey(0);                           // Get the key press
   }
   else
   {
      shift=bioskey(2) & 15;                    // only want bits 0-3 so mask off higher 4 bits
      if (shift)   key=shift;
      // Disable autorepeat of modifiers
      if (key==FIRE)                            // LCTRL
         if (f1_press==1) key=0;
         else f1_press=1;
      else f1_press=0;
      if (key==THRUST)                          // L_ALT
         if (f2_press==1) key=0;
         else f2_press=1;
      else f2_press=0;
      if (key==RSHIFT)                          // R_SHIFT
         if (f3_press==1) key=0;
         else f3_press=1;
      else f3_press=0;
      if (key==LSHIFT)                          // L_SHIFT
         if (f4_press==1) key=0;
         else f4_press=1;
      else f4_press=0;
   }
   
   if (mousefound) processmouse();              // 3 Feb 2020, read every frame, ignore sample rate
   // Convert mouse movement into key presses. Makes things so much easier.
   if (MouseY < 0 && optz[o_mouse]==3) key = keyz[k_pgame];   // Trackball Up    = Up
   if (MouseY > 0 && optz[o_mouse]==3) key = keyz[k_ngame];   // Trackball Down  = Down
   if (MouseX < 0 && optz[o_mouse]==3) key = keyz[k_pclone];  // Trackball Left  = Left
   if (MouseX > 0 && optz[o_mouse]==3) key = keyz[k_nclone];  // Trackball Right = Right
   if (MouseX < 0 && optz[o_mouse]!=3) key = keyz[k_pgame];   // Spinner   Left  = Up
   if (MouseX > 0 && optz[o_mouse]!=3) key = keyz[k_ngame];   // Spinner   Right = Down

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
Check whether a mouse driver is installed
*******************************************************************/
int initmouse()
{
   in.x.ax = 0;                              // Mouse Reset/Get Mouse Installed Flag
   int86 (Mouse_INT, &in, &out);
   return out.x.ax;
}


/******************************************************************
Get the amount by which the mouse has been moved since last check
   Mouse types:   0 = No device
                  1 = X-Axis Spinner
                  2 = Y-Axis Spinner
                  3 = Trackball/Mouse
*******************************************************************/
void processmouse()
{
   in.x.ax = 0x0b;                           // Read Mouse Motion Counters
   int86 (Mouse_INT, &in, &out);

   mdx += out.x.cx;                          // Read X axis
   mdy += out.x.dx;                          // Read Y axis

   if (mdx > 32768) mdx -= 65536;            // Correction for negative values
   if (mdy > 32768) mdy -= 65536;            // Correction for negative values

   if (optz[o_mouse])
   {
      MouseX = mdx/optz[o_msens];          // use the integer part
      MouseY = mdy/optz[o_msens];
      mdx = mdx%optz[o_msens];             // retain the fractional part
      mdy = mdy%optz[o_msens];
      //printf("mdx: %d mdy: %d\n", mdx, mdy);

      // If spinner selected, discard the axis not in use, the spinner might really be a mouse
      if (optz[o_mouse] == 1) MouseY = 0;  // Spinner which moves X-axis
      if (optz[o_mouse] == 2)              // Spinner which moves Y-axis
      {
         MouseX = MouseY;                  // Convert to X axis
         MouseY = 0;                       // Discard Y axis
      }

      if (optz[o_mrevX]) MouseX = -MouseX; // Reverse X axis if selected
      if (optz[o_mrevY]) MouseY = -MouseY; // Reverse Y axis if selected
   }
   else
   {
      MouseX = 0;                          // if we said we don't have a mouse then
      MouseY = 0;                          // set all the values to zero - there might
      mdx=0;                               // still be a mouse connected giving values
      mdy=0;                               // which we don't want to have an effect
   }
}


/******************************************************************
   Get the current mouse co-ords into mx and my
*******************************************************************/
void mousepos(int *mx, int *my)
{
   int x, y;
   in.x.ax = 0x03;                           // Get Mouse Position and Button Status
   int86 (Mouse_INT, &in, &out);
   x = out.x.cx;
   y = out.x.dx;
   x = (x * 1.6) - X_MAX;
   if (x > X_MAX) x = X_MAX;
   y = Y_MAX - (y * 4);
   *mx = x;
   *my = y;
}


/******************************************************************
   Write to keyboard LEDs value held in global variable LEDstate
 - only if state has changed
*******************************************************************/
void setLEDs(int value)
{
   if (LEDstate != value)
   {
      LEDstate = value;
      asm("movb $0xed, %al\n\t"
      "out %al, $0x60\n\t"
      "nop\n\t"
      "movb _LEDstate, %al\n\t"
      "out %al, $0x60");
   }
}


/******************************************************************
Get state of modifiers so we can set LEDs appropriately on exit
*******************************************************************/
int GetModifierStatus(void)
{
   int status = (bioskey(2) & 112) >> 4;     // read lock key status
   return status;
}


/******************************************************
Send the frame to the ZVG, exit if it went pear shaped
*******************************************************/
int sendframe(void)
{
   uint   err;
   //BOOL   stopped;
   tmrWaitFrame();                           // wait for next frame time
   err = zvgFrameSend();                     // send next frame
   if (err)
   {
      zvgError( err);
      zvgFrameClose();                       // fix up all the ZVG stuff
      exit(1);
   }
   if (SoundActivated)
   {
      AUpdateAudio();
   }

   return err;
}


/******************************************************
Close everything off for a graceful exit
*******************************************************/
void ShutdownAll(void)
{
   CloseSEAL();
   setLEDs(GetModifierStatus());             // restore correct LED status
   if (ZVGPresent && optz[o_redozvg])
   {
      zvgFrameClose();                       // fix up all the ZVG stuff
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
   zvgFrameSetRGB15(r*bright, g*bright, b*bright);
}


/*******************************************************************
 Draw a vector - pass the start, end points, and the x and y offsets
  Uses global rotation variable to determine orientation
********************************************************************/
void drawvector(point p1, point p2, float x_trans, float y_trans)
{
   // Standard - no rotation
   if (optz[o_rot] == 0)
   zvgFrameVector(p1.x + x_trans, p1.y + y_trans, p2.x + x_trans, p2.y + y_trans);
   // rotated LEFT (90 deg CW)
   if (optz[o_rot] == 1)
   zvgFrameVector(p1.y + y_trans, -(p1.x + x_trans), p2.y + y_trans, -(p2.x + x_trans));
   // Rotated 180 deg
   if (optz[o_rot] == 2)
   zvgFrameVector(-(p1.x + x_trans), -(p1.y + y_trans), -(p2.x + x_trans), -(p2.y + y_trans));
   // rotated RIGHT (90 deg CCW)
   if (optz[o_rot] == 3)
   zvgFrameVector(-(p1.y + y_trans), (p1.x + x_trans), -(p2.y + y_trans), (p2.x + x_trans));
}


/********************************************************************
   (Optionally) close ZVG and execute MAME, restart ZVG when done
********************************************************************/
void RunGame(char *gameargs, char *zvgargs)
{
   (void)         zvgargs;
   unsigned int   err;
   char           command[80];
   //setLEDs(0);
   setLEDs(S_LED);
   CloseSEAL();
   
   if (ZVGPresent)
   {
      if (optz[o_redozvg])
      {
         zvgFrameClose();              // Close the ZVG
      }
      sprintf(command, "./vmm.bat %s %s", gameargs, zvgargs);
   }
   else
   {
      sprintf(command, "./vmm.bat %s", gameargs);
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
   SoundActivated=InitialiseSEAL();
}


/********************************************************************
   Play a sound sample
********************************************************************/
void playsound(int picksound)
{
   if ((optz[o_volume] > 0) && (SoundActivated))
   {
      ASetVoiceVolume(hVoice, optz[o_volume]);
      switch(picksound)
      {
         case sSFury:
            APlayVoice(hVoice, gSFury);
            break;
         case sNuke:
            APlayVoice(hVoice, gNuke);
            break;
         case sExplode1:
            APlayVoice(hVoice, gExplode1);
            break;
         case sFire1:
            APlayVoice(hVoice, gFire1);
            break;
         case sExplode2:
            APlayVoice(hVoice, gExplode2);
            break;
         case sFire2:
            APlayVoice(hVoice, gFire2);
            break;
         case sExplode3:
            APlayVoice(hVoice, gExplode3);
            break;
         default:
            break;
      }
   }
}


/********************************************************************
   Initialise SEAL - open audio device
********************************************************************/
int InitialiseSEAL(void)
{
   uint      rc;
   AUDIOCAPS caps;
   AInitialize();

   info.nDeviceId = AUDIO_DEVICE_MAPPER;
   info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
   info.nSampleRate = 44100;
   if ((rc = AOpenAudio(&info)) != AUDIO_ERROR_NONE)
   {
      CHAR szText[80];
      AGetErrorText(rc, szText, sizeof(szText) - 1);
      printf("ERROR: %s\n", szText);
      exit(0);
   }

   /* print information */
   AGetAudioDevCaps(info.nDeviceId, &caps);
   printf("%s at %d-bit %s %u Hz detected\n",
   caps.szProductName,
   info.wFormat & AUDIO_FORMAT_16BITS ? 16 : 8,
   info.wFormat & AUDIO_FORMAT_STEREO ? "stereo" : "mono",
   info.nSampleRate);

   if (ALoadWaveFile("vmmsnd/sfury9.wav",   &gSFury, 0))     printf("Error loading sfury9\n");
   if (ALoadWaveFile("vmmsnd/elim2.wav",    &gFire1, 0))     printf("Error loading elim2\n");
   if (ALoadWaveFile("vmmsnd/efire.wav",    &gFire2, 0))     printf("Error loading efire\n");
   if (ALoadWaveFile("vmmsnd/explode1.wav", &gExplode1, 0))  printf("Error loading explode1\n");
   if (ALoadWaveFile("vmmsnd/explode2.wav", &gExplode2, 0))  printf("Error loading explode2\n");
   if (ALoadWaveFile("vmmsnd/explode3.wav", &gExplode3, 0))  printf("Error loading explode3\n");
   if (ALoadWaveFile("vmmsnd/nuke1.wav",    &gNuke, 0))      printf("Error loading nuke1\n");

   AOpenVoices(1);
   ACreateAudioVoice(&hVoice);
   ASetVoiceVolume(hVoice, 64);
   ASetVoicePanning(hVoice, 127);

   return 1;
}


/********************************************************************
   Initialise SEAL - open audio device
********************************************************************/
void CloseSEAL(void)
{
   AFreeWaveFile(gSFury);
   AFreeWaveFile(gFire1);
   AFreeWaveFile(gFire2);
   AFreeWaveFile(gExplode1);
   AFreeWaveFile(gExplode2);
   AFreeWaveFile(gExplode3);
   AFreeWaveFile(gNuke);

   AStopVoice(hVoice);
   ADestroyAudioVoice(hVoice);
   ACloseVoices();

   ACloseAudio();
}
