/*
 * debugger.c: debugger for Atari Vector game simulator
 *
 * Copyright 1991, 1992, 1993, 1996, 2003 Hedley Rainnie and Eric Smith
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
 * $Id: debugger.c,v 1.1 2018/07/31 01:19:44 pi Exp $
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef FREESTANDING
#include <ff.h>
#include <baremetal/rpi-aux.h>
#include <baremetal/rpi-base.h>
#include <baremetal/rpi-gpio.h>
extern char * getErrorText(int errNo);
#else
#endif
  

#include "memory.h"
#include "debugger.h"
#include "sim6502.h"
#include "dis6502.h"
#include "game.h"
#include "display.h"

unsigned long cyc_wraps;


#define MAX_ARGS 16

static int runflag;

static int next_addr = 0;

struct hdr 
{
  unsigned short pc;
  unsigned short a;
  unsigned short x;
  unsigned short y;
  unsigned short sp;
  unsigned short flags;
  unsigned long totcycles;
  unsigned long cyc_wraps;
  unsigned long irq_cycle;
  unsigned long icount;
};
#ifdef FREESTANDING
static int dumpworld (char *name)
{
  FILE *fp;
  struct hdr h;
  byte tagr, tagw;
  byte val;
  long i;
  
  FRESULT rc_rd = FR_DISK_ERR;
  FIL file_object_wr;
  // always as a "new file"
  rc_rd = f_open(&file_object_wr, name, (unsigned char) FA_CREATE_ALWAYS|FA_WRITE);

  if (rc_rd != FR_OK)
  {
    printf("Could not open file %s (%s) \r\n", name, getErrorText(rc_rd));
    printf("Cannot save to dumpfile %s\r\n", name);
    return 1;
  }
  h.pc = save_PC - 1;
  h.a = save_A;
  h.x = save_X;
  h.y = save_Y;
  h.sp = SP;
  h.flags = save_flags;
#ifdef INST_COUNT
  h.icount = icount;
#else
  h.icount = 0;
#endif
  h.totcycles = save_totcycles;
#ifdef WRAP_CYC_COUNT
  h.cyc_wraps = cyc_wraps;
#else
  h.cyc_wraps = 0;
#endif
  h.irq_cycle = irq_cycle;
  

  unsigned int lenSaved=0;
  rc_rd = f_write(&file_object_wr, &h, sizeof(h), &lenSaved);
  if ( rc_rd!= FR_OK)
  {
    printf("File not saved (1) (size written = %i) (error: %s)\r\n", lenSaved, getErrorText(rc_rd));
    f_close(&file_object_wr);
    return 1;
  }
  
  for(i=0;i < 65536;i++) 
    {
      tagr = mem[i].tagr;
      tagw = mem[i].tagw;
      val  = mem[i].cell;
      rc_rd = f_write(&file_object_wr,&tagr, 1, &lenSaved);
      rc_rd = f_write(&file_object_wr,&tagw, 1, &lenSaved);
      rc_rd = f_write(&file_object_wr,&val, 1, &lenSaved);
    }
  f_close(&file_object_wr);
  
  
  return (0);
}

int reload (char *name)
{
  struct hdr h;
  byte tagr, tagw;
  byte val;
  long i;
    
  
  FRESULT rc_rd = FR_DISK_ERR;
  FIL file_object_rd;
  rc_rd = f_open(&file_object_rd, name, (unsigned char) FA_READ);
  if (rc_rd != FR_OK)
  {
    printf("Could not open file %s (%s) \r\n", name, getErrorText(rc_rd));
      printf("Cannot read dumpfile %s\r\n", name);
    return 0;
  }

  unsigned int lenLoaded=0;
  rc_rd = f_read(&file_object_rd, &h, sizeof(h), &lenLoaded);
  if (( rc_rd!= FR_OK) || (sizeof(h) != lenLoaded))
  {
    printf("Read(1) of %s fails (len loaded: %i) (Error: %s)\r\n", name, lenLoaded, getErrorText(rc_rd));
    f_close(&file_object_rd);
    return 0;
  }
  save_PC = h.pc;
  save_A = h.a;
  save_X = h.x;
  save_Y = h.y;
  SP = h.sp;
  save_flags = h.flags;
#ifdef INST_COUNT
  icount = h.icount;
#endif
  irq_cycle = h.irq_cycle;
  save_totcycles = h.totcycles;
#ifdef WRAP_CYC_COUNT
  cyc_wraps = h.cyc_wraps;
#endif
    
  for(i=0;i < 65536;i++) 
    {
      rc_rd = f_read(&file_object_rd, &tagr,1,  &lenLoaded);
      rc_rd = f_read(&file_object_rd, &tagw,1,  &lenLoaded);
      rc_rd = f_read(&file_object_rd, &val,1,  &lenLoaded);
      mem [i].tagr = tagr;
      mem [i].tagw = tagw;
      mem [i].cell = val;
    }

  f_close(&file_object_rd);
  return (0);
}
#else
static int dumpworld (char *s)
{
  FILE *fp;
  struct hdr h;
  byte tagr, tagw;
  byte val;
  long i;
  
  fp = fopen(s, "wb");
  if(!fp) 
    {
      printf("Cannot save to dumpfile %s\n", s);
      return (1);
    }
  h.pc = save_PC - 1;
  h.a = save_A;
  h.x = save_X;
  h.y = save_Y;
  h.sp = SP;
  h.flags = save_flags;
#ifdef INST_COUNT
  h.icount = icount;
#else
  h.icount = 0;
#endif
  h.totcycles = save_totcycles;
#ifdef WRAP_CYC_COUNT
  h.cyc_wraps = cyc_wraps;
#else
  h.cyc_wraps = 0;
#endif
  h.irq_cycle = irq_cycle;
  fwrite(&h, sizeof(h), 1, fp);
    
  for(i=0;i < 65536;i++) 
    {
      tagr = mem[i].tagr;
      tagw = mem[i].tagw;
      val  = mem[i].cell;
      fwrite (&tagr, 1, 1, fp);
      fwrite (&tagw, 1, 1, fp);
      fwrite (&val, 1, 1, fp);
    }
  fclose(fp);
  return (0);
}

int reload (char *s)
{
  FILE *fp;
  struct hdr h;
  byte tagr, tagw;
  byte val;
  long i;
    
  fp = fopen(s, "rb");
  if(!fp) 
    {
      printf("Cannot read dumpfile %s\n", s);
      return (1);
    }
  fread(&h, sizeof(h), 1, fp);
  save_PC = h.pc;
  save_A = h.a;
  save_X = h.x;
  save_Y = h.y;
  SP = h.sp;
  save_flags = h.flags;
#ifdef INST_COUNT
  icount = h.icount;
#endif
  irq_cycle = h.irq_cycle;
  save_totcycles = h.totcycles;
#ifdef WRAP_CYC_COUNT
  cyc_wraps = h.cyc_wraps;
#endif
    
  for(i=0;i < 65536;i++) 
    {
      fread (&tagr, 1, 1, fp);
      fread (&tagw, 1, 1, fp);
      fread (&val, 1, 1, fp);
      mem [i].tagr = tagr;
      mem [i].tagw = tagw;
      mem [i].cell = val;
    }

  fclose(fp);
  return (0);
}
#endif


static char hex_digits [] = "0123456789abcdef";


/*
 * strtol() doesn't complain about leftovers, so I use my own
 *
 * This is really only intended for input of unsigned longs; it returns -1
 * if the string isn't entirely composed of hex digits.
 */

