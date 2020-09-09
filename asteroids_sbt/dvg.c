/*
 * display.c: Atari DVG and AVG simulators
 *
 * Copyright 1991, 1992, 1996, 2003 Eric Smith
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
 * $Id: display.c 2 2003-08-20 01:51:05Z eric $


 2006-10-31 modified by dwelch

 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include <vectrex/vectrexInterface.h>
#include <pitrex/pitrexio-gpio.h>

#define MAXSTACK 4

#define VCTR 0
#define HALT 1
#define SVEC 2
#define STAT 3
#define CNTR 4
#define JSRL 5
#define RTSL 6
#define JMPL 7
#define SCAL 8

#define DVCTR 0x01
#define DLABS 0x0a
#define DHALT 0x0b
#define DJSRL 0x0c
#define DRTSL 0x0d
#define DJMPL 0x0e
#define DSVEC 0x0f

#define twos_comp_val(num,bits) ((num&(1<<(bits-1)))?(num|~((1<<bits)-1)):(num&((1<<bits)-1)))

extern unsigned char ram[0x8000];
#define ReadMemory(x)  (ram[x])

#define SETTINGS_SIZE 1024
unsigned char settingsBlob[SETTINGS_SIZE];

int textinit ( void )
{
#ifdef BROKEN
  vectrexinit(1);
  v_init(1);
  v_noSound();
#else
  vectrexinit(1);
  v_init();
  usePipeline = 1;
  v_setRefresh(60);
#endif
  v_loadSettings("asteroids_sbt", settingsBlob, SETTINGS_SIZE);
  return 0;
}
void handleSound();


#define SCREEN_H 1024
#define SCREEN_W 1024
char displBrowseModeData[120];
void draw_line ( long oldx, long oldy, long currentx, long currenty, int xyz, int z)
{
    if(z==0) return;

    oldy=SCREEN_H-oldy-250;
    currenty=SCREEN_H-currenty-250;

  if (browseMode)
  {
    v_directDraw32HintedDebug(oldx=(oldx - 512)*32, oldy=(oldy - 512)*32, currentx=(currentx - 512)*32,currenty=(currenty - 512)*32, z*8, 0, displBrowseModeData);
    displBrowseModeData[0]=0;
    
  }
  else
    v_directDraw32(oldx=(oldx - 512)*32, oldy=(oldy - 512)*32, currentx=(currentx - 512)*32,currenty=(currenty - 512)*32, z*8);

}


void draw_brightDot ( long oldx, long oldy, long currentx, long currenty, int xyz, int z)
{
    oldy=SCREEN_H-oldy-250;
    currenty=SCREEN_H-currenty-250;

int olddwell = v_dotDwell;
int ran = random()%200;
v_dotDwell = 200+ran;
  if (browseMode)
  {
    v_directDraw32HintedDebug(oldx=(oldx - 512)*32, oldy=(oldy - 512)*32, currentx=(currentx - 512)*32,currenty=(currenty - 512)*32, z*8, PL_BASE_FORCE_USE_DOT_DWELL, displBrowseModeData);
    displBrowseModeData[0]=0;
  }
  else
    v_directDraw32Hinted(oldx=(oldx - 512)*32, oldy=(oldy - 512)*32, currentx=(currentx - 512)*32,currenty=(currenty - 512)*32, z*8, PL_BASE_FORCE_USE_DOT_DWELL);
v_dotDwell = olddwell;
}



long keycode = 0L;

unsigned long readkeypad ( void )
{
  return keycode;
}

extern int browseMode;
void dvg_draw_screen (void)
{
    int pc;
    int sp;
    int stack [MAXSTACK];

    long scale;
    int statz;

    long currentx;
    long currenty;

    int done = 0;

    int firstwd, secondwd;
    int opcode;

    long x, y;
    int z, temp;
    int a;

    long oldx, oldy;
    long deltax, deltay;

    pc = 0;
    sp = 0;
    scale = 0;
    statz = 0;
    currentx = 1023 * 8192;
    currenty = 512 * 8192;
    //currentx = 512 * 8192;
    //currenty = 1023 * 8192;

    while (!done)
    {
        firstwd = ReadMemory(0x4000+(pc<<1)+1); firstwd<<=8; firstwd|=ReadMemory(0x4000+(pc<<1)+0);
        opcode = firstwd >> 12;
        pc++;
        if ((opcode >= 0 /* DVCTR */) && (opcode <= DLABS))
        {
            secondwd = ReadMemory(0x4000+(pc<<1)+1); secondwd<<=8; secondwd|=ReadMemory(0x4000+(pc<<1)+0);
            pc++;
        }


        switch (opcode)
        {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
                y = firstwd & 0x03ff; if (firstwd & 0x0400) y = -y;
                x = secondwd & 0x03ff;if (secondwd & 0x400) x = -x;
                z = secondwd >> 12;
                oldx = currentx; oldy = currenty;
                temp = (scale + opcode) & 0x0f;
                if (temp > 9) temp = -1;
                deltax = (x << 21) >> (30 - temp);
                deltay = (y << 21) >> (30 - temp);
                currentx += deltax;
                currenty -= deltay;
if (browseMode)
{
    sprintf(displBrowseModeData, "DVG PC=%04x, SP=%02x, X=%08x, Y=%08x, Z=%04x, A=%04x\r\n", pc, sp, x, y, z, a);
}

                draw_brightDot (oldx, oldy, currentx, currenty, 7, z);
//                draw_line (oldx, oldy, currentx, currenty, 7, z);
                break;

            case DLABS:
                x = twos_comp_val (secondwd, 12);
                y = twos_comp_val (firstwd, 12);
                /*
                x = secondwd & 0x07ff;
                if (secondwd & 0x0800)
                x = x - 0x1000;
                y = firstwd & 0x07ff;
                if (firstwd & 0x0800)
                y = y - 0x1000;
                */
                scale = secondwd >> 12;
                currentx = x;
                currenty = (896 - y);
                break;

            case DHALT:
                done = 1;
                break;

            case DJSRL:
                a = firstwd & 0x0fff;
                stack [sp] = pc;
                if (sp == (MAXSTACK - 1))
                {
                    printf ("\n*** Vector generator stack overflow! ***\n");
                    done = 1;
                    sp = 0;
                }
                else
                    sp++;
                pc = a;
                break;

            case DRTSL:
                if (sp == 0)
                {
                    printf ("\n*** Vector generator stack underflow! ***\n");
                    done = 1;
                    sp = MAXSTACK - 1;
                }
                else
                    sp--;
                pc = stack [sp];
                break;

            case DJMPL:
                a = firstwd & 0x0fff;
                pc = a;
                break;

            case DSVEC:
                y = firstwd & 0x0300; if (firstwd & 0x0400) y = -y;
                x = (firstwd & 0x03) << 8; if (firstwd & 0x04) x = -x;
                z = (firstwd >> 4) & 0x0f;
                temp = 2 + ((firstwd >> 2) & 0x02) + ((firstwd >> 11) & 0x01);
                oldx = currentx; oldy = currenty;
                temp = (scale + temp) & 0x0f;
                if (temp > 9) temp = -1;
                deltax = (x << 21) >> (30 - temp);
                deltay = (y << 21) >> (30 - temp);
                currentx += deltax;
                currenty -= deltay;
if (browseMode)
{
    sprintf(displBrowseModeData, "DVG PC=%04x, SP=%02x, X=%08x, Y=%08x, Z=%04x, A=%04x\r\n", pc, sp, x, y, z, a);
}
                draw_line (oldx, oldy, currentx, currenty, 7, z);
                break;

            default:
                printf("internal error\n");
                done = 1;
        }
    }
    // page_flip();
    // should wait for 50hz timer here
    // INSERT PITREX TIMING CODE HERE
    
    v_WaitRecal();
    handleSound();
    v_playAllSFX();
    v_doSound();
