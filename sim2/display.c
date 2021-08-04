void avg_reset(unsigned long cyc);

void avg_go(unsigned long cyc);
int avg_done(unsigned long cyc);
void avg_draw_vector_list_t();
void avg_halt(int dummy);

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
 * $Id: display.c,v 1.1 2018/07/31 01:19:44 pi Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include <vectrex/osWrapper.h>
#include <vectrex/vectrexInterface.h>

#include "game.h"
#include "memory.h"
#include "display.h"

int dvg = 0;
int portrait = 0;

#ifdef VG_DEBUG
int trace_vgo = 0;
int vg_step = 0; /* single step the vector generator */
int vg_print = 0;
unsigned long last_vgo_cyc=0;
unsgined long vgo_count=0;
#endif /* VG_DEBUG */

int vg_busy = 0;
unsigned long vg_done_cyc; /* cycle after which VG will be done */
unsigned vector_mem_offset;
int frame = 0;

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

char *avg_mnem[] = { "vctr", "halt", "svec", "stat", "cntr", "jsrl", "rtsl",
		 "jmpl", "scal" };

char *dvg_mnem[] = { "????", "vct1", "vct2", "vct3",
		     "vct4", "vct5", "vct6", "vct7",
		     "vct8", "vct9", "labs", "halt",
		     "jsrl", "rtsl", "jmpl", "svec" };

#define map_addr(n) (((n)<<1)+vector_mem_offset)

#define max(x,y) (((x)>(y))?(x):(y))

             
char displBrowseModeData[160];

             
             
             
static int vector_timer (long deltax, long deltay)
{
  deltax = labs (deltax);
  deltay = labs (deltay);
  return max (deltax, deltay) >> 17;
}

#ifdef NEVER_USED
static void dvg_vector_timer (int scale)
{
  vg_done_cyc += 4 << scale;
}
#endif

int vg_step =0;
int vg_print = 1;
int vgo_count =0;
int trace_vgo = 0;
extern FILE *debugf;
#define DEBUG_OUT2(...) do { ; } while(0)
#define DEBUG_OUT(...) do { ; } while(0)
//#define DEBUG_OUT(...) do { if (!debugf) debugf = fopen("debug-sim.log", "w"); if (debugf) { fprintf(debugf,__VA_ARGS__); fflush(debugf); } } while(0)
//#define DEBUG_OUT2(...) do { if (!debugf) debugf = fopen("debug-sim.log", "w"); if (debugf) { fprintf(debugf,__VA_ARGS__); fflush(debugf); } } while(0)
//#define VG_DEBUG

