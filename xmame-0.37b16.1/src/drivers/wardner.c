/***************************************************************************

		ToaPlan game hardware from 1987
		--------------------------------
		Driver by: Quench


Supported games:

	Toaplan Board Number:	TP-009
	Taito Game Number:		B25
		Wardners Forest (World)
		Pyros			(USA)
		Wardna no Mori	(Japan)

Notes:
		Basically the same video and machine hardware as Flying shark,
		  except for the Main CPU which is a Z80 here.
		See twincobr.c machine and video drivers to complete the
		  hardware setup.
		Also see Input Port definition header below, for instructions
		  on how to enter test mode.

**************************** Memory & I/O Maps *****************************
Z80:(0)  Main CPU
0000-6fff Main ROM
7000-7fff Main RAM
8000-ffff Level and scenery ROMS. This is banked with the following
8000-8fff Sprite RAM
a000-adff Pallette RAM
ae00-afff Spare unused, but tested Pallette RAM
c000-c7ff Sound RAM - shared with C000-C7FF in Z80(1) RAM

in:
50		DSW A
52		DSW B
54		Player 1 controls
56		Player 2 controls
58		VBlank (bit 7) and coin-in/start inputs
60		LSB data from char display layer
61		MSB data from char display layer
62		LSB data from BG   display layer
63		MSB data from BG   display layer
64		LSB data from FG   display layer
65		MSB data from FG   display layer

out:
00		6845 CRTC offset register
02		6845 CRTC register data
10		char scroll LSB   < Y >
11		char scroll MSB   < Y >
12		char scroll LSB     X
13		char scroll MSB     X
14		char LSB RAM offset     20h * 40h  (0-07ff) and (4000-47ff) ???
15		char MSB RAM offset
20		BG   scroll LSB   < Y >
21		BG   scroll MSB   < Y >
22		BG   scroll LSB     X
23		BG   scroll MSB     X
24		BG   LSB RAM offset     40h * 40h  (0-0fff)
25		BG   MSB RAM offset
30		FG   scroll LSB   < Y >
31		FG   scroll MSB   < Y >
32		FG   scroll LSB     X
33		FG   scroll MSB     X
34		FG   LSB RAM offset     40h * 40h  (0-0fff)
35		FG   MSB RAM offset
40		spare scroll LSB  < Y >  (Not used)
41		spare scroll MSB  < Y >  (Not used)
5a-5c	Control registers
		bits 7-4 always 0
		bits 3-1 select the control signal to drive.
		bit   0  is the value passed to the control signal.
5a		data
		00-01	INT line to TMS320C10 DSP (Active low trigger)
		0c-0d	lockout for coin A input (Active low lockout)
		0e-0f	lockout for coin B input (Active low lockout)
5c		data
		00-01	???
		02-03	???
		04-05	Active low INTerrupt to Z80(0) for screen refresh
		06-07	Flip Screen (Active high flips)
		08-09	Background RAM display bank switch
		0a-0b	Foreground ROM display bank switch (not used here)
		0c-0d	??? (what the hell does this do ?)
60		LSB data to char display layer
61		MSB data to char display layer
62		LSB data to BG   display layer
63		MSB data to BG   display layer
64		LSB data to FG   display layer
65		MSB data to FG   display layer
70		ROM bank selector for Z80(0) address 8000-ffff
		data
		00  switch ROM from 8000-ffff out, and put sprite/palette/sound RAM back.
		02  switch lower half of B25-18.ROM  ROM to 8000-ffff
		03  switch upper half of B25-18.ROM  ROM to 8000-ffff
		04  switch lower half of B25-19.ROM  ROM to 8000-ffff
		05  switch upper half of B25-19.ROM  ROM to 8000-ffff
		07  switch               B25-30.ROM  ROM to 8000-ffff



Z80:(1)  Sound CPU
0000-7fff Main ROM
8000-807f RAM ???
c000-cfff Sound RAM , $C000-C7FF shared with $C000-C7FF in Z80(0) ram



TMS320C10 DSP: Harvard type architecture. RAM and ROM on seperate data buses.
0000-05ff ROM 16-bit opcodes (word access only). Moved to $8000-8bff for
				 MAME compatibility. View this ROM in the debugger at $8000h
0000-0090 Internal RAM (words).

in:
01		data read from addressed Z80:(0) address space (Main RAM/Sprite RAM)

out:
00		address of Z80:(0) to read/write to
01		data to write to addressed Z80:(0) address space (Main RAM/Sprite RAM)
03		bit 15 goes to BIO line of TMS320C10. BIO is a polled input line.


***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/crtc6845.h"
#include "cpu/tms32010/tms32010.h"



/******************** Machine stuff **********************/
void wardner_reset(void);
READ_HANDLER( wardner_mainram_r );
WRITE_HANDLER( wardner_mainram_w );
WRITE_HANDLER( wardner_control_w );
WRITE_HANDLER( wardner_coin_dsp_w );
READ16_HANDLER( twincobr_dsp_r );
WRITE16_HANDLER( twincobr_dsp_w );

