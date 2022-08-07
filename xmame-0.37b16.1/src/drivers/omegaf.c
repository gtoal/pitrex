/***************************************************************************

Omega Fighter
----------------------
driver by Yochizo

This driver is heavily dependent on the Raine source.
Very thanks to Richard Bush and the Raine team.


Supported games :
==================
 Omega Fighter     (C) 1989 UPL
 Atomic Robokid    (C) 1988 UPL

Known issues :
================
 - Dip switch settings in Atomic Robokid may be wrong.
 - Cocktail mode has not been supported yet.
 - Omega Fighter has a input protection. Currently it is hacked instead
   of emulated.
 - I don't know if Omega Fighter uses sprite overdraw flag or not.
 - Sometimes sprites stays behind the screen in Atomic Robokid due to
   incomplete sprite overdraw emulation.
 - Currently it has not been implemented palette marking in sprite
   overdraw mode.
 - When RAM and ROM check and color test mode, the palette is overflows.
   16 bit color is needed ?

TODO :
========
 - Correct dip switch settings in Atomic Robokid
 - Support cocktail mode
 - Emulate input protection for Omega Fighter.

NOTE :
========
 - To skip dip setting display, press 1P + 2P start in Atomic Robokid.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/**************************************************************************
  Variables
**************************************************************************/

extern unsigned char *omegaf_fg_videoram;
extern size_t omegaf_fgvideoram_size;

extern unsigned char *omegaf_bg0_scroll_x;
extern unsigned char *omegaf_bg1_scroll_x;
extern unsigned char *omegaf_bg2_scroll_x;
extern unsigned char *omegaf_bg0_scroll_y;
extern unsigned char *omegaf_bg1_scroll_y;
extern unsigned char *omegaf_bg2_scroll_y;

WRITE_HANDLER( omegaf_bg0_bank_w );
WRITE_HANDLER( omegaf_bg1_bank_w );
WRITE_HANDLER( omegaf_bg2_bank_w );
READ_HANDLER( omegaf_bg0_videoram_r );
READ_HANDLER( omegaf_bg1_videoram_r );
READ_HANDLER( omegaf_bg2_videoram_r );
WRITE_HANDLER( omegaf_bg0_videoram_w );
WRITE_HANDLER( omegaf_bg1_videoram_w );
WRITE_HANDLER( omegaf_bg2_videoram_w );
WRITE_HANDLER( robokid_bg0_videoram_w );
WRITE_HANDLER( robokid_bg1_videoram_w );
WRITE_HANDLER( robokid_bg2_videoram_w );
WRITE_HANDLER( omegaf_bg0_scrollx_w );
WRITE_HANDLER( omegaf_bg1_scrollx_w );
WRITE_HANDLER( omegaf_bg2_scrollx_w );
WRITE_HANDLER( omegaf_bg0_scrolly_w );
WRITE_HANDLER( omegaf_bg1_scrolly_w );
WRITE_HANDLER( omegaf_bg2_scrolly_w );
WRITE_HANDLER( omegaf_fgvideoram_w );
WRITE_HANDLER( omegaf_bg0_enabled_w );
WRITE_HANDLER( omegaf_bg1_enabled_w );
WRITE_HANDLER( omegaf_bg2_enabled_w );
WRITE_HANDLER( omegaf_sprite_overdraw_w );

int omegaf_vh_start(void);
int robokid_vh_start(void);
void omegaf_vh_stop(void);
void omegaf_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

static int omegaf_bank_latch = 2;


/**************************************************************************
  Initializers
**************************************************************************/

static void init_omegaf(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* Hack the input protection. $00 and $01 code is written to $C005 */
	/* and $C006.                                                      */

	RAM[0x029a] = 0x00;
	RAM[0x029b] = 0x00;
	RAM[0x02a6] = 0x00;
	RAM[0x02a7] = 0x00;

	RAM[0x02b2] = 0xC9;
	RAM[0x02b5] = 0xC9;
	RAM[0x02c9] = 0xC9;
	RAM[0x02f6] = 0xC9;

	RAM[0x05f0] = 0x00;
	RAM[0x054c] = 0x04;
	RAM[0x0557] = 0x03;


	/* Fix ROM check */

	RAM[0x0b8d] = 0x00;
	RAM[0x0b8e] = 0x00;
	RAM[0x0b8f] = 0x00;
}


