/*****************************************************************************

Ikki (c) 1985 Sun Electronics

	Driver by Uki

	20/Jun/2001 -

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void ikki_vh_convert_color_prom(unsigned char *palette,
	unsigned short *colortable,const unsigned char *color_prom);
void ikki_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static UINT8 *ikki_sharedram;

/****************************************************************************/

WRITE_HANDLER( ikki_scroll_w );
WRITE_HANDLER( ikki_scrn_ctrl_w );

WRITE_HANDLER( ikki_sharedram_w )
{
	ikki_sharedram[offset] = data;
}

READ_HANDLER( ikki_sharedram_r )
{
	return ikki_sharedram[offset];
}

READ_HANDLER( ikki_e000_r )
{
/* bit1: interrupt type?, bit0: CPU2 busack? */

	if (cpu_getiloops() == 0)
		return 0;
	return 2;
}

/****************************************************************************/

static MEMORY_READ_START( ikki_readmem1 )
	{ 0x0000, 0x9fff, MRA_ROM },

	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, ikki_sharedram_r },
	{ 0xd000, 0xd7ff, MRA_RAM }, /* videoram */

	{ 0xe000, 0xe000, ikki_e000_r },
	{ 0xe001, 0xe001, input_port_0_r }, /* dsw 1 */
	{ 0xe002, 0xe002, input_port_1_r }, /* dsw 2 */
	{ 0xe003, 0xe003, input_port_4_r }, /* other inputs */
	{ 0xe004, 0xe004, input_port_2_r }, /* player1 */
	{ 0xe005, 0xe005, input_port_3_r }, /* player2 */
MEMORY_END

static MEMORY_WRITE_START( ikki_writemem1 )
	{ 0x0000, 0x9fff, MWA_ROM },

	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, ikki_sharedram_w },
	{ 0xd000, 0xd7ff, videoram_w, &videoram, &videoram_size },

	{ 0xe008, 0xe008, ikki_scrn_ctrl_w },
	{ 0xe009, 0xe009, MWA_NOP }, /* coin counter? */
	{ 0xe00a, 0xe00b, ikki_scroll_w },

MEMORY_END

static MEMORY_READ_START( ikki_readmem2 )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xc000, 0xc7ff, spriteram_r },
	{ 0xc800, 0xcfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( ikki_writemem2 )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc800, 0xcfff, MWA_RAM, &ikki_sharedram },

	{ 0xd801, 0xd801, SN76496_0_w },
	{ 0xd802, 0xd802, SN76496_1_w },
MEMORY_END


/****************************************************************************/

INPUT_PORTS_START( ikki )
	PORT_START  /* dsw1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x00, "2 Players Game" )
	PORT_DIPSETTING(    0x00, "2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Credit" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 1-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0xf0, 0x00, "Coin1 / Coin2" )
	PORT_DIPSETTING(    0x00, "1C 1C / 1C 1C" )
	PORT_DIPSETTING(    0x10, "2C 1C / 2C 1C" )
	PORT_DIPSETTING(    0x20, "2C 1C / 1C 3C" )
	PORT_DIPSETTING(    0x30, "1C 1C / 1C 2C" )
	PORT_DIPSETTING(    0x40, "1C 1C / 1C 3C" )
	PORT_DIPSETTING(    0x50, "1C 1C / 1C 4C" )
	PORT_DIPSETTING(    0x60, "1C 1C / 1C 5C" )
	PORT_DIPSETTING(    0x70, "1C 1C / 1C 6C" )
	PORT_DIPSETTING(    0x80, "1C 2C / 1C 2C" )
	PORT_DIPSETTING(    0x90, "1C 2C / 1C 4C" )
	PORT_DIPSETTING(    0xa0, "1C 2C / 1C 5C" )
	PORT_DIPSETTING(    0xb0, "1C 2C / 1C 10C" )
	PORT_DIPSETTING(    0xc0, "1C 2C / 1C 11C" )
	PORT_DIPSETTING(    0xd0, "1C 2C / 1C 12C" )
	PORT_DIPSETTING(    0xe0, "1C 2C / 1C 6C" )
	PORT_DIPSETTING(    0xf0, DEF_STR( Free_Play ) )

	PORT_START  /* dsw2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "1 (Normal)" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x06, "4 (Difficult)" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 2-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 2-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 2-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* e004 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )

	PORT_START /* e005 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START /* e003 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
INPUT_PORTS_END

/****************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	2048,   /* 2048 characters */
	3,      /* 3 bits per pixel */
	{0,16384*8,16384*8*2},
	{7,6,5,4,3,2,1,0},
	{8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7},
	8*8
};