static int wardner_membank = 0;
extern int twincobr_intenable;

data8_t *wardner_sharedram;
data8_t *wardner_spare_pal_ram;

extern data8_t *wardner_mainram;


/******************** Video stuff **********************/
WRITE_HANDLER( wardner_videoram_w );
READ_HANDLER ( wardner_videoram_r );
WRITE_HANDLER( wardner_bglayer_w );
WRITE_HANDLER( wardner_fglayer_w );
WRITE_HANDLER( wardner_txlayer_w );
WRITE_HANDLER( wardner_bgscroll_w );
WRITE_HANDLER( wardner_fgscroll_w );
WRITE_HANDLER( wardner_txscroll_w );
WRITE_HANDLER( wardner_exscroll_w );

int  toaplan0_vh_start(void);
void toaplan0_vh_stop(void);
void toaplan0_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void toaplan0_eof_callback(void);

extern int twincobr_display_on;



static int wardner_interrupt(void)
{
	if (twincobr_intenable) {
		twincobr_intenable = 0;
		return interrupt();
	}
	else return ignore_interrupt();
}


static WRITE_HANDLER( CRTC_reg_sel_w )
{
	crtc6845_address_w(offset, data);
}

static WRITE_HANDLER( CRTC_data_w )
{
	crtc6845_register_w(0, data);
	twincobr_display_on = 1;
}

static READ_HANDLER( wardner_sprite_r )
{
	int shift = (offset & 1) * 8;
	return spriteram16[offset/2] >> shift;
}

static WRITE_HANDLER( wardner_sprite_w )
{
	if (offset & 1)
		spriteram16[offset/2] = (spriteram16[offset/2] & 0x00ff) | (data << 8);
	else
		spriteram16[offset/2] = (spriteram16[offset/2] & 0xff00) | data;
}

static READ_HANDLER( wardner_sharedram_r )
{
	return wardner_sharedram[offset];
}

static WRITE_HANDLER( wardner_sharedram_w )
{
	wardner_sharedram[offset] = data;
}

static READ_HANDLER( wardner_spare_pal_ram_r )
{
	return wardner_spare_pal_ram[offset];
}

static WRITE_HANDLER( wardner_spare_pal_ram_w )
{
	wardner_spare_pal_ram[offset] = data;
}

static READ_HANDLER( wardner_ram_rom_r )
{
	int wardner_data = 0;

	if (wardner_membank == 0)
	{
		int wardner_bank = offset + 0x8000;
		offset &= 0xfff;

		switch (wardner_bank & 0xe000)
		{
			case 0x8000: wardner_data = wardner_sprite_r(offset); break;
			case 0xa000: if (offset < 0xe00) { wardner_data = paletteram_r(offset); }
						 else { wardner_data = wardner_spare_pal_ram_r(offset & 0x1ff); }
						 break;
			case 0xc000: if (offset < 0x800) { wardner_data = wardner_sharedram_r(offset & 0x7ff); }
						 break;
			default:	 break;
		}
	}
	else
	{
		unsigned char *wardner_rom = memory_region(REGION_CPU1);
		int wardner_rombank = 0x8000 * wardner_membank;

		wardner_data = wardner_rom[wardner_rombank + offset];
	}
	return wardner_data;
}

