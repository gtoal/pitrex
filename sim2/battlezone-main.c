/*
https://6502disassembly.com/va-battlezone/



audio_indices   .eq     $ef    {addr/4}   ;index into sfx table for 4 channels
                   audio_values    .eq     $f3    {addr/4}   ;values for AUDF1, AUDC1, AUDF2, AUDC2
                   audio_dur_ctr   .eq     $f7    {addr/4}   ;audio duration counter
                   audio_rep_ctr   .eq     $fb    {addr/4}   ;audio repetition counter

                   
                   
5126: a9 02        :SkipCheck      lda     #$02              ;flash messages based on game frame
5128: 25 c6                        and     frame_counter
512a: f0 1e                        beq     :SkipEnMsg        ;currently not showing, branch
512c: a5 d0                        lda     enemy_ang_delt_abs ;check angle to enemy
512e: c9 16                        cmp     #$16
5130: 90 18                        bcc     :SkipEnMsg        ;it's in front, bail                   
            
            
5194: 2c 45 03                     bit     score_100k_flag   ;just broke 100K points?
5197: 30 29                        bmi     :Play100kBoom     ;yes, try to play post-1812 Overture explosion            
            
51c2: ad ed 02     :Play100kBoom   lda     strange_one_a     ;#$01; if it's not zero or one, we will never clear
51c5: 4a                           lsr     A                 ; the "1812 Overture is playing" flag
51c6: 05 ef                        ora     audio_indices     ;nonzero if playing sound on channel 1
51c8: d0 f5                        bne     :JumpMainLoop     ;1812 Overture still playing, don't do explosion yet
51ca: 8d 45 03                     sta     score_100k_flag   ;A-reg=0
51cd: a5 15                        lda     dsnd_ctrl_val
51cf: 09 02                        ora     #$02              ;d-sound: explosion=loud
51d1: 85 15                        sta     dsnd_ctrl_val
51d3: a9 ff                        lda     #$ff              ;play for ~1 sec
51d5: 85 0f                        sta     dsnd_expl_ctr
51d7: d0 e6                        bne     :JumpMainLoop            
            
            
5371: a9 80                        lda     #$80              ;sound: 1812 Overture
5373: 20 85 79                     jsr     StartSoundEffect            
            
55e1: 20 85 79     :PlaySound      jsr     StartSoundEffect
                   ; Update discrete sounds.  The value in $15 holds the bits for cannon volume,
                   ; explosion volume, and engine rev up/down.  The bits for actually playing
                   ; cannon and explosion sounds are set here, based on countdown timers.
55e4: a5 0f        :Cont           lda     dsnd_expl_ctr     ;are we playing the explosion sound?            
            
55f3: a5 bd                        lda     dsnd_cnon_ctr     ;are we playing the cannon firing sound?
55f5: f0 08                        beq     :NoCannon         ;no, branch
55f7: c6 bd                        dec     dsnd_cnon_ctr     ;yes, decrement the play counter            

5652: a9 00                        lda     #$00
5654: 8d 2f 18                     sta     POKEY_SKCTL       ;disable input
5657: 85 ec                        sta     bonus_coin_count  ;reset bonus coin counter
5659: 8d 4b 03                     sta     attract_logo_flag ;not showing the logo
565c: 8d 40 18                     sta     DSOUND_CTRL       ;disable all sound
565f: 85 b8                        sta     score             ;reset score


6d2e: 20 6a 7a     DrawScoreLives  jsr     VgCenter          ;center beam



                   ; 
                   ; For each sound effect, these are the indices into the SFX audio data table for
                   ; the start of the stream that drives AUDF1, AUDC1, AUDF2, and AUDC2.  Zero
                   ; values indicate the register is not used by this effect.
                   ; 
7864: 00 00 15 1b  sfx_indices     .bulk   $00,$00,$15,$1b   ;$01
7868: 2d 57 00 00                  .bulk   $2d,$57,$00,$00   ;$02
786c: 21 27 00 00                  .bulk   $21,$27,$00,$00   ;$04
7870: 00 00 61 67                  .bulk   $00,$00,$61,$67   ;$08
7874: 00 00 01 0f                  .bulk   $00,$00,$01,$0f   ;$10
7878: 85 8f 00 00                  .bulk   $85,$8f,$00,$00   ;$20
787c: 95 a7 00 00                  .bulk   $95,$a7,$00,$00   ;$40
7880: ad d3 d9 d3                  .bulk   $ad,$d3,$d9,$d3   ;$80
7884: 00           sfx_zero_a      .dd1    $00
7885: 00           sfx_zero_b      .dd1    $00


                   ; Initiates a sound effect on audio channel 1 and/or 2.
                   ; 
                   ; The effect is chosen by setting a single bit in the A-reg.  Setting A-reg to
                   ; zero causes the value in the stack pointer to be used instead (this does not
                   ; appear to be used by Battlezone).
                   ; 
                   ;   $01: ch1: radar ping
                   ;   $02: ch1: collided with object
                   ;   $04: ch2: quiet "merp"
                   ;   $08: ch2: extra life notification (4 high-pitched beeps)
                   ;   $10: ch2: new enemy alert (three boops)
                   ;   $20: ch1: saucer hit (played in a loop while saucer fades out)
                   ;   $40: ch1: saucer sound (played in a loop while saucer alive)
                   ;   $80: ch1+2: nine notes from 1812 Overture
                   ; 
                   ; On entry:
                   ;   A-reg: effect choice
                   ; 
                   ; On exit:
                   ;   A/X/Y preserved
                   ; 
                   StartSoundEffect
7985: 48                           pha                       ;push A-reg




Battle Zone 
- vectors "crunch"
- vectrex interface "switch" environments (maximum distance etc...) ... and switch back
- vectrex interface add "force calibration" right
- vectrex interface add deflok
- add astroids ultra bright
- add sounds to battle zone / asteroids
- do a "slow" savety version of the pipeline
- do dipswitch settings
*/