static struct GfxLayout spritelayout =
{
	16,32,  /* 16*32 characters */
	256,    /* 256 characters */
	3,      /* 3 bits per pixel */
	{16384*8*2,16384*8,0},
	{7,6,5,4,3,2,1,0,
		8*16+7,8*16+6,8*16+5,8*16+4,8*16+3,8*16+2,8*16+1,8*16+0},
	{8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7,
	8*8,8*9,8*10,8*11,8*12,8*13,8*14,8*15,
	8*32,8*33,8*34,8*35,8*36,8*37,8*38,8*39,
	8*40,8*41,8*42,8*43,8*44,8*45,8*46,8*47},
	8*8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0000, &charlayout,   512, 64 },
	{ REGION_GFX1, 0x0000, &spritelayout, 0,   64 },
	{ -1 } /* end of array */
};


static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	{ 8000000/2, 8000000/2 },
	{ 75, 75 }
};


static struct MachineDriver machine_driver_ikki =
{
	{
		{
			CPU_Z80,
			8000000/2, /* 4.000MHz */
			ikki_readmem1,ikki_writemem1,0,0,
			interrupt,2
		},
		{
			CPU_Z80,
			8000000/2, /* 4.000MHz */
			ikki_readmem2,ikki_writemem2,0,0,
			interrupt,2
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	32*8, 32*8, { 1*8, 31*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	256,1024,
	ikki_vh_convert_color_prom,

	VIDEO_TYPE_RASTER ,
	0,
	generic_vh_start,
	generic_vh_stop,
	ikki_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	},
};

/****************************************************************************/

ROM_START( ikki )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "tvg17_1",  0x0000,  0x2000, 0xcb28167c )
	ROM_CONTINUE(         0x8000,  0x2000 )
	ROM_LOAD( "tvg17_2",  0x2000,  0x2000, 0x756c7450 )
	ROM_LOAD( "tvg17_3",  0x4000,  0x2000, 0x91f0a8b6 )
	ROM_LOAD( "tvg17_4",  0x6000,  0x2000, 0x696fcf7d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sub CPU */
	ROM_LOAD( "tvg17_5",  0x0000,  0x2000, 0x22bdb40e )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprite */
	ROM_LOAD( "tvg17_6",  0x0000,  0x4000, 0xdc8aa269 )
	ROM_LOAD( "tvg17_7",  0x4000,  0x4000, 0x0e9efeba )
	ROM_LOAD( "tvg17_8",  0x8000,  0x4000, 0x45c9087a )

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* bg */
	ROM_LOAD( "tvg17_9",  0x8000,  0x4000, 0xc594f3c5 )
	ROM_LOAD( "tvg17_10", 0x4000,  0x4000, 0x2e510b4e )
	ROM_LOAD( "tvg17_11", 0x0000,  0x4000, 0x35012775 )

	ROM_REGION( 0x0700, REGION_PROMS, 0 ) /* color PROMs */
	ROM_LOAD( "prom17_3", 0x0000,  0x0100, 0xdbcd3bec ) /* R */
	ROM_LOAD( "prom17_4", 0x0100,  0x0100, 0x9eb7b6cf ) /* G */
	ROM_LOAD( "prom17_5", 0x0200,  0x0100, 0x9b30a7f3 ) /* B */
	ROM_LOAD( "prom17_6", 0x0300,  0x0200, 0x962e619d ) /* sprite */
	ROM_LOAD( "prom17_7", 0x0500,  0x0200, 0xb1f5148c ) /* bg */

	ROM_REGION( 0x0200, REGION_USER1, 0 )
	ROM_LOAD( "prom17_1", 0x0000,  0x0100, 0xca0af30c ) /* video attribute */
	ROM_LOAD( "prom17_2", 0x0100,  0x0100, 0xf3c55174 ) /* unknown */
ROM_END

GAME( 1985, ikki, 0, ikki, ikki, 0, ROT0, "Sun Electronics", "Ikki (Japan)" )