/**************************************************************************
  Interrupts
**************************************************************************/

static int omegaf_interrupt(void)
{
	return 0x00d7;	/* RST 10h */
}


/**************************************************************************
  Inputs
**************************************************************************/

INPUT_PORTS_START( omegaf )
	PORT_START			/* Player 1 inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* Player 2 inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* System inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START			/* DSW 0 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x06, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x04, "Hardest" )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )

	PORT_START 			/* DSW 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20k" )
	PORT_DIPSETTING(    0x03, "30k" )
	PORT_DIPSETTING(    0x01, "50k" )
	PORT_DIPSETTING(    0x02, "100k" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
INPUT_PORTS_END

INPUT_PORTS_START( robokid )
	PORT_START			/* Player 1 inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* Player 2 inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* System inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START			/* DSW 0 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "50k, then every 100k" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START 			/* DSW 1 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
INPUT_PORTS_END

INPUT_PORTS_START( robokidj )
	PORT_START			/* Player 1 inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* Player 2 inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* System inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )	/* keep pressed during boot to enter service mode */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START			/* DSW 0 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "30k, then every 50k" )
	PORT_DIPSETTING(	0x00, "50k, then every 80k" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START 			/* DSW 1 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
INPUT_PORTS_END


/**************************************************************************
  Memory handlers
**************************************************************************/

static WRITE_HANDLER( omegaf_bankselect_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bankaddress;

	if ( (data & 0x0f) != omegaf_bank_latch )
	{
		omegaf_bank_latch = data & 0x0f;

		if (omegaf_bank_latch < 2)
			bankaddress = omegaf_bank_latch * 0x4000;
		else
			bankaddress = 0x10000 + ( (omegaf_bank_latch - 2) * 0x4000);
		cpu_setbank( 1, &RAM[bankaddress] );	 /* Select 16 banks of 16k */
	}
}


/**************************************************************************
  Memory maps
**************************************************************************/

static MEMORY_READ_START( omegaf_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc000, input_port_2_r },			/* system input */
	{ 0xc001, 0xc001, input_port_0_r },			/* player 1 input */
	{ 0xc002, 0xc002, input_port_1_r },			/* player 2 input */
	{ 0xc003, 0xc003, input_port_3_r },			/* DSW 1 input */
	{ 0xc004, 0xc004, input_port_4_r },			/* DSW 2 input */
	{ 0xc005, 0xc005, MRA_NOP },
	{ 0xc006, 0xc006, MRA_NOP },
	{ 0xc100, 0xc105, MRA_RAM },
	{ 0xc200, 0xc205, MRA_RAM },
	{ 0xc300, 0xc305, MRA_RAM },
	{ 0xc400, 0xc7ff, omegaf_bg0_videoram_r },	/* BG0 video RAM */
	{ 0xc800, 0xcbff, omegaf_bg1_videoram_r },	/* BG1 video RAM */
	{ 0xcc00, 0xcfff, omegaf_bg2_videoram_r },	/* BG2 video RAM */
	{ 0xd000, 0xd7ff, MRA_RAM },				/* FG RAM */
	{ 0xd800, 0xdfff, paletteram_r },			/* palette RAM */
	{ 0xe000, 0xf9ff, MRA_RAM },				/* RAM */
	{ 0xfa00, 0xffff, MRA_RAM },				/* sprite RAM */
MEMORY_END

