/***************************************************************************

						-= Jaleco Mega System 1 =-

					driver by	Luca Elia (l.elia@tin.it)


To enter service mode in some games press service1+F3.


Year + Game							System		Protection
----------------------------------------------------------------------------
88	Legend of Makai (World) /		Z
	Makai Densetsu  (Japan)			Z
	P-47  (World) /					A
	P-47  (Japan)					A
	Kick Off (Japan)				A		*
	Takeda Shingen (Japan)			A		*	      Encryption (key 1)
	Iga Ninjyutsuden (Japan)		A		*	Yes + Encryption (key 1)
89	Astyanax          (World) /		A		*	Yes + Encryption (key 2)
	The Lord of King  (Japan)		A	 	*	Yes + Encryption (key 2)
	Hachoo!							A		*	Yes + Encryption (key 2)
	Jitsuryoku!! Pro Yakyuu (Japan)	A			Yes + Encryption (key 2)
	Plus Alpha						A			Yes + Encryption (key 2)
	Saint Dragon					A			Yes + Encryption (key 1)
90	RodLand  (World) /				A			      Encryption (key 3)
	RodLand  (Japan)				A			      Encryption (key 2)
	Phantasm        (Japan)	/		A			      Encryption (key 1)
91	Avenging Spirit (World) 		B			Inputs
	Earth Defense Force				B			Inputs
	64th Street  (World) /			C		*	Inputs
	64th Street  (Japan)			C		*	Inputs
92	Soldam (Japan)					A			      Encryption (key 2)
	Big Striker						C		*	Inputs
93	Chimera Beast					C		*	Inputs
	Cybattler						C			Inputs
	Peek-a-Boo!						D			Inputs
--------------------------------------------^-------------------------------
											|
							The Priority Prom is missing for these games !



Hardware	Main CPU	Sound CPU	Sound Chips
-----------------------------------------------------------
MS1 - Z		68000		Z80			YM2203c
MS1 - A		68000		68000		YM2151		2xOKI-M6295
MS1 - B		68000		68000		YM2151		2xOKI-M6295
MS1 - C		68000		68000		YM2151		2xOKI-M6295
MS1 - D		68000		-			-			  OKI-M6295
-----------------------------------------------------------



Main CPU	RW		MS1-A/Z			MS1-B			MS1-C			MS1-D
-----------------------------------------------------------------------------------
ROM			R	000000-03ffff	000000-03ffff	000000-07ffff	000000-03ffff
								080000-0bffff
Video Regs	 W	084000-0843ff	044000-0443ff	0c0000-0cffff	0c0000-0cffff
Palette		RW	088000-0887ff	048000-0487ff	0f8000-0f87ff	0d8000-0d87ff
Object RAM	RW	08e000-08ffff	04e000-04ffff	0d2000-0d3fff	0ca000-0cbfff
Scroll 0	RW	090000-093fff	050000-053fff	0e0000-0e3fff	0d0000-0d3fff
Scroll 1	RW	094000-097fff	054000-057fff	0e8000-0ebfff	0e8000-0ebfff
Scroll 2	RW	098000-09bfff	058000-05bfff	0f0000-0f3fff	-
Work RAM	RW	0f0000-0fffff*	060000-07ffff*	1f0000-1fffff*	1f0000-1fffff
Input Ports R	080000-080009	0e0000-0e0001**	0d8000-d80001**	100000-100001**
-----------------------------------------------------------------------------------
*  Some games use mirror addresses
** Through protection.



Sound CPU		RW		MS1-A			MS1-B			MS1-C			MS1-D
-----------------------------------------------------------------------------------
ROM				R	000000-01ffff	000000-01ffff	000000-01ffff	 No Sound CPU
Latch #1		R	040000-040001	<				060000-060001
Latch #2		 W	060000-060001	<				<
2151 reg		 W	080000-080001	<				<
2151 data		 W	080002-080003	<				<
2151 status		R 	080002-080003	<				<
6295 #1 data	 W 	0a0000-0a0003	<				<
6295 #1 status	R 	0a0000-0a0001	<				<
6295 #2 data	 W 	0c0000-0c0003	<				<
6295 #2 status	R 	0c0000-0c0001	<				<
RAM				RW	0f0000-0f3fff	0e0000-0effff?	<
-----------------------------------------------------------------------------------


								Issues / To Do
								--------------

- There's a 512 byte PROM in the video section (different for every game)
  that controls the priorities. It's been dumped for only a few games, so
  we have to use fake data for the missing ones.

- Making the M6295 status register return 0 fixes the music tempo in
  avspirit, 64street, astyanax etc. but makes most of the effects in
  hachoo disappear! Define SOUND_HACK to 0 to turn this hack off
  This seems to be some Jaleco magic at work (strange protection?). The
  bootleg version of rodlandj has one instruction patched out to do exactly
  the same thing that we are doing (ignoring the 6295 status).

- Iganinju doesn't work properly: I have to patch lev3 irq and it severely
  slows down at times. Strangely, it gets *better* by lowering the main CPU
  clock from 12 to 7 MHz.
  This is likely an interrupt timing issue: changing the order in
  interrupt_A() from 3 2 1 to 1 2 3 makes it work reasonably well for a very
  short while.

- VERY bad sprite lag in iganinju and plusalph and generally others.
  Is this a sprites buffer issue ?

- Understand a handful of unknown bits in video regs


***************************************************************************/

/* This will fix tempo and samples in at least avsprit, 64street, astyanax */
/* but will break at least one game: hachoo */

#define SOUND_HACK 1

#include "driver.h"
#include "drivers/megasys1.h"
#include "vidhrdw/generic.h"


/* Variables only used here: */

static data16_t ip_select, ip_select_values[5];


/* Variables defined in vidhrdw: */



/* Functions defined in vidhrdw: */

void megasys1_convert_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *prom);
int  megasys1_vh_start(void);
void megasys1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

READ16_HANDLER( megasys1_vregs_C_r );

WRITE16_HANDLER( megasys1_vregs_A_w );
WRITE16_HANDLER( megasys1_vregs_C_w );
WRITE16_HANDLER( megasys1_vregs_D_w );




void megasys1_init_machine(void)
{
	ip_select = 0;	/* reset protection */
}



/*
**
**	Main cpu data
**
**
*/


/***************************************************************************
						[ Main CPU - System A / Z ]
***************************************************************************/

static READ16_HANDLER( coins_r )	{return readinputport(0);}	/* < 00 | Coins > */
static READ16_HANDLER( player1_r )	{return readinputport(1);}	/* < 00 | Player 1 > */
static READ16_HANDLER( player2_r )	{return readinputport(2) * 256 +
									        readinputport(3);}		/* < Reserve | Player 2 > */
static READ16_HANDLER( dsw1_r )		{return readinputport(4);}							/*   DSW 1 */
static READ16_HANDLER( dsw2_r )		{return readinputport(5);}							/*   DSW 2 */
static READ16_HANDLER( dsw_r )		{return readinputport(4) * 256 +
									        readinputport(5);}		/* < DSW 1 | DSW 2 > */


#define INTERRUPT_NUM_A		3
int interrupt_A(void)
{
	switch ( cpu_getiloops() )
	{
		case 0:		return 3;
		case 1:		return 2;
		case 2:		return 1;
		default:	return ignore_interrupt();
	}
}




static MEMORY_READ16_START( readmem_A )
	MEMORY_ADDRESS_BITS(20)
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x080000, 0x080001, coins_r },
	{ 0x080002, 0x080003, player1_r },
	{ 0x080004, 0x080005, player2_r },
	{ 0x080006, 0x080007, dsw_r },
	{ 0x080008, 0x080009, soundlatch2_word_r },	/* from sound cpu */
	{ 0x084000, 0x084fff, MRA16_RAM },
	{ 0x088000, 0x0887ff, paletteram16_word_r },
	{ 0x08e000, 0x08ffff, MRA16_RAM },
	{ 0x090000, 0x093fff, MRA16_RAM },
	{ 0x094000, 0x097fff, MRA16_RAM },
	{ 0x098000, 0x09bfff, MRA16_RAM },
	{ 0x0f0000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem_A )
	MEMORY_ADDRESS_BITS(20)
 	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x084000, 0x0843ff, megasys1_vregs_A_w, &megasys1_vregs },
	{ 0x088000, 0x0887ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x08e000, 0x08ffff, MWA16_RAM, &megasys1_objectram },
	{ 0x090000, 0x093fff, megasys1_scrollram_0_w, &megasys1_scrollram_0 },
	{ 0x094000, 0x097fff, megasys1_scrollram_1_w, &megasys1_scrollram_1 },
	{ 0x098000, 0x09bfff, megasys1_scrollram_2_w, &megasys1_scrollram_2 },
	{ 0x0f0000, 0x0fffff, MWA16_RAM, &megasys1_ram },
MEMORY_END



/***************************************************************************
							[ Main CPU - System B ]
***************************************************************************/

#define INTERRUPT_NUM_B		3
int interrupt_B(void)
{
	switch (cpu_getiloops())
	{
		case 0:		return 4;
		case 1:		return 1;
		default:	return 2;
	}
}



/*			 Read the input ports, through a protection device:

 ip_select_values must contain the 5 codes sent to the protection device
 in order to obtain the status of the following 5 input ports:

		 Coins	Player1		Player2		DSW1		DSW2

 in that order.			*/

static READ16_HANDLER( ip_select_r )
{
	int i;

/*	Coins	P1		P2		DSW1	DSW2 */
/*	57		53		54		55		56		< 64street */
/*	37		35		36		33		34		< avspirit */
/*	58		54		55		56		57		< bigstrik */
/*	56		52		53		54		55		< cybattlr */
/* 	20		21		22		23		24		< edf */

	/* f(x) = ((x*x)>>4)&0xFF ; f(f($D)) == 6 */
	if ((ip_select & 0xF0) == 0xF0) return 0x000D;

	for (i = 0; i < 5; i++)	if (ip_select == ip_select_values[i]) break;

	switch (i)
	{
			case 0 :	return coins_r(0,0);	break;
			case 1 :	return player1_r(0,0);	break;
			case 2 :	return player2_r(0,0);	break;
			case 3 :	return dsw1_r(0,0);		break;
			case 4 :	return dsw2_r(0,0);		break;
			default	 :	return 0x0006;
	}
}

static WRITE16_HANDLER( ip_select_w )
{
	COMBINE_DATA(&ip_select);
	cpu_cause_interrupt(0,2);
}


static MEMORY_READ16_START( readmem_B )
	MEMORY_ADDRESS_BITS(20)
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x044000, 0x044fff, MRA16_RAM },
	{ 0x048000, 0x0487ff, paletteram16_word_r },
	{ 0x04e000, 0x04ffff, MRA16_RAM },
	{ 0x050000, 0x053fff, MRA16_RAM },
	{ 0x054000, 0x057fff, MRA16_RAM },
	{ 0x058000, 0x05bfff, MRA16_RAM },
	{ 0x060000, 0x07ffff, MRA16_RAM },
	{ 0x080000, 0x0bffff, MRA16_ROM },
	{ 0x0e0000, 0x0e0001, ip_select_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem_B )
	MEMORY_ADDRESS_BITS(20)
 	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x044000, 0x0443ff, megasys1_vregs_A_w, &megasys1_vregs },
	{ 0x048000, 0x0487ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x04e000, 0x04ffff, MWA16_RAM, &megasys1_objectram },
	{ 0x050000, 0x053fff, megasys1_scrollram_0_w, &megasys1_scrollram_0 },
	{ 0x054000, 0x057fff, megasys1_scrollram_1_w, &megasys1_scrollram_1 },
	{ 0x058000, 0x05bfff, megasys1_scrollram_2_w, &megasys1_scrollram_2 },
	{ 0x060000, 0x07ffff, MWA16_RAM, &megasys1_ram },
	{ 0x080000, 0x0bffff, MWA16_ROM },
	{ 0x0e0000, 0x0e0001, ip_select_w },
MEMORY_END



/***************************************************************************
							[ Main CPU - System C ]
***************************************************************************/


#define INTERRUPT_NUM_C	INTERRUPT_NUM_B
#define interrupt_C		interrupt_B

static MEMORY_READ16_START( readmem_C )
	MEMORY_ADDRESS_BITS(21)
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x0c0000, 0x0cffff, megasys1_vregs_C_r },
	{ 0x0d2000, 0x0d3fff, MRA16_RAM },
	{ 0x0e0000, 0x0e3fff, MRA16_RAM },
	{ 0x0e8000, 0x0ebfff, MRA16_RAM },
	{ 0x0f0000, 0x0f3fff, MRA16_RAM },
	{ 0x0f8000, 0x0f87ff, paletteram16_word_r },
	{ 0x0d8000, 0x0d8001, ip_select_r },
	{ 0x1f0000, 0x1fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem_C )
	MEMORY_ADDRESS_BITS(21)
 	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x0c0000, 0x0cffff, megasys1_vregs_C_w, &megasys1_vregs },
	{ 0x0d2000, 0x0d3fff, MWA16_RAM, &megasys1_objectram },
	{ 0x0e0000, 0x0e3fff, megasys1_scrollram_0_w, &megasys1_scrollram_0 },
	{ 0x0e8000, 0x0ebfff, megasys1_scrollram_1_w, &megasys1_scrollram_1 },
	{ 0x0f0000, 0x0f3fff, megasys1_scrollram_2_w, &megasys1_scrollram_2 },
	{ 0x0f8000, 0x0f87ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x0d8000, 0x0d8001, ip_select_w },
	{ 0x1f0000, 0x1fffff, MWA16_RAM, &megasys1_ram },
MEMORY_END




/***************************************************************************
							[ Main CPU - System D ]
***************************************************************************/

#define INTERRUPT_NUM_D		1
int interrupt_D(void)
{
	return 2;
}

