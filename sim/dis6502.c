/*
 * dis6502.c: 6502 disassembler for Atari Vector game simulator
 *
 * Copyright 1993 Eric Smith
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
 * $Id: dis6502.c,v 1.1 2018/07/31 01:19:44 pi Exp $
 */

#include <stdio.h>
#include <string.h>

#include "memory.h"


#define INH  0
#define ACC  1
#define IMM  2
#define ZP   3
#define ZPX  4
#define ZPY  5
#define ABS  6
#define ABSX 7
#define ABSY 8
#define IND  9
#define INDX 10
#define INDY 11
#define REL  12

/*    INH  ACC  IMM  ZP  ZPX   ZPY   ABS ABSX  ABSY  IND  INDX   INDY   REL*/
static int bytes[] = 
    { 1,   1,   2,   2,  2,    2,    3,  3,    3,    3,   2,     2,     2 };
char *prefix[] = 
    { "",  "a", "#", "", "",   "",   "", "",   "",   "(", "(",   "(",   "" };
char *suffix[] = 
    { "",  "",  "",  "", ",x", ",y", "", ",x", ",y", ")", ",x)", "),y", "" };


struct InstInfo
  {
    char *mnemonic;
    int addrmode;
  };

struct InstInfo inst_info[] = 
{
/* 0x00 */ {"brk",INH }, {"ora",INDX}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"ora",ZP  }, {"asl",ZP  }, {"???",INH },
	   {"php",INH }, {"ora",IMM }, {"asl",ACC }, {"???",INH },
	   {"???",INH }, {"ora",ABS }, {"asl",ABS }, {"???",INH },
/* 0x10 */ {"bpl",REL }, {"ora",INDY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"ora",ZPX }, {"asl",ZPX }, {"???",INH },
	   {"clc",INH }, {"ora",ABSY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"ora",ABSX}, {"asl",ABSX}, {"???",INH },
/* 0x20 */ {"jsr",ABS }, {"and",INDX}, {"???",INH }, {"???",INH },
	   {"bit",ZP  }, {"and",ZP  }, {"rol",ZP  }, {"???",INH },
	   {"plp",INH }, {"and",IMM }, {"rol",ACC }, {"???",INH },
	   {"bit",ABS }, {"and",ABS }, {"rol",ABS }, {"???",INH },
/* 0x30 */ {"bmi",REL }, {"and",INDY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"and",ZPX }, {"rol",ZPX }, {"???",INH },
	   {"sec",INH }, {"and",ABSY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"and",ABSX}, {"rol",ABSX}, {"???",INH },
/* 0x40 */ {"rti",INH }, {"eor",INDX}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"eor",ZP  }, {"lsr",ZP  }, {"???",INH },
	   {"pha",INH }, {"eor",IMM }, {"lsr",ACC }, {"???",INH },
	   {"jmp",ABS }, {"eor",ABS }, {"lsr",ABS }, {"???",INH },
/* 0x50 */ {"bvc",REL }, {"eor",INDY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"eor",ZPX }, {"lsr",ZPX }, {"???",INH },
	   {"cli",INH }, {"eor",ABSY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"eor",ABSX}, {"lsr",ABSX}, {"???",INH },
/* 0x60 */ {"rts",INH }, {"adc",INDX}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"adc",ZP  }, {"ror",ZP  }, {"???",INH },
	   {"pla",INH }, {"adc",IMM }, {"ror",ACC }, {"???",INH },
	   {"jmp",IND }, {"adc",ABS }, {"ror",ABS }, {"???",INH },
/* 0x70 */ {"bvs",REL }, {"adc",INDY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"adc",ZPX }, {"ror",ZPX }, {"???",INH },
	   {"sei",INH }, {"adc",ABSY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"adc",ABSX}, {"ror",ABSX}, {"???",INH },
/* 0x80 */ {"???",INH }, {"sta",INDX}, {"???",INH }, {"???",INH },
	   {"sty",ZP  }, {"sta",ZP  }, {"stx",ZP  }, {"???",INH },
	   {"dey",INH }, {"???",INH }, {"txa",INH }, {"???",INH },
	   {"sty",ABS }, {"sta",ABS }, {"stx",ABS }, {"???",INH },
