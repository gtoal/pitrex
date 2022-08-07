/***************************************************************************

							-= American Speedway =-

					driver by	Luca Elia (l.elia@tin.it)


CPU  :	Z80A x 2
Sound:	YM2151


(c)1987 Enerdyne Technologies, Inc. / PGD

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables & functions defined in vidhrdw: */

WRITE_HANDLER( amspdwy_videoram_w );
WRITE_HANDLER( amspdwy_colorram_w );
WRITE_HANDLER( amspdwy_paletteram_w );
WRITE_HANDLER( amspdwy_flipscreen_w );

int  amspdwy_vh_start(void);
void amspdwy_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/***************************************************************************


									Main CPU


***************************************************************************/

/*
	765-----	Buttons
	---4----	Sgn(Wheel Delta)
	----3210	Abs(Wheel Delta)

	Or last value when wheel delta = 0
*/
#define AMSPDWY_WHEEL_R( _n_ ) \
READ_HANDLER( amspdwy_wheel_##_n_##_r ) \
{ \
	static data8_t wheel_old, ret; \
	data8_t wheel = readinputport(5 + _n_); \
	if (wheel != wheel_old) \
	{ \
		wheel = (wheel & 0x7fff) - (wheel & 0x8000); \
		if (wheel > wheel_old)	ret = ((+wheel) & 0xf) | 0x00; \
		else					ret = ((-wheel) & 0xf) | 0x10; \
		wheel_old = wheel; \
	} \
	return ret | readinputport(2 + _n_); \
}
AMSPDWY_WHEEL_R( 0 )
AMSPDWY_WHEEL_R( 1 )


READ_HANDLER( amspdwy_sound_r )
{
	return (YM2151_status_port_0_r(0) & ~ 0x30) | readinputport(4);
}

WRITE_HANDLER( amspdwy_sound_w )
{
	soundlatch_w(0,data);
	cpu_set_nmi_line(1,PULSE_LINE);
}

static MEMORY_READ_START( amspdwy_readmem )
	{ 0x0000, 0x7fff, MRA_ROM				},	/* ROM */
/*	{ 0x8000, 0x801f, MRA_RAM				},	// Palette */
	{ 0x9000, 0x93ff, videoram_r			},	/* Layer */
	{ 0x9400, 0x97ff, videoram_r			},	/* Mirror? */
	{ 0x9800, 0x9bff, colorram_r			},	/* Layer */
	{ 0x9c00, 0x9fff, MRA_RAM				},	/* Unused? */
	{ 0xa000, 0xa000, input_port_0_r		},	/* DSW 1 */
	{ 0xa400, 0xa400, input_port_1_r		},	/* DSW 2 */
	{ 0xa800, 0xa800, amspdwy_wheel_0_r		},	/* Player 1 */
	{ 0xac00, 0xac00, amspdwy_wheel_1_r		},	/* Player 2 */
	{ 0xb400, 0xb400, amspdwy_sound_r		},	/* YM2151 Status + Buttons */
	{ 0xc000, 0xc0ff, MRA_RAM				},	/* Sprites */
	{ 0xe000, 0xe7ff, MRA_RAM				},	/* Work RAM */
MEMORY_END

static MEMORY_WRITE_START( amspdwy_writemem )
	{ 0x0000, 0x7fff, MWA_ROM							},	/* ROM */
	{ 0x8000, 0x801f, amspdwy_paletteram_w, &paletteram	},	/* Palette */
	{ 0x9000, 0x93ff, amspdwy_videoram_w, &videoram		},	/* Layer */
	{ 0x9400, 0x97ff, amspdwy_videoram_w				},	/* Mirror? */
	{ 0x9800, 0x9bff, amspdwy_colorram_w, &colorram		},	/* Layer */
	{ 0x9c00, 0x9fff, MWA_RAM							},	/* Unused? */
/*	{ 0xa000, 0xa000, MWA_NOP							},	// ? */
	{ 0xa400, 0xa400, amspdwy_flipscreen_w				},	/* Toggle Flip Screen? */
	{ 0xb000, 0xb000, MWA_NOP							},	/* ? Exiting IRQ */
	{ 0xb400, 0xb400, amspdwy_sound_w					},	/* To Sound CPU */
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size	},	/* Sprites */
	{ 0xe000, 0xe7ff, MWA_RAM							},	/* Work RAM */