static MEMORY_READ16_START( readmem_D )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x0c0000, 0x0c9fff, MRA16_RAM },
	{ 0x0ca000, 0x0cbfff, MRA16_RAM },
	{ 0x0d0000, 0x0d3fff, MRA16_RAM },
	{ 0x0d4000, 0x0d7fff, MRA16_RAM },
	{ 0x0d8000, 0x0d87ff, paletteram16_word_r },
	{ 0x0db000, 0x0db7ff, paletteram16_word_r },
	{ 0x0e0000, 0x0e0001, dsw_r },
	{ 0x0e8000, 0x0ebfff, MRA16_RAM },
	{ 0x0f0000, 0x0f0001, coins_r }, /* Coins + P1&P2 Buttons */
	{ 0x0f8000, 0x0f8001, OKIM6295_status_0_lsb_r },
/*	{ 0x100000, 0x100001  protection */
	{ 0x1f0000, 0x1fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem_D )
 	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x0c0000, 0x0c9fff, megasys1_vregs_D_w, &megasys1_vregs },
	{ 0x0ca000, 0x0cbfff, MWA16_RAM, &megasys1_objectram },
	{ 0x0d0000, 0x0d3fff, megasys1_scrollram_1_w, &megasys1_scrollram_1 },
	{ 0x0d4000, 0x0d7fff, megasys1_scrollram_2_w, &megasys1_scrollram_2 },
	{ 0x0d8000, 0x0d87ff, paletteram16_RRRRRGGGGGBBBBBx_word_w },
	{ 0x0db000, 0x0db7ff, paletteram16_RRRRRGGGGGBBBBBx_word_w, &paletteram16 },
	{ 0x0e8000, 0x0ebfff, megasys1_scrollram_0_w, &megasys1_scrollram_0 },
	{ 0x0f8000, 0x0f8001, OKIM6295_data_0_lsb_w },
/*	{ 0x100000, 0x100001  protection */
	{ 0x1f0000, 0x1fffff, MWA16_RAM, &megasys1_ram },
MEMORY_END




/*
**
**	Sound cpu data
**
**
*/

/*
							[ Sound CPU interrupts ]

	[MS1-A]
		astyanax				all rte
		hachoo					all reset the program, but the status
								register is set to 2700
		iganinju				all rte
		p47 & p47j				all rte
		phantasm				all rte (4 is different, but rte)
		plusalph				all rte
		rodland	& rodlandj		all rte (4 is different, but rte)
		stdragon				4]read & store sound command and echo to main cpu
								rest: rte
	[MS1-B]
		avspirit				all rte (4 is different, but rte)
		edf						all rte (4 is different, but rte)

	[MS1-C]
		64street				all rte (4 is different, but rte)
		chimerab				all rte
		cybattlr
			1;3;5-7]400	busy loop
			2]40c	read & store sound command and echo to main cpu
			4]446	rte


 These games almost always don't use the interrupts to drive the music
 tempo (cybattlr and stdragon do!) but use the YM2151 timers instead
 (they poll the status register). Since those timers are affected by
 the YM2151 clock, it's this latter that ultimately decides the music
 tempo.

 Note that some games' music is severely slowed down and out of sync
 (avspirit, 64street) by the fact that the game waits for some samples
 to be played entirely (M6295 status register polled) but they take
 too much time (and raising the M6295 clock rate would, on the other
 hand, screw the pitch of the samples)

 A temporary fix is to make the status of this chip return 0...
 unfortunately, this trick makes most of the effects disappear in at
 least one game: hachoo!

*/

/* See stdragon, which is interrupt driven */
#define SOUND_INTERRUPT_NUM		8
static int sound_interrupt(void)
{
	return 4;
}


/***************************************************************************
							[ Sound CPU - System A ]
***************************************************************************/


static MEMORY_READ16_START( sound_readmem_A )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x040000, 0x040001, soundlatch_word_r },
	{ 0x080002, 0x080003, YM2151_status_port_0_lsb_r },
#if SOUND_HACK
	{ 0x0a0000, 0x0a0001, MRA16_NOP },
	{ 0x0c0000, 0x0c0001, MRA16_NOP },
#else
	{ 0x0a0000, 0x0a0001, OKIM6295_status_0_lsb_r },
	{ 0x0c0000, 0x0c0001, OKIM6295_status_1_lsb_r },
#endif
	{ 0x0e0000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( sound_writemem_A )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x060000, 0x060001, soundlatch2_word_w },	/* to main cpu */
	{ 0x080000, 0x080001, YM2151_register_port_0_lsb_w },
	{ 0x080002, 0x080003, YM2151_data_port_0_lsb_w },
	{ 0x0a0000, 0x0a0003, OKIM6295_data_0_lsb_w },
	{ 0x0c0000, 0x0c0003, OKIM6295_data_1_lsb_w },
	{ 0x0e0000, 0x0fffff, MWA16_RAM },
MEMORY_END






/***************************************************************************
						[ Sound CPU - System B / C ]
***************************************************************************/


static MEMORY_READ16_START( sound_readmem_B )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x040000, 0x040001, soundlatch_word_r },	/* from main cpu */
	{ 0x060000, 0x060001, soundlatch_word_r },	/* from main cpu */
	{ 0x080002, 0x080003, YM2151_status_port_0_lsb_r },
#if SOUND_HACK
	{ 0x0a0000, 0x0a0001, MRA16_NOP },
	{ 0x0c0000, 0x0c0001, MRA16_NOP },
#else
	{ 0x0a0000, 0x0a0001, OKIM6295_status_0_lsb_r },
	{ 0x0c0000, 0x0c0001, OKIM6295_status_1_lsb_r },
#endif
	{ 0x0e0000, 0x0effff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( sound_writemem_B )
	{ 0x000000, 0x01ffff, MWA16_ROM	 },
	{ 0x040000, 0x040001, soundlatch2_word_w },	/* to main cpu */
	{ 0x060000, 0x060001, soundlatch2_word_w },	/* to main cpu */
	{ 0x080000, 0x080001, YM2151_register_port_0_lsb_w },
	{ 0x080002, 0x080003, YM2151_data_port_0_lsb_w },
	{ 0x0a0000, 0x0a0003, OKIM6295_data_0_lsb_w },
	{ 0x0c0000, 0x0c0003, OKIM6295_data_1_lsb_w },
	{ 0x0e0000, 0x0effff, MWA16_RAM },
MEMORY_END

#define sound_readmem_C		sound_readmem_B
#define sound_writemem_C	sound_writemem_B






/***************************************************************************
						[ Sound CPU - System Z ]
***************************************************************************/



static MEMORY_READ_START( sound_readmem_z80 )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_z80 )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, MWA_NOP }, /* ?? */
MEMORY_END


static PORT_READ_START( sound_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
PORT_END







static struct GfxLayout tilelayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
		0*4+32*16, 1*4+32*16, 2*4+32*16, 3*4+32*16, 4*4+32*16, 5*4+32*16, 6*4+32*16, 7*4+32*16},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32},
	32*16*2
};

static struct GfxDecodeInfo gfxdecodeinfo_Z[] =
{
	{ REGION_GFX1, 0, &tilelayout,   256*0, 16 },	/* [0] Scroll 0 */
	{ REGION_GFX2, 0, &tilelayout,   256*2, 16 },	/* [1] Scroll 1 */
	{ REGION_GFX3, 0, &spritelayout, 256*1, 16 },	/* [2] Sprites */
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo_ABC[] =
{
	{ REGION_GFX1, 0, &tilelayout,   256*0, 16 },	/* [0] Scroll 0 */
	{ REGION_GFX2, 0, &tilelayout,   256*1, 16 },	/* [1] Scroll 1 */
	{ REGION_GFX3, 0, &tilelayout,   256*2, 16 },	/* [2] Scroll 2 (unused in system D) */
	{ REGION_GFX4, 0, &spritelayout, 256*3, 16 },	/* [3] Sprites */
	{ -1 }
};


/***************************************************************************

							Machine Driver Macros

***************************************************************************/

/***************************************************************************

						[  Mega System 1 A,B and C ]

						  2x68000 2xM6295 1xYM2151

***************************************************************************/

/* Provided by Jim Hernandez: 3.5MHz for FM, 30KHz (!) for ADPCM */

static struct YM2151interface ym2151_interface =
{
	1,
	7000000/2,
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct OKIM6295interface okim6295_interface =
{
	2,
	{ 4000000/132, 4000000/132 },	/* seems appropriate */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ MIXER(30,MIXER_PAN_LEFT), MIXER(30,MIXER_PAN_RIGHT) }
};

#define MEGASYS1_MACHINE( _type_, _main_clock_,_sound_clock_) \
static struct MachineDriver machine_driver_system_##_type_ = \
{ \
	{ \
		{ \
			CPU_M68000, \
			_main_clock_, \
			readmem_##_type_,writemem_##_type_,0,0, \
			interrupt_##_type_,INTERRUPT_NUM_##_type_ \
		}, \
		{ \
			CPU_M68000 | CPU_AUDIO_CPU, \
			_sound_clock_, \
			sound_readmem_##_type_,sound_writemem_##_type_,0,0, \
			sound_interrupt,SOUND_INTERRUPT_NUM, \
		}, \
	}, \
	60, DEFAULT_60HZ_VBLANK_DURATION, \
	1, \
	megasys1_init_machine, \
	/* video hardware */ \
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 }, \
	gfxdecodeinfo_ABC, \
	1024, 1024, \
	megasys1_convert_prom, \
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE, \
	0, \
	megasys1_vh_start, \
	0, \
	megasys1_vh_screenrefresh, \
	/* sound hardware */ \
	SOUND_SUPPORTS_STEREO,0,0,0, \
	{ \
		{	SOUND_YM2151,	&ym2151_interface	}, \
		{	SOUND_OKIM6295,	&okim6295_interface	}  \
	} \
};


/* OSC:	4, 7, 12 MHz */
MEGASYS1_MACHINE( A, 12000000,7000000 )

/* OSC:	8, 7, 12 MHz */
MEGASYS1_MACHINE( B, 12000000,7000000 )

/* OSC: 7, 24 MHz */
MEGASYS1_MACHINE( C, 12000000,7000000 )

/* OSC:	4, 8, 7, 12 MHz - Type A, but only one interrupt per frame on the sound CPU */
static struct MachineDriver machine_driver_jitsupro =
{
	{
		{
			CPU_M68000,
			12000000,
			readmem_A,writemem_A,0,0,
			interrupt_A,INTERRUPT_NUM_A
		},
		{
			CPU_M68000 | CPU_AUDIO_CPU,
			7000000,
			sound_readmem_A,sound_writemem_A,0,0,
			sound_interrupt,1,
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	megasys1_init_machine,
	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo_ABC,
	1024, 1024,
	megasys1_convert_prom,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	megasys1_vh_start,
	0,
	megasys1_vh_screenrefresh,
	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM2151,	&ym2151_interface	},
		{	SOUND_OKIM6295,	&okim6295_interface	}
	}
};


/***************************************************************************

							[ Mega System 1 D ]

							  1x68000 1xM6295

KLOV entry for peekaboo: Jaleco board no. PB-92127A. Main CPU: Motorola 68000P10

***************************************************************************/


static struct OKIM6295interface okim6295_interface_D =
{
	1,
	{ 12000 },
	{ REGION_SOUND1 },
	{ 100 }
};

static struct MachineDriver machine_driver_system_D =
{
	{
		{
			CPU_M68000,
			10000000,	/* ? */
			readmem_D,writemem_D,0,0,
			interrupt_D,INTERRUPT_NUM_D
		}
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	megasys1_init_machine,
	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo_ABC,
	1024, 1024,
	megasys1_convert_prom,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	megasys1_vh_start,
	0,
	megasys1_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{	SOUND_OKIM6295,	&okim6295_interface_D	}
	}
};




/***************************************************************************

							[  Mega System 1 Z ]

							 68000+Z80 1xYM2203

							OSC:	5, 12 MHz

***************************************************************************/


static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}


static struct YM2203interface ym2203_interface =
{
	1,
	1500000,	/* ??? */
	{ YM2203_VOL(50,50) },
	{ 0 },
	{ 0 },
	{ 0	},
	{ 0 },
	{ irq_handler }
};

static struct MachineDriver machine_driver_system_Z =
{
	{
		{
			CPU_M68000,
			6000000, /* ??? */
			readmem_A,writemem_A,0,0,
			interrupt_A,INTERRUPT_NUM_A
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000, /* ??? */
			sound_readmem_z80,sound_writemem_z80,sound_readport,sound_writeport,
			ignore_interrupt,0	/* irq generated by YM2203 */
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,
	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo_Z,
	256*3, 256*3,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	megasys1_vh_start,
	0,
	megasys1_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0, /* This system is mono */
	{
		{	SOUND_YM2203,	&ym2203_interface	},
	}
};





/***************************************************************************

								ROMs Loading

***************************************************************************/



/***************************************************************************

							[ 64th Street ]

(World version)
interrupts:	1] 10eac:	disabled while b6c4=6 (10fb6 test)
						if (8b1c)	8b1c<-0
							color cycle
							copies 800 bytes 98da->8008

			2] 10f28:	switch b6c4
						0	RTE
						2	10f44:	M[b6c2]<-d8000; b6c4<-4
						4	10f6c:	next b6c2 & d8000.	if (b6c2>A)	b6c2,4<-0
														else		b6c4  <-2
						6	10f82: b6c6<-(d8001) b6c7<-FF (test)

			4] 10ed0:	disabled while b6c4=6 (10fb6 test)
						watchdog 8b1e
						many routines...
						b6c2<-0

13ca	print a string: a7->screen disp.l(base=f0004),src.l
13ea	print a string: a1->(chars)*
1253c	hw test (table of tests at 125c6)		*TRAP#D*
125f8	mem test (table of mem tests at 126d4)
1278e	input test (table of tests at 12808)
128a8	sound test	12a08	crt test
12aca	dsw test (b68e.w = dswa.b|dswb.b)

ff8b1e.w	incremented by int4, when >= b TRAP#E (software watchdog error)
ff9df8.w	*** level ***

***************************************************************************/

ROM_START( 64street )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "64th_03.rom", 0x000000, 0x040000, 0xed6c6942 )
	ROM_LOAD16_BYTE( "64th_02.rom", 0x000001, 0x040000, 0x0621ed1d )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "64th_08.rom", 0x000000, 0x010000, 0x632be0c1 )
	ROM_LOAD16_BYTE( "64th_07.rom", 0x000001, 0x010000, 0x13595d01 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "64th_01.rom", 0x000000, 0x080000, 0x06222f90 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "64th_06.rom", 0x000000, 0x080000, 0x2bfcdc75 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "64th_09.rom", 0x000000, 0x020000, 0xa4a97db4 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "64th_05.rom", 0x000000, 0x080000, 0xa89a7020 )
	ROM_LOAD( "64th_04.rom", 0x080000, 0x080000, 0x98f83ef6 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "64th_11.rom", 0x000000, 0x020000, 0xb0b8a65c )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "64th_10.rom", 0x000000, 0x040000, 0xa3390561 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom",        0x0000, 0x0200, 0x00000000 )