static WRITE_HANDLER( wardner_ramrom_banks_w )
{
	wardner_membank = data;
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x6fff, MRA_ROM },			/* Main CPU ROM code */
	{ 0x7000, 0x7fff, wardner_mainram_r },	/* Main RAM */
	{ 0x8000, 0xffff, wardner_ram_rom_r },	/* Overlapped RAM/Banked ROM - See below */
	/* memory layout in bank 0 is really as follows */
/*	{ 0x8000, 0x8fff, wardner_sprite_r },	   Sprite RAM data    */
/*	{ 0x9000, 0x9fff, MRA_ROM },			   Banked ROM    */
/*	{ 0xa000, 0xadff, paletteram_r },		   Palette RAM    */
/*	{ 0xae00, 0xafff, MRA_BANK2 },			   Unused Palette RAM    */
/*	{ 0xb000, 0xbfff, MRA_ROM },			   Banked ROM    */
/*	{ 0xc000, 0xc7ff, MRA_BANK3 },			   Shared RAM with Sound CPU RAM    */
/*	{ 0xc800, 0xffff, MRA_ROM },			   Banked ROM    */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0x7000, 0x7fff, wardner_mainram_w, &wardner_mainram },
	{ 0x8000, 0x8fff, wardner_sprite_w, (data8_t **)&spriteram16, &spriteram_size },
	{ 0x9000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xadff, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },
	{ 0xae00, 0xafff, wardner_spare_pal_ram_w, &wardner_spare_pal_ram },
	{ 0xc000, 0xc7ff, wardner_sharedram_w, &wardner_sharedram },
	{ 0xb000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x807f, MRA_BANK4 },
	{ 0xc000, 0xc7ff, wardner_sharedram_r },
	{ 0xc800, 0xcfff, MRA_BANK5 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x807f, MWA_BANK4 },
	{ 0xc000, 0xc7ff, wardner_sharedram_w },
	{ 0xc800, 0xcfff, MWA_BANK5 },
MEMORY_END

static MEMORY_READ16_START( DSP_readmem )
	{ 0x0000, 0x011f, MRA16_RAM },	/* 90h words internal RAM */
	{ 0x8000, 0x8bff, MRA16_ROM },	/* 600h words. The real DSPs ROM is at */
									/* address 0 */
									/* View it at 8000h in the debugger */
MEMORY_END

static MEMORY_WRITE16_START( DSP_writemem )
	{ 0x0000, 0x011f, MWA16_RAM },
	{ 0x8000, 0x8bff, MWA16_ROM },
MEMORY_END


static PORT_READ_START( readport )
	{ 0x50, 0x50, input_port_3_r },			/* DSW A */
	{ 0x52, 0x52, input_port_4_r },			/* DSW B */
	{ 0x54, 0x54, input_port_1_r },			/* Player 1 */
	{ 0x56, 0x56, input_port_2_r },			/* Player 2 */
	{ 0x58, 0x58, input_port_0_r },			/* V-Blank/Coin/Start */
	{ 0x60, 0x65, wardner_videoram_r },		/* data from video layer RAM */
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, CRTC_reg_sel_w },
	{ 0x02, 0x02, CRTC_data_w },
	{ 0x10, 0x13, wardner_txscroll_w },		/* scroll text layer */
	{ 0x14, 0x15, wardner_txlayer_w },		/* offset in text video RAM */
	{ 0x20, 0x23, wardner_bgscroll_w },		/* scroll bg layer */
	{ 0x24, 0x25, wardner_bglayer_w },		/* offset in bg video RAM */
	{ 0x30, 0x33, wardner_fgscroll_w },		/* scroll fg layer */
	{ 0x34, 0x35, wardner_fglayer_w },		/* offset in fg video RAM */
	{ 0x40, 0x43, wardner_exscroll_w },		/* scroll extra layer (not used) */
	{ 0x60, 0x65, wardner_videoram_w },		/* data for video layer RAM */
	{ 0x5a, 0x5a, wardner_coin_dsp_w },		/* Machine system control */
	{ 0x5c, 0x5c, wardner_control_w },		/* Machine system control */
	{ 0x70, 0x70, wardner_ramrom_banks_w },	/* ROM bank select */
