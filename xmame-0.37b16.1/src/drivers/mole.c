/*	MOLE ATTACK    YACHIYO  1982 */
/*	known clones: "Holy Moly" */
/* */
/*	emulated by Jason Nelson, Phil Stroffolino */
/*	known issues: */
/*		some dips not mapped */
/*		protection isn't fully understood, but game seems to be */
/*		ok. */
/* */
/*	buttons are laid out as follows: */
/*	7	8	9 */
/*	4	5	6 */
/*	1	2	3 */
/* */
/* Working RAM notes: */
/* 0x2e0					number of credits */
/* 0x2F1					coin up trigger */
/* 0x2F2					round counter */
/* 0x2F3					flag value */
/* 0x2FD					hammer aim for attract mode */
/* 0x2E1-E2					high score */
/* 0x2ED-EE					score */
/* 0x301-309				presence and height of mole in each hole, from bottom left */
/* 0x30a */
/* 0x32E-336				if a hammer is above a mole. (Not the same as collision) */
/* 0x337					dip switch related */
/* 0x338					dip switch related */
/* 0x340				    hammer control: manual=0; auto=1 */
/* 0x34C					round point 10s. */
/* 0x34D				    which bonus round pattern to use for moles. */
/* 0x349					button pressed (0..8 / 0xff) */
/* 0x350					number of players */
/* 0x351					irq-related */
/* 0x361 */
/* 0x362 */
/* 0x363 */
/* 0x364 */
/* 0x366					mirrors tile bank */
/* 0x36B					controls which player is playing. (1 == player 2); */
/* 0x3DC					affects mole popup */
/* 0x3E5					round point/passing point control? */
/* 0x3E7					round point/passing point control? */

#include "driver.h"
#include "vidhrdw/generic.h"

extern void moleattack_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern int moleattack_vh_start( void );
extern void moleattack_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void moleattack_vh_stop( void );

WRITE_HANDLER( moleattack_videoram_w );
WRITE_HANDLER( moleattack_tilesetselector_w );

static struct GfxLayout tile_layout =
{
	8,8,	/* character size */
	512,	/* number of characters */
	3,		/* number of bitplanes */
	{ 0x0000*8, 0x1000*8, 0x2000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfx_decode[] = {
	{ 1, 0x0000, &tile_layout, 0x00, 1 },
	{ 1, 0x3000, &tile_layout, 0x00, 1 },
	{ -1 }
};

static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	2000000, /* 2 MHz? */
	{ 100 }, /* volume */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

READ_HANDLER( mole_prot_r ){
	/*	Following are all known examples of Mole Attack
	**	code reading from the protection circuitry:
	**
	**	5b0b:
	**	ram[0x0361] = (ram[0x885+ram[0x8a5])&ram[0x886]
	**	ram[0x0363] = ram[0x886]
	**
	**	53c9:
	**	ram[0xe0] = ram[0x800]+ram[0x802]+ram[0x804]
	**	ram[0xea] = ram[0x828]
	**
	**	ram[0xe2] = (ram[0x806]&ram[0x826])|ram[0x820]
	**	ram[0xe3] = ram[0x826]
	**
	**	ram[0x361] = (ram[0x8cd]&ram[0x8ad])|ram[0x8ce]
	**	ram[0x362] = ram[0x8ae] = 0x32
	**
	**	ram[0x363] = ram[0x809]+ram[0x829]+ram[0x828]
	**	ram[0x364] = ram[0x808]
	*/

	switch( offset ){
	case 0x08: return 0xb0; /* random mole placement */
	case 0x26:
		if( cpu_get_pc() == 0x53d7 ){
			return 0x06; /* bonus round */
		}
		else { /* pc == 0x515b, 0x5162 */
			return 0xc6; /* game start */
		}
	case 0x86: return 0x91; /* game over */
	case 0xae: return 0x32; /* coinage */
	}

	/*	The above are critical protection reads.
	**	It isn't clear what effect (if any) the
	**	remaining reads have; for now we simply
	**	return 0x00
	*/
	return 0x00;
}

static MEMORY_READ_START( moleattack_readmem )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x0800, 0x08ff, mole_prot_r },
	{ 0x5000, 0x7fff, MRA_ROM },
	{ 0x8d00, 0x8d00, input_port_0_r },
	{ 0x8d40, 0x8d40, input_port_1_r },
	{ 0x8d80, 0x8d80, input_port_2_r },
	{ 0x8dc0, 0x8dc0, input_port_3_r },
	{ 0xd000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( moleattack_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x5000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, moleattack_videoram_w },
	{ 0x8400, 0x8400, moleattack_tilesetselector_w},
	{ 0x8c00, 0x8c00, AY8910_write_port_0_w },
	{ 0x8c01, 0x8c01, AY8910_control_port_0_w },
	{ 0x8d00, 0x8d00, MWA_NOP }, /* watchdog? */
	{ 0xd000, 0xffff, MWA_ROM },
MEMORY_END

struct MachineDriver machine_driver_mole =
{
	{
		{
			CPU_M6502,
			4000000, /* ? */
			moleattack_readmem,moleattack_writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1, /* single CPU */
	0,
	/* video hardware */
	40*8, 25*8, { 0*8, 40*8-1, 0*8, 25*8-1 },
	gfx_decode,
	8,8,moleattack_vh_convert_color_prom,
	VIDEO_TYPE_RASTER,
	0,
	moleattack_vh_start,
	moleattack_vh_stop,
	moleattack_vh_screenrefresh,
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

ROM_START( mole ) /* ALL ROMS ARE 2732 */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "m3a.5h",	0x5000, 0x1000, 0x5fbbdfef )
	ROM_RELOAD(			0xd000, 0x1000)
	ROM_LOAD( "m2a.7h",	0x6000, 0x1000, 0xf2a90642 )
	ROM_RELOAD(			0xe000, 0x1000 )
	ROM_LOAD( "m1a.8h",	0x7000, 0x1000, 0xcff0119a )
	ROM_RELOAD(			0xf000, 0x1000 )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mea.4a",	0x0000, 0x1000, 0x49d89116 )
	ROM_LOAD( "mca.6a",	0x1000, 0x1000, 0x04e90300 )
	ROM_LOAD( "maa.9a",	0x2000, 0x1000, 0x6ce9442b )
	ROM_LOAD( "mfa.3a",	0x3000, 0x1000, 0x0d0c7d13 )
	ROM_LOAD( "mda.5a",	0x4000, 0x1000, 0x41ae1842 )
	ROM_LOAD( "mba.8a",	0x5000, 0x1000, 0x50c43fc9 )
ROM_END

INPUT_PORTS_START( mole )
	PORT_START /* 0x8d00 */
	PORT_DIPNAME( 0x01, 0x00, "Round Points" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, "A" )
	PORT_DIPSETTING(	0x04, "B" )
	PORT_DIPSETTING(	0x08, "C" )
	PORT_DIPSETTING(	0x0c, "D" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, "A" )
	PORT_DIPSETTING(	0x10, "B" )
	PORT_DIPSETTING(	0x20, "C" )
	PORT_DIPSETTING(	0x30, "D" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) ) /* unused */
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) ) /* unused */
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START /* 0x8d40 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON5 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON8 )

	PORT_START /* 0x8d80 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON9 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Cocktail ) )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START /* 0x8dc0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON8 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON9 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


GAME( 1982, mole, 0, mole, mole, 0, ROT0, "Yachiyo Electronics, Ltd.", "Mole Attack" )