ROM_END


ROM_START( 64streej )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "91105-3.bin", 0x000000, 0x040000, 0xa211a83b )
	ROM_LOAD16_BYTE( "91105-2.bin", 0x000001, 0x040000, 0x27c1f436 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "64th_08.rom", 0x000000, 0x010000, 0x632be0c1 )
	ROM_LOAD16_BYTE( "64th_07.rom", 0x000001, 0x010000, 0x13595d01 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "64th_01.rom", 0x000000, 0x080000, 0x06222f90 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "64th_06.rom", 0x000000, 0x080000, 0x2bfcdc75 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "64th_09.rom", 0x000000, 0x020000, 0xa4a97db4 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "64th_05.rom", 0x000000, 0x080000, 0xa89a7020 )
	ROM_LOAD( "64th_04.rom", 0x080000, 0x080000, 0x98f83ef6 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "64th_11.rom", 0x000000, 0x020000, 0xb0b8a65c )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "64th_10.rom", 0x000000, 0x040000, 0xa3390561 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom",        0x0000, 0x0200, 0x00000000 )
ROM_END



INPUT_PORTS_START( 64street )
	COINS
/*	fire	jump */
	JOY_2BUTTONS(IPF_PLAYER1)
	RESERVE				/* Unused */
	JOY_2BUTTONS(IPF_PLAYER2)

	PORT_START
	COINAGE_8BITS

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x60, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

INPUT_PORTS_END


/***************************************************************************

					[ The Astyanax ] / [ The Lord of King ]

interrupts:	1] 1aa	2] 1b4

***************************************************************************/


ROM_START( astyanax )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "astyan2.bin", 0x00000, 0x20000, 0x1b598dcc )
	ROM_LOAD16_BYTE( "astyan1.bin", 0x00001, 0x20000, 0x1a1ad3cf )
	ROM_LOAD16_BYTE( "astyan3.bin", 0x40000, 0x10000, 0x097b53a6 )
	ROM_LOAD16_BYTE( "astyan4.bin", 0x40001, 0x10000, 0x1e1cbdb2 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "astyan5.bin",  0x000000, 0x010000, 0x11c74045 )
	ROM_LOAD16_BYTE( "astyan6.bin",  0x000001, 0x010000, 0xeecd4b16 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "astyan11.bin", 0x000000, 0x020000, 0x5593fec9 )
	ROM_LOAD( "astyan12.bin", 0x020000, 0x020000, 0xe8b313ec )
	ROM_LOAD( "astyan13.bin", 0x040000, 0x020000, 0x5f3496c6 )
	ROM_LOAD( "astyan14.bin", 0x060000, 0x020000, 0x29a09ec2 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "astyan15.bin", 0x000000, 0x020000, 0x0d316615 )
	ROM_LOAD( "astyan16.bin", 0x020000, 0x020000, 0xba96e8d9 )
	ROM_LOAD( "astyan17.bin", 0x040000, 0x020000, 0xbe60ba06 )
	ROM_LOAD( "astyan18.bin", 0x060000, 0x020000, 0x3668da3d )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "astyan19.bin", 0x000000, 0x020000, 0x98158623 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "astyan20.bin", 0x000000, 0x020000, 0xc1ad9aa0 )
	ROM_LOAD( "astyan21.bin", 0x020000, 0x020000, 0x0bf498ee )
	ROM_LOAD( "astyan22.bin", 0x040000, 0x020000, 0x5f04d9b1 )
	ROM_LOAD( "astyan23.bin", 0x060000, 0x020000, 0x7bd4d1e7 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "astyan9.bin",  0x000000, 0x020000, 0xa10b3f17 )
	ROM_LOAD( "astyan10.bin", 0x020000, 0x020000, 0x4f704e7a )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "astyan7.bin",  0x000000, 0x020000, 0x319418cc )
	ROM_LOAD( "astyan8.bin",  0x020000, 0x020000, 0x5e5d2a22 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom",         0x0000, 0x0200, 0x00000000 )
ROM_END

ROM_START( lordofk )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "lokj02.bin", 0x00000, 0x20000, 0x0d7f9b4a )
	ROM_LOAD16_BYTE( "lokj01.bin", 0x00001, 0x20000, 0xbed3cb93 )
	ROM_LOAD16_BYTE( "lokj03.bin", 0x40000, 0x20000, 0xd8702c91 )
	ROM_LOAD16_BYTE( "lokj04.bin", 0x40001, 0x20000, 0xeccbf8c9 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "astyan5.bin",  0x000000, 0x010000, 0x11c74045 )
	ROM_LOAD16_BYTE( "astyan6.bin",  0x000001, 0x010000, 0xeecd4b16 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "astyan11.bin", 0x000000, 0x020000, 0x5593fec9 )
	ROM_LOAD( "astyan12.bin", 0x020000, 0x020000, 0xe8b313ec )
	ROM_LOAD( "astyan13.bin", 0x040000, 0x020000, 0x5f3496c6 )
	ROM_LOAD( "astyan14.bin", 0x060000, 0x020000, 0x29a09ec2 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "astyan15.bin", 0x000000, 0x020000, 0x0d316615 )
	ROM_LOAD( "astyan16.bin", 0x020000, 0x020000, 0xba96e8d9 )
	ROM_LOAD( "astyan17.bin", 0x040000, 0x020000, 0xbe60ba06 )
	ROM_LOAD( "astyan18.bin", 0x060000, 0x020000, 0x3668da3d )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "astyan19.bin", 0x000000, 0x020000, 0x98158623 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "astyan20.bin", 0x000000, 0x020000, 0xc1ad9aa0 )
	ROM_LOAD( "astyan21.bin", 0x020000, 0x020000, 0x0bf498ee )
	ROM_LOAD( "astyan22.bin", 0x040000, 0x020000, 0x5f04d9b1 )
	ROM_LOAD( "astyan23.bin", 0x060000, 0x020000, 0x7bd4d1e7 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "astyan9.bin",  0x000000, 0x020000, 0xa10b3f17 )
	ROM_LOAD( "astyan10.bin", 0x020000, 0x020000, 0x4f704e7a )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "astyan7.bin",  0x000000, 0x020000, 0x319418cc )
	ROM_LOAD( "astyan8.bin",  0x020000, 0x020000, 0x5e5d2a22 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom",         0x0000, 0x0200, 0x00000000 )
ROM_END


INPUT_PORTS_START( astyanax )
	COINS						/* IN0 0x80001.b */
/*	fire	jump	magic */
	JOY_3BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE						/* IN2 0x80004.b */
	JOY_3BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80006.b */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
/*	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )	// 1_2 shown in test mode */
/*	PORT_DIPSETTING(    0x05, DEF_STR( 1C_1C ) )	// 1_3 */
/*	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )	// 1_4 */
/*	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )	// 1_5 */
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
/*	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )	// 1_2 shown in test mode */
/*	PORT_DIPSETTING(    0x28, DEF_STR( 1C_1C ) )	// 1_3 */
/*	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )	// 1_4 */
/*	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )	// 1_5 */
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/* according to manual */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) /* according to manual */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) /* according to manual */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "30k 70k 110k then every 30k" )
	PORT_DIPSETTING(    0x00, "50k 100k then every 40k" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Swap 1P/2P Controls" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END



/***************************************************************************

					[ Avenging Spirit ] / [ Phantasm ]

(Avspirit)
interrupts:	2,3, 5,6,7]		move.w  $e0000.l, $78e9e.l
							andi.w  #$ff, $78e9e.l
			4] 78b20 software watchdog (78ea0 enables it)


fd6		reads e0000 (values FF,06,34,35,36,37)
ffa		e0000<-6 test

79584.w *** level ***

1] E9C
2] ED4
3] F4C		rte
4-7] ED2	rte

***************************************************************************/

ROM_START( avspirit )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )		/* Main CPU Code: 00000-3ffff & 80000-bffff */
	ROM_LOAD16_BYTE( "spirit05.rom",  0x000000, 0x020000, 0xb26a341a )
	ROM_CONTINUE (                    0x080000, 0x020000 )
	ROM_LOAD16_BYTE(  "spirit06.rom", 0x000001, 0x020000, 0x609f71fe )
	ROM_CONTINUE (                    0x080001, 0x020000 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "spirit01.rom",  0x000000, 0x020000, 0xd02ec045 )
	ROM_LOAD16_BYTE( "spirit02.rom",  0x000001, 0x020000, 0x30213390 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "spirit12.rom",  0x000000, 0x080000, 0x728335d4 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "spirit11.rom",  0x000000, 0x080000, 0x7896f6b0 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "spirit09.rom",  0x000000, 0x020000, 0x0c37edf7 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "spirit10.rom",  0x000000, 0x080000, 0x2b1180b3 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "spirit14.rom",  0x000000, 0x040000, 0x13be9979 )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "spirit13.rom",  0x000000, 0x040000, 0x05bc04d9 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "ph.bin",        0x0000, 0x0200, 0x8359650a )
ROM_END


ROM_START( phantasm )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "phntsm02.bin", 0x000000, 0x020000, 0xd96a3584 )
	ROM_LOAD16_BYTE( "phntsm01.bin", 0x000001, 0x020000, 0xa54b4b87 )
	ROM_LOAD16_BYTE( "phntsm03.bin", 0x040000, 0x010000, 0x1d96ce20 )
	ROM_LOAD16_BYTE( "phntsm04.bin", 0x040001, 0x010000, 0xdc0c4994 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "phntsm05.bin", 0x000000, 0x010000, 0x3b169b4a )
	ROM_LOAD16_BYTE( "phntsm06.bin", 0x000001, 0x010000, 0xdf2dfb2e )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
/*	ROM_LOAD( "phntsm14.bin",  0x000000, 0x080000, 0x728335d4 ) */
	ROM_LOAD( "spirit12.rom",  0x000000, 0x080000, 0x728335d4 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
/*	ROM_LOAD( "phntsm18.bin",  0x000000, 0x080000, 0x7896f6b0 ) */
	ROM_LOAD( "spirit11.rom",  0x000000, 0x080000, 0x7896f6b0 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
/*	ROM_LOAD( "phntsm19.bin",  0x000000, 0x020000, 0x0c37edf7 ) */
	ROM_LOAD( "spirit09.rom",  0x000000, 0x020000, 0x0c37edf7 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
/*	ROM_LOAD( "phntsm23.bin",  0x000000, 0x080000, 0x2b1180b3 ) */
	ROM_LOAD( "spirit10.rom",  0x000000, 0x080000, 0x2b1180b3 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* Samples */
/*	ROM_LOAD( "phntsm10.bin", 0x000000, 0x040000, 0x13be9979 ) */
	ROM_LOAD( "spirit14.rom", 0x000000, 0x040000, 0x13be9979 )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 )		/* Samples */
/*	ROM_LOAD( "phntsm08.bin", 0x000000, 0x040000, 0x05bc04d9 ) */
	ROM_LOAD( "spirit13.rom", 0x000000, 0x040000, 0x05bc04d9 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "ph.bin",        0x0000, 0x0200, 0x8359650a )
ROM_END


INPUT_PORTS_START( avspirit )
	COINS
	JOY_2BUTTONS(IPF_PLAYER1)
	RESERVE
	JOY_2BUTTONS(IPF_PLAYER2)

	PORT_START
	COINAGE_8BITS

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END



/***************************************************************************

							[ Big Striker ]

PCB: RB-91105A EB911009-20045

Note: RAM is ff0000-ffffff while sprites live in 1f8000-1f87ff

interrupts:	1]
			2]
			4]

$885c/e.w	*** time (BCD) ***

***************************************************************************/

ROM_START( bigstrik )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "91105v11.3", 0x000000, 0x020000, 0x5d6e08ec )
	ROM_LOAD16_BYTE( "91105v11.2", 0x000001, 0x020000, 0x2120f05b )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "91105v10.8", 0x000000, 0x010000, 0x7dd69ece )
	ROM_LOAD16_BYTE( "91105v10.7", 0x000001, 0x010000, 0xbc2c1508 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "91021.01",   0x000000, 0x080000, 0xf1945858 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "91021.03",   0x000000, 0x080000, 0xe88821e5 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "91105v11.9", 0x000000, 0x020000, 0x7be1c50c )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "91021.02",   0x000000, 0x080000, 0x199819ca )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "91105v10.11", 0x000000, 0x040000, BADCRC(0xa1f13dd5) )	/* 1xxxxxxxxxxxxxxxxx = 0xFF */

	ROM_REGION( 0x40000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "91105v10.10", 0x000000, 0x040000, BADCRC(0xe4f8fc8d) )	/* 1xxxxxxxxxxxxxxxxx = 0xFF */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom",         0x0000, 0x0200, 0x00000000 )