//poll_buttons();  // currently buttons are non-responsive. trying to work out why...
/*void poll_buttons(void)*/ {
  int mx, my;
  keycode = 0L;    // vecterm_get_buttons(); // READ ONCE PER FRAME

  v_readButtons();
  v_readJoystick1Analog();

  mx = currentJoy1X*5/2; // crude but the best I've found
  my = currentJoy1Y*5/2;

  if ((mx < -50) || (currentButtonState&1)) keycode |= 0x00001000; // ROTATE LEFT
  if ((mx > 50)  || (currentButtonState&2)) keycode |= 0x00002000; // ROTATE RIGHT
  if (((my > 50) && (mx > -40) && (mx < 40)) || (currentButtonState&4)) keycode |= 0x00800000; // THRUST
  if (currentButtonState&8) keycode |= 0x00400000; // FIRE
  if (((my < -90) && (mx > -40) && (mx < 40)) || ((keycode&0x00C00000) == 0x00C00000)) keycode = 0x00000100;
  // Not yet implemented: START/HYPERSPACE when THRUST+FIRE both pressed or joystick down -
  //   - combination is problematic as people are likely to try to fire while moving.  Also looked
  //     at, but not implemented, joystick-down as a hyperspace button.
  // Need to pick something before release!
  // These values are passed back to the game via readkeypad() above
}
}

