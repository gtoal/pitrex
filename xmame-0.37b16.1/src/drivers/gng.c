/***************************************************************************

Ghosts'n Goblins
Diamond Run


Notes:
- Diamond Run doesn't use all the ROMs. d5 is not used at all, and the second
  half of d3o is not used either. There are 0x38 levels in total, the data for
  levels 0x00-0x0f is taken from ROM space 0x8000-0xbfff, the data for levels
  0x10-0x37 is taken from the banked space 0x4000-0x5fff (5 banks) (see the
  table at 0xc66f). Actually, looking at the code it seems to roll overs at
  level 0x2f, and indeed the data for levels 0x30-0x37 isn't valid (player
  starts into a wall, or there are invisible walls, etc.)
  The 0x6000-0x7fff ROM space doesn't seem to be used: instead the game writes
  to 6048 and reads from 6000. Silly copy protection?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"



extern unsigned char *gng_fgvideoram;
extern unsigned char *gng_bgvideoram;
WRITE_HANDLER( gng_fgvideoram_w );
WRITE_HANDLER( gng_bgvideoram_w );
WRITE_HANDLER( gng_bgscrollx_w );
WRITE_HANDLER( gng_bgscrolly_w );
WRITE_HANDLER( gng_flipscreen_w );
int gng_vh_start(void);
void gng_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void gng_eof_callback(void);



static WRITE_HANDLER( gng_bankswitch_w )
{
	unsigned char *rom = memory_region(REGION_CPU1);


	if (data == 4)
	{
		cpu_setbank(1,rom + 0x4000);
	}
	else
	{
		cpu_setbank(1,rom + 0x10000 + (data & 3) * 0x2000);
	}
}

static WRITE_HANDLER( gng_coin_counter_w )
{
	coin_counter_w(offset,data);
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x2fff, MRA_RAM },
	{ 0x3000, 0x3000, input_port_0_r },
	{ 0x3001, 0x3001, input_port_1_r },
	{ 0x3002, 0x3002, input_port_2_r },
	{ 0x3003, 0x3003, input_port_3_r },
	{ 0x3004, 0x3004, input_port_4_r },
	{ 0x3c00, 0x3c00, MRA_NOP },	/* watchdog? */
	{ 0x4000, 0x5fff, MRA_BANK1 },
	{ 0x6000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x1dff, MWA_RAM },
	{ 0x1e00, 0x1fff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x2000, 0x27ff, gng_fgvideoram_w, &gng_fgvideoram },
	{ 0x2800, 0x2fff, gng_bgvideoram_w, &gng_bgvideoram },
	{ 0x3800, 0x38ff, paletteram_RRRRGGGGBBBBxxxx_split2_w, &paletteram_2 },
	{ 0x3900, 0x39ff, paletteram_RRRRGGGGBBBBxxxx_split1_w, &paletteram },
	{ 0x3a00, 0x3a00, soundlatch_w },
	{ 0x3b08, 0x3b09, gng_bgscrollx_w },
	{ 0x3b0a, 0x3b0b, gng_bgscrolly_w },
	{ 0x3c00, 0x3c00, MWA_NOP },   /* watchdog? */
	{ 0x3d00, 0x3d00, gng_flipscreen_w },
