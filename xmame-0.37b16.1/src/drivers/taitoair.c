/***************************************************************************

Taito Air System
----------------

Midnight Landing        *** not dumped, 1987? ***
Top Landing             (c) 1988 Taito
Air Inferno             (c) 1990 Taito


(Thanks to Raine team for their preliminary drivers)



System specs	(from TaitoH: incorrect!)
------------

 CPU   : MC68000 (12 MHz) x 1, Z80 (4 MHz?, sound CPU) x 1
 Sound : YM2610, YM3016?
 OSC   : 20.000 MHz, 8.000 MHz, 24.000 MHz
 Chips : TC0070RGB (Palette?)
         TC0220IOC (Input)
         TC0140SYT (Sound communication)
         TC0130LNB (???)
         TC0160ROM (???)
         TC0080VCO (Video?)

From Ainferno readme
--------------------

Location     Type       File ID
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CPU IC5       9016*     C45-01
CPU IC4       9016*     C45-02
CPU IC3       9016*     C45-03
CPU IC2       9016*     C45-04
CPU IC1       9016*     C45-05
CPU IC31      9016*     C45-06
VID IC28     27C010     C45-11
VID IC29     27C010     C45-12
VID IC30     27C010     C45-13
VID IC31     27C010     C45-14
VID IC40     27C010     C45-15
VID IC41     27C010     C45-16
VID IC42     27C010     C45-17
VID IC43     27C010     C45-18
CPU IC14     27C010     C45-20
CPU IC42     27C010     C45-21
CPU IC43     27C010     C45-22
CPU IC43     27C010     C45-23
CPU IC6      LH5763     C45-24
CPU IC35     LH5763     C45-25
CPU IC13     27C010     C45-28

VID IC6    PAL16L8B     C45-07
VID IC62   PAL16L8B     C45-08
VID IC63   PAL16L8B     C45-09
VID IC2    PAL20L8B     C45-10
CPU IC76   PAL16L8B     C45-26
CPU IC114  PAL16L8B     C45-27
CPU IC60   PAL20L8B     B62-02
CPU IC62   PAL20L8B     B62-03
CPU IC63   PAL20L8B     B62-04
CPU IC82   PAL16L8B     B62-07
VID IC23   PAL16L8B     B62-08
VID IC26   PAL16L8B     B62-11
VID IC27   PAL16L8B     B62-12


Notes:  CPU - CPU PCB      K1100586A  M4300186A
        VID - Video PCB    K1100576A  M4300186A


Known TC0080VCO issues	(from TaitoH driver)
----------------------

 - Y coordinate of sprite zooming is non-linear, so currently implemented
   hand-tuned value and this is used for only Record Breaker.
 - Sprite and BG1 priority bit is not understood. It is defined by sprite
   priority in Record Breaker and by zoom value and some scroll value in
   Dynamite League. So, some priority problems still remain.
 - Background zoom effect is not working in flip screen mode.
 - Sprite zoom is a bit wrong.


TODO	(TC0080VCO issues shared with TaitoH driver)
----

 - Need to implement BG1 : sprite priority. Currently not clear how this works.
 - Fix sprite coordinates.
 - Improve zoom y coordinate.


TODO
----

Video section hung off TaitoH driver, it should be separate.

TMS320C25 needs to be emulated. Are the short interleaved roms
its program?

3d graphics h/w: do the gradiation ram and line ram map to
hardware which creates the 3d background scenes? If so,
presumably the TMS320C25 is being used as a co-processor to
relieve the 68000 of 3d calculations... and the results are
shoved into gradiation/line ram to drive the graphics h/w.

"Power common ram" is presumably for communication with an MCU
controlling the sit-in-cabinet (deluxe mechanized version only).

[Offer dip-selectable kludge of the analogue stick inputs so that
keyboard play is possible.]

DIPs


Topland
-------

Sprite/tile priority bad.

After demo game in attract, palette seems too dark for a while.
Palette corruption has occured with areas incorrectly cleared
or faded. (Perhaps 68000 relies on feedback from co-processor
in determining what parts of palette ram to write?)

Mechanized cabinet has a problem with test mode: there is
code at $d72 calling a sub which tests a silly amount of "power
common ram"; $80000 words (only one byte per word used).
Probably the address map wraps, and only $400 separate words
are actually accessed ?

Game hangs when you run out of time: code is looping and
interrupts are taken but nothing seems to happen.


Ainferno
--------

Sprite/tile priority bad.


Log
---

Topland
-------
cpu #0 (PC=000016E6): unmapped word write to 00140000 = 0500 & FF00

Ainferno
--------
cpu #0 (PC=00019410): unmapped word write to 00980000 = 0000 & FFFF
cpu #0 (PC=00019412): unmapped word write to 00980002 = 0000 & FFFF
cpu #0 (PC=00019414): unmapped word write to 00980004 = 0000 & FFFF
cpu #0 (PC=00019416): unmapped word write to 00980006 = 0000 & FFFF
cpu #0 (PC=00019418): unmapped word write to 00980008 = 0000 & FFFF
cpu #0 (PC=0001941A): unmapped word write to 0098000A = 0000 & FFFF
cpu #0 (PC=0001941C): unmapped word write to 0098000C = 0000 & FFFF
cpu #0 (PC=0001941E): unmapped word write to 0098000E = 0000 & FFFF
cpu #0 (PC=00001EA4): unmapped word write to 00140000 = 0005 & 00FF
cpu #0 (PC=00001EBC): unmapped word write to 00830000 = A6E7 & FFFF

 
****************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/taitosnd.h"
#include "vidhrdw/taitoic.h"

static data16_t *taitoh_68000_mainram;

int		recordbr_vh_start(void);
void 		syvalion_vh_stop (void);
void		recordbr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static WRITE16_HANDLER( airsys_paletteram16_w )	/* xxBBBBxRRRRxGGGG */
{
	int a,r,g,b;
	COMBINE_DATA(&paletteram16[offset]);

	a = paletteram16[offset];

	r = (a >> 0) & 0x0f;
	g = (a >> 5) & 0x0f;
	b = (a >> 10) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_change_color(offset,r,g,b);
}