int explosionShadow=0; // 0x3600
int thumbShadow=0; // 0x3a00
int latch0Saucer=0;  //0x3c00
int latch1SaucerFire=0; //0x3c01 
int latch2SaucerSel=0; // 0x3c02
int latch3Thrust=0; //0x3c03
int latch4ShipFire=0;// 0x3c04
int latch5Life=0;// 0x3c05
int latch6Unkown=0;// 0x3c06
int latch7Unkown=0;// 0x3c07

// SOUNDS
// AYFX - Data of file[]= "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_NewLife.afx"
unsigned char Ast_NewLife_data[]=
{
 0xEC, 0x6C, 0x00, 0x00, 0xAE, 0x20, 0x00, 0x8E, 0x8E, 0x80,
 0x80, 0x80, 0x8E, 0x8E, 0x8E, 0x80, 0x80, 0x80, 0x8E, 0x8E,
 0x8E, 0x80, 0x80, 0x80, 0x8E, 0x8E, 0x8E, 0x80, 0x80, 0x80,
 0x8E, 0x8E, 0x8E, 0x80, 0x80, 0x80, 0x8E, 0x8E, 0x8E, 0x80,
 0x80, 0x80, 0x8E, 0x8E, 0x8E, 0x80, 0x80, 0x80, 0x8E, 0x8E,
 0x8E, 0x80, 0x80, 0x80, 0x8E, 0x8E, 0x8E, 0x80, 0x80, 0x8E,
 0x8E, 0x8E, 0x80, 0x80, 0x80, 0x8E, 0x8E, 0x8E, 0x80, 0x80,
 0x80, 0xB0, 0x00, 0x00, 0xD0, 0x20
 };
 // AYFX - Data of file[]= "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_PlayerAstExplosion.afx"
unsigned char Ast_PlayerAstExplosion_data[]=
{
   0x6F,  0x00,  0x02,  0x1F,  0x0F,  0x0F,  0x0F,  0x0F,  0x0E,  0x0E,
   0x0E,  0x0D,  0x0D,  0x0D,  0x0C,  0x0C,  0x0B,  0x0B,  0x0B,  0x0B,
   0x0A,  0x0A,  0x0A,  0x09,  0x09,  0x09,  0x08,  0x08,  0x08,  0x07,
   0x07,  0x07,  0x07,  0x06,  0x06,  0x06,  0x06,  0x05,  0x05,  0x05,
   0xD0,  0x20
 };
 // AYFX - Data of file[]= "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_PlayerDeath.afx"
unsigned char Ast_PlayerDeath_data[]=
{
   0xF0,  0x00,  0x00,  0x00,  0x90,  0x90,  0x90,  0x90,  0x90,  0x90,
   0x90,  0x90,  0x90,  0x90,  0x90,  0x90,  0x90,  0x2F,  0x00,  0x02,
   0x2F,  0x00,  0x06,  0x0F,  0x0F,  0x0F,  0x0F,  0x0F,  0x0F,  0x0F,
   0x0E,  0x0E,  0x0E,  0x0E,  0x0E,  0x0E,  0x0E,  0x0E,  0x0D,  0x0D,
   0x0D,  0x0D,  0x0D,  0x0D,  0x0D,  0x0D,  0x0C,  0x0C,  0x0C,  0x0C,
   0x0C,  0x0C,  0x0C,  0x0C,  0x0B,  0x0B,  0x0B,  0x0B,  0x0B,  0x0B,
   0x0B,  0x0B,  0x0A,  0x0A,  0x0A,  0x0A,  0x0A,  0x0A,  0x0A,  0x0A,
   0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x08,  0x08,
   0x08,  0x08,  0x08,  0x08,  0x08,  0x08,  0x07,  0x07,  0x07,  0x07,
   0x07,  0x07,  0x07,  0x07,  0x07,  0x07,  0x07,  0x06,  0x06,  0x06,
   0x06,  0x06,  0x06,  0x06,  0x06,  0x06,  0x06,  0x06,  0x06,  0x05,
   0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,
   0x05,  0x04,  0x04,  0x04,  0x04,  0x04,  0x04,  0x04,  0x04,  0x04,
   0x04,  0x04,  0x04,  0x03,  0x03,  0x03,  0x03,  0x03,  0x03,  0x03,
   0x03,  0x03,  0x03,  0x03,  0x03,  0x03,  0x03,  0x03,  0x03,  0x02,
   0x02,  0x02,  0x02,  0x02,  0x02,  0x02,  0x02,  0x02,  0x02,  0x02,
   0x02,  0x02,  0x02,  0x02,  0x02,  0x01,  0x01,  0x01,  0x01,  0x01,
   0x01,  0x01,  0x01,  0x01,  0x01,  0x01,  0x01,  0x01,  0x01,  0x01,
   0xD0,  0x20
 };
 // AYFX - Data of file[]= "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_PlayerShot.afx"