MEMORY_END


READ_HANDLER( amspdwy_port_r )
{
	data8_t *Tracks = memory_region(REGION_CPU1)+0x10000;
	return Tracks[offset];
}

static PORT_READ_START( amspdwy_readport )
	{ 0x0000, 0x7fff, amspdwy_port_r	},
PORT_END



/***************************************************************************


								Sound CPU


***************************************************************************/

static MEMORY_READ_START( amspdwy_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM					},	/* ROM */
	{ 0x9000, 0x9000, soundlatch_r				},	/* From Main CPU */
	{ 0xc000, 0xdfff, MRA_RAM					},	/* Work RAM */
	{ 0xffff, 0xffff, MRA_NOP					},	/* ??? IY = FFFF at the start ? */
MEMORY_END

static MEMORY_WRITE_START( amspdwy_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM					},	/* ROM */
/*	{ 0x8000, 0x8000, MWA_NOP					},	// ? Written with 0 at the start */
	{ 0xa000, 0xa000, YM2151_register_port_0_w	},	/* YM2151 */
	{ 0xa001, 0xa001, YM2151_data_port_0_w		},	/* */
	{ 0xc000, 0xdfff, MWA_RAM					},	/* Work RAM */
MEMORY_END




/***************************************************************************


								Input Ports


***************************************************************************/

INPUT_PORTS_START( amspdwy )

	PORT_START	/* IN0 - DSW 1 */
	PORT_DIPNAME( 0x01, 0x00, "Character Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Show Arrows" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x10, 0x00, "Steering Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_BIT(     0xe0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 - DSW 2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) ) */
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x0c, "Hardest" )
	PORT_DIPNAME( 0x10, 0x00, "Time" )
	PORT_DIPSETTING(    0x10, "45 sec" )
	PORT_DIPSETTING(    0x00, "60 sec" )
	PORT_BIT(     0xe0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 - Player 1 Wheel + Coins */
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* wheel */
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_HIGH, IPT_COIN1, 2 )	/* 2-3f */

	PORT_START	/* IN3 - Player 2 Wheel + Coins */
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_HIGH, IPT_COIN2, 2 )

	PORT_START	/* IN4 - Player 1&2 Pedals + YM2151 Sound Status */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_SPECIAL )

	PORT_START	/* IN5 - Player 1 Analog Fake Port */
	PORT_ANALOGX( 0xffff, 0x0000, IPT_DIAL | IPF_PLAYER1, 15, 20, 0, 0, KEYCODE_LEFT, KEYCODE_RIGHT, 0, 0 )

	PORT_START	/* IN6 - Player 2 Analog Fake Port */
	PORT_ANALOGX( 0xffff, 0x0000, IPT_DIAL | IPF_PLAYER2, 15, 20, 0, 0, KEYCODE_D, KEYCODE_G, 0, 0 )

INPUT_PORTS_END




/***************************************************************************


								Graphics Layouts


***************************************************************************/

static struct GfxLayout layout_8x8x2 =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static struct GfxDecodeInfo amspdwy_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x2,   0, 8 }, /* [0] Layer & Sprites */
	{ -1 }
};



/***************************************************************************


								Machine Drivers


***************************************************************************/


static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface amspdwy_ym2151_interface =
{
	1,
	3000000,	/* ? */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ irq_handler }
};