static void dvg_draw_vector_list (void)
{
  frame++;
//  printf("Frame: %i\r\n", frame);
  static int pc;
  static int sp;
  static int stack [MAXSTACK];

  static long scale;
//static int statz;

  static long currentx;
  static long currenty;

  //int displayCount =0;
  int done = 0;

  int firstwd, secondwd=0;
  int opcode;

  long x, y;
  int z, temp;
  int a;

  long oldx, oldy;
  long deltax, deltay;

#if 0
  if (!cont)
#endif
    {
      pc = 0;
      sp = 0;
      scale = 0;
      // statz = 0;
      if (portrait)
	{
	  currentx = 1023 * 8192;
	  currenty = 512 * 8192;
	}
      else
	{
	  currentx = 512 * 8192;
	  currenty = 1023 * 8192;
	}
    }

#ifdef VG_DEBUG
  open_page (vg_step);
#else
  open_page (0);
#endif

  while (!done)
    {
      vg_done_cyc += 1;//8;
DEBUG_OUT( "      vg_done at: %i (+ loop)\r\n", vg_done_cyc);
#ifdef VG_DEBUG
      if (vg_step)
	{
	  printf ("Current beam position: (%d, %d)\r\n", , currenty);
	  getchar();
	}
#endif
      firstwd = memrdwd (map_addr (pc), 0, 0);
      opcode = firstwd >> 12;
#ifdef VG_DEBUG
      if (vg_print) 	printf ("%4x: %4x ", map_addr (pc), firstwd);
#endif
      pc++;
      if ((opcode >= 0 /* DVCTR */) && (opcode <= DLABS))
	{
	  secondwd = memrdwd (map_addr (pc), 0, 0);
	  pc++;
#ifdef VG_DEBUG
	  if (vg_print) printf ("%4x  ", secondwd);
#endif
	}
#ifdef VG_DEBUG
      else
	if (vg_print)  printf ("      ");
#endif

#ifdef VG_DEBUG
      if (vg_print) printf ("%s ", dvg_mnem [opcode]);
#endif

      switch (opcode)
	{
	case 0:
#ifdef DVG_OP_0_ERR
	  printf ("Error: DVG opcode 0!  Addr %4x Instr %4x %4x\r\n", map_addr (pc-2), firstwd, secondwd);
	  done = 1;
	  break;
#endif
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	  y = firstwd & 0x03ff;
	  if (firstwd & 0x0400)
	    y = -y;
	  x = secondwd & 0x03ff;
	  if (secondwd & 0x400)
	    x = -x;
	  z = secondwd >> 12;
#ifdef VG_DEBUG
	  if (vg_print)
	    {
	      printf ("(%d,%d) z: %d scal: %d", x, y, z, opcode);
	    }
#endif
#if 0
	  if (vg_step)
	    {
	      printf ("\nx: %x  x<<21: %x  (x<<21)>>%d: %x\r\n", x, x<<21, 30-(scale+opcode), (x<<21)>>(30-(scale+opcode)));
	      printf ("y: %x  y<<21: %x  (y<<21)>>%d: %x\r\n", y, y<<21, 30-(scale+opcode), (y<<21)>>(30-(scale+opcode)));
	    }
#endif
	  oldx = currentx; oldy = currenty;
	  temp = (scale + opcode) & 0x0f;
	  if (temp > 9)
	    temp = -1;
	  deltax = (x << 21) >> (30 - temp);
	  deltay = (y << 21) >> (30 - temp);
#if 0
	  if (vg_step)
	    {
	      printf ("deltax: %x  deltay: %x\r\n", deltax, deltay);
	    }
#endif
	  currentx += deltax;
	  currenty -= deltay;
//	  dvg_vector_timer (temp);
//---------
    int _scale = (scale + opcode) & 0xf;
    int fin = 0xfff - (((2 << _scale) & 0x7ff) ^ 0xfff);
    const int cycles = 8 * fin;
    vg_done_cyc += cycles/8;
//---------
  DEBUG_OUT("long cycles: %i\n",cycles);
      
      
      
      
      
      
      
      
      
      
      
      
      
      
	  draw_line (oldx, oldy, currentx, currenty, 7, z);
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
#ifdef VG_DEBUG
	  if (vg_print)
	    {
	      printf ("(%d,%d) scal: %d", x, y, secondwd >> 12);
	    }
#endif
	  break;

	case DHALT:
#ifdef VG_DEBUG
	  if (vg_print)
	    if ((firstwd & 0x0fff) != 0)
	      printf ("(%d?)", firstwd & 0x0fff);
#endif
	  done = 1;
	  break;

	case DJSRL:
	  a = firstwd & 0x0fff;
#ifdef VG_DEBUG
	  if (vg_print)
	    printf ("%4x", map_addr(a));
#endif
	  stack [sp] = pc;
	  if (sp == (MAXSTACK - 1))
	    {
	      printf ("\n*** Vector generator stack overflow! ***\r\n");
	      done = 1;
	      sp = 0;
	    }
	  else
	    sp++;
	  pc = a;
	  break;
	
	case DRTSL:
#ifdef VG_DEBUG
	  if (vg_print)
	    if ((firstwd & 0x0fff) != 0)
	      printf ("(%d?)", firstwd & 0x0fff);
#endif
	  if (sp == 0)
	    {
	      printf ("\n*** Vector generator stack underflow! ***\r\n");
	      done = 1;
	      sp = MAXSTACK - 1;
	    }
	  else
	    sp--;
	  pc = stack [sp];
	  break;

	case DJMPL:
	  a = firstwd & 0x0fff;
#ifdef VG_DEBUG
	  if (vg_print)
	    printf ("%4x", map_addr(a));
#endif
	  pc = a;
	  break;
	
	case DSVEC:
	  y = firstwd & 0x0300;
	  if (firstwd & 0x0400)
	    y = -y;
	  x = (firstwd & 0x03) << 8;
	  if (firstwd & 0x04)
	    x = -x;
	  z = (firstwd >> 4) & 0x0f;
	  temp = 2 + ((firstwd >> 2) & 0x02) + ((firstwd >> 11) & 0x01);
#ifdef VG_DEBUG3000
	  if (vg_print)
	    {
	      printf ("(%d,%d) z: %d scal: %d", x, y, z, temp);
	    }
#endif
#if 0
	  if (vg_step)
	    {
	      printf ("\nx: %x  x<<21: %x  (x<<21)>>%d: %x\r\n", x, x<<21, 30-(scale+temp), (x<<21)>>(30-(scale+temp)));
	      printf ("y: %x  y<<21: %x  (y<<21)>>%d: %x\r\n", y, y<<21, 30-(scale+temp), (y<<21)>>(30-(scale+temp)));
	    }
#endif
	  oldx = currentx; oldy = currenty;
	  temp = (scale + temp) & 0x0f;
	  if (temp > 9)
	    temp = -1;
	  deltax = (x << 21) >> (30 - temp);
	  deltay = (y << 21) >> (30 - temp);
#if 0
	  if (vg_step)
	    {
	      printf ("deltax: %x  deltay: %x\r\n", deltax, deltay);
	    }
#endif
	  currentx += deltax;
	  currenty -= deltay;
//	  dvg_vector_timer (temp);
      
      
//--------------------
#ifdef NEVER_USED
    int _scale2 = (scale +
                    (((deltay & 0x800) >> 11)
                    | (((deltax & 0x800) ^ 0x800) >> 10)
                    | ((deltax & 0x800)  >> 9))) & 0xf;

    int fin2 = 0xfff - (((2 << _scale2) & 0x7ff) ^ 0xfff);
    const int cycles2 = 8 * fin2;
//  vg_done_cyc += cycles2;
#endif
    vg_done_cyc += ((4 << temp)*4)/8;
  DEBUG_OUT("short cycles: %i\n",(4 << temp)*4);

//--------------------
      
      
      
      

      
      
      
      
      
      
	  draw_line (oldx, oldy, currentx, currenty, 7, z);
	  break;

	default:
	  printf ("display: internal error\r\n");
	  done = 1;
	}
#ifdef VG_DEBUG
      if (vg_print) printf ("\n");
#endif
    }

  close_page ();
}