static long hex_strtol (char *s)
{
  long val = 0;
  char *p;

  if (! *s)
    return (-1);  /* must have at least one digit! */

  while (*s)
    {
      p = strchr (hex_digits, tolower (*s));
      if (! p)
	return (-1);
      val = (val << 4) + (p - & hex_digits [0]);
      s++;
    }
  return (val);
}


static int parse_address (char *s)
{
  long val;

  if (strcasecmp (s, "pc") == 0)
    return (save_PC);

  if (strcasecmp (s, "sp") == 0)
    return (SP + 0x0100);

  val = hex_strtol (s);
  if ((val >= 0) && (val <= 0xffff))
    return (val);

  return (-1);
}


static int parse_byte (char *s)
{
  long val;

  val = hex_strtol (s);
  if ((val >= 0) && (val <= 0xff))
    return (val);

  return (-1);
}


static void dump_range (word addr1, word addr2)
{
  word addr;
  byte data;

  breakflag = 0;

  for (; addr1 <= addr2; addr1 += 16)
    {
      printf ("%04x: ", addr1);
      for (addr = addr1; addr < (addr1 + 16); addr++)
	{
	  if (addr <= addr2)
	    {
	      data = memrd (addr, 0, 0);
	      if (breakflag)
		{
		  addr2 = addr - 1;
		  printf ("-- ");
		}
	      else
		printf ("%02x ", data);
	    }
	  else
	    printf ("   ");
	}
      for (addr = addr1; addr < (addr1 + 16); addr++)
	{
	  if (addr <= addr2)
	    {
	      data = memrd (addr, 0, 0);
	      if (breakflag)
		{
		  addr2 = addr - 1;
		  printf (" ");
		}
	      else if ((data >= 0x20) && (data <= 0x7e))
		printf ("%c", data);
	      else
		printf (".");
	    }
	  else
	    printf (" ");
	}
      printf ("\r\n");
    }

    fflush(stdout);
  breakflag = 0;
}


