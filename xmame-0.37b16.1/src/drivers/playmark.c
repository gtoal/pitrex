/***************************************************************************

Big Twins
World Beach Volley

driver by Nicola Salmoria

The games run on different, but similar, hardware. The sprite system is the
same (almost - the tile size is different).

Even if the two games are from the same year, World Beach Volley is much more
advanced - more colourful, and stores setting in an EEPROM.

An interesting thing about this hardware is that the same gfx ROMs are used
to generate both 8x8 and 16x16 tiles for different tilemaps.


TODO:
- Sound is controlled by a pic16c57 whose ROM is missing.

Big Twins:
- The pixel bitmap might be larger than what I handle, or the vertical scroll
  register has an additional meaning. The usual scroll value is 0x7f0, the game
  is setting it to 0x5f0 while updating the bitmap, so this should either scroll
  the changing region out of view, or disable it. During gameplay, the image
  that scrolls down might have to be different since the scroll register is in
  the range 0x600-0x6ff.
  As it is handled now, it is certainly wrong because after game over the
  bitmap is left on screen.

World Beach Volley:
- sprite/tile priority issue during attract mode (plane should go behind palm)
- The histogram functions don't seem to work.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"


extern data16_t *bigtwin_bgvideoram;
extern size_t bigtwin_bgvideoram_size;
extern data16_t *wbeachvl_videoram1,*wbeachvl_videoram2,*wbeachvl_videoram3;

int bigtwin_vh_start(void);
void bigtwin_vh_stop(void);
int wbeachvl_vh_start(void);
WRITE16_HANDLER( wbeachvl_txvideoram_w );
WRITE16_HANDLER( wbeachvl_fgvideoram_w );
WRITE16_HANDLER( wbeachvl_bgvideoram_w );
WRITE16_HANDLER( bigtwin_paletteram_w );
WRITE16_HANDLER( bigtwin_bgvideoram_w );
WRITE16_HANDLER( bigtwin_scroll_w );
WRITE16_HANDLER( wbeachvl_scroll_w );
void bigtwin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void wbeachvl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static WRITE16_HANDLER( coinctrl_w )
{
	if (ACCESSING_MSB)
	{
		coin_counter_w(0,data & 0x0100);
		coin_counter_w(1,data & 0x0200);
	}
}


/***************************************************************************

  EEPROM

***************************************************************************/

static struct EEPROM_interface eeprom_interface =
{
	6,			/* address bits */
	16,			/* data bits */
	"110",		/*  read command */
	"101",		/* write command */
	0,			/* erase command */
	"100000000",/* lock command */
	"100110000"	/* unlock command */
};

static void wbeachvl_nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
	{
		EEPROM_save(file);
	}
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
			EEPROM_load(file);
		else
		{
			UINT8 *init = malloc(128);
			if (init)
			{
				memset(init,0,128);
				EEPROM_set_data(init,128);
				free(init);
			}
		}
	}
}

static READ16_HANDLER( wbeachvl_port0_r )
{
	int bit;

	bit = EEPROM_read_bit() << 7;

	return (input_port_0_r(0) & 0x7f) | bit;
}

static WRITE16_HANDLER( wbeachvl_coin_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		/* bits 0-3 are coin counters? (only 0 used?) */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);
		coin_counter_w(2,data & 0x04);
		coin_counter_w(3,data & 0x08);

		/* bits 5-7 control EEPROM */
		EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_write_bit(data & 0x80);
		EEPROM_set_clock_line((data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
	}
}



static MEMORY_READ16_START( bigtwin_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x440000, 0x4403ff, MRA16_RAM },
	{ 0x700010, 0x700011, input_port_0_word_r },
	{ 0x700012, 0x700013, input_port_1_word_r },
	{ 0x700014, 0x700015, input_port_2_word_r },
	{ 0x70001a, 0x70001b, input_port_3_word_r },
	{ 0x70001c, 0x70001d, input_port_4_word_r },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( bigtwin_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x304000, 0x304001, MWA16_NOP },	/* watchdog? irq ack? */
	{ 0x440000, 0x4403ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x500000, 0x5007ff, wbeachvl_fgvideoram_w, &wbeachvl_videoram2 },
{ 0x500800, 0x501fff, MWA16_NOP },	/* unused RAM? */
	{ 0x502000, 0x503fff, wbeachvl_txvideoram_w, &wbeachvl_videoram1 },
{ 0x504000, 0x50ffff, MWA16_NOP },	/* unused RAM? */
	{ 0x510000, 0x51000b, bigtwin_scroll_w },
	{ 0x51000c, 0x51000d, MWA16_NOP },	/* always 3? */
	{ 0x600000, 0x67ffff, bigtwin_bgvideoram_w, &bigtwin_bgvideoram, &bigtwin_bgvideoram_size },
	{ 0x700016, 0x700017, coinctrl_w },
	{ 0x70001e, 0x70001f, MWA16_NOP },/*sound_command_w }, */
	{ 0x780000, 0x7807ff, bigtwin_paletteram_w, &paletteram16 },
