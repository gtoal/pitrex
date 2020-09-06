/*
 * mathbox.c: math box simulation (Battlezone/Red Baron/Tempest)
 *
 * Copyright 1991, 1992, 1993, 1996 Eric Smith
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
 * $Id: mathbox.c,v 1.1 2018/07/31 01:19:45 pi Exp $
 */

#include <stdio.h>
#include "mathbox.h"

/* math box scratch registers */
s16 mb_reg [16];

/* math box result */
s16 mb_result = 0;

#define REG0 mb_reg [0x00]
#define REG1 mb_reg [0x01]
#define REG2 mb_reg [0x02]
#define REG3 mb_reg [0x03]
#define REG4 mb_reg [0x04]
#define REG5 mb_reg [0x05]
#define REG6 mb_reg [0x06]
#define REG7 mb_reg [0x07]
#define REG8 mb_reg [0x08]
#define REG9 mb_reg [0x09]
#define REGa mb_reg [0x0a]
#define REGb mb_reg [0x0b]
#define REGc mb_reg [0x0c]
#define REGd mb_reg [0x0d]
#define REGe mb_reg [0x0e]
#define REGf mb_reg [0x0f]


/*define MB_TEST*/

void mb_go (int addr, u8 data)
{
  s32 mb_temp;  /* temp 32-bit multiply results */
  s16 mb_q;     /* temp used in division */
  int msb;

#ifdef MB_TEST
  fprintf (stderr, "math box command %02x data %02x  ", addr, data);
#endif

  switch (addr)
    {
    case 0x00: mb_result = REG0 = (REG0 & 0xff00) | data;        break;
    case 0x01: mb_result = REG0 = (REG0 & 0x00ff) | (data << 8); break;
    case 0x02: mb_result = REG1 = (REG1 & 0xff00) | data;        break;
    case 0x03: mb_result = REG1 = (REG1 & 0x00ff) | (data << 8); break;
    case 0x04: mb_result = REG2 = (REG2 & 0xff00) | data;        break;
    case 0x05: mb_result = REG2 = (REG2 & 0x00ff) | (data << 8); break;
    case 0x06: mb_result = REG3 = (REG3 & 0xff00) | data;        break;
    case 0x07: mb_result = REG3 = (REG3 & 0x00ff) | (data << 8); break;
    case 0x08: mb_result = REG4 = (REG4 & 0xff00) | data;        break;
    case 0x09: mb_result = REG4 = (REG4 & 0x00ff) | (data << 8); break;

    case 0x0a: mb_result = REG5 = (REG5 & 0xff00) | data;        break;
      /* note: no function loads low part of REG5 without performing a computation */

    case 0x0c: mb_result = REG6 = data; break;
      /* note: no function loads high part of REG6 */

    case 0x15: mb_result = REG7 = (REG7 & 0xff00) | data;        break;
    case 0x16: mb_result = REG7 = (REG7 & 0x00ff) | (data << 8); break;

    case 0x1a: mb_result = REG8 = (REG8 & 0xff00) | data;        break;
    case 0x1b: mb_result = REG8 = (REG8 & 0x00ff) | (data << 8); break;

    case 0x0d: mb_result = REGa = (REGa & 0xff00) | data;        break;
    case 0x0e: mb_result = REGa = (REGa & 0x00ff) | (data << 8); break;
    case 0x0f: mb_result = REGb = (REGb & 0xff00) | data;        break;
    case 0x10: mb_result = REGb = (REGb & 0x00ff) | (data << 8); break;

    case 0x17: mb_result = REG7; break;
    case 0x19: mb_result = REG8; break;
    case 0x18: mb_result = REG9; break;

    case 0x0b:

      REG5 = (REG5 & 0x00ff) | (data << 8); 

      REGf = 0xffff;
      REG4 -= REG2;
      REG5 -= REG3;

    step_048:

      mb_temp = ((s32) REG0) * ((s32) REG4);
      REGc = mb_temp >> 16;
      REGe = mb_temp & 0xffff;

      mb_temp = ((s32) -REG1) * ((s32) REG5);
      REG7 = mb_temp >> 16;
      mb_q = mb_temp & 0xffff;

      REG7 += REGc;

      /* rounding */
      REGe = (REGe >> 1) & 0x7fff;
      REGc = (mb_q >> 1) & 0x7fff;
      mb_q = REGc + REGe;
      if (mb_q < 0)
	REG7++;

      mb_result = REG7;

      if (REGf < 0)
	break;

      REG7 += REG2;

      /* fall into command 12 */

    case 0x12:

      mb_temp = ((s32) REG1) * ((s32) REG4);
      REGc = mb_temp >> 16;
      REG9 = mb_temp & 0xffff;

      mb_temp = ((s32) REG0) * ((s32) REG5);
      REG8 = mb_temp >> 16;
      mb_q = mb_temp & 0xffff;

      REG8 += REGc;

      /* rounding */
      REG9 = (REG9 >> 1) & 0x7fff;
      REGc = (mb_q >> 1) & 0x7fff;
      REG9 += REGc;
      if (REG9 < 0)
	REG8++;
      REG9 <<= 1;  /* why? only to get the desired load address? */

      mb_result = REG8;

      if (REGf < 0)
	break;

      REG8 += REG3;

      REG9 &= 0xff00;

      /* fall into command 13 */

    case 0x13:
#ifdef MB_TEST
      fprintf (stderr, "\nR7: %04x  R8: %04x  R9: %04x\n", REG7, REG8, REG9);
#endif

      REGc = REG9;
      mb_q = REG8;
      goto step_0bf;

    case 0x14: 
      REGc = REGa;
      mb_q = REGb;

    step_0bf:
      REGe = REG7 ^ mb_q;  /* save sign of result */
      REGd = mb_q;
      if (mb_q >= 0)
	mb_q = REGc;
      else
	{
	  REGd = - mb_q - 1;
	  mb_q = - REGc - 1;
	  if ((mb_q < 0) && ((mb_q + 1) < 0))
	    REGd++;
	  mb_q++;
	}

    /* step 0c9: */
      /* REGc = abs (REG7) */
      if (REG7 >= 0)
	REGc = REG7;
      else
        REGc = -REG7;

      REGf = REG6;  /* step counter */

      do
	{
	  REGd -= REGc;
	  msb = ((mb_q & 0x8000) != 0);
	  mb_q <<= 1;
	  if (REGd >= 0)
	    mb_q++;
	  else
	    REGd += REGc;
	  REGd <<= 1;
	  REGd += msb;
	}
      while (--REGf >= 0);

      if (REGe >= 0)
	mb_result = mb_q;
      else
	mb_result = - mb_q;
      break;

    case 0x11:
      REG5 = (REG5 & 0x00ff) | (data << 8); 
      REGf = 0x0000;  /* do everything in one step */
      goto step_048;
      break;

    case 0x1c:
      /* window test? */
      REG5 = (REG5 & 0x00ff) | (data << 8); 
      do
	{
	  REGe = (REG4 + REG7) >> 1;
	  REGf = (REG5 + REG8) >> 1;
	  if ((REGb < REGe) && (REGf < REGe) && ((REGe + REGf) >= 0))
	    { REG7 = REGe; REG8 = REGf; }
	  else
	    { REG4 = REGe; REG5 = REGf; }
	}
      while (--REG6 >= 0);

      mb_result = REG8;
      break;

    case 0x1d:
      REG3 = (REG3 & 0x00ff) | (data << 8);

      REG2 -= REG0;
      if (REG2 < 0)
	REG2 = -REG2;

      REG3 -= REG1;
      if (REG3 < 0)
	REG3 = -REG3;

      /* fall into command 1e */

    case 0x1e:
      /* result = max (REG2, REG3) + 3/8 * min (REG2, REG3) */
      if (REG3 >= REG2)
        { REGc = REG2; REGd = REG3; }
      else
	{ REGd = REG2; REGc = REG3; }
      REGc >>= 2;
      REGd += REGc;
      REGc >>= 1;
      mb_result = REGd = (REGc + REGd);
      break;

    case 0x1f:
      printf ("math box function 0x1f\r\n");
      /* $$$ do some computation here (selftest? signature analysis? */
      break;
    }

#ifdef MB_TEST
  fprintf (stderr, "  result %04x\n", mb_result & 0xffff);
#endif
}
