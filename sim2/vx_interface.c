#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "game.h"

#include <vectrex/vectrexInterface.h>

#include "malban.h"

// choosing a fixed scale here and adjusting per game in draw_line.
// this scale lets us draw with a 1:1 aspect ratio, ie drawing a
// square 100 units on a side actually looks like a square.

// may need some adjustment for clipping off-screen in the vecterm package

#define SCREEN_W (1280 * 3 / 4)
#define SCREEN_H 1280

/*
 * no_interface.c: null display for Atari Vector game simulator
 *
 * Copyright 1993, 1996 Eric Smith
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: vx_interface.c,v 1.1 2018/07/31 01:19:45 pi Exp $
 */


#include "display.h"
#include "memory.h"


int smallwindow;
int window_width, window_height;
int yates_config = 0;
int onlyOnejoystick = 1;
char *ProgName = "NameUnset";

//#define SETTINGS_SIZE 1024
//unsigned char settingsBlob[SETTINGS_SIZE];
extern int vectrexinit (char viaconfig);

void gameCommand(void)
{
	printf("Atari Vector game simulator, Copyright 1993, 1996 Eric Smith\r\n");
}
Command vecSimCommandList[] =
{
	{1,"game", "g", "game | g -> display game information\r\n" ,  gameCommand },
	{0,"", "", "" ,  (void (*)(void)) 0 }
};


// for now INI setting just stupidly overwrite other saved settings!
static int simIniHandler(void* user, const char* section, const char* name, const char* value)
{
  // cascading ini files
  // first check if there are "general" entries
  if (iniHandler(user, section, name, value) == 1) return 1;

  
  #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  #define MATCH_NAME(n) strcmp(name, n) == 0
  
  if (MATCH("BATTLE_ZONE", "YATES_INPUT")) yates_config = atoi(value); 
  
  
/*
 * No special values as of yet
  if (MATCH("VECTREX_EXACT", "ROM_FILE")) strcpy(romName, value); else 
  {
      printf("Ini-Handler, unkown entry: %s = %s", name, value);
      return 0;  / * unknown section/name, error * /
  }
*/  
  return 1;
}
int OFFSET_X= 500;
int OFFSET_Y= 300;
int MUL_X= 35;
int MUL_Y= 40;

void setDimensions(int offsetx, int offsety, int mulx, int muly)
{
 OFFSET_X= offsetx;
 OFFSET_Y= offsety;
 MUL_X= mulx;
 MUL_Y= muly;
}

void init_graphics ( int p_smallwindow, int p_use_pixmap, int p_line_width, char *window_name)
{
  // INIT VECTREX GRAPHICS HERE
  
  vectrexinit(1);
  v_init();
//  v_loadSettings(ProgName, settingsBlob, SETTINGS_SIZE);
#ifdef PITREX_DEBUG
  userCommandList = vecSimCommandList;
#endif

  int pv = 0;
  if ((pv = ini_parse("sim.ini", simIniHandler, 0)) < 0) 
  {
        printf("sim.ini not loaded!\n\r");
  }
  else
    printf("sim.ini loaded!\n\r");
  int i;
  char iniBuf[80];
  for (i=0; ProgName[i]!=0;i++)
    iniBuf[i] = ProgName[i];
  iniBuf[i++] = '.';
  iniBuf[i++] = 'i';
  iniBuf[i++] = 'n';
  iniBuf[i++] = 'i';
  iniBuf[i++] = 0;
  pv = 0;
  if ((pv = ini_parse(iniBuf, simIniHandler, 0)) < 0) 
  {
        printf("%s not loaded!\n\r", iniBuf);
  }
  else
        printf("%s loaded!\n\r", iniBuf);
}

void term_graphics (void)
{
}

#define uses_analog_joystick(x) (((x) == LUNAR_LANDER) || ((x) == RED_BARON)|| ((x) == TEMPEST))


void (*customInputHandler) (void) = 0;