/*	{ 0xe00000, 0xe00001, ?? written on startup */
	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( wbeachvl_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x440000, 0x440fff, MRA16_RAM },
	{ 0x500000, 0x501fff, MRA16_RAM },
	{ 0x504000, 0x505fff, MRA16_RAM },
	{ 0x508000, 0x509fff, MRA16_RAM },
	{ 0xff0000, 0xffffff, MRA16_RAM },
	{ 0x710010, 0x710011, wbeachvl_port0_r },
	{ 0x710012, 0x710013, input_port_1_word_r },
	{ 0x710014, 0x710015, input_port_2_word_r },
	{ 0x710018, 0x710019, input_port_3_word_r },
	{ 0x71001a, 0x71001b, input_port_4_word_r },
/*	{ 0x71001c, 0x71001d, ?? */
MEMORY_END

static MEMORY_WRITE16_START( wbeachvl_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x440000, 0x440fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x500000, 0x501fff, wbeachvl_bgvideoram_w, &wbeachvl_videoram3 },
	{ 0x504000, 0x505fff, wbeachvl_fgvideoram_w, &wbeachvl_videoram2 },
	{ 0x508000, 0x509fff, wbeachvl_txvideoram_w, &wbeachvl_videoram1 },
	{ 0x510000, 0x51000b, wbeachvl_scroll_w },
	{ 0x51000c, 0x51000d, MWA16_NOP },	/* always 3? */
/*	{ 0x700000, 0x700001, ?? written on startup */
	{ 0x710016, 0x710017, wbeachvl_coin_eeprom_w },
	{ 0x71001e, 0x71001f, MWA16_NOP },/*sound_command_w }, */
	{ 0x780000, 0x780fff, paletteram16_RRRRRGGGGGBBBBBx_word_w, &paletteram16 },
	{ 0xff0000, 0xffffff, MWA16_RAM },
#if 0
	{ 0x700016, 0x700017, coinctrl_w },
#endif
MEMORY_END