static void show_mem (word addr)
{
  byte val;
  val = memrd(addr, 0, 0);
  printf("mem[%04x]: %02x, tagr: %02x, tagw: %02x\r\n",addr,val,mem[addr].tagr, mem[addr].tagw);
    fflush(stdout);
}

static void formatdump(byte a, byte x, byte y, byte p, byte s)
{
  char flagstr[9];
  
  flagstr[0] = (p & N_BIT) ? 'N' : 'n';
  flagstr[1] = (p & V_BIT) ? 'V' : 'v';
  flagstr[2] = (p & B_BIT) ? 'B' : 'b';
  flagstr[3] = (p & D_BIT) ? 'D' : 'd';
  flagstr[4] = (p & I_BIT) ? 'I' : 'i';
  flagstr[5] = (p & Z_BIT) ? 'Z' : 'z';
  flagstr[6] = (p & C_BIT) ? 'C' : 'c';
  flagstr[7] = 0;
  printf("A:%02x X:%02x Y:%02x %s SP:1%02x", a, x, y, flagstr, s);
}


static void dumpregs (void)
{
    disasm_6502 (save_PC);
    formatdump(save_A, save_X, save_Y, save_flags, SP);
#ifdef WRAP_CYC_COUNT
    printf ("  wrap: %lu", cyc_wraps);
#endif
    printf ("  cyc: %lu", save_totcycles);
    printf ("  irq_cyc: %lu", irq_cycle);
    printf ("\r\n");
    fflush(stdout);
}


static void list_breakpoints (void)
{
  int i;
	
  for (i=0; i < 65536; i++) 
    {
      if(((mem [i].tagr & BREAKTAG) == BREAKTAG) ||
	 ((mem [i].tagw & BREAKTAG) == BREAKTAG))
	{
	  printf ("%04x", i);
	  if (mem [i].tagr & BREAKTAG)
	    printf (" RD");
	  if (mem [i].tagw & BREAKTAG)
	    printf (" WR");
	  printf ("\r\n");
	}
    }
fflush(stdout);
  
}

static int break_cmd (int argc, char *argv[])
{
  int addr;

  int on_read = 1;
  int on_write = 1;

  if ((argc < 1) || (argc > 3))
    return (-1);

  if (argc == 1)
    {
      list_breakpoints ();
      return (0);
    }

  addr = parse_address (argv [1]);
  if (addr < 0)
    return (-1);

  if (argc == 3)
    {
      if (strcasecmp (argv [2], "r") == 0)
	on_write = 0;
      else if (strcasecmp (argv [2], "w") == 0)
	on_read = 0;
      else
	return (-1);
    }

  if (on_read)
    mem [addr].tagr |= BREAKTAG;
  else
    mem [addr].tagr &= ~ BREAKTAG;

  if (on_write)
    mem [addr].tagw |= BREAKTAG;
  else
    mem [addr].tagw &= ~ BREAKTAG;

  return (0);
}

