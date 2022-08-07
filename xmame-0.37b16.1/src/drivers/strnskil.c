/*****************************************************************************

Strength & Skill (c) 1984 Sun Electronics

	Driver by Uki

	19/Jun/2001 -

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void strnskil_vh_convert_color_prom(unsigned char *palette,
	unsigned short *colortable,const unsigned char *color_prom);
void strnskil_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static UINT8 *strnskil_sharedram;

/****************************************************************************/

WRITE_HANDLER( strnskil_scroll_x_w );
WRITE_HANDLER( strnskil_scrl_ctrl_w );

WRITE_HANDLER( strnskil_sharedram_w )
{
	strnskil_sharedram[offset] = data;
}

READ_HANDLER( strnskil_sharedram_r )
{
	return strnskil_sharedram[offset];
}

READ_HANDLER( strnskil_d800_r )
{
/* bit0: interrupt type?, bit1: CPU2 busack? */

	if (cpu_getiloops() == 0)
		return 0;
	return 1;
}

/****************************************************************************/

static READ_HANDLER( protection_r )
{
	int res;

	switch (cpu_get_pc())
	{
		case 0x6066:	res = 0xa5;	break;
		case 0x60dc:	res = 0x20;	break;	/* bits 0-3 unknown */
		case 0x615d:	res = 0x30;	break;	/* bits 0-3 unknown */
		case 0x61b9:	res = 0x60|(rand()&0x0f);	break;	/* bits 0-3 unknown */
		case 0x6219:	res = 0x77;	break;
		case 0x626c:	res = 0xb4;	break;
		default:		res = 0xff; break;
	}

	logerror("%04x: protection_r -> %02x\n",cpu_get_pc(),res);
	return res;
}

static WRITE_HANDLER( protection_w )
{
	logerror("%04x: protection_w %02x\n",cpu_get_pc(),data);
}

/****************************************************************************/

static MEMORY_READ_START( strnskil_readmem1 )
	{ 0x0000, 0x9fff, MRA_ROM },

	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, strnskil_sharedram_r },
	{ 0xd000, 0xd7ff, MRA_RAM }, /* videoram */

	{ 0xd800, 0xd800, strnskil_d800_r },
	{ 0xd801, 0xd801, input_port_0_r }, /* dsw 1 */
	{ 0xd802, 0xd802, input_port_1_r }, /* dsw 2 */
	{ 0xd803, 0xd803, input_port_4_r }, /* other inputs */
	{ 0xd804, 0xd804, input_port_2_r }, /* player1 */
	{ 0xd805, 0xd805, input_port_3_r }, /* player2 */

	{ 0xd806, 0xd806, protection_r }, /* protection data read (pettanp) */
MEMORY_END

static MEMORY_WRITE_START( strnskil_writemem1 )
	{ 0x0000, 0x9fff, MWA_ROM },

	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, strnskil_sharedram_w },
	{ 0xd000, 0xd7ff, videoram_w, &videoram, &videoram_size },

	{ 0xd808, 0xd808, strnskil_scrl_ctrl_w },
	{ 0xd809, 0xd809, MWA_NOP }, /* coin counter? */
	{ 0xd80a, 0xd80b, strnskil_scroll_x_w },

/*	{ 0xd80c, 0xd80c, MWA_NOP },		   protection reset?    */
	{ 0xd80d, 0xd80d, protection_w },	/* protection data write (pettanp) */
MEMORY_END

static MEMORY_READ_START( strnskil_readmem2 )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xc000, 0xc7ff, spriteram_r },
	{ 0xc800, 0xcfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( strnskil_writemem2 )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc800, 0xcfff, MWA_RAM, &strnskil_sharedram },

	{ 0xd801, 0xd801, SN76496_0_w },
	{ 0xd802, 0xd802, SN76496_1_w },
MEMORY_END


/****************************************************************************/