/*	{ 0x3d01, 0x3d01, reset sound cpu? */
	{ 0x3d02, 0x3d03, gng_coin_counter_w },
	{ 0x3e00, 0x3e00, gng_bankswitch_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END



static MEMORY_READ_START( sound_readmem )
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, soundlatch_r },
	{ 0x0000, 0x7fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xe000, YM2203_control_port_0_w },
	{ 0xe001, 0xe001, YM2203_write_port_0_w },
	{ 0xe002, 0xe002, YM2203_control_port_1_w },
	{ 0xe003, 0xe003, YM2203_write_port_1_w },
	{ 0x0000, 0x7fff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( gng )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(	0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(	0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x10, "Coinage affects" )
	PORT_DIPSETTING(	0x10, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Coin_B ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x03, "3" )
	PORT_DIPSETTING(	0x02, "4" )
	PORT_DIPSETTING(	0x01, "5" )
	PORT_DIPSETTING(	0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x18, "20000 70000 70000" )
	PORT_DIPSETTING(	0x10, "30000 80000 80000" )
	PORT_DIPSETTING(	0x08, "20000 80000" )
	PORT_DIPSETTING(	0x00, "30000 80000" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x60, "Normal" )
	PORT_DIPSETTING(	0x20, "Difficult" )
	PORT_DIPSETTING(	0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* identical to gng, but the "unknown" dip switch is Invulnerability */
INPUT_PORTS_START( makaimur )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(	0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(	0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x10, "Coinage affects" )
	PORT_DIPSETTING(	0x10, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Coin_B ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x03, "3" )
	PORT_DIPSETTING(	0x02, "4" )
	PORT_DIPSETTING(	0x01, "5" )
	PORT_DIPSETTING(	0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x18, "20000 70000 70000" )
	PORT_DIPSETTING(	0x10, "30000 80000 80000" )
	PORT_DIPSETTING(	0x08, "20000 80000" )
	PORT_DIPSETTING(	0x00, "30000 80000" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x60, "Normal" )
	PORT_DIPSETTING(	0x20, "Difficult" )
	PORT_DIPSETTING(	0x00, "Very Difficult" )
	PORT_BITX(	  0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( diamond )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	 /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x01, "3" )
	PORT_DIPSETTING(	0x02, "4" )
	PORT_DIPSETTING(	0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x00, "*1" )
	PORT_DIPSETTING(	0x04, "*2" )
	PORT_DIPSETTING(	0x08, "*3" )
	PORT_DIPSETTING(	0x0c, "*4" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(	0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x00, "Unknown DSW1 7" )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown DSW2 1" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Unknown DSW2 2" )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Unknown DSW2 3" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Unknown DSW2 4" )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x00, "*1" )
	PORT_DIPSETTING(	0x10, "*2" )
	PORT_DIPSETTING(	0x20, "*3" )
	PORT_DIPSETTING(	0x30, "*4" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown DSW2 7" )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Unknown DSW2 8" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};
static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};
static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	 0x80, 16 },	/* colors 0x80-0xbf */
	{ REGION_GFX2, 0, &tilelayout,	 0x00,	8 },	/* colors 0x00-0x3f */
	{ REGION_GFX3, 0, &spritelayout, 0x40,	4 },	/* colors 0x40-0x7f */
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz (?) */
	{ YM2203_VOL(20,40), YM2203_VOL(20,40) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver_gng =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1500000,			/* 1.5 MHz ? */
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 MHz (?) */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	gng_eof_callback,
	gng_vh_start,
	0,
	gng_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gng )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "gg4.bin",      0x04000, 0x4000, 0x66606beb ) /* 4000-5fff is page 4 */
	ROM_LOAD( "gg3.bin",      0x08000, 0x8000, 0x9e01c65e )
	ROM_LOAD( "gg5.bin",      0x10000, 0x8000, 0xd6397b2b ) /* page 0, 1, 2, 3 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "gg2.bin",      0x0000, 0x8000, 0x615f5b6f )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gg1.bin",      0x00000, 0x4000, 0xecfccf07 ) /* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gg11.bin",     0x00000, 0x4000, 0xddd56fa9 ) /* tiles 0-1 Plane 1*/
	ROM_LOAD( "gg10.bin",     0x04000, 0x4000, 0x7302529d ) /* tiles 2-3 Plane 1*/
	ROM_LOAD( "gg9.bin",      0x08000, 0x4000, 0x20035bda ) /* tiles 0-1 Plane 2*/
	ROM_LOAD( "gg8.bin",      0x0c000, 0x4000, 0xf12ba271 ) /* tiles 2-3 Plane 2*/
	ROM_LOAD( "gg7.bin",      0x10000, 0x4000, 0xe525207d ) /* tiles 0-1 Plane 3*/
	ROM_LOAD( "gg6.bin",      0x14000, 0x4000, 0x2d77e9b2 ) /* tiles 2-3 Plane 3*/

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "gg17.bin",     0x00000, 0x4000, 0x93e50a8f ) /* sprites 0 Plane 1-2 */
	ROM_LOAD( "gg16.bin",     0x04000, 0x4000, 0x06d7e5ca ) /* sprites 1 Plane 1-2 */
	ROM_LOAD( "gg15.bin",     0x08000, 0x4000, 0xbc1fe02d ) /* sprites 2 Plane 1-2 */
	ROM_LOAD( "gg14.bin",     0x0c000, 0x4000, 0x6aaf12f9 ) /* sprites 0 Plane 3-4 */
	ROM_LOAD( "gg13.bin",     0x10000, 0x4000, 0xe80c3fca ) /* sprites 1 Plane 3-4 */
	ROM_LOAD( "gg12.bin",     0x14000, 0x4000, 0x7780a925 ) /* sprites 2 Plane 3-4 */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbp24s10.14k", 0x0000, 0x0100, 0x0eaf5158 )  /* video timing (not used) */
	ROM_LOAD( "63s141.2e",    0x0100, 0x0100, 0x4a1285a4 )  /* priority (not used) */