/***********************************************************
				INPUTS
***********************************************************/

static READ16_HANDLER( stick_input_r )
{
	switch( offset )
	{
		case 0x00:	/* "counter 1" lo */
			return input_port_4_word_r(0,0);

		case 0x01:	/* "counter 2" lo */
			return input_port_5_word_r(0,0);

		case 0x02:	/* "counter 1" hi */
			return (input_port_4_word_r(0,0) &0xff00) >> 8;

		case 0x03:	/* "counter 2" hi */
			return (input_port_5_word_r(0,0) &0xff00) >> 8;
	}

	return 0;
}

static READ16_HANDLER( stick2_input_r )
{
	switch( offset )
	{
		case 0x00:	/* "counter 3" lo */
			return input_port_6_word_r(0,0);

		case 0x02:	/* "counter 3" hi */
			return (input_port_6_word_r(0,0) &0xff00) >> 8;
	}

	return 0;
}


static int banknum = -1;

static void reset_sound_region(void)
{
	cpu_setbank(1, memory_region(REGION_CPU2) + (banknum * 0x4000) + 0x10000);
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	banknum = (data - 1) & 3;
	reset_sound_region();
}


/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ16_START( airsys_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x0c0000, 0x0cffff, MRA16_RAM },			/* 68000 RAM */
	{ 0x180000, 0x183fff, MRA16_RAM },			/* "gradiation ram (0)" */
	{ 0x184000, 0x187fff, MRA16_RAM },			/* "gradiation ram (1)" */
	{ 0x188000, 0x18bfff, paletteram16_word_r },	/* "color ram" */
	{ 0x800000, 0x820fff, TC0080VCO_word_r },		/* tilemaps, sprites */
	{ 0x908000, 0x90ffff, MRA16_RAM },			/* "line ram" */
	{ 0x910000, 0x91ffff, MRA16_RAM },			/* "dsp common ram" (TMS320C25) */
	{ 0xa00000, 0xa00007, stick_input_r },
	{ 0xa00100, 0xa00107, stick2_input_r },
	{ 0xa00200, 0xa0020f, TC0220IOC_halfword_r },	/* other I/O */
	{ 0xa80000, 0xa80001, MRA16_NOP },
	{ 0xa80002, 0xa80003, taitosound_comm16_lsb_r },
	{ 0xb00000, 0xb007ff, MRA16_RAM },			/* "power common ram" (mecha drive) */
MEMORY_END

