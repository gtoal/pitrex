/***************************************************************************

	D-Con							(c) 1992 Success

	Success seems related to Seibu - this game runs on Seibu hardware.

	Emulation by Bryan McPhail, mish@tendril.co.uk

	Coin inputs are handled by the sound CPU, so they don't work with sound
	disabled. Use the service switch instead.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"

WRITE16_HANDLER( dcon_background_w );
WRITE16_HANDLER( dcon_foreground_w );
WRITE16_HANDLER( dcon_midground_w );
WRITE16_HANDLER( dcon_text_w );
WRITE16_HANDLER( dcon_control_w );

int dcon_vh_start(void);
void dcon_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern data16_t *dcon_back_data,*dcon_fore_data,*dcon_mid_data,*dcon_scroll_ram,*dcon_textram;

/***************************************************************************/

static MEMORY_READ16_START( readmem )
	{ 0x00000, 0x7ffff, MRA16_ROM },
	{ 0x80000, 0x8bfff, MRA16_RAM },
	{ 0x8c000, 0x8c7ff, MRA16_RAM },
	{ 0x8c800, 0x8cfff, MRA16_RAM },
	{ 0x8d000, 0x8d7ff, MRA16_RAM },
	{ 0x8e800, 0x8f7ff, MRA16_RAM },
	{ 0x8f800, 0x8ffff, MRA16_RAM },
	{ 0xa0000, 0xa000d, seibu_main_word_r },
	{ 0xe0000, 0xe0001, input_port_1_word_r },
	{ 0xe0002, 0xe0003, input_port_2_word_r },
	{ 0xe0004, 0xe0005, input_port_3_word_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x00000, 0x7ffff, MWA16_ROM },
	{ 0x80000, 0x8bfff, MWA16_RAM },
	{ 0x8c000, 0x8c7ff, dcon_background_w, &dcon_back_data },
	{ 0x8c800, 0x8cfff, dcon_foreground_w, &dcon_fore_data },
	{ 0x8d000, 0x8d7ff, dcon_midground_w, &dcon_mid_data },
	{ 0x8d800, 0x8e7ff, dcon_text_w, &dcon_textram },
	{ 0x8e800, 0x8f7ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x8f800, 0x8ffff, MWA16_RAM, &spriteram16 },
	{ 0xa0000, 0xa000d, seibu_main_word_w },
	{ 0xc001c, 0xc001d, dcon_control_w },
	{ 0xc0020, 0xc002b, MWA16_RAM, &dcon_scroll_ram },
	{ 0xc0000, 0xc00ff, MWA16_NOP },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( dcon )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout dcon_charlayout =
{
	8,8,		/* 8*8 characters */
	4096,
	4,			/* 4 bits per pixel */
	{ 0,4,(0x10000*8)+0,0x10000*8+4,  },
	{ 3,2,1,0, 11,10,9,8 ,8,9,10,11,0,1,2,3, },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128
};

static struct GfxLayout dcon_tilelayout =
{
	16,16,	/* 16*16 tiles */
	4096,		/* 2048*4 tiles */
	4,		/* 4 bits per pixel */
	{ 8,12, 0,4 },
	{
		3,2,1,0,19,18,17,16,
		512+3,512+2,512+1,512+0,
		512+11+8,512+10+8,512+9+8,512+8+8,
	},
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
	},
	1024
};

static struct GfxLayout dcon_spritelayout =
{
	16,16,	/* 16*16 tiles */
	4096*4,		/* 2048*4 tiles */
	4,		/* 4 bits per pixel */
	{  8,12, 0,4 },
	{
		3,2,1,0,19,18,17,16,
		512+3,512+2,512+1,512+0,
		512+11+8,512+10+8,512+9+8,512+8+8,
	},
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
	},
	1024
};

static struct GfxDecodeInfo dcon_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &dcon_charlayout,    1024+768, 16 },
	{ REGION_GFX2, 0, &dcon_tilelayout,    1024+0,   16 },
	{ REGION_GFX3, 0, &dcon_tilelayout,    1024+512, 16 },
	{ REGION_GFX4, 0, &dcon_tilelayout,    1024+256, 16 },
	{ REGION_GFX5, 0, &dcon_spritelayout,         0, 64 },
	{ -1 } /* end of array */
};

/******************************************************************************/

/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(4000000,8000,REGION_SOUND1);

static struct MachineDriver machine_driver_dcon =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			readmem,writemem,0,0,
			m68_level4_irq,1
		},
		{
			SEIBU_SOUND_SYSTEM_CPU(4000000)
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* CPU interleave  */
	seibu_sound_init_1,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 0*8, 28*8-1 },

	dcon_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dcon_vh_start,
	0,
	dcon_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
	}
};

/***************************************************************************/

ROM_START( dcon )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE("p0-0",   0x000000, 0x20000, 0xa767ec15 )
	ROM_LOAD16_BYTE("p0-1",   0x000001, 0x20000, 0xa7efa091 )
	ROM_LOAD16_BYTE("p1-0",   0x040000, 0x20000, 0x3ec1ef7d )
	ROM_LOAD16_BYTE("p1-1",   0x040001, 0x20000, 0x4b8de320 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	 /* 64k code for sound Z80 */
	ROM_LOAD( "fm",           0x000000, 0x08000, 0x50450faa )
	ROM_CONTINUE(             0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fix0",  0x000000, 0x10000, 0xab30061f ) /* chars */
	ROM_LOAD( "fix1",  0x010000, 0x10000, 0xa0582115 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bg1",   0x000000, 0x80000, 0xeac43283 ) /* tiles */

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "bg3",   0x000000, 0x80000, 0x1408a1e0 ) /* tiles */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "bg2",   0x000000, 0x80000, 0x01864eb6 ) /* tiles */

	ROM_REGION( 0x200000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD( "obj0",  0x000000, 0x80000, 0xc3af37db ) /* sprites */
	ROM_LOAD( "obj1",  0x080000, 0x80000, 0xbe1f53ba )
	ROM_LOAD( "obj2",  0x100000, 0x80000, 0x24e0b51c )
	ROM_LOAD( "obj3",  0x180000, 0x80000, 0x5274f02d )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	 /* ADPCM samples */
	ROM_LOAD( "pcm", 0x000000, 0x20000, 0xd2133b85 )
ROM_END

/***************************************************************************/

GAMEX( 1992, dcon, 0, dcon, dcon, 0, ROT0, "Success (Seibu hardware)", "D-Con", GAME_NO_COCKTAIL )