ROM_END

ROM_START( gnga )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "gng.n10",      0x04000, 0x4000, 0x60343188 )
	ROM_LOAD( "gng.n9",       0x08000, 0x4000, 0xb6b91cfb )
	ROM_LOAD( "gng.n8",       0x0c000, 0x4000, 0xa5cfa928 )
	ROM_LOAD( "gng.n13",      0x10000, 0x4000, 0xfd9a8dda )
	ROM_LOAD( "gng.n12",      0x14000, 0x4000, 0x13cf6238 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "gg2.bin",      0x0000, 0x8000, 0x615f5b6f )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gg1.bin",      0x00000, 0x4000, 0xecfccf07 ) /* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gg11.bin",     0x00000, 0x4000, 0xddd56fa9 ) /* tiles 0-1 Plane 1*/
	ROM_LOAD( "gg10.bin",     0x04000, 0x4000, 0x7302529d ) /* tiles 2-3 Plane 1*/
	ROM_LOAD( "gg9.bin",      0x08000, 0x4000, 0x20035bda ) /* tiles 0-1 Plane 2*/
	ROM_LOAD( "gg8.bin",      0x0c000, 0x4000, 0xf12ba271 ) /* tiles 2-3 Plane 2*/
	ROM_LOAD( "gg7.bin",      0x10000, 0x4000, 0xe525207d ) /* tiles 0-1 Plane 3*/
	ROM_LOAD( "gg6.bin",      0x14000, 0x4000, 0x2d77e9b2 ) /* tiles 2-3 Plane 3*/

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "gg17.bin",     0x00000, 0x4000, 0x93e50a8f ) /* sprites 0 Plane 1-2 */
	ROM_LOAD( "gg16.bin",     0x04000, 0x4000, 0x06d7e5ca ) /* sprites 1 Plane 1-2 */
	ROM_LOAD( "gg15.bin",     0x08000, 0x4000, 0xbc1fe02d ) /* sprites 2 Plane 1-2 */
	ROM_LOAD( "gg14.bin",     0x0c000, 0x4000, 0x6aaf12f9 ) /* sprites 0 Plane 3-4 */
	ROM_LOAD( "gg13.bin",     0x10000, 0x4000, 0xe80c3fca ) /* sprites 1 Plane 3-4 */
	ROM_LOAD( "gg12.bin",     0x14000, 0x4000, 0x7780a925 ) /* sprites 2 Plane 3-4 */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbp24s10.14k", 0x0000, 0x0100, 0x0eaf5158 )  /* video timing (not used) */
	ROM_LOAD( "63s141.2e",    0x0100, 0x0100, 0x4a1285a4 )  /* priority (not used) */
ROM_END