/*
 * main.c: Atari Vector game simulator
 *
 * Copyright 1991-1993, 1996, 2003 Hedley Rainnie, Doug Neubauer, and Eric Smith
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
 * $Id: main.c,v 1.1 2018/07/31 01:19:45 pi Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../sim/memory.h"
#include "../sim/game.h"
#include "../sim/display.h"
#include "../sim/sim6502.h"

#include <vectrex/osWrapper.h>
#include <vectrex/vectrexInterface.h>

#ifndef FREESTANDING
#include "malban.h"
#endif

char gameMemory[4*65536];
extern void setCustomClipping(int enabled, int x0, int y0, int x1, int y1);
//extern int bufferType; // 0 = none, 1 = double buffer, 2 = auto buffer (if pipeline is empty -> use previous (battlezone!)


// in vx_interface.c
extern int yates_config;
extern int onlyOnejoystick;

// for now INI setting just stupidly overwrite other saved settings!
static int bzIniHandler(void* user, const char* section, const char* name, const char* value)
{
  // cascading ini files
  // first check if there are "general" entries
  if (iniHandler(user, section, name, value) == 1) return 1;

  
  #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  #define MATCH_NAME(n) strcmp(name, n) == 0
  
  if (MATCH("BATTLE_ZONE", "YATES_INPUT")) yates_config = atoi(value); 
  if (MATCH("BATTLE_ZONE", "ONLY_ONE_JOYSTICK")) onlyOnejoystick = atoi(value); 

  return 1;
}

int useSamples=1;

unsigned char fire1Sample[10000];
int fire1Size;
int fire1Rate = 10000;

unsigned char fire2Sample[10000];
int fire2Size;
int fire2Rate = 10000;

unsigned char explosion1Sample[30000];
int explostion1Size;
int explosion1Rate = 10000;

unsigned char explosion2Sample[20000];
int explostion2Size;
int explosion2Rate = 10000;

unsigned char engine1Sample[4000];
int engine1Size;
int engine1Rate = 10000;

unsigned char engine2Sample[4000];
int engine2Size;
int engine2Rate = 10000;

//AM_RANGE(0x1840, 0x1840) AM_WRITE(bzone_sounds_w)



extern void (*gameCallback)(int);
int oldDiscreteSound = 0;
static inline void handleDiscretSound()
{
  /*
  DSOUND_CTRL     .eq     $1840             ;W control discrete sound H/W (see file comment)

                   *   DSOUND_CTRL ($1840):                                                       *
                   *     $80: motor enable - 1=engine sound enabled                               *
                   *     $40: start LED - controls LED on cabinet start button                    *
                   *     $20: sound enable - 0=mute all sound (incl. POKEY)                       *
                   *     $10: engine rev - 0=rev down, 1=rev up                                   *
                   *     $08: cannon fire volume - 0=soft 1=loud                                  *
                   *     $04: cannon fire enable - set to 1 while cannon sound playing            *
                   *     $02: explosion volume - 0=soft 1=loud                                    *
                   *     $01: explosion enable - set to 1 while explosion sound playing 
*/
  static int explosionPlaying = 0;
  static int firePlaying = 0;
  static int enginePlaying = 0;

// engine for now is LEFT OUT
// it should be "mixed" with the
// other discrete channels - 
// this is not working yet
// and I stopped after a few minutes :-)  
  
  
#define CORRECT_DISCRETE() \
  do {\
 /* if (v_getSample2Playing()==0) {enginePlaying=0;} */ \
  if (v_getSamplePlaying()==0) {explosionPlaying =0;firePlaying =0;}\
  if (!firePlaying) mem[0x1840].cell = mem[0x1840].cell & (~(0x04));\
  if (!enginePlaying) mem[0x1840].cell = mem[0x1840].cell & (~(0x80));\
  if (!explosionPlaying) mem[0x1840].cell = mem[0x1840].cell & (~(0x01));\
  oldDiscreteSound = currentDiscreteSound; \
  }while (0)
    
  unsigned char currentDiscreteSound = mem[0x1840].cell;
//  printf("Discrete: %02x", currentDiscreteSound);
  if ((currentDiscreteSound&0x20) == 0)
  {
    v_stopSamplePlaying();
    CORRECT_DISCRETE();
    return;
  }
  // new sound triggered
  if (((oldDiscreteSound & 0x04) == 0) && ((currentDiscreteSound & 0x04) == 0x04) )
  {
    // trigger new shot
    explosionPlaying = 0;
    firePlaying = 1;
    if (currentDiscreteSound&8)
    {
      // play loud sample
      v_playIRQSample(fire1Sample, fire1Size, fire1Rate, PLAY_ONCE);
    }
    else
    {
      // play soft sample
      v_playIRQSample(fire2Sample, fire2Size, fire2Rate, PLAY_ONCE);
    }
  }
  else
  // new sound triggered
  if (((oldDiscreteSound & 0x01) == 0) && ((currentDiscreteSound & 0x01) == 0x01) )
  {
    explosionPlaying = 1;
    firePlaying = 0;
    
    if (currentDiscreteSound&0x10)
    {
      // rev up
      v_playIRQSample(explosion1Sample, explostion1Size, explosion1Rate, PLAY_ONCE);
    }
    else
    {
      // rev down
      v_playIRQSample(explosion2Sample, explostion2Size, explosion2Rate, PLAY_ONCE);
    }
  }
