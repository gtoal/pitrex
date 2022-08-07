/* "Gladiator"
 * (C) 1984 SNK
 */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

/* known issues:
	sound/music doesn't sound good (but it might be correct)
	cocktail support is missing
*/

extern void aso_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern int sgladiat_vh_start( void );
extern void sgladiat_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh );
extern void snk_vh_stop( void );

#define SNK_NMI_ENABLE	1
#define SNK_NMI_PENDING	2

static int cpuA_latch, cpuB_latch;
static unsigned char *shared_ram, *shared_ram2;
static int bSoundCPUBusy;

static struct AY8910interface ay8910_interface = {
	2,	/* number of chips */
	2000000, /* 2 MHz? */
	{ 50,50 }, /* volume */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct GfxLayout tile_layout =
{
	8,8,
	256,
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	256
};

static struct GfxLayout sprite_layout =
{
	16,16,
	256,
	3,
	{ 2*0x2000*8,1*0x2000*8,0*0x2000*8},
	{ 7,6,5,4,3,2,1,0, 15,14,13,12,11,10,9,8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	256
};

static struct GfxDecodeInfo tnk3_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0, &tile_layout,	128*3,  8 },
	{ REGION_GFX2, 0x0, &tile_layout,	128*1, 16 },
	{ REGION_GFX3, 0x0, &sprite_layout,	128*0, 16 },
	{ -1 }
};

/************************************************************************/

static READ_HANDLER( shared_ram_r )
{
	return shared_ram[offset];
}
static WRITE_HANDLER( shared_ram_w )
{
	shared_ram[offset] = data;
}

static READ_HANDLER( shared_ram2_r )
{
	return shared_ram2[offset];
}
static WRITE_HANDLER( shared_ram2_w )
{
	shared_ram2[offset] = data;
}

/************************************************************************/

static WRITE_HANDLER( snk_soundlatch_w )
{
	bSoundCPUBusy = 1;
	soundlatch_w( offset, data );

	/* trigger NMI on sound CPU */
	cpu_cause_interrupt( 2, Z80_NMI_INT );
}

static READ_HANDLER( snk_sound_ack_r )
{
	bSoundCPUBusy = 0;
	return 0xff;
}

/************************************************************************/

static READ_HANDLER( sgladiat_cpuA_nmi_r )
{
	/* trigger NMI on CPUB */
	if( cpuB_latch & SNK_NMI_ENABLE )
	{
		cpu_cause_interrupt( 1, Z80_NMI_INT );
		cpuB_latch = 0;
	}
	else
	{
		cpuB_latch |= SNK_NMI_PENDING;
	}
	return 0xff;
}

static WRITE_HANDLER( sgladiat_cpuA_nmi_w )
{
	/* enable NMI on CPUA */
	if( cpuA_latch&SNK_NMI_PENDING )
	{
		cpu_cause_interrupt( 0, Z80_NMI_INT );
		cpuA_latch = 0;
	}
	else
	{
		cpuA_latch |= SNK_NMI_ENABLE;
	}
}

static READ_HANDLER( sgladiat_cpuB_nmi_r )
{
	/* trigger NMI on CPUA */
	if( cpuA_latch & SNK_NMI_ENABLE )
	{
		cpu_cause_interrupt( 0, Z80_NMI_INT );
		cpuA_latch = 0;
	}
	else
	{
		cpuA_latch |= SNK_NMI_PENDING;
	}
	return 0xff;
}

static WRITE_HANDLER( sgladiat_cpuB_nmi_w )
{
	/* enable NMI on CPUB */
	if( cpuB_latch&SNK_NMI_PENDING )
	{
		cpu_cause_interrupt( 1, Z80_NMI_INT );
		cpuB_latch = 0;
	}
	else
	{
		cpuB_latch |= SNK_NMI_ENABLE;
	}
}