ROM_START( gngt )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "mm04",         0x04000, 0x4000, 0x652406f6 ) /* 4000-5fff is page 4 */
	ROM_LOAD( "mm03",         0x08000, 0x8000, 0xfb040b42 )
	ROM_LOAD( "mm05",         0x10000, 0x8000, 0x8f7cff61 ) /* page 0, 1, 2, 3 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "gg2.bin",      0x0000, 0x8000, 0x615f5b6f )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gg1.bin",      0x00000, 0x4000, 0xecfccf07 ) /* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gg11.bin",     0x00000, 0x4000, 0xddd56fa9 ) /* tiles 0-1 Plane 1*/
	ROM_LOAD( "gg10.bin",     0x04000, 0x4000, 0x7302529d ) /* tiles 2-3 Plane 1*/
	ROM_LOAD( "gg9.bin",      0x08000, 0x4000, 0x20035bda ) /* tiles 0-1 Plane 2*/
	ROM_LOAD( "gg8.bin",      0x0c000, 0x4000, 0xf12ba271 ) /* tiles 2-3 Plane 2*/
	ROM_LOAD( "gg7.bin",      0x10000, 0x4000, 0xe525207d ) /* tiles 0-1 Plane 3*/
	ROM_LOAD( "gg6.bin",      0x14000, 0x4000, 0x2d77e9b2 ) /* tiles 2-3 Plane 3*/

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "gg17.bin",     0x00000, 0x4000, 0x93e50a8f ) /* sprites 0 Plane 1-2 */
	ROM_LOAD( "gg16.bin",     0x04000, 0x4000, 0x06d7e5ca ) /* sprites 1 Plane 1-2 */
	ROM_LOAD( "gg15.bin",     0x08000, 0x4000, 0xbc1fe02d ) /* sprites 2 Plane 1-2 */
	ROM_LOAD( "gg14.bin",     0x0c000, 0x4000, 0x6aaf12f9 ) /* sprites 0 Plane 3-4 */
	ROM_LOAD( "gg13.bin",     0x10000, 0x4000, 0xe80c3fca ) /* sprites 1 Plane 3-4 */
	ROM_LOAD( "gg12.bin",     0x14000, 0x4000, 0x7780a925 ) /* sprites 2 Plane 3-4 */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbp24s10.14k", 0x0000, 0x0100, 0x0eaf5158 )  /* video timing (not used) */
	ROM_LOAD( "63s141.2e",    0x0100, 0x0100, 0x4a1285a4 )  /* priority (not used) */
ROM_END

ROM_START( makaimur )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "10n.rom",      0x04000, 0x4000, 0x81e567e0 ) /* 4000-5fff is page 4 */
	ROM_LOAD( "8n.rom",       0x08000, 0x8000, 0x9612d66c )
	ROM_LOAD( "12n.rom",      0x10000, 0x8000, 0x65a6a97b ) /* page 0, 1, 2, 3 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "gg2.bin",      0x0000, 0x8000, 0x615f5b6f )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gg1.bin",      0x00000, 0x4000, 0xecfccf07 ) /* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gg11.bin",     0x00000, 0x4000, 0xddd56fa9 ) /* tiles 0-1 Plane 1*/
	ROM_LOAD( "gg10.bin",     0x04000, 0x4000, 0x7302529d ) /* tiles 2-3 Plane 1*/
	ROM_LOAD( "gg9.bin",      0x08000, 0x4000, 0x20035bda ) /* tiles 0-1 Plane 2*/
	ROM_LOAD( "gg8.bin",      0x0c000, 0x4000, 0xf12ba271 ) /* tiles 2-3 Plane 2*/
	ROM_LOAD( "gg7.bin",      0x10000, 0x4000, 0xe525207d ) /* tiles 0-1 Plane 3*/
	ROM_LOAD( "gg6.bin",      0x14000, 0x4000, 0x2d77e9b2 ) /* tiles 2-3 Plane 3*/

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "gng13.n4",     0x00000, 0x4000, 0x4613afdc ) /* sprites 0 Plane 1-2 */
	ROM_LOAD( "gg16.bin",     0x04000, 0x4000, 0x06d7e5ca ) /* sprites 1 Plane 1-2 */
	ROM_LOAD( "gg15.bin",     0x08000, 0x4000, 0xbc1fe02d ) /* sprites 2 Plane 1-2 */
	ROM_LOAD( "gng16.l4",     0x0c000, 0x4000, 0x608d68d5 )     /* sprites 0 Plane 3-4 */
	ROM_LOAD( "gg13.bin",     0x10000, 0x4000, 0xe80c3fca ) /* sprites 1 Plane 3-4 */
	ROM_LOAD( "gg12.bin",     0x14000, 0x4000, 0x7780a925 ) /* sprites 2 Plane 3-4 */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbp24s10.14k", 0x0000, 0x0100, 0x0eaf5158 )  /* video timing (not used) */
	ROM_LOAD( "63s141.2e",    0x0100, 0x0100, 0x4a1285a4 )  /* priority (not used) */
