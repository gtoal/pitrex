#ifndef _EXTERNS_H_
#define	_EXTERNS_H_

#define CARRYBIT (1<<12)
#define A0BIT 1

//#define FALSE 0
//#define TRUE 1


/* for Zonn translation :) */
#define SAR16(var,arg)    ( ( (signed short int) var ) >> arg )

/* for setting/checking the A0 flag */
#define SETA0(var)    ( RCacc_a0 = var )
#define GETA0()       ( RCacc_a0 )

/* for setting/checking the Carry flag */
#define SETFC(val)    ( RCflag_C = val )
#define GETFC()       ( ( RCflag_C >> 8 ) & 0xFF )

/* Contorted sign-extend macro only evaluates its parameter once, and
   executes in two shifts.  This is far more efficient that any other
   sign extend procedure I can think of, and relatively safe */

#define SEX(twelvebit) ((((int)twelvebit) << (int)((sizeof(int)*8)-12)) \
                          >> (int)((sizeof(int)*8)-12))

#define SW_ABORT           0x100          /* for ioSwitches */

#define UINT32 unsigned int
#define UINT16 unsigned short int
#define UINT8  unsigned char

#define INT32  signed int
#define INT16  signed short int
#define INT8   signed char


/* Now the same information for Graham's machine */

/* Define new types for the c-cpu emulator */
typedef short unsigned int CINEWORD;      /* 12bits on the C-CPU */
typedef unsigned char      CINEBYTE;      /* 8 (or less) bits on the C-CPU */
typedef short signed int   CINESWORD;     /* 12bits on the C-CPU */
typedef signed char        CINESBYTE;     /* 8 (or less) bits on the C-CPU */

typedef unsigned long int  CINELONG;

typedef enum
{
	state_A = 0,
	state_AA,
	state_B,
	state_BB
} CINESTATE;

/* INP $8 ? */
#define IO_START   0x80
/* INP $7 ? */
#define IO_SHIELDS 0x40
#define IO_FIRE    0x20
#define IO_DOWN    0x10
#define IO_UP      0x08
#define IO_LEFT    0x04
#define IO_RIGHT   0x02

#define IO_KNOWNBITS (IO_START | IO_SHIELDS | IO_FIRE | IO_DOWN | IO_LEFT | IO_RIGHT)


/* These are used in the translated code:
ops.c:      register_B = cmp_new = (( ioSwitches >> 0x1 ) & 0x01); // shields...
ops.c:      register_B = cmp_new = (( ioSwitches >> 0x4 ) & 0x01);
ops.c:      register_B = cmp_new = (( ioSwitches >> 0x5 ) & 0x01);

ops.c:      register_B = cmp_new = (( ioSwitches >> 0x7 ) & 0x01); // coin state
ops.c:      register_B = cmp_new = (( ioSwitches >> 0x0 ) & 0x01); // quarters per game
*/
#define SW_QUARTERS_PER_GAME 0x01
#define SW_COIN   0x80



/* initial value of shields (on a DIP) */
#define SW_SHIELDS80 ((1<<1) | (1<<4) | (1<<5))  /* 000 */
#define SW_SHIELDS70 ((1<<1) | (0<<4) | (1<<5))  /* 010 */
#define SW_SHIELDS60 ((1<<1) | (1<<4) | (0<<5))  /* 001 */
#define SW_SHIELDS50 ((1<<1) | (0<<4) | (0<<5))  /* 011 */
#define SW_SHIELDS40 ((0<<1) | (1<<4) | (1<<5))  /* 100 */
#define SW_SHIELDS30 ((0<<1) | (0<<4) | (1<<5))  /* 110 */
#define SW_SHIELDS20 ((0<<1) | (1<<4) | (0<<5))  /* 101 */
#define SW_SHIELDS15 ((0<<1) | (0<<4) | (0<<5))  /* 111 */
#define SW_SHIELDS15 ((0<<1) | (0<<4) | (0<<5))  /* 111 */
#define SW_SHIELDS SW_SHIELDS80

#define SW_SHIELDMASK ((1<<1) | (1<<4) | (1<<5))  /* 000 */


extern void ERRORf(char *fmt, ...);
extern int get_coin_state(void);
extern int get_quarters_per_game(void);
extern void set_watchdog(void);
extern int get_shield_bit0(void);
extern int get_shield_bit1(void);
extern int get_shield_bit2(void);
extern int get_switch_bit(int n);
extern int get_io_bit(int n);
extern int get_io_laserstyle(void);
extern int get_io_moveright(void);
extern int get_io_moveleft(void);
extern int get_io_moveup(void);
extern int get_io_movedown(void);
extern int get_io_fire(void);
extern int get_io_shields(void);
extern int get_io_startbutton(void);
extern void put_io_bit(int pos, int how);
extern void set_sound_data(int bit);
extern void set_sound_addr_A(int bit);
extern void set_sound_addr_B(int bit);
extern void set_sound_addr_C(int bit);
extern void strobe_sound_on(void);
extern void strobe_sound_off(void);
extern void update_sound(int);

extern void init_graph(void);
extern void end_graph(void);
extern void save_config(void);
extern int load_config(void);
extern void cineReset(void);
extern void MULS_START(int bits);
extern void MULS_END(void);
extern void MUL(void);
extern void CinemaClearScreen(void);
extern void CinemaVectorData(int FromX, int FromY, int ToX, int ToY, int vgColour);
extern void CinemaVectorDataHinted(int FromX, int FromY, int ToX, int ToY, int vgColour, int hint);
extern void cineSetJMI(UINT8 j);
extern void cineSetMSize(UINT8 m);
extern void cineSetMonitor(UINT8 m);
extern void cineReleaseTimeslice(void);
extern void cineExecuteFrame(void);
extern void initTailGunner(void);
extern void reset_coin_counter(int bit);
extern void cineExecute0000(void);
extern void cineExecute0400(void);
extern void cineExecute0800(void);
extern void cineExecute0c00(void);
extern void cineExecute1000(void);
extern void cineExecute1400(void);
extern void cineExecute1800(void);
extern void cineExecute1c00(void);

#include "mdep.h"

#endif