//#define VG_DEBUG
static void avg_draw_vector_list()
{
  frame++;
  static int pc;
  static int sp;
  static int stack [MAXSTACK];

  static long scale;
  static int statz;
  static int color;

  static long currentx;
  static long currenty;

  int done = 0;

  int firstwd, secondwd;
  int opcode;

  int x, y, z, b, l, a;
#ifdef VG_DEBUG
  int d;
#endif
//int displayCount=0;

  
  
  
  long oldx, oldy;
  long deltax, deltay;


  pc = 0;
  sp = 0;
  scale = 16384;
  statz = 0;
  color = 0;
  if (portrait)
  {
	  currentx = 384 * 8192;
	  currenty = 512 * 8192;
  }
  else
  {
	  currentx = 512 * 8192;
	  currenty = 384 * 8192;
  }

  firstwd = memrdwd (map_addr (pc), 0, 0);
  secondwd = memrdwd (map_addr (pc+1), 0, 0);
  if ((firstwd == 0) && (secondwd == 0))
  {
    printf ("VGO with zeroed vector memory\r\n");
    return;
  }
    

#ifdef VG_DEBUG
  open_page (vg_step);
#else
  open_page (0);
#endif
  while (!done)
    {
      vg_done_cyc += 8;
      
#ifdef VG_DEBUG
      if (vg_step) getchar();
#endif
      firstwd = memrdwd (map_addr (pc), 0, 0);
      opcode = firstwd >> 13;
#ifdef VG_DEBUG
      if (vg_print) printf ("%4x: %4x ", map_addr (pc), firstwd);
#endif
      pc++;
      if (opcode == VCTR)
	{
	  secondwd = memrdwd (map_addr (pc), 0, 0);
	  pc++;
#ifdef VG_DEBUG
	  if (vg_print)  printf ("%4x  ", secondwd);
#endif
	}
#ifdef VG_DEBUG
      else
	if (vg_print)  printf ("      ");
#endif

      if ((opcode == STAT) && ((firstwd & 0x1000) != 0))
	opcode = SCAL;

#ifdef VG_DEBUG
      if (vg_print) 	printf ("%s ", avg_mnem [opcode]);
#endif

      switch (opcode)
	{
	case VCTR:
	  x = twos_comp_val (secondwd,13);
	  y = twos_comp_val (firstwd,13);
	  z = 2 * (secondwd >> 13);
#ifdef VG_DEBUG
	  if (vg_print)    printf ("%d,%d,", x, y);
#endif
	  if (z == 0)
	    {
#ifdef VG_DEBUG
	      if (vg_print) printf ("blank");
#endif
	    }
	  else if (z == 2)
	    {
	      z = statz;
#ifdef VG_DEBUG
	      if (vg_print) printf ("stat (%d)", z);
#endif
	    }
#ifdef VG_DEBUG
	  else
	    if (vg_print)   printf ("%d", z);
#endif
	  oldx = currentx; oldy = currenty;
	  deltax = x * scale; deltay = y * scale;
	  currentx += deltax;
	  currenty -= deltay;
	  vg_done_cyc += vector_timer (deltax, deltay);
      
      
      
	  draw_line (oldx>>13, oldy>>13, currentx>>13, currenty>>13, color, z);
//printf("\nline color: %i, intensity: %i\n", color, z);
	  break;
	
	case SVEC:
	  x = twos_comp_val (firstwd, 5) << 1;
	  y = twos_comp_val (firstwd >> 8, 5) << 1;
	  z = 2 * ((firstwd >> 5) & 7);
#ifdef VG_DEBUG
	  if (vg_print)   printf ("%d,%d,", x, y);
#endif
	  if (z == 0)
	    {
#ifdef VG_DEBUG
	      if (vg_print) printf ("blank");
#endif
	    }
	  else if (z == 2)
	    {
	      z = statz;
#ifdef VG_DEBUG
	      if (vg_print) 	printf ("stat");
#endif
	    }
#ifdef VG_DEBUG
	  else
	    if (vg_print)   printf ("%d", z);
#endif
	  oldx = currentx; oldy = currenty;
	  deltax = x * scale; deltay = y * scale;
	  currentx += deltax;
	  currenty -= deltay;
      vg_done_cyc += vector_timer (deltax, deltay);

      
      draw_line (oldx>>13, oldy>>13, currentx>>13, currenty>>13, color, z);
//printf("\nline color: %i, intensity: %i\n", color, z);
	  break;
	
	case STAT:
	  color = firstwd & 0x0f;
	  statz = (firstwd >> 4) & 0x0f;
#ifdef VG_DEBUG
	  if (vg_print)   printf ("z: %d color: %d", statz, color);
#endif
	  /* should do e, h, i flags here! */
	  break;
      
	case SCAL:
	  b = (firstwd >> 8) & 0x07;
	  l = firstwd & 0xff;
	  scale = (16384 - (l << 6)) >> b;
	  /* scale = (1.0-(l/256.0)) * (2.0 / (1 << b)); */
#ifdef VG_DEBUG
	  if (vg_print)
      {
        printf ("bin: %d, lin: ", b);
        if (l > 0x80) printf ("(%d?)", l);
        else printf ("%d", l);
        printf (" scale: %f", (scale/8192.0));
      }
#endif
	  break;
	
	case CNTR:
#ifdef VG_DEBUG
	  d = firstwd & 0xff;
	  if (vg_print)
      {
        if (d != 0x40) 	printf ("%d", d);
      }
#endif
	  if (portrait)
      {
        currentx = 384 * 8192;
        currenty = 512 * 8192;
      }
        else
      {
        currentx = 512 * 8192;
        currenty = 384 * 8192;
      }
	  break;
	
	case RTSL:
#ifdef VG_DEBUG
	  if (vg_print)
	    if ((firstwd & 0x1fff) != 0)     printf ("(%d?)", firstwd & 0x1fff);
#endif
	  if (sp == 0)
      {
        printf ("\n*** Vector generator stack underflow! ***\r\n");
        done = 1;
        sp = MAXSTACK - 1;
      }
	  else
	    sp--;
	  pc = stack [sp];
	  break;

	case HALT:
#ifdef VG_DEBUG
	  if (vg_print)
	    if ((firstwd & 0x1fff) != 0)     printf ("(%d?)", firstwd & 0x1fff);
#endif
	  done = 1;
	  break;

	case JMPL:
	  a = firstwd & 0x1fff;
#ifdef VG_DEBUG
	  if (vg_print)     printf ("%4x", map_addr(a));
#endif
	  pc = a;
	  break;
	
	case JSRL:
	  a = firstwd & 0x1fff;
#ifdef VG_DEBUG
	  if (vg_print)  printf ("%4x", map_addr(a));
#endif
	  stack [sp] = pc;
	  if (sp == (MAXSTACK - 1))
      {
        printf ("\n*** Vector generator stack overflow! ***\r\n");
        done = 1;
        sp = 0;
      }
	  else
	    sp++;
	  pc = a;
	  break;
	
	default:
	  printf ("display: 2 internal error\r\n");
	}
#ifdef VG_DEBUG
      if (vg_print) 	printf ("\n");
#endif
    }

  close_page ();
}