void handle_input (void) // call once per frame
{
//  printf("Normalinput handler: %p\n\r", customInputHandler);
  if (customInputHandler != 0)
  {
    customInputHandler();
    return;
  }
#define JOYSTICK_CENTER_MARGIN 20
  if (uses_analog_joystick(game))
  {
	// build a "0" zone

    joystick.x = currentJoy1X;  // 0x80   0  0x7f
    if ((currentJoy1X>0) && (currentJoy1X<JOYSTICK_CENTER_MARGIN)) joystick.x= 0;
    if ((currentJoy1X<0) && (currentJoy1X>-JOYSTICK_CENTER_MARGIN)) joystick.x=  0;
    joystick.y = currentJoy1Y;  // 0x80   0  0x7f
    if ((currentJoy1Y>0) && (currentJoy1Y<JOYSTICK_CENTER_MARGIN)) joystick.y=  0;
    if ((currentJoy1Y<0) && (currentJoy1Y>-JOYSTICK_CENTER_MARGIN)) joystick.y=  0;

  }
  else
  {
	// build a "0" zone

    joystick.x = currentJoy1X;  // 0x80   0  0x7f
    if ((currentJoy1X>0) && (currentJoy1X<JOYSTICK_CENTER_MARGIN)) joystick.x= 0;
    if ((currentJoy1X<0) && (currentJoy1X>-JOYSTICK_CENTER_MARGIN)) joystick.x=  0;
    joystick.y = currentJoy1Y;  // 0x80   0  0x7f
    if ((currentJoy1Y>0) && (currentJoy1Y<JOYSTICK_CENTER_MARGIN)) joystick.y=  0;
    if ((currentJoy1Y<0) && (currentJoy1Y>-JOYSTICK_CENTER_MARGIN)) joystick.y=  0;
  }

  start1 = currentButtonState & 0x01;  // button 1 on port 1
  start2 = currentButtonState & 0x02;  // button 2 on port 1

  if ((currentButtonState & 0x06) == 0x6) // button 2+3 together
  {
    cslot_left++;
    cslot_right++;
    cslot_util++;

    slam++; // what the heck is "slam"
  }
  self_test = 0; // not implemented

  if (game == BATTLEZONE)
    {
      
      if (yates_config)
      {
        switches [0].leftfwd = (currentButtonState & 0x10)?1:0; // button 1 port 2
        switches [0].leftrev = (currentButtonState & 0x20)?1:0; // button 2 port 2
        switches [0].rightfwd = (currentButtonState & 0x40)?1:0; // button 3 port 2
        switches [0].rightrev = (currentButtonState & 0x80)?1:0; // button 4 port 2
        switches [0].fire = currentJoy2X>JOYSTICK_CENTER_MARGIN;  //Joy2 analog x axis pulled high      
      }
      if (onlyOnejoystick)
      {
        switches [0].fire = (currentButtonState & 0x08)?1:0; // button 4 port 1
        switches [0].leftfwd = (currentJoy1Y>JOYSTICK_CENTER_MARGIN) || (currentJoy1X>JOYSTICK_CENTER_MARGIN);
        switches [0].leftrev = (currentJoy1Y<-JOYSTICK_CENTER_MARGIN) || (currentJoy1X<-JOYSTICK_CENTER_MARGIN);
        switches [0].rightfwd = (currentJoy1Y>JOYSTICK_CENTER_MARGIN) || (currentJoy1X<-JOYSTICK_CENTER_MARGIN);
        switches [0].rightrev = currentJoy1Y<-JOYSTICK_CENTER_MARGIN  || (currentJoy1X>JOYSTICK_CENTER_MARGIN);


          if ((currentJoy1X>JOYSTICK_CENTER_MARGIN) && (currentJoy1Y>JOYSTICK_CENTER_MARGIN))
          {
              switches [0].leftfwd = 1;
              switches [0].leftrev = 0;
              switches [0].rightfwd = 0;
              switches [0].rightrev = 0;
          }
          if ((currentJoy1X>JOYSTICK_CENTER_MARGIN) && (currentJoy1Y<JOYSTICK_CENTER_MARGIN))
          {
              switches [0].leftfwd = 0;
              switches [0].leftrev = 0;
              switches [0].rightfwd = 0;
              switches [0].rightrev = 1;
          }
          if ((currentJoy1X<-JOYSTICK_CENTER_MARGIN) && (currentJoy1Y>JOYSTICK_CENTER_MARGIN))
          {
              switches [0].leftfwd = 0;
              switches [0].leftrev = 0;
              switches [0].rightfwd = 1;
              switches [0].rightrev = 0;
          }
          if ((currentJoy1X<-JOYSTICK_CENTER_MARGIN) && (currentJoy1Y<JOYSTICK_CENTER_MARGIN))
          {
              switches [0].leftfwd = 0;
              switches [0].leftrev = 1;
              switches [0].rightfwd = 0;
              switches [0].rightrev = 0;
          }
     
      }
      else
      {
        switches [0].leftfwd = currentJoy1Y>JOYSTICK_CENTER_MARGIN;
        switches [0].leftrev = currentJoy1Y<-JOYSTICK_CENTER_MARGIN;
        switches [0].rightfwd = currentJoy2Y>JOYSTICK_CENTER_MARGIN;
        switches [0].rightrev = currentJoy2Y<-JOYSTICK_CENTER_MARGIN;
        switches [0].fire = (currentButtonState & 0x08)?1:0; // button 4 port 1
      }
      
      
    }
  else if (game == RED_BARON)
  {
      // center Joy is 0x80
      // max Joy is 0xc0
      // min Joy is 0x40
      signed char y = (signed char) ((signed char)joystick.y)/2;
      signed char x = (signed char) ((signed char)joystick.x)/2;
      joystick.y = 128 + y;
      joystick.x = 128 + x;
    
      switches [0].fire = (currentButtonState & 0x08)?1:0; // button 4 port 1
  }
  else if (game == LUNAR_LANDER)
  {
      switches [0].left = currentJoy1X<-JOYSTICK_CENTER_MARGIN;
      switches [0].right = currentJoy1X>JOYSTICK_CENTER_MARGIN;
      if (((signed char)joystick.y)<0) joystick.y = 0;
  }
  else  if (game == BLACK_WIDOW)
  {
#define bw_up    left
#define bw_down  right
#define bw_left  shield
#define bw_right fire
      // MOVE
      switches [0].bw_up = currentJoy1Y>30; // UP
      switches [0].bw_down = currentJoy1Y<-30; // DOWN
      switches [0].bw_right =  currentJoy1X<-30;// RIGHT
      switches [0].thrust = (currentButtonState & 0x04)?1:0; // not used
      switches [0].hyper = (currentButtonState & 0x02)?1:0; //not used
      switches [0].bw_left = currentJoy1X>30; // LEFT for BW
      switches [0].abort = 0; // no input

      // FIRE
      switches [1].bw_up = currentJoy2Y>30;
      switches [1].bw_down = currentJoy2Y<-30;
      switches [1].bw_right = currentJoy2X<-30; 
      switches [1].thrust = (currentButtonState & 0x40)?1:0; // not used
      switches [1].hyper = (currentButtonState & 0x20)?1:0; // not used
      switches [1].bw_left = currentJoy2X>30;
      switches [1].abort = 0; // no input
 
      // also uses 
      //  start1 = currentButtonState & 0x01;  // button 1 on port 1
      //  start2 = currentButtonState & 0x02;  // button 2 on port 1
      
    }
    else
    {
      switches [0].left = currentJoy1X<-JOYSTICK_CENTER_MARGIN;
      switches [0].right = currentJoy1X>JOYSTICK_CENTER_MARGIN;
      switches [0].fire = (currentButtonState & 0x08)?1:0; // button 4 port 1
      switches [0].thrust = (currentButtonState & 0x04)?1:0; // button 3 port 1
      switches [0].hyper = (currentButtonState & 0x02)?1:0; // button 2 port 1
      switches [0].shield = (currentButtonState & 0x01)?1:0; // button 1 port 1
      switches [0].abort = 0; // no input
      
      switches [1].left = currentJoy2X<-JOYSTICK_CENTER_MARGIN;
      switches [1].right = currentJoy2X>JOYSTICK_CENTER_MARGIN;
      switches [1].fire = (currentButtonState & 0x80)?1:0; // button 4 port 2
      switches [1].thrust = (currentButtonState & 0x40)?1:0; // button 3 port 2
      switches [1].hyper = (currentButtonState & 0x20)?1:0; // button 2 port 2
      switches [1].shield = (currentButtonState & 0x10)?1:0; // button 1 port 2
      switches [1].abort = 0; // no input
    }
}
extern int browseMode;
char simBrowseModeData2[80];
extern char displBrowseModeData[160];