ROM_END

ROM_START( makaimuc )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "mj04c.bin",      0x04000, 0x4000, 0x1294edb1 )   /* 4000-5fff is page 4 */
	ROM_LOAD( "mj03c.bin",      0x08000, 0x8000, 0xd343332d )
	ROM_LOAD( "mj05c.bin",      0x10000, 0x8000, 0x535342c2 )   /* page 0, 1, 2, 3 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "gg2.bin",      0x0000, 0x8000, 0x615f5b6f )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gg1.bin",      0x00000, 0x4000, 0xecfccf07 ) /* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gg11.bin",     0x00000, 0x4000, 0xddd56fa9 ) /* tiles 0-1 Plane 1*/
	ROM_LOAD( "gg10.bin",     0x04000, 0x4000, 0x7302529d ) /* tiles 2-3 Plane 1*/
	ROM_LOAD( "gg9.bin",      0x08000, 0x4000, 0x20035bda ) /* tiles 0-1 Plane 2*/
	ROM_LOAD( "gg8.bin",      0x0c000, 0x4000, 0xf12ba271 ) /* tiles 2-3 Plane 2*/
	ROM_LOAD( "gg7.bin",      0x10000, 0x4000, 0xe525207d ) /* tiles 0-1 Plane 3*/
	ROM_LOAD( "gg6.bin",      0x14000, 0x4000, 0x2d77e9b2 ) /* tiles 2-3 Plane 3*/

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "gng13.n4",     0x00000, 0x4000, 0x4613afdc ) /* sprites 0 Plane 1-2 */
	ROM_LOAD( "gg16.bin",     0x04000, 0x4000, 0x06d7e5ca ) /* sprites 1 Plane 1-2 */
	ROM_LOAD( "gg15.bin",     0x08000, 0x4000, 0xbc1fe02d ) /* sprites 2 Plane 1-2 */
	ROM_LOAD( "gng16.l4",     0x0c000, 0x4000, 0x608d68d5 )     /* sprites 0 Plane 3-4 */
	ROM_LOAD( "gg13.bin",     0x10000, 0x4000, 0xe80c3fca ) /* sprites 1 Plane 3-4 */
	ROM_LOAD( "gg12.bin",     0x14000, 0x4000, 0x7780a925 ) /* sprites 2 Plane 3-4 */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbp24s10.14k", 0x0000, 0x0100, 0x0eaf5158 )  /* video timing (not used) */
	ROM_LOAD( "63s141.2e",    0x0100, 0x0100, 0x4a1285a4 )  /* priority (not used) */
ROM_END

ROM_START( makaimug )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "mj04g.bin",      0x04000, 0x4000, 0x757c94d3 )   /* 4000-5fff is page 4 */
	ROM_LOAD( "mj03g.bin",      0x08000, 0x8000, 0x61b043bb )
	ROM_LOAD( "mj05g.bin",      0x10000, 0x8000, 0xf2fdccf5 )   /* page 0, 1, 2, 3 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "gg2.bin",      0x0000, 0x8000, 0x615f5b6f )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gg1.bin",      0x00000, 0x4000, 0xecfccf07 ) /* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gg11.bin",     0x00000, 0x4000, 0xddd56fa9 ) /* tiles 0-1 Plane 1*/
	ROM_LOAD( "gg10.bin",     0x04000, 0x4000, 0x7302529d ) /* tiles 2-3 Plane 1*/
	ROM_LOAD( "gg9.bin",      0x08000, 0x4000, 0x20035bda ) /* tiles 0-1 Plane 2*/
	ROM_LOAD( "gg8.bin",      0x0c000, 0x4000, 0xf12ba271 ) /* tiles 2-3 Plane 2*/
	ROM_LOAD( "gg7.bin",      0x10000, 0x4000, 0xe525207d ) /* tiles 0-1 Plane 3*/
	ROM_LOAD( "gg6.bin",      0x14000, 0x4000, 0x2d77e9b2 ) /* tiles 2-3 Plane 3*/

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "gng13.n4",     0x00000, 0x4000, 0x4613afdc ) /* sprites 0 Plane 1-2 */
	ROM_LOAD( "gg16.bin",     0x04000, 0x4000, 0x06d7e5ca ) /* sprites 1 Plane 1-2 */
	ROM_LOAD( "gg15.bin",     0x08000, 0x4000, 0xbc1fe02d ) /* sprites 2 Plane 1-2 */
	ROM_LOAD( "gng16.l4",     0x0c000, 0x4000, 0x608d68d5 ) /* sprites 0 Plane 3-4 */
	ROM_LOAD( "gg13.bin",     0x10000, 0x4000, 0xe80c3fca ) /* sprites 1 Plane 3-4 */
	ROM_LOAD( "gg12.bin",     0x14000, 0x4000, 0x7780a925 ) /* sprites 2 Plane 3-4 */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbp24s10.14k", 0x0000, 0x0100, 0x0eaf5158 )  /* video timing (not used) */
	ROM_LOAD( "63s141.2e",    0x0100, 0x0100, 0x4a1285a4 )  /* priority (not used) */