int vg_done (unsigned long cyc)
{
  if (game ==TEMPEST)
	return avg_done(cyc);

  if (vg_busy && (cyc > vg_done_cyc))
    vg_busy = 0;
  DEBUG_OUT( "vg_done: vg_busy: %i, cyc:%i, cycDone:%i\r\n",vg_busy, cyc, vg_done_cyc);
  return (! vg_busy);
}

int drop_frames = 0;
static int df = 1;
int last_vgo_cyc = 0;

void vg_go (unsigned long cyc)
{
  vg_busy = 1;
  vg_done_cyc = cyc + 8;
DEBUG_OUT( "vg_go: start %i, vg_done at: %i\r\n",cyc, vg_done_cyc);
#ifdef VG_DEBUG
  vgo_count++;
  if (trace_vgo) printf("VGO #%d at cycle %d, delta %d\r\n", vgo_count, cyc, cyc-last_vgo_cyc);
  last_vgo_cyc = cyc;
#endif

  if (--df == 0)
  {
      df = (drop_frames > 0) ? drop_frames : 1;
      if (dvg)
      {
        dvg_draw_vector_list ();
      }
      else
      {
        if (game ==TEMPEST)
		{
		  avg_go(cyc);
		}
        else
          avg_draw_vector_list();
      }
    }
}