/*
  
  // engine on sample channel 2
  if (((oldDiscreteSound & 0x80) == 0) && ((currentDiscreteSound & 0x80) == 0x80) )
  {
    enginePlaying = 1;
    
    if (currentDiscreteSound&2)
    {
      // play loud sample
      v_playIRQSample2(engine1Sample, engine1Size, engine1Rate, PLAY_ONCE);
    }
    else
    {
      // play soft sample
      v_playIRQSample2(engine2Sample, engine1Size, engine1Rate, PLAY_ONCE);
    }
  }
*/  
  CORRECT_DISCRETE();  
}


void battleZoneCallback(int type)
{
  if (type == 1)
  {
    handleDiscretSound();
  }

}



void skipInPipeline(VectorPipelineBase ** cpbPointer, int count)
{
  VectorPipelineBase *cpb = *cpbPointer;
  for (int i=0; i<count;i++)
  {
    cpb = cpb->next;
    if (cpb==0) break;
  }
  *cpbPointer = cpb;
}



typedef struct 
{
  int count;
  signed int xOffset;
  signed int xStart;
  signed int yStart;
  signed int xEnd;
  signed int yEnd;
} SaveVectorTest;

SaveVectorTest singleInRange[4] = 
{
  {47,0,     -14980,15040,-14700,15040}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};

SaveVectorTest enemyRight[4] = 
{
  {43,0,     -10640,12960,-10640,12480}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};
SaveVectorTest enemyLeft[4] = 
{
  {35,0,     -11200,12960,-11200,12480}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};
SaveVectorTest enemyRear[4] = 
{
  {43,0,     -10360,12480,-10360,12800}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};

SaveVectorTest motionText[4] = 
{
  {91,0,     -14980,13760,-14980,14240}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};

SaveVectorTest scoreText[4] = 
{
  {-1, 0,     4900,13440, 5460,13440}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};
SaveVectorTest pressStartText[4] = 
{
  {42, 0,    -4340,640, -4340,1600}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};
SaveVectorTest gameOverText[4] = 
{
  {41, 0,    3360,4960, 3780,4480}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};
SaveVectorTest greatScoreText[4] = 
{
  {41, 0,    -8540,4480, -8540,5440}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};
SaveVectorTest highScoreText[4] = 
{
  {41, 0,    -3500,7040, -3500,8000}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};
SaveVectorTest smallHighScoreText[4] = 
{
  {-1, 0,    4900,11840, 4900,12320}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};
// score upper right corner - last vector of E
SaveVectorTest scoreInGameText[4] = 
{
  {-1, 0,    8680,13920, 8260,13920}, // english
  {75,-3000, -3500,13500,-2940,13500}, // german
  {60,-2000, -3500,13500,-3500,14300}, // french
  {43,-500,  -3500,13500,-3500,14700}, // spanish
};


int shrinkCounter =0;    
unsigned char dipLanguageSetting = 0; // english
int startSingleEnemyCounter =0;

int vpInit_singleEnemy=0;
int vpInit_leftEnemy=0;
int vpInit_rightEnemy=0;
int vpInit_rearEnemy=0;
int vpInit_motion=0;
int vpInit_pressStart = 0;


int rightCounter = 0;
int leftCounter = 0;
int rearCounter = 0;
int motionCounter = 0;
int pressStartCounter = 0;

int highscoreSaved = 1;
int highscoreLoaded = 0;


unsigned char scoreBuf[60];
static inline void saveHighScore()
{
  if (highscoreSaved)return;

  
  for (int i=0;i<60;i++)
  {
    scoreBuf[i] = mem[0X300+i].cell;
  }

  highscoreSaved = 1;
  FILE *out=0;
  out = fopen("battlezone.scr", "w"); 
  if (out == 0) 
  {
    printf("Can't save highscore. File Error.");
    return;
  }

  unsigned int lenSaved=0;
  lenSaved = fwrite(scoreBuf, 60, 1, out); // 10 * 2 byte score
  fclose(out);
  printf("Highscore saved.");
}

static inline void loadHighScore()
{
  if (highscoreLoaded) return;
  
  highscoreLoaded = 1;
  FILE *fileRead;
  fileRead = fopen("battlezone.scr", "rb");
  if (fileRead == 0)
  {
/*
    unsigned char defaultHS[]= 
    {
      0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
      0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x17, 0x0B, 0x16, 0x17, 0x0B, 0x16, 0x17, 0x0B, 0x16, 
      0x17, 0x0B, 0x16, 0x17, 0x0B, 0x16, 0x17, 0x0B, 0x16, 0x17, 0x0B, 0x16, 0x17, 0x0B, 0x16, 0x17, 
      0x0B, 0x16, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD9
    };
    for (int i=0;i<64;i++)
    {
      ram[0x1d+i] = defaultHS[i];
    }
*/    
    return;
  }
  unsigned int lenLoaded=0;
  lenLoaded = fread(scoreBuf, 60, 1, fileRead);   // 10 * 2 byte score
  fclose(fileRead);

  for (int i=0;i<60;i++)
  {
    mem[0X300+i].cell = scoreBuf[i];
  }
  printf("Highscore loaded.");
}