char helperData[240];



void draw_line(int FromX, int FromY, int ToX, int ToY, int Colour15, int z) // z is 0:12 in lunar, colour is always 7
{
  if (z == 0) return; // MOVE, possibly to realign at 0,0
  // sign-extend
  ToX = ToX << 16;
  ToX = ToX >> 16;
  FromX = FromX << 16;
  FromX = FromX >> 16;
  ToY = ToY << 16;
  ToY = ToY >> 16;
  FromY = FromY << 16;
  FromY = FromY >> 16;
  FromY = 700-FromY; ToY = 700-ToY; // trial and error
  
  if ( (game == BATTLEZONE) || (game == RED_BARON))
  {
    if (browseMode)
    {
      sprintf(helperData, "%s%s", displBrowseModeData,simBrowseModeData2 );
      
      
      v_directDraw32HintedDebug((FromX - OFFSET_X)*MUL_X, (FromY - OFFSET_Y)*MUL_Y, (ToX- OFFSET_X)*MUL_X,(ToY- OFFSET_Y)*MUL_Y, z*8, 0, helperData);
      
      
      displBrowseModeData[0]=0;
      simBrowseModeData2[0]=0;
    }
    else
    {
      int intensity = z*8;
      if  (game == BATTLEZONE)
      {
        if ((((FromY - OFFSET_Y)*MUL_Y)>10000) && (intensity == 0x60))
          intensity = 0x40;
      }
      
      v_directDraw32((FromX - OFFSET_X)*MUL_X, (FromY - OFFSET_Y)*MUL_Y, (ToX- OFFSET_X)*MUL_X,(ToY- OFFSET_Y)*MUL_Y, intensity);
    }
  }
  else
  {
    if (browseMode)
    {
      sprintf(helperData, "%s%s", displBrowseModeData,simBrowseModeData2 );
      v_directDraw32HintedDebug((FromX - OFFSET_X)*MUL_X, (FromY - OFFSET_Y)*MUL_Y, (ToX- OFFSET_X)*MUL_X,(ToY- OFFSET_Y)*MUL_Y, z*4+Colour15*8, 0, helperData);
      displBrowseModeData[0]=0;
      simBrowseModeData2[0]=0;
    }
    else
      v_directDraw32((FromX - OFFSET_X)*MUL_X, (FromY - OFFSET_Y)*MUL_Y, (ToX- OFFSET_X)*MUL_X,(ToY- OFFSET_Y)*MUL_Y, z*4+Colour15*8);
  }
}