static MEMORY_WRITE16_START( airsys_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x0c0000, 0x0cffff, MWA16_RAM, &taitoh_68000_mainram },
/*	{ 0x140000, 0x140001, MWA16_NOP },	// ??? */
	{ 0x180000, 0x183fff, MWA16_RAM },			/* "gradiation ram (0)" */
	{ 0x184000, 0x187fff, MWA16_RAM },			/* "gradiation ram (1)" */
	{ 0x188000, 0x18bfff, airsys_paletteram16_w, &paletteram16 },
	{ 0x800000, 0x820fff, TC0080VCO_word_w },		/* tilemaps, sprites */
	{ 0x908000, 0x90ffff, MWA16_RAM },			/* "line ram" */
	{ 0x910000, 0x91ffff, MWA16_RAM },			/* "dsp common ram" (TMS320C25) */
	{ 0xa00200, 0xa0020f, TC0220IOC_halfword_w },	/* I/O */
	{ 0xa80000, 0xa80001, taitosound_port16_lsb_w },
	{ 0xa80002, 0xa80003, taitosound_comm16_lsb_w },
	{ 0xb00000, 0xb007ff, MWA16_RAM },			/* "power common ram" (mecha drive) */
MEMORY_END

/***********************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP },		/* pan control */
	{ 0xee00, 0xee00, MWA_NOP }, 		/* ? */
	{ 0xf000, 0xf000, MWA_NOP }, 		/* ? */
	{ 0xf200, 0xf200, sound_bankswitch_w },
MEMORY_END


/************************************************************
			   INPUT PORTS & DIPS
************************************************************/

#define TAITO_COINAGE_JAPAN_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_COINAGE_US_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, "Price to Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0xc0, "Same as Start" )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_START( topland )
	PORT_START  /* DSWA */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Mechanized (alt)?" )
	PORT_DIPSETTING(    0x01, "Standard (alt) ?" )
	PORT_DIPSETTING(    0x02, "Mechanized" )
	PORT_DIPSETTING(    0x03, "Standard" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START  /* DSWB, all bogus !!! */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000k only" )
	PORT_DIPSETTING(    0x0c, "1500k only" )
	PORT_DIPSETTING(    0x04, "2000k only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	/* "door" (!) */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )	/* slot down */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )	/* slot up */
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )	/* handle */
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	/* freeze ??? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	/* The range of these sticks reflects the range test mode displays.
	   Eventually we want standard 0-0xff input range and a scale-up later
	   in the stick_r routines.  And fake DSW with self-centering option
	   to make keyboard control feasible! */

	PORT_START  /* Stick 1 (4) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_X | IPF_PLAYER1, 30, 40, 0xf800, 0x7ff )

	PORT_START  /* Stick 2 (5) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_Y | IPF_PLAYER1, 30, 40, 0xf800, 0x7ff )

	PORT_START  /* Stick 3 (6) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_Y | IPF_PLAYER2, 30, 40, 0xf800, 0x7ff )
INPUT_PORTS_END

INPUT_PORTS_START( ainferno )
	PORT_START  /* DSWA */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Mechanized (alt)?" )
	PORT_DIPSETTING(    0x01, "Special Sensors" )	/* on its test mode screen */
	PORT_DIPSETTING(    0x02, "Mechanized" )
	PORT_DIPSETTING(    0x03, "Standard" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_US_8

	PORT_START  /* DSWB, all bogus !!! */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000k only" )
	PORT_DIPSETTING(    0x0c, "1500k only" )
	PORT_DIPSETTING(    0x04, "2000k only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )	/* lever */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )	/* handle x */
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	/* handle y */
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_BUTTON4 | IPF_PLAYER1 )	/* fire */
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON5 | IPF_PLAYER1 )	/* pedal r */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON6 | IPF_PLAYER1 )	/* pedal l */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )	/* freeze (code at $7d6 hangs) */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	/* The range of these sticks reflects the range test mode displays.
	   Eventually we want standard 0-0xff input range and a scale-up later
	   in the stick_r routines. And fake DSW with self-centering option
	   to make keyboard control feasible! */

	PORT_START  /* Stick 1 (4) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_X | IPF_PLAYER1, 30, 40, 0xf800, 0x7ff )

	PORT_START  /* Stick 2 (5) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_Y | IPF_PLAYER1, 30, 40, 0xf800, 0x7ff )

	PORT_START  /* Stick 3 (6) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_Y | IPF_PLAYER2, 30, 40, 0xf800, 0x7ff )
INPUT_PORTS_END


/************************************************************
				GFX DECODING
************************************************************/