INPUT_PORTS_START( bigtwin )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "Italian" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Censor Pictures" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Coin Mode" )
	PORT_DIPSETTING(    0x01, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	/* TODO: support coin mode 2 */
	PORT_DIPNAME( 0x1e, 0x1e, "Coinage Mode 1" )
	PORT_DIPSETTING(    0x14, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x16, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x1a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x1e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x12, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
#if 0
	PORT_DIPNAME( 0x06, 0x06, "Coin A Mode 2" )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x18, 0x18, "Coin B Mode 2" )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
#endif
	PORT_DIPNAME( 0x20, 0x20, "Minimum Credits to Start" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( wbeachvl )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* ?? see code at 746a. sound status? */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxLayout spritelayout =
{
	32,32,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			48*8+0, 48*8+1, 48*8+2, 48*8+3, 48*8+4, 48*8+5, 48*8+6, 48*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
			64*8, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8,
			72*8, 73*8, 74*8, 75*8, 76*8, 77*8, 78*8, 79*8 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &spritelayout, 0x200, 16 },	/* colors 0x200-0x2ff */
	{ REGION_GFX1, 0, &tilelayout,   0x000,  8 },	/* colors 0x000-0x07f */
	{ REGION_GFX1, 0, &charlayout,   0x080,  8 },	/* colors 0x080-0x0ff */
	/* background bitmap uses colors 0x100-0x1ff */
	{ -1 } /* end of array */
};


static struct GfxLayout wcharlayout =
{
	8,8,
	RGN_FRAC(1,6),
	6,
	{ RGN_FRAC(5,6), RGN_FRAC(4,6), RGN_FRAC(3,6), RGN_FRAC(2,6), RGN_FRAC(1,6), RGN_FRAC(0,6) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout wtilelayout =
{
	16,16,
	RGN_FRAC(1,6),
	6,
	{ RGN_FRAC(5,6), RGN_FRAC(4,6), RGN_FRAC(3,6), RGN_FRAC(2,6), RGN_FRAC(1,6), RGN_FRAC(0,6) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

/* tiles are 6 bpp, sprites only 5bpp */
static struct GfxLayout wspritelayout =
{
	16,16,
	RGN_FRAC(1,6),
	5,
	{ RGN_FRAC(4,6), RGN_FRAC(3,6), RGN_FRAC(2,6), RGN_FRAC(1,6), RGN_FRAC(0,6) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxDecodeInfo wbeachvl_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &wspritelayout, 0x600, 16 },	/* colors 0x600-0x7ff */
	{ REGION_GFX1, 0, &wtilelayout,   0x000, 16 },	/* colors 0x000-0x3ff */
	{ REGION_GFX1, 0, &wcharlayout,   0x400,  8 },	/* colors 0x400-0x5ff */
	{ -1 } /* end of array */
};



static struct OKIM6295interface okim6295_interface =
{
	1,					/* 1 chip */
	{ 8000 },			/* 8000Hz frequency? */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};



static struct MachineDriver machine_driver_bigtwin =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz? */
			bigtwin_readmem,bigtwin_writemem,0,0,
			m68_level2_irq,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	bigtwin_vh_start,
	bigtwin_vh_stop,
	bigtwin_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_wbeachvl =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz? */
			wbeachvl_readmem,wbeachvl_writemem,0,0,
			m68_level2_irq,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },
	wbeachvl_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	wbeachvl_vh_start,
	0,
	wbeachvl_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	},

	wbeachvl_nvram_handler
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bigtwin )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "bt_02.bin",    0x000000, 0x80000, 0xe6767f60 )
	ROM_LOAD16_BYTE( "bt_03.bin",    0x000001, 0x80000, 0x5aba6990 )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* sound (missing) */
	ROM_LOAD( "pic16c57",     0x0000, 0x0800, 0x00000000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bt_04.bin",    0x00000, 0x40000, 0x6f628fbc )
	ROM_LOAD( "bt_05.bin",    0x40000, 0x40000, 0x6a9b1752 )
	ROM_LOAD( "bt_06.bin",    0x80000, 0x40000, 0x411cf852 )
	ROM_LOAD( "bt_07.bin",    0xc0000, 0x40000, 0x635c81fd )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bt_08.bin",    0x00000, 0x20000, 0x2749644d )
	ROM_LOAD( "bt_09.bin",    0x20000, 0x20000, 0x1d1897af )
	ROM_LOAD( "bt_10.bin",    0x40000, 0x20000, 0x2a03432e )
	ROM_LOAD( "bt_11.bin",    0x60000, 0x20000, 0x2c980c4c )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "bt_01.bin",    0x00000, 0x40000, 0xff6671dc )
ROM_END

ROM_START( wbeachvl )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "wbv_02.bin",   0x000000, 0x40000, 0xc7cca29e )
	ROM_LOAD16_BYTE( "wbv_03.bin",   0x000001, 0x40000, 0xdb4e69d5 )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* sound (missing) */
	ROM_LOAD( "pic16c57",     0x0000, 0x0800, 0x00000000 )

	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "wbv_10.bin",   0x000000, 0x80000, 0x50680f0b )
	ROM_LOAD( "wbv_04.bin",   0x080000, 0x80000, 0xdf9cbff1 )
	ROM_LOAD( "wbv_11.bin",   0x100000, 0x80000, 0xe59ad0d1 )
	ROM_LOAD( "wbv_05.bin",   0x180000, 0x80000, 0x51245c3c )
	ROM_LOAD( "wbv_12.bin",   0x200000, 0x80000, 0x36b87d0b )
	ROM_LOAD( "wbv_06.bin",   0x280000, 0x80000, 0x9eb808ef )
	ROM_LOAD( "wbv_13.bin",   0x300000, 0x80000, 0x7021107b )
	ROM_LOAD( "wbv_07.bin",   0x380000, 0x80000, 0x4fff9fe8 )
	ROM_LOAD( "wbv_14.bin",   0x400000, 0x80000, 0x0595e675 )
	ROM_LOAD( "wbv_08.bin",   0x480000, 0x80000, 0x07e4b416 )
	ROM_LOAD( "wbv_15.bin",   0x500000, 0x80000, 0x4e1a82d2 )
	ROM_LOAD( "wbv_09.bin",   0x580000, 0x20000, 0x894ce354 )
	/* 5a0000-5fffff is empty */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "wbv_01.bin",   0x00000, 0x100000, 0xac33f25f )
ROM_END



GAMEX( 1995, bigtwin,  0, bigtwin,  bigtwin,  0, ROT0_16BIT, "Playmark", "Big Twin", GAME_NO_COCKTAIL | GAME_NO_SOUND )
GAMEX( 1995, wbeachvl, 0, wbeachvl, wbeachvl, 0, ROT0_16BIT, "Playmark", "World Beach Volley", GAME_NO_COCKTAIL | GAME_NO_SOUND )