void vg_reset (unsigned long cyc)
{
  vg_busy = 0;
#ifdef _VG_DEBUG
  if (trace_vgo)
    printf ("vector generator reset @%04x\r\n", PC);
#endif
  if (game ==TEMPEST)
	avg_reset(0);
}








#define OP0 (m_op & 1)
#define OP1 (m_op & 2)
#define OP2 (m_op & 4)
#define OP3 (m_op & 8)

#define ST3 (m_state_latch & 8)


#define u8 unsigned char
#define u16 unsigned short int
#define s32 signed int

u16 m_pc;
u8 m_sp;
u16 m_dvx;
u16 m_dvy;
u8 m_dvy12;
u16 m_timer;
u16 m_stack[4];
u16 m_data;

u8 m_state_latch;
u8 m_int_latch;
u8 m_scale;
u8 m_bin_scale;
u8 m_intensity;
u8 m_color;
u8 m_enspkl;
u8 m_spkl_shift;
u8 m_map;

u16 m_hst;
u16 m_lst;
u16 m_izblank;

u8 m_op;
u8 m_halt;
u8 m_sync_halt;

u16 m_xdac_xor;
u16 m_ydac_xor;

s32 m_xpos;
s32 m_ypos;

s32 m_clipx_min;
s32 m_clipy_min;
s32 m_clipx_max;
s32 m_clipy_max;

int m_xmin, m_xmax, m_ymin, m_ymax;
int m_xcenter, m_ycenter;


u16 m_vectorram_offset;
u16 m_colorram_offset;
u8 *m_prom;
u8 avg_prom[256];

#define rdColor(c) memrd((c)+m_colorram_offset,0,0)
#define rdVram(r) memrd((r)+m_vectorram_offset,0,0)
#define rdProm(p) avg_prom[(p)]



int old_x=0;
int old_y=0;
extern void draw_line2(int FromX, int FromY, int ToX, int ToY, int Colour15, int z); // z is 0:12 in lunar, colour is always 7

void vg_add_point_buf(int x, int y, int color, int intensity)
{
#define SHIFT_T 10
// printf("draw_line: %i,%i,%i,%i,%i,%i\n", (old_x>>SHIFT_T), -(old_y>>SHIFT_T), (x>>SHIFT_T), -(y>>SHIFT_T), color, intensity>>4);
//  draw_line2 ((old_x>>SHIFT_T), -(old_y>>SHIFT_T), (x>>SHIFT_T), -(y>>SHIFT_T), color, intensity>>4);
  draw_line2 ((old_y>>SHIFT_T), -(old_x>>SHIFT_T), (y>>SHIFT_T), -(x>>SHIFT_T), color, intensity>>4);
	old_x = x;
	old_y = y;
}