static int clear_cmd (int argc, char *argv[])
{
  int addr;

  if ((argc < 1) || (argc > 2))
    return (-1);

  if (argc == 1)
    {
      for (addr = 0; addr < 65536; addr++)
	{
	  mem [addr].tagr &= ~ BREAKTAG;
	  mem [addr].tagw &= ~ BREAKTAG;
	}
      return (0);
    }

  addr = parse_address (argv [1]);

  if (addr < 0)
    return (-1);

  mem [addr].tagr &= ~ BREAKTAG;
  mem [addr].tagw &= ~ BREAKTAG;

  return (0);
}


static int go_cmd (int argc, char *argv[])
{
  if (argc != 1)
    return (-1);

  stepflag = 0;
  traceflag = 0;
  runflag = 1;

  return (0);
}


static int step_cmd (int argc, char *argv[])
{
  if (argc != 1)
    return (-1);

  stepflag = 1;
  traceflag = 0;
  runflag = 1;

  return (0);
}


static int trace_cmd (int argc, char *argv[])
{
  if (argc != 1)
    return (-1);

  stepflag = 1;
  traceflag = 1;
  runflag = 1;

  return (0);
}


static int dump_cmd (int argc, char *argv[])
{
  int first, last;

  if ((argc < 1) || (argc > 3))
    return (-1);

  first = next_addr;
  if (argc >= 2)
    first = parse_address (argv [1]);
  last = first + 255;
  if (argc == 3)
    last = parse_address (argv [2]);

  if ((first < 0) || (last < first))
    return (-1);

  dump_range (first, last);

  next_addr = last + 1;

  return (0);
}


static int dis_cmd (int argc, char *argv[])
{
  int first = next_addr;
  int last = 0;
  int count = 20;

  if ((argc < 1) || (argc > 3))
    return (-1);

  if (argc >= 2)
      first = parse_address (argv [1]);
      
  if (argc == 3)
    {
      last = parse_address (argv [2]);
      count = 0;
    }

  if (first < 0)
    return (-1);

  if (last && (last < first))
    return (-1);

  while (count || (first <= last))
    {
      first += disasm_6502 (first);
      printf ("\r\n");
      fflush(stdout);
      if (count) 	count--;
    }

  next_addr = first;

  return (0);
}


static int read_cmd (int argc, char *argv[])
{
  int addr;

  if (argc != 2)
    return (-1);

  addr = parse_address (argv [1]);

  if (addr < 0)
    return (-1);

  show_mem (addr);

  return (0);
}


static int write_cmd (int argc, char *argv[])
{
  int addr, val;

  if (argc != 3)
    return (-1);

  addr = parse_address (argv [1]);

  if (addr < 0)
    return (-1);

  val = parse_byte (argv [2]);

  if (val < 0)
    return (-1);

  memwr (addr, val, 0, 0);
  show_mem (addr);

  return (0);
}


static int quit_cmd (int argc, char *argv[])
{
  if (argc != 1)
    return (-1);
// return to 2normal" command line
//  exit (0);
  return 0;
}


static int save_cmd (int argc, char *argv[])
{
  if (argc != 2)
    return (-1);
  return (dumpworld (argv [1]));
}


static int load_cmd (int argc, char *argv[])
{
  if (argc != 2)
    return (-1);
  return (reload (argv [1]));
}


static int reg_cmd (int argc, char *argv[])
{
  int val;

  if (argc == 1)
    {
      dumpregs ();
      return (0);
    }

  if (argc != 3)
    return (-1);

  if (sscanf (argv [2], "%x", & val) != 1)
    return (-1);

  if (strcasecmp (argv [1], "a") == 0)
    save_A = val;
  else if (strcasecmp (argv [1], "x") == 0)
    save_X = val;
  else if (strcasecmp (argv [1], "y") == 0)
    save_Y = val;
  else if (strcasecmp (argv [1], "p") == 0)
    save_flags = val;
  else if (strcasecmp (argv [1], "s") == 0)
    SP = val;
  else
    {
      printf ("invalid register\r\n");
    fflush(stdout);
      return (-1);
    }

  return (0);
}