VectorPipelineBase singleEnemySave[100];
VectorPipelineBase leftEnemySave[100];
VectorPipelineBase rightEnemySave[100];
VectorPipelineBase rearEnemySave[100];
VectorPipelineBase motionSave[200];
VectorPipelineBase pressStartSave[100];

static inline void saveList(VectorPipelineBase *from, int count, VectorPipelineBase *to, int offset, int intensity)
{
  if (offset <0)
  {
    for (int i=0;i<-offset;i++)
    {
      from = from->previous;
    }
  }
  
  for (int i=0;i<count;i++)
  {
    to[i].yStart = from->yStart;
    to[i].xStart = from->xStart;
    to[i].yEnd = from->yEnd;
    to[i].xEnd = from->xEnd;
    to[i].y0 = from->y0;
    to[i].x0 = from->x0;
    to[i].y1 = from->y1;
    to[i].x1 = from->x1;
    to[i].pattern = from->pattern;
    to[i].intensity = intensity;//cpb->intensity;// -20;
    
    to[i].sms = from->sms;
    to[i].timingForced = from->timingForced;
    to[i].force = from->force;
    to[i].debug[0] = 0;
    
    if (i==count-1)
      to[i].next = 0;
    else
      to[i].next = &to[i+1];

    if (i==0)
      to[i].previous = 0;
    else
      to[i].previous = &to[i-1];

    from = from->next;
  }
}

static inline void addListNext(VectorPipelineBase ** to, int count, VectorPipelineBase *from)
{
  VectorPipelineBase *oldNext = (*to)->next;

  (*to)->next = &from[0];
  from[0].previous = (*to);
  from[count-1].next = oldNext;
  
  if (oldNext != 0)
    oldNext->previous = &from[count-1];
}

// 0 offset adds HERE (replacing the current here)
// 1 adds next
// 2 ...
static inline void addList(VectorPipelineBase ** to, int count, VectorPipelineBase *from, int offset)
{
  // insert the list at "offset" vectors ahead into the pipeline
  VectorPipelineBase **to2 = to;
  for (int i=0;i<offset-1; i++)
  {
   to2 = (VectorPipelineBase **) &((*to2)->next);
  }
  if (offset!=0) 
  {
    addListNext(to, count, from);
    return;
  }
  // replace self
  
  VectorPipelineBase *oldPrevious = (*to2)->previous;
  VectorPipelineBase *oldNext = (*to2);

  from[0].previous = oldPrevious;
  from[count-1].next = oldNext;
  
  if (oldPrevious != 0)
    oldPrevious->next = &from[0];

  if (oldNext != 0)
    oldNext->previous = &from[count-1];
  *to2 = &from[0];
}

int anyAdditionalTextDetected = 0;
int insertDone = 0;
int singleDetected =0;
int singleInsertDone = 0;
int additionalInsertDone = 0;
int motionDetected = 0;
int motionInserted = 0;
int pressStartInserted = 0;
int pressStartDetected = 0;
int replaceDone = 0;