static struct GfxLayout tilelayout =
{
	16,16,	/* 16x16 pixels */
	RGN_FRAC(1,4),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 0x40000*8+4, 0x40000*8, 0x40000*8+12, 0x40000*8+8,
	    0x80000*8+4, 0x80000*8, 0x80000*8+12, 0x80000*8+8,
	    0xc0000*8+4, 0xc0000*8, 0xc0000*8+12, 0xc0000*8+8 },
	{ 0*16, 1*16, 2*16,  3*16,  4*16,  5*16,  6*16,  7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static struct GfxDecodeInfo airsys_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0, 32*16 },
	{ -1 } /* end of array */
};


/************************************************************
				YM2610 (SOUND)
************************************************************/

/* Handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface airsys_ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 4 MHz */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND2 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};


/************************************************************
				MACHINE DRIVERS
************************************************************/

static struct MachineDriver machine_driver_airsys =
{
	{
		{
			CPU_M68000,
			24000000 / 2,		/* 12 MHz ??? */
			airsys_readmem, airsys_writemem, 0, 0,
			m68_level5_irq, 1
		},
		{
			CPU_Z80,
			8000000 / 2,		/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	10,
	0,

	/* video hardware */
	64*16, 64*16, { 0*16, 32*16-1, 3*16, 28*16-1 },
	airsys_gfxdecodeinfo,
	512*16, 512*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	recordbr_vh_start,
	syvalion_vh_stop,
	recordbr_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2610,
			&airsys_ym2610_interface
		},
	}
};


/*************************************************************
				   DRIVERS

Ainferno may be missing an 0x2000 byte rom from the video
board - possibly?
*************************************************************/