static int option_cmd (int argc, char *argv[])
{
  int reg, val;

  if (argc == 1)
    {
      for (reg = 0; reg < MAX_OPT_REG; reg++)
	printf ("option register %d = %02x\r\n", reg, optionreg [reg]);
    fflush(stdout);
      return (0);
    }
  else if (argc == 3)
    {
      reg = parse_byte (argv [1]);
      val = parse_byte (argv [2]);
      if ((reg < 0) || (reg > (MAX_OPT_REG - 1)))
	{
	  printf( "invalid register\r\n");
    fflush(stdout);
	  return (1);
	}
      optionreg [reg] = val;
      return (0);
    }
  return (-1);
}


static int xyzzy_cmd (int argc, char *argv [])
{
  int c1, c2, c3, c4;
#if 0
  int i, v;
#endif

  if (argc != 1)
    return (-1);

  if (game == GRAVITAR) {
      c1 = mem [0x0d].cell | (mem [0x0e].cell << 8);
      c2 = mem [0x0c].cell;
      c3 = mem [0x0f].cell | (mem [0x10].cell << 8);
      c4 = mem [0x0b].cell;
      printf ("Ship loc (%4x.%2x, %4x.%2x)\r\n", c1, c2, c3, c4);

      c1 = mem [0x13].cell | (mem [0x12].cell << 8);
      c2 = mem [0x14].cell;
      c3 = mem [0x16].cell | (mem [0x15].cell << 8);
      c4 = mem [0x17].cell;
      printf ("Velocity (%4x.%2x, %4x.%2x)\r\n", c1, c2, c3, c4);

#if 0
      printf ("Stars:\r\n");
      for (i = 15; i >= 0; i--)
	{
	  c1 = mem [0x3ce+i].cell | (mem [0x3de+i].cell << 8);
	  c2 = mem [0x3ee+i].cell | (mem [0x3fe+i].cell << 8);
	  v = mem [0x40e+i].cell;
	  printf ("%2d  %4x  %4x  %2x\r\n", i, c1, c2, v);
	}
#endif
  } else printf ("Nothing happens here.\r\n");

    fflush(stdout);
  return (0);
}


static int help_cmd (int argc, char *argv[]);


typedef int (*cmd_handler)(int argc, char *argv[]);

typedef struct
{
  char *name;
  cmd_handler handler;
  int min_chr;
  char *usage;
} cmd_entry;


cmd_entry cmd_table [] =
{
  { "break", break_cmd,     1, "Break [<addr> [r|w]]            list or set breakpoints\r\n" },
  { "clear", clear_cmd,     1, "Clear [<addr>]                  clear breakpoints\r\n" },
  { "disassemble", dis_cmd, 2, "DIsassemble [<addr> [<addr>]]   disassemble from addr\r\n" },
  { "dump", dump_cmd,       1, "Dump [<addr> [<addr>]]          dump memory\r\n" },
  { "go", go_cmd,           1, "Go                              go\r\n" },
  { "help", help_cmd,       1, "Help                            list commands\r\n" },
  { "load", load_cmd,       1, "Load <filename>                 load the world\r\n" },
  { "option", option_cmd,   1, "Option [<switch> [<value>]]     examine or set option switch settings\n" },
  { "quit", quit_cmd,       4, "QUIT                            quit simulator\r\n" },
  { "read", read_cmd,       3, "REAd <addr>                     read memory at addr\r\n" },
  { "register", reg_cmd,    1, "Register [<reg> <val>]          display or set registers\r\n" },
  { "save", save_cmd,       2, "SAve <filename>                 save the world\r\n" },
  { "step", step_cmd,       1, "Step                            single-step one instruction\r\n" },
  { "trace", trace_cmd,     1, "Trace                           go in trace mode\r\n" },
  { "write", write_cmd,     1, "Write <addr> <val>              write val to memory at addr\r\n" },
  { "xyzzy", xyzzy_cmd,     1, "Xyzzy\r\n" },
  { NULL, NULL, 0, NULL }
};
static int find_cmd (char *s)
{
  int i;
  int len = strlen (s);
  for (i = 0; cmd_table [i].name; i++)
  {
    if ((len >= cmd_table [i].min_chr) && (strncasecmp (s, cmd_table [i].name, len) == 0))
      return (i);
  }
  return (-1);
}