unsigned char Ast_PlayerShot_data[]=
{
  0xEC, 0x6C, 0x00, 0x00, 0xAD, 0x71, 0x00, 0xAE, 0x78, 0x00,
  0xAE, 0x7F, 0x00, 0xAD, 0x87, 0x00, 0xAD, 0x9A, 0x00, 0x8C,
  0xAC, 0xB4, 0x00, 0xAC, 0xC4, 0x00, 0xAC, 0xF0, 0x00, 0xAC,
  0x0E, 0x01, 0xAC, 0x68, 0x01, 0xB0, 0x00, 0x00, 0xD0, 0x20,
 };
 
 // AYFX - Data of file[]= "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_Saucer.afx"
unsigned char Ast_Saucer_data[]=
{
  0xEC, 0x49, 0x00, 0x00, 0xAC, 0x40, 0x00, 0xAC, 0x39, 0x00,
  0xAC, 0x4C, 0x00, 0xAB, 0x6C, 0x00, 0xAB, 0x93, 0x00, 0xAB,
  0x72, 0x00, 0xAB, 0x55, 0x00, 0xB0, 0x00, 0x00, 0xD0, 0x20

 };
 // AYFX - Data of file[]= "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_SaucerAstExplosion.afx"
unsigned char Ast_SaucerAstExplosion_data[]=
{
   0x6F,  0x00,  0x02,  0x1F,  0x0F,  0x0F,  0x0F,  0x0F,  0x0E,  0x0E,
   0x0E,  0x0E,  0x0D,  0x0D,  0x0D,  0x0C,  0x0C,  0x0B,  0x0B,  0x0B,
   0x0B,  0x0A,  0x0A,  0x0A,  0x09,  0x09,  0x09,  0x08,  0x08,  0x08,
   0x07,  0x07,  0x07,  0x07,  0x06,  0x06,  0x06,  0x06,  0x05,  0x05,
   0x05,  0xD0,  0x20
 };
 // AYFX - Data of file[]= "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_SaucerShot.afx"
unsigned char Ast_SaucerShot_data[]=
{
  0xEC, 0x40, 0x00, 0x00, 0xAC, 0x39, 0x00, 0xAC, 0x4C, 0x00,
  0xAB, 0x6C, 0x00, 0xAB, 0x93, 0x00, 0xAB, 0x72, 0x00, 0xAB,
  0x55, 0x00, 0xB0, 0x00, 0x00, 0xD0, 0x20
};
 // AYFX - Data of file[]= "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_ThumbHi.afx"
unsigned char Ast_ThumbHi_data[]=
{
   0xEF, 0x97, 0x03, 0x00, 0x8E, 0x8D, 0xB0, 0x00, 0x00, 0xD0,  0x20,
 };
 
 // AYFX - Data of file[]= "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_ThumbLo.afx"
unsigned char Ast_ThumbLo_data[]=
{
   0xEF, 0x00, 0x04, 0x00, 0x8E, 0x8D, 0xB0, 0x00, 0x00, 0xD0,  0x20,
};

// AYFX - Data of file[]= "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_ThumbSaucer.afx"
unsigned char Ast_ThumbSaucer_data[]=
{
   0xEF,  0x00,  0x05,  0x00,  0x8E,  0x8D,  0xD0,  0x20 ,
 };
 // AYFX - Data of file[]= "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_PlayerThrust.afx"
unsigned char Ast_PlayerThrust_data[]=
{
   0x7C,  0x00,  0x00,  0x00,  0x1C,  0x1C,  0xD0,  0x20,
};
// AYFX - Data of file: "/Users/chrissalo/NetBeansProjects/Vide/tmp/AstSound/Ast_PlayerThrustOff.afx"
 unsigned char Ast_PlayerThrustOff_data[]=
{
  0xF0, 0x00, 0x00, 0x00, 0xD0, 0x20
};
 
 int explosionOngoing = 0;