int checkForSpecialVectors(VectorPipelineBase ** cpbPointer,VectorPipeline **plPointer, int *fpc, int *cx, int *cy) 
{
  VectorPipeline *pl=*plPointer;
  VectorPipelineBase *cpb = *cpbPointer;
  
  // on first vector in draw list, reset some stuff
  if (*fpc == 0)
  {
    
    
    anyAdditionalTextDetected = 0;
    singleDetected = 0;
    insertDone = 0;
    singleInsertDone = 0;
    additionalInsertDone = 0;
    motionDetected = 0;
    motionInserted = 0;
    pressStartDetected = 0;
    pressStartInserted = 0;
    replaceDone = 0;
    return CV_SPECIAL_NONE; // do not skip
  }

  
  // if landscape is drawn use double timer
  if ((cpb->xStart - cpb->xEnd >30000) && (cpb->yStart == 640)  && (cpb->yEnd == 640))
    useDoubleTimer = 1;
  
  
  /*
  from 
  PL DRAW: xStart: 420, x:0 x*s:0, yStart: -6360, y:95 y*s:3800 -> xEnd: 420, yEnd: -2360 scale:40
  PL DRAW: xStart: 420, x:0 x*s:0, yStart: -6360, y:95 y*s:3800 -> xEnd: 420, yEnd: -2360 scale:40
  
  to 
  
  PL DRAW: xStart: 420, x:0 x*s:0, yStart: 3640, y:95 y*s:3800 -> xEnd: 420, yEnd: 7640 scale:40
  PL DRAW: xStart: 420, x:0 x*s:0, yStart: 3640, y:95 y*s:3800 -> xEnd: 420, yEnd: 7640 scale:40
*/  
  
  
  // todo after rotating RADAR part force a zero
  // otherwise the SCORE jumbles a bit, when "near"
  
    
  
  
  if (    (cpb->xStart == greatScoreText[dipLanguageSetting].xStart) && (cpb->yStart == greatScoreText[dipLanguageSetting].yStart) 
      && (cpb->xEnd == greatScoreText[dipLanguageSetting].xEnd) && (cpb->yEnd == greatScoreText[dipLanguageSetting].yEnd) ) // pre scale, rotation coordinates
  {
    highscoreSaved = 0;
  }
  if (    (cpb->xStart == highScoreText[dipLanguageSetting].xStart) && (cpb->yStart == highScoreText[dipLanguageSetting].yStart) 
      && (cpb->xEnd == highScoreText[dipLanguageSetting].xEnd) && (cpb->yEnd == highScoreText[dipLanguageSetting].yEnd) ) // pre scale, rotation coordinates
  {
    if (highscoreSaved == 0)
      saveHighScore();
    useDoubleTimer = 0;
  }
  
  // last vector of E
  if ((cpb->xStart == scoreInGameText[dipLanguageSetting].xStart) && (cpb->yStart == scoreInGameText[dipLanguageSetting].yStart)  && (replaceDone==0))
  {
    loadHighScore();
    replaceDone = 1;
   
    VectorPipelineBase *eVector = cpb; // last vector or SCORE
    VectorPipelineBase *three0Vector = cpb->next;
    

    VectorPipelineBase *seek = three0Vector->next;
    VectorPipelineBase *last0 = three0Vector->next;
    for (int i=0;i<11; i++)
    {
      last0 = last0->next;
    }

    VectorPipelineBase *realScoreVector = last0->next; // this should be the first vector of the REAL score

    
    while (1)
    {
      // small highscore start
      if ((seek->xStart == smallHighScoreText[dipLanguageSetting].xStart) && (seek->yStart == smallHighScoreText[dipLanguageSetting].yStart) )
        break;
      seek = seek->next;
    }
    // seek is the first vector of the text "HIGH SCORE"

    VectorPipelineBase *replaceAfter = seek->previous; // last score vector (without 3 zeroes that is)
    // thus from "next" to "replaceAfter" should be the real score
    
    
    // fix the new place
    replaceAfter->next = three0Vector; // after the real score, there should be the 3 000
    seek->previous = last0;   // the vector befor the "HIGH SCORE" should be the last of the 3 000

    // fix cpb
    three0Vector->previous = replaceAfter;
    last0->next = seek;

    // fix the old place
    eVector->next = realScoreVector;
    realScoreVector->previous = eVector;
    
  }
  
  
  
  // single ENEMY IN RANGE
  // this is the first Vector of letter "E" 8nemy in Range)
  // if it is not visible -> than we must add the darker "ENEMY in range"
  //   PL DRAW: xStart: -14980, x:56 x*s:168, yStart: 15040, y:0 y*s:0 -> xEnd: -14700, yEnd: 15040 scale:3
  if (    (cpb->xStart == singleInRange[dipLanguageSetting].xStart) && (cpb->yStart == singleInRange[dipLanguageSetting].yStart) 
      && (cpb->xEnd == singleInRange[dipLanguageSetting].xEnd) && (cpb->yEnd == singleInRange[dipLanguageSetting].yEnd) ) // pre scale, rotation coordinates
  {
    if (singleInsertDone) return CV_SPECIAL_NONE;
      
    startSingleEnemyCounter = 8;
    singleDetected = 1;
    
    if (vpInit_singleEnemy == 0)
    {
      printf("Saving List...\n\r");
      saveList(cpb, singleInRange[dipLanguageSetting].count, singleEnemySave,0,0x30);
      vpInit_singleEnemy = 1;
    }
    return CV_SPECIAL_NONE; // do not skip
  }

  //"I" of right
  //PL DRAW: xStart: -10780, x:56 x*s:168, yStart: 12960, y:0 y*s:0 -> xEnd: -10500, yEnd: 12960 scale:3 
  if (    (cpb->xStart == enemyRight[dipLanguageSetting].xStart) && (cpb->yStart == enemyRight[dipLanguageSetting].yStart) 
      && (cpb->xEnd == enemyRight[dipLanguageSetting].xEnd) && (cpb->yEnd == enemyRight[dipLanguageSetting].yEnd) ) // pre scale, rotation coordinates
  {
    if (additionalInsertDone) return CV_SPECIAL_NONE;
    rightCounter = 7;
    anyAdditionalTextDetected = 1;

    if (vpInit_rightEnemy == 0)
    {
      printf("Saving List right...\n\r");
      saveList(cpb, enemyRight[dipLanguageSetting].count, rightEnemySave,-31,0x30);
      vpInit_rightEnemy = 1;
    }
    return CV_SPECIAL_NONE; // do not skip
  }

  //"L" of Left
  if (    (cpb->xStart == enemyLeft[dipLanguageSetting].xStart) && (cpb->yStart == enemyLeft[dipLanguageSetting].yStart) 
      && (cpb->xEnd == enemyLeft[dipLanguageSetting].xEnd) && (cpb->yEnd == enemyLeft[dipLanguageSetting].yEnd) ) // pre scale, rotation coordinates
  {
    if (additionalInsertDone) return CV_SPECIAL_NONE;
    leftCounter = 7;
    anyAdditionalTextDetected = 1;

    if (vpInit_leftEnemy == 0)
    {
      printf("Saving List left...\n\r");
      saveList(cpb, enemyLeft[dipLanguageSetting].count, leftEnemySave,-24,0x30);
      vpInit_leftEnemy = 1;
    }
    return CV_SPECIAL_NONE; // do not skip
  }

  //"A" of REAR
  if (    (cpb->xStart == enemyRear[dipLanguageSetting].xStart) && (cpb->yStart == enemyRear[dipLanguageSetting].yStart) 
      && (cpb->xEnd == enemyRear[dipLanguageSetting].xEnd) && (cpb->yEnd == enemyRear[dipLanguageSetting].yEnd) ) // pre scale, rotation coordinates
  {
    if (additionalInsertDone) return CV_SPECIAL_NONE;
    rearCounter = 7;
    anyAdditionalTextDetected = 1;

    if (vpInit_rearEnemy == 0)
    {
      printf("Saving List rear...\n\r");
      saveList(cpb, enemyRear[dipLanguageSetting].count, rearEnemySave,-33,0x30);
      vpInit_rearEnemy = 1;
    }
    return CV_SPECIAL_NONE; // do not skip
  }
  
  
  //"M" of Motion
  if (    (cpb->xStart == motionText[dipLanguageSetting].xStart) && (cpb->yStart == motionText[dipLanguageSetting].yStart) 
      && (cpb->xEnd == motionText[dipLanguageSetting].xEnd) && (cpb->yEnd == motionText[dipLanguageSetting].yEnd) ) // pre scale, rotation coordinates
  {
    if (motionInserted) return CV_SPECIAL_NONE;
    motionCounter = 13;
    motionDetected = 1;

    if (vpInit_motion == 0)
    {
      printf("Saving List motion...\n\r");
      saveList(cpb, motionText[dipLanguageSetting].count, motionSave, 0,0x30);
      vpInit_motion = 1;
    }
    return CV_SPECIAL_NONE; // do not skip
  }

  //"P" of PressStart
  if (    (cpb->xStart == pressStartText[dipLanguageSetting].xStart) && (cpb->yStart == pressStartText[dipLanguageSetting].yStart) 
      && (cpb->xEnd == pressStartText[dipLanguageSetting].xEnd) && (cpb->yEnd == pressStartText[dipLanguageSetting].yEnd) ) // pre scale, rotation coordinates
  {
    if (pressStartInserted) return CV_SPECIAL_NONE;
    pressStartCounter = 13;
    pressStartDetected = 1;

    if (vpInit_pressStart == 0)
    {
      printf("Saving List press Start...\n\r");
      saveList(cpb, pressStartText[dipLanguageSetting].count, pressStartSave, 0,0x10);
      vpInit_pressStart = 1;
    }
    return CV_SPECIAL_NONE; // do not skip
  }
  
  //"R" of GameOver
  if (    (cpb->xStart == gameOverText[dipLanguageSetting].xStart) && (cpb->yStart == gameOverText[dipLanguageSetting].yStart) 
      && (cpb->xEnd == gameOverText[dipLanguageSetting].xEnd) && (cpb->yEnd == gameOverText[dipLanguageSetting].yEnd) ) // pre scale, rotation coordinates
  {
    // check if next vector is player start, if not - display it
    VectorPipelineBase *cpb2 = cpb->next;
    if (    (cpb2->xStart == pressStartText[dipLanguageSetting].xStart) && (cpb2->yStart == pressStartText[dipLanguageSetting].yStart) 
        && (cpb2->xEnd == pressStartText[dipLanguageSetting].xEnd) && (cpb2->yEnd == pressStartText[dipLanguageSetting].yEnd) ) // pre scale, rotation coordinates
    {
      return CV_SPECIAL_NONE; // do not skip
    }
    else
    {
      if ((pressStartCounter>0) && (pressStartInserted==0))
      {
        pressStartInserted = 1;
        pressStartCounter--;
        addList(cpbPointer, pressStartText[dipLanguageSetting].count, pressStartSave, 1);
      }
      return CV_SPECIAL_NONE;//CV_SPECIAL_NONE; // do not skip
    }
  }
  
  /*
   These go after enemy in range, and before "direction of enemies", if not present, than directly before life tanks 
   
  // normal reticel +1
  PL DRAW: xStart: -2205, x:0 x*s:0, yStart: -1360, y:-83 y*s:-830 -> xEnd: -2205, yEnd: -2360 scale:10
  
  
  // special reticle +1 10 
  PL DRAW: xStart: 420, x:0 x*s:0, yStart: -2360, y:88 y*s:1408 -> xEnd: 420, yEnd: -760 scale:16
  
  */
  
  
  
  
  
  
  
  if (mem[0x14].cell ==0) // enemy is alive
  {
    // enemy is in range!
    // frame counter can not be used,
    // since the generated pipeline not necessarily
    // still "uses" the same framecounter
    // pipelining "confuses" timing with emulation!        
    //        if ((lastFrameCount & 2) == 0) // framecounter is dividable by 2
    // find the radar
//    if ((cpb->xStart == 1680) && (cpb->yStart == 15360) && (!singleDetected))
    if ( ((cpb->xStart == 420) && (cpb->yStart == -6360) && (cpb->xEnd == 420) && (cpb->yEnd == -2360) && (!singleDetected) && (!singleInsertDone))
        || 
        ((cpb->xStart == 4900) && (cpb->yStart == 15040) && (cpb->xEnd == 4690) && (cpb->yEnd == 15280) && (!singleDetected) && (!singleInsertDone))
    )
    {
      // check if there is already an enemy in range in list!
      if (singleDetected) 
      {
        ; // yes there is
        return CV_SPECIAL_NONE; // do not skip 
      }
      // no there is not - should we put something there?
      
      if (startSingleEnemyCounter >0)
      {
        singleInsertDone = 1;
        startSingleEnemyCounter--;
        // only show the list, if "BATTLE ZONE" is not scrolling in
        // attract_logo_flag .eq   $034b             ;bool 00/ff: in "attract" mode, showing logo
        if (mem[0x14].cell !=0xff) 
        {
          addList(cpbPointer, singleInRange[dipLanguageSetting].count, singleEnemySave, 0);
        }
      }
      return CV_SPECIAL_NONE;//CV_SPECIAL_NONE; // do not skip
    }
    
    // first line of "TANKS" from score
    // additional text is displayed directly before that
//    if (   (cpb->xStart == scoreText[dipLanguageSetting].xStart) 
//        && (cpb->yStart == scoreText[dipLanguageSetting].yStart) && (additionalInsertDone==0) )

 if (   ((mem[0xce].cell != 0xff) &&  (cpb->xStart == scoreText[dipLanguageSetting].xStart)  && (cpb->yStart == scoreText[dipLanguageSetting].yStart) && (additionalInsertDone==0) )
   || ((mem[0xce].cell == 0xff) &&  (cpb->xStart == 4900)                                  && (cpb->yStart == 15040)                                && (additionalInsertDone==0) ) )
    {
      // we are now BEFORE the first score Vector
      // if no text was detected
      // and we have not already inserted something
      // and some of the counter is not 0
      // than we add a darkened vectorlist

      if ((motionCounter) && (motionInserted == 0) && (motionDetected == 0))
      {
        motionCounter--;
        // display enemy to rear
        if (mem[0x14].cell !=0xff)  // only show the list, if "BATTLE ZONE" is not scrolling in
          addList(cpbPointer, motionText[dipLanguageSetting].count, motionSave, 0);
        motionInserted=1;
      }

      if ((anyAdditionalTextDetected == 0) && (insertDone==0))
      {
        // no text detected, perhaps we should draw something? check counters...
        if (rightCounter)
        {
          additionalInsertDone = 1;
          rightCounter--;
          // display enemy to right
          if (mem[0x14].cell !=0xff)  // only show the list, if "BATTLE ZONE" is not scrolling in
            addList(cpbPointer, enemyRight[dipLanguageSetting].count, rightEnemySave, 0);
          insertDone=1;
        }
        else if (leftCounter)
        {
          additionalInsertDone = 1;
          leftCounter--;
          // display enemy to left
          if (mem[0x14].cell !=0xff)  // only show the list, if "BATTLE ZONE" is not scrolling in
            addList(cpbPointer, enemyLeft[dipLanguageSetting].count, leftEnemySave, 0);
          insertDone=1;
        }
        else if (rearCounter)
        {
          additionalInsertDone = 1;
          rearCounter--;
          // display enemy to rear
          if (mem[0x14].cell !=0xff)  // only show the list, if "BATTLE ZONE" is not scrolling in
            addList(cpbPointer, enemyRear[dipLanguageSetting].count, rearEnemySave, 0);
          insertDone=1;
        }

        return CV_SPECIAL_NONE;//CV_SPECIAL_NONE; // do not skip
      }
    }
          
  }
  return CV_SPECIAL_NONE; // do not skip
}