static READ_HANDLER( sgladiat_inp0_r )
{
	int result = readinputport( 0 );
	if( bSoundCPUBusy ) result |= 0x20; /* sound CPU busy bit */
	return result;
}

static WRITE_HANDLER( sglatiat_flipscreen_w )
{
	/* 0xa006 */
	/* x-------	screen is flipped */
}

static MEMORY_READ_START( sgladiat_readmem_cpuA )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xa000, sgladiat_inp0_r },
	{ 0xa100, 0xa100, input_port_1_r }, /* joy1 */
	{ 0xa200, 0xa200, input_port_2_r }, /* joy2 */
	{ 0xa400, 0xa400, input_port_3_r }, /* dsw1 */
	{ 0xa500, 0xa500, input_port_4_r }, /* dsw2 */
	{ 0xa700, 0xa700, sgladiat_cpuA_nmi_r },
	{ 0xd800, 0xdfff, MRA_RAM }, /* spriteram */
	{ 0xe000, 0xe7ff, MRA_RAM }, /* videoram */
	{ 0xe800, 0xefff, MRA_RAM }, /* work ram */
	{ 0xf000, 0xf7ff, MRA_RAM }, /* shareram */
MEMORY_END

static MEMORY_WRITE_START( sgladiat_writemem_cpuA )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa300, 0xa300, snk_soundlatch_w },
	{ 0xa600, 0xa600, sglatiat_flipscreen_w },
	{ 0xa700, 0xa700, sgladiat_cpuA_nmi_w },
	{ 0xd000, 0xd7ff, MWA_RAM, &shared_ram2 },
/*		{ 0xd200, 0xd200, MWA_RAM },    ?0x24    */
/*		{ 0xd300, 0xd300, MWA_RAM },    ------xx: msb scrollx    */
/*		{ 0xd400, 0xd400, MWA_RAM },    xscroll (sprite)    */
/*		{ 0xd500, 0xd500, MWA_RAM },    yscroll (sprite)    */
/*		{ 0xd600, 0xd600, MWA_RAM },    xscroll (bg)    */
/*		{ 0xd700, 0xd700, MWA_RAM },    yscroll (bg)    */
	{ 0xd800, 0xdfff, MWA_RAM, &spriteram },
	{ 0xe000, 0xe7ff, MWA_RAM, &videoram },
	{ 0xe800, 0xefff, MWA_RAM },
	{ 0xf000, 0xf7ff, MWA_RAM, &shared_ram },
MEMORY_END

static MEMORY_READ_START( sgladiat_readmem_cpuB )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xa000, 0xa000, sgladiat_cpuB_nmi_r },
	{ 0xc000, 0xcfff, spriteram_r }, /* 0xc800..0xffff is videoram */
	{ 0xd800, 0xdfff, shared_ram2_r },
	{ 0xe000, 0xe7ff, shared_ram_r },
MEMORY_END

static MEMORY_WRITE_START( sgladiat_writemem_cpuB )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa000, sgladiat_cpuB_nmi_w },
	{ 0xa600, 0xa600, sglatiat_flipscreen_w },
	{ 0xc000, 0xcfff, spriteram_w }, /* 0xc800..0xffff is videoram */
	{ 0xd800, 0xdfff, shared_ram2_w },
	{ 0xe000, 0xe7ff, shared_ram_w },
MEMORY_END

static MEMORY_READ_START( sgladiat_readmem_sound )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xc000, 0xc000, snk_sound_ack_r },
MEMORY_END

static MEMORY_WRITE_START( sgladiat_writemem_sound )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xe000, 0xe000, AY8910_control_port_0_w },
	{ 0xe001, 0xe001, AY8910_write_port_0_w },
	{ 0xe004, 0xe004, AY8910_control_port_1_w },
	{ 0xe005, 0xe005, AY8910_write_port_1_w },
MEMORY_END

static PORT_READ_START( sgladiat_readport )
	{ 0x00, 0x00, MRA_NOP },