PORT_END

static PORT_READ_START( sound_readport )
	{ 0x00, 0x00, YM3812_status_port_0_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x01, 0x01, YM3812_write_port_0_w },
PORT_END

static PORT_READ16_START( DSP_readport )
	{ 0x02, 0x03, twincobr_dsp_r },
PORT_END
static PORT_WRITE16_START( DSP_writeport )
	{ 0x00,  0x07, twincobr_dsp_w },
PORT_END



/*****************************************************************************
	Input Port definitions

	There is a test mode for button/switch tests. To enter Test mode,
	set the Cross Hatch Pattern DSW to on, restart and then press
	player 1 start button when in the cross-hatch screen.
*****************************************************************************/

#define  WARDNER_PLAYER_INPUT( player )										 \
	PORT_START 				/* Player 1 button 3 skips video RAM tests */	 \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | player ) \
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | player ) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | player ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | player ) \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | player)	/* Fire */		 \
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | player)	/* Jump */		 \
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | player)	/* Shot C */	 \
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON4 | player)	/* Shot D */

#define  WARDNER_SYSTEM_INPUTS												\
	PORT_START				/* test button doesnt seem to do anything ? */	\
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )		/* Service button */	\
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )								\
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Test button */		\
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )								\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )								\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )							\
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )							\
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )	/* V-Blank */

#define  PYROS_DSW_A									\
	PORT_START		/* DSW A */							\
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )		\
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )		\
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )		\
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )	\
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )			\
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )			\
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )	\
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )			\
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )			\
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )	\
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )			\
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )			\
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )		\
	PORT_DIPSETTING(	0x20, DEF_STR( 2C_1C ) )		\
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )		\
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_3C ) )		\
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_2C ) )		\
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )		\
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ) )		\
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )		\
	PORT_DIPSETTING(	0xc0, DEF_STR( 2C_3C ) )		\
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_2C ) )

#define  WARDNER_DSW_B									\
	PORT_START		/* DSW B */							\
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )	\
	PORT_DIPSETTING(	0x01, "Easy" )					\
	PORT_DIPSETTING(	0x00, "Normal" )				\
	PORT_DIPSETTING(	0x02, "Hard" )					\
	PORT_DIPSETTING(	0x03, "Hardest" )				\
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )	\
	PORT_DIPSETTING(	0x00, "30000 & 80000" )			\
	PORT_DIPSETTING(	0x04, "50000 & 100000" )		\
	PORT_DIPSETTING(	0x08, "30000" )					\
	PORT_DIPSETTING(	0x0c, "50000" )					\
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )		\
	PORT_DIPSETTING(	0x30, "1" )						\
	PORT_DIPSETTING(	0x00, "3" )						\
	PORT_DIPSETTING(	0x10, "4" )						\
	PORT_DIPSETTING(	0x20, "5" )						\
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		\
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )			\
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )			\
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )		\
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )			\
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )



INPUT_PORTS_START( wardner )
	WARDNER_SYSTEM_INPUTS
	WARDNER_PLAYER_INPUT( IPF_PLAYER1 )
	WARDNER_PLAYER_INPUT( IPF_PLAYER2 )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_6C ) )

	WARDNER_DSW_B
INPUT_PORTS_END

