/* #define DUALCPU to create output suitable for dual CPU debugging */
#ifdef IS_LONG64
#define U_LONG unsigned int
#define S_LONG signed int
#else
#define U_LONG unsigned long
#define S_LONG signed long
#endif


/************************************************************************
 *                                                                      *
 * Cinematronics Binary Code Translator                                 *
 *                                                                      *
 * This program is used to generate a disassembled listing of a program	*
 * written for the Cinematronics CPU cards in a way such that if        *
 * recompiled with a C compiler it will generate a working program.	*
 *                                                                      *
 * Author:	Zonn Moore                                              *
 * Hacked by:	Graham Toal                                             *
 * Version:	0.1.9                                                   *
 * Date:	21Jan00                                                 *
 *                                                                      *
 * Revision History:                                                    *
 * -----------------                                                    *
 *                                                                      *
 * Version 0.1, 01/03/00                                                *
 *    Initial GT release.                                               *
 *                                                                      *
 * Conversion from disassember to translator, 01/02/00			*
 *    Using routines from MAME CCPU emulator                            *
 *                                                                      *
 * Version 1.0, 01/31/97                                                *
 *    Initial release.                                                  *
 *                                                                      *
 * Version 1.1, 07/31/97                                                *
 *    Removed '[j]' from JMP instructructions.                          *
 *                                                                      *
 * Released to public domain by the author, distribute freely.		*
 ************************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include    <assert.h>

#ifndef __MSDOS__
#include	<unistd.h>
#include	<fcntl.h>
#include	<sys/types.h>
#endif

#ifndef TRUE
#define TRUE (0==0)
#define FALSE (0!=0)
#endif


#define PASS_1  1
#define PASS_2  2
#define PASS_3  3

/* We do several passes over the program, to mark the rom addresses
   with special info, such as 'this is a jump target, so forget
   all cached register values', or 'this instruction requires
   the previous instruction to remember intermediate accumulator
   results' which would otherwise be removed by optimisation.
   Unfortunately I've realised there are cases where each pass
   affects code generation and that it may not even ever converge :-(
 */

int pass = PASS_1;

/*

   The TAG variables are used to set bits in an array whose size
   is the same number of words as the roms are bytes.  They are
   set on PASS1 which does not generate any code, and are used
   to modify code generation in PASS2

 */

/* This opcode requires that cmp_old is valid */
#define TAG_REQUIRES_CMP_OLD 1

/* This opcode requires that cmp_new is valid */
#define TAG_REQUIRES_CMP_NEW  2

/* This opcode must make a note of the next state at runtime */
#define TAG_NOTESTATE 4

/* This opcode is the target of a jump from state_X 
   If it is an ACC instruction and gets jumpers from different
   states, we must generate different code depending on the state.
   All jump destinations should set all registers to DEADBEEF for
   now which will turn off unsafe optimisations */

#define TAG_JUMPTARGET_A 8
#define TAG_JUMPTARGET_AA 16
#define TAG_JUMPTARGET_B 32
#define TAG_JUMPTARGET_BB 64

#define TAG_OPCODE 128

#define TAG_JUMPINST 256

#define TAG_JUMPTARGET (TAG_JUMPTARGET_A|TAG_JUMPTARGET_AA|TAG_JUMPTARGET_B|TAG_JUMPTARGET_BB)

#include "ccpu.h"

#define CARRYBIT (1<<12)
#define A0BIT 1

CINESTATE state;

/*

  The real PC is only 12 bits, but it has an immutable top 4 bits
  representing the current rom page.  In this code I am making
  the PC 16 bits rather than 12 and storing the current rom number
  in there with it.  Note that the top bits are NOT the same as
  the P register below, because P is only transferred into the PC
  when the longjump instructions are executed.

  Any instructions that save the PC must mask it with 0xFFF as they
  do so.  I'm not sure which instruction that is but I believe
  there is one, for building subroutines out of.

  Loads of the PC that are *not* longjumps should preserve the top
  4 bits of the extended PC register, so as not to accidentally
  drop out of the current rom page.

 */

U_LONG register_PC; /* address of instruction currently examined */

/*

  When we set P we set it to what the CCPU really thinks it is.  However
  I'm considering also setting P_rompage to P<<12 and P_rampage to P<<4
  for use in instructions which use P, to save shifting them on read;
  i.e. we generally read these registers more often than we set them.

 */