/*
                   *   DSW0 ($0A00): LLBBMMTT                                                     *
                   *     LL=language (00=English, 01=French, 10=German, 11=Spanish)               *
                   *     BB=bonus tank score (00=none, 01=15K/100K, 10=25K/100K, 11=50K/100K)     *
                   *     MM=missile appears at score (5K, 10K, 20K, 30K)                          *
                   *     TT=number of starting tanks (value + 2) 
*/

//unsigned char dipLanguageSetting = 0; // english
unsigned char dipLivesSetting = 2; // 4 lives
unsigned char dipBonusSetting = 1; // 15 /100
unsigned char dipMissileSetting = 0; // 5K
unsigned char dipSpeedSetting = 41;

DipSwitchSetting dipLanguage = 
{
  "LANGUAGE",
  &dipLanguageSetting,
  { 
    {"ENGLISH", 0}, 
    {"GERMAN", 1},
    {"FRENCH", 2},
    {"SPANISH", 3},
    {0,0}
  }
};
DipSwitchSetting dipLives = 
{
  "LIVES",
  &dipLivesSetting,
  { 
    {"5", 0},
    {"4", 1}, 
    {"3", 2},
    {"2", 3}, 
    {0,0}
  }
};
DipSwitchSetting dipBonusTank = 
{
  "BONUS TANK",
  &dipBonusSetting,
  { 
    {"NONE", 0}, 
    {"15K/100K", 1},
    {"25K/100K", 2}, 
    {"50K/100K", 3},
    {0,0}
  }
};
DipSwitchSetting dipMissile = 
{
  "MISSILES",
  &dipMissileSetting,
  { 
    {"5K", 0}, 
    {"10K", 1},
    {"20K", 2}, 
    {"30K", 3},
    {0,0}
  }
};
DipSwitchSetting dipSpeed = 
{
  "GAME SPEED",
  &dipSpeedSetting,
  { 
    {"41HZ (ORIGINAL)", 41}, 
    {"50HZ", 50},
    {"60HZ", 60}, 
    {"70Hz", 70},
    {0,0}
  }
};