static MEMORY_WRITE_START( omegaf_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc000, soundlatch_w },
	{ 0xc001, 0xc001, MWA_NOP },
	{ 0xc002, 0xc002, omegaf_bankselect_w },
	{ 0xc003, 0xc003, omegaf_sprite_overdraw_w },
	{ 0xc004, 0xc004, MWA_NOP },							/* input protection */
	{ 0xc005, 0xc005, MWA_NOP },							/* input protection */
	{ 0xc006, 0xc006, MWA_NOP },							/* input protection */
	{ 0xc100, 0xc101, omegaf_bg0_scrollx_w, &omegaf_bg0_scroll_x },
	{ 0xc102, 0xc103, omegaf_bg0_scrolly_w, &omegaf_bg0_scroll_y },
	{ 0xc104, 0xc104, omegaf_bg0_enabled_w },				/* BG0 enabled */
	{ 0xc105, 0xc105, omegaf_bg0_bank_w },					/* BG0 bank select */
	{ 0xc200, 0xc201, omegaf_bg1_scrollx_w, &omegaf_bg1_scroll_x },
	{ 0xc202, 0xc203, omegaf_bg1_scrolly_w, &omegaf_bg1_scroll_y },
	{ 0xc204, 0xc204, omegaf_bg1_enabled_w },				/* BG1 enabled */
	{ 0xc205, 0xc205, omegaf_bg1_bank_w },					/* BG1 bank select */
	{ 0xc300, 0xc301, omegaf_bg2_scrollx_w, &omegaf_bg2_scroll_x },
	{ 0xc302, 0xc303, omegaf_bg2_scrolly_w, &omegaf_bg2_scroll_y },
	{ 0xc304, 0xc304, omegaf_bg2_enabled_w },				/* BG2 enabled */
	{ 0xc305, 0xc305, omegaf_bg2_bank_w },					/* BG2 bank select */
	{ 0xc400, 0xc7ff, omegaf_bg0_videoram_w },				/* BG0 video RAM */
	{ 0xc800, 0xcbff, omegaf_bg1_videoram_w },				/* BG1 video RAM */
	{ 0xcc00, 0xcfff, omegaf_bg2_videoram_w },				/* BG2 video RAM */
	{ 0xd000, 0xd7ff, omegaf_fgvideoram_w, &omegaf_fg_videoram },
	{ 0xd800, 0xdfff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xe000, 0xf9ff, MWA_RAM },							/* RAM */
	{ 0xfa00, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
MEMORY_END

static MEMORY_READ_START( robokid_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc7ff, paletteram_r },			/* paletrte RAM */
	{ 0xc800, 0xcfff, MRA_RAM },				/* FG RAM */
	{ 0xd000, 0xd3ff, omegaf_bg2_videoram_r },
	{ 0xd400, 0xd7ff, omegaf_bg1_videoram_r },
	{ 0xd800, 0xdbff, omegaf_bg0_videoram_r },
	{ 0xdc00, 0xdc00, input_port_2_r },			/* system input */
	{ 0xdc01, 0xdc01, input_port_0_r },			/* player 1 input */
	{ 0xdc02, 0xdc02, input_port_1_r },			/* player 2 input */
	{ 0xdc03, 0xdc03, input_port_3_r },			/* DSW 1 input */
	{ 0xdc04, 0xdc04, input_port_4_r },			/* DSW 2 input */
	{ 0xdd00, 0xdd05, MRA_RAM },
	{ 0xde00, 0xde05, MRA_RAM },
	{ 0xdf00, 0xdf05, MRA_RAM },
	{ 0xe000, 0xf9ff, MRA_RAM },				/* RAM */
	{ 0xfa00, 0xffff, MRA_RAM },				/* sprite RAM */
MEMORY_END

static MEMORY_WRITE_START( robokid_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xc800, 0xcfff, omegaf_fgvideoram_w, &omegaf_fg_videoram },
	{ 0xd000, 0xd3ff, robokid_bg2_videoram_w },				/* BG2 video RAM */
	{ 0xd400, 0xd7ff, robokid_bg1_videoram_w },				/* BG1 video RAM */
	{ 0xd800, 0xdbff, robokid_bg0_videoram_w },				/* BG0 video RAM */
	{ 0xdc00, 0xdc00, soundlatch_w },
	{ 0xdc02, 0xdc02, omegaf_bankselect_w },
	{ 0xdc03, 0xdc03, omegaf_sprite_overdraw_w },
	{ 0xdd00, 0xdd01, omegaf_bg0_scrollx_w, &omegaf_bg0_scroll_x },
	{ 0xdd02, 0xdd03, omegaf_bg0_scrolly_w, &omegaf_bg0_scroll_y },
	{ 0xdd04, 0xdd04, omegaf_bg0_enabled_w },				/* BG0 enabled */
	{ 0xdd05, 0xdd05, omegaf_bg0_bank_w },					/* BG0 bank select */
	{ 0xde00, 0xde01, omegaf_bg1_scrollx_w, &omegaf_bg1_scroll_x },
	{ 0xde02, 0xde03, omegaf_bg1_scrolly_w, &omegaf_bg1_scroll_y },
	{ 0xde04, 0xde04, omegaf_bg1_enabled_w },				/* BG1 enabled */
	{ 0xde05, 0xde05, omegaf_bg1_bank_w },					/* BG1 bank select */
	{ 0xdf00, 0xdf01, omegaf_bg2_scrollx_w, &omegaf_bg2_scroll_x },
	{ 0xdf02, 0xdf03, omegaf_bg2_scrolly_w, &omegaf_bg2_scroll_y },
	{ 0xdf04, 0xdf04, omegaf_bg2_enabled_w },				/* BG2 enabled */
	{ 0xdf05, 0xdf05, omegaf_bg2_bank_w },					/* BG2 bank select */
	{ 0xe000, 0xf9ff, MWA_RAM },							/* RAM */
	{ 0xfa00, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
	{ 0xefee, 0xefee, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xe000, MWA_NOP },
	{ 0xeff5, 0xeff6, MWA_NOP },	/* sample frequency ??? */
	{ 0xefee, 0xefee, MWA_NOP },	/* chip command ?? */
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x0000, 0x0000, YM2203_status_port_0_r },
	{ 0x0001, 0x0001, YM2203_read_port_0_r },
	{ 0x0080, 0x0080, YM2203_status_port_1_r },
	{ 0x0081, 0x0081, YM2203_read_port_1_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x0000, 0x0000, YM2203_control_port_0_w },
	{ 0x0001, 0x0001, YM2203_write_port_0_w },
	{ 0x0080, 0x0080, YM2203_control_port_1_w },
	{ 0x0081, 0x0081, YM2203_write_port_1_w },
PORT_END


/**************************************************************************
  GFX decoding
**************************************************************************/

static struct GfxLayout omegaf_charlayout =
{
	8, 8,	/* 8x8 characters */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 32*0, 32*1, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7 },
	8*32
};

