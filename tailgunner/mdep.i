#include "externs.h"

#ifndef _MDEP_C
#define _MDEP_C 1

static UINT8 bBailOut = FALSE;

UINT8 rom[0x2000] =
#include "translate/tailgunner-data.c"	/* Preload whole eprom here */
  static UINT8 ccpu_jmi_dip = 0;	/* as set by cineSetJMI */
static UINT8 ccpu_msize = 0;	/* as set by cineSetMSize */
static UINT8 ccpu_monitor = 0;	/* as set by cineSetMonitor */

UINT8 bFlipX;
UINT8 bFlipY;
UINT8 bSwapXY;
/*UINT8 bOverlay;*/
volatile static UINT32 dwElapsedTicks = 0;

UINT16 ioSwitches;
UINT16 ioInputs;
UINT8 ioOutputs = 0;		/* Where are these used??? */

INT16 JoyX;
INT16 JoyY;
UINT8 bNewFrame;

INT32 sdwGameXSize;
INT32 sdwGameYSize;
INT32 sdwXOffset = 0;
INT32 sdwYOffset = 0;



  /* C-CPU context information begins --  */
CINEWORD register_PC = 0;	/* C-CPU registers; program counter */
					/*register */ CINEWORD register_A = 0;
					/* A-Register (accumulator) */
					/*register */ CINEWORD register_B = 0;
					/* B-Register (accumulator) */
CINEWORD /*CINEBYTE*/ register_I = 0;	/* I-Register (last access RAM location) */
CINEWORD register_J = 0;	/* J-Register (target address for JMP opcodes) */
CINEWORD /*CINEBYTE*/ register_P = 0;	/* Page-Register (4 bits, shifts to high short nibble for code, hight byte nibble for ram) */
CINEWORD FromX = 0;		/* X-Register (start of a vector) */
CINEWORD FromY = 0;		/* Y-Register (start of a vector) */
CINEWORD register_T = 0;	/* T-Register (vector draw length timer) */
CINEWORD flag_C = 0;		/* C-CPU flags; carry. Is word sized, instead
				 * of CINEBYTE, so we can do direct assignment
				 * and then change to BYTE during inspection.
				 */

CINEWORD cmp_old = 0;		/* last accumulator value */
CINEWORD cmp_new = 0;		/* new accumulator value */
CINEWORD /*CINEBYTE*/ acc_a0 = 0;	/* bit0 of A-reg at last accumulator access */

CINESTATE state = state_A;	/* C-CPU state machine current state */
CINEWORD ram[256];		/* C-CPU ram (for all pages) */

CINEWORD vgColour = 0;
CINEBYTE vgShiftLength = 0;	/* number of shifts loaded into length reg */
int bailOut = 0;
int ccpu_ICount = 0;		/* */

  /* -- Context information ends. */

int ccpudebug = 0;		/* default is off */

static int startflag = IO_START;
static int coinflag = 0;
static int shieldsflag = IO_SHIELDS;	/* Whether shields are up (mouse or key) */
static int fireflag = IO_FIRE;
static int quarterflag = SW_QUARTERS_PER_GAME;	/* 1 quarter per game */

const int xmousethresh = 2;
const int ymousethresh = 2;
static int mickeyx = 0;
static int mickeyy = 0;
static int mousecode = 0;
static int sound_addr = 0;
static int sound_addr_A = 0;
static int sound_addr_B = 0;
static int sound_addr_C = 0;
static int sound_data = 0;

void
ERRORf (char *fmt, ...)
{
}

void
reset_coin_counter (int RCstate)
{
  coinflag = SW_COIN;
  ioSwitches |= SW_COIN;
}

int
get_coin_state (void)
{
  return ((coinflag >> 7) & 1);
}

int
get_quarters_per_game (void)
{
  return (quarterflag & 1);
}

void
set_watchdog (void)
{
}

int
get_shield_bit0 (void)
{
  return ((SW_SHIELDS >> 5) & 1);
}

int
get_shield_bit1 (void)
{
  return ((SW_SHIELDS >> 4) & 1);
}

int
get_shield_bit2 (void)
{
  return ((SW_SHIELDS >> 1) & 1);
}

int
get_switch_bit (int n)
{
  return (((int) ioSwitches >> n) & 1);
}

int
get_io_bit (int n)
{
  // crashes on firing ...         if (n == 13) return 1;
  // crashes on pressing start ... if (n == 14) return 1;
  return (((startflag | shieldsflag | fireflag | mousecode /* | inputs */ ) >>
	   n) & 1);
}

int
get_io_laserstyle (void)
{
  return 1;
}

int
get_io_moveright (void)
{
  return (((startflag | shieldsflag | fireflag | mousecode /* | inputs */ ) >>
	   1) & 1);
}

int
get_io_moveleft (void)
{
  return (((startflag | shieldsflag | fireflag | mousecode /* | inputs */ ) >>
	   2) & 1);
}