DipSwitchSetting *battleZoneSwitches[] = 
{
  &dipLanguage,
  &dipLives,
  &dipBonusTank,
  &dipMissile,
  &dipSpeed,
  0,
};


void applyDips()
{
  // These can be written "directly", since they go to different memory locations
  // beware other machines, where settings might need "OR" or "AND" 
  unsigned char dipSetting = dipLanguageSetting;
  dipSetting=dipSetting<<2;
  dipSetting=dipSetting|dipBonusSetting;
  dipSetting=dipSetting<<2;
  dipSetting=dipSetting|dipMissileSetting;
  dipSetting=dipSetting<<2;
  dipSetting=dipSetting|dipBonusSetting;
  mem[0xa00].cell = dipSetting; 
  v_setClientHz(dipSpeedSetting);
  optionreg [0] = dipSetting; 
}

signed char enterDip=0;


int handleInput(void) 
{
  //  v_doInputMapping(asteroidMappings);

    if (enterDip)
    {
      v_getDipSettings(battleZoneSwitches, "BZONE DIP SWITCHES");
      applyDips();
      enterDip = 0;

      shrinkCounter =0;    
      startSingleEnemyCounter =0;
      vpInit_singleEnemy=0;
      vpInit_leftEnemy=0;
      vpInit_rightEnemy=0;
      vpInit_rearEnemy=0;
      vpInit_motion=0;
      vpInit_pressStart = 0;
      rightCounter = 0;
      leftCounter = 0;
      rearCounter = 0;
      motionCounter = 0;
      pressStartCounter = 0;
    }
    return 0;
}