INPUT_PORTS_START( pyros )
	WARDNER_SYSTEM_INPUTS
	WARDNER_PLAYER_INPUT( IPF_PLAYER1 )
	WARDNER_PLAYER_INPUT( IPF_PLAYER2 )
	PYROS_DSW_A

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "30000 & 80000" )
	PORT_DIPSETTING(	0x04, "50000 & 100000" )
	PORT_DIPSETTING(	0x08, "50000" )
	PORT_DIPSETTING(	0x0c, "100000" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPSETTING(	0x20, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(	0x40, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( wardnerj )
	WARDNER_SYSTEM_INPUTS
	WARDNER_PLAYER_INPUT( IPF_PLAYER1 )
	WARDNER_PLAYER_INPUT( IPF_PLAYER2 )
	PYROS_DSW_A
	WARDNER_DSW_B
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	3,		/* 3 bits per pixel */
	{ 0*2048*8*8, 1*2048*8*8, 2*2048*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	8,8,	/* 8*8 tiles */
	4096,	/* 4096 tiles */
	4,		/* 4 bits per pixel */
	{ 0*4096*8*8, 1*4096*8*8, 2*4096*8*8, 3*4096*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every tile takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,		/* 4 bits per pixel */
	{ 0*2048*32*8, 1*2048*32*8, 2*2048*32*8, 3*2048*32*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


/* handler called by the 3812 emulator when the internal timers cause an IRQ */
static void irqhandler(int linestate)
{
	cpu_set_irq_line(1,0,linestate);
}

static struct YM3812interface ym3812_interface =
{
	1,				/* 1 chip */
	24000000/7,		/* 3.43 MHz ??? */
	{ 100 },		/* volume */
	{ irqhandler },
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout,   1536, 32 },	/* colors 1536-1791 */
	{ REGION_GFX2, 0x00000, &tilelayout,   1280, 16 },	/* colors 1280-1535 */
	{ REGION_GFX3, 0x00000, &tilelayout,   1024, 16 },	/* colors 1024-1079 */
	{ REGION_GFX4, 0x00000, &spritelayout,    0, 64 },	/* colors    0-1023 */
	{ -1 } /* end of array */
};


static struct MachineDriver machine_driver_wardner =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			24000000/4,			/* 6 MHz ??? - Real board crystal is 24MHz */
			readmem,writemem,
			readport, writeport,
			wardner_interrupt, 1
		},
		{
			CPU_Z80,
			24000000/7,			/* 3.43 MHz ??? */
			sound_readmem, sound_writemem,
			sound_readport, sound_writeport,
			ignore_interrupt, 0	/* IRQs are caused by the YM3812 */
		},
		{
			CPU_TMS320C10,
			24000000/7,			/* 3.43 MHz ??? */
			DSP_readmem, DSP_writemem,
			DSP_readport, DSP_writeport,
			ignore_interrupt, 0	/* IRQs are caused by Z80(0) */
		}
	},
	56, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	100,									/* 100 CPU slices per frame */
	wardner_reset,

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1792, 1792,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_BUFFERS_SPRITERAM,
	toaplan0_eof_callback,
	toaplan0_vh_start,
	toaplan0_vh_stop,
	toaplan0_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		}
	}
};