ROM_START( topland )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 68000 */
	ROM_LOAD16_BYTE( "b62_41.43",  0x00000, 0x20000, 0x28264798 )
	ROM_LOAD16_BYTE( "b62_40.14",  0x00001, 0x20000, 0xdb872f7d )
	ROM_LOAD16_BYTE( "b62_25.42",  0x40000, 0x20000, 0x1bd53a72 )
	ROM_LOAD16_BYTE( "b62_24.13",  0x40001, 0x20000, 0x845026c5 )
	ROM_LOAD16_BYTE( "b62_23.41",  0x80000, 0x20000, 0xef3a971c )
	ROM_LOAD16_BYTE( "b62_22.12",  0x80001, 0x20000, 0x94279201 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 */
	ROM_LOAD( "b62-42.34", 0x00000, 0x04000, 0x389230e0 )
	ROM_CONTINUE(          0x10000, 0x0c000 )

	/* Plus a TMS320C25 */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* 16x16 tiles */
	ROM_LOAD16_BYTE( "b62-33.39",  0x000000, 0x20000, 0x38786867 )
	ROM_LOAD16_BYTE( "b62-36.48",  0x000001, 0x20000, 0x4259e76a )
	ROM_LOAD16_BYTE( "b62-29.27",  0x040000, 0x20000, 0xefdd5c51 )
	ROM_LOAD16_BYTE( "b62-34.40",  0x040001, 0x20000, 0xa7e10ca4 )
	ROM_LOAD16_BYTE( "b62-35.47",  0x080000, 0x20000, 0xcba7bac5 )
	ROM_LOAD16_BYTE( "b62-30.28",  0x080001, 0x20000, 0x30e37cb8 )
	ROM_LOAD16_BYTE( "b62-31.29",  0x0c0000, 0x20000, 0x3feebfe3 )
	ROM_LOAD16_BYTE( "b62-32.30",  0x0c0001, 0x20000, 0x66806646 )

	ROM_REGION( 0xa0000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b62-13.1",  0x00000, 0x20000, 0xb37dc3ea )
	ROM_LOAD( "b62-14.2",  0x20000, 0x20000, 0x617948a3 )
	ROM_LOAD( "b62-15.3",  0x40000, 0x20000, 0xe35ffe81 )
	ROM_LOAD( "b62-16.4",  0x60000, 0x20000, 0x203a5c27 )
	ROM_LOAD( "b62-17.5",  0x80000, 0x20000, 0x36447066 )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "b62-18.31", 0x00000, 0x20000, 0x3a4e687a )

	ROM_REGION( 0x10000, REGION_USER1, 0 )	/* unknown */
	ROM_LOAD( "b62-28.22", 0x08000, 0x02000, 0xc4be68a6 )	/* video board */

	/* TMS320C25 code ? */
	/* Interleaved pair, $a34 words of code */
	ROM_LOAD16_BYTE( "b62-20.6",  0x00000, 0x02000, 0xa4afe958 )	/* cpu board */
	ROM_LOAD16_BYTE( "b62-21.35", 0x00001, 0x02000, 0x5f38460d )	/* cpu board */
ROM_END

ROM_START( ainferno )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 68000 */
	ROM_LOAD16_BYTE( "c45_22.43", 0x00000, 0x20000, 0x50300926 )
	ROM_LOAD16_BYTE( "c45_20.14", 0x00001, 0x20000, 0x39b189d9 )
	ROM_LOAD16_BYTE( "c45_21.42", 0x40000, 0x20000, 0x1b687241 )
	ROM_LOAD16_BYTE( "c45_28.13", 0x40001, 0x20000, 0xc7cd2567 )

	/* 0x80000 to 0xbffff is empty for this game */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 */
	ROM_LOAD( "c45-23.34", 0x00000, 0x04000, 0xd0750c78 )
	ROM_CONTINUE(          0x10000, 0x0c000 )

	/* Plus a TMS320C25 */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* 16x16 tiles */
	ROM_LOAD16_BYTE( "c45-11.28", 0x000000, 0x20000, 0xd9b4b77c )
	ROM_LOAD16_BYTE( "c45-15.40", 0x000001, 0x20000, 0xd4610698 )
	ROM_LOAD16_BYTE( "c45-12.29", 0x040000, 0x20000, 0x4ae305b8 )
	ROM_LOAD16_BYTE( "c45-16.41", 0x040001, 0x20000, 0xc6eb93b0 )
	ROM_LOAD16_BYTE( "c45-13.30", 0x080000, 0x20000, 0x69b82af6 )
	ROM_LOAD16_BYTE( "c45-17.42", 0x080001, 0x20000, 0x0dbee000 )
	ROM_LOAD16_BYTE( "c45-14.31", 0x0c0000, 0x20000, 0x481b6f29 )
	ROM_LOAD16_BYTE( "c45-18.43", 0x0c0001, 0x20000, 0xba7ecf3b )

	ROM_REGION( 0xa0000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c45-01.5",  0x00000, 0x20000, 0x052997b2 )
	ROM_LOAD( "c45-02.4",  0x20000, 0x20000, 0x2fc0a88e )
	ROM_LOAD( "c45-03.3",  0x40000, 0x20000, 0x0e1e5b5f )
	ROM_LOAD( "c45-04.2",  0x60000, 0x20000, 0x6d081044 )
	ROM_LOAD( "c45-05.1",  0x80000, 0x20000, 0x6c59a808 )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c45-06.31", 0x00000, 0x20000, 0x6a7976d4 )

	ROM_REGION( 0x10000, REGION_USER1, 0 )

	/* TMS320C25 code ? */
	/* Interleaved pair, $e3f words of code */
	ROM_LOAD16_BYTE( "c45-24.6",  0x00000, 0x02000, 0x1013d937 )
	ROM_LOAD16_BYTE( "c45-25.35", 0x00001, 0x02000, 0xc0d39f95 )

	/* Readme says 7 pals on video board and 6 on cpu board */
ROM_END


static void init_ainferno(void)
{
#if 1
	data16_t *ROM = (data16_t *)memory_region(REGION_CPU1);

/* code at $569a tests to see if co processor has zeroed a memory
   word at $910000... if it doesn't fairly soon then TMS ERROR is
   reported */

	ROM[0x56a0/2] = 0x4e75;		/* rts */
#endif

	state_save_register_int("sound1", 0, "sound region", &banknum);
	state_save_register_func_postload(reset_sound_region);
}


static void init_topland(void)
{
#if 1
	data16_t *ROM = (data16_t *)memory_region(REGION_CPU1);

/* code at $8a0? tests to see if co processor has zeroed a memory
   word at $910000... if it doesn't fairly soon then TMS ERROR is
   reported */
	ROM[0x8ae/2] = 0x4e75;		/* rts */

/* remove 0-0x3ffff rom checksum check */
	ROM[0x4a0/2] = 0x4e71;		/* nop */
	ROM[0x4a2/2] = 0x4e71;

/* code here hangs while word at $910000 is 0xffff */
	ROM[0xe1ce/2] = 0x4e71;
#endif

	state_save_register_int("sound1", 0, "sound region", &banknum);
	state_save_register_func_postload(reset_sound_region);
}


/*  ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR  COMPANY  FULLNAME */
GAMEX( 1988, topland,  0,        airsys,   topland,  topland,  ROT0,    "Taito Corporation Japan", "Top Landing (World)", GAME_NOT_WORKING )
GAMEX( 1990, ainferno, 0,        airsys,   ainferno, ainferno, ROT0,    "Taito America Corporation", "Air Inferno (US)", GAME_NOT_WORKING )