static struct GfxLayout omegaf_spritelayout =
{
	16, 16,	/* 16x16 characters */
	1024,
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
		64*8+0, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 32*0, 32*1, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7,
		32*8, 32*9, 32*10, 32*11, 32*12, 32*13, 32*14, 32*15 },
	16*64
};

static struct GfxLayout omegaf_bigspritelayout =
{
	32, 32,	/* 32x32 characters */
	256,
	4,
	{ 0, 1, 2, 3 },
	{	0, 4, 8, 12, 16, 20, 24, 28,
		64*8+0, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28,
		128*16+0, 128*16+4, 128*16+8, 128*16+12, 128*16+16, 128*16+20, 128*16+24, 128*16+28,
		128*16+64*8+0, 128*16+64*8+4, 128*16+64*8+8, 128*16+64*8+12, 128*16+64*8+16, 128*16+64*8+20, 128*16+64*8+24, 128*16+64*8+28 },
	{ 32*0, 32*1, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7,
		32*8, 32*9, 32*10, 32*11, 32*12, 32*13, 32*14, 32*15,
		64*16+32*0, 64*16+32*1, 64*16+32*2, 64*16+32*3, 64*16+32*4, 64*16+32*5, 64*16+32*6, 64*16+32*7,
		64*16+32*8, 64*16+32*9, 64*16+32*10, 64*16+32*11, 64*16+32*12, 64*16+32*13, 64*16+32*14, 64*16+32*15, 64*16+32*16 },
	16*64*4
};

