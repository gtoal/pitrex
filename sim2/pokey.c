/* from BZ disasm

https://6502disassembly.com/va-battlezone/Battlezone.html

POKEY audio has four channels, with two 8-bit I/O locations per channel (AUDFn
                   ; and AUDCn).  The sound effects defined by this data are played on channels 1
                   ; and 2.
                   ; 
                   ; The AUDFn setting determines frequency.  Larger value == lower pitch.
                   ; 
                   ; The AUDCn value is NNNFVVVV, where N is a noise / distortion setting, F is
                   ; "forced volume-only output" enable, and V is the volume level.
                   ; 
                   ; In the table below, each chunk has 4 values:
                   ;  +00 initial value
                   ;  +01 duration
                   ;  +03 increment
                   ;  +04 repetition count
                   ; 
                   ; The sound specified by the value is played until the duration reaches zero. 
                   ; If the repetition count is nonzero, the value is increased or decreased by the
                   ; increment, and the duration is reset.  When the repetition count reaches zero,
                   ; the next chunk is loaded.  If the chunk has the value $00, the sequence ends. 
                   ; The counters are updated by the 250Hz NMI.
                   ; 
                   ; Because AUDFn and AUDCn are specified by different chunks, care must be taken
                   ; to ensure the durations run out at the same time.
                   ; 
*/                   
                   
                   

/*
 * pokey.c: POKEY chip simulation functions
 *
 * Copyright 1991, 1992, 1993, 1996 Hedley Rainnie and Eric Smith
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
 * $Id: pokey.c 2 2003-08-20 01:51:05Z eric $
 */

#include <stdlib.h>
#include <stdio.h>

#include <vectrex/vectrexInterface.h>

#include "memory.h"
#include "pokey.h"


#define MAX_REG 16


/* read registers */
#define POT0 0x0
#define POT1 0x1
#define POT2 0x2
#define POT3 0x3
#define POT4 0x4
#define POT5 0x5
#define POT6 0x6
#define POT7 0x7
#define ALLPOT 0x8
#define KBCODE 0x9
#define RANDOM 0xa
#define IRQSTAT 0xe
#define SKSTAT 0xf

/* write registers */
#define AUDF1 0x0
#define AUDC1 0x1
#define AUDF2 0x2
#define AUDC2 0x3
#define AUDF3 0x4
#define AUDC3 0x5
#define AUDF4 0x6
#define AUDC4 0x7
#define AUDCTL 0x8
#define STIMER 0x9
#define SKRES 0xa
#define POTGO 0xb
#define SEROUT 0xd
#define IRQEN 0xe
#define SKCTL 0xf


char *pokey_rreg_name [] =
{
  "POT0", "POT1", "POT2", "POT3", 
  "POT4", "POT5", "POT6", "POT7", 
  "ALLPOT", "KBCODE", "RANDOM", "unused0xB", 
  "unused0xC", "unused0xD", "IRQSTAT", "SKSTAT"
};

char *pokey_wreg_name [] =
{
  "AUDF1", "AUDC1", "AUDF2", "AUDC2",
  "AUDF3", "AUDC3", "AUDF4", "AUDC4", 
  "AUDCTL", "STIMER", "SKRES", "POTGO",
  "unused0xC", "SEROUT", "IRQEN", "SKCTL" 
};


byte pokey_rreg [MAX_POKEY][MAX_REG];

byte pokey_wreg [MAX_POKEY][MAX_REG];

#ifdef POKEY_DEBUG
byte pokey_wreg_inited [MAX_POKEY][MAX_REG] = { { 0 } };
#endif
#include "game.h"
static int oldSpinner = 0;

static int spinnerSum = 0;