ROM_END


INPUT_PORTS_START( bigstrik )
	COINS
/*	pass	shoot	feint */
	JOY_3BUTTONS(IPF_PLAYER1)
	RESERVE
	JOY_3BUTTONS(IPF_PLAYER2)

	PORT_START			/* IN4 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_3C ) )
/*	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) ) */
/*	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) ) */
/*	PORT_DIPSETTING(    0x02, DEF_STR( 2C_3C ) ) */
/*	PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ) ) */
/*	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) ) */
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_3C ) )
/*	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) ) */
/*	PORT_DIPSETTING(    0x30, DEF_STR( 2C_3C ) ) */
/*	PORT_DIPSETTING(    0x20, DEF_STR( 2C_3C ) ) */
/*	PORT_DIPSETTING(    0x10, DEF_STR( 2C_3C ) ) */
/*	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) ) */
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START			/* IN5 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy"    )
	PORT_DIPSETTING(    0x06, "Normal"  )
	PORT_DIPSETTING(    0x04, "Hard"    )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x18, 0x18, "Time" )
	PORT_DIPSETTING(    0x00, "Very Short" )
	PORT_DIPSETTING(    0x10, "Short" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Long" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "1 Credit 2 Play" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

INPUT_PORTS_END


/***************************************************************************

							[ Chimera Beast ]

interrupts:	1,3]
			2, 5,6]
			4]

Note: This game was a prototype

***************************************************************************/

ROM_START( chimerab )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "prg3.bin", 0x000000, 0x040000, 0x70f1448f )
	ROM_LOAD16_BYTE( "prg2.bin", 0x000001, 0x040000, 0x821dbb85 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "prg8.bin", 0x000000, 0x010000, 0xa682b1ca )
	ROM_LOAD16_BYTE( "prg7.bin", 0x000001, 0x010000, 0x83b9982d )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "s1.bin",   0x000000, 0x080000, 0xe4c2ac77 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "s2.bin",   0x000000, 0x080000, 0xfafb37a5 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "scr3.bin", 0x000000, 0x020000, 0x5fe38a83 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "b2.bin",   0x000000, 0x080000, 0x6e7f1778 )
	ROM_LOAD( "b1.bin",   0x080000, 0x080000, 0x29c0385e )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "voi11.bin", 0x000000, 0x040000, 0x14b3afe6 )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "voi10.bin", 0x000000, 0x040000, 0x67498914 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom",         0x0000, 0x0200, 0x00000000 )
ROM_END

INPUT_PORTS_START( chimerab )

	COINS
/*	fire	jump	unused?(shown in service mode, but not in instructions) */
	JOY_2BUTTONS(IPF_PLAYER1)
	RESERVE				/* Unused */
	JOY_2BUTTONS(IPF_PLAYER2)

	PORT_START			/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x60, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START			/* DSW B */
	COINAGE_8BITS

INPUT_PORTS_END



/***************************************************************************

								[ Cybattler ]

interrupts:	1,3]	408
			2, 5,6]	498
					1fd2c2.w routine index:
					0:	4be>	1fd2c0.w <- d8000
					2:	4ca>	1fd2d0+(1fd2c4.w) <- d8000.	next
					4:	4ee>	1fd2c4.w += 2.
											S	P1	P2	DB	DA
								d8000 <-	56	52	53	55	54
								1fd000+ 	00	02	04	06	08
								depending on 1fd2c4.		previous
					6:	4be again

			4]		452

c2208 <- 1fd040	(layers enable)
c2200 <- 1fd042	(sprite control)
c2308 <- 1fd046	(screen control)
c2004 <- 1fd054 (scroll 0 ctrl)	c2000 <- 1fd220 (scroll 0 x)	c2002 <- 1fd222 (scroll 1 y)
c200c <- 1fd05a (scroll 1 ctrl)	c2008 <- 1fd224 (scroll 1 x)	c200a <- 1fd226 (scroll 2 y)
c2104 <- 1fd060 (scroll 2 ctrl)	c2100 <- 1fd228 (scroll 2 x)	c2102 <- 1fd22a (scroll 3 y)

1f0010.w	*** level (0,1,..) ***
1fb044.l	*** score / 10 ***

***************************************************************************/

ROM_START( cybattlr )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "cb_03.rom", 0x000000, 0x040000, 0xbee20587 )
	ROM_LOAD16_BYTE( "cb_02.rom", 0x000001, 0x040000, 0x2ed14c50 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "cb_08.rom", 0x000000, 0x010000, 0xbf7b3558 )
	ROM_LOAD16_BYTE( "cb_07.rom", 0x000001, 0x010000, 0x85d219d7 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "cb_m01.rom", 0x000000, 0x080000, 0x1109337f )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "cb_m04.rom", 0x000000, 0x080000, 0x0c91798e )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "cb_09.rom",  0x000000, 0x020000, 0x37b1f195 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "cb_m03.rom", 0x000000, 0x080000, 0x4cd49f58 )
	ROM_LOAD( "cb_m02.rom", 0x080000, 0x080000, 0x882825db )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "cb_11.rom", 0x000000, 0x040000, 0x59d62d1f )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "cb_10.rom", 0x000000, 0x040000, 0x8af95eed )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "pr-91028.12",  0x0000, 0x0200, 0xcfe90082 )
ROM_END

INPUT_PORTS_START( cybattlr )

	COINS
/*	fire	sword */
	JOY_2BUTTONS(IPF_PLAYER1)
	RESERVE				/* Unused */
	JOY_2BUTTONS(IPF_PLAYER2)

	PORT_START			/* IN - DSW 1 - 1fd2d9.b, !1fd009.b */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START			/* IN - DSW 2 - 1fd2d7.b, !1fd007.b */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )	/* 1 bit */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )	/* 1 bit */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END




/***************************************************************************

						 [ Earth Defense Force ]

interrupts:	2,3]	543C>	move.w  $e0000.l,	$60da6.l
							move.w  #$ffff,		$60da8.l
			4,5,6]	5928 +	move.w	#$ffff,		$60010.l

89e			(a7)+ -> 44000.w & 6000e.w
8cc			(a7)+ -> 44204.w ; 4420c.w ; 4400c.w
fc0			(a7)+ -> 58000 (string)

616f4.w		*** lives ***
60d8a.w		*** level(1..) ***

***************************************************************************/

ROM_START( edf )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )		/* Main CPU Code: 00000-3ffff & 80000-bffff */
	ROM_LOAD16_BYTE( "edf_05.rom",  0x000000, 0x020000, 0x105094d1 )
	ROM_CONTINUE (                  0x080000, 0x020000 )
	ROM_LOAD16_BYTE( "edf_06.rom",  0x000001, 0x020000, 0x94da2f0c )
	ROM_CONTINUE (                  0x080001, 0x020000 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "edf_01.rom",  0x000000, 0x020000, 0x2290ea19 )
	ROM_LOAD16_BYTE( "edf_02.rom",  0x000001, 0x020000, 0xce93643e )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "edf_m04.rom",  0x000000, 0x080000, 0x6744f406 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "edf_m05.rom",  0x000000, 0x080000, 0x6f47e456 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "edf_09.rom",   0x000000, 0x020000, 0x96e38983 )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "edf_m03.rom",  0x000000, 0x080000, 0xef469449 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "edf_m02.rom",  0x000000, 0x040000, 0xfc4281d2 )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "edf_m01.rom",  0x000000, 0x040000, 0x9149286b )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom.14m",    0x0000, 0x0200, 0x1d877538 )
ROM_END

INPUT_PORTS_START( edf )
	COINS
/*	fire	unfold_weapons */
	JOY_2BUTTONS(IPF_PLAYER1)
	RESERVE
	JOY_2BUTTONS(IPF_PLAYER2)

	PORT_START			/* IN4 */
	COINAGE_6BITS
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START			/* IN5 0x66007.b */
	PORT_DIPNAME( 0x07, 0x07, "DSW-B bits 2-0" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPNAME( 0x40, 0x40, "DSW-B bit 6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END



/***************************************************************************

								[ Hachoo! ]

***************************************************************************/


ROM_START( hachoo )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "hacho02.rom", 0x000000, 0x020000, 0x49489c27 )
	ROM_LOAD16_BYTE( "hacho01.rom", 0x000001, 0x020000, 0x97fc9515 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "hacho05.rom", 0x000000, 0x010000, 0x6271f74f )
	ROM_LOAD16_BYTE( "hacho06.rom", 0x000001, 0x010000, 0xdb9e743c )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "hacho14.rom", 0x000000, 0x080000, 0x10188483 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "hacho15.rom", 0x000000, 0x020000, 0xe559347e )
	ROM_LOAD( "hacho16.rom", 0x020000, 0x020000, 0x105fd8b5 )
	ROM_LOAD( "hacho17.rom", 0x040000, 0x020000, 0x77f46174 )
	ROM_LOAD( "hacho18.rom", 0x060000, 0x020000, 0x0be21111 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "hacho19.rom", 0x000000, 0x020000, 0x33bc9de3 )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "hacho20.rom", 0x000000, 0x020000, 0x2ae2011e )
	ROM_LOAD( "hacho21.rom", 0x020000, 0x020000, 0x6dcfb8d5 )
	ROM_LOAD( "hacho22.rom", 0x040000, 0x020000, 0xccabf0e0 )
	ROM_LOAD( "hacho23.rom", 0x060000, 0x020000, 0xff5f77aa )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "hacho09.rom", 0x000000, 0x020000, 0xe9f35c90 )
	ROM_LOAD( "hacho10.rom", 0x020000, 0x020000, 0x1aeaa188 )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "hacho07.rom", 0x000000, 0x020000, 0x06e6ca7f )
	ROM_LOAD( "hacho08.rom", 0x020000, 0x020000, 0x888a6df1 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom",         0x0000, 0x0200, 0x00000000 )
ROM_END


INPUT_PORTS_START( hachoo )
	COINS						/* IN0 0x80001.b */
/*	fire	jump */
	JOY_2BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE						/* IN2 0x80004.b */
	JOY_2BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80006.b */
	COINAGE_6BITS
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-0" )	/* unused? */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )	/* unused? */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )	/* unused? */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )	/* unused? */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )	/* unused? */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "?Difficulty?" )	/* 4 & 5 */
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )	/* unused? */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END



/***************************************************************************

							[ Iga Ninjyutsuden ]

interrupts:	1] 420(does nothing)
			2] 500
			3] 410(it doesn't save registers on the stack!!)

f0004.l		*** hi score (BCD) ***
f000c.l		*** score (BCD) ***
f002a.w		*** lives ***
f010c.w		credits

***************************************************************************/


ROM_START( iganinju )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "iga_02.bin", 0x000000, 0x020000, 0xbd00c280 )
	ROM_LOAD16_BYTE( "iga_01.bin", 0x000001, 0x020000, 0xfa416a9e )
	ROM_LOAD16_BYTE( "iga_03.bin", 0x040000, 0x010000, 0xde5937ad )
	ROM_LOAD16_BYTE( "iga_04.bin", 0x040001, 0x010000, 0xafaf0480 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "iga_05.bin", 0x000000, 0x010000, 0x13580868 )
	ROM_LOAD16_BYTE( "iga_06.bin", 0x000001, 0x010000, 0x7904d5dd )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "iga_14.bin", 0x000000, 0x040000, 0xc707d513 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "iga_18.bin", 0x000000, 0x080000, 0x6c727519 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "iga_19.bin", 0x000000, 0x020000, 0x98a7e998 )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "iga_23.bin", 0x000000, 0x080000, 0xfb58c5f4 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "iga_10.bin", 0x000000, 0x040000, 0x67a89e0d )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "iga_08.bin", 0x000000, 0x040000, 0x857dbf60 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom",         0x0000, 0x0200, 0x00000000 )
ROM_END


INPUT_PORTS_START( iganinju )

	COINS						/* IN0 0x80001.b */
/*	fire	jump */
	JOY_2BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE						/* IN2 0x80004.b */
	JOY_2BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START 			/* IN4 0x80006.b */
	COINAGE_6BITS
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Freeze Screen", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* IN5 - 0x80007.b */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Inifinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "50k" )
	PORT_DIPSETTING(    0x00, "200k" )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "?Difficulty?" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END



/***************************************************************************

						[ Jitsuryoku!! Pro Yakyuu ]

(JPN Ver.)
(c)1989 Jaleco
Mega-System
MB-8842
A-Type
CPU  :TMP68000P-8 x2
Sound:YM2151,YM3012
OSC  :12.000MHz,7.000MHz

Sub
MB-M02A (EB-88003-3001-1)
Sound:OKI M6295
OSC  :4.000MHz
Other:JALECO GS-88000

BS.BPR       [85b30ac4] (82S131)

***************************************************************************/

ROM_START( jitsupro )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "jp_2.bin", 0x000000, 0x020000, 0x5d842ff2 )
	ROM_LOAD16_BYTE( "jp_1.bin", 0x000001, 0x020000, 0x0056edec )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "jp_5.bin", 0x000000, 0x010000, 0x84454e9e )	/* 11xxxxxxxxxxxxxx = 0xFF */
	ROM_LOAD16_BYTE( "jp_6.bin", 0x000001, 0x010000, 0x1fa9b75b )	/* 11xxxxxxxxxxxxxx = 0xFF */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "jp_14.bin", 0x000000, 0x080000, 0xdb112abf )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "jp_18.bin", 0x000000, 0x080000, 0x3ed855e3 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "jp_19.bin", 0x000000, 0x020000, 0xff59111f )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "jp_23.bin", 0x000000, 0x080000, 0x275f48bd )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "jp_10.bin", 0x000000, 0x040000, 0x178e43c0 )	/* FIRST AND SECOND HALF IDENTICAL */
	ROM_CONTINUE(          0x000000, 0x040000             )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "jp_8.bin",  0x000000, 0x040000, 0xeca67632 )	/* FIRST AND SECOND HALF IDENTICAL */
	ROM_CONTINUE(          0x000000, 0x040000             )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "bs.bpr",    0x0000, 0x0200, 0x85b30ac4 )