/********************************************************************
 *
 *  AVG handler functions
 *
 *  AVG is in many ways different from DVG. The only thing they have
 *  in common is the state machine approach. There are small
 *  differences among the AVGs, mostly related to color and vector
 *  clipping.
 *
 *******************************************************************/
void avg_init(u16 vram, u16 cram)
{
	/* AVG PROM */
//	ROM_REGION( 0x100, "avg:prom", 0 )
//	ROM_LOAD( "136002-125.d7",   0x0000, 0x0100, CRC(5903af03) SHA1(24bc0366f394ad0ec486919212e38be0f08d0239) )
	m_prom = avg_prom;

//    2000-2FFF  R/W   D  D  D  D  D  D  D  D   Vector Ram (4K)
//    3000-3FFF   R    D  D  D  D  D  D  D  D   Vector Rom (4K)
	m_vectorram_offset=vram;
	

//    0800-080F   W                D  D  D  D   Colour ram
	m_colorram_offset=cram;
		
	m_pc = 0;
	m_sp = 0;
	m_dvx = 0;
	m_dvy = 0;
	m_dvy12 = 0;
	m_timer = 0;
	m_stack[0] = 0;
	m_stack[1] = 0;
	m_stack[2] = 0;
	m_stack[3] = 0;
	m_data = 0;

	m_state_latch = 0;
	m_int_latch = 0;
	m_scale = 0;
	m_bin_scale = 0;
	m_intensity = 0;
	m_color = 0;
	m_enspkl = 0;
	m_spkl_shift = 0;
	m_map = 0;

	m_hst = 0;
	m_lst = 0;
	m_izblank = 0;

	m_op = 0;
	m_halt = 0;
	m_sync_halt = 0;

	m_xdac_xor = 0;
	m_ydac_xor = 0;

	m_xpos = 0;
	m_ypos = 0;


	m_xcenter = 0;
	m_ycenter = 0;
    old_x=0;
    old_y=0;
	

//    m_xcenter = ((m_xmax - m_xmin) / 2) << 15;
//    m_ycenter = ((m_ymax - m_ymin) / 2) << 15;

    
    /*
     * The x and y DACs use 10 bit of the counter values which are in
     * two's complement representation. The DAC input is xored with
     * 0x200 to convert the value to unsigned.
     */
    m_xdac_xor = 0x200;
    m_ydac_xor = 0x200;
}
 

u8 state_addr() // avg_state_addr
{
	return (((m_state_latch >> 4) ^ 1) << 7)
		| (m_op << 4)
		| (m_state_latch & 0xf);
}


void update_databus() // avg_data
{
	m_data = rdVram((m_pc&0x1fff) ^ 1); // 0x2000 being the vectorram+rom length - this should not go out of bounds!
}

void vggo() // avg_vggo
{
	m_pc = 0;
	m_sp = 0;
}


void vgrst() // avg_vgrst
{
	m_state_latch = 0;
	m_bin_scale = 0;
	m_scale = 0;
	m_color = 0;
}

int handler_0() // avg_latch0
{
	m_dvy = (m_dvy & 0x1f00) | m_data;
	m_pc++;

	return 0;
}

int handler_1() // avg_latch1
{
	m_dvy12 = (m_data >> 4) & 1;
	m_op = m_data >> 5;

	m_int_latch = 0;
	m_dvy = (m_dvy12 << 12) | ((m_data & 0xf) << 8);
	m_dvx = 0;
	m_pc++;

	return 0;
}

int handler_2() // avg_latch2
{
	m_dvx = (m_dvx & 0x1f00) | m_data;
	m_pc++;

	return 0;
}

int handler_3() // avg_latch3
{
	m_int_latch = m_data >> 4;
	m_dvx = ((m_int_latch & 1) << 12) | ((m_data & 0xf) << 8) | (m_dvx & 0xff);
	m_pc++;

	return 0;
}