U_LONG register_P; /* cached value of Page register from previous LDP # */

U_LONG register_J; /* cached value of jump register from previous LDJ # */
U_LONG register_I; /* cached value of Index register from previous LDI # */
U_LONG register_A; /* cached value of A register from previous LDAA # */
U_LONG register_B; /* cached value of A register from previous LDAB # */
U_LONG this_page;  /* top 4 bits of PC.  May not keep this variable */

/* known variables that are always constant for tailgunner */
U_LONG ccpu_monitor = CCPU_MONITOR_BILEV; /* arcade monitor type */
U_LONG ccpu_msize = CCPU_MEMSIZE_8K;      /* ram for tailgunner board */
U_LONG ccpu_jmi_dip = 0; /* EI or MI - Tailgunner is EI */


/* Remember if you change these, to change tailgunr.c to match! */
/* Really should be a run-time parameter */

#ifdef __MSDOS__
int comments = FALSE; /* Currently don't generate quite so many comments */
int tracing = FALSE; /* run-time opcode/register trace */
int before = FALSE; /* run-time opcode/register trace format tweak */
int debug = FALSE;   /* run-time machine debug */
int jumptableflag = 0; // or 1
#ifdef SUPEROPTIMISE
int disp_opcodes = FALSE; /* Currently don't generate quite so many comments */
int slave_pc = FALSE; /* Helps slightly when diagnosing callback routines */
#else
int disp_opcodes = TRUE; /* Currently don't generate quite so many comments */
int slave_pc = FALSE; /* Helps slightly when diagnosing callback routines */
#endif
#else

// I've tweaked some of these flags in order to generate a
// version that Neil can use to test code generation with.

// Turn some of these back to TRUE when debugging

int comments = FALSE; /* Currently don't generate quite so many comments */
int tracing = FALSE; /* run-time opcode/register trace */
int before = FALSE; /* run-time opcode/register trace format tweak */
int debug = FALSE;   /* run-time machine debug */
int disp_opcodes = TRUE; // COMPAT: FALSE; /* Currently don't generate quite so many comments */
int jumptableflag = FALSE; // COMPAT: TRUE; 
int slave_pc = FALSE; /* Helps slightly when diagnosing callback routines */
#endif

#ifdef SUPEROPTIMISE
int loggingCycles = FALSE; 
#else
/* We currently need this one in order to do benchmark timing calculations */
int loggingCycles = FALSE; /* Don't keep a runtime count of cycles burned */
#endif
#include "opcodes.h"

#ifndef __MSDOS__
/* Some DOS emulation procedures since this code didn't come from Unix */
void strlwr(char *s)
{
  int c;
  if (s == NULL) return;
  for (;;) {
    c = *s++;
    if (c == '\0') break;
    if (isalpha(c) && isupper(c)) {
      c = tolower(c);
    }
  }
}

off_t filelength(int fd)
{
  off_t curpos = lseek(fd, 0, SEEK_CUR);
  off_t l = lseek(fd, 0, SEEK_END);
  lseek(fd, curpos, SEEK_SET);
  return(l);
}
#endif

U_LONG context_register_A[0x2000];   /* A-Register (accumulator) */
U_LONG context_register_B[0x2000];   /* B-Register (accumulator) */
U_LONG context_register_I[0x2000];   /* I-Register (last access RAM location) */
U_LONG context_register_J[0x2000];   /* J-Register (target address for JMP opcodes) */
U_LONG context_register_P[0x2000];   /* Page-Register (4 bits, shifts to high short nibble for code, hight byte nibble for ram) */