static struct GfxLayout omegaf_bglayout =
{
	16, 16,	/* 16x16 characters */
	4096,
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
		64*8+0, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 32*0, 32*1, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7,
		32*8, 32*9, 32*10, 32*11, 32*12, 32*13, 32*14, 32*15 },
	16*64
};

static struct GfxLayout robokid_spritelayout =
{
	16, 16,	/* 16x16 characters */
	2048,
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
		64*8+0, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 32*0, 32*1, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7,
		32*8, 32*9, 32*10, 32*11, 32*12, 32*13, 32*14, 32*15 },
	16*64
};

static struct GfxLayout robokid_bigspritelayout =
{
	32, 32,	/* 32x32 characters */
	512,
	4,
	{ 0, 1, 2, 3 },
	{	0, 4, 8, 12, 16, 20, 24, 28,
		64*8+0, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28,
		128*16+0, 128*16+4, 128*16+8, 128*16+12, 128*16+16, 128*16+20, 128*16+24, 128*16+28,
		128*16+64*8+0, 128*16+64*8+4, 128*16+64*8+8, 128*16+64*8+12, 128*16+64*8+16, 128*16+64*8+20, 128*16+64*8+24, 128*16+64*8+28 },
	{ 32*0, 32*1, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7,
		32*8, 32*9, 32*10, 32*11, 32*12, 32*13, 32*14, 32*15,
		64*16+32*0, 64*16+32*1, 64*16+32*2, 64*16+32*3, 64*16+32*4, 64*16+32*5, 64*16+32*6, 64*16+32*7,
		64*16+32*8, 64*16+32*9, 64*16+32*10, 64*16+32*11, 64*16+32*12, 64*16+32*13, 64*16+32*14, 64*16+32*15, 64*16+32*16 },
	16*64*4
};

static struct GfxDecodeInfo omegaf_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &omegaf_bglayout,         0*16, 16},
	{ REGION_GFX2, 0, &omegaf_bglayout,         0*16, 16},
	{ REGION_GFX3, 0, &omegaf_bglayout,         0*16, 16},
	{ REGION_GFX4, 0, &omegaf_spritelayout,    32*16, 16},
	{ REGION_GFX4, 0, &omegaf_bigspritelayout, 32*16, 16},
	{ REGION_GFX5, 0, &omegaf_charlayout,      48*16, 16},
	{ -1} /* end of array */
};

static struct GfxDecodeInfo robokid_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &omegaf_bglayout,          0*16, 16},
	{ REGION_GFX2, 0, &omegaf_bglayout,          0*16, 16},
	{ REGION_GFX3, 0, &omegaf_bglayout,          0*16, 16},
	{ REGION_GFX4, 0, &robokid_spritelayout,    32*16, 16},
	{ REGION_GFX4, 0, &robokid_bigspritelayout, 32*16, 16},
	{ REGION_GFX5, 0, &omegaf_charlayout,       48*16, 16},
	{ -1} /* end of array */
};


/**************************************************************************
  Machine drivers
**************************************************************************/