/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START( wardner )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* Banked Main Z80 code */
	ROM_LOAD( "wardner.17", 0x00000, 0x08000, 0xc5dd56fd )	/* Main Z80 code */
	ROM_LOAD( "b25-18.rom", 0x10000, 0x10000, 0x9aab8ee2 )	/* OBJ ROMs */
	ROM_LOAD( "b25-19.rom", 0x20000, 0x10000, 0x95b68813 )
	ROM_LOAD( "wardner.20", 0x38000, 0x08000, 0x347f411b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "b25-16.rom", 0x00000, 0x08000, 0xe5202ff8 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* Space for Co-Processor TMS320C10 */

	ROM_REGION( 0x1000, REGION_USER1, 0 )	/* Co-Processor TMS320C10 MCU code */
	ROM_LOAD_NIB_HIGH( "82s137.1d",  0x0000, 0x0400, 0xcc5b3f53 ) /* msb */
	ROM_LOAD_NIB_LOW ( "82s137.1e",  0x0000, 0x0400, 0x47351d55 )
	ROM_LOAD_NIB_HIGH( "82s131.3b",  0x0400, 0x0200, 0x9dfffaff )
	ROM_LOAD_NIB_LOW ( "82s131.3a",  0x0400, 0x0200, 0x712bad47 )
	ROM_LOAD_NIB_HIGH( "82s137.3d",  0x0800, 0x0400, 0x70b537b9 ) /* lsb */
	ROM_LOAD_NIB_LOW ( "82s137.3e",  0x0800, 0x0400, 0x6edb2de8 )
	ROM_LOAD_NIB_HIGH( "82s131.2a",  0x0c00, 0x0200, 0xac843ca6 )
	ROM_LOAD_NIB_LOW ( "82s131.1a",  0x0c00, 0x0200, 0x50452ff8 )

	ROM_REGION( 0x0c000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "wardner.07", 0x00000, 0x04000, 0x1392b60d )
	ROM_LOAD( "wardner.06", 0x04000, 0x04000, 0x0ed848da )
	ROM_LOAD( "wardner.05", 0x08000, 0x04000, 0x79792c86 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )	/* fg tiles */
	ROM_LOAD( "b25-12.rom",  0x00000, 0x08000, 0x15d08848 )
	ROM_LOAD( "b25-15.rom",  0x08000, 0x08000, 0xcdd2d408 )
	ROM_LOAD( "b25-14.rom",  0x10000, 0x08000, 0x5a2aef4f )
	ROM_LOAD( "b25-13.rom",  0x18000, 0x08000, 0xbe21db2b )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )	/* bg tiles */
	ROM_LOAD( "b25-08.rom",  0x00000, 0x08000, 0x883ccaa3 )
	ROM_LOAD( "b25-11.rom",  0x08000, 0x08000, 0xd6ebd510 )
	ROM_LOAD( "b25-10.rom",  0x10000, 0x08000, 0xb9a61e81 )
	ROM_LOAD( "b25-09.rom",  0x18000, 0x08000, 0x585411b7 )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "b25-01.rom",  0x00000, 0x10000, 0x42ec01fb )
	ROM_LOAD( "b25-02.rom",  0x10000, 0x10000, 0x6c0130b7 )
	ROM_LOAD( "b25-03.rom",  0x20000, 0x10000, 0xb923db99 )
	ROM_LOAD( "b25-04.rom",  0x30000, 0x10000, 0x8059573c )

	ROM_REGION( 0x260, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "82s129.b19",  0x000, 0x100, 0x24e7d62f )	/* sprite priority control ?? */
	ROM_LOAD( "82s129.b18",  0x100, 0x100, 0xa50cef09 )	/* sprite priority control ?? */
	ROM_LOAD( "82s123.b21",  0x200, 0x020, 0xf72482db )	/* sprite control ?? */
	ROM_LOAD( "82s123.c6",   0x220, 0x020, 0xbc88cced )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "82s123.f1",   0x240, 0x020, 0x4fb5df2a )	/* tile to sprite priority ?? */
ROM_END