void draw_line2(int FromX, int FromY, int ToX, int ToY, int Colour15, int z) // z is 0:12 in lunar, colour is always 7
{
  if (z == 0) return; // MOVE, possibly to realign at 0,0
  
    if (browseMode)
    {
      sprintf(helperData, "%s%s", displBrowseModeData,simBrowseModeData2 );
      v_directDraw32HintedDebug(FromX, FromY, ToX,ToY, z*4+Colour15*8, 0, helperData);
      displBrowseModeData[0]=0;
      simBrowseModeData2[0]=0;
    }
    else
      v_directDraw32(FromX, FromY, ToX,ToY, z*4+Colour15*8);
}

void open_page (int step)
{
  v_playAllSFX(); // fill double buffers
  v_doSound(); // doublebuffer to PSG
  v_readButtons();
  v_readJoystick1Analog();
//  if (gameCallback != 0) gameCallback();
  v_WaitRecal();
  handle_input();

  if (browseMode)
  {
    displBrowseModeData[0]=0;
    simBrowseModeData2[0]=0;
  }
}


void close_page (void)
{
  // code executes faster than realtime, slow it down here to 50hz
  // (though 60hz would be correct speed)
  // OR WAIT FOR VECTREX 1/50Hz HERE?
}


int repeat_frame (void)
{
  return (false);
}


void bell (void)
{
}