static struct YM2203interface ym2203_interface =
{
	2,	 /* 2 chips */
	12000000/8,
	{ YM2203_VOL(35, 35), YM2203_VOL(35, 35)},
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct MachineDriver machine_driver_omegaf =
{
	{
		{
			CPU_Z80,
			12000000/2,		/* 12000000/2 ??? */
			omegaf_readmem, omegaf_writemem, 0, 0,	/* very sensitive to these settings */
			omegaf_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,		/* 12000000/3 ??? */
			sound_readmem,  sound_writemem,
			sound_readport, sound_writeport,
			interrupt, 2
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,									/* number of slices per frame */
	0,

	128*16, 32*16, { 0*8, 32*8-1, 4*8, 28*8-1 },
	omegaf_gfxdecodeinfo,
	64*16, 64*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	omegaf_vh_start,
	omegaf_vh_stop,
	omegaf_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

static struct MachineDriver machine_driver_robokid =
{
	{
		{
			CPU_Z80,
			12000000/2,		/* 12000000/2 ??? */
			robokid_readmem, robokid_writemem, 0, 0,	/* very sensitive to these settings */
			omegaf_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,		/* 12000000/3 ??? */
			sound_readmem,  sound_writemem,
			sound_readport, sound_writeport,
			interrupt, 2
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,									/* number of slices per frame */
	0,

	32*16, 32*16, { 0*8, 32*8-1, 4*8, 28*8-1 },
	robokid_gfxdecodeinfo,
	64*16, 64*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	robokid_vh_start,
	omegaf_vh_stop,
	omegaf_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};


/**************************************************************************
  ROM loaders
**************************************************************************/

ROM_START( omegaf )
	ROM_REGION( 0x48000, REGION_CPU1, 0 )						/* main CPU */
	ROM_LOAD( "1.5",          0x00000, 0x08000, 0x57a7fd96 )
	ROM_CONTINUE(             0x10000, 0x18000 )
	ROM_LOAD( "6.4l",         0x28000, 0x20000, 0x6277735c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )						/* sound CPU */
	ROM_LOAD( "7.7m",         0x00000, 0x10000, 0xd40fc8d5 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )		/* BG0 */
	ROM_LOAD( "2back1.27b",   0x00000, 0x80000, 0x21f8a32e )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )		/* BG1 */
	ROM_LOAD( "1back2.15b",   0x00000, 0x80000, 0x6210ddcc )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )		/* BG2 */
	ROM_LOAD( "3back3.5f",    0x00000, 0x80000, 0xc31cae56 )

	ROM_REGION( 0x20000, REGION_GFX4, ROMREGION_DISPOSE )		/* sprite */
	ROM_LOAD( "8.23m",        0x00000, 0x20000, 0x0bd2a5d1 )

	ROM_REGION( 0x08000, REGION_GFX5, ROMREGION_DISPOSE )		/* FG */
	ROM_LOAD( "4.18h",        0x00000, 0x08000, 0x9e2d8152 )
ROM_END

ROM_START( omegafs )
	ROM_REGION( 0x48000, REGION_CPU1, 0 )						/* main CPU */
	ROM_LOAD( "5.3l",         0x00000, 0x08000, 0x503a3e63 )
	ROM_CONTINUE(             0x10000, 0x18000 )
	ROM_LOAD( "6.4l",         0x28000, 0x20000, 0x6277735c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )						/* sound CPU */
	ROM_LOAD( "7.7m",         0x00000, 0x10000, 0xd40fc8d5 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )		/* BG0 */
	ROM_LOAD( "2back1.27b",   0x00000, 0x80000, 0x21f8a32e )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )		/* BG1 */
	ROM_LOAD( "1back2.15b",   0x00000, 0x80000, 0x6210ddcc )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )		/* BG2 */
	ROM_LOAD( "3back3.5f",    0x00000, 0x80000, 0xc31cae56 )

	ROM_REGION( 0x20000, REGION_GFX4, ROMREGION_DISPOSE )		/* sprite */
	ROM_LOAD( "8.23m",        0x00000, 0x20000, 0x0bd2a5d1 )

	ROM_REGION( 0x08000, REGION_GFX5, ROMREGION_DISPOSE )		/* FG */
	ROM_LOAD( "4.18h",        0x00000, 0x08000, 0x9e2d8152 )
ROM_END

ROM_START( robokid )
	ROM_REGION( 0x48000, REGION_CPU1, 0 )						/* main CPU */
	ROM_LOAD( "robokid1.18j", 0x00000, 0x08000, 0x378c21fc )
	ROM_CONTINUE(             0x10000, 0x08000 )
	ROM_LOAD( "robokid2.18k", 0x18000, 0x10000, 0xddef8c5a )
	ROM_LOAD( "robokid3.15k", 0x28000, 0x10000, 0x05295ec3 )
	ROM_LOAD( "robokid4.12k", 0x38000, 0x10000, 0x3bc3977f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )						/* sound CPU */
	ROM_LOAD( "robokid.k7",   0x00000, 0x10000, 0xf490a2e9 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )		/* BG0 */
	ROM_LOAD( "robokid.19c",  0x00000, 0x10000, 0x02220421 )
	ROM_LOAD( "robokid.20c",  0x10000, 0x10000, 0x02d59bc2 )
	ROM_LOAD( "robokid.17d",  0x20000, 0x10000, 0x2fa29b99 )
	ROM_LOAD( "robokid.18d",  0x30000, 0x10000, 0xae15ce02 )
	ROM_LOAD( "robokid.19d",  0x40000, 0x10000, 0x784b089e )
	ROM_LOAD( "robokid.20d",  0x50000, 0x10000, 0xb0b395ed )
	ROM_LOAD( "robokid.19f",  0x60000, 0x10000, 0x0f9071c6 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )		/* BG1 */
	ROM_LOAD( "robokid.12c",  0x00000, 0x10000, 0x0ab45f94 )
	ROM_LOAD( "robokid.14c",  0x10000, 0x10000, 0x029bbd4a )
	ROM_LOAD( "robokid.15c",  0x20000, 0x10000, 0x7de67ebb )
	ROM_LOAD( "robokid.16c",  0x30000, 0x10000, 0x53c0e582 )
	ROM_LOAD( "robokid.17c",  0x40000, 0x10000, 0x0cae5a1e )
	ROM_LOAD( "robokid.18c",  0x50000, 0x10000, 0x56ac7c8a )
	ROM_LOAD( "robokid.15d",  0x60000, 0x10000, 0xcd632a4d )
	ROM_LOAD( "robokid.16d",  0x70000, 0x10000, 0x18d92b2b )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )		/* BG2 */
	ROM_LOAD( "robokid.12a",  0x00000, 0x10000, 0xe64d1c10 )
	ROM_LOAD( "robokid.14a",  0x10000, 0x10000, 0x8f9371e4 )
	ROM_LOAD( "robokid.15a",  0x20000, 0x10000, 0x469204e7 )
	ROM_LOAD( "robokid.16a",  0x30000, 0x10000, 0x4e340815 )
	ROM_LOAD( "robokid.17a",  0x40000, 0x10000, 0xf0863106 )
	ROM_LOAD( "robokid.18a",  0x50000, 0x10000, 0xfdff7441 )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )		/* sprite */
	ROM_LOAD( "robokid.15f",  0x00000, 0x10000, 0xba61f5ab )
	ROM_LOAD( "robokid.16f",  0x10000, 0x10000, 0xd9b399ce )
	ROM_LOAD( "robokid.17f",  0x20000, 0x10000, 0xafe432b9 )
	ROM_LOAD( "robokid.18f",  0x30000, 0x10000, 0xa0aa2a84 )

	ROM_REGION( 0x08000, REGION_GFX5, ROMREGION_DISPOSE )		/* FG */
	ROM_LOAD( "robokid.b9",   0x00000, 0x08000, 0xfac59c3f )