INPUT_PORTS_START( strnskil )
	PORT_START  /* dsw1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Unknown 1-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
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
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown 2-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Unknown 2-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
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

	PORT_START /* d804 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )

	PORT_START /* d805 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START /* d803 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
INPUT_PORTS_END

INPUT_PORTS_START( pettanp )
	PORT_START  /* dsw1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
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
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000 50000" )
	PORT_DIPSETTING(    0x02, "20000 80000" )
	PORT_DIPSETTING(    0x04, "20000" )
	PORT_DIPSETTING(    0x06, "None" )
	PORT_DIPNAME( 0x08, 0x00, "Second Practice" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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

	PORT_START /* d804 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )

	PORT_START /* d805 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )

	PORT_START /* d803 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH )
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
	1024,   /* 1024 characters */
	3,      /* 3 bits per pixel */
	{0,8192*8,8192*8*2},
	{7,6,5,4,3,2,1,0},
	{8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7},
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 characters */
	256,    /* 256 characters */
	3,      /* 3 bits per pixel */
	{8192*8*2,8192*8,0},
	{7,6,5,4,3,2,1,0,
		8*16+7,8*16+6,8*16+5,8*16+4,8*16+3,8*16+2,8*16+1,8*16+0},
	{8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7,
		8*8,8*9,8*10,8*11,8*12,8*13,8*14,8*15},
	8*8*4
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