PORT_END

static struct MachineDriver machine_driver_sgladiat =
{
	{
		{
			CPU_Z80,
			4000000,
			sgladiat_readmem_cpuA,sgladiat_writemem_cpuA,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz (?) */
			sgladiat_readmem_cpuB,sgladiat_writemem_cpuB,0,0,
			interrupt,1
		},
		{
			CPU_Z80,/* | CPU_AUDIO_CPU, */
			4000000,	/* 4 MHz (?) */
			sgladiat_readmem_sound,sgladiat_writemem_sound,sgladiat_readport,0,
			interrupt,2
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	36*8, 28*8, { 0*8+16, 36*8-1-16, 1*8, 28*8-1 },

	tnk3_gfxdecodeinfo,
	1024,1024,
	aso_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	sgladiat_vh_start,
	snk_vh_stop,
	sgladiat_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

ROM_START( sgladiat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for cpuA code */
	ROM_LOAD( "glad.005",	0x0000, 0x4000, 0x4bc60f0b )
	ROM_LOAD( "glad.004",	0x4000, 0x4000, 0xdb557f46 )
	ROM_LOAD( "glad.003",	0x8000, 0x2000, 0x55ce82b4 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for cpuB code */
	ROM_LOAD( "glad.002",	0x0000, 0x4000, 0x8350261c )
	ROM_LOAD( "glad.001",	0x4000, 0x4000, 0x5ee9d3fb )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for sound code */
	ROM_LOAD( "glad.007",  0x0000, 0x2000, 0xc25b6731 )
	ROM_LOAD( "glad.006",  0x2000, 0x2000, 0x2024d716 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "glad.011",	0x0000, 0x2000, 0x305bb294 ) /* foreground tiles */

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE ) /* background tiles */
	ROM_LOAD( "glad.012",	0x0000, 0x2000, 0xb7dd519f )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE ) /* 16x16 sprites */
	ROM_LOAD( "glad.008", 0x0000, 0x2000, 0xbcf42587 )
	ROM_LOAD( "glad.009", 0x2000, 0x2000, 0x912a20e0 )
	ROM_LOAD( "glad.010", 0x4000, 0x2000, 0x8b1db3a5 )

	ROM_REGION( 0xc00, REGION_PROMS, 0 )
	ROM_LOAD( "82s137.001",  0x000, 0x400, 0xd9184823 )
	ROM_LOAD( "82s137.002",  0x400, 0x400, 0x1a6b0953 )
	ROM_LOAD( "82s137.003",  0x800, 0x400, 0xc0e70308 )
ROM_END

INPUT_PORTS_START( sgladiat )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* sound CPU status */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 - copied from TNK3! */
	PORT_BITX( 0x01,    0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Walk everywhere", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xc0, "20k 60k" )
	PORT_DIPSETTING(    0x80, "40k 90k" )
	PORT_DIPSETTING(    0x40, "50k 120k" )
	PORT_DIPSETTING(    0x00, "None" )

	PORT_START	/* DSW2 - copied from TNK3!  */
	PORT_DIPNAME( 0x01, 0x01, "Bonus Occurrence" )
	PORT_DIPSETTING(    0x01, "1st & every 2nd" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, "Easy?" )
	PORT_DIPSETTING(    0x04, "Normal?" )
	PORT_DIPSETTING(    0x02, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
	PORT_DIPNAME( 0x18, 0x10, "Game Mode" )
	PORT_DIPSETTING(    0x18, "Demo Sounds Off" )
	PORT_DIPSETTING(    0x10, "Demo Sounds On" )
	PORT_DIPSETTING(    0x00, "Freeze" )
	PORT_BITX( 0,       0x08, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
INPUT_PORTS_END

GAMEX( 1984, sgladiat, 0, sgladiat, sgladiat, 0, 0,   "SNK", "Gladiator 1984", GAME_NO_COCKTAIL )