/* 0x90 */ {"bcc",REL }, {"sta",INDY}, {"???",INH }, {"???",INH },
	   {"sty",ZPX }, {"sta",ZPX }, {"stx",ZPY }, {"???",INH },
	   {"tya",INH }, {"sta",ABSY}, {"txs",INH }, {"???",INH },
	   {"???",INH }, {"sta",ABSX}, {"???",INH }, {"???",INH },
/* 0xa0 */ {"ldy",IMM }, {"lda",INDX}, {"ldx",IMM }, {"???",INH },
	   {"ldy",ZP  }, {"lda",ZP  }, {"ldx",ZP  }, {"???",INH },
	   {"tay",INH }, {"lda",IMM }, {"tax",INH }, {"???",INH },
	   {"ldy",ABS }, {"lda",ABS }, {"ldx",ABS }, {"???",INH },
/* 0xb0 */ {"bcs",REL }, {"lda",INDY}, {"???",INH }, {"???",INH },
	   {"ldy",ZPX }, {"lda",ZPX }, {"ldx",ZPY }, {"???",INH },
	   {"clv",INH }, {"lda",ABSY}, {"tsx",INH }, {"???",INH },
	   {"ldy",ABSX}, {"lda",ABSX}, {"ldx",ABSY}, {"???",INH },
/* 0xc0 */ {"cpy",IMM }, {"cmp",INDX}, {"???",INH }, {"???",INH },
	   {"cpy",ZP  }, {"cmp",ZP  }, {"dec",ZP  }, {"???",INH },
	   {"iny",INH }, {"cmp",IMM }, {"dex",INH }, {"???",INH },
	   {"cpy",ABS }, {"cmp",ABS }, {"dec",ABS }, {"???",INH },
/* 0xd0 */ {"bne",REL }, {"cmp",INDY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"cmp",ZPX }, {"dec",ZPX }, {"???",INH },
	   {"cld",INH }, {"cmp",ABSY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"cmp",ABSX}, {"dec",ABSX}, {"???",INH },
/* 0xe0 */ {"cpx",IMM }, {"sbc",INDX}, {"???",INH }, {"???",INH },
	   {"cpx",ZP  }, {"sbc",ZP  }, {"inc",ZP  }, {"???",INH },
	   {"inx",INH }, {"sbc",IMM }, {"nop",INH }, {"???",INH },
	   {"cpx",ABS }, {"sbc",ABS }, {"inc",ABS }, {"???",INH },
/* 0xf0 */ {"beq",REL }, {"sbc",INDY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"sbc",ZPX }, {"inc",ZPX }, {"???",INH },
	   {"sed",INH }, {"sbc",ABSY}, {"???",INH }, {"???",INH },
	   {"???",INH }, {"sbc",ABSX}, {"inc",ABSX}, {"???",INH }
};


char disbuffer [30];

int disasm_6502 (word addr)
{
  byte opcode;
  int numbytes;
  int addrmode;
  int operand = 0;
  int holdbrkflag = breakflag;
  char *b;
  
  b = disbuffer;
  opcode = memrd (addr, 0, 0);

  addrmode = inst_info [opcode].addrmode;
  numbytes = bytes [addrmode];

  switch(numbytes)
    {
    case 1:
      sprintf (b, "%04x: %02x    ", addr, opcode);
      break;
    case 2:
      operand = memrd (addr+1, 0, 0);
      sprintf (b, "%04x: %02x%02x  ", addr, opcode, operand);
      break;
    case 3:
      operand = memrdwd(addr+1, 0, 0);
      sprintf (b, "%04x: %02x%02x%02x", addr, opcode, operand & 0xff, operand >> 8);
      break;
    default:
      sprintf (b, "%04x: %02x????", addr, opcode);
      break;
    }
  b += strlen (b);

  if (addrmode == REL)
    {
      if (operand & 0x80)
	operand |= 0xff00;
      operand += addr + numbytes;
      operand &= 0xffff;
    }

  sprintf (b, " %s %s", inst_info [opcode].mnemonic, prefix [inst_info [opcode].addrmode]);
  b += strlen (b);

  if ((numbytes == 3) || (addrmode == REL))
    sprintf (b, "%04x", operand);
  else if (numbytes == 2)
    sprintf (b, "%02x", operand);
  b += strlen (b);

  sprintf (b, "%s ", suffix [inst_info [opcode].addrmode]);
  b += strlen (b);

//  printf ("%-27s", disbuffer);

  breakflag = holdbrkflag;
   return (numbytes);
}