byte pokey_read (int pokeynum, int reg, int PC, unsigned long cyc)
{
  switch (reg)
    {
    case RANDOM:
      if ((pokey_wreg [pokeynum] [SKCTL] & 0x03) != 0x00)
        pokey_rreg [pokeynum] [RANDOM] = (rand () >> 12) & 0xff;
      if ((game == TEMPEST) && (pokeynum==1)) return 0xff;
      return (pokey_rreg [pokeynum] [RANDOM]);
    case ALLPOT:
      if ((game == TEMPEST) && (pokeynum==0)) 
      {
        // joystick.x = 30 - 127
        // joystick.x = -30 - -127
        signed char spin = (signed char)joystick.x;
        
        if (spin >0)
          spin = (spin-30)/30;
        else if (spin <0)
          spin = (spin+30)/30;
        
        // spin something like -3 -> +3
        
        spinnerSum += spin;
        if (spinnerSum>7)
        {
          oldSpinner+=spin;
          spinnerSum-=7;
        }
        if (spinnerSum<-7)
        {
          oldSpinner+=spin;
          spinnerSum+=7;
        }
/*        
    joystick.x = currentJoy1X;  // 0x80   0  0x7f


                     ; BITS 0-3: Encoder Wheel
                     ; BIT  4  : Cocktail detection
                     ; BIT  5  : Switch #1 at D/E2
                     ; BITS 6-7: Unused.

*/                     
        int ret = 0;
        ret = oldSpinner & 0x0f;
        return ret;
      }
      else if ((game == TEMPEST) && (pokeynum==1)) 
      {
/*        


    GAME OPTIONS:
    (4-position switch at D/E2 on Math Box PCB)

    1   2   3   4                   Meaning
    -------------------------------------------------------------------------
        Off                         Minimum rating range: 1, 3, 5, 7, 9
        On                          Minimum rating range tied to high score
            Off Off                 Medium difficulty (see notes)
            Off On                  Easy difficulty (see notes)
            On  Off                 Hard difficulty (see notes)
            On  On                  Medium difficulty (see notes)
            
            
                     ; BIT 0: D/E2 switch #2
                     ; BIT 1: D/E2 switch #3
                     ; BIT 2: D/E2 switch #4
                     ; BIT 3: Fire Button
                     ; BIT 4: Zapper Button
                     ; BIT 5: Start Player 1 Button
                     ; BIT 6: Start Player 2 Button
                     ; BIT 7: Unused.
*/                     
        int ret = 0;
        if (switches [0].fire) ret = ret | 0x04; // EASY
        if (switches [0].fire) ret = ret | 0x08;
        if (switches [0].thrust) ret = ret | 0x10;
        if (start1) ret = ret | 0x20;
        if (start2) ret = ret | 0x40;

        return ret;
      }
      else if (pokeynum==0)
        return 0xf;
      else return 0;
    default:
#ifdef POKEY_DEBUG
      printf ("pokey %d read reg %1x (%s)\n", pokeynum, reg, pokey_rreg_name [reg]);
#endif
      return (pokey_rreg [pokeynum] [RANDOM]);
    }
}

typedef struct {
  uint8_t hi;
  uint8_t lo;
} FreqTrans;

// distortion 10
// 8bit