/* This procedure MUST be kept in synch with the one in optimise.c */
void load_invariants()
{
    FILE *context_file = fopen("tailgunr.opt", "r");
    int i, rc;
    /* Initialise everything to 'unknown' */
    for (i = 0; i < 0x2000; i++) {
      context_register_A[i] = 0xFEEDBEEFUL;
      context_register_B[i] = 0xFEEDBEEFUL;
      context_register_I[i] = 0xFEEDBEEFUL;
      context_register_J[i] = 0xFEEDBEEFUL;
      context_register_P[i] = 0xFEEDBEEFUL;
    }
    if (context_file != NULL) {
      int tot = 0;
      /* All executions are merged over the long term */
      for (;;) {
        /* Read in results of previous executions */
        rc = fscanf(context_file, "L%x ", &i);
        if (rc != 1) break;
        tot += 1;
        fscanf(context_file, "A %lx ", &context_register_A[i]);
        fscanf(context_file, "B %lx ", &context_register_B[i]);
        fscanf(context_file, "I %lx ", &context_register_I[i]);
        fscanf(context_file, "J %lx ", &context_register_J[i]);
        fscanf(context_file, "P %lx\n", &context_register_P[i]);
      }
      fclose(context_file);
      fprintf(stderr, "OPTIMISE: pulled in %d previous records\n", tot);
    }
 }

/* Formatting help */

char cur_tabs[256];

void push_indent(char *s)
{
  strcat(cur_tabs, s);
}

void pop_indent(char *s)
{
  cur_tabs[strlen(cur_tabs)-strlen(s)] = '\0';
}

/* Define some common data types */

typedef unsigned char	uchar;

/* This stuff is exbedded from main() */
int		inFile, err, ii;
static char	fileName[131];
int		nameLen, romSize, startAdr, endAdr, romidx, objidx;
int		oldStart, oldEnd;
long		tempSize;
unsigned int	objSize, brk_f;
char	       *gamename;

/* Testing: was 12 */
#define	TAB_OPCODE	14		/* Column to place opcode */
#define	TAB_PARAM	(TAB_OPCODE+5)	/* Column to place parameters */

/* Default filname extensions */

#define	EXT1		".t7"
#define	EXT2		".p7"
#define	EXT3		".u7"
#define	EXT4		".r7"

#include "disasm.h"

static uchar rom[0x2000] =
#include "tailgunner-data.c"

FILE    *codefile = NULL; /* Dump code on final pass to codefile */
U_LONG	*romFlags;/* Special hints for optimiser */
char	 OpcBfr[81];  /* Temp buffer used to build opcodes */
char	 DspBfr[81];  /* Line display buffer */
uchar	 Extd_f;      /* Extended diss flag, if set display extended insts. */

#define internal_test(o) do_internal_test(o, __LINE__, __FILE__)
void do_internal_test(int opcode, const int line, const char *file)
{
  /* This is called at the top of every opcode procedure.  Good opportunity
     to do things that always need done... */
  if (pass == PASS_1) {
    romFlags[register_PC] |= TAG_OPCODE;
  }
  if (opcode != rom[register_PC]) {
    fprintf(stderr, "Consistency error at line %0d in %s\n", line, file);
    exit(1);
  }
  if (loggingCycles) {
    fprintf(codefile, "%sccpu_ICount += %0d;\n", cur_tabs, ccpu_cycles[opcode]);
  }
  /* If turning off register optimisations, set the regs to DEADBEEF here ... */
}

int require_note_state(int register_PC)
{
  return(romFlags[register_PC] & TAG_NOTESTATE);
}


/* Short macros */

/* May soon take these and put the expansions inline */

void explore(int startAdr, int endAdr);

char *Jump(void) {
#ifdef DUALCPU
#define DOIT "return"
#else
#ifdef SUPEROPTIMISE
#define DOIT "goto *lab[register_PC]"
#else
#define DOIT (jumptableflag == 1 ? "goto *lab[register_PC]" : "break")
#endif
#endif
  int savedpc;
  static char buffer[256];
  if (register_J != 0xDEADBEEFUL) {
    savedpc = (register_J | (register_PC & 0xF000));
    explore(savedpc, endAdr);
    if (loggingCycles) {
#ifdef SUPEROPTIMISE
      sprintf(buffer, "{ccpu_ICount -= 2; goto L%04x;}", savedpc);
#else
      sprintf(buffer, "{register_PC = 0x%04x; ccpu_ICount -= 2; %s;}", savedpc, DOIT);
#endif
      /* correct instruction execution count for jump taken */
    } else {
#ifdef SUPEROPTIMISE
      sprintf(buffer, "goto L%04x", savedpc);
#else
      sprintf(buffer, "{register_PC = 0x%04x; %s;}", savedpc, DOIT);
#endif
    }
    if (savedpc == 0xDEADBEEF) {
      fprintf(stderr, "*** BIG PROBLEM!  PC=deadbeef\n");
    } else {
      romFlags[register_PC = savedpc] |= TAG_JUMPTARGET; /* For now.  Needs be more selectiove */
    }
  } else {
    int offset;
    if (register_PC != 0xDEADBEEFUL) {
      if ((register_PC & (~0xfff)) == 0) {
        offset = sprintf(buffer, "{register_PC = register_J;");
      } else {
        offset = sprintf(buffer, "{register_PC = 0x%04lx | register_J;", register_PC & 0xf000);
      }
      if (loggingCycles) offset = sprintf(buffer+offset, " ccpu_ICount -= 2;");
      sprintf(buffer+offset, " %s;}", DOIT);
    } else {
      offset = sprintf(buffer, "{register_PC = (register_PC & 0xf000) | register_J;");
      if (loggingCycles) offset = sprintf(buffer+offset, " ccpu_ICount -= 2;");
      sprintf(buffer+offset, " %s;}", DOIT);
    }
    /* correct instruction execution count for jump taken */
    fprintf(stderr, "Unknown jump at 0x%04lx\n", register_PC); /* register_PC can be deadbeef!  Shouldn't be, at translate time... */
  }
  return(buffer);
}