static struct MachineDriver machine_driver_strnskil =
{
	{
		{
			CPU_Z80,
			8000000/2, /* 4.000MHz */
			strnskil_readmem1,strnskil_writemem1,0,0,
			interrupt,2
		},
		{
			CPU_Z80,
			8000000/2, /* 4.000MHz */
			strnskil_readmem2,strnskil_writemem2,0,0,
			interrupt,2
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,
	0,

	32*8, 32*8, { 1*8, 31*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	256,1024,
	strnskil_vh_convert_color_prom,

	VIDEO_TYPE_RASTER ,
	0,
	generic_vh_start,
	generic_vh_stop,
	strnskil_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	},
};

/****************************************************************************/

ROM_START( strnskil )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "tvg3.7",  0x0000,  0x2000, 0x31fd793a )
	ROM_CONTINUE(        0x8000,  0x2000 )
	ROM_LOAD( "tvg4.8",  0x2000,  0x2000, 0xc58315b5 )
	ROM_LOAD( "tvg5.9",  0x4000,  0x2000, 0x29e7ded5 )
	ROM_LOAD( "tvg6.10", 0x6000,  0x2000, 0x8b126a4b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sub CPU */
	ROM_LOAD( "tvg1.2",  0x0000,  0x2000, 0xb586b753 )
	ROM_LOAD( "tvg2.3",  0x2000,  0x2000, 0x8bd71bb6 )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprite */
	ROM_LOAD( "tvg7.90",   0x0000,  0x2000, 0xee3bd593 )
	ROM_LOAD( "tvg8.92",   0x2000,  0x2000, 0x1b265360 )
	ROM_LOAD( "tvg9.94",   0x4000,  0x2000, 0x776c7ca6 )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* bg */
	ROM_LOAD( "tvg12.102", 0x0000,  0x2000, 0x68b9d888 )
	ROM_LOAD( "tvg11.101", 0x2000,  0x2000, 0x7f2179ff )
	ROM_LOAD( "tvg10.100", 0x4000,  0x2000, 0x321ad963 )

	ROM_REGION( 0x0800, REGION_PROMS, 0 ) /* color PROMs */
	ROM_LOAD( "15-3.prm", 0x0000,  0x0100, 0xdbcd3bec ) /* R */
	ROM_LOAD( "15-4.prm", 0x0100,  0x0100, 0x9eb7b6cf ) /* G */
	ROM_LOAD( "15-5.prm", 0x0200,  0x0100, 0x9b30a7f3 ) /* B */
	ROM_LOAD( "15-1.prm", 0x0300,  0x0200, 0xd4f5b3d7 ) /* sprite */
	ROM_LOAD( "15-2.prm", 0x0500,  0x0200, 0xcdffede9 ) /* bg */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* scroll control PROM */
	ROM_LOAD( "15-6.prm", 0x0000,  0x0100, 0xec4faf5b )
ROM_END

ROM_START( guiness )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "tvg3.15", 0x0000,  0x2000, 0x3a605ad8 )
	ROM_CONTINUE(        0x8000,  0x2000 )
	ROM_LOAD( "tvg4.8",  0x2000,  0x2000, 0xc58315b5 )
	ROM_LOAD( "tvg5.9",  0x4000,  0x2000, 0x29e7ded5 )
	ROM_LOAD( "tvg6.10", 0x6000,  0x2000, 0x8b126a4b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sub CPU */
	ROM_LOAD( "tvg1.2",  0x0000,  0x2000, 0xb586b753 )
	ROM_LOAD( "tvg2.3",  0x2000,  0x2000, 0x8bd71bb6 )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprite */
	ROM_LOAD( "tvg7.90",   0x0000,  0x2000, 0xee3bd593 )
	ROM_LOAD( "tvg8.92",   0x2000,  0x2000, 0x1b265360 )
	ROM_LOAD( "tvg9.94",   0x4000,  0x2000, 0x776c7ca6 )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* bg */
	ROM_LOAD( "tvg12.15", 0x0000,  0x2000, 0xa82c923d )
	ROM_LOAD( "tvg11.15", 0x2000,  0x2000, 0xd432c96f )
	ROM_LOAD( "tvg10.15", 0x4000,  0x2000, 0xa53959d6 )

	ROM_REGION( 0x0800, REGION_PROMS, 0 ) /* color PROMs */
	ROM_LOAD( "15-3.prm", 0x0000,  0x0100, 0xdbcd3bec ) /* R */
	ROM_LOAD( "15-4.prm", 0x0100,  0x0100, 0x9eb7b6cf ) /* G */
	ROM_LOAD( "15-5.prm", 0x0200,  0x0100, 0x9b30a7f3 ) /* B */
	ROM_LOAD( "15-1.prm", 0x0300,  0x0200, 0xd4f5b3d7 ) /* sprite */
	ROM_LOAD( "15-2.prm", 0x0500,  0x0200, 0xcdffede9 ) /* bg */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* scroll control PROM */
	ROM_LOAD( "15-6.prm", 0x0000,  0x0100, 0xec4faf5b )
ROM_END

ROM_START( pettanp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "tvg2-16a.7",  0x0000,  0x2000, 0x4cbbbd01 )
	ROM_CONTINUE(            0x8000,  0x2000 )
	ROM_LOAD( "tvg3-16a.8",  0x2000,  0x2000, 0xaaa0420f )
	ROM_LOAD( "tvg4-16a.9",  0x4000,  0x2000, 0x43306369 )
	ROM_LOAD( "tvg5-16a.10", 0x6000,  0x2000, 0xda9c635f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sub CPU */
	ROM_LOAD( "tvg1-16.2",   0x0000,  0x2000, 0xe36009f6 )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprite */
	ROM_LOAD( "tvg6-16.90",  0x0000,  0x2000, 0x6905d9d5 )
	ROM_LOAD( "tvg7-16.92",  0x2000,  0x2000, 0x40d02bfd )
	ROM_LOAD( "tvg8-16.94",  0x4000,  0x2000, 0xb18a2244 )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* bg */
	ROM_LOAD( "tvg11-16.102",0x0000,  0x2000, 0x327b7a29 )
	ROM_LOAD( "tvg10-16.101",0x2000,  0x2000, 0x624ac061 )
	ROM_LOAD( "tvg9-16.100", 0x4000,  0x2000, 0xc477e74c )

	ROM_REGION( 0x0700, REGION_PROMS, 0 ) /* color PROMs */
	ROM_LOAD( "16-3.66",  0x0000,  0x0100, 0xdbcd3bec ) /* R */
	ROM_LOAD( "16-4.67",  0x0100,  0x0100, 0x9eb7b6cf ) /* G */
	ROM_LOAD( "16-5.68",  0x0200,  0x0100, 0x9b30a7f3 ) /* B */
	ROM_LOAD( "16-1.148", 0x0300,  0x0200, 0x777e2770 ) /* sprite */
	ROM_LOAD( "16-2.97",  0x0500,  0x0200, 0x7f95d4b2 ) /* bg */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* scroll control PROM */
/*	ROM_LOAD( "16-6",     0x0000,  0x0100, 0x00000000 ) */

	ROM_REGION( 0x1000, REGION_USER2, 0 ) /* protection? */
	ROM_LOAD( "tvg12-16.2", 0x0000,  0x1000, 0x3abc6ba8 )
ROM_END

GAME(  1984, strnskil, 0,        strnskil, strnskil, 0, ROT0, "Sun Electronics", "Strength & Skill" )
GAME(  1984, guiness,  strnskil, strnskil, strnskil, 0, ROT0, "Sun Electronics", "The Guiness (Japan)" )
GAMEX( 1984, pettanp,  0,        strnskil, pettanp,  0, ROT0, "Sun Electronics", "Pettan Pyuu (Japan)", GAME_UNEMULATED_PROTECTION )