int handler_4() // avg_strobe0
{
	if (OP0)
	{
		m_stack[m_sp & 3] = m_pc;
	}
	else
	{
		/*
		 * Normalization is done to get roughly constant deflection
		 * speeds. See Jed's essay why this is important. In addition
		 * to the intensity and overall time saving issues it is also
		 * needed to avoid accumulation of DAC errors. The X/Y DACs
		 * only use bits 3-12. The normalization ensures that the
		 * first three bits hold no important information.
		 *
		 * The circuit doesn't check for dvx=dvy=0. In this case
		 * shifting goes on as long as VCTR, SCALE and CNTR are
		 * low. We cut off after 16 shifts.
		 */
		int i = 0;
		while ((((m_dvy ^ (m_dvy << 1)) & 0x1000) == 0)
				&& (((m_dvx ^ (m_dvx << 1)) & 0x1000) == 0)
				&& (i++ < 16))
		{
			m_dvy = (m_dvy & 0x1000) | ((m_dvy << 1) & 0x1fff);
			m_dvx = (m_dvx & 0x1000) | ((m_dvx << 1) & 0x1fff);
			m_timer >>= 1;
			m_timer |= 0x4000 | (OP1 << 6);
		}

		if (OP1)
			m_timer &= 0xff;
	}

	return 0;
}


int avg_common_strobe1()
{
	if (OP2)
	{
		if (OP1)
			m_sp = (m_sp - 1) & 0xf;
		else
			m_sp = (m_sp + 1) & 0xf;
	}
	return 0;
}

int handler_5() // avg_strobe1
{
	if (OP2 == 0)
	{
		for (int i = m_bin_scale; i > 0; i--)
		{
			m_timer >>= 1;
			m_timer |= 0x4000 | (OP1 << 6);
		}
		if (OP1)
			m_timer &= 0xff;
	}

	return avg_common_strobe1();
}


int avg_common_strobe2()
{
	if (OP2)
	{
		if (OP0)
		{
			m_pc = m_dvy << 1;

			if (m_dvy == 0)
			{
				/*
				 * Tempest and Quantum keep the AVG in an endless
				 * loop. I.e. at one point the AVG jumps to address 0
				 * and starts over again. The main CPU updates vector
				 * RAM while AVG is running. The hardware takes care
				 * that the AVG doesn't read vector RAM while the CPU
				 * writes to it. Usually we wait until the AVG stops
				 * (halt flag) and then draw all vectors at once. This
				 * doesn't work for Tempest and Quantum so we wait for
				 * the jump to zero and draw vectors then.
				 *
				 * Note that this has nothing to do with the real hardware
				 * because for a vector monitor it is perfectly okay to
				 * have the AVG drawing all the time. In the emulation we
				 * somehow have to divide the stream of vectors into
				 * 'frames'.
				 */
			}
		}
		else
		{
			m_pc = m_stack[m_sp & 3];
		}
	}
	else
	{
		if (m_dvy12)
		{
			m_scale = m_dvy & 0xff;
			m_bin_scale = (m_dvy >> 8) & 7;
		}
	}

	return 0;
}

int handler_6() // avg_strobe2
{
	if ((OP2 == 0) && (m_dvy12 == 0))
	{
		m_color = m_dvy & 0x7;
		m_intensity = (m_dvy >> 4) & 0xf;
	}

	return avg_common_strobe2();
}

int avg_common_strobe3()
{
	int cycles = 0;

	m_halt = OP0;

	if ((m_op & 5) == 0)
	{
		if (OP1)
		{
			cycles = 0x100 - (m_timer & 0xff);
		}
		else
		{
			cycles = 0x8000 - m_timer;
		}
		m_timer = 0;

		m_xpos += ((((m_dvx >> 3) ^ m_xdac_xor) - 0x200) * cycles * (m_scale ^ 0xff)) >> 4;
		m_ypos -= ((((m_dvy >> 3) ^ m_ydac_xor) - 0x200) * cycles * (m_scale ^ 0xff)) >> 4;
	}
	if (OP2)
	{
		cycles = 0x8000 - m_timer;
		m_timer = 0;
		m_xpos = m_xcenter;
		m_ypos = m_ycenter;
		vg_add_point_buf(m_xpos, m_ypos, 0, 0);
	}

	return cycles;
}

int handler_7() // avg_strobe3
{
	const int cycles = avg_common_strobe3();

	if ((m_op & 5) == 0)
	{
		vg_add_point_buf(m_xpos, m_ypos, m_color, (((m_int_latch >> 1) == 1)? m_intensity: m_int_latch & 0xe) << 4);
	}

	return cycles;
}

/*************************************
 *
 *  Tempest handler functions
 *
 *************************************/

int tempest_handler_6() // tempest_strobe2
{
	if ((OP2 == 0) && (m_dvy12 == 0))
	{
		/* Contrary to previous documentation in MAME,
		Tempest does not have the m_enspkl bit. */
		if (m_dvy & 0x800)
			m_color = m_dvy & 0xf;
		else
			m_intensity = (m_dvy >> 4) & 0xf;
	}

	return avg_common_strobe2();
}