ROM_END


INPUT_PORTS_START( jitsupro )

	COINS						/* IN0 0x80001.b */
	/*	shoot	change view		change bat */
	JOY_3BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE						/* IN2 0x80004.b */
	JOY_3BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80006.b */
	COINAGE_6BITS
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0*" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1*" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )	/* $200-140 */
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )	/* $400-140 */
	PORT_DIPNAME( 0x3c, 0x3c, "Unknown 2-2345*" )
	PORT_DIPSETTING(    0x3c, "0"  )
	PORT_DIPSETTING(    0x38, "1"  )
	PORT_DIPSETTING(    0x34, "2"  )
	PORT_DIPSETTING(    0x30, "3"  )
	PORT_DIPSETTING(    0x2c, "4"  )
	PORT_DIPSETTING(    0x28, "5"  )
	PORT_DIPSETTING(    0x24, "6"  )
	PORT_DIPSETTING(    0x20, "7"  )
	PORT_DIPSETTING(    0x1c, "8"  )
	PORT_DIPSETTING(    0x18, "9"  )
	PORT_DIPSETTING(    0x14, "10" )
	PORT_DIPSETTING(    0x10, "11" )
	PORT_DIPSETTING(    0x0c, "12" )
	PORT_DIPSETTING(    0x08, "13" )
	PORT_DIPSETTING(    0x04, "14" )
	PORT_DIPSETTING(    0x00, "15" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6*" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END



/***************************************************************************

							[ Kick Off ]

WARNING: The sound CPU writes and read in the 9000-ffff area

interrupts:	1-2]	rte
			3]		timer
			4-7]	loop forever

f0128/a.w	*** Time (minutes/seconds BCD) ***
f012c/e.w	*** Goals (P1/P2) ***

Notes:
	* Coin B and Test are ignored
	* The alternate control method (selectable through a DSW)
	  isn't implemented: the program tests the low 4 bits of
	  the joystick inputs ($80002, $80004) but not the buttons.
	  I can't get the players to move
	* Pressing P1 or P2 Start while the game boots pops up
	  a rudimental sprites or tiles browser

***************************************************************************/

ROM_START( kickoff )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "kioff03.rom", 0x000000, 0x010000, 0x3b01be65 )
	ROM_LOAD16_BYTE( "kioff01.rom", 0x000001, 0x010000, 0xae6e68a1 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "kioff09.rom", 0x000000, 0x010000, 0x1770e980 )
	ROM_LOAD16_BYTE( "kioff19.rom", 0x000001, 0x010000, 0x1b03bbe4 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "kioff05.rom", 0x000000, 0x020000, 0xe7232103 )
	ROM_LOAD( "kioff06.rom", 0x020000, 0x020000, 0xa0b3cb75 )
	ROM_LOAD( "kioff07.rom", 0x040000, 0x020000, 0xed649919 )
	ROM_LOAD( "kioff10.rom", 0x060000, 0x020000, 0xfd739fec )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	/* scroll 1 is unused */

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "kioff16.rom", 0x000000, 0x020000, 0x22c46314 )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "kioff27.rom", 0x000000, 0x020000, 0xca221ae2 )
	ROM_LOAD( "kioff18.rom", 0x020000, 0x020000, 0xd7909ada )
	ROM_LOAD( "kioff17.rom", 0x040000, 0x020000, 0xf171559e )
	ROM_LOAD( "kioff26.rom", 0x060000, 0x020000, 0x2a90df1b )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "kioff20.rom", 0x000000, 0x020000, 0x5c28bd2d )
	ROM_LOAD( "kioff21.rom", 0x020000, 0x020000, 0x195940cf )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	/* same rom for 2 oki chips ?? Unlikely */
	ROM_LOAD( "kioff20.rom", 0x000000, 0x020000, 0x5c28bd2d )
	ROM_LOAD( "kioff21.rom", 0x020000, 0x020000, 0x195940cf )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom",    0x0000, 0x0200, 0x00000000 )
ROM_END

INPUT_PORTS_START( kickoff )
	COINS						/* IN0 0x80001.b ->  !f0008/a.w  */
/*	shoot	pass */
	JOY_2BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b ->  !f000c/e.w  */
	RESERVE						/* IN2 0x80004.b --> !f0010/11.w */
	JOY_2BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b /               */

	PORT_START			/* IN4 0x80006.b */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )	/* unused? */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )	/* unused? */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Freeze Screen", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Text" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )	/* show "Japan Only" warning */

	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x03, 0x03, "Time" )	/* -> !f0082.w */
	PORT_DIPSETTING(    0x03, "3'" )
	PORT_DIPSETTING(    0x02, "4'" )
	PORT_DIPSETTING(    0x01, "5'" )
	PORT_DIPSETTING(    0x00, "6'" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )	/* unused? */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )	/* unused? */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "?Difficulty?" )	/* -> !f0084.w */
	PORT_DIPSETTING(    0x30, "0" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x40, 0x00, "Controls" )
	PORT_DIPSETTING(    0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "Joystick" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************

							[ Legend of Makai ]

***************************************************************************/

ROM_START( lomakai )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "lom_30.rom", 0x000000, 0x020000, 0xba6d65b8 )
	ROM_LOAD16_BYTE( "lom_20.rom", 0x000001, 0x020000, 0x56a00dc2 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Sound CPU Code (Z80) */
	ROM_LOAD( "lom_01.rom",  0x0000, 0x10000, 0x46e85e90 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "lom_05.rom", 0x000000, 0x020000, 0xd04fc713 )

	ROM_REGION( 0x010000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "lom_08.rom", 0x000000, 0x010000, 0xbdb15e67 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "lom_06.rom", 0x000000, 0x020000, 0xf33b6eed )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Unknown PROMs */
	ROM_LOAD( "makaiden.9",  0x0000, 0x0100, 0x3567065d )
	ROM_LOAD( "makaiden.10", 0x0100, 0x0100, 0xe6709c51 )
ROM_END

ROM_START( makaiden )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "makaiden.3a", 0x000000, 0x020000, 0x87cf81d1 )
	ROM_LOAD16_BYTE( "makaiden.2a", 0x000001, 0x020000, 0xd40e0fea )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Sound CPU Code (Z80) */
	ROM_LOAD( "lom_01.rom",  0x0000, 0x10000, 0x46e85e90 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "lom_05.rom", 0x000000, 0x020000, 0xd04fc713 )

	ROM_REGION( 0x010000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "makaiden.8", 0x000000, 0x010000, 0xa7f623f9 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "lom_06.rom", 0x000000, 0x020000, 0xf33b6eed )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Unknown PROMs */
	ROM_LOAD( "makaiden.9",  0x0000, 0x0100, 0x3567065d )
	ROM_LOAD( "makaiden.10", 0x0100, 0x0100, 0xe6709c51 )
ROM_END

INPUT_PORTS_START( lomakai )
	COINS						/* IN0 0x80001.b */
/*	fire	jump */
	JOY_2BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE						/* IN2 0x80004.b */
	JOY_2BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80006.b */
	COINAGE_6BITS_2
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "?Difficulty?" )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x10, "Hardest" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************

							 [ P - 47 ]

(Japan version)
interrupts:	1]	53e		2] 540

517a		print word string: (a6)+,(a5)+$40. FFFF ends
5dbc		print string(s) to (a1)+$40: a6-> len.b,x.b,y.b,(chars.b)*
726a		prints screen
7300		ram test
7558		ip test
75e6(7638 loop)	sound test
	84300.w		<-f1002.w	?portrait F/F on(0x0100)/off(0x0000)
	84308.w		<-f1004.w	sound code

7736(7eb4 loop)	scroll 0 test
	9809c		color
	980a0		hscroll
	980a4		vscroll
	980a8		charsize

	7e1e		prepare screen
	7e84		get user input
	7faa		vhscroll
	80ce		print value.l from a0

785c(78b8 loop)	obj check 1		84000.w	<-0x0E	84100.w	<-0x101
	9804c	size
	98050	number		(0e.w bit 11-0)
	98054	color code	(08.w bit 2-0)
	98058	H flip		(08.w bit 6)
	9805c	V flip		(08.w bit 7)
	98060	priority	(08.w bit 3)
	98064	mosaic		(08.w bit 11-8)
	98068	mosaic sol.	(08.w bit 12)

7afe(7cfe loop)	obj check 2		84000.w	<-0x0f	84100.w	<-0x00
	9804a	obj num	(a4-8e000)/8
	9804e	H-rev	a4+02.w
	98052	V-rev	a4+04.w
	98056	CG-rev	a4+06.w
	9805a	Rem.Eff bit   4 of 84100
	98060	Rem.Num bit 3-0 of 84100 (see 7dd4)

TRAP#2		pause?
f0104.w		*** initial lives ***
f002a/116.w	<-!80000
f0810.w		<-!80002
f0c00.w		<-!80004
f0018.w		*** level ***


***************************************************************************/

ROM_START( p47 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "p47us3.bin", 0x000000, 0x020000, 0x022e58b8 )
	ROM_LOAD16_BYTE( "p47us1.bin", 0x000001, 0x020000, 0xed926bd8 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "p47j_9.bin",  0x000000, 0x010000, 0xffcf318e )
	ROM_LOAD16_BYTE( "p47j_19.bin", 0x000001, 0x010000, 0xadb8c12e )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "p47j_5.bin",  0x000000, 0x020000, 0xfe65b65c )
	ROM_LOAD( "p47j_6.bin",  0x020000, 0x020000, 0xe191d2d2 )
	ROM_LOAD( "p47j_7.bin",  0x040000, 0x020000, 0xf77723b7 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "p47j_23.bin", 0x000000, 0x020000, 0x6e9bc864 )
	ROM_RELOAD(              0x020000, 0x020000 )	/* why? */
	ROM_LOAD( "p47j_12.bin", 0x040000, 0x020000, 0x5268395f )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "p47us16.bin", 0x000000, 0x010000, 0x5a682c8f )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "p47j_27.bin", 0x000000, 0x020000, 0x9e2bde8e )
	ROM_LOAD( "p47j_18.bin", 0x020000, 0x020000, 0x29d8f676 )
	ROM_LOAD( "p47j_26.bin", 0x040000, 0x020000, 0x4d07581a )
	ROM_RELOAD(              0x060000, 0x020000 )	/* why? */

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "p47j_20.bin", 0x000000, 0x020000, 0x2ed53624 )
	ROM_LOAD( "p47j_21.bin", 0x020000, 0x020000, 0x6f56b56d )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "p47j_10.bin", 0x000000, 0x020000, 0xb9d79c1e )
	ROM_LOAD( "p47j_11.bin", 0x020000, 0x020000, 0xfa0d1887 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom.14m",    0x0000, 0x0200, 0x1d877538 )
ROM_END


ROM_START( p47j )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "p47j_3.bin", 0x000000, 0x020000, 0x11c655e5 )
	ROM_LOAD16_BYTE( "p47j_1.bin", 0x000001, 0x020000, 0x0a5998de )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "p47j_9.bin",  0x000000, 0x010000, 0xffcf318e )
	ROM_LOAD16_BYTE( "p47j_19.bin", 0x000001, 0x010000, 0xadb8c12e )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "p47j_5.bin",  0x000000, 0x020000, 0xfe65b65c )
	ROM_LOAD( "p47j_6.bin",  0x020000, 0x020000, 0xe191d2d2 )
	ROM_LOAD( "p47j_7.bin",  0x040000, 0x020000, 0xf77723b7 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "p47j_23.bin", 0x000000, 0x020000, 0x6e9bc864 )
	ROM_RELOAD(              0x020000, 0x020000 )	/* why? */
	ROM_LOAD( "p47j_12.bin", 0x040000, 0x020000, 0x5268395f )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "p47j_16.bin", 0x000000, 0x010000, 0x30e44375 )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "p47j_27.bin", 0x000000, 0x020000, 0x9e2bde8e )
	ROM_LOAD( "p47j_18.bin", 0x020000, 0x020000, 0x29d8f676 )
	ROM_LOAD( "p47j_26.bin", 0x040000, 0x020000, 0x4d07581a )
	ROM_RELOAD(              0x060000, 0x020000 )	/* why? */

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "p47j_20.bin", 0x000000, 0x020000, 0x2ed53624 )
	ROM_LOAD( "p47j_21.bin", 0x020000, 0x020000, 0x6f56b56d )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "p47j_10.bin", 0x000000, 0x020000, 0xb9d79c1e )
	ROM_LOAD( "p47j_11.bin", 0x020000, 0x020000, 0xfa0d1887 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom.14m",    0x0000, 0x0200, 0x1d877538 )
ROM_END

INPUT_PORTS_START( p47 )

	COINS						/* IN0 0x80001.b */
/*	fire	bomb */
	JOY_2BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE						/* IN2 0x80004.b */
	JOY_2BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80006.b */
	COINAGE_6BITS_2
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x10, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END




/***************************************************************************

							[ Peek-a-Boo! ]

interrupts:
	1] 		506>	rte
	2] 		50a>	move.w  #$ffff, $1f0006.l
					jsr     $46e0.l				rte
	3] 		51c>	rte
	4] 		520>	move.w  #$ffff, $1f000a.l	rte
	5-7]	53c>	rte

3832	Show error (d7 = ram segment where error occurred)
		1 after d8000 ok. 3 after e0000&d0000 ok. 4 after ram&rom ok

003E5E: 0000 3E72	[0]	Color Ram
003E62: 0000 3E86	[1]	Video Ram
003E66: 0000 3E9A	[2]	Sprite Ram
003E6A: 0000 3EB0	[3]	Work Ram
003E6E: 0000 3EC4	[4]	ROM

000000-03ffff	rom (3f760 chksum)
1f0000-1fffff	ram
0d0000-0d3fff	text
0d8000-0d87ff	palette (+200 = text palette)
0e8000-0ebfff	layer
0e0000-0e0001	2 dips, 1f003a<-!
0f0000-0f0001	2 controls
0f8000-0f8001	???

010000-010001	protection\watchdog;
	fb -> fb
	9x ->	0		watchdog reset?
			else	samples bank?
					$1ff010 = sample
					$1ff014 = bank = sample - $22 (33DC: 1 1 2 3 4 5 6 6 6 6)
						samples:	bank:
						$00-21		0
						$22-2b		1-6
000000-01ffff
020000-03ffff	banked

	51 -> paddle p1
	52 -> paddle p2
	4bba waits for 1f000a to go !0, then clears 1f000a (int 4)
	4bca waits (100000) & FF == 3
	sequence $81, $71, $67 written


Scroll x,y,ctrl:
c2000<-1f0010		c2002<-1f0014		c2004<-1f000c

Scroll x,y,ctrl:
c2008<-1f0018		c200a<-1f001c		c200c<-1f000e

Layers ctrl:
c2208<-1f0024<<8 + 1f0026		c2308<-1f0022 | 1f002c

Sprite bank + ??
c2108<-1f005a + 1f0060 + 1f0062 + 1f0068

Sprite ctrl:
c2200<-0

1f0000.w	routine index, table at $fae:
	0: 4E40
	1: 4EC2
	2: 4F2C
	3: 4F70
	4: 4FBC
	5: 533A
	6: 5382
	7: 556E

1f003c/40	paddle p1/p2
1f0260/4.l	*** p1/p2 score/10 (BCD) ***
1f02e6/8.w	*** p1/p2 current lives ***
			Bonus lives:	20K  100K  250K  500K 1000K
1f02ee		current player (0/1)
1f0380		hi score


***************************************************************************/

ROM_START( peekaboo )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 CPU Code */
	ROM_LOAD16_BYTE( "j3", 0x000000, 0x020000, 0xf5f4cf33 )
	ROM_LOAD16_BYTE( "j2", 0x000001, 0x020000, 0x7b3d430d )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "5",       0x000000, 0x080000, 0x34fa07bb )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "4",       0x000000, 0x020000, 0xf037794b )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	/* Unused */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "1",       0x000000, 0x080000, 0x5a444ecf )

	ROM_REGION( 0x120000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "peeksamp.124", 0x000000, 0x020000, 0xe1206fa8 )
	ROM_CONTINUE(             0x040000, 0x0e0000 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "priority.69",    0x000000, 0x200, 0xb40bff56 )
