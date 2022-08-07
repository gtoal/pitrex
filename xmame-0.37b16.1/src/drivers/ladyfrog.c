/***************************************************************************

Lady Frog, or is it Dragon Punch, or is Lady Frog the name of a bootleg
Dragon Punch?

The program jumps straight away to an unmapped memory address. I don't know,
maybe there's a ROM missing.

ladyfrog.001 contains
VIDEO COMPUTER SYSTEM  (C)1989 DYNAX INC  NAGOYA JAPAN  DRAGON PUNCH  VER. 1.30

***************************************************************************/

#include "driver.h"


void ladyfrog_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	palette_recalc();
}



static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },
	{ 0xffc000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },
	{ 0xffc000, 0xffffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x7000, 0x7fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x7000, 0x7fff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport )
PORT_END

static PORT_WRITE_START( sound_writeport )
/*	{ 0x12, 0x12, },	ym2151? */
/*	{ 0x13, 0x13, }, */
PORT_END



INPUT_PORTS_START( ladyfrog )
	PORT_START
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,  256, 16 },
	{ -1 } /* end of array */
};



static struct MachineDriver machine_driver_ladyfrog =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 MHz??? */
			readmem,writemem,0,0,
			m68_level6_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz??? */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupt mode 0 + NMI */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */				\
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0, 256-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	0,
	0,
	ladyfrog_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};



ROM_START( ladyfrog )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "ladyfrog.002",	0x000000, 0x080000, 0x724cf022 )
	ROM_LOAD16_BYTE( "ladyfrog.006",	0x000001, 0x080000, 0xe52a7ae2 )
	ROM_LOAD16_BYTE( "ladyfrog.003",	0x100000, 0x080000, 0xa1d49967 )
	ROM_LOAD16_BYTE( "ladyfrog.007",	0x100001, 0x080000, 0xe5805c4e )
	ROM_LOAD16_BYTE( "ladyfrog.004",	0x200000, 0x080000, 0x709281f5 )
	ROM_LOAD16_BYTE( "ladyfrog.008",	0x200001, 0x080000, 0x39adcba4 )
	ROM_LOAD16_BYTE( "ladyfrog.005",	0x300000, 0x080000, 0xb683160c )
	ROM_LOAD16_BYTE( "ladyfrog.009",	0x300001, 0x080000, 0xe475fb76 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "ladyfrog.001",        0x0000, 0x20000, 0xba9eb1c6 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ladyfrog.010",       0x00000, 0x20000, 0x51fd0e1a )
	ROM_LOAD( "ladyfrog.011",       0x20000, 0x20000, 0x610bf6f3 )
	ROM_LOAD( "ladyfrog.012",       0x40000, 0x20000, 0x466ede67 )
	ROM_LOAD( "ladyfrog.013",       0x60000, 0x20000, 0xfad3e8be )
ROM_END



GAME( 19??, ladyfrog, 0, ladyfrog, ladyfrog, 0, ROT0, "Dynax? Comad?", "Dragon Punch? Lady Frog?" )