ROM_END

ROM_START( diamond )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "d3o",          0x04000, 0x4000, 0xba4bf9f1 ) /* 4000-5fff is page 4 */
	ROM_LOAD( "d3",           0x08000, 0x8000, 0xf436d6fa )
	ROM_LOAD( "d5o",          0x10000, 0x8000, 0xae58bd3a ) /* page 0, 1, 2, 3 */
	ROM_LOAD( "d5",           0x18000, 0x8000, 0x453f3f9e ) /* is this supposed to be used? */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "d2",           0x0000, 0x8000, 0x615f5b6f )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d1",           0x00000, 0x4000, 0x3a24e504 ) /* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "d11",          0x00000, 0x4000, 0x754357d7 ) /* tiles 0-1 Plane 1*/
	ROM_LOAD( "d10",          0x04000, 0x4000, 0x7531edcd ) /* tiles 2-3 Plane 1*/
	ROM_LOAD( "d9",           0x08000, 0x4000, 0x22eeca08 ) /* tiles 0-1 Plane 2*/
	ROM_LOAD( "d8",           0x0c000, 0x4000, 0x6b61be60 ) /* tiles 2-3 Plane 2*/
	ROM_LOAD( "d7",           0x10000, 0x4000, 0xfd595274 ) /* tiles 0-1 Plane 3*/
	ROM_LOAD( "d6",           0x14000, 0x4000, 0x7f51dcd2 ) /* tiles 2-3 Plane 3*/

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "d17",          0x00000, 0x4000, 0x8164b005 ) /* sprites 0 Plane 1-2 */
	ROM_LOAD( "d14",          0x04000, 0x4000, 0x6f132163 ) /* sprites 0 Plane 3-4 */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "prom1",        0x0000, 0x0100, 0x0eaf5158 )  /* video timing (not used) */
	ROM_LOAD( "prom2",        0x0100, 0x0100, 0x4a1285a4 )  /* priority (not used) */
ROM_END



static READ_HANDLER( diamond_hack_r )
{
	return 0;
}

static void init_diamond(void)
{
	install_mem_read_handler(0,0x6000,0x6000,diamond_hack_r);
}



GAME( 1985, gng,	  0,   gng, gng,	  0,	   ROT0, "Capcom", "Ghosts'n Goblins (World? set 1)" )
GAME( 1985, gnga,	  gng, gng, gng,	  0,	   ROT0, "Capcom", "Ghosts'n Goblins (World? set 2)" )
GAME( 1985, gngt,	  gng, gng, gng,	  0,	   ROT0, "Capcom (Taito America license)", "Ghosts'n Goblins (US)" )
GAME( 1985, makaimur, gng, gng, makaimur, 0,	   ROT0, "Capcom", "Makai-Mura" )
GAME( 1985, makaimuc, gng, gng, makaimur, 0,	   ROT0, "Capcom", "Makai-Mura (Revision C)" )
GAME( 1985, makaimug, gng, gng, makaimur, 0,	   ROT0, "Capcom", "Makai-Mura (Revision G)" )
GAME( 1989, diamond,  0,   gng, diamond,  diamond, ROT0, "KH Video", "Diamond Run" )