ROM_END

INPUT_PORTS_START( peekaboo )

	PORT_START		/* IN0 - COINS + P1&P2 Buttons - .b */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN3 )		/* called "service" */
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN4 )		/* called "test" */
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 )		/* called "stage clear" */
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON4 )		/* called "option" */
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define PEEKABOO_PADDLE(_FLAG_)	\
	PORT_ANALOG( 0x00ff, 0x0080, IPT_PADDLE | _FLAG_, 50, 10, 0x0018, 0x00e0 )

	PORT_START      	/* IN1 - paddle p1 */
	PEEKABOO_PADDLE(IPF_PLAYER1)

	RESERVE				/* IN2 - fake port */
	PORT_START      	/* IN3 - paddle p2 */
	PEEKABOO_PADDLE(/*IPF_PLAYER2*/ IPF_COCKTAIL)

	PORT_START			/* IN4 - DSW 1 - 1f003a.b<-e0000.b */
	COINAGE_6BITS_2
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )		/* 1f0354<- */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )		/* 1f0022/6e<-! */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* IN5 - DSW 2 - 1f003b.b<-e0001.b */
	PORT_DIPNAME( 0x03, 0x03, "Unknown 2-0&1" )				/* 1f0358<-! */
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Movement?" )					/* 1f0392<-! */
	PORT_DIPSETTING(    0x08, "Paddles" )
	PORT_DIPSETTING(    0x00, "Buttons" )
	PORT_DIPNAME( 0x30, 0x30, "Nudity" )					/* 1f0356<-! */
	PORT_DIPSETTING(    0x30, "Female and Male (Full)" )
	PORT_DIPSETTING(    0x20, "Female (Full)" )
	PORT_DIPSETTING(    0x10, "Female (Partial)" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )			/* 1f006a<-! */
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "(controls?)Unknown 2-7" )	/* 1f0074<-! */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )	/* num of controls? */
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END



data16_t protection_val;

/* Read the input ports, through a protection device */
static READ16_HANDLER( protection_peekaboo_r )
{
	switch (protection_val)
	{
		case 0x02:	return 0x03;
		case 0x51:	return player1_r(0,0);
		case 0x52:	return player2_r(0,0);
		default:	return protection_val;
	}
}

static WRITE16_HANDLER( protection_peekaboo_w )
{
	static int bank;

	COMBINE_DATA(&protection_val);

	if ((protection_val & 0x90) == 0x90)
	{
		unsigned char *RAM = memory_region(okim6295_interface_D.region[0]);
		int new_bank = (protection_val & 0x7) % 7;

		if (bank != new_bank)
		{
			memcpy(&RAM[0x20000],&RAM[0x40000 + 0x20000*new_bank],0x20000);
			bank = new_bank;
		}
	}

	cpu_cause_interrupt(0,4);
}


/***************************************************************************

							[ Plus Alpha ]
						  (aka Flight Alpha)

f2ef8.w		bombs
f309e.w		*** lives       ***
f30a4.l		*** score (BCD) ***

***************************************************************************/

ROM_START( plusalph )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "pa-rom2.bin", 0x000000, 0x020000, 0x33244799 )
	ROM_LOAD16_BYTE( "pa-rom1.bin", 0x000001, 0x020000, 0xa32fdcae )
	ROM_LOAD16_BYTE( "pa-rom3.bin", 0x040000, 0x010000, 0x1b739835 )
	ROM_LOAD16_BYTE( "pa-rom4.bin", 0x040001, 0x010000, 0xff760e80 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "pa-rom5.bin", 0x000000, 0x010000, 0xddc2739b )
	ROM_LOAD16_BYTE( "pa-rom6.bin", 0x000001, 0x010000, 0xf6f8a167 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "pa-rom11.bin", 0x000000, 0x020000, 0xeb709ae7 )
	ROM_LOAD( "pa-rom12.bin", 0x020000, 0x020000, 0xcacbc350 )
	ROM_LOAD( "pa-rom13.bin", 0x040000, 0x020000, 0xfad093dd )
	ROM_LOAD( "pa-rom14.bin", 0x060000, 0x020000, 0xd3676cd1 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "pa-rom15.bin", 0x000000, 0x020000, 0x8787735b )
	ROM_LOAD( "pa-rom16.bin", 0x020000, 0x020000, 0xa06b813b )
	ROM_LOAD( "pa-rom17.bin", 0x040000, 0x020000, 0xc6b38a4b )
	/* empty place */

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "pa-rom19.bin", 0x000000, 0x010000, 0x39ef193c )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "pa-rom20.bin", 0x000000, 0x020000, 0x86c557a8 )
	ROM_LOAD( "pa-rom21.bin", 0x020000, 0x020000, 0x81140a88 )
	ROM_LOAD( "pa-rom22.bin", 0x040000, 0x020000, 0x97e39886 )
	ROM_LOAD( "pa-rom23.bin", 0x060000, 0x020000, 0x0383fb65 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "pa-rom9.bin",  0x000000, 0x020000, 0x065364bd )
	ROM_LOAD( "pa-rom10.bin", 0x020000, 0x020000, 0x395df3b2 )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "pa-rom7.bin",  0x000000, 0x020000, 0x9f5d800e )
	ROM_LOAD( "pa-rom8.bin",  0x020000, 0x020000, 0xae007750 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom.14m",     0x0000, 0x0200, 0x1d877538 )
ROM_END

INPUT_PORTS_START( plusalph )
	COINS						/* IN0 0x80001.b */
/*	fire	bomb */
	JOY_2BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE						/* IN2 0x80004.b */
	JOY_2BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80006.b */
	COINAGE_6BITS
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Freeze Screen", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Inifinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x04, 0x04, "Bombs" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "70k and every 130k")
	PORT_DIPSETTING(    0x00, "100k and every 200k")
	PORT_DIPNAME( 0x30, 0x30, "?Difficulty?" )
	PORT_DIPSETTING(    0x30, "3" )	/* 1 */
	PORT_DIPSETTING(    0x20, "2" )	/* 3 */
	PORT_DIPSETTING(    0x10, "1" )	/* 2 */
	PORT_DIPSETTING(    0x00, "0" )	/* 0 */
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************

							[ RodLand ]

(World version)
interrupts:	1] 418->3864: rts	2] 420: move.w #-1,f0010; jsr 3866	3] rte

213da	print test error (20c12 = string address 0-4)

f0018->84200	f0020->84208	f0028->84008
f001c->84202	f0024->8420a	f002c->8400a
f0012->84204	f0014->8420c	f0016->8400c

7fe		d0.w -> 84000.w & f000e.w
81a		d0/d1/d2 & $D -> 84204 / 8420c /8400c

***************************************************************************/

ROM_START( rodland )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "rl_02.rom", 0x000000, 0x020000, 0xc7e00593 )
	ROM_LOAD16_BYTE( "rl_01.rom", 0x000001, 0x020000, 0x2e748ca1 )
	ROM_LOAD16_BYTE( "rl_03.rom", 0x040000, 0x010000, 0x62fdf6d7 )
	ROM_LOAD16_BYTE( "rl_04.rom", 0x040001, 0x010000, 0x44163c86 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "rl_05.rom", 0x000000, 0x010000, 0xc1617c28 )
	ROM_LOAD16_BYTE( "rl_06.rom", 0x000001, 0x010000, 0x663392b2 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "rl_23.rom", 0x000000, 0x020000, 0xac60e771 )
	ROM_CONTINUE(          0x030000, 0x010000 )
	ROM_CONTINUE(          0x050000, 0x010000 )
	ROM_CONTINUE(          0x020000, 0x010000 )
	ROM_CONTINUE(          0x040000, 0x010000 )
	ROM_CONTINUE(          0x060000, 0x020000 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "rl_18.rom", 0x000000, 0x080000, 0xf3b30ca6 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "rl_19.bin", 0x000000, 0x020000, 0x124d7e8f )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "rl_14.rom", 0x000000, 0x080000, 0x08d01bf4 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "rl_10.rom", 0x000000, 0x040000, 0xe1d1cd99 )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "rl_08.rom", 0x000000, 0x040000, 0x8a49d3a7 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "rl.bin",    0x0000, 0x0200, 0x8914e72d )
ROM_END

ROM_START( rodlandj )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "rl_2.bin", 0x000000, 0x020000, 0xb1d2047e )
	ROM_LOAD16_BYTE( "rl_1.bin", 0x000001, 0x020000, 0x3c47c2a3 )
	ROM_LOAD16_BYTE( "rl_3.bin", 0x040000, 0x010000, 0xc5b1075f )
	ROM_LOAD16_BYTE( "rl_4.bin", 0x040001, 0x010000, 0x9ec61048 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "rl_05.rom", 0x000000, 0x010000, 0xc1617c28 )
	ROM_LOAD16_BYTE( "rl_06.rom", 0x000001, 0x010000, 0x663392b2 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	/* address and data lines are mangled, but otherwise identical to rl_23.rom */
	ROM_LOAD( "rl_14.bin", 0x000000, 0x080000, 0x8201e1bb )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "rl_18.rom", 0x000000, 0x080000, 0xf3b30ca6 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "rl_19.bin", 0x000000, 0x020000, 0x124d7e8f )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	/* was a bad dump (first and second half identical), reconstructed from rl_14.rom */
	ROM_LOAD( "rl_23.bin", 0x000000, 0x080000, 0x936db174 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "rl_10.rom", 0x000000, 0x040000, 0xe1d1cd99 )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "rl_08.rom", 0x000000, 0x040000, 0x8a49d3a7 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "rl.bin",    0x0000, 0x0200, 0x8914e72d )
ROM_END

/* 100% identical to rodlandj, but not encrypted */
ROM_START( rodlndjb )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "rl19.bin", 0x000000, 0x010000, 0x028de21f )
	ROM_LOAD16_BYTE( "rl17.bin", 0x000001, 0x010000, 0x9c720046 )
	ROM_LOAD16_BYTE( "rl20.bin", 0x020000, 0x010000, 0x3f536d07 )
	ROM_LOAD16_BYTE( "rl18.bin", 0x020001, 0x010000, 0x5aa61717 )
	ROM_LOAD16_BYTE( "rl_3.bin", 0x040000, 0x010000, 0xc5b1075f )
	ROM_LOAD16_BYTE( "rl_4.bin", 0x040001, 0x010000, 0x9ec61048 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "rl02.bin", 0x000000, 0x010000, 0xd26eae8f )
	ROM_LOAD16_BYTE( "rl01.bin", 0x000001, 0x010000, 0x04cf24bc )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "rl_23.rom", 0x000000, 0x020000, 0xac60e771 )
	ROM_CONTINUE(          0x030000, 0x010000 )
	ROM_CONTINUE(          0x050000, 0x010000 )
	ROM_CONTINUE(          0x020000, 0x010000 )
	ROM_CONTINUE(          0x040000, 0x010000 )
	ROM_CONTINUE(          0x060000, 0x020000 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "rl_18.rom", 0x000000, 0x080000, 0xf3b30ca6 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "rl_19.bin", 0x000000, 0x020000, 0x124d7e8f )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "rl_14.rom", 0x000000, 0x080000, 0x08d01bf4 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "rl_10.rom", 0x000000, 0x040000, 0xe1d1cd99 )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "rl_08.rom", 0x000000, 0x040000, 0x8a49d3a7 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "rl.bin",    0x0000, 0x0200, 0x8914e72d )