FreqTrans pt[]=
{
  {000,000}, // 0
  {000,015}, // 1
  {000,016}, // 2
  {000,017}, // 3
  {000,020}, // 4
  {000,021}, // 5
  {000,022}, // 6
  {000,023}, // 7
  {000,024}, // 8
  {000,025}, // 9
  {000,026}, // a
  {000,027}, // b
  {000,030}, // c
  {000,031}, // d; --------------------  
  {000,033}, // e - 0 1 5 3 ; C, Ocatve 7
  {000,034}, // f - 0 1 5 3 ; B, Ocatve 7
  {000,036}, // 10 - 0 1 5 3 ; A#, Ocatve 7
  {000,040}, // 11 - 0 1 5 3 ; A, Ocatve 7
  {000,042}, // 12 - 0 1 5 3 ; G#, Ocatve 7
  {000,044}, // 13 - 0 1 5 3 ; G, Ocatve 7
{000,045}, // 14
  {000,046}, // 15 - 0 1 5 3 ; F#, Ocatve 7
  {000,050}, // 16 - 0 1 5 3 ; F, Ocatve 7
  {000,052}, // 17 - 0 1 5 3 ; E, Ocatve 7
{000,053}, // 18
  {000,055}, // 19 - 0 1 5 3 ; D#, Ocatve 7
  {000,060}, // 1a - 0 1 5 3 ; D, Ocatve 7
{000,061}, // 1b
  {000,062}, // 1c - 0 1 5 3 ; C#, Ocatve 7; --------------------  
  {000,065}, // 1d - 0 1 5 3 ; C, Ocatve 6
  {000,067}, // 1e
  {000,071}, // 1f - 0 1 5 3 ; B, Ocatve 6
{000,072}, // 20
  {000,074}, // 21 - 0 1 5 3 ; A#, Ocatve 6
{000,076}, // 22
  {001,000}, // 23 - 0 1 5 3 ; A, Ocatve 6
{001,000}, // 24
  {001,003}, // 25 - 0 1 5 3 ; G#, Ocatve 6
{001,004}, // 26
{001,005}, // 27
  {001,007}, // 28 - 0 1 5 3 ; G, Ocatve 6
{001,011}, // 29
  {001,014}, // 2a - 0 1 5 3 ; F#, Ocatve 6
{001,015}, // 2b
{001,016}, // 2c
  {001,020}, // 2d - 0 1 5 3 ; F, Ocatve 6
{001,022}, // 2e
  {001,025}, // 2f - 0 1 5 3 ; E, Ocatve 6
{001,027}, // 30
{001,030}, // 31
  {001,032}, // 32 - 0 1 5 3 ; D#, Ocatve 6
{001,033}, // 33
{001,035}, // 34
  {001,037}, // 35 - 0 1 5 3 ; D, Ocatve 6
{001,040}, // 36
{001,041}, // 37
{001,043}, // 38
  {001,045}, // 39 - 0 1 5 3 ; C#, Ocatve 6
{001,047}, // 3a
{001,051}, // 3b ; --------------------  
  {001,053}, // 3c - 0 1 5 3 ; C, Ocatve 5
{001,053}, // 3d
{001,055}, // 3e
{001,057}, // 3f
  {001,061}, // 40 - 0 1 5 3 ; B, Ocatve 5
{001,063}, // 41
{001,065}, // 42
{001,067}, // 43
  {001,070}, // 44 - 0 1 5 3 ; A#, Ocatve 5
{001,072}, // 45
{001,074}, // 46
{001,076}, // 47
  {001,077}, // 48 - 0 1 5 3 ; A, Ocatve 5
{002,002}, // 49
{002,004}, // 4a
{002,006}, // 4b
  {002,007}, // 4c - 0 1 5 3 ; G#, Ocatve 5
{002,011}, // 4d
{002,013}, // 4e
{002,014}, // 4f
{002,015}, // 50
  {002,017}, // 51 - 0 1 5 3 ; G, Ocatve 5
{002,021}, // 52
{002,023}, // 53
{002,025}, // 54
  {002,027}, // 55 - 0 1 5 3 ; F#, Ocatve 5
{002,031}, // 56
{002,033}, // 57
{002,034}, // 58
{002,036}, // 59
{002,037}, // 5a
  {002,040}, // 5b - 0 1 5 3 ; F, Ocatve 5
{002,042}, // 5c
{002,044}, // 5d
{002,046}, // 5e
{002,050}, // 5f
  {002,052}, // 60 - 0 1 5 3 ; E, Ocatve 5
{002,054}, // 61
{002,056}, // 62
{002,060}, // 63
{002,061}, // 64
{002,062}, // 65
  {002,064}, // 66 - 0 1 5 3 ; D#, Ocatve 5
{002,066}, // 67
{002,070}, // 68
{002,072}, // 69
{002,073}, // 6a
{002,075}, // 6b
  {002,076}, // 6c - 0 1 5 3 ; D, Ocatve 5
{003,000}, // 6d
{003,002}, // 6e
{003,004}, // 6f
{003,006}, // 70
{003,010}, // 71
  {003,012}, // 72 - 0 1 5 3 ; C#, Ocatve 5
{003,014}, // 73
{003,016}, // 74
{003,020}, // 75
{003,022}, // 76
{003,023}, // 77
{003,024}, // 78
  {003,026}, // 79 - 0 1 5 3 ; C, Ocatve 4
{003,030}, // 7a
{003,032}, // 7b
{003,034}, // 7c
{003,036}, // 7d
{003,037}, // 7e
{003,041}, // 7f
  {003,042}, // 80 - 0 1 5 3 ; B, Ocatve 4
{003,043}, // 7f
{003,045}, // 7f
{003,047}, // 7f
{003,052}, // 7f
{003,054}, // 7f
{003,056}, // 7f
{003,057}, // 7f
  {003,060}, // 88 - 0 1 5 3 ; A#, Ocatve 4
{003,062}, // 7f
{003,064}, // 7f
{003,066}, // 7f
{003,071}, // 7f
{003,073}, // 7f
{003,074}, // 7f
{003,074}, // 7f
  {003,076}, // 90 - 0 1 5 3 ; A, Ocatve 4
{004,001}, // 7f
{004,003}, // 7f
{004,005}, // 7f
{004,007}, // 7f
{004,011}, // 7f
{004,012}, // 7f
{004,013}, // 7f
{004,014}, // 7f
  {004,015}, // 99 - 0 1 5 3 ; G#, Ocatve 4
{004,017}, // 7f
{004,021}, // 7f
{004,022}, // 7f
{004,024}, // 7f
{004,025}, // 7f
{004,027}, // 7f
{004,031}, // 7f
{004,033}, // 7f
  {004,035}, // A2 - 0 1 5 3 ; G, Ocatve 4
{004,037}, // 7f
{004,041}, // 7f
{004,043}, // 7f
{004,044}, // 7f
{004,046}, // 7f
{004,047}, // 7f
{004,051}, // 7f
{004,052}, // 7f
{004,054}, // 7f
{004,055}, // 7f
  {004,056}, // AD - 0 1 5 3 ; F#, Ocatve 4
{004,060}, // 7f
{004,062}, // 7f
{004,064}, // 7f
{004,066}, // 7f
{004,070}, // 7f
{004,072}, // 7f
{004,074}, // 7f
{004,076}, // 7f
  {005,000}, // B6 - 0 1 5 3 ; F, Ocatve 4
{005,002}, // 7f
{005,004}, // 7f
{005,006}, // 7f
{005,010}, // 7f
{005,012}, // 7f
{005,014}, // 7f
{005,016}, // 7f
{005,017}, // 7f
{005,020}, // 7f
{005,021}, // 7f
  {005,023}, // C1 - 0 1 5 3 ; E, Ocatve 4
{005,025}, // 7f
{005,026}, // 7f
{005,030}, // 7f
{005,032}, // 7f
{005,034}, // 7f
{005,036}, // 7f
{005,041}, // 7f
{005,043}, // 7f
{005,045}, // 7f
{005,046}, // 7f
  {005,050}, // CC - 0 1 5 3 ; D#, Ocatve 4
{005,051}, // 7f
{005,053}, // 7f
{005,055}, // 7f
{005,056}, // 7f
{005,060}, // 7f
{005,061}, // 7f
{005,062}, // 7f
{005,064}, // 7f
{005,066}, // 7f
{005,070}, // 7f
{005,072}, // 7f
{005,073}, // 7f
  {005,075}, // D9 - 0 1 5 3 ; D, Ocatve 4
{006,000}, // 7f
{006,001}, // 7f
{006,003}, // 7f
{006,005}, // 7f
{006,007}, // 7f
{006,011}, // 7f
{006,013}, // 7f
{006,014}, // 7f
{006,016}, // 7f
{006,017}, // 7f
{006,021}, // 7f
{006,022}, // 7f
  {006,024}, // E6 - 0 1 5 3 ; C#, Ocatve 4
{006,026}, // 7f
{006,027}, // 7f
{006,031}, // 7f
{006,032}, // 7f
{006,034}, // 7f
{006,036}, // 7f
{006,040}, // 7f
{006,041}, // 7f
{006,043}, // 7f
{006,045}, // 7f
{006,047}, // 7f
{006,051}, // 7f
  {006,054}, // F3 - 0 1 5 3 ; C, Ocatve 3
{006,056}, // 7f
{006,061}, // 7f
{006,063}, // 7f
{006,064}, // 7f
{006,066}, // 7f
{006,070}, // 7f
{006,071}, // 7f
{006,072}, // 7f
{006,073}, // 7f
{006,074}, // 7f
{006,075}, // 7f
{006,077} // 7f
};
void pokey_write (int pokeynum, int reg, byte val, int PC, unsigned long cyc)
{
#ifdef POKEY_DEBUG
  if (! pokey_wreg_inited [pokeynum] [reg])
    {
      pokey_wreg_inited [pokeynum] [reg] = 1;
      pokey_wreg [pokeynum] [reg] = val + 1;  /* make sure we log it */
    }
#endif
//      printf ("pokey %d reg %1x (%s) write data %02x\n", pokeynum, reg, pokey_wreg_name [reg], val);
  
/* BZ
                   POKEY_AUDF1     .eq     $1820             ;W audio channel 1 frequency
                   POKEY_AUDC1     .eq     $1821             ;W audio channel 1 control
                   POKEY_AUDF2     .eq     $1822             ;W audio channel 2 frequency
                   POKEY_AUDC2     .eq     $1823             ;W audio channel 2 control
                   POKEY_AUDF3     .eq     $1824             ;W audio channel 3 frequency
                   POKEY_AUDC3     .eq     $1825             ;W audio channel 3 control
                   POKEY_AUDF4     .eq     $1826             ;W audio channel 4 frequency
                   POKEY_AUDC4     .eq     $1827             ;W audio channel 4 control
                   POKEY_ALLPOT    .eq     $1828             ;R read 8 line POT port state
                   POKEY_AUDCTL    .eq     $1828             ;W audio control
                   POKEY_RANDOM    .eq     $182a             ;R random number
                   POKEY_POTGO     .eq     $182b             ;W start POT scan sequence
                   POKEY_SKCTL     .eq     $182f             ;W serial port control
*/                   
                   
//  trying to map pokey 1-3 to psg 1-3
  pokey_wreg [pokeynum] [reg] = val;
  
  if (game == BATTLEZONE) 
  {
    if ((mem[0x1840].cell&0x20) == 0)
    {
      v_writePSG_double_buffered(8, 0);
      v_writePSG_double_buffered(9, 0);
      v_writePSG_double_buffered(10, 0);
//      v_writePSG_double_buffered(7, 0xff-8-4); // disabled joystick :-)
      return;
    }
  }
  

  
  // psgFreq;
  // channel A
  if ((reg == 0) || (reg == 6))
  {
    v_writePSG_double_buffered(0, pt[val].lo);
    v_writePSG_double_buffered(1, pt[val].hi);
  }
  if ((reg == 1) || (reg == 7))
  {
   
    unsigned char vol = val & 0xf;

    if (vol <= 4)
    vol = vol <<2;
    if (vol >0xf) vol = 0xf;
    
    uint8_t reg7 = v_readPSG_double_buffered(0x07);
    v_writePSG_double_buffered(8, vol);
    if ((val & 0xf0) == 0xa0)
    {
      // pure tone
      reg7 = reg7 |8; // disable noise
      reg7 = reg7 & ~(1); // enable tone
    }
    else
    {
      // assume noise
      reg7 = reg7 |1; // disable tone
      reg7 = reg7 & ~(8); // enable noise
    }
    v_writePSG_double_buffered(7, reg7);
  }

  // channel B
  if (reg == 2)
  {
    v_writePSG_double_buffered(2, pt[val].lo);
    v_writePSG_double_buffered(3, pt[val].hi);
  }
  if (reg == 3) 
  {
    unsigned char vol = val & 0xf;
    
    if (vol <= 4)
    vol = vol <<2;
    if (vol >0xf) vol = 0xf;
    
    uint8_t reg7 = v_readPSG_double_buffered(0x07);
    v_writePSG_double_buffered(9, vol);
    if ((val & 0xf0) == 0xa0)
    {
      // pure tone
      reg7 = reg7 |16; // disable noise
      reg7 = reg7 & ~(2); // enable tone
    }
    else
    {
      // assume noise
      reg7 = reg7 |2; // disable tone
      reg7 = reg7 & ~(16); // enable noise
    }
    v_writePSG_double_buffered(7, reg7);
  }

  // channel C
  if (reg == 4)
  {
    v_writePSG_double_buffered(4, pt[val].lo);
    v_writePSG_double_buffered(5, pt[val].hi);
  }
  if (reg == 5) 
  {
    unsigned char vol = val & 0xf;
    
    if (vol <= 4)
    vol = vol <<2;
    if (vol >0xf) vol = 0xf;
    
    
    uint8_t reg7 = v_readPSG_double_buffered(0x07);
    v_writePSG_double_buffered(10, vol);
    if ((val & 0xf0) == 0xa0)
    {
      // pure tone
      reg7 = reg7 |32; // disable noise
      reg7 = reg7 & ~(4); // enable tone
    }
    else
    {
      // assume noise
      reg7 = reg7 |4; // disable tone
      reg7 = reg7 & ~(32); // enable noise
    }
    v_writePSG_double_buffered(7, reg7);
  }
  
  
  
}