/* CHANNELS:
 * top is higher priority within channel
 * 
 * 0 Player LIFE
 *   Player Fire
 *   Saucer Fire
 * 
 * 1 Saucer sound
 *   Thrust
 * 
 * 2 Explosions
 *   Thumb
 */
void handleSound()
{
  // CHANNEL 2
  if ((explosionShadow == ReadMemory(0x3600)) && (explosionOngoing))
  {
    explosionOngoing = 0;
  }
  if (explosionShadow != ReadMemory(0x3600)) 
  {
//printf("Sound EXP: %02x -> %02x \r\n", explosionShadow,ReadMemory(0x3600));
    if (ReadMemory(0x21B) == 0xA0)// && (explosionShadow != 0x81))// player explosion
    {
      if (explosionOngoing!=2)
      {
        explosionOngoing = 2;
        v_playSFXCont(Ast_PlayerDeath_data, 0, 0);
        v_playSFXCont(Ast_PlayerDeath_data, 1, 0);
        v_playSFXCont(Ast_PlayerDeath_data, 2, 0);
      }
    }
    if (explosionOngoing==0) 
    {
      explosionOngoing = 1;
      v_playSFXStart(Ast_PlayerAstExplosion_data, 2, 0);
    }
  }

  if (thumbShadow != ReadMemory(0x3a00))
  {
    /*
    printf("Sound Thumb: %02x -> %02x \r\n", thumbShadow,ReadMemory(0x3a00));
    Sound Thumb: 00 -> 14 
    Sound Thumb: 14 -> 04 
    Sound Thumb: 04 -> 10 
    Sound Thumb: 10 -> 00     
    */
    if (!explosionOngoing)
    {
      if (ReadMemory(0x3c01) == 0) // no saucer fire
      {
        if (ReadMemory(0x3a00) == 0x14) v_playSFXCont(Ast_ThumbHi_data, 2, 0);
        if (ReadMemory(0x3a00) == 0x10) v_playSFXCont(Ast_ThumbLo_data, 2, 0);
      }
    }
  }
  
  // CHANNEL 1
  if (latch0Saucer != ReadMemory(0x3c00))
  {
  }
  if (ReadMemory(0x3c00) != 0)
  {
	 // will restart when saucer is still active due to 0x3c00 "watch"
	 v_playSFXCont(Ast_Saucer_data, 1, 0);
  }

  if (latch3Thrust != ReadMemory(0x3c03))
  {
  }
  if (ReadMemory(0x3c03) != 0)
  {
	 if (!ReadMemory(0x3c00))
	  v_playSFXCont(Ast_PlayerThrust_data, 1, 0);
  }
  else
  {
	 if (!ReadMemory(0x3c00))
	  v_playSFXCont(Ast_PlayerThrustOff_data, 1, 0);
  }

  // CHANNEL 0  
  if (latch5Life != ReadMemory(0x3c05))
  {
    if (ReadMemory(0x3c05) != 0x00)
        v_playSFXCont(Ast_NewLife_data, 0, 0);
  }

  if (latch4ShipFire != ReadMemory(0x3c04))
  {
    if (ReadMemory(0x3c04) == 0x80)
    {
        if (ReadMemory(0x3c05) == 0x00)
          v_playSFXCont(Ast_PlayerShot_data, 0, 0);
    }
  }
  if (latch1SaucerFire != ReadMemory(0x3c01))
  {
    if (ReadMemory(0x3c01) !=0)
    {
      if (ReadMemory(0x3c05) == 0x00)
        if (ReadMemory(0x3c04) == 0x00)
          v_playSFXCont(Ast_SaucerShot_data, 0, 0);
    }
  }

  if (latch2SaucerSel != ReadMemory(0x3c02))
    printf("Sound Sel: %02x -> %02x \r\n!!!!!!!!!!!!!!\r\n", latch2SaucerSel,ReadMemory(0x3c02));

  explosionShadow = ReadMemory(0x3600);
  thumbShadow = ReadMemory(0x3a00);
  latch0Saucer = ReadMemory(0x3c00);
  latch1SaucerFire = ReadMemory(0x3c01);
  latch2SaucerSel = ReadMemory(0x3c02);
  latch3Thrust = ReadMemory(0x3c03);
  latch4ShipFire = ReadMemory(0x3c04);
  latch5Life = ReadMemory(0x3c05);
}