ROM_END


INPUT_PORTS_START( rodland )

	COINS						/* IN0 0x80001.b */
/*	fire	ladder */
	JOY_2BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE						/* IN2 0x80004.b */
	JOY_2BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80006.b */
	COINAGE_6BITS
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) /* according to manual */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) /* according to manual */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Inifinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x10, "Default episode" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy?" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************

							[ Saint Dragon ]

			*** Press coin on startup to enter test mode ***

interrupts:	1] rte	2] 620	3] 5e6

***************************************************************************/

ROM_START( stdragon )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "jsd-02.bin", 0x000000, 0x020000, 0xcc29ab19 )
	ROM_LOAD16_BYTE( "jsd-01.bin", 0x000001, 0x020000, 0x67429a57 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "jsd-05.bin", 0x000000, 0x010000, 0x8c04feaa )
	ROM_LOAD16_BYTE( "jsd-06.bin", 0x000001, 0x010000, 0x0bb62f3a )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "jsd-11.bin", 0x000000, 0x020000, 0x2783b7b1 )
	ROM_LOAD( "jsd-12.bin", 0x020000, 0x020000, 0x89466ab7 )
	ROM_LOAD( "jsd-13.bin", 0x040000, 0x020000, 0x9896ae82 )
	ROM_LOAD( "jsd-14.bin", 0x060000, 0x020000, 0x7e8da371 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "jsd-15.bin", 0x000000, 0x020000, 0xe296bf59 )
	ROM_LOAD( "jsd-16.bin", 0x020000, 0x020000, 0xd8919c06 )
	ROM_LOAD( "jsd-17.bin", 0x040000, 0x020000, 0x4f7ad563 )
	ROM_LOAD( "jsd-18.bin", 0x060000, 0x020000, 0x1f4da822 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "jsd-19.bin", 0x000000, 0x010000, 0x25ce807d )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "jsd-20.bin", 0x000000, 0x020000, 0x2c6e93bb )
	ROM_LOAD( "jsd-21.bin", 0x020000, 0x020000, 0x864bcc61 )
	ROM_LOAD( "jsd-22.bin", 0x040000, 0x020000, 0x44fe2547 )
	ROM_LOAD( "jsd-23.bin", 0x060000, 0x020000, 0x6b010e1a )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "jsd-09.bin", 0x000000, 0x020000, 0xe366bc5a )
	ROM_LOAD( "jsd-10.bin", 0x020000, 0x020000, 0x4a8f4fe6 )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "jsd-07.bin", 0x000000, 0x020000, 0x6a48e979 )
	ROM_LOAD( "jsd-08.bin", 0x020000, 0x020000, 0x40704962 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom.14m",    0x0000, 0x0200, 0x1d877538 )
ROM_END

INPUT_PORTS_START( stdragon )
	COINS						/* IN0 0x80001.b */
/*	fire	fire */
	JOY_2BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE						/* IN2 0x80004.b */
	JOY_2BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80006.b */
	COINAGE_6BITS_2
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )	/* used? */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )	/* unused? */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )	/* unused? */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "?Difficulty?" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************

								[ Soldam ]

(Japan version)
f00c2.l	*** score/10 (BCD) ***

The country code is at ROM address $3a9d, copied to RAM address
f0025: 0 = japan, 1 = USA. Change f0025 to 1 to have all the
text in english.

***************************************************************************/

ROM_START( soldamj )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "soldam2.bin", 0x000000, 0x020000, 0xc73d29e4 )
	ROM_LOAD16_BYTE( "soldam1.bin", 0x000001, 0x020000, 0xe7cb0c20 )
	ROM_LOAD16_BYTE( "soldam3.bin", 0x040000, 0x010000, 0xc5382a07 )
	ROM_LOAD16_BYTE( "soldam4.bin", 0x040001, 0x010000, 0x1df7816f )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "soldam5.bin", 0x000000, 0x010000, 0xd1019a67 )
	ROM_LOAD16_BYTE( "soldam6.bin", 0x000001, 0x010000, 0x3ed219b4 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "soldam14.bin", 0x000000, 0x080000, 0x26cea54a )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "soldam18.bin", 0x000000, 0x080000, 0x7d8e4712 )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "soldam19.bin", 0x000000, 0x020000, 0x38465da1 )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "soldam23.bin", 0x000000, 0x080000, 0x0ca09432 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "soldam10.bin", 0x000000, 0x040000, 0x8d5613bf )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "soldam8.bin",  0x000000, 0x040000, 0xfcd36019 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "soldam.m14",   0x0000, 0x0200, 0x8914e72d )
ROM_END

INPUT_PORTS_START( soldamj )
	COINS						/* IN0 0x80001.b */
	/*	turn	turn	(3rd button is shown in service mode, but seems unused) */
	JOY_2BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE						/* IN2 0x80004.b */
	JOY_2BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80006.b */
	COINAGE_6BITS_2
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On )  )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy"   )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard"   )
	PORT_DIPSETTING(    0x01, "Hadest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Games To Play (Vs)" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On )  )
	PORT_DIPNAME( 0x20, 0x20, "Credits To Start (Vs)" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x40, 0x40, "Credits To Continue (Vs)" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On )  )

INPUT_PORTS_END

READ16_HANDLER( soldamj_spriteram16_r )
{
	return spriteram16[offset];
}
WRITE16_HANDLER( soldamj_spriteram16_w )
{
	if (offset < 0x800/2)	COMBINE_DATA(&spriteram16[offset]);
}



/***************************************************************************

							[ Takeda Shingen ]

***************************************************************************/

ROM_START( tshingen )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "takeda2.bin", 0x000000, 0x020000, 0x6ddfc9f3 )
	ROM_LOAD16_BYTE( "takeda1.bin", 0x000001, 0x020000, 0x1afc6b7d )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "takeda5.bin", 0x000000, 0x010000, 0xfbdc51c0 )
	ROM_LOAD16_BYTE( "takeda6.bin", 0x000001, 0x010000, 0x8fa65b69 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "takeda11.bin", 0x000000, 0x020000, 0xbf0b40a6 )
	ROM_LOAD( "takeda12.bin", 0x020000, 0x020000, 0x07987d89 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "takeda15.bin", 0x000000, 0x020000, 0x4c316b79 )
	ROM_LOAD( "takeda16.bin", 0x020000, 0x020000, 0xceda9dd6 )
	ROM_LOAD( "takeda17.bin", 0x040000, 0x020000, 0x3d4371dc )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "takeda19.bin", 0x000000, 0x010000, 0x2ca2420d )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "takeda20.bin", 0x000000, 0x020000, 0x1bfd636f )
	ROM_LOAD( "takeda21.bin", 0x020000, 0x020000, 0x12fb006b )
	ROM_LOAD( "takeda22.bin", 0x040000, 0x020000, 0xb165b6ae )
	ROM_LOAD( "takeda23.bin", 0x060000, 0x020000, 0x37cb9214 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "takeda9.bin",  0x000000, 0x020000, 0xdb7f3f4f )
	ROM_LOAD( "takeda10.bin", 0x020000, 0x020000, 0xc9959d71 )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "shing_07.rom",  0x000000, 0x020000, 0xc37ecbdc )
	ROM_LOAD( "shing_08.rom",  0x020000, 0x020000, 0x36d56c8c )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom",    0x0000, 0x0200, 0x00000000 )
ROM_END

ROM_START( tshingna )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "shing_02.rom", 0x000000, 0x020000, 0xd9ab5b78 )
	ROM_LOAD16_BYTE( "shing_01.rom", 0x000001, 0x020000, 0xa9d2de20 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Sound CPU Code */
	ROM_LOAD16_BYTE( "takeda5.bin", 0x000000, 0x010000, 0xfbdc51c0 )
	ROM_LOAD16_BYTE( "takeda6.bin", 0x000001, 0x010000, 0x8fa65b69 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "takeda11.bin", 0x000000, 0x020000, 0xbf0b40a6 )
	ROM_LOAD( "shing_12.rom", 0x020000, 0x020000, 0x5e4adedb )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* Scroll 1 */
	ROM_LOAD( "shing_15.rom", 0x000000, 0x020000, 0x9db18233 )
	ROM_LOAD( "takeda16.bin", 0x020000, 0x020000, 0xceda9dd6 )
	ROM_LOAD( "takeda17.bin", 0x040000, 0x020000, 0x3d4371dc )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "shing_19.rom", 0x000000, 0x010000, 0x97282d9d )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "shing_20.rom", 0x000000, 0x020000, 0x7f6f8384 )
	ROM_LOAD( "takeda21.bin", 0x020000, 0x020000, 0x12fb006b )
	ROM_LOAD( "takeda22.bin", 0x040000, 0x020000, 0xb165b6ae )
	ROM_LOAD( "takeda23.bin", 0x060000, 0x020000, 0x37cb9214 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )		/* Samples */
	ROM_LOAD( "takeda9.bin",  0x000000, 0x020000, 0xdb7f3f4f )
	ROM_LOAD( "takeda10.bin", 0x020000, 0x020000, 0xc9959d71 )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )		/* Samples */
	ROM_LOAD( "shing_07.rom",  0x000000, 0x020000, 0xc37ecbdc )
	ROM_LOAD( "shing_08.rom",  0x020000, 0x020000, 0x36d56c8c )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )		/* Priority PROM */
	ROM_LOAD( "prom",    0x0000, 0x0200, 0x00000000 )
ROM_END

INPUT_PORTS_START( tshingen )
	COINS						/* IN0 0x80001.b */
	/* sword_left	sword_right		jump */
	JOY_3BUTTONS(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE						/* IN2 0x80004.b */
	JOY_3BUTTONS(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80006.b */
	COINAGE_6BITS
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On )  )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On )  )

	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Inifinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "20k" )
	PORT_DIPSETTING(    0x04, "30k" )
	PORT_DIPSETTING(    0x08, "40k" )
	PORT_DIPSETTING(    0x00, "50k" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Difficulty ) ) /* damage when hit */
	PORT_DIPSETTING(    0x30, "Easy"    ) /* 0 */
	PORT_DIPSETTING(    0x10, "Normal"  ) /* 1 */
	PORT_DIPSETTING(    0x20, "Hard"    ) /* 2 */
	PORT_DIPSETTING(    0x00, "Hardest" ) /* 3 */
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On )  )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On )  )

INPUT_PORTS_END




/***************************************************************************

							 Code Decryption

***************************************************************************/