int rgb_t(u8 r, u8 g, u8 b)
{
  return ((r+g+b)/3) / 32;
}
int tempest_handler_7() // tempest_strobe3
{
	const int cycles = avg_common_strobe3();

	if ((m_op & 5) == 0)
	{
		const u8 data = rdColor(m_color);
		const u8 bit3 = (~data >> 3) & 1;
		const u8 bit2 = (~data >> 2) & 1;
		const u8 bit1 = (~data >> 1) & 1;
		const u8 bit0 = (~data >> 0) & 1;

		const u8 r = bit1 * 0xf3 + bit0 * 0x0c;
		const u8 g = bit3 * 0xf3;
		const u8 b = bit2 * 0xf3;

		int x = m_xpos;
		int y = m_ypos;

		vg_add_point_buf(y - m_ycenter + m_xcenter,
							x - m_xcenter + m_ycenter, rgb_t(r, g, b),
							(((m_int_latch >> 1) == 1)? m_intensity: m_int_latch & 0xe) << 4);
	}

	return cycles;
}

/*************************************
 *
 *  halt functions
 *
 *************************************/

void avg_halt(int dummy)
{
	m_halt = dummy;
	m_sync_halt = dummy;
}

/********************************************************************
 *
 * State Machine
 *
 * The state machine is a 256x4 bit PROM connected to a latch. The
 * address of the next state is generated from the latched previous
 * state, an op code and the halt flag. Op codes come from vector
 * RAM/ROM. The state machine is clocked with 1.5 MHz. Three bits of
 * the state are decoded and used to trigger various parts of the
 * hardware.
 *
 *******************************************************************/

void avg_draw_vector_list_t()
{
	int cycles = 0;
	int oldHalt = 0;
	//int round=0;
	open_page (0);
	if (!m_halt)
	{
	  do
	  {
		  /* Get next state */
		  m_state_latch = (m_state_latch & 0x10) | (rdProm(state_addr()) & 0xf);

		  if (ST3)
		  {
			  /* Read vector RAM/ROM */
			  // "reading" after the last AVG mem
			  // 0xffff
			  // after handler 0 + handler 1
			  // this generates a "Unknown rd addr 4001 data 00 tag 03"
			  // break...
			  // 
			  update_databus();

			  /* Decode state and call the corresponding handler */
			  switch(m_state_latch & 7) {
				  case 0 : cycles += handler_0(); break;
				  case 1 : cycles += handler_1(); break;
				  case 2 : cycles += handler_2(); break;
				  case 3 : cycles += handler_3(); break;
				  case 4 : cycles += handler_4(); break;
				  case 5 : cycles += handler_5(); break;
				  case 6 : cycles += tempest_handler_6() /*handler_6()*/; break;
				  case 7 : cycles += tempest_handler_7() /*handler_7()*/; break;
			  }
		  }

		  /* If halt flag was set, let CPU catch up before we make halt visible */
  //		if (m_halt && !(m_state_latch & 0x10))
  //			m_vg_halt_timer->adjust(attotime::from_hz(MASTER_CLOCK) * cycles, 1);

		  if ((m_halt) && (oldHalt==0))
		  {
			oldHalt = 1;
		  } 
		  m_state_latch = (m_halt << 4) | (m_state_latch & 0xf);
		  cycles += 8;
	  }
	  while ((m_pc!=0) && (!m_halt));
	}
//    printf("vg did run for: %i\n", cycles);
  close_page ();
}


/*************************************
 *
 *  VG halt/vggo
 *
 ************************************/

int avg_done(unsigned long cyc)
{
  
    return m_halt ? 1 : 0;
//    return m_sync_halt ? 1 : 0;
}

void avg_go(unsigned long cyc)
{
  vggo();
	/*
	if (m_sync_halt && (m_nvect > 10))
	{
		/ *
		 * This is a good time to start a new frame. Major Havoc
		 * sometimes sets VGGO after a very short vector list. That's
		 * why we ignore frames with less than 10 vectors.
		 * /
		 
		 // non tempest
	}
*/
	
	avg_halt(0);
	avg_draw_vector_list_t();
 }

/*************************************
 *
 *  Reset
 *
 ************************************/

void avg_reset (unsigned long cyc)
{
	vgrst();
	avg_halt(1);
}