static struct MachineDriver machine_driver_amspdwy =
{
	{
		{
			CPU_Z80 | CPU_16BIT_PORT,
			3000000,	/* ? */
			amspdwy_readmem,  amspdwy_writemem,
			amspdwy_readport, 0,
			interrupt, 1	/* IRQ: 60Hz, NMI: retn */
		},
		{
			CPU_Z80,	/* Can't be disabled: the YM2151 timers must work */
			3000000,	/* ? */
			amspdwy_sound_readmem, amspdwy_sound_writemem,
			0, 0,
			ignore_interrupt, 1		/* IRQ: YM2151, NMI: main CPU */
		}
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0+16, 256-16-1 },
	amspdwy_gfxdecodeinfo,
	32, 32,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	amspdwy_vh_start,
	0,
	amspdwy_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM2151,	&amspdwy_ym2151_interface	}
	}
};




/***************************************************************************


								ROMs Loading


***************************************************************************/



/***************************************************************************

							American Speedway

USES TWO Z80 CPU'S W/YM2151 SOUND
THE NUMBERS WITH THE NAMES ARE PROBABLY CHECKSUMS

NAME    LOCATION    TYPE
------------------------
AUDI9363 U2         27256   CONN BD
GAME5807 U33         "       "
TRKS6092 U34         "       "
HIHIE12A 4A         2732    REAR BD
HILO9B3C 5A          "       "
LOHI4644 2A          "       "
LOLO1D51 1A          "       "

						American Speedway (Set 2)

1987 Enerdyne Technologies, Inc. Has Rev 4 PGD written on the top board.

Processors
------------------
Dual Z80As
YM2151     (sound)

RAM
------------------
12 2114
5  82S16N

Eproms
==================

Name        Loc   TYpe   Checksum
----------  ----  -----  --------
Game.u22    U33   27256  A222
Tracks.u34  U34   27256  6092
Audio.U02   U2    27256  9363
LOLO1.1A    1A    2732   1D51
LOHI.2A     2A    2732   4644
HIHI.4A     3/4A  2732   E12A
HILO.5A     5A    2732   9B3C

***************************************************************************/

ROM_START( amspdwy )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "game5807.u33", 0x00000, 0x8000, 0x88233b59 )
	ROM_LOAD( "trks6092.u34", 0x10000, 0x8000, 0x74a4e7b7 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Sound Z80 Code */
	ROM_LOAD( "audi9463.u2", 0x00000, 0x8000, 0x61b0467e )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )	/* Layer + Sprites */
	ROM_LOAD( "hilo9b3c.5a", 0x0000, 0x1000, 0xf50f864c )
	ROM_LOAD( "hihie12a.4a", 0x1000, 0x1000, 0x3d7497f3 )
	ROM_LOAD( "lolo1d51.1a", 0x2000, 0x1000, 0x58701c1c )
	ROM_LOAD( "lohi4644.2a", 0x3000, 0x1000, 0xa1d802b1 )
ROM_END

ROM_START( amspdwya )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "game.u33",     0x00000, 0x8000, 0xfacab102 )
	ROM_LOAD( "trks6092.u34", 0x10000, 0x8000, 0x74a4e7b7 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Sound Z80 Code */
	ROM_LOAD( "audi9463.u2", 0x00000, 0x8000, 0x61b0467e )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )	/* Layer + Sprites */
	ROM_LOAD( "hilo9b3c.5a", 0x0000, 0x1000, 0xf50f864c )
	ROM_LOAD( "hihie12a.4a", 0x1000, 0x1000, 0x3d7497f3 )
	ROM_LOAD( "lolo1d51.1a", 0x2000, 0x1000, 0x58701c1c )
	ROM_LOAD( "lohi4644.2a", 0x3000, 0x1000, 0xa1d802b1 )
ROM_END


/* (C) 1987 ETI 8402 MAGNOLIA ST. #C SANTEE, CA 92071 */

GAME( 1987, amspdwy,  0,       amspdwy, amspdwy, 0, ROT0, "Enerdyne Technologies, Inc.", "American Speedway (set 1)" )
GAME( 1987, amspdwya, amspdwy, amspdwy, amspdwy, 0, ROT0, "Enerdyne Technologies, Inc.", "American Speedway (set 2)" )