static void phantasm_rom_decode(int cpu)
{
	data16_t	*RAM	=	(data16_t *) memory_region(REGION_CPU1+cpu);
	int i,		size	=	memory_region_length(REGION_CPU1+cpu);
	if (size > 0x40000)	size = 0x40000;

	for (i = 0 ; i < size/2 ; i++)
	{
		data16_t x,y;

		x = RAM[i];

/* [0] def0 189a bc56 7234 */
/* [1] fdb9 7531 eca8 6420 */
/* [2] 0123 4567 ba98 fedc */
#define BITSWAP_0	BITSWAP16(x,0xd,0xe,0xf,0x0,0x1,0x8,0x9,0xa,0xb,0xc,0x5,0x6,0x7,0x2,0x3,0x4)
#define BITSWAP_1	BITSWAP16(x,0xf,0xd,0xb,0x9,0x7,0x5,0x3,0x1,0xe,0xc,0xa,0x8,0x6,0x4,0x2,0x0)
#define BITSWAP_2	BITSWAP16(x,0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0xb,0xa,0x9,0x8,0xf,0xe,0xd,0xc)

		if		(i < 0x08000/2)	{ if ( (i | (0x248/2)) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x10000/2)	{ y = BITSWAP_2; }
		else if	(i < 0x18000/2)	{ if ( (i | (0x248/2)) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x20000/2)	{ y = BITSWAP_1; }
		else 					{ y = BITSWAP_2; }

#undef	BITSWAP_0
#undef	BITSWAP_1
#undef	BITSWAP_2

		RAM[i] = y;
	}

}

void astyanax_rom_decode(int cpu)
{
	data16_t	*RAM	=	(data16_t *) memory_region(REGION_CPU1+cpu);
	int i,		size	=	memory_region_length(REGION_CPU1+cpu);
	if (size > 0x40000)	size = 0x40000;

	for (i = 0 ; i < size/2 ; i++)
	{
		data16_t x,y;

		x = RAM[i];

/* [0] def0 a981 65cb 7234 */
/* [1] fdb9 7531 8ace 0246 */
/* [2] 4567 0123 ba98 fedc */

#define BITSWAP_0	BITSWAP16(x,0xd,0xe,0xf,0x0,0xa,0x9,0x8,0x1,0x6,0x5,0xc,0xb,0x7,0x2,0x3,0x4)
#define BITSWAP_1	BITSWAP16(x,0xf,0xd,0xb,0x9,0x7,0x5,0x3,0x1,0x8,0xa,0xc,0xe,0x0,0x2,0x4,0x6)
#define BITSWAP_2	BITSWAP16(x,0x4,0x5,0x6,0x7,0x0,0x1,0x2,0x3,0xb,0xa,0x9,0x8,0xf,0xe,0xd,0xc)

		if		(i < 0x08000/2)	{ if ( (i | (0x248/2)) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x10000/2)	{ y = BITSWAP_2; }
		else if	(i < 0x18000/2)	{ if ( (i | (0x248/2)) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x20000/2)	{ y = BITSWAP_1; }
		else 					{ y = BITSWAP_2; }

#undef	BITSWAP_0
#undef	BITSWAP_1
#undef	BITSWAP_2

		RAM[i] = y;
	}
}

static void rodland_rom_decode(int cpu)
{
	data16_t	*RAM	=	(data16_t *) memory_region(REGION_CPU1+cpu);
	int i,		size	=	memory_region_length(REGION_CPU1+cpu);
	if (size > 0x40000)	size = 0x40000;

	for (i = 0 ; i < size/2 ; i++)
	{
		data16_t x,y;

		x = RAM[i];

/* [0] d0a9 6ebf 5c72 3814	[1] 4567 0123 ba98 fedc */
/* [2] fdb9 ce07 5318 a246	[3] 4512 ed3b a967 08fc */
#define BITSWAP_0	BITSWAP16(x,0xd,0x0,0xa,0x9,0x6,0xe,0xb,0xf,0x5,0xc,0x7,0x2,0x3,0x8,0x1,0x4);
#define BITSWAP_1	BITSWAP16(x,0x4,0x5,0x6,0x7,0x0,0x1,0x2,0x3,0xb,0xa,0x9,0x8,0xf,0xe,0xd,0xc);
#define	BITSWAP_2	BITSWAP16(x,0xf,0xd,0xb,0x9,0xc,0xe,0x0,0x7,0x5,0x3,0x1,0x8,0xa,0x2,0x4,0x6);
#define	BITSWAP_3	BITSWAP16(x,0x4,0x5,0x1,0x2,0xe,0xd,0x3,0xb,0xa,0x9,0x6,0x7,0x0,0x8,0xf,0xc);

		if		(i < 0x08000/2)	{	if ( (i | (0x248/2)) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x10000/2)	{	if ( (i | (0x248/2)) != i ) {y = BITSWAP_2;} else {y = BITSWAP_3;} }
		else if	(i < 0x18000/2)	{	if ( (i | (0x248/2)) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x20000/2)	{ y = BITSWAP_1; }
		else 					{ y = BITSWAP_3; }

#undef	BITSWAP_0
#undef	BITSWAP_1
#undef	BITSWAP_2
#undef	BITSWAP_3

		RAM[i] = y;
	}
}


static void rodlandj_gfx_unmangle(int region)
{
	data8_t *rom = memory_region(REGION_GFX1+region);
	int size = memory_region_length(REGION_GFX1+region);
	data8_t *buffer;
	int i;

	/* data lines swap: 76543210 -> 64537210 */
	for (i = 0;i < size;i++)
		rom[i] =   (rom[i] & 0x27)
				| ((rom[i] & 0x80) >> 4)
				| ((rom[i] & 0x48) << 1)
				| ((rom[i] & 0x10) << 2);

	buffer = malloc(size);
	if (!buffer) return;

	memcpy(buffer,rom,size);

	/* address lines swap: ..dcba9876543210 -> ..acb8937654d210 */
	for (i = 0;i < size;i++)
	{
		int a =    (i &~0x2508)
				| ((i & 0x2000) >> 10)
				| ((i & 0x0400) << 3)
				| ((i & 0x0100) << 2)
				| ((i & 0x0008) << 5);
		rom[i] = buffer[a];
	}

	free(buffer);
}

static void jitsupro_gfx_unmangle(int region)
{
	data8_t *rom = memory_region(REGION_GFX1+region);
	int size = memory_region_length(REGION_GFX1+region);
	data8_t *buffer;
	int i;

	/* data lines swap: 76543210 -> 43576210 */
	for (i = 0;i < size;i++)
		rom[i] =   BITSWAP8(rom[i],0x4,0x3,0x5,0x7,0x6,0x2,0x1,0x0);

	buffer = malloc(size);
	if (!buffer) return;

	memcpy(buffer,rom,size);

	/* address lines swap: fedcba9876543210 -> fe8cb39d7654a210 */
	for (i = 0;i < size;i++)
	{
		int a = (i & ~0xffff) |
	BITSWAP16(i,0xf,0xe,0x8,0xc,0xb,0x3,0x9,0xd,0x7,0x6,0x5,0x4,0xa,0x2,0x1,0x0);

		rom[i] = buffer[a];
	}

	free(buffer);
}




static void init_64street(void)
{
/*	data16_t *RAM = (data16_t *) memory_region(REGION_CPU1); */
/*	RAM[0x006b8/2] = 0x6004;		// d8001 test */
/*	RAM[0x10EDE/2] = 0x6012;		// watchdog */

	ip_select_values[0] = 0x57;
	ip_select_values[1] = 0x53;
	ip_select_values[2] = 0x54;
	ip_select_values[3] = 0x55;
	ip_select_values[4] = 0x56;
}

static void init_astyanax(void)
{
	data16_t *RAM;

	astyanax_rom_decode(0);

	RAM = (data16_t *) memory_region(REGION_CPU1);
	RAM[0x0004e6/2] = 0x6040;	/* protection */
}

static void init_avspirit(void)
{
	ip_select_values[0] = 0x37;
	ip_select_values[1] = 0x35;
	ip_select_values[2] = 0x36;
	ip_select_values[3] = 0x33;
	ip_select_values[4] = 0x34;

	/* kludge: avspirit has 0x10000 bytes of RAM while edf has 0x20000. The */
	/* following is needed to make vh_start() pick the correct address */
	/* for spriteram16. */
	megasys1_ram += 0x10000/2;
}

static void init_bigstrik(void)
{
	ip_select_values[0] = 0x58;
	ip_select_values[1] = 0x54;
	ip_select_values[2] = 0x55;
	ip_select_values[3] = 0x56;
	ip_select_values[4] = 0x57;
}

static void init_chimerab(void)
{
	/* same as cybattlr */
	ip_select_values[0] = 0x56;
	ip_select_values[1] = 0x52;
	ip_select_values[2] = 0x53;
	ip_select_values[3] = 0x54;
	ip_select_values[4] = 0x55;
}

static void init_cybattlr(void)
{
	ip_select_values[0] = 0x56;
	ip_select_values[1] = 0x52;
	ip_select_values[2] = 0x53;
	ip_select_values[3] = 0x54;
	ip_select_values[4] = 0x55;
}

static void init_edf(void)
{
	ip_select_values[0] = 0x20;
	ip_select_values[1] = 0x21;
	ip_select_values[2] = 0x22;
	ip_select_values[3] = 0x23;
	ip_select_values[4] = 0x24;
}

static void init_hachoo(void)
{
	data16_t *RAM;

	astyanax_rom_decode(0);

	RAM  = (data16_t *) memory_region(REGION_CPU1);
	RAM[0x0006da/2] = 0x6000;	/* protection */
}

static void init_iganinju(void)
{
	data16_t *RAM;

	phantasm_rom_decode(0);

	RAM  = (data16_t *) memory_region(REGION_CPU1);
	RAM[0x02f000/2] = 0x835d;	/* protection */

	RAM[0x00006e/2] = 0x0420;	/* the only game that does */
								/* not like lev 3 interrupts */
}

static WRITE16_HANDLER( OKIM6295_data_0_both_w )
{
	if (ACCESSING_LSB)	OKIM6295_data_0_w(0, (data >> 0) & 0xff );
	else				OKIM6295_data_0_w(0, (data >> 8) & 0xff );
}
static WRITE16_HANDLER( OKIM6295_data_1_both_w )
{
	if (ACCESSING_LSB)	OKIM6295_data_1_w(0, (data >> 0) & 0xff );
	else				OKIM6295_data_1_w(0, (data >> 8) & 0xff );
}

static void init_jitsupro(void)
{
	data16_t *RAM  = (data16_t *) memory_region(REGION_CPU1);

	astyanax_rom_decode(0);		/* Code */

	jitsupro_gfx_unmangle(0);	/* Gfx */
	jitsupro_gfx_unmangle(3);

	RAM[0x436/2] = 0x4e71;	/* protection */
	RAM[0x438/2] = 0x4e71;	/* */

	/* the sound code writes oki commands to both the lsb and msb */
	install_mem_write16_handler(1, 0xa0000, 0xa0003, OKIM6295_data_0_both_w);
	install_mem_write16_handler(1, 0xc0000, 0xc0003, OKIM6295_data_1_both_w);
}

static void init_peekaboo(void)
{
	install_mem_read16_handler (0, 0x100000, 0x100001, protection_peekaboo_r);
	install_mem_write16_handler(0, 0x100000, 0x100001, protection_peekaboo_w);
}

static void init_phantasm(void)
{
	phantasm_rom_decode(0);
}

static void init_plusalph(void)
{
	data16_t *RAM;

	astyanax_rom_decode(0);

	RAM  = (data16_t *) memory_region(REGION_CPU1);
	RAM[0x0012b6/2] = 0x0000;	/* protection */
}

static void init_rodland(void)
{
	rodland_rom_decode(0);
}

static void init_rodlandj(void)
{
	rodlandj_gfx_unmangle(0);
	rodlandj_gfx_unmangle(3);

	astyanax_rom_decode(0);
}

static void init_soldam(void)
{
	astyanax_rom_decode(0);

	/* Sprite RAM is mirrored. Why? */
	install_mem_read16_handler (0, 0x8c000, 0x8cfff, soldamj_spriteram16_r);
	install_mem_write16_handler(0, 0x8c000, 0x8cfff, soldamj_spriteram16_w);
}

static void init_stdragon(void)
{
	data16_t *RAM;

	phantasm_rom_decode(0);

	RAM  = (data16_t *) memory_region(REGION_CPU1);
	RAM[0x00045e/2] = 0x0098;	/* protection */
}



GAME( 1988, lomakai,  0,        system_Z, lomakai,  0,        ROT0,       "Jaleco", "Legend of Makai (World)" )
GAME( 1988, makaiden, lomakai,  system_Z, lomakai,  0,        ROT0,       "Jaleco", "Makai Densetsu (Japan)" )
GAME( 1988, p47,      0,        system_A, p47,      0,        ROT0,       "Jaleco", "P-47 - The Phantom Fighter (World)" )
GAME( 1988, p47j,     p47,      system_A, p47,      0,        ROT0,       "Jaleco", "P-47 - The Freedom Fighter (Japan)" )
GAME( 1988, kickoff,  0,        system_A, kickoff,  0,        ROT0,       "Jaleco", "Kick Off (Japan)" )
GAME( 1988, tshingen, 0,        system_A, tshingen, phantasm, ROT0,       "Jaleco", "Takeda Shingen (Japan, Japanese)" )
GAME( 1988, tshingna, tshingen, system_A, tshingen, phantasm, ROT0,       "Jaleco", "Shingen Samurai-Fighter (Japan, English)" )
GAME( 1988, iganinju, 0,        system_A, iganinju, iganinju, ROT0,       "Jaleco", "Iga Ninjyutsuden (Japan)" )
GAME( 1989, astyanax, 0,        system_A, astyanax, astyanax, ROT0_16BIT, "Jaleco", "The Astyanax" )
GAME( 1989, lordofk,  astyanax, system_A, astyanax, astyanax, ROT0_16BIT, "Jaleco", "The Lord of King (Japan)" )
GAMEX(1989, hachoo,   0,        system_A, hachoo,   hachoo,   ROT0,       "Jaleco", "Hachoo!", GAME_IMPERFECT_SOUND )
GAME( 1989, jitsupro, 0,        jitsupro, jitsupro, jitsupro, ROT0,       "Jaleco", "Jitsuryoku!! Pro Yakyuu (Japan)" )
GAME( 1989, plusalph, 0,        system_A, plusalph, plusalph, ROT270,     "Jaleco", "Plus Alpha" )
GAME( 1989, stdragon, 0,        system_A, stdragon, stdragon, ROT0,       "Jaleco", "Saint Dragon" )
GAME( 1990, rodland,  0,        system_A, rodland,  rodland,  ROT0,       "Jaleco", "Rod-Land (World)" )
GAME( 1990, rodlandj, rodland,  system_A, rodland,  rodlandj, ROT0,       "Jaleco", "Rod-Land (Japan)" )
GAME( 1990, rodlndjb, rodland,  system_A, rodland,  0,        ROT0,       "Jaleco", "Rod-Land (Japan bootleg)" )
GAME( 1991, avspirit, 0,        system_B, avspirit, avspirit, ROT0,       "Jaleco", "Avenging Spirit" )
GAME( 1990, phantasm, avspirit, system_A, avspirit, phantasm, ROT0,       "Jaleco", "Phantasm (Japan)" )
GAME( 1991, edf,      0,        system_B, edf,      edf,      ROT0,       "Jaleco", "Earth Defense Force" )
GAME( 1991, 64street, 0,        system_C, 64street, 64street, ROT0,       "Jaleco", "64th. Street - A Detective Story (World)" )
GAME( 1991, 64streej, 64street, system_C, 64street, 64street, ROT0,       "Jaleco", "64th. Street - A Detective Story (Japan)" )
GAME( 1992, soldamj,  0,        system_A, soldamj,  soldam,   ROT0,       "Jaleco", "Soldam (Japan)" )
GAME( 1992, bigstrik, 0,        system_C, bigstrik, bigstrik, ROT0,       "Jaleco", "Big Striker" )
GAME( 1993, chimerab, 0,        system_C, chimerab, chimerab, ROT0,       "Jaleco", "Chimera Beast" )
GAME( 1993, cybattlr, 0,        system_C, cybattlr, cybattlr, ROT90,      "Jaleco", "Cybattler" )
GAME( 1993, peekaboo, 0,        system_D, peekaboo, peekaboo, ROT0,       "Jaleco", "Peek-a-Boo!" )