int
get_io_moveup (void)
{
  return (((startflag | shieldsflag | fireflag | mousecode /* | inputs */ ) >>
	   3) & 1);
}

int
get_io_movedown (void)
{
  return (((startflag | shieldsflag | fireflag | mousecode /* | inputs */ ) >>
	   4) & 1);
}

int
get_io_fire (void)
{
  return (((startflag | shieldsflag | fireflag | mousecode /* | inputs */ ) >>
	   5) & 1);
}

int
get_io_shields (void)
{
  return (((startflag | shieldsflag | fireflag | mousecode /* | inputs */ ) >>
	   6) & 1);
}

int
get_io_startbutton (void)
{
  return (((startflag | shieldsflag | fireflag | mousecode /* | inputs */ ) >>
	   7) & 1);
}

void
put_io_bit (int pos, int how)
{
  if (how == 1)
    {
      /* Set a bit */
      ioInputs = ioInputs | (1 << pos);
    }
  else
    {
      /* Clear a bit */
      ioInputs = ioInputs & (~(1 << pos));
    }
}

void
set_sound_data (int bit)
{
  sound_data = bit;
}

void
set_sound_addr_A (int bit)
{
  sound_addr_A = bit;
}

void
set_sound_addr_B (int bit)
{
  sound_addr_B = bit;
}

void
set_sound_addr_C (int bit)
{
  sound_addr_C = bit;
}

/* These don't yet do anything obvious or useful.  Debugging needed... */
void
strobe_sound_on (void)
{
  int sound_addr_tmp =
    (sound_addr_C << 2) | (sound_addr_B << 1) | sound_addr_A;
  sound_addr = sound_addr_tmp;
  update_sound(1);
}

void
strobe_sound_off (void)
{
  int sound_addr_tmp =
    (sound_addr_C << 2) | (sound_addr_B << 1) | sound_addr_A;
  sound_addr = sound_addr_tmp;
  update_sound(0);
}

extern void v_init(void);
void
init_graph (void)
{
  v_init();
}

extern void vectrexclose(void);
void
end_graph (void)
{
  vectrexclose();
}

void
save_config (void)
{
}

int
load_config (void)
{
  return (TRUE);
}

void
cineReset (void)
{
}

/* From running "gprof" on this code, we found that it spent something
   like 20% of its time in MUL.  The code below replaces the multiple
   single steps of a shift & add multiplier by a single C multiply.
   Fortunately the tailgunner code does not look at Flag_C or any of the
   other affected pseudo-registers used in the original MUL code. */

void
MULS_START (int bits) // 12-bit registers. Not all needed if data known to be shorter
{
  int Q; // NOTE: these *MUST* be ints, not CINEWORD, otherwise >> below is unsigned
  Q = SEX(ram[register_I])*SEX(register_A);
  register_B = (Q >> (bits-1))&0xfff;
  register_A = (Q << (13-bits))&0xfff;
}

void
MULS_END (void)
{
  // in first iteration we kept the original MULs to compare against our shortcut.  Not needed now.
#ifdef NEVER
  if ((fast_register_A != register_A) || (fast_register_B != register_B)) {
    fprintf(stderr, "MUL%d: %03x * %03x -> Q=%07x should be: %03x.%03x vs opt: %03x.%03x  flag_C=%04x\n", saved_bits, saved_ram, saved_A, Q, register_B,register_A, fast_register_B,fast_register_A, flag_C);
  }
#endif
}

void
MUL (void)
{
  // Now handled by optimised code.  Could remove MULs from the translator for neater code if wanted
#ifdef NEVER
  /* opMULirg_A_AA (e3) */
  cmp_new = ram[register_I];
  register_B <<= 4;		/* get sign bit 15 */
  register_B |= (register_A >> 8);	/* bring in A high nibble */
  register_A = ((register_A & 0xFF) << 8) | (0xe3);	/* pick up opcode */
  if (register_A & 0x100)	/* 1bit shifted out? */
    {
      acc_a0 = register_A = (register_A >> 8) | ((register_B & 0xFF) << 8);
      register_A >>= 1;
      register_A &= 0xFFF;
      register_B =
	((unsigned short int) (((signed short int) register_B) >>
			       (signed short int) 4)) /* SAR */ ;
      cmp_old = register_B & 0x0F;
      register_B =
	((unsigned short int) (((signed short int) register_B) >>
			       (signed short int) 1)) /* SAR */ ;
      register_B &= 0xFFF;
      flag_C = (register_B += cmp_new);
      register_B &= 0xFFF;
    }
  else
    {
      register_A =
	(register_A >> 8) | /* Bhigh | Alow */ ((register_B & 0xFF) << 8);
      cmp_old = acc_a0 = register_A & 0xFFF;
      flag_C = (cmp_old + cmp_new);
      register_A >>= 1;
      register_A &= 0xFFF;
      register_B =
	((unsigned short int) (((signed short int) register_B) >>
			       (signed short int) 5)) /* SAR */ ;
      register_B &= 0xFFF;
    }
#endif
}

#endif /* _MDEP_C */