ROM_START( pyros )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* Banked Z80 code */
	ROM_LOAD( "b25-29.rom", 0x00000, 0x08000, 0xb568294d )	/* Main Z80 code */
	ROM_LOAD( "b25-18.rom", 0x10000, 0x10000, 0x9aab8ee2 )	/* OBJ ROMs */
	ROM_LOAD( "b25-19.rom", 0x20000, 0x10000, 0x95b68813 )
	ROM_LOAD( "b25-30.rom", 0x38000, 0x08000, 0x5056c799 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "b25-16.rom", 0x00000, 0x08000, 0xe5202ff8 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* Space for Co-Processor TMS320C10 */

	ROM_REGION( 0x1000, REGION_USER1, 0 )	/* Co-Processor TMS320C10 MCU code */
	ROM_LOAD_NIB_HIGH( "82s137.1d",  0x0000, 0x0400, 0xcc5b3f53 ) /* msb */
	ROM_LOAD_NIB_LOW ( "82s137.1e",  0x0000, 0x0400, 0x47351d55 )
	ROM_LOAD_NIB_HIGH( "82s131.3b",  0x0400, 0x0200, 0x9dfffaff )
	ROM_LOAD_NIB_LOW ( "82s131.3a",  0x0400, 0x0200, 0x712bad47 )
	ROM_LOAD_NIB_HIGH( "82s137.3d",  0x0800, 0x0400, 0x70b537b9 ) /* lsb */
	ROM_LOAD_NIB_LOW ( "82s137.3e",  0x0800, 0x0400, 0x6edb2de8 )
	ROM_LOAD_NIB_HIGH( "82s131.2a",  0x0c00, 0x0200, 0xac843ca6 )
	ROM_LOAD_NIB_LOW ( "82s131.1a",  0x0c00, 0x0200, 0x50452ff8 )

	ROM_REGION( 0x0c000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "b25-35.rom", 0x00000, 0x04000, 0xfec6f0c0 )
	ROM_LOAD( "b25-34.rom", 0x04000, 0x04000, 0x02505dad )
	ROM_LOAD( "b25-33.rom", 0x08000, 0x04000, 0x9a55fcb9 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )	/* fg tiles */
	ROM_LOAD( "b25-12.rom",  0x00000, 0x08000, 0x15d08848 )
	ROM_LOAD( "b25-15.rom",  0x08000, 0x08000, 0xcdd2d408 )
	ROM_LOAD( "b25-14.rom",  0x10000, 0x08000, 0x5a2aef4f )
	ROM_LOAD( "b25-13.rom",  0x18000, 0x08000, 0xbe21db2b )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )	/* bg tiles */
	ROM_LOAD( "b25-08.rom",  0x00000, 0x08000, 0x883ccaa3 )
	ROM_LOAD( "b25-11.rom",  0x08000, 0x08000, 0xd6ebd510 )
	ROM_LOAD( "b25-10.rom",  0x10000, 0x08000, 0xb9a61e81 )
	ROM_LOAD( "b25-09.rom",  0x18000, 0x08000, 0x585411b7 )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "b25-01.rom",  0x00000, 0x10000, 0x42ec01fb )
	ROM_LOAD( "b25-02.rom",  0x10000, 0x10000, 0x6c0130b7 )
	ROM_LOAD( "b25-03.rom",  0x20000, 0x10000, 0xb923db99 )
	ROM_LOAD( "b25-04.rom",  0x30000, 0x10000, 0x8059573c )

	ROM_REGION( 0x260, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "82s129.b19",  0x000, 0x100, 0x24e7d62f )	/* sprite priority control ?? */
	ROM_LOAD( "82s129.b18",  0x100, 0x100, 0xa50cef09 )	/* sprite priority control ?? */
	ROM_LOAD( "82s123.b21",  0x200, 0x020, 0xf72482db )	/* sprite control ?? */
	ROM_LOAD( "82s123.c6",   0x220, 0x020, 0xbc88cced )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "82s123.f1",   0x240, 0x020, 0x4fb5df2a )	/* tile to sprite priority ?? */
ROM_END