ROM_END

ROM_START( robokidj )
	ROM_REGION( 0x48000, REGION_CPU1, 0 )						/* main CPU */
	ROM_LOAD( "1.29",         0x00000, 0x08000, 0x59a1e2ec )
	ROM_CONTINUE(             0x10000, 0x08000 )
	ROM_LOAD( "2.30",         0x18000, 0x10000, 0xe3f73476 )
	ROM_LOAD( "robokid3.15k", 0x28000, 0x10000, 0x05295ec3 )
	ROM_LOAD( "robokid4.12k", 0x38000, 0x10000, 0x3bc3977f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )						/* sound CPU */
	ROM_LOAD( "robokid.k7",   0x00000, 0x10000, 0xf490a2e9 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )		/* BG0 */
	ROM_LOAD( "robokid.19c",  0x00000, 0x10000, 0x02220421 )
	ROM_LOAD( "robokid.20c",  0x10000, 0x10000, 0x02d59bc2 )
	ROM_LOAD( "robokid.17d",  0x20000, 0x10000, 0x2fa29b99 )
	ROM_LOAD( "robokid.18d",  0x30000, 0x10000, 0xae15ce02 )
	ROM_LOAD( "robokid.19d",  0x40000, 0x10000, 0x784b089e )
	ROM_LOAD( "robokid.20d",  0x50000, 0x10000, 0xb0b395ed )
	ROM_LOAD( "robokid.19f",  0x60000, 0x10000, 0x0f9071c6 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )		/* BG1 */
	ROM_LOAD( "robokid.12c",  0x00000, 0x10000, 0x0ab45f94 )
	ROM_LOAD( "robokid.14c",  0x10000, 0x10000, 0x029bbd4a )
	ROM_LOAD( "robokid.15c",  0x20000, 0x10000, 0x7de67ebb )
	ROM_LOAD( "robokid.16c",  0x30000, 0x10000, 0x53c0e582 )
	ROM_LOAD( "robokid.17c",  0x40000, 0x10000, 0x0cae5a1e )
	ROM_LOAD( "robokid.18c",  0x50000, 0x10000, 0x56ac7c8a )
	ROM_LOAD( "robokid.15d",  0x60000, 0x10000, 0xcd632a4d )
	ROM_LOAD( "robokid.16d",  0x70000, 0x10000, 0x18d92b2b )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )		/* BG2 */
	ROM_LOAD( "robokid.12a",  0x00000, 0x10000, 0xe64d1c10 )
	ROM_LOAD( "robokid.14a",  0x10000, 0x10000, 0x8f9371e4 )
	ROM_LOAD( "robokid.15a",  0x20000, 0x10000, 0x469204e7 )
	ROM_LOAD( "robokid.16a",  0x30000, 0x10000, 0x4e340815 )
	ROM_LOAD( "robokid.17a",  0x40000, 0x10000, 0xf0863106 )
	ROM_LOAD( "robokid.18a",  0x50000, 0x10000, 0xfdff7441 )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )		/* sprite */
	ROM_LOAD( "robokid.15f",  0x00000, 0x10000, 0xba61f5ab )
	ROM_LOAD( "robokid.16f",  0x10000, 0x10000, 0xd9b399ce )
	ROM_LOAD( "robokid.17f",  0x20000, 0x10000, 0xafe432b9 )
	ROM_LOAD( "robokid.18f",  0x30000, 0x10000, 0xa0aa2a84 )

	ROM_REGION( 0x08000, REGION_GFX5, ROMREGION_DISPOSE )		/* FG */
	ROM_LOAD( "robokid.b9",   0x00000, 0x08000, 0xfac59c3f )
ROM_END


/*   ( YEAR  NAME      PARENT   MACHINE  INPUT    INIT      MONITOR COMPANY  FULLNAME                 FLAGS ) */
GAMEX( 1988, robokid,  0,       robokid, robokid, 0,        ROT0,   "UPL",  "Atomic Robokid",         GAME_NO_COCKTAIL )
GAMEX( 1988, robokidj, robokid, robokid, robokidj,0,        ROT0,   "UPL",  "Atomic Robokid (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1989, omegaf,   0,       omegaf,  omegaf,  omegaf,   ROT270, "UPL",  "Omega Fighter",          GAME_NO_COCKTAIL )
GAMEX( 1989, omegafs,  omegaf,  omegaf,  omegaf,  omegaf,   ROT270, "UPL",  "Omega Fighter Special",  GAME_NO_COCKTAIL )