extern void (*customInputHandler) (void);

void bzInputHandler()
{
//  printf("CustomInput");
#define JOYSTICK_CENTER_MARGIN 20
  // build a "0" zone
  joystick.x = currentJoy1X;  // 0x80   0  0x7f
  if ((currentJoy1X>0) && (currentJoy1X<JOYSTICK_CENTER_MARGIN)) joystick.x= 0;
  if ((currentJoy1X<0) && (currentJoy1X>-JOYSTICK_CENTER_MARGIN)) joystick.x=  0;
  joystick.y = currentJoy1Y;  // 0x80   0  0x7f
  if ((currentJoy1Y>0) && (currentJoy1Y<JOYSTICK_CENTER_MARGIN)) joystick.y=  0;
  if ((currentJoy1Y<0) && (currentJoy1Y>-JOYSTICK_CENTER_MARGIN)) joystick.y=  0;

  start1 = currentButtonState & 0x01;  // button 1 on port 1
  start2 = currentButtonState & 0x02;  // button 2 on port 1

  if ((currentButtonState & 0x01) && (currentButtonState & 0x08))
    enterDip = 1;
  
  if (yates_config)
  {
    switches [0].leftfwd = (currentButtonState & 0x10)?1:0; // button 1 port 2
    switches [0].leftrev = (currentButtonState & 0x20)?1:0; // button 2 port 2
    switches [0].rightfwd = (currentButtonState & 0x40)?1:0; // button 3 port 2
    switches [0].rightrev = (currentButtonState & 0x80)?1:0; // button 4 port 2
    switches [0].fire = currentJoy2X>JOYSTICK_CENTER_MARGIN;  //Joy2 analog x axis pulled high      
  }
  else if (onlyOnejoystick)
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
  handleInput();
}

int main(int argc, char *argv[])
{
  int smallwindow = 1;
  int use_pixmap = 1;
  int line_width = 0;
  ProgName = "battlezone"; // defined in display.h

  init_graphics (smallwindow, use_pixmap, line_width, game_name (game));

  // special vectrex  interface settings for Bzone
  v_setRefresh(50);
  v_setClientHz(41); // this should be 
  v_setupIRQHandling();
  v_enableSoundOut(1);
  v_enableButtons(1);
  v_enableJoystickAnalog(1,1,1,1);
  useDoubleTimer = 1;
  keepDotsTogether = 1;
  
  // todo for german that is bad
  setCustomClipping(1,-16000,-16000,16000,16000);
  gameCallback = battleZoneCallback;
  checkExternal = checkForSpecialVectors;
  customInputHandler = bzInputHandler;

  useSamples = 1;

  fire1Size = v_loadRAW("samples/bzone/fire1.raw", fire1Sample);
  fire2Size = v_loadRAW("samples/bzone/fire2.raw", fire2Sample);
  explostion1Size = v_loadRAW("samples/bzone/explosion1.raw", explosion1Sample);
  explostion2Size = v_loadRAW("samples/bzone/explosion2.raw", explosion2Sample);
  engine1Size = v_loadRAW("samples/bzone/engine1.raw", engine1Sample);
  engine2Size = v_loadRAW("samples/bzone/engine2.raw", engine2Sample);

  
  int pv = 0;
  if ((pv = ini_parse("battlezone.ini", bzIniHandler, 0)) < 0) 
  {
        printf("battlezone.ini not loaded!\n\r");
  }
  else
    printf("battlezone.ini loaded!\n\r");
  
  
  
  mem = (elem *) gameMemory;
  game = pick_game (SINGLE_GAME);

  setup_game ();

  save_PC = (memrd(0xfffd,0,0) << 8) | memrd(0xfffc,0,0);
  save_A = 0;
  save_X = 0;
  save_Y = 0;
  save_flags = 0;
  save_totcycles = 0;
  irq_cycle = 8192;
  applyDips();  
  sim_6502 ();
  while (1)
  {
    ; // do not "return" - never call _libc_fini_array etc...
  }
}