char *SAR(char *var, char *arg) {
  static char buffer[128];
  sprintf(buffer, "((signed short int)(((signed short int)%s) >> (signed short int)%s)) /* SAR */", var, arg);
  /* ANSI C paranoia here */
  return(buffer);
}

#include "disopc.h"

#include "macros.h"

int started = 0;

void explore(int startAdr, int endAdr)
{
U_LONG saved_PC = register_PC,
              saved_J = register_J,
              saved_I = register_I,
              saved_A = register_A,
              saved_B = register_B;

/* What about register_P ???? */

if (startAdr == 0x1000) fprintf(stderr, "EXPLORING 1000\n");

  if ((pass == PASS_2) && (started == 1)) return;

  if ((pass == PASS_2) && (startAdr == 0) && (endAdr == 0x1fff) && (started == 0)) {
    started = 1;
  }
  while (startAdr <= endAdr)
  {
      int caselabel = startAdr;
if (startAdr == 0x1000) fprintf(stderr, "DECODING 1000\n");

      /* Kill recursion on pass1 - we've already encoded from here */
      if ((pass == PASS_1) && ((romFlags[startAdr] & TAG_OPCODE) != 0)) {
        break;
      }
      /* Skip code generation for any instructions which can't be reached */
      if ((pass == PASS_2) && ((romFlags[startAdr] & TAG_OPCODE) == 0)) {
        for (;;) {
          if (startAdr == 0x1000) break;

          if (romFlags[startAdr] & TAG_OPCODE) break;

          startAdr += 1;
	}
        caselabel = startAdr;
      }
      if (startAdr > endAdr) break;
      if (startAdr > 0x1fc7 /* Tailgunner hack */) break;
      /* Do I need to DEADBEEF everything here??? */

      memset(DspBfr, ' ', 80);				/* set to blanks */
      DspBfr[80] = '\0';

      /* dissOpcode sets register_PC for no good reason.  Would be 
         better done here, no? */
      dissOpcode(DspBfr, startAdr, &objSize, &brk_f);	/* setup buffer */

      for (ii = 79; DspBfr[ii] == ' '; ii--)
        ;

      DspBfr[ii+1] = '\0';											/* terminate line */

      if (((startAdr & 0xFFF) == 0x000) && (startAdr != 0)) {
        /* start of a new rom bank */
        fprintf(codefile, "/*********************************************************/\n");
      }

      if ((romFlags[startAdr] & TAG_JUMPTARGET) != 0) {
        register_P = 0xDEADBEEFUL;  /* We don't yet have code to merge contexts at jump destinations */
        register_I = 0xDEADBEEFUL;
        register_A = 0xDEADBEEFUL;
        register_B = 0xDEADBEEFUL;
        /*register_J = 0xDEADBEEFUL;*/ /* Actually this one *IS* known, but paranoia for now... */
#ifdef DUALCPU
      }
#endif

	  fprintf(codefile, "\n");
#ifdef DUALCPU
        fprintf(codefile, "      return;\n\n");
#endif
        if (jumptableflag) {
	    fprintf(codefile, "L%04x:\n", caselabel);
	  } else {
          fprintf(codefile, "case 0x%04x:\n", caselabel);
	  }
        if (comments) fprintf(codefile, " /* romflags %04lx */", romFlags[startAdr]);
        fprintf(codefile, "\n");
#ifdef DETECT_INVARIANTS
        fprintf(codefile, "      note_invariants(0x%04x);\n", caselabel);
#endif
#ifdef USE_INVARIANTS
        if (context_register_P[caselabel] != 0xFEEDBEEFUL) {
          fprintf(codefile, "      /* Invariants: register_P = 0x%01lx", context_register_P[caselabel]);
          if (context_register_I[caselabel] != 0xDEADBEEF) fprintf(codefile, " register_I = 0x%02lx", context_register_I[caselabel]);
          if (context_register_A[caselabel] != 0xDEADBEEF) fprintf(codefile, " register_A = 0x%02lx", context_register_A[caselabel]);
          fprintf(codefile, " */;\n");
          register_P = context_register_P[caselabel]; /* Tell optimiser! */
          register_I = context_register_I[caselabel];
          register_A = context_register_A[caselabel];
        }
#endif
#ifndef DUALCPU
      }
#endif
      if (comments) {
        fprintf(codefile, "%s/* %s */\n", cur_tabs, DspBfr);
      }

      if (((startAdr & 0xFFF) == 0x000) && (startAdr != 0)) {
        /* start of a new rom bank */
        fprintf(codefile, "%sstate = state_A; /* Even if it's not! :-) */\n", cur_tabs);
      }

      if (tracing) {
        if (before) {
          fprintf(codefile, "#ifdef BEFORE\n");
          fprintf(codefile, "%straceregs(register_A, register_B, register_P, register_I, register_J, flag_C, acc_a0, cmp_new,  cmp_old, ccpu_ICount);\n", cur_tabs);
          fprintf(codefile, "#endif\n");
	}
        fprintf(codefile, "#ifdef TRACING\n");
        fprintf(codefile, "%straceinst(\"%s\");\n", cur_tabs, DspBfr);
        fprintf(codefile, "#endif\n");
      }


      /* This may not be the right test here but it'll do for now */
      if (require_note_state(register_PC)) {

        /* When we're not sure of the state at compile time, 
           generate code to do the right thing at run time ... */

        fprintf(codefile, "%sif (state == state_A) {\n", cur_tabs);
          push_indent("  ");
          cineops[state_A][opCode](opCode);
          pop_indent("  ");
        fprintf(codefile, "%s} else if (state == state_AA) {\n", cur_tabs);
         push_indent("  ");
          cineops[state_AA][opCode](opCode);
          pop_indent("  ");
        fprintf(codefile, "%s} else if (state == state_B) {\n", cur_tabs);
          push_indent("  ");
          cineops[state_B][opCode](opCode);
          pop_indent("  ");
        fprintf(codefile, "%s} else if (state == state_BB) {\n", cur_tabs);
          push_indent("  ");
          cineops[state_BB][opCode](opCode);
          pop_indent("  ");
        fprintf(codefile, "%s} else {\n", cur_tabs);
        fprintf(codefile, "%s  /* error */\n", cur_tabs);
        fprintf(codefile, "%s}\n", cur_tabs);

      } else {

        /* we need these damned brackets because of declarations
           inside the ops.  Would prefer to fix it there. */

/*        fprintf(codefile, "%s{\n", cur_tabs);*/
        if (opCode == /*opMULirg_A_AA*/ 0xe3) {
	  if (rom[register_PC+1] == 0xe3) {
	    int i, muls = 1;
	    while (rom[register_PC+muls] == 0xe3) {
	      muls += 1;
	    }
            fprintf(codefile, "%sMULS_START(%d);  // register_PC = %04x\n", cur_tabs, muls, register_PC);
	    muls -= 1;
	    for (i = 0; i <= muls; i++) cineops[state][opCode](opCode); // mulhack
            register_PC += muls; startAdr += muls; // skip all but 1 which is done elsewhere
	    fprintf(codefile, "%sMULS_END();  // register_PC = %04x\n", cur_tabs, register_PC);
	  } else {
            cineops[state][opCode](opCode); // single MUL
	  }
	} else {
          cineops[state][opCode](opCode);
	}
/*        fprintf(codefile, "%s}\n", cur_tabs);*/

      }

      /* return on all jumps, because the rest of the code would have
         been tagged by the recursive call to 'explore' in the jump
         procedures */
      if ((pass == PASS_1) && ((romFlags[startAdr] & TAG_JUMPINST) != 0)) break; /* end recursion */

      if (tracing) {

        /* trace should show regs in entry, instr, regs on exit -
           except when the opcode itself prints something to stderr */

        fprintf(codefile, "#ifdef TRACING\n");
        fprintf(codefile, "%straceregs(register_A, register_B, register_P, register_I, register_J, flag_C, acc_a0, cmp_new, cmp_old, ccpu_ICount);\n", cur_tabs);
        fprintf(codefile, "%stracenl();\n", cur_tabs);
        fprintf(codefile, "#endif\n");
      }

      if ((pass == PASS_1) && (((startAdr + objSize) & 0xFFF) < (startAdr & 0xFFF)) ) {
        /* don't follow code over a page boundary on exploratory pass */
        break;
      }

      startAdr += objSize;

/*
      if (brk_f)
        putchar('\n');  /-* print break *-/
 */
  }
  register_PC = saved_PC;
  register_J = saved_J;
  register_I = saved_I;
  register_A = saved_A;
  register_B= saved_B;
}