ROM_START( wardnerj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* Banked Z80 code */
	ROM_LOAD( "b25-17.bin",  0x00000, 0x08000, 0x4164dca9 )	/* Main Z80 code */
	ROM_LOAD( "b25-18.rom",  0x10000, 0x10000, 0x9aab8ee2 )	/* OBJ ROMs */
	ROM_LOAD( "b25-19.rom",  0x20000, 0x10000, 0x95b68813 )
	ROM_LOAD( "b25-20.bin",  0x38000, 0x08000, 0x1113ad38 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "b25-16.rom", 0x00000, 0x08000, 0xe5202ff8 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* Space for Co-Processor TMS320C10 */

	ROM_REGION( 0x1000, REGION_USER1, 0 )	/* Co-Processor TMS320C10 MCU code */
	ROM_LOAD_NIB_HIGH( "82s137.1d",  0x0000, 0x0400, 0xcc5b3f53 ) /* msb */
	ROM_LOAD_NIB_LOW ( "82s137.1e",  0x0000, 0x0400, 0x47351d55 )
	ROM_LOAD_NIB_HIGH( "82s131.3b",  0x0400, 0x0200, 0x9dfffaff )
	ROM_LOAD_NIB_LOW ( "82s131.3a",  0x0400, 0x0200, 0x712bad47 )
	ROM_LOAD_NIB_HIGH( "82s137.3d",  0x0800, 0x0400, 0x70b537b9 ) /* lsb */
	ROM_LOAD_NIB_LOW ( "82s137.3e",  0x0800, 0x0400, 0x6edb2de8 )
	ROM_LOAD_NIB_HIGH( "82s131.2a",  0x0c00, 0x0200, 0xac843ca6 )
	ROM_LOAD_NIB_LOW ( "82s131.1a",  0x0c00, 0x0200, 0x50452ff8 )

	ROM_REGION( 0x0c000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "b25-07.bin", 0x00000, 0x04000, 0x50e329e0 )
	ROM_LOAD( "b25-06.bin", 0x04000, 0x04000, 0x3bfeb6ae )
	ROM_LOAD( "b25-05.bin", 0x08000, 0x04000, 0xbe36a53e )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )	/* fg tiles */
	ROM_LOAD( "b25-12.rom",  0x00000, 0x08000, 0x15d08848 )
	ROM_LOAD( "b25-15.rom",  0x08000, 0x08000, 0xcdd2d408 )
	ROM_LOAD( "b25-14.rom",  0x10000, 0x08000, 0x5a2aef4f )
	ROM_LOAD( "b25-13.rom",  0x18000, 0x08000, 0xbe21db2b )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )	/* bg tiles */
	ROM_LOAD( "b25-08.rom",  0x00000, 0x08000, 0x883ccaa3 )
	ROM_LOAD( "b25-11.rom",  0x08000, 0x08000, 0xd6ebd510 )
	ROM_LOAD( "b25-10.rom",  0x10000, 0x08000, 0xb9a61e81 )
	ROM_LOAD( "b25-09.rom",  0x18000, 0x08000, 0x585411b7 )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "b25-01.rom",  0x00000, 0x10000, 0x42ec01fb )
	ROM_LOAD( "b25-02.rom",  0x10000, 0x10000, 0x6c0130b7 )
	ROM_LOAD( "b25-03.rom",  0x20000, 0x10000, 0xb923db99 )
	ROM_LOAD( "b25-04.rom",  0x30000, 0x10000, 0x8059573c )

	ROM_REGION( 0x260, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "82s129.b19",  0x000, 0x100, 0x24e7d62f )	/* sprite priority control ?? */
	ROM_LOAD( "82s129.b18",  0x100, 0x100, 0xa50cef09 )	/* sprite priority control ?? */
	ROM_LOAD( "82s123.b21",  0x200, 0x020, 0xf72482db )	/* sprite control ?? */
	ROM_LOAD( "82s123.c6",   0x220, 0x020, 0xbc88cced )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "82s123.f1",   0x240, 0x020, 0x4fb5df2a )	/* tile to sprite priority ?? */
ROM_END


static void init_wardner(void)
{
	data8_t *source = memory_region(REGION_USER1);
	data16_t *dest = (data16_t *)&memory_region(REGION_CPU3)[TMS320C10_PGM_OFFSET];
	int A;

	/* The ROM loader fixes the nibble images. Here we fix the byte ordering. */
	for (A = 0;A < 0x0600;A++)
		dest[A] = (source[A] << 8) | source[A + 0x800];
}



GAME( 1987, wardner,  0,       wardner, wardner,  wardner, ROT0, "[Toaplan] Taito Corporation Japan", "Wardner (World)" )
GAME( 1987, pyros,    wardner, wardner, pyros,    wardner, ROT0, "[Toaplan] Taito America Corporation", "Pyros (US)" )
GAME( 1987, wardnerj, wardner, wardner, wardnerj, wardner, ROT0, "[Toaplan] Taito Corporation", "Wardna no Mori (Japan)" )