static int help_cmd (int argc, char *argv[])
{
  int i;
  if (argc == 1)
  {
    for (i = 0; cmd_table [i].name; i++)
      printf( cmd_table [i].usage);
    printf( "\r\nCommands may be abbreviated to the portion listed in caps.\r\n");
    fflush(stdout);
    return (0);
  }

  if (argc != 2)
    return (-1);

  i = find_cmd (argv [1]);
  if (i < 0)
    {
      printf( "unrecognized command (1)\r\n");
    fflush(stdout);
      return (1);
    }

  printf( cmd_table [i].usage);
    fflush(stdout);
  return (0);
}



static void execute_command (int argc, char *argv [])
{
  int i;

  i = find_cmd (argv [0]);

  if (i < 0)
  {
    printf( "unrecognized command\r\n");
    fflush(stdout);
    return;
  }

  if ((*cmd_table [i].handler)(argc, argv) < 0)
    printf( "Usage: %s (%08x)", cmd_table [i].usage, (*cmd_table [i].handler)(argc, argv));
  fflush(stdout);
}

#define MAX_LINE 200
#ifdef FREESTANDING
static char commandBuffer[MAX_LINE];
static char *commandBufferPointer;
static int commandBufferCounter;

char *readline(char *prompt)
{
  commandBufferPointer = commandBuffer;
  commandBufferCounter = 0;
  char inbuf [MAX_LINE];
  if (prompt)
  {
    while (*prompt != 0) RPI_AuxMiniUartWrite(*prompt++);
    fflush(stdout);
  }
  while (1)
  {
    while (RPI_AuxMiniUartReadPending())
    {
      char r = RPI_AuxMiniUartRead();

        RPI_AuxMiniUartWrite(r);
        if (r != '\n')
        {
                if (commandBufferCounter<MAX_LINE-2)
                {
                        *commandBufferPointer++ = r;
                        *commandBufferPointer = (char) 0;
                        commandBufferCounter++;
                }
        }
        else
        {
          return (commandBuffer);
        }
    }
  }
  return (commandBuffer);
}
#else

/*
 * print prompt, get a line of input, return a copy
 * caller must free
 */
char *readline (char *prompt)
{
  char inbuf [MAX_LINE];
  if (prompt)
    printf (prompt);
  fgets (inbuf, MAX_LINE, stdin);
  return (strdup (& inbuf [0]));
}
#endif
int debugger (int type)
{
  char *cmd;
  char *s;
  int argc;
  char *argv [MAX_ARGS];

  runflag = 0;

  if (type != STEP)
    traceflag = 0;

  breakflag = 0;

#ifdef COUNT_INTERRUPTS
  printf ("Interrupts: %d\r\n", int_count);
    fflush(stdout);
#endif

  switch(type) 
    {
    case INTBREAK: printf("User break, PC=%04x\r\n",save_PC);  break;
    case ERRORBRK: printf("Error, PC=%04x\r\n",save_PC);       break;
    case BREAKPT:  printf("breakpoint, PC=%04x\r\n",save_PC);  break;
    case BREAK:    printf("break instruction, PC=%04x\r\n",save_PC); break;
    case INSTTAG:  printf("instruction fetch from non-memory, PC=%04x\r\n", save_PC); break;
    case STEP:     break;
    default:       printf("badbrkpt\r\n"); break;
    }

    fflush(stdout);
  dumpregs();
  printf("\r\n");
    fflush(stdout);

  while((! traceflag) && (! runflag))
    {
      cmd = readline ("> ");
      if (cmd)
	{
#ifdef USE_HISTORY
	  if (*cmd)
	    add_history (cmd);
#endif
	  argc = 0;
	  for (s = cmd; (argc < MAX_ARGS) && ((s = strtok (s, " \t\r\n")) != NULL); s = NULL)
      {
        argv [argc++] = s;
      }
    fflush(stdout);

	  if (argc)
	    execute_command (argc, argv);

	}
    }
  breakflag = 0;
  return 0; // is this what is expected by caller?
}