int main(int argc, char **argv)
{

  printf ("char size = %i\n", sizeof(char));
  printf ("short size = %i\n", sizeof(short));
  printf ("int size = %i\n", sizeof(int));
  printf ("long size = %i\n", sizeof(long int));
  printf ("long long size = %i\n", sizeof(long long int));
  
  
  /* Should all command line options for debug, tracing etc */

  gamename = "tailgunr";
  startAdr = 0x0000; endAdr = 0x1FFF;
  Extd_f = 0;					/* reset flags */
  strcpy(cur_tabs, "      ");

  load_invariants();

  romFlags = (U_LONG *)calloc(0x2000, 4);	/* Flags */

  if (romFlags == 0) {
    fputs("Not enough memory for rom flags!\n", stderr);
    exit(0);
  }

  /* get file name and read in ROMs */

  nameLen = strlen(gamename);		/* get length of filename */

  memcpy(fileName, gamename, nameLen);	/* get games name */

  /* open EXT1 (lower, even, ROM) */

  romSize = 0x2000;

  /* Also set these inside the loop on any label, or inside the
     opcode procedures whenever a register is altered other
     than by a constant */

  romFlags[0] = TAG_JUMPTARGET_A;

  oldStart = startAdr; oldEnd = endAdr;

  for (pass = PASS_1; pass <= PASS_2; pass++) {

    fprintf(stderr, "Compilation pass %d; start = %04x  end = %04x\n", pass, startAdr, endAdr);

    state = state_A;
    /* Actually on entry they're all 0 in the emulator */
    register_P = 0xDEADBEEFUL;
    register_I = 0xDEADBEEFUL;
    register_A = 0xDEADBEEFUL;
    register_B = 0xDEADBEEFUL;
    this_page = 0x0000;

    startAdr = oldStart; endAdr = oldEnd;

    if (pass == PASS_1) {
      if (codefile != NULL) fclose(codefile);
      codefile = fopen("/dev/null", "w"); // unix-dependant :-( 
      if (codefile == NULL) codefile = fopen("devnull", "w");
    }

    explore(startAdr, endAdr);
    explore(0x1000, endAdr);

    /* BUG! */
    /* Well, apparently there _are_ some calculated jumps in this program,
       and one of them (at 1480:) jumps to 1600:.  We need to not only
       explore the code, but ensure that a label is dumped at that
       address.  we should probably do the same for *all* data segments
       which follow unconditional jumps.  But that is for another
       night... I need to go sleep now... */


    {
      FILE *jumps = fopen("tailgunr.jmp", "r");
      char line[128], *s;
      S_LONG d;
      int i=0;

      // Now that we know all the jump dests, this needs to be cleaned up 

      // sep 2019: fixing an old bug!:
      romFlags[0x0781] |= TAG_JUMPTARGET; explore(0x0781, endAdr);
      romFlags[0x078F] |= TAG_JUMPTARGET; explore(0x078F, endAdr);
      romFlags[0x079C] |= TAG_JUMPTARGET; explore(0x079C, endAdr);
      
      romFlags[0x1600] |= TAG_JUMPTARGET; explore(0x1600, endAdr);
      romFlags[0x1671] |= TAG_JUMPTARGET; explore(0x1671, endAdr);
      romFlags[0x168f] |= TAG_JUMPTARGET; explore(0x168f, endAdr);
      romFlags[0x180f] |= TAG_JUMPTARGET; explore(0x180f, endAdr);
      romFlags[0x182d] |= TAG_JUMPTARGET; explore(0x182d, endAdr);
      romFlags[0x184b] |= TAG_JUMPTARGET; explore(0x184b, endAdr);
      romFlags[0x1878] |= TAG_JUMPTARGET; explore(0x1878, endAdr);
      romFlags[0x18a0] |= TAG_JUMPTARGET; explore(0x18a0, endAdr);
      romFlags[0x198f] |= TAG_JUMPTARGET; explore(0x198f, endAdr);
      romFlags[0x19d7] |= TAG_JUMPTARGET; explore(0x19d7, endAdr);
      romFlags[0x19fb] |= TAG_JUMPTARGET; explore(0x19fb, endAdr);
      romFlags[0x1a0b] |= TAG_JUMPTARGET; explore(0x1a0b, endAdr);

      if (jumps == NULL) {
        fprintf(stderr, "We need a tailgunr.jmp file containing integers, one per line\n");
        exit(1);
      }

      for (;;) {
        s = fgets(line, 127, jumps);
        if (s == NULL) break;
        i += 1;
        s = strchr(line, '\n'); if (s != NULL) *s = '\n';
        s = strchr(line, '\r'); if (s != NULL) *s = '\n';
        d = atol(line);
        fprintf(stderr, "Added from file: %d ('%s', 0x%04x)\n", (int)d, line, (int)d);
        romFlags[(int)d] |= TAG_JUMPTARGET; explore((int)d, endAdr);
      }
      if (i == 0) {
        fprintf(stderr, "tailgunr.jmp appears to be empty\n");
        exit(1);
      }
      if (jumps != NULL) fclose(jumps);
    }

    if (pass == PASS_1) {
      FILE *dispatchfile = NULL;
      if (codefile != NULL) fclose(codefile);
#ifdef DUALCPU
      codefile = fopen("tailgunr-exec.c", "w"); /* unix-dependant, and tailgunr-dependent :-( */
      fprintf(codefile, "void gtExec(void) {\n#include \"dispatch-exec.h\"\n\n  goto *lab[register_PC];\n");
      dispatchfile = fopen("dispatch-exec.h", "w"); /* unix-dependant, and tailgunr-dependent :-( */
#else
      codefile = fopen("tailgunr-ops.c", "w"); /* unix-dependant, and tailgunr-dependent :-( */
      if (jumptableflag) fprintf(codefile, "goto *lab[register_PC];\n");
      dispatchfile = fopen("dispatch-ops.h", "w"); /* unix-dependant, and tailgunr-dependent :-( */
#endif
      romFlags[0x1000] |= TAG_JUMPTARGET_A;
      {
        int labad;
	  if (jumptableflag) {
          fprintf(dispatchfile, "static void *lab[0x2000] = {\n");
          for (labad = 0; labad < 0x2000; labad++){
#ifdef DUALCPU
            if ((romFlags[labad] & TAG_OPCODE) != 0) {
#else
            if ((romFlags[labad] & TAG_JUMPTARGET) != 0) {
#endif
              fprintf(dispatchfile, "&&L%04x, ", labad);
  	      } else {
              fprintf(dispatchfile, "&&Lelse, ");
            }
            if ((labad&7) == 7) fprintf(dispatchfile, "\n");
          }
          fprintf(dispatchfile, "};\n");
        }
      }
      if (dispatchfile != NULL) fclose(dispatchfile);
      dispatchfile = NULL;
    }
  }
#ifdef DUALCPU
  fprintf(codefile, "      return;\nLelse: UNFINISHED(\"ELSE\");\n} /* End of gtExec() */\n");
#endif
  if (codefile != NULL) fclose(codefile);

  fprintf(stderr, "ROMFLAGS[1000] = %04lx\n", romFlags[0x1000]);

  exit(0);
  return(0);
}
